#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>

#include "../util.h"
#include "../mt.h"
#include "../command_api.h"
#include "cmd_parser.h"
#include "y.tab.h"

void scan_string(const char * str);

bool receive(int sock) {
    size_t bufLength;
    uint8_t* buf = receiveProtobuf(sock, &bufLength);
    if (!buf) {
        return false;
    }

    struct message msg = protobufToMsg(buf, bufLength);
    free(buf);

    printf("%s\n", msg.info);
    free(msg.info);
    return true;
}

bool handleCommand(int sock) {
    char * inputCmd = malloc(sizeof(char) * MAX_MSG_LENGTH);

    if (!readCmd(inputCmd, sizeof(char) * MAX_MSG_LENGTH)) {
        free(inputCmd);
        return false;
    }

    if (strcmp(inputCmd, "\n") == 0) {
        free(inputCmd);
        return true;
    }

    char* parsingError = NULL;
    struct command cmd;

    scan_string(inputCmd);
    if (yyparse(&cmd, &parsingError) != 0) {
        printf("Parsing error: %s.\n", parsingError);
        free(parsingError);
        free(inputCmd);
        return true;
    }

    free(inputCmd);

    size_t outputLength;
    uint8_t* output = cmdToProtobuf(&cmd, &outputLength);
    if (!sendProtobuf(sock, output, outputLength)) {
        destroyCommand(&cmd);
        free(output);
        return false;
    }

    free(output);
    destroyCommand(&cmd);
    return receive(sock);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printError("Invalid arguments: client PORT [COMMANDS_FILE]");
    }

    int port = (int) strtol(argv[1], NULL, 10);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
        printError("Socket in not created");
    }

	struct in_addr server_addr;
	if (!inet_aton(LOCALHOST, &server_addr)) {
        printError("Error: inet_aton");
    }

	struct sockaddr_in connection;
	connection.sin_family = AF_INET;
	memcpy(&connection.sin_addr, &server_addr, sizeof(server_addr));
	connection.sin_port = htons(port);
	if (connect(sock, (const struct sockaddr*) &connection, sizeof(connection)) != 0) {
        printError("Failed connection");
    }

    while (handleCommand(sock));

    printf("Disconnected.\n");
	return 0;
}
