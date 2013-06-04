/*
 * lexical_analy.c
 *
 *
 *	analyze lexical of tinyC
 *
 *  Created on: 2011-5-15
 *      Author: leiming
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "global.h"
#include "lexical_analy.h"

/*全局变量*/
struct _src_buff src_buff;		/*一个词法分析数据缓冲器*/

/*
 * 获取源文件中的标记。放入src_buff.token中
 *
 * @return: 0 成功获取标记
 * 					-1 到文件末尾或者出现错误
 */
int get_token(void){	/*每次调用此函数时都是一个新的记号开始*/
	unsigned char is_real;	/*标志是否实型*/
	token_type type;


	is_real = 0;

DUMP_LEX_STR(">>>");
	memset(src_buff.token_buff , 0 , TOKEN_BUFF_LEN);	/*清空上一个记号内容*/
	src_buff.index_token = 0;

start_match:
DUMP_LEX_STR("start_match\n");

	switch(src_buff.line_buff[src_buff.index_line]){
/*--------------------------------------------------------------------------------------------------------------*/
	case ' ':																/*空格或者制表符号。忽略之进行下一步分析*/
	case '\t':
//DUMP_INFO(src_buff.line , space or tab\n);
		src_buff.index_line++;
		goto start_match;
		break;	/*break space tab */

/*--------------------------------------------------------------------------------------------------------------*/
	case '\n':																/*换行符号*/
		/*已经取到了当前行末尾 读取源文件下一行*/
DUMP_INFO(src_buff.line , "ctrl");
		memset(src_buff.line_buff , 0 , LINE_BUFF_LEN);
		if(!fgets(src_buff.line_buff , LINE_BUFF_LEN , src_buff.file)){
			DUMP_ERR("end file!\n");
			src_buff.token = END_FILE;
			return -1;
		}
		src_buff.line++;
		src_buff.index_line = 0;
		src_buff.token = CTRL;
		return 0;
		break;	/*break \n */

/*--------------------------------------------------------------------------------------------------------------*/
	case ';':															/*空语句 ; */
DUMP_INFO(src_buff.line , ";");
		src_buff.index_line++;
		src_buff.token = NULL_STATE;
		src_buff.token_buff[0] = ';';
		return 0;
		break;	/* ; */

/*--------------------------------------------------------------------------------------------------------------*/
	case '=':														/*赋值操作符  = 或者相等操作符 == */
		src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
		src_buff.index_token++;
		src_buff.index_line++;

		if(src_buff.line_buff[src_buff.index_line] == '='){	/*相等操作符 *= */
			src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
			src_buff.index_line++;
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = EQUALITY;
			return 0;
		}else{	/*赋值操作符*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = ASSIGN;
			return 0;
		}
		break; /* = */

/*--------------------------------------------------------------------------------------------------------------*/
	case '{':														/*左大括弧 { */
DUMP_INFO(src_buff.line , "{");
		src_buff.index_line++;
		src_buff.token = LBRACE;
		src_buff.token_buff[0] = '{';
		return 0;
		break;	/* { */

/*--------------------------------------------------------------------------------------------------------------*/
	case '}':														/*右大括弧 } */
DUMP_INFO(src_buff.line , "}");
		src_buff.index_line++;
		src_buff.token = RBRACE;
		src_buff.token_buff[0] = '}';
		return 0;
		break;	/* { */

/*--------------------------------------------------------------------------------------------------------------*/
	case '(':														/*左小括弧  ( */
DUMP_INFO(src_buff.line , "(");
		src_buff.index_line++;
		src_buff.token = LPARENT;
		src_buff.token_buff[0] = '(';
		return 0;
		break;	/* ( */
/*--------------------------------------------------------------------------------------------------------------*/
	case ')':														/*右小括弧 ) */
DUMP_INFO(src_buff.line , ")");
		src_buff.index_line++;
		src_buff.token = RPARENT;
		src_buff.token_buff[0] = ')';
		return 0;
		break; /* ) */
/*--------------------------------------------------------------------------------------------------------------*/
	case '[':														/*下标操作符  [ */
DUMP_INFO(src_buff.line , "[");
		src_buff.index_line++;
		src_buff.token = SUBSCRIPT;
		src_buff.token_buff[0] = '[';
		return 0;
		break; /* [ */
/*--------------------------------------------------------------------------------------------------------------*/
	case ']':														/*下标操作符结束标志 ]*/
DUMP_INFO(src_buff.line , "]");
		src_buff.index_line++;
		src_buff.token = END_SUBSCRIPT;
		src_buff.token_buff[0] = ']';
		return 0;
		break; /* ] */
/*--------------------------------------------------------------------------------------------------------------*/
	case '.':														/*选择操作符 .*/
DUMP_INFO(src_buff.line , ".");
		src_buff.index_line++;
		src_buff.token = SELECT;
		src_buff.token_buff[0] = '.';
		return 0;
		break; /* . */

/*--------------------------------------------------------------------------------------------------------------*/
	case '$':														/*取地址符 $*/
DUMP_INFO(src_buff.line , "$");
		src_buff.index_line++;
		src_buff.token = PREFIX;
		src_buff.token_buff[0] = '$';
		return 0;
		break; /* $ */

/*--------------------------------------------------------------------------------------------------------------*/
	case '!':														/*前缀操作符  ! 或者 !$ 或者关系操作符 != */
		src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
		src_buff.index_token++;
		src_buff.index_line++;

		if(src_buff.line_buff[src_buff.index_line] == '='){	/*关系操作符号!=*/
			src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
			src_buff.index_line++;
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = EQUALITY;
			return 0;
		}else if(src_buff.line_buff[src_buff.index_line] == '$'){	/*解除引用操作符 !$*/
			src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
			src_buff.index_line++;
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = PREFIX;
			return 0;
		}else{	/*非操作符号 ! */
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = PREFIX;
			return 0;
		}
		break; /* ! */

/*--------------------------------------------------------------------------------------------------------------*/
	case '+':														/*前缀操作符  ++ 或者 算术操作符 +  或者赋值操作符 += */
		src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
		src_buff.index_token++;
		src_buff.index_line++;

		if(src_buff.line_buff[src_buff.index_line] == '='){	/*赋值操作符+=*/
			src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
			src_buff.index_line++;
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = ASSIGN;
			return 0;
		}else if(src_buff.line_buff[src_buff.index_line] == '+'){	/*前缀操作符 ++*/
			src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
			src_buff.index_line++;
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = PREFIX;
			return 0;
		}else{	/*算术操作符 + */
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = ADDITIVE;
			return 0;
		}
		break; /* + */

/*--------------------------------------------------------------------------------------------------------------*/
	case '-':														/*前缀操作符  -- 或者 算术操作符 -  或者赋值操作符 -= 或者选择符-> */
		src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
		src_buff.index_token++;
		src_buff.index_line++;

		if(src_buff.line_buff[src_buff.index_line] == '='){	/*赋值操作符-=*/
			src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
			src_buff.index_line++;
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = ASSIGN;
			return 0;
		}else if(src_buff.line_buff[src_buff.index_line] == '-'){	/*前缀操作符 --*/
			src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
			src_buff.index_line++;
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = PREFIX;
			return 0;
		}else if(src_buff.line_buff[src_buff.index_line] == '>'){	/*选择操作符->*/
			src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
			src_buff.index_line++;
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = SELECT;
			return 0;
		}else{	/*算术操作符 - */
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = ADDITIVE;
			return 0;
		}
		break; /* - */

/*--------------------------------------------------------------------------------------------------------------*/
	case '*':														/*可能是算术乘法操作符 * 也可能是赋值符 *= */
		src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
		src_buff.index_token++;
		src_buff.index_line++;

		if(src_buff.line_buff[src_buff.index_line] == '='){	/*赋值操作符 *= */
			src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
			src_buff.index_line++;
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = ASSIGN;
			return 0;
		}else{	/*算术乘法操作符*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = MULTIPLE;
			return 0;
		}
		break; /* * */

/*--------------------------------------------------------------------------------------------------------------*/
	case '%':														/*可能是算术取摸操作符 % 也可能是赋值符 %= */
		src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
		src_buff.index_token++;
		src_buff.index_line++;

		if(src_buff.line_buff[src_buff.index_line] == '='){	/*赋值操作符 *= */
			src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
			src_buff.index_line++;
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = ASSIGN;
			return 0;
		}else{	/*算术取模操作符*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = MULTIPLE;
			return 0;
		}
		break; /* % */

/*--------------------------------------------------------------------------------------------------------------*/
	case '"':														/*字符串常量 STR_VAL “ xxxx" */
		src_buff.index_line++;

		while(1){
			switch(src_buff.line_buff[src_buff.index_line]){
			case '"':	/*正常的字符串常量结束标志*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
				src_buff.index_line++;
				src_buff.token = STR_VAL;
				return 0;
				break;
			case '\n': /*在结束标志前换行。出现错误*/
				printf("line %d: syntax error: const string ends without \" \n" , src_buff.line);
				src_buff.token = ERROR;
				return -1;
				break;
			default:	/*其他字符都当作字符串常量的内容*/
				src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
				src_buff.index_token++;
				src_buff.index_line++;
				break;
			}
		}	/*while*/
		break;	/*STR_VAL*/

/*--------------------------------------------------------------------------------------------------------------*/
	case '\'':														/*字符型常量 CHAR_VAL 'a' */
		src_buff.index_line++;

		switch(src_buff.line_buff[src_buff.index_line]){
		case '\'':	/*空的字符常量*/
			printf("line %d: syntax error: empty const char \n" , src_buff.line);
			src_buff.token = ERROR;
			return -1;
			break;
		case '\n': /*在结束标志前换行。出现错误*/
			printf("line %d: syntax error: const char ends without ' \n" , src_buff.line);
			src_buff.token = ERROR;
			return -1;
			break;
		case '\\':	/*转义字符*/
			src_buff.index_line++;

			switch(src_buff.line_buff[src_buff.index_line]){
			case 'a':	/* \a */
				src_buff.token_buff[0] = '\a';
				break;
			case 'b':	/* \b */
				src_buff.token_buff[0] = '\b';
				break;
			case 'f':	/* \f */
				src_buff.token_buff[0] = '\f';
				break;
			case 'n': /* \n */
				src_buff.token_buff[0] = '\n';
				break;
			case 'r':	/* \r */
				src_buff.token_buff[0] = '\r';
				break;
			case 't':	/* \t */
				src_buff.token_buff[0] = '\t';
				break;
			case 'v':	/* \v */
				src_buff.token_buff[0] = '\v';
				break;
			case '"':	/* \" */
				src_buff.token_buff[0] = '\"';
				break;
			case '\'':	/* \' */
				src_buff.token_buff[0] = '\'';
				break;
			case '\\':	/* \\ */
				src_buff.token_buff[0] = '\\';
				break;
			case '?':	/* \? */
				src_buff.token_buff[0] = '\?';
				break;
			default:	/* unknow escape sequence*/
				printf("line %d: syntax error: unknow escape sequence: \\%c \n" , src_buff.line , src_buff.line_buff[src_buff.index_line]);
				src_buff.token = ERROR;
				return -1;
				break;
			}
printf("line %d:   escape: \%c\n" , src_buff.line , src_buff.line_buff[src_buff.index_line]);
			src_buff.index_line++;
			if(src_buff.line_buff[src_buff.index_line] == '\''){	/*必须以' 结尾*/
				src_buff.index_line++;
				src_buff.token = CHAR_VAL;
				return 0;
			}else{
				printf("line %d: syntax error: const char ends without ' \n" , src_buff.line);
				src_buff.token = ERROR;
				return -1;
			}
			break;	/*转义字符*/

		default:	/*其他字符都当作字符串常量的内容*/
			src_buff.token_buff[0] = src_buff.line_buff[src_buff.index_line];
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.index_line++;
			if(src_buff.line_buff[src_buff.index_line] == '\''){	/*必须以' 结尾*/
				src_buff.index_line++;
				src_buff.token = CHAR_VAL;
				return 0;
			}else{
				printf("line %d: syntax error: more than one const char \n" , src_buff.line);
				src_buff.token = ERROR;
				return -1;
			}
			break;
		}
		break;	/*CHAR_VAL*/

/*--------------------------------------------------------------------------------------------------------------*/
	case '/':																/*/ 开头的序列。可能为算术符号 也可能为注释*/
		src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
		src_buff.index_token++;
		src_buff.index_line++;

		switch(src_buff.line_buff[src_buff.index_line]){		/*继续往前考察下一个字符*/
		case '/':	/*单行注释*/
			/*读取源文件下一行*/
DUMP_INFO(src_buff.line , "single comment");
			memset(src_buff.line_buff , 0 , LINE_BUFF_LEN);
			memset(src_buff.token_buff , 0 , TOKEN_BUFF_LEN);	/*清空之间在token缓冲区的字符*/
			src_buff.index_token = 0;
			if(!fgets(src_buff.line_buff , LINE_BUFF_LEN , src_buff.file)){
				DUMP_ERR("end file!\n");
				src_buff.token = END_FILE;
				return -1;
			}
			src_buff.line++;
			src_buff.index_line = 0;
			goto start_match;	/*忽略单行注释进行下一行的获取。*/

		case '*':	/*多行注释*/
			memset(src_buff.token_buff , 0 , TOKEN_BUFF_LEN);
			src_buff.index_token = 0;

			src_buff.index_line++;
			while(1){
				if(src_buff.line_buff[src_buff.index_line] == '*'){	/*如果发现是*需要向前看是否连接/ 如果是标明注释结束否则继续*/
					src_buff.index_line++;
					if(src_buff.line_buff[src_buff.index_line] == '/'){
DUMP_INFO(src_buff.line , "multiple comment");
						src_buff.index_line++;
						goto start_match;	/*这是唯一正确的出口*/
					}
					continue;
				}

				if(src_buff.line_buff[src_buff.index_line] == '\n'){	/*注释中出现换行符号*/
					memset(src_buff.line_buff , 0 , LINE_BUFF_LEN);
					if(!fgets(src_buff.line_buff , LINE_BUFF_LEN , src_buff.file)){
						DUMP_ERR("end file!\n");
						src_buff.token = END_FILE;
						return -1;	/*表明是没有结束符号的注释到了文件末尾错误出口*/
					}
					src_buff.line++;
					src_buff.index_line = 0;
					continue;
				}

				src_buff.index_line++;	/*其余任意字符都忽略*/

			}	/*end while*/
			break;


		case '=':	/*赋值符号 /= */
			src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.index_line++;
			src_buff.token = ASSIGN;
			return 0;

		default:	/*其他任意字符标明该标记是除法*/
DUMP_INFO(src_buff.line , "/");
			src_buff.token = MULTIPLE;
			return 0;
		}
		break;	/*break / */

/*--------------------------------------------------------------------------------------------------------------*/
	case '0'...'9':														/*整形常量INT_VAL 或者实型常量REAL_VAL*/
		src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
		src_buff.index_token++;
		src_buff.index_line++;

		while(1){
			switch(src_buff.line_buff[src_buff.index_line]){
			case '0'...'9':
				src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
				src_buff.index_token++;
				src_buff.index_line++;
				break;
			case '.':	/*表明是实型常量*/
				if(is_real == 0){	/*修改标志为实型*/
					is_real = 1;
					src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
					src_buff.index_token++;
					src_buff.index_line++;
				}else{	/*表明出现了多个小数点*/
					printf("line %d: syntax error: too many points \n" , src_buff.line);
					src_buff.token = ERROR;
					return -1;
				}
				break;
			default:	/*数字结束*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
				src_buff.token = is_real ? REAL_VAL : INT_VAL;
				return 0;
			}
		}	/*while*/
		break;	/*INT_VAL or REAL_VAL*/

/*--------------------------------------------------------------------------------------------------------------*/
	case '_':																						/*IDENTIFIER*/
	case 'a'...'z':
	case 'A'...'Z':
		src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
		src_buff.index_token++;
		src_buff.index_line++;

		while(1){

			switch(src_buff.line_buff[src_buff.index_line]){
			case '_':	/*任意_ 字符 数字组合*/
			case 'a'...'z':
			case 'A'...'Z':
			case '0'...'9':
				src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
				src_buff.index_token++;
				src_buff.index_line++;
				break;

			default:	/*其余字符退出获取*/
				/*根据token内容检验是否关键字*/
				if(strcmp(src_buff.token_buff , "sizeof") == 0){	/*sizeof 这是操作符*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
					src_buff.token = SIZEOF;
					return 0;
				}
				if(strcmp(src_buff.token_buff , "if") == 0){	/*if*/
DUMP_INFO(src_buff.line ,src_buff.token_buff);
					src_buff.token = IF;
					return 0;
				}
				if(strcmp(src_buff.token_buff , "while") == 0){	/*while*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
					src_buff.token = WHILE;
					return 0;
				}
				if(strcmp(src_buff.token_buff , "else") == 0){	/*else*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
					src_buff.token = ELSE;
					return 0;
				}
				if(strcmp(src_buff.token_buff , "for") == 0){	/*for*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
					src_buff.token = FOR;
					return 0;
				}
				if(strcmp(src_buff.token_buff , "switch") == 0){	/*switch*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
					src_buff.token = SWITCH;
					return 0;
				}
				if(strcmp(src_buff.token_buff , "do") == 0){	/*do*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
					src_buff.token = DO;
					return 0;
				}
				if(strcmp(src_buff.token_buff , "case") == 0){	/*case*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
					src_buff.token = CASE;
					return 0;
				}
				if(strcmp(src_buff.token_buff , "cast") == 0){	/*cast*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
					src_buff.token = CAST;
					return 0;
				}
				if(strcmp(src_buff.token_buff , "return") == 0){	/*return*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
					src_buff.token = RETURN;
					return 0;
				}
				if(strcmp(src_buff.token_buff , "break") == 0){	/*break*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
					src_buff.token = BREAK;
					return 0;
				}
				if(strcmp(src_buff.token_buff , "continue") == 0){	/*continue*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
					src_buff.token = CONTINUE;
					return 0;
				}
				if(strcmp(src_buff.token_buff , "default") == 0){	/*default*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
					src_buff.token = DEFAULT;
					return 0;
				}
				if(strcmp(src_buff.token_buff , "struct") == 0){	/*struct*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
					src_buff.token = STRUCT_UNION;
					return 0;
				}
				if(strcmp(src_buff.token_buff , "structdef") == 0){	/*structdef*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
					src_buff.token = STRUCTDEF;
					return 0;
				}
				if(strcmp(src_buff.token_buff , "function") == 0){	/*function*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
					src_buff.token = FUNCTION;
					return 0;
				}
				if(strcmp(src_buff.token_buff , "struct") == 0 || strcmp(src_buff.token_buff , "union") == 0){	/*struct or union*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
					src_buff.token = STRUCT_UNION;
					return 0;
				}
				if(strcmp(src_buff.token_buff , "enum") == 0){	/*enum*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
					src_buff.token = ENUM;
					return 0;
				}
				if(strcmp(src_buff.token_buff , "extern") == 0 || strcmp(src_buff.token_buff , "static") == 0 || strcmp(src_buff.token_buff , "auto") == 0 ||
					strcmp(src_buff.token_buff , "register") == 0){		/*storage class*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
					src_buff.token = STORAGE_CLASS;
					return 0;
				}
				if(strcmp(src_buff.token_buff , "const") == 0 || strcmp(src_buff.token_buff , "volatile") == 0){	/*type qualifier*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
					src_buff.token = TYPE_QUALIFIER;
					return 0;
				}
				if(strcmp(src_buff.token_buff , "void") == 0 || strcmp(src_buff.token_buff , "char") == 0 || strcmp(src_buff.token_buff , "short") == 0 ||
					strcmp(src_buff.token_buff , "int") == 0	|| strcmp(src_buff.token_buff , "long") == 0 || strcmp(src_buff.token_buff , "float") == 0 ||
					strcmp(src_buff.token_buff , "double") == 0){	/*type*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
					src_buff.token = TYPE;
					return 0;
				}
				if(strcmp(src_buff.token_buff , "label") == 0){	/*label*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
					src_buff.token = LABEL;
					return 0;
				}
				if(strcmp(src_buff.token_buff , "goto") == 0){	/*goto*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
					src_buff.token = GOTO;
					return 0;
				}

				/*最后确定为identifer*/
DUMP_INFO(src_buff.line , src_buff.token_buff);
				src_buff.token = IDENTIFIER;
				return 0;

			}

	}
	break;	/*break identifier*/

/*--------------------------------------------------------------------------------------------------------------*/
	case '<':														/*移位操作符  << 或者 关系操作符 <  或者 <= */
		src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
		src_buff.index_token++;
		src_buff.index_line++;

		if(src_buff.line_buff[src_buff.index_line] == '<'){	/*移位操作符 <<*/
			src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
			src_buff.index_line++;
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = SHIFT;
			return 0;
		}else if(src_buff.line_buff[src_buff.index_line] == '='){	/*关系操作符 <=*/
			src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
			src_buff.index_line++;
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = RELATION;
			return 0;
		}else{	/*关系操作符 < */
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = RELATION;
			return 0;
		}
		break; /* < */

/*--------------------------------------------------------------------------------------------------------------*/
	case '>':														/*移位操作符  >> 或者 关系操作符 >  或者 >= */
		src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
		src_buff.index_token++;
		src_buff.index_line++;

		if(src_buff.line_buff[src_buff.index_line] == '>'){	/*移位操作符 >>*/
			src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
			src_buff.index_line++;
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = SHIFT;
			return 0;
		}else if(src_buff.line_buff[src_buff.index_line] == '='){	/*关系操作符 >=*/
			src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
			src_buff.index_line++;
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = RELATION;
			return 0;
		}else{	/*关系操作符 < */
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = RELATION;
			return 0;
		}
		break; /* > */

/*--------------------------------------------------------------------------------------------------------------*/
	case '&':														/*按位与操作符  & 或者 逻辑与操作符 &&  或者 赋值操作符 &= */
		src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
		src_buff.index_token++;
		src_buff.index_line++;

		if(src_buff.line_buff[src_buff.index_line] == '&'){	/*逻辑与操作符 &&*/
			src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
			src_buff.index_line++;
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = LOGIC_AND;
			return 0;
		}else if(src_buff.line_buff[src_buff.index_line] == '='){	/*赋值操作符 &=*/
			src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
			src_buff.index_line++;
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = ASSIGN;
			return 0;
		}else{	/*按位与操作符 & */
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = BIT_AND;
			return 0;
		}
		break; /* & */

/*--------------------------------------------------------------------------------------------------------------*/
	case '|':														/*按位或操作符  | 或者 逻辑或操作符 ||  或者 赋值操作符 |= */
		src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
		src_buff.index_token++;
		src_buff.index_line++;

		if(src_buff.line_buff[src_buff.index_line] == '|'){	/*逻辑或操作符 ||*/
			src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
			src_buff.index_line++;
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = LOGIC_OR;
			return 0;
		}else if(src_buff.line_buff[src_buff.index_line] == '='){	/*赋值操作符 |=*/
			src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
			src_buff.index_line++;
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = ASSIGN;
			return 0;
		}else{	/*按位或操作符 | */
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = BIT_OR;
			return 0;
		}
		break; /* | */

/*--------------------------------------------------------------------------------------------------------------*/
	case '^':														/*可能是按位异或操作符 ^ 也可能是赋值符 ^= */
		src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
		src_buff.index_token++;
		src_buff.index_line++;

		if(src_buff.line_buff[src_buff.index_line] == '='){	/*赋值操作符 ^= */
			src_buff.token_buff[src_buff.index_token] = src_buff.line_buff[src_buff.index_line];
			src_buff.index_line++;
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = ASSIGN;
			return 0;
		}else{	/*按位异或操作符^ */
DUMP_INFO(src_buff.line , src_buff.token_buff);
			src_buff.token = BIT_XOR;
			return 0;
		}
		break; /* ^ */

/*--------------------------------------------------------------------------------------------------------------*/
	case '~':														/*前缀操作符 ~*/
DUMP_INFO(src_buff.line , "~");
		src_buff.index_line++;
		src_buff.token = PREFIX;
		src_buff.token_buff[0] = '~';
		return 0;
		break; /* ~ */

/*--------------------------------------------------------------------------------------------------------------*/
	case ',':														/*逗号操作符 , */
DUMP_INFO(src_buff.line , ",");
		src_buff.index_line++;
		src_buff.token = SEQUEN;
		src_buff.token_buff[0] = ',';
		return 0;
		break; /* , */

/*--------------------------------------------------------------------------------------------------------------*/
	case '?':														/*条件操作符 ?*/
		src_buff.token_buff[0] = '?';
DUMP_INFO(src_buff.line , src_buff.token_buff);
		src_buff.index_line++;
		src_buff.token = CONDITION;
		return 0;
		break; /* ? */

/*--------------------------------------------------------------------------------------------------------------*/
	case ':':														/*条件操作符结束标志 :*/
		src_buff.token_buff[0] = ':';
DUMP_INFO(src_buff.line , src_buff.token_buff);
		src_buff.index_line++;
		src_buff.token = END_CONDITION;
		return 0;
		break; /* : */

/*--------------------------------------------------------------------------------------------------------------*/		type = END_FILE;
	default:														/*出现不合法标识符*/
		printf("line %d: syntax error: illegal character \n" , src_buff.line);
		src_buff.token = ERROR;
		return -1;
	}
	return -1;
}

/*
 * 与当前src_buff.token类型进行匹配
 * @param token: 欲匹配的标记类型。
 * @return:
 * 0:匹配成功
 * -1:匹配失败
 */
int match(token_type token){
	if(src_buff.token == token){

		while(1){	/*取下一个非换行的token*/
			get_token();
			if(src_buff.token != CTRL){
				break;
			}
		}
		return 0;

	}else{
		return -1;
	}


}
