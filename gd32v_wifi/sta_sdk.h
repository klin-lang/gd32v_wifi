/* Thin STA helpers for Klin over the GigaDevice VW55x Wi-Fi SDK.
 * Heap / OSAL task / eloop / lwIP DHCP are SDK contracts (not Klin magic).
 * DHCP (dynamic IP) is the default on this tag.
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Optional static IPv4 (`wifi_set_vif_ip` / `IP_ADDR_STATIC_IPV4`). 0,0,0 = DHCP. */
int klin_gd32v_wifi_sta_set_static_ip(uint32_t ip, uint32_t gw, uint32_t netmask);

/** Optional DHCP hostname (`wifi_vif_hostname_set`). Empty = SDK default. */
int klin_gd32v_wifi_sta_set_hostname(const char *name);

/** `wifi_management_init` — LwIP + eloop + management task. Call once. */
int klin_gd32v_wifi_sta_init(void);

/** `wifi_management_connect(ssid, pass, blocked=1)` (AN158 §5.2). */
int klin_gd32v_wifi_sta_connect(const char *ssid, const char *pass);

/** Last blocked-connect result (0 = ok). timeout_ms unused when already done. */
int klin_gd32v_wifi_sta_wait_connected(int timeout_ms);

/** 1 after a successful `sta_connect`. */
int klin_gd32v_wifi_sta_connected(void);

/** Poll `wifi_get_vif_ip` (vif 0) until IPv4 or timeout_ms (-1 = forever). */
int klin_gd32v_wifi_sta_wait_ip(int timeout_ms);

uint32_t klin_gd32v_wifi_sta_ip_u32(void);
uint32_t klin_gd32v_wifi_sta_gateway_u32(void);
uint32_t klin_gd32v_wifi_sta_netmask_u32(void);

int klin_gd32v_wifi_sta_disconnect(void);
int klin_gd32v_wifi_sta_stop(void);
void klin_gd32v_wifi_sta_log_ip_info(void);

/** Associated-AP link (after `sta_connected`). Each call → SDK. */
int klin_gd32v_wifi_sta_rssi(void);
int klin_gd32v_wifi_sta_channel(void);
int klin_gd32v_wifi_sta_authmode(void);
int klin_gd32v_wifi_sta_ap_ssid(char *out, int max_len);
void klin_gd32v_wifi_sta_log_link(void);

/** Max APs kept after `klin_gd32v_wifi_scan_start` (fixed; documented). */
int klin_gd32v_wifi_scan_max(void);

/** `wifi_management_scan(blocked=1)` (AN158 §5.1). Needs `sta_init`.
 * Copies up to `scan_max` APs into a fixed C table (no Klin heap).
 * timeout_ms unused — the SDK blocks until SCAN_DONE.
 */
int klin_gd32v_wifi_scan_start(int timeout_ms);

int klin_gd32v_wifi_scan_count(void);
int klin_gd32v_wifi_scan_rssi(int index);
int klin_gd32v_wifi_scan_channel(int index);
/** `wifi_ap_auth_mode_t` as i32 (0 = OPEN, 3 = WPA2, …). */
int klin_gd32v_wifi_scan_authmode(int index);
/** Copy SSID into caller buffer. Returns length excluding NUL, or -1. */
int klin_gd32v_wifi_scan_ssid(int index, char *out, int max_len);
void klin_gd32v_wifi_scan_log(void);

#ifdef __cplusplus
}
#endif
