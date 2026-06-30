from flask import Flask, render_template, redirect, url_for, make_response
from supabase import create_client
import datetime
import paho.mqtt.client as mqtt

app = Flask(__name__)

# Configuración de Supabase
SUPABASE_URL = "https://qkxypwhqwyodrlpggcqf.supabase.co"
SUPABASE_KEY = "REEMPLAZAR_CON_TU_LLAVE"
supabase = create_client(SUPABASE_URL, SUPABASE_KEY)

# Configuración MQTT
MQTT_BROKER = "broker.hivemq.com"
MQTT_TOPIC = "estanque/accion"
client = mqtt.Client()
client.connect(MQTT_BROKER, 1883, 60)
client.loop_start()

# Variable global para rastrear el nivel del estanque
nivel_actual = 0

@app.route('/')
def index():
    # Enviamos 'nivel' al template para que la barra sepa cuánto medir
    resp = make_response(render_template('index.html', nivel=nivel_actual))
    resp.headers['Refresh'] = '3'
    return resp

@app.route('/accion/<comando>')
def accion(comando):
    global nivel_actual
    
    # Lógica de nivel (0 a 5)
    if comando == "LLENAR" and nivel_actual < 5:
        nivel_actual += 1
    elif comando == "VACIAR" and nivel_actual > 0:
        nivel_actual -= 1
    
    # 1. Registrar en Supabase con hora ajustada a Chile (UTC - 4 horas)
    hora_chile = datetime.datetime.utcnow() - datetime.timedelta(hours=4)
    data = {"evento": comando, "fecha": hora_chile.isoformat()}
    supabase.table("eventos_estanque").insert(data).execute()
    
    # 2. Enviar comando por MQTT
    client.publish(MQTT_TOPIC, comando)
    print(f"COMANDO ENVIADO POR MQTT: {comando}")
    
    return redirect(url_for('index'))

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
