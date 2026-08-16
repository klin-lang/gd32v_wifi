/* STA bring-up for Klin apps on GD32VW553.
 * Real path: GigaDevice VW55x Wi-Fi BLE SDK (`wifi_management` / `wifi_net_ip`).
 * Host path: stubs when SDK headers are not on the include path (klin test).
 * `@v0.5.0` APSTA: `concurrent_set` / `concurrent_get` (needs CFG_WIFI_CONCURRENT).
 * `@v0.6.0` roaming: `roaming_set` / `roaming` / `roaming_rssi_th`.
 */
#include "sta_sdk.h"

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
#if defined(__has_include)
#if __has_include("wlan_config.h")
#include "wlan_config.h"
#endif
#if __has_include("macif_vif.h")
#include "macif_vif.h"
#define KLIN_GD32V_WIFI_HAVE_MACIF_VIF 1
#endif
#if __has_include("wifi_vif.h")
#include "wifi_vif.h"
#define KLIN_GD32V_WIFI_HAVE_WIFI_VIF 1
#endif
#if __has_include("wifi_netlink.h")
#include "wifi_netlink.h"
#define KLIN_GD32V_WIFI_HAVE_NETLINK 1
#endif
#endif
#if defined(CFG_WIFI_CONCURRENT)
#define KLIN_GD32V_WIFI_HAVE_CONCURRENT 1
#endif
#endif

#define KLIN_GD32V_WIFI_SCAN_MAX 16
#define KLIN_GD32V_WIFI_SSID_MAX 33
#define KLIN_GD32V_WIFI_HOSTNAME_MAX 32

typedef struct {
    char ssid[KLIN_GD32V_WIFI_SSID_MAX];
    int8_t rssi;
    uint8_t channel;
    uint8_t authmode;
} klin_gd32v_wifi_scan_row_t;

static int s_inited;
static int s_connected;
static int s_last_connect;
static uint32_t s_ip;
static uint32_t s_gw;
static uint32_t s_mask;
static klin_gd32v_wifi_scan_row_t s_scan[KLIN_GD32V_WIFI_SCAN_MAX];
static int s_scan_count;
static int s_use_static;
static uint32_t s_static_ip;
static uint32_t s_static_gw;
static uint32_t s_static_mask;
static char s_hostname[KLIN_GD32V_WIFI_HOSTNAME_MAX];
static char s_assoc_ssid[KLIN_GD32V_WIFI_SSID_MAX];
static uint8_t s_assoc_auth;
#ifndef KLIN_GD32V_WIFI_HAVE_SDK
static int s_concurrent;
static int s_roaming;
static int8_t s_roaming_rssi_th;
#endif

#ifdef KLIN_GD32V_WIFI_HAVE_SDK

static int klin_gd32v_wifi_refresh_ip(void)
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

static int klin_gd32v_wifi_apply_static_ip(void)
{
    struct wifi_ip_addr_cfg cfg;

    memset(&cfg, 0, sizeof(cfg));
    cfg.mode = IP_ADDR_STATIC_IPV4;
    cfg.default_output = 1;
    cfg.ipv4.addr = s_static_ip;
    cfg.ipv4.gw = s_static_gw;
    cfg.ipv4.mask = s_static_mask;
    return wifi_set_vif_ip(0, &cfg);
}

static int klin_gd32v_wifi_apply_hostname(void)
{
#ifdef KLIN_GD32V_WIFI_HAVE_WIFI_VIF
    if (s_hostname[0] == '\0') {
        return 0;
    }
    return wifi_vif_hostname_set(0, s_hostname);
#else
    return 0;
#endif
}

int klin_gd32v_wifi_sta_set_static_ip(uint32_t ip, uint32_t gw, uint32_t netmask)
{
    if (ip == 0 && gw == 0 && netmask == 0) {
        s_use_static = 0;
        s_static_ip = 0;
        s_static_gw = 0;
        s_static_mask = 0;
        return 0;
    }
    s_use_static = 1;
    s_static_ip = ip;
    s_static_gw = gw;
    s_static_mask = netmask;
    if (s_connected) {
        return klin_gd32v_wifi_apply_static_ip();
    }
    return 0;
}

int klin_gd32v_wifi_sta_set_hostname(const char *name)
{
    if (name == NULL || name[0] == '\0') {
        s_hostname[0] = '\0';
        return 0;
    }
    strncpy(s_hostname, name, sizeof(s_hostname) - 1);
    s_hostname[sizeof(s_hostname) - 1] = '\0';
    if (s_inited) {
        return klin_gd32v_wifi_apply_hostname();
    }
    return 0;
}

int klin_gd32v_wifi_sta_init(void)
{
    int e;

    if (s_inited) {
        return 0;
    }
#ifdef KLIN_GD32V_WIFI_HAVE_NETLINK
    /* APSTA: SoftAP path may already have called wifi_management_init. */
    if (wifi_work_status != WIFI_CLOSED && wifi_work_status != WIFI_CLOSING) {
        s_inited = 1;
        (void)klin_gd32v_wifi_apply_hostname();
        return 0;
    }
#endif
    e = wifi_management_init();
    if (e != 0) {
        return e;
    }
    s_inited = 1;
    (void)klin_gd32v_wifi_apply_hostname();
    return 0;
}

int klin_gd32v_wifi_sta_connect(const char *ssid, const char *pass)
{
    char ssid_buf[33];
    char pass_buf[64];
    size_t n;

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
    memcpy(ssid_buf, ssid, n + 1);
    if (pass == NULL) {
        pass_buf[0] = '\0';
    } else {
        n = strlen(pass);
        if (n > 63) {
            return -1;
        }
        memcpy(pass_buf, pass, n + 1);
    }
    s_last_connect = wifi_management_connect(ssid_buf, pass_buf, 1);
    s_connected = (s_last_connect == 0);
    if (s_connected) {
        memset(s_assoc_ssid, 0, sizeof(s_assoc_ssid));
        memcpy(s_assoc_ssid, ssid_buf, strlen(ssid_buf));
        s_assoc_auth = 3;
        if (s_use_static) {
            (void)klin_gd32v_wifi_apply_static_ip();
        }
    }
    return s_last_connect;
}

int klin_gd32v_wifi_sta_wait_connected(int timeout_ms)
{
    (void)timeout_ms;
    return s_last_connect;
}

int klin_gd32v_wifi_sta_connected(void)
{
    return s_connected;
}

int klin_gd32v_wifi_sta_wait_ip(int timeout_ms)
{
    int waited;

    if (!s_connected) {
        return -1;
    }
    if (s_use_static) {
        (void)klin_gd32v_wifi_apply_static_ip();
    }
    waited = 0;
    while (1) {
        if (klin_gd32v_wifi_refresh_ip() == 0 && s_ip != 0) {
            return 0;
        }
        if (timeout_ms >= 0 && waited >= timeout_ms) {
            return -1;
        }
        sys_ms_sleep(1);
        waited = waited + 1;
    }
}

uint32_t klin_gd32v_wifi_sta_ip_u32(void)
{
    (void)klin_gd32v_wifi_refresh_ip();
    return s_ip;
}

uint32_t klin_gd32v_wifi_sta_gateway_u32(void)
{
    (void)klin_gd32v_wifi_refresh_ip();
    return s_gw;
}

uint32_t klin_gd32v_wifi_sta_netmask_u32(void)
{
    (void)klin_gd32v_wifi_refresh_ip();
    return s_mask;
}

int klin_gd32v_wifi_sta_disconnect(void)
{
    s_connected = 0;
    s_ip = 0;
    s_gw = 0;
    s_mask = 0;
    s_assoc_ssid[0] = '\0';
    s_assoc_auth = 0;
    return wifi_management_disconnect();
}

int klin_gd32v_wifi_sta_stop(void)
{
    s_connected = 0;
    s_inited = 0;
    s_ip = 0;
    s_gw = 0;
    s_mask = 0;
    s_scan_count = 0;
    wifi_management_deinit();
    return 0;
}

static int klin_gd32v_wifi_freq_to_channel(uint16_t freq)
{
    if (freq == 2484) {
        return 14;
    }
    if (freq >= 2412 && freq <= 2472) {
        return (int)((freq - 2407) / 5);
    }
    if (freq >= 1 && freq <= 14) {
        return (int)freq;
    }
    return 0;
}

static uint8_t klin_gd32v_wifi_akm_to_auth(uint32_t akm)
{
    int has_none = (akm & (1u << MAC_AKM_NONE)) != 0;
    int has_pre = (akm & (1u << MAC_AKM_PRE_RSN)) != 0;
    int has_psk = (akm & ((1u << MAC_AKM_PSK) | (1u << MAC_AKM_PSK_SHA256) |
                          (1u << MAC_AKM_FT_PSK))) != 0;
    int has_sae = (akm & ((1u << MAC_AKM_SAE) | (1u << MAC_AKM_FT_OVER_SAE))) != 0;

    if (has_none && !has_pre && !has_psk && !has_sae) {
        return (uint8_t)AUTH_MODE_OPEN;
    }
    if (has_pre && !has_psk && !has_sae) {
        return (uint8_t)AUTH_MODE_WEP;
    }
    if (has_sae && has_psk) {
        return (uint8_t)AUTH_MODE_WPA2_WPA3;
    }
    if (has_sae) {
        return (uint8_t)AUTH_MODE_WPA3;
    }
    if (has_pre && has_psk) {
        return (uint8_t)AUTH_MODE_WPA_WPA2;
    }
    if (has_psk) {
        return (uint8_t)AUTH_MODE_WPA2;
    }
    if (has_pre) {
        return (uint8_t)AUTH_MODE_WPA;
    }
    return (uint8_t)AUTH_MODE_UNKNOWN;
}

static void klin_gd32v_wifi_scan_collect(int idx, struct mac_scan_result *result)
{
    int n;

    (void)idx;
    if (s_scan_count >= KLIN_GD32V_WIFI_SCAN_MAX || result == NULL) {
        return;
    }
    memset(&s_scan[s_scan_count], 0, sizeof(s_scan[s_scan_count]));
    n = (int)result->ssid.length;
    if (n < 0) {
        n = 0;
    }
    if (n > KLIN_GD32V_WIFI_SSID_MAX - 1) {
        n = KLIN_GD32V_WIFI_SSID_MAX - 1;
    }
    memcpy(s_scan[s_scan_count].ssid, result->ssid.array, (size_t)n);
    s_scan[s_scan_count].ssid[n] = '\0';
    s_scan[s_scan_count].rssi = result->rssi;
    if (result->chan != NULL) {
        s_scan[s_scan_count].channel =
            (uint8_t)klin_gd32v_wifi_freq_to_channel(result->chan->freq);
    }
    s_scan[s_scan_count].authmode = klin_gd32v_wifi_akm_to_auth(result->akm);
    s_scan_count = s_scan_count + 1;
}

int klin_gd32v_wifi_scan_start(int timeout_ms)
{
    int e;

    (void)timeout_ms;
    if (!s_inited) {
        return -1;
    }
    s_scan_count = 0;
    e = wifi_management_scan(1, NULL);
    if (e != 0) {
        return e;
    }
    return wifi_netlink_scan_results_print(0, klin_gd32v_wifi_scan_collect);
}

int klin_gd32v_wifi_sta_rssi(void)
{
#ifdef KLIN_GD32V_WIFI_HAVE_MACIF_VIF
    if (!s_connected) {
        return 0;
    }
    return (int)macif_vif_sta_rssi_get(0);
#else
    if (!s_connected) {
        return 0;
    }
    return 0;
#endif
}

int klin_gd32v_wifi_sta_channel(void)
{
    if (!s_connected) {
        return 0;
    }
#ifdef KLIN_GD32V_WIFI_HAVE_MACIF_VIF
    {
        uint8_t ch = 0;
        if (macif_vif_current_chan_get(0, &ch) == 0) {
            return (int)ch;
        }
    }
#endif
#ifdef KLIN_GD32V_WIFI_HAVE_WIFI_VIF
    return (int)wifi_vif_tab[0].sta.cfg.channel;
#else
    return 0;
#endif
}

int klin_gd32v_wifi_sta_authmode(void)
{
    if (!s_connected) {
        return 0;
    }
#ifdef KLIN_GD32V_WIFI_HAVE_WIFI_VIF
    return (int)klin_gd32v_wifi_akm_to_auth(wifi_vif_tab[0].sta.cfg.akm);
#else
    return (int)s_assoc_auth;
#endif
}

int klin_gd32v_wifi_sta_ap_ssid(char *out, int max_len)
{
    int n;
#ifdef KLIN_GD32V_WIFI_HAVE_WIFI_VIF
    const char *ssid;
#endif

    if (out == NULL || max_len <= 0) {
        return -1;
    }
    if (!s_connected) {
        out[0] = '\0';
        return -1;
    }
#ifdef KLIN_GD32V_WIFI_HAVE_WIFI_VIF
    ssid = wifi_vif_tab[0].sta.cfg.ssid;
    n = (int)strlen(ssid);
#else
    n = (int)strlen(s_assoc_ssid);
#endif
    if (n >= max_len) {
        n = max_len - 1;
    }
#ifdef KLIN_GD32V_WIFI_HAVE_WIFI_VIF
    memcpy(out, ssid, (size_t)n);
#else
    memcpy(out, s_assoc_ssid, (size_t)n);
#endif
    out[n] = '\0';
    return n;
}

void klin_gd32v_wifi_sta_log_link(void)
{
    if (!s_connected) {
        printf("gd32v_wifi: link (not associated)\n");
        return;
    }
    printf("gd32v_wifi: link rssi=%d ch=%u auth=%u ssid=%s\n",
           klin_gd32v_wifi_sta_rssi(), (unsigned)klin_gd32v_wifi_sta_channel(),
           (unsigned)klin_gd32v_wifi_sta_authmode(), s_assoc_ssid);
}

int klin_gd32v_wifi_concurrent_supported(void)
{
#if defined(KLIN_GD32V_WIFI_HAVE_CONCURRENT)
    return 1;
#else
    return 0;
#endif
}

int klin_gd32v_wifi_concurrent_set(int enable)
{
#if defined(KLIN_GD32V_WIFI_HAVE_CONCURRENT)
    int e;

    if (!s_inited) {
        return -1;
    }
    e = wifi_management_concurrent_set(enable ? 1 : 0);
    if (e != 0) {
        return e;
    }
    if (enable && !wifi_management_concurrent_get()) {
        return -1;
    }
    return 0;
#else
    (void)enable;
    return -1;
#endif
}

int klin_gd32v_wifi_concurrent_get(void)
{
#if defined(KLIN_GD32V_WIFI_HAVE_CONCURRENT)
    return wifi_management_concurrent_get() ? 1 : 0;
#else
    return 0;
#endif
}

int klin_gd32v_wifi_roaming_set(int enable, int rssi_th)
{
    int8_t th;

    if (!s_inited) {
        return -1;
    }
    if (rssi_th > 127) {
        rssi_th = 127;
    }
    if (rssi_th < -128) {
        rssi_th = -128;
    }
    th = (int8_t)rssi_th;
    return wifi_management_roaming_set(enable ? 1 : 0, th);
}

int klin_gd32v_wifi_roaming_get(void)
{
    if (!s_inited) {
        return 0;
    }
    return wifi_management_roaming_get(NULL) ? 1 : 0;
}

int klin_gd32v_wifi_roaming_rssi_th(void)
{
    int8_t th;

    if (!s_inited) {
        return 0;
    }
    th = 0;
    (void)wifi_management_roaming_get(&th);
    return (int)th;
}

#else /* host stubs — no SDK headers */

int klin_gd32v_wifi_concurrent_supported(void)
{
    return 1;
}

int klin_gd32v_wifi_concurrent_set(int enable)
{
    if (!s_inited) {
        return -1;
    }
    s_concurrent = enable ? 1 : 0;
    return 0;
}

int klin_gd32v_wifi_concurrent_get(void)
{
    return s_concurrent ? 1 : 0;
}

int klin_gd32v_wifi_roaming_set(int enable, int rssi_th)
{
    if (!s_inited) {
        return -1;
    }
    if (rssi_th > 127) {
        rssi_th = 127;
    }
    if (rssi_th < -128) {
        rssi_th = -128;
    }
    s_roaming = enable ? 1 : 0;
    if (enable && rssi_th != 0) {
        s_roaming_rssi_th = (int8_t)rssi_th;
    }
    if (!enable) {
        /* keep last threshold for get, matching SDK leave-on-disable */
    }
    return 0;
}

int klin_gd32v_wifi_roaming_get(void)
{
    return s_roaming ? 1 : 0;
}

int klin_gd32v_wifi_roaming_rssi_th(void)
{
    return (int)s_roaming_rssi_th;
}


int klin_gd32v_wifi_sta_set_static_ip(uint32_t ip, uint32_t gw, uint32_t netmask)
{
    if (ip == 0 && gw == 0 && netmask == 0) {
        s_use_static = 0;
        s_static_ip = 0;
        s_static_gw = 0;
        s_static_mask = 0;
        return 0;
    }
    s_use_static = 1;
    s_static_ip = ip;
    s_static_gw = gw;
    s_static_mask = netmask;
    if (s_connected) {
        s_ip = ip;
        s_gw = gw;
        s_mask = netmask;
    }
    return 0;
}

int klin_gd32v_wifi_sta_set_hostname(const char *name)
{
    if (name == NULL || name[0] == '\0') {
        s_hostname[0] = '\0';
        return 0;
    }
    strncpy(s_hostname, name, sizeof(s_hostname) - 1);
    s_hostname[sizeof(s_hostname) - 1] = '\0';
    return 0;
}

int klin_gd32v_wifi_sta_init(void)
{
    s_inited = 1;
    return 0;
}

int klin_gd32v_wifi_sta_connect(const char *ssid, const char *pass)
{
    (void)pass;
    if (!s_inited || ssid == NULL || ssid[0] == '\0') {
        s_last_connect = -1;
        s_connected = 0;
        return -1;
    }
    s_last_connect = 0;
    s_connected = 1;
    memset(s_assoc_ssid, 0, sizeof(s_assoc_ssid));
    strncpy(s_assoc_ssid, ssid, sizeof(s_assoc_ssid) - 1);
    s_assoc_auth = 3;
    if (s_use_static) {
        s_ip = s_static_ip;
        s_gw = s_static_gw;
        s_mask = s_static_mask;
    } else {
        /* 192.168.1.50 / .1 / 255.255.255.0 — same pack as Klin `ipv4` */
        s_ip = 192u | (168u << 8) | (1u << 16) | (50u << 24);
        s_gw = 192u | (168u << 8) | (1u << 16) | (1u << 24);
        s_mask = 255u | (255u << 8) | (255u << 16) | (0u << 24);
    }
    return 0;
}

int klin_gd32v_wifi_sta_wait_connected(int timeout_ms)
{
    (void)timeout_ms;
    return s_last_connect;
}

int klin_gd32v_wifi_sta_connected(void)
{
    return s_connected;
}

int klin_gd32v_wifi_sta_wait_ip(int timeout_ms)
{
    (void)timeout_ms;
    if (!s_connected || s_ip == 0) {
        return -1;
    }
    return 0;
}

uint32_t klin_gd32v_wifi_sta_ip_u32(void)
{
    return s_ip;
}

uint32_t klin_gd32v_wifi_sta_gateway_u32(void)
{
    return s_gw;
}

uint32_t klin_gd32v_wifi_sta_netmask_u32(void)
{
    return s_mask;
}

int klin_gd32v_wifi_sta_disconnect(void)
{
    s_connected = 0;
    s_ip = 0;
    s_gw = 0;
    s_mask = 0;
    s_assoc_ssid[0] = '\0';
    s_assoc_auth = 0;
    return 0;
}

int klin_gd32v_wifi_sta_stop(void)
{
    s_connected = 0;
    s_inited = 0;
    s_ip = 0;
    s_gw = 0;
    s_mask = 0;
    s_scan_count = 0;
    s_concurrent = 0;
    s_roaming = 0;
    s_roaming_rssi_th = 0;
    return 0;
}

int klin_gd32v_wifi_sta_rssi(void)
{
    if (!s_connected) {
        return 0;
    }
    return 0 - 42;
}

int klin_gd32v_wifi_sta_channel(void)
{
    if (!s_connected) {
        return 0;
    }
    return 6;
}

int klin_gd32v_wifi_sta_authmode(void)
{
    if (!s_connected) {
        return 0;
    }
    return (int)s_assoc_auth;
}

int klin_gd32v_wifi_sta_ap_ssid(char *out, int max_len)
{
    int n;

    if (out == NULL || max_len <= 0) {
        return -1;
    }
    if (!s_connected) {
        out[0] = '\0';
        return -1;
    }
    n = (int)strlen(s_assoc_ssid);
    if (n >= max_len) {
        n = max_len - 1;
    }
    memcpy(out, s_assoc_ssid, (size_t)n);
    out[n] = '\0';
    return n;
}

void klin_gd32v_wifi_sta_log_link(void)
{
    if (!s_connected) {
        printf("gd32v_wifi: link (not associated)\n");
        return;
    }
    printf("gd32v_wifi: link rssi=%d ch=%u auth=%u ssid=%s\n",
           klin_gd32v_wifi_sta_rssi(), (unsigned)klin_gd32v_wifi_sta_channel(),
           (unsigned)klin_gd32v_wifi_sta_authmode(), s_assoc_ssid);
}

int klin_gd32v_wifi_scan_start(int timeout_ms)
{
    (void)timeout_ms;
    if (!s_inited) {
        s_scan_count = 0;
        return -1;
    }
    memset(&s_scan[0], 0, sizeof(s_scan[0]));
    memcpy(s_scan[0].ssid, "klin-ap", 8);
    s_scan[0].rssi = (int8_t)-50;
    s_scan[0].channel = 6;
    s_scan[0].authmode = 3; /* AUTH_MODE_WPA2 */
    s_scan_count = 1;
    return 0;
}

#endif

void klin_gd32v_wifi_sta_log_ip_info(void)
{
    unsigned a = (unsigned)(s_ip & 255u);
    unsigned b = (unsigned)((s_ip >> 8) & 255u);
    unsigned c = (unsigned)((s_ip >> 16) & 255u);
    unsigned d = (unsigned)((s_ip >> 24) & 255u);
    printf("gd32v_wifi ip %u.%u.%u.%u\n", a, b, c, d);
}

int klin_gd32v_wifi_scan_max(void)
{
    return KLIN_GD32V_WIFI_SCAN_MAX;
}

int klin_gd32v_wifi_scan_count(void)
{
    return s_scan_count;
}

int klin_gd32v_wifi_scan_rssi(int index)
{
    if (index < 0 || index >= s_scan_count) {
        return 0;
    }
    return (int)s_scan[index].rssi;
}

int klin_gd32v_wifi_scan_channel(int index)
{
    if (index < 0 || index >= s_scan_count) {
        return 0;
    }
    return (int)s_scan[index].channel;
}

int klin_gd32v_wifi_scan_authmode(int index)
{
    if (index < 0 || index >= s_scan_count) {
        return 0;
    }
    return (int)s_scan[index].authmode;
}

int klin_gd32v_wifi_scan_ssid(int index, char *out, int max_len)
{
    int n;

    if (out == NULL || max_len <= 0) {
        return -1;
    }
    if (index < 0 || index >= s_scan_count) {
        out[0] = '\0';
        return -1;
    }
    n = (int)strlen(s_scan[index].ssid);
    if (n >= max_len) {
        n = max_len - 1;
    }
    memcpy(out, s_scan[index].ssid, (size_t)n);
    out[n] = '\0';
    return n;
}

void klin_gd32v_wifi_scan_log(void)
{
    int i;

    for (i = 0; i < s_scan_count; i++) {
        printf("gd32v_wifi scan: [%d] ch=%u rssi=%d auth=%u ssid=%s\n", i,
               (unsigned)s_scan[i].channel, (int)s_scan[i].rssi,
               (unsigned)s_scan[i].authmode, s_scan[i].ssid);
    }
}
