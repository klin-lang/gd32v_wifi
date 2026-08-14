# Scan on GD32VW553

Hardware demo for [`gd32v_wifi`](../../README.md) `sta_init` / `scan_start`.

Needs [GD32VW55x_WiFi_BLE_SDK](https://github.com/GigaDeviceSemiconductor/GD32VW55x_WiFi_BLE_SDK)
on the include/link path to produce an ELF. Host `make emit` uses C stubs.

Needs `sta_init` first (SoftAP-only cannot scan). Results: up to 16 APs in a
fixed C table. SSID goes into a **caller** buffer.

```sh
make emit KLIN=/path/to/klin/bin/klin.dart
```
