#include <microkit.h>

extern uintptr_t uart_base;

#define REG_PTR(offset) ((volatile uint32_t *)((uart_base) + (offset)))

#if defined(CONFIG_PLAT_HIFIVE)

#define UART_TX_DATA_MASK  0xFF
#define UART_TX_DATA_FULL  BIT(31)

#define UART_RX_DATA_MASK   0xFF
#define UART_RX_DATA_EMPTY  BIT(31)

#define UART_TX_INT_EN     BIT(0)
#define UART_RX_INT_EN     BIT(1)

#define UART_TX_INT_PEND     BIT(0)
#define UART_RX_INT_PEND     BIT(1)
#define UART_BAUD_DIVISOR 4340

struct uart {
    uint32_t txdata;
    uint32_t rxdata;
    uint32_t txctrl;
    uint32_t rxctrl;
    uint32_t ie;
    uint32_t ip;
    uint32_t div;
};
typedef volatile struct uart uart_regs_t;
static inline uart_regs_t* regs = uart_base;

void _sddf_putchar(char character)
{
    if (!(regs->txdata & UART_TX_DATA_FULL)) {
        if (c == '\n' && (d->flags & SERIAL_AUTO_CR)) {
            regs->txdata = '\r' & UART_TX_DATA_MASK;
            while(regs->txdata & UART_TX_DATA_FULL) {}
        }
        regs->txdata = c & UART_TX_DATA_MASK;
        return c;
    } else {
        return -1;
    }
}

#endif

#ifdef CONFIG_PLAT_ODROIDC4

#define UART_STATUS 0xC
#define UART_WFIFO 0x0
#define UART_TX_FULL (1 << 21)

void _sddf_putchar(char character)
{
    while ((*REG_PTR(UART_STATUS) & UART_TX_FULL)) {}
    *REG_PTR(UART_WFIFO) = character & 0x7f;
}

#endif