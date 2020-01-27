/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "driver/uart.h"
#include "driver/gpio.h"

#include "esp_spiffs.h"

#include "mongoose.h"

#define PORT CONFIG_EXAMPLE_PORT

#define ECHO_TEST_TXD  (GPIO_NUM_32)
#define ECHO_TEST_RXD  (GPIO_NUM_16)
#define ECHO_TEST_RTS  (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS  (UART_PIN_NO_CHANGE)
#define BUF_SIZE (512)

#define DOCUMENT_ROOT "/spiffs"
#define MG_LISTEN_ADDR "8081"
#define DOCUMENT_ROOT "/spiffs"
volatile bool connected = false;

static const char *TAG = "uart_gateway";

QueueHandle_t  qRx=NULL;
QueueHandle_t  qTx=NULL;


static void udp_server_task(void *pvParameters)
{
    char rx_buffer[512];
    char addr_str[512];
    int addr_family;
    int ip_protocol;

    char buffTx[512];
    int lenBuffTx = 0;
    uint8_t rec;

    while (1) {

#ifdef CONFIG_EXAMPLE_IPV4
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
#else // IPV6
        struct sockaddr_in6 dest_addr;
        bzero(&dest_addr.sin6_addr.un, sizeof(dest_addr.sin6_addr.un));
        dest_addr.sin6_family = AF_INET6;
        dest_addr.sin6_port = htons(PORT);
        addr_family = AF_INET6;
        ip_protocol = IPPROTO_IPV6;
        inet6_ntoa_r(dest_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket bound, port %d", PORT);

        while (1) {
            ESP_LOGI(TAG, "Waiting for data");
            struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(source_addr);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
            ESP_LOGI(TAG, "Received %d bytes", len);

            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                // Get the sender's ip address as string
                if (source_addr.sin6_family == PF_INET) {
                    inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                } else if (source_addr.sin6_family == PF_INET6) {
                    inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
                }

                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string...
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                ESP_LOGI(TAG, "%s", rx_buffer);

                // Send to UART
                // uart_write_bytes(UART_NUM_2, (const char *) rx_buffer, len);


            	while(xQueueReceive(qRx, &rec,(TickType_t )(1/portTICK_PERIOD_MS))) {
            		// printf("value received on queue: %c \n", (char)rec);
            		buffTx[lenBuffTx++] = rec;
            		if(lenBuffTx > 511)
            			break;
            	}
                ESP_LOGI(TAG, "Length TX Buffer : %d", lenBuffTx);
            	buffTx[lenBuffTx] = 0;

            	int err;
            	if(lenBuffTx>0) {
            		// from uart
            		// printf("value received on queue: %s \n", buffTx);
            		err = sendto(sock, buffTx, lenBuffTx, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
            		lenBuffTx = 0;
            	} else {
                    // Echo
                    err = sendto(sock, rx_buffer, len, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
            	}
                if (err < 0) {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                    break;
                }
            }
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

static void uart_task(void *arg)
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 921600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_driver_install(UART_NUM_2, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS);

    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    while (1) {
        // Read data from the UART
        int len = uart_read_bytes(UART_NUM_2, data, BUF_SIZE, 1 / portTICK_RATE_MS);
        ESP_LOGI(TAG, "Data from UART Length: %d", len);

        if(qRx != NULL) {
        	for(int i=0;i<len;i++) {
        		xQueueSend(qRx, (void *)&data[i], (TickType_t )0);
        	}
        }

        // Write data back to the UART
        // uart_write_bytes(UART_NUM_2, (const char *) data, len);
    }
}

static void blink_task(void *arg)
{
    gpio_pad_select_gpio(GPIO_NUM_33);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(GPIO_NUM_33, GPIO_MODE_OUTPUT);
    while(1) {
        /* Blink off (output low) */
    	// printf("Turning off the LED\n");
        gpio_set_level(GPIO_NUM_33, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        /* Blink on (output high) */
        // printf("Turning on the LED\n");
        gpio_set_level(GPIO_NUM_33, 1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void mountFileSystem(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
}

static void mg_ev_handler(struct mg_connection *nc, int ev, void *p)
{
    static const char *reply_fmt =
        "HTTP/1.0 200 OK\r\n"
        "Connection: close\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Hello %s\n";

    static char fname[64];

    struct websocket_message *wm = (struct websocket_message *) p;

    switch (ev) {

    case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
        /* New websocket connection. Tell everybody. */
        printf("new websocket connection\n");
        connected = true;
        break;
    }
    case MG_EV_WEBSOCKET_FRAME:
        /* Echo received frame back */
        mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, wm->data, wm->size);
        break;

    case MG_EV_ACCEPT: {
        char addr[32];
        mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                            MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
        printf("Connection %p from %s\n", nc, addr);
        break;
    }
    case MG_EV_HTTP_REQUEST: {
        char addr[32];
        struct http_message *hm = (struct http_message *) p;
        mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                            MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);

        printf("HTTP request from %s: %.*s %.*s\n", addr, (int) hm->method.len,
               hm->method.p, (int) hm->uri.len, hm->uri.p);

        strcpy(fname, "/spiffs");
        if (!mg_vcmp(&hm->uri, "/") || !mg_vcmp(&hm->uri, "/index"))
            strcat(fname, "/index.html");
        else
            strncat(fname, hm->uri.p, hm->uri.len);

        printf("file %s\n", fname);
        mg_http_serve_file(nc, hm, fname,
                           mg_mk_str(""), mg_mk_str(""));


              // mg_printf(nc, reply_fmt, addr);
              // nc->flags |= MG_F_SEND_AND_CLOSE;

        break;
    }
    case MG_EV_CLOSE: {
        printf("Connection %p closed\n", nc);
        connected = false;
        break;
    }
    }
}


void app_main(void)
{
    char msg[128];
    int count = 0;
    int tcount = 0;

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    mountFileSystem();

    printf("Starting in %d seconds...\n", 1);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    ESP_ERROR_CHECK(example_connect());

    qRx = xQueueCreate(512, sizeof(uint8_t));
    qTx = xQueueCreate(512, sizeof(char));

    struct mg_mgr mgr;
    struct mg_connection *nc;

    printf("Starting web-server on port %s\n", MG_LISTEN_ADDR);

    mg_mgr_init(&mgr, NULL);

    nc = mg_bind(&mgr, MG_LISTEN_ADDR, mg_ev_handler);
    if (nc == NULL) {
        printf("Error setting up listener!\n");
        return;
    }
    mg_set_protocol_http_websocket(nc);

    // xTaskCreate(udp_server_task, "udp_server", 4096, NULL, 5, NULL);
    // xTaskCreate(uart_task, "uart_task", 4096, NULL, 10, NULL);
    xTaskCreate(blink_task, "blink_task", 1024, NULL, 12, NULL);

    /* Processing events */
    while (1) {
        mg_mgr_poll(&mgr, 1000);

        if (connected && tcount > 2) {
            struct mg_connection* c = mg_next(nc->mgr, NULL);
            sprintf(msg, "{ \"temp\":%d, \"msg\": \"%s\" }", count++, "deneme\\n");
            mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, msg, strlen(msg));
            tcount = 0;
        }
        tcount++;
    }
}
