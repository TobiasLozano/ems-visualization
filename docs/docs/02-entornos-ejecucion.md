---
sidebar_position: 3
slug: /entornos-ejecucion
title: 3. Entornos de Ejecución
---

# 3. Entornos de Ejecución y Puesta en Marcha

El sistema puede operarse conectándose al **servidor remoto desplegado** (sin instalar dependencias complejas en tu equipo) o desplegando la plataforma completa en tu **entorno local** mediante contenedores Docker.

---

## Opción A: Conexión al Servidor Desplegado en Producción

Si deseas operar el sistema de inmediato para demostraciones, pruebas de campo o visualización remota:

* **Interfaz Web de Telemetría (Flask):** [`https://ems-application-cqwocu-4ebfba-51-222-139-162.sslip.io/`](https://ems-application-cqwocu-4ebfba-51-222-139-162.sslip.io/)
* **Plataforma Grafana:** [`https://ems-application-cqwocu-68ba27-51-222-139-162.sslip.io/?orgId=1`](https://ems-application-cqwocu-68ba27-51-222-139-162.sslip.io/?orgId=1)
  * **Usuario:** `admin`
  * **Contraseña:** `admin`

### ¿Cómo operar con el servidor remoto?
1. Conecta el hardware STM32 a través del conversor USB-TTL a tu PC.
2. Abre la URL del servidor web en tu navegador Chrome o Edge.
3. Presiona **"Conectar Puerto Serial"** para transmitir los datos locales hacia la base de datos del servidor.
4. Abre la pestaña de Grafana en el servidor para observar los gráficos actualizándose en tiempo real.

---

## Opción B: Despliegue en Entorno Local (Docker Compose)

Si vas a realizar pruebas de laboratorio sin conexión a Internet o deseas modificar el código de la aplicación, puedes desplegar la infraestructura en tu propia máquina.

### 1. Requisitos Previos
* **Docker Engine:** Versión 20.10+ ([Instalar Docker](https://docs.docker.com/get-docker/))
* **Docker Compose:** Versión 2.0+
* **Navegador Web:** Google Chrome, Microsoft Edge u Opera

### 2. Configuración y Arranque Rápido

Clona el repositorio y sitúate en la raíz del proyecto:
```bash
git clone https://github.com/TobiasLozano/ems-visualization.git
cd ems-visualization
```

Copia la plantilla de variables de entorno:
```bash
cp .env.example .env
```

Contenido por defecto de `.env`:
```ini
DB_HOST=postgres
DB_PORT=5432
DB_NAME=sensors_db
DB_USER=postgres
DB_PASSWORD=postgres
SERIAL_PORT=/dev/ttyUSB0
SERIAL_BAUDRATE=9600
```

Construye y levanta los microservicios en segundo plano:
```bash
docker compose up -d --build
```

### 3. Verificación de Microservicios

Comprueba el estado de los contenedores:
```bash
docker compose ps
```

Deberás observar los contenedores en ejecución:
* `solar_postgres`: Motor PostgreSQL 15, puerto `5433:5432` (dev) / `5432:5432` (prod), estado `Up (healthy)`.
* `solar_webapp`: Servidor Flask REST, puerto `5000:5000`, estado `Up`.
* `solar_grafana`: Servidor Grafana OSS, puerto `3001:3000`, estado `Up`.
* `solar_docs`: Manual interactivo Docusaurus, puerto `8080:80`, estado `Up`.

### 4. Permisos de Puerto Serial en Linux (Solución de Error)

En sistemas operativos Linux (Ubuntu, Debian, Fedora), el acceso a puertos USB seriales (`/dev/ttyUSB0` o `/dev/ttyACM0`) está restringido al grupo `dialout`. Si recibes un error de permisos:

```bash
# Añadir tu usuario al grupo dialout
sudo usermod -a -G dialout $USER

# Alternativa rápida temporal:
sudo chmod 666 /dev/ttyUSB0
```
*Nota: Cierra e inicia sesión de nuevo para que los cambios de grupo surtan efecto.*

:::info[Alternativas de Ingesta en Local: Web vs. Terminal]
Para interactuar con el sistema en tu entorno local cuentas con dos alternativas de adquisición:
* **Pruebas mediante la Aplicación Web (Método de Adquisición 1 - Web Serial API):** Con los contenedores en marcha, puedes realizar pruebas y operar el sistema directamente abriendo la interfaz web en [`http://localhost:5000`](http://localhost:5000) desde Google Chrome o Microsoft Edge, conectando el puerto serial directamente desde el navegador de forma interactiva y sin dependencias de terminal.
* **Ejecución Directa desde la Terminal (`pyserial` - Script Standalone):** El uso de `pyserial` mediante el script `serial_reader.py` se puede usar cuando se busca ejecutar directamente desde la terminal en caso de que se quiera ejecutar sin dependencias externas del navegador, en entornos sin interfaz gráfica (headless) o para procesos automatizados en segundo plano.
:::

### 5. Operación del script de adquisición en entorno local

Con el entorno local activo, el script `serial_reader.py` (Modo B del capítulo 2) captura la trama del STM32 y la inserta directamente en PostgreSQL, sin necesidad de abrir el navegador.

**Paso 1. Levantar la base de datos y Grafana**
```bash
docker compose up -d postgres grafana
```

**Paso 2. Ajustar el `.env` para ejecutar el script fuera de Docker**

El script corre en tu máquina y no dentro de la red de Docker, por lo que debe apuntar al puerto que el contenedor publica en el equipo:
```ini
DB_HOST=localhost
DB_PORT=5433
```

**Paso 3. Instalar dependencias y ejecutar el lector serial**
```bash
pip install pyserial psycopg2-binary python-dotenv
python backend/serial_reader.py
```

Al iniciar, la consola confirma la conexión serial y de base de datos, y luego imprime cada trama recibida:
```text
2026-05-26 17:21:44,462 - INFO - Serial: /dev/ttyUSB0 @ 9600 baud
2026-05-26 17:21:44,490 - INFO - BD conectada
2026-05-26 17:21:44,515 - INFO - Tablas creadas
2026-05-26 17:21:45,102 - INFO - datos: {'temp_panel': 31.2, 'temp_bat': 28.5, 'volt_panel': 18.4, 'amp_panel': 2.3, 'volt_bat': 12.4, 'volt_bat_f': 12.43, 'amp_bat': 1.1, 'volt_load': 24.0, 'volt_load_f': 23.97, 'amp_load': 0.9, 'boost_en': 1, 'boost_pwm': 142, 'mode': 'BOOST_CV'}
```

Desde ese momento puedes abrir Grafana en [`http://localhost:3001`](http://localhost:3001) y ver las curvas actualizarse en tiempo real.

---

## URLs de Acceso Local

* **Aplicación Web:** [http://localhost:5000](http://localhost:5000)
* **Grafana:** [http://localhost:3001](http://localhost:3001) (Usuario: `admin` · Contraseña: `admin`)
* **Manual Interactivo (Docs):** [http://localhost:8080](http://localhost:8080)
* **Base de Datos:** `localhost:5433` (dev) / `localhost:5432` (prod) (Base de datos: `sensors_db`)
