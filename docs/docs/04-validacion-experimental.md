---
sidebar_position: 5
slug: /validacion-experimental
title: 5. Validación Experimental
---

import Figure from '@site/src/components/Figure';

# 5. Validación Experimental

Este capítulo describe los cinco casos de prueba ejecutados sobre el prototipo físico del EMS. Cada caso incluye su objetivo, las condiciones del ensayo, el resultado observado, el rango temporal exacto en el que quedó registrado y la captura de Grafana correspondiente.

**Condiciones comunes a todos los ensayos**

* Bus DC central regulado a 24 V.
* Carga resistiva calibrada de 15 Ω.
* Banco de baterías Li-Ion 3S2P.
* Generación con panel fotovoltaico de 50 W en exteriores (Montería, Córdoba), salvo en la prueba de carga de batería, que usó una fuente de laboratorio fija de 24 V.
* Telemetría del STM32 registrada en PostgreSQL a 1 muestra por segundo.

## Cómo consultar los datos de un caso

Todas las mediciones de cada ensayo permanecen almacenadas en la base de datos. Para volver a verlas o descargarlas:

1. Abre el Panel Web (ver capítulo 4).
2. En el módulo de descarga, copia el bloque **Rango para copiar** del caso que te interesa.
3. Presiona **Pegar rango desde Grafana**. Los campos `DESDE` y `HASTA` se completan automáticamente. También puedes escribir esos mismos valores a mano.
4. Elige la variable (o el paquete ZIP con las 8 tablas) y descarga en CSV o JSON.

---

## Caso 1. Carga de batería con perfil CC/CV

**Objetivo.** Comprobar que el convertidor Buck carga el banco de baterías con corriente constante y luego con tensión constante.

**Condiciones.** Fuente de laboratorio fija de 24 V como entrada. Convertidor Buck a 50 kHz gestionado por el STM32.

**Resultado observado.**
* Etapa de corriente constante (CC): la corriente de carga se regula en **1 A** mientras la tensión de la batería sube.
* Etapa de tensión constante (CV): la tensión se mantiene en **12.6 V** y la corriente decae gradualmente.
* Eficiencia experimental de la etapa: **53.6 %**.

**Rango para copiar** (duración: 29 min 34 s)

```json
{"from":"2026-05-13T15:54:50","to":"2026-05-13T16:24:24"}
```

<Figure
  src="/img/resultados/Carga_Bateria/carga_1.png"
  title="Corriente y tensión durante la carga CC/CV"
  figureNumber="5.1"
  description="Fase de corriente constante a 1 A y posterior decaimiento al alcanzar la tensión de corte."
  maxWidth="760px"
/>

<Figure
  src="/img/resultados/Carga_Bateria/carga_2.png"
  title="Detalle de estabilización de las curvas"
  figureNumber="5.2"
  description="Transición de CC a CV y meseta de tensión en 12.6 V."
  maxWidth="920px"
/>

---

## Caso 2. Gestión de energía con demanda de 15 W

**Objetivo.** Verificar que el panel solar abastece por sí solo una demanda de 15 W.

**Condiciones.** Carga de 15 Ω con consumo de 15 W y radiación solar suficiente.

**Resultado observado.** El panel cubrió la demanda de forma casi autónoma. La batería intervino de manera mínima durante todo el ensayo.

**Rango para copiar** (duración: 23 min 47 s)

```json
{"from":"2026-05-26T17:21:44","to":"2026-05-26T17:45:31"}
```

<Figure
  src="/img/resultados/15W/prueba_15w.png"
  title="Telemetría del ensayo a 15 W"
  figureNumber="5.3"
  description="El panel solar suministra la carga y la corriente de batería es mínima."
/>

---

## Caso 3. Gestión de energía con demanda de 20 W

**Objetivo.** Comprobar el comportamiento dinámico cuando la irradiancia varía o resulta insuficiente.

**Condiciones.** Carga de 15 Ω con consumo de 20 W y radiación solar variable.

**Resultado observado.** Cuando la generación solar no alcanzó a cubrir la demanda, la batería compensó el déficit. El bus DC se mantuvo estable durante toda la prueba.

**Rango para copiar** (duración: 48 min 02 s)

```json
{"from":"2026-05-25T18:40:53","to":"2026-05-25T19:28:55"}
```

<Figure
  src="/img/resultados/20W/prueba_20w.png"
  title="Telemetría del ensayo a 20 W"
  figureNumber="5.4"
  description="La corriente de batería se ajusta para mantener constante la tensión del bus DC."
/>

---

## Caso 4. Gestión de energía con demanda de 25 W (respaldo BAT_CV)

**Objetivo.** Validar la respuesta sostenida del sistema cuando el panel no alcanza para la demanda.

**Condiciones.** Carga de 15 Ω con consumo de 25 W.

**Resultado observado.** El sistema conmutó de forma automática y sostenida al modo de respaldo de batería (`BAT_CV`). La batería entregó corrientes de descarga de hasta **2.5 A** y la carga mantuvo continuidad de suministro en todo momento.

**Rango para copiar** (duración: 46 min 05 s)

```json
{"from":"2026-05-26T18:17:32","to":"2026-05-26T19:03:37"}
```

<Figure
  src="/img/resultados/25W/prueba_25w.png"
  title="Telemetría del ensayo a 25 W"
  figureNumber="5.5"
  description="Respaldo continuo de la batería con picos de descarga de hasta 2.5 A."
/>

---

## Caso 5. Protección térmica de la batería

**Objetivo.** Validar el disparo por sobretemperatura y el rearme por histéresis.

**Condiciones.** Calentamiento controlado con pistola de calor aplicada directamente sobre el sensor de temperatura de la batería.

**Resultado observado.**
* Al superar **60 °C**, el sistema desactiva de inmediato el suministro (la temperatura se observa en `temp_bat`).
* El sistema permanece bloqueado mientras la temperatura baja.
* Solo reanuda la operación cuando la temperatura desciende por debajo de **55 °C** (histéresis de 5 °C).

**Rango para copiar** (duración: 10 min 46 s)

```json
{"from":"2026-05-25T17:59:14","to":"2026-05-25T18:10:00"}
```

<Figure
  src="/img/resultados/Termica/termica_1.png"
  title="Disparo por sobretemperatura"
  figureNumber="5.6"
  description="Al superar 60 °C el suministro se corta de inmediato."
  maxWidth="760px"
/>

<Figure
  src="/img/resultados/Termica/termica_2.png"
  title="Enfriamiento y restablecimiento por histéresis"
  figureNumber="5.7"
  description="El sistema se reactiva únicamente tras descender por debajo de 55 °C."
  maxWidth="920px"
/>
