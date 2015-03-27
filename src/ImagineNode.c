#include "ImagineNode.h"
#include "util.h"
#include "usart.h"
#include "esp8266.h"
#include "timer.h"

#include <string.h>
#include <stdlib.h>

// cannot get unique id from esp8266, use internal cpu id.
unsigned long *id = (unsigned long*) 0x1FFFF7E8;
// store the server ip addr for sending pings to.
char server_ip_addr[17];
#define SERVER_PORT 50042
#define SERVER_MESSAGE_TYPE "UDP"

#define ESP8266_PACKET_RX "+IPD"
#define CONFIG_STR "setup"

#define PACKET_TIMEOUT 88000
// TODO: add ssid/pass config

char ESP8266_config_page[] = "<form \
method=get><label>SSID</label><br><input  type='text' name='ssid' \
maxlength='30' size='15'><br><label>Password</label><br><input \
type='password' name='password' maxlength='30' \
size='15'><br><br><input  type='submit' value='Connect' ></form>";

char ESP8266_config_header[100] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length:";

void IN_init() {
  TIM_delay(4400);

  // make sure that the ip addr is invalid on startup.
  server_ip_addr[0] = '\0';
  server_ip_addr[16] = '\0';

  // ESP8266 config after init
  USART_PutString(HOST_USART,"### START ###\n");


  ESP8266_connect("linksys","\0");
  if(ESP8266_isConnected()) {
    USART_PutString(HOST_USART,"ESP8266 Connected!\n");
  } else {
    USART_PutString(HOST_USART,"### ESP8266 CONNECT FAIL ###\n");
  }

  if(ESP8266_setCIPMUX(1)) { // NOTE: CIPMUX has to be 1 to have
    // server mode
    USART_PutString(HOST_USART,"ESP8266 set CIPMUX 1\n");
  } else {
    USART_PutString(HOST_USART,"### ESP8266 CIPMUX FAIL ###\n");
  }

  if(ESP8266_setupServer(1, SERVER_PORT)) {
    USART_PutString(HOST_USART,"ESP8266 server started!\n");
  } else {
    USART_PutString(HOST_USART,"### ESP8266 SERVER FAIL ###\n");
  }

  // TODO: add fallback to make access point for config if
  // connecting fails

  // wait for "special" strings to act upon
  USART_setMatch(ESP8266_USART, ESP8266_PACKET_RX); // string to search for in server mode
  USART_setMatch(HOST_USART, CONFIG_STR); // string to search for to config i guess
}

void IN_handleServer() {
  char message[32];

  // check if we are pinged by the server
  if(USART_checkMatch(ESP8266_USART) == 0) {
    USART_PutString(HOST_USART,"Packet received!\n");

    // packet formatting...
    // byte 11 is start of packet data
    // wait until message comes through
    char* packetStart = strstr((char*)g_usart_rx[1], ESP8266_PACKET_RX);
    if(packetStart == NULL) {
      //USART_PutString(HOST_USART,"String not found!");
      USART_resetMatch(ESP8266_USART);
      USART_resetRXBuffer(ESP8266_USART);
      return; // why isn't it in the rx buffer...
    }

    // wait for data to come in...
    TIM_initTimeout(PACKET_TIMEOUT);
    while(strstr(packetStart, "\0") == NULL && !TIM_checkTimeout());
    if(TIM_checkTimeout()) {
      USART_PutString(HOST_USART,"Timeout!");
      USART_resetMatch(ESP8266_USART);
      USART_resetRXBuffer(ESP8266_USART);
      return;
    }

    // data starts after the colon
    switch(*(strstr(packetStart,":")+1)) {
    case '0':
      // next 16 bytes (max) should by IP addr of server saved as string.
      USART_PutChar(HOST_USART,'\n');
      USART_PutString(HOST_USART,(strstr(packetStart,":")+2));
      USART_PutChar(HOST_USART,'\n');
      strcpy(server_ip_addr, (strstr(packetStart,":")+2));

      USART_PutString(HOST_USART,"IP received:");
      USART_PutString(HOST_USART,server_ip_addr);
      USART_PutChar(HOST_USART,'\n');
      USART_PutString(HOST_USART,"Sending id to server...\n");
      message[0] = '0';
      message[13] = '\0';
      memcpy(&(message[1]), (void*) 0x1FFFF7E8, 12); // 12 byte id copy
      // char 5 is the channel id
      //ESP8266_sendServerData(*(packetStart+5), message, 13);
      // we already have the ip, so no need to actually reply to the
      // request. force the proper port.
      IN_sendToServer(message, 14);

      USART_resetMatch(ESP8266_USART);
      USART_resetRXBuffer(ESP8266_USART);

      break;
      // using switch case to make responding to server messages easier
    default:
      break;
    }


    // host sending config
    if(USART_checkMatch(HOST_USART) == 0) {
      /* TODO:  actually implement this...*/
    }
    USART_resetMatch(ESP8266_USART);
    USART_resetMatch(HOST_USART);
  }
}

int IN_sendToServer(char* message, int len) {
  if(server_ip_addr[0] == '\0') {
    // can't really do much if we don't know where the server lives
    return 1;
  }

  char buf[32];
  itoa(buf, SERVER_PORT, 10);

  return ESP8266_sendPacket(SERVER_MESSAGE_TYPE,
                            server_ip_addr,
                            buf,
                            message, len);
}
