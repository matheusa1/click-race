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
#define DATA 200
#define FIN 201

#define TIMEOUT 1000

#define JUIZ 3
#define ORIGEM 45

RF24 radio(CE_PIN, CSN_PIN);
Bounce debouncer = Bounce();

//address[0] juiz
//address[1] jogador
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
  radio.setChannel(8);
  radio.setAutoAck(false);
  radio.setCRCLength(RF24_CRC_DISABLED);
  radio.setDataRate(RF24_250KBPS);
  
  radio.openWritingPipe(address[1]);
  radio.openReadingPipe(1, address[0]);

  radio.startListening();
  Serial.println("Player - Aguardando início...");
}

int escutaHandShake(int comandoEsperado) {
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
            if (remetente == JUIZ && destino == ORIGEM && comando == comandoEsperado) {
                recebidoComSucesso = 1;
            }
        }
    }
    return recebidoComSucesso;
}

void enviaDados(int comando, int dado) {
  byte pacote[4] = {ORIGEM, JUIZ, (byte)comando, (byte)dado};
  radio.stopListening();
  
  unsigned long inicio = millis();
        while(millis() - inicio < 200){
          radio.startListening();
          delayMicroseconds(50);
          radio.stopListening();

          if(!radio.testCarrier()){
            if (radio.write(&pacote, sizeof(pacote))) {
              Serial.print("Enviado comando ");
              Serial.print(comando);
              Serial.println(" para o juiz ");
              radio.startListening();
              continue;
            } else {
              Serial.println("Falha no envio.");
            }
          }
          delayMicroseconds(200);
        }

        radio.startListening();

  if (radio.write(&pacote, sizeof(pacote))) {
    Serial.println("Falha no envio.");
  }

  
}

int processaHandShake() {
    enviaDados(SYN_ACK, 0);
    int resultado = escutaHandShake(ACK);
    if(resultado == 1){
      Serial.println("Handshake concluído! Aguardando comando de início (DATA)...");
    } else {
      Serial.println("Handshake falhou!");
    }
    return resultado;
}

void processaComandoRF() {
  if (radio.available()) {
    byte pacote[4];
    radio.read(&pacote, sizeof(pacote));

    uint8_t remetente = pacote[0];
    uint8_t destino = pacote[1];
    int comando = pacote[2];

    Serial.print("Recebido comando ");
      Serial.print(comando);
      Serial.print(" da origem ");
      Serial.println(remetente);

    if (destino != ORIGEM || remetente != JUIZ) {
      return; 
    }

    switch(comando) {
      case SYN:
        processaHandShake();
        break;

      case DATA:
        contador = 0;
        jogoAtivo = true;
        Serial.println("COMANDO DATA RECEBIDO - Jogo iniciado! Contador zerado.");
        break;

      case FIN:
        jogoAtivo = false;
        Serial.println("COMANDO FIN RECEBIDO - Jogo finalizado!");
        break;
    }
  }
}

void loop() {
  processaComandoRF();
  
  debouncer.update();
  if (debouncer.fell() && jogoAtivo) {
    contador++;
    Serial.print("Clique: ");
    Serial.println(contador);
    enviaDados(0, contador); 
  }
}