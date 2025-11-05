#include <SPI.h>
#include "printf.h"
#include "RF24.h"
#include <ArduinoJson.h>

#define CE_PIN 7
#define CSN_PIN 8

// Estados do jogo
enum EGameStatus {
  AWAITING = 1,
  IN_PROGRESS = 2,
  FINISHED = 3
};

RF24 radio(CE_PIN, CSN_PIN);

uint64_t address[2] = {0x3030303030LL, 0x3030303031LL};
uint8_t origem = 1;
byte payloadRX[32];

// Contadores e estado do jogo
int contadorP1 = 0;
int contadorP2 = 0;
EGameStatus gameStatus = AWAITING;
int winner = 0;
unsigned long lastStatusSend = 0;
bool resetSent = false;

void setup() {
  Serial.begin(115200);
  
  if (!radio.begin()) {
    Serial.println(F("Radio hardware not responding!!"));
    while (1) {}
  }

  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(76);
  radio.setPayloadSize(sizeof(payloadRX));
  radio.setAutoAck(false);
  radio.setCRCLength(RF24_CRC_DISABLED);
  radio.setDataRate(RF24_250KBPS);

  radio.openWritingPipe(address[0]);
  radio.openReadingPipe(1, address[0]); // Player 1
  radio.openReadingPipe(2, address[1]); // Player 2

  Serial.println("esperando começar");
}

void enviaReset() {
  // Envia comando de reset para ambos os players
  byte resetCmd[5] = {origem, 0, 2, 0, 0}; // [origem, broadcast, RESET, 0, 0]
  
  radio.flush_tx();
  
  unsigned long inicio = millis();
  while (millis() - inicio < 200) {
    radio.startListening();
    delayMicroseconds(50);
    radio.stopListening();
    
    if (!radio.testCarrier()) {
      radio.write(&resetCmd, sizeof(resetCmd));
      Serial.println("Reset enviado para players");
      resetSent = true;
      break;
    }
    delayMicroseconds(200);
  }
}

void enviaACK(uint8_t destino) {
  byte ack[3] = {origem, destino, 1};
  radio.flush_tx();
  radio.write(&ack, sizeof(ack));
}

void calculaVencedor() {
  if (contadorP1 > contadorP2) {
    winner = 1;
  } else if (contadorP2 > contadorP1) {
    winner = 2;
  } else {
    winner = 0; // Empate
  }
}

void enviaJSONSerial() {
  calculaVencedor();
  
  StaticJsonDocument<512> doc;
  doc["status"] = gameStatus;
  
  JsonArray players = doc.createNestedArray("players");
  
  JsonObject player1 = players.createNestedObject();
  player1["id"] = 1;
  player1["clicks"] = contadorP1;
  
  JsonObject player2 = players.createNestedObject();
  player2["id"] = 2;
  player2["clicks"] = contadorP2;
  
  doc["winner"] = winner;
  
  String jsonString;
  serializeJson(doc, jsonString);
  Serial.println(jsonString);
}

void processaComandoSerial() {
  if (Serial.available()) {
    String comando = Serial.readStringUntil('\n');
    comando.trim();
    
    if (comando == "1") {
      gameStatus = AWAITING;
      resetSent = false;
      Serial.println("esperando começar");
    } else if (comando == "2") {
      gameStatus = IN_PROGRESS;
      // Zera contadores localmente
      contadorP1 = 0;
      contadorP2 = 0;
      winner = 0;
      // Envia reset para players
      enviaReset();
      enviaJSONSerial(); // Envia estado inicial
    } else if (comando == "3") {
      gameStatus = FINISHED;
      enviaJSONSerial();
    }
  }
}

void processaPacoteRF() {
  if (radio.available()) {
    byte pacote[5];
    radio.read(&pacote, sizeof(pacote));
    
    // Só processa se o jogo estiver em progresso
    if (gameStatus == IN_PROGRESS && pacote[1] == origem && pacote[2] == 0) {
      uint8_t remetente = pacote[0];
      int contador = pacote[3];
      
      if (remetente == 3) { // Player 1
        contadorP1 = contador;
      } else if (remetente == 2) { // Player 2
        contadorP2 = contador;
      }
      
      enviaACK(remetente);
      enviaJSONSerial(); // Atualiza JSON a cada clique
    }
  }
}

void loop() {
  // Processa comandos do Serial (juiz)
  processaComandoSerial();
  
  // No estado AWAITING, não faz nada com o RF
  if (gameStatus == AWAITING) {
    // Apenas envia mensagem periódica
    if (millis() - lastStatusSend > 3000) {
      Serial.println("esperando começar");
      lastStatusSend = millis();
    }
    delay(100);
    return;
  }
  
  // Nos estados IN_PROGRESS e FINISHED, escuta o RF
  radio.startListening();
  processaPacoteRF();
  
  delay(50);
}