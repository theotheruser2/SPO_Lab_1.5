#pragma once

#include <inttypes.h>
#include <stddef.h>
#include <stdbool.h>

#define NAME_ARR_BLOCK_SIZE 512

/*
 * Storage structure:
 *
 * struct storage:
 *  - fd <int>
 *  - root node <uint64_t>
 *
 * struct node:
 *  - name <uint64_t>
 *  - next <uint64_t>
 *  - child <uint64_t>
 *  - value <uint64_t>
 */

#define NAME_OFFSET 0
#define NEXT_OFFSET 1
#define CHILD_OFFSET 2
#define VALUE_OFFSET 3

#define SIGNATURE ("DEPT")

struct storage {
    int fd;
    uint64_t root;
};

struct node {
    uint64_t addr;
    char* name;
    uint64_t next;
    uint64_t child;
    char* value;
};

struct storage* storageInit(int fd);
struct storage* storageOpen(int fd);
struct storage* storageInitRoot(int fd, struct storage* storage, struct node* rootNode);

struct node* storageFindNode(struct storage* storage, char** path, size_t pathLen);
uint64_t storageFindChildren(struct storage* storage, struct node* parentNode, char* childrenName);

void storageFreeNode(struct node* node);

void storageCreateNode(struct storage* storage, struct node* parentNode, struct node* newNode);
void storageUpdateNode(struct storage* storage, struct node* node, char* newValue);
void storageDeleteNode(struct storage* storage, struct node* node, uint64_t childrenAddr, bool isDelValue);

char** storageGetAllChildrenName(struct storage* storage, struct node* parentNode);
