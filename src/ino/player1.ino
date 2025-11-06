#include <SPI.h>
#include "printf.h"
#include "RF24.h"
#include <Bounce2.h>

#define CE_PIN 7
#define CSN_PIN 8
#define BUTTON_PIN A0

#define SYN 100
#define SYN_ACK 101
#define ACK 102

#define TIMEOUT 1000

#define JUIZ 3
#define ORIGEM 45

RF24 radio(CE_PIN, CSN_PIN);
Bounce debouncer = Bounce();

uint64_t address[2] = {0x3030303030LL, 0x3030303030LL};

int contador = 0;
bool jogoAtivo = false;

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(25);

  if (!radio.begin()) {
    Serial.println(F("Radio hardware not responding!!"));
    while (1) {}
  }

  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(76);
  radio.setPayloadSize(5);
  radio.setAutoAck(false);
  radio.setCRCLength(RF24_CRC_DISABLED);
  radio.setDataRate(RF24_250KBPS);

  radio.openWritingPipe(address[0]);
  radio.openReadingPipe(1, address[1]);

  Serial.println("Player 1 - Aguardando início...");
}

int escutaHandShake(int comandoEsperado) {
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

      if (remetente != JUIZ || destino != ORIGEM) {
        continue;
      }

      if (comando == comandoEsperado) {
        recebidoComSucesso = 1;
      }
    }
  }
  return recebidoComSucesso;
}

int processaHandShake() {
    //envia o SYN_ACK
    enviaDados(SYN_ACK, 0);
    // espera receber o ACK
   int resultado = escutaHandShake(ACK);
   if(resultado == 1){
      Serial.println("Handshake concluído com sucesso!");
   }else{
      Serial.println("Handshake falhou!");
   }
   return resultado;
}

void processaComandoRF() {
  if (radio.available()) {
    byte pacote[4];
    radio.read(&pacote, sizeof(pacote));

    if (pacote[1] == ORIGEM && pacote[2] == SYN) {
      int resultado = processaHandShake();
      if(resultado == 1){
          contador = 0;
          jogoAtivo = true;
          Serial.println("Jogo iniciado! Contador zerado.");
      }#include <SPI.h>
      #include "printf.h"
      #include "RF24.h"
      #include <Bounce2.h>

      #define CE_PIN 7
      #define CSN_PIN 8
      #define BUTTON_PIN A0

      #define SYN 100
      #define SYN_ACK 101
      #define ACK 102

      #define TIMEOUT 1000

      #define JUIZ 3
      #define ORIGEM 45

      RF24 radio(CE_PIN, CSN_PIN);
      Bounce debouncer = Bounce();

      uint64_t address[2] = {0x3030303030LL, 0x3030303030LL};

      int contador = 0;
      bool jogoAtivo = false;

      void setup() {
        Serial.begin(115200);

        pinMode(BUTTON_PIN, INPUT_PULLUP);
        debouncer.attach(BUTTON_PIN);
        debouncer.interval(25);

        if (!radio.begin()) {
          Serial.println(F("Radio hardware not responding!!"));
          while (1) {}
        }

        radio.setPALevel(RF24_PA_MAX);
        radio.setChannel(76);
        radio.setPayloadSize(5);
        radio.setAutoAck(false);
        radio.setCRCLength(RF24_CRC_DISABLED);
        radio.setDataRate(RF24_250KBPS);

        radio.openWritingPipe(address[0]);
        radio.openReadingPipe(1, address[1]);

        Serial.println("Player 1 - Aguardando início...");
      }

      int escutaHandShake(int comandoEsperado) {
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

            if (remetente != JUIZ || destino != ORIGEM) {
              continue;
            }

            if (comando == comandoEsperado) {
              recebidoComSucesso = 1;
            }
          }
        }
        return recebidoComSucesso;
      }

      int processaHandShake() {
          //envia o SYN_ACK
          enviaDados(SYN_ACK, 0);
          // espera receber o ACK
         int resultado = escutaHandShake(ACK);
         if(resultado == 1){
            Serial.println("Handshake concluído com sucesso!");
         }else{
            Serial.println("Handshake falhou!");
         }
         return resultado;
      }

      void processaComandoRF() {
        if (radio.available()) {
          byte pacote[4];
          radio.read(&pacote, sizeof(pacote));

          if (pacote[1] == ORIGEM && pacote[2] == SYN) {
            int resultado = processaHandShake();
            if(resultado == 1){
                contador = 0;
                jogoAtivo = true;
                Serial.println("Jogo iniciado! Contador zerado.");
            }
          }
        }
      }

      void enviaDados(int comando, int dado) {
        byte pacote[4] = {ORIGEM, JUIZ, comando, dado};

        radio.flush_tx();

        unsigned long inicio = millis();
        while (millis() - inicio < 100) {
          radio.startListening();
          delayMicroseconds(50);
          radio.stopListening();

          if (!radio.testCarrier()) {
            bool success = radio.write(&pacote, sizeof(pacote));

            if (success) {
                Serial.print("Enviado ");
                        Serial.print(pacote[2]);
                        Serial.print(" para juiz ");
                        Serial.println(JUIZ);
              return;
            }
          }
          delayMicroseconds(200);
        }
      }

      void loop() {
        // Sempre escuta por comandos do receptor
        radio.startListening();
        processaComandoRF();

        debouncer.update();

        if (debouncer.fell() && jogoAtivo) {
          contador++;
          Serial.print("Clique: ");
          Serial.println(contador);
          enviaDados(0, contador);
        }

        delay(10);
      }

    }
  }
}

void enviaDados(int comando, int dado) {
  byte pacote[4] = {ORIGEM, JUIZ, comando, dado};

  radio.flush_tx();

  unsigned long inicio = millis();
  while (millis() - inicio < 100) {
    radio.startListening();
    delayMicroseconds(50);
    radio.stopListening();

    if (!radio.testCarrier()) {
      bool success = radio.write(&pacote, sizeof(pacote));

      if (success) {
          Serial.print("Enviado ");
                  Serial.print(pacote[2]);
                  Serial.print(" para juiz ");
                  Serial.println(JUIZ);
        return;
      }
    }
    delayMicroseconds(200);
  }
}

void loop() {
  // Sempre escuta por comandos do receptor
  radio.startListening();
  processaComandoRF();

  debouncer.update();

  if (debouncer.fell() && jogoAtivo) {
    contador++;
    Serial.print("Clique: ");
    Serial.println(contador);
    enviaDados(0, contador);
  }

  delay(10);
}
