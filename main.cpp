

#include <Arduino.h>

#include <ESP32SPISlave.h>

#include "driver/gpio.h"
#include <stdint.h>
#include <Wire.h>
#include <LiquidCrystal.h>

//LEDs
#define LED_ROJO  17
#define LED_VERDE 16
#define LED_AZUL  4

//Potenciómetro
#define POT 34

//SPI
#define SPI_SCK  18
#define SPI_MISO 19
#define SPI_MOSI 23
#define SPI_CS   5

#define QUEUE_SIZE 1

ESP32SPISlave slave;
uint8_t spi_tx[3] = {0, 0, 0};
uint8_t spi_rx[3] = {0, 0, 0};

//I2C
#define I2C_DEV_ADDR 0x32

//LCD de 4 bits
#define RS 32
#define E  33
#define D4 25
#define D5 26
#define D6 27
#define D7 14

LiquidCrystal lcd( RS, E, D4, D5, D6, D7);

//Variables
volatile uint16_t sensorADC = 0;
volatile uint8_t comandoI2C = 0;

char ultimoLED = '-';
bool ledActivo = false;

unsigned long tiempoInicioLED = 0;
unsigned long tiempoLED = 0;

int ledActual = -1;

// Tiempos
unsigned long tiempoLCD = 0;
unsigned long tiempoADC = 0;

//Funciones
void apagarLEDs();
void activarLED(uint8_t led, unsigned long tiempo);
void actualizarLED();
void actualizarLCD();

void onReceive(int len);
void onRequest();

void apagarLEDs()  {
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_AZUL, LOW);
}

void activarLED(uint8_t led, unsigned long tiempo)  {
  apagarLEDs();
  ledActual = led;
  tiempoLED = tiempo;
  tiempoInicioLED = millis();

  ledActivo = true;
  if (led == 1)  {
    digitalWrite(LED_ROJO, HIGH);
    ultimoLED = 'R';
    Serial.println("LED ROJO -> ON");
  }
  else if (led == 2)  {
    digitalWrite(LED_VERDE, HIGH);
    ultimoLED = 'G';
    Serial.println("LED VERDE -> ON");
  }
  else if (led == 3)  {
    digitalWrite(LED_AZUL, HIGH);
    ultimoLED = 'B';
    Serial.println("LED AZUL -> ON");
  }
}

void actualizarLED()  {
  if (!ledActivo)  {
    return;
  }
  if (millis() - tiempoInicioLED >= tiempoLED)  {
    apagarLEDs();
    ledActivo = false;
    ledActual = -1;
    Serial.println("LED -> OFF");
    Serial.println("Operacion SPI terminada");
    Serial.println();
  }
}

void actualizarLCD()  {
  uint16_t valorADCraw = sensorADC;
  // Convertir 12 8 bits
  uint16_t valorADC = (valorADCraw * 255UL) / 4095UL;
  // Calcular voltaje
  float voltaje = valorADCraw * (3.3 / 4095.0);
  //Fila1
  lcd.setCursor(0, 0);
  lcd.print("Volt");
  lcd.setCursor(7, 0);
  lcd.print("ADC");
  lcd.setCursor(13, 0);
  lcd.print("LED");

  // Fila 2
  lcd.setCursor(0, 1);
  lcd.print("     ");
  lcd.setCursor(0, 1);
  lcd.print(voltaje, 2);
  // Fila 2: ADC
  lcd.setCursor(7, 1);
  lcd.print("    ");
  lcd.setCursor(7, 1);
  lcd.print(valorADC);
  // Ultimo LED
  lcd.setCursor(14, 1);
  lcd.print(" ");
  lcd.setCursor(14, 1);
  lcd.print(ultimoLED);
}



void setup() {
  Serial.begin(115200);
  
  //ADC
  pinMode(POT, INPUT);
  analogReadResolution(12);
  sensorADC = analogRead(POT);
  
  //LEDs
  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AZUL, OUTPUT);
  apagarLEDs();

  //LCD
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Iniciando...");

  //SPI
  slave.setDataMode(SPI_MODE0);
  slave.setQueueSize(QUEUE_SIZE);
  slave.begin(VSPI,SPI_SCK,SPI_MISO,SPI_MOSI,SPI_CS);
  
  Serial.println();
  Serial.println("=============================");
  Serial.println("ESP32 - PROYECTO 2");
  Serial.println("=============================");
  Serial.println();

  Serial.println("SPI SLAVE configurado:");
  Serial.println();

  //I2C
  Wire.begin((uint8_t)I2C_DEV_ADDR);
  Wire.onReceive(onReceive);
  Wire.onRequest(onRequest);
  Serial.print("I2C esclavo listo. Direccion: 0x");
  Serial.println(I2C_DEV_ADDR, HEX);
  delay(1000);
  lcd.clear();
  
  actualizarLCD();
}

void loop() {
  //ADC actualizar
  if (millis() - tiempoADC >= 20)  {
    tiempoADC = millis();
    sensorADC = analogRead(POT);
  }

  //SPI
  if (slave.hasTransactionsCompletedAndAllResultsHandled())  {
    slave.queue(spi_tx,spi_rx,3);
    slave.trigger();
  }
  if (slave.hasTransactionsCompletedAndAllResultsReady(QUEUE_SIZE))  {
    slave.numBytesReceivedAll();
    uint8_t ledRecibido = spi_rx[0];
    uint16_t tiempoRecibido =  ((uint16_t)spi_rx[1] << 8) |spi_rx[2];

    //Si se recibe
    Serial.println();
    Serial.println("=============================");
    Serial.println("COMANDO SPI RECIBIDO");
    Serial.println("=============================");

    Serial.print("LED: ");
    Serial.println(ledRecibido);
    Serial.print("MSB: ");
    Serial.println(spi_rx[1]);
    Serial.print("LSB: ");
    Serial.println(spi_rx[2]);
    Serial.print("Tiempo: ");
    Serial.print(tiempoRecibido);
    Serial.println(" ms");

    //Recibe valor de LED
    if (ledRecibido >= 1 && ledRecibido <= 3)  {
      if (tiempoRecibido > 0)  {
        Serial.println("Comando valido");
        activarLED(ledRecibido, tiempoRecibido);
      }
      else  {
        Serial.println("ERROR: tiempo invalido");
      }
    }
    else  {
      Serial.println("ERROR: LED invalido");
    }
    Serial.println();
  }

  actualizarLED();
  //Actualizar LCD
  if (millis() - tiempoLCD >= 200)  {
    tiempoLCD = millis();
    actualizarLCD();
  }
  
}

//I2C
void onRequest()  {
    uint16_t valorADC = sensorADC;

    uint8_t datos[2];
    datos[0] = valorADC & 0xFF;
    datos[1] = (valorADC >> 8) & 0xFF;

    Wire.write(datos, 2);
}
//Recibe comando de I2C
void onReceive(int len)  {
    while (Wire.available())
    {
        comandoI2C = Wire.read();
    }
}

