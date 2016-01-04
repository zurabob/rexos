/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_rtc.h"
#include "stm32_config.h"
#include "../../userspace/rtc.h"
#include "stm32_core_private.h"
#include "sys_config.h"
#include "../../userspace/sys.h"
#include "../../userspace/time.h"
#include "../../userspace/irq.h"
#include "../../userspace/systime.h"

#if defined(STM32L0)
#define RTC_EXTI_LINE                               20
#endif

void stm32_rtc_isr(int vector, void* param)
{
#if defined(STM32F1)
    while ((RTC->CRL & RTC_CRL_RTOFF) == 0) {}
    RTC->CRL = 0;
#else
    EXTI->PR = 1 << RTC_EXTI_LINE;
    RTC->ISR &= ~RTC_ISR_WUTF;
#endif
    systime_second_pulse();
}

static inline void backup_on()
{
#if defined(STM32F1) || defined(STM32F2) || defined(STM32F4)
    //enable BACKUP interface
    RCC->APB1ENR |= RCC_APB1ENR_BKPEN;
#elif defined(STM32L0)
    PWR->CR |= PWR_CR_DBP;
    //HSE as clock source can cause faults on pin reset, so reset backup domain is required
#if !(LSE_VALUE)
    RCC->CSR |= RCC_CSR_RTCRST;
    __NOP();
    __NOP();
    __NOP();
    RCC->CSR &= ~RCC_CSR_RTCRST;
#endif

    __disable_irq();
    RTC->WPR = 0xca;
    RTC->WPR = 0x53;
    __enable_irq();
#endif
    PWR->CR |= PWR_CR_DBP;
}

static inline void backup_off()
{
#if defined(STM32F1) || defined(STM32F2) || defined(STM32F4)
    //disable POWER and BACKUP interface
    PWR->CR &= ~PWR_CR_DBP;
    RCC->APB1ENR &= ~RCC_APB1ENR_BKPEN;
#elif defined(STM32L0)
    __disable_irq();
    RTC->WPR = 0x00;
    RTC->WPR = 0xff;
    __enable_irq();
    PWR->CR &= ~PWR_CR_DBP;
#endif
}

static void enter_configuration()
{
#if defined(STM32F1)
    while ((RTC->CRL & RTC_CRL_RTOFF) == 0) {}
    RTC->CRL |= RTC_CRL_CNF;
#else
    RTC->ISR |= RTC_ISR_INIT;
    while ((RTC->ISR & RTC_ISR_INITF) == 0) {}
#endif
}

static void leave_configuration()
{
#if defined(STM32F1)
    RTC->CRL &= ~RTC_CRL_CNF;
    while ((RTC->CRL & RTC_CRL_RTOFF) == 0) {}
#else
    RTC->ISR &= ~RTC_ISR_INIT;
#endif
}

void stm32_rtc_init()
{
    backup_on();

    //backup domain reset?
#if defined(STM32F1)
    if (RCC->BDCR == 0)
    {
        //turn on 32khz oscillator
        RCC->BDCR |= RCC_BDCR_LSEON;
        while ((RCC->BDCR & RCC_BDCR_LSERDY) == 0) {}
        //select LSE as clock source
        RCC->BDCR |= (01ul << 8ul);
        //turn on RTC
        RCC->BDCR |= RCC_BDCR_RTCEN;

        enter_configuration();

        //prescaller to 1 sec
        RTC->PRLH = (LSE_VALUE >> 16) & 0xf;
        RTC->PRLL = (LSE_VALUE & 0xffff) - 1;

        //reset counter & alarm
        RTC->CNTH = 0;
        RTC->CNTL = 0;
        RTC->ALRH = 0;
        RTC->ALRL = 0;

        leave_configuration();
    }

    //wait for APB1<->RTC_CORE sync
    RTC->CRL &= RTC_CRL_RSF;
    while ((RTC->CRL & RTC_CRL_RSF) == 0) {}

    //enable second pulse
    while ((RTC->CRL & RTC_CRL_RTOFF) == 0) {}
    RTC->CRH |= RTC_CRH_SECIE;
#else

    if ((RCC->CSR & RCC_CSR_RTCEN) == 0)
    {
        RCC->CSR &= ~(3 << 16);

#if (LSE_VALUE)
        //turn on 32khz oscillator
        RCC->CSR |= RCC_CSR_LSEON;
        while ((RCC->CSR & RCC_CSR_LSERDY) == 0) {}
        //select LSE as clock source
        RCC->CSR |= RCC_CSR_RTCSEL_LSE;
#else
        //select HSE as clock source
        RCC->CSR |= RCC_CSR_RTCSEL_HSE;
#endif
        //turn on RTC
        RCC->CSR |= RCC_CSR_RTCEN;
        enter_configuration();

        //prescaller to 1 sec
#if (LSE_VALUE)
        RTC->PRER = ((128 - 1) << 16) | (LSE_VALUE / 128 - 1);
#else
        RTC->PRER = ((64 - 1) << 16) | (1000000 / 64 - 1);
#endif

        //00:00
        RTC->TR = 0;
        //01.01.2015, thursday
        RTC->DR = 0x00158101;
        //setup second tick
        RTC->CR &= ~RTC_CR_WUTE;
        while ((RTC->ISR & RTC_ISR_WUTWF) == 0) {}
        RTC->CR |= 4;
        RTC->WUTR = 0;
    }
    else
        enter_configuration();
    RTC->CR |= RTC_CR_WUTE | RTC_CR_WUTIE;
    leave_configuration();

    RTC->ISR &= ~RTC_ISR_WUTF;
    //setup EXTI for second pulse
    EXTI->IMR |= 1 << RTC_EXTI_LINE;
    EXTI->RTSR |= 1 << RTC_EXTI_LINE;
#endif
    irq_register(RTC_IRQn, stm32_rtc_isr, NULL);
    NVIC_EnableIRQ(RTC_IRQn);
    NVIC_SetPriority(RTC_IRQn, 13);
}

TIME* stm32_rtc_get(TIME* time)
{
#if defined(STM32F1)
    unsigned long value = (unsigned long)(((RTC->CNTH) << 16ul) | (RTC->CNTL));
    time->ms = (value % SEC_IN_DAY) * 1000;
    time->day = value / SEC_IN_DAY + EPOCH_DATE;
    return time;
#else
    //fucking shit
    struct tm ts;
    ts.tm_sec = ((RTC->TR >> 4) & 7) * 10 + (RTC->TR & 0xf);
    ts.tm_min = ((RTC->TR >> 12) & 7) * 10 + ((RTC->TR >> 8) & 0xf);
    ts.tm_hour = ((RTC->TR >> 20) & 3) * 10 + ((RTC->TR >> 16) & 0xf);
    ts.tm_msec = 0;

    ts.tm_mday = ((RTC->DR >> 4) & 3) * 10 + (RTC->DR & 0xf);
    ts.tm_mon = ((RTC->DR >> 12) & 1) * 10 + ((RTC->DR >> 8) & 0xf);
    ts.tm_year = ((RTC->DR >> 20) & 0xf) * 10 + ((RTC->DR >> 16) & 0xf) + 2000;

    return mktime(&ts, time);
#endif
}

void stm32_rtc_set(TIME* time)
{
    enter_configuration();
#if defined(STM32F1)
    unsigned long value = 0;
    if (time->day >= EPOCH_DATE)
        value = (time->day - EPOCH_DATE) * SEC_IN_DAY;
    value += time->ms / 1000;
    RTC->CNTH = (uint16_t)(value >> 16ul);
    RTC->CNTL = (uint16_t)(value & 0xffff);

#else
    //fucking shit
    struct tm ts;
    gmtime(time, &ts);
    ts.tm_year -= 2000;
    RTC->TR = ((ts.tm_hour / 10) << 20) | ((ts.tm_hour % 10) << 16) |
              ((ts.tm_min / 10) << 12)  | ((ts.tm_min % 10) << 8)   |
              ((ts.tm_sec / 10) << 4)   | (ts.tm_sec % 10);
    RTC->DR = ((ts.tm_year / 10) << 20) | ((ts.tm_year % 10) << 16) |
              ((ts.tm_mon / 10) << 12)  | ((ts.tm_mon % 10) << 8)   |
              ((ts.tm_mday / 10) << 4)  | (ts.tm_mday % 10) | (1 << 13);
#endif
    leave_configuration();
}

void stm32_rtc_request(IPC* ipc)
{
    TIME time;
    switch (HAL_ITEM(ipc->cmd))
    {
    case RTC_GET:
        stm32_rtc_get(&time);
        ipc->param1 = (unsigned int)time.day;
        ipc->param2 = (unsigned int)time.ms;
        break;
    case RTC_SET:
        time.day = ipc->param1;
        time.ms = ipc->param2;
        stm32_rtc_set(&time);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}

void stm32_rtc_disable()
{
#if defined(STM32F1)
    while ((RTC->CRL & RTC_CRL_RTOFF) == 0) {}
    RTC->CRH = 0;
#else //STM32F1
    enter_configuration();
    RTC->CR &= ~(RTC_CR_WUTE | RTC_CR_WUTIE | RTC_CR_TSE | RTC_CR_TSIE | RTC_CR_ALRAE | RTC_CR_ALRAIE | RTC_CR_ALRBE | RTC_CR_ALRBIE);
    leave_configuration();
#endif
}
