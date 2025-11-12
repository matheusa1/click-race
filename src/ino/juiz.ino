#include "RF24.h"
#include "printf.h"
#include <ArduinoJson.h>
#include <SPI.h>

#define CE_PIN 7
#define CSN_PIN 8


enum EGameStatus {
  AWAITING = 1,
  IN_PROGRESS = 2,
  FINISHED = 3
};

#define SYN 100
#define SYN_ACK 101
#define ACK 102
#define DATA 200
#define FIN 201

#define TIMEOUT 1000

RF24 radio(CE_PIN, CSN_PIN);

#define ORIGEM 3
#define PLAYER_1 45
#define PLAYER_2 9

//address[0] é para o Player 1
//address[1] é para o Player 2
uint64_t address[2] = {0x3030303030LL, 0x3030303031LL};

int contadorP1 = 0;
int contadorP2 = 0;
int diferencialP1 = -1;
int diferencialP2 = -1;
EGameStatus gameStatus = AWAITING;
int winner = -1;
unsigned long lastStatusSend = 0;

void setup() {
  Serial.begin(115200);

  if (!radio.begin()) {
    Serial.println(F("Radio hardware not responding!!"));
    while (1) {}
  }

  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(8);
  radio.setAutoAck(false);
  radio.setCRCLength(RF24_CRC_DISABLED);
  radio.setDataRate(RF24_250KBPS);

  // pipe para cada jogador
  radio.openReadingPipe(1, address[0]);
  radio.openReadingPipe(2, address[1]);

  radio.startListening();
  Serial.println("esperando começar");
}

void sendCommandToPlayer(uint64_t playerPipe, uint8_t playerAddr, int command) {
  radio.stopListening();
  radio.openWritingPipe(playerPipe);

  byte cmdPacket[4] = {ORIGEM, playerAddr, command, 0};

  if (radio.write(&cmdPacket, sizeof(cmdPacket))) {
    Serial.print("Enviado comando ");
    Serial.print(command);
    Serial.print(" para o jogador ");
    Serial.println(playerAddr);
  } else {
    Serial.print("Falha ao enviar comando para o jogador ");
    Serial.println(playerAddr);
  }

  radio.startListening();
}


int escutaHandShake(int comandoEsperado, uint8_t playerAddress) {
  int recebidoComSucesso = 0;
  radio.startListening();
  unsigned long inicio = millis();
  while (millis() - inicio < TIMEOUT && recebidoComSucesso == 0) {
    byte pipeNum;
    if (radio.available(&pipeNum)) {
      byte pacote[4];
      radio.read(&pacote, sizeof(pacote));

      uint8_t remetente = pacote[0];
      uint8_t destino = pacote[1];
      int comando = pacote[2];

      if (remetente != playerAddress || destino != ORIGEM) {
        continue;
      }

      if (comando == comandoEsperado) {
        recebidoComSucesso = 1;
      }
    }
  }
  return recebidoComSucesso;
}

int resetPlayer(uint64_t playerPipe, int playerAddress) {
    int conectado = 0;
    unsigned long iniciof = millis();
    int estado = 0;
    while (conectado == 0 && millis() - iniciof < 5000) {
        radio.stopListening();
        radio.openWritingPipe(playerPipe);

        byte resetCmd[4] = {ORIGEM, (uint8_t)playerAddress, 0, 0};
        if (estado == 0) {
          resetCmd[2] = SYN;
        } else if (estado == 1) {
          resetCmd[2] = ACK;
        }

        if(radio.write(&resetCmd, sizeof(resetCmd))){
            Serial.print("Enviado ");
            Serial.print(resetCmd[2]);
            Serial.print(" para player ");
            Serial.println(playerAddress);
        }

        radio.startListening();

        if (estado == 0) {
          if (escutaHandShake(SYN_ACK, playerAddress) == 1) {
            estado = 1;
          }
        } else if (estado == 1) {
          conectado = 1;
        }
    }
    return conectado;
}

int enviaReset() {
  if (resetPlayer(address[0], PLAYER_1) == 0) {
    Serial.println("Erro no handshake com player 1");
    return 0;
  }
  if (resetPlayer(address[1], PLAYER_2) == 0) {
    Serial.println("Erro no handshake com player 2");
    return 0;
  }
  return 1;
}

void calculaVencedor() {
  if (contadorP1 > contadorP2) {
    winner = PLAYER_1;
  } else if (contadorP2 > contadorP1) {
    winner = PLAYER_2;
  } else {
    winner = 0;
  }
}
void enviaJSONSerial() {
  StaticJsonDocument<512> doc;
  doc["status"] = gameStatus;

  JsonArray players = doc.createNestedArray("players");

  JsonObject player1 = players.createNestedObject();
  player1["id"] = PLAYER_1;
  player1["clicks"] = contadorP1;

  JsonObject player2 = players.createNestedObject();
  player2["id"] = PLAYER_2;
  player2["clicks"] = contadorP2;

  if(winner != -1) doc["winner"] = winner;
  String jsonString;
  serializeJson(doc, jsonString);
  Serial.println(jsonString);
}

void resetaJogo() {
  contadorP1 = 0;
  contadorP2 = 0;
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
        int resultadoHandshake = enviaReset();
        if(resultadoHandshake == 1) {
            Serial.println("Handshakes concluidos. Enviando comando DATA para iniciar o jogo.");
            sendCommandToPlayer(address[0], PLAYER_1, DATA);
            sendCommandToPlayer(address[1], PLAYER_2, DATA);
            gameStatus = IN_PROGRESS;
            enviaJSONSerial();
        }
        else {
          Serial.println("ERRO_NO_HANDSHAKE");
        }
    } else if (comando == "3") {
      Serial.println("Enviando comando FIN para parar o jogo.");
      sendCommandToPlayer(address[0], PLAYER_1, FIN);
      sendCommandToPlayer(address[1], PLAYER_2, FIN);

      gameStatus = FINISHED;
      calculaVencedor();
      enviaJSONSerial();
    }
  }
}

void processaPacoteRF() {
  byte pipeNum;
  if (radio.available(&pipeNum)) {
    byte pacote[4];
    radio.read(&pacote, sizeof(pacote));

    if (gameStatus == IN_PROGRESS && pacote[1] == ORIGEM && pacote[2] == 0) {
      uint8_t remetente = pacote[0];
      int contador = pacote[3];

      if (remetente == PLAYER_1) {
        if(diferencialP1 == -1) diferencialP1 = contador;
        contadorP1 = contador - diferencialP1;
      } else if (remetente == PLAYER_2) {
        if(diferencialP2 == -1) diferencialP2 = contador;
        contadorP2 = contador - diferencialP2;
      }

      enviaJSONSerial();
    }
  }
}

void loop() {
  processaComandoSerial();
  if (gameStatus == AWAITING) {
    if (millis() - lastStatusSend > 3000) {
      Serial.println("esperando começar");
      lastStatusSend = millis();
    }
    return;
  }
  processaPacoteRF();
}
