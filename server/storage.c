#include <unistd.h>
#include <stdint.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>

#include "storage.h"
#include "../util.h"

const uint64_t nullAddr = 0;

struct storage* storageInit(int fd) {
    lseek(fd, 0, SEEK_SET);

    write(fd, SIGNATURE, 4);

    uint64_t p = 0;
    write(fd, &p, sizeof(p));

    struct storage* storage = malloc(sizeof(*storage));

    storage->fd = fd;
    storage->root = 0;
    return storage;
}

struct storage* storageOpen(int fd) {
    lseek(fd, 0, SEEK_SET);

    char sign[4];
    if (read(fd, sign, 4) != 4) {
        errno = EINVAL;
        return NULL;
    }

    if (memcmp(sign, SIGNATURE, 4) != 0) {
        errno = EINVAL;
        return NULL;
    }

    struct storage* storage = malloc(sizeof(*storage));

    storage->fd = fd;
    read(fd, &storage->root, sizeof(storage->root));
    return storage;
}

static uint64_t storageWrite(int fd, void* buf, size_t length) {
    uint64_t offset = lseek(fd, 0, SEEK_END);

    write(fd, buf, length);
    return offset;
}

uint64_t storageWriteString(int fd, const char* str) {
    uint16_t length = strlen(str);

    uint64_t offset = storageWrite(fd, &length, sizeof(length));
    write(fd, str, length);
    return offset;
}

struct storage* storageInitRoot(int fd, struct storage* storage, struct node* rootNode) {
    uint64_t nameAddr = storageWriteString(fd, rootNode->name);
    uint64_t valueAddr;
    if (rootNode->value != NULL) {
        valueAddr = storageWriteString(fd, rootNode->value);
    }
    else {
        valueAddr = 0;
    }

    uint64_t offset = storageWrite(fd, &nameAddr, sizeof(nameAddr));
    storageWrite(fd, &rootNode->next, sizeof(rootNode->next));
    storageWrite(fd, &rootNode->child, sizeof(rootNode->child));
    storageWrite(fd, &valueAddr, sizeof(valueAddr));

    storage->root = offset;
    lseek(fd, 4, SEEK_SET);
    write(fd, &offset, sizeof(offset));

    return storage;
}

// lseek must be called beforehand
static char* storageReadString(int fd) {
    uint16_t current_pos = lseek(fd, 0, SEEK_CUR);

    if (current_pos == 0)
        return NULL;

    uint16_t length;
    read(fd, &length, sizeof(length));

    char* str = malloc(sizeof(int8_t) * (length + 1));
    read(fd, str, length);
    str[length] = '\0';

    return str;
}

// returns NULL if didn't find a node
struct node* storageFindNode(struct storage* storage, char** path, size_t pathLen) {
    uint64_t nodeAddr = storage->root;

    size_t pathIndex = 0;
    while (nodeAddr) {
        lseek(storage->fd, (__off_t) nodeAddr, SEEK_SET);

        uint64_t nodeNameAddr, nodeNextAddr, nodeChildAddr;
        read(storage->fd, &nodeNameAddr, sizeof(nodeNameAddr));
        read(storage->fd, &nodeNextAddr, sizeof(nodeNextAddr));
        read(storage->fd, &nodeChildAddr, sizeof(nodeChildAddr));

        lseek(storage->fd, (__off_t) nodeNameAddr, SEEK_SET);
        char* nodeName = storageReadString(storage->fd);
        if (nodeName == NULL) {
            nodeAddr = nodeNextAddr;
            continue;
        }
        if (strcmp(nodeName, path[pathIndex]) != 0) {
            free(nodeName);
            nodeAddr = nodeNextAddr;
            continue;
        }

        // if node is found
        if (pathIndex == pathLen - 1) {
            uint64_t nodeValueAddr;
            lseek(storage->fd, (__off_t) (nodeAddr + sizeof(uint64_t) * VALUE_OFFSET), SEEK_SET);
            read(storage->fd, &nodeValueAddr, sizeof(nodeValueAddr));

            lseek(storage->fd, (__off_t) nodeValueAddr, SEEK_SET);
            char* nodeValue = storageReadString(storage->fd);

            struct node* nodePtr = malloc(sizeof(*nodePtr));
            nodePtr->addr = nodeAddr;
            nodePtr->name = nodeName;
            nodePtr->next = nodeNextAddr;
            nodePtr->child = nodeChildAddr;
            nodePtr->value = nodeValue;

            return nodePtr;
        }

        // else go down
        pathIndex += 1;
        nodeAddr = nodeChildAddr;
        free(nodeName);
    }

    return NULL;
}

uint64_t storageFindChildren(struct storage* storage, struct node* parentNode, char* childrenName) {
    uint64_t nodeAddr = parentNode->child;

    while (nodeAddr) {
        lseek(storage->fd, (__off_t) nodeAddr, SEEK_SET);

        uint64_t nodeNameAddr, nodeNextAddr;
        read(storage->fd, &nodeNameAddr, sizeof(nodeNameAddr));
        read(storage->fd, &nodeNextAddr, sizeof(nodeNextAddr));

        lseek(storage->fd, (__off_t) nodeNameAddr, SEEK_SET);
        char* nodeName = storageReadString(storage->fd);
        if (nodeName == NULL) {
            nodeAddr = nodeNextAddr;
            continue;
        }

        if (strcmp(nodeName, childrenName) != 0) {
            free(nodeName);
            nodeAddr = nodeNextAddr;
            continue;
        }

        free(nodeName);
        return nodeAddr;
    }
    return 0;
}

void storageFreeNode(struct node* node) {
    free(node->name);
    free(node->value);
    free(node);
}

char** storageGetAllChildrenName(struct storage* storage, struct node* parentNode) {
    char** nameArr = malloc(sizeof(char*) * NAME_ARR_BLOCK_SIZE);
    size_t i = 0;
    size_t nameArrSize = NAME_ARR_BLOCK_SIZE;

    uint64_t currentNodeAddr;
    uint64_t nextNodeAddr = parentNode->child;
    while (nextNodeAddr) {
        currentNodeAddr = nextNodeAddr;
        lseek(storage->fd, (__off_t) (nextNodeAddr + sizeof(uint64_t) * NEXT_OFFSET), SEEK_SET);
        read(storage->fd, &nextNodeAddr, sizeof(uint64_t));

        uint64_t nameAddr;
        lseek(storage->fd, (__off_t) (currentNodeAddr + sizeof(uint64_t) * NAME_OFFSET), SEEK_SET);
        read(storage->fd, &nameAddr, sizeof(nameAddr));

        lseek(storage->fd, (__off_t) nameAddr, SEEK_SET);
        nameArr[i] = storageReadString(storage->fd);
        i++;

        if (i == nameArrSize) {
            nameArrSize += NAME_ARR_BLOCK_SIZE;
            nameArr = realloc(nameArr, sizeof(char*) * nameArrSize);
        }
    }

    nameArr[i] = 0;
    return nameArr;
}

void storageCreateNode(struct storage* storage, struct node* parentNode, struct node* newNode) {
    uint64_t nameAddr = storageWriteString(storage->fd, newNode->name);

    uint64_t valueAddr;
    if (newNode->value != NULL) {
        valueAddr = storageWriteString(storage->fd, newNode->value);
    }
    else {
        valueAddr = 0;
    }

    uint64_t offset = storageWrite(storage->fd, &nameAddr, sizeof(nameAddr));
    newNode->addr = offset;

    storageWrite(storage->fd, &newNode->next, sizeof(newNode->next));
    storageWrite(storage->fd, &newNode->child, sizeof(newNode->child));
    storageWrite(storage->fd, &valueAddr, sizeof(valueAddr));

    // найти последнюю ноду на этом уровне и добавить ей next = offset
    // если на этом уровне еще нет нод - добавить родителю child = offset

    uint64_t currentNodeAddr = 0;
    uint64_t nextNodeAddr = parentNode->child;
    while (nextNodeAddr) {
        currentNodeAddr = nextNodeAddr;
        lseek(storage->fd, (__off_t) (nextNodeAddr + sizeof(uint64_t) * NEXT_OFFSET), SEEK_SET);
        read(storage->fd, &nextNodeAddr, sizeof(uint64_t));
    }

    if (currentNodeAddr == 0) {
        lseek(storage->fd, (__off_t) (parentNode->addr + sizeof(uint64_t) * CHILD_OFFSET), SEEK_SET);
        write(storage->fd, &offset, sizeof(uint64_t));
    }
    else {
        lseek(storage->fd, (__off_t) (currentNodeAddr + sizeof(uint64_t) * NEXT_OFFSET), SEEK_SET);
        write(storage->fd, &offset, sizeof(uint64_t));
    }
}

void storageUpdateNode(struct storage* storage, struct node* node, char* newValue){
    uint64_t oldValueAddr;
    lseek(storage->fd, (__off_t) (node->addr + sizeof(uint64_t) * VALUE_OFFSET), SEEK_SET);
    read(storage->fd, &oldValueAddr, sizeof(oldValueAddr));

    if (oldValueAddr != 0) {
        uint16_t oldValueLen;
        lseek(storage->fd, (__off_t) oldValueAddr, SEEK_SET);
        read(storage->fd, &oldValueLen, sizeof(oldValueLen));

        uint16_t newValueLen = (uint16_t) strlen(newValue);
        if (newValueLen <= oldValueLen) {
            lseek(storage->fd, (__off_t) oldValueAddr, SEEK_SET);
            write(storage->fd, &newValueLen, sizeof(newValueLen));
            lseek(storage->fd, (__off_t) (oldValueAddr + sizeof(newValueLen)), SEEK_SET);
            write(storage->fd, newValue, sizeof(char) * newValueLen);
            return;
        }
    }

    uint64_t newValueAddr = storageWriteString(storage->fd, newValue);
    lseek(storage->fd, (__off_t) (node->addr + sizeof(uint64_t) * VALUE_OFFSET), SEEK_SET);
    write(storage->fd, &newValueAddr, sizeof(newValueAddr));
}

void storageDeleteNode(struct storage* storage, struct node* parentNode, uint64_t childrenAddr, bool isDelValue) {
    if (isDelValue) {
        lseek(storage->fd, (__off_t) (childrenAddr + sizeof(uint64_t) * VALUE_OFFSET), SEEK_SET);
        write(storage->fd, &nullAddr, sizeof(nullAddr));
    } else {
        uint64_t currentNodeAddr = 0;
        uint64_t nextNodeAddr = parentNode->child;
        while (nextNodeAddr != childrenAddr) {
            currentNodeAddr = nextNodeAddr;
            lseek(storage->fd, (__off_t) (nextNodeAddr + sizeof(uint64_t) * NEXT_OFFSET), SEEK_SET);
            read(storage->fd, &nextNodeAddr, sizeof(nextNodeAddr));
        }

        lseek(storage->fd, (__off_t) (childrenAddr + (sizeof(uint64_t) * NEXT_OFFSET)), SEEK_SET);
        read(storage->fd, &nextNodeAddr, sizeof(nextNodeAddr));

        if (currentNodeAddr == 0) {
            lseek(storage->fd, (__off_t) (parentNode->addr + sizeof(uint64_t) * CHILD_OFFSET), SEEK_SET);
        } else {
            lseek(storage->fd, (__off_t) (currentNodeAddr + sizeof(uint64_t) * NEXT_OFFSET), SEEK_SET);
        }

        write(storage->fd, &nextNodeAddr, sizeof(nextNodeAddr));
    }
}
