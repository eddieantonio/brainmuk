# Get absolute path to containing directory: http://stackoverflow.com/a/324782
TOP := $(dir $(CURDIR)/$(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))
LOCAL_INCLUDE_DIR := $(realpath $(TOP)include)

# Special compiler flags
CFLAGS := -std=c11 -Wall -pedantic $(CFLAGS)
CPPFLAGS := -I$(LOCAL_INCLUDE_DIR) $(CPPFLAGS)
# Create .d for each .o file.
CPP_ADD_DEFINES = -MMD

# The shell command to rerun.
WATCH_COMMAND = make test
# I like to use this in iTerm 2:
#
# 	make watch WATCH_COMMAND='printf "\e]50;ClearScrollback\a"; make test'
#
# It clears scrollback when ever make test is rerun; makes isolating compile
# errors and the latest test failures quite easy.

# Options directed to the test launcher.
# Useful is -v -s [suite]
TEST_OPTIONS =

NAME = brainmuk
VERSION = 0.1.0

BIN = $(NAME)
TESTBIN = tests/$(NAME)_tests
SRCS = $(wildcard src/*.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))
DEPS := $(patsubst %.c,%.d,$(SRCS))

DISTNAME = $(NAME)-$(VERSION)

### Symbolic targets ###

all: $(BIN)

clean:
	-$(RM) $(BIN) $(TESTBIN)
	-$(RM) $(OBJS)
	-$(RM) $(DEPS)
	-$(RM) $(DISTNAME).tar.gz

dist: $(DISTNAME).tar.gz

test: $(TESTBIN)
	./$< $(TEST_OPTIONS)

full-test: $(BIN) test
	./$< tests/hello.bf

# Requires rerun <https://github.com/alexch/rerun> to be installed.
watch:
	@if hash rerun ; \
		then exit 0; \
		else echo "Requires rerun:\n\t$$ gem install rerun"; exit -1; fi
	-rerun --exit --clear --pattern '**/*.{c,h}' -- '$(WATCH_COMMAND)'

### Actual targets! ###

# Include dependencies from -d
-include $(DEPS)

# Generates the binaries from the objects
$(BIN): $(BIN).c $(OBJS)
$(TESTBIN): $(TESTBIN).c $(OBJS)

# Override the general C to object file conversion to generate defines.
%.o: %.c
	$(COMPILE.c) $(CPP_ADD_DEFINES) -o $@ $<

$(DISTNAME).tar.gz: $(SRCS) Makefile LICENSE README.md
	git archive HEAD --prefix=$(DISTNAME)/ | gzip > $@

include/bf_version.h: Makefile
	echo '#define BF_VERSION "$(VERSION)"' > $@

.PHONY: all clean dist test full-test watch
