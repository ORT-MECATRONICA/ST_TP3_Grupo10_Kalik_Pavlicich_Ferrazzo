//Grupo10_Kalik_Pavlicich_Ferrazzo

//Librerias
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h> //Libreria para conectar con telegram
#include <ArduinoJson.h> //Libreria necesaria para el bot de telegram

TaskHandle_t Task1;
TaskHandle_t Task2;

//Wifi
const char* ssid = "ORT-IoT";
const char* password = "NuevaIOT$25";

// Initialize Telegram BOT
#define BOTtoken "7525131885:AAFPvWp5yNZh9hGeqOli2F-FkV-qupSYYzs"  // El bor roken para que el ESP32 pueda interactura con el ESP32
#define CHAT_ID "7982476800"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

//Ajustes de la pantalla
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//DHT Sensor de temperatura y humedad
#define DHTPIN 23      
#define DHTTYPE DHT11  
DHT dht(DHTPIN, DHTTYPE);

//Variables de millis()
unsigned long TiempoUltimoCambio = 0;
unsigned long TiempoUltimoCambio2 = 0;
const long Intervalo = 5000;
unsigned long lastTimeBotRan = 0;
const int botRequestDelay = 2000;
unsigned long TiempoAhora;

//Estados de la maquina de estados y mas
#define PANTALLA1 1
#define ESTADO_CONFIRMACION1 2
#define ESTADO_CONFIRMACION2 3
#define PANTALLA2 4
#define SUBIR 5
#define BAJAR 6
void MAQUINA_DE_ESTADOS();

#define PULSADO 0
#define NO_PULSADO 1

#define BOTON1 34
#define BOTON2 35
#define LED1 25
#define LED2 26

//Variables generales
int lectura1;
int lectura2;
int estado = 1;
int VALOR_UMBRAL = 28;
int contador1;
int CONTADOR = 0;
float t;
int flag_umbral = 0;

void setup() {
  
  Serial.begin(115200);

//DHT
  dht.begin();

//Pinmodes
  pinMode(BOTON1, INPUT);
  pinMode(BOTON2, INPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

 xTaskCreatePinnedToCore(
                    Task1code,   /* Task function. */
                    "Task1",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */                  
  delay(500); 

  //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(
                    Task2code,   /* Task function. */
                    "Task2",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task2,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */
    delay(500); 

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
  }

  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  client.setInsecure();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  bot.sendMessage(CHAT_ID, "Bot started up", "");

  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.display();
}



void MAQUINA_DE_ESTADOS() {

  TiempoAhora = millis();
  switch (estado) {
    case PANTALLA1:
      if (TiempoAhora - TiempoUltimoCambio >= Intervalo)  ///delay sin bloqueo
      {
        TiempoUltimoCambio = TiempoAhora;  /// importante actualizar el tiempo
        t = dht.readTemperature();
        display.clearDisplay();
        display.setCursor(0, 20);
        display.print("Temp: ");
        display.print(t);
        display.print("°C ");

        display.setCursor(0, 50);
        display.print("Valor Umbral: ");
        display.print(VALOR_UMBRAL);
        display.display();
      }
      
//Secuencia de botones
if (TiempoAhora - TiempoUltimoCambio2 >= Intervalo && CONTADOR != 0)  ///delay sin bloqueo
      {
        CONTADOR = 0;
      }

      if (lectura1 == PULSADO && CONTADOR == 0) {
        CONTADOR = 1;
        estado = ESTADO_CONFIRMACION1;
      }
      if (lectura2 == PULSADO && CONTADOR == 2) {
        CONTADOR = 3;
        estado = ESTADO_CONFIRMACION1;
      }
      if (lectura1 == PULSADO && CONTADOR == 4) {
        CONTADOR = 5;
        estado = ESTADO_CONFIRMACION1;
      }


//Flag para prender el led cuando supera el umbral y mandar el mensaje por telegram
      if (t >= VALOR_UMBRAL) {
        if(flag_umbral < 1){
          flag_umbral = 1;
        }
      }
      else {
        flag_umbral = 0;
      }

      break;

    case ESTADO_CONFIRMACION1:
    //Secuencai de botones
      if (lectura1 == NO_PULSADO && CONTADOR == 1) {
        CONTADOR = 2;
        TiempoUltimoCambio2 = TiempoAhora;
        estado = PANTALLA1;
      }
      if (lectura2 == NO_PULSADO && CONTADOR == 3) {
        CONTADOR = 4;
        TiempoUltimoCambio2 = TiempoAhora;
        estado = PANTALLA1;
      }

      if (lectura1 == NO_PULSADO && CONTADOR == 5) {
        CONTADOR = 0;
        estado = PANTALLA2;
      }
      break;

    case PANTALLA2:
      digitalWrite(LED1, LOW);

      display.clearDisplay();
      display.setCursor(0, 20);
      display.print("Valor Umbral: ");
      display.print(VALOR_UMBRAL);
      display.display();


      if (lectura1 == PULSADO && lectura2 == PULSADO) {
        estado = ESTADO_CONFIRMACION2;
      }
      if (lectura1 == PULSADO) {
        contador1 = 1;
        estado = SUBIR;
      }
      if (lectura2 == PULSADO) {
        contador1 = 1;
        estado = BAJAR;
      }
      break;

    case ESTADO_CONFIRMACION2:
      if (lectura1 == NO_PULSADO && lectura2 == NO_PULSADO) {
        estado = PANTALLA1;
      }
      break;

    case SUBIR:
      if (lectura2 == PULSADO) {
        estado = ESTADO_CONFIRMACION2;
      }

      if (lectura1 == NO_PULSADO) {
        digitalWrite(LED1, HIGH);
        if (contador1 == 1) {
          VALOR_UMBRAL = VALOR_UMBRAL + 1;
          contador1 = 0;
        }
        estado = PANTALLA2;
      }
      break;

    case BAJAR:
      if (lectura1 == PULSADO) {
        estado = ESTADO_CONFIRMACION2;
      }

      if (lectura2 == NO_PULSADO) {
        digitalWrite(LED1, HIGH);
        if (contador1 == 1) {
          VALOR_UMBRAL = VALOR_UMBRAL - 1;
          contador1 = 0;
        }
        estado = PANTALLA2;
      }
      break;
  }
}

//Loop 1
void Task1code( void * pvParameters ){
  for(;;){    //El for permite que se quede en loop
 lectura1 = digitalRead(BOTON1);
  lectura2 = digitalRead(BOTON2);

  Serial.println(CONTADOR);
  MAQUINA_DE_ESTADOS();

  //bot.sendMessage(CHAT_ID, "Hola, soy Felipe Alfiz, tambien conocido como EL_FOFO_BOT", "");
  } 
  //vTaskDelay(100 / portTICK_PERIOD_MS); // Pausa de 100 ms Esto reduce la carga del procesador y mejora la estabilidad.
}

void Task2code( void * pvParameters ){
  for(;;){
    
    if (millis() - lastTimeBotRan > botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      for (int i = 0; i < numNewMessages; i++) {
        String text = bot.messages[i].text;
        String chat_id = bot.messages[i].chat_id;
        String from_name = bot.messages[i].from_name;

        Serial.printf("Mensaje de %s: %s\n", from_name.c_str(), text.c_str());

        if (text == "/temperatura") {
          float t = dht.readTemperature();
          if (isnan(t)) {
            bot.sendMessage(chat_id, "⚠️ Error al leer la temperatura", "");
          } else {
            String temp_msg = "🌡️ La temperatura actual es: " + String(t) + " °C";
            bot.sendMessage(chat_id, temp_msg, "");
          }
        } else {
          bot.sendMessage(chat_id, "Comando no reconocido. Usa /temperatura", "");
        }
      }
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
  
  //Si la temperatura supera el umbral
        if (flag_umbral == 1) {
          bot.sendMessage(CHAT_ID, "La temperatura supero el umbral", "");
          digitalWrite(LED1, HIGH);
          flag_umbral = 2; // este flag es para que no se repita el mensaje todo el rato
        }
       if(flag_umbral == 0) {
        digitalWrite(LED1, LOW);
      }
  }
  
      //vTaskDelay(100 / portTICK_PERIOD_MS); // Pausa de 100 ms Esto reduce la carga del procesador y mejora la estabilidad.
}

void loop() {
 
}