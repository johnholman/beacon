/*
 * Beacon - firmware for an IR emitting beacon
 *    
 * v0.1,  14 Nov 23 - generates 180 ms on 500 ms off bursts of 38 kHz carrier
 * v0.2,  27 Nov 23 - includes short data pulses followed by 100 ms recovery before the 180 ms pulse
 * v0.3,   4 Dec 23 - support continuous and single transmission "flash" modes
 * v0.4,   5 Dec 23 - support for local transmission control
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "pio_beacon.pio.h"
#include "hardware/clocks.h"

#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/inet.h"
#include "lwip/apps/mqtt_priv.h"
#include "lwip/apps/mqtt.h"

#define BUTTON_GPIO 14

// GPIO to drive IR emitter with bursts of 38Hz carrier
#define OUTPUT_GPIO 15

// GPIO for burst generator PIO SM (tx_pio) to signal bursts to carrier generating PIO (rx_pio)
#define BURST_GPIO 16

// GPIO reflecting whether in continuous mode
#define STATUS_GPIO 17


mqtt_client_t client;
bool connected = false;

bool continuous = true;  // default is to transmit continuously
int nshort = 0;    //  without any short pulses
bool flash_now = false;   // set when flash has been requested and not yet actioned

static void mqtt_sub_request_cb(void *arg, err_t result)
{
  /* Just print the result code here for simplicity,
     normal behaviour would be to take some action if subscribe fails like
     notifying user, retry subscribe or disconnect from server */
  printf("Subscribe result: %d\n", result);
}

void msg_info_cb(void * arg, const char* topic, u32_t len) {
  printf("message received for topic %s payload length %u\n", topic, (unsigned int) len);
}

static void msg_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
  printf("Incoming publish payload with length %d, flags %u\n", len, (unsigned int)flags);

  if (flags & MQTT_DATA_FLAG_LAST) {    
    puts("whole message received");
    /* Last fragment of payload received (or whole part if payload fits receive buffer
       See MQTT_VAR_HEADER_BUFFER_LEN)  */

    if (len == 1) {
        char cmd = data[0];
        printf("command %c received\n", cmd);
        
        if (cmd == 'c') {
            // continuous mode on
            continuous = true;
            gpio_put(STATUS_GPIO, 1);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            puts("continuous mode on");
        } else if (cmd == 'n') {
            // continuous mode off
            continuous = false;
            gpio_put(STATUS_GPIO, 0);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            puts("continuous mode off\n");
        } else if  (cmd >=  '0' && cmd <= '9') {          
            // set number of short pulses to send with each transmission
            nshort = cmd - '0';
        } else if (cmd == 'f') {
          // flash once when not in continuous mode
            flash_now = true;
        } else {
            printf("bad command %u\n", cmd);
        }
    } else {
        printf("bad message length\n");
    } 
  }
}

static void mqtt_connection_cb(mqtt_client_t *client, void *id, mqtt_connection_status_t status) {
    err_t err;
    if (status == MQTT_CONNECT_ACCEPTED) {
        printf("mqtt_connection_cb: Successfully connected with arg %s\n", id);
        connected = true;
    } else {
    printf("mqtt_connection_cb: Disconnected, reason: %d\n", status);
    }
     
    /* Setup callback for incoming publish requests */
    mqtt_set_inpub_callback(client, msg_info_cb, msg_data_cb, "");
    
    /* Subscribe to a topic named "subtopic" with QoS level 1, call mqtt_sub_request_cb with result */ 
    // note no locking needed as in a callback
    char topic[12];
    snprintf(topic, sizeof(topic), "beacon/%s", id);
    printf("subscribing to topic %s\n", topic);

    err = mqtt_subscribe(client, topic, 1, mqtt_sub_request_cb, "");

    if (err != ERR_OK) {
      printf("mqtt_subscribe return: %d\n", err);
    } 

    // connect_broker(client, );
  }  


void connect_broker(mqtt_client_t *client, char* broker_ip, char* id) {

    struct mqtt_connect_client_info_t ci;
    err_t err;
  
    /* Setup an empty client info structure */
    memset(&ci, 0, sizeof(ci));
  
    /* Set client name to beacon-N where N is the id */ 
    char client_name[12];
    snprintf(client_name, sizeof(client_name), "beacon-%s", id);
    printf("client name: %s\n", client_name);

    ci.client_id = client_name;
  
    /* Initiate client and connect to server, if this fails immediately an error code is returned
     otherwise mqtt_connection_cb will be called with connection result after attempting 
     to establish a connection with the server. 
     For now MQTT version 3.1.1 is always used */
  
    
    ip_addr_t ip_addr;
    ipaddr_aton(broker_ip, &ip_addr);

    // wrapping for safety but might not be needed
    cyw43_arch_lwip_begin();
    err = mqtt_client_connect(client, &ip_addr, MQTT_PORT, mqtt_connection_cb, id, &ci);
    cyw43_arch_lwip_end();
  
    /* For now just print the result code if something goes wrong */
    if (err != ERR_OK) {
        printf("mqtt_connect request failed with error %d\n", err);
    } else {
        printf("mqtt_connect request returned OK\n");
    }

    // client->keep_alive = 4;
    // printf("keepalive content: %d\n", client->keep_alive);
}

/* Called when publish is complete either with sucess or failure */
static void mqtt_pub_request_cb(void *arg, err_t result)
{
  if(result != ERR_OK) {
    printf("Publish result: %d\n", result);
  } else {
    printf("published OK\n");
  }

}



void publish(mqtt_client_t *client, char* topic, char* msg)
{
  err_t err;
  u8_t qos = 2; /* 0 1 or 2, see MQTT specification */
  u8_t retain = 0; /* No don't retain such crappy payload... */

  printf("publishing %s on topic %s\n", msg, topic);
  // definitely do need to wrap here!
  cyw43_arch_lwip_begin();
  err = mqtt_publish(client, topic, msg, strlen(msg), qos, retain, mqtt_pub_request_cb, "");
  cyw43_arch_lwip_end();

  if(err != ERR_OK) {
    printf("Publish err: %d\n", err);
  }

}


int main() {

    puts("beacon v0.3 - continuous and flash modes");

    stdio_init_all();

    // configure pin to reflect whether in continuous mode
    gpio_init(STATUS_GPIO);
    gpio_set_dir(STATUS_GPIO, GPIO_OUT);

    gpio_init(BURST_GPIO);
    gpio_set_dir(BURST_GPIO, GPIO_OUT);

    gpio_init(BUTTON_GPIO);
    gpio_set_dir(BUTTON_GPIO, GPIO_IN);
    gpio_pull_up(BUTTON_GPIO);

    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();

    char* id;
    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        return 1;
    } else {
        printf("Connected.\n");
        printf("IP address: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
        id  = strrchr(ip4addr_ntoa(netif_ip4_addr(netif_list)), '.');
        id++;
        printf("ID: %s\n", id);
    }


  connect_broker(&client, "192.168.1.90", id);

  for(;;) {
    if (connected)
      break;
    puts("not yet connected");
    sleep_ms(1000);
  }

  char announcement[20];
  snprintf(announcement, sizeof(announcement), "beacon %s started", id);
  publish(&client, "beacon/announce", announcement);

    // use the first PIO block
    PIO pio = pio0;

    // load the blink PIO program into this block and remember location
    uint burst_offset = pio_add_program(pio, &burst_program);

    uint carrier_offset = pio_add_program(pio, &carrier_program);

    //  configure two state machines to run the two blink PIO programs
    // controlling different pins
    uint carrier_sm = pio_claim_unused_sm(pio, true);
    carrier_program_init(pio, carrier_sm, carrier_offset, OUTPUT_GPIO, BURST_GPIO);

    uint burst_sm = pio_claim_unused_sm(pio, true);
    // burst_program_init(pio, burst_sm, burst_offset, BURST_GPIO);
    burst_program_init(pio, burst_sm, burst_offset, BURST_GPIO);

    // set PIO clock rate to slow 3200 Hz for the burst state machine
    // each PIO cycle lasts 5/16 ms - 32 of them in 10 ms
    int f = clock_get_hz(clk_sys);
    printf("clock frequency %d Hz\n", f);
    float burst_div = f/3200.0;
    printf("setting burst state machine clock divider to %f\n", burst_div);
    pio_sm_set_clkdiv(pio, burst_sm, burst_div);

    // set carrer PIO clock to 20 cycles for each period of the 38kHz carrier
    float carrier_div = f/(38 * 20 * 1000.0);
    printf("setting carrier state machine clock divider to %f\n", carrier_div);
    pio_sm_set_clkdiv(pio, carrier_sm, carrier_div);


    uint32_t ret;
    while (true) {
        bool button_pressed = !gpio_get(BUTTON_GPIO);
        // printf("button pressed %s\n", button_pressed? "true" : "false");
        if (flash_now || continuous || button_pressed) {
          flash_now = false;
          printf("requesting flash with data %d\n", nshort);
          pio_sm_put_blocking(pio, burst_sm, nshort);
          ret = pio_sm_get_blocking(pio, burst_sm);
          printf("PIO returned %d\n", ret);
        }
    }
}


