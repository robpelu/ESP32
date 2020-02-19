/* WiFi station Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include <stdio.h> /* printf, sprintf */
#include <stdlib.h> /* exit, atoi, malloc, free */
#include <unistd.h> /* read, write, close */


#include <sys/socket.h> /* socket, connect */
#include <netdb.h> /* struct hostent, gethostbyname */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */

/* The examples use WiFi configuration that you can set via 'make menuconfig'.
   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      "WUTEC-Estudiantes"
#define EXAMPLE_ESP_WIFI_PASS      "ley19043"
#define EXAMPLE_ESP_MAXIMUM_RETRY  4

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
int connect_to_socket(char* host, char* port);

/* The event group allows multiple bits for each event, but we only care about one event 
 * - are we connected to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;

static const char *TAG = "wifi station";

static int s_retry_num = 0;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        {
            if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
                esp_wifi_connect();
                xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
                s_retry_num++;
                ESP_LOGI(TAG,"retry to connect to the AP");
            }
            ESP_LOGI(TAG,"connect to the AP fail\n");
            break;
        }
    default:
        break;
    }
    return ESP_OK;
}

void wifi_init_sta()
{
    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}


int s=0; //socket
int r=0;
int bytes_received=0;
#define BUFFER_SIZE 1000
char send_buf[BUFFER_SIZE];

void connect_and_receive()
{
	//espera a que el bit WIFI_CONNECTED_BIT se haya prendido. Eso se hace una vez que se obtiene la IP
	xEventGroupWaitBits(
            s_wifi_event_group,   /* The event group being tested. */
            WIFI_CONNECTED_BIT, /* The bits within the event group to wait for. */
            pdFALSE,        /* BIT_0 & BIT_4 should be cleared before returning. */
            pdFALSE,       /* Don't wait for both bits, either bit will do. */
            portMAX_DELAY);/* Wait a maximum of 100ms for either bit to be set. */


   //int total = strlen(send_buf);
    int bytes_sent =0;
    while(1)
    {

        if(s == 0)
        {
            if(connect_to_socket("pillsandcare.com", "80") == 0)
                ESP_LOGI(TAG, "Conectado a puerto 80 !");
        }
        

	//armar la consulta HTTP GET
	sprintf(send_buf, "GET /%s HTTP/1.1\r\nHost:%s \r\n\r\n\r\n", "esp32/pepito.php?nombre=Robert", "pillsandcare.com");


        do {
            r = send(s, send_buf + bytes_sent , BUFFER_SIZE - bytes_sent - 1, 0);
            bytes_sent += r;

        } while(r > 0);




	//leer lo que devuelve el servidor web e imprimir.

	do {
            bzero(send_buf, BUFFER_SIZE);
            r = recv(s, send_buf, BUFFER_SIZE-1 ,0);
            send_buf[r] = '\0'; //termino string recibido
            if(strlen(send_buf) > 0)
            {    
                ESP_LOGI(TAG, "=============  Respuesta del servidor= %s ", send_buf);
            }
        } while(r > 0);



        close(s);
        s=0;
        r = bytes_sent = 0;

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

}



int connect_to_socket(char* host, char* port)
{
    struct addrinfo *res;

    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    int err=-1;
    
    err = getaddrinfo(host, port, &hints, &res);
    
    if(err != 0 || res == NULL) {
        ESP_LOGI(TAG, "DNS lookup to %s:%s failed err=%d res=%p", host, port, err, res);
        return -1;
    }
    
    s = socket(res->ai_family, res->ai_socktype, 0);
    if(s < 0) {
        ESP_LOGI(TAG, "... Failed to allocate socket.");
        freeaddrinfo(res);
        return -1;           
    }
    
    if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
        ESP_LOGI(TAG, "... socket connect failed errno=%d", errno);
        close(s);
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    return 0;
}




void app_main()
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
    xTaskCreate(connect_and_receive, "connect_and_receive", 30000, NULL, 5, NULL);

}
