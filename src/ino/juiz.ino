#include <SPI.h>
#include "printf.h"
#include "RF24.h"

#define CE_PIN 7
#define CSN_PIN 8
#define TIMEOUT 100

#define DATA 0
#define ACK 1

RF24 radio(CE_PIN, CSN_PIN);

// Endereços para os dois transmissores
uint64_t address[2] = {0x3030303030LL, 0x3030303031LL};
uint8_t origem = 1; // ID do receptor
byte payload[5] = {0, 1, 2, 3, 4};
byte payloadRX[5] = {0, 1, 2, 3, 4};

// Variáveis para armazenar os contadores
int contador[2] = {0, 0};

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  if (!radio.begin()) {
    Serial.println(F("Radio hardware not responding!!"));
    while (1) {}
  }

  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(37);
  radio.setPayloadSize(sizeof(payload));
  radio.setAutoAck(false);
  radio.setCRCLength(RF24_CRC_DISABLED);
  radio.setDataRate(RF24_250KBPS);

  // Abre pipes para os dois transmissores
  radio.openWritingPipe(address[0]);
  radio.openReadingPipe(1, address[0]); // Transmissor 1
  radio.openReadingPipe(2, address[1]); // Transmissor 2

  radio.printPrettyDetails();
}

void printPacote(byte *pac, int tamanho) {
  Serial.print(F("Recebido de "));
  Serial.print(pac[0]);
  Serial.print(F(" | Contador: "));
  Serial.print(pac[3]);
  Serial.print(F(" | Dados: "));
  for (int i = 4; i < tamanho; i++) {
    Serial.print(pac[i]);
  }
  Serial.println();
}

void enviaACK(int destino) {
  radio.flush_tx();
  payload[0] = origem;
  payload[1] = destino;
  payload[2] = ACK;
  
  unsigned long inicio = millis();
  while (millis() - inicio < TIMEOUT) {
    radio.startListening();
    delayMicroseconds(50);
    radio.stopListening();
    
    if (!radio.testCarrier()) {
      radio.write(&payload[0], 3);
      return;
    }
    delayMicroseconds(270);
  }
  Serial.println("Timeout ACK!");
}

void loop() {
  // Verifica dados do Transmissor 1
  if (recebeDados(1)) {
    contador[0] = payloadRX[3];
    enviaACK(payloadRX[0]);
  }

  // Verifica dados do Transmissor 2
  if (recebeDados(2)) {
    contador[1] = payloadRX[3];
    enviaACK(payloadRX[0]);
  }

  // Exibe contadores atuais
  Serial.print("Contadores: [");
  Serial.print(contador[0]);
  Serial.print(", ");
  Serial.print(contador[1]);
  Serial.println("]");
  delay(1000);
}

bool recebeDados(uint8_t pipe) {
  radio.startListening();
  unsigned long inicio = millis();
  
  while (millis() - inicio < TIMEOUT) {
    if (radio.available()) {
      delayMicroseconds(160);
      int tamanho = radio.getPayloadSize();
      radio.read(&payloadRX[0], tamanho);

      if (payloadRX[1] == origem && payloadRX[2] == DATA) {
        printPacote(payloadRX, tamanho);
        return true;
      }
    }
  }
  return false;
}