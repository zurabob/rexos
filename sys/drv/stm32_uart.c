/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_uart.h"
#include "stm32_power.h"
#include "../sys_call.h"
#include "error.h"
#include "../../userspace/lib/stdlib.h"
#include "../../userspace/lib/stdio.h"
#include "../../userspace/stream.h"
#include "../../userspace/irq.h"
#include "stm32_config.h"

//one page
#define UART_STREAM_SIZE                                256

void stm32_uart();

typedef struct {
    UART_PORT port;
    PIN tx_pin, rx_pin;
    IPC tx_ipc, rx_ipc;
    int error;
    HANDLE tx_stream, rx_stream, tx_handle, rx_handle, process;
    unsigned int tx_total;
    unsigned int tx_chunk_pos, tx_chunk_size;
    char tx_buf[UART_TX_BUF_SIZE];
    char rx_char;
    int rx_free;
} UART;

const REX __STM32_UART = {
    //name
    "STM32 uart",
    //size
    UART_PROCESS_SIZE,
    //priority - driver priority. Setting priority lower than other drivers can cause IPC overflow on SYS_INFO
    89,
    //flags
    PROCESS_FLAGS_ACTIVE,
    //ipc size
    10,
    //function
    stm32_uart
};

typedef USART_TypeDef* USART_TypeDef_P;

#if defined(STM32F10X_LD) || defined(STM32F10X_LD_VL)
#define UARTS_COUNT                                         2
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {37, 38};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {14, 17};
static const PIN UART_TX_PINS[UARTS_COUNT] =                {A9, A2};
static const PIN UART_RX_PINS[UARTS_COUNT] =                {A10, A3};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2};

#elif defined(STM32F10X_MD) || defined(STM32F10X_MD_VL)
#define UARTS_COUNT                                         3
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {37, 38, 39};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {14, 17, 18};
static const PIN UART_TX_PINS[UARTS_COUNT] =                {A9, A2, B10};
static const PIN UART_RX_PINS[UARTS_COUNT] =                {A10, A3, B11};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2, USART3};

#elif defined(STM32F1)
#define UARTS_COUNT                                         5
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {37, 38, 39, 52, 53};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {14, 17, 18, 19, 20};
static const PIN UART_TX_PINS[UARTS_COUNT] =                {A9, A2, B10, C10, C12};
static const PIN UART_RX_PINS[UARTS_COUNT] =                {A10, A3, B11, C11, D2};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2, USART3, UART4, UART5};

#elif defined(STM32F2) || defined(STM32F40_41xxx)
#define UARTS_COUNT                                         6
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {37, 38, 39, 52, 53, 71};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {4, 17, 18, 19, 20, 5};
static const PIN UART_TX_PINS[UARTS_COUNT] =                {A9, A2, B10, C10, C12, C6};
static const PIN UART_RX_PINS[UARTS_COUNT] =                {A10, A3, B11, C11, D2, C7};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2, USART3, UART4, UART5, USART6};

#elif defined(STM32F4)
#define UARTS_COUNT                                         8
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {37, 38, 39, 52, 53, 71, 82, 83};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {4, 17, 18, 19, 20, 5, 30, 31};
static const PIN UART_TX_PINS[UARTS_COUNT] =                {A9, A2, B10, C10, C12, C6, F7, F1};
static const PIN UART_RX_PINS[UARTS_COUNT] =                {A10, A3, B11, C11, D2, C7, F6, E0};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2, USART3, UART4, UART5, USART6, UART7, UART8};
#endif

void stm32_uart_on_isr(int vector, void* param)
{
    UART* uart = (UART*)param;
    uint16_t sr = UART_REGS[uart->port]->SR;

    //transmit more
    if ((sr & USART_SR_TXE) && uart->tx_chunk_size)
    {
        UART_REGS[uart->port]->DR = uart->tx_buf[uart->tx_chunk_pos++];
        //no more
        if (uart->tx_chunk_pos >= uart->tx_chunk_size)
        {
            uart->tx_chunk_pos = uart->tx_chunk_size = 0;
            uart->tx_ipc.process = uart->process;
            uart->tx_ipc.cmd = IPC_UART_ISR_WRITE_CHUNK_COMPLETE;
            uart->tx_ipc.param1 = (unsigned int)uart;
            ipc_ipost(&uart->tx_ipc);
            UART_REGS[uart->port]->CR1 &= ~USART_CR1_TXEIE;
            UART_REGS[uart->port]->CR1 |= USART_CR1_TCIE;

        }
    }
    //transmission completed and no more data. Disable transmitter
    else if ((sr & USART_SR_TC) &&  uart->tx_total == 0)
        UART_REGS[uart->port]->CR1 &= ~(USART_CR1_TE | USART_CR1_TCIE);
    //decode error, if any
    if ((sr & (USART_SR_PE | USART_SR_FE | USART_SR_NE | USART_SR_ORE)))
    {
        if (sr & USART_SR_ORE)
            uart->error = ERROR_OVERFLOW;
        else
        {
            __REG_RC32(UART_REGS[uart->port]->DR);
            if (sr & USART_SR_FE)
                uart->error = ERROR_UART_FRAME;
            else if (sr & USART_SR_PE)
                uart->error = ERROR_UART_PARITY;
            else if  (sr & USART_SR_NE)
                uart->error = ERROR_UART_NOISE;
        }
    }

    //receive data
    if (sr & USART_SR_RXNE)
    {
        uart->rx_char = UART_REGS[uart->port]->DR;
        uart->rx_ipc.process = uart->process;
        uart->rx_ipc.cmd = IPC_UART_ISR_RX_CHAR;
        uart->rx_ipc.param1 = (unsigned int)uart;
        ipc_ipost(&uart->rx_ipc);
    }
}

void uart_write_kernel(const char *const buf, unsigned int size, void* param)
{
    int i;
    UART_REGS[(UART_PORT)param]->CR1 |= USART_CR1_TE;
    for(i = 0; i < size; ++i)
    {
        while ((UART_REGS[(UART_PORT)param]->SR & USART_SR_TXE) == 0) {}
        UART_REGS[(UART_PORT)param]->DR = buf[i];
    }
    UART_REGS[(UART_PORT)param]->CR1 |= USART_CR1_TCIE;
    //transmitter will be disabled int next IRQ TC
}

bool uart_set_baudrate(UART* uart, const UART_BAUD* config)
{
    unsigned int clock;
    HANDLE power = sys_get_object(SYS_OBJECT_POWER);
    if (power == INVALID_HANDLE)
    {
        error(ERROR_NOT_FOUND);
        return false;
    }
    UART_REGS[uart->port]->CR1 &= ~USART_CR1_UE;

    if (config->data_bits == 8 && config->parity != 'N')
        UART_REGS[uart->port]->CR1 |= USART_CR1_M;
    else
        UART_REGS[uart->port]->CR1 &= ~USART_CR1_M;

    if (config->parity != 'N')
    {
        UART_REGS[uart->port]->CR1 |= USART_CR1_PCE;
        if (config->parity == 'O')
            UART_REGS[uart->port]->CR1 |= USART_CR1_PS;
        else
            UART_REGS[uart->port]->CR1 &= ~USART_CR1_PS;
    }
    else
        UART_REGS[uart->port]->CR1 &= ~USART_CR1_PCE;

    UART_REGS[uart->port]->CR2 = (config->stop_bits == 1 ? 0 : 2) << 12;
    UART_REGS[uart->port]->CR3 = 0;

    if (uart->port == UART_1 || uart->port == UART_6)
        clock = get(power, IPC_GET_CLOCK, STM32_CLOCK_APB2, 0, 0);
    else
        clock = get(power, IPC_GET_CLOCK, STM32_CLOCK_APB1, 0, 0);
    if (clock == 0)
        return false;
    unsigned int mantissa, fraction;
    mantissa = (25 * clock) / (4 * (config->baud));
    fraction = ((mantissa % 100) * 8 + 25)  / 50;
    mantissa = mantissa / 100;
    UART_REGS[uart->port]->BRR = (mantissa << 4) | fraction;

    UART_REGS[uart->port]->CR1 |= USART_CR1_UE | USART_CR1_PEIE;
    UART_REGS[uart->port]->CR3 |= USART_CR3_EIE;

    UART_REGS[uart->port]->CR1 |= USART_CR1_TE;
    return true;
}

UART* uart_enable(UART_PORT port, UART_ENABLE* ue)
{
    UART* uart = NULL;
    HANDLE gpio = sys_get_object(SYS_OBJECT_GPIO);
    if (gpio == INVALID_HANDLE)
    {
        error(ERROR_NOT_FOUND);
        return NULL;
    }
    if (port >= UARTS_COUNT)
    {
        error(ERROR_NOT_SUPPORTED);
        return NULL;
    }
    uart = (UART*)malloc(sizeof(UART));
    if (uart == NULL)
    {
        error(ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    uart->port = port;
    uart->tx_pin = ue->tx;
    uart->rx_pin = ue->rx;
    uart->error = ERROR_OK;
    uart->tx_stream = INVALID_HANDLE;
    uart->tx_handle = INVALID_HANDLE;
    uart->rx_stream = INVALID_HANDLE;
    uart->rx_handle = INVALID_HANDLE;
    uart->tx_total = 0;
    uart->tx_chunk_pos = uart->tx_chunk_size = 0;
    uart->process = process_get_current();
    uart->rx_free = UART_STREAM_SIZE;
    if (uart->tx_pin == PIN_DEFAULT)
        uart->tx_pin = UART_TX_PINS[uart->port];
    if (uart->rx_pin == PIN_DEFAULT)
        uart->rx_pin = UART_RX_PINS[uart->port];

    if (uart->tx_pin != PIN_UNUSED)
    {
        uart->tx_stream = stream_create(UART_STREAM_SIZE);
        if (uart->tx_stream == INVALID_HANDLE)
        {
            free(uart);
            return NULL;
        }
        uart->tx_handle = stream_open(uart->tx_stream);
        if (uart->tx_handle == INVALID_HANDLE)
        {
            stream_destroy(uart->tx_stream);
            free(uart);
            return NULL;
        }
        stream_start_listen(uart->tx_stream, (void*)uart);
    }
    if (uart->rx_pin != PIN_UNUSED)
    {
        uart->rx_stream = stream_create(UART_STREAM_SIZE);
        if (uart->rx_stream == INVALID_HANDLE)
        {
            stream_close(uart->tx_handle);
            stream_destroy(uart->tx_stream);
            free(uart);
            return NULL;
        }
        uart->rx_handle = stream_open(uart->rx_stream);
        if (uart->rx_handle == INVALID_HANDLE)
        {
            stream_destroy(uart->rx_stream);
            stream_close(uart->tx_handle);
            stream_destroy(uart->tx_stream);
            free(uart);
            return NULL;
        }
    }

    //setup pins
#if defined(STM32F1)
    //turn on remapping
    if (((uart->tx_pin != UART_TX_PINS[uart->port]) && (uart->tx_pin != PIN_UNUSED)) ||
        ((uart->rx_pin != UART_RX_PINS[uart->port]) && (uart->rx_pin != PIN_UNUSED)))
    {
        RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
        switch (uart->tx_pin)
        {
        case B6:
            AFIO->MAPR |= AFIO_MAPR_USART1_REMAP;
            break;
        case D5:
            AFIO->MAPR |= AFIO_MAPR_USART2_REMAP;
            break;
        case C10:
            AFIO->MAPR |= AFIO_MAPR_USART3_REMAP_PARTIALREMAP;
            break;
        case D8:
            AFIO->MAPR |= AFIO_MAPR_USART3_REMAP_FULLREMAP;
            break;
        default:
            error(ERROR_NOT_SUPPORTED);
            free(uart);
            return NULL;
        }
    }
#endif
    if (uart->tx_pin != PIN_UNUSED)
    {
#if defined(STM32F1)
        ack(gpio, IPC_ENABLE_PIN_SYSTEM, uart->tx_pin, GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);
#elif defined(STM32F2) || defined(STM32F4)
        ack(gpio, IPC_ENABLE_PIN_SYSTEM, uart->tx_pin, GPIO_MODE_AF | GPIO_OT_PUSH_PULL |  GPIO_SPEED_HIGH, uart->port < UART_4 ? AF7 : AF8);
#endif
    }

    if (uart->rx_pin != PIN_UNUSED)
    {
#if defined(STM32F1)
        ack(gpio, IPC_ENABLE_PIN_SYSTEM, uart->rx_pin, GPIO_MODE_INPUT_FLOAT, false);
#elif defined(STM32F2) || defined(STM32F4)
        ack(gpio, IPC_ENABLE_PIN_SYSTEM, uart->rx_pin, , GPIO_MODE_AF | GPIO_SPEED_HIGH, uart->port < UART_4 ? AF7 : AF8);
#endif
    }
    //power up
    if (uart->port == UART_1 || uart->port == UART_6)
        RCC->APB2ENR |= 1 << UART_POWER_PINS[uart->port];
    else
        RCC->APB1ENR |= 1 << UART_POWER_PINS[uart->port];

    //enable core
    UART_REGS[uart->port]->CR1 |= USART_CR1_UE;
    //enable receiver
    if (uart->rx_pin != PIN_UNUSED)
        UART_REGS[uart->port]->CR1 |= USART_CR1_RE | USART_CR1_RXNEIE;

    uart_set_baudrate(uart, &ue->baud);
    //enable interrupts
    irq_register(UART_VECTORS[uart->port], stm32_uart_on_isr, (void*)uart);
    NVIC_EnableIRQ(UART_VECTORS[uart->port]);
    NVIC_SetPriority(UART_VECTORS[uart->port], 15);
    return uart;
}

static inline bool uart_disable(UART* uart)
{
    HANDLE gpio = sys_get_object(SYS_OBJECT_GPIO);
    if (gpio == INVALID_HANDLE)
    {
        error(ERROR_NOT_FOUND);
        return false;
    }
    //disable interrupts
    NVIC_DisableIRQ(UART_VECTORS[uart->port]);
    irq_unregister(UART_VECTORS[uart->port]);

    //disable receiver
    if (uart->rx_pin != PIN_UNUSED)
        UART_REGS[uart->port]->CR1 &= ~(USART_CR1_RE | USART_CR1_RXNEIE);
    //disable core
    UART_REGS[uart->port]->CR1 &= ~USART_CR1_UE;
    //power down
    if (uart->port == UART_1 || uart->port == UART_6)
        RCC->APB2ENR &= ~(1 << UART_POWER_PINS[uart->port]);
    else
        RCC->APB1ENR &= ~(1 << UART_POWER_PINS[uart->port]);

    //disable pins
    if (uart->tx_pin != PIN_UNUSED)
    {
        stream_stop_listen(uart->tx_stream);
        stream_close(uart->tx_handle);
        stream_destroy(uart->tx_stream);
        ack(gpio, IPC_DISABLE_PIN, uart->tx_pin, 0, 0);
    }
    if (uart->rx_pin != PIN_UNUSED)
    {
        stream_close(uart->rx_handle);
        stream_destroy(uart->rx_stream);
        ack(gpio, IPC_DISABLE_PIN, uart->rx_pin, 0, 0);
    }

#if defined(STM32F1)
    //turn off remapping
    if (((uart->tx_pin != UART_TX_PINS[uart->port]) && (uart->tx_pin != PIN_UNUSED)) ||
        ((uart->rx_pin != UART_RX_PINS[uart->port]) && (uart->rx_pin != PIN_UNUSED)))
    {
        switch (uart->tx_pin)
        {
        case B6:
            AFIO->MAPR &= ~AFIO_MAPR_USART1_REMAP;
            break;
        case D5:
            AFIO->MAPR &= ~AFIO_MAPR_USART2_REMAP;
            break;
        case C10:
            AFIO->MAPR &= ~AFIO_MAPR_USART3_REMAP_PARTIALREMAP;
            break;
        case D8:
            AFIO->MAPR &= ~AFIO_MAPR_USART3_REMAP_FULLREMAP;
            break;
        default:
            break;
        }
        //if not used anymore, save some power
        if (AFIO->MAPR == 0x00000000 && AFIO->MAPR2 == 0x00000000)
            RCC->APB2ENR &= RCC_APB2ENR_AFIOEN;
    }
#endif
    free(uart);
    return true;
}

static void stm32_uart_write_chunk(UART* uart)
{
    unsigned int to_read = UART_TX_BUF_SIZE;
    if (uart->tx_total < to_read)
        to_read = uart->tx_total;
    if (stream_read(uart->tx_handle, uart->tx_buf, to_read))
    {
        uart->tx_chunk_pos = 0;
        uart->tx_chunk_size = to_read;
        uart->tx_total -= to_read;
        //start transaction
        UART_REGS[uart->port]->CR1 |= USART_CR1_TE | USART_CR1_TXEIE;
    }
}

static inline void stm32_uart_loop(UART** uarts)
{
    IPC ipc;
    for (;;)
    {
        ipc_read_ms(&ipc, 0, 0);
        switch (ipc.cmd)
        {
        case IPC_PING:
            ipc_post(&ipc);
            break;
        case SYS_SET_STDIO:
            open_stdout();
            ipc_post(&ipc);
            break;
        case IPC_UART_ENABLE:
            //TODO need direct IO
            break;
        case IPC_UART_DISABLE:
            if (ipc.param1 < UARTS_COUNT && uarts[ipc.param1])
            {
                if (uart_disable(uarts[ipc.param1]))
                {
                    uarts[ipc.param1] = NULL;
                    ipc_post(&ipc);
                }
                else
                    ipc_post_error(ipc.process, get_last_error());
            }
            else
                ipc_post_error(ipc.process, ERROR_INVALID_PARAMS);
            break;
        case IPC_UART_SET_BAUDRATE:
            //TODO need direct IO
            break;
        case IPC_UART_GET_BAUDRATE:
            //TODO need direct IO
            break;
        case IPC_UART_FLUSH:
            if (ipc.param1 < UARTS_COUNT && uarts[ipc.param1])
            {
                if (uarts[ipc.param1]->tx_stream != INVALID_HANDLE)
                {
                    stream_flush(uarts[ipc.param1]->tx_stream);
                    uarts[ipc.param1]->tx_total = 0;
                    __disable_irq();
                    uarts[ipc.param1]->tx_chunk_pos = uarts[ipc.param1]->tx_chunk_size = 0;
                    __enable_irq();
                }
                if (uarts[ipc.param1]->rx_stream != INVALID_HANDLE)
                    stream_flush(uarts[ipc.param1]->rx_stream);
                ipc_post(&ipc);
            }
            else
                ipc_post_error(ipc.process, ERROR_INVALID_PARAMS);
            break;
        case IPC_UART_GET_TX_STREAM:
            if (ipc.param1 < UARTS_COUNT && uarts[ipc.param1])
            {
                ipc.param1 = uarts[ipc.param1]->tx_stream;
                ipc_post(&ipc);
            }
            else
                ipc_post_error(ipc.process, ERROR_INVALID_PARAMS);
            break;
        case IPC_UART_GET_RX_STREAM:
            if (ipc.param1 < UARTS_COUNT && uarts[ipc.param1])
            {
                ipc.param1 = uarts[ipc.param1]->rx_stream;
                ipc_post(&ipc);
            }
            else
                ipc_post_error(ipc.process, ERROR_INVALID_PARAMS);
            break;
        case IPC_UART_GET_LAST_ERROR:
            if (ipc.param1 < UARTS_COUNT && uarts[ipc.param1])
            {
                ipc.param1 = uarts[ipc.param1]->error;
                ipc_post(&ipc);
            }
            else
                ipc_post_error(ipc.process, ERROR_INVALID_PARAMS);
            break;
        case IPC_UART_CLEAR_ERROR:
            if (ipc.param1 < UARTS_COUNT && uarts[ipc.param1])
            {
                uarts[ipc.param1]->error = ERROR_OK;
                ipc_post(&ipc);
            }
            else
                ipc_post_error(ipc.process, ERROR_INVALID_PARAMS);
            break;
        case IPC_STREAM_WRITE:
            ((UART*)ipc.param3)->tx_total += ipc.param2;
            //read here is safe
            if (((UART*)ipc.param3)->tx_chunk_size == 0)
                //idle. Can start next transaction.
                stm32_uart_write_chunk(((UART*)ipc.param3));
            //message from kernel, no response
            break;
        case IPC_UART_ISR_WRITE_CHUNK_COMPLETE:
            //make sure we didn't loose any IPC STREAM_WRITE
            if (((UART*)ipc.param1)->tx_total == 0)
                ((UART*)ipc.param1)->tx_total = stream_get_size(((UART*)ipc.param1)->tx_stream);
            if (((UART*)ipc.param1)->tx_total)
                stm32_uart_write_chunk(((UART*)ipc.param1));
            //message from ISR, no response
            break;
        case IPC_UART_ISR_RX_CHAR:
            //caching calls to svc
            if (((UART*)ipc.param1)->rx_free == 0)
                ((UART*)ipc.param1)->rx_free = stream_get_free(((UART*)ipc.param1)->rx_stream);
            //if stream is full, char will be discarded
            if (((UART*)ipc.param1)->rx_free)
            {
                stream_write(((UART*)ipc.param1)->rx_handle, &((UART*)ipc.param1)->rx_char, 1);
                ((UART*)ipc.param1)->rx_free--;
            }
            //message from ISR, no response
            break;
        default:
            ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
            break;
        }
    }
}

void stm32_uart()
{
    UART* uarts[UARTS_COUNT] = {0};
    sys_ack(SYS_SET_OBJECT, SYS_OBJECT_UART, 0, 0);

#if (UART_STDIO)
    UART_ENABLE ue;
    ue.tx = UART_STDIO_TX;
    ue.rx = UART_STDIO_RX;
    ue.baud.data_bits = UART_STDIO_DATA_BITS;
    ue.baud.parity = UART_STDIO_PARITY;
    ue.baud.stop_bits = UART_STDIO_STOP_BITS;
    ue.baud.baud = UART_STDIO_BAUD;
    uarts[UART_STDIO_PORT] = uart_enable(UART_STDIO_PORT, &ue);

    //setup kernel printk dbg
    setup_dbg(uart_write_kernel, (void*)UART_STDIO_PORT);
    //setup system stdio
    sys_ack(SYS_SET_OBJECT, SYS_OBJECT_STDOUT_STREAM, uarts[UART_STDIO_PORT]->tx_stream, 0);
    sys_ack(SYS_SET_OBJECT, SYS_OBJECT_STDIN_STREAM, uarts[UART_STDIO_PORT]->rx_stream, 0);
    //say early processes, that stdio is setted up
    sys_ack(SYS_SET_STDIO, 0, 0, 0);
    //power
    ack(sys_get_object(SYS_OBJECT_POWER), SYS_SET_STDIO, 0, 0, 0);
    //gpio
    ack(sys_get_object(SYS_OBJECT_GPIO), SYS_SET_STDIO, 0, 0, 0);
    //timer
    ack(sys_get_object(SYS_OBJECT_TIMER), SYS_SET_STDIO, 0, 0, 0);
#endif //UART_STDIO

    stm32_uart_loop(uarts);
}