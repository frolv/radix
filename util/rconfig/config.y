/*
 * util/rconfig/rconfig.y
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

%{
#include <stdio.h>

#include "parser.h"
#include "rconfig.h"
#include "scanner.h"

void yyerror(yyscan_t scanner, struct rconfig_file *config, const char *err)
{
	fprintf(stderr, "%s:%d: %s\n", config->path,
	        yyget_lineno(scanner), err);
}
%}

%code requires {
#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void *yyscan_t;
#endif

struct rconfig_file;
}

%define api.pure full
%define parse.error verbose

%union {
	char *string;
}

%lex-param   {yyscan_t scanner}
%parse-param {yyscan_t scanner}
%parse-param {struct rconfig_file *rconfig_file}

%token TOKEN_CONFIGFILE TOKEN_SECTION TOKEN_CONFIG
%token TOKEN_TYPE TOKEN_DESC TOKEN_DEFAULT TOKEN_RANGE TOKEN_OPTION
%token TOKEN_INT TOKEN_BOOL TOKEN_OPTIONS
%token TOKEN_TRUE TOKEN_FALSE TOKEN_INTEGER TOKEN_STRING
%token TOKEN_ID
%token TOKEN_TAB

%type <string> section_header

%start rconfig_file

%%

rconfig_file
	: configfile_header
	| configfile_header { prepare_sections(rconfig_file); }
	  configfile_sections
	;

configfile_header
	: TOKEN_CONFIGFILE TOKEN_ID {
		rconfig_file->name = strdup(yyget_text(scanner));
	}
	;

configfile_sections
	: configfile_section
	| configfile_sections configfile_section
	;

configfile_section
	: section_header config_list {
		add_section(rconfig_file, $1, NULL);
	}
	;

section_header
	: TOKEN_SECTION TOKEN_ID {
		$$ = strdup(yyget_text(scanner));
	}
	;

config_list
	: config
	| config_list config
	;

config
	: config_name settings_list
	;

config_name
	: TOKEN_CONFIG TOKEN_ID
	;

settings_list
	: indented_setting
	| settings_list indented_setting
	;

indented_setting
	: TOKEN_TAB setting
	;

setting
	: type_setting
	| default_setting
	| desc_setting
	| range_setting
	| option_setting
	;

type_setting
	: TOKEN_TYPE type
	;

type
	: TOKEN_BOOL
	| TOKEN_INT
	| TOKEN_OPTIONS
	;

default_setting
	: TOKEN_DEFAULT bool
	| TOKEN_DEFAULT TOKEN_INTEGER
	;

bool
	: TOKEN_TRUE
	| TOKEN_FALSE
	;

desc_setting
	: TOKEN_DESC TOKEN_STRING
	;

range_setting
	: TOKEN_RANGE TOKEN_INTEGER TOKEN_INTEGER
	;

option_setting
	: TOKEN_OPTION TOKEN_INTEGER TOKEN_STRING
	;

%%
