#include <stdlib.h>


typedef struct buffer_item {
uint8_t data[30];
uint16_t cksum;
} BUFFER_ITEM;

#define BUFFER_SIZE 10


