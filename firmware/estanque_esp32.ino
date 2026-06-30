#include <WiFi.h>
#include <PubSubClient.h>

// Definición de credenciales de red
// Se necesita la red WiFi y el broker MQTT para la comunicación IoT
const char* ssid = "edgar";
const char* password = "12345678";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);

// Definición de pines para los LEDs (Hardware)
// 12: LED de llenado, 13: LED de vaciado, 14: LED de estado lleno
const int PIN_LLENAR = 12;
const int PIN_VACIAR = 13;
const int PIN_LLENO  = 14; 

// Variables para llevar la cuenta del nivel del estanque
int contadorLlenado = 0;
int contadorVaciado = 0;

// Función que se ejecuta automáticamente cuando llega un mensaje desde la web
void callback(char* topic, byte* payload, unsigned int length) {
  String comando = "";
  // Convierto los datos recibidos (bytes) a un formato de texto (string)
  for (int i = 0; i < length; i++) comando += (char)payload[i];
  
  Serial.print("--- Comando recibido: ");
  Serial.println(comando);

  // Lógica de llenado: aumenta el contador y maneja los estados de los LEDs
  if (comando == "LLENAR") {
    contadorLlenado++;
    contadorVaciado = 0; // Reinicio el vaciado para evitar conflictos de estado
    Serial.print("Clicks de Llenado: "); Serial.println(contadorLlenado);
    
    // Activo el LED de llenado y apago el de vaciado
    digitalWrite(PIN_LLENAR, HIGH);
    digitalWrite(PIN_VACIAR, LOW);
    
    // Si se completan los 5 pasos, el estanque se marca como lleno
    if (contadorLlenado >= 5) {
      digitalWrite(PIN_LLENAR, LOW); 
      digitalWrite(PIN_LLENO, HIGH); 
      Serial.println(">>> ACCION: ESTANQUE LLENO");
    }
    
  } else if (comando == "VACIAR") {
    // Lógica de vaciado: disminuye el nivel o resetea el estanque
    contadorVaciado++;
    contadorLlenado = 0; // Reinicio el llenado porque estamos vaciando
    Serial.print("Clicks de Vaciado: "); Serial.println(contadorVaciado);
    
    // Enciendo el LED de vaciado y apago el resto
    digitalWrite(PIN_VACIAR, HIGH);
    digitalWrite(PIN_LLENAR, LOW);
    digitalWrite(PIN_LLENO, LOW);
    
    // Si el vaciado llega al máximo (5), reseteo el sistema
    if (contadorVaciado >= 5) {
      digitalWrite(PIN_VACIAR, LOW); 
      Serial.println(">>> ACCION: ESTANQUE VACIADO");
      contadorVaciado = 0; 
    }
  }
}

// Función encargada de intentar reconectarse al servidor MQTT si se cae la red
void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conectar a MQTT...");
    if (client.connect("ESP32_Estanque")) {
      Serial.println(" Conectado!");
      // Me suscribo al canal para escuchar las órdenes desde la web
      client.subscribe("estanque/accion");
    } else {
      // Espero 5 segundos antes de reintentar para no saturar la red
      delay(5000);
    }
  }
}

void setup() {
  // Inicializo comunicación serial para monitorear el ESP32 en el PC
  Serial.begin(115200);
  
  // Configuro los pines como salida para controlar los LEDs
  pinMode(PIN_LLENAR, OUTPUT);
  pinMode(PIN_VACIAR, OUTPUT);
  pinMode(PIN_LLENO, OUTPUT);
  
  // Estado inicial de seguridad: todo apagado
  digitalWrite(PIN_LLENAR, LOW);
  digitalWrite(PIN_VACIAR, LOW);
  digitalWrite(PIN_LLENO, LOW);

  // Inicio la conexión WiFi con los datos definidos arriba
  WiFi.begin(ssid, password);
  Serial.println("\nConectando a WiFi...");
  
  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 20) {
    delay(500);
    Serial.print(".");
    intentos++;
  }
  
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Conectado!");
  }
  
  // Configuro la conexión con el broker MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

// El loop mantiene al ESP32 escuchando permanentemente por nuevos comandos
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // Procesa mensajes entrantes y mantiene la conexión viva
}
