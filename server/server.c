#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>

#include "storage.h"
#include "../command_api.h"
#include "../util.h"

static volatile bool closing = false;

static void close_handler(int sig, siginfo_t * info, void * context) {
    closing = true;
}

struct message* handleRequestCreate(struct storage* storage, char** tokenizedPath, size_t pathLen, struct apiCreateParams* params) {
    struct message* response = malloc(sizeof(*response));

    struct node* parentNode = storageFindNode(storage, tokenizedPath, (size_t) pathLen - 1);

    if (parentNode == NULL) {
        response->status = 0;
        response->info = strdup("Node is not found");
        return response;
    }
    uint64_t childrenAddr = storageFindChildren(storage, parentNode, tokenizedPath[pathLen - 1]);

    if (childrenAddr == 0) {
        struct node newNode = {
                .name = tokenizedPath[pathLen - 1],
                .next = 0,
                .child = 0,
                .value = params->value
        };

        storageCreateNode(storage, parentNode, &newNode);

        response->status = 1;
        response->info = strdup("Node is successfully created!");
    } else {
        response->status = 0;
        response->info = strdup("Node already exists");
    }
    storageFreeNode(parentNode);

    return response;
}

struct message* handleRequestRead(struct storage* storage, char** tokenizedPath, size_t pathLen) {
    struct message* response = malloc(sizeof(*response));
    char* messageInfo = malloc(sizeof(char) * MAX_MSG_LENGTH);
    bzero(messageInfo, sizeof(char) * MAX_MSG_LENGTH);

    struct node* node = storageFindNode(storage, tokenizedPath, pathLen);

    if (node == NULL) {
        response->status = 0;
        response->info = strdup("Node is not found");
        free(messageInfo);
        return response;
    }

    char** nameArr = storageGetAllChildrenName(storage, node);

    strcat(messageInfo, node->name);
    if (node->value != NULL) {
        strcat(messageInfo, " [");
        strcat(messageInfo, node->value);
        strcat(messageInfo, "]");
    }

    if (nameArr[0] != NULL)  {
        for (size_t i = 0; nameArr[i] != NULL; i++) {
            strcat(messageInfo, "\n  - ");
            strcat(messageInfo, nameArr[i]);
        }
    }

    response->status = 1;
    response->info = messageInfo;

    freeStringArray(nameArr);
    storageFreeNode(node);

    return response;
}

struct message* handleRequestUpdate(struct storage* storage, char** tokenizedPath, size_t pathLen, struct apiUpdateParams* params) {
    struct message* response = malloc(sizeof(*response));

    struct node* node = storageFindNode(storage, tokenizedPath, pathLen);

    if (node == NULL) {
        response->status = 0;
        response->info = strdup("Node is not found");
        return response;
    }

    storageUpdateNode(storage, node, params->value);
    response->status = 1;
    response->info = strdup("The value is successfully updated");

    storageFreeNode(node);

    return response;
}

struct message* handleRequestDelete(struct storage* storage, char** tokenizedPath, size_t pathLen, struct apiDeleteParams* params) {
    struct message* response = malloc(sizeof(*response));

    struct node* parentNode = storageFindNode(storage, tokenizedPath, (size_t) pathLen - 1);

    if (parentNode == NULL) {
        response->status = 0;
        response->info = strdup("Target node is not found");
        return response;
    }

    uint64_t childrenAddr = storageFindChildren(storage, parentNode, tokenizedPath[pathLen - 1]);

    if (childrenAddr != 0) {
        storageDeleteNode(storage, parentNode, childrenAddr, params->isDelValue);
        if (params->isDelValue) {
            response->status = 1;
            response->info = strdup("Value is successfully deleted!");
        } else {
            response->status = 1;
            response->info = strdup("Node is successfully deleted!");
        }
    } else {
        response->status = 0;
        response->info = strdup("Node is not found");
    }

    storageFreeNode(parentNode);
    return response;
}

struct message* handleRequest(struct storage* storage, struct command* command) {
    struct message* response;

    char** tokenizedPath = command->path;
    size_t pathLen = stringArrayLen(tokenizedPath);

    // create a.b
    // ["root", "a", "b", NULL]
    // pathLen = 3

    switch (command->apiAction) {
        case COMMAND_CREATE: {
            response = handleRequestCreate(storage, tokenizedPath, pathLen, &command->apiCreateParams);
            break;
        }
        case COMMAND_READ: {
            response = handleRequestRead(storage, tokenizedPath, pathLen);
            break;
        }
        case COMMAND_UPDATE: {
            response = handleRequestUpdate(storage, tokenizedPath, pathLen, &command->apiUpdateParams);
            break;
        }
        case COMMAND_DELETE: {
            response = handleRequestDelete(storage, tokenizedPath, pathLen, &command->apiDeleteParams);
            break;
        }
    }

    return response;
}

void handleClient(struct storage* storage, int socket) {
    while (!closing) {
        size_t protobufLength;
        uint8_t* protobuf = receiveProtobuf(socket, &protobufLength);
        if (!protobuf) {
            break;
        }

        struct command* command = protobufToStruct(protobuf, protobufLength, ROOT_NODE_NAME);
        free(protobuf);

        if (command == NULL && !closing) {
            printf("Error: bad command message\n");
            continue;
        }

        struct message* response = handleRequest(storage, command);
        uint8_t* responseBytes = responseToProtobuf(response, &protobufLength);
        sendProtobuf(socket, responseBytes, protobufLength);

        freeCommand(command);
        free(response->info);
        free(response);
        free(responseBytes);
    }

    close(socket);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printError("Invalid arguments: server [PORT] [STORAGE_FILE] [COMMANDS_FILE]\n");
    }

    int outputFD = open(argv[2], O_RDWR);
    struct storage *storage;

    if (outputFD < 0 && errno != ENOENT) {
        perror("Error while opening the file");
        return errno;
    }

    if (outputFD < 0 && errno == ENOENT) {
        outputFD = open(argv[2], O_CREAT | O_RDWR, 0644);

        struct node rootNode = {.name = ROOT_NODE_NAME, .next = 0, .child = 0, .value = NULL};
        storage = storageInit(outputFD);
        storage = storageInitRoot(outputFD, storage, &rootNode);
    } else {
        storage = storageOpen(outputFD);
        if (!storage) {
            perror("Bad file");
            return -1;
        }
    }

    int sock, port;
    int optval = 1;

    port = (int) strtol(argv[1], NULL, 10);
    if (port == 0) {
        perror("Enter the correct port");
        return errno;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        printError("Error while opening socket");

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

    struct sockaddr_in name;
    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(port);

    if (bind(sock, (void *) &name, sizeof(name))) {
        printError("Binding tcp socket");
    }

    if (listen(sock, 1) == -1) {
        printError("listen");
    }

    {
        struct sigaction sa;

        sa.sa_sigaction = close_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_SIGINFO;

        sigaction(SIGINT, &sa, NULL);
        sigaction(SIGTERM, &sa, NULL);
    }

    printf("Server has started!\n");

    char ipStr[INET_ADDRSTRLEN];
    struct sockaddr clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    while (!closing) {
        int newSocket = accept(sock, &clientAddr, &clientLen);
        if (closing) {
            break;
        }

        struct sockaddr_in * pV4Addr = (struct sockaddr_in *) &clientAddr;
        struct in_addr ipAddr = pV4Addr->sin_addr;
        inet_ntop(AF_INET, &ipAddr, ipStr, INET_ADDRSTRLEN);

        if (newSocket < 0) {
            printError("Error connection");
        }

        printf("Client %s connected\n", ipStr);

        handleClient(storage, newSocket);

        printf("Client %s disconnected\n", ipStr);
    }

    close(sock);
    free(storage);
    close(outputFD);

    printf("Bye!\n");
    return 0;
}
