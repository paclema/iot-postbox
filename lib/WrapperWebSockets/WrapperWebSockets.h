#ifndef WrapperWebSockets_H
#define WrapperWebSockets_H

#include <Arduino.h>

// WebSockets Includes
#include <WebSocketsServer.h>

#define  MAX_LIST_OBJECT_FUNCTIONS 10

static WebSocketsServer* webSocket ;

class WrapperWebSockets{

// private:
unsigned int numClients = 0;

// List of objects keys:
String listObjets[MAX_LIST_OBJECT_FUNCTIONS];

// List of Object values for the listObjects. It represents the value for the key
// and it will be the String result of the function:
String (*listObjetFunctions[MAX_LIST_OBJECT_FUNCTIONS])();
int listObjetsIndex = 0;

int port = 81;

public:

  void setPort(int port) {this->port = port; };
  void init(void);
  static void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
  void publishClients(void);
  void handle(void);
  bool addObjectToPublish(String key, String (*valueFunction)());

};

#endif
