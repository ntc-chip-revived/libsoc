#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

#include "libsoc_spi.h"
#include "libsoc_debug.h"

/**
 * 
 * This gpio_test is intended to be run on beaglebone white hardware
 * and uses pins P9_42(gpio7) and P9_27 (gpio115) connected together.
 *
 * The GPIO_OUTPUT and INPUT defines can be changed to support any board
 * all you need to do is ensure that the two pins are connected together
 * 
 */
 
#define SPI_DEVICE   1
#define CHIP_SELECT  0

#define WREN  0x06
#define WRDI  0x04
#define WRITE 0x02
#define READ  0x03
#define RDSR  0x05

static uint8_t tx[35], rx[35];

uint8_t read_status_register(spi* spi_dev) {
    
  printf("Reading STATUS register\n");
    
  tx[0] = RDSR;
  tx[1] = 0;
  rx[0] = 0;
  rx[1] = 0;
  
  libsoc_spi_rw(spi_dev, tx, rx, 2);
  
  printf("STATUS is 0x%02x\n", rx[1]);
  
  return rx[1];
}

int write_page(spi* spi_dev, uint16_t page_address, uint8_t* data, int len) {
  
  printf("Writing to page %d\n", page_address);
  
  tx[0] = WRITE;
  
  tx[1] = (page_address >> 8);
  tx[2] = page_address;
  
  if (len > 32) {
    printf("Page size is 32 bytes\n");
    return EXIT_FAILURE;
  }
  
  int i;
  
  for (i=0; i<len; i++) {
  
    tx[(i+3)] = data[i];
  }
   
  libsoc_spi_write(spi_dev, tx, (len+3));
  
  int status;
  
  do {
  
    status = read_status_register(spi_dev);
    
    if (status & 0x01) {
      printf("Write in progress...\n");
    } else {
      printf("Write finished...\n");
    }
    
  } while (status & 0x1);
  
  return EXIT_SUCCESS;
  
}

int read_page(spi* spi_dev, uint16_t address, uint8_t* data, int len) {
  
  tx[0] = READ;
  
  tx[1] = (address >> 8);
  tx[2] = address;
  
  printf("Reading page address %d\n", address);
  
  libsoc_spi_rw(spi_dev, tx, rx, (len+3));
  
  int i;
  
  for (i=0; i<len; i++)
  {
    data[i] = rx[(i+3)];
  }
  
  return EXIT_SUCCESS;
}

int set_write_enable(spi* spi_dev) {
  
  tx[0] = WREN;
  
  printf("Setting write enable bit\n");
   
  libsoc_spi_write(spi_dev, tx, 1);
  
  return EXIT_SUCCESS;
}

int main()
{
  libsoc_set_debug(1);
   
  uint8_t status;
  int i;
   
  spi* spi_dev = libsoc_spi_init(SPI_DEVICE, CHIP_SELECT);

  libsoc_spi_set_mode(spi_dev, MODE_0);
  libsoc_spi_get_mode(spi_dev);
  
  libsoc_spi_set_speed(spi_dev, 1000000);
  libsoc_spi_get_speed(spi_dev);
  
  libsoc_spi_set_bits_per_word(spi_dev, BITS_8);
  libsoc_spi_get_bits_per_word(spi_dev);
  
  status = read_status_register(spi_dev);
  
  set_write_enable(spi_dev);
  
  status = read_status_register(spi_dev);
  
  if (!(status & 0x02)) {
    printf("Write Enable was not set, failure\n");
    goto free;
  }
  
  int len = 32;
  
  uint8_t data[32], data_read[32];
  
  memset(data, 0, len);
  
  struct timeval t1;
  gettimeofday(&t1, NULL);
  srand(t1.tv_usec * t1.tv_sec);
  
  for (i=0; i<len; i++) {
    data[i] = rand() % 255;
  }
  
  write_page(spi_dev, 0, data, len);
  
  read_page(spi_dev, 0, data_read, len);
  
  for (i=0; i<len; i++) {
    printf("data[%d] = 0x%02x : data_read[%d] = 0x%02x", i, data[i], i, data_read[i]);
    
    if (data[i] == data_read[i]) {
      printf(" : Correct\n");
    } else {
      printf(" : Incorrect\n");
    }
  }
  
  free:
  
  libsoc_spi_free(spi_dev);

  return EXIT_SUCCESS;
}
