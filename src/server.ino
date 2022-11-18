#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#define LED_R  4

const char *ssid = "T4G1"; //Digite o nome da sua rede aqui
const char *password = "SenhaForte123!"; //Digite a senha da rede, deve conter no mínimo 8 caractéres se não dá erro.
WebServer server(80);

const char * WIFI_FTM_SSID = "WiFi_FTM_Responder"; // SSID of AP that has FTM Enabled
const char * WIFI_FTM_PASS = "ftm_responder"; // STA Password

// Configurações FTM
// Números de FTMs solicitados em termos de 4 ou 8 estouros (valores permitidos - 0 (Sem pref), 16, 24, 32, 64)
const uint8_t FTM_FRAME_COUNT = 16;
// Período de tempo de requisição entre estouros de FTM em centésimos de milisegundos (valores permitidos - 0 (Sem pref) ou 2-255)
const uint16_t FTM_BURST_PERIOD = 2;

// Semaphore para sinal quando o sinal do Reporte FTM é recebido
xSemaphoreHandle ftmSemaphore;
// Status do Reporte FTM recebido
bool ftmSuccess = true;

// Manipulador de reportes de FTm com a calculação da informação da ida e volta
void onFtmReport(arduino_event_t *event) {
  const char * status_str[5] = {"SUCCESS", "UNSUPPORTED", "CONF_REJECTED", "NO_RESPONSE", "FAIL"};
  wifi_event_ftm_report_t * report = &event->event_info.wifi_ftm_report;
  // Coloca o status global do reporte
  ftmSuccess = report->status == FTM_STATUS_SUCCESS;
  if (ftmSuccess) {
    // A distância estimada em metros varia de acordo com alguns fatores (leia README)
    Serial.printf("FTM Estimate: Distance: %.2f m, Return Time: %u ns\n", (float)report->dist_est / 100.0 - 40, report->rtt_est);
    // Ponteiro para o Reporte FTM com várias entrada, deve ser solto após uso
    free(report->ftm_report_data);
  } else {
    Serial.print("FTM Error: ");
    Serial.println(status_str[report->status]);
  }
  // Sinal que o reporte recebe
  xSemaphoreGive(ftmSemaphore);
}

// Inicia uma Sessão FTM e espera pelo reporte FTM
bool getFtmReport(){
  if(!WiFi.initiateFTM(FTM_FRAME_COUNT, FTM_BURST_PERIOD)){
    Serial.println("FTM Error: Initiate Session Failed");
    return false;
  }
  // Espera o sinal de que o reporte foi recebido e retorna true se o status foi sucesso
  return xSemaphoreTake(ftmSemaphore, portMAX_DELAY) == pdPASS && ftmSuccess;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_R, OUTPUT);

  WiFi.softAP(ssid, password); // Remova "password" caso não queria que o Wi-Fi tenha senha
  IPAddress ESP_IP = WiFi.softAPIP();
  Serial.print("Wi-Fi: ");
  Serial.println(ssid);
  Serial.print("IP: "); // O IP que aparecer aqui coloque no navegador para acessar a página web do ESP32 que será criada logo abaixo.
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
    // Cria um semaphore binário (inicializador pego e pode ser pego/dado de qualquer núcleo/ISR)
  ftmSemaphore = xSemaphoreCreateBinary();
  
  // Ouve eventos de reporte de FTM
  WiFi.onEvent(onFtmReport, ARDUINO_EVENT_WIFI_FTM_REPORT);
  
  // Conecta ao AP que tem FTM ativado
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
void handleRoot(){
    String html = "";
    html += "<meta charset='UTF-8'>";
    html += "<h1>CONEXÃO</h1>";
    html += "Clique <a href=\"/on\">aqui</a> para ligar o LED. <br><br><br>";
    html += "Clique <a href=\"/off\">aqui</a> para desligar o LED. <br><br><br>";
    html += "<h3>Autores: T4G1</h3>";
    server.send(200, "text/html", html);
}

void handleOn(){
    digitalWrite(LED_R, HIGH);
    handleRoot();
}

void handleOff(){
    digitalWrite(LED_R, LOW);
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

void loop() {
  server.handleClient();
  getFtmReport();
  delay(1000);
}
