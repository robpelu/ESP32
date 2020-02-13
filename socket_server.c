  
/*  WiFi softAP Example
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

#include <sys/socket.h> /* socket, connect */
#include <netdb.h> /* struct hostent, gethostbyname */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */

/* The examples use WiFi configuration that you can set via 'make menuconfig'.
   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      "taller_esp32"
#define EXAMPLE_ESP_WIFI_PASS      "utec12345"
#define EXAMPLE_MAX_STA_CONN       5

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

static const char *TAG = "wifi softAP";

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "station:"MACSTR" join, AID=%d",
                 MAC2STR(event->event_info.sta_connected.mac),
                 event->event_info.sta_connected.aid);
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "station:"MACSTR"leave, AID=%d",
                 MAC2STR(event->event_info.sta_disconnected.mac),
                 event->event_info.sta_disconnected.aid);
        break;
    default:
        break;
    }
    return ESP_OK;
}

void wifi_init_softap()
{
    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished.SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}

#define BUF_SIZE 1000

void socket_server_task(void *param) 
{    
    struct sockaddr_in clientAddress;
    struct sockaddr_in serverAddress;
    int clientSock, r, valtrue = 1;
    fd_set readfds;
    int sock=0;

    if( (sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        ESP_LOGE(TAG, "socket: %d %s", sock, strerror(errno));
        goto END;
    }

    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&valtrue,sizeof(int));
    setsockopt(sock,SOL_SOCKET,SO_REUSEPORT,&valtrue,sizeof(int));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(12345);

    int rc  = bind(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (rc < 0) {
        ESP_LOGE(TAG, "bind: %d %s", rc, strerror(errno));
        goto END;
    }
    
    ESP_LOGI(TAG, "esperando conexiones...");
    rc = listen(sock, 5);
    if (rc < 0) {
        ESP_LOGE(TAG, "listen: %d %s", rc, strerror(errno));
        goto END;
    }
    char *data_str = malloc(BUF_SIZE);
    int sizeUsed = 0;

    socklen_t clientAddressLength = sizeof(clientAddress);
AGAIN:

    while(1)
    {
        clientSock = accept(sock, (struct sockaddr *)&clientAddress, &clientAddressLength);
        if (clientSock < 0) {
            ESP_LOGE(TAG, "accept: %d %s", clientSock, strerror(errno));
            goto AGAIN;
        }
        ESP_LOGI(TAG, "conexion aceptada...");

        do {
            bzero(data_str, BUF_SIZE);
            r = recv(clientSock, data_str, BUF_SIZE-1,0);
            data_str[r] = '\0'; //termino string recibido
            if(strlen(data_str) > 0)
            {    
                ESP_LOGI(TAG, "=============  Mensaje del cliente = %s ", data_str);
            }
        } while(r > 0);
        close(clientSock);
    }
    END:
    
    ESP_LOGI(TAG, "SALIENDO DE SOCKET SERVER ============= MATANDO TAREA");
    
    vTaskDelete(NULL);
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
    
    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();
    xTaskCreate(socket_server_task, "socket_server_task", 30000, NULL, 5, NULL);


}

