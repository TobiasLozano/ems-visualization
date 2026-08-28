---
sidebar_position: 3
title: Máquina de Estados
---

# Máquina de Estados y Algoritmos de Control

El firmware implementa una **máquina de estados finita** con tres modos de operación, gestionada por la función `updateModeStateMachine()`.

## Diagrama General de la Máquina de Estados

```mermaid
stateDiagram-v2
    [*] --> PANEL_MPPT : Arranque normal

    PANEL_MPPT --> BAT_CV : Vbus < 16.0V<br/>AND temp_bat_ok
    BAT_CV --> PANEL_MPPT : Vbus > 17.5V<br/>(hold 2s)
    BAT_CV --> PANEL_MPPT : temp_bat ≥ 60°C

    PANEL_MPPT --> LOW_BAT : Vbat ≤ 9.50V
    BAT_CV --> LOW_BAT : Vbat ≤ 9.50V

    LOW_BAT --> PANEL_MPPT : Vbat ≥ 10.20V

    note right of PANEL_MPPT
        Panel solar alimenta la carga.
        Algoritmo P&O optimiza potencia.
        PWM batería = 0.
    end note

    note right of BAT_CV
        Batería alimenta la carga.
        Regulación de voltaje constante.
        PWM panel = 0.
    end note

    note right of LOW_BAT
        Batería agotada.
        Todos los PWM = 0.
        Espera recuperación.
    end note
```

## Modos de Operación

### `MODE_PANEL_MPPT` — Panel Solar como Fuente Principal

En este modo, el convertidor Boost del panel está activo y ejecuta el algoritmo **MPPT (Maximum Power Point Tracking)** para extraer la máxima potencia del panel fotovoltaico.

- **PWM Panel:** Activo (controlado por P&O)
- **PWM Batería:** Desactivado (duty = 0)
- **Condición de salida:** El voltaje del bus cae por debajo de `VBUS_TO_BAT` (16.0V), indicando que el panel no puede sostener la carga.

### `MODE_BAT_CV` — Batería como Respaldo

Cuando el panel no puede abastecer la demanda, la batería entra como fuente de respaldo. El convertidor Buck mantiene el voltaje del bus en la ventana de **voltaje constante** (17.8V–18.2V).

- **PWM Panel:** Desactivado (duty = 0)
- **PWM Batería:** Activo (regulación CV)
- **Condición de salida:** El voltaje del bus supera `VBUS_TO_PANEL` (17.5V) durante al menos 2 segundos, **O** la temperatura de la batería excede 60°C.

### `MODE_LOW_BAT` — Batería Agotada

Estado de protección. Todos los PWM están desactivados hasta que la batería se recupere por encima de `VBAT_RECOVER` (10.20V).

---

## Tiempo Muerto (Deadtime)

Cada transición entre modos pasa por un **deadtime de 500ms** donde ambos PWM están desactivados para prevenir cortocircuitos entre fuentes:

```mermaid
sequenceDiagram
    participant SM as State Machine
    participant PWM as PWM Outputs

    SM->>PWM: disableAllPWM()
    Note over PWM: Deadtime: 500ms
    SM->>SM: switching = true
    SM-->>SM: Esperar 500ms...
    SM->>SM: switching = false
    SM->>PWM: Activar nuevo modo
```

```cpp
static inline void beginSwitchTo(Mode next) {
    switching = true;
    pendingMode = next;
    switchStartMs = millis();
    disableAllPWM();           // Apagar TODO
    vbusAboveSinceMs = 0;
}
```

---

## Algoritmo MPPT: Perturbar y Observar (P&O)

El algoritmo P&O busca el **punto de máxima potencia** del panel solar perturbando el duty cycle del PWM y observando si la potencia aumenta o disminuye.

### Diagrama de Flujo

```mermaid
flowchart TD
    START["controlPanelMPPT()"] --> CHECK{"Vpanel < 5.0V?"}
    CHECK -->|Sí| MIN["duty = MIN_ACTIVE<br/>lastPower = 0"]
    CHECK -->|No| CALC["P = Vpanel × Ipanel"]
    CALC --> CMP{"P ≥ lastPower?"}
    CMP -->|Sí - Potencia subió| SAME["Mantener dirección<br/>duty += lastDelta"]
    CMP -->|No - Potencia bajó| FLIP["Invertir dirección<br/>lastDelta = -lastDelta<br/>duty += lastDelta"]
    SAME --> CLAMP["duty = constrain(duty, 10, 255)"]
    FLIP --> CLAMP
    MIN --> APPLY["setPanelDuty(duty)"]
    CLAMP --> APPLY
```

### Código del Algoritmo

```cpp
void controlPanelMPPT() {
    float vpanel = volt_panel_filt;
    float ipanel = amp_panel_filt;

    // Limitar tasa de cambio a 20 steps/segundo
    unsigned long now = millis();
    unsigned long minTimeBetween = 1000UL / PWM_MAX_CHANGE_PER_SEC;
    if (now - lastPwmChangeMs < minTimeBetween) {
        setPanelDuty(panelDuty);
        return;
    }

    int oldDuty = panelDuty;

    if (vpanel < PANEL_MIN_VOLTAGE) {
        // Panel sin suficiente voltaje → duty mínimo
        panelDuty = PANEL_PWM_MIN_ACTIVE;
        lastPower = 0.0f;
    } else {
        float power = vpanel * ipanel;

        // Perturbar y Observar
        if (power >= lastPower)
            panelDuty += lastDelta;    // Misma dirección
        else {
            lastDelta = -lastDelta;     // Invertir
            panelDuty += lastDelta;
        }

        lastPower = power;
    }

    panelDuty = constrain(panelDuty, PANEL_PWM_MIN_ACTIVE, PWM_MAX);
    if (panelDuty != oldDuty) lastPwmChangeMs = now;
    setPanelDuty(panelDuty);
}
```

:::info Tasa de Convergencia
El paso del MPPT es de **1 unidad de duty** cada **50ms** (máximo 20 cambios/segundo). Esto permite convergencia suave sin oscilaciones bruscas que dañen los componentes.
:::

---

## Regulación CV de Batería

El control de voltaje constante ajusta el duty cycle del convertidor Buck para mantener el voltaje del bus dentro de la ventana `[17.8V, 18.2V]`:

```mermaid
flowchart TD
    START["controlBatCV()"] --> TEMP{"temp_bat_ok?"}
    TEMP -->|No| SWITCH["→ PANEL_MPPT"]
    TEMP -->|Sí| VBAT{"Vbat ≤ 9.50V?"}
    VBAT -->|Sí| LOW["→ LOW_BAT"]
    VBAT -->|No| CV{"Vbus?"}
    CV -->|"< 17.8V"| UP["duty += 1<br/>(más energía)"]
    CV -->|"> 18.2V"| DOWN["duty -= 1<br/>(menos energía)"]
    CV -->|"En rango"| HOLD["Mantener duty"]
    UP --> APPLY["setBatDuty(duty)"]
    DOWN --> APPLY
    HOLD --> APPLY
```

```cpp
void controlBatCV() {
    float vbus = volt_bus_filt;

    if (!temp_bat_ok) {
        beginSwitchTo(MODE_PANEL_MPPT);
        return;
    }

    if (vbus < VBUS_CV_LOW)       batDuty += PWM_STEP;  // Subir
    else if (vbus > VBUS_CV_HIGH) batDuty -= PWM_STEP;  // Bajar

    batDuty = constrain(batDuty, BAT_PWM_MIN_ACTIVE, PWM_MAX);
    setBatDuty(batDuty);
}
```

---

## Lógica Completa de `updateModeStateMachine()`

```mermaid
flowchart TD
    START["updateModeStateMachine()"] --> SW{"switching?"}
    SW -->|Sí| DEAD{"deadtime ≥ 500ms?"}
    DEAD -->|No| END1["return (esperar)"]
    DEAD -->|Sí| APPLY["Aplicar pendingMode<br/>Inicializar PWM"]

    SW -->|No| BAT_PRESENT{"Vbat > 5.0V?"}
    BAT_PRESENT -->|No| FORCE_PANEL["→ PANEL_MPPT"]

    BAT_PRESENT -->|Sí| TEMP_CHECK{"mode == BAT_CV<br/>AND !temp_bat_ok?"}
    TEMP_CHECK -->|Sí| FORCE_PANEL2["→ PANEL_MPPT"]

    TEMP_CHECK -->|No| CRITICAL{"Vbat ≤ 9.00V?"}
    CRITICAL -->|Sí| CRIT_OFF["→ LOW_BAT<br/>Apagado inmediato"]

    CRITICAL -->|No| LOW{"mode == LOW_BAT?"}
    LOW -->|Sí| RECOVER{"Vbat ≥ 10.20V?"}
    RECOVER -->|Sí| TO_PANEL["→ PANEL_MPPT"]
    RECOVER -->|No| WAIT["Esperar recuperación"]

    LOW -->|No| UV{"Vbat ≤ 9.50V?"}
    UV -->|Sí| TO_LOW["→ LOW_BAT"]

    UV -->|No| MODE_CHECK{"mode actual?"}
    MODE_CHECK -->|PANEL_MPPT| BUS_LOW{"Vbus < 16.0V?"}
    BUS_LOW -->|Sí AND temp_ok| TO_BAT["→ BAT_CV"]
    BUS_LOW -->|No| STAY_PANEL["Mantener MPPT"]

    MODE_CHECK -->|BAT_CV| BUS_HIGH{"Vbus > 17.5V<br/>por 2 segundos?"}
    BUS_HIGH -->|Sí| TO_PANEL2["→ PANEL_MPPT"]
    BUS_HIGH -->|No| STAY_BAT["Mantener BAT_CV"]
```

Esta función se ejecuta cada **200ms** y evalúa, en orden de prioridad:

1. **Deadtime activo** → Esperar.
2. **Batería ausente** → Forzar panel.
3. **Sobretemperatura** → Salir de batería.
4. **Voltaje crítico** → Apagado de emergencia.
5. **Bajo voltaje** → Modo LOW_BAT.
6. **Conmutación por bus** → Cambiar entre MPPT y CV según el voltaje del bus.
