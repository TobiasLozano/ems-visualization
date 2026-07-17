import os
import psycopg2
import csv
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
    
    if not start or not end:
        return "Missing start or end date", 400
        
    conn = None
    cursor = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor()
        
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
        
        def generate():
            try:
                yield 'id,timestamp,value\n'
                for row in cursor:
                    # row is (id, timestamp, value)
                    yield f"{row[0]},{row[1]},{row[2]}\n"
            finally:
                cursor.close()
                conn.close()
                
        response = Response(generate(), mimetype='text/csv')
        response.headers.set("Content-Disposition", f"attachment; filename={table_name}_{start}_to_{end}.csv")
        return response
        
    except Exception as e:
        if 'cursor' in locals() and cursor:
            cursor.close()
        if 'conn' in locals() and conn:
            conn.close()
        return str(e), 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
