# Multichat – OBS Studio Plugin

Natives OBS-Studio-Plugin (C++/Qt6) für Arch Linux. Es vereint Twitch- und
YouTube-Live-Chat sowie Alerts direkt in OBS. Kein Browser-Source, kein
Electron, keine externe App, kein Overlay.

## Funktionen

- Zwei Docks in OBS:
  - **Multichat** – kombinierter Chat beider Plattformen, jede Nachricht mit
    einem Plattform-Icon (rundes Markenfarben-Badge) statt Textkürzel. Unten
    ein Zahnrad-Button, der das Einstellungsfenster öffnet.
  - **Multichat Alerts** – eigenes Dock. Jeder Alert hat einen Haken-Button;
    ein Klick blendet ihn einzeln mit Animation aus (kein Sammel-Löschen).
- **Einstellungen** als eigenes Fenster über **Werkzeuge → Multichat Settings**
  oder den Zahnrad-Button im Chat-Dock. (OBS bietet keine öffentliche API, um
  eine eigene Seite in den eingebauten Einstellungen-Dialog einzuhängen.)
- Bequemer Login: Klick auf „Anmelden“ öffnet automatisch den System-Browser;
  nach dem Bestätigen schließt das Plugin die Anmeldung selbst ab.
- Alerts:
  - Twitch: Follow, Subscription, Gift Subs, Bits/Cheers, Raids
  - YouTube: Membership, Super Chat, Super Sticker
- Tokens und Client-Secrets liegen im System-Schlüsselbund (libsecret). Läuft
  kein Secret-Service-Dienst, werden sie als Fallback in
  `~/.config/obs-multichat/tokens.json` (Rechte 0600) gespeichert, damit der
  Login einen OBS-Neustart übersteht.
- Falls der automatische Browser-Login scheitert (Browser öffnet nicht oder
  Port 3000 belegt), bietet das Plugin eine manuelle Anleitung mit Einfügen der
  Weiterleitungs-Adresse.
- Konfiguration unter `~/.config/obs-multichat/config.json` (ohne Secrets).

## Voraussetzungen

OBS Studio 30+ (Qt6) mit Entwicklungsdateien:

```
sudo pacman -S --needed base-devel cmake obs-studio qt6-base qt6-websockets libsecret pkgconf
```

## Bauen & Installieren

### Variante A: Paket via makepkg (systemweit, /usr)

Aus dem Projektverzeichnis:

```
makepkg -si
```

### Variante B: Update-Skript (sauberer Neubau + pacman)

```
./scripts/update.sh     # löscht den alten Build, baut neu, installiert via pacman
```

Das Skript ruft intern `makepkg -sif` auf (pacman fragt nach dem sudo-Passwort).
Optional nur bauen ohne Installation: `./scripts/build.sh`.

Hinweis: Falls du das Plugin früher einmal ins Benutzerverzeichnis kopiert hast,
entferne diese Kopie, damit es nicht doppelt geladen wird:
`rm -rf ~/.config/obs-studio/plugins/multichat`

Danach OBS neu starten. Die Docks erscheinen unter **Docks → Multichat** und
**Docks → Multichat Alerts**; die Einstellungen unter **Werkzeuge → Multichat
Settings**.

## Ein-Klick-Login (Felder ausblenden)

Ein Login **ganz ohne** Client ID ist bei Twitch und Google technisch nicht
möglich — jede „Mit … anmelden“-Schaltfläche hat eine Client ID, die der
Entwickler fest eingebaut hat. Du kannst dir aber genau dieses Erlebnis bauen:
trag deine IDs **einmal** in `credentials.env` ein, dann werden sie beim Bauen
fest eingebacken, die Eingabefelder verschwinden und es bleibt nur der
Login-Button (klicken → Weiterleitung → angemeldet).

1. Apps einmalig registrieren (siehe „OAuth einrichten“ unten):
   - Twitch: Public client, Redirect `http://localhost:3000` → Client ID
   - Google: Desktop-App → Client ID + Secret
2. In `credentials.env` eintragen:
   ```
   TWITCH_CLIENT_ID=deine_twitch_client_id
   YOUTUBE_CLIENT_ID=deine_google_client_id
   YOUTUBE_CLIENT_SECRET=dein_google_secret
   ```
3. `./scripts/update.sh` (oder `makepkg -sif`) – fertig.

Leere Werte lassen das jeweilige Feld weiterhin im Einstellungsfenster sichtbar,
sodass du die ID stattdessen dort eintragen kannst (bleibt dauerhaft gespeichert).
Hinweis: Beim erneuten Entpacken eines neuen Tarballs wird `credentials.env`
überschrieben — vorher sichern oder Werte erneut eintragen.



Das Plugin liefert keine fremden Zugangsdaten mit; du trägst im
Einstellungsfenster deine eigenen ein.

### Twitch (Implicit Flow, ohne Secret + Loopback)

Twitch unterstützt kein PKCE; für einen Login **ohne Client-Secret** wird daher
der Implicit-Flow genutzt. Nachteil: kein Refresh-Token — läuft das Token ab,
einfach erneut „Mit Twitch anmelden“ klicken.

1. <https://dev.twitch.tv/console/apps> → „Anwendung registrieren“ (2FA am Konto
   muss aktiviert sein).
2. **OAuth-Redirect-URL: `http://localhost:3000`** (exakt so).
3. Kategorie beliebig. **Client-Type: Public** (kein Secret nötig).
4. Erstellen, dann „Verwalten“: **Client ID** notieren.
5. Im Einstellungsfenster Twitch Client ID und Kanalname (Login-Name, klein)
   eintragen, **Speichern**.

Scopes: `chat:read chat:edit bits:read channel:read:subscriptions
moderator:read:followers`. Für Follow-Alerts musst du Moderator/Broadcaster des
Kanals sein. Wichtig: Port 3000 muss beim Login frei sein.

### YouTube / Google (Authorization Code Flow mit PKCE + Loopback)

1. <https://console.cloud.google.com/> → Projekt anlegen.
2. „YouTube Data API v3“ aktivieren.
3. OAuth-Zustimmungsbildschirm: Nutzertyp **Extern**, dich als **Testnutzer**
   eintragen.
4. Anmeldedaten → „OAuth-Client-ID erstellen“ → Anwendungstyp **„Desktop-App“**
   (erlaubt die Loopback-Anmeldung).
5. Client ID und Client Secret im Einstellungsfenster eintragen, **Speichern**.

Scope: `https://www.googleapis.com/auth/youtube.readonly`. Google verlangt das
Client-Secret auch mit PKCE (bei „Desktop-App“ gilt es nicht als geheim), daher
bleibt das Secret-Feld bei YouTube nötig. Im Status „Testing“ läuft der
Refresh-Token nach 7 Tagen ab; zum Vermeiden die App veröffentlichen. YouTube
findet nur bei **aktivem** Livestream einen Live-Chat.

## Benutzung

1. OBS starten, **Werkzeuge → Multichat Settings** öffnen.
2. Zugangsdaten eintragen, **Speichern**.
3. „Mit Twitch anmelden“ bzw. „Mit YouTube anmelden“ – der Browser öffnet sich
   automatisch, Zugriff bestätigen, fertig (kein Code abtippen).
4. Chat und Alerts erscheinen in ihren jeweiligen Docks.

## Hinweis zu den Icons

Die Plattform-Icons im Chat sind eigene, schlicht gehaltene Symbole in den
Markenfarben (Sprechblase für Twitch, Play-Dreieck für YouTube). Die offiziellen,
markenrechtlich geschützten Logodateien werden bewusst nicht mitgeliefert.

## Lizenz

GPL-2.0
