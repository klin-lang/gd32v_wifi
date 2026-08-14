# gd32v_wifi

Thin **GigaDevice VW55x Wi‑Fi** bindings for [Klin](https://github.com/klin-lang/klin)
(**STA** + **SoftAP** + **scan** + **link** + **static IP**).

The radio is in the **silicon**; this package does **not** belong in
[`machine_gd32v`](https://github.com/klin-lang/machine_gd32v) (MMIO Pin…Adc).
Same split as MicroPython `machine` vs `network` — see Klin
[061](https://github.com/klin-lang/klin/blob/main/issues/061-micropython-machine-api.md)
and [126](https://github.com/klin-lang/klin/blob/main/issues/126-gd32v-wifi-sdk.md).

C engine = **[GD32VW55x_WiFi_BLE_SDK](https://github.com/GigaDeviceSemiconductor/GD32VW55x_WiFi_BLE_SDK)**
(`wifi_management`, `wifi_net_ip`, lwIP, OSAL). Klin is a thin FFI client
(`@[link("sta_sdk.c")]` / `@[link("ap_sdk.c")]` + `@[cimport]`). SDK heap /
management task / eloop / DHCP(S) are **SDK contracts**, not hidden Klin
allocation.

**Not** [`esp_wifi`](https://github.com/klin-lang/esp_wifi) — that is ESP-IDF.

**STA IP mode:** default = **DHCP (dynamic)**; optional `sta_set_static_ip` /
`sta_set_hostname`. **SoftAP IP:** SDK default (typically `192.168.4.1` + DHCPS);
optional `ap_set_ip`. Do **not** mix `sta_*` and `ap_*` on this tag (APSTA later).
Scan needs `sta_init` (SoftAP-only cannot scan). BLE is a later package
(`gd32v_ble`).

## Status (`@v0.4.0`)

| API | Notes |
|---|---|
| `sta_init` | `wifi_management_init` (once) |
| `sta_set_static_ip(ip, gw, mask)` | Optional `wifi_set_vif_ip` / `IP_ADDR_STATIC_IPV4`. `0,0,0` = DHCP. Prefer before `sta_connect` |
| `sta_set_hostname(name)` | Optional `wifi_vif_hostname_set` (vif 0). Empty = SDK default |
| `sta_connect(ssid, pass)` | `wifi_management_connect(..., blocked=1)` (AN158 §5.2) |
| `sta_wait_connected` / `sta_connected` | Last blocked-connect result |
| `sta_wait_ip(timeout_ms)` | Poll `wifi_get_vif_ip` vif 0 (`-1` = forever) |
| `sta_ip_u32` / `sta_gateway_u32` / `sta_netmask_u32` | After DHCP or static |
| `sta_rssi` / `sta_channel` / `sta_authmode` | Associated-AP link after `sta_connected`. Each call → SDK |
| `sta_ap_ssid(out, max_len)` | Associated SSID into **caller** buffer |
| `sta_log_link` | Debug `printf` of rssi/channel/auth/ssid |
| `sta_disconnect` / `sta_stop` | `wifi_management_disconnect` / `deinit` |
| `sta_log_ip_info` | Debug `printf` |
| `ap_init` | Same `wifi_management_init` (once) |
| `ap_set_ip(ip, gw, mask)` | Optional AP IPv4 + DHCPS (`IP_ADDR_DHCP_SERVER`). `0,0,0` = SDK default |
| `ap_start(ssid, pass, channel)` | `wifi_management_ap_start` (AN158 §5.3). Empty pass = OPEN; else WPA2 (`AUTH_MODE_WPA2`). Channel `1`…`13` |
| `ap_wait_started` / `ap_started` | Last `ap_start` result (`i32` 1/0) |
| `ap_wait_ip` / `ap_ip_u32` / `ap_gateway_u32` / `ap_netmask_u32` | SoftAP vif 0 |
| `ap_station_num` | Associated STA count (`macif_vif_ap_assoc_info_get`) |
| `ap_stop` | `wifi_management_ap_stop` (does not `deinit`) |
| `ap_log_ip_info` | Debug `printf` |
| `scan_start(timeout_ms)` | `wifi_management_scan(blocked=1)` (AN158 §5.1). After `sta_init`. Fixed table of **16** APs. `timeout_ms` unused (SDK blocks) |
| `scan_max` / `scan_count` | Cap / stored count |
| `scan_ssid(index, out, max_len)` | Copy SSID into **caller** buffer |
| `scan_rssi` / `scan_channel` / `scan_authmode` / `scan_log` | After `scan_start`. Auth = `wifi_ap_auth_mode_t` |
| `err_ok` / `ipv4` | Same shape as `esp_wifi` |

`version()` → `4`.

Host `klin test` uses stubs when `wifi_management.h` is not on the include path
(`__has_include`). Do **not** call the factory-style init on a host and expect
RF.

## Requirements

- [Klin](https://github.com/klin-lang/klin) compiler
- Official SDK on the include/link path to build a board ELF
- AN158 Wi‑Fi Development Guide (GigaDevice)

## Layout

```text
gd32v_wifi/
  version.kl
  sta.kl / ap.kl / scan.kl
  sta_sdk.c / ap_sdk.c
  *_test.kl
examples/sta_connect/ # needs SDK to link
examples/softap/      # needs SDK to link
examples/scan/        # needs SDK to link
examples/smoke/       # host emit-c
```

## Usage (STA)

```klin
import "github/klin-lang/gd32v_wifi" wifi

fn main() {
    let mut e = wifi.sta_init()
    if e != wifi.err_ok() {
        return
    }
    e = wifi.sta_connect("myssid", "mypass")
    if e != wifi.err_ok() {
        return
    }
    e = wifi.sta_wait_ip(20000)
    if e != wifi.err_ok() {
        return
    }
    wifi.sta_log_ip_info()
    wifi.sta_log_link()
}
```

Optional static IPv4 (prefer before `sta_connect`):

```klin
    let _h = wifi.sta_set_hostname("klin-wifi")
    let _s = wifi.sta_set_static_ip(
        wifi.ipv4(10, 0, 0, 20),
        wifi.ipv4(10, 0, 0, 1),
        wifi.ipv4(255, 255, 255, 0)
    )
```

## Usage (SoftAP)

```klin
import "github/klin-lang/gd32v_wifi" wifi

fn main() {
    let mut e = wifi.ap_init()
    if e != wifi.err_ok() {
        return
    }
    e = wifi.ap_start("klin-ap", "klinpass1", 6)
    if e != wifi.err_ok() {
        return
    }
    e = wifi.ap_wait_ip(5000)
    if e != wifi.err_ok() {
        return
    }
    wifi.ap_log_ip_info()
    let _n = wifi.ap_station_num()
}
```

Optional AP IPv4 + DHCPS (prefer before `ap_start`):

```klin
    let _ip = wifi.ap_set_ip(
        wifi.ipv4(192, 168, 8, 1),
        wifi.ipv4(192, 168, 8, 1),
        wifi.ipv4(255, 255, 255, 0)
    )
```

## Usage (scan)

```klin
import "github/klin-lang/gd32v_wifi" wifi

fn main() {
    let mut e = wifi.sta_init()
    if e != wifi.err_ok() {
        return
    }
    e = wifi.scan_start(15000)
    if e != wifi.err_ok() {
        return
    }
    wifi.scan_log()
    let mut ssid: [33]u8
    let _n = wifi.scan_ssid(0, cast(*mut u8, &ssid[0]), 33)
}
```

```sh
klin get github/klin-lang/gd32v_wifi@v0.4.0
```

## Tests

```sh
dart run /path/to/klin/bin/klin.dart test gd32v_wifi/
```

## License

MIT
