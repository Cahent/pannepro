/*
  panne.pro Statusgeber - Arduino GIGA R1 WiFi + GIGA Display Shield
  Display: 800 x 480 (Querformat), Touch: GT911
  Version 0.3 - Querformat, lokale Statuswahl und WLAN

  Benötigte Bibliotheken:
  - Arduino_GigaDisplay_GFX
  - Arduino_GigaDisplayTouch

  WICHTIG: Aktuell wird KEIN Status an den Server gesendet.
  TLS / API-Authentifizierung wird separat implementiert.
*/
#include <WiFi.h>
#include <Arduino_GigaDisplay_GFX.h>
#include <Arduino_GigaDisplayTouch.h>

GigaDisplay_GFX display;
Arduino_GigaDisplayTouch touch;

// Hier die Bezeichnung deines Pannendienst-Fahrzeugs eintragen:
const char* PS_FAHRZEUG = "PS 01";

// WLAN-Zugangsdaten lokal eintragen, niemals mit echtem Passwort nach GitHub pushen.
const char* WIFI_SSID = "DEIN_WLAN_NAME";
const char* WIFI_PASS = "DEIN_WLAN_PASSWORT";
const uint32_t WIFI_RETRY_MS = 15000;
const uint32_t WIFI_CHECK_MS = 1000;
uint32_t lastWifiAttemptMs = 0;
uint32_t lastWifiCheckMs = 0;
bool wifiConnected = false;
bool wifiStarted = false;

enum Status : uint8_t {
  VERFUEGBAR = 0,
  IM_EINSATZ = 1,
  NICHT_VERFUEGBAR = 2
};

Status aktuellerStatus = NICHT_VERFUEGBAR;

const uint16_t SCHWARZ = 0x0000;
const uint16_t WEISS   = 0xFFFF;
const uint16_t GRUEN   = 0x07E0;
const uint16_t ORANGE  = 0xFD20;
const uint16_t ROT     = 0xF800;
const uint16_t GRAU    = 0xBDF7;

const int16_t SCREEN_W = 800;
const int16_t SCREEN_H = 480;
const int16_t BTN_X = 40;
const int16_t BTN_W = 720;
const int16_t BTN_H = 77;
const int16_t BTN_Y[3] = {80, 174, 268};

const char* label[3] = {
  "VERFUEGBAR", "IM EINSATZ", "NICHT VERFUEGBAR"
};
const char* statusText[3] = {
  "Verfuegbar", "Im Einsatz", "Nicht verfuegbar"
};
const uint16_t buttonColor[3] = {GRUEN, ORANGE, ROT};

bool touchWasDown = false;
uint32_t lastTouchMs = 0;

void centeredText(const char* text, int16_t centerX, int16_t topY,
                  uint8_t size, uint16_t color) {
  display.setTextSize(size);
  display.setTextColor(color);
  display.setTextWrap(false);

  int16_t bx, by;
  uint16_t bw, bh;
  display.getTextBounds(text, 0, 0, &bx, &by, &bw, &bh);
  display.setCursor(centerX - (int16_t)bw / 2 - bx, topY - by);
  display.print(text);
}

void drawScreen() {
  display.fillScreen(SCHWARZ);
  centeredText("panne.pro Statusgeber", SCREEN_W / 2, 15, 3, WEISS);
  display.drawFastHLine(40, 61, 720, GRAU);
  display.setTextSize(1);
  display.setTextColor(wifiConnected ? GRUEN : ROT);
  display.setCursor(600, 67);
  display.print(wifiConnected ? "WLAN: Verbunden" : "WLAN: Offline");

  for (int i = 0; i < 3; i++) {
    display.fillRoundRect(BTN_X, BTN_Y[i], BTN_W, BTN_H, 12, buttonColor[i]);
    if (aktuellerStatus == (Status)i) {
      display.drawRoundRect(BTN_X - 4, BTN_Y[i] - 4,
                            BTN_W + 8, BTN_H + 8, 15, WEISS);
      display.drawRoundRect(BTN_X - 3, BTN_Y[i] - 3,
                            BTN_W + 6, BTN_H + 6, 15, WEISS);
    }
    centeredText(label[i], SCREEN_W / 2, BTN_Y[i] + 22, 4, WEISS);
  }

  display.setTextSize(3);
  display.setTextColor(WEISS);
  display.setCursor(40, 376);
  display.print("PS-Fahrzeug: ");
  display.print(PS_FAHRZEUG);

  display.setTextSize(3);
  display.setCursor(40, 425);
  display.print("Aktueller Status: ");
  display.print(statusText[aktuellerStatus]);
}

// Die GT911-Bibliothek meldet Touchwerte in Portrait-Rohkoordinaten:
// rawX = 0..479, rawY = 0..799.
// Bei display.setRotation(1) wird daraus Landscape (800x480).
void touchToLandscape(const GDTpoint_t& p, int16_t& x, int16_t& y) {
  x = p.y;
  y = 479 - p.x;
}

void selectStatus(Status next) {
  if (next == aktuellerStatus) return;
  aktuellerStatus = next;
  drawScreen();
  Serial.print("Neuer Status: ");
  Serial.println(statusText[aktuellerStatus]);
  // Hier spaeter die authentifizierte HTTPS-API aufrufen.
}

void updateWiFi() {
  const uint32_t now = millis();
  if (now - lastWifiCheckMs < WIFI_CHECK_MS) return;
  lastWifiCheckMs = now;

  const bool connectedNow = (WiFi.status() == WL_CONNECTED);
  if (connectedNow != wifiConnected) {
    wifiConnected = connectedNow;
    Serial.println(wifiConnected ? "WLAN verbunden." : "WLAN getrennt.");
    if (wifiConnected) {
      Serial.print("IP-Adresse: ");
      Serial.println(WiFi.localIP());
    }
    drawScreen();
  }

  if (!connectedNow && (!wifiStarted || now - lastWifiAttemptMs >= WIFI_RETRY_MS)) {
    if (strcmp(WIFI_SSID, "DEIN_WLAN_NAME") == 0) {
      if (!wifiStarted) Serial.println("Bitte WIFI_SSID und WIFI_PASS eintragen.");
      wifiStarted = true;
      return;
    }
    wifiStarted = true;
    lastWifiAttemptMs = now;
    Serial.print("Verbinde WLAN: ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  }
}

void setup() {
  Serial.begin(115200);
  display.begin();
  display.setRotation(1);
  if (!touch.begin()) {
    Serial.println("FEHLER: Touchcontroller nicht gefunden.");
  }
  drawScreen();
  updateWiFi();
}

void loop() {
  updateWiFi();
  GDTpoint_t points[5];
  const uint8_t count = touch.getTouchPoints(points);

  // Eine Auswahl pro Beruehrung; erst nach Loslassen erneut ausloesen.
  if (count == 0) {
    touchWasDown = false;
  } else if (!touchWasDown && millis() - lastTouchMs > 120) {
    touchWasDown = true;
    lastTouchMs = millis();

    int16_t x, y;
    touchToLandscape(points[0], x, y);
    Serial.print("Touch: "); Serial.print(x);
    Serial.print(", "); Serial.println(y);

    if (x >= BTN_X && x < BTN_X + BTN_W) {
      for (int i = 0; i < 3; i++) {
        if (y >= BTN_Y[i] && y < BTN_Y[i] + BTN_H) {
          selectStatus((Status)i);
          break;
        }
      }
    }
  }
  delay(10);
}
