
#include "ringbuffer_dma_sm.h"
#include <stdlib.h>	// rand
#include <string.h>

//*************************************************************************

	extern UART_HandleTypeDef huart1;
	extern UART_HandleTypeDef huart3;

	#define RX_BUFFER_SIZE 0xFF

	extern DMA_HandleTypeDef hdma_usart3_rx;
	RingBuffer_DMA rx_buffer3;
	uint8_t rx_circular_buffer3[RX_BUFFER_SIZE];

	/* ESP8266 variables */
	char wifi_cmd[520];

	uint32_t rx_count = 0;
	uint8_t position_cmd = 0;
	char cmd_sm_char[512];

//*************************************************************************

void UART_Read (void);

//*************************************************************************



uint32_t RingBuffer_DMA_Count(RingBuffer_DMA * buffer) {
	// get counter returns the number of remaining data units in the current DMA Stream transfer (total size - received count)
	// current head = start + (size - received count)
	uint8_t const * head = buffer->data + buffer->size - __HAL_DMA_GET_COUNTER(buffer->hdma);
	uint8_t const * tail = buffer->tail_ptr;
	if (head >= tail)
		return head - tail;
	else
		return head - tail + buffer->size;
}
//*************************************************************************

uint8_t RingBuffer_DMA_GetByte(RingBuffer_DMA * buffer) {
	// get counter returns the number of remaining data units in the current DMA Stream transfer (total size - received count)
	// current head = start + (size - received count)
	uint8_t const * head = buffer->data + buffer->size - __HAL_DMA_GET_COUNTER(buffer->hdma);
	uint8_t const * tail = buffer->tail_ptr;

	if (head != tail) {
		uint8_t c = *buffer->tail_ptr++;
		if (buffer->tail_ptr >= buffer->data + buffer->size) {
			buffer->tail_ptr -= buffer->size;
		}
		return c;
	}

	return 0;
}
//*************************************************************************

void RingBuffer_DMA_Init(RingBuffer_DMA * buffer, DMA_HandleTypeDef * hdma, uint8_t * data, uint32_t size) {
	buffer->data 	= data; // set array
	buffer->size 	= size; // and its size
	buffer->hdma 	= hdma; // initialized DMA
	buffer->tail_ptr = data; // tail == head == start of array
}

void RingBuffer_DMA_Connect(void) {
	/* Init RingBuffer_DMA object */
	RingBuffer_DMA_Init(&rx_buffer3, &hdma_usart3_rx, rx_circular_buffer3, RX_BUFFER_SIZE);
	HAL_UART_Receive_DMA(&huart3, rx_circular_buffer3, RX_BUFFER_SIZE);


	sprintf(wifi_cmd,"Hello ThingSpeak\r\n");
	HAL_UART_Transmit(&huart1, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 100);
	HAL_Delay(100);

	/* CONNECT TO WIFI ROUTER */
	/* Simple ping */
	sprintf(wifi_cmd, "AT+RST\r\n");	//		sprintf(wifi_cmd, "AT+RST\r\n");
	HAL_UART_Transmit(&huart1, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 100);
	HAL_UART_Transmit(&huart3, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 100);
	HAL_Delay(500);
	UART_Read();

	/* Turn on message echo */
	sprintf(wifi_cmd, "ATE1\r\n");	//		sprintf(wifi_cmd, "ATE1\r\n");
	HAL_UART_Transmit(&huart1, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 100);
	HAL_UART_Transmit(&huart3, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 100);
	HAL_Delay(100);
	UART_Read();

	/* Display version info */
	sprintf(wifi_cmd, "AT+GMR\r\n");
	HAL_UART_Transmit(&huart1, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 100);
	HAL_UART_Transmit(&huart3, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 100);
	HAL_Delay(500);
	UART_Read();


	/* Set to client mode */
	sprintf(wifi_cmd, "AT+CWMODE=1\r\n");	//		sprintf(wifi_cmd, "AT+CWMODE_CUR=1\r\n");
	HAL_UART_Transmit(&huart1, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 100);
	HAL_UART_Transmit(&huart3, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 100);
	HAL_Delay(100);
	UART_Read();
	HAL_Delay(2000);

	/* Connect to network */
	sprintf(wifi_cmd, "AT+CWJAP_CUR=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PASS);
	HAL_UART_Transmit(&huart1, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 100);
	HAL_UART_Transmit(&huart3, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 100);
	HAL_Delay(10000);
	UART_Read();
	/* CONNECTED (hope so) */
	/* Check for IP */
	sprintf(wifi_cmd, "AT+CIFSR\r\n");	//		sprintf(wifi_cmd, "AT+CIFSR\r\n");
	HAL_UART_Transmit(&huart1, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 100);
	HAL_UART_Transmit(&huart3, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 100);
	HAL_Delay(5000);
	UART_Read();

	sprintf(wifi_cmd,"WiFi Started.\r\n");
	HAL_UART_Transmit(&huart1, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 100);

}
//*************************************************************************

void UART_Read (void)  {
	rx_count = RingBuffer_DMA_Count(&rx_buffer3);
	while (rx_count--) {
		uint8_t getByte = RingBuffer_DMA_GetByte(&rx_buffer3);	/* Read out one byte from RingBuffer */
		if (getByte == '\n') { /* If \n process command */
			/* Terminate string with \0 */
			cmd_sm_char[position_cmd] = 0;
			position_cmd = 0;

			sprintf(wifi_cmd,"%s\r\n", cmd_sm_char);
			HAL_UART_Transmit(&huart1, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 100);
		  	HAL_Delay(10);
		} else if (getByte == '\r') {
			continue;	/* If \r skip */
		} else {
			cmd_sm_char[position_cmd++ % RX_BUFFER_SIZE] = getByte;
		}
	}
 }
//======================================================================

void RingBuffer_DMA_Main(char* _info_str, char *_api_key_string) {
	char http_req[0xFF];
	sprintf(http_req, "GET /update?api_key=%s", _api_key_string);
	strcat(http_req, _info_str);

	/* Connect to server */
	sprintf(wifi_cmd, "AT+CIPSTART=\"TCP\",\"%s\",80\r\n", THINGSPEAK_ADDRESS);
	HAL_UART_Transmit(&huart1, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 100);
	HAL_UART_Transmit(&huart3, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 100);
	HAL_Delay(2000);
	UART_Read();

	/* Send data length (length of request) */
	sprintf(wifi_cmd, "AT+CIPSEND=%d\r\n", strlen(http_req));
	HAL_UART_Transmit(&huart1, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 100);
	HAL_UART_Transmit(&huart3, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 100);
	HAL_Delay(2000); // wait for ">"
	UART_Read();

	/* Send data */
	HAL_UART_Transmit(&huart1, (uint8_t *)http_req, strlen(http_req), 100);
	HAL_UART_Transmit(&huart3, (uint8_t *)http_req, strlen(http_req), 100);
	HAL_Delay(1000);
	UART_Read();
}
//*************************************************************************
