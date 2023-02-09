/* Functions that help setting up AXIDMA transfers.
   Copyright (C) 2023  Timo Grautstueck
    
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>. */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>             // Memory setting and copying
#include "libaxidma.h"          // Interface of the AXI DMA library

int const VIB_BUFF_MAX = 330 * sizeof(int);

struct dma_transfer {
  int tx_channel;
  int rx_channel;
  size_t tx_buff_size;
  size_t rx_buff_size;
  char *tx_buffer;
  char *rx_buffer;
};
  
void print_buffer(int *buffer, int size) {
  
  for (int i = 0; i < size / sizeof(int); i++)
    printf("%d\t", buffer[i]);
  
  printf("\n");
}

int sum_buffer(int *buffer, int start, int end) {
  int ret = 0;
  int i = (int) (start / sizeof(int));
    
  for(; i < end / sizeof(int); i++)
    ret += buffer[i];

  return ret;
}

void calc_output(int *buffer, int size) {
  int j = (int) (size / VIB_BUFF_MAX);
  int r = (int) (size % VIB_BUFF_MAX);
  
  printf("Calculated Output =>\t");
  for(int i = 0; i<j; i++) {
    printf("%d\t", sum_buffer(buffer, VIB_BUFF_MAX*i, (i+1) * VIB_BUFF_MAX));
  }
  if(r)
    printf("%d\n", sum_buffer(buffer, VIB_BUFF_MAX*j, VIB_BUFF_MAX*j+r));
  else
    printf("\n");
}

int transfer_buffer_oneway(axidma_dev_t axidma_dev, struct dma_transfer *dma_t)
{
  printf("INFO: Trying to transfer the buffer into the PL\n");
  int rc = axidma_oneway_transfer(axidma_dev, dma_t->tx_channel, dma_t->tx_buffer,
				  dma_t->tx_buff_size, true);
  if(rc < 0) {
    perror("Unable to perform the DMA-Transfer!\n");
    
    return rc;
  } else printf("INFO: Performed DMA-Transfer into the PL.\n");

  return rc;
}

int transfer_buffer_twoway(axidma_dev_t axidma_dev, struct dma_transfer *dma_t)
{
  printf("INFO: Trying to transfer the buffer into the PL and back.\n");
  int rc = axidma_twoway_transfer(axidma_dev, dma_t->tx_channel, dma_t->tx_buffer,
				  dma_t->tx_buff_size, NULL, dma_t->rx_channel,
				  dma_t->rx_buffer, dma_t->rx_buff_size, NULL, true);
  if(rc < 0) {
    perror("Unable to perform the DMA-Transfer!\n");
    
    return rc;
  } else printf("INFO: Performed DMA-Transfer into the PL.\n");

  return rc;
}

char* extract_val(char *pattern, char **payload)
{
  const char *pattern_end = ",";
  char *target = NULL;
  char *start;
  
  if (start = strstr(*payload, pattern))
    {
      start += strlen(pattern);
      if (*payload = strstr(start, pattern_end))
	{
	  target = (char *)malloc(*payload - start + 1);
	  memcpy(target, start, *payload - start);
	} else printf("ERROR: pattern could not be found!\n");
      return target;
    }
}
void fill_tx_buffer(char *tx_buffer, size_t tx_buff_size, char *payload)
{
  size_t cnt = 0;
  int *trans_buffer = (int *)tx_buffer;
  
  while(cnt < tx_buff_size / sizeof(int)) {
    trans_buffer[cnt++] = atoi(extract_val("x\":", &payload));
    trans_buffer[cnt++] = atoi(extract_val("y\":", &payload));
    trans_buffer[cnt++] = atoi(extract_val("z\":", &payload));
  }
  
  //printf("Transmitbuffer:\n");
  //print_buffer((int *)tx_buffer, tx_buff_size);
}

//TODO: Pass struct instead of buffer
void fill_buffers(char *tx_buffer, size_t tx_buff_size,
		 char *rx_buffer, size_t rx_buff_size,
		 char *payload)
{
  size_t cnt = 0;
  
  int *trans_buffer = (int *)tx_buffer;
  int *receive_buffer = (int *)rx_buffer;
  
  while(cnt < tx_buff_size / sizeof(int)) {
    trans_buffer[cnt++] = atoi(extract_val("x\":", &payload));
    trans_buffer[cnt++] = atoi(extract_val("y\":", &payload));
    trans_buffer[cnt++] = atoi(extract_val("z\":", &payload));
  }
  
  cnt = 0; 
  while(cnt < rx_buff_size / sizeof(int)) {
    receive_buffer[cnt++] = cnt;
  }

  printf("Transmitbuffer ");
  print_buffer((int *)tx_buffer, tx_buff_size);
  printf("Receivebuffer ");
  print_buffer((int *)rx_buffer, rx_buff_size);
}

int init_axidma_trans_oneway(axidma_dev_t axidma_dev, struct dma_transfer *dma_t,
			     const array_t *tx_chan) 
{
  dma_t->tx_buff_size = VIB_BUFF_MAX;
  
  // TODO: 
  /*axidma_dev = axidma_init();
  if(axidma_dev == NULL) {
    fprintf(stderr, "Failed to initialize the AXI DMA device!\n");
    
    return -1;
  } else printf("INFO: Initialized AXI DMA device.\n");
  */
  tx_chan = axidma_get_dma_tx(axidma_dev);
  if(tx_chan->len < 1) {
    perror("No transmit channels were found.\n");
    axidma_destroy(axidma_dev);
    
    return -1;
  } else printf("INFO: Set tx channel.\n");

  if(dma_t->tx_channel != tx_chan->data[0])
    dma_t->tx_channel = tx_chan->data[0];

  dma_t->tx_buffer = axidma_malloc(axidma_dev, dma_t->tx_buff_size);
  if(dma_t->tx_buffer == NULL) {
    perror("Unable to allocate transmit buffer from the AXI DMA device.");
    axidma_destroy(axidma_dev);
    
    return -1;
  } else printf("INFO: Allocated memory for tx_buffer.\n");

  return 0;
}

int init_axidma_trans_twoway(axidma_dev_t axidma_dev, struct dma_transfer *dma_t, const array_t *tx_chan, const array_t *rx_chan) 
{
  dma_t->tx_buff_size = VIB_BUFF_MAX;
  dma_t->rx_buff_size = VIB_BUFF_MAX;
  
  // TODO: 
  /*axidma_dev = axidma_init();
  if(axidma_dev == NULL) {
    fprintf(stderr, "Failed to initialize the AXI DMA device!\n");
    
    return -1;
  } else printf("INFO: Initialized AXI DMA device.\n");
  */
  tx_chan = axidma_get_dma_tx(axidma_dev);
  if(tx_chan->len < 1) {
    perror("No transmit channels were found.\n");
    axidma_destroy(axidma_dev);
    
    return -1;
  } else printf("INFO: Set tx channel.\n");

  rx_chan = axidma_get_dma_rx(axidma_dev);
  if (rx_chan->len < 1) {
    perror("No receive channels were found.\n");
    
    return -1;
  } else printf("INFO: Set rx channel.\n");

  if(dma_t->tx_channel == -1 && dma_t->rx_channel == -1) {
    dma_t->tx_channel = tx_chan->data[0];
    dma_t->rx_channel = rx_chan->data[0];
  }
  
  dma_t->tx_buffer = axidma_malloc(axidma_dev, dma_t->tx_buff_size);
  if(dma_t->tx_buffer == NULL) {
    perror("Unable to allocate transmit buffer from the AXI DMA device.");
    axidma_destroy(axidma_dev);
    
    return -1;
  } else printf("INFO: Allocated memory for tx_buffer.\n");

  dma_t->rx_buffer = axidma_malloc(axidma_dev, dma_t->rx_buff_size);
  if(dma_t->rx_buffer == NULL) {
    perror("Unable to allocate recieve buffer from the AXI DMA device.");
    axidma_destroy(axidma_dev);
    
    return -1;
  } else printf("INFO: Allocated memory for rx_buffer.\n");

  
  return 0;
}
