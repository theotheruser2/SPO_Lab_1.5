%parse-param {struct command* result} {char ** error}

// %code requires {
// #include <stdbool.h>
// }

%{
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "../command_api.h"
#include "../util.h"

int yylex(void);
void yyerror(struct command* result, char** error, const char* str);
%}

%union {
    struct command cmd;
    char** str_array;
    char* str;
}

%token T_NEW T_GET T_SET T_DEL
%token<str> T_IDENTIFIER T_STR_LITERAL

%type<cmd> command_line command get_command new_command set_command del_command
%type<str_array> path path_non_empty
%type<str> path_segment value

%%

command_line
    : command YYEOF { *result = $1; }
    ;

command
    : get_command
    | new_command
    | set_command
    | del_command
    ;

get_command
    : T_GET path    { $$ = (struct command) { .apiAction = COMMAND_READ, .path = $2 }; }
    ;

new_command
    : T_NEW path_non_empty            {
        $$ = (struct command) {
            .apiAction = COMMAND_CREATE,
            .path = $2,
            .apiCreateParams = { .value = NULL }
        };
    }
    | T_NEW path_non_empty '=' value  {
        $$ = (struct command) {
            .apiAction = COMMAND_CREATE,
            .path = $2,
            .apiCreateParams = { .value = $4 }
        };
    }
    ;

set_command
    : T_SET path_non_empty '=' value  {
        $$ = (struct command) {
            .apiAction = COMMAND_UPDATE,
            .path = $2,
            .apiUpdateParams = { .value = $4 }
        };
    }
    ;

del_command
    : T_DEL path_non_empty        {
        $$ = (struct command) {
            .apiAction = COMMAND_DELETE,
            .path = $2,
            .apiDeleteParams = { .isDelValue = false }
        };
    }
    | T_DEL path_non_empty '='    {
        $$ = (struct command) {
            .apiAction = COMMAND_DELETE,
            .path = $2,
            .apiDeleteParams = { .isDelValue = true }
        };
    }
    ;

path
    : /* empty */ {
        $$ = malloc(sizeof(char*));
        $$[0] = NULL;
    }
    | path_non_empty
    ;

path_non_empty
    : path_segment {
        $$ = malloc(sizeof(char*) * 2);
        $$[0] = $1;
        $$[1] = NULL;
    }
    | path '.' path_segment {
        size_t length = stringArrayLen($1);

        $$ = realloc($1, sizeof(char*) * (length + 2));
        $$[length] = $3;
        $$[length + 1] = NULL;
    }
    ;

path_segment
    : T_IDENTIFIER
    | T_STR_LITERAL
    ;

value
    : T_STR_LITERAL
    ;

%%

void yyerror(struct command* result, char** error, const char* str) {
    free(*error);

    *error = strdup(str);
}
