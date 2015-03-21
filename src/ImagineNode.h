#ifndef IMAGINENODE_H
#define IMAGINENODE_H

void IN_init();
void IN_handleServer();
int IN_sendToServer(char* message, int len);

#endif /* IMAGINENODE_H */
