#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <MFRC522.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include "time.h"
#define RFID_SS_SDA   21
#define RFID_RST      14
#define led_vermelho  2
#define led_verde     15
#define buzzer        40

float distancia;
char *ID_RFID;
char *tipoRFID;

int estadoDoProcedimento = 0;

int iniciar = 0;

/////////////////////////////////////////////////////////////////////////////////////////

const char *ssid = "Inteli-COLLEGE"; // Mude o nome do Wi-Fi quando realizar a configuração inicial.
const char *password = "QazWsx@123";
// Mude a senha do Wi-Fi quando realizar a configuração inicial.
// Senhas menores do que 8 caracteres causam tentativas de conexão infinitas,
// porém já foi reportado que senhas de 9 caracteres também retornam os mesmos erros,
// por isso recomenda-se senhas fortes de 10 caracteres para cima.

char jsonOutput[128];

/////////////////////////////////////////////////////////////////////////////////////////

const char * WIFI_FTM_SSID = "WiFi_FTM_Responder"; // Mude o nome do ESP32 com FTM caso necessário
const char * WIFI_FTM_PASS = "ftm_responder"; // A lógica da senha se aplica aqui também

// Configurações do FTM
// Número de frames FTM de requisiões em termos de 4 ou 8 bursts (valores permitidos - 0 (Sem preferência), 16, 24, 32, 64)
const uint8_t FTM_FRAME_COUNT = 16;
// Período de tempo de requisição entre bursts de FTM consecutivos em centésimos (100's) de milissegundos (valores permitidos - 0 (Sem preferência) ou 2-255)
const uint16_t FTM_BURST_PERIOD = 2;

xSemaphoreHandle ftmSemaphore;
// Status do reporte FTM recebido
bool ftmSuccess = true;

void onFtmReport(arduino_event_t *event) {
  const char * status_str[5] = {"SUCCESS", "UNSUPPORTED", "CONF_REJECTED", "NO_RESPONSE", "FAIL"};
  wifi_event_ftm_report_t * report = &event->event_info.wifi_ftm_report;
  ftmSuccess = report->status == FTM_STATUS_SUCCESS;
  if (ftmSuccess) {
    distancia = (float)report->dist_est / 100.0 - 40;
    Serial.printf("FTM Estimado: Distância: %.2f m, Tempo de Retorno: %u ns\n", distancia, report->rtt_est);
    free(report->ftm_report_data);
  } else {
    Serial.print("Erro FTM: ");
    Serial.println(status_str[report->status]);
  }
  // O sinal que o FTM Report foi recebido.
  xSemaphoreGive(ftmSemaphore);
}

// Inicia uma sessão FTM e espera pelo FTM Report.
bool getFtmReport(){
  if(!WiFi.initiateFTM(FTM_FRAME_COUNT, FTM_BURST_PERIOD)){
    Serial.println("FTM Error: Initiate Session Failed");
    return false;
  }
  // Espera pelo sinal que o reporte foi recebdio e retorna true se o status foi de sucesso.
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
       Serial.println("\nID do RFID: " + String(codigoRFIDLido));
    }
  public:
    // Portas específicas do RFID.
    LeitorRFID(MFRC522 *leitor){
      rfid = leitor;
      rfid->PCD_Init();
      // Serial.printf("MOSI: %i MISO: %i SCK: %i SS: %i\n",MOSI,MISO,SCK,SS);
    };
    // Resultado da leitura.
    char *tipoCartao(){
      MFRC522::PICC_Type piccType = rfid->PICC_GetType(rfid->uid.sak);
      // Serial.println(rfid->PICC_GetTypeName(piccType));
      return((char *)rfid->PICC_GetTypeName(piccType));
    };

    int cartaoFoiLido(){
      return(cartaoJaLido);
    };
    void leCartao(){
      // Detecta o cartão
      if (rfid->PICC_IsNewCardPresent()) { // Verifica se o cartão está presente.
        iniciar = 7;
        Serial.println("\nCartão presente, iniciando a leitura.");
          while (iniciar != 0){
            digitalWrite(led_verde, HIGH);
            delay(80);
            digitalWrite(led_verde, LOW);
            delay(80);
            iniciar -= 1;
          }
        // Termina a validação
        if (rfid->PICC_ReadCardSerial()) { // NUID foi lido
          Serial.println("\nCartão lido com sucesso.");
          digitalWrite(led_verde, HIGH);
          cartaoJaLido = 1;
          processaCodigoLido();
          rfid->PICC_HaltA();
          rfid->PCD_StopCrypto1();
          tone(buzzer, 3000, 500);
          delay(1000);
          digitalWrite(led_verde, LOW);
        }
        else {
          Serial.println("\nErro: Cartão removido antes do término da leitura.\n");
          digitalWrite(led_vermelho, HIGH);
          tone(buzzer, 110, 300);
          delay(200);
          digitalWrite(led_vermelho, LOW);
          delay(200);
          digitalWrite(led_vermelho, HIGH);
          tone(buzzer, 100, 300);
          delay(200);
          digitalWrite(led_vermelho, LOW);
        }
      }else{
        // Reinicia as variáveis caso o cartão seja removido antes da leitura completa
        iniciar = 7;
        digitalWrite(led_verde, LOW);
      }
    };
    // Printa as informações do cartão lido
    char *cartaoLido(){
      return(codigoRFIDLido);
    };
    void resetarLeitura(){
      cartaoJaLido = 0;
      iniciar = 10;
    }
};

LeitorRFID *leitor = NULL;

void wifi_FTM(const char *WIFI_FTM_SSID, const char *WIFI_FTM_PASS, const uint8_t FTM_FRAME_COUNT, const uint8_t FTM_BURST_PERIOD) {
  Serial.print("Conectando-se a antena FTM.");
  WiFi.begin(WIFI_FTM_SSID, WIFI_FTM_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n\nConectado a antena FTM.");
}

void wifi_json(const char *ssid, const char *password, char *ID_RFID, float distancia, int estadoDoProcedimento) {
  WiFi.begin(ssid, password);
  Serial.print("Conectando-se ao Wi-Fi.");

  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(500); 
  }
  Serial.println("\nConectado a rede Wi-Fi " + String(ssid));
  // Serial.print("IP address: ");
  // Serial.println(WiFi.localIP());
  
  if (WiFi.status() == WL_CONNECTED){
    HTTPClient client;
    client.begin("http://10.128.65.250:3000/pessoa");
    client.addHeader("Content-Type", "application/json");
    
    const size_t CAPACITY = JSON_OBJECT_SIZE(4);
    StaticJsonDocument<CAPACITY> doc;

    JsonObject object = doc.to<JsonObject>();
    object["RFID"] = ID_RFID;
    object["distancia"] = distancia;
    object["estado"] = estadoDoProcedimento;

    serializeJson(doc, jsonOutput);
    
    int httpCode = client.POST(String(jsonOutput));

    if (httpCode > 0) {
      String payload = client.getString();
      Serial.println("\nCódigo de status: " + String(httpCode));
      Serial.println(payload);

      client.end();
    }
    else {
      Serial.println("Erro ao conectar-se ao servidor. Dados não enviados e sessão encerrada.\n");
    }
  }
  else {
    Serial.println("Conexão perdida com o servidor.\n"); 
  }
}

void setup() {

  Serial.begin(115200);
  Wire.begin();
  SPI.begin();
  
  Serial.printf("\nSessões FTM são feitas com contagens de %i frames e períodos de burst de %i ms.\n\n", FTM_FRAME_COUNT, FTM_BURST_PERIOD);

  ftmSemaphore = xSemaphoreCreateBinary();
  
  WiFi.onEvent(onFtmReport, ARDUINO_EVENT_WIFI_FTM_REPORT);
  
/////////////////////////////////////////////////////////////////////////////////////////

  pinMode(led_vermelho, OUTPUT);
  pinMode(led_verde, OUTPUT);
  pinMode(buzzer, OUTPUT);
  //------------------------//
  leitor = new LeitorRFID(&rfidBase);
  //------------------------//
}

void loop() {

  delay(1000);
  Serial.flush();
  // Enquanto não for detectado nada, ele vai ficar procurando um cartão até achar um e iniciar as funções.
  leitor->leCartao();
  if(leitor->cartaoFoiLido()){
    ID_RFID = leitor-> cartaoLido();
    tipoRFID = leitor->tipoCartao();
    Serial.println("Tipo do RFID: " + String(tipoRFID));
    Serial.printf("Estado do RFID: %i\n", estadoDoProcedimento);
    wifi_FTM(WIFI_FTM_SSID, WIFI_FTM_PASS, FTM_FRAME_COUNT, FTM_BURST_PERIOD);
    getFtmReport();
    Serial.println("Distância até o valet: " + String(distancia) + " m.\n");
    wifi_json(ssid, password, ID_RFID, distancia, estadoDoProcedimento);
    
    if (estadoDoProcedimento >= 3){
      estadoDoProcedimento = 0;
    }
    else {
      estadoDoProcedimento += 1;
    }
    leitor->resetarLeitura();
    delay(1000);
  }
  Serial.println("Procurando cartão...");
}
