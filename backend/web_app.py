import os
import json
import psycopg2
import csv
import zipfile
import io
from datetime import datetime
from flask import Flask, render_template, jsonify, request, Response
from dotenv import load_dotenv

load_dotenv()

app = Flask(__name__)

DB_HOST = os.getenv('DB_HOST', 'postgres')
DB_PORT = int(os.getenv('DB_PORT', '5432'))
DB_NAME = os.getenv('DB_NAME', 'sensors_db')
DB_USER = os.getenv('DB_USER', 'postgres')
DB_PASSWORD = os.getenv('DB_PASSWORD', 'postgres')

def get_db_connection():
    return psycopg2.connect(
        host=DB_HOST,
        port=DB_PORT,
        database=DB_NAME,
        user=DB_USER,
        password=DB_PASSWORD
    )

def init_db():
    tables = {
        'temp_panel': 'Temperatura Panel',
        'temp_bat': 'Temperatura Batería',
        'volt_panel': 'Voltaje Panel',
        'amp_panel': 'Corriente Panel',
        'volt_bat': 'Voltaje Batería',
        'amp_bat': 'Corriente Batería',
        'volt_load': 'Voltaje Carga',
        'amp_load': 'Corriente Carga'
    }
    conn = None
    cursor = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor()
        for table_name in tables.keys():
            cursor.execute(f"""
                CREATE TABLE IF NOT EXISTS {table_name} (
                    id SERIAL PRIMARY KEY,
                    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                    value FLOAT
                );
                CREATE INDEX IF NOT EXISTS idx_{table_name}_timestamp
                ON {table_name}(timestamp);
            """)
        conn.commit()
    except Exception as e:
        print(f"Error inicializando BD: {e}")
    finally:
        if cursor:
            cursor.close()
        if conn:
            conn.close()

# Initialize DB on app startup
init_db()

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/tables', methods=['GET'])
def get_tables():
    conn = None
    cursor = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor()
        
        # We only want tables that are likely sensor data (exclude system tables etc)
        # Assuming all sensor tables are in the public schema
        cursor.execute("""
            SELECT table_name
            FROM information_schema.tables
            WHERE table_schema = 'public'
        """)
        tables = cursor.fetchall()
        
        table_info = []
        for (table_name,) in tables:
            cursor.execute(f"SELECT COUNT(*) FROM {table_name}")
            row_count = cursor.fetchone()[0]
            
            cursor.execute(f"SELECT pg_size_pretty(pg_total_relation_size('{table_name}'))")
            size = cursor.fetchone()[0]
            
            table_info.append({
                'name': table_name,
                'row_count': row_count,
                'size': size
            })
            
        return jsonify(table_info)
    except Exception as e:
        return jsonify({'error': str(e)}), 500
    finally:
        if cursor:
            cursor.close()
        if conn:
            conn.close()

@app.route('/api/download/<table_name>', methods=['GET'])
def download_data(table_name):
    start = request.args.get('start')
    end = request.args.get('end')
    format_type = request.args.get('format', 'csv').lower()
    
    if not start or not end:
        return "Missing start or end date", 400
        
    conn = None
    cursor = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor()
        
        clean_start = str(start).replace(':', '-').replace(' ', '_')
        clean_end = str(end).replace(':', '-').replace(' ', '_')

        # Caso: Descargar todas las tablas en un archivo ZIP
        if table_name.lower() == "all":
            cursor.execute("""
                SELECT table_name
                FROM information_schema.tables
                WHERE table_schema = 'public'
            """)
            tables = [r[0] for r in cursor.fetchall()]
            
            zip_buffer = io.BytesIO()
            with zipfile.ZipFile(zip_buffer, 'w', zipfile.ZIP_DEFLATED) as zf:
                for t in tables:
                    cursor.execute(f"SELECT id, timestamp, value FROM {t} WHERE timestamp >= %s AND timestamp <= %s ORDER BY timestamp ASC", (start, end))
                    rows = cursor.fetchall()
                    if format_type == "json":
                        records = [
                            {
                                "id": r[0],
                                "timestamp": r[1].isoformat() if hasattr(r[1], "isoformat") else str(r[1]),
                                "value": float(r[2]) if r[2] is not None else None
                            }
                            for r in rows
                        ]
                        zf.writestr(f"{t}.json", json.dumps(records, indent=2))
                    else:
                        csv_io = io.StringIO()
                        writer = csv.writer(csv_io)
                        writer.writerow(["id", "timestamp", "value"])
                        for r in rows:
                            writer.writerow([r[0], r[1], r[2]])
                        zf.writestr(f"{t}.csv", csv_io.getvalue())
                        
            cursor.close()
            conn.close()
            zip_buffer.seek(0)
            ext_label = "JSON" if format_type == "json" else "CSV"
            response = Response(zip_buffer.getvalue(), mimetype="application/zip")
            response.headers.set("Content-Disposition", f"attachment; filename=EMS_Telemetria_Todas_Tablas_{clean_start}_to_{clean_end}_{ext_label}.zip")
            return response
        
        # Verify table name to prevent SQL injection
        cursor.execute("""
            SELECT table_name
            FROM information_schema.tables
            WHERE table_schema = 'public' AND table_name = %s
        """, (table_name,))
        if not cursor.fetchone():
            return "Invalid table name", 400
            
        # Execute query for data
        query = f"SELECT id, timestamp, value FROM {table_name} WHERE timestamp >= %s AND timestamp <= %s ORDER BY timestamp ASC"
        cursor.execute(query, (start, end))
        
        clean_start = str(start).replace(':', '-').replace(' ', '_')
        clean_end = str(end).replace(':', '-').replace(' ', '_')

        if format_type == 'json':
            def generate_json():
                try:
                    yield chr(91) + chr(10)
                    first = True
                    for row in cursor:
                        if not first:
                            yield chr(44) + chr(10)
                        first = False
                        record = {
                            "id": row[0],
                            "timestamp": row[1].isoformat() if hasattr(row[1], "isoformat") else str(row[1]),
                            "value": float(row[2]) if row[2] is not None else None
                        }
                        yield "  " + json.dumps(record)
                    yield chr(10) + chr(93)
                finally:
                    cursor.close()
                    conn.close()
            response = Response(generate_json(), mimetype="application/json")
            response.headers.set("Content-Disposition", f"attachment; filename={table_name}_{clean_start}_to_{clean_end}.json")
            return response
        else:
            def generate_csv():
                try:
                    yield "id,timestamp,value" + chr(10)
                    for row in cursor:
                        yield f"{row[0]},{row[1]},{row[2]}" + chr(10)
                finally:
                    cursor.close()
                    conn.close()
            response = Response(generate_csv(), mimetype="text/csv")
            response.headers.set("Content-Disposition", f"attachment; filename={table_name}_{clean_start}_to_{clean_end}.csv")
            return response
    except Exception as e:
        if 'cursor' in locals() and cursor:
            cursor.close()
        if 'conn' in locals() and conn:
            conn.close()
        return str(e), 500

@app.route('/api/serial-data', methods=['POST'])
def receive_serial_data():
    data = request.json
    if not data:
        return jsonify({'error': 'No JSON received'}), 400
        
    conn = None
    cursor = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor()
        
        timestamp = datetime.now()
        metricas_validas = {
            'temp_panel', 'temp_bat', 'volt_panel', 'amp_panel',
            'volt_bat', 'amp_bat', 'volt_load', 'amp_load'
        }
        
        inserted = 0
        for metric, value in data.items():
            if metric in metricas_validas and isinstance(value, (int, float)):
                cursor.execute(
                    f"INSERT INTO {metric} (timestamp, value) VALUES (%s, %s)",
                    (timestamp, float(value))
                )
                inserted += 1
                
        conn.commit()
        return jsonify({'status': 'success', 'inserted': inserted}), 200
        
    except Exception as e:
        if conn:
            conn.rollback()
        return jsonify({'error': str(e)}), 500
    finally:
        if cursor:
            cursor.close()
        if conn:
            conn.close()

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
