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

#ifdef CONFIG_IDF_TARGET_ESP32
#define CHIP_NAME "ESP32"
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S2BETA
#define CHIP_NAME "ESP32-S2 Beta"
#endif

#define PORT CONFIG_EXAMPLE_PORT

#define ECHO_TEST_TXD  (GPIO_NUM_32)
#define ECHO_TEST_RXD  (GPIO_NUM_16)
#define ECHO_TEST_RTS  (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS  (UART_PIN_NO_CHANGE)
#define BUF_SIZE (256)

#define MG_LISTEN_ADDR "8081"
#define DOCUMENT_ROOT "/spiffs"
volatile bool connected = false;

static const char *TAG = "uart_gateway";
char msg[128];
int count = 0;

QueueHandle_t  qRx=NULL;
QueueHandle_t  qTx=NULL;


/*

    char buffTx[512];
    int lenBuffTx = 0;
    uint8_t rec;

	while(xQueueReceive(qRx, &rec,(TickType_t )(1/portTICK_PERIOD_MS))) {
		buffTx[lenBuffTx++] = rec;
		if(lenBuffTx > 511)
			break;
	}
	ESP_LOGI(TAG, "Length TX Buffer : %d", lenBuffTx);
	buffTx[lenBuffTx] = 0;
*/

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

static void uart_task(void *arg)
{
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
        int len = uart_read_bytes(UART_NUM_2, data, BUF_SIZE, 100 / portTICK_RATE_MS);
        if(len > 0) {
            // ESP_LOGI(TAG, "Data from UART Length: %d - %x - %x", len, data[38], data[39]);
            // ESP_LOGI(TAG, "Channel 0 data: %02X %02X %02X %02X", data[0], data[1], data[2], data[3]);

        	/*
        	sprintf(msg, "Channel 0 - 3 : %f - %f - %f - %f",
            		*(float *)&data[0],
					*(float *)&data[4],
					*(float *)&data[8],
					*(float *)&data[12]);
			*/

        	float temp = data[16] + data[17] / 256.0f;

            sprintf(msg, "{ \"temp\":%f, \"msg\": \"%f - %f - %f - %f\\n\" }",
            		temp,
            		*(float *)&data[0],
					*(float *)&data[4],
					*(float *)&data[8],
					*(float *)&data[12]);


        	ESP_LOGI(TAG, "%s", msg);

            if(qRx != NULL) {
            	for(int i=0;i<len;i++) {
            		xQueueSend(qRx, (void *)&data[i], (TickType_t )0);
            	}
            }
        }
        // Write data back to the UART
        // uart_write_bytes(UART_NUM_2, (const char *) data, len);
    }
}

static void blink_task(void *arg)
{
    gpio_pad_select_gpio(GPIO_NUM_33);
    gpio_set_direction(GPIO_NUM_33, GPIO_MODE_OUTPUT);
    while(1) {
        gpio_set_level(GPIO_NUM_33, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(GPIO_NUM_33, 1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}


static void mg_ev_handler(struct mg_connection *nc, int ev, void *p)
{
    static char fname[64];
    struct websocket_message *wm = (struct websocket_message *) p;

    switch (ev) {

		case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
			printf("new websocket connection\n");
			connected = true;
			break;
		}
		case MG_EV_WEBSOCKET_FRAME:
			mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, wm->data, wm->size);
			printf("ws data %s", wm->data);
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

    xTaskCreate(uart_task, "uart_task", 4096, NULL, 10, NULL);
    xTaskCreate(blink_task, "blink_task", 1024, NULL, 12, NULL);

    /* Processing events */
    while (1) {
        mg_mgr_poll(&mgr, 1000);

        if (connected && tcount > 2) {
            struct mg_connection* c = mg_next(nc->mgr, NULL);
            // sprintf(msg, "{ \"temp\":%d, \"msg\": \"%s\" }", count++, "deneme\\n");
            count++;
            mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, msg, strlen(msg));
            tcount = 0;
        }
        tcount++;
    }
}
