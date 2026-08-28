# Código Fuente del Sistema EMS - Microgrid DC

```text
// ==========================================
// ARCHIVO: firmware/MPPT - BATERÍA 20W - EMS con protección térmica.ino
// ==========================================

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


// ==========================================
// ARCHIVO: backend/serial_reader.py
// ==========================================

#!/usr/bin/env python3
import serial
import psycopg2
import os
import json
import time
import logging
from datetime import datetime
from dotenv import load_dotenv

load_dotenv()

LOG_LEVEL = os.getenv('LOG_LEVEL', 'INFO').upper()
logging.basicConfig(
    level=getattr(logging, LOG_LEVEL, logging.INFO),
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

SERIAL_PORT = os.getenv('SERIAL_PORT', '/dev/ttyUSB0')
SERIAL_BAUDRATE = int(os.getenv('SERIAL_BAUDRATE', '9600'))
DB_HOST = os.getenv('DB_HOST', 'postgres')
DB_PORT = int(os.getenv('DB_PORT', '5432'))
DB_NAME = os.getenv('DB_NAME', 'sensors_db')
DB_USER = os.getenv('DB_USER', 'postgres')
DB_PASSWORD = os.getenv('DB_PASSWORD', 'postgres')


class SerialReader:
    def __init__(self):
        self.ser = None
        self.conn = None
        self.cursor = None

    def conectar(self):
        try:
            self.ser = serial.Serial(SERIAL_PORT, SERIAL_BAUDRATE, timeout=1)
            logger.info(f"Serial: {SERIAL_PORT} @ {SERIAL_BAUDRATE} baud")
        except Exception as e:
            logger.error(f"Error serial: {e}")
            return False

        try:
            self.conn = psycopg2.connect(
                host=DB_HOST, port=DB_PORT, database=DB_NAME,
                user=DB_USER, password=DB_PASSWORD
            )
            self.cursor = self.conn.cursor()
            logger.info("BD conectada")
        except Exception as e:
            logger.error(f"Error BD: {e}")
            return False

        self.crear_tabla()
        return True

    def crear_tabla(self):
        tables = {
            'temp_panel': 'Temperatura Panel',
            'temp_bat': 'Temperatura Batería',
            'volt_panel': 'Voltaje Panel',
            'amp_panel': 'Corriente Panel',
            'volt_bat': 'Voltaje Batería',
            'amp_bat': 'Corriente Batería',
            'volt_load': 'Voltaje Carga',
            'amp_load': 'Corriente Carga'
        }

        for table_name in tables.keys():
            self.cursor.execute(f"""
                CREATE TABLE IF NOT EXISTS {table_name} (
                    id SERIAL PRIMARY KEY,
                    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                    value FLOAT
                );
                CREATE INDEX IF NOT EXISTS idx_{table_name}_timestamp
                ON {table_name}(timestamp);
            """)
        self.conn.commit()
        logger.info("Tablas creadas")

    def guardar(self, metric, value, timestamp):
        try:
            self.cursor.execute(
                f"INSERT INTO {metric} (timestamp, value) VALUES (%s, %s)",
                (timestamp, value)
            )
            self.conn.commit()
            # logger.info(f"{metric}={value}")
        except Exception as e:
            logger.error(f"Error guardando en {metric}: {e}")
            self.conn.rollback()

    def extraer_json(self, linea):
        """Extrae el objeto JSON de una línea que puede traer ruido serial."""
        if not linea:
            return None

        # Muchos dispositivos seriales envían bytes nulos o texto de arranque.
        limpia = linea.replace('\x00', '').strip()
        if not limpia:
            return None

        ini = limpia.find('{')
        fin = limpia.rfind('}')
        if ini == -1 or fin == -1 or fin <= ini:
            return None

        return limpia[ini:fin + 1]

    def procesar_linea(self, linea):
        try:
            linea_json = self.extraer_json(linea)
            if not linea_json:
                return

            logger.debug(f"json: {linea_json}")
            datos = json.loads(linea_json)
            if not isinstance(datos, dict):
                return

            logger.info(f"datos: {datos}")
            
            timestamp = datetime.now()
            metricas_validas = {
                'temp_panel', 'temp_bat', 'volt_panel', 'amp_panel',
                'volt_bat', 'amp_bat', 'volt_load', 'amp_load'
            }
            for metric, value in datos.items():
                if metric in metricas_validas and isinstance(value, (int, float)):
                    # logger.info(f"Nuevo dato recibido -> {metric}={value}")

                    self.guardar(metric, float(value), timestamp)
        except json.JSONDecodeError:
            # Línea con formato no-JSON completo; se ignora para no inundar logs.
            logger.debug(f"Línea descartada: {repr(linea)}")
        except Exception as e:
            logger.error(f"Error procesando: {e}")

    def leer(self):
        if not self.conectar():
            return

        logger.info("Leyendo puerto serial...")
        try:
            while True:
                if self.ser.in_waiting:
                    linea = self.ser.readline().decode('utf-8', errors='ignore')
                    if linea:
                        self.procesar_linea(linea)
                time.sleep(0.01)
        except KeyboardInterrupt:
            logger.info("Detenido")
        finally:
            if self.ser:
                self.ser.close()
            if self.cursor:
                self.cursor.close()
            if self.conn:
                self.conn.close()


if __name__ == "__main__":
    SerialReader().leer()


// ==========================================
// ARCHIVO: backend/web_app.py
// ==========================================

import os
import psycopg2
import csv
from datetime import datetime
from flask import Flask, render_template, jsonify, request, Response
from dotenv import load_dotenv

load_dotenv()

app = Flask(__name__)

DB_HOST = os.getenv('DB_HOST', 'postgres')
DB_PORT = int(os.getenv('DB_PORT', '5432'))
DB_NAME = os.getenv('DB_NAME', 'sensors_db')
DB_USER = os.getenv('DB_USER', 'postgres')
DB_PASSWORD = os.getenv('DB_PASSWORD', 'postgres')

def get_db_connection():
    return psycopg2.connect(
        host=DB_HOST,
        port=DB_PORT,
        database=DB_NAME,
        user=DB_USER,
        password=DB_PASSWORD
    )

def init_db():
    tables = {
        'temp_panel': 'Temperatura Panel',
        'temp_bat': 'Temperatura Batería',
        'volt_panel': 'Voltaje Panel',
        'amp_panel': 'Corriente Panel',
        'volt_bat': 'Voltaje Batería',
        'amp_bat': 'Corriente Batería',
        'volt_load': 'Voltaje Carga',
        'amp_load': 'Corriente Carga'
    }
    conn = None
    cursor = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor()
        for table_name in tables.keys():
            cursor.execute(f"""
                CREATE TABLE IF NOT EXISTS {table_name} (
                    id SERIAL PRIMARY KEY,
                    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                    value FLOAT
                );
                CREATE INDEX IF NOT EXISTS idx_{table_name}_timestamp
                ON {table_name}(timestamp);
            """)
        conn.commit()
    except Exception as e:
        print(f"Error inicializando BD: {e}")
    finally:
        if cursor:
            cursor.close()
        if conn:
            conn.close()

# Initialize DB on app startup
init_db()

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/tables', methods=['GET'])
def get_tables():
    conn = None
    cursor = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor()
        
        # We only want tables that are likely sensor data (exclude system tables etc)
        # Assuming all sensor tables are in the public schema
        cursor.execute("""
            SELECT table_name
            FROM information_schema.tables
            WHERE table_schema = 'public'
        """)
        tables = cursor.fetchall()
        
        table_info = []
        for (table_name,) in tables:
            cursor.execute(f"SELECT COUNT(*) FROM {table_name}")
            row_count = cursor.fetchone()[0]
            
            cursor.execute(f"SELECT pg_size_pretty(pg_total_relation_size('{table_name}'))")
            size = cursor.fetchone()[0]
            
            table_info.append({
                'name': table_name,
                'row_count': row_count,
                'size': size
            })
            
        return jsonify(table_info)
    except Exception as e:
        return jsonify({'error': str(e)}), 500
    finally:
        if cursor:
            cursor.close()
        if conn:
            conn.close()

@app.route('/api/download/<table_name>', methods=['GET'])
def download_data(table_name):
    start = request.args.get('start')
    end = request.args.get('end')
    
    if not start or not end:
        return "Missing start or end date", 400
        
    conn = None
    cursor = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor()
        
        # Verify table name to prevent SQL injection
        cursor.execute("""
            SELECT table_name
            FROM information_schema.tables
            WHERE table_schema = 'public' AND table_name = %s
        """, (table_name,))
        if not cursor.fetchone():
            return "Invalid table name", 400
            
        # Execute query for data
        query = f"SELECT id, timestamp, value FROM {table_name} WHERE timestamp >= %s AND timestamp <= %s ORDER BY timestamp ASC"
        cursor.execute(query, (start, end))
        
        def generate():
            try:
                yield 'id,timestamp,value\n'
                for row in cursor:
                    # row is (id, timestamp, value)
                    yield f"{row[0]},{row[1]},{row[2]}\n"
            finally:
                cursor.close()
                conn.close()
                
        response = Response(generate(), mimetype='text/csv')
        response.headers.set("Content-Disposition", f"attachment; filename={table_name}_{start}_to_{end}.csv")
        return response
        
    except Exception as e:
        if 'cursor' in locals() and cursor:
            cursor.close()
        if 'conn' in locals() and conn:
            conn.close()
        return str(e), 500

@app.route('/api/serial-data', methods=['POST'])
def receive_serial_data():
    data = request.json
    if not data:
        return jsonify({'error': 'No JSON received'}), 400
        
    conn = None
    cursor = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor()
        
        timestamp = datetime.now()
        metricas_validas = {
            'temp_panel', 'temp_bat', 'volt_panel', 'amp_panel',
            'volt_bat', 'amp_bat', 'volt_load', 'amp_load'
        }
        
        inserted = 0
        for metric, value in data.items():
            if metric in metricas_validas and isinstance(value, (int, float)):
                cursor.execute(
                    f"INSERT INTO {metric} (timestamp, value) VALUES (%s, %s)",
                    (timestamp, float(value))
                )
                inserted += 1
                
        conn.commit()
        return jsonify({'status': 'success', 'inserted': inserted}), 200
        
    except Exception as e:
        if conn:
            conn.rollback()
        return jsonify({'error': str(e)}), 500
    finally:
        if cursor:
            cursor.close()
        if conn:
            conn.close()

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)


// ==========================================
// ARCHIVO: sql_scripts/init.sql
// ==========================================

-- Tabla: Temperatura Panel
CREATE TABLE IF NOT EXISTS temp_panel (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    value FLOAT
);
CREATE INDEX IF NOT EXISTS idx_temp_panel_timestamp ON temp_panel(timestamp);

-- Tabla: Temperatura Batería
CREATE TABLE IF NOT EXISTS temp_bat (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    value FLOAT
);
CREATE INDEX IF NOT EXISTS idx_temp_bat_timestamp ON temp_bat(timestamp);

-- Tabla: Voltaje Panel
CREATE TABLE IF NOT EXISTS volt_panel (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    value FLOAT
);
CREATE INDEX IF NOT EXISTS idx_volt_panel_timestamp ON volt_panel(timestamp);

-- Tabla: Corriente Panel
CREATE TABLE IF NOT EXISTS amp_panel (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    value FLOAT
);
CREATE INDEX IF NOT EXISTS idx_amp_panel_timestamp ON amp_panel(timestamp);

-- Tabla: Voltaje Batería
CREATE TABLE IF NOT EXISTS volt_bat (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    value FLOAT
);
CREATE INDEX IF NOT EXISTS idx_volt_bat_timestamp ON volt_bat(timestamp);

-- Tabla: Corriente Batería
CREATE TABLE IF NOT EXISTS amp_bat (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    value FLOAT
);
CREATE INDEX IF NOT EXISTS idx_amp_bat_timestamp ON amp_bat(timestamp);

-- Tabla: Voltaje Carga/Load
CREATE TABLE IF NOT EXISTS volt_load (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    value FLOAT
);
CREATE INDEX IF NOT EXISTS idx_volt_load_timestamp ON volt_load(timestamp);

-- Tabla: Corriente Carga/Load
CREATE TABLE IF NOT EXISTS amp_load (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    value FLOAT
);
CREATE INDEX IF NOT EXISTS idx_amp_load_timestamp ON amp_load(timestamp);


// ==========================================
// ARCHIVO: sql_scripts/seed.sql
// ==========================================

-- Seed data: últimas 24h, una lectura cada 15 min (96 registros por tabla)

INSERT INTO temp_panel (timestamp, value)
SELECT NOW() - (interval '15 minutes' * s), 45 + random() * 20
FROM generate_series(0, 95) s;

INSERT INTO temp_bat (timestamp, value)
SELECT NOW() - (interval '15 minutes' * s), 30 + random() * 15
FROM generate_series(0, 95) s;

INSERT INTO volt_panel (timestamp, value)
SELECT NOW() - (interval '15 minutes' * s), 17 + random() * 4
FROM generate_series(0, 95) s;

INSERT INTO amp_panel (timestamp, value)
SELECT NOW() - (interval '15 minutes' * s), 2 + random() * 3
FROM generate_series(0, 95) s;

INSERT INTO volt_bat (timestamp, value)
SELECT NOW() - (interval '15 minutes' * s), 12 + random() * 2
FROM generate_series(0, 95) s;

INSERT INTO amp_bat (timestamp, value)
SELECT NOW() - (interval '15 minutes' * s), 1 + random() * 5
FROM generate_series(0, 95) s;

INSERT INTO volt_load (timestamp, value)
SELECT NOW() - (interval '15 minutes' * s), 11.5 + random() * 2
FROM generate_series(0, 95) s;

INSERT INTO amp_load (timestamp, value)
SELECT NOW() - (interval '15 minutes' * s), 0.5 + random() * 2
FROM generate_series(0, 95) s;


// ==========================================
// ARCHIVO: docker-compose.yml
// ==========================================

services:
  postgres:
    build:
      context: .
      dockerfile: backend/Dockerfile.postgres
    image: ${DOCKER_USERNAME:-miusuario}/ems-postgres:latest
    container_name: solar_postgres
    environment:
      POSTGRES_USER: ${DB_USER:-postgres}
      POSTGRES_PASSWORD: ${DB_PASSWORD:-postgres}
      POSTGRES_DB: ${DB_NAME:-sensors_db}
    ports:
      - "5433:5432"
    volumes:
      - postgres_data:/var/lib/postgresql/data
    healthcheck:
      test: ["CMD-SHELL", "pg_isready -U ${DB_USER:-postgres}"]
      interval: 10s
      timeout: 5s
      retries: 5
    networks:
      - solar_net
    restart: unless-stopped



#  serial_reader:
#    build:
#      context: .
#      dockerfile: backend/Dockerfile.reader
#    image: ${DOCKER_USERNAME:-miusuario}/ems-reader:latest
#    container_name: solar_reader
#    environment:
#      SERIAL_PORT: ${SERIAL_PORT:-/dev/ttyUSB0}
#      SERIAL_BAUDRATE: ${SERIAL_BAUDRATE:-9600}
#      DB_HOST: postgres
#      DB_PORT: 5432
#      DB_NAME: ${DB_NAME:-sensors_db}
#      DB_USER: ${DB_USER:-postgres}
#      DB_PASSWORD: ${DB_PASSWORD:-postgres}
#    devices:
#      - /dev/ttyUSB0:/dev/ttyUSB0
#    depends_on:
#      postgres:
#        condition: service_healthy
#    networks:
#      - solar_net
#    restart: unless-stopped

  grafana:
    build:
      context: .
      dockerfile: grafana/Dockerfile.grafana
    image: ${DOCKER_USERNAME:-miusuario}/ems-grafana:latest
    container_name: solar_grafana
    ports:
      - "3000:3000"
    volumes:
      - grafana_data:/var/lib/grafana
    depends_on:
      - postgres
    networks:
      - solar_net
    restart: unless-stopped

  web_app:
    build:
      context: .
      dockerfile: backend/Dockerfile.web
    image: ${DOCKER_USERNAME:-miusuario}/ems-webapp:latest
    container_name: solar_webapp
    environment:
      DB_HOST: postgres
      DB_PORT: 5432
      DB_NAME: ${DB_NAME:-sensors_db}
      DB_USER: ${DB_USER:-postgres}
      DB_PASSWORD: ${DB_PASSWORD:-postgres}
    ports:
      - "5000:5000"
    depends_on:
      postgres:
        condition: service_healthy
    networks:
      - solar_net
    restart: unless-stopped

volumes:
  postgres_data:
  grafana_data:

networks:
  solar_net:
    driver: bridge


// ==========================================
// ARCHIVO: docker-compose.prod.yml
// ==========================================

services:
  postgres:
    image: ${DOCKER_USERNAME:-tobiaslozano}/ems-postgres:latest
    container_name: solar_postgres
    environment:
      POSTGRES_USER: ${DB_USER:-postgres}
      POSTGRES_PASSWORD: ${DB_PASSWORD:-postgres}
      POSTGRES_DB: ${DB_NAME:-sensors_db}
    ports:
      - "5432:5432"
    volumes:
      - postgres_data:/var/lib/postgresql/data
    healthcheck:
      test: ["CMD-SHELL", "pg_isready -U ${DB_USER:-postgres}"]
      interval: 10s
      timeout: 5s
      retries: 5
    networks:
      - solar_net
    restart: unless-stopped

#  serial_reader:
#    image: ${DOCKER_USERNAME:-tobiaslozano}/ems-reader:latest
#    container_name: solar_reader
#    environment:
#      SERIAL_PORT: ${SERIAL_PORT:-/dev/ttyUSB0}
#      SERIAL_BAUDRATE: ${SERIAL_BAUDRATE:-9600}
#      DB_HOST: postgres
#      DB_PORT: 5432
#      DB_NAME: ${DB_NAME:-sensors_db}
#      DB_USER: ${DB_USER:-postgres}
#      DB_PASSWORD: ${DB_PASSWORD:-postgres}
#    devices:
#      - /dev/ttyUSB0:/dev/ttyUSB0
#    depends_on:
#      postgres:
#        condition: service_healthy
#    networks:
#      - solar_net
#    restart: unless-stopped

  grafana:
    image: ${DOCKER_USERNAME:-tobiaslozano}/ems-grafana:latest
    container_name: solar_grafana
    ports:
      - "3000:3000"
    volumes:
      - grafana_data:/var/lib/grafana
    depends_on:
      - postgres
    networks:
      - solar_net
    restart: unless-stopped

  web_app:
    image: ${DOCKER_USERNAME:-tobiaslozano}/ems-webapp:latest
    container_name: solar_webapp
    environment:
      DB_HOST: postgres
      DB_PORT: 5432
      DB_NAME: ${DB_NAME:-sensors_db}
      DB_USER: ${DB_USER:-postgres}
      DB_PASSWORD: ${DB_PASSWORD:-postgres}
    ports:
      - "5000:5000"
    depends_on:
      postgres:
        condition: service_healthy
    networks:
      - solar_net
    restart: unless-stopped

volumes:
  postgres_data:
  grafana_data:

networks:
  solar_net:
    driver: bridge



```
