#include <string.h>
#include <stdlib.h>

#include "command_api.h"
#include "api.pb-c.h"
#include "util.h"

void freeStringArray(char** array) {
    for (size_t i = 0; *(array + i); i++) {
        free(*(array + i));
    }
    free(array);
}

// path free !!!
// path is initialized by (ROOT_NODE_NAME ".")
struct command* protobufToStruct(uint8_t* protobuf, size_t protobufLength, char* rootPath) {
    struct command* command = malloc(sizeof(*command));

    Command* protobufCommand = command__unpack(NULL, protobufLength, protobuf);
    if (!protobufCommand) {
        return NULL;
    }

    command->path = malloc(sizeof(char*) * (protobufCommand->n_path + 2));
    command->path[0] = strdup(rootPath);
    command->path[protobufCommand->n_path + 1] = NULL;
    for (int i = 0; i < protobufCommand->n_path; ++i) {
        command->path[i + 1] = strdup(protobufCommand->path[i]);
    }

    switch (protobufCommand->type_case) {
        case COMMAND__TYPE_CREATE:
            command->apiAction = COMMAND_CREATE;
            command->apiCreateParams.value = strdupOrNull(protobufCommand->create->value);
            break;

        case COMMAND__TYPE_READ:
            command->apiAction = COMMAND_READ;
            break;

        case COMMAND__TYPE_UPDATE:
            command->apiAction = COMMAND_UPDATE;
            command->apiUpdateParams.value = strdupOrNull(protobufCommand->update->value);
            break;

        case COMMAND__TYPE_DELETE:
            command->apiAction = COMMAND_DELETE;
            command->apiDeleteParams.isDelValue = protobufCommand->delete_->value;
            break;

        default:
            return NULL;
    }

    command__free_unpacked(protobufCommand, NULL);
    return command;
}

void destroyCommand(const struct command* command) {
    switch (command->apiAction) {
        case COMMAND_CREATE:
            free(command->apiCreateParams.value);
            break;

        case COMMAND_UPDATE:
            free(command->apiUpdateParams.value);
            break;

        case COMMAND_READ:
        case COMMAND_DELETE:
            break;
    }

    freeStringArray(command->path);
}

void freeCommand(struct command* command) {
    if (command) destroyCommand(command);
    free(command);
}

uint8_t* responseToProtobuf(struct message* response, size_t* protobufLength) {
    Message message = MESSAGE__INIT;
    message.status = response->status;
    message.info = response->info;

    *protobufLength = message__get_packed_size(&message);
    uint8_t* protobuf = malloc(*protobufLength);
    if (protobuf) {
        message__pack(&message, protobuf);
    }

    return protobuf;
}
