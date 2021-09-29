
#include "ringbuffer_dma_sm.h"
#include "ringbuffer_dma_parol.h"
#include <stdlib.h>	// rand
#include <string.h>

//*************************************************************************

	extern UART_HandleTypeDef huart1;
	extern UART_HandleTypeDef huart3;

	#define RX_BUFFER_SIZE 256

	extern DMA_HandleTypeDef hdma_usart3_rx;
	RingBuffer_DMA rx_buffer3;
	uint8_t rx_circular_buffer3[RX_BUFFER_SIZE];

	/* ESP8266 variables */
	char wifi_cmd[512];

	uint32_t rx_count = 0;
	uint8_t icmd_sm_u8 = 0;
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
	buffer->data = data; // set array
	buffer->size = size; // and its size
	buffer->hdma = hdma; // initialized DMA
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
	sprintf(wifi_cmd, "AT+RST\r\n");	//		sprintf(wifi_cmd, "AT+RST\r\n");	//	Перезапустить модуль
	HAL_UART_Transmit(&huart1, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 1000);
	HAL_UART_Transmit(&huart3, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 1000);
	HAL_Delay(500);
	UART_Read();

	/* Turn on message echo */
	sprintf(wifi_cmd, "ATE1\r\n");	//		sprintf(wifi_cmd, "ATE1\r\n");	//	включает эхо.
	HAL_UART_Transmit(&huart1, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 1000);
	HAL_UART_Transmit(&huart3, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 1000);
	HAL_Delay(100);
	UART_Read();

	/* Display version info */
	sprintf(wifi_cmd, "AT+GMR\r\n");	//	Проверить информацию о версии
	HAL_UART_Transmit(&huart1, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 1000);
	HAL_UART_Transmit(&huart3, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 1000);
	HAL_Delay(500);
	UART_Read();


	/* Set to client mode */
	sprintf(wifi_cmd, "AT+CWMODE=1\r\n");	//		sprintf(wifi_cmd, "AT+CWMODE_CUR=1\r\n");	//	режим работы ESP8266: 1 - клиент
	HAL_UART_Transmit(&huart1, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 1000);
	HAL_UART_Transmit(&huart3, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 1000);
	HAL_Delay(100);
	UART_Read();
	HAL_Delay(2000);

	/* Connect to network */
	sprintf(wifi_cmd, "AT+CWJAP_CUR=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PASS);	//	подключение к точке доступа
	HAL_UART_Transmit(&huart1, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 1000);
	HAL_UART_Transmit(&huart3, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 1000);
	HAL_Delay(10000);
	UART_Read();
	/* CONNECTED (hope so) */
	/* Check for IP */
	sprintf(wifi_cmd, "AT+CIFSR\r\n");	//		sprintf(wifi_cmd, "AT+CIFSR\r\n");	//	получение локального IP-адреса.
	HAL_UART_Transmit(&huart1, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 1000);
	HAL_UART_Transmit(&huart3, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 1000);
	HAL_Delay(5000);
	UART_Read();

	sprintf(wifi_cmd,"WiFi Started\r\n");
	HAL_UART_Transmit(&huart1, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 1000);

}
//*************************************************************************

void UART_Read (void)  {
	/* Check number of bytes in RingBuffer */
	rx_count = RingBuffer_DMA_Count(&rx_buffer3);
//	sprintf(wifi_cmd,"DMA_count= %d\r\n", (int)rx_count);
//	HAL_UART_Transmit(&huart1, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 100);

	/* Process each byte individually */
	while (rx_count--) {
		uint8_t b = RingBuffer_DMA_GetByte(&rx_buffer3);	/* Read out one byte from RingBuffer */
		if (b == '\n') { /* If \n process command */
			/* Terminate string with \0 */
			cmd_sm_char[icmd_sm_u8] = 0;
			icmd_sm_u8 = 0;
			/* Display received messages */

			sprintf(wifi_cmd,"%s\r\n", cmd_sm_char);
			HAL_UART_Transmit(&huart1, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 100);
		  	HAL_Delay(100);
		}
		else if (b == '\r') {
			continue;	/* If \r skip */
		}
		else {
			cmd_sm_char[icmd_sm_u8++ % 512] = b;	/* If regular character, put it into cmd_sm_char[] */
		}
	}
 }	// void UART_Read(void)
//======================================================================

void RingBuffer_DMA_Main(char* info) {
	char http_req_2[200];
	sprintf(http_req_2, "GET /update?api_key=%s", THINGSPEAK_API_KEY);
	strcat(http_req_2, info);

	/* Connect to server */
	sprintf(wifi_cmd, "AT+CIPSTART=\"TCP\",\"%s\",80\r\n", THINGSPEAK_ADDRESS);
	HAL_UART_Transmit(&huart1, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 1000);
	HAL_UART_Transmit(&huart3, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 1000);
	HAL_Delay(2000);
	UART_Read();

	/* Send data length (length of request) */
	sprintf(wifi_cmd, "AT+CIPSEND=%d\r\n", strlen(http_req_2));
	HAL_UART_Transmit(&huart1, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 1000);
	HAL_UART_Transmit(&huart3, (uint8_t *)wifi_cmd, strlen(wifi_cmd), 1000);
	HAL_Delay(2000); // wait for ">"
	UART_Read();

	/* Send data */
	HAL_UART_Transmit(&huart1, (uint8_t *)http_req_2, strlen(http_req_2), 1000);
	HAL_UART_Transmit(&huart3, (uint8_t *)http_req_2, strlen(http_req_2), 1000);
	HAL_Delay(100);
	UART_Read();
}
//*************************************************************************
