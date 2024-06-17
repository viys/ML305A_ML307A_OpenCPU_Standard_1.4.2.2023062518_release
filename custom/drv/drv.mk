DRV_UART_DIR := custom/drv

#uart
OC_FILES +=  $(DRV_UART_DIR)/uart/drv_virt_at.c
OC_FILES +=  $(DRV_UART_DIR)/uart/drv_uart.c
INC      += -I'$(DRV_UART_DIR)/uart'

#lw_oopc
INC      += -I'$(DRV_UART_DIR)/lw_oopc'

#gpio
OC_FILES +=  $(DRV_UART_DIR)/gpio/drv_gpio.c
INC      += -I'$(DRV_UART_DIR)/gpio'

#socket
OC_FILES +=  $(DRV_UART_DIR)/socket/drv_socket.c
INC      += -I'$(DRV_UART_DIR)/socket'

#adc
OC_FILES +=  $(DRV_UART_DIR)/adc/drv_adc.c
INC      += -I'$(DRV_UART_DIR)/adc'

#mqtt
OC_FILES +=  $(DRV_UART_DIR)/mqtt/drv_mqtt.c
INC      += -I'$(DRV_UART_DIR)/mqtt'

#mqtt
OC_FILES +=  $(DRV_UART_DIR)/ntp/drv_ntp.c
INC      += -I'$(DRV_UART_DIR)/ntp'

#shell
OC_FILES +=  $(DRV_UART_DIR)/shell/drv_shell.c
INC      += -I'$(DRV_UART_DIR)/shell'

#audio
OC_FILES +=  $(DRV_UART_DIR)/audio/mp3_array.c
OC_FILES +=  $(DRV_UART_DIR)/audio/mp3_linknet.c
OC_FILES +=  $(DRV_UART_DIR)/audio/mp3_callstart.c
OC_FILES +=  $(DRV_UART_DIR)/audio/mp3_openlock.c
OC_FILES +=  $(DRV_UART_DIR)/audio/mp3_closelock.c
OC_FILES +=  $(DRV_UART_DIR)/audio/mp3_vfy_err.c
OC_FILES +=  $(DRV_UART_DIR)/audio/mp3_open_failed.c
OC_FILES +=  $(DRV_UART_DIR)/audio/mp3_close_failed.c
OC_FILES +=  $(DRV_UART_DIR)/audio/drv_audio.c
INC      += -I'$(DRV_UART_DIR)/audio'
