/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "credentials.h"

// results of wifi scan
#define MAX_SCAN_RESULTS 50

// buffer size required for SSID string including terminator
#define MAX_SSID_LEN sizeof(((cyw43_ev_scan_result_t *)0)->ssid)

typedef struct
{
    char ssids[MAX_SCAN_RESULTS][MAX_SSID_LEN + 1];
    int cnt;
} scan_results_t;

scan_results_t scan_results;

/* wifi scan callback function
 Invoked once for each discovered network to add new result to the scan result table
 */
static int scan_result_cb(void *env, const cyw43_ev_scan_result_t *result)
{
    // env argument points to the scan results table
    scan_results_t *s_results = (scan_results_t *)env;
    // add the SSID field to the scan results table if not full
    if (result && s_results->cnt < MAX_SCAN_RESULTS)
    {
        int cnt = s_results->cnt;
        int copy_len = result->ssid_len <= MAX_SSID_LEN ? result->ssid_len : MAX_SSID_LEN;
        memcpy(s_results->ssids[cnt], result->ssid, copy_len);
        scan_results.ssids[cnt][copy_len] = '\0';
        s_results->cnt++;

        // print results of the scan
        printf("call: %d ssid: %-32s rssi: %4d chan: %3d mac: %02x:%02x:%02x:%02x:%02x:%02x sec: %u\n",
               cnt, s_results->ssids[cnt],
               result->ssid, result->rssi, result->channel,
               result->bssid[0], result->bssid[1], result->bssid[2], result->bssid[3], result->bssid[4], result->bssid[5],
               result->auth_mode);
    }
    return 0;
}

/* scan for wifi networks, search the results and return information about the best network found, or NULL
   if there no acceptable network
*/
const wifi_info_t *get_wifi_info()
{

    // start wifi scan
    cyw43_wifi_scan_options_t scan_options = {0};
    cyw43_ev_scan_result_t scan_result;
    int err = cyw43_wifi_scan(&cyw43_state, &scan_options, &scan_results, scan_result_cb);

    if (err)
    {
        return NULL;
    }

    // wait until the scan has finished
    while (cyw43_wifi_scan_active(&cyw43_state))
    {
        sleep_ms(100);
    }

    int nresults = scan_results.cnt;
    printf("collected results: \n");
    for (int idx = 0; idx < nresults; idx++)
    {
        printf("%d ssid: %s\n", idx, scan_results.ssids[idx]);
    }

    printf("candidate network ssids\n");
    for (int i = 0; i < NUM_CANDIDATE_NETWORKS; i++)
    {
        const char *ssid = candidate_networks[i].ssid;
        printf("ssid: %s\n", ssid);
    }

    const wifi_info_t *wifi_info = NULL;

    for (int candidate_num = 0; candidate_num < NUM_CANDIDATE_NETWORKS; candidate_num++)
    {
        for (int found_num = 0; found_num < nresults; found_num++)
        {
            const char *cssid = candidate_networks[candidate_num].ssid;
            const char *fssid = scan_results.ssids[found_num];
            printf("comparing %s and %s\n", cssid, fssid);
            if (strcmp(cssid, fssid) == 0)
            {
                printf("best network ssid: %s\n", cssid);
                wifi_info = &candidate_networks[candidate_num];
                return wifi_info;
            }
        }
    }
    // no acceptable network found
    return wifi_info;
}

// int main()
// {
//     stdio_init_all();

//     if (cyw43_arch_init())
//     {
//         printf("failed to initialise\n");
//         return 1;
//     }

//     cyw43_arch_enable_sta_mode();

//     sleep_ms(2000); // time to get the serial monitor going!

//     // start wifi scan
//     cyw43_wifi_scan_options_t scan_options = {0};
//     cyw43_ev_scan_result_t scan_result;
//     int err = cyw43_wifi_scan(&cyw43_state, &scan_options, &scan_results, scan_result_cb);

//     if (!err)
//     {
//         // wait until the scan has finished
//         while (cyw43_wifi_scan_active(&cyw43_state))
//         {
//             sleep_ms(100);
//         }

//         // get information about the best available network to connect to, if any
//         const wifi_info_t *info = get_wifi_info();
//         if (info)
//         {
//             printf("attempting to connect to ssid %s\n", info->ssid);
//         }
//         else
//         {
//             printf("candidate wifi network not found\n");
//         }
//     }
//     else
//     {
//         printf("scan failed to start\n");
//     }

//     while (true)
//     {
//         sleep_ms(1000);
//     }

//     cyw43_arch_deinit();
//     return 0;
// }
