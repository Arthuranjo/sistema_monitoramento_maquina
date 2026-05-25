#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

// ================= DEFINES =================
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

// ================= VARIÁVEIS =================
float currentTemp = 25.0;
float currentVibration = 1.0;

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
  for (int i = 0; i < length; i++) msg += (char)payload[i];

  String topicStr = String(topic);

  Serial.print("Mensagem recebida: ");
  Serial.println(msg);

  if (topicStr == "industria4/tempMax") {
    maxTemp = msg.toFloat();
  }

  else if (topicStr == "industria4/tempMin") {
    minTemp = msg.toFloat();
  }

  else if (topicStr == "industria4/vibLimit") {
    vibrationLimit = msg.toFloat();
  }

  else if (topicStr == "industria4/buzzer") {
    ledcWriteTone(BUZZER_PIN, msg == "on" ? 1000 : 0);
  }

  else if (topicStr == "industria4/control/temp") {

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("CONTROLANDO");
    display.println("TEMPERATURA");
    display.display();

    delay(3000);

    tempControlled = true;
    tempControlTime = millis();
    currentTemp = 30.0;
    tempLocked = true;

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    lastSensorTemp = temp.temperature;
  }

  else if (topicStr == "industria4/control/vib") {

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("CONTROLANDO");
    display.println("VIBRACAO");
    display.display();

    delay(3000);

    vibControlled = true;
    vibControlTime = millis();
    currentVibration = 1.2;
    vibLocked = true;

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    lastSensorVib = sqrt(
      a.acceleration.x * a.acceleration.x +
      a.acceleration.y * a.acceleration.y +
      a.acceleration.z * a.acceleration.z
    ) / 9.81;
  }
}

// ================= WIFI =================
void setup_wifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
}

// ================= MQTT =================
void reconnectMQTT() {

  while (!client.connected()) {

    String clientId = "ESP32Client-" + String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {

      client.subscribe("industria4/tempMax");
      client.subscribe("industria4/tempMin");
      client.subscribe("industria4/vibLimit");
      client.subscribe("industria4/buzzer");
      client.subscribe("industria4/control/temp");
      client.subscribe("industria4/control/vib");

    } else {
      delay(3000);
    }
  }
}

// ================= SETUP =================
void setup() {

  Serial.begin(115200);
  setup_wifi();

  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  pinMode(BUZZER_PIN, OUTPUT);
  ledcAttach(BUZZER_PIN, 2000, 8);

  Wire.begin(21, 22);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextSize(1);
  display.setTextColor(WHITE);

  mpu.begin();
}

// ================= LOOP =================
void loop() {

  if (!client.connected()) reconnectMQTT();
  client.loop();

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  if (tempControlled && millis() - tempControlTime > CONTROL_DURATION)
    tempControlled = false;

  if (vibControlled && millis() - vibControlTime > CONTROL_DURATION)
    vibControlled = false;

  // ================= TEMPERATURA =================
  if (!tempControlled) {
    float sensorTemp = temp.temperature;

    if (tempLocked) {
      if (abs(sensorTemp - lastSensorTemp) > 2.0) tempLocked = false;
    } else {
      currentTemp = sensorTemp;
    }
  }

  // ================= VIBRAÇÃO =================
  if (!vibControlled) {
    float sensorVib = sqrt(
      a.acceleration.x * a.acceleration.x +
      a.acceleration.y * a.acceleration.y +
      a.acceleration.z * a.acceleration.z
    ) / 9.81;

    if (vibLocked) {
      if (abs(sensorVib - lastSensorVib) > 0.2) vibLocked = false;
    } else {
      currentVibration = sensorVib;
    }
  }

  bool vibrationAlert = currentVibration > vibrationLimit;
  bool highTempAlert = currentTemp > maxTemp;
  bool lowTempAlert = currentTemp < minTemp;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Monitoramento");

  display.setCursor(0, 18);
  display.print("Temp: ");
  display.println(currentTemp);

  display.setCursor(0, 33);
  display.print("Vib: ");
  display.println(currentVibration);

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

  if (vibrationAlert || highTempAlert || lowTempAlert) {
    ledcWriteTone(BUZZER_PIN, 1000);

    display.setCursor(0, 50);
    if (vibrationAlert) display.println("ALERTA: VIB");
    else if (highTempAlert) display.println("TEMP ALTA");
    else display.println("TEMP BAIXA");

  } else {
    ledcWriteTone(BUZZER_PIN, 0);
    display.setCursor(0, 50);
    display.println("STATUS NORMAL");
  }

  display.display();
  delay(500);
}