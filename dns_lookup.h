#ifndef DNSLOOKUP_H
#define DNSLOOKUP_H

#include "lwip/dns.h"

int dns_lookup(const char* hostname, ip_addr_t *addr);

#endif // DNSLOOKUP_H