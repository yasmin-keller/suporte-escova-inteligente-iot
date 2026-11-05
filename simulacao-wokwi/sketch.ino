#include <WiFi.h>
#include <PubSubClient.h>

#define BTN_PIN 4
#define BUZZER_PIN 14
#define LED_R 25
#define LED_G 26
#define LED_B 27

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "broker.hivemq.com";
const char* mqtt_topic = "mackenzie/gabrielyasmin/escova";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

const int TEMPO_ESCOVACAO = 10000; // 10 segundos
bool cicloAtivo = false;
bool modoRepouso = true; // Simula a flag de Deep Sleep

void setup_wifi() {
  Serial.print("A ligar à rede Wi-Fi ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi ligado!");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("A tentar a ligação MQTT...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("Ligado ao broker MQTT!");
    } else {
      Serial.print("Falhou, rc=");
      Serial.print(client.state());
      Serial.println(" Nova tentativa em 2 segundos...");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, LOW);

  Serial.println("Arranque do sistema concluído.");
  Serial.println("Modo de Ultra Baixo Consumo (Deep Sleep) Ativado [Simulado].");
}

void loop() {
  // Lógica para detetar a remoção da escova (Botão pressionado / LOW)
  if (digitalRead(BTN_PIN) == LOW && modoRepouso == true && !cicloAtivo) {
    modoRepouso = false; // "Acorda" do sono simulado
    cicloAtivo = true;
    Serial.println("------------------------------------------");
    Serial.println("Wake-up via GPIO 4! Escova retirada.");
    Serial.println("A iniciar temporizador de escovagem (LED Azul)...");

    unsigned long startTime = millis();
    unsigned long lastBlink = 0;
    bool ledState = false;

    // Temporizador de Escovagem não-bloqueante
    while (millis() - startTime < TEMPO_ESCOVACAO) {
      if (millis() - lastBlink >= 500) {
        lastBlink = millis();
        ledState = !ledState;
        digitalWrite(LED_B, ledState ? HIGH : LOW);
      }
      // Mantém a simulação fluida
      delay(10);
    }

    digitalWrite(LED_B, LOW);
    Serial.println("Tempo concluído! Aviso sonoro ativado.");

    digitalWrite(BUZZER_PIN, HIGH);
    delay(1000);
    digitalWrite(BUZZER_PIN, LOW);

    // Envio da Telemetria MQTT
    Serial.println("A iniciar rádio Wi-Fi para transmissão...");
    setup_wifi();
    client.setServer(mqtt_server, mqtt_port);
    reconnect();

    if (client.connected()) {
      String payload = "{\"evento\":\"escovagem_concluida\", \"duracao_segundos\": " + String(TEMPO_ESCOVACAO/1000) + "}";
      client.publish(mqtt_topic, payload.c_str());
      Serial.println("--> Payload enviado para: " + String(mqtt_topic));
      Serial.println("--> Dados: " + payload);
      delay(500);
    }

    // Desliga o rádio para poupar energia (Desconexão explícita para o projeto)
    client.disconnect();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println("Rádio Wi-Fi desligado.");

    cicloAtivo = false;
  }

  // Verifica se a escova foi devolvida (Botão não pressionado / HIGH) para entrar em repouso novamente
  if (digitalRead(BTN_PIN) == HIGH && !modoRepouso && !cicloAtivo) {
    Serial.println("Escova devolvida. A desativar periféricos...");
    Serial.println("Modo de Ultra Baixo Consumo (Deep Sleep) Ativado [Simulado].");
    modoRepouso = true;
  }

  delay(50); // Estabiliza a leitura do botão principal (debouncing simples)
}
