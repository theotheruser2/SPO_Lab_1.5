#include "util.h"

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

// takes array of strings, last element of which is NULL
size_t stringArrayLen(char** arr) {
    if (!arr) return 0;

    int i = 0;
    while (*(arr + i)) {
        i += 1;
    }
    return i;
}

ssize_t readAll(int fd, void* buf, size_t size) {
    size_t remaining = size;
    uint8_t* ptr = buf;

    ssize_t ret;
    while ((ret = read(fd, ptr, remaining)) > 0) {
        remaining -= ret;
        ptr += ret;

        if (remaining == 0) {
            return (ssize_t) size;
        }
    }

    return ret;
}

ssize_t writeAll(int fd, const void* buf, size_t size) {
    const uint8_t* ptr = buf;
    size_t remaining = size;

    ssize_t ret;
    while ((ret = write(fd, ptr, remaining)) > 0) {
        remaining -= ret;
        ptr += ret;

        if (remaining == 0) {
            return (ssize_t) size;
        }
    }

    return ret;
}

uint8_t * receiveProtobuf(int socket, size_t* length) {
    if (readAll(socket, length, sizeof(length)) <= 0) return NULL;

    uint8_t* buf = malloc(*length);
    if (readAll(socket, buf, *length) <= 0) {
        free(buf);
        return NULL;
    }

    return buf;
}

bool sendProtobuf(int socket, uint8_t* protobuf, size_t protobufLength) {
    if (writeAll(socket, &protobufLength, sizeof(protobufLength)) <= 0) return false;
    if (writeAll(socket, protobuf, protobufLength) <= 0) return false;
    return true;
}
