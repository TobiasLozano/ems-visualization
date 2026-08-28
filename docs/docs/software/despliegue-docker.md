---
sidebar_position: 1
title: Despliegue Docker
---

# Despliegue con Docker Compose

El sistema EMS se distribuye como un conjunto de **imágenes Docker** preconfiguradas que se orquestan con Docker Compose.

## Imágenes del Sistema

| Imagen | Contenido | Puerto |
|--------|----------|--------|
| `tobiaslozano/ems-postgres` | PostgreSQL 15 Alpine + script SQL de respaldo | 5432 |
| `tobiaslozano/ems-webapp` | Flask + Gunicorn + interfaz web | 5000 |
| `tobiaslozano/ems-grafana` | Grafana con dashboards y datasource preconfigurados | 3000 |

## Archivos de Configuración

El proyecto incluye dos archivos de compose:

- **`docker-compose.yml`** — Desarrollo local (construye las imágenes desde los Dockerfiles).
- **`docker-compose.prod.yml`** — Producción (usa imágenes preconstruidas desde Docker Hub).

## Modo Desarrollo

```bash
# Clonar el repositorio
git clone https://github.com/TobiasLozano/ems-visualization.git
cd ems-visualization

# Levantar todos los servicios (construye las imágenes)
docker compose -f docker-compose.yml up -d --build
```

### Estructura del `docker-compose.yml`

```yaml
services:
  postgres:
    build:
      context: .
      dockerfile: backend/Dockerfile.postgres
    image: tobiaslozano/ems-postgres:latest
    environment:
      POSTGRES_USER: postgres
      POSTGRES_PASSWORD: postgres
      POSTGRES_DB: sensors_db
    ports:
      - "5432:5432"
    healthcheck:
      test: ["CMD-SHELL", "pg_isready -U postgres"]
      interval: 10s
      timeout: 5s
      retries: 5

  web_app:
    build:
      context: .
      dockerfile: backend/Dockerfile.web
    image: tobiaslozano/ems-webapp:latest
    environment:
      DB_HOST: postgres
      DB_PORT: 5432
      DB_NAME: sensors_db
      DB_USER: postgres
      DB_PASSWORD: postgres
    ports:
      - "5000:5000"
    depends_on:
      postgres:
        condition: service_healthy

  grafana:
    build:
      context: .
      dockerfile: backend/Dockerfile.grafana
    image: tobiaslozano/ems-grafana:latest
    ports:
      - "3000:3000"
    depends_on:
      - postgres
```

## Modo Producción

```bash
# En el servidor remoto (sin necesidad de los fuentes)
docker compose -f docker-compose.prod.yml up -d
```

:::caution Puertos
Es necesario asegurar que los puertos **5000** (Web App) y **3000** (Grafana) estén abiertos en el firewall del servidor. **No se debe exponer el puerto 5432** (PostgreSQL) al público.
:::

## Variables de Entorno

La configuración se puede personalizar creando un archivo `.env` en la raíz del proyecto:

```bash
# .env
DOCKER_USERNAME=tobiaslozano
DB_USER=postgres
DB_PASSWORD=tu_password_seguro
DB_NAME=sensors_db
```

## Aprovisionamiento Automático de Grafana

El dashboard de Grafana y la conexión a la base de datos se configuran de forma **completamente automática** al levantar el contenedor, gracias al sistema de *provisioning* de Grafana. No es necesaria ninguna configuración manual.

Para acceder al panel:

1. Abrir `http://<ip-servidor>:3000` en un navegador.
2. Ingresar con las credenciales por defecto:
   - **Usuario:** `admin`
   - **Password:** `admin`
   *(Se solicitará cambiar la contraseña en el primer inicio de sesión).*

### Dashboard Preconfigurado

Al ingresar, se encuentra el dashboard **"Sistema EMS - Microgrid DC"**, que incluye:

- **Paneles de Potencia Instantánea**: `P = V × I` para Panel, Batería y Carga.
- **Curvas de Voltaje y Corriente**: Gráficas dinámicas con ventanas de tiempo ajustables.
- **Monitoreo Térmico**: Indicadores de temperatura en tiempo real.

![Dashboard de Grafana](/img/resultados/dashboard.png)

### Datasource PostgreSQL (Provisioning)

El datasource se provisiona mediante el archivo `grafana/provisioning/datasources/datasource.yml`. La configuración apunta internamente al contenedor `solar_postgres` en la red de Docker (`solar_net`), utilizando credenciales seguras.

Los dashboards se cargan automáticamente desde la carpeta `grafana/dashboards/` definidos en `grafana/provisioning/dashboards/dashboards.yml`.

## Requisitos para Web Serial API en Producción

La **Web Serial API** (usada para conectar el Arduino desde el navegador) requiere un **contexto seguro**. Esto significa:

- ✅ `http://localhost` funciona sin SSL.
- ❌ `http://dominio-servidor.com` **NO** funciona.
- ✅ `https://dominio-servidor.com` funciona con certificado SSL.

### Configuración recomendada: Nginx + Let's Encrypt

```bash
# Instalar Nginx y Certbot
sudo apt install nginx certbot python3-certbot-nginx

# Obtener certificado SSL gratuito
sudo certbot --nginx -d dominio.com
```

Ejemplo de configuración Nginx como proxy inverso:

```nginx
server {
    listen 443 ssl;
    server_name dominio.com;

    ssl_certificate /etc/letsencrypt/live/dominio.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/dominio.com/privkey.pem;

    location / {
        proxy_pass http://localhost:5000;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }
}
```
