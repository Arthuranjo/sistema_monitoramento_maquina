COMANDOS PARA CONECTAR O SERVIDOR EMQX NO PROJETO:

vá na aba online test: 
username: esp32
Password: 123456

Na parte de subscriptions:
Topic: industria4/#

Após isso clique em subscribe

Agora é só rodar o hardware

# Sistema Inteligente de Monitoramento de Máquinas com ESP32

Projeto desenvolvido para monitoramento industrial utilizando ESP32, sensor MPU6050, display OLED e integração MQTT com EMQX Cloud.

## Objetivo

O sistema realiza o monitoramento em tempo real de:

- Temperatura
- Vibração da máquina

Quando valores críticos são detectados:
- O buzzer é acionado
- Alertas aparecem no display OLED
- Os dados são enviados via MQTT para o EMQX Cloud

---

# Tecnologias Utilizadas

- ESP32
- MPU6050
- OLED SSD1306
- MQTT
- EMQX Cloud
- Wokwi
- PlatformIO
- VSCode

---

# Funcionalidades

- Monitoramento contínuo de temperatura
- Monitoramento contínuo de vibração
- Alertas sonoros via buzzer
- Exibição das informações no display OLED
- Envio de dados em tempo real via MQTT
- Integração com EMQX Cloud
- Simulação completa no Wokwi

---

# Estrutura do Projeto

- `src/main.cpp` → código principal do ESP32
- `diagram.json` → configuração do circuito no Wokwi
- `platformio.ini` → configuração do PlatformIO

---

# Bibliotecas Utilizadas

O PlatformIO instala automaticamente as dependências abaixo:

- Adafruit MPU6050
- Adafruit Unified Sensor
- Adafruit GFX Library
- Adafruit SSD1306
- PubSubClient

---

# Como Executar o Projeto

## 1. Instalar programas necessários

- VSCode
- Extensão PlatformIO IDE
- Extensão Wokwi for VSCode

---

## 2. Clonar o repositório

```bash
git clone https://github.com/Arthuranjo/sistema_monitoramento_maquina.git