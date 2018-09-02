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
#include "spp.h"
#include "pwm.h"

int switch1 = 0, switch2 = 0;
int slider_red = 0, slider_green = 0, slider_blue = 0;

void handle_line(char *line)
{
	int len = strlen(line);
	if (strcmp(line, "get:config") == 0)
	{
		spp_printf("config:channel:C:static:name:RGB\r\n");
		spp_printf("config:channel:C:type:switch\r\n");
		spp_printf("config:channel:C:mode:sink\r\n");
		spp_printf("config:channel:C:method:push\r\n");
		spp_printf("config:channel:D:static:name:Slider Red\r\n");
		spp_printf("config:channel:D:type:slider\r\n");
		spp_printf("config:channel:D:mode:sink\r\n");
		spp_printf("config:channel:D:method:push\r\n");
		spp_printf("config:channel:D:color:000000:ff0000\r\n");
		spp_printf("config:channel:D:parent:C\r\n");
		spp_printf("config:channel:E:static:name:Slider Green\r\n");
		spp_printf("config:channel:E:type:slider\r\n");
		spp_printf("config:channel:E:mode:sink\r\n");
		spp_printf("config:channel:E:method:push\r\n");
		spp_printf("config:channel:E:color:000000:00ff00\r\n");
		spp_printf("config:channel:E:parent:C\r\n");
		spp_printf("config:channel:F:static:name:Slider Blue\r\n");
		spp_printf("config:channel:F:type:slider\r\n");
		spp_printf("config:channel:F:mode:sink\r\n");
		spp_printf("config:channel:F:method:push\r\n");
		spp_printf("config:channel:F:color:000000:0000ff\r\n");
		spp_printf("config:channel:F:parent:C\r\n");
		spp_printf("C%d\r\n", switch2);
		spp_printf("D%x\r\n", slider_red);
		spp_printf("E%x\r\n", slider_green);
		spp_printf("F%x\r\n", slider_blue);
	}
	else if (line[0] == 'C' && len > 1)
	{
		switch2 = line[1] == '0' ? 0 : 1;
		pwm_set_duty(0, (float)(switch2 ? slider_red : 0) * 100 / 255);
		pwm_set_duty(1, (float)(switch2 ? slider_green : 0) * 100 / 255);
		pwm_set_duty(2, (float)(switch2 ? slider_blue : 0) * 1000 / 255);
	}
	else if (line[0] == 'D' && len > 1)
	{
		slider_red = (int)strtol(line + 1, NULL, 16);
		pwm_set_duty(0, (float)(switch2 ? slider_red : 0) * 100 / 255);
	}
	else if (line[0] == 'E' && len > 1)
	{
		slider_green = (int)strtol(line + 1, NULL, 16);
		pwm_set_duty(1, (float)(switch2 ? slider_green : 0) * 100 / 255);
	}
	else if (line[0] == 'F' && len > 1)
	{
		slider_blue = (int)strtol(line + 1, NULL, 16);
		pwm_set_duty(2, (float)(switch2 ? slider_blue : 0) * 100 / 255);
	}
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
