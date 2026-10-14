---
sidebar_position: 2
title: Extracción de Datos
---

import Figure from '@site/src/components/Figure';

# Extracción y Procesamiento de Datos

El sistema ofrece dos vías para extraer y procesar los datos almacenados en PostgreSQL: la **interfaz web** y el **script Python** `serial_reader.py`.

## Extracción desde la Interfaz Web

La interfaz web dispone del módulo **"Descarga y Exportación de Datos"**, diseñado para unificar la consulta temporal y permitir la descarga individual o masiva de las variables registradas.

<Figure
  src="/img/software/S-02_selector_variable_descarga.png"
  title="Selección de Variable o Paquete Consolidado para Descarga"
  figureNumber="2.1"
  description="Menú desplegable para elegir entre todas las variables en formato ZIP consolidado o una serie específica (voltaje, corriente o temperatura)."
/>

### Flujo de Exportación en 3 Pasos:

1. **¿Qué datos deseas exportar?:**
   - Selecciona **"Todas las variables (Archivo ZIP consolidado)"** para generar una descarga empaquetada de las 8 tablas.
   - O selecciona una variable individual de la microrred (`volt_panel`, `amp_bat`, `temp_panel`, etc.).

2. **Definir el Rango de Tiempo:**
   - **Accesos directos rápidos:** Presiona `5 min`, `15 min`, `1 hora`, `6 horas`, `24 horas`, `7 días` o `Todo el historial`.
   - **Selector personalizado:** Haz clic en los campos `DESDE` y `HASTA` para abrir el calendario interactivo y especificar fecha y hora exactas.

<Figure
  src="/img/software/S-02_selector_rango_fechas.png"
  title="Selector de Rango de Fecha y Hora Personalizado"
  figureNumber="2.2"
  description="Selector interactivo tipo Grafana para definir el intervalo de inicio y fin de las mediciones."
/>

3. **Sincronización con Grafana ("Pegar rango desde Grafana"):**
   - Si estás inspeccionando una ventana de tiempo en Grafana, haz clic en **"Pegar rango desde Grafana"** en la barra superior del selector.
   - Pega la URL del dashboard o el fragmento de parámetros (ej: `?from=now-6h&to=now` o el rango en ISO) para aplicar las fechas inmediatamente en el formulario.

4. **Descargar Archivo:**
   - Haz clic en **"Exportar en CSV"** para análisis en Excel, MATLAB, Python o Pandas.
   - Haz clic en **"Exportar en JSON"** para integración directa en servicios web o procesamiento estructurado.

---

### API REST

La descarga se realiza internamente mediante el endpoint:

```
GET /api/download/<nombre_tabla>?start=<fecha_inicio>&end=<fecha_fin>
```

**Parámetros:**

| Parámetro | Formato | Ejemplo |
|-----------|---------|---------|
| `start` | `YYYY-MM-DD HH:mm:ss` | `2026-05-13 15:54:50` |
| `end` | `YYYY-MM-DD HH:mm:ss` | `2026-05-13 16:24:24` |

**Ejemplo con `curl`:**

```bash
curl -o volt_panel_data.csv \
  "http://localhost:5000/api/download/volt_panel?start=2026-05-13%2015:54:50&end=2026-05-13%2016:24:24"
```

El CSV resultante tiene el formato:

```csv
id,timestamp,value
1,2026-05-13 15:54:51.123456,18.432
2,2026-05-13 15:54:52.234567,18.501
...
```

---

## Extracción Nativa desde Grafana

Grafana ofrece una funcionalidad integrada para exportar los datos de cualquier gráfica directamente a un archivo CSV. Esto es útil si quieres exportar exactamente los mismos datos que estás visualizando, con el mismo rango de tiempo y resolución.

### Paso a Paso en Grafana

1. Abrir el panel de Grafana (`http://<ip-servidor>:3001` o `http://localhost:3001`).
2. Ubica la gráfica de la cual deseas extraer los datos (ej: "Voltaje de Panel").
3. Hacer clic en el **título de la gráfica** para abrir el menú desplegable.
4. Selecciona **Inspect** > **Data**.
5. Se abrirá un panel lateral mostrando los datos en formato tabular.
6. Hacer clic en el botón **Download CSV**.

:::tip[Opciones de Exportación]
Al descargar desde Grafana, es posible habilitar "Data options" para aplicar transformaciones (por ejemplo, exportar la serie de tiempo formateada o extraer solo ciertos campos calculados).
:::

---

## Script Python: `serial_reader.py`

El script `serial_reader.py` es una herramienta standalone que puede ejecutarse fuera de Docker para leer datos del puerto serial e insertarlos directamente en PostgreSQL.

### Arquitectura del Script

```mermaid
flowchart TD
    A["serial_reader.py"] --> B["Abrir puerto serial<br/>(pySerial)"]
    B --> C["Conectar a PostgreSQL<br/>(psycopg2)"]
    C --> D["Crear tablas<br/>si no existen"]
    D --> E{"Leer línea<br/>del serial"}
    E -->|Línea recibida| F["extraer_json()"]
    F --> G["procesar_linea()"]
    G --> H{"¿JSON válido?"}
    H -->|Sí| I["guardar()<br/>INSERT INTO tabla"]
    H -->|No| E
    I --> E
    E -->|Ctrl+C| J["Cerrar conexiones"]
```

### Configuración

El script se configura mediante **variables de entorno** o un archivo `.env`:

```bash
# Conexión Serial
SERIAL_PORT=/dev/ttyUSB0      # Puerto del microcontrolador
SERIAL_BAUDRATE=9600          # Velocidad del puerto serial

# Conexión a PostgreSQL
DB_HOST=localhost              # Host de la base de datos
DB_PORT=5432                   # Puerto de PostgreSQL
DB_NAME=sensors_db             # Nombre de la base de datos
DB_USER=postgres               # Usuario
DB_PASSWORD=postgres           # Contraseña

# Logging
LOG_LEVEL=INFO                 # Nivel de log (DEBUG, INFO, WARNING, ERROR)
```

### Ejecución Standalone

```bash
# Instalar dependencias
pip install pyserial psycopg2-binary python-dotenv

# Ejecutar el script
python backend/serial_reader.py
```

### Funciones Principales

#### `extraer_json(linea)`

Extrae el objeto JSON de una línea serial que puede contener ruido (bytes nulos, texto de arranque del microcontrolador, etc.):

```python
def extraer_json(self, linea):
    """Extrae el objeto JSON de una línea que puede traer ruido serial."""
    if not linea:
        return None

    # Limpiar bytes nulos y espacios
    limpia = linea.replace('\x00', '').strip()
    if not limpia:
        return None

    # Buscar las llaves del JSON
    ini = limpia.find('{')
    fin = limpia.rfind('}')
    if ini == -1 or fin == -1 or fin <= ini:
        return None

    return limpia[ini:fin + 1]
```

:::tip[Robustez]
Esta función es tolerante a líneas como `\x00\x00{"volt_panel":18.5}\r\n` que son comunes al iniciar la comunicación serial con microcontroladores STM32.
:::

#### `procesar_linea(linea)`

Parsea el JSON y filtra solo las **8 métricas válidas**, descartando campos como `mode`, `switching`, `pwm_panel`, etc. que el firmware envía pero que no se almacenan:

```python
metricas_validas = {
    'temp_panel', 'temp_bat', 'volt_panel', 'amp_panel',
    'volt_bat', 'amp_bat', 'volt_load', 'amp_load'
}

for metric, value in datos.items():
    if metric in metricas_validas and isinstance(value, (int, float)):
        self.guardar(metric, float(value), timestamp)
```

#### `guardar(metric, value, timestamp)`

Inserta un registro en la tabla correspondiente:

```python
def guardar(self, metric, value, timestamp):
    self.cursor.execute(
        f"INSERT INTO {metric} (timestamp, value) VALUES (%s, %s)",
        (timestamp, value)
    )
    self.conn.commit()
```

### Formato JSON del Microcontrolador

El STM32 envía una trama JSON por segundo con el siguiente formato:

```json
{
  "volt_panel": 19.234,
  "volt_panel_f": 19.180,
  "amp_panel": 1.456,
  "amp_panel_f": 1.420,
  "pow_panel": 27.236,
  "volt_load": 17.890,
  "volt_load_f": 17.850,
  "amp_load": 1.123,
  "volt_bat": 11.820,
  "volt_bat_f": 11.800,
  "amp_bat": 0.456,
  "amp_bat_f": 0.440,
  "temp_bat": 32.5,
  "temp_bat_ok": 1,
  "mode": "PANEL_MPPT",
  "switching": 0,
  "pwm_panel": 128,
  "pwm_bat": 0
}
```

:::info[Nota]
Solo se almacenan las 8 métricas principales (`volt_panel`, `amp_panel`, `volt_bat`, `amp_bat`, `volt_load`, `amp_load`, `temp_panel`, `temp_bat`). Los campos filtrados (`_f`) y de control (`mode`, `pwm_*`) son usados solo por el firmware para depuración.
:::
