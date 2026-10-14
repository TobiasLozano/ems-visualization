---
sidebar_position: 3
slug: /arquitectura-fisica
title: 3. Arquitectura General y Física del Hardware
---

import InteractiveDiagram from '@site/src/components/InteractiveDiagram';

# 3. Arquitectura General y Física del Hardware

Siguiendo el recorrido natural de implementación del sistema, la arquitectura física y los circuitos electrónicos constituyen la base operativa sobre la cual se sustenta el monitoreo y control digital. Sin el hardware de potencia e instrumentación, el sistema de software carece de variables que procesar.

---

## Topología del Bus DC Central (24 V)

La arquitectura eléctrica del prototipo gira en torno a un **Bus DC central de 24 V nominales**. Este bus actúa como nodo de convergencia e intercambio energético entre las tres ramas del sistema:
1. **Rama de Generación (Panel Solar):** Conectada al bus a través de un convertidor elevador **Boost**. Eleva la tensión variable del panel (12 V a 18.5 V) hasta el nivel regulado del bus.
2. **Rama de Almacenamiento (Batería de Litio 3S2P):** Conectada mediante un convertidor bidireccional **Buck-Boost**:
   - En **Modo Buck (Descendente):** Extrae energía del bus de 24 V para recargar la batería de litio siguiendo el perfil de carga segura CC/CV (12.6 V máx).
   - En **Modo Boost (Ascendente):** Inyecta energía desde la batería (11.1 V nominales) hacia el bus de 24 V cuando la radiación solar cae y el panel no alcanza a sostener el consumo.
3. **Rama de Consumo (Carga):** Conectada directamente al bus central de 24 V, alimentando una resistencia calibrada de 15 Ω para ensayos controlados de potencia (15 W, 20 W y 25 W).

<InteractiveDiagram
  src="/img/diagramas/D-03_topologia_microrred_dc.png"
  title="Topología en Bloques de la Microrred DC y Puntos de Medición"
  figureNumber="3.1"
  description="Arquitectura de hardware con acoplamiento en Bus DC de 24 V, convertidores de potencia (Boost MPPT y Buck-Boost bidireccional), nodos de medición V/I y ramas de panel, batería y carga."
/>

---

## Simulación y Validación Previa en PSIM

Antes de proceder a la construcción de las tarjetas electrónicas, el circuito de potencia completo fue modelado y simulado en el software especializado **PSIM**. Este entorno permitió:
- Modelar el comportamiento físico no lineal de una celda fotovoltaica de 50 W ingresando parámetros de irradiancia solar y temperatura ambiente.
- Programar directamente el código del algoritmo Perturbar y Observar (P&O) en lenguaje C dentro de bloques funcionales de control para simular la conmutación de los MOSFETs.
- Verificar la convergencia y estabilidad del bus DC en torno a los 24 V ante cambios bruscos de irradiancia.

<div className="figure-container">
  <img 
    src="/img/diagramas/D-03_esquematico_PSIM_potencia_ref.png" 
    alt="Esquemático de simulación del sistema en PSIM" 
  />
  <div className="figure-caption">
    <strong>Figura 3.2:</strong> Esquemático de simulación del sistema de potencia en PSIM, integrando el convertidor Boost del panel, convertidor Buck-Boost de la batería, sensores e inyección de control MPPT en C.
  </div>
</div>

---

## Diseño y Fabricación de la Tarjeta de Control (PCB KiCad)

La tarjeta de control alberga la etapa de acondicionamiento de señales, los zócalos para el microcontrolador STM32 (Blue Pill), los controladores de compuerta (*gate drivers*) para los MOSFETs y los conectores de los módulos de sensores.

- **Diseño en KiCad:** Se concibió en dos capas (*top* y *bottom*), empleando huellas (*footprints*) personalizadas para los módulos de medición y aplicando un plano de tierra con relleno de cobre para minimizar el ruido electromagnético de conmutación.
- **Fabricación Propia:** Se empleó el método de transferencia térmica (planchado sobre baquela virgen) seguido de atacado químico con cloruro férrico para revelar las pistas de cobre.

<div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(300px, 1fr))', gap: '16px', margin: '24px 0' }}>
  <div className="figure-container" style={{ margin: 0 }}>
    <img 
      src="/hardware/F-01_kicad_pcb_layout.png" 
      alt="Diseño en KiCad del ruteo de la PCB de control" 
    />
    <div className="figure-caption">
      <strong>Figura 3.3 (a):</strong> Ruteo a doble cara de la tarjeta de control en KiCad.
    </div>
  </div>
  <div className="figure-container" style={{ margin: 0 }}>
    <img 
      src="/hardware/F-01_kicad_pcb_layout_3d.png" 
      alt="Render 3D de la PCB de control" 
    />
    <div className="figure-caption">
      <strong>Figura 3.3 (b):</strong> Vista tridimensional de la tarjeta de control con sus conectores.
    </div>
  </div>
</div>

<div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(300px, 1fr))', gap: '16px', margin: '24px 0' }}>
  <div className="figure-container" style={{ margin: 0 }}>
    <img 
      src="/hardware/F-02_pcb_fabricacion_atacado.jpg" 
      alt="Proceso de planchado y atacado químico" 
    />
    <div className="figure-caption">
      <strong>Figura 3.4 (a):</strong> Transferencia térmica y revelado en cloruro férrico de la PCB de control.
    </div>
  </div>
  <div className="figure-container" style={{ margin: 0 }}>
    <img 
      src="/hardware/F-03_tarjeta_control_ensamblada_PARCIAL_sin_STM32.jpg" 
      alt="Tarjeta de control con zócalos y soldaduras" 
    />
    <div className="figure-caption">
      <strong>Figura 3.4 (b):</strong> Tarjeta de control soldada con zócalos de inserción rápida para el STM32 y circuitos auxiliares.
    </div>
  </div>
</div>
 