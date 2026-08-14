/* Thin STA helpers for Klin over the GigaDevice VW55x Wi-Fi SDK.
 * Heap / OSAL task / eloop / lwIP DHCP are SDK contracts (not Klin magic).
 * DHCP (dynamic IP) is the default on this tag.
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif
