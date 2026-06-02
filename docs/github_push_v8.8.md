# GitHub push v8.8

Wersja kodu do wypchniecia:

- `v8.8_TB`
- branch: `main`
- tag do utworzenia: `v8.8`

Zakres zmian 8.8:

- nowy ekran `SD_PLAYER`
- sterowanie dotykiem na `SD_PLAYER`
- okladki albumow
- rozszerzone ustawienia zegara, motywu i NeoPixel
- poprawki trybow pracy, SD resume i WWW

Artefakty release trzymane lokalnie, nie do commita:

- `zbudujradioadmin/firmware/yoRadio_8.8_ILI9488_*.bin`
- `zbudujradioadmin/firmware/spiffs_8.8.bin`
- `zbudujradioadmin/firmware/www.zip`

Sugerowany commit:

- `v8.8`

Sugerowane komendy po weryfikacji:

```powershell
git push origin main
git push origin v8.8
```
