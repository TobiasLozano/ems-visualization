---
sidebar_position: 4
slug: /monitoreo-grafana
title: 4. Monitoreo en Grafana y Uso del Panel Web
---

import Figure from '@site/src/components/Figure';

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
