#include "usart.h"
#include "timer.h"
#include "esp8266.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "util.h"

#define ESP8266_LONG_TIMEOUT 500
#define ESP8266_SHORT_TIMEOUT 50

int cipmux;


unsigned char ESP8266_sendCommand(char* command, int timeout);

unsigned char ESP8266_init(void) {
  TIM_delay(44000);
  // reset device for safety
  USART_PutString(ESP8266_USART,"AT+RST\r\n");
  ESP8266_waitForReady(ESP8266_LONG_TIMEOUT);
  // make sure that we are a client, may want to make mesh network
  // later on though
  ESP8266_sendCommand("AT+CWMODE=1\r\n", ESP8266_SHORT_TIMEOUT);
  if(ESP8266_test()) {
    ESP8266_sendCommand("ATE0\r\n",ESP8266_SHORT_TIMEOUT); // disable echo back

    return 1;
  }
  cipmux = 0;
  return 0;
}

// test the
unsigned char ESP8266_test(void) {
  return ESP8266_sendCommand("AT\r\n",ESP8266_SHORT_TIMEOUT);
}

unsigned char ESP8266_connect(char* ssid, char* pass) {
  USART_PutString(ESP8266_USART,"AT+CWJAP=\"");
  USART_PutString(ESP8266_USART,ssid);
  USART_PutString(ESP8266_USART,"\",\"");
  USART_PutString(ESP8266_USART,pass);
  return ESP8266_sendCommand("\"\r\n",ESP8266_LONG_TIMEOUT);
}

unsigned char ESP8266_isConnected(void) {
  return ESP8266_sendCommand("AT+CWJAP?\r\n",ESP8266_SHORT_TIMEOUT);
}

unsigned char ESP8266_setCIPMUX(int val) {
  USART_PutString(ESP8266_USART,"AT+CIPMUX=");
  if(val) {
    USART_PutChar(ESP8266_USART,'1');
    cipmux = 1;
  } else {
    USART_PutChar(ESP8266_USART,'0');
    cipmux = 0;
  }
  return ESP8266_sendCommand("\"\r\n",ESP8266_SHORT_TIMEOUT);
}

unsigned char ESP8266_setupServer(char multi, unsigned int port) {
  char buf[6];

  USART_PutString(ESP8266_USART,"AT+CIPSERVER=");
  if(multi) {
    USART_PutChar(ESP8266_USART,'1');
  } else {
    USART_PutChar(ESP8266_USART,'0');
  }
  USART_PutChar(ESP8266_USART,',');
  itoa(buf,port,10);
  USART_PutString(ESP8266_USART,buf);
  return ESP8266_sendCommand("\"\r\n",ESP8266_SHORT_TIMEOUT);
}

unsigned char ESP8266_sendServerData(char channel, char* data, int length) {
  char buf[32];
  USART_PutString(ESP8266_USART,"AT+CIPSEND=");
  USART_PutChar(ESP8266_USART,channel);
  USART_PutChar(ESP8266_USART,',');
  // assume only single connection mode
  itoa(buf, length,10);
  USART_PutString(ESP8266_USART,buf);
  USART_PutString(ESP8266_USART,"\r\n");
  if(!ESP8266_waitForPacketStart(ESP8266_LONG_TIMEOUT)) {
    return 0;
  }

  USART_PutString(ESP8266_USART,data);
  USART_PutString(ESP8266_USART,"AT+CIPCLOSE=");
  USART_PutChar(ESP8266_USART,channel);
  ESP8266_sendCommand("\r\n", ESP8266_SHORT_TIMEOUT);

  return 1;
}

unsigned char ESP8266_sendPacket(char* type, char* ip, char* port,
                                 char* data, int length) {
  char buf[6];
  int i;
  // start session
  USART_PutString(ESP8266_USART,"AT+CIPSTART=");
  if(cipmux) {
    USART_PutString(ESP8266_USART,"4,"); // id is not used anywhere else.
  }
  USART_PutChar(ESP8266_USART,'\"');
  USART_PutString(ESP8266_USART,type);
  USART_PutString(ESP8266_USART,"\",\"");
  USART_PutString(ESP8266_USART,ip);
  USART_PutString(ESP8266_USART,"\",");
  USART_PutString(ESP8266_USART,port);
  if(!ESP8266_sendCommand("\r\n",ESP8266_SHORT_TIMEOUT)) {
    // module timed out, send close just in case
    ESP8266_sendCommand("AT+CIPCLOSE\r\n", ESP8266_SHORT_TIMEOUT);
    return -1;
  }

  // assume that the data is not null terminated,
  itoa(buf,length,10);
  USART_PutString(ESP8266_USART,"AT+CIPSEND=");
  if(cipmux) {
    USART_PutString(ESP8266_USART,"4,"); // id is not used anywhere else.
  }
  USART_PutString(ESP8266_USART,buf);
  USART_PutString(ESP8266_USART,"\r\n");
  if(!ESP8266_waitForPacketStart(ESP8266_LONG_TIMEOUT)) {
    ESP8266_sendCommand("AT+CIPCLOSE\r\n", ESP8266_SHORT_TIMEOUT);
    return -2;
  }
  for(i = 0; i < length; i++) {
    USART_PutChar(ESP8266_USART,data[i]);
  }
  if(!ESP8266_sendCommand("\r\n",ESP8266_LONG_TIMEOUT)) {
    if(cipmux) {
      ESP8266_sendCommand("AT+CIPCLOSE=4\r\n", ESP8266_SHORT_TIMEOUT);
    } else {
      ESP8266_sendCommand("AT+CIPCLOSE\r\n", ESP8266_SHORT_TIMEOUT);
    }

    return -3;
  }
  if(cipmux) {
    return ESP8266_sendCommand("AT+CIPCLOSE=4\r\n", ESP8266_SHORT_TIMEOUT);
  } 
  return ESP8266_sendCommand("AT+CIPCLOSE\r\n", ESP8266_SHORT_TIMEOUT);
  
}


// timeout in ms
unsigned char ESP8266_sendCommand(char* command, int timeout) {
  USART_PutString(ESP8266_USART,command);

  // start checking for timeout
  // this may have to be a bit smarter in the end
  return ESP8266_waitForOK(timeout);

}

unsigned char ESP8266_waitForOK(int timeout) {
  return USART_waitForString(ESP8266_USART, "OK", timeout*sysTicksPerMillisecond);
}


unsigned char ESP8266_waitForReady(int timeout) {
  return USART_waitForString(ESP8266_USART, "ready", timeout*sysTicksPerMillisecond);
}

unsigned char ESP8266_waitForPacketStart(int timeout) {
  return USART_waitForString(ESP8266_USART, ">", timeout*sysTicksPerMillisecond);
}
