// SPDX-FileCopyrightText: 2025 Rafael G. Martins <rafael@rafaelmartins.eng.br>
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <stdbool.h>
#include <stdint.h>

#define SPI_FLASH_PAGE_SIZE 256

void spi_flash_init(void);
bool spi_flash_task(void);

bool spi_flash_read(uint8_t addr0, uint8_t addr1, uint8_t addr2);
bool spi_flash_write(uint8_t addr0, uint8_t addr1, uint8_t addr2, const uint8_t *data, uint32_t data_len);
bool spi_flash_erase_sector(uint8_t addr0, uint8_t addr1, uint8_t addr2);
bool spi_flash_erase_block(uint8_t addr0, uint8_t addr1, uint8_t addr2);
bool spi_flash_erase_chip(void);
bool spi_flash_status(void);
bool spi_flash_jedec_id(void);
bool spi_flash_powerup(void);
bool spi_flash_powerdown(void);
bool spi_flash_is_locked(void);

// callbacks
void spi_flash_erase_sector_cb(void);
void spi_flash_erase_block_cb(void);
void spi_flash_erase_chip_cb(void);
void spi_flash_write_cb(bool verified);
void spi_flash_read_cb(const uint8_t *buf, uint32_t len);
void spi_flash_status_cb(uint8_t status);
void spi_flash_jedec_id_cb(uint8_t manufacturer_id, uint16_t device_id);
void spi_flash_powerup_cb(void);
void spi_flash_powerdown_cb(void);
