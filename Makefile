PS5_HOST ?= 192.168.0.135
PS5_PORT ?= 9021

ifdef PS5_PAYLOAD_SDK
    include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
else
    $(error PS5_PAYLOAD_SDK is undefined)
endif

CFLAGS := -Wall -Werror -g

all: ddd_status.elf

ddd_status.elf: main.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f ddd_status.elf

test: ddd_status.elf
	$(PS5_DEPLOY) -h $(PS5_HOST) -p $(PS5_PORT) $^

