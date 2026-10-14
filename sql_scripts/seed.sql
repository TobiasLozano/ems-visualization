-- Seed data representativo del Sistema EMS: últimas 24h, una lectura cada 15 min (96 registros por tabla)
-- Ajustado al Bus DC de 24V nominal, MPPT solar y banco de baterías Li-Ion 3S2P

INSERT INTO temp_panel (timestamp, value)
SELECT NOW() - (interval '15 minutes' * s), round((34.0 + random() * 6.0)::numeric, 1)
FROM generate_series(0, 95) s;

INSERT INTO temp_bat (timestamp, value)
SELECT NOW() - (interval '15 minutes' * s), round((27.5 + random() * 4.0)::numeric, 1)
FROM generate_series(0, 95) s;

INSERT INTO volt_panel (timestamp, value)
SELECT NOW() - (interval '15 minutes' * s), round((16.8 + random() * 1.8)::numeric, 2)
FROM generate_series(0, 95) s;

INSERT INTO amp_panel (timestamp, value)
SELECT NOW() - (interval '15 minutes' * s), round((1.2 + random() * 1.2)::numeric, 2)
FROM generate_series(0, 95) s;

INSERT INTO volt_bat (timestamp, value)
SELECT NOW() - (interval '15 minutes' * s), round((12.1 + random() * 0.5)::numeric, 2)
FROM generate_series(0, 95) s;

INSERT INTO amp_bat (timestamp, value)
SELECT NOW() - (interval '15 minutes' * s), round((0.6 + random() * 0.5)::numeric, 2)
FROM generate_series(0, 95) s;

INSERT INTO volt_load (timestamp, value)
SELECT NOW() - (interval '15 minutes' * s), round((23.85 + random() * 0.3)::numeric, 2)
FROM generate_series(0, 95) s;

INSERT INTO amp_load (timestamp, value)
SELECT NOW() - (interval '15 minutes' * s), round((0.75 + random() * 0.3)::numeric, 2)
FROM generate_series(0, 95) s;
