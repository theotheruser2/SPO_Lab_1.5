#pragma once

#include <stdbool.h>
#include <stdint.h>

struct apiCreateParams {
    char* value;
};

struct apiUpdateParams {
    char* value;
};

struct apiDeleteParams {
    bool isDelValue;
};

enum apiAction {
    COMMAND_CREATE = 0,
    COMMAND_READ = 1,
    COMMAND_UPDATE = 2,
    COMMAND_DELETE = 3
};

struct command {
    enum apiAction apiAction;
    char** path;
    union {
        struct apiCreateParams apiCreateParams;
        struct apiUpdateParams apiUpdateParams;
        struct apiDeleteParams apiDeleteParams;
    };
};

struct message {
    int status;
    char* info;
};

void freeStringArray(char** path);

struct command* protobufToStruct(uint8_t* protobuf, size_t protobufLength, char* rootPath);
void destroyCommand(const struct command* command);
void freeCommand(struct command* command);

uint8_t* responseToProtobuf(struct message* response, size_t* protobufLength);
