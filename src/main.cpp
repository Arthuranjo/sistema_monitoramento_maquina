#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <Arduino.h>
//teste
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define BUZZER_PIN 18

// WiFi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ================= EMQX CLOUD MQTT =================
const char* mqtt_server = "hfb72712.ala.eu-central-1.emqxsl.com";
const int mqtt_port = 8883;
const char* mqtt_user = "esp32";
const char* mqtt_pass = "123456";

// TLS client
WiFiClientSecure espClient;
PubSubClient client(espClient);

// Devices
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_MPU6050 mpu;

// Limites
float vibrationLimit = 2.5;
float maxTemp = 40.0;
float minTemp = 20.0;

// ================= MQTT CALLBACK =================
void callback(char* topic, byte* payload, unsigned int length) {

  String msg = "";

  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.print("Mensagem recebida: ");
  Serial.println(msg);

  // ================= TEMPERATURA =================
  if (String(topic) == "industria4/tempMax") {
    maxTemp = msg.toFloat();

    Serial.print("Novo limite temp max: ");
    Serial.println(maxTemp);
  }

  if (String(topic) == "industria4/tempMin") {
    minTemp = msg.toFloat();

    Serial.print("Novo limite temp min: ");
    Serial.println(minTemp);
  }

  // ================= VIBRAÇÃO =================
  if (String(topic) == "industria4/vibLimit") {
    vibrationLimit = msg.toFloat();

    Serial.print("Novo limite vibracao: ");
    Serial.println(vibrationLimit);
  }

  // ================= BUZZER =================
  if (String(topic) == "industria4/buzzer") {

    if (msg == "off") {
      ledcWriteTone(BUZZER_PIN, 0);
    }

    if (msg == "on") {
      ledcWriteTone(BUZZER_PIN, 1000);
    }
  }
}

// ================= WIFI =================
void setup_wifi() {

  delay(10);

  Serial.println();
  Serial.print("Conectando WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// ================= MQTT RECONNECT =================
void reconnectMQTT() {

  while (!client.connected()) {

    Serial.print("Conectando MQTT...");

    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {

      Serial.println(" conectado!");

      client.subscribe("industria4/tempMax");
      client.subscribe("industria4/tempMin");
      client.subscribe("industria4/vibLimit");
      client.subscribe("industria4/buzzer");

      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("MQTT CONECTADO");
      display.display();

      delay(1500);

    } else {

      Serial.print(" falhou, estado=");
      Serial.println(client.state());

      Serial.println(" tentando novamente em 3s");

      delay(3000);
    }
  }
}

// ================= SETUP =================
void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println("INICIANDO SISTEMA");

  setup_wifi();

  // 🔐 necessário para EMQX TLS no Wokwi
  espClient.setInsecure();

  client.setServer(mqtt_server, mqtt_port);

  client.setCallback(callback);

  pinMode(BUZZER_PIN, OUTPUT);

  ledcAttach(BUZZER_PIN, 2000, 8);

  Wire.begin(21, 22);

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {

    Serial.println("Falha OLED");

    while (1);
  }

  display.clearDisplay();

  display.setTextSize(1);

  display.setTextColor(WHITE);

  // MPU6050
  if (!mpu.begin()) {

    Serial.println("MPU nao encontrado");

    display.clearDisplay();

    display.setCursor(0, 0);

    display.println("ERRO MPU6050");

    display.display();

    while (1);
  }

  display.clearDisplay();

  display.setCursor(0, 0);

  display.println("Sistema Iniciado");

  display.display();

  delay(1500);
}

// ================= LOOP =================
void loop() {

  if (!client.connected()) {
    reconnectMQTT();
  }

  client.loop();

  sensors_event_t a, g, temp;

  mpu.getEvent(&a, &g, &temp);

  // Vibração
  float vibration = sqrt(
    (a.acceleration.x * a.acceleration.x) +
    (a.acceleration.y * a.acceleration.y) +
    (a.acceleration.z * a.acceleration.z)
  ) / 9.81;

  // Temperatura
  float temperature = temp.temperature;

  // Alertas
  bool vibrationAlert = vibration > vibrationLimit;
  bool highTempAlert = temperature > maxTemp;
  bool lowTempAlert = temperature < minTemp;

  // OLED
  display.clearDisplay();

  display.setCursor(0, 0);
  display.println("Monitoramento");

  display.setCursor(0, 18);
  display.print("Temp: ");
  display.print(temperature);
  display.println(" C");

  display.setCursor(0, 33);
  display.print("Vib: ");
  display.print(vibration);
  display.println(" g");

  // ================= MQTT SEND =================
  static unsigned long lastMsg = 0;

  if (millis() - lastMsg > 2000) {

    lastMsg = millis();

    char tempString[8];

    dtostrf(temperature, 1, 2, tempString);

    client.publish("industria4/temperatura", tempString);

    char vibString[8];

    dtostrf(vibration, 1, 2, vibString);

    client.publish("industria4/vibracao", vibString);
  }

  // ================= ALERTAS =================
  if (vibrationAlert || highTempAlert || lowTempAlert) {

    // Liga buzzer
    ledcWriteTone(BUZZER_PIN, 1000);

    display.setCursor(0, 50);

    if (vibrationAlert) {

      display.println("ALERTA: VIB");

    } else if (highTempAlert) {

      display.println("TEMP ALTA");

    } else if (lowTempAlert) {

      display.println("TEMP BAIXA");
    }

  } else {

    // Desliga buzzer
    ledcWriteTone(BUZZER_PIN, 0);

    display.setCursor(0, 50);

    display.println("STATUS NORMAL");
  }

  display.display();

  // ================= SERIAL =================
  Serial.print("Temp: ");
  Serial.print(temperature);

  Serial.print(" | Vib: ");
  Serial.print(vibration);

  if (vibrationAlert) {
    Serial.print(" | ALERTA VIB");
  }

  if (highTempAlert) {
    Serial.print(" | ALERTA TEMP ALTA");
  }

  if (lowTempAlert) {
    Serial.print(" | ALERTA TEMP BAIXA");
  }

  Serial.println();

  delay(500);
}