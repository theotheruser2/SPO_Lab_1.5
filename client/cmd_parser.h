#ifndef SPO_CMD_PARSER_H
#define SPO_CMD_PARSER_H
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "../util.h"
#include "../mt.h"
#include "../command_api.h"

bool readCmd(char* inputCmd, size_t size);
uint8_t* cmdToProtobuf(struct command* cmd, size_t* protobufLength);
struct message protobufToMsg(const uint8_t *protobuf, size_t protobufLength);

#endif
