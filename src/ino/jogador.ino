#include <SPI.h>
#include "printf.h"
#include "RF24.h"
#include <Bounce2.h>

#define CE_PIN 7
#define CSN_PIN 8
#define BUTTON_PIN A0
#define TIMEOUT 100

#define DATA 0
#define ACK 1

RF24 radio(CE_PIN, CSN_PIN);
Bounce debouncer = Bounce();

// Configuração do transmissor
uint64_t address[2] = {0x3030303030LL, 0x3030303030LL}; // Alterar para endereço único
uint8_t origem = 2; // ID único para cada transmissor
uint8_t destino = 1; // ID do receptor
byte payload[5] = {0, 1, 2, 3, 4};

int contador = 0;
bool lastButtonState = false;

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
  radio.setChannel(37);
  radio.setPayloadSize(sizeof(payload));
  radio.setAutoAck(false);
  radio.setCRCLength(RF24_CRC_DISABLED);
  radio.setDataRate(RF24_250KBPS);

  radio.openWritingPipe(address[0]);
  radio.openReadingPipe(1, address[1]);

  radio.printPrettyDetails();
  Serial.println("Pressione o botão para enviar dados...");
}

void enviaDados() {
  radio.flush_tx();
  payload[0] = origem;
  payload[1] = destino;
  payload[2] = DATA;
  payload[3] = contador;

  unsigned long inicio = millis();
  while (millis() - inicio < TIMEOUT) {
    radio.startListening();
    delayMicroseconds(50);
    radio.stopListening();
    
    if (!radio.testCarrier()) {
      radio.write(&payload[0], 5);
      
      if (aguardaACK()) {
        Serial.print("Envio confirmado! Contador: ");
        Serial.println(contador);
        return;
      }
    }
    delayMicroseconds(270);
  }
  Serial.println("Falha no envio!");
}

bool aguardaACK() {
  radio.startListening();
  unsigned long inicio = millis();
  
  while (millis() - inicio < TIMEOUT) {
    if (radio.available()) {
      delayMicroseconds(160);
      byte ackData[3];
      radio.read(&ackData[0], 3);
      
      if (ackData[1] == origem && ackData[2] == ACK) {
        return true;
      }
    }
  }
  return false;
}

void loop() {
  debouncer.update();
  bool buttonState = debouncer.read();
  
  if (buttonState == LOW && lastButtonState == HIGH) {
    contador++;
    Serial.print("Botão pressionado! Enviando: ");
    Serial.println(contador);
    enviaDados();
    delay(500); // Debounce artificial
  }
  
  lastButtonState = buttonState;
}