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
