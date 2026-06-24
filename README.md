# Sistema Solar - Monitoreo en Tiempo Real

Sistema Docker para leer datos de sensores via USB-TTL del Arduino y visualizar histórico en un frontend web.

## Requisitos

- Docker y Docker Compose
- Arduino/ESP32 con sensores conectado por USB-TTL
- Permisos en puerto serial

## Verificar puerto serial

```bash
# Listar puertos disponibles
ls /dev/ttyUSB*

# Si ves /dev/ttyUSB0 está listo
# Si es otro puerto, actualizar .env
```

## Arrancar Sistema

```bash
# Construir y levantar contenedores
docker-compose up -d

# Ver estado de contenedores
docker-compose ps

# Ver logs en tiempo real
docker-compose logs -f serial_reader
```

## Acceso a Grafana

```
http://localhost:3000
```

> Grafana está incluido en el stack para visualizaciones avanzadas, y es el componente principal para visualizar los datos de la base de datos.
> La base de datos y los dashboards se autoconfiguran al iniciar el contenedor.

## Datos Esperados

Arduino envía JSON cada 5 segundos:

```json
{
  "temp_panel": 45.2,
  "temp_bat": 42.1,
  "volt_panel": 48.5,
  "amp_panel": 3.2,
  "volt_bat": 48.3,
  "amp_bat": 2.8,
  "volt_load": 48.1,
  "amp_load": 1.5
}
```

Almacenados en 8 tablas time-series (una por sensor):
- `temp_panel`, `temp_bat` - Temperaturas en °C
- `volt_panel`, `volt_bat`, `volt_load` - Voltajes en V (0-50V)
- `amp_panel`, `amp_bat`, `amp_load` - Corrientes en A
Cada tabla contiene:
- `timestamp`: fecha/hora automática
- `value`: valor numérico

## Cambiar Configuración

Editar `.env`:
```bash
SERIAL_PORT=/dev/ttyUSB0      # Puerto serial
SERIAL_BAUDRATE=9600          # Velocidad
DB_PASSWORD=postgres           # Contraseña BD
```

Reiniciar servicio:
```bash
docker-compose restart serial_reader
```

## Conectar a Base de Datos

```bash
# Acceder a PostgreSQL
docker-compose exec postgres psql -U postgres -d sensors_db

# Ver últimos datos de cualquier tabla
SELECT * FROM temp_panel ORDER BY timestamp DESC LIMIT 10;
SELECT * FROM volt_load ORDER BY timestamp DESC LIMIT 10;
SELECT * FROM amp_load ORDER BY timestamp DESC LIMIT 10;

# Ver promedio por tabla
SELECT AVG(value), MAX(value), MIN(value) FROM temp_panel;
SELECT AVG(value), MAX(value), MIN(value) FROM volt_load;
```

## Solución de Problemas

**Puerto serial no encontrado:**
```bash
# Ver en dmesg
dmesg | tail -20

# Cambiar permisos (temporal)
sudo chmod 666 /dev/ttyUSB0
```

**Serial Reader falla:**
```bash
# Ver logs detallados
docker-compose logs serial_reader

# Verificar velocidad coincidir con Arduino
# Editar .env y reiniciar
```

**Sin datos en BD:**
1. Verificar Arduino envía datos: `cat /dev/ttyUSB0`
2. Ver logs: `docker-compose logs -f serial_reader`
3. Revisar que JSON tenga las 7 métricas

## Parar Sistema

```bash
# Detener (preserva datos)
docker-compose stop

# Eliminar contenedores (preserva datos)
docker-compose down

# Eliminar TODO (¡cuidado!)
docker-compose down -v
```

## Archivos

- `docker-compose.yml` - Configuración servicios
- `backend/serial_reader.py` - Backend lector serial
- `backend/Dockerfile.reader` - Imagen lector serial
- `arduino_example.ino` - Ejemplo código Arduino
- `.env` - Variables de configuración
- `sql_scripts/init.sql` - Crear tablas en BD
- `sql_scripts/seed.sql` - Datos de prueba para la BD
- `backend/requirements.txt` - Dependencias de Python
- `grafana/provisioning/` - Auto-configuración de Data Sources y Dashboards
- `grafana/dashboards/` - Archivos JSON de los dashboards
