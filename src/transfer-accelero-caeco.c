/* Application to transfer Accelerometerdata via AXIDMA to the CAECO-IP.
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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include "libaxidma.h"          // Interface ot the AXI DMA library
#include <mqtt.h>
#include <signal.h>
#include "mqtt-c/templates/posix_sockets.h"

#define CAECO_SIGNAL 23

static char *tx_buf;
static axidma_dev_t axidma_dev;
const size_t tx_size = 330 * sizeof(int);
static int tx_channel = 0, cnt = 0;;
static bool caeco_rdy = true, run = true;

void fill_tx_buffer(char *tx_buffer, size_t tx_buff_size, char *payload);
void print_buffer(int *buffer, int size);
/**
 * @brief The function will be called whenever a PUBLISH message is received.
 */
void publish_callback(void** unused, struct mqtt_response_publish *published);
/**
 * @brief The client's refresher. This function triggers back-end routines to
 *        handle ingress/egress traffic to the broker.
 *
 * @note All this function needs to do is call \ref __mqtt_recv and
 *       \ref __mqtt_send every so often. I've picked 100 ms meaning that
 *       client ingress/egress traffic will be handled every 100 ms.
 */
void* client_refresher(void* client);
/**
 * @brief Safelty closes the \p sockfd and cancels the \p client_daemon before \c exit.
 */
void exit_example(int status, int sockfd, pthread_t *client_daemon);
void signal_keyboard(int signal);
void signal_caeco(int signal);

int main(int argc, const char *argv[])
{
  int rc = 0;
  int fd;
      
  /* Register signal handler */
  signal(CAECO_SIGNAL, signal_caeco);
  signal(SIGINT, signal_keyboard);
  printf("Userspace PID: %d\n", getpid());
    	
  /* Open the dev-file from caeco, to register the PID of the task in Kernelspace */
  fd = open("/dev/caeco", O_RDONLY);
  if(fd < 0) {
    printf("Could not open device file\n");
    return -1;
  }

  /** =====AXI-DMA===== */
  const array_t *tx_chans;
  axidma_dev = axidma_init();
      
  if(axidma_dev == NULL) {
    fprintf(stderr, "Failed to initialize the AXI DMA device!\n");
        
    return -1;
  } else printf("INFO: Initialized AXI DMA device.\n");

  /* Map memory regions for the transmit buffer */
  tx_buf = axidma_malloc(axidma_dev, tx_size);
  if (tx_buf == NULL) {
    perror("Unable to allocate transmit buffer from the AXI DMA device.");
    rc = -1;
    goto destroy_axidma;
  }
      
  tx_chans = axidma_get_dma_tx(axidma_dev);
  if (tx_chans->len < 1) {
    fprintf(stderr, "Error: No transmit channels were found.\n");
    rc = -1;
    goto free_tx_buf;
  }
      
  if (tx_channel == -1) {
    tx_channel = tx_chans->data[0];
  }
  printf("Using transmit channel %d\n", tx_channel);
      
  /** =====MQTT===== */
  /* This can be changed */
  const char* addr = "10.3.2.209";
  const char* port = "1883";
  const char* topic = "things/AC-67-B2-63-DD-58/properties/vibration";
  const char* user = NULL;
  const char* password = NULL;
      
  /* open the non-blocking TCP socket (connecting to the broker) */
  int sockfd = open_nb_socket(addr, port);

  if (sockfd == -1) {
    perror("Failed to open socket: ");
    exit_example(EXIT_FAILURE, sockfd, NULL);
  }

  /* setup a client */
  struct mqtt_client client;
  unsigned char sendbuf[1024];
  unsigned char recvbuf[4096];

  /* Initialize MQTT-Client */
  mqtt_init(&client, sockfd, sendbuf, sizeof(sendbuf),
    	    recvbuf, sizeof(recvbuf), publish_callback);
      
  /* Create an anonymous session */
  const char* client_id = NULL;
  /* Ensure we have a clean session */
  uint8_t connect_flags = MQTT_CONNECT_CLEAN_SESSION;
  /* Send connection request to the broker. */
  mqtt_connect(&client, client_id, NULL, NULL, 0,
    	       user, password, connect_flags, 400);

  /* check that we don't have any errors */
  if (client.error != MQTT_OK) {
    fprintf(stderr, "error: %s\n", mqtt_error_str(client.error));
    exit_example(EXIT_FAILURE, sockfd, NULL);
  }

  /* start a thread to refresh the client (handle egress and ingree client traffic) */
  pthread_t client_daemon;
  if(pthread_create(&client_daemon, NULL, client_refresher, &client)) {
    fprintf(stderr, "Failed to start client daemon.\n");
    exit_example(EXIT_FAILURE, sockfd, NULL);
  }

  /* subscribe */
  mqtt_subscribe(&client, topic, 0);
      
  /* block */
  while(run);

  /* exiting */
  printf("\nDisconnecting from %s\n", addr);
  sleep(1);

  /* close socket and cancle mqtt client thread */
  exit_example(EXIT_SUCCESS, sockfd, &client_daemon);
  /* close the fd to trigger release on the driver side 
     to unregister the current task */
  close(fd);
      
 free_tx_buf:
  axidma_free(axidma_dev, tx_buf, tx_size);
 destroy_axidma:
  axidma_destroy(axidma_dev);
 ret:
  return rc;

}

void exit_example(int status, int sockfd, pthread_t *client_daemon)
{
  printf("Amount of Transfers: %d\n", cnt);
  if (sockfd != -1)
    close(sockfd);
  if (client_daemon != NULL)
    pthread_cancel(*client_daemon);
  exit(status);
}


void publish_callback(void** unused, struct mqtt_response_publish *published)
{
  /* Fill the buffer asynchronously */
  fill_tx_buffer(tx_buf, tx_size, (char *) published->application_message);
  /*printf("Received: %s\n", (const char*) published->application_message);
    printf("\n Buffer:");
    print_buffer((int *)tx_buf, tx_size);*/
  if(caeco_rdy) {
    printf("INFO: Transfering buffer\n");
    cnt++;
    if(axidma_oneway_transfer(axidma_dev, tx_channel, tx_buf, tx_size, true))
      printf("INFO: Something went wrong could not transfer the buffer");
    caeco_rdy = false;
  }
}

void* client_refresher(void* client)
{
  while(1)
    {
      mqtt_sync((struct mqtt_client*) client);
      usleep(100000U);
    }
  return NULL;
}

void signal_caeco(int signal)
{
  printf("Signal from caeco\n.");
  caeco_rdy = true;
}

void signal_keyboard(int signal)
{
  printf("Exiting...\n");
  run = false;
}
