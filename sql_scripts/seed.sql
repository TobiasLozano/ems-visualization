-- Seed data: últimas 24h, una lectura cada 15 min (96 registros por tabla)

INSERT INTO temp_panel (timestamp, value)
SELECT NOW() - (interval '15 minutes' * s), 45 + random() * 20
FROM generate_series(0, 95) s;

INSERT INTO temp_bat (timestamp, value)
SELECT NOW() - (interval '15 minutes' * s), 30 + random() * 15
FROM generate_series(0, 95) s;

INSERT INTO volt_panel (timestamp, value)
SELECT NOW() - (interval '15 minutes' * s), 17 + random() * 4
FROM generate_series(0, 95) s;

INSERT INTO amp_panel (timestamp, value)
SELECT NOW() - (interval '15 minutes' * s), 2 + random() * 3
FROM generate_series(0, 95) s;

INSERT INTO volt_bat (timestamp, value)
SELECT NOW() - (interval '15 minutes' * s), 12 + random() * 2
FROM generate_series(0, 95) s;

INSERT INTO amp_bat (timestamp, value)
SELECT NOW() - (interval '15 minutes' * s), 1 + random() * 5
FROM generate_series(0, 95) s;

INSERT INTO volt_load (timestamp, value)
SELECT NOW() - (interval '15 minutes' * s), 11.5 + random() * 2
FROM generate_series(0, 95) s;

INSERT INTO amp_load (timestamp, value)
SELECT NOW() - (interval '15 minutes' * s), 0.5 + random() * 2
FROM generate_series(0, 95) s;
