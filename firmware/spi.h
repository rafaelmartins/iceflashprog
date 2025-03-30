// SPDX-FileCopyrightText: 2025 Rafael G. Martins <rafael@rafaelmartins.eng.br>
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifndef SPI_BUFFER_SIZE
#define SPI_BUFFER_SIZE 260
#endif

void spi_init(void);
uint8_t* spi_lock(uint32_t len);
bool spi_start_transfer(void);
bool spi_task(void);
bool spi_is_locked(void);

// callbacks
void spi_transfer_complete_cb(const uint8_t *buf, uint32_t buf_len);
void spi_hook_cb(bool before);
