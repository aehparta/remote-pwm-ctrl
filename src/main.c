/*
 * spp_streamer.c
 */

// *****************************************************************************
/* EXAMPLE_START(spp_streamer): Send test data via SPP as fast as possible.
 *
 * @text After RFCOMM connections gets open, request a
 * RFCOMM_EVENT_CAN_SEND_NOW via rfcomm_request_can_send_now_event().
 * @text When we get the RFCOMM_EVENT_CAN_SEND_NOW, send data and request another one.
 *
 * @text Note: To test, run the example, pair from a remote
 * device, and open the Virtual Serial Port.
 */
// *****************************************************************************

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <btstack.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "spp.h"
#include "pwm.h"

int switch1 = 0;
int slider_red = 0, slider_green = 0, slider_blue = 0;

void handle_line(char *line)
{
	int len = strlen(line);
	if (strcmp(line, "get config") == 0) {
		spp_printf(
		    "device:name:RGB LED\n"
		    "C:name:RGB\n"
		    "C:type:switch\n"
		    "C:mode:sink\n"
		    "C:method:push\n"
		    "D:name:Slider Red\n"
		    "D:type:slider\n"
		    "D:mode:sink\n"
		    "D:method:push\n"
		    "D:color:#000000,#ff0000\n"
		    "D:parent:C\n"
		    "E:name:Slider Green\n"
		    "E:type:slider\n"
		    "E:mode:sink\n"
		    "E:method:push\n"
		    "E:color:#000000,#00ff00\n"
		    "E:parent:C\n"
		    "F:name:Slider Blue\n"
		    "F:type:slider\n"
		    "F:mode:sink\n"
		    "F:method:push\n"
		    "F:color:#000000,#0000ff\n"
		    "F:parent:C\n"
		    "C%d\n"
		    "D%x\n"
		    "E%x\n"
		    "F%x\n",
		    switch1,
		    slider_red,
		    slider_green,
		    slider_blue);
		return;
	}
	if (len < 3) {
		return;
	}
	if (line[1] != '=') {
		return;
	}

	if (line[0] == 'C') {
		switch1 = line[2] == '0' ? 0 : 1;
		pwm_set_duty(0, (float)(switch1 ? slider_red : 0) * 100 / 255);
		pwm_set_duty(1, (float)(switch1 ? slider_green : 0) * 100 / 255);
		pwm_set_duty(2, (float)(switch1 ? slider_blue : 0) * 1000 / 255);
	} else if (line[0] == 'D') {
		slider_red = (int)strtol(line + 2, NULL, 16);
		pwm_set_duty(0, (float)(switch1 ? slider_red : 0) * 100 / 255);
	} else if (line[0] == 'E') {
		slider_green = (int)strtol(line + 2, NULL, 16);
		pwm_set_duty(1, (float)(switch1 ? slider_green : 0) * 100 / 255);
	} else if (line[0] == 'F') {
		slider_blue = (int)strtol(line + 2, NULL, 16);
		pwm_set_duty(2, (float)(switch1 ? slider_blue : 0) * 100 / 255);
	}

	ESP_LOGI("app", "switch: %u, R: %u, G: %u, B: %u", switch1, slider_red, slider_green, slider_blue);
}

void task(void *p)
{
	while (1) {
		char *data = spp_recv(NULL);
		if (data) {
			handle_line(data);
			free(data);
		}
		vTaskDelay(10 / portTICK_RATE_MS);
	}
}

int btstack_main(int argc, const char * argv[])
{
	pwm_init();

	pwm_set_duty(0, 0);
	pwm_set_duty(1, 0);
	pwm_set_duty(2, 0);

	spp_init();

	xTaskCreate(task, "test task", 4096, NULL, 5, NULL);

	return 0;
}
