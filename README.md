# Sistema de Control de Energía Fotovoltaica con Algoritmos MPPT para Carga de Baterías de Litio, Protección Térmica y Suministro a Carga Regulada

[![Docker Compose](https://img.shields.io/badge/Docker%20Compose-v2.0+-blue?logo=docker&logoColor=white)](https://www.docker.com/)
[![PostgreSQL](https://img.shields.io/badge/PostgreSQL-15-336791?logo=postgresql&logoColor=white)](https://www.postgresql.org/)
[![Grafana](https://img.shields.io/badge/Grafana-OSS-F46800?logo=grafana&logoColor=white)](https://grafana.com/)
[![Flask](https://img.shields.io/badge/Flask-API%20REST-000000?logo=flask&logoColor=white)](https://flask.palletsprojects.com/)
[![STM32](https://img.shields.io/badge/Firmware-STM32%20(Blue%20Pill)-03234B?logo=stmicroelectronics&logoColor=white)](https://www.st.com/)
[![License: Academic](https://img.shields.io/badge/License-Propiedad%20UPB-red.svg)](https://www.upb.edu.co/)

**Energy Management System (EMS) para Microrredes DC con Integración FV-BESS**  
Plataforma tecnológica integral de hardware embebido, circuitos de conmutación de potencia, adquisición continua de telemetría y observabilidad en tiempo real.

---

## Ficha de Registro del Prototipo Tecnológico

| Campo | Especificación Formal |
|:---|:---|
| **Nombre del Proyecto** | **Energy Management System (EMS) para Microrredes DC: Integración de Generación Solar Fotovoltaica y Almacenamiento en Baterías** |
| **Código Institucional** | `EMS-UPB-2026-V1` |
| **Tipo de Producto** | Prototipo funcional de software, firmware embebido y circuitos de potencia. |
| **Versión** | `v1.0.0 (Release Académico / Operativo)` |
| **Entidad Titular** | **Universidad Pontificia Bolivariana (UPB)** — Seccional Montería / Grupo SEC-LNCE |
| **Instituciones Aliadas** | **Universitat Politècnica de València (UPV)**, España · **Universidad de Guadalajara (UdeG)**, México |
| **Desarrollador de Software** | **Ing. Tobias Lozano Florez** |
| **Líder del Proyecto** | **MSc. Fabián Alonso Lara Vargas** (UPB Seccional Montería / SEC-LNCE) |
| **Co-investigadores** | Dr. Carlos Vargas Salgado (UPV) · Dr. Jesús Águila Téllez (UdeG) · Ing. Miguel Ortiz (UPB) · Ing. Daniel Espinosa (UPB) |
| **Panel Web (Producción)** | [`https://ems-application-cqwocu-4ebfba-51-222-139-162.sslip.io/`](https://ems-application-cqwocu-4ebfba-51-222-139-162.sslip.io/) |
| **Dashboard de Grafana** | [`https://ems-application-cqwocu-68ba27-51-222-139-162.sslip.io/?orgId=1`](https://ems-application-cqwocu-68ba27-51-222-139-162.sslip.io/?orgId=1) |
| **Manual de Usuario Web** | [`https://ems-application-cqwocu-f25d6d-51-222-139-162.sslip.io/`](https://ems-application-cqwocu-f25d6d-51-222-139-162.sslip.io/) |

---

## Descripción General del Sistema

El sistema resuelve de manera unificada dos retos críticos en microrredes aisladas en corriente continua:
1. **Naturaleza no lineal del generador fotovoltaico:** Extrae en tiempo real la máxima potencia disponible de un panel solar de **50 W** mediante un algoritmo Perturbar y Observar (**P&O Modificado**) acoplado a un convertidor elevador Boost conmutado a **80 kHz**.
2. **Fragilidad electroquímica y térmica de baterías de litio:** Administra el almacenamiento seguro en un banco de celdas Li-Ion **3S2P** (11.1 V nominal, 12.6 V máx, 5200 mAh) mediante regulación estricta **Corriente Constante / Voltaje Constante (CC/CV)** a **50 kHz** con **protección térmica activa** ante sobretemperatura ($T \ge 60\,^\circ\text{C}$).

Toda la energía converge en un **Bus DC central regulado a 24 V nominales**, suministrando potencia ininterrumpida a una carga resistiva calibrada de 15 $\Omega$ con ensayos a 15 W, 20 W y 25 W.

---

## Arquitectura de Hardware y Potencia

```text
  ┌─────────────────┐       ┌──────────────────┐
  │ Panel Solar PV  │──────►│ Convertidor Boost│──────┐
  │ 50W (17V-22.7V) │       │ PWM @ 80 kHz     │      │
  └─────────────────┘       └──────────────────┘      │
          │                           │               │
     [INA + ACS]                 [Control MPPT]       ▼
                                            ═════════════════════════ BUS DC 24V
                                                      ▲               ▲
  ┌─────────────────┐       ┌──────────────────┐      │               │
  │ Batería Li-Ion  │◄─────►│ Conv. Buck-Boost │◄─────┘               │
  │ 3S2P (11.1-12.6)│       │ Bidireccional    │                      │
  └─────────────────┘       │ 50 kHz (CC/CV)   │                      │
          │                 └──────────────────┘                      │
     [INA+ACS+MAX]                                            ┌──────────────────┐
                                                              │ Carga Calibrada  │
                                                              │ 15 Ω (15W - 25W) │
                                                              └──────────────────┘
```

### Especificaciones de Circuitos
* **Bus DC Central:** 24.0 V nominal ($\pm 0.5\,\text{V}$ en régimen permanente).
* **Convertidor Boost (Lado Panel):** Eleva de 12–18.5 V a 24 V mediante modulación PWM en pin `PA3` a **80 kHz**, conducido por gate driver **IR2110** y MOSFET **IRFZ44N**.
* **Convertidor Buck-Boost Bidireccional (Lado Batería):** 
  * *Modo Buck (Descendente):* Carga CC/CV hacia la batería desde el bus de 24 V a **50 kHz** (pin `PA6`).
  * *Modo Boost (Ascendente):* Respaldo dinámico hacia el bus de 24 V cuando la radiación solar decae.
* **Instrumentación Distribuida:**
  * Corriente y Voltaje: Sensores digitales **INA226** (I2C) y sensores analógicos de efecto Hall **ACS712-05B**.
  * Temperatura: Termocupla tipo K acoplada a convertidor SPI **MAX6675** con resolución de 0.25 °C.
* **Fabricación de Hardware:** Tarjeta de control PCB diseñada en KiCad a dos capas con plano de masa en cobre; etapa de potencia ensamblada con baquela reforzada para soportar hasta 3 A continuos.

---

## Algoritmos de Control y Firmware Embebido

El firmware está programado para el microcontrolador **STM32F103C8T6 (Blue Pill)**:

### 1. MPPT Perturbar y Observar (P&O) Modificado
Calcula la potencia instantánea $P[k] = V[k] \times I[k]$ y compara con el ciclo previo:
* Si $\Delta P > 0$ y $\Delta V > 0 \implies$ incrementa el ciclo de trabajo del PWM (`duty++`).
* Si $\Delta P < 0$ y $\Delta V > 0 \implies$ decrementa el ciclo de trabajo (`duty--`).
* **Restricción de sobretensión en Bus:** Si $V_{\text{bus}} \ge 24.0\,\text{V}$, el algoritmo congela el ciclo de trabajo para impedir sobretensiones en la carga.

### 2. Algoritmo de Carga de Batería (Perfil CC/CV)
* **Fase Corriente Constante (CC):** Inyecta corriente regulada de **1.0 A** mientras $V_{\text{bat}} < 12.60\,\text{V}$.
* **Fase Voltaje Constante (CV):** Al alcanzar 12.60 V, mantiene la tensión constante mientras la corriente decae gradualmente.
* **Corte Seguro:** Al descender la corriente por debajo de 100 mA, finaliza el ciclo para evitar degradación electroquímica.

### 3. Máquina de Estados Finita (FSM) y Protecciones
* `MODE_PANEL_MPPT` (Modo 1): Fuente principal solar activa con MPPT; convertidor de batería apagado.
* `MODE_BAT_CV` (Modo 2): Batería asume el respaldo dinámico del bus ante caída de radiación solar.
* `MODE_LOW_BAT` (Modo 3): Bloqueo por descarga profunda si $V_{\text{bat}} \le 9.50\,\text{V}$; se reactiva al recuperar $10.20\,\text{V}$.
* **Tiempo Muerto (*Deadtime* = 500 ms):** Toda conmutación apaga temporalmente ambos PWMs para prevenir cortocircuitos entre fuentes.
* **Protección Térmica Activa:** Si la termocupla registra $T \ge 60\,^\circ\text{C}$, se cancelan los ciclos de trabajo de conmutación de inmediato y el sistema no reanuda hasta descender a $\le 55\,^\circ\text{C}$ (histéresis de 5 °C).

### 4. Inventario de Archivos de Firmware Suministrados (`firmware/`)

El repositorio incluye el código fuente completo en lenguaje C++ (Arduino Core para STM32 / STM32Duino) para los diferentes escenarios de validación experimental dentro del directorio [`firmware/`](firmware/):

| Archivo | Escenario / Ensayo | Características Principales |
|:---|:---:|:---|
| [`MPPT - BATERÍA 20W - EMS con protección térmica.ino`](firmware/MPPT%20-%20BATER%C3%8DA%2020W%20-%20EMS%20con%20protecci%C3%B3n%20t%C3%A9rmica.ino) | **20 W (Principal)** | Firmware principal de producción. Integra control MPPT P&O (80 kHz), gestión bidireccional de batería, FSM completa y **protección térmica activa con termocupla MAX6675** vía SPI ($T \ge 60\,^\circ\text{C}$ con histéresis de 5 °C). |
| [`MPPT BAT a 15w.ino`](firmware/MPPT%20BAT%20a%2015w.ino) | **15 W** | Firmware calibrado para ensayo con carga reducida de 15 W. Control MPPT, algoritmo de carga/descarga y reporte serial JSON a 9600 baud. |
| [`mppt bat 25w.ino`](firmware/mppt%20bat%2025w.ino) | **25 W** | Firmware configurado para pruebas de alta exigencia (25 W). Regulación de tensión en bus de 24 V, seguimiento de máxima potencia y balance dinámico con banco de baterías. |
| [`cc cv funcional 1A 12.6v .ino`](firmware/cc%20cv%20funcional%201A%2012.6v%20.ino) | **Validación CC/CV** | Sketch de caracterización y prueba aislada para el algoritmo de carga de batería de iones de litio (corriente constante a 1.0 A, tensión constante a 12.6 V y corte seguro < 100 mA). |

#### Requisitos de Compilación y Flasheo:
* **Entorno:** Arduino IDE o PlatformIO con paquete **STM32 Cores** (*Generic STM32F103C series*).
* **Librerías Requeridas:** `ArduinoJson` (v6.x), `Wire` (I2C para sensores INA226) y `SPI` (para termocupla MAX6675).
* **Parámetros Serial:** `9600 baud`, `8N1`, salida en tramas JSON estructuradas a 1 Hz por el puerto UART1 (`PA9` TX / `PA10` RX).

---

## Arquitectura de Software y Telemetría

La plataforma de software opera en microservicios orquestados mediante **Docker Compose** sobre la red aislada `solar_net`:

```text
[ Microcontrolador STM32 ]
            │ (UART @ 9600 baud, 8N1, 1 Hz)
            ▼
┌────────────────────────────────────────────────────────┐
│ MODOS DE ADQUISICIÓN:                                  │
│   Modo A: Web Serial API (Navegador -> HTTP POST)      │
│   Modo B: Script Standalone (serial_reader.py)         │
└────────────────────────────────────────────────────────┘
            │
            ▼
┌─────────────────────────┐        ┌─────────────────────────┐
│ solar_webapp            │───────►│ solar_postgres          │
│ (Flask REST API :5000)  │        │ (sensors_db :5432)      │
│ - Exportación CSV / ZIP │        │ - 8 tablas temporales   │
└─────────────────────────┘        └─────────────────────────┘
                                                ▲
                                                │ (Lectura SQL indexada)
                                   ┌─────────────────────────┐
                                   │ solar_grafana           │
                                   │ (Dashboard :3001)       │
                                   └─────────────────────────┘
```

### Protocolo de Trama Serial JSON
El microcontrolador emite cada segundo (**1 Hz**) una trama serial compacta delimitada por salto de línea (`\n`):

```json
{"temp_panel":31.2,"temp_bat":28.5,"volt_panel":18.4,"amp_panel":2.3,"volt_bat":12.4,"volt_bat_f":12.43,"amp_bat":1.1,"volt_load":24.0,"volt_load_f":23.97,"amp_load":0.9,"boost_en":1,"boost_pwm":142,"mode":"BOOST_CV"}
```

| Clave | Magnitud Física | Unidad | Tipo de Dato |
|:---:|:---|:---:|:---:|
| `volt_panel` / `amp_panel` | Tensión y Corriente del Panel Solar | V / A | Float |
| `volt_bat` / `amp_bat` | Tensión y Corriente del Banco de Baterías | V / A | Float |
| `volt_bat_f` | Tensión filtrada de batería | V | Float |
| `volt_load` / `amp_load` | Tensión y Corriente en la Carga | V / A | Float |
| `volt_load_f` | Tensión filtrada en la carga | V | Float |
| `temp_panel` / `temp_bat` | Temperatura del Panel y de Batería | °C | Float |
| `boost_en` / `boost_pwm` | Estado de habilitación y valor PWM del Boost | 0/1, 0–255 | Integer |
| `mode` | Modo operativo (`BOOST_CV`, `PANEL_MPPT`, `LOW_BAT`) | Cadena | String |

### Modelo de Datos en PostgreSQL
El sistema persiste la telemetría en **8 tablas independientes**, correspondientes a cada una de las magnitudes físicas monitoreadas:
* `volt_panel`, `amp_panel`, `temp_panel`
* `volt_bat`, `amp_bat`, `temp_bat`
* `volt_load`, `amp_load`

Cada tabla comparte el esquema:
```sql
CREATE TABLE IF NOT EXISTS <nombre_metrica> (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    value FLOAT
);
CREATE INDEX IF NOT EXISTS idx_<nombre_metrica>_timestamp ON <nombre_metrica>(timestamp);
```

**Ventaja Arquitectónica:** Permite a Grafana ejecutar consultas SQL directas (`SELECT timestamp, value FROM <metrica> WHERE $__timeFilter(timestamp)`) sobre el índice sin sobrecarga de joins ni unnesting, garantizando alta velocidad de renderizado y desacoplamiento ante desconexiones de sensores.

---

## Validación Experimental y Rendimiento

Ensayos de validación realizados sobre el hardware real en Montería (Córdoba), con panel de 50 W en exteriores a temperatura ambiente de ~30 °C y carga resistiva calibrada de 15 $\Omega$:

### Casos de Ensayo Registrados
1. **Caso 1 — Carga de batería CC/CV:** 1 A en corriente constante, 12.6 V en tensión constante, duración 29 min 34 s.
2. **Caso 2 — Gestión a 15 W:** Panel fotovoltaico abastece la demanda de forma autónoma con aporte de batería casi nulo.
3. **Caso 3 — Gestión a 20 W:** Respaldo dinámico de batería compensando fluctuaciones de irradiancia solar.
4. **Caso 4 — Gestión a 25 W (BAT_CV):** Conmutación sostenida a modo batería con corrientes de descarga de hasta 2.5 A manteniendo el bus regulado.
5. **Caso 5 — Protección térmica activa:** Disparo inmediato y desconexión al superar 60 °C, rearme automático al descender por debajo de 55 °C.

### Tabla Consolidada de Eficiencia por Etapa

| Etapa del Sistema / Modo de Operación | Eficiencia Experimental (%) | % del Ciclo en Transición (Pérdidas de Conmutación) | Frecuencia de Operación | Observaciones Técnicas |
|:---|:---:|:---:|:---:|:---|
| **Panel Solar a Bus DC (Modo Boost)** | **97.8%** | 7.7% | 80 kHz | Excelente rendimiento en seguimiento MPPT sobre el convertidor elevador. |
| **Batería a Bus DC (Modo Boost)** | **95.8%** | 4.9% | 50 kHz | Inyección de potencia eficiente hacia el bus central de 24 V en modo respaldo. |
| **Bus DC a Batería (Modo Buck - CC/CV)** | **53.6%** | 10.6% | 50 kHz | Limitada por la dinámica del algoritmo CC/CV y disipación en el driver flotante IR2110. |
| **Bus DC a Carga Resistiva Pura (Modo Buck)** | **82.7%** | N/A | 50 kHz | Comprueba que el convertidor rinde eficientemente fuera de la zona no lineal de la batería. |
| **Eficiencia Global de Seguimiento MPPT** | **97.0%** | N/A | 80 kHz | Rastreo efectivo del punto de máxima potencia solar (P&O). |

---

## Guía de Inicio Rápido

### Opción 1: Acceso a la Plataforma en Producción
* **Panel Web de Adquisición y Exportación:** [`https://ems-application-cqwocu-4ebfba-51-222-139-162.sslip.io/`](https://ems-application-cqwocu-4ebfba-51-222-139-162.sslip.io/)
* **Dashboard de Grafana:** [`https://ems-application-cqwocu-68ba27-51-222-139-162.sslip.io/?orgId=1`](https://ems-application-cqwocu-68ba27-51-222-139-162.sslip.io/?orgId=1) (acceso: `admin` / `admin`).
* **Manual de Usuario Web:** [`https://ems-application-cqwocu-f25d6d-51-222-139-162.sslip.io/`](https://ems-application-cqwocu-f25d6d-51-222-139-162.sslip.io/)

### Opción 2: Despliegue en Entorno Local con Docker Compose

```bash
# 1. Clonar el repositorio
git clone https://github.com/TobiasLozano/ems-visualization.git
cd ems-visualization

# 2. Configurar variables de entorno
cp .env.example .env

# 3. Compilar y levantar microservicios en segundo plano
docker compose up -d --build

# 4. Verificar estado de los contenedores
docker compose ps
```

* **Web App Flask:** [http://localhost:5000](http://localhost:5000)
* **Grafana:** [http://localhost:3001](http://localhost:3001) (`admin` / `admin`)
* **PostgreSQL:** `localhost:5433` (mapeo externo) / `5432` (interno de red `solar_net`)
* **Documentación Web:** [http://localhost:8080](http://localhost:8080)

---

## Manual de Usuario y Documentación Técnica

La documentación del proyecto se encuentra estructurada en el portal web interactivo desarrollado en Docusaurus:

### Capítulos del Manual de Usuario
1. **Identificación del Prototipo:** Ficha técnica formal, registro de producto e instituciones participantes.
2. **Modos de Adquisición y Hardware:** Conexión de puertos serie, cableado USB-TTL, permisos en Linux y uso del Modo Web vs Modo Script.
3. **Entornos de Ejecución:** Comparativa entre despliegue en desarrollo (`localhost`) y producción (`sslip.io`), configuración de variables de entorno y mapeo de puertos.
4. **Monitoreo en Grafana y Panel Web:** Navegación por dashboards en tiempo real, widgets interactivos y módulo de exportación de telemetría a CSV / ZIP.
5. **Validación Experimental:** Registro detallado de los 5 casos de prueba con rangos de tiempo listos para copiar y analizar.
6. **Solución de Problemas:** Diagnóstico rápido de errores comunes de conexión serial, permisos y estado de contenedores Docker.

### Compilación y Exportación a PDF

```bash
cd docs
npm install
npm run build         # Compilación de la documentación estática
npm run export-pdf     # Generación automatizada del manual en formato PDF
```

---

## Equipo del Proyecto

* **Líder de Proyecto:** MSc. Fabián Alonso Lara Vargas — *UPB Seccional Montería (Grupo SEC-LNCE)*
* **Desarrollador de Software:** Ing. Tobias Lozano Florez — *Arquitectura de Software y Telemetría*
* **Asesores Externos:**
  * Dr. Carlos Vargas Salgado — *Universitat Politècnica de València (UPV), España*
  * Dr. Jesús Águila Téllez — *Universidad de Guadalajara (UdeG), México*
* **Investigadores:** Ing. Miguel Ortiz Padilla · Ing. Daniel Espinosa Corrales — *UPB Montería*