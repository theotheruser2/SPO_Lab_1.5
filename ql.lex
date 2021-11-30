%option noyywrap case-insensitive

%{
#define YY_NO_INPUT
#define YY_NO_UNPUT

#include "../command_api.h"
#include "y.tab.h"


static char * identifier_str() {
    char * const str = malloc(sizeof(char) * (yyleng + 1));

    if (str) {
        memcpy(str, yytext, yyleng);
        str[yyleng] = '\0';
    }

    return str;
}

static char * quoted_str() {
    char * str = malloc(sizeof(*str) * yyleng);

    int j = 0;
    for (int i = 1; i < yyleng - 1; ++i, ++j) {
        if (yytext[i] == '\\') {
            ++i;
        }

        str[j] = yytext[i];
    }

    str[j] = '\0';
    return str;
}
%}

S [ \n\b\t\f\r]
W [a-zA-Z_]
D [0-9]

I {W}({W}|{D})*

%%

{S}     ;

new     return T_NEW;
get     return T_GET;
set     return T_SET;
del     return T_DEL;

{I}     yylval.str = identifier_str(); return T_IDENTIFIER;

\'(\\.|[^'\\])*\'   yylval.str = quoted_str(); return T_STR_LITERAL;
\"(\\.|[^"\\])*\"   yylval.str = quoted_str(); return T_STR_LITERAL;

.       return yytext[0];

%%

void scan_string(const char * str) {
    yy_switch_to_buffer(yy_scan_string(str));
}
