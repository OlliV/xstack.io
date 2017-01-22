CC := gcc
CCFLAGS := -Wall -Wextra -Wno-unused-parameter -g
CCFLAGS += --std=gnu99 -pthread -MMD -include config.h

# Daemon
DSRC := src/linux/linux_ether.c $(wildcard src/*.c) $(wildcard util/*.c)
# Library
LSRC := $(wildcard lib/*.c) $(wildcard util/*.c)
# Examples
ESRC := $(wildcard examples/*.c)

.SUFFIXES: .c .o

#LOBJ := $($(addprefix build_lib/, $(LSRC)):.c=.o)
LOBJ := $(addprefix build_lib/, $(LSRC:.c=.o))
EBIN := $(ESRC:.c=)

all: inetd libxstack doc

doc:
	doxygen
	cd latex && make

-include inetd.d

inetd: $(DSRC)
	@echo "CC $@"
	@$(CC) $(CCFLAGS) -iquote include $(DSRC) -o $@

libxstack: libxstack.a

build_lib:
	mkdir -p build_lib/lib
	mkdir -p build_lib/util

$(LOBJ): $(LSRC) build_lib
	@echo "CC $@"
	$(eval TMP_NAME = $*.c)
	$(eval SRC_FILE = $(TMP_NAME:build_lib/%=%))
	@$(CC) $(CCFLAGS) -iquote include -c $(SRC_FILE) -o $@

libxstack.a: $(LOBJ)
	@echo "AR $@"
	ar rcs $@ $(LOBJ)

examples: libxstack $(EBIN)

$(EBIN): libxstack.a $(ESRC)
	@echo "CC $@"
	@$(CC) $(CCFLAGS) -iquote include $@.c -L. -lxstack -o $@

clean:
	$(RM) inetd
	$(RM) inetd.d
	$(RM) -r build_lib
	$(RM) libxstack.a
	$(RM) $(EBIN)
	$(RM) $(EBIN:=.d)
	$(RM) -r latex

.PHONY: doc libxstack examples clean
