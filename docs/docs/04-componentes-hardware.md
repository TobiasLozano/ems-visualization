---
sidebar_position: 4
slug: /componentes-hardware
title: 4. Componentes del Prototipo e Instrumentación
---

import InteractiveDiagram from '@site/src/components/InteractiveDiagram';

# 4. Componentes del Prototipo e Instrumentación

Esta sección detalla los componentes físicos que conforman las tres etapas de hardware del prototipo: **etapa de potencia**, **instrumentación sensorial** y **unidad de procesamiento**.

---

## 1. Hardware de Potencia y Convertidores DC-DC

El prototipo incorpora dos convertidores conmutados de diseño dedicado:

### Convertidor Boost (Lado del Panel Solar)
- **Función:** Elevar la tensión del panel fotovoltaico (15 V – 19 V) hacia la tensión del Bus DC (24 V), operando como elemento actuador del algoritmo MPPT.
- **Topología:** Convertidor elevador no aislado.
- **Semiconductor Activo:** MOSFET de potencia de canal N **IRFZ44N** (V_DS = 55 V, I_D = 49 A, R_DS(on) = 17.5 mΩ).
- **Frecuencia de Conmutación:** **80 kHz**, generada por el canal de temporizador por hardware `TIM2_CH4` del STM32.
- **Componentes Pasivos:** Inductor toroidal de potencia con núcleo de ferrita de bajas pérdidas y condensadores electrolíticos de baja ESR.

### Convertidor Bidireccional Buck-Boost (Lado de la Batería)
- **Función:** Operación en dos cuadrantes entre el bus central de 24 V y el banco de baterías de litio 3S2P (9.0 V – 12.6 V).
- **Modo Buck:** Regula la corriente y tensión de recarga de la batería (CC/CV) extrayendo energía del bus DC.
- **Modo Boost:** Descarga la batería hacia el bus para suministrar potencia de respaldo a la carga resistiva.
- **Frecuencia de Conmutación:** **50 kHz**, generada mediante modulación por ancho de pulso en el pin `PA6`.
- **Driver de Compuerta:** Circuito controlador **IR2110** con condensador de *bootstrap* para el accionamiento flotante del MOSFET de lado alto (*high-side*).

---

## 2. Detalle de Conexiones y Borneras Rotuladas

Para facilitar el ensamblado, mantenimiento y diagnóstico, cada placa de potencia dispone de borneras tipo bloque de terminales claramente identificadas:

<div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(320px, 1fr))', gap: '20px', margin: '24px 0' }}>
  <div className="figure-container" style={{ margin: 0 }}>
    <img 
      src="/hardware/F-05_conexiones_rotuladas_boost_PV.png" 
      alt="Conexiones rotuladas del Convertidor Boost del Panel" 
    />
    <div className="figure-caption">
      <strong>Figura 4.1:</strong> Convertidor Boost (Lado Panel): identificación de borneras para Entrada PV, Sensores V/I PV, Señal de driver MOSFET, Salida hacia Carga y Sensor V Load.
    </div>
  </div>

  <div className="figure-container" style={{ margin: 0 }}>
    <img 
      src="/hardware/F-05_conexiones_rotuladas_buckboost_bateria.png" 
      alt="Conexiones rotuladas del Convertidor Bidireccional Batería" 
    />
    <div className="figure-caption">
      <strong>Figura 4.2:</strong> Convertidor Buck-Boost (Lado Batería): identificación de borneras para Entrada Bus DC, Conexión Batería 3S, Señal driver, Conexión Load y sensores de V/I.
    </div>
  </div>
</div>

---

## 3. Instrumentación Sensorial

Para que el STM32 cuente con visibilidad completa del estado energético en tiempo real, se monitorean las tres ramas mediante 7 transductores físicos:

| Variable Medida | Transductor | Interfaz | Pin / Dirección | Rango de Trabajo | Resolución |
|:---|:---|:---|:---|:---|:---|
| **Tensión Panel Solar** | INA226 | I2C | Dirección `0x41` | 0 V – 36 V | 1.25 mV/LSB |
| **Tensión Batería 3S** | INA226 | I2C | Dirección `0x40` | 0 V – 36 V | 1.25 mV/LSB |
| **Tensión Bus DC / Carga** | INA226 | I2C | Dirección `0x44` | 0 V – 36 V | 1.25 mV/LSB |
| **Corriente Panel Solar** | ACS712-05B | ADC 12 bits | Pin `PA0` (ADC1_IN0) | -5 A a +5 A | 185 mV/A |
| **Corriente Bus / Carga** | ACS712-05B | ADC 12 bits | Pin `PA1` (ADC1_IN1) | -5 A a +5 A | 185 mV/A |
| **Corriente Batería** | ACS712-05B | ADC 12 bits | Pin `PA2` (ADC1_IN2) | -5 A a +5 A | 185 mV/A |
| **Temperatura Batería** | MAX6675 | SPI | CS: `PA8`, SCK: `PB13`, MISO: `PB14` | 0 °C – 1024 °C | 0.25 °C |
 
## 4. Unidad de Control (STM32F103) y Pinout Completo

El cerebro del sistema es un microcontrolador **STM32F103C8T6 (núcleo ARM Cortex-M3 a 72 MHz)** montado en placa tipo *Blue Pill*:

```text
               ┌─────────────────────────────────────┐
               │         STM32F103 (Blue Pill)       │
               │                                     │
   ACS Panel ──┤ PA0 (ADC1_IN0)            PB6 (SCL) ├── I2C Bus Sensores
   ACS Bus   ──┤ PA1 (ADC1_IN1)            PB7 (SDA) ├── INA226 (x3)
   ACS Bat   ──┤ PA2 (ADC1_IN2)                      │
   PWM Panel ──┤ PA3 (TIM2_CH4 @ 80kHz)   PB13 (SCK) ├── SPI MAX6675
   PWM Bat   ──┤ PA6 (PWM @ 50kHz)        PB14 (MISO)├── SPI MAX6675
   MAX CS    ──┤ PA8 (SPI Chip Select)               │
   Serial TX ──┤ PA9 (USART1_TX)         PA10 (RX)   ├── Hacia USB-TTL
               └─────────────────────────────────────┘
```

<div className="figure-container">
  <img 
    src="/hardware/F-07_montaje_rotulado_alterno.png" 
    alt="Banco de pruebas físico integrado con rótulos de subsistemas" 
  />
  <div className="figure-caption">
    <strong>Figura 4.3:</strong> Banco de pruebas completamente cableado: integración de la tarjeta de control, los dos convertidores DC-DC, el pack de baterías 3S2P, el banco de resistencias y la conexión al bus.
  </div>
</div>
