#include <WiFi.h>
#include <PubSubClient.h>

// Mapeamento de Hardware (Atuadores e Sensores)
#define BTN_PIN 4      // Chave fim de curso (Sensor Eletromecânico)
#define BUZZER_PIN 14  // Buzzer Piezoelétrico (Feedback Sonoro)
#define LED_R 25       // LED RGB - Canal Vermelho
#define LED_G 26       // LED RGB - Canal Verde
#define LED_B 27       // LED RGB - Canal Azul (Feedback Visual)

// Credenciais de Rede (TCP/IP)
// Na implementação física, estas são geridas pela biblioteca WiFiManager
const char* ssid = "SEU_WIFI";
const char* password = "SUA_SENHA";

// Configurações do Protocolo de Comunicação (MQTT)
const char* mqtt_server = "broker.hivemq.com";
const char* mqtt_topic = "mackenzie/gabrielyasmin/escova";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

// Parâmetro de Saúde Bucal (ODS 3)
// Configurado para 120.000 ms (2 minutos). Reduzir na demonstração em vídeo.
const int TEMPO_ESCOVACAO = 120000;

void setup_wifi() {
  Serial.print("Estabelecendo conexão TCP/IP com: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectividade Wi-Fi estabelecida.");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Negociando conexão com Broker MQTT...");
    String clientId = "ESP32-Holder-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("Conexão MQTT bem-sucedida!");
    } else {
      Serial.print("Falha. Código de erro: ");
      Serial.print(client.state());
      Serial.println(" Retentativa em 5 segundos.");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  // Inicialização de GPIOs
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  // Força o estado LOW nos atuadores
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, LOW);

  // Identifica o gatilho de inicialização do microcontrolador
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("Interrupção Externa (EXT0): Escova removida do suporte.");

    // Início do ciclo de escovação guiado
    unsigned long startTime = millis();
    unsigned long lastBlink = 0;
    bool ledState = false;

    while (millis() - startTime < TEMPO_ESCOVACAO) {
      if (millis() - lastBlink >= 500) {
        lastBlink = millis();
        ledState = !ledState;
        digitalWrite(LED_B, ledState ? HIGH : LOW);
      }
      yield(); // Manutenção preventiva do Watchdog Timer
    }

    digitalWrite(LED_B, LOW);
    Serial.println("Tempo de higienização atingido.");

    // Feedback sonoro de conclusão
    digitalWrite(BUZZER_PIN, HIGH);
    delay(1000);
    digitalWrite(BUZZER_PIN, LOW);

    // Ativa rádio e transmite pacote de telemetria
    setup_wifi();
    client.setServer(mqtt_server, mqtt_port);
    reconnect();

    if (client.connected()) {
      String payload = "{\"evento\":\"escovacao_concluida\", \"duracao_segundos\": " + String(TEMPO_ESCOVACAO/1000) + "}";
      client.publish(mqtt_topic, payload.c_str());
      Serial.println("Telemetria MQTT publicada: " + payload);
      delay(500);
    }

    // Desativa módulo de rádio para isolamento energético
    client.disconnect();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  } else {
    Serial.println("Inicialização primária de energia.");
  }

  // Barreira de segurança: impede o Deep Sleep se a escova ainda estiver fora (botão solto/LOW)
  while(digitalRead(BTN_PIN) == LOW){
      delay(100);
      yield();
  }

  Serial.println("Armando gatilho de Wake-up no GPIO 4. Ingressando em Deep Sleep...");
  // Configura a interrupção externa 0 para acordar o chip quando o pino 4 for aterrado (LOW)
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, 0);
  delay(100);
  esp_deep_sleep_start();
}

void loop() {
  // O loop principal é inoperante. A arquitetura é baseada inteiramente
  // em interrupções de hardware e hibernação na função setup().
}
