# Im AUR veröffentlichen

Das AUR baut Pakete sauber aus einer **öffentlich herunterladbaren Quelle**.
Darum brauchst du zwei Schritte: (A) den Code auf GitHub veröffentlichen, dann
(B) ein AUR-Paket anlegen, das genau diese Release-Quelle baut.

Wichtig: Die AUR-Version backt **keine** OAuth-Schlüssel ein (öffentliche Pakete
dürfen keine Geheimnisse enthalten). Nutzer tragen ihre Client-ID/Secret zur
Laufzeit in `~/.config/obs-multichat/credentials.env` ein oder direkt im
Einstellungsfenster. Das Plugin liest beides automatisch.

## A) Code auf GitHub veröffentlichen

1. Repo anlegen (z. B. `obs-multichat`) und den Projektinhalt pushen:
   ```
   git init
   git add .
   git commit -m "obs-multichat 3.3.6"
   git branch -M main
   git remote add origin https://github.com/DEINUSER/obs-multichat.git
   git push -u origin main
   ```
2. Eine Version taggen (muss zur pkgver passen, mit fuehrendem v):
   ```
   git tag v3.3.6
   git push origin v3.3.6
   ```
   GitHub stellt daraus automatisch
   `https://github.com/DEINUSER/obs-multichat/archive/refs/tags/v3.3.6.tar.gz`
   bereit – genau das laedt die PKGBUILD.

Tipp: Lege eine echte `LICENSE` (GPL-2.0) ins Repo. Committe NICHT deine private
`credentials.env`.

## B) AUR-Paket anlegen

Voraussetzungen einmalig:
- AUR-Konto auf https://aur.archlinux.org anlegen und einen SSH-Public-Key im
  Profil hinterlegen.
- Lokal `base-devel` und `git` installiert.

1. Pruefe, ob der Name frei ist: https://aur.archlinux.org/packages/?K=obs-multichat
   (sonst eindeutigen Namen waehlen, z. B. `obs-multichat-git`).
2. Leeres AUR-Repo klonen:
   ```
   git clone ssh://aur@aur.archlinux.org/obs-multichat.git
   cd obs-multichat
   ```
3. Die Datei `packaging/aur/PKGBUILD` aus diesem Projekt hineinkopieren und
   `DEINUSER` sowie die Maintainer-Zeile anpassen.
4. Echte Pruefsummen eintragen (statt SKIP) – empfohlen fuers AUR:
   ```
   updpkgsums            # aus pacman-contrib; fuellt sha256sums automatisch
   ```
5. Lokal testen, dass es sauber baut und installiert:
   ```
   makepkg -si
   ```
6. `.SRCINFO` erzeugen (Pflicht im AUR):
   ```
   makepkg --printsrcinfo > .SRCINFO
   ```
7. Committen und pushen:
   ```
   git add PKGBUILD .SRCINFO
   git commit -m "obs-multichat 3.3.6"
   git push
   ```

Fertig – andere koennen es dann mit einem AUR-Helper installieren:
```
yay -S obs-multichat
```

## Updates spaeter

Bei einer neuen Version: `pkgver` in der PKGBUILD erhoehen, neuen Git-Tag
`vX.Y.Z` auf GitHub pushen, dann im AUR-Repo `updpkgsums`,
`makepkg --printsrcinfo > .SRCINFO`, committen und pushen.

## Hinweise zu AUR-Richtlinien

- Keine Binaerdateien oder Geheimnisse im AUR-Repo – nur PKGBUILD und .SRCINFO
  (und ggf. .install/Patches).
- `pkgrel` bei reinen Packaging-Aenderungen erhoehen, `pkgver` nur bei neuer
  Upstream-Version.
- Wenn du lieber immer den neuesten Stand baust, biete zusaetzlich ein
  `obs-multichat-git`-Paket an (source per git, pkgver()-Funktion).
