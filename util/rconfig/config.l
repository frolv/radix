/*
 * util/rconfig/rconfig.l
 * Copyright (C) 2017 Alexei Frolov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

%option reentrant bison-bridge
%option yylineno

%{
#include "parser.h"
#include "rconfig.h"

#define YY_NO_INPUT
#define YY_NO_UNPUT
%}

digit                   [0-9]
letter                  [a-zA-Z]
id_nondigit             {letter}|"_"
string_char             [^\n"]

%%

configfile                              { return TOKEN_CONFIGFILE; }
section                                 { return TOKEN_SECTION; }
config                                  { return TOKEN_CONFIG; }

type                                    { return TOKEN_TYPE; }
desc                                    { return TOKEN_DESC; }
default                                 { return TOKEN_DEFAULT; }
range                                   { return TOKEN_RANGE; }
option                                  { return TOKEN_OPTION; }

int                                     { return TOKEN_INT; }
bool                                    { return TOKEN_BOOL; }
options                                 { return TOKEN_OPTIONS; }

true                                    { return TOKEN_TRUE; }
false                                   { return TOKEN_FALSE; }
-?{digit}*                              { return TOKEN_INTEGER; }
\"{string_char}*\"                      { return TOKEN_STRING; }

{id_nondigit}({id_nondigit}|{digit})*   { return TOKEN_ID; }

\t                                      { return TOKEN_TAB; }
[ \n\r]                                 { /* skip other whitespace */ }
#.*                                     { /* hash for line comments */ }
.                                       { exit(1); }

%%
