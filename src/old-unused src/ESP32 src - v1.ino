#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <MFRC522.h>
#include <SPI.h>
#include <AHT10.h>
#define RFID_SS_SDA   21
#define RFID_RST      14
#define led_vermelho  2
#define led_verde     15
#define buzzer        40
int iniciar = 0;

AHT10Class AHT10;

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
}

void loop() {
  Serial.println(String("") + "Humidity(%RH):\t\t" + AHT10.GetHumidity() + "%");
  Serial.println(String("") + "Temperature(℃):\t" + AHT10.GetTemperat xure() + "℃");

  delay(1000);

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
