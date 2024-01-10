include tools/scripts/config.mk

CUSTOM_DIR := custom

#application
APPLACTION_DIR := custom/applaction
OC_FILES +=  $(APPLACTION_DIR)/app.c
OC_FILES +=  $(APPLACTION_DIR)/main.c
INC      += -I'$(APPLACTION_DIR)'

ifeq ($(strip $(DEMO_SUPPORT)),n)
include $(CUSTOM_DIR)/drv/drv.mk
include $(CUSTOM_DIR)/bsp/bsp.mk
endif
