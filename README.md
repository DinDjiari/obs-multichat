# Multichat – OBS Studio Plugin

Natives OBS-Studio-Plugin (C++/Qt6) für Arch Linux. Es erscheint als Dock
namens **Multichat** direkt in OBS und vereint Twitch- und YouTube-Live-Chat
sowie Alerts. Kein Browser-Source, kein Electron, keine externe App, kein
localhost-Webserver, kein Overlay.

## Funktionen

- Twitch- und YouTube-Login direkt im Dock (OAuth 2.0 Device Flow, System-Browser)
- Twitch-Chat (IRC über WebSocket) und YouTube-Live-Chat (Data API v3) in einer Liste
- Plattform-Badge (TW / YT) pro Nachricht
- Alerts:
  - Twitch: Follow, Subscription, Gift Subs, Bits/Cheers, Raids
  - YouTube: Membership, Super Chat, Super Sticker
- OBS-Dark-Theme
- Tabs: **Chat | Alerts | Einstellungen**
- Tokens sicher im System-Schlüsselbund (libsecret / Secret Service)
- Konfiguration unter `~/.config/obs-multichat/config.json`

## Voraussetzungen

OBS Studio 30+ (Qt6) mit Entwicklungsdateien, dazu:

```
sudo pacman -S --needed base-devel cmake obs-studio qt6-base qt6-websockets libsecret pkgconf
```

## Bauen & Installieren (Arch Linux)

Aus dem Projektverzeichnis (dort, wo die `PKGBUILD` liegt):

```
makepkg -si
```

Das Plugin wird nach `/usr/lib/obs-plugins/multichat.so` installiert, die
Daten nach `/usr/share/obs/obs-plugins/multichat/`.

### Alternativ: direkt mit CMake (ohne Paket)

```
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build --parallel
sudo cmake --install build
```

Danach OBS neu starten. Das Dock erscheint unter **Docks → Multichat**
(falls nicht sichtbar, im Menü „Docks“ aktivieren).

## OAuth einrichten

Aus Sicherheitsgründen liefert das Plugin keine fremden API-Schlüssel mit. Du
trägst deine eigenen Zugangsdaten im Tab **Einstellungen** ein.

### Twitch

1. <https://dev.twitch.tv/console/apps> → „Register Your Application“.
2. OAuth-Redirect-URL: `http://localhost` (wird beim Device Flow nicht genutzt,
   ist aber ein Pflichtfeld).
3. Kategorie: beliebig (z. B. „Application Integration“).
4. **Client-Type: Public** (Device Flow benötigt einen öffentlichen Client).
5. Client-ID kopieren und im Tab Einstellungen unter „Twitch Client ID“ eintragen.
6. Den Twitch-Kanalnamen (Login-Name, klein) eintragen.

Verwendete Scopes: `chat:read chat:edit bits:read channel:read:subscriptions
moderator:read:followers`.

### YouTube / Google

1. <https://console.cloud.google.com/> → Projekt anlegen.
2. „YouTube Data API v3“ aktivieren.
3. OAuth-Zustimmungsbildschirm konfigurieren (Testnutzer = dein Konto reicht).
4. Anmeldedaten → „OAuth-Client-ID erstellen“ → Anwendungstyp
   **„Fernseher und Geräte mit begrenzter Eingabe“** (TV and Limited Input).
5. Client-ID und Client-Secret im Tab Einstellungen eintragen.

Verwendeter Scope: `https://www.googleapis.com/auth/youtube.readonly`.

## Benutzung

1. OBS starten, Dock **Multichat** öffnen.
2. Tab **Einstellungen**: Twitch Client ID + Kanal sowie YouTube Client ID +
   Secret eintragen, **Speichern**.
3. „Mit Twitch anmelden“ bzw. „Mit YouTube anmelden“ klicken. Es erscheint ein
   Gerätecode; der wird in die Zwischenablage kopiert und der Browser geöffnet.
   Code eingeben, Zugriff bestätigen.
4. Nach erfolgreicher Anmeldung verbinden sich die Chats automatisch. YouTube
   benötigt einen **aktiven** Livestream, damit ein Live-Chat gefunden wird.

## Hinweise

- Twitch-Follows werden über EventSub (WebSocket) abonniert; dafür ist der Scope
  `moderator:read:followers` nötig und du musst Moderator/Broadcaster des Kanals sein.
- YouTube-API-Quote: Das Polling richtet sich nach `pollingIntervalMillis` der
  API (mindestens 2 s), um die Quote zu schonen.
- Tokens liegen im Secret Service (z. B. GNOME Keyring / KWallet-Bridge), nicht
  im Klartext in der Config.

## Lizenz

GPL-2.0
