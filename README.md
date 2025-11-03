# ESP32-Homecenter

Projekt urządzenia zarządzającego (master) innymi urządzeniami (slave) w rodzinie. Komunikacja za pomocą tematów MQTT, każde urządzenie przy starcie broadcastuje swój temat na którym są wysyłane instrukcje, w przypadku większej ilości obiektów do sterowania za pomocą jednego slave'a każdy obiekt może mieć swój temat.

Projekt urządzeń sterowanych jest w procesie tworzenia, po weryfikacji działania projektu sterującego zajmę się drugą częścią, ponieważ duża część kodu jest wspólna.

## Kompilacja

Do skompilowania programu potrzebny jest ESP-IDF v5.3+.

_idf.py build_ oraz _idf.py flash_

Schemat podłączeń dostępny w [**schemat.pdf**](./schemat.pdf)

## Implementacja

Projekt według rad na targach pracy używa driverów zamiast bezpośredniej ingerencji w rejestry.
Oparty jest o bibliotekę [Espressif-MQTT](https://components.espressif.com/components/espressif/mqtt/versions/1.0.0/readme).

Wykorzystuje przełączniki CherryMX oraz wyświetlacz OLED SH1106.
