#!/bin/bash

# Ejecutar como postgres
psql -U postgres << EOF

-- Crear base de datos Grafana
CREATE DATABASE IF NOT EXISTS grafana_db;
\c grafana_db

-- Crear base de datos sensores
\c postgres
CREATE DATABASE IF NOT EXISTS sensors_db;
\c sensors_db

-- Tabla: Temperatura Panel
CREATE TABLE IF NOT EXISTS temp_panel (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    value FLOAT
);
CREATE INDEX IF NOT EXISTS idx_temp_panel_timestamp ON temp_panel(timestamp);

-- Tabla: Temperatura Batería
CREATE TABLE IF NOT EXISTS temp_bat (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    value FLOAT
);
CREATE INDEX IF NOT EXISTS idx_temp_bat_timestamp ON temp_bat(timestamp);

-- Tabla: Voltaje Panel
CREATE TABLE IF NOT EXISTS volt_panel (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    value FLOAT
);
CREATE INDEX IF NOT EXISTS idx_volt_panel_timestamp ON volt_panel(timestamp);

-- Tabla: Corriente Panel
CREATE TABLE IF NOT EXISTS amp_panel (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    value FLOAT
);
CREATE INDEX IF NOT EXISTS idx_amp_panel_timestamp ON amp_panel(timestamp);

-- Tabla: Voltaje Batería
CREATE TABLE IF NOT EXISTS volt_bat (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    value FLOAT
);
CREATE INDEX IF NOT EXISTS idx_volt_bat_timestamp ON volt_bat(timestamp);

-- Tabla: Corriente Batería
CREATE TABLE IF NOT EXISTS amp_bat (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    value FLOAT
);
CREATE INDEX IF NOT EXISTS idx_amp_bat_timestamp ON amp_bat(timestamp);

-- Tabla: Voltaje Carga/Load
CREATE TABLE IF NOT EXISTS volt_load (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    value FLOAT
);
CREATE INDEX IF NOT EXISTS idx_volt_load_timestamp ON volt_load(timestamp);

-- Tabla: Corriente Carga/Load
CREATE TABLE IF NOT EXISTS amp_load (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    value FLOAT
);
CREATE INDEX IF NOT EXISTS idx_amp_load_timestamp ON amp_load(timestamp);

EOF
