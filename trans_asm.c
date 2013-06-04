/*
 * trans_asm.c
 *
 *	将通过词法、语法、语义分析之后的源文件及相关数据结构翻译成AT&T语法的汇编文件
 *
 *  Created on: 2011-6-23
 *      Author: leiming
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "global.h"
#include "syntax_analy.h"
#include "semantic_analy.h"

static FILE *asm_file;

/*static functions*/
static void output_data_section(void);	/*填充汇编数据区 实际是root_table的变量*/
static void output_rodata_section(void);	/*填充汇编只读数据区 实际是const_table的常量以及root_table的const常量*/
static void output_bss_section(void);	/*填充汇编bss数据区 实际是定义为struct的变量和数组变量*/
static void output_global(void);	/*输入全局过程或者变量*/

static int trans_code(syntax_node *node);	/*翻译代码*/

/*static variable*/
static sym_tb_entry *trans_entry;	/*正在翻译的表项*/

static char trans_label[ASM_LABEL_LEN];		/*正在翻译的标签*/
static unsigned short nr_label;	/*记录标签号。用于合成内部标签，使其不重复*/
/*
 * 翻译成汇编文件
 * @param0 src_file: 源文件名
 */
int trans_asm(char *src_file){
	char *needle;
	char dest_file[COMPILE_FILE_NAME_LEN];

	/*修改为源文件名.s*/
	memset(dest_file , 0 , COMPILE_FILE_NAME_LEN);
	strcpy(dest_file , src_file);
	needle = strstr(dest_file , ".");
	if(needle){
		memset(needle , 0 , strlen(needle));
	}
	strcat(dest_file , ".s");

	/*创建输入文件*/
	if((asm_file = fopen(dest_file , "w+")) == NULL){
		printf("can not create %s\n" , dest_file);
		return -1;
	}
	printf("create %s success!\n" , dest_file);

	output_data_section();
	output_rodata_section();
	output_bss_section();
	fputs(".section .text\n" , asm_file);
	fputs(".global _start" , asm_file);	/*默认导出入口函数*/
	output_global();
	fputs("\n" , asm_file);
	trans_code(syntax_tree);
	/*关闭文件*/
	fclose(asm_file);

	return 0;
}


/////////////////////////PRIVATE FUNCTIONS///////////////////////////////////

/*填充汇编数据区 实际是root_table的变量以及数组*/
static void output_data_section(void){
	unsigned char i;
	sym_tb_entry *p;
	sym_tb_entry *next;

	fputs(".section .data\n" , asm_file);	/*首先输出段名*/

	/*遍历该表的所有表头如果表头有表项则遍历其链接的表项串*/
	for(i=0; i<SYM_TB_SIZE; i++){

		if(root_table->entry_heads[i]){	/*如果有表项则遍历该表项串*/
			p = root_table->entry_heads[i];

			while(p){	/*遍历表项串*/

				if(p->entry_type == VAR_ENTRY){	/*如果是变量*/
					if(strcmp(p->type_qualifier , "const") != 0 && strcmp(p->storage_class , "extern") != 0){	/*不能是常量也不能是外部变量*/

						if(strlen(p->nr_pointer) == 0){	/*如果不是指针则需要考察具体类型。否则指针类型作为int类型*/
							if(strcmp(p->type_specific , "char") == 0){	/*字节*/
								fputs("\t" , asm_file);
								fputs(p->asm_label , asm_file);
								fputs(": " , asm_file);
								fputs(".int 0\n" , asm_file);
							}else if(strcmp(p->type_specific , "short") == 0){	/*短整型*/
								fputs("\t" , asm_file);
								fputs(p->asm_label , asm_file);
								fputs(": " , asm_file);
								fputs(".int 0\n" , asm_file);
							}else if(strcmp(p->type_specific , "int") == 0){	/*整型*/
								fputs("\t" , asm_file);
								fputs(p->asm_label , asm_file);
								fputs(": " , asm_file);
								fputs(".int 0\n" , asm_file);
							}else if(strcmp(p->type_specific , "long") == 0){	/*长整型*/
								fputs("\t" , asm_file);
								fputs(p->asm_label , asm_file);
								fputs(": " , asm_file);
								fputs(".long 0\n" , asm_file);
							}else if(strcmp(p->type_specific , "float") == 0){	/*浮点型*/
								fputs("\t" , asm_file);
								fputs(p->asm_label , asm_file);
								fputs(": " , asm_file);
								fputs(".float 0\n" , asm_file);
							}else if(strcmp(p->type_specific , "double") == 0){	/*双精度型*/
								fputs("\t" , asm_file);
								fputs(p->asm_label , asm_file);
								fputs(": " , asm_file);
								fputs(".double 0\n" , asm_file);
							}else{	/*struct类型不处理*/

							}

						}else{	/*指针 地址作为int占用4字节*/
							fputs("\t" , asm_file);
							fputs(p->asm_label , asm_file);
							fputs(": " , asm_file);
							fputs(".int 0\n" , asm_file);
						}

					}

				}	/*end if: var_entry*/

				p = p->next;
			}	/*end while 遍历表项串*/

		}	/*end if*/

	}	/*end for 遍历整个表*/

}

/*填充汇编只读数据区 实际是const_table的常量以及root_table的const常量*/
static void output_rodata_section(void){
	unsigned char i;
	sym_tb_entry *p;
	sym_tb_entry *next;

	fputs(".section .rodata\n" , asm_file);	/*首先输出段名*/

	/*遍历root_table表的所有表头如果表头有表项则遍历其链接的表项串*/
	for(i=0; i<SYM_TB_SIZE; i++){

		if(root_table->entry_heads[i]){	/*如果有表项则遍历该表项串*/
			p = root_table->entry_heads[i];

			while(p){	/*遍历表项串*/

				if(p->entry_type == VAR_ENTRY){	/*如果是变量*/
					if(strcmp(p->type_qualifier , "const") == 0 && strcmp(p->storage_class , "extern") != 0){	/*必须是常量且不能是外部常量*/

						if(strlen(p->nr_pointer) == 0){	/*如果不是指针则需要考察具体类型。否则指针类型作为int类型*/
							if(strcmp(p->type_specific , "char") == 0){	/*字节*/
								fputs("\t" , asm_file);
								fputs(p->asm_label , asm_file);
								fputs(": " , asm_file);
								fputs(".int 0\n" , asm_file);
							}else if(strcmp(p->type_specific , "short") == 0){	/*短整型*/
								fputs("\t" , asm_file);
								fputs(p->asm_label , asm_file);
								fputs(": " , asm_file);
								fputs(".int 0\n" , asm_file);
							}else if(strcmp(p->type_specific , "int") == 0){	/*整型*/
								fputs("\t" , asm_file);
								fputs(p->asm_label , asm_file);
								fputs(": " , asm_file);
								fputs(".int 0\n" , asm_file);
							}else if(strcmp(p->type_specific , "long") == 0){	/*长整型*/
								fputs("\t" , asm_file);
								fputs(p->asm_label , asm_file);
								fputs(": " , asm_file);
								fputs(".long 0\n" , asm_file);
							}else if(strcmp(p->type_specific , "float") == 0){	/*浮点型*/
								fputs("\t" , asm_file);
								fputs(p->asm_label , asm_file);
								fputs(": " , asm_file);
								fputs(".float 0\n" , asm_file);
							}else if(strcmp(p->type_specific , "double") == 0){	/*双精度型*/
								fputs("\t" , asm_file);
								fputs(p->asm_label , asm_file);
								fputs(": " , asm_file);
								fputs(".double 0\n" , asm_file);
							}else{	/*struct类型不处理*/

							}

						}else{	/*指针 地址作为int占用4字节*/
							fputs("\t" , asm_file);
							fputs(p->asm_label , asm_file);
							fputs(": " , asm_file);
							fputs(".int 0\n" , asm_file);
						}

					}

				}	/*end if: var_entry*/

				p = p->next;
			}	/*end while 遍历表项串*/

		}	/*end if*/

	}	/*end for 遍历整个表*/


	/*遍历const_table表的所有表头如果表头有表项则遍历其链接的表项串*/
	for(i=0; i<SYM_TB_SIZE; i++){

		if(const_table->entry_heads[i]){	/*如果有表项则遍历该表项串*/
			p = const_table->entry_heads[i];

			while(p){	/*遍历表项串*/
				fputs("\t" , asm_file);
				fputs(p->asm_label , asm_file);
				if(p->name[0] == '"'){	/*字符串常量*/
					fputs(": .asciz " , asm_file);
					fputs(p->name , asm_file);	/*常量项的名称实际就是字符串常量值(以“ 开始)*/
					fputs("\"\n" , asm_file);
				}else{	/*字符常量*/
					fputs(": .ascii " , asm_file);
					fputs(&(p->name[1]) , asm_file);	/*常量项的名称实际就是字符常量值(以' 开始)*/
					fputs("\n" , asm_file);
				}

				p = p->next;
			}	/*end while 遍历表项串*/

		}	/*end if*/

	}	/*end for 遍历整个表*/

}

/*填充汇编bss数据区 实际是定义为struct的变量或者数组变量*/
static void output_bss_section(void){
	unsigned char i;
	sym_tb_entry *p;
	sym_tb_entry *next;
	char buff[TYPE_SPECIFIC_LEN];
	char *needle;

	fputs(".section .bss\n" , asm_file);	/*首先输出段名*/

	/*遍历root_table表的所有表头如果表头有表项则遍历其链接的表项串*/
	for(i=0; i<SYM_TB_SIZE; i++){

		if(root_table->entry_heads[i]){	/*如果有表项则遍历该表项串*/
			p = root_table->entry_heads[i];

			while(p){	/*遍历表项串*/

				if(strcmp(p->type_qualifier , "const") != 0 && strcmp(p->storage_class , "extern") != 0){	/*必须不是常量且不能是外部变量*/

					if(p->entry_type == VAR_ENTRY){	/*如果是变量则必须结构体*/

						needle = strstr(p->type_specific , ".");

						if(needle){	/*不为空则表明是结构体*/

							if(strcmp(p->storage_class , "static") == 0){	/*判断是静态变量*/
								fputs("\t.lcomm " , asm_file);
							}else{
								fputs("\t.comm " , asm_file);
							}

							memset(buff , 0 , TYPE_SPECIFIC_LEN);
							sprintf(buff , "%d" , p->align_size);	/*这里使用结构体对齐后长度*/

							fputs(p->asm_label , asm_file);
							fputs(" , " , asm_file);
							fputs(buff , asm_file);
							fputs("\n" , asm_file);
						}

					}	/*end if: var_entry*/


					if(p->entry_type == ARRAY_ENTRY){	/*如果是数组*/

						if(strcmp(p->storage_class , "static") == 0){	/*判断是静态变量*/
							fputs("\t.lcomm " , asm_file);
						}else{
							fputs("\t.comm " , asm_file);
						}

						memset(buff , 0 , TYPE_SPECIFIC_LEN);
						sprintf(buff , "%d" , p->align_size);	/*这里记录的是数组单个成员的对齐长度*/

						fputs(p->asm_label , asm_file);
						fputs(" , " , asm_file);
						fputs(buff , asm_file);
						fputs(" * " , asm_file);
						fputs(p->attr.array_len , asm_file);
						fputs("\n" , asm_file);

					}	/*end if: array_entry*/


				}	/*end if: not const*/

				p = p->next;
			}	/*end while 遍历表项串*/

		}	/*end if*/

	}	/*end for 遍历整个表*/

}

/*输入全局过程或者变量*/
static void output_global(void){
	unsigned char i;
	sym_tb_entry *p;
	sym_tb_entry *next;


	/*遍历该表的所有表头如果表头有表项则遍历其链接的表项串*/
	for(i=0; i<SYM_TB_SIZE; i++){

		if(root_table->entry_heads[i]){	/*如果有表项则遍历该表项串*/
			p = root_table->entry_heads[i];

			while(p){	/*遍历表项串*/
				/*凡是不以static修饰的皆可以作为全局变量或过程.extern修饰的外部变量也不用作全局*/

				/*变量或者过程*/
				if(p->entry_type == VAR_ENTRY || p->entry_type == FUNC_ENTRY || p->entry_type == ARRAY_ENTRY){

					if(strcmp(p->storage_class , "static") != 0 && strcmp(p->storage_class , "extern") != 0){
						fputs(" , " , asm_file);
						fputs(p->asm_label , asm_file);
					}

				}
				p = p->next;
			}	/*end while 遍历表项串*/

		}	/*end if*/

	}	/*end for 遍历整个表*/

}

/*翻译代码
 * @return: -1:翻译过程中出错 中止翻译
 * 0: 无错
 */
static int trans_code(syntax_node *node){
	sym_tb_entry *entry;
	sym_tb_entry *entry_tmp;
	int ret;
	char buff[16];	/*缓冲*/
	char *needle;

	if(!node){	/*表示没有该节点*/
		return 0;
	}

	/*根据种类不同进行区分*/
	if(node->kind == START_KIND){	/*起始类别*/
		return trans_code(node->first_child);
	}
	/////////////////////////////////////////////////////////////////////////////////
	if(node->kind == FUNC_KIND){	/*函数类别*/
		if(node->type == FUNC_DECL){	/*如果是函数声明不翻译*/
			return trans_code(node->sibling);
		}
		/*函数定义*/
		entry = get_entry(root_table , node->attr.type_declare.var_name);
		trans_entry = entry;

		if(strcmp(entry->name , "main") == 0){
			fputs("_start:\n" , asm_file);	/*主函数作为入口函数需要_start标志*/
		}else{
			fputs(entry->asm_label , asm_file);
			fputs(":\n" , asm_file);
		}

		/*函数开始的工作*/
		memset(buff , 0 , 16);
		fputs("\tpushl %ebp\n\tmovl %esp , %ebp\n\tsubl $" , asm_file);
		/*开辟局部变量空间*/
		sprintf(buff , "%d" , entry->align_size);
		fputs(buff , asm_file);
		fputs(" , %esp\n\n" , asm_file);

		/*函数主体代码*/
		ret = trans_code(node->second_child);	/*翻译函数体*/
		if(ret == -1){
			return -1;
		}

		/*函数结束后的处理工作*/
		memset(buff , 0 , 16);
		fputs("\n\tmovl %ebp , %esp\n\tpushl %ebp\n\tret\n" , asm_file);


		return trans_code(node->sibling);	/*翻译兄弟*/
	}
	/////////////////////////////////////////////////////////////////////////////////
	if(node->kind == STMT_KIND){
		switch(node->type){
		case STMT:		/*---------------------------------STMT*/
			return trans_code(node->first_child);
			break;
		case EXP_STMT:		/*---------------------------------EXP_STMT*/
			ret = trans_code(node->first_child);
			if(ret == -1){
				return -1;
			}
			return trans_code(node->sibling);

		case LABEL_STMT:
		case SELECT_STMT:
		case ITER_STMT:
			return trans_code(node->sibling);
			break;
		case JUMP_STMT:
			if(strcmp(node->attr.content , "return") == 0){	/*若是返回语句*/
				ret = trans_code(node->first_child);	/*先翻译孩子*/
				if(ret == -1){
					return -1;
				}

				/*函数结束后的处理工作*/
				memset(buff , 0 , 16);
				fputs("\n\tmovl %ebp , %esp\n\tpushl %ebp\n\tret\n" , asm_file);
			}




			return trans_code(node->sibling);
			break;
		case VAR_STMT:	/*直接翻译其兄弟节点*/
		case NULL_STMT:
		case STRUCT_STMT:
			return trans_code(node->sibling);
		case STRUCT_MEMBER:	/*不会遍历到此种节点*/
			printf("line %s: trans error: translate struct member %s\n" , node->line , node->attr.type_declare.var_name);
			return -1;
			break;
		}
	}
	/////////////////////////////////////////////////////////////////////////////////
	if(node->kind == EXP_KIND){
		switch(node->type){
		case SEQ_EXP:
		case ASSIGN_EXP:
		case LOGIC_OR_EXP:
		case LOGIC_AND_EXP:
		case BIT_OR_EXP:
		case BIT_XOR_EXP:
		case BIT_AND_EXP:
		case EQUAL_EXP:
		case RELATION_EXP:
		case SHIFT_EXP:
		case ADDITIVE_EXP:
		case MULTI_EXP:
		case CAST_EXP:
			return 0;
		case UNARY_EXP:	/*---------------------------------UNARY_EXP*/
			if(strlen(node->attr.opt_declare.opt_name) == 0){	/*如果没有操作符*/

				switch(node->attr.opt_declare.opt_content[0]){
				case '"':	/*字符串常量*/
					entry = get_entry(const_table , node->attr.opt_declare.opt_content);	/*根据常量值找到对应的项目*/
					if(!entry){	/*如果还没有找到那么说明出现错误*/
						printf("line: %s: translation error: can not find const %s in const table\n" , node->line , node->attr.opt_declare.opt_content);
						return -1;
					}

					fputs("\tmovl $" , asm_file);	/*将字符串的地址赋予eax返回*/
					fputs(entry->asm_label , asm_file);
					fputs(" , %eax\n" , asm_file);

					return 0;

				case '\'':	/*字符型常量*/
					entry = get_entry(const_table , node->attr.opt_declare.opt_content);	/*根据常量值找到对应的项目*/
					if(!entry){	/*如果还没有找到那么说明出现错误*/
						printf("line: %s: translation error: can not find const %s in const table\n" , node->line , node->attr.opt_declare.opt_content);
						return -1;
					}

					fputs("\txor eax , eax\n\tmovb " , asm_file);	/*将字符放入al*/
					fputs(entry->asm_label , asm_file);
					fputs(" , %al\n" , asm_file);

					return 0;

				case '.': /*实型常量*/
//
					return 0;

				case '#':	/*整型常量*/
					fputs("\tmovl $" , asm_file);	/*将立即数赋予eax返回*/
					fputs(&(node->attr.opt_declare.opt_content[1]) , asm_file);
					fputs(" , %eax\n" , asm_file);

					return 0;

				default:	/*标识符*/

					/*在当前函数的主体中查找*/
					entry = get_entry(trans_entry->attr.function_table.block_table , node->attr.opt_declare.opt_content);
					if(!entry){
						/*如果没有找到则在形参中查找*/
						entry = get_entry(trans_entry->attr.function_table.param_table , node->attr.opt_declare.opt_content);

						if(!entry){	/*如果还没有找到就在根作用域查找*/
							entry = get_entry(root_table , node->attr.opt_declare.opt_content);
						}
					}
					if(!entry){	/*如果还没有找到那么说明出现错误*/
						printf("line: %s: translation error: unknown identifier %s\n" , node->line , node->attr.opt_declare.opt_content);
						return -1;
					}


					/*检测是否结构体成员*/
					if(node->first_child && node->first_child->type == MEMBER_SELECT){	/*是结构体成员*/
						return 0;
						/*找到结构体原型*/
						needle = strstr(entry->type_specific , ".");
						needle++;	/*needle指向向结构体类型名*/
						entry = get_entry(root_table , needle);

						/*enry指向当前描述结构体的项。需要找到结构体成员的类型*/
						entry_tmp = get_entry(entry->attr.struct_table , node->first_child->attr.opt_declare.opt_content);
						entry = entry_tmp;
					}


					/*检测是否函数调用*/
					if(node->first_child && node->first_child->type == ARGUMENT_SPEC){	/*函数调用*/
						return 0;
					}


					/*检测是否是数组*/
					if(entry->entry_type == ARRAY_ENTRY){
						printf("array %s\n" , entry->name);

						if(node->first_child && node->first_child->type == ELEMENT_SELECT){	/*是数组成员*/

						}else{	/*不是使用数组成员 而是数组名*/


						}
						return 0;
					}

					/*普通变量*/
					fputs("\tmovl " , asm_file);
					fputs(entry->asm_label , asm_file);
					fputs(" , %eax\n" , asm_file);
					return 0;

				}	/*end switch*/



			}else{	/*---------------具有操作符*/


			}




		default:
			return 0;
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	return -1;
}






