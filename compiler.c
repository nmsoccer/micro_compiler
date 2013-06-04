/*
 * compiler.c
 *
 *  Created on: 2011-5-18
 *      Author: leiming
 */



#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "global.h"
#include "lexical_analy.h"
#include "syntax_analy.h"
#include "semantic_analy.h"
#include "trans_asm.h"

char compile_file_name[COMPILE_FILE_NAME_LEN];

int main(int argc , char **argv){
	unsigned char i;
	int ret;

	printf("tinyC compiler starts...\n");
	if(argc < 2){	/*需要输入源文件名*/
		DUMP_ERR("ERR: Please input source file name\n");
		return -1;
	}

	/*保存编译文件名字*/
	memset(compile_file_name , 0 , COMPILE_FILE_NAME_LEN);
	strcpy(compile_file_name , argv[1]);

	/*预处理源文件字符缓冲区*/
	memset(&src_buff , 0 , sizeof(struct _src_buff));

	src_buff.file = fopen(argv[1] , "r+");
	if(!src_buff.file){
		printf("open %s failed!\n" , argv[1]);
		return -1;
	}

	/*首先读入一行*/
	if(!fgets(src_buff.line_buff , LINE_BUFF_LEN , src_buff.file)){
		DUMP_ERR("end file!\n");
		return -1;
	}
	src_buff.token = NULL_TOKEN;	/*空标记*/
	src_buff.line = 1;	/*当前是第一行*/
	src_buff.index_line = 0;
	src_buff.index_token = 0;

	while(1){	/*获取第一个有效token*/
		get_token();

		if(src_buff.token == ERROR){
			break;
		}
		if(src_buff.token == END_FILE){
			break;
		}
		if(src_buff.token == CTRL){	/*如果是换行记号需要继续获取下一个记号*/
			continue;
		}
		/*第一个非ctrl记号*/
		DUMP_STR("get first effective token\n");
		break;
	}

	/*构造语法树之根结点*/
	syntax_tree = (syntax_node *)malloc(sizeof(syntax_node));
	memset(syntax_tree , 0 , sizeof(syntax_node));
	syntax_tree->kind = START_KIND;
	syntax_tree->type = START;
	syntax_tree->first_child = syntax_tree->second_child = syntax_tree->third_child = syntax_tree->sibling = NULL;

	if(syntax_analy() == 0){/*之后进入语法分析，构造语法树之其他部分*/
		DUMP_STR("create syntax tree success\n");
	}

	DUMP_STR("print syntax tree...\n");
	syntax_error = 0;
	print_syntax_tree(syntax_tree);

	/*如果没有出现语法错误。进行语义分析*/
	if(syntax_error == 0){
		semantic_analy();

		/*进行代码生成工作。主要是生成汇编文件 GAS格式*/
		ret = trans_asm(compile_file_name);
	}





	/*销毁语法树*/
	if(delete_syntax_tree(syntax_tree) == 0){
		DUMP_STR("delete syntax tree ...\n");
	}

	/*销毁所有符号表*/
	delete_sym_table(root_table);
	delete_sym_table(const_table);
	DUMP_STR("delete symbol table...\n");

	/*关闭文件描述符*/
	DUMP_STR("I am here\n");
	fclose(src_buff.file);

	return 0;

}


