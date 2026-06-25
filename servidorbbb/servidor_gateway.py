# -*- coding: utf-8 -*-
import json
import math
import logging
import paho.mqtt.client as mqtt
from urllib.request import Request, urlopen

# ==========================================
# CONFIGURAÇÕES E CREDENCIAIS
# ==========================================
SUPABASE_URL = "https://bfhvrgqrjqgxjpflxzrd.supabase.co/rest/v1/clima_ambiente"
SUPABASE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImJmaHZyZ3FyanFneGpwZmx4enJkIiwicm9sZSI6InNlcnZpY2Vfcm9sZSIsImlhdCI6MTc4MTExNTYyOSwiZXhwIjoyMDk2NjkxNjI5fQ.bjcWiO0Xd09C6pCfsEWXXBgZ-P_pXTMucuCyYTpTU-4"
MQTT_BROKER = "localhost"
MQTT_PORT = 1883
TOPICS = ["grupo2/clima", "zigbee2mqtt/#"]

# Configuração de Logs para facilitar o Debug no VS Code
logging.basicConfig(level=logging.INFO, format='%(asctime)s - [%(levelname)s] - %(message)s')

def calcular_ponto_orvalho(t, h):
    """Calcula o Dew Point (Ponto de Orvalho)."""
    a, b = 17.625, 243.04
    h = max(h, 0.1)
    alpha = ((a * t) / (b + t)) + math.log(h / 100.0)
    return round((b * alpha) / (a - alpha), 2)

def salvar_na_nuvem(temp, umid, orvalho, origem):
    """Envia dados para o Supabase."""
    try:
        dados = json.dumps({"temperatura": temp, "umidade": umid, "ponto_orvalho": orvalho}).encode('utf-8')
        req = Request(SUPABASE_URL, data=dados)
        req.add_header('apikey', SUPABASE_KEY.strip())
        req.add_header('Authorization', 'Bearer ' + SUPABASE_KEY.strip())
        req.add_header('Content-Type', 'application/json')
        req.add_header('Prefer', 'return=minimal')
        urlopen(req).read()
        logging.info(f"Nuvem [Origem: {origem}] -> T:{temp}°C U:{umid}% DP:{orvalho}°C")
    except Exception as e:
        logging.error(f"Erro na API Supabase: {e}")

def on_message(client, userdata, msg):
    """Callback do MQTT que processa dados do ESP32 ou Zigbee."""
    try:
        payload = msg.payload.decode("utf-8")
        dados = json.loads(payload)
        t, h, orig = 0.0, 0.0, ""

        if msg.topic == "grupo2/clima":
            t, h, orig = float(dados["temperatura"]), float(dados["umidade"]), "ESP32"
        elif "zigbee2mqtt" in msg.topic and "bridge" not in msg.topic:
            t, h, orig = float(dados.get("temperature", 0)), float(dados.get("humidity", 0)), "Zigbee"
        else:
            return
        
        if t != 0 or h != 0:
            salvar_na_nuvem(t, h, calcular_ponto_orvalho(t, h), orig)
            
    except Exception as e:
        logging.warning(f"Erro no processamento da mensagem ({msg.topic}): {e}")

# ==========================================
# EXECUÇÃO DO GATEWAY
# ==========================================
if __name__ == "__main__":
    client = mqtt.Client()
    client.on_connect = lambda c, u, f, rc: [client.subscribe(t) for t in TOPICS]
    client.on_message = on_message
    
    logging.info("Gateway IoT Iniciado. Aguardando sensores...")
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.loop_forever()