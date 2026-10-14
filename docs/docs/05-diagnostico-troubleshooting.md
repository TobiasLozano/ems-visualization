---
sidebar_position: 6
slug: /diagnostico-troubleshooting
title: 6. Solución de Problemas
---

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
