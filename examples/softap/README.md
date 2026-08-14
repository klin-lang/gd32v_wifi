# SoftAP on GD32VW553

Hardware demo for [`gd32v_wifi`](../../README.md) `ap_init` / `ap_start` / `ap_station_num`.

Needs [GD32VW55x_WiFi_BLE_SDK](https://github.com/GigaDeviceSemiconductor/GD32VW55x_WiFi_BLE_SDK)
on the include/link path to produce an ELF. Host `make emit` uses C stubs.

Default: SSID `klin-ap`, WPA2 `klinpass1`, channel 6. Typical AP IPv4
`192.168.4.1`. Edit `ap.kl`. Do not mix with `sta_*` on this tag (APSTA later).

```sh
make emit KLIN=/path/to/klin/bin/klin.dart
```
