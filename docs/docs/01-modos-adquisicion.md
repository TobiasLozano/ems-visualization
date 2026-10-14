---
sidebar_position: 2
slug: /modos-adquisicion
title: 2. Modos de Adquisición y Hardware
---

import Figure from '@site/src/components/Figure';

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
