/* SoftAP bring-up for Klin apps on GD32VW553.
 * Real path: GigaDevice VW55x Wi-Fi BLE SDK (`wifi_management_ap_start`).
 * Host path: stubs when SDK headers are not on the include path (klin test).
 */
#include "ap_sdk.h"

#include <stdio.h>
#include <string.h>

#if defined(__has_include)
#if __has_include("wifi_management.h")
#define KLIN_GD32V_WIFI_HAVE_SDK 1
#endif
#endif

#ifdef KLIN_GD32V_WIFI_HAVE_SDK
#include "wifi_management.h"
#include "wifi_net_ip.h"
#include "wrapper_os.h"
#endif

static int s_inited;
static int s_started;
static int s_last_start;
static uint32_t s_ip;
static uint32_t s_gw;
static uint32_t s_mask;

#ifdef KLIN_GD32V_WIFI_HAVE_SDK

static int klin_gd32v_wifi_ap_refresh_ip(void)
{
    struct wifi_ip_addr_cfg cfg;
    memset(&cfg, 0, sizeof(cfg));
    if (wifi_get_vif_ip(0, &cfg) != 0) {
        return -1;
    }
    s_ip = cfg.ipv4.addr;
    s_gw = cfg.ipv4.gw;
    s_mask = cfg.ipv4.mask;
    return 0;
}

int klin_gd32v_wifi_ap_init(void)
{
    int e;

    if (s_inited) {
        return 0;
    }
    e = wifi_management_init();
    if (e != 0) {
        return e;
    }
    s_inited = 1;
    return 0;
}

int klin_gd32v_wifi_ap_start(const char *ssid, const char *pass, int channel)
{
    char ssid_buf[33];
    char pass_buf[64];
    size_t n;
    wifi_ap_auth_mode_t auth;
    char *pass_arg;

    if (!s_inited) {
        return -1;
    }
    if (ssid == NULL || ssid[0] == '\0') {
        return -1;
    }
    n = strlen(ssid);
    if (n > 32) {
        return -1;
    }
    if (channel < 1 || channel > 13) {
        return -1;
    }
    memcpy(ssid_buf, ssid, n + 1);
    if (pass == NULL || pass[0] == '\0') {
        pass_arg = NULL;
        auth = AUTH_MODE_OPEN;
    } else {
        n = strlen(pass);
        if (n < 8 || n > 63) {
            return -1;
        }
        memcpy(pass_buf, pass, n + 1);
        pass_arg = pass_buf;
        auth = AUTH_MODE_WPA2;
    }
    s_last_start = wifi_management_ap_start(ssid_buf, pass_arg, (uint32_t)channel, auth, 0);
    s_started = (s_last_start == 0);
    if (s_started) {
        (void)klin_gd32v_wifi_ap_refresh_ip();
    }
    return s_last_start;
}

int klin_gd32v_wifi_ap_wait_started(int timeout_ms)
{
    (void)timeout_ms;
    return s_last_start;
}

int klin_gd32v_wifi_ap_started(void)
{
    return s_started;
}

int klin_gd32v_wifi_ap_wait_ip(int timeout_ms)
{
    int waited;

    if (!s_started) {
        return -1;
    }
    waited = 0;
    while (1) {
        if (klin_gd32v_wifi_ap_refresh_ip() == 0 && s_ip != 0) {
            return 0;
        }
        if (timeout_ms >= 0 && waited >= timeout_ms) {
            return -1;
        }
        sys_ms_sleep(1);
        waited = waited + 1;
    }
}

uint32_t klin_gd32v_wifi_ap_ip_u32(void)
{
    (void)klin_gd32v_wifi_ap_refresh_ip();
    return s_ip;
}

uint32_t klin_gd32v_wifi_ap_gateway_u32(void)
{
    (void)klin_gd32v_wifi_ap_refresh_ip();
    return s_gw;
}

uint32_t klin_gd32v_wifi_ap_netmask_u32(void)
{
    (void)klin_gd32v_wifi_ap_refresh_ip();
    return s_mask;
}

int klin_gd32v_wifi_ap_stop(void)
{
    s_started = 0;
    s_ip = 0;
    s_gw = 0;
    s_mask = 0;
    return wifi_management_ap_stop();
}

#else /* host stubs — no SDK headers */

int klin_gd32v_wifi_ap_init(void)
{
    s_inited = 1;
    return 0;
}

int klin_gd32v_wifi_ap_start(const char *ssid, const char *pass, int channel)
{
    size_t n;

    if (!s_inited || ssid == NULL || ssid[0] == '\0') {
        s_last_start = -1;
        s_started = 0;
        return -1;
    }
    n = strlen(ssid);
    if (n > 32) {
        s_last_start = -1;
        s_started = 0;
        return -1;
    }
    if (channel < 1 || channel > 13) {
        s_last_start = -1;
        s_started = 0;
        return -1;
    }
    if (pass != NULL && pass[0] != '\0') {
        n = strlen(pass);
        if (n < 8 || n > 63) {
            s_last_start = -1;
            s_started = 0;
            return -1;
        }
    }
    s_last_start = 0;
    s_started = 1;
    /* 192.168.4.1 / .1 / 255.255.255.0 — same pack as Klin `ipv4` */
    s_ip = 192u | (168u << 8) | (4u << 16) | (1u << 24);
    s_gw = 192u | (168u << 8) | (4u << 16) | (1u << 24);
    s_mask = 255u | (255u << 8) | (255u << 16) | (0u << 24);
    return 0;
}

int klin_gd32v_wifi_ap_wait_started(int timeout_ms)
{
    (void)timeout_ms;
    return s_last_start;
}

int klin_gd32v_wifi_ap_started(void)
{
    return s_started;
}

int klin_gd32v_wifi_ap_wait_ip(int timeout_ms)
{
    (void)timeout_ms;
    if (!s_started || s_ip == 0) {
        return -1;
    }
    return 0;
}

uint32_t klin_gd32v_wifi_ap_ip_u32(void)
{
    return s_ip;
}

uint32_t klin_gd32v_wifi_ap_gateway_u32(void)
{
    return s_gw;
}

uint32_t klin_gd32v_wifi_ap_netmask_u32(void)
{
    return s_mask;
}

int klin_gd32v_wifi_ap_stop(void)
{
    s_started = 0;
    s_ip = 0;
    s_gw = 0;
    s_mask = 0;
    return 0;
}

#endif

void klin_gd32v_wifi_ap_log_ip_info(void)
{
    unsigned a = (unsigned)(s_ip & 255u);
    unsigned b = (unsigned)((s_ip >> 8) & 255u);
    unsigned c = (unsigned)((s_ip >> 16) & 255u);
    unsigned d = (unsigned)((s_ip >> 24) & 255u);
    printf("gd32v_wifi ap %u.%u.%u.%u\n", a, b, c, d);
}
