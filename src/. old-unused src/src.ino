#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <MFRC522.h>
#include <SPI.h>
#include <AHT10.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <esp_now.h>
#define RFID_SS_SDA   21
#define RFID_RST      14
#define led_vermelho  2
#define led_verde     15
#define buzzer        40

//Pino utilizado para leitura
//e que será enviado o valor
#define PIN 3

//Canal usado para conexão
#define CHANNEL 1

//Se MASTER estiver definido 
//o compilador irá compilar o Master.ino
//Se quiser compilar o Slave.ino remova ou
//comente a linha abaixo
#define MASTER

//Estrutura com informações
//sobre o próximo peer
esp_now_peer_info_t peer;


const char *ssid = "ESP T4G1";
const char *password = "Turma4Grupo1";
WebServer server(80);

int iniciar = 0;

//AHT10Class AHT10;

MFRC522 rfidBase = MFRC522(RFID_SS_SDA, RFID_RST);
class LeitorRFID{
  private:
    // Declara variáveis das funções
    char codigoRFIDLido[100] = "";
    char codigoRFIDEsperado[100] = "";
    MFRC522 *rfid = NULL;
    int cartaoDetectado = 0;
    int cartaoJaLido = 0;
    // Leitura e Regulagem de tempo de operação (sla o que isso significa ???????)
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
        if (rfid->PICC_ReadCardSerial()) { // NUID has been readed
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

//////////////////////////////
void setup() {

  Serial.begin(115200);
  Wire.begin();
  SPI.begin();

  // if (AHT10.begin(eAHT10Address_Low)) {
  //   Serial.println("Init AHT10 Success!");
  // }
  // else {
  //   Serial.println("Init AHT10 Failed!");
  // }

  pinMode(led_vermelho, OUTPUT);
  pinMode(led_verde, OUTPUT);
  pinMode(buzzer, OUTPUT);
  digitalWrite(led_vermelho, HIGH);
  // //------------------------//
  leitor = new LeitorRFID(&rfidBase);
  //------------------------//
  // Serial.print("MOSI: "); Serial.println(MOSI);
  // Serial.print("MISO: "); Serial.println(MISO);
  // Serial.print("SCK: "); Serial.println(SCK);
  // Serial.print("SS: "); Serial.println(SS);
  WiFi.softAP(ssid, password);
  IPAddress ESP_IP = WiFi.softAPIP();
  Serial.print("Wi-Fi: ");
  Serial.println(ssid);
  Serial.print("IP: ");
  Serial.println(ESP_IP);
  server.begin();
  Serial.println("Servidor Iniciado.");
  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }
  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void handleRoot(){
  String html = "";
  html += "<meta charset='UTF-8'>";
  html += "<h1>CONEXÃO???</h1>";
  html += "Clique <a href=\"/on\">aqui</a> para ligar o LED. <br><br><br>";
  html += "Clique <a href=\"/off\">aqui</a> para desligar o LED. <br><br><br>";
  html += "<h3>Autores: T4G1</h3>";
  server.send(200, "text/html", html);
}
  
void handleOn(){
  digitalWrite(led_vermelho, HIGH);
  handleRoot();
}

void handleOff(){
  digitalWrite(led_vermelho, LOW);
  handleRoot();
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n"; 
  }
  server.send(404, "text/plain", message);
}

//######################ESP-NOW############################

//Função para inicializar o modo station
void modeStation(){
    //Colocamos o ESP em modo station
    WiFi.mode(WIFI_STA);
    //Mostramos no Monitor Serial o Mac Address 
    //deste ESP quando em modo station
    Serial.print("Mac Address in Station: "); 
    Serial.println(WiFi.macAddress());
}

//Função de inicialização do ESPNow
void InitESPNow() {
    //Se a inicialização foi bem sucedida
    if (esp_now_init() == ESP_OK) {
        Serial.println("ESPNow Init Success");
    }
    //Se houve erro na inicialização
    else {
        Serial.println("ESPNow Init Failed");
        ESP.restart();
    }
}

//Função que adiciona um novo peer
//através de seu endereço MAC
void addPeer(uint8_t *peerMacAddress){
    //Informamos o canal
    peer.channel = CHANNEL;
    //0 para não usar criptografia ou 1 para usar
    peer.encrypt = 0;
    //Copia o endereço do array para a estrutura
    memcpy(peer.peer_addr, peerMacAddress, 6);
    //Adiciona o slave
    esp_now_add_peer(&peer);
}

//#########################################################

void loop() {
  //Serial.println(String("") + "Humidity(%RH):\t\t" + AHT10.GetHumidity() + "%");
  //Serial.println(String("") + "Temperature(℃):\t" + AHT10.GetTemperat xure() + "℃");
  server.handleClient();
  delay(2);
  //delay(1000);

  // Enquanto não for detectado nada, ele vai ficar procurando um cartão até achar um. Ai ele começa a acessar as funções
  // Serial.println("Lendo Cartao:");
  leitor->leCartao();
  if(leitor->cartaoFoiLido()){
    Serial.println(leitor->tipoCartao());
    Serial.println(leitor->cartaoLido());
    leitor->resetarLeitura();
    delay(1000);
  }
}
