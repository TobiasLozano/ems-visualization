---
sidebar_position: 2
slug: /descripcion-general
title: 2. Descripción General del Sistema
---

import Figure from '@site/src/components/Figure';

# 2. Descripción General del Sistema

## Contexto Energético y Planteamiento del Problema

El aprovechamiento eficiente de las fuentes de energía renovable no convencional (FNCER) constituye uno de los desafíos tecnológicos prioritarios para la transición energética. Colombia cuenta con un recurso solar excepcional, especialmente en la **Región Caribe** y el departamento de **Córdoba**, donde los niveles de irradiancia solar global y temperaturas promedio cercanas a los **30 °C** ofrecen condiciones representativas para la investigación de sistemas fotovoltaicos aislados.

Sin embargo, la integración práctica de la energía solar presenta dos desafíos críticos de control e instrumentación:

1. **Naturaleza no lineal del generador fotovoltaico:** Un panel solar no suministra potencia ni tensión constante; su punto de operación varía continuamente con la irradiancia y la temperatura según la curva característica Corriente-Voltaje (**I-V**). En dicha curva existe un único punto donde la potencia generada es máxima: el **Punto de Máxima Potencia (MPP)**. La relación entre la potencia máxima real y el producto teórico de la tensión de circuito abierto (**Voc**) y la corriente de cortocircuito (**Isc**) define el *Fill Factor* (**FF**), parámetro que exige algoritmos activos de rastreo para maximizar el factor de planta.
2. **Fragilidad electroquímica y térmica de las baterías de litio:** A diferencia de las baterías convencionales de plomo-ácido, las celdas de iones de litio exigen una gestión estricta basada en el perfil de **Corriente Constante / Voltaje Constante (CC/CV)**. La sobrecarga, la sobrecorriente o el incremento de temperatura por encima de los límites de seguridad degradan irreversiblemente el material activo de los electrodos y pueden derivar en embalamiento térmico (*thermal runaway*).

El **Sistema EMS (Energy Management System)** implementado resuelve estos dos desafíos de forma unificada mediante un microcontrolador de 32 bits, convertidores DC-DC conmutados de alta frecuencia y una plataforma de observabilidad en tiempo real.

---

## Objetivos del Proyecto

### Objetivo General
Desarrollar un software EMS funcional y escalable para microrredes DC con integración FV-batería, que permita monitoreo en tiempo real, control inteligente de potencia y registro histórico de datos.

### Objetivos Específicos
1. **Firmware & Control Embebido (STM32):** Implementar el firmware en microcontrolador STM32 con algoritmo MPPT Perturb & Observe (P&O) para la máxima extracción de energía solar fotovoltaica y regulación de voltaje constante (CV) para la gestión segura de baterías.
2. **Infraestructura Software & Base de Datos:** Desplegar la plataforma integrada mediante contenedores Docker Compose, incorporando un backend REST en Flask, base de datos PostgreSQL para series temporales de potencia y SOC, y dashboards autoconfigurados en Grafana.
3. **Supervisión & Control Web en Tiempo Real:** Desarrollar una interfaz web interactiva basada en Web Serial API para la comunicación serial directa con el STM32 mediante conexión HTTPS, habilitando supervisión y control local o remoto.
4. **Propiedad Intelectual & Transferencia:** Registrar el software desarrollado como producto tecnológico ante la Dirección Nacional de Derechos de Autor (DNDA) y la UPB Seccional Montería, garantizando la protección y transferencia institucional.

---

## Módulos y Estructura del Repositorio `ems-visualization`

La plataforma se organiza en cuatro capas funcionales articuladas en la estructura del código fuente:

| Módulo en Repositorio | Tecnología | Función Principal |
|:---|:---|:---|
| `/firmware` | STM32 (`.ino` / C++) | Algoritmo MPPT P&O, regulación CV de batería, temporizadores PWM y sensórica. |
| `/backend` | Flask + Python 3.10 | API REST, puente de lectura serial, procesamiento de tramas e interfaz con BD. |
| `/sql_scripts` | PostgreSQL 15 | Esquema relacional optimizado para series temporales de potencia y estado del sistema. |
| `/grafana` | Grafana OSS | Dashboards autoconfigurados: potencia FV, corriente de batería y consumo de carga. |
| `/docs` | Docusaurus + React | Documentación técnica interactiva, arquitectura de hardware y manuales de puesta en marcha. |

---

## Arquitectura Operativa del Sistema de Software

El ecosistema de software y telemetría está diseñado para operar de forma continua, desacoplando el lazo de control en tiempo real del almacenamiento y la visualización:

1. **Capa de Control Embebido (Firmware STM32):**
   - Ejecuta localmente los algoritmos de potencia: **MPPT Perturbar y Observar (P&O)** a 80 kHz sobre el convertidor Boost del panel, y **regulación de voltaje constante (CV)** a 50 kHz sobre el convertidor Buck-Boost de la batería.
   - Aplica protección térmica activa supervisada por termocupla (bloqueo por temperatura ≥ 60 °C).
2. **Capa de Comunicación Serial:**
   - Emite periódicamente (1 Hz) una trama JSON delimitada por salto de línea a través del puerto UART a 9600 baudios con las 8 magnitudes físicas instantáneas.
3. **Capa de Ingesta y Persistencia (Backend):**
   - Ofrece dos vías de ingesta desacopladas: interfaz web interactiva mediante **Web Serial API** o script independiente de consola (**`serial_reader.py`**).
   - Valida y parsea las tramas para su inserción automática en el motor de base de datos **PostgreSQL**.
4. **Capa de Observabilidad (Grafana OSS):**
   - Consulta el motor de base de datos para renderizar series temporales en tiempo real, balance neto de energía, indicadores de estado operativo y alertas visuales.

---

## Flujo General de Telemetría

<Figure
  src="/img/diagramas/D-01_flujo_general_datos.svg"
  title="Flujo General de Datos y Telemetría del Sistema EMS"
  figureNumber="2.1"
  description="Arquitectura de telemetría de 4 capas: Hardware/Sensores → Puerto Serial USB → Base de datos PostgreSQL → Visualización Grafana y API Flask."
/>

*Para la guía paso a paso de conexión en cada modo de ingesta, consulte el [Capítulo 2: Modos de Adquisición](/modos-adquisicion). Para los detalles de infraestructura y base de datos, consulte el [Anexo 6: Arquitectura de Software y Base de Datos](/arquitectura-software).*
