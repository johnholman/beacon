#include "pico/cyw43_arch.h"

#include "lwip/dns.h"

typedef enum {
    DNS_STATUS_IDLE,          // Lookup has not started yet
    DNS_STATUS_IN_PROGRESS,   // Lookup initiated, waiting for callback
    DNS_STATUS_RESOLVED,   // Lookup completed successfully
    DNS_STATUS_FAILED         // Lookup completed and failed
} dns_lookup_status_t;

static volatile dns_lookup_status_t dns_lookup_status = DNS_STATUS_IDLE;


static void ip_request_finished(const char *hostname, const ip_addr_t *ipaddr, void *ip) {
    if (ipaddr) {
        dns_lookup_status = DNS_STATUS_RESOLVED;
        // copy result of lookup to caller's variable
        *((ip_addr_t *)ip)  = *ipaddr;
//        printf("resolution successful for %s: " IPSTR "\n", hostname, IP2STR(&s_mqtt_broker_ip));
        printf("address %s\n", ipaddr_ntoa(ipaddr));
    } else {
        dns_lookup_status = DNS_STATUS_FAILED;
        printf("address request failed\n");
    }
}

int dns_lookup(const char* hostname, ip_addr_t *addr) {
    dns_lookup_status = DNS_STATUS_IN_PROGRESS;

    cyw43_arch_lwip_begin();
    err_t err = dns_gethostbyname(hostname, addr, ip_request_finished, addr);
    cyw43_arch_lwip_end();

    if (err == ERR_OK) {
        dns_lookup_status = DNS_STATUS_RESOLVED;
        printf("resolved immediately\n");
        return 0;
    } else if (err != ERR_INPROGRESS) {
        dns_lookup_status = DNS_STATUS_FAILED;
        printf("lookup failed immediately\n");
        return -1;
    }

    while (dns_lookup_status != DNS_STATUS_RESOLVED ) {
        switch(dns_lookup_status) {
            case DNS_STATUS_IN_PROGRESS:
                printf("still looking\n");
                sleep_ms(1000);
                break;

            case DNS_STATUS_FAILED:
                printf("lookup failed after query\n");
                return -1;
        }
    }

    // success
    printf("lookup succeeded after query\n");
    return 0;

}
