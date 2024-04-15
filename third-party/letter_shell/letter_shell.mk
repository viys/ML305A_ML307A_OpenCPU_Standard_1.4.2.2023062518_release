
ETHIRDPARTY_CJSON_DIR := third-party/letter_shell

OC_FILES +=  $(ETHIRDPARTY_CJSON_DIR)/shell_cmd_list.c
OC_FILES +=  $(ETHIRDPARTY_CJSON_DIR)/shell_companion.c
OC_FILES +=  $(ETHIRDPARTY_CJSON_DIR)/shell_ext.c
OC_FILES +=  $(ETHIRDPARTY_CJSON_DIR)/shell.c
# OC_FILES +=  $(ETHIRDPARTY_CJSON_DIR)/shell_port.c
INC      += -I'$(ETHIRDPARTY_CJSON_DIR)/'