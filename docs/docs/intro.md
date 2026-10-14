---
sidebar_position: 1
slug: /
title: 1. Identificación del Prototipo
---

import Figure from '@site/src/components/Figure';

# 1. Identificación del Prototipo y Presentación del Sistema

### Manual de Usuario y Operación — Sistema EMS para Microrredes DC

Este documento constituye la guía de operación y ficha técnica descriptiva del prototipo tecnológico desarrollado como Trabajo de Grado en Ingeniería Electrónica en la **Universidad Pontificia Bolivariana (UPB)**, Seccional Montería, en articulación con el grupo de investigación **SEC-LNCE**.

<div className="pdf-download-card">
  <div>
    <h3 style={{ margin: 0, color: 'var(--ifm-color-primary)' }}>Manual de Usuario Completo en PDF</h3>
    <div style={{ margin: '4px 0 0 0', fontSize: '0.9rem', color: 'var(--ifm-color-content-secondary)' }}>
      Genera y descarga la versión consolidada del manual con los 6 capítulos de operación, telemetría y validación experimental.
    </div>
  </div>
  <div style={{ display: 'flex', gap: '10px', alignItems: 'center', flexWrap: 'wrap' }}>
    <a 
      className="pdf-download-card-btn" 
      href="/documento-completo?print=true" 
      target="_blank" 
      rel="noopener noreferrer"
    >
      Generar PDF Consolidado →
    </a>
  </div>
</div>

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

