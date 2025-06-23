// credentials.h
// IMPORTANT: Add this file to .gitignore!

#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#include "wifi_scan.h"

// Define the number of known Wi-Fi networks
#define NUM_CANDIDATE_NETWORKS 2

// Array of known Wi-Fi networks preferred connection order
const wifi_info_t candidate_networks[NUM_CANDIDATE_NETWORKS] = {
    // Network 1: private robot network has first priority
    {"net1", "xxxxx", "192.168.8.1"},

    // fall back to home wifi if private network not available
    {"net2", "xxxxx", "192.168.1.90"},
};

#endif // CREDENTIALS_H
