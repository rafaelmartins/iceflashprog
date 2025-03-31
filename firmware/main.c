// SPDX-FileCopyrightText: 2025 Rafael G. Martins <rafael@rafaelmartins.eng.br>
// SPDX-License-Identifier: GPL-2.0-only

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <stm32f0xx.h>

#include <usbd.h>

#include "spi.h"
#include "spi_flash.h"

typedef enum {
    COMMAND_POWER_UP = 1,
    COMMAND_POWER_DOWN,
    COMMAND_JEDEC_ID,
    COMMAND_READ,
    COMMAND_ERASE_SECTOR,
} command_t;

typedef enum {
    STATUS_OK = 0,
    STATUS_UNPOWERED,
    STATUS_INVALID_REQUEST,
    STATUS_INVALID_COMMAND_ID,
    STATUS_INVALID_FLASH_PAGE_READ,
    STATUS_INVALID_FLASH_PAGE_WRITE,
    STATUS_LOCKED,
} status_t;

typedef struct __attribute__((packed)) {
    uint8_t report_id;  // always 1
    uint8_t address[3];
    uint8_t data[256];
} flash_page_request_t;

typedef struct __attribute__((packed)) {
    uint8_t report_id;  // always 1
    uint8_t data[256];
} flash_page_response_t;

typedef struct __attribute__((packed)) {
    uint8_t report_id;  // always 2
    uint8_t command;
    uint8_t data[3];
} command_request_t;

typedef struct __attribute__((packed)) {
    uint8_t report_id;  // always 2
    uint8_t status;
    uint8_t data[3];
} command_response_t;

static bool powered = false;

static bool set_flash_rx = false;
static bool set_flash_tx = false;
static bool set_response = false;

static uint8_t buf[SPI_FLASH_PAGE_SIZE + 4];
static uint16_t buf_idx = 0;


static void
send_response(status_t status, uint8_t *data, uint32_t data_len)
{
    command_response_t *response = (command_response_t*) buf;
    response->report_id = 2;
    response->status = powered ? status : STATUS_UNPOWERED;
    memset(response->data, 0, sizeof(response->data));
    if (data != NULL)
        memcpy(response->data, data, data_len <= 3 ? data_len : 3);
    set_response = true;
}


void
spi_flash_powerup_cb(void)
{
    powered = true;
    send_response(STATUS_OK, NULL, 0);
}

void
spi_flash_powerdown_cb(void)
{
    powered = false;
    send_response(STATUS_OK, NULL, 0);
}

void
spi_flash_jedec_id_cb(uint8_t manufacturer_id, uint16_t device_id)
{
    uint8_t buff[] = {manufacturer_id, device_id >> 8, device_id};
    send_response(STATUS_OK, buff, sizeof(buff));
}

void
spi_flash_read_cb(const uint8_t *buff, uint32_t len)
{
    if (len != SPI_FLASH_PAGE_SIZE) {
        send_response(STATUS_INVALID_FLASH_PAGE_READ, NULL, 0);
        return;
    }

    flash_page_response_t *response = (flash_page_response_t*) buf;
    response->report_id = 1;
    memcpy(response->data, buff, len);
    buf_idx = 0;
    set_flash_tx = true;
}

void
spi_flash_erase_sector_cb(void)
{
    send_response(STATUS_OK, NULL, 0);
}

void
spi_flash_write_cb(bool verified)
{
    send_response(verified ? STATUS_OK : STATUS_INVALID_FLASH_PAGE_WRITE, NULL, 0);
}


void
usbd_in_cb(uint8_t ept)
{
    if (ept != 1)
        return;

    if (set_flash_tx) {
        if ((sizeof(flash_page_response_t) - buf_idx) > USBD_EP1_IN_SIZE) {
            usbd_in(ept, buf + buf_idx, USBD_EP1_IN_SIZE);
            buf_idx += USBD_EP1_IN_SIZE;
        } else {
            usbd_in(ept, buf + buf_idx, sizeof(flash_page_response_t) - buf_idx);
            buf_idx = 0;
            set_flash_tx = false;
            usbd_out_enable(1);
        }
        return;
    }

    if (set_response) {
        usbd_in(ept, buf, sizeof(command_response_t));
        set_response = false;
        usbd_out_enable(1);
    }
}

void
usbd_out_cb(uint8_t ept)
{
    if (ept != 1)
        return;

    if (set_flash_rx) {
        uint16_t len = usbd_out(ept, buf + buf_idx, USBD_EP1_OUT_SIZE, false);
        buf_idx += len;

        if (buf_idx == sizeof(flash_page_request_t)) {
            buf_idx = 0;
            set_flash_rx = false;
            flash_page_request_t *request = (flash_page_request_t*) buf;
            if (!spi_flash_write(request->address[0], request->address[1], request->address[2], request->data, sizeof(request->data)))
                send_response(STATUS_LOCKED, NULL, 0);
            return;
        }

        usbd_out_enable(ept);
        return;
    }

    uint8_t buff[USBD_EP1_OUT_SIZE];
    uint16_t len = usbd_out(ept, buff, sizeof(buff), false);

    switch (buff[0]) {
    case 1:
        memcpy(buf, buff, len);
        buf_idx = len;
        set_flash_rx = true;
        usbd_out_enable(ept);
        break;

    case 2:
        if (len != sizeof(command_request_t)) {
            send_response(STATUS_INVALID_REQUEST, NULL, 0);
            break;
        }

        command_request_t *request = (command_request_t*) buff;

        switch ((command_t) request->command) {
        case COMMAND_POWER_UP:
            if (!spi_flash_powerup())
                send_response(STATUS_LOCKED, NULL, 0);
            break;

        case COMMAND_POWER_DOWN:
            if (!spi_flash_powerdown())
                send_response(STATUS_LOCKED, NULL, 0);
            break;

        case COMMAND_JEDEC_ID:
            if (!spi_flash_jedec_id())
                send_response(STATUS_LOCKED, NULL, 0);
            break;

        case COMMAND_READ:
            if (!spi_flash_read(request->data[0], request->data[1], request->data[2]))
                send_response(STATUS_LOCKED, NULL, 0);
            break;

        case COMMAND_ERASE_SECTOR:
            if (!spi_flash_erase_sector(request->data[0], request->data[1], request->data[2]))
                send_response(STATUS_LOCKED, NULL, 0);
            break;

        default:
            send_response(STATUS_INVALID_COMMAND_ID, NULL, 0);
            return;
        }
        break;
    }
}


void
usbd_reset_hook_cb(bool before)
{
    if (before)
        GPIOA->BSRR = GPIO_BSRR_BS_15;
}

void
usbd_set_address_hook_cb(uint8_t addr)
{
    (void) addr;
    GPIOA->BSRR = GPIO_BSRR_BR_15;
}

void
spi_hook_cb(bool before)
{
    GPIOA->BSRR = before ? GPIO_BSRR_BS_15 : GPIO_BSRR_BR_15;
}


static void
clock_init(void)
{
    // 1 flash wait cycle required to operate @ 48MHz (RM0091 section 3.5.1)
    FLASH->ACR &= ~FLASH_ACR_LATENCY;
    FLASH->ACR |= FLASH_ACR_LATENCY;
    while ((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY);

    RCC->CR2 |= RCC_CR2_HSI48ON;
    while ((RCC->CR2 & RCC_CR2_HSI48RDY) != RCC_CR2_HSI48RDY);

#ifdef SPI_MAX_FREQ
    RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLMUL);
    RCC->CFGR |= RCC_CFGR_PLLSRC_HSI48_PREDIV | RCC_CFGR_PLLMUL3;

    RCC->CFGR2 &= ~RCC_CFGR2_PREDIV;
    RCC->CFGR2 |= RCC_CFGR2_PREDIV_DIV4;

    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) != RCC_CR_PLLRDY);

    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE | RCC_CFGR_SW);
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE_DIV1 | RCC_CFGR_SW_PLL;
    while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

    SystemCoreClock = 36000000;
#else
    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE | RCC_CFGR_SW);
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE_DIV1 | RCC_CFGR_SW_HSI48;
    while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI48);

    SystemCoreClock = 48000000;
#endif
}


int
main(void)
{
    clock_init();

    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    GPIOA->MODER &= ~GPIO_MODER_MODER15;
    GPIOA->MODER |= GPIO_MODER_MODER15_0;
    GPIOA->BSRR = GPIO_BSRR_BR_15;

    usbd_init();
    spi_flash_init();

    while (true) {
        if (spi_flash_task())
            continue;

        usbd_task();
    }
    return 0;
}
