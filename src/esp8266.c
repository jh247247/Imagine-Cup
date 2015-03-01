#include "usart.h"
#include "timer.h"
#include "esp8266.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "util.h"

/* TODO: clean this garbage up */


unsigned char ESP8266_sendCommand(char* command, int timeout);

unsigned char ESP8266_init(void) {
  USART2_PutString("AT+RST\r\n");
  ESP8266_waitForReady(5000);
  if(ESP8266_test()) {
    ESP8266_sendCommand("ATE0\r\n",100); // disable echo back

    return 1;
  }

  return 0;
}

// test the
unsigned char ESP8266_test(void) {
  return ESP8266_sendCommand("AT\r\n",500);
}

unsigned char ESP8266_connect(char* ssid, char* pass) {
  USART2_PutString("AT+CWJAP=\"");
  USART2_PutString(ssid);
  USART2_PutString("\",\"");
  USART2_PutString(pass);
  return ESP8266_sendCommand("\"\r\n",20000);
}

unsigned char ESP8266_isConnected(void) {
  return ESP8266_sendCommand("AT+CWJAP?\r\n",500);
}

unsigned char ESP8266_setCIPMUX(int val) {
  USART2_PutString("AT+CIPMUX=");
  if(val) {
    USART2_PutChar('1');
  } else {
    USART2_PutChar('0');
  }
  return ESP8266_sendCommand("\"\r\n",500);
}

unsigned char ESP8266_setupServer(char multi, unsigned int port) {
  char buf[6];

  USART2_PutString("AT+CIPSERVER=");
  if(multi) {
    USART2_PutChar('1');
  } else {
    USART2_PutChar('0');
  }
  USART2_PutChar(',');
  itoa(buf,port,10);
  USART2_PutString(buf);
  return ESP8266_sendCommand("\"\r\n",500);
}

unsigned char ESP8266_sendServerData(char* data, int length) {
  char buf[32];
  USART2_PutString("AT+CIPSEND=");
  // assume only single connection mode
  itoa(buf, length,10);
  USART2_PutString(buf);
  USART2_PutString("\r\n");
  if(!ESP8266_waitForPacketStart(5000)) {
    return 0;
  }
  USART2_PutString(data);
  return 1;
}

unsigned char ESP8266_sendPacket(char* type, char* ip, char* port,
                                 char* data, int length) {
  char buf[6];
  int i;
  // start session
  USART2_PutString("AT+CIPSTART=\"");
  USART2_PutString(type);
  USART2_PutString("\",\"");
  USART2_PutString(ip);
  USART2_PutString("\",");
  USART2_PutString(port);
  if(!ESP8266_sendCommand("\r\n",5000)) {
    // module timed out, send close just in case
    ESP8266_sendCommand("AT+CIPCLOSE\r\n", 0);
    return 0;
  }

  // assume that the data is not null terminated,
  itoa(buf,length,10);
  USART2_PutString("AT+CIPSEND=");
  USART2_PutString(buf);
  USART2_PutString("\r\n");
  if(!ESP8266_waitForPacketStart(5000)) {
    ESP8266_sendCommand("AT+CIPCLOSE\r\n", 0);
    return 0;
  }
  for(i = 0; i < length; i++) {
    USART2_PutChar(data[i]);
  }
  if(!ESP8266_sendCommand("\r\n",5000)) {
    ESP8266_sendCommand("AT+CIPCLOSE\r\n", 0);
    return 0;
  }
  return ESP8266_sendCommand("AT+CIPCLOSE\r\n", 5000);
}



// timeout in ms
unsigned char ESP8266_sendCommand(char* command, int timeout) {
  USART2_PutString(command);

  // start checking for timeout
  // this may have to be a bit smarter in the end
  return ESP8266_waitForOK(timeout);

}




unsigned char ESP8266_waitForOK(int timeout) {
  return USART_waitForString(USART2, "OK", timeout*sysTicksPerMillisecond);
}


unsigned char ESP8266_waitForReady(int timeout) {
  return USART_waitForString(USART2, "ready", timeout*sysTicksPerMillisecond);
}

unsigned char ESP8266_waitForPacketStart(int timeout) {
  return USART_waitForString(USART2, ">", timeout*sysTicksPerMillisecond);
}
