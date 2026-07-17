# Sistema Solar - Monitoreo en Tiempo Real

Sistema Docker para lectura de datos de sensores vía USB-TTL desde dispositivos Arduino/ESP32 y visualización del histórico en un frontend web.

## Requisitos

- Docker y Docker Compose instalados.
- Dispositivo Arduino o ESP32 con sensores, conectado mediante interfaz USB-TTL.
- Permisos adecuados en el puerto serial del sistema operativo.

## Verificación del puerto serial

```bash
# Listar puertos disponibles
ls /dev/ttyUSB*

# Si aparece /dev/ttyUSB0, el puerto está listo para usarse.
# En caso de detectar un puerto diferente, es necesario actualizar el archivo .env.
```

## Arranque del Sistema (Localmente)

```bash
# Construcción y levantamiento de contenedores
docker-compose up -d

# Verificación del estado de los contenedores
docker-compose ps

# Visualización de logs en tiempo real
docker-compose logs -f serial_reader
```

## Publicación y Ejecución con DockerHub

Para publicar imágenes en DockerHub (con el backup de la base de datos ya incluido) y permitir su ejecución en otros servidores, se debe seguir el siguiente procedimiento:

1. **Configuración del usuario de DockerHub:**
   En el archivo `.env`, se debe agregar la variable `DOCKER_USERNAME` con el usuario correspondiente:
   ```bash
   DOCKER_USERNAME=usuario_dockerhub
   ```

2. **Construcción y subida de imágenes:**
   ```bash
   # Inicio de sesión en DockerHub
   docker login
   
   # Construcción de imágenes utilizando los Dockerfiles
   docker-compose build
   
   # Subida de las imágenes al repositorio en DockerHub
   docker-compose push
   ```

3. **Ejecución en entorno de despliegue:**
   Para desplegar el sistema, únicamente se requieren los archivos `docker-compose.yml` y `.env`. Al ejecutar:
   ```bash
   docker-compose up -d
   ```
   Las imágenes se descargarán automáticamente desde DockerHub, integrando los datos de la base de datos (backup) y la configuración preestablecida de Grafana.

## Acceso a Grafana y Portal Web

- **Portal Web (Sistema EMS):** `http://localhost:5000`
- **Grafana:** `http://localhost:3000`

> Grafana está incluido en el stack para visualizaciones avanzadas, actuando como componente principal para la observación de datos. Tanto la base de datos como los dashboards se autoconfiguran durante el inicio del contenedor.

## Datos Esperados

El dispositivo Arduino/ESP32 debe enviar un objeto JSON cada 5 segundos con el siguiente formato:

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

Estos datos son almacenados en 8 tablas de tipo time-series (una por cada sensor):
- `temp_panel`, `temp_bat` - Mediciones de temperatura en °C
- `volt_panel`, `volt_bat`, `volt_load` - Mediciones de voltaje en V (0-50V)
- `amp_panel`, `amp_bat`, `amp_load` - Mediciones de corriente en A

Cada tabla registra los siguientes campos:
- `timestamp`: fecha y hora de registro automático
- `value`: valor numérico de la lectura

## Modificación de la Configuración

Para cambiar parámetros del sistema, se debe editar el archivo `.env`:
```bash
SERIAL_PORT=/dev/ttyUSB0       # Puerto serial de conexión
SERIAL_BAUDRATE=9600           # Velocidad de transmisión
DB_PASSWORD=postgres           # Contraseña de la base de datos
```

Tras modificar el archivo, es necesario reiniciar el servicio lector:
```bash
docker-compose restart serial_reader
```

## Conexión a la Base de Datos

```bash
# Acceso interactivo a PostgreSQL
docker-compose exec postgres psql -U postgres -d sensors_db

# Consulta de los últimos registros de las tablas
SELECT * FROM temp_panel ORDER BY timestamp DESC LIMIT 10;
SELECT * FROM volt_load ORDER BY timestamp DESC LIMIT 10;
SELECT * FROM amp_load ORDER BY timestamp DESC LIMIT 10;

# Cálculo de promedios y valores extremos por tabla
SELECT AVG(value), MAX(value), MIN(value) FROM temp_panel;
SELECT AVG(value), MAX(value), MIN(value) FROM volt_load;
```

## Solución de Problemas

**Puerto serial no detectado:**
```bash
# Inspección de los logs del kernel
dmesg | tail -20

# Modificación temporal de permisos del puerto
sudo chmod 666 /dev/ttyUSB0
```

**Fallo en el servicio Serial Reader:**
```bash
# Visualización de logs detallados
docker-compose logs serial_reader

# Se debe verificar que la velocidad (baudrate) configurada coincida con la del microcontrolador.
# En caso de error, modificar el archivo .env y reiniciar.
```

**Ausencia de datos en la base de datos:**
1. Verificación del envío de datos desde el Arduino: `cat /dev/ttyUSB0`
2. Revisión del lector serial: `docker-compose logs -f serial_reader`
3. Confirmación de que la cadena JSON recibida contenga los nombres de métricas exactos.

## Detención del Sistema

```bash
# Detención de servicios (preserva los datos)
docker-compose stop

# Eliminación de contenedores (preserva los datos)
docker-compose down

# Eliminación completa, incluyendo volúmenes de datos (¡Acción destructiva!)
docker-compose down -v
```

## Estructura de Archivos

- `docker-compose.yml` - Configuración de servicios locales
- `docker-compose.prod.yml` - Configuración para despliegue en producción
- `backend/serial_reader.py` - Script de backend para lectura del puerto serial
- `backend/web_app.py` - API web Flask para descarga de CSV e información de tablas
- `backend/Dockerfile.reader` - Definición de imagen para lector serial
- `backend/Dockerfile.web` - Definición de imagen para aplicación web
- `backend/Dockerfile.postgres` - Definición de imagen para base de datos
- `.env` - Variables de entorno
- `sql_scripts/init.sql` - Creación de esquema en base de datos
- `sql_scripts/seed.sql` - Datos de prueba iniciales
- `backend/requirements.txt` - Dependencias del entorno Python
- `grafana/provisioning/` - Auto-configuración de fuentes de datos y paneles
- `grafana/dashboards/` - Modelos JSON de los dashboards
- `grafana/Dockerfile.grafana` - Definición de imagen para Grafana

## Fuente DockerHub

https://hub.docker.com/repositories/tobiaslozano