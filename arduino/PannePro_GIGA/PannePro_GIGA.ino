/*
 * panne.pro Statusgeber V0.4.1
 * Arduino GIGA R1 WiFi + GIGA Display Shield, Querformat 800x480
 *
 * Veraenderung erst "Server bestaetigt", wenn die Laravel API
 * HTTP 200 UND JSON mit "ok":true zurueckliefert.
 * Bei Fehler wird nach 15s erneut gesendet. Heartbeat jede Minute.
 *
 * Arduino Mbed OS GIGA Boards, Bibliotheken:
 *   Arduino_GigaDisplay_GFX, Arduino_GigaDisplayTouch
 *   WiFi, WiFiSSLClient (aus dem GIGA Boardpaket)
 *
 * WLAN, DEVICE_ID und API_TOKEN vor dem Flashen eintragen.
 * Achtung: Dieses oeffentliche GitHub-Repo enthaelt nur Platzhalter.
 * Nicht mit echten Zugangsdaten committen.
 */
#include <WiFi.h>
#include <WiFiSSLClient.h>
#include <Arduino_GigaDisplay_GFX.h>
#include <Arduino_GigaDisplayTouch.h>
#include <string.h>

const char* WIFI_SSID = "DEIN_WLAN_NAME";
const char* WIFI_PASS = "DEIN_WLAN_PASSWORT";
const char* PS_FAHRZEUG = "PS 01";
const char* API_HOST = "panne.pro";
const char* API_PATH = "/api/v1/devices/status";
const char* DEVICE_ID = "PP-0001";          // exakt wie im Adminbereich
const char* API_TOKEN = "HIER_DEIN_64_STELLIGER_API_TOKEN";

const uint32_t WIFI_RETRY_MS = 15000;
const uint32_t SEND_RETRY_MS = 15000;
const uint32_t HEARTBEAT_MS = 60000;
const uint32_t WIFI_CHECK_MS = 1000;
const uint32_t HTTP_TIMEOUT_MS = 6500;

// Touch-Zuordnung (bei abweichender Display-Hardware anpassbar):
// 0: x=rawY, y=479-rawX (bisheriger Wert)
// 1: x=rawY, y=rawX
// 2: x=799-rawY, y=rawX
// 3: x=799-rawY, y=479-rawX
// 4: x=rawX, y=rawY (Controller liefert bereits Querformat)
// 5: x=799-rawX, y=479-rawY
const uint8_t TOUCH_MAPPING = 0;

GigaDisplay_GFX display;
Arduino_GigaDisplayTouch touch;

enum Status : uint8_t { VERFUEGBAR, IM_EINSATZ, NICHT_VERFUEGBAR };
enum Quittung : uint8_t { UNBESTAETIGT, SENDET, BESTAETIGT, FEHLER };
Status aktuellerStatus = NICHT_VERFUEGBAR;
Quittung quittung = UNBESTAETIGT;

const uint16_t SCHWARZ = 0x0000;
const uint16_t WEISS = 0xFFFF;
const uint16_t GRUEN = 0x07E0;
const uint16_t ORANGE = 0xFD20;
const uint16_t ROT = 0xF800;
const uint16_t GRAU = 0xBDF7;
const int16_t BTN_X = 40, BTN_W = 720, BTN_H = 77;
const int16_t BTN_Y[] = {80, 174, 268};
const char* labels[] = {"VERFUEGBAR", "IM EINSATZ", "NICHT VERFUEGBAR"};
const char* textStatus[] = {"Verfuegbar", "Im Einsatz", "Nicht verfuegbar"};
const char* apiStatus[] = {"available", "busy", "unavailable"};
const uint16_t buttonColor[] = {GRUEN, ORANGE, ROT};

bool wifiConnected = false;
bool wifiStarted = false;
bool touchWasDown = false;
bool pendingStatus = true;
uint32_t lastWifiAttempt = 0;
uint32_t lastWifiCheck = 0;
uint32_t lastSendAttempt = 0;
uint32_t lastAck = 0;
uint32_t lastTouch = 0;

void centeredText(const char* value, int16_t centerX, int16_t topY,
                  uint8_t fontSize, uint16_t color) {
  display.setTextWrap(false);
  display.setTextSize(fontSize);
  display.setTextColor(color);
  int16_t bx, by;
  uint16_t w, h;
  display.getTextBounds(value, 0, 0, &bx, &by, &w, &h);
  display.setCursor(centerX - (int16_t)w / 2 - bx, topY - by);
  display.print(value);
}

void drawWifiIndicator() {
  display.fillRect(588, 63, 172, 15, SCHWARZ);
  display.setTextSize(1);
  display.setTextColor(wifiConnected ? GRUEN : ROT);
  display.setCursor(600, 67);
  display.print(wifiConnected ? "WLAN: Verbunden" : "WLAN: Offline");
}

void drawStatusArea() {
  // Nur die untere Textzeile und die Markierungen aktualisieren.
  display.fillRect(34, 365, 760, 112, SCHWARZ);
  display.setTextColor(WEISS);
  display.setTextSize(3);
  display.setCursor(40, 376);
  display.print("PS-Fahrzeug: ");
  display.print(PS_FAHRZEUG);
  display.setCursor(40, 410);
  display.print("Aktueller Status: ");
  display.print(textStatus[aktuellerStatus]);
}

void drawAckArea() {
  display.fillRect(36, 449, 760, 30, SCHWARZ);
  display.setTextSize(2);
  display.setCursor(40, 451);
  switch (quittung) {
    case SENDET:
      display.setTextColor(ORANGE);
      display.print("Server: Sende...");
      break;
    case BESTAETIGT:
      display.setTextColor(GRUEN);
      display.print("Server: Bestaetigt");
      break;
    case FEHLER:
      display.setTextColor(ROT);
      display.print("Server: NICHT bestaetigt - Wiederholung");
      break;
    default:
      display.setTextColor(ORANGE);
      display.print("Server: Noch nicht bestaetigt");
  }
}

void drawButton(uint8_t i) {
  display.fillRoundRect(BTN_X-5, BTN_Y[i]-5, BTN_W+10, BTN_H+10, 15, SCHWARZ);
  display.fillRoundRect(BTN_X, BTN_Y[i], BTN_W, BTN_H, 12, buttonColor[i]);
  if (aktuellerStatus == (Status)i) {
    display.drawRoundRect(BTN_X - 4, BTN_Y[i] - 4,
                          BTN_W + 8, BTN_H + 8, 15, WEISS);
    display.drawRoundRect(BTN_X - 3, BTN_Y[i] - 3,
                          BTN_W + 6, BTN_H + 6, 15, WEISS);
  }
  centeredText(labels[i], 400, BTN_Y[i] + 22, 4, WEISS);
}

void drawScreen() {
  display.fillScreen(SCHWARZ);
  centeredText("panne.pro Statusgeber", 400, 15, 3, WEISS);
  display.drawFastHLine(40, 61, 720, GRAU);
  drawWifiIndicator();
  for (int i = 0; i < 3; ++i) drawButton(i);
  drawStatusArea();
  drawAckArea();
}

void touchToLandscape(const GDTpoint_t& p, int16_t& x, int16_t& y) {
  switch (TOUCH_MAPPING) {
    case 1: x = p.y; y = p.x; break;
    case 2: x = 799 - p.y; y = p.x; break;
    case 3: x = 799 - p.y; y = 479 - p.x; break;
    case 4: x = p.x; y = p.y; break;
    case 5: x = 799 - p.x; y = 479 - p.y; break;
    default: x = p.y; y = 479 - p.x; break;
  }
}

void selectStatus(Status next) {
  if (next == aktuellerStatus && !pendingStatus) return;
  Status vorherigerStatus = aktuellerStatus;
  aktuellerStatus = next;
  pendingStatus = true;
  quittung = UNBESTAETIGT;
  lastSendAttempt = millis() - SEND_RETRY_MS;
  Serial.print("Gewaehlter Status: ");
  Serial.println(textStatus[aktuellerStatus]);
  drawButton((uint8_t)vorherigerStatus);
  drawButton((uint8_t)aktuellerStatus);
  drawStatusArea();
  drawAckArea();
}

void updateWiFi() {
  uint32_t now = millis();
  if (now - lastWifiCheck < WIFI_CHECK_MS) return;
  lastWifiCheck = now;
  bool connected = (WiFi.status() == WL_CONNECTED);
  if (connected != wifiConnected) {
    wifiConnected = connected;
    if (connected) {
      Serial.print("WLAN verbunden; IP: ");
      Serial.println(WiFi.localIP());
      lastSendAttempt = millis() - SEND_RETRY_MS;
    } else {
      Serial.println("WLAN getrennt.");
      pendingStatus = true; // erst nach erneutem Ack als aktuell bestaetigt
      quittung = FEHLER;
    }
    drawWifiIndicator();
    drawAckArea();
  }

  if (!connected && (!wifiStarted || now - lastWifiAttempt >= WIFI_RETRY_MS)) {
    if (strcmp(WIFI_SSID, "DEIN_WLAN_NAME") == 0) {
      if (!wifiStarted) Serial.println("Bitte WLAN-Zugangsdaten konfigurieren.");
      wifiStarted = true;
      return;
    }
    wifiStarted = true;
    lastWifiAttempt = now;
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.println("WLAN-Verbindungsversuch gestartet.");
  }
}

// Rueckgabewert bedeutet: Server hat die Anfrage gespeichert und mit ok:true
// quittiert. Eine bloess bestehende WLAN-Verbindung ist keine Quittung.
// TLS-Zertifikatpruefung des GIGA WiFiSSLClient darf nicht deaktiviert werden.
bool sendStatus() {
  if (!wifiConnected) return false;
  if (strcmp(API_TOKEN, "HIER_DEIN_64_STELLIGER_API_TOKEN") == 0 ||
      strlen(API_TOKEN) != 64) {
    Serial.println("API_TOKEN fehlt oder hat nicht 64 Zeichen.");
    return false;
  }

  WiFiSSLClient client;
  Serial.print("HTTPS an ");
  Serial.println(API_HOST);
  if (!client.connect(API_HOST, 443)) {
    Serial.println("HTTPS/TLS-Verbindung fehlgeschlagen (Zertifikat/DNS pruefen).");
    client.stop();
    return false;
  }

  String payload = String("{\"device_id\":\"") + DEVICE_ID +
                   "\",\"status\":\"" + apiStatus[aktuellerStatus] +
                   "\",\"capacity\":" + (aktuellerStatus == VERFUEGBAR ? "1" : "0") + "}";
  client.print(String("POST ") + API_PATH + " HTTP/1.1\r\n");
  client.print(String("Host: ") + API_HOST + "\r\n");
  client.print("User-Agent: panne-pro-giga/0.4\r\n");
  client.print("Authorization: Bearer ");
  client.print(API_TOKEN);
  client.print("\r\nContent-Type: application/json\r\n");
  client.print("Accept: application/json\r\nConnection: close\r\n");
  client.print("Content-Length: ");
  client.print(payload.length());
  client.print("\r\n\r\n");
  client.print(payload);

  String response;
  response.reserve(2048);
  uint32_t started = millis();
  while (millis() - started < HTTP_TIMEOUT_MS) {
    while (client.available() > 0) {
      char c = (char) client.read();
      if (response.length() < 4096) response += c;
    }
    if (!client.connected() && !client.available()) break;
    delay(5);
  }
  client.stop();

  // Laravel liefert eine einfache JSON-Antwort mit HTTP 200 und "ok":true.
  // Keine Quittung bei 401/422/500, HTML-Fehlerseite oder Time-out.
  bool http200 = response.startsWith("HTTP/1.1 200 ") ||
                 response.startsWith("HTTP/1.0 200 ");
  int headerEnd = response.indexOf("\r\n\r\n");
  String body = headerEnd >= 0 ? response.substring(headerEnd + 4) : "";
  body.replace(" ", "");
  body.replace("\r", "");
  body.replace("\n", "");
  bool accepted = http200 && body.indexOf("\"ok\":true") >= 0 &&
                  body.indexOf("\"provider_id\":") >= 0;
  if (!accepted) {
    Serial.print("Keine Serverquittung. HTTP: ");
    Serial.println(response.substring(0, response.indexOf('\n') >= 0 ?
                                       response.indexOf('\n') : 0));
  } else {
    Serial.println("Server hat Status bestaetigt (HTTP 200, ok:true).");
  }
  return accepted;
}

void updateServer() {
  if (!wifiConnected) return;
  const uint32_t now = millis();
  if (pendingStatus) {
    if ((uint32_t)(now - lastSendAttempt) < SEND_RETRY_MS) return;
  } else if ((uint32_t)(now - lastAck) < HEARTBEAT_MS) {
    return;
  }

  lastSendAttempt = now;
  quittung = SENDET;
  drawAckArea();
  bool ok = sendStatus();
  if (ok) {
    pendingStatus = false;
    quittung = BESTAETIGT;
    lastAck = millis();
  } else {
    pendingStatus = true;
    quittung = FEHLER;
  }
  drawAckArea();
}

void setup() {
  Serial.begin(115200);
  display.begin();
  display.setRotation(1);
  if (!touch.begin()) Serial.println("FEHLER: Touchcontroller nicht gefunden.");
  drawScreen();
  updateWiFi();
}

void loop() {
  updateWiFi();
  GDTpoint_t points[5];
  const uint8_t contacts = touch.getTouchPoints(points);
  if (contacts == 0) {
    touchWasDown = false;
  } else if (!touchWasDown && millis() - lastTouch > 120) {
    touchWasDown = true;
    lastTouch = millis();
    int16_t x, y;
    touchToLandscape(points[0], x, y);
    Serial.print("Touch roh: x=");
    Serial.print(points[0].x);
    Serial.print(" y=");
    Serial.print(points[0].y);
    Serial.print(" | umgerechnet: x=");
    Serial.print(x);
    Serial.print(" y=");
    Serial.println(y);
    if (x >= BTN_X && x < BTN_X + BTN_W) {
      for (int i = 0; i < 3; i++) {
        if (y >= BTN_Y[i] && y < BTN_Y[i] + BTN_H) {
          selectStatus((Status)i);
          break;
        }
      }
    }
  }
  updateServer();
  delay(10);
}
