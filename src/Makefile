# Makefile to build the applications of the repo.
# Copyright (C) 2023  Timo Grautstueck

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

CC=$(CROSS_COMPILE)gcc

AXIDMA_DIR=xilinx_axidma/
AXIDMA_INC=$(AXIDMA_DIR)include/
MQTT_DIR=mqtt-c/
MQTT_INC=$(MQTT_DIR)include/
MQTT_SRC=$(MQTT_DIR)src/*.c

BIN=../bin/

all: test-transfer-caeco transfer-accelero-caeco test-transfer-sum cleanup


axidma_transfer: $(AXIDMA_DIR)library/libaxidma.c
	$(CC) -c $@.c $^ -I$(AXIDMA_INC)

test-transfer-caeco: mkdir axidma_transfer 
	$(CC) $@.c -o $@ -I$(AXIDMA_INC) *.o
	mv $@ $(BIN)

transfer-accelero-caeco: mkdir axidma_transfer
	$(CC) $@.c -o $@ -I$(AXIDMA_INC) -I$(MQTT_INC) $(MQTT_SRC) *.o -lpthread
	mv $@ $(BIN)

test-transfer-sum: mkdir axidma_transfer 
	$(CC) $@.c -o $@ -I$(AXIDMA_INC) *.o
	mv $@ $(BIN)


mkdir:
	if [ ! -d $(BIN) ]; then mkdir $(BIN); fi
link:
	$(CC) *.o
cleanup:
	rm *.o
