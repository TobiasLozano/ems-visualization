# Sistema EMS — Microgrid DC

El **Energy Management System (EMS)** para Microrredes DC es una plataforma integral de monitoreo, control y adquisición de datos. Permite gestionar de manera inteligente la energía proveniente de un panel fotovoltaico y una batería de respaldo hacia una carga, utilizando un microcontrolador STM32 y una arquitectura de software basada en Docker.

## Características Principales

*   **Firmware de Control (STM32):** Algoritmo MPPT (Perturbar y Observar) para máxima extracción solar y regulación CV para gestión térmica de batería.
*   **Adquisición Web:** Uso de **Web Serial API** para lectura directa de datos del microcontrolador desde el navegador sin necesidad de drivers adicionales.
*   **Backend Completo:** Almacenamiento en **PostgreSQL** y API construida en **Flask**.
*   **Visualización:** Dashboard en tiempo real autoconfigurado con **Grafana**.
*   **Documentación:** Sitio web interactivo generado con **Docusaurus**.

---

## 🚀 Guía de Inicio Rápido (Desarrollo Local)

Para ejecutar el sistema completo de visualización y base de datos en una máquina local:

### 1. Requisitos Previos
*   [Docker](https://docs.docker.com/get-docker/) y [Docker Compose](https://docs.docker.com/compose/install/) instalados.
*   Dispositivo STM32 (o Arduino/ESP32) con el firmware cargado y conectado por USB.

### 2. Levantar los Servicios Docker
Clona el repositorio y ejecuta Docker Compose para construir y levantar los contenedores:

```bash
git clone https://github.com/TobiasLozano/ems-visualization.git
cd ems-visualization

# Construye e inicia PostgreSQL, Flask App y Grafana
docker compose -f docker-compose.yml up -d --build
```

### 3. Acceder al Sistema
Una vez que los contenedores estén corriendo, abre tu navegador web:

*   **Aplicación Web (Gestión y Adquisición):** [http://localhost:5000](http://localhost:5000)
    *   *Usa el botón "Conectar Puerto Serial" para iniciar la lectura de datos desde el hardware.*
*   **Grafana (Visualización):** [http://localhost:3000](http://localhost:3000)
    *   **Usuario:** `admin`
    *   **Contraseña:** `admin`
    *   *(El dashboard "Sistema EMS - Microgrid DC" y la conexión a la base de datos ya están configurados automáticamente).*

---

## 📚 Documentación del Proyecto

El repositorio incluye una documentación exhaustiva (arquitectura, parámetros de firmware, API) construida con Docusaurus.

Para visualizarla localmente:

```bash
# Requiere Node.js instalado
cd docs
npm install
npm run start
```
La documentación estará disponible en `http://localhost:3000` (o el puerto que asigne Node.js, habitualmente el 3000, si Grafana ya lo está usando, Node usará el 3001).

---

## ⚙️ Métodos de Extracción de Datos

El sistema soporta dos formas de leer los datos seriales del microcontrolador:

1.  **Modo Web (Recomendado):** A través de la Web App en `http://localhost:5000` usando Web Serial API. *Nota: Para usar esto en un servidor en la nube, se requiere obligatoriamente HTTPS.*
2.  **Modo Script (Standalone):** Ejecutando el script de Python localmente. Ideal para depuración sin abrir el navegador.
    ```bash
    pip install pyserial psycopg2-binary python-dotenv
    python backend/serial_reader.py
    ```

---

## 🌍 Despliegue en Producción

Para desplegar este sistema en un servidor VPS o en la nube (ej. AWS, DigitalOcean), se recomienda usar las imágenes preconstruidas publicadas en Docker Hub para evitar compilar en el servidor de destino.

### 1. Ejecutar en el Servidor
En el servidor remoto, solo necesitas el archivo de producción:

```bash
docker compose -f docker-compose.prod.yml up -d
```

### 2. Contexto Seguro (HTTPS)
Debido a que la Web Serial API requiere un contexto seguro para acceder a los puertos USB del equipo cliente, **es obligatorio configurar un proxy inverso con SSL** (ej. Nginx + Let's Encrypt). 

En la [Documentación Oficial del Proyecto (carpeta docs/)](./docs/docs/software/despliegue-docker.md) se encuentra la guía paso a paso para configurar Nginx y Certbot.

---

## 📂 Estructura del Repositorio

*   `/backend` - API REST en Flask y script lector de Python.
*   `/docs` - Código fuente de la documentación en Docusaurus.
*   `/firmware` - Código `.ino` para el microcontrolador STM32.
*   `/grafana` - Archivos de aprovisionamiento (datasources y dashboards json).
*   `/resultados` - Evidencias y capturas de los ensayos realizados.
*   `/sql_scripts` - Scripts de creación de esquemas para PostgreSQL.