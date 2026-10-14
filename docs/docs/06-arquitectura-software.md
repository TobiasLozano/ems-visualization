---
sidebar_position: 6
slug: /arquitectura-software
title: 6. Arquitectura de Software y Base de Datos
---

import InteractiveDiagram from '@site/src/components/InteractiveDiagram';

# 6. Arquitectura de Software y Base de Datos

El backend del sistema EMS implementa una arquitectura orientada a servicios contenerizados para la ingesta, almacenamiento en series de tiempo y visualización continua de variables eléctricas y térmicas.

---

## 1. Infraestructura de Microservicios con Docker Compose

La plataforma se orquesta mediante Docker Compose sobre una red tipo bridge aislada denominada `solar_net`. Esta configuración aísla la capa de almacenamiento y servicios internos, gestionando la comunicación entre contenedores mediante resolución DNS interna de Docker:

<InteractiveDiagram
  src="/img/diagramas/D-02_arquitectura_docker.svg"
  title="Arquitectura de Servicios Docker Compose y Red solar_net"
  figureNumber="6.1"
  description="Microservicios encapsulados: PostgreSQL (persistencia interna en puerto 5432), Flask Web App (puerto 5000), Grafana (puerto host 3001 / interno 3000) y Docs (puerto host 8080 / interno 80)."
/>

---

## 2. Telemetría y Formato JSON Emitido por el STM32

El firmware del microcontrolador STM32 transmite periódicamente cada segundo (1 Hz) una trama estructurada en formato JSON por el puerto serie UART (`9600 baud, 8N1`):

```json
{"temp_panel":31.2,"temp_bat":28.5,"volt_panel":18.4,"amp_panel":2.3,"volt_bat":12.4,"volt_bat_f":12.43,"amp_bat":1.1,"volt_load":24.0,"volt_load_f":23.97,"amp_load":0.9,"boost_en":1,"boost_pwm":142,"mode":"BOOST_CV"}
```

:::info[Tratamiento de Variables en Backend]
El microcontrolador emite lecturas instantáneas, lecturas filtradas (`_f`) y estados de control (`boost_en`, `boost_pwm`, `mode`). Para la persistencia histórica en base de datos, el backend procesa y almacena las **8 magnitudes físicas fundamentales**: `volt_panel`, `amp_panel`, `temp_panel`, `volt_bat`, `amp_bat`, `temp_bat`, `volt_load` y `amp_load`.
:::

---

## 3. Ingesta Técnica y Pipeline de Sanitización

A nivel de arquitectura de software, la ingesta de telemetría hacia la base de datos se resuelve a través de dos componentes técnicos:

### Endpoint de Ingesta REST (`POST /api/serial-data`)
La aplicación web Flask expone el endpoint REST `/api/serial-data` para recibir los paquetes JSON transmitidos por el cliente del navegador:
- Valida la estructura del JSON y castea los valores a tipos de punto flotante (`float`).
- Ejecuta inserciones parametrizadas individuales en PostgreSQL utilizando un pool de conexiones gestionado por `psycopg2`.
- Retorna confirmación HTTP (`200 OK`) con código JSON para control de flujo en el frontend.

### Sanitización de Trama UART (`serial_reader.py`)
En el modo de adquisición directa por script, transitorios de conmutación PWM del convertidor Boost o reinicios del microcontrolador pueden inyectar bytes espurios o nulos (`\x00`) en el búfer serial del host. El script incorpora una función de filtrado y delimitación que extrae de forma determinista la carga útil JSON antes del parseo:

```python
def extraer_json(self, linea):
    limpia = linea.replace('\x00', '').strip()
    ini = limpia.find('{')
    fin = limpia.rfind('}')
    if ini == -1 or fin == -1 or fin <= ini:
        return None
    return limpia[ini:fin + 1]
```

---

## 4. Modelo de Datos en PostgreSQL

La base de datos relacional PostgreSQL (`sensors_db`) almacena la telemetría en **8 tablas independientes**, donde cada tabla corresponde exclusivamente a una de las variables físicas del prototipo: `volt_panel`, `amp_panel`, `temp_panel`, `volt_bat`, `amp_bat`, `temp_bat`, `volt_load` y `amp_load`.

### Esquema DDL e Indexación Temporal

Cada tabla implementa un esquema idéntico y minimalista optimizado para registros de series de tiempo:

```sql
CREATE TABLE IF NOT EXISTS <nombre_metrica> (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    value FLOAT
);

CREATE INDEX IF NOT EXISTS idx_<nombre_metrica>_timestamp 
    ON <nombre_metrica>(timestamp);
```

### Optimización Arquitectónica para Consultas en Grafana

El sistema está optimizado específicamente para la visualización en Grafana mediante este esquema de tablas independientes:

1. **Consultas Directas sin Operaciones de Desanidado:** Cada panel de Grafana ejecuta una consulta SQL nativa y directa de la forma:
   ```sql
   SELECT timestamp, value 
   FROM volt_panel 
   WHERE $__timeFilter(timestamp) 
   ORDER BY timestamp ASC;
   ```
   Al estar cada magnitud en su propia tabla, no se requieren operaciones computacionalmente costosas como `PIVOT`, filtrado relacional por columna de clave/etiqueta (`WHERE metric = 'volt_panel'`) ni desnormalización en tiempo de ejecución.
2. **Uso Óptimo del Índice de Timestamp:** El planificador de consultas de PostgreSQL resuelve el rango temporal `$__timeFilter(timestamp)` mediante un *Index Scan* sobre `idx_<metrica>_timestamp`, permitiendo a Grafana renderizar paneles con miles de puntos históricos en milisegundos.
3. **Desacoplamiento ante Falla de Transductores:** Si un sensor físico se desconecta o reporta datos no válidos, su tabla correspondiente simplemente suspende la inserción de filas, permitiendo que el resto de las métricas continúe registrándose sin bloqueos de transacción ni valores nulos forzados.

---

## 5. Aprovisionamiento Automático de Grafana 

La instancia de Grafana se inicializa de forma declarativa y desatendida mediante aprovisionamiento automatizado (*Infrastructure as Code*), garantizando portabilidad inmediata entre entornos:

- **Fuente de Datos (`grafana/provisioning/datasources/datasource.yml`):**
  Configura la conexión predeterminada con PostgreSQL apuntando al host interno `postgres:5432`, base de datos `sensors_db`, credenciales parametrizadas y modo SSL deshabilitado para comunicación dentro de la red aislada `solar_net`.
- **Dashboards Declarativos (`grafana/provisioning/dashboards/dashboards.yml`):**
  Aprovisiona el dashboard maestro desde `EMS_Dashboard.json`, cargando automáticamente los paneles de instrumentación, curvas temporales de voltaje y corriente, balances de potencia y estado del convertidor DC-DC sin requerir intervención manual en la interfaz gráfica.
