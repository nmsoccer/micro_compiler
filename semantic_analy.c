/*
 * semantic_analy.c
 *
 *  Created on: 2011-6-1
 *      Author: leiming
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "global.h"
#include "syntax_analy.h"
#include "semantic_analy.h"



/*根符号表。记录公共变量、结构体与函数. */
sym_table *root_table;
sym_table *const_table;
/*静态变量*/
static int sym_addr(char *key);		/*符号表哈希函数。根据输入的字符串得到其在哈希表的地址*/
static int fill_sym_table(sym_table *table , syntax_node *node);	/*根据相应的语法树节点信息填充符号表*/
static unsigned int check_type(syntax_node *node);	/*检查类型*/
static int fill_asm_label_root(sym_table *table , char *prefix);	/*分别为根符号表、函数、结构体符号表填充相应符号在汇编中的标记*/
static int fill_asm_label_function(sym_table *table , char *prefix);
static int fill_asm_label_struct(sym_table *table , char *prefix);
static int fill_asm_label_const(sym_table *table , char *prefix);
static int print_const_table(sym_table *table , char *name);/*打印常量表*/
static int print_sym_table(sym_table *table , char *name);/*打印符号表*/

static sym_table *table_level;	/*用于在函数中指示当前所在的符号表*/
static unsigned int struct_size;	/*用于记录结构体大小*/
static unsigned int struct_align_size;	/*用于记录结构体4字节对齐之后的大小*/

/*
 * 语义分析函数
 */
 int semantic_analy(void){
	 int ret;
	 unsigned int type_ret;

	DUMP_SEM_STR("Ready to semantic analyze...\n");

	/*构造根与常数符号表*/
	root_table = (sym_table *)malloc(sizeof(sym_table));
	memset(root_table , 0 , sizeof(sym_table));

	const_table = (sym_table *)malloc(sizeof(sym_table));
	memset(const_table , 0 , sizeof(sym_table));
	/*填充符号表*/
	ret = fill_sym_table(root_table , syntax_tree);

	if(ret != 0){	/*构造符号表失败*/
		printf("fill sym table failed!\n");
		return -1;
	}

	/*检查类型*/
	printf("checking type......\n");
	type_ret = check_type(syntax_tree);

	if(type_ret & TYPE_ERROR){	/*检查类型出错*/
		return -1;
	}

	printf("type all matched!\n");
	printf("-----------------------------\n");

	/*将符号表的符号填充汇编标签*/
	fill_asm_label_root(root_table , compile_file_name);
	fill_asm_label_const(const_table , compile_file_name);
	/*打印符号表*/
	print_const_table(const_table , "const");
	printf("-----------------------------\n");
	print_sym_table(root_table , "ROOT");
	printf("-----------------------------\n");


	 return 0;
 }



 /*删除符号表及其表项*/
int delete_sym_table(sym_table *table){
	unsigned char i;
	sym_tb_entry *p;
	sym_tb_entry *next;

	if(!table){	/*table为空 返回-1*/
		return -1;
	}

	/*遍历该表的所有表头如果表头有表项则删除其链接的表项串*/
	for(i=0; i<SYM_TB_SIZE; i++){

		if(table->entry_heads[i]){	/*如果有表项则删除该表项串*/
			p = table->entry_heads[i];

			while(p){	/*删除表项串*/
				if(p->entry_type == FUNC_ENTRY){	/*如果是函数或者结构体类型表项需要提前删除表现其实现之符号表*/
					delete_sym_table(p->attr.function_table.param_table);
					delete_sym_table(p->attr.function_table.block_table);
				}else if(p->entry_type == STRUCT_ENTRY){
					delete_sym_table(p->attr.struct_table);
				}

				next = p->next;
				free(p);
				p = next;
			}	/*end while 删除表项串*/

		}	/*end if*/

	}	/*end for 遍历整个表*/

	return 0;
}

/*根据名字获取其在符号表里的表项指针。
 * @parm table: 需要检索的符号表
 * @key: 需要查找的名字
 * @return: NULL 没有找到对应项
 * 					否则是表项指针
 */
sym_tb_entry *get_entry(sym_table *table , char *key){
	int addr;
	sym_tb_entry *p;

	if(!table){	/*表为空*/
		return NULL;
	}

	addr = sym_addr(key);	/*名字的位置与表无关*/

	p = table->entry_heads[addr];	/*检查此表在该位置的表项串*/
	while(p){

		if(strcmp(p->name , key) == 0){	/*某表项名字匹配则找到*/
			return p;
		}

		p = p->next;
	}

	return NULL;	/*没有找到*/
}



/*根据相应的语法树节点信息填充符号表
 * @param table: 欲填充的符号表
 * @param node: 搜寻到的语法树节点
 * @return: -1出错
 * 					0成功
 */
static int fill_sym_table(sym_table *table , syntax_node *node){
	sym_tb_entry *entry;
	sym_tb_entry *entry_tmp;
	int entry_addr;	/*表项在表中的地址*/
	int ret;	/*返回值*/

	char *needle;

	if(!table){
		return 0;
	}
	if(!node){
		return 0;
	}

	/*开始种类节点直接寻找其子节点*/
	if(node->kind == START_KIND){
		if(node->first_child){
			ret = fill_sym_table(table , node->first_child);
		}
		return ret;
	}

	/*函数类型会产生新的符号表 */
	if(node->kind == FUNC_KIND){
		entry = get_entry(table , node->attr.type_declare.var_name);	/*根据函数名获取该表中对应表项*/

		if(entry == NULL){	/*如果表项为空则新建立一个表项和符号表并插入之*/
			entry = (sym_tb_entry *)malloc(sizeof(sym_tb_entry));
			memset(entry , 0 , sizeof(sym_tb_entry));

			/*填充该名字的相关信息*/
			entry->entry_type = FUNC_ENTRY;	/*表项类型*/
			entry->src_addr = table->src_addr;
			table->src_addr++;

			strcpy(entry->name , node->attr.type_declare.var_name);	/*名字的存储类型、限定、指针深度、具体类型*/
			strcpy(entry->storage_class , node->attr.type_declare.storage_class);
			strcpy(entry->type_qualifier , node->attr.type_declare.type_qualifier);
			strcpy(entry->nr_pointer , node->attr.type_declare.nr_pointer);
			strcpy(entry->type_specific , node->attr.type_declare.type_specific);

			/*建造该函数的符号表*/
			entry->attr.function_table.param_table = (sym_table *)malloc(sizeof(sym_table));	/*无论是声明还是定义都需要构造参数表*/
			memset(entry->attr.function_table.param_table , 0 , sizeof(sym_table));
			((sym_table *)entry->attr.function_table.param_table)->parent_entry = entry;
			if(node->type == FUNC_DEFINE){	/*如果是函数定义则需要构造主体表*/
				entry->attr.function_table.block_table = (sym_table *)malloc(sizeof(sym_table));
				memset(entry->attr.function_table.block_table , 0 , sizeof(sym_table));
				((sym_table *)entry->attr.function_table.block_table)->parent_entry = entry;
			}

			/*将名字插入到当前表的表项串中*/
			entry_addr = sym_addr(entry->name);

			if(table->entry_heads[entry_addr] == NULL)	{	/*这是第一项*/
				entry->next = NULL;
				table->entry_heads[entry_addr] = entry;
			}else{	/*已经有了表项*/
				entry->next = table->entry_heads[entry_addr]->next;
				table->entry_heads[entry_addr]->next = entry;
			}
		}else{	/*该表项之前已经出现则需要仔细核对每一项*/

			if(entry->entry_type != FUNC_ENTRY){	/*如果发现不是函数类型表项证明出现了重名*/
				printf("line %s: semantic error: name %s belongs to different type!\n" , node->line , entry->name);
				return -1;
			}

			if(strcmp(entry->type_qualifier , node->attr.type_declare.type_qualifier) != 0){	/*比较类型限定符*/
				printf("line %s: semantic error: name %s type qualifier differs: %s and %s!\n" , node->line , entry->name , node->attr.type_declare.type_qualifier
							 , entry->type_qualifier);
				return -1;
			}

			if(strcmp(entry->nr_pointer , node->attr.type_declare.nr_pointer) != 0){	/*比较指针层数*/
				printf("line %s: semantic error: name %s pointer differs: %s and %s!\n" , node->line , entry->name , node->attr.type_declare.nr_pointer
							 , entry->nr_pointer);
				return -1;
			}

			if(strcmp(entry->type_specific , node->attr.type_declare.type_specific) != 0){	/*比较具体类型*/
				printf("line %s: semantic error: function %s type  differs: %s and %s!\n" , node->line , entry->name , node->attr.type_declare.type_specific
							 , entry->type_specific);
				return -1;
			}


			if(node->type == FUNC_DECL && !entry->attr.function_table.block_table){	/*函数声明遇见了之前声明*/
				printf("line %s: semantic error: function %s multi declare!\n" , node->line , entry->name);
				return -1;
			}

			if(node->type == FUNC_DEFINE && entry->attr.function_table.block_table){	/*函数定义遇见了之前定义*/
				printf("line %s: semantic error: function %s multi define!\n" , node->line , entry->name);
				return -1;
			}


			/*没有出现错误 则可能出现函数定义遇见之前的声明或者声明遇见定义*/

			if(node->type == FUNC_DEFINE){	/*定义遇见之前的函数声明*/
				if(!entry->attr.function_table.block_table){	/*函数定义时发现主体表还未定义则构造之*/
					entry->attr.function_table.block_table = (sym_table *)malloc(sizeof(sym_table));
					memset(entry->attr.function_table.block_table , 0 , sizeof(sym_table));
					((sym_table *)entry->attr.function_table.block_table)->parent_entry = entry;
				}
				/*不需要再填充形参*/
				ret = fill_sym_table(entry->attr.function_table.block_table , node->second_child);	/*用函数体成员填充函数体符号表*/
				if(ret == -1){
					return ret;
				}

				/*函数节点其子孩子在此时已经处理完。只需考察其兄弟孩子即可*/
				return  fill_sym_table(table , node->sibling);
			}


			if(node->type == FUNC_DECL){	/*函数声明遇见了之前的定义*/
				return fill_sym_table(table , node->sibling);	/*已经填充完毕直接考察其兄弟*/
			}

		}	/*end check multi define or declare*/

		/*第一次出现函数定义或声明的情况*/
		if(node->type == FUNC_DECL){	/*函数声明节点的first_child子串为形参*/
			ret = fill_sym_table(entry->attr.function_table.param_table , node->first_child); /*用函数形参成员填充函数体符号表*/
			if(ret == -1){	/*出错*/
				return -1;
			}

			/*函数节点其子孩子在此时已经处理完。只需考察其兄弟孩子即可*/
			return  fill_sym_table(table , node->sibling);
		}

		if(node->type == FUNC_DEFINE){	/*函数定义节点的first_child子串为参数；second_child为定义主体*/
			ret = fill_sym_table(entry->attr.function_table.param_table , node->first_child);	/*用函数形参成员填充函数体符号表*/
			if(ret == -1){	/*出错直接返回*/
				return ret;
			}
			ret = fill_sym_table(entry->attr.function_table.block_table , node->second_child);	/*用函数体成员填充函数体符号表*/
			if(ret == -1){
				return ret;
			}

			/*函数节点其子孩子在此时已经处理完。只需考察其兄弟孩子即可*/
			return  fill_sym_table(table , node->sibling);

		}
	}


	/*语句类型*/
	if(node->kind == STMT_KIND){

		if(node->type == STRUCT_STMT){	/*结构体定义会产生新的符号表*/
			entry = get_entry(table , node->attr.content);	/*根据结构体类型名获取该表中对应表项*/

			if(entry == NULL){	/*如果表项为空则新建立一个表项和符号表并插入之*/
				entry = (sym_tb_entry *)malloc(sizeof(sym_tb_entry));
				memset(entry , 0 , sizeof(sym_tb_entry));

				/*填充该名字的相关信息*/
				entry->entry_type = STRUCT_ENTRY;	/*表项类型*/
				entry->src_addr = table->src_addr;
				table->src_addr++;

				strcpy(entry->name , node->attr.content);	/*名字*/

				/*建造该结构体的符号表*/
				entry->attr.struct_table = (sym_table *)malloc(sizeof(sym_table));
				memset(entry->attr.struct_table , 0 , sizeof(sym_table));
				((sym_table *)entry->attr.struct_table)->parent_entry = entry;

				/*将名字插入到当前表的表项串中*/
				entry_addr = sym_addr(entry->name);

				if(table->entry_heads[entry_addr] == NULL)	{	/*这是第一项*/
					entry->next = NULL;
					table->entry_heads[entry_addr] = entry;
				}else{	/*已经有了表项*/
					entry->next = table->entry_heads[entry_addr]->next;
					table->entry_heads[entry_addr]->next = entry;
				}

			}else{
				/*出现了重名*/
				printf("line %s: semantic error: name %s is already existed!\n" , node->line , entry->name);
				return -1;
			}

			/*结构体类型定义节点的first_child子串为内部成员*/
			struct_size = 0;	/*用于记录当前结构体的大小*/
			struct_align_size = 0;
			ret = fill_sym_table(entry->attr.struct_table , node->first_child);	/*用结构体成员填充结构体符号表*/
			if(ret == -1){
				return ret;
			}

			entry->size = struct_size;	/*在遍历结束结构体成员之后设置当前结构体大小*/
			entry->align_size = struct_align_size;

			/*结构体类型定义节点其子孩子在此时已经处理完。只需考察其兄弟孩子即可*/
			return fill_sym_table(table , node->sibling);
		}


		if(node->type == STRUCT_MEMBER){	/*结构体成员*/
			entry = get_entry(table , node->attr.type_declare.var_name);	/*根据结构体类型名获取该表中对应表项*/

			if(entry == NULL){	/*如果表项为空则新建立一个表项和符号表并插入之*/
				entry = (sym_tb_entry *)malloc(sizeof(sym_tb_entry));
				memset(entry , 0 , sizeof(sym_tb_entry));

				/*填充该名字的相关信息*/
				if(node->attr.type_declare.array_len[0] != 0x00){
					entry->entry_type = ARRAY_ENTRY;	/*数组类型*/
					strcpy(entry->attr.array_len , node->attr.type_declare.array_len);
				}else{
					entry->entry_type = VAR_ENTRY;	/*变量类型*/
				}
				entry->src_addr = table->src_addr;
				table->src_addr++;

				strcpy(entry->name , node->attr.type_declare.var_name);	/*名字、指针深度、具体类型*/
				strcpy(entry->nr_pointer , node->attr.type_declare.nr_pointer);
				strcpy(entry->type_specific , node->attr.type_declare.type_specific);
				/*设置当前成员长度*/
				if(strlen(entry->nr_pointer) == 0){	/*如果不是指针类型则需要考察具体类型*/
					if(strcmp(entry->type_specific , "char") == 0){	/*字节*/
						entry->size = CHAR_SIZE;
						entry->align_size = ALIGN_BYTE;
					}else if(strcmp(entry->type_specific , "short") == 0){	/*短整型*/
						entry->size = SHORT_SIZE;
						entry->align_size = ALIGN_BYTE;
					}else if(strcmp(entry->type_specific , "int") == 0){	/*整型*/
						entry->size = INT_SIZE;
						entry->align_size = ALIGN_BYTE;
					}else if(strcmp(entry->type_specific , "long") == 0){	/*长整型*/
						entry->size = LONG_SIZE;
						entry->align_size = ALIGN_BYTE;
					}else if(strcmp(entry->type_specific , "float") == 0){	/*浮点型*/
						entry->size = FLOAT_SIZE;
						entry->align_size = ALIGN_BYTE;
					}else if(strcmp(entry->type_specific , "double") == 0){	/*双精度型*/
						entry->size = DOUBLE_SIZE;
						entry->align_size = DOUBLE_SIZE;	/*double长度是8字节已经4字节对齐*/
					}else{	/*struct类型*/
						needle = strstr(entry->type_specific , ".");
						if(needle){	/*必然是struct.xx*/
							needle++;
							entry_tmp = get_entry(root_table , needle);	/*寻找struct类型原型*/
							if(!entry_tmp || entry_tmp->size == 0){ /*没有找到 说明之前并未定义目标struct类型*/
								printf("line %s: semantic error: type %s unresoved!\n" , node->line , needle);
								return -1;
							}else{	/*找到类型原型 则将其大小赋予当前成员变量大小*/
								entry->size = entry_tmp->size;
								entry->align_size = entry_tmp->align_size;
							}
						}
					}

				}else{/*指针类型则是INT_SIZE*/
					entry->size = INT_SIZE;
					entry->align_size = ALIGN_BYTE;
				}

				struct_size += entry->size;	/*修改当前结构体大小*/
				struct_align_size += entry->align_size;

				/*将名字插入到当前表的表项串中*/
				entry_addr = sym_addr(entry->name);

				if(table->entry_heads[entry_addr] == NULL)	{	/*这是第一项*/
					entry->next = NULL;
					table->entry_heads[entry_addr] = entry;
				}else{	/*已经有了表项*/
					entry->next = table->entry_heads[entry_addr]->next;
					table->entry_heads[entry_addr]->next = entry;
				}
			}else{
				/*出现了重名*/
				printf("line %s: semantic error: name %s is already existed!\n" , node->line , entry->name);
				return -1;
			}

			/*结构体成员节点只有first_child孩子*/
			if(node->first_child){
				return fill_sym_table(table , node->first_child);
			}
			return 0;
		}

		if(node->type == VAR_STMT){	/*变量定义*/
			entry = get_entry(table , node->attr.type_declare.var_name);	/*根据变量名获取该表中对应表项*/

			if(entry == NULL){	/*如果表项为空则新建立一个表项和符号表并插入之*/
				entry = (sym_tb_entry *)malloc(sizeof(sym_tb_entry));
				memset(entry , 0 , sizeof(sym_tb_entry));

				/*填充该名字的相关信息*/
				if(node->attr.type_declare.array_len[0] != 0x00){
					entry->entry_type = ARRAY_ENTRY;	/*数组类型*/
					strcpy(entry->attr.array_len , node->attr.type_declare.array_len);
				}else{
					entry->entry_type = VAR_ENTRY;	/*变量类型*/
				}
				entry->src_addr = table->src_addr;
				table->src_addr++;

				strcpy(entry->name , node->attr.type_declare.var_name);	/*名字的存储类型、限定、指针深度、具体类型*/
				strcpy(entry->storage_class , node->attr.type_declare.storage_class);
				strcpy(entry->type_qualifier , node->attr.type_declare.type_qualifier);
				strcpy(entry->nr_pointer , node->attr.type_declare.nr_pointer);
				strcpy(entry->type_specific , node->attr.type_declare.type_specific);
				/*设置当前成员长度*/
				if(strlen(entry->nr_pointer) == 0){	/*如果不是指针类型则需要考察具体类型*/
					if(strcmp(entry->type_specific , "char") == 0){	/*字节*/
						entry->size = CHAR_SIZE;
						entry->align_size = ALIGN_BYTE;
					}else if(strcmp(entry->type_specific , "short") == 0){	/*短整型*/
						entry->size = SHORT_SIZE;
						entry->align_size = ALIGN_BYTE;
					}else if(strcmp(entry->type_specific , "int") == 0){	/*整型*/
						entry->size = INT_SIZE;
						entry->align_size = ALIGN_BYTE;
					}else if(strcmp(entry->type_specific , "long") == 0){	/*长整型*/
						entry->size = LONG_SIZE;
						entry->align_size = ALIGN_BYTE;
					}else if(strcmp(entry->type_specific , "float") == 0){	/*浮点型*/
						entry->size = FLOAT_SIZE;
						entry->align_size = ALIGN_BYTE;
					}else if(strcmp(entry->type_specific , "double") == 0){	/*双精度型*/
						entry->size = DOUBLE_SIZE;
						entry->align_size = DOUBLE_SIZE;
					}else{	/*struct类型*/
						needle = strstr(entry->type_specific , ".");
						if(needle){	/*必然是struct.xx*/
							needle++;
							entry_tmp = get_entry(root_table , needle);	/*寻找struct类型原型*/
							if(!entry_tmp || entry_tmp->size == 0){ /*没有找到 说明之前并未定义目标struct类型*/
								printf("line %s: semantic error: type %s unresoved!\n" , node->line , needle);
								return -1;
							}else{	/*找到类型原型 则将其大小赋予当前成员变量大小*/
								entry->size = entry_tmp->size;
								entry->align_size = entry_tmp->align_size;
							}
						}
					}

				}else{/*指针类型则是INT_SIZE*/
					entry->size = INT_SIZE;
					entry->align_size = ALIGN_BYTE;
				}

				/*将名字插入到当前表的表项串中*/
				entry_addr = sym_addr(entry->name);

				if(table->entry_heads[entry_addr] == NULL)	{	/*这是第一项*/
					entry->next = NULL;
					table->entry_heads[entry_addr] = entry;
				}else{	/*已经有了表项*/
					entry->next = table->entry_heads[entry_addr]->next;
					table->entry_heads[entry_addr]->next = entry;
				}
			}else{

				if(entry->entry_type != VAR_ENTRY){	/*如果发现不是变量类型表项证明出现了重名*/
					printf("line %s: semantic error: name %s is already existed!\n" , node->line , entry->name);
					return -1;
				}

				if(table != root_table){	/*只有在全局作用域下才可能出现相同名字变量与定义同时存在的情况。其他作用域只能有唯一变量名*/
					printf("line %s: semantic error: name %s is already existed!\n" , node->line , entry->name);
					return -1;
				}

				if(strcmp(entry->type_qualifier , node->attr.type_declare.type_qualifier) != 0){	/*比较类型限定符*/
					printf("line %s: semantic error: name %s type qualifier differs: %s and %s!\n" , node->line , entry->name , node->attr.type_declare.type_qualifier
								 , entry->type_qualifier);
					return -1;
				}

				if(strcmp(entry->nr_pointer , node->attr.type_declare.nr_pointer) != 0){	/*比较指针层数*/
					printf("line %s: semantic error: name %s pointer differs: %s and %s!\n" , node->line , entry->name , node->attr.type_declare.nr_pointer
								 , entry->nr_pointer);
					return -1;
				}

				if(strcmp(entry->type_specific , node->attr.type_declare.type_specific) != 0){	/*比较具体类型*/
					printf("line %s: semantic error: name %s type  differs: %s and %s!\n" , node->line , entry->name , node->attr.type_declare.type_specific
								 , entry->type_specific);
					return -1;
				}

				if(strcmp(entry->storage_class , node->attr.type_declare.storage_class) == 0){	/*若存储类型也相等说明了重复定义或声明*/
					printf("line %s: semantic error: name %s multi define!\n" , node->line , entry->name);
					return -1;
				}

				/*无错 则可能出现变量定义遇见之前的声明或者声明遇见定义*/
			}

			/*变量节点的子节点与兄弟节点与其相同符号表作用域*/
			if(node->first_child){
				ret = fill_sym_table(table , node->first_child);
				if(ret == -1){
					return -1;
				}
			}
			if(node->sibling){
				ret = fill_sym_table(table , node->sibling);
				if(ret == -1){
					return -1;
				}
			}
			return 0;
		}

		if(node->type == STMT){	/*普通节点*/
			if(node->first_child){
				return fill_sym_table(table , node->first_child);
			}
			return 0;
		}

		/*其他类型的语句节点考察所有孩子及兄弟*/
		if(node->first_child){
			ret = fill_sym_table(table , node->first_child);
			if(ret == -1){	/*出错直接返回*/
				return -1;
			}
		}
		if(node->second_child){
			ret = fill_sym_table(table , node->second_child);
			if(ret == -1){	/*出错直接返回*/
				return -1;
			}
		}
		if(node->third_child){
			ret = fill_sym_table(table , node->third_child);
			if(ret == -1){	/*出错直接返回*/
				return -1;
			}
		}
		if(node->last_child){
			ret = fill_sym_table(table , node->last_child);
			if(ret == -1){	/*出错直接返回*/
				return -1;
			}
		}
		return fill_sym_table(table , node->sibling);

	}

	/*表达式类型*/
	if(node->kind == EXP_KIND){

		if(node->type == UNARY_EXP){	/*只考察一元表达式记录的字符和字符串常量*/

			if(strlen(node->attr.opt_declare.opt_name) == 0){

				if(node->attr.opt_declare.opt_content[0] == '\'' || node->attr.opt_declare.opt_content[0] == '"'){/*字符或字符串常量*/

					entry = get_entry(const_table , node->attr.opt_declare.opt_content);	/*根据常量值获取该表中对应表项*/

					if(entry == NULL){	/*如果表项为空则新建立一个表项和符号表并插入之*/
						entry = (sym_tb_entry *)malloc(sizeof(sym_tb_entry));
						memset(entry , 0 , sizeof(sym_tb_entry));

						/*填充该名字的相关信息*/
						entry->entry_type = CONST_ENTRY;
						entry->src_addr = const_table->src_addr;
						const_table->src_addr++;

						strcpy(entry->name , node->attr.opt_declare.opt_content);	/*存储常量值*/

						/*将名字插入到当前表的表项串中*/
						entry_addr = sym_addr(entry->name);

						if(const_table->entry_heads[entry_addr] == NULL)	{	/*这是第一项*/
							entry->next = NULL;
							const_table->entry_heads[entry_addr] = entry;
						}else{	/*已经有了表项*/
							entry->next = const_table->entry_heads[entry_addr]->next;
							const_table->entry_heads[entry_addr]->next = entry;
						}
					}	/*end if 建立新项*/


				}	/*end if 字符或字符串常量*/

				/*其他类型常量不记录*/
			}

		}	/*一元操作符*/


		/*考察所有孩子及兄弟*/
		if(node->first_child){
			ret = fill_sym_table(table , node->first_child);
			if(ret == -1){	/*出错直接返回*/
				return -1;
			}
		}
		if(node->second_child){
			ret = fill_sym_table(table , node->second_child);
			if(ret == -1){	/*出错直接返回*/
				return -1;
			}
		}
		if(node->third_child){
			ret = fill_sym_table(table , node->third_child);
			if(ret == -1){	/*出错直接返回*/
				return -1;
			}
		}
		if(node->last_child){
			ret = fill_sym_table(table , node->last_child);
			if(ret == -1){	/*出错直接返回*/
				return -1;
			}
		}
		return fill_sym_table(table , node->sibling);
	}	/*表达式节点*/


	/*never run here*/
	printf("line %s: fatal error: unknown kind node\n" , node->line);
	return -1;
}

/*检查类型
 * @return: 类型信息
*/
static unsigned int check_type(syntax_node *node){
	unsigned char flag_array_name;	/*是否数组名*/

	unsigned int type_ret;	/*返回类型*/
	unsigned int left_type;	/*左子类型*/
	unsigned int right_type;	/*右子类型*/
	unsigned int nr_pointer;	/*指针层次*/

	char buff[OPT_CONTENT_LEN];	/*操作符内容缓冲*/
	char *needle;	/*分割字符串的标识*/

	sym_tb_entry *entry;	/*标识所在符号表的项*/
	sym_tb_entry *entry_tmp;

	if(!node){	/*节点为空返回无类型*/
		return NO_TYPE;
	}

	flag_array_name = 0;

	type_ret = NO_TYPE;
	switch(node->kind){
	case START_KIND:		/*START_KIND*/
		if(node->first_child){
			type_ret = check_type(node->first_child);
		}
		return type_ret;
		break;	/*break START_KIND*/

	case FUNC_KIND:		/*FUNC_KIND*/
		if(node->type == FUNC_DECL){

			return check_type(node->sibling);	/*函数声明直接检查其兄弟*/

		}else if(node->type == FUNC_DEFINE){	/*函数定义先检查函数体然后检查其兄弟*/

			entry = get_entry(root_table , node->attr.type_declare.var_name);/*进入函数定义需要改变符号表*/
			table_level = entry->attr.function_table.block_table;

			type_ret = check_type(node->second_child);
			if(type_ret & ERROR_MASK){	/*如果出错就返回*/
				return type_ret;
			}

			table_level = root_table;		/*从函数定义返回根符号表*/
			return check_type(node->sibling);
		}else{
			/*never run here*/
		}
		break;	/*break FUNC_KIND*/

	case STMT_KIND:	/*STMT_KIND*/

		switch(node->type){

		case STMT:
			return check_type(node->first_child);	/*直接检查孩子。该节点并无兄弟*/

		case STRUCT_STMT:
		case NULL_STMT:
			return check_type(node->sibling);	/*直接检查兄弟*/

		case LABEL_STMT:
		case JUMP_STMT:
		case VAR_STMT:
		case EXP_STMT:
			type_ret = check_type(node->first_child);	/*首先检查孩子*/
			if(type_ret & ERROR_MASK){	/*出错*/
				return type_ret;
			}
			return check_type(node->sibling);	/*最后检查兄弟*/

		case SELECT_STMT:
			type_ret = check_type(node->first_child);	/*首先检查孩子*/
			if(type_ret & ERROR_MASK){	/*出错*/
				return type_ret;
			}
			type_ret = check_type(node->second_child);	/*之后检查第二个孩子*/
			if(type_ret & ERROR_MASK){	/*出错*/
				return type_ret;
			}
			type_ret = check_type(node->third_child);	/*之后检查第三个孩子(可能有)*/
			if(type_ret & ERROR_MASK){	/*出错*/
				return type_ret;
			}
			return check_type(node->sibling);	/*最后检查兄弟*/

		case ITER_STMT:
			type_ret = check_type(node->first_child);	/*首先检查孩子*/
			if(type_ret & ERROR_MASK){	/*出错*/
				return type_ret;
			}
			type_ret = check_type(node->second_child);	/*之后检查第二个孩子*/
			if(type_ret & ERROR_MASK){	/*出错*/
				return type_ret;
			}
			type_ret = check_type(node->third_child);	/*之后检查第三个孩子(可能有)*/
			if(type_ret & ERROR_MASK){	/*出错*/
				return type_ret;
			}
			type_ret = check_type(node->last_child);	/*之后检查第最后一个孩子(可能有)*/
			if(type_ret & ERROR_MASK){	/*出错*/
				return type_ret;
			}
			return check_type(node->sibling);	/*最后检查兄弟*/

		default:	/*will never happen*/
			DUMP_ERR("fatal error happened!\n");
			type_ret = TYPE_ERROR;
			return type_ret;
		}
		break;	/*break STMT_KIND*/

	case EXP_KIND:

		switch(node->type){

		case SEQ_EXP:
			left_type = check_type(node->first_child);	/*首先检查大孩子*/
			if(left_type & ERROR_MASK){	/*出错*/
				return left_type;
			}

			right_type = check_type(node->second_child);	/*之后检查第二个孩子*/
			if(right_type & ERROR_MASK){	/*出错*/
				return right_type;
			}
			return left_type;	/*返回左子类型*/

		case ASSIGN_EXP:
			left_type = check_type(node->first_child);	/*首先检查大孩子*/
			if(left_type & ERROR_MASK){	/*出错*/
				return left_type;
			}
			if((left_type & STRUCT_TYPE) && !(left_type & POINTER_MASK)){	/*不能直接处理结构体*/
				printf("line: %s: semantic error: illegal operator %s to struct \n" , node->line , node->attr.opt_declare.opt_name);
				left_type |= TYPE_ERROR;
				return left_type;
			}
			if(left_type & CONST_QF){	/*不能赋值给常量*/
				printf("line: %s: semantic error: can not assign value to const number \n" , node->line);
				left_type |= TYPE_ERROR;
				return left_type;
			}

			right_type = check_type(node->second_child);	/*之后检查第二个孩子*/
			if(right_type & ERROR_MASK){	/*出错*/
				return right_type;
			}
			if((right_type & STRUCT_TYPE) && !(right_type & POINTER_MASK)){	/*不能直接处理结构体*/
				printf("line: %s: semantic error: illegal operator %s to struct \n" , node->line , node->attr.opt_declare.opt_name);
				right_type |= TYPE_ERROR;
				return right_type;
			}

			if((left_type & ~QUALIFIER_MASK) != (right_type & ~QUALIFIER_MASK)){	/*若左右两边类型除去限定符之外不相等那么发出错误消息*/
				printf("line %s: semantic error: type not mach!\n" , node->line);
				left_type |= ERROR_MASK;
			}
			return left_type;	/*返回左子类型*/

		case LOGIC_OR_EXP:
		case LOGIC_AND_EXP:
		case BIT_OR_EXP:
		case BIT_XOR_EXP:
		case BIT_AND_EXP:
		case SHIFT_EXP:
		case ADDITIVE_EXP:
		case MULTI_EXP:
			left_type = check_type(node->first_child);	/*首先检查大孩子*/
			if(left_type & ERROR_MASK){	/*出错*/
				return left_type;
			}
			if((left_type & STRUCT_TYPE) && !(left_type & POINTER_MASK)){	/*不能直接处理结构体*/
				printf("line: %s: semantic error: illegal operator %s to struct \n" , node->line , node->attr.opt_declare.opt_name);
				left_type |= TYPE_ERROR;
				return left_type;
			}

			right_type = check_type(node->second_child);	/*之后检查第二个孩子*/
			if(right_type & ERROR_MASK){	/*出错*/
				return right_type;
			}
			if((right_type & STRUCT_TYPE) && !(right_type & POINTER_MASK)){	/*不能直接处理结构体*/
				printf("line: %s: semantic error: illegal operator %s to struct \n" , node->line , node->attr.opt_declare.opt_name);
				right_type |= TYPE_ERROR;
				return right_type;
			}

			if((left_type & ~QUALIFIER_MASK) != (right_type & ~QUALIFIER_MASK)){	/*若左右两边类型除去限定符之外不相等那么发出错误消息*/
				printf("line %s: semantic error: type not mach!\n" , node->line);
				left_type |= ERROR_MASK;
			}
			return left_type;	/*返回左子类型*/


		case EQUAL_EXP:
		case RELATION_EXP:
			left_type = check_type(node->first_child);	/*首先检查大孩子*/
			if(left_type & ERROR_MASK){	/*出错*/
				return left_type;
			}
			if((left_type & STRUCT_TYPE) && !(left_type & POINTER_MASK)){	/*不能直接比较结构体*/
				printf("line: %s: semantic error: illegal operator %s to struct \n" , node->line , node->attr.opt_declare.opt_name);
				left_type |= TYPE_ERROR;
				return left_type;
			}

			right_type = check_type(node->second_child);	/*之后检查第二个孩子*/
			if(right_type & ERROR_MASK){	/*出错*/
				return right_type;
			}
			if((right_type & STRUCT_TYPE) && !(right_type & POINTER_MASK)){	/*不能直接比较结构体*/
				printf("line: %s: semantic error: illegal operator %s to struct \n" , node->line , node->attr.opt_declare.opt_name);
				right_type |= TYPE_ERROR;
				return right_type;
			}

			if((left_type & ~QUALIFIER_MASK) != (right_type & ~QUALIFIER_MASK)){	/*若左右两边类型除去限定符之外不相等那么发出错误消息*/
				printf("line %s: semantic error: type not mach!\n" , node->line);
				left_type |= ERROR_MASK;
			}
			left_type &= ~TYPE_MASK;
			left_type |= INT_TYPE;
			return left_type;	/*必须返回整型*/


		case CAST_EXP:
			type_ret = check_type(node->first_child);
			printf("it is:%d\n" , type_ret);
			if(type_ret & ERROR_MASK){	/*出错*/
				return type_ret;
			}

			memset(buff , 0 , OPT_CONTENT_LEN);
			strcpy(buff , node->attr.opt_declare.opt_content);/*形如type$$ 或者 struct.struct_type 或者 struct.struct_type$$*/
			printf("buff is:%s\n" , buff);
			needle = strstr(buff , "$");	/*首先找到指针层次*/
			if(needle){
				nr_pointer = strlen(needle);
				needle[0] = 0x00;
				printf("buff is:%s\n" , buff);
			}else{
				nr_pointer = 0;
			}

			needle = strstr(buff , ".");	/*划分类型 用于区分type 与struct.struct_type*/
			if(needle){
				needle[0] = 0x00;
			}


			if(strcmp(buff , "void") == 0){	/*转换成void类型*/
				type_ret &= ~TYPE_MASK;
				type_ret |= VOID_TYPE;
			}else if(strcmp(buff , "char") == 0){ /*转换成char类型*/
				type_ret &= ~TYPE_MASK;
				type_ret |= CHAR_TYPE;
			}else if(strcmp(buff , "short") == 0){ /*转换成short类型*/
				type_ret &= ~TYPE_MASK;
				type_ret |= SHORT_TYPE;
			}else if(strcmp(buff , "int") == 0){ /*转换成int类型*/
				type_ret &= ~TYPE_MASK;
				type_ret |= INT_TYPE;
			}else if(strcmp(buff , "long") == 0){ /*转换成long类型*/
				type_ret &= ~TYPE_MASK;
				type_ret |= LONG_TYPE;
			}else if(strcmp(buff , "float") == 0){ /*转换成float类型*/
				type_ret &= ~TYPE_MASK;
				type_ret |= FLOAT_TYPE;
			}else if(strcmp(buff , "double") == 0){ /*转换成double类型*/
				type_ret &= ~TYPE_MASK;
				type_ret |= DOUBLE_TYPE;
			}else{ /*转换成struct类型*/
				type_ret &= ~TYPE_MASK;
				type_ret |= STRUCT_TYPE;
			}

			printf("nr_pointer is:%d\n" , nr_pointer);

			if(nr_pointer == 0){	/*考察指针层数*/

			}else if(nr_pointer == 1){	/*一级指针*/
				type_ret &= ~POINTER_MASK;
				type_ret |= POINTER_1L;
			}else if(nr_pointer == 2){	/*二级指针*/
				type_ret &= ~POINTER_MASK;
				type_ret |= POINTER_2L;
			}else if(nr_pointer == 3){	/*三级指针*/
				type_ret &= ~POINTER_MASK;
				type_ret |= POINTER_3L;
			}else if(nr_pointer == 4){	/*四级指针*/
				type_ret &= ~POINTER_MASK;
				type_ret |= POINTER_4L;
			}else{
				;
			}
			return type_ret;


		case UNARY_EXP:
			if(strlen(node->attr.opt_declare.opt_name) != 0){	/*有操作符的一元表达式*/
				if(strcmp(node->attr.opt_declare.opt_name , "sizeof") == 0){	/*如果是sizeof则返回整型*/
					type_ret &= NO_TYPE;
					type_ret |= INT_TYPE;
					return type_ret;
				}else if(strcmp(node->attr.opt_declare.opt_name , "()") == 0){	/*如果是()则返回first_child的类型*/

					return check_type(node->first_child);

				}else{	/*普通的一元操作符*/
					entry = get_entry(table_level , node->attr.opt_declare.opt_content);	/*在当前符号表或者上层符号表查找标识符信息*/
					if(!entry){

						if(table_level->parent_entry && table_level->parent_entry->entry_type == FUNC_ENTRY){	/*这可能是函数的定义域*/
							/*考察形参*/
							entry = get_entry(table_level->parent_entry->attr.function_table.param_table , node->attr.opt_declare.opt_content);
						}

						if(!entry){	/*如果还没有找到就在根作用域查找*/
							entry = get_entry(root_table , node->attr.opt_declare.opt_content);
						}
					}

					if(!entry){	/*如果还没有找到那么说明出现错误*/
						printf("line: %s: check type error: unknown identifier %s\n" , node->line , node->attr.opt_declare.opt_content);
						type_ret &= NO_TYPE;
						type_ret |= TYPE_ERROR;
						return type_ret;
					}

					/*检测是否结构体成员*/
					if(node->first_child && node->first_child->type == MEMBER_SELECT){	/*是结构体成员*/
						/*找到结构体原型*/
						memset(buff , 0 , OPT_CONTENT_LEN);
						strcpy(buff , entry->type_specific);
						needle = strstr(buff , ".");
						needle++;	/*needle指向向结构体类型名*/
						entry = get_entry(root_table , needle);

						/*enry指向当前描述结构体的项。需要找到结构体成员的类型*/
						entry_tmp = get_entry(entry->attr.struct_table , node->first_child->attr.opt_declare.opt_content);
						entry = entry_tmp;
					}

					/*检测是否是数组*/
					if(entry->entry_type == ARRAY_ENTRY){
						printf("array %s\n" , entry->name);

						if(node->first_child && node->first_child->type == ELEMENT_SELECT){	/*是数组成员*/
							printf("array element!\n");
						}else{	/*不是使用数组成员 而是数组名*/
							printf("array name!\n");
							flag_array_name = 1;
						}

					}


					/*根据相关标识的原始类型进行设置*/
					type_ret &= NO_TYPE;

					if(strcmp(entry->type_qualifier , "static") == 0){	/*设置类型限定符*/
						type_ret |= STATIC_QF;
					}else if(strcmp(entry->type_qualifier , "const") == 0){
						type_ret |= CONST_QF;
					}else{
						;
					}

					if(strcmp(entry->type_specific , "void") == 0){	/*void 类型*/
						type_ret |= VOID_TYPE;
					}else if(strcmp(entry->type_specific , "char") == 0){	/*char 类型*/
						type_ret |= CHAR_TYPE;
					}else if(strcmp(entry->type_specific , "short") == 0){	/*short 类型*/
						type_ret |= SHORT_TYPE;
					}else if(strcmp(entry->type_specific , "int") == 0){	/*int 类型*/
						type_ret |= INT_TYPE;
					}else if(strcmp(entry->type_specific , "long") == 0){	/*long 类型*/
						type_ret |= LONG_TYPE;
					}else if(strcmp(entry->type_specific , "float") == 0){	/*float 类型*/
						type_ret |= FLOAT_TYPE;
					}else if(strcmp(entry->type_specific , "double") == 0){	/*double 类型*/
						type_ret |= DOUBLE_TYPE;
					}else{	/*struct 类型*/
						type_ret |= STRUCT_TYPE;
					}

					if(strlen(entry->nr_pointer) == 0){	/*考察指针层数*/

						if(strcmp(node->attr.opt_declare.opt_name , "$") == 0){	/*取地址*/
							if(flag_array_name){	/*如果是数组名则类型是其成员的上一级地址*/
								type_ret |= POINTER_2L;
							}else{
								type_ret |= POINTER_1L;
							}
						}else if(strcmp(node->attr.opt_declare.opt_name , "!$") == 0){	/*解除引用*/
							if(flag_array_name){	/*如果是数组名则类型是其首成员*/

							}else{
								printf("line: %s: check type error: illegal dereferrece %s\n" , node->line , node->attr.opt_declare.opt_content);
								type_ret |= TYPE_ERROR;
								return type_ret;
							}
						}


					}else if(strlen(entry->nr_pointer) == 1){	/*一级指针*/

						if(strcmp(node->attr.opt_declare.opt_name , "$") == 0){	/*取地址*/
							if(flag_array_name){	/*如果是数组名则类型是其成员的上一级地址*/
								type_ret |= POINTER_3L;
							}else{
								type_ret |= POINTER_2L;	/*正常取一级指针地址成为二级指针*/
							}
						}else if(strcmp(node->attr.opt_declare.opt_name , "!$") == 0){	/*解除引用*/
							if(flag_array_name){	/*如果是数组名*/
								type_ret |= POINTER_1L;
							}
							/*解除一级指针*/
						}else{
							if(flag_array_name){	/*如果是数组名*/
								type_ret |= POINTER_2L;
							}else{
								type_ret |= POINTER_1L;	/*一级指针*/
							}
						}

					}else if(strlen(entry->nr_pointer) == 2){	/*二级指针*/

						if(strcmp(node->attr.opt_declare.opt_name , "$") == 0){	/*取地址*/
							if(flag_array_name){	/*如果是数组名则类型是其成员的上一级地址*/
								type_ret |= POINTER_4L;
							}else{
								type_ret |= POINTER_3L;	/*正常取二级指针地址成为三级指针*/
							}
						}else if(strcmp(node->attr.opt_declare.opt_name , "!$") == 0){	/*解除引用*/
							if(flag_array_name){	/*如果是数组名*/
								type_ret |= POINTER_2L;
							}else{
								type_ret |= POINTER_1L;/*解除二级指针为一级*/
							}
						}else{
							if(flag_array_name){	/*如果是数组名*/
								type_ret |= POINTER_3L;
							}else{
								type_ret |= POINTER_2L;	/*成为二级指针*/
							}
						}

					}else if(strlen(entry->nr_pointer) == 3){	/*三级指针*/

						if(strcmp(node->attr.opt_declare.opt_name , "$") == 0){	/*取地址*/
							if(flag_array_name){	/*如果是数组名则类型是其成员的上一级地址*/
								printf("line: %s: check type error: array name %s reference > 4L\n" , node->line , node->attr.opt_declare.opt_content);
								type_ret |= TYPE_ERROR;
								return type_ret;
							}else{
								type_ret |= POINTER_4L;	/*成为四级指针*/
							}
						}else if(strcmp(node->attr.opt_declare.opt_name , "!$") == 0){	/*解除引用*/
							if(flag_array_name){	/*如果是数组名*/
								type_ret |= POINTER_3L;
							}else{
								type_ret |= POINTER_2L;/*解除三级指针为二级*/
							}
						}else{
							if(flag_array_name){	/*如果是数组名*/
								type_ret |= POINTER_4L;
							}else{
								type_ret |= POINTER_3L;	/*成为三级指针*/
							}
						}

					}else if(strlen(entry->nr_pointer) == 4){	/*四级指针*/

						if(strcmp(node->attr.opt_declare.opt_name , "$") == 0){	/*取地址*/
							printf("line: %s: check type error: %s reference > 4L\n" , node->line , node->attr.opt_declare.opt_content);
							type_ret |= TYPE_ERROR;
							return type_ret;
						}else if(strcmp(node->attr.opt_declare.opt_name , "!$") == 0){	/*解除引用*/
							if(flag_array_name){	/*如果是数组名*/
								type_ret |= POINTER_4L;
							}else{
								type_ret |= POINTER_3L;/*解除四级指针为三级*/
							}
						}else{
							if(flag_array_name){	/*如果是数组名*/
								printf("line: %s: check type error: array name %s reference > 4L\n" , node->line , node->attr.opt_declare.opt_content);
								type_ret |= TYPE_ERROR;
								return type_ret;
							}else{
								type_ret |= POINTER_4L;	/*成为四级指针*/
							}
						}

					}else{	/*超出四级指针*/
						printf("line: %s: semantic error: %s reference > 4L\n" , node->line , node->attr.opt_declare.opt_content);
						type_ret |= TYPE_ERROR;
						return type_ret;
					}	/*end check pointer*/

					/*其他的操作符不会影响类型*/

					if((type_ret & STRUCT_TYPE) && !(type_ret & POINTER_MASK)){	/*如果直接算术操作结构体则会出现错误*/
						printf("line: %s: semantic error: illegal operator %s to %s \n" , node->line , node->attr.opt_declare.opt_name ,
									node->attr.opt_declare.opt_content);
						type_ret |= TYPE_ERROR;
						return type_ret;
					}

					return type_ret;

				}

			}

			/*没有操作符那么可能是常量或者标识符*/
			if(strlen(node->attr.opt_declare.opt_name) == 0){
				type_ret &= NO_TYPE;

				switch(node->attr.opt_declare.opt_content[0]){
				case '"':	/*字符串常量 返回字符型指针*/
					type_ret |= CONST_QF;
					type_ret |= CHAR_TYPE;
					type_ret |= POINTER_1L;
					return type_ret;

				case '\'':	/*字符型常量*/
					type_ret |= CONST_QF;
					type_ret |= CHAR_TYPE;
					return type_ret;

				case '.': /*实型常量*/
					type_ret |= CONST_QF;
					type_ret |= DOUBLE_TYPE;
					return type_ret;

				case '#':	/*整型常量*/
					type_ret |= CONST_QF;
					type_ret |= INT_TYPE;
					return type_ret;

				default:	/*标识符*/

					entry = get_entry(table_level , node->attr.opt_declare.opt_content);	/*在当前符号表或者上层符号表查找标识符信息*/
					if(!entry){

						if(table_level->parent_entry && table_level->parent_entry->entry_type == FUNC_ENTRY){	/*这可能是函数的定义域*/
							/*考察形参*/
							entry = get_entry(table_level->parent_entry->attr.function_table.param_table , node->attr.opt_declare.opt_content);
						}

						if(!entry){	/*如果还没有找到就在根作用域查找*/
							entry = get_entry(root_table , node->attr.opt_declare.opt_content);
						}
					}
					if(!entry){	/*如果还没有找到那么说明出现错误*/
						printf("line: %s: check type error: unknown identifier %s\n" , node->line , node->attr.opt_declare.opt_content);
						type_ret &= NO_TYPE;
						type_ret |= TYPE_ERROR;
						return type_ret;
					}


					/*检测是否结构体成员*/
					if(node->first_child && node->first_child->type == MEMBER_SELECT){	/*是结构体成员*/
						/*找到结构体原型*/
						memset(buff , 0 , OPT_CONTENT_LEN);
						strcpy(buff , entry->type_specific);
						needle = strstr(buff , ".");
						needle++;	/*needle指向向结构体类型名*/
						entry = get_entry(root_table , needle);

						/*enry指向当前描述结构体的项。需要找到结构体成员的类型*/
						entry_tmp = get_entry(entry->attr.struct_table , node->first_child->attr.opt_declare.opt_content);
						entry = entry_tmp;
					}


					/*检测是否是数组*/
					if(entry->entry_type == ARRAY_ENTRY){
						printf("array %s\n" , entry->name);

						if(node->first_child && node->first_child->type == ELEMENT_SELECT){	/*是数组成员*/
							printf("array element!\n");
						}else{	/*不是使用数组成员 而是数组名*/
							printf("array name!\n");
							flag_array_name = 1;
						}

					}

					/*根据相关标识的原始类型进行设置*/

					if(strcmp(entry->type_qualifier , "static") == 0){	/*设置类型限定符*/
						type_ret |= STATIC_QF;
					}else if(strcmp(entry->type_qualifier , "const") == 0){
						type_ret |= CONST_QF;
					}else{
						;
					}

					if(strcmp(entry->type_specific , "void") == 0){	/*void 类型*/
						type_ret |= VOID_TYPE;
					}else if(strcmp(entry->type_specific , "char") == 0){	/*char 类型*/
						type_ret |= CHAR_TYPE;
					}else if(strcmp(entry->type_specific , "short") == 0){	/*short 类型*/
						type_ret |= SHORT_TYPE;
					}else if(strcmp(entry->type_specific , "int") == 0){	/*int 类型*/
						type_ret |= INT_TYPE;
					}else if(strcmp(entry->type_specific , "long") == 0){	/*long 类型*/
						type_ret |= LONG_TYPE;
					}else if(strcmp(entry->type_specific , "float") == 0){	/*float 类型*/
						type_ret |= FLOAT_TYPE;
					}else if(strcmp(entry->type_specific , "double") == 0){	/*double 类型*/
						type_ret |= DOUBLE_TYPE;
					}else{	/*struct 类型*/
						type_ret |= STRUCT_TYPE;
					}

					if(strlen(entry->nr_pointer) == 0){	/*考察指针层数*/
						if(flag_array_name){	/*如果是数组名则类型是其成员的上一级地址*/
							type_ret |= POINTER_1L;
						}
					}else if(strlen(entry->nr_pointer) == 1){	/*一级指针*/
						if(flag_array_name){	/*如果是数组名则类型是其成员的上一级地址*/
							type_ret |= POINTER_2L;
						}else{
							type_ret |= POINTER_1L;
						}
					}else if(strlen(entry->nr_pointer) == 2){	/*二级指针*/
						if(flag_array_name){	/*如果是数组名则类型是其成员的上一级地址*/
							type_ret |= POINTER_3L;
						}else{
							type_ret |= POINTER_2L;
						}
					}else if(strlen(entry->nr_pointer) == 3){	/*三级指针*/
						if(flag_array_name){	/*如果是数组名则类型是其成员的上一级地址*/
							type_ret |= POINTER_4L;
						}else{
							type_ret |= POINTER_3L;
						}
					}else if(strlen(entry->nr_pointer) == 4){	/*四级指针*/
						if(flag_array_name){	/*如果是数组名则类型是其成员的上一级地址*/
							printf("line: %s: check type error: array name %s reference > 4L\n" , node->line , node->attr.opt_declare.opt_content);
							type_ret |= TYPE_ERROR;
							return type_ret;
						}else{
							type_ret |= POINTER_4L;
						}
					}else{
						printf("line: %s: semantic error: %s reference > 4L\n" , node->line , node->attr.opt_declare.opt_content);
						type_ret |= TYPE_ERROR;
						return type_ret;
					}	/*end check pointer*/


					return type_ret;

				}	/*end switch opt_content[0]*/
			}	/*end unary exp*/
			return 0;

		default: /*will never happen*/
			DUMP_ERR("fatal error happened!\n");
			type_ret = TYPE_ERROR;
			return type_ret;
		}
		break; /*break EXP_KIND*/

	default:	/*will never happen*/
		DUMP_ERR("fatal error happened!\n");
		type_ret = TYPE_ERROR;
		return type_ret;
	}

	return 0;

}

/*打印常数表*/
static int print_const_table(sym_table *table , char *name){
	unsigned char i;
	sym_tb_entry *p;
	sym_tb_entry *next;

	if(!table){	/*table为空 返回-1*/
		return -1;
	}

	/*遍历该表的所有表头如果表头有表项则遍历其链接的表项串*/
	printf("-----------------table %s  CONST---------<%d> elements\n" , name , table->src_addr);
	for(i=0; i<SYM_TB_SIZE; i++){

		if(table->entry_heads[i]){	/*如果有表项则删除该表项串*/
			p = table->entry_heads[i];

			while(p){	/*遍历表项串*/
				printf("%s const    %s    %d\n" , p->name , p->asm_label , p->src_addr);
				p = p->next;
			}	/*end while 遍历表项串*/

		}	/*end if*/

	}	/*end for 遍历整个表*/

	return 0;
}

/*打印符号表*/
static int print_sym_table(sym_table *table , char *name){
	unsigned char i;
	sym_tb_entry *p;
	sym_tb_entry *next;
	char buff[50];

	if(!table){	/*table为空 返回-1*/
		return -1;
	}

	/*遍历该表的所有表头如果表头有表项则遍历其链接的表项串*/
	printf("----------------- table %s-------------<%d> elements\n" , name , table->src_addr);
	for(i=0; i<SYM_TB_SIZE; i++){

		if(table->entry_heads[i]){	/*如果有表项则删除该表项串*/
			p = table->entry_heads[i];

			while(p){	/*遍历表项串*/
				if(p->entry_type == FUNC_ENTRY){	/*如果是函数或者结构体类型表项需要提前删除表现其实现之符号表*/
					printf("%s	function    %s%s    %s    %dB  %dB  |%d\n" , p->name , p->nr_pointer , p->type_specific , p->asm_label , p->size ,
							p->align_size , p->src_addr);
				}else if(p->entry_type == STRUCT_ENTRY){
					printf("%s	struct	%s    %dB  %dB  |%d\n" , p->name , p->asm_label , p->size , p->align_size , p->src_addr);
				}else if(p->entry_type == VAR_ENTRY){
					printf("%s	variable    %s%s    %s    %dB  %dB  |%d\n" , p->name , p->nr_pointer , p->type_specific , p->asm_label , p->size ,
							p->align_size , p->src_addr);
				}else if(p->entry_type == ARRAY_ENTRY){
					printf("%s	array    %s%s %s   %s    %dB  %dB  |%d\n" , p->name , p->nr_pointer , p->type_specific , p->attr.array_len , p->asm_label ,
							p->size , p->align_size , p->src_addr);
				}

				p = p->next;
			}	/*end while 遍历表项串*/

		}	/*end if*/

	}	/*end for 遍历整个表*/

	/*遍历具有符号表的表项*/
	for(i=0; i<SYM_TB_SIZE; i++){

		if(table->entry_heads[i]){	/*如果有表项则遍历该表项串*/
			p = table->entry_heads[i];

			while(p){	/*遍历表项串*/
				if(p->entry_type == FUNC_ENTRY){	/*如果是函数或者结构体类型表项需要打印其符号表*/
					memset(buff , 0 , 50);
					strcpy(buff , p->name);
					strcat(buff , " FUNCTION PARAM");
					print_sym_table(p->attr.function_table.param_table , buff);
					memset(buff , 0 , 50);
					strcpy(buff , p->name);
					strcat(buff , " FUNCTION BLOCK");
					print_sym_table(p->attr.function_table.block_table , buff);
				}else if(p->entry_type == STRUCT_ENTRY){
					memset(buff , 0 , 50);
					strcpy(buff , p->name);
					strcat(buff , " STRUCT");
					print_sym_table(p->attr.struct_table , buff);
				}else{
				}

				p = p->next;
			}	/*end while 遍历表项串*/

		}	/*end if*/

	}	/*end for 遍历整个表*/

	return 0;
}

/*分别为根符号表、函数、结构体符号表填充相应符号在汇编中的标记*/
static int fill_asm_label_root(sym_table *table , char *prefix){
	unsigned char i;
	sym_tb_entry *p;
	sym_tb_entry *next;
	char buff[ASM_LABEL_LEN];
	char *needle;

	if(!table){	/*table为空 返回-1*/
		return -1;
	}

	/*遍历该表的所有表头如果表头有表项则遍历其链接的表项串*/
	for(i=0; i<SYM_TB_SIZE; i++){

		if(table->entry_heads[i]){	/*如果有表项则遍历该表项串*/
			p = table->entry_heads[i];

			while(p){	/*遍历表项串*/

				if(p->entry_type == FUNC_ENTRY){	/*如果是函数或者结构体类型表项需要填充其符号表*/

					/*如果没有函数体实现或者由extern修饰则说明外部符号。需要保持原名*/
					if(!p->attr.function_table.block_table || strcmp(p->storage_class , "extern") == 0){
						strcpy(p->asm_label , p->name);
					}else{

						/*将文件名处理 xx.xx 变化为 xx_* 的格式*/
						memset(buff , 0 , ASM_LABEL_LEN);
						strcpy(buff , prefix);

						needle = strstr(buff , ".");	/*将文件名中.之后的内容清除*/
						if(needle){
							memset(needle , 0 , strlen(needle));
						}
						strcat(buff , "_");
						/*最终修改为汇编标签 prefix_varname的形式*/
						strcat(buff , p->name);
						/*将其存入汇编标签中*/
						strcpy(p->asm_label , buff);

					}

					fill_asm_label_function(p->attr.function_table.param_table , "param");	/*填充形参及函数主体*/
					fill_asm_label_function(p->attr.function_table.block_table , "block");
				}else if(p->entry_type == STRUCT_ENTRY){
					fill_asm_label_struct(p->attr.struct_table , p->asm_label);
				}else if(p->entry_type == VAR_ENTRY || p->entry_type == ARRAY_ENTRY){

					/*如果由extern修饰则说明外部符号。需要保持原名*/
					if(strcmp(p->storage_class , "extern") == 0){
						strcpy(p->asm_label , p->name);
					}else{

						/*将文件名处理 xx.xx 变化为 xx_* 的格式*/
						memset(buff , 0 , ASM_LABEL_LEN);
						strcpy(buff , prefix);

						needle = strstr(buff , ".");	/*将文件名中.之后的内容清除*/
						if(needle){
							memset(needle , 0 , strlen(needle));
						}
						strcat(buff , "_");
						/*最终修改为汇编标签 prefix_varname的形式*/
						strcat(buff , p->name);
						/*将其存入汇编标签中*/
						strcpy(p->asm_label , buff);

					}
				}else{
					printf("error: const %s in normal table\n" , p->name);
					return -1;
				}

				p = p->next;
			}	/*end while 遍历表项串*/

		}	/*end if*/

	}	/*end for 遍历整个表*/

	return 0;
}

/*填充函数中相关变量的汇编符号。同时可以获得函数体中局部变量的总共大小*/
static int fill_asm_label_function(sym_table *table , char *prefix){
	unsigned char i , j;

	sym_tb_entry *p;
	sym_tb_entry *next;

	unsigned long offset;	/*变量在栈的偏移*/
	char line[STR_LINE_LEN];	/*变量出现的地址字符缓冲区*/
	char buff[ASM_LABEL_LEN];

	unsigned int size;	/*函数局部变量总共大小*/
	unsigned int align_size;	/*对齐后的局部变量总大小*/

	if(!table || !prefix){	/*表或者前缀为空返回*/
		return -1;
	}

	/*填充形参*/
	if(strcmp(prefix , "param") == 0){
		/*寄存器间接寻址。offset(ebp)的形式。即参数为offset(ebp). offset是此参数相对偏移 等于之前参数对齐后大小的和 + ebp+eip的值(8)*/
		/*C类型的入栈方式。从右至左入栈正好参数在栈中的序数与扫描而得的顺序一致*/
		offset = 8;	/*ebp + eip*/
		size = 0;
		align_size = 0;

		for(i=0; i<table->src_addr; i++){	/*每循环一次获取src_addr == i的形参 设置其asm_label  一共有table->src_addr个形参*/

			for(j=0; j<SYM_TB_SIZE; j++){		/*遍历该表的所有表头如果表头有表项则遍历其链接的表项串*/

				if(table->entry_heads[j]){	/*如果有表项则遍历该表项串*/
					p = table->entry_heads[j];

					while(p){	/*遍历表项串*/

						if(p->src_addr == i){	/*如果找到src_addr == i的表项则将该表项中填写汇编符号。退出循环寻找下一个表项*/
							memset(buff , 0 , ASM_LABEL_LEN);
							memset(line , 0 , STR_LINE_LEN);

							sprintf(line , "%d" , offset);
							/*构造offset(ebp)*/
							strcat(buff , line);
							strcat(buff , "(ebp)");

							strcpy(p->asm_label , buff);

							size += p->size;
							align_size += p->align_size;

							offset += p->align_size;	/*修改偏移*/
							goto fill_param_table;	/*出口*/
						}

						p = p->next;
					}	/*end while 遍历表项串*/

				}	/*end if*/

			}	/*end for 遍历整个表*/

fill_param_table:
			;
		}	/*end for 顺序遍历所有table->src_addr*/

		table->total_size = size;
		table->total_align_size = align_size;
		return 0;
	}


	/*填充函数体*/
	if(strcmp(prefix , "block") == 0){

		/*寄存器间接寻址。offset(ebp)的形式。即参数为offset(ebp). offset是此参数相对偏移 等于之前参数对齐后大小的和*/
		/*需要依照局部变量定义顺序开辟局部变量栈空间 同时可以记录该函数体局部变量实际大小与对齐后总和*/
		offset = 0;
		size = 0;
		align_size = 0;

		for(i=0; i<table->src_addr; i++){	/*每循环一次获取src_addr == i的局部变量 设置其asm_label  一共有table->src_addr个局部变量*/

			for(j=0; j<SYM_TB_SIZE; j++){		/*遍历该表的所有表头如果表头有表项则遍历其链接的表项串*/

				if(table->entry_heads[j]){	/*如果有表项则遍历该表项串*/
					p = table->entry_heads[j];

					while(p){	/*遍历表项串*/

						if(p->src_addr == i){	/*如果找到src_addr == i的表项则将该表项中填写汇编符号。退出循环寻找下一个表项*/
							memset(buff , 0 , ASM_LABEL_LEN);
							memset(line , 0 , STR_LINE_LEN);

							/*计算局部变量大小*/
							if(p->entry_type == ARRAY_ENTRY){	/*如果是数组需要计算数组总共大小*/
								size += p->size * atoi(p->attr.array_len);
								align_size += p->align_size * atoi(p->attr.array_len);

								offset += p->align_size * atoi(p->attr.array_len);	/*偏移绝对值*/
							}else{	/*其他类型变量*/
								size += p->size;
								align_size += p->align_size;
								offset += p->align_size;
							}

							sprintf(line , "%d" , -offset);	/*注意是向下偏移*/
							/*构造offset(ebp)*/
							strcat(buff , line);
							strcat(buff , "(ebp)");

							strcpy(p->asm_label , buff);
							goto fill_block_table;	/*出口*/
						}

						p = p->next;
					}	/*end while 遍历表项串*/

				}	/*end if*/

			}	/*end for 遍历整个表*/

fill_block_table:
			;
		}	/*end for 顺序遍历所有table->src_addr*/


		table->parent_entry->size = size;	/*函数大小记录的是该函数体局部变量总共大小*/
		table->parent_entry->align_size = align_size;

		return 0;
	}

	/*出错*/
	DUMP_ERR("fill asm label function failed!\n");
	return -1;
}

static int fill_asm_label_struct(sym_table *table , char *prefix){
	return 0;
}

static int fill_asm_label_const(sym_table *table , char *prefix){
	unsigned char i;
	sym_tb_entry *p;
	sym_tb_entry *next;
	char *needle;
	char buff[ASM_LABEL_LEN];
	char line[STR_LINE_LEN];	/*变量出现的地址字符缓冲区*/


	if(!table){	/*table为空 返回-1*/
		return -1;
	}

	/*遍历该表的所有表头如果表头有表项则遍历其链接的表项串*/
	for(i=0; i<SYM_TB_SIZE; i++){

		if(table->entry_heads[i]){	/*如果有表项则遍历该表项串*/
			p = table->entry_heads[i];

			while(p){	/*遍历表项串*/

				/*将文件名处理 xx.xx 变化为 xx_* 的格式*/
				memset(buff , 0 , ASM_LABEL_LEN);
				strcpy(buff , prefix);

				needle = strstr(buff , ".");	/*将文件名中.之后的内容清除*/
				if(needle){
					memset(needle , 0 , strlen(needle));
				}
				strcat(buff , "_const");

				/*最终修改为汇编标签 prefix_varname的形式*/
				memset(line , 0 , STR_LINE_LEN);
				sprintf(line , "%d" , p->src_addr);
				strcat(buff , line);

				/*将其存入汇编标签中*/
				strcpy(p->asm_label , buff);

				p = p->next;
			}	/*end while 遍历表项串*/

		}	/*end if*/

	}	/*end for 遍历整个表*/
	return 0;
}




/*符号表哈希函数。
 * 根据输入的字符串得到其在哈希表的地址
 * @param key: 输入的字符串
 * @return: 在哈希表的地址
 */
static int sym_addr(char *key){
	unsigned short i;
	unsigned long temp;

	temp = 0;
	i = 0;
	while(key[i] != 0x00){
		temp = (temp * SYM_TB_COEFF + key[i]) % SYM_TB_SIZE;
		i++;
	}

	return temp;
}
