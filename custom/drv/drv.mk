DRV_UART_DIR := custom/drv

#uart
OC_FILES +=  $(DRV_UART_DIR)/uart/drv_virt_at.c
OC_FILES +=  $(DRV_UART_DIR)/uart/drv_uart.c
INC      += -I'$(DRV_UART_DIR)/uart'

#lw_oopc
INC      += -I'$(DRV_UART_DIR)/lw_oopc'