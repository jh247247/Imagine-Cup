#ifndef ESP8266_H
#define ESP8266_H

unsigned char ESP8266_init(void);
unsigned char ESP8266_test(void);
unsigned char ESP8266_connect(char* ssid, char* pass);
unsigned char ESP8266_isConnected(void);
unsigned char ESP8266_sendPacket(char* type, char* ip, char* port,
                                 char* data, int length);

unsigned char ESP8266_waitForReady(int timeout);
unsigned char ESP8266_waitForOK(int timeout);
unsigned char ESP8266_waitForPacketStart(int timeout);

unsigned char ESP8266_setCIPMUX(int val);
unsigned char ESP8266_setupServer(char en, unsigned int port);
unsigned char ESP8266_sendServerData(char* data, int length);

char *itoa(char *a,int i, int r);
#endif /* ESP8266_H */
