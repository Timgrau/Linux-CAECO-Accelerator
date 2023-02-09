CC = $(CROSS_COMPILE)gcc
all:
	cd src/ && $(MAKE) CC="$(CC)"
