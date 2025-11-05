#include <SPI.h>
#include "printf.h"
#include "RF24.h"
#include <Bounce2.h>

#define CE_PIN 7
#define CSN_PIN 8
#define BUTTON_PIN A0

RF24 radio(CE_PIN, CSN_PIN);
Bounce debouncer = Bounce();

uint64_t address[2] = {0x3030303030LL, 0x3030303030LL};
uint8_t origem = 3;
uint8_t destino = 1;

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

void processaComandoRF() {
  if (radio.available()) {
    byte pacote[5];
    radio.read(&pacote, sizeof(pacote));
    
    // Verifica se é comando de reset (tipo 2)
    if (pacote[2] == 2) {
      contador = 0;
      jogoAtivo = true;
      Serial.println("Jogo iniciado! Contador zerado.");
    }
    
    // Verifica se é ACK para nossos dados
    else if (pacote[1] == origem && pacote[2] == 1) {
      // ACK recebido, nada a fazer
    }
  }
}

void enviaDados() {
  if (!jogoAtivo) return;
  
  byte pacote[5] = {origem, destino, 0, contador, 0};
  
  radio.flush_tx();
  
  unsigned long inicio = millis();
  while (millis() - inicio < 100) {
    radio.startListening();
    delayMicroseconds(50);
    radio.stopListening();
    
    if (!radio.testCarrier()) {
      bool success = radio.write(&pacote, sizeof(pacote));
      
      if (success) {
        radio.startListening();
        delayMicroseconds(300);
        radio.stopListening();
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
    enviaDados();
  }
  
  delay(10);
}