#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <MFRC522.h>
#include <SPI.h>
#include <HTTPClient.h>;
#include <ArduinoJson.h>;
#include "WiFi.h"
#include "time.h"
#define RFID_SS_SDA   21
#define RFID_RST      14
#define led_vermelho  2
#define led_verde     15
#define buzzer        40

float distancia;
char *ID_RFID;

int iniciar = 0;

/////////////////////////////////////////////////////////////////////////////////////////

const char *ssid = "Inteli-COLLEGE";
const char *password = "QazWsx@123";
char jsonOutput[128];

/////////////////////////////////////////////////////////////////////////////////////////

const char * WIFI_FTM_SSID = "WiFi_FTM_Responder"; // SSID of AP that has FTM Enabled
const char * WIFI_FTM_PASS = "ftm_responder"; // STA Password

// FTM settings
// Number of FTM frames requested in terms of 4 or 8 bursts (allowed values - 0 (No pref), 16, 24, 32, 64)
const uint8_t FTM_FRAME_COUNT = 16;
// Requested time period between consecutive FTM bursts in 100’s of milliseconds (allowed values - 0 (No pref) or 2-255)
const uint16_t FTM_BURST_PERIOD = 2;

// Semaphore to signal when FTM Report has been received
xSemaphoreHandle ftmSemaphore;
// Status of the received FTM Report
bool ftmSuccess = true;

// FTM report handler with the calculated data from the round trip
void onFtmReport(arduino_event_t *event) {
  const char * status_str[5] = {"SUCCESS", "UNSUPPORTED", "CONF_REJECTED", "NO_RESPONSE", "FAIL"};
  wifi_event_ftm_report_t * report = &event->event_info.wifi_ftm_report;
  // Set the global report status
  ftmSuccess = report->status == FTM_STATUS_SUCCESS;
  if (ftmSuccess) {
    // The estimated distance in meters may vary depending on some factors (see README file)
    distancia = (float)report->dist_est / 100.0 - 40;
    Serial.printf("FTM Estimate: Distance: %.2f m, Return Time: %u ns\n", distancia, report->rtt_est);
    // Pointer to FTM Report with multiple entries, should be freed after use
    free(report->ftm_report_data);
  } else {
    Serial.print("FTM Error: ");
    Serial.println(status_str[report->status]);
  }
  // Signal that report is received
  xSemaphoreGive(ftmSemaphore);
}

// Initiate FTM Session and wait for FTM Report
bool getFtmReport(){
  if(!WiFi.initiateFTM(FTM_FRAME_COUNT, FTM_BURST_PERIOD)){
    Serial.println("FTM Error: Initiate Session Failed");
    return false;
  }
  // Wait for signal that report is received and return true if status was success
  return xSemaphoreTake(ftmSemaphore, portMAX_DELAY) == pdPASS && ftmSuccess;
}

/////////////////////////////////////////////////////////////////////////////////////////

MFRC522 rfidBase = MFRC522(RFID_SS_SDA, RFID_RST);
class LeitorRFID{
  private:
    // Declara variáveis das funções
    char codigoRFIDLido[100] = "";
    char codigoRFIDEsperado[100] = "";
    MFRC522 *rfid = NULL;
    int cartaoDetectado = 0;
    int cartaoJaLido = 0;
    // Leitura e regulagem de tempo de operação
    void processaCodigoLido(){
      char codigo[3*rfid->uid.size+1];
      codigo[0] = 0;
      char temp[10];
      for(int i=0; i < rfid->uid.size; i++){
        sprintf(temp,"%X",rfid->uid.uidByte[i]);
        strcat(codigo,temp);
      }
      // Printa o resultado da leitura
      codigo[3*rfid->uid.size+1] = 0;
      strcpy(codigoRFIDLido,codigo);
      Serial.println(codigoRFIDLido);
    }
  public:
    // Portas RFID se não nn funfa
    LeitorRFID(MFRC522 *leitor){
      rfid = leitor;
      rfid->PCD_Init();
      Serial.printf("MOSI: %i MISO: %i SCK: %i SS: %i\n",MOSI,MISO,SCK,SS);
    };
    // Resultado leitura (q?)
    char *tipoCartao(){
      MFRC522::PICC_Type piccType = rfid->PICC_GetType(rfid->uid.sak);
      Serial.println(rfid->PICC_GetTypeName(piccType));
      return((char *)rfid->PICC_GetTypeName(piccType));
    };
    int cartaoPresente(){
      return(cartaoDetectado);
    };
    int cartaoFoiLido(){
      return(cartaoJaLido);
    };
    void leCartao(){
      // Detecta o cartão
      if (rfid->PICC_IsNewCardPresent()) { // new tag is available
        iniciar = 7;
        Serial.println("Cartao presente");
          while (iniciar != 0){
            digitalWrite(led_verde, HIGH);
            delay(80);
            digitalWrite(led_verde, LOW);
            delay(80);
            iniciar -= 1;
          }
        cartaoDetectado = 1;
        // Termina a validação
        if (rfid->PICC_ReadCardSerial()) { // NUID foi lido
          Serial.println("Cartao lido");
          digitalWrite(led_verde, HIGH);
          cartaoJaLido = 1;
          processaCodigoLido();
          rfid->PICC_HaltA(); // halt PICC
          rfid->PCD_StopCrypto1(); // stop encryption on PCD
          tone(buzzer, 3000, 500);
          delay(1000);
        }
      }else{
        // Reinicia as variáveis caso o cartão seja removido antes da leitura completa
        cartaoDetectado = 0;
        iniciar = 7;
        digitalWrite(led_verde, LOW);
      }
    };
    // Printa as informações do cartão lido
    char *cartaoLido(){
      return(codigoRFIDLido);
    };
    void resetarLeitura(){
      cartaoDetectado = 0;
      cartaoJaLido = 0;
      iniciar = 10;
    }
    // Para o LCD, mostra a atual situação do card, como Scanneado, sem dispositivos encontrados, etc."
    void listI2CPorts(){
      Serial.println("\nI2C Scanner");
      byte error, address;
      int nDevices;
      Serial.println("Scanning...");
      nDevices = 0;
      for(address = 1; address < 127; address++ ) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0) {
          Serial.print("I2C device found at address 0x");
          if (address<16) {
            Serial.print("0");
          }
          Serial.println(address,HEX);
          nDevices++;
        }
        else if (error==4) {
          Serial.print("Unknow error at address 0x");
          if (address<16) {
            Serial.print("0");
          }
          Serial.println(address,HEX);
        }
      }
      if (nDevices == 0) {
        Serial.println("No I2C devices found\n");
      }
      else {
        Serial.println("done\n");
      }
    };
};
LeitorRFID *leitor = NULL;

void wifi_FTM(const char *WIFI_FTM_SSID, const char *WIFI_FTM_PASS, const uint8_t FTM_FRAME_COUNT, const uint8_t FTM_BURST_PERIOD) {
  // Connect to AP that has FTM Enabled
  Serial.println("Connecting to FTM Responder");
  WiFi.begin(WIFI_FTM_SSID, WIFI_FTM_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi Connected");

  Serial.print("Initiating FTM session with Frame Count ");
  Serial.print(FTM_FRAME_COUNT);
  Serial.print(" and Burst Period ");
  Serial.print(FTM_BURST_PERIOD * 100);
  Serial.println(" ms");
}

void wifi_json(const char *ssid, const char *password, char *ID_RFID, float distancia) {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(500); 
  }
  Serial.println("\nConnected to WiFi network");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  if (WiFi.status() == WL_CONNECTED){
    HTTPClient client;
    client.begin("http://10.128.64.156:3000/pessoa");
    client.addHeader("Content-Type", "application/json");
    
    const size_t CAPACITY = JSON_OBJECT_SIZE(3);
    StaticJsonDocument<CAPACITY> doc;

    JsonObject object = doc.to<JsonObject>();
    object["RFID"] = ID_RFID;
    object["distancia"] = distancia;

    serializeJson(doc, jsonOutput);
    
    int httpCode = client.POST(String(jsonOutput));

    if (httpCode > 0) {
      String payload = client.getString();
      Serial.println("\nStatus Code: " + String(httpCode));
      Serial.println(payload);

      client.end();
    }
    else {
      Serial.println("Error on HTTP request");
    }
  }
  else {
    Serial.println("Connection lost"); 
  }
}

void setup() {

  Serial.begin(115200);
  Wire.begin();
  SPI.begin();

  // Create binary semaphore (initialized taken and can be taken/given from any thread/ISR)
  ftmSemaphore = xSemaphoreCreateBinary();
  
  // Listen for FTM Report events
  WiFi.onEvent(onFtmReport, ARDUINO_EVENT_WIFI_FTM_REPORT);
  
/////////////////////////////////////////////////////////////////////////////////////////

  pinMode(led_vermelho, OUTPUT);
  pinMode(led_verde, OUTPUT);
  pinMode(buzzer, OUTPUT);
  digitalWrite(led_vermelho, HIGH);
  //------------------------//
  leitor = new LeitorRFID(&rfidBase);
  //------------------------//
}

// Para fazer o json: char - ID_RFID, float - distancia

void loop() {

  delay(2000);
  wifi_FTM(WIFI_FTM_SSID, WIFI_FTM_PASS, FTM_FRAME_COUNT, FTM_BURST_PERIOD);
  // Enquanto não for detectado nada, ele vai ficar procurando um cartão até achar um. Ai ele começa a acessar as funções
  Serial.println("Procurando cartão...");
  leitor->leCartao();
  if(leitor->cartaoFoiLido()){
    ID_RFID = leitor-> cartaoLido();
    Serial.println(leitor->tipoCartao());
    Serial.println(leitor->cartaoLido());

    getFtmReport();
    Serial.print("RFID LIDO: ");
    Serial.println(ID_RFID);
    Serial.print("Distância até o valet: ");
    Serial.println(distancia);

    wifi_json(ssid, password, ID_RFID, distancia);
    
    leitor->resetarLeitura();
    delay(1000);
  }
}
