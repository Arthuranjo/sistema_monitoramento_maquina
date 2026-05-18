
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

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define BUZZER_PIN 18

// ================= WIFI =================
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ================= MQTT =================
const char* mqtt_server = "hfb72712.ala.eu-central-1.emqxsl.com";
const int mqtt_port = 8883;
const char* mqtt_user = "esp32";
const char* mqtt_pass = "123456";

// ================= CLIENT =================
WiFiClientSecure espClient;
PubSubClient client(espClient);

// ================= DEVICES =================
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_MPU6050 mpu;

// ================= LIMITES =================
float vibrationLimit = 2.5;
float maxTemp = 40.0;
float minTemp = 20.0;

// ================= VARIÁVEIS CONTROLADAS =================
float currentTemp = 25.0;
float currentVibration = 1.0;

float tempBase = 0;
float vibBase = 0;

float lastSensorTemp = 0;
float lastSensorVib = 0;

bool tempControlled = false;
bool vibControlled = false;

bool tempLocked = false;
bool vibLocked = false;

unsigned long tempControlTime = 0;
unsigned long vibControlTime = 0;

const int CONTROL_DURATION = 5000;

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

  // ================= CONTROLE TEMPERATURA =================
  if (String(topic) == "industria4/control/temp") {

    Serial.println("CONTROLANDO TEMPERATURA");

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("CONTROLANDO");
    display.println("TEMPERATURA");
    display.display();

    delay(3000);

    // simula correção
    tempControlled = true;
    tempControlTime = millis();
    currentTemp = 30.0;
    tempLocked = true;

    currentTemp = 30.0;
    tempLocked = true;

    // 🔥 salva valor REAL do sensor
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    lastSensorTemp = temp.temperature;

    Serial.println("TEMPERATURA NORMALIZADA");
  }

  // ================= CONTROLE VIBRAÇÃO =================
  if (String(topic) == "industria4/control/vib") {

    Serial.println("CONTROLANDO VIBRACAO");

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("CONTROLANDO");
    display.println("VIBRACAO");
    display.display();

    delay(3000);

    // simula redução
    vibControlled = true;
    vibControlTime = millis();

    currentVibration = 1.2;
    vibLocked = true;

    currentVibration = 1.2;
    vibLocked = true;

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    lastSensorVib = sqrt(
      (a.acceleration.x * a.acceleration.x) +
      (a.acceleration.y * a.acceleration.y) +
      (a.acceleration.z * a.acceleration.z)
    ) / 9.81;

    Serial.println("VIBRACAO NORMALIZADA");
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

// ================= MQTT =================
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

      client.subscribe("industria4/control/temp");
      client.subscribe("industria4/control/vib");

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

  espClient.setInsecure();

  client.setServer(mqtt_server, mqtt_port);

  client.setCallback(callback);

  pinMode(BUZZER_PIN, OUTPUT);

  ledcAttach(BUZZER_PIN, 2000, 8);

  Wire.begin(21, 22);

  // ================= OLED =================
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {

    Serial.println("Falha OLED");

    while (1);
  }

  display.clearDisplay();

  display.setTextSize(1);

  display.setTextColor(WHITE);

  // ================= MPU =================
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

    // ================= LIBERA CONTROLE =================
  if (tempControlled && millis() - tempControlTime > CONTROL_DURATION) {
    tempControlled = false;
  }

  if (vibControlled && millis() - vibControlTime > CONTROL_DURATION) {
    vibControlled = false;
  }

  // ================= TEMPERATURA =================
  if (!tempControlled) {

  float sensorTemp = temp.temperature;

  if (tempLocked){

    if (abs(sensorTemp - lastSensorTemp) > 2.0) {
      tempLocked = false;
    }

  } else{
    currentTemp = sensorTemp;
  }
  // só atualiza se houver mudança REAL no sensor
  
  }
  // ================= VIBRAÇÃO =================
  if (!vibControlled) {

  float sensorVib = sqrt(
    (a.acceleration.x * a.acceleration.x) +
    (a.acceleration.y * a.acceleration.y) +
    (a.acceleration.z * a.acceleration.z)
  ) / 9.81;

  if (vibLocked) {

    if (abs(sensorVib - lastSensorVib) > 0.2) {
      vibLocked = false;
      }
    } else {
      currentVibration = sensorVib;
    }
  }

  // ================= ALERTAS =================
  bool vibrationAlert = currentVibration > vibrationLimit;

  bool highTempAlert = currentTemp > maxTemp;

  bool lowTempAlert = currentTemp < minTemp;

  // ================= OLED =================
  display.clearDisplay();

  display.setCursor(0, 0);
  display.println("Monitoramento");

  display.setCursor(0, 18);
  display.print("Temp: ");
  display.print(currentTemp);
  display.println(" C");

  display.setCursor(0, 33);
  display.print("Vib: ");
  display.print(currentVibration);
  display.println(" g");

  // ================= MQTT SEND =================
  static unsigned long lastMsg = 0;

  if (millis() - lastMsg > 2000) {

    lastMsg = millis();

    char tempString[8];

    dtostrf(currentTemp, 1, 2, tempString);

    client.publish("industria4/temperatura", tempString);

    char vibString[8];

    dtostrf(currentVibration, 1, 2, vibString);

    client.publish("industria4/vibracao", vibString);
  }

  // ================= ALERTAS =================
  if (vibrationAlert || highTempAlert || lowTempAlert) {

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

    ledcWriteTone(BUZZER_PIN, 0);

    display.setCursor(0, 50);

    display.println("STATUS NORMAL");
  }

  display.display();

  // ================= SERIAL =================
  Serial.print("Temp: ");
  Serial.print(currentTemp);

  Serial.print(" | Vib: ");
  Serial.print(currentVibration);

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
