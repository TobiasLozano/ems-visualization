---
id: anexo-referencia-tecnica
title: "Anexo: Referencia Técnica y Arquitectura de Ingeniería"
sidebar_label: "Referencia Técnica Integral"
sidebar_position: 10
---

:::info[Anexo de Consulta Técnica y Especificaciones de Ingeniería]
Este documento consolida la arquitectura tecnológica global, el diagrama de flujo general de telemetría y sirve como cuadro de mando de navegación para los **anexos técnicos de ingeniería** del prototipo.
:::

# Sistema de Control de Energía Fotovoltaica con Algoritmos MPPT para Carga de Baterías de Litio, Protección Térmica y Suministro a Carga Regulada

**Energy Management System (EMS) para Microrredes DC con Integración FV-BESS**  
Plataforma tecnológica de hardware embebido, circuitos de potencia, telemetría continua y observabilidad en tiempo real.

---

## Mapa de Navegación de Anexos Técnicos

Para consultar los modelos matemáticos detallados, cálculos de dimensionamiento, esquemáticos en KiCad y código fuente de cada subsistema, accede a las secciones dedicadas:

| Anexo Técnico | Nombre del Capítulo | Contenido Principal y Entregables |
|:---:|:---|:---|
| **[Capítulo 1](/)** | **Identificación del Prototipo** | Ficha formal institucional UPB (`EMS-UPB-2026-V1`), investigadores y registro de producto. |
| **[Anexo 2](/descripcion-general)** | **Descripción General del Prototipo** | Justificación del reto energético, diagrama de flujo general de datos y objetivos de control. |
| **[Anexo 3](/arquitectura-fisica)** | **Arquitectura General y Física** | Topología de Bus DC (24 V), simulación previa en PSIM, diseño en KiCad y fabricación de PCB. |
| **[Anexo 4](/componentes-hardware)** | **Componentes e Instrumentación** | Especificaciones de semiconductores (IRFZ44N, IR2110), sensores INA226/ACS712/MAX6675 y pinout STM32. |
| **[Anexo 5](/funcionamiento-algoritmos)** | **Funcionamiento de Algoritmos** | Algoritmo MPPT P&O a 80 kHz, perfil CC/CV a 50 kHz, máquina de estados FSM y tiempos muertos (500 ms). |
| **[Anexo 6](/arquitectura-software)** | **Arquitectura de Software y DB** | Microservicios Docker Compose, aislamiento de red `solar_net`, 8 tablas de series temporales en PostgreSQL. |
 