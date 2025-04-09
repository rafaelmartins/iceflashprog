// SPDX-FileCopyrightText: 2025 Rafael G. Martins <rafael@rafaelmartins.eng.br>
// SPDX-License-Identifier: GPL-2.0-only

#include <stm32f0xx.h>

#include "watchdog.h"


void
watchdog_init(void)
{
    RCC->CSR |= RCC_CSR_LSION;
    while ((RCC->CSR & RCC_CSR_LSIRDY) != RCC_CSR_LSIRDY);

#ifdef WATCHDOG_STOP_ON_HALT
    RCC->APB2ENR |= RCC_APB2ENR_DBGMCUEN;
    DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;
#endif

    IWDG->KR = 0xcccc;
    IWDG->KR = 0x5555;
    IWDG->PR = IWDG_PR_PR;
    while ((IWDG->SR & IWDG_SR_PVU) == IWDG_SR_PVU);
    watchdog_set_reload_default();
}


void
watchdog_reload(void)
{
    IWDG->KR = 0xaaaa;
}


static inline void
set_reload(uint16_t reload)
{
    while ((IWDG->SR & IWDG_SR_RVU) == IWDG_SR_RVU);
    IWDG->KR = 0x5555;
    IWDG->RLR = reload;
    while ((IWDG->SR & IWDG_SR_RVU) == IWDG_SR_RVU);
    IWDG->KR = 0xaaaa;
}

void
watchdog_set_reload_default(void)
{
    set_reload(156 - 1);  // ~1s
}

void
watchdog_set_reload_erase_chip(void)
{
    set_reload(2343 - 1);  // ~15s
}
