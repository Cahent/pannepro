/* Panne-pro GIGA R1 WiFi + GIGA Display Shield
 * Benötigte Arduino-Bibliotheken: Arduino_GigaDisplay_GFX, Arduino_GigaDisplayTouch
 * Bitte WLAN, Domain/HTTPS und Schlüssel anpassen.
 * Demo-Grundgerüst: prüfe Display-Touch-API gegen installierte Bibliotheksversion.
 */
#include <WiFi.h>
#include <Arduino_GigaDisplay_GFX.h>
#include <Arduino_GigaDisplayTouch.h>

const char* WIFI_SSID = "WLAN_NAME";
const char* WIFI_PASS = "WLAN_PASSWORT";
const char* API_HOST = "panne-pro.example.de"; // Echte TLS-Domain, keine rohe IP
const char* DEVICE_ID = "PP-0001";
const char* API_TOKEN = "HIER_64_ZEICHEN_API_TOKEN_EINTRAGEN";
const int API_PORT = 443;
const char* API_PATH = "/api/v1/devices/status";

GigaDisplay_GFX display;
Arduino_GigaDisplayTouch touch;
const char* state = "unavailable";
unsigned long lastSent = 0;

void drawUI() {
 display.fillScreen(0x0000);
 display.setTextColor(0xFFFF);
 display.setTextSize(3);
 display.setCursor(20,20); display.print("Panne-pro");
 const char* labels[] = {"VERFUEGBAR", "IM EINSATZ", "NICHT VERFUEGBAR"};
 const uint16_t colors[] = {0x05E0,0xFCE0,0xC800};
 for(int i=0;i<3;i++){
  int y=100+i*120;display.fillRect(40,y,720,95,colors[i]);
  display.setTextColor(0xFFFF);display.setCursor(75,y+30);display.print(labels[i]);
 }
 display.setTextColor(0xFFFF); display.setTextSize(2);
 display.setCursor(20,465);display.print("Aktuell: ");display.print(state);
}

// Nie setInsecure() verwenden: Zertifikatsprüfung bleibt aktiv.
// Firmware-Transport wird separat an die verifizierte ArduinoCore-mbed
// TLS-Client-API und das Zertifikat der produktiven Domain angepasst.
// Ohne verifizierten TLS-Transport erfolgt absichtlich KEIN Versand.
void sendStatus() {
 Serial.println("TX nicht konfiguriert: TLS-Transport fuer GIGA implementieren und pruefen.");
}

void setup(){
 Serial.begin(115200);
 display.begin();touch.begin();drawUI();
 WiFi.begin(WIFI_SSID,WIFI_PASS);
 lastSent=millis();
}
void loop(){
 GDTpoint_t points[5];
 if(touch.getTouchPoints(points)>0){
  GDTpoint_t p=points[0];
  if(p.x>=40 && p.x<=760){
   if(p.y>=100 && p.y<195) state="available";
   else if(p.y>=220 && p.y<315) state="busy";
   else if(p.y>=340 && p.y<435) state="unavailable";
   drawUI(); // In der fertigen Firmware: Status sofort per TLS senden.
   delay(350);
  }
 }
 if(millis()-lastSent>60000){lastSent=millis();sendStatus();}
}
