#include "RF24.h"
#include "printf.h"
#include <ArduinoJson.h>
#include <SPI.h>

#define CE_PIN 7
#define CSN_PIN 8

// Estados do jogo
enum EGameStatus { AWAITING = 1, IN_PROGRESS = 2, FINISHED = 3 };

#define SYN 100
#define SYN_ACK 101
#define ACK 102

#define TIMEOUT 1000

RF24 radio(CE_PIN, CSN_PIN);

#define ORIGEM 3
#define PLAYER_1 45
#define PLAYER_2 9

uint64_t address[2] = {0x3030303030LL, 0x3030303031LL};
byte payloadRX[32];

// Contadores e estado do jogo
int contadorP1 = 0;
int contadorP2 = 0;

int diferencialP1 = -1;
int diferencialP2 = -1;

EGameStatus gameStatus = AWAITING;

int winner = -1;

unsigned long lastStatusSend = 0;
bool resetSent = false;

void setup() {
  Serial.begin(115200);

  if (!radio.begin()) {
    Serial.println(F("Radio hardware not responding!!"));
    while (1) {
    }
  }

  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(13);
  radio.setPayloadSize(sizeof(payloadRX));
  radio.setAutoAck(false);
  radio.setCRCLength(RF24_CRC_DISABLED);
  radio.setDataRate(RF24_250KBPS);

  radio.openWritingPipe(address[0]);
  radio.openReadingPipe(1, address[0]); // Player 1
  radio.openReadingPipe(2, address[1]); // Player 2

  Serial.println("esperando começar");
}

int escutaHandShake(int comandoEsperado, uint8_t playerAddress) {
  // loop até receber o ack ou o timeout estourar
  int recebidoComSucesso = 0;
  radio.startListening();
  unsigned long inicio = millis();
  while (millis() - inicio < TIMEOUT && recebidoComSucesso == 0) {
    if (radio.available()) {
      byte pacote[4];
      radio.read(&pacote, sizeof(pacote));

      uint8_t remetente = pacote[0];
      uint8_t destino = pacote[1];
      int comando = pacote[2];

      Serial.print("Recebido ");
      Serial.print(pacote[2]);
      Serial.print(" de ");
      Serial.println(remetente);

      if (remetente != playerAddress || destino != ORIGEM) {
          Serial.print("Origem ");
          Serial.print(remetente);
          Serial.print(" ou destino ");
          Serial.print(destino);
          Serial.print(" inválido");
          Serial.println();
        continue;
      }

      if (comando == comandoEsperado) {
          Serial.print("Comando ");
          Serial.print(comando);
          Serial.print(" recebido com sucesso");
          Serial.println();
        recebidoComSucesso = 1;
      }
    }
  }
  return recebidoComSucesso;
}

int resetPlayer(int playerAddress) {
  byte resetCmd[4] = {ORIGEM, playerAddress, 2,
                      0}; // {origem, endereço, comando, dados}
  // THREE_HAND_SHAKE_CODE
  int conectado = 0;
  unsigned long iniciof = millis();
  int estado = 0;
  while (conectado == 0 && millis() - iniciof < 10000) {
    radio.flush_tx();
    unsigned long inicio = millis();

    while (millis() - inicio < 100) {
      radio.startListening();
      delayMicroseconds(40);
      radio.stopListening();

      if (!radio.testCarrier()) {
        if (estado == 0) {
          resetCmd[2] = SYN;
        } else if (estado == 1) {
          resetCmd[2] = ACK;
        }
        radio.write(&resetCmd, sizeof(resetCmd));

        Serial.print("Enviado ");
        Serial.print(resetCmd[2]);
        Serial.print(" para player ");
        Serial.println(playerAddress);

        break;
      }
      delayMicroseconds(200);
    }

    if (estado == 0) {
      int resultado = escutaHandShake(SYN_ACK, playerAddress);
      if (resultado == 1) {
        estado = 1;
        continue;
      }
    } else if (estado == 1) {
      conectado = 1;
    }
  }

  return conectado;
}

int enviaReset() {
  int resultado = resetPlayer(PLAYER_1);
  if (resultado == 0) {
    Serial.println("Erro ao resetar player 1");
    return 0;
  }

  delay(2000);

  resultado = resetPlayer(PLAYER_2);
  if (resultado == 0) {
    Serial.println("Erro ao resetar player 2");
    return 0;
  }
  return 1;
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
  StaticJsonDocument<512> doc;
  doc["status"] = gameStatus;

  JsonArray players = doc.createNestedArray("players");

  JsonObject player1 = players.createNestedObject();
  player1["id"] = 1;
  player1["clicks"] = contadorP1;

  JsonObject player2 = players.createNestedObject();
  player2["id"] = 2;
  player2["clicks"] = contadorP2;

  if(winner != -1) doc["winner"] = winner;

  String jsonString;
  serializeJson(doc, jsonString);
  Serial.println(jsonString);
}

void resetaJogo() {
  contadorP1 = 0;
  contadorP2 = 0;

  // reseta os diferenciais
  diferencialP1 = -1;
  diferencialP2 = -1;
  winner = -1;
}

void processaComandoSerial() {
  if (Serial.available()) {
    String comando = Serial.readStringUntil('\n');
    comando.trim();

    if (comando == "1") {
      resetaJogo();
      gameStatus = AWAITING;
      Serial.println("esperando começar");
    } else if (comando == "2") {
        resetaJogo();
        int resultado = enviaReset();
        if(resultado == 0) {
            return;
        }
      gameStatus = IN_PROGRESS;
      resetaJogo();
      enviaJSONSerial(); // Envia estado inicial
    } else if (comando == "3") {
      gameStatus = FINISHED;
      calculaVencedor();
      enviaJSONSerial();
    }
  }
}

void processaPacoteRF() {
  if (radio.available()) {
    byte pacote[4];
    radio.read(&pacote, sizeof(pacote));

    // Só processa se o jogo estiver em progresso
    if (gameStatus == IN_PROGRESS && pacote[1] == ORIGEM && pacote[2] == 0) {
      uint8_t remetente = pacote[0];
      int contador = pacote[3];

      if (remetente == PLAYER_1) { // Player 1
        if(diferencialP1 == -1) diferencialP1 = contador;
        contadorP1 = contador - diferencialP1;
      } else if (remetente == PLAYER_2) { // Player 2
          if(diferencialP2 == -1) diferencialP2 = contador;
          contadorP2 = contador - diferencialP2;
      }

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
