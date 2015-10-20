CC := gcc
CCFLAGS := -Wall -Wextra -g

SRC := src/linux/linux_ether.c $(wildcard src/*.c)

inetd: $(SRC)
	@echo "CC $@"
	@$(CC) $(CCFLAGS) -iquote include $(SRC) -o $@

clean:
	$(RM) inetd
