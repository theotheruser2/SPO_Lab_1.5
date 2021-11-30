#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define printError(x) do { fprintf(stderr, "%s\n", x); exit(1); } while (0)

#define REG_BUFFER_SIZE 4096
#define MAX_MSG_LENGTH (4096 + 128)
#define ROOT_NODE_NAME "root"
// len of string (ROOT_NODE_NAME ".")

size_t stringArrayLen(char** arr);

ssize_t readAll(int fd, void* buf, size_t size);
ssize_t writeAll(int fd, const void* buf, size_t size);

uint8_t * receiveProtobuf(int socket, size_t* length);
bool sendProtobuf(int socket, uint8_t* protobuf, size_t protobufLength);

static inline char* strdupOrNull(char* str) {
    if (!str) return NULL;
    return strdup(str);
}
