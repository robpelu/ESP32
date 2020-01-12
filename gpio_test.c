* C source, ASCII text, with CRLF line terminators
/* GPIO Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"


#define GPIO_OUTPUT_IO_23    23
#define GPIO_OUTPUT_IO_12     12
#define GPIO_OUTPUT_PIN_SEL  (1ULL<<GPIO_OUTPUT_IO_23)

#define T1 "TASK1"
#define T2 "TASK2"
#define MAIN "MAIN"

static void gpio_task_example(void* arg)
{
    for(;;) {
        gpio_set_level(GPIO_OUTPUT_IO_23, 1);
        vTaskDelay(500 / portTICK_RATE_MS);
        gpio_set_level(GPIO_OUTPUT_IO_23, 0);
        vTaskDelay(500 / portTICK_RATE_MS);
	//ESP_LOGI(T1, "Tarea de parpadeo de LED's corriendo...");
    }

}


static void gpio_task_read(void* arg)
{
    while(1) {
        if(gpio_get_level(GPIO_OUTPUT_IO_12) == 0)
        {
		ESP_LOGI(T2, ".");
	}
	else
    		vTaskDelay(10 / portTICK_RATE_MS);
    }

}

void app_main()
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    //io_conf.pull_down_en = 0;
    //disable pull-up mode
    //io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);



    //gpio_config_t io_conf1;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_INPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = (1ULL<<GPIO_OUTPUT_IO_12);
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 1;
    //configure GPIO with the given settings
    gpio_config(&io_conf);



    //crear tareas
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    xTaskCreate(gpio_task_read, "gpio_task_read", 2048, NULL, 10, NULL);

    while(1) {
        vTaskDelay(2000 / portTICK_RATE_MS);
	//ESP_LOGI(MAIN, "Main sigue corriendo en el fondo haciendo nada....");
    }
}


