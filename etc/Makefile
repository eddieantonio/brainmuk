# Get absolute path to containing directory: http://stackoverflow.com/a/324782
TOP := $(dir $(CURDIR)/$(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))
LOCAL_INCLUDE_DIR := $(realpath $(TOP)../include)

CFLAGS = -std=c11 -fpic
CPPFLAGS := -I$(LOCAL_INCLUDE_DIR) $(CPPFLAGS)

ifeq ($(shell uname -s),Darwin)
OBJDUMP = otool -jtv
else
OBJDUMP = objdump -d
endif

all: instructions.lst miscellanea.lst

%.lst: %.o
	$(OBJDUMP) $< > $@

.PHONY: all
