#ifndef RINGBUFFER_DMA_H_INCLUDED
#define RINGBUFFER_DMA_H_INCLUDED

//#include "stm32f4xx_hal.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include "dma.h"

typedef struct {
    uint8_t * data;
    uint32_t size;
    DMA_HandleTypeDef * hdma;
    uint8_t const * tail_ptr;
} RingBuffer_DMA;

void RingBuffer_DMA_Init(RingBuffer_DMA * buffer, DMA_HandleTypeDef * hdma, uint8_t * data, uint32_t size);
uint8_t RingBuffer_DMA_GetByte(RingBuffer_DMA * buffer);
uint32_t RingBuffer_DMA_Count(RingBuffer_DMA * buffer);

void RingBuffer_DMA_Connect(void);
void RingBuffer_DMA_Main(char* info);

#endif /* RINGBUFFER_H_INCLUDED */
