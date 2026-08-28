#include <Arduino.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <math.h>
#include <SPI.h>

/* ===================== INA226 ===================== */
#define INA226_CONFIG_REG       0x00
#define INA226_BUS_VOLTAGE_REG  0x02

#define INA_PANEL_ADDR          0x41
#define INA_BAT_ADDR            0x40
#define INA_BUS_ADDR            0x44  // bus DC

/* ===================== ACS712 ===================== */
#define ACS_PANEL_PIN           PA0
#define ACS_BUS_PIN             PA1
#define ACS_BAT_PIN             PA2

#define ACS_COUNT               3
#define ACS_SAMPLES             16
#define ADC_RESOLUTION          4095.0f
#define VREF                    3.3f
#define ACS_SENS_ADC            0.178f

const uint8_t acsPins[ACS_COUNT] = { ACS_PANEL_PIN, ACS_BUS_PIN, ACS_BAT_PIN };

float acsOffsetVoltage[ACS_COUNT] = {
  2.3336f,  // panel
  2.3467f,  // bus/carga
  2.3651f   // batería
};

/* ===================== MAX6675 (Temp batería) ===================== */
// Usas PA8 como CS (según tu sketch)
#define MAX6675_CS_BAT          PA8
// SPI1 remap: SCK=PB13, MISO=PB14, MOSI=PB15 (no usado)
#define MAX6675_SCK             PB13
#define MAX6675_MISO            PB14
#define MAX6675_MOSI            PB15

#define TEMP_PERIOD_MS          1000UL
#define TEMP_BAT_MAX_C          60.0f   // si supera, salir de batería
#define TEMP_BAT_RECOVER_C      55.0f   // para volver a permitir batería (histéresis)

/* ===================== PWM: panel (HW timer) + bat (analogWrite) ===================== */
#define PANEL_PWM_PIN           PA3   // TIM2_CH4
#define BAT_PWM_PIN             PA6   // analogWrite @ 50 kHz (global)

#define PANEL_PWM_FREQ          80000
#define BAT_PWM_FREQ            50000

#define PWM_MAX                 255
#define PANEL_PWM_MIN_ACTIVE    10
#define PANEL_PWM_START         50
#define BAT_PWM_MIN_ACTIVE      20
#define BAT_PWM_START           50

/* ===================== Timing ===================== */
#define CONTROL_PERIOD_MS       200UL
#define LOG_PERIOD_MS           1000UL
#define PWM_STEP                1
#define PWM_MAX_CHANGE_PER_SEC  20

/* ===================== MPPT / límites panel ===================== */
#define PANEL_MIN_VOLTAGE       5.0f

/* ===================== Battery CV setpoint ===================== */
#define VBUS_CV_LOW             17.8f
#define VBUS_CV_HIGH            18.2f

/* ===================== Conmutación ===================== */
#define VBUS_TO_BAT             16.0f
#define VBUS_TO_PANEL           17.5f
#define VBUS_TO_PANEL_HOLD_MS   2000UL
#define MODE_SWITCH_DEADTIME_MS 500UL

/* ===================== Protección batería 3S ===================== */
#define VBAT_UNDERVOLTAGE       9.50f
#define VBAT_RECOVER            10.20f
#define VBAT_CRITICAL           9.00f

/* Si no hay batería, operar con panel */
#define VBAT_PRESENT_MIN        5.0f

/* ===================== Globals ===================== */
float volt_panel = 0.0f;
float volt_bat   = 0.0f;
float volt_bus   = 0.0f;

float amp_panel  = 0.0f;
float amp_bat    = 0.0f;
float amp_bus    = 0.0f;

/* Temp batería (MAX6675) */
float temp_bat_c = NAN;
bool  temp_bat_ok = true; // “óptima” para permitir batería

unsigned long lastTempReadMs = 0;

/* Filtros */
float volt_panel_filt = 0.0f; bool volt_panel_init = false;
float volt_bus_filt   = 0.0f; bool volt_bus_init   = false;
float volt_bat_filt   = 0.0f; bool volt_bat_init   = false;

float amp_panel_filt  = 0.0f; bool amp_panel_init  = false;
float amp_bat_filt    = 0.0f; bool amp_bat_init    = false;

/* MPPT P&O */
float lastPower = 0.0f;
int   lastDelta = +1;
int   panelDuty = PANEL_PWM_START;

/* BAT CV */
int   batDuty   = BAT_PWM_START;

/* Estado */
enum Mode { MODE_PANEL_MPPT, MODE_BAT_CV, MODE_LOW_BAT };
Mode mode = MODE_PANEL_MPPT;

bool switching = false;
Mode pendingMode = MODE_PANEL_MPPT;
unsigned long switchStartMs = 0;

unsigned long lastControlMs    = 0;
unsigned long lastSensorReadMs = 0;
unsigned long lastLogMs        = 0;

unsigned long lastPwmChangeMs  = 0;
unsigned long vbusAboveSinceMs = 0;

/* ===================== PWM timer panel ===================== */
HardwareTimer *tPanel = nullptr;

/* ===================== Protos ===================== */
void setupINA226(uint8_t addr);
uint16_t readRegister16(uint8_t addr, uint8_t reg);
void writeRegister16(uint8_t addr, uint8_t reg, uint16_t value);

float readVoltageRaw(uint8_t addr);
float readVoltageCorrected(uint8_t addr);

uint16_t readADCRaw(uint8_t pin);
float readADCVoltageFromRaw(uint16_t raw);
float readCurrentACS(uint8_t index);

void updateSensorGlobals();

void setupPWM();
void setPanelDuty(int duty);
void setBatDuty(int duty);
void disableAllPWM();

void controlPanelMPPT();
void controlBatCV();
void updateModeStateMachine();
void resetMPPT();

void setupMAX6675();
float readMAX6675C(uint8_t csPin);
void updateBatteryTemp();

void logJSON();

/* ===================== Setup ===================== */
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\nHybrid PANEL_MPPT <-> BAT_CV + MAX6675 temp bat");

  Wire.begin();
  Wire.setClock(100000);

  setupINA226(INA_PANEL_ADDR);
  setupINA226(INA_BAT_ADDR);
  setupINA226(INA_BUS_ADDR);

  analogReadResolution(12);
  pinMode(ACS_PANEL_PIN, INPUT_ANALOG);
  pinMode(ACS_BUS_PIN,   INPUT_ANALOG);
  pinMode(ACS_BAT_PIN,   INPUT_ANALOG);

  setupMAX6675();

  setupPWM();
  disableAllPWM();

  updateSensorGlobals();
  updateBatteryTemp(); // primera lectura

  /* Arranque */
  if (volt_bat <= VBAT_UNDERVOLTAGE && volt_bat > VBAT_PRESENT_MIN) {
    mode = MODE_LOW_BAT;
    disableAllPWM();
  } else {
    mode = MODE_PANEL_MPPT;
    setBatDuty(0);
    setPanelDuty(PANEL_PWM_START);
    resetMPPT();
  }

  logJSON();
}

/* ===================== Loop ===================== */
void loop() {
  unsigned long now = millis();

  if (now - lastSensorReadMs >= CONTROL_PERIOD_MS) {
    lastSensorReadMs = now;
    updateSensorGlobals();
  }

  if (now - lastTempReadMs >= TEMP_PERIOD_MS) {
    lastTempReadMs = now;
    updateBatteryTemp();
  }

  if (now - lastControlMs >= CONTROL_PERIOD_MS) {
    lastControlMs = now;

    updateModeStateMachine();

    if (switching) {
      // deadtime: apagado dentro del state machine
    } else if (mode == MODE_PANEL_MPPT) {
      controlPanelMPPT();
    } else if (mode == MODE_BAT_CV) {
      controlBatCV();
    } else {
      disableAllPWM();
    }
  }

  if (now - lastLogMs >= LOG_PERIOD_MS) {
    lastLogMs = now;
    logJSON();
  }
}

/* ===================== MAX6675 ===================== */
void setupMAX6675() {
  pinMode(MAX6675_CS_BAT, OUTPUT);
  digitalWrite(MAX6675_CS_BAT, HIGH);

  SPI.setSCLK(MAX6675_SCK);
  SPI.setMISO(MAX6675_MISO);
  SPI.setMOSI(MAX6675_MOSI); // no usado por MAX6675 pero requerido
  SPI.begin();

  // Nota: beginTransaction por lectura (más seguro); aquí solo dejamos SPI listo.
}

float readMAX6675C(uint8_t csPin) {
  uint16_t raw = 0;

  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

  digitalWrite(csPin, LOW);
  delayMicroseconds(5);
  raw = SPI.transfer16(0x0000);
  digitalWrite(csPin, HIGH);

  SPI.endTransaction();

  // Bit D2 = 1 → termocupla desconectada
  if (raw & 0x0004) {
    return NAN;
  }

  raw >>= 3;
  return raw * 0.25f;
}

void updateBatteryTemp() {
  temp_bat_c = readMAX6675C(MAX6675_CS_BAT);

  // Si no hay lectura válida, por seguridad NO permitimos batería
  if (isnan(temp_bat_c)) {
    temp_bat_ok = false;
    return;
  }

  // Histéresis
  if (temp_bat_c >= TEMP_BAT_MAX_C) temp_bat_ok = false;
  else if (temp_bat_c <= TEMP_BAT_RECOVER_C) temp_bat_ok = true;
}

/* ===================== Sensors ===================== */
void updateSensorGlobals() {
  volt_panel = readVoltageCorrected(INA_PANEL_ADDR);
  volt_bat   = readVoltageCorrected(INA_BAT_ADDR);
  volt_bus   = readVoltageCorrected(INA_BUS_ADDR);

  amp_panel  = readCurrentACS(0);
  amp_bus    = readCurrentACS(1);
  amp_bat    = readCurrentACS(2);

  if (!volt_panel_init) { volt_panel_filt = volt_panel; volt_panel_init = true; }
  else { volt_panel_filt = 0.85f * volt_panel_filt + 0.15f * volt_panel; }

  if (!volt_bus_init) { volt_bus_filt = volt_bus; volt_bus_init = true; }
  else { volt_bus_filt = 0.85f * volt_bus_filt + 0.15f * volt_bus; }

  if (!volt_bat_init) { volt_bat_filt = volt_bat; volt_bat_init = true; }
  else { volt_bat_filt = 0.90f * volt_bat_filt + 0.10f * volt_bat; }

  if (!amp_panel_init) { amp_panel_filt = amp_panel; amp_panel_init = true; }
  else { amp_panel_filt = 0.90f * amp_panel_filt + 0.10f * amp_panel; }

  if (!amp_bat_init) { amp_bat_filt = amp_bat; amp_bat_init = true; }
  else { amp_bat_filt = 0.90f * amp_bat_filt + 0.10f * amp_bat; }
}

/* ===================== Mode machine ===================== */
static inline void beginSwitchTo(Mode next) {
  switching = true;
  pendingMode = next;
  switchStartMs = millis();
  disableAllPWM();
  vbusAboveSinceMs = 0;
}

void updateModeStateMachine() {
  float vbus = volt_bus_filt;
  float vbat = volt_bat_filt;
  unsigned long now = millis();

  bool batPresent = (vbat > VBAT_PRESENT_MIN);

  /* 0) Si estamos en transición */
  if (switching) {
    disableAllPWM();

    if (now - switchStartMs >= MODE_SWITCH_DEADTIME_MS) {
      switching = false;
      mode = pendingMode;

      if (mode == MODE_PANEL_MPPT) {
        setBatDuty(0);
        panelDuty = PANEL_PWM_START;
        setPanelDuty(panelDuty);
        resetMPPT();
      } else if (mode == MODE_BAT_CV) {
        setPanelDuty(0);
        batDuty = BAT_PWM_START;
        setBatDuty(batDuty);
      } else {
        disableAllPWM();
      }

      lastPwmChangeMs = now;
    }
    return;
  }

  /* 1) Si no hay batería, forzar panel siempre que sea posible */
  if (!batPresent) {
    if (mode != MODE_PANEL_MPPT) {
      beginSwitchTo(MODE_PANEL_MPPT);
    }
    return;
  }

  /* 2) Si estamos en BAT_CV y la temp excede 60°C → pasar a MPPT */
  if (mode == MODE_BAT_CV && !temp_bat_ok) {
    beginSwitchTo(MODE_PANEL_MPPT);
    return;
  }

  /* 3) Protecciones batería (solo si está presente) */
  if (vbat <= VBAT_CRITICAL) {
    mode = MODE_LOW_BAT;
    disableAllPWM();
    return;
  }

  if (mode == MODE_LOW_BAT) {
    disableAllPWM();
    if (vbat >= VBAT_RECOVER) beginSwitchTo(MODE_PANEL_MPPT);
    return;
  }

  if (vbat <= VBAT_UNDERVOLTAGE) {
    mode = MODE_LOW_BAT;
    disableAllPWM();
    return;
  }

  /* 4) Lógica cambio por bus (pero NO entrar a batería si temp no es óptima) */
  if (mode == MODE_PANEL_MPPT) {
    if (vbus < VBUS_TO_BAT) {
      if (temp_bat_ok) {
        beginSwitchTo(MODE_BAT_CV);
      } else {
        // panel cayó, pero temp no óptima: quedarse en MPPT como pediste
      }
    }
  } else if (mode == MODE_BAT_CV) {
    if (vbus > VBUS_TO_PANEL) {
      if (vbusAboveSinceMs == 0) vbusAboveSinceMs = now;
      if (now - vbusAboveSinceMs >= VBUS_TO_PANEL_HOLD_MS) beginSwitchTo(MODE_PANEL_MPPT);
    } else {
      vbusAboveSinceMs = 0;
    }
  }
}

/* ===================== Control: Panel MPPT ===================== */
void controlPanelMPPT() {
  float vpanel = volt_panel_filt;
  float ipanel = amp_panel_filt;

  unsigned long now = millis();
  unsigned long minTimeBetween = 1000UL / PWM_MAX_CHANGE_PER_SEC;
  if (now - lastPwmChangeMs < minTimeBetween) {
    setPanelDuty(panelDuty);
    return;
  }

  int oldDuty = panelDuty;

  if (vpanel < PANEL_MIN_VOLTAGE) {
    panelDuty = PANEL_PWM_MIN_ACTIVE;
    lastPower = 0.0f;
  } else {
    float power = vpanel * ipanel;

    if (power >= lastPower) panelDuty += lastDelta;
    else { lastDelta = -lastDelta; panelDuty += lastDelta; }

    lastPower = power;
  }

  panelDuty = constrain(panelDuty, PANEL_PWM_MIN_ACTIVE, PWM_MAX);
  if (panelDuty != oldDuty) lastPwmChangeMs = now;
  setPanelDuty(panelDuty);
}

/* ===================== Control: Battery CV ===================== */
void controlBatCV() {
  float vbus = volt_bus_filt;
  float vbat = volt_bat_filt;

  if (!temp_bat_ok) {
    beginSwitchTo(MODE_PANEL_MPPT);
    return;
  }

  if (vbat <= VBAT_UNDERVOLTAGE) {
    mode = MODE_LOW_BAT;
    disableAllPWM();
    return;
  }

  if (batDuty < BAT_PWM_MIN_ACTIVE) batDuty = BAT_PWM_MIN_ACTIVE;

  unsigned long now = millis();
  unsigned long minTimeBetween = 1000UL / PWM_MAX_CHANGE_PER_SEC;
  if (now - lastPwmChangeMs < minTimeBetween) {
    setBatDuty(batDuty);
    return;
  }

  int oldDuty = batDuty;

  if (vbus < VBUS_CV_LOW) batDuty += PWM_STEP;
  else if (vbus > VBUS_CV_HIGH) batDuty -= PWM_STEP;

  batDuty = constrain(batDuty, BAT_PWM_MIN_ACTIVE, PWM_MAX);
  if (batDuty != oldDuty) lastPwmChangeMs = now;
  setBatDuty(batDuty);
}

/* ===================== PWM setup (como pediste) ===================== */
void setupPWM() {
  pinMode(PANEL_PWM_PIN, OUTPUT);
  pinMode(BAT_PWM_PIN, OUTPUT);

  // PA3 → TIM2_CH4 → 80 kHz
  tPanel = new HardwareTimer(TIM2);
  tPanel->setMode(4, TIMER_OUTPUT_COMPARE_PWM1, PANEL_PWM_PIN);
  tPanel->setOverflow(PANEL_PWM_FREQ, HERTZ_FORMAT);
  tPanel->setCaptureCompare(4, 0, PERCENT_COMPARE_FORMAT); // start off
  tPanel->resume();

  // BAT: analogWrite @ 50 kHz (global para analogWrite)
  analogWriteFrequency(BAT_PWM_FREQ);
  analogWrite(BAT_PWM_PIN, 0);
}

void setPanelDuty(int duty) {
  panelDuty = constrain(duty, 0, PWM_MAX);
  float pct = (panelDuty * 100.0f) / 255.0f;
  tPanel->setCaptureCompare(4, pct, PERCENT_COMPARE_FORMAT);
}

void setBatDuty(int duty) {
  batDuty = constrain(duty, 0, PWM_MAX);
  analogWrite(BAT_PWM_PIN, batDuty);
}

void disableAllPWM() {
  setPanelDuty(0);
  setBatDuty(0);
}

void resetMPPT() {
  lastPower = 0.0f;
  lastDelta = +1;
}

/* ===================== Logging (tus keys intactas) ===================== */
void logJSON() {
  StaticJsonDocument<640> doc;

  doc["volt_panel"]   = round(volt_panel      * 1000.0f) / 1000.0f;
  doc["volt_panel_f"] = round(volt_panel_filt * 1000.0f) / 1000.0f;
  doc["amp_panel"]    = round(amp_panel        * 1000.0f) / 1000.0f;
  doc["amp_panel_f"]  = round(amp_panel_filt   * 1000.0f) / 1000.0f;
  doc["pow_panel"]    = round(volt_panel_filt * amp_panel_filt * 1000.0f) / 1000.0f;

  doc["volt_load"]     = round(volt_bus        * 1000.0f) / 1000.0f;
  doc["volt_load_f"]   = round(volt_bus_filt   * 1000.0f) / 1000.0f;
  doc["amp_load"]      = round(amp_bus          * 1000.0f) / 1000.0f;

  doc["volt_bat"]     = round(volt_bat        * 1000.0f) / 1000.0f;
  doc["volt_bat_f"]   = round(volt_bat_filt   * 1000.0f) / 1000.0f;
  doc["amp_bat"]      = round(amp_bat          * 1000.0f) / 1000.0f;
  doc["amp_bat_f"]    = round(amp_bat_filt     * 1000.0f) / 1000.0f;

  doc["temp_bat"]     = isnan(temp_bat_c) ? 0.0f : round(temp_bat_c * 10.0f) / 10.0f;
  doc["temp_bat_ok"]  = temp_bat_ok ? 1 : 0;

  doc["mode"] = (mode == MODE_PANEL_MPPT) ? "PANEL_MPPT" :
                (mode == MODE_BAT_CV)    ? "BAT_CV" :
                (mode == MODE_LOW_BAT)   ? "LOW_BAT" : "UNKNOWN";
  doc["switching"] = switching ? 1 : 0;
  doc["pwm_panel"] = panelDuty;
  doc["pwm_bat"]   = batDuty;

  serializeJson(doc, Serial);
  Serial.println();
}

/* ===================== INA226 voltages ===================== */
float readVoltageRaw(uint8_t addr) {
  uint16_t raw = readRegister16(addr, INA226_BUS_VOLTAGE_REG);
  return raw * 1.25e-3f;
}

float readVoltageCorrected(uint8_t addr) {
  float measured = readVoltageRaw(addr);

  // regla validada por ti: panel y bus +0.6 si measured > 0.6; bat sin offset
  if (addr == INA_BAT_ADDR) return measured;

  if (addr == INA_PANEL_ADDR || addr == INA_BUS_ADDR) {
    if (measured <= 0.6f) return 0.0f;
    return measured + 0.6f;
  }

  return measured;
}

/* ===================== ACS712 ===================== */
uint16_t readADCRaw(uint8_t pin) {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < ACS_SAMPLES; i++) {
    sum += analogRead(pin);
    delayMicroseconds(100);
  }
  return (uint16_t)(sum / ACS_SAMPLES);
}

float readADCVoltageFromRaw(uint16_t raw) {
  return (raw / ADC_RESOLUTION) * VREF;
}

float readCurrentACS(uint8_t index) {
  if (index >= ACS_COUNT) return 0.0f;
  uint16_t raw = readADCRaw(acsPins[index]);
  float voltage = readADCVoltageFromRaw(raw);
  return (voltage - acsOffsetVoltage[index]) / ACS_SENS_ADC;
}

/* ===================== INA226 I2C ===================== */
void setupINA226(uint8_t addr) {
  writeRegister16(addr, INA226_CONFIG_REG, 0x4527);
}

uint16_t readRegister16(uint8_t addr, uint8_t reg) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0;
  if (Wire.requestFrom(addr, (uint8_t)2) != 2) return 0;
  return ((uint16_t)Wire.read() << 8) | Wire.read();
}

void writeRegister16(uint8_t addr, uint8_t reg, uint16_t value) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write((uint8_t)(value >> 8));
  Wire.write((uint8_t)(value & 0xFF));
  Wire.endTransmission();
}
