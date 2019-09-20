
#include "spp.h"
#include <btstack.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>


static btstack_packet_callback_registration_t hci_event_callback_registration;
static int rfcomm_mtu = 0;
static uint16_t rfcomm_cid = 0;
static QueueHandle_t recv_queue, send_queue;


static void spp_recv_char(char byte)
{
	static char *recv_line = NULL;
	static int recv_len = 0;

	/* check if line end was received */
	if (byte == '\n' || byte == '\r') {
		if (recv_len > 0 && recv_line) {
			xQueueSendToBackFromISR(recv_queue, &recv_line, NULL);
		} else if (recv_line) {
			free(recv_line);
		}
		recv_line = NULL;
		recv_len = 0;
		return;
	}

	/* append to current line */
	recv_len++;
	if (recv_len < 2) {
		recv_len++;
	}
	recv_line = realloc(recv_line, recv_len);
	recv_line[recv_len - 2] = byte;
	recv_line[recv_len - 1] = '\0';
}

static void spp_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
	UNUSED(channel);

	bd_addr_t event_addr;
	uint8_t rfcomm_channel_nr;

	switch (packet_type) {
	case HCI_EVENT_PACKET:
		switch (hci_event_packet_get_type(packet)) {

		case HCI_EVENT_PIN_CODE_REQUEST:
			// inform about pin code request
			printf("Pin code request - using '0000'\n");
			hci_event_pin_code_request_get_bd_addr(packet, event_addr);
			gap_pin_code_response(event_addr, "0000");
			break;

		case HCI_EVENT_USER_CONFIRMATION_REQUEST:
			// inform about user confirmation request
			printf("SSP User Confirmation Request with numeric value '%06u'\n", little_endian_read_32(packet, 8));
			printf("SSP User Confirmation Auto accept\n");
			break;

		case RFCOMM_EVENT_INCOMING_CONNECTION:
			// data: event (8), len(8), address(48), channel (8), rfcomm_cid (16)
			rfcomm_event_incoming_connection_get_bd_addr(packet, event_addr);
			rfcomm_channel_nr = rfcomm_event_incoming_connection_get_server_channel(packet);
			rfcomm_cid = rfcomm_event_incoming_connection_get_rfcomm_cid(packet);
			printf("RFCOMM channel %u requested for %s\n", rfcomm_channel_nr, bd_addr_to_str(event_addr));
			rfcomm_accept_connection(rfcomm_cid);
			break;

		case RFCOMM_EVENT_CHANNEL_OPENED:
			// data: event(8), len(8), status (8), address (48), server channel(8), rfcomm_cid(16), max frame size(16)
			if (rfcomm_event_channel_opened_get_status(packet)) {
				printf("RFCOMM channel open failed, status %u\n", rfcomm_event_channel_opened_get_status(packet));
			} else {
				rfcomm_cid = rfcomm_event_channel_opened_get_rfcomm_cid(packet);
				rfcomm_mtu = rfcomm_event_channel_opened_get_max_frame_size(packet);
				printf("RFCOMM channel open succeeded. New RFCOMM Channel ID %u, max frame size %u\n", rfcomm_cid, rfcomm_mtu);

				// disable page/inquiry scan to get max performance
				gap_discoverable_control(0);
				gap_connectable_control(0);

				rfcomm_request_can_send_now_event(rfcomm_cid);
			}
			break;

		case RFCOMM_EVENT_CAN_SEND_NOW: {
			char *data = NULL;
			if (xQueueReceiveFromISR(send_queue, &data, NULL) == pdTRUE) {
				char *p = data;
				int len = strlen(data);
				while (len > 0) {
					int n = len > rfcomm_mtu ? rfcomm_mtu : len;
					rfcomm_send(rfcomm_cid, (void *)p, n);
					p += n;
					len -= n;
				}
				free(data);
			}
		}
		break;

		case RFCOMM_EVENT_CHANNEL_CLOSED:
			printf("RFCOMM channel closed\n");
			rfcomm_cid = 0;
			/* re-enable page/inquiry scan again */
			gap_discoverable_control(1);
			gap_connectable_control(1);
			break;

		default:
			break;
		}
		break;

	case RFCOMM_DATA_PACKET:
		for (uint16_t i = 0; i < size; i++) {
			spp_recv_char((char)packet[i]);
		}
		break;

	default:
		break;
	}
}

static void spp_heartbeat_handler(struct btstack_timer_source *ts)
{
	// printf("\ncheck queue, %u\n", rfcomm_cid);
	if (rfcomm_cid) {
		char *p = NULL;
		if (xQueuePeekFromISR(send_queue, &p) == pdTRUE) {
			rfcomm_request_can_send_now_event(rfcomm_cid);
		}
	}
	btstack_run_loop_set_timer(ts, 1);
	btstack_run_loop_add_timer(ts);
}

int spp_init(void)
{
	static uint8_t spp_service_buffer[150];
	static btstack_timer_source_t heartbeat;

	/* initialize queues */
	recv_queue = xQueueCreate(100, sizeof(char *));
	send_queue = xQueueCreate(100, sizeof(char *));

	/* register for HCI events */
	hci_event_callback_registration.callback = &spp_packet_handler;
	hci_add_event_handler(&hci_event_callback_registration);

	l2cap_init();

	rfcomm_init();
	rfcomm_register_service(spp_packet_handler, 1, 0xffff);

	/* init SDP, create record for SPP and register with SDP */
	sdp_init();
	memset(spp_service_buffer, 0, sizeof(spp_service_buffer));
	spp_create_sdp_record(spp_service_buffer, 0x10001, 1, "Lightning");
	sdp_register_service(spp_service_buffer);

	/* short-cut to find other SPP Streamer */
	gap_set_class_of_device(0x1234);

	gap_ssp_set_io_capability(SSP_IO_CAPABILITY_DISPLAY_YES_NO);
	gap_set_local_name("Lightning 00:00:00:00:00:00");
	gap_discoverable_control(1);

	/* heartbeat */
	heartbeat.process = &spp_heartbeat_handler;
	btstack_run_loop_set_timer(&heartbeat, 1);
	btstack_run_loop_add_timer(&heartbeat);

	/* turn on! */
	hci_power_control(HCI_POWER_ON);

	return 0;
}

int spp_printf(const char *format, ...)
{
	char *data = NULL;
	va_list args;
	va_start(args, format);
	vasprintf(&data, format, args);
	va_end(args);
	xQueueSendToBackFromISR(send_queue, &data, NULL);
	// rfcomm_request_can_send_now_event(rfcomm_cid);
	return 0;
}

char *spp_recv(int *size)
{
	char *data = NULL;
	if (xQueueReceiveFromISR(recv_queue, &data, NULL) != pdTRUE) {
		return NULL;
	}
	if (size) {
		*size = strlen(data);
	}
	return data;
}