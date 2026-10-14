---
slug: /documento-completo
title: Manual Consolidado (PDF)
displayed_sidebar: null
---

import Figure from '@site/src/components/Figure';

<div className="manual-pdf-header" style={{ textAlign: 'center', padding: '30px 20px', borderBottom: '2px solid var(--ifm-color-primary)', marginBottom: '32px' }}>
  <div style={{ fontSize: '1.2rem', fontWeight: 800, color: 'var(--ifm-color-primary)', textTransform: 'uppercase', letterSpacing: '1px' }}>
    Manual de Usuario y Operación del Software
  </div>
  <h1 style={{ margin: '12px 0 6px 0', fontSize: '2.2rem' }}>
    Sistema EMS para Microrredes en Corriente Continua
  </h1>
  <div style={{ fontSize: '1rem', color: 'var(--ifm-color-content-secondary)' }}>
    Control Fotovoltaico MPPT · Carga Segura de Baterías CC/CV · Protección Térmica Activa · Monitoreo Web y Grafana
  </div>
  <div style={{ marginTop: '18px', display: 'flex', justifyContent: 'center', gap: '14px', flexWrap: 'wrap' }}>
    <button
      className="button button--primary button--lg"
      style={{ padding: '12px 28px', fontSize: '1.05rem', fontWeight: 'bold', boxShadow: '0 4px 14px rgba(0,0,0,0.15)', cursor: 'pointer' }}
      onClick={() => typeof window !== 'undefined' && window.print()}
    >
      Imprimir / Guardar Manual como PDF
    </button>
  </div>
</div>


# 1. Identificación del Prototipo y Presentación del Sistema

### Manual de Usuario y Operación — Sistema EMS para Microrredes DC

Este documento constituye la guía de operación y ficha técnica descriptiva del prototipo tecnológico desarrollado como Trabajo de Grado en Ingeniería Electrónica en la **Universidad Pontificia Bolivariana (UPB)**, Seccional Montería, en articulación con el grupo de investigación **SEC-LNCE**.



---

## Ficha Técnica del Producto

<div className="ficha-tecnica">

| Campo | Especificación Formal |
|:---|:---|
| **Nombre del Proyecto** | **Energy Management System (EMS) para Microrredes DC: Integración de Generación Solar Fotovoltaica y Almacenamiento en Baterías** |
| **Tipo de Producto** | Prototipo tecnológico funcional de software y hardware embebido (Monitoreo web, control de potencia y telemetría). |
| **Institución Titular** | **Universidad Pontificia Bolivariana (UPB)** — Seccional Montería / Grupo SEC-LNCE. |
| **Instituciones Aliadas** | **Universitat Politècnica de València (UPV)**, España · **Universidad de Guadalajara (UdeG)**, México. |
| **Desarrollador de Software** | **Ing. Tobias Lozano Florez** |
| **Líder del Proyecto** | **MSc. Fabián Alonso Lara Vargas** (UPB Seccional Montería / SEC-LNCE) |
| **Destino de Registro** | **Dirección Nacional de Derechos de Autor (DNDA)** y Banco de Prototipos UPB Montería. |
| **Servidor en Producción** | [`https://tobias-ems-o633sc-4dd19b-51-222-139-162.sslip.io/`](https://tobias-ems-o633sc-4dd19b-51-222-139-162.sslip.io/) |
| **Repositorio Oficial** | [GitHub: TobiasLozano/ems-visualization](https://github.com/TobiasLozano/ems-visualization) |

</div>

---

## Equipo Técnico-Científico del Proyecto

La concepción, desarrollo e implementación del EMS articula capacidades interinstitucionales e internacionales:

| Investigador / Ingeniero | Institución | Rol en el Proyecto | Función Principal |
|:---|:---|:---|:---|
| **Fabián Alonso Lara Vargas** | UPB Seccional Montería / SEC-LNCE | **Líder del Proyecto** | Dirección técnica, integración FV-BESS y coordinación científica. |
| **Dr. Carlos Vargas Salgado** | Universitat Politècnica de València (UPV) | **Co-investigador Externo** | Asesoría en gestión energética y validación del EMS. |
| **Dr. Jesús Águila Téllez** | Universidad de Guadalajara (UdeG) | **Co-investigador Externo** | Soporte en control de microrredes y análisis dinámico de resultados. |
| **Ing. Tobias Lozano Florez** | Externo (Desarrollador) | **Desarrollador de Software** | Diseño, arquitectura, despliegue y mantenimiento de la plataforma EMS. |
| **Ing. Miguel Ángel Ortiz Padilla** | UPB Seccional Montería | **Co-investigador** | Implementación de firmware STM32 y ejecución de pruebas de campo. |
| **Ing. Daniel Espinosa Corrales** | UPB Seccional Montería | **Co-investigador** | Modelado y validación experimental de la microrred DC. |

---

## Resumen Ejecutivo y Especificaciones del Sistema

### Propósito del EMS
Desarrollar e implementar un sistema automático de gestión energética (**Energy Management System - EMS**) aplicado a una microrred fotovoltaica en corriente continua (DC) a pequeña escala. El prototipo maximiza la extracción de energía disponible del panel solar mediante algoritmos de Seguimiento del Punto de Máxima Potencia (**MPPT**), gestiona la carga y respaldo seguro de una batería de iones de litio mediante perfiles de Corriente Constante / Voltaje Constante (**CC/CV**), integra protección térmica activa y proporciona observabilidad en tiempo real a través de telemetría y contenedores Docker.

### Entradas del Sistema
1. **Generación Fotovoltaica:** Módulo solar monocristalino de **50 W** (Vmp = 17.0 V, Imp = 2.94 A, Voc = 22.7 V).
2. **Almacenamiento Electroquímico:** Pack de baterías de iones de litio en configuración **3S2P** (11.1 V nominal, 12.6 V carga plena, capacidad 5200 mAh).
3. **Instrumentación Sensorial Distribuida:**
   - **Tensión:** 3 módulos **INA226** vía bus digital I2C (resolución de 1.25 mV).
   - **Corriente:** 3 sensores de efecto Hall **ACS712** mediante canales ADC analógicos de 12 bits.
   - **Temperatura:** Sensor digital **MAX6675** con termocupla tipo K vía bus SPI (resolución de 0.25 °C).

### Procesamiento y Control
- **Unidad de Control:** Microcontrolador de 32 bits **STM32F103C8T6 (ARM Cortex-M3 @ 72 MHz)**.
- **Algoritmo MPPT:** Perturbar y Observar (**P&O Modificado**) operando a frecuencia de conmutación de **80 kHz** con limitador activo de sobretensión en bus a **24.0 V**.
- **Algoritmo de Almacenamiento:** Control CC/CV con etapa de Corriente Constante a **1.0 A**, Voltaje Constante a **12.60 V** y corte de corriente residual (**I ≤ 0.35 A**).
- **Máquina de Estados de Potencia:** Conmutación entre modos `PANEL_MPPT`, `BAT_CV` y `LOW_BAT` con tiempo muerto de seguridad (*deadtime*) de **500 ms** para suprimir conducción cruzada.
- **Supervisión Térmica:** Interrupción automática de la batería si **T ≥ 60 °C** con histéresis de reconexión a **55 °C**.

### Salidas del Sistema
1. **Potencia Regulada:** Bus central de corriente continua a **24.0 V** suministrando potencia continua a carga resistiva calibrada (**15 Ω**).
2. **Telemetría Digital:** Transmisión serial continua de tramas formateadas en JSON vía interfaz UART a **9600 baud** (1 muestra/segundo).
3. **Almacenamiento Histórico:** Base de datos relacional **PostgreSQL 15** en contenedor Dockerizado con indexación temporal.
4. **Visualización Gráfica:** Cuadro de mando en tiempo real autoprovisionado en **Grafana** y servidor web de exportación en **Flask**.

### Usuarios Destinatarios
- Investigadores y docentes del área de energías renovables y microrredes inteligentes.
- Estudiantes de ingeniería para prácticas de electrónica de potencia y sistemas embebidos.
- Operadores e instaladores de microrredes aisladas que requieran telemetría e inspección de bancos de baterías de litio.

---

## Montaje Físico del Prototipo

<div className="figure-container">
  <img 
    src="/hardware/F-07_montaje_completo_banco.jpg" 
    alt="Montaje Físico Completo del Prototipo EMS" 
  />
  <div className="figure-caption">
    <strong>Figura 1.1:</strong> Vista general del prototipo físico integrado sobre el banco de pruebas: tarjeta de control STM32, módulos convertidores DC-DC, pack de baterías de litio 3S2P y cableado de instrumentación.
  </div>
</div>

# 2. Modos de Adquisición y Conexión de Hardware

Para capturar la telemetría del sistema de potencia, el microcontrolador **STM32F103C8T6 (Blue Pill)** transmite continuamente los valores medidos a través de su puerto serial **UART1** a **9600 baudios**, en forma de una trama JSON por segundo (**1 Hz**).

---

## Conexión Física del Conversor USB-TTL

Antes de iniciar cualquier software, conecta el conversor USB-TTL a tu ordenador y a los pines de comunicación de la tarjeta de control:

| Pin Conversor USB-TTL | Pin STM32 (Blue Pill) | Función |
|:---:|:---:|:---|
| **RXD** | **PA9 (TX USART1)** | Recepción de datos en el host desde el STM32. |
| **TXD** | **PA10 (RX USART1)** | Transmisión de comandos desde el host (opcional). |
| **GND** | **GND** | Masa común obligatoria para referencia de señal. |
| **VCC (5V / 3.3V)** | *No conectar* | La STM32 debe alimentarse de su propia fuente o puerto micro-USB. |

:::warning[Precaución de Cableado en STM32]
Asegúrate de conectar las líneas seriales a **PA9** y **PA10** (puerto USART1 nativo). **No conectar** a `PA0`-`PA2` (entradas analógicas de los sensores ACS712) ni a `PA3` (señal de modulación PWM del convertidor Boost a 80 kHz).
:::

---

## Modo A: Adquisición Web (Web Serial API)

Este es el método recomendado para operación interactiva. Utiliza la interfaz nativa del navegador estándar (Google Chrome, Microsoft Edge u Opera) para comunicarse directamente con el puerto USB sin necesidad de instalar scripts de Python ni drivers serie adicionales.

<Figure
  src="/img/diagramas/D-04_adquisicion_modo_web_serial.png"
  title="Cadena de Adquisición mediante Web Serial API y Flask"
  figureNumber="2.1"
  description="Flujo: STM32 → UART → Conversor USB-TTL → Web Serial API (Navegador) → HTTP POST → Flask API REST → Base de datos PostgreSQL → Grafana."
/>

### Características del Modo Web
* **Sin instalación de librerías:** no requiere instalar Python, controladores serie adicionales ni entornos virtuales.
* **Adquisición interactiva:** la captura y la ingesta se ejecutan directamente en la pestaña del navegador mediante Web Serial API.
* **Transmisión transparente:** el navegador decodifica las líneas del puerto serie y envía cada trama JSON al backend Flask mediante peticiones HTTP `POST /api/serial-data`.

:::info[Requisito de Seguridad del Navegador]
La Web Serial API exige un **Contexto Seguro**. Esto significa que solo opera a través de `http://localhost` o dominios protegidos con certificado SSL (`https://`). Si accedes remotamente por IP sin SSL, usa el Modo B o configura un túnel seguro.
:::

El procedimiento paso a paso para operar la interfaz web, autorizar el puerto USB y verificar la terminal de telemetría se describe en el **Capítulo 4: Monitoreo en Grafana y Uso del Panel Web (Parte 2)**.

---

## Modo B: Adquisición por Script Standalone (`serial_reader.py`)

Este es el **método nativo y recomendado para el Entorno Local**. Cuando levantas el sistema en tu PC mediante Docker Compose, la base de datos PostgreSQL (`localhost:5433` / `5432`) y Grafana (`localhost:3001`) se ejecutan de forma local y continua. El script `serial_reader.py` corre localmente en tu máquina conectándose directamente al hardware para una ingesta totalmente desatendida.

<Figure
  src="/img/diagramas/D-04_adquisicion_modo_script_python.png"
  title="Cadena de Adquisición Directa en Entorno Local (Script Python + Docker)"
  figureNumber="2.2"
  description="Flujo en entorno local: STM32 → USB-TTL → serial_reader.py → Inserción SQL directa (psycopg2) → PostgreSQL local (Docker) → Grafana local."
/>

### Características del Modo B
* **Operación desatendida y continua:** no requiere mantener abierta una pestaña del navegador ni lidiar con permisos de Web Serial API.
* **Inserción SQL directa:** el script escribe en las 8 tablas de series temporales de PostgreSQL con `psycopg2`.
* **Auto-reconexión y sanitización:** descarta ruido de línea, caracteres nulos (`\x00`) y mensajes de arranque del STM32 antes de interpretar la trama. Si el cable USB se desconecta, reintenta la conexión automáticamente.

La instalación de dependencias y los comandos de arranque del script se describen en el capítulo 3, sección **Operación del script de adquisición en entorno local**.

---

## Formato de la Trama Serial JSON

Independientemente del modo de adquisición, el STM32 envía en cada ciclo exactamente esta estructura:

```json
{"temp_panel":31.2,"temp_bat":28.5,"volt_panel":18.4,"amp_panel":2.3,"volt_bat":12.4,"volt_bat_f":12.43,"amp_bat":1.1,"volt_load":24.0,"volt_load_f":23.97,"amp_load":0.9,"boost_en":1,"boost_pwm":142,"mode":"BOOST_CV"}
```

| Campo | Descripción | Unidad | Se guarda en la base de datos |
|:---|:---|:---:|:---:|
| `temp_panel` | Temperatura del panel solar. | °C | Sí |
| `temp_bat` | Temperatura de la batería (termocupla). | °C | Sí |
| `volt_panel` | Tensión en bornes del panel solar. | V | Sí |
| `amp_panel` | Corriente del panel solar. | A | Sí |
| `volt_bat` | Tensión del banco de baterías (valor instantáneo). | V | Sí |
| `volt_bat_f` | Tensión de la batería filtrada (media móvil). | V | No |
| `amp_bat` | Corriente de la batería. Positiva al cargar, negativa al descargar. | A | Sí |
| `volt_load` | Tensión en la carga (bus DC de 24 V, valor instantáneo). | V | Sí |
| `volt_load_f` | Tensión de la carga filtrada (media móvil). | V | No |
| `amp_load` | Corriente suministrada a la carga. | A | Sí |
| `boost_en` | Estado del convertidor Boost: `1` habilitado, `0` deshabilitado. | 0 / 1 | No |
| `boost_pwm` | Ciclo de trabajo aplicado al PWM del Boost. | - | No |
| `mode` | Modo de operación: `BOOST_CV` (Boost habilitado) o `LOW_BAT` (Boost deshabilitado). | texto | No |

Consideraciones sobre los valores:
* Las magnitudes instantáneas se redondean a 1 decimal y las filtradas (`_f`) a 2 decimales.
* Si el sensor de temperatura no entrega lectura válida, el campo se envía como `0.0`.
* El Panel Web y el script `serial_reader.py` guardan únicamente las 8 magnitudes físicas marcadas con **Sí**. El resto de campos solo se visualiza en la terminal de mediciones.

# 3. Entornos de Ejecución y Puesta en Marcha

El sistema puede operarse conectándose al **servidor remoto desplegado** (sin instalar dependencias complejas en tu equipo) o desplegando la plataforma completa en tu **entorno local** mediante contenedores Docker.

---

## Opción A: Conexión al Servidor Desplegado en Producción

Si deseas operar el sistema de inmediato para demostraciones, pruebas de campo o visualización remota:

* **Interfaz Web de Telemetría (Flask):** [`https://tobias-ems-o633sc-4dd19b-51-222-139-162.sslip.io/`](https://tobias-ems-o633sc-4dd19b-51-222-139-162.sslip.io/)
* **Plataforma Grafana:** [`https://tobias-ems-o633sc-9ec290-51-222-139-162.sslip.io/?orgId=1`](https://tobias-ems-o633sc-9ec290-51-222-139-162.sslip.io/?orgId=1)
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

# 4. Monitoreo en Grafana y Exportación desde el Panel Web

La plataforma divide claramente dos funciones operativas: **Grafana** se encarga de la visualización y observabilidad en tiempo real de la microrred, mientras que el **Panel Web (`solar_webapp`)** es el centro de control para la gestión de conexión y la exportación de datos históricos.

---

## Parte 1: Observabilidad y Monitoreo en Tiempo Real (Grafana)

El cuadro de mando en **Grafana OSS** viene preconfigurado con conexión automática a la base de datos PostgreSQL `sensors_db`.

### Acceso al Dashboard
* **Acceso Local:** [http://localhost:3001](http://localhost:3001)
* **Acceso Remoto (Servidor):** [`https://tobias-ems-o633sc-9ec290-51-222-139-162.sslip.io/?orgId=1`](https://tobias-ems-o633sc-9ec290-51-222-139-162.sslip.io/?orgId=1)
* **Credenciales por defecto:** `admin` / `admin`

<Figure
  src="/img/software/S-03_dashboard_grafana_completo.png"
  title="Cuadro de Mando Principal en Grafana para el Sistema EMS"
  figureNumber="4.1"
  description="Panel interactivo en tiempo real con diagrama unifilar de la microrred DC (panel solar, banco de baterías y carga), botón de acceso directo 'Ver datos' hacia el panel web y curvas de potencia."
/>

### Cuadrantes y Métricas de Monitoreo
1. **Panel solar:** tensión (`volt_panel`), corriente (`amp_panel`) y potencia calculada como el producto de ambas. Permite evaluar el punto de máxima potencia extraído por el algoritmo P&O a 80 kHz.
2. **Banco de baterías:** tensión (`volt_bat`), corriente (`amp_bat`) y potencia. Una corriente positiva indica carga bajo perfil CC/CV; una negativa indica respaldo activo hacia el bus central.
3. **Carga y bus DC de 24 V:** tensión (`volt_load`), corriente (`amp_load`) y potencia entregada a la resistencia calibrada de 15 Ω (15 W, 20 W o 25 W según el ensayo).
4. **Temperaturas:** `temp_bat` (celdas de la batería) y `temp_panel` (panel solar). La protección térmica corta el suministro al alcanzar 60 °C en la batería.

:::info[Navegación Temporal en Grafana]
En la esquina superior derecha puedes ajustar la ventana de visualización (`Last 5 minutes`, `Last 1 hour`, etc.) y fijar el auto-refresco en **5s** o **10s** para seguir los ensayos en vivo.
:::

---

## Parte 2: Uso del Panel Web y Exportación de Telemetría

:::info[El Panel Web es la Herramienta de Conexión y Exportación]
El **Panel Web** ([`https://tobias-ems-o633sc-4dd19b-51-222-139-162.sslip.io/`](https://tobias-ems-o633sc-4dd19b-51-222-139-162.sslip.io/) en producción o [http://localhost:5000](http://localhost:5000) en local) es el centro operativo para conectar el puerto serie del STM32 y descargar mediciones históricas. Grafana se reserva exclusivamente para la supervisión gráfica en tiempo real.
:::

<Figure
  src="/img/software/S-01_panel_web_principal.png"
  title="Panel Web del Sistema EMS"
  figureNumber="4.2"
  description="Interfaz principal desarrollada en Flask que integra el módulo de descarga consolidada, tarjeta de conexión serial USB y visualización de telemetría."
/>

### Paso a paso para operar en Modo Web (Conexión Serial USB)

1. Conecta el conversor USB-TTL al puerto USB de tu computadora.
2. Abre tu navegador web compatible (Google Chrome, Microsoft Edge u Opera) y accede a la interfaz:
   - En servidor remoto: [`https://tobias-ems-o633sc-4dd19b-51-222-139-162.sslip.io/`](https://tobias-ems-o633sc-4dd19b-51-222-139-162.sslip.io/)
   - En entorno local: [`http://localhost:5000`](http://localhost:5000)
3. En el bloque de **"Conexión de Medición Serial (USB)"**, haz clic en **"Conectar Dispositivo USB"**.
4. El navegador desplegará la ventana nativa de permisos para seleccionar el puerto serie asignado al conversor USB-TTL conectado al STM32:

<Figure
  src="/img/software/S-01_popup_web_serial.png"
  title="Selección y Autorización de Puerto Serial mediante Web Serial API"
  figureNumber="4.3"
  description="Diálogo nativo de autorización del navegador (Chrome / Edge) seleccionando el dispositivo CDC ACM / USB Serial."
/>

5. Al presionar **Allow** (o **Conectar**), el indicador cambiará a **"CONECTADO"** y la terminal de mediciones comenzará a imprimir las tramas entrantes en tiempo real, almacenándolas de forma automática en PostgreSQL:

<Figure
  src="/img/software/S-01_terminal_mediciones_activa.png"
  title="Terminal de Telemetría Serial en Ejecución Continua"
  figureNumber="4.4"
  description="Ingesta activa de paquetes JSON de voltaje, corriente y temperatura a 9600 baudios."
/>

:::info[Requisito de Seguridad del Navegador]
La Web Serial API exige un **Contexto Seguro**. Esto significa que solo opera a través de `http://localhost` o dominios protegidos con certificado SSL (`https://`). Si accedes remotamente por IP sin SSL, usa el Modo B o configura un túnel seguro.
:::

---

### Procedimiento de Exportación en el Panel Web

<Figure
  src="/img/software/S-02_selector_variable_descarga.png"
  title="Selección de Variable o Paquete Consolidado para Descarga"
  figureNumber="4.5"
  description="Menú interactivo para seleccionar la descarga empaquetada de las 8 tablas en archivo ZIP o una variable individual de la microrred."
/>

#### Paso 1: Selección de Variable o Paquete ZIP
* Selecciona **"Todas las variables (Archivo ZIP consolidado)"** para que el servidor empaquete en un solo archivo `.zip` los datos de las 8 tablas de la microrred (`volt_panel`, `amp_panel`, `temp_panel`, `volt_bat`, `amp_bat`, `temp_bat`, `volt_load`, `amp_load`).
* O bien, selecciona una métrica individual de la lista desplegable.

<Figure
  src="/img/software/S-02_selector_rango_fechas.png"
  title="Selector de Rango de Fecha y Hora con Sincronización de Grafana"
  figureNumber="4.6"
  description="Selector interactivo que incluye accesos directos por duración, calendario personalizado y el botón para pegar rangos desde Grafana."
/>

#### Paso 2: Definir el Rango Temporal
El panel web ofrece tres formas de delimitar el tiempo de las mediciones:
* **Botones de Acceso Rápido:** Haz clic en `5 min`, `15 min`, `1 hora`, `6 horas`, `24 horas`, `7 días` o `Todo el historial` para fijar el intervalo con un solo clic.
* **Calendario Personalizado:** Haz clic en los campos `DESDE` y `HASTA` para abrir el selector de fecha y hora exacta.
* **Sincronización Inteligente ("Pegar rango desde Grafana"):**
  1. Si estás observando una anomalía o curva específica en Grafana, copia la URL del navegador de Grafana (que contiene parámetros como `?from=now-1h&to=now` o marcas ISO).
  2. En el panel web, presiona **"Pegar rango desde Grafana"** y pega el texto copiado.
  3. El panel web traducirá automáticamente los parámetros de Grafana a fechas exactas y las cargará en el formulario.

#### Paso 3: Descargar en CSV o JSON
* **Exportar en CSV:** Genera archivos tabulares delimitados por coma, ideales para importar inmediatamente en Microsoft Excel, MATLAB, Python (Pandas) o R.
* **Exportar en JSON:** Genera estructuras de datos ligeras con marcas de tiempo y valor, preparadas para integración web o procesamiento en servicios backend.

# 5. Validación Experimental

Este capítulo describe los cinco casos de prueba ejecutados sobre el prototipo físico del EMS. Cada caso incluye su objetivo, las condiciones del ensayo, el resultado observado, el rango temporal exacto en el que quedó registrado y la captura de Grafana correspondiente.

**Condiciones comunes a todos los ensayos**

* Bus DC central regulado a 24 V.
* Carga resistiva calibrada de 15 Ω.
* Banco de baterías Li-Ion 3S2P.
* Generación con panel fotovoltaico de 50 W en exteriores (Montería, Córdoba), salvo en la prueba de carga de batería, que usó una fuente de laboratorio fija de 24 V.
* Telemetría del STM32 registrada en PostgreSQL a 1 muestra por segundo.

## Cómo consultar los datos de un caso

Todas las mediciones de cada ensayo permanecen almacenadas en la base de datos. Para volver a verlas o descargarlas:

1. Abre el Panel Web (ver capítulo 4).
2. En el módulo de descarga, copia el bloque **Rango para copiar** del caso que te interesa.
3. Presiona **Pegar rango desde Grafana**. Los campos `DESDE` y `HASTA` se completan automáticamente. También puedes escribir esos mismos valores a mano.
4. Elige la variable (o el paquete ZIP con las 8 tablas) y descarga en CSV o JSON.

---

## Caso 1. Carga de batería con perfil CC/CV

**Objetivo.** Comprobar que el convertidor Buck carga el banco de baterías con corriente constante y luego con tensión constante.

**Condiciones.** Fuente de laboratorio fija de 24 V como entrada. Convertidor Buck a 50 kHz gestionado por el STM32.

**Resultado observado.**
* Etapa de corriente constante (CC): la corriente de carga se regula en **1 A** mientras la tensión de la batería sube.
* Etapa de tensión constante (CV): la tensión se mantiene en **12.6 V** y la corriente decae gradualmente.
* Eficiencia experimental de la etapa: **53.6 %**.

**Rango para copiar** (duración: 29 min 34 s)

```json
{"from":"2026-05-13T15:54:50","to":"2026-05-13T16:24:24"}
```

<Figure
  src="/img/resultados/Carga_Bateria/carga_1.png"
  title="Corriente y tensión durante la carga CC/CV"
  figureNumber="5.1"
  description="Fase de corriente constante a 1 A y posterior decaimiento al alcanzar la tensión de corte."
  maxWidth="760px"
/>

<Figure
  src="/img/resultados/Carga_Bateria/carga_2.png"
  title="Detalle de estabilización de las curvas"
  figureNumber="5.2"
  description="Transición de CC a CV y meseta de tensión en 12.6 V."
  maxWidth="920px"
/>

---

## Caso 2. Gestión de energía con demanda de 15 W

**Objetivo.** Verificar que el panel solar abastece por sí solo una demanda de 15 W.

**Condiciones.** Carga de 15 Ω con consumo de 15 W y radiación solar suficiente.

**Resultado observado.** El panel cubrió la demanda de forma casi autónoma. La batería intervino de manera mínima durante todo el ensayo.

**Rango para copiar** (duración: 23 min 47 s)

```json
{"from":"2026-05-26T17:21:44","to":"2026-05-26T17:45:31"}
```

<Figure
  src="/img/resultados/15W/prueba_15w.png"
  title="Telemetría del ensayo a 15 W"
  figureNumber="5.3"
  description="El panel solar suministra la carga y la corriente de batería es mínima."
/>

---

## Caso 3. Gestión de energía con demanda de 20 W

**Objetivo.** Comprobar el comportamiento dinámico cuando la irradiancia varía o resulta insuficiente.

**Condiciones.** Carga de 15 Ω con consumo de 20 W y radiación solar variable.

**Resultado observado.** Cuando la generación solar no alcanzó a cubrir la demanda, la batería compensó el déficit. El bus DC se mantuvo estable durante toda la prueba.

**Rango para copiar** (duración: 48 min 02 s)

```json
{"from":"2026-05-25T18:40:53","to":"2026-05-25T19:28:55"}
```

<Figure
  src="/img/resultados/20W/prueba_20w.png"
  title="Telemetría del ensayo a 20 W"
  figureNumber="5.4"
  description="La corriente de batería se ajusta para mantener constante la tensión del bus DC."
/>

---

## Caso 4. Gestión de energía con demanda de 25 W (respaldo BAT_CV)

**Objetivo.** Validar la respuesta sostenida del sistema cuando el panel no alcanza para la demanda.

**Condiciones.** Carga de 15 Ω con consumo de 25 W.

**Resultado observado.** El sistema conmutó de forma automática y sostenida al modo de respaldo de batería (`BAT_CV`). La batería entregó corrientes de descarga de hasta **2.5 A** y la carga mantuvo continuidad de suministro en todo momento.

**Rango para copiar** (duración: 46 min 05 s)

```json
{"from":"2026-05-26T18:17:32","to":"2026-05-26T19:03:37"}
```

<Figure
  src="/img/resultados/25W/prueba_25w.png"
  title="Telemetría del ensayo a 25 W"
  figureNumber="5.5"
  description="Respaldo continuo de la batería con picos de descarga de hasta 2.5 A."
/>

---

## Caso 5. Protección térmica de la batería

**Objetivo.** Validar el disparo por sobretemperatura y el rearme por histéresis.

**Condiciones.** Calentamiento controlado con pistola de calor aplicada directamente sobre el sensor de temperatura de la batería.

**Resultado observado.**
* Al superar **60 °C**, el sistema desactiva de inmediato el suministro (la temperatura se observa en `temp_bat`).
* El sistema permanece bloqueado mientras la temperatura baja.
* Solo reanuda la operación cuando la temperatura desciende por debajo de **55 °C** (histéresis de 5 °C).

**Rango para copiar** (duración: 10 min 46 s)

```json
{"from":"2026-05-25T17:59:14","to":"2026-05-25T18:10:00"}
```

<Figure
  src="/img/resultados/Termica/termica_1.png"
  title="Disparo por sobretemperatura"
  figureNumber="5.6"
  description="Al superar 60 °C el suministro se corta de inmediato."
  maxWidth="760px"
/>

<Figure
  src="/img/resultados/Termica/termica_2.png"
  title="Enfriamiento y restablecimiento por histéresis"
  figureNumber="5.7"
  description="El sistema se reactiva únicamente tras descender por debajo de 55 °C."
  maxWidth="920px"
/>

# 6. Diagnóstico y Solución de Problemas (Troubleshooting)

Guía rápida para diagnosticar y solucionar los problemas operativos más frecuentes al conectar el hardware o desplegar la plataforma.

---

## 1. Problemas de Comunicación Serial

### Síntoma: "Permission denied: '/dev/ttyUSB0'" en Linux
* **Causa:** El usuario del sistema operativo no pertenece al grupo `dialout`, el cual tiene permisos de lectura y escritura sobre los dispositivos serie.
* **Solución:**
  ```bash
  sudo usermod -a -G dialout $USER
  ```
  Cierra e inicia sesión de nuevo en tu sistema operativo para que el cambio de grupo surta efecto. Como solución temporal inmediata, puedes ejecutar:
  ```bash
  sudo chmod 666 /dev/ttyUSB0
  ```

### Síntoma: El botón "Conectar Puerto Serial" no abre la ventana emergente
* **Causa:** Estás accediendo a la aplicación web a través de una dirección IP no segura (ej. `http://192.168.1.50:5000`). La **Web Serial API** requiere obligatoriamente un **Contexto Seguro**.
* **Solución:**
  1. Accede únicamente mediante **`http://localhost:5000`** en tu máquina local.
  2. Si necesitas acceder remotamente sin certificado SSL, utiliza el **Modo B** (`python backend/serial_reader.py`) directamente en el equipo que tiene conectado el USB.

### Síntoma: Se reciben caracteres extraños o no se detecta el formato JSON
* **Causa:** Desajuste en la velocidad en baudios o falta de referencia de masa común.
* **Solución:**
  1. Verifica que la velocidad del puerto esté configurada exactamente en **`9600`** baudios.
  2. Asegúrate de que el pin **GND** del conversor USB-TTL esté firmemente conectado a la masa (**GND**) del STM32. Sin masa común, la señal serial se corrompe por ruido eléctrico.

---

## 2. Problemas con Contenedores Docker

### Síntoma: "Bind for 0.0.0.0:5432 failed: port is already allocated"
* **Causa:** Ya tienes una instancia local de PostgreSQL, Grafana o un servidor web ejecutándose en tu equipo en los mismos puertos.
* **Solución:**
  Identifica y detén el servicio local en conflicto:
  ```bash
  # En Linux/macOS para liberar el puerto 5432 de PostgreSQL local
  sudo systemctl stop postgresql
  ```
  O modifica los puertos en el archivo `.env` antes de levantar Docker Compose.

### Síntoma: Uno de los contenedores se detiene inesperadamente
* **Solución:** Inspecciona los registros en tiempo real del contenedor afectado:
  ```bash
  # Ver logs del backend Flask
  docker compose logs -f solar_webapp

  # Ver logs de la base de datos
  docker compose logs -f solar_postgres
  ```

---

## 3. Problemas con Grafana y Base de Datos

### Síntoma: Los paneles de Grafana muestran "No Data"
* **Causa:** Los microservicios están activos, pero el proceso de adquisición (Modo Web o Modo Script) no está transmitiendo datos a la base de datos.
* **Solución:**
  1. Comprueba si se están insertando filas en PostgreSQL:
     ```bash
     docker exec -it solar_postgres psql -U postgres -d sensors_db -c "SELECT COUNT(*) FROM volt_panel;"
     ```
  2. Si el contador no aumenta, verifica que hayas presionado el botón **"Conectar Puerto Serial"** en la Web App o que el script `serial_reader.py` esté corriendo y reportando tramas en la consola.
  3. En Grafana, revisa el selector de rango de tiempo superior derecho y selecciona **"Last 5 minutes"** con auto-refresco en **5s**.

### Síntoma: "Datasource connection error"
* **Solución:**
  Verifica que el contenedor de base de datos esté saludable:
  ```bash
  docker compose ps
  ```
  Si `solar_postgres` no está `healthy`, reinicia el servicio con:
  ```bash
  docker compose restart solar_postgres
  ```
