#include "usart.h"
#include "timer.h"
#include "esp8266.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "util.h"

unsigned char ESP8266_sendCommand(char* command, int timeout);

// everything is now handled on the chip, so not much to do...
unsigned char ESP8266_init(void) {
  return USART_waitForString(ESP8266_USART, "CONN", 0);
}




