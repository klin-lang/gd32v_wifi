/* Thin SoftAP helpers for Klin over the GigaDevice VW55x Wi-Fi SDK.
 * Heap / OSAL task / eloop / DHCPS are SDK contracts (not Klin magic).
 * Default AP IPv4 is the SDK SoftAP lease (typically 192.168.4.1).
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Optional AP IPv4 + DHCPS (`wifi_set_vif_ip` / `IP_ADDR_DHCP_SERVER`). 0,0,0 = SDK default. */
int klin_gd32v_wifi_ap_set_ip(uint32_t ip, uint32_t gw, uint32_t netmask);

/** `wifi_management_init` — same stack as STA. Call once. */
int klin_gd32v_wifi_ap_init(void);

/** `wifi_management_ap_start` (AN158 §5.3 / §4.4.8).
 * Empty pass → AUTH_MODE_OPEN; else AUTH_MODE_WPA2 (password length ≥ 8).
 * channel 1…13. hidden = 0 (broadcast SSID).
 */
int klin_gd32v_wifi_ap_start(const char *ssid, const char *pass, int channel);

/** Last blocked ap_start result (0 = ok). timeout_ms unused when already done. */
int klin_gd32v_wifi_ap_wait_started(int timeout_ms);

/** 1 after a successful `ap_start`. */
int klin_gd32v_wifi_ap_started(void);

/** Poll `wifi_get_vif_ip` (vif 0) until IPv4 or timeout_ms (-1 = forever). */
int klin_gd32v_wifi_ap_wait_ip(int timeout_ms);

uint32_t klin_gd32v_wifi_ap_ip_u32(void);
uint32_t klin_gd32v_wifi_ap_gateway_u32(void);
uint32_t klin_gd32v_wifi_ap_netmask_u32(void);

int klin_gd32v_wifi_ap_stop(void);
void klin_gd32v_wifi_ap_log_ip_info(void);

/** Associated STA count (`macif_vif_ap_assoc_info_get`). 0 if not started. */
int klin_gd32v_wifi_ap_station_num(void);

#ifdef __cplusplus
}
#endif
