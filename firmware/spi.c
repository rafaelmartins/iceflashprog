// SPDX-FileCopyrightText: 2025 Rafael G. Martins <rafael@rafaelmartins.eng.br>
// SPDX-License-Identifier: GPL-2.0-only

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <stm32f0xx.h>

#include "spi.h"

static uint8_t tx_buf[SPI_BUFFER_SIZE];
static uint8_t rx_buf[SPI_BUFFER_SIZE];
static uint32_t locked_len = 0;


void
spi_init(void)
{
    // PA4 - SPI1_NSS (controled by sw)
    // PA5 - SPI1_SCK
    // PA6 - SPI1_MISO
    // PA7 - SPI1_MOSI

    RCC->AHBENR |= RCC_AHBENR_DMA1EN | RCC_AHBENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    GPIOA->OTYPER |= GPIO_OTYPER_OT_4;
    GPIOA->PUPDR |= GPIO_PUPDR_PUPDR4;
    GPIOA->MODER |= GPIO_MODER_MODER4_0 | GPIO_MODER_MODER5_1 | GPIO_MODER_MODER6_1 | GPIO_MODER_MODER7_1;
    GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR4 | GPIO_OSPEEDER_OSPEEDR5 | GPIO_OSPEEDER_OSPEEDR6 | GPIO_OSPEEDER_OSPEEDR7;
    GPIOA->BSRR = GPIO_BSRR_BS_4;

    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI
#ifndef SPI_MAX_FREQ
    | SPI_CR1_BR_0
#endif
    ;
    SPI1->CR2 = SPI_CR2_RXNEIE | SPI_CR2_TXEIE | SPI_CR2_FRXTH | SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0;
    SPI1->CR1 |= SPI_CR1_SPE;

    DMA1_Channel2->CPAR = (uint32_t) &(SPI1->DR);
    DMA1_Channel2->CCR = DMA_CCR_PL | DMA_CCR_MINC | DMA_CCR_TCIE;

    DMA1_Channel3->CPAR = (uint32_t) &(SPI1->DR);
    DMA1_Channel3->CCR = DMA_CCR_DIR | DMA_CCR_PL | DMA_CCR_MINC | DMA_CCR_TCIE;
}


uint8_t*
spi_lock(uint32_t len)
{
    if (locked_len != 0 || len > sizeof(tx_buf))
        return NULL;

    locked_len = len;
    return tx_buf;
}


bool
spi_start_transfer(void)
{
    if (locked_len == 0)
        return false;

    GPIOA->BSRR = GPIO_BSRR_BR_4;

    spi_hook_cb(true);

    DMA1_Channel2->CMAR = (uint32_t) rx_buf;
    DMA1_Channel2->CNDTR = locked_len;
    DMA1_Channel2->CCR |= DMA_CCR_EN;

    DMA1_Channel3->CMAR = (uint32_t) tx_buf;
    DMA1_Channel3->CNDTR = locked_len;
    DMA1_Channel3->CCR |= DMA_CCR_EN;

    SPI1->CR2 |= SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN;

    return true;
}


bool
spi_task()
{
    if ((DMA1->ISR & DMA_ISR_TCIF2) != DMA_ISR_TCIF2)
        return false;

    SPI1->CR2 &= ~(SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);

    DMA1->IFCR = DMA_IFCR_CTCIF2 | DMA_IFCR_CTCIF3;
    DMA1_Channel2->CCR &= ~DMA_CCR_EN;
    DMA1_Channel3->CCR &= ~DMA_CCR_EN;

    GPIOA->BSRR = GPIO_BSRR_BS_4;

    spi_transfer_complete_cb(rx_buf, locked_len);

    locked_len = 0;

    spi_hook_cb(false);
    return true;
}


bool
spi_is_locked(void)
{
    return locked_len != 0;
}
