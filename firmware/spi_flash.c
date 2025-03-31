// SPDX-FileCopyrightText: 2025 Rafael G. Martins <rafael@rafaelmartins.eng.br>
// SPDX-License-Identifier: GPL-2.0-only

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <stm32f0xx.h>

#include "spi.h"
#include "spi_flash.h"

typedef enum {
    WRITE = 0x02,
    READ = 0x03,
    STATUS = 0x05,
    WRITE_ENABLE = 0x06,
    ERASE_SECTOR = 0x20,
    JEDEC_ID = 0x9f,
    POWER_UP = 0xab,
    POWER_DOWN = 0xb9,
} instruction_t;

static bool waiting_wel_erase_sector = false;
static bool waiting_wel_write = false;
static bool waiting_erase_sector = false;
static bool waiting_write = false;
static bool start_write = false;
static bool start_erase_sector = false;

static uint8_t flash_buf[SPI_BUFFER_SIZE];
static uint32_t flash_buf_locked_len = 0;

static instruction_t instruction = 0;


void
spi_flash_init(void)
{
    // B0 - CRESET

    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    GPIOB->MODER |= GPIO_MODER_MODER0_0;
    GPIOB->BSRR = GPIO_BSRR_BS_0;

    TIM3->PSC = SystemCoreClock / 10000 - 1;  // 0.1ms per tick
    TIM3->ARR = 10 - 1;  // 1ms
    TIM3->DIER = TIM_DIER_UIE;

    spi_init();
}


bool
spi_flash_task(void)
{
    if (start_erase_sector || start_write) {
        uint8_t *buf = spi_lock(flash_buf_locked_len);
        if (buf == NULL)
            return false;

        if (start_erase_sector) {
            start_erase_sector = false;
            waiting_erase_sector = true;
        }
        else if (start_write) {
            start_write = false;
            waiting_write = true;
        }

        memcpy(buf, flash_buf, flash_buf_locked_len);
        instruction = buf[0];
        spi_start_transfer();
        return true;
    }

    if ((TIM3->SR & TIM_SR_UIF) == TIM_SR_UIF) {
        TIM3->SR &= ~TIM_SR_UIF;
        spi_flash_status();
        return true;
    }

    return spi_task();
}


bool
spi_flash_read(uint8_t addr0, uint8_t addr1, uint8_t addr2)
{
    uint8_t *buf = spi_lock(SPI_FLASH_PAGE_SIZE + 4);
    if (buf == NULL)
        return false;

    buf[0] = instruction = READ;
    buf[1] = addr0;
    buf[2] = addr1;
    buf[3] = addr2;
    return spi_start_transfer();
}


bool
spi_flash_write(uint8_t addr0, uint8_t addr1, uint8_t addr2, const uint8_t *data, uint32_t data_len)
{
    if (flash_buf_locked_len != 0)
        return false;

    uint8_t *buf = spi_lock(1);
    if (buf == NULL)
        return false;

    flash_buf_locked_len = SPI_BUFFER_SIZE;
    flash_buf[0] = WRITE;
    flash_buf[1] = addr0;
    flash_buf[2] = addr1;
    flash_buf[3] = addr2;
    memcpy(flash_buf + 4, data, data_len);

    waiting_wel_write = true;
    buf[0] = instruction = WRITE_ENABLE;
    return spi_start_transfer();
}


bool
spi_flash_erase_sector(uint8_t addr0, uint8_t addr1, uint8_t addr2)
{
    if (flash_buf_locked_len != 0)
        return false;

    uint8_t *buf = spi_lock(1);
    if (buf == NULL)
        return false;

    flash_buf_locked_len = 4;
    flash_buf[0] = ERASE_SECTOR;
    flash_buf[1] = addr0;
    flash_buf[2] = addr1;
    flash_buf[3] = addr2;

    waiting_wel_erase_sector = true;
    buf[0] = instruction = WRITE_ENABLE;
    return spi_start_transfer();
}


bool
spi_flash_status(void)
{
    uint8_t *buf = spi_lock(2);
    if (buf == NULL)
        return false;

    buf[0] = instruction = STATUS;
    buf[1] = 0;
    return spi_start_transfer();
}


bool
spi_flash_jedec_id(void)
{
    uint8_t *buf = spi_lock(4);
    if (buf == NULL)
        return false;

    buf[0] = instruction = JEDEC_ID;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 0;
    return spi_start_transfer();
}


bool
spi_flash_powerup(void)
{
    uint8_t *buf = spi_lock(1);
    if (buf == NULL)
        return false;

    GPIOB->BSRR = GPIO_BSRR_BR_0;

    buf[0] = instruction = POWER_UP;
    return spi_start_transfer();
}


bool
spi_flash_powerdown(void)
{
    uint8_t *buf = spi_lock(1);
    if (buf == NULL)
        return false;

    buf[0] = instruction = POWER_DOWN;
    return spi_start_transfer();
}


bool
spi_flash_is_locked(void)
{
    return spi_is_locked() || flash_buf_locked_len != 0;
}


void
spi_transfer_complete_cb(const uint8_t *buf, uint32_t buf_len)
{
    if (buf_len == 0)
        return;

    switch (instruction) {
    case WRITE:
        break;

    case READ:
        spi_flash_read_cb(buf + 4, buf_len - 4);
        break;

    case STATUS:
        if (waiting_wel_erase_sector && (buf[1] & (1 << 1))) {
            waiting_wel_erase_sector = false;
            start_erase_sector = true;
        }
        else if (waiting_wel_write && (buf[1] & (1 << 1))) {
            waiting_wel_write = false;
            start_write = true;
        }
        else if (waiting_erase_sector && ((buf[1] & (1 << 0)) == 0)) {
            waiting_erase_sector = false;
            TIM3->CR1 &= ~TIM_CR1_CEN;
            spi_flash_erase_sector_cb();
            flash_buf_locked_len = 0;
        }
        else if (waiting_write && ((buf[1] & (1 << 0)) == 0)) {
            waiting_write = false;
            TIM3->CR1 &= ~TIM_CR1_CEN;
            spi_flash_write_cb();
            flash_buf_locked_len = 0;
        }

        spi_flash_status_cb(buf[1]);
        break;

    case WRITE_ENABLE:
        TIM3->EGR = TIM_EGR_UG;
        TIM3->CR1 |= TIM_CR1_CEN;
        break;

    case ERASE_SECTOR:
        break;

    case JEDEC_ID:
        spi_flash_jedec_id_cb(buf[1], (buf[2] << 8) | buf[3]);
        break;

    case POWER_UP:
        spi_flash_powerup_cb();
        break;

    case POWER_DOWN:
        GPIOB->BSRR = GPIO_BSRR_BS_0;
        spi_flash_powerdown_cb();
        break;
    }
}
