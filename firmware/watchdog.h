// SPDX-FileCopyrightText: 2025 Rafael G. Martins <rafael@rafaelmartins.eng.br>
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

void watchdog_init(void);
void watchdog_reload(void);
void watchdog_set_reload_default(void);
void watchdog_set_reload_erase_chip(void);
