#include "cmd_parser.h"

#include "api.pb-c.h"

// protobuf => struct message
struct message protobufToMsg(const uint8_t *protobuf, size_t protobufLength) {
    Message* protobufMessage = message__unpack(NULL, protobufLength, protobuf);

    int status = protobufMessage->status;
    char* info = strdupOrNull(protobufMessage->info);
    message__free_unpacked(protobufMessage, NULL);

    return (struct message) {.status = status, .info = info};
}

uint8_t* cmdToProtobuf(struct command* cmd, size_t* protobufLength) {
    Command command = COMMAND__INIT;
    void* ptr = NULL;

    command.path = cmd->path;
    command.n_path = stringArrayLen(cmd->path);

    switch (cmd->apiAction) {
        case COMMAND_CREATE:
            command.type_case = COMMAND__TYPE_CREATE;
            ptr = command.create = malloc(sizeof(CreateCommand));
            create_command__init(command.create);
            command.create->value = cmd->apiCreateParams.value;
            break;

        case COMMAND_UPDATE:
            command.type_case = COMMAND__TYPE_UPDATE;
            ptr = command.update = malloc(sizeof(UpdateCommand));
            update_command__init(command.update);
            command.update->value = cmd->apiUpdateParams.value;
            break;

        case COMMAND_DELETE:
            command.type_case = COMMAND__TYPE_DELETE;
            ptr = command.delete_ = malloc(sizeof(DeleteCommand));
            delete_command__init(command.delete_);
            command.delete_->value = cmd->apiDeleteParams.isDelValue;
            break;

        case COMMAND_READ:
            command.type_case = COMMAND__TYPE_READ;
            ptr = command.read = malloc(sizeof(ReadCommand));
            read_command__init(command.read);
            break;
    }

    *protobufLength = command__get_packed_size(&command);
    uint8_t* protobuf = malloc(*protobufLength);
    command__pack(&command, protobuf);

    free(ptr);
    return protobuf;
}

bool readCmd(char* inputCmd, size_t size) {
    return fgets(inputCmd, (int) size, stdin) != NULL;
}
