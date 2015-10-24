CC := gcc
CCFLAGS := -Wall -Wextra -g
CCFLAGS += -include config.h

SRC := src/linux/linux_ether.c $(wildcard src/*.c)

doc:
	doxygen
	cd latex && make

inetd: $(SRC)
	@echo "CC $@"
	@$(CC) $(CCFLAGS) -iquote include $(SRC) -o $@

clean:
	$(RM) inetd
	$(RM) latex

.PHONY: clean doc
