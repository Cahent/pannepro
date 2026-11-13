# Panne-pro – Installationsanleitung (Laravel 13 Overlay)

## Zielumgebung
Ubuntu 24.04 · Plesk · PHP 8.3.36 · MariaDB/MySQL · TLS-Domain.

**Wichtig:** Dieses ZIP ist eine funktionale Projekt-Erweiterung für ein **frisch angelegtes Laravel-13-Projekt**. Es enthält **nicht** den Laravel-Core und keine Composer-Abhängigkeiten. Live-Betrieb, Firmware und Server wurden nicht getestet. Ohne Domain mit gültigem HTTPS-Zertifikat sollten keine Geräte angebunden werden.

### 1. Plesk vorbereiten
1. In Plesk **eine Domain oder Subdomain** für Panne-pro erstellen. Eine öffentliche IP allein reicht für die empfohlene TLS-Geräteanbindung nicht aus.
2. SSL/TLS-Zertifikat aktivieren (z.B. Let's Encrypt), HTTP auf HTTPS umleiten.
3. PHP 8.3 auswählen, benötigte PHP-Erweiterungen aktivieren (`pdo_mysql`, `mbstring`, `openssl`, `curl`, `xml`, `ctype`, `fileinfo`, `dom`, `tokenizer`).
4. In Plesk eine **eigene MariaDB/MySQL-Datenbank** mit eigenem Benutzer anlegen.

### 2. Laravel frisch erstellen (SSH als Domain-Systembenutzer)
```bash
# Pfad an deine Plesk-Domain anpassen; Beispiel:
cd /var/www/vhosts/DEINE-DOMAIN/httpdocs
# WICHTIG: Ziel muss leer sein, ggf. in separatem Verzeichnis arbeiten.
/opt/plesk/php/8.3/bin/php /usr/local/bin/composer create-project laravel/laravel panne-pro "^13.0"
cd panne-pro
```
**Composer-Pfad kann abweichen.** Prüfe `which composer` und `composer --version`; falls vorhanden, z.B. `composer create-project laravel/laravel panne-pro "^13.0"` mit passender CLI-PHP-Version. Nicht blind bestehende Website-Dateien löschen.

### 3. Overlay hineinkopieren
Alle Ordner/Dateien aus diesem ZIP **in das Laravel-Projektverzeichnis** kopieren und vorhandene Beispiel-Dateien (inkl. `bootstrap/app.php`, `routes/web.php`, `routes/console.php` bleibt unverändert) wie vorgesehen ersetzen. Laravel-Core-Dateien nicht überschreiben.

### 4. `.env` einstellen
Laravel legt bereits eine `.env` an. Trage die Werte aus `.env.pannepro.example` dort passend ein, ohne die existierenden Schlüssel wie `APP_KEY` zu löschen. `APP_KEY` erzeugen:
```bash
/opt/plesk/php/8.3/bin/php artisan key:generate
/opt/plesk/php/8.3/bin/php artisan migrate --force
/opt/plesk/php/8.3/bin/php artisan db:seed --force
/opt/plesk/php/8.3/bin/php artisan pannepro:admin admin@deine-domain.de
/opt/plesk/php/8.3/bin/php artisan optimize:clear
/opt/plesk/php/8.3/bin/php artisan config:cache
/opt/plesk/php/8.3/bin/php artisan view:cache
```
Admin-Passwort wird interaktiv eingegeben, nicht als Kommandoargument.

### 5. Document Root – entscheidend
**Nur** `<LARAVEL_PROJEKT>/public` darf öffentlich vom Webserver ausgeliefert werden. In Plesk unter Hosting-Einstellungen den Dokumentenstamm relativ zur Domain auf `httpdocs/panne-pro/public` setzen **wenn das Projekt in `httpdocs/panne-pro` liegt**. Je nach Plesk-Konfiguration kann der Stammpfad abweichen. Falls Plesk keine passende Änderung erlaubt, das Projekt getrennt von `httpdocs` anlegen und `httpdocs` ausschließlich als Laravel-`public` bereitstellen – nicht einfach die komplette Projektwurzel freigeben.

Für Laravel müssen `storage` und `bootstrap/cache` durch den Webserver-/Domain-Benutzer beschreibbar sein. Keine pauschalen `chmod 777` verwenden.

### 6. Inbetriebnahme
1. `https://DEINE-DOMAIN/up` prüfen (Laravel-Healthcheck).
2. `https://DEINE-DOMAIN/register` testen: ein neues Konto muss Rolle `customer` erhalten.
3. Mit Admin-Konto `/admin` öffnen, einen Dienstleister und ein Gerät anlegen.
4. Geräte-Schlüssel sofort sicher speichern. Er ist später nicht wieder abrufbar.
5. API zuerst mit `examples/device-status.sh` gegen HTTPS testen. Das Dashboard wird alle 15 Sekunden aktualisiert.
6. Nach 3 Minuten ohne Statusmeldung zeigt die Webseite `Keine Verbindung`.

### Sicherheits-/Betriebshinweise
- Nicht mit `APP_DEBUG=true` oder `APP_ENV=local` öffentlich betreiben.
- Registrierungen sind zunächst offen; für echten Betrieb: E-Mail-Verifizierung, Rechtstexte, Datenschutzhinweise, Registrierungsschutz (z.B. Captcha), Audit-Logging, Backups und Monitoring ergänzen.
- API-Schlüssel sind serverseitig nur als SHA-256-Hash gespeichert. Bei Verlust neues Gerät/Token erzeugen und altes Gerät deaktivieren oder entfernen (administrative UI zur Sperrung ist noch nicht implementiert).
- Für *mehrere* Geräte je Dienstleister muss die Status-Aggregation/Disposition fachlich festgelegt werden. Aktuell gilt die letzte Meldung des zuletzt sendenden Geräts für den gesamten Dienstleister.
- Die Karte ist eine **schematische Vorschau**, noch keine exakten Regionsgrenzen.
- Die Arduino-Skizze enthält **noch keinen produktiven HTTPS-Versand**, bewusst keine unsichere Umgehung der TLS-Prüfung. Als nächster Schritt muss die genaue GIGA-TLS-Bibliothek/Version festgelegt und am Board getestet werden.
