---
sidebar_position: 5
slug: /funcionamiento-algoritmos
title: 5. Funcionamiento del EMS y Algoritmos de Control
---

import InteractiveDiagram from '@site/src/components/InteractiveDiagram';

# 5. Funcionamiento del EMS y Algoritmos de Control

El **Energy Management System (EMS)** ejecuta una lógica de control jerárquica programada en el microcontrolador STM32. Su función es balancear continuamente la generación solar y el almacenamiento para abastecer la demanda de forma ininterrumpida y proteger los componentes físicos.

---

## 1. Arquitectura Energética y Estrategia de Gestión

Existe una relación funcional bien definida entre el modo de **generación fotovoltaica**, el modo de **respaldo de la batería** y el modo de **recarga de la batería**:

1. **Prioridad Solar:** Toda la energía demandada por la carga es suministrada preferencialmente por el panel solar mediante el algoritmo MPPT.
2. **Déficit de Generación (Respaldo por Batería):** Si la irradiancia disminuye o el consumo supera la potencia instantánea del panel, el bus DC experimenta una caída de tensión. Al cruzar el umbral de **16.0 V**, el EMS activa la batería en modo Boost para suplir el déficit de energía y estabilizar el bus.
3. **Excedente de Generación (Recarga de Batería):** En condiciones de radiación abundante donde la carga resistiva demande menos de lo que el panel puede entregar, el exceso de energía se deriva a través del convertidor Buck hacia la batería bajo el perfil regulado CC/CV.

---

## 2. Algoritmo MPPT: Perturbar y Observar (P&O) Modificado

El algoritmo tradicional de Perturbar y Observar calcula la potencia instantánea *P(k) = V(k) × I(k)* y la compara con la del ciclo anterior *P(k-1)*. Si la potencia aumentó, continúa perturbando el ciclo de trabajo del PWM en la misma dirección; si disminuyó, invierte el sentido de perturbación.

### Modificación Clave para Protección de Bus
En sistemas fotovoltaicos acoplados directamente a un bus DC, si la carga se desconecta o disminuye súbitamente, el algoritmo P&O tradicional continuaría elevando la tensión del bus por encima de los límites nominales de los condensadores y semiconductores.

El algoritmo implementado introduce una **restricción activa de sobretensión**:
> **Regla de Protección de Tensión:**  
> `Si V_bus ≥ 24.0 V ⟹ Duty = Duty - 1` *(Independientemente de la dirección que ordene P&O)*

<InteractiveDiagram
  src="/img/diagramas/D-06_algoritmo_mppt_po.svg"
  title="Diagrama de Flujo del Algoritmo MPPT Perturbar y Observar Modificado"
  figureNumber="5.1"
  description="Lógica de rastreo de máxima potencia del panel a 80 kHz con lazo de protección que clampéa la tensión del bus a 24.0 V ante sobretensiones."
/>

### Implementación en firmware
```cpp
void controlPanelMPPT() {
    float vpanel = volt_panel_filt;
    float ipanel = amp_panel_filt;
    float vbus   = volt_bus_filt;

    if (vpanel < PANEL_MIN_VOLTAGE) {
        panelDuty = PANEL_PWM_MIN_ACTIVE;
        lastPower = 0.0f;
    } else {
        float power = vpanel * ipanel;
        if (power >= lastPower)
            panelDuty += lastDelta;
        else {
            lastDelta = -lastDelta;
            panelDuty += lastDelta;
        }
        lastPower = power;
    }

    // Restricción activa de seguridad en el bus DC:
    if (vbus >= 24.0f) {
        panelDuty -= 1;
    }

    panelDuty = constrain(panelDuty, PANEL_PWM_MIN_ACTIVE, PWM_MAX);
    setPanelDuty(panelDuty);
}
```

---

## 3. Algoritmo de Carga de Batería de Litio (Perfil CC/CV)

Para prolongar la vida útil de las celdas de litio 3S2P (5200 mAh) y evitar su degradación electroquímica, la recarga se ejecuta en dos etapas automáticas:

1. **Etapa CC (Corriente Constante):** Se regula la corriente de carga a **1.0 A**. Si *I_bat < 0.95 A*, el ciclo de trabajo del convertidor Buck aumenta; si *I_bat > 1.05 A*, se reduce.
2. **Etapa CV (Voltaje Constante):** Al alcanzar una tensión de celda total de **12.55 V**, el algoritmo conmuta automáticamente a regulación de tensión para mantener la batería entre **12.55 V y 12.60 V**, reduciendo la corriente de forma exponencial.
3. **Criterio de Fin de Carga (`MODE_DONE`):** La carga concluye y el PWM se apaga a 0% cuando se cumplen simultáneamente dos condiciones:
   > **Condición de Corte:** `V_bat ≥ 12.57 V` y `I_bat ≤ 0.35 A`

<InteractiveDiagram
  src="/img/diagramas/D-07_algoritmo_carga_cc_cv.svg"
  title="Diagrama de Flujo del Algoritmo de Carga CC/CV de Batería de Litio"
  figureNumber="5.2"
  description="Transición de fase CC a CV con modulación PWM a 50 kHz y desconexión segura en MODE_DONE."
/>

---

## 4. Máquina de Estados Finita (FSM) y Tiempo Muerto

El firmware implementa una máquina de estados determinista evaluada cada **200 ms** por la función `updateModeStateMachine()`:

<InteractiveDiagram
  src="/img/diagramas/D-05_maquina_estados_ems.svg"
  title="Máquina de Estados de Control Energético del Firmware EMS"
  figureNumber="5.3"
  description="Transiciones entre modos PANEL_MPPT, BAT_CV y LOW_BAT con deadtime de 500 ms e histéresis de voltaje y temperatura."
/>

### Mecanismo de Tiempo Muerto (Deadtime de 500 ms)
Para evitar que el convertidor Boost del panel y el convertidor de la batería conduzcan simultáneamente provocando cortocircuitos o corrientes circulantes destructivas entre fuentes, toda transición de estado involucra un período de reposo forzado:
```cpp
static inline void beginSwitchTo(Mode next) {
    switching = true;
    pendingMode = next;
    switchStartMs = millis();
    disableAllPWM(); // Apagado instantáneo de ambos PWM
}
```
Durante los 500 ms de *deadtime*, los ciclos de trabajo de ambos convertidores se fuerzan a 0%.

---

## 5. Sistema de Protección Térmica Activa

La temperatura de la batería de litio es leída cada segundo mediante la termocupla conectada al sensor MAX6675.

- **Umbral de Disparo Crítico:** Si **T_bat ≥ 60.0 °C**, el sistema declara inmediatamente `temp_bat_ok = false`, desactiva el PWM de la batería y expulsa al sistema del modo `BAT_CV`, obligando a operar únicamente con el panel solar o apagando la salida.
- **Histéresis de Recuperación (5 °C):** Para evitar conmutaciones erráticas y destructivas mientras la batería se enfría, el sistema solo permite reactivar el respaldo de batería cuando la temperatura desciende por debajo de **55.0 °C**.
