#include "WiFi.h"
// Constantes de nome do Wi-Fi e senha
const char * WIFI_FTM_SSID = "WiFi_FTM_Responder";
const char * WIFI_FTM_PASS = "ftm_responder";

void setup() {
  Serial.begin(115200);
  Serial.println("Starting SoftAP with FTM Responder support");
  // Abilita o AP com suporte para FTM (último argumento é 'true')
  WiFi.softAP(WIFI_FTM_SSID, WIFI_FTM_PASS, 1, 0, 4, true);
}

void loop() {
  delay(1000);
}
