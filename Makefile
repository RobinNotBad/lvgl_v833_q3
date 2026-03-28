#
# Makefile
#
LVGL_DIR_NAME ?= lvgl
LVGL_DIR ?= ${shell pwd}
CFLAGS ?= -O3 -g0 -I$(LVGL_DIR) -I$(LVGL_DIR)/include -Wall -Wshadow -Wundef -Wmissing-prototypes -Wno-discarded-qualifiers -Wextra -Wno-unused-function -Wno-error=strict-prototypes -Wpointer-arith -fno-strict-aliasing -Wno-error=cpp -Wuninitialized -Wmaybe-uninitialized -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wformat-security -Wno-ignored-qualifiers -Wno-error=pedantic -Wno-sign-compare -Wno-error=missing-prototypes -Wdouble-promotion -Wclobbered -Wdeprecated -Wempty-body -Wshift-negative-value -Wstack-usage=2048 -Wno-unused-value -fdiagnostics-color=always

LDFLAGS ?= -L$(LVGL_DIR)/lib -Wl,-rpath,'/mnt/UDISK/lib:/mnt/app/lib:/mnt/app/dendro/lib' -Wl,--disable-new-dtags -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc -lswresample -lswscale -lz -lasound -lfreetype -ltimidity #-lcurl -lssl -lcrypto

BIN = dendro_run


#Collect the files to compile
MAINSRC = ./main.c

include $(LVGL_DIR)/lvgl/lvgl.mk
include $(LVGL_DIR)/lv_drivers/lv_drivers.mk
include $(LVGL_DIR)/pages/pages.mk
include $(LVGL_DIR)/platform/platform.mk
include $(LVGL_DIR)/views/views.mk
include $(LVGL_DIR)/cJSON/cJSON.mk
include $(LVGL_DIR)/lv_lib_100ask/lv_lib_100ask.mk

#CSRCS +=$(LVGL_DIR)/mouse_cursor_icon.c 

OBJEXT ?= .o

AOBJS = $(ASRCS:.S=$(OBJEXT))
COBJS = $(CSRCS:.c=$(OBJEXT))

MAINOBJ = $(MAINSRC:.c=$(OBJEXT))

SRCS = $(ASRCS) $(CSRCS) $(MAINSRC)
OBJS = $(AOBJS) $(COBJS)

## MAINOBJ -> OBJFILES

all: default

%.o: %.c
	@$(CC)  $(CFLAGS) -c $< -o $@
	@echo "CC  $<"
    
default: $(AOBJS) $(COBJS) $(MAINOBJ)
	@echo "CC  LINK ALL"
	@$(CC) -o $(BIN) $(MAINOBJ) $(AOBJS) $(COBJS) $(LDFLAGS)
	@echo "ALL DONE"
	
clean: 
	rm -f $(BIN) $(AOBJS) $(COBJS) $(MAINOBJ)

