# STA connect on GD32VW553

Hardware demo for [`gd32v_wifi`](../../README.md) `sta_connect`.

`make emit` produces C. Linking an ELF needs
[GD32VW55x_WiFi_BLE_SDK](https://github.com/GigaDeviceSemiconductor/GD32VW55x_WiFi_BLE_SDK)
on the include/link path (AN158). Edit SSID/pass in `sta.kl`.

```sh
make emit KLIN=/path/to/klin/bin/klin.dart
```

Do not use [`esp_wifi`](https://github.com/klin-lang/esp_wifi) on this SoC
(ESP-IDF, wrong engine).
