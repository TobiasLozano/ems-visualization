#include <Arduino.h>
#include <Wire.h>
#include <ArduinoJson.h>

/* ============================================================
   INA226
   ============================================================ */
#define INA226_CONFIG_REG       0x00
#define INA226_BUS_VOLTAGE_REG  0x02

#define INA_PANEL_ADDR          0x41
#define INA_BAT_ADDR            0x40
#define INA_LOAD_ADDR           0x44

/* ============================================================
   ACS712
   ============================================================ */
#define ACS_PANEL_PIN           PA0
#define ACS_LOAD_PIN            PA1
#define ACS_BAT_PIN             PA2

#define ACS_COUNT               3
#define ACS_SAMPLES             16

const uint8_t acsPins[ACS_COUNT] = {
  ACS_PANEL_PIN,
  ACS_LOAD_PIN,
  ACS_BAT_PIN
};

float acsOffsetVoltage[ACS_COUNT] = {
  2.3336f,   // Panel
  2.3467f,   // Carga / bus
  2.3651f    // Batería
};

#define ADC_RESOLUTION          4095.0f
#define VREF                    3.3f
#define ACS_SENS_ADC            0.178f

/* ============================================================
   PWM BUCK
   ============================================================ */
#define PWM_PIN                 PA7
#define PWM_MIN                 0
#define PWM_MIN_ACTIVE          25
#define PWM_MAX                 180
#define PWM_START               30

/* ============================================================
   Control
   ============================================================ */
#define CONTROL_PERIOD_MS       200UL
#define LOG_PERIOD_MS           200UL
#define PWM_STEP                1
#define PWM_MAX_CHANGE_PER_SEC  10

/* ============================================================
   Modo CC
   ============================================================ */
#define IREF_CC                 1.00f
#define IREF_LOW                0.95f
#define IREF_HIGH               1.05f

/* ============================================================
   Modo CV para batería 3S Li-ion
   ============================================================ */
#define VCV_TARGET              12.60f
#define VCV_ENTER               12.50f
#define VCV_HYST                0.05f

#define CV_VOLT_LOW             12.55f
#define CV_VOLT_HIGH            12.60f

/* Fin de carga en CV */
#define I_END_CHARGE            0.35f

/* Seguridad */
#define VIN_PRESENT_THRESHOLD   5.0f
#define VIN_HEADROOM_MIN        2.0f
#define VBAT_OVERVOLTAGE        12.75f

/* ============================================================
   Estado
   ============================================================ */
enum ChargeMode {
  MODE_CC = 0,
  MODE_CV = 1,
  MODE_DONE = 2
};

ChargeMode chargeMode = MODE_CC;

int pwmDuty = PWM_START;

unsigned long lastControlMs    = 0;
unsigned long lastLogMs        = 0;
unsigned long lastPwmChangeMs  = 0;

/* ============================================================
   Variables medidas
   ============================================================ */
float volt_panel = 0.0f;
float volt_bat   = 0.0f;
float volt_load  = 0.0f;

float amp_panel  = 0.0f;
float amp_bat    = 0.0f;
float amp_load   = 0.0f;

float volt_bat_filt = 0.0f;
bool  volt_bat_filt_init = false;

float amp_bat_filt = 0.0f;
bool  amp_bat_filt_init = false;

/* ============================================================
   Prototipos
   ============================================================ */
void     writeRegister16(uint8_t addr, uint8_t reg, uint16_t value);
uint16_t readRegister16(uint8_t addr, uint8_t reg);
void     setupINA226(uint8_t addr);

float    getINAOffset(uint8_t addr);
float    applyVoltageOffset(float measuredVoltage, float offset);
float    readVoltage(uint8_t addr);

uint16_t readADCRaw(uint8_t pin);
float    readADCVoltageFromRaw(uint16_t raw);
float    readCurrentACS(uint8_t index, float voltage);

void     updateMeasurements();
void     setPWMDuty(int duty);
void     logJSON();
const char* modeToString(ChargeMode mode);

/* ============================================================
   SETUP
   ============================================================ */
void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin();
  Wire.setClock(100000);

  setupINA226(INA_PANEL_ADDR);
  setupINA226(INA_BAT_ADDR);
  setupINA226(INA_LOAD_ADDR);

  analogReadResolution(12);
  pinMode(ACS_PANEL_PIN, INPUT_ANALOG);
  pinMode(ACS_LOAD_PIN,  INPUT_ANALOG);
  pinMode(ACS_BAT_PIN,   INPUT_ANALOG);

  setPWMDuty(PWM_START);

  updateMeasurements();

  if (volt_bat_filt >= VCV_ENTER) {
    chargeMode = MODE_CV;
  } else {
    chargeMode = MODE_CC;
  }

  logJSON();
}

/* ============================================================
   LOOP
   ============================================================ */
void loop() {
  unsigned long now = millis();

  if (now - lastControlMs >= CONTROL_PERIOD_MS) {
    lastControlMs = now;

    updateMeasurements();

    float vbus = volt_load;
    float vbat = volt_bat_filt;
    float ibat = amp_bat_filt;

    bool busPresent     = (vbus >= VIN_PRESENT_THRESHOLD);
    bool enoughHeadroom = (vbus >= (vbat + VIN_HEADROOM_MIN));
    bool overVoltage    = (vbat > VBAT_OVERVOLTAGE);

    if (!busPresent || !enoughHeadroom || overVoltage) {
      setPWMDuty(0);
    } else if (chargeMode == MODE_DONE) {
      setPWMDuty(0);
    } else {
      if (pwmDuty < PWM_MIN_ACTIVE) {
        pwmDuty = PWM_MIN_ACTIVE;
      }

      unsigned long timeSinceLastChange   = now - lastPwmChangeMs;
      unsigned long minTimeBetweenChanges = 1000UL / PWM_MAX_CHANGE_PER_SEC;
      bool canChangePWM = (timeSinceLastChange >= minTimeBetweenChanges);

      /* Cambio de modo CC -> CV */
      if (chargeMode == MODE_CC && vbat >= VCV_ENTER) {
        chargeMode = MODE_CV;
      }

      /* Histéresis opcional de regreso CV -> CC si cae bastante */
      if (chargeMode == MODE_CV && vbat < (VCV_ENTER - VCV_HYST)) {
        chargeMode = MODE_CC;
      }

      if (canChangePWM) {
        int oldDuty = pwmDuty;

        if (chargeMode == MODE_CC) {
          if (ibat < IREF_LOW) {
            pwmDuty += PWM_STEP;
          } else if (ibat > IREF_HIGH) {
            pwmDuty -= PWM_STEP;
          }
        } else if (chargeMode == MODE_CV) {
          if (vbat < CV_VOLT_LOW) {
            pwmDuty += PWM_STEP;
          } else if (vbat > CV_VOLT_HIGH) {
            pwmDuty -= PWM_STEP;
          }

          /* Fin de carga: en CV, corriente baja */
          if (vbat >= (VCV_TARGET - 0.03f) && ibat <= I_END_CHARGE) {
            chargeMode = MODE_DONE;
            pwmDuty = 0;
          }
        }

        if (pwmDuty != oldDuty) {
          lastPwmChangeMs = now;
        }
      }

      if (chargeMode == MODE_DONE) {
        setPWMDuty(0);
      } else {
        pwmDuty = constrain(pwmDuty, PWM_MIN_ACTIVE, PWM_MAX);
        setPWMDuty(pwmDuty);
      }
    }
  }

  if (now - lastLogMs >= LOG_PERIOD_MS) {
    lastLogMs = now;
    logJSON();
  }
}

/* ============================================================
   Medición
   ============================================================ */
void updateMeasurements() {
  volt_panel = readVoltage(INA_PANEL_ADDR);
  volt_bat   = readVoltage(INA_BAT_ADDR);
  volt_load  = readVoltage(INA_LOAD_ADDR);

  if (!volt_bat_filt_init) {
    volt_bat_filt      = volt_bat;
    volt_bat_filt_init = true;
  } else {
    volt_bat_filt = 0.90f * volt_bat_filt + 0.10f * volt_bat;
  }

  {
    uint16_t raw = readADCRaw(ACS_PANEL_PIN);
    float voltage = readADCVoltageFromRaw(raw);
    amp_panel = readCurrentACS(0, voltage);
  }
  {
    uint16_t raw = readADCRaw(ACS_LOAD_PIN);
    float voltage = readADCVoltageFromRaw(raw);
    amp_load = readCurrentACS(1, voltage);
  }
  {
    uint16_t raw = readADCRaw(ACS_BAT_PIN);
    float voltage = readADCVoltageFromRaw(raw);
    amp_bat = readCurrentACS(2, voltage);
  }

  if (!amp_bat_filt_init) {
    amp_bat_filt      = amp_bat;
    amp_bat_filt_init = true;
  } else {
    amp_bat_filt = 0.90f * amp_bat_filt + 0.10f * amp_bat;
  }
}

/* ============================================================
   JSON log
   ============================================================ */
void logJSON() {
  StaticJsonDocument<256> doc;

  doc["volt_panel"] = round(volt_panel * 1000.0f) / 1000.0f;
  doc["amp_panel"]  = round(amp_panel  * 1000.0f) / 1000.0f;

  doc["volt_bat"]   = round(volt_bat   * 1000.0f) / 1000.0f;
  doc["volt_bat_f"] = round(volt_bat_filt * 1000.0f) / 1000.0f;

  doc["amp_bat"]    = round(amp_bat    * 1000.0f) / 1000.0f;
  doc["amp_bat_f"]  = round(amp_bat_filt * 1000.0f) / 1000.0f;

  doc["volt_load"]  = round(volt_load  * 1000.0f) / 1000.0f;
  doc["amp_load"]   = round(amp_load   * 1000.0f) / 1000.0f;

  doc["pwm"]        = pwmDuty;
  doc["mode"]       = modeToString(chargeMode);

  serializeJson(doc, Serial);
  Serial.println();
}

const char* modeToString(ChargeMode mode) {
  switch (mode) {
    case MODE_CC:   return "CC";
    case MODE_CV:   return "CV";
    case MODE_DONE: return "DONE";
    default:        return "UNKNOWN";
  }
}

/* ============================================================
   INA226 voltaje con offset
   ============================================================ */
float readVoltage(uint8_t addr) {
  uint16_t raw = readRegister16(addr, INA226_BUS_VOLTAGE_REG);
  float measuredVoltage = raw * 1.25e-3f;
  float offset = getINAOffset(addr);
  return applyVoltageOffset(measuredVoltage, offset);
}

float getINAOffset(uint8_t addr) {
  switch (addr) {
    case INA_PANEL_ADDR: return 0.6f;
    case INA_BAT_ADDR:   return 0.0f;
    case INA_LOAD_ADDR:  return 0.6f;
    default:             return 0.0f;
  }
}

float applyVoltageOffset(float measuredVoltage, float offset) {
  if (offset <= 0.0f)            return measuredVoltage;
  if (measuredVoltage <= offset) return 0.0f;
  return measuredVoltage + offset;
}

/* ============================================================
   ACS712
   ============================================================ */
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

float readCurrentACS(uint8_t index, float voltage) {
  if (index >= ACS_COUNT) return 0.0f;
  return (voltage - acsOffsetVoltage[index]) / ACS_SENS_ADC;
}

/* ============================================================
   PWM
   ============================================================ */
void setPWMDuty(int duty) {
  pwmDuty = constrain(duty, PWM_MIN, PWM_MAX);
  analogWrite(PWM_PIN, pwmDuty);
}

/* ============================================================
   INA226 I2C
   ============================================================ */
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
