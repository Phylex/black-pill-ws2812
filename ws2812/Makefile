PROJECT = ws2812
BUILD_DIR = bin

SHARED_DIR = ../common
CFILES = ws2812.c
CFILES += libws2812.c
#AFILES += 

# specify the device for the linkerscript generator
DEVICE=stm32f411CEU6
# OOCD_FILE = target/stm32f4x

# You shouldn't have to edit anything below here.
VPATH += $(SHARED_DIR)
INCLUDES += $(patsubst %,-I%, . $(SHARED_DIR))
OPENCM3_DIR=../libopencm3

include $(OPENCM3_DIR)/mk/genlink-config.mk
include ../rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk
