/*
 * syntax_analy.c
 *
 *  Created on: 2011-5-18
 *      Author: leiming
 */

/*
 *对源文件进行自顶向下递归语法分析。并由此构造一棵语法树
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "global.h"
#include "lexical_analy.h"
#include "syntax_analy.h"

/*全局变量*/
syntax_node *syntax_tree;		/*一棵语法解析树*/
unsigned char syntax_error;	/*标志是否出现语法错误*/

/*
 * 对每个非终结符进行递归下降分析
 */
static syntax_node *stmt(void);		/*语句序列*/
static syntax_node *label_stmt(void);	/*标记语句*/
static syntax_node *exp_stmt(void);		/*表达式语句*/
static syntax_node *select_stmt(void);	/*选择语句*/
static syntax_node *iteration_stmt(void);	/*迭代语句*/
static syntax_node *jump_stmt(void);	/*跳转语句*/
static syntax_node *null_stmt(void);	/*空语句*/
static syntax_node *struct_stmt(void);	/*结构体类型定义语句*/
static syntax_node *variable_stmt(void); /*变量声明/定义语句*/
static syntax_node *function_stmt(void); /*函数声明/定义语句*/

static syntax_node *expression(void);		/*表达式*/
static syntax_node *seq_exp(void);			/*句号表达式*/
static syntax_node *assign_exp(void);		/*赋值表达式*/
static syntax_node *logic_or_exp(void);	/*逻辑或表达式*/
static syntax_node *logic_and_exp(void);	/*逻辑与表达式*/
static syntax_node *bit_or_exp(void);		/*按位或表达式*/
static syntax_node *bit_xor_exp(void);	/*按位异或表达式*/
static syntax_node *bit_and_exp(void);	/*按位与表达式*/
static syntax_node *equal_exp(void);		/*等价表达式*/
static syntax_node *relation_exp(void);	/*关系表达式*/
static syntax_node *shift_exp(void);			/*移位表达式*/
static syntax_node *additive_exp(void);	/*加减表达式*/
static syntax_node *multi_exp(void);		/*乘除取摸表达式*/
static syntax_node *cast_exp(void);			/*类型转换表达式*/
static syntax_node *unary_exp(void);		/*一元表达式*/
static syntax_node *postfix_phrase(void); /*后缀短语*/
static syntax_node *element_select(void);	/*元素选择*/
static syntax_node *member_select(void);	/*成员选择*/
static syntax_node *argument_spec(void);	/*参数描述*/

/*
 *
 *
 * 构造一棵语法树。树根为全局变量syntax_tree
 * @return: 0 成功
 *                -1 失败
 */
int syntax_analy(void){
	syntax_node *node;
	syntax_node *prev_child;
	syntax_node *next_child;

	node = syntax_tree;

	/*获取其第一个子语句结点如果有*/
	/*检测是否变量声明/定义语句*/
	prev_child = variable_stmt();
	if(prev_child){
		node->first_child = prev_child;

		if(prev_child->error){	/*如果出错那么不进行后续分析。错误节点添加到此树后。将此树标记为错误。返回*/
			node->error = 1;
			return -1;
		}
		goto start_add_next_child;	/*如果有第一个孩子而且不出错 那么去添加其他孩子*/
	}
	/*检测是否函数声明/定义语句*/
	prev_child = function_stmt();
	if(prev_child){
		node->first_child = prev_child;

		if(prev_child->error){	/*如果出错那么不进行后续分析。错误节点添加到此树后。将此树标记为错误。返回*/
			node->error = 1;
			return -1;
		}
		goto start_add_next_child;	/*如果有第一个孩子而且不出错 那么去添加其他孩子*/
	}
	/*检测是否结构体类型定义语句*/
	prev_child = struct_stmt();
	if(prev_child){
		node->first_child = prev_child;

		if(prev_child->error){	/*如果出错那么不进行后续分析。错误节点添加到此树后。将此树标记为错误。返回*/
			node->error = 1;
			return -1;
		}
		goto start_add_next_child;	/*如果有第一个孩子而且不出错 那么去添加其他孩子*/
	}

	/*说明为空*/
	return -1;

	/*添加其他孩子*/
start_add_next_child:
	while(1){	/*所有之后的孩子都添加为prev_child的兄弟*/
		/*检测是否变量声明/定义语句*/
		next_child = variable_stmt();
		if(next_child){
			prev_child->sibling = next_child;

			if(next_child->error == 1){	/*如果出错那么不进行后续分析。错误节点添加到此树后。将此树标记为错误。退出循环*/
				node->error = 1;
				break;
			}

			prev_child = next_child;	/*将当前孩子作为兄长。继续寻找其兄弟*/
			continue;
		}
		/*检测是否函数声明/定义语句*/
		next_child = function_stmt();
		if(next_child){
			prev_child->sibling = next_child;

			if(next_child->error == 1){	/*如果出错那么不进行后续分析。错误节点添加到此树后。将此树标记为错误。退出循环*/
				node->error = 1;
				break;
			}

			prev_child = next_child;	/*将当前孩子作为兄长。继续寻找其兄弟*/
			continue;
		}
		/*检测是否结构体类型定义语句*/
		next_child = struct_stmt();
		if(next_child){
			prev_child->sibling = next_child;

			if(next_child->error == 1){	/*如果出错那么不进行后续分析。错误节点添加到此树后。将此树标记为错误。退出循环*/
				node->error = 1;
				break;
			}

			prev_child = next_child;	/*将当前孩子作为兄长。继续寻找其兄弟*/
			continue;
		}

		/*没有其他兄弟*/
		break;
	}

	return 0;

}


/*
 * 删除所构建的语法分析树
 */
int delete_syntax_tree(syntax_node *node){
	syntax_node *sibling;

	if(node->first_child){	/*删除此棵子树*/
		delete_syntax_tree(node->first_child);
	}


	if(node->second_child){
		delete_syntax_tree(node->second_child);
	}

	if(node->third_child){
		delete_syntax_tree(node->third_child);
	}

	if(node->last_child){
		delete_syntax_tree(node->last_child);
	}

	if(node->sibling){	/*删除兄弟子树*/
		delete_syntax_tree(node->sibling);
	}

	free(node);
	return 0;
}

/*
 * 打印构成的语法树信息
 *  @return: 检查出错误节点返回-1
 *  成功返回0
 */
int print_syntax_tree(syntax_node *node){
	switch(node->kind){

	case START_KIND:							/*起始类型节点*/
DUMP_SYN_STR("---start node\n");
	/*打印具体信息*/
	if(node->attr.content && node->error == 0){
DUMP_SYN_STR(node->attr.content);
DUMP_SYN_STR(" ");
	}
DUMP_SYN_STR("\n");
	break;

	case STMT_KIND:						/*语句节点*/
DUMP_SYN_STR("---statement  ");

		switch(node->type){
		case STMT:
DUMP_SYN_STR("  common stmt\n");
			break;

		case LABEL_STMT:
DUMP_SYN_STR("  label stmt  ");
DUMP_SYN_STR(node->line);
DUMP_SYN_STR("  ");
		/*打印具体信息*/
		if(node->attr.content && node->error == 0){
DUMP_SYN_STR(node->attr.content);
DUMP_SYN_STR(" ");
		}
DUMP_SYN_STR("\n");
			break;

		case EXP_STMT:
DUMP_SYN_STR("  exp stmt ");
DUMP_SYN_STR(node->line);
DUMP_SYN_STR("  ");
DUMP_SYN_STR("\n");
			break;

		case SELECT_STMT:
DUMP_SYN_STR("  select stmt  ");
DUMP_SYN_STR(node->line);
DUMP_SYN_STR("  ");
		/*打印具体信息*/
		if(node->attr.content && node->error == 0){
DUMP_SYN_STR(node->attr.content);
DUMP_SYN_STR(" ");
		}
DUMP_SYN_STR("\n");
			break;

		case ITER_STMT:
DUMP_SYN_STR("  iter stmt ");
DUMP_SYN_STR(node->line);
DUMP_SYN_STR("  ");
		/*打印具体信息*/
		if(node->attr.content && node->error == 0){
DUMP_SYN_STR(node->attr.content);
DUMP_SYN_STR(" ");
		}
		DUMP_SYN_STR("\n");
			break;

		case JUMP_STMT:
DUMP_SYN_STR("  jump stmt  ");
DUMP_SYN_STR(node->line);
DUMP_SYN_STR("  ");
		/*打印具体信息*/
		if(node->attr.content && node->error == 0){
DUMP_SYN_STR(node->attr.content);
DUMP_SYN_STR(" ");
		}
DUMP_SYN_STR("\n");
			break;

		case STRUCT_STMT:
DUMP_SYN_STR("  struct stmt  ");
DUMP_SYN_STR(node->line);
DUMP_SYN_STR("  ");
		/*打印具体信息*/
		if(node->attr.content && node->error == 0){
DUMP_SYN_STR(node->attr.content);
DUMP_SYN_STR(" ");
		}
DUMP_SYN_STR("\n");
			break;

		case STRUCT_MEMBER:
DUMP_SYN_STR("  struct member  ");
		/*打印具体信息*/
		if(node->attr.type_declare.type_specific && node->error == 0){
DUMP_SYN_STR(node->attr.type_declare.type_specific);
DUMP_SYN_STR(" ");
		}
		if(node->attr.type_declare.nr_pointer && node->error == 0){
DUMP_SYN_STR(node->attr.type_declare.nr_pointer);
DUMP_SYN_STR(" ");
		}
		if(node->attr.type_declare.var_name && node->error == 0){
DUMP_SYN_STR(node->attr.type_declare.var_name);
DUMP_SYN_STR(" ");
		}
DUMP_SYN_STR("\n");
			break;

		case VAR_STMT:

			/*如果出现直接定义void变量则出现错误*/
			if(strcmp(node->attr.type_declare.type_specific , "void") == 0 && strlen(node->attr.type_declare.nr_pointer) == 0){
				printf("line: %s: syntax error: can not define void type %s directly!\n" , node->line , node->attr.type_declare.var_name);
				syntax_error = 1;
				return -1;
			}

DUMP_SYN_STR("  var stmt  ");
DUMP_SYN_STR(node->line);
DUMP_SYN_STR("  ");
			/*打印具体类型信息*/
			if(node->attr.type_declare.storage_class && node->error == 0){
DUMP_SYN_STR(node->attr.type_declare.storage_class);
DUMP_SYN_STR(" ");
			}
			if(node->attr.type_declare.type_qualifier && node->error == 0){
DUMP_SYN_STR(node->attr.type_declare.type_qualifier);
DUMP_SYN_STR(" ");
			}
			if(node->attr.type_declare.type_specific && node->error == 0){
DUMP_SYN_STR(node->attr.type_declare.type_specific);
DUMP_SYN_STR(" ");
			}
			if(node->attr.type_declare.nr_pointer && node->error == 0){
DUMP_SYN_STR(node->attr.type_declare.nr_pointer);
DUMP_SYN_STR(" ");
			}
			if(node->attr.type_declare.var_name && node->error == 0){
DUMP_SYN_STR(node->attr.type_declare.var_name);
DUMP_SYN_STR(" ");
			}
DUMP_SYN_STR("\n");
			break;

		case NULL_STMT:
DUMP_SYN_STR("  null stmt\n");
			break;
		default:
			break;
		}

	if(node->error){
		syntax_error = 1;
DUMP_SYN_STR("				!!!error\n");
		if(node->attr.content){	/*如果有具体错误消息 打印之*/
			printf("%s\n" , node->attr.content);
		}
	}

	break;

	case EXP_KIND:						/*表达式节点*/
DUMP_SYN_STR("---exp node  ");

	if(node->error){
		syntax_error =1;
DUMP_SYN_STR("				!!!error\n");
		if(node->attr.content){	/*如果有具体错误消息 打印之*/
			printf("%s\n" , node->attr.content);
		}
	}else{
		switch(node->type){
		case SEQ_EXP:
			DUMP_SYN_STR(node->line);
			DUMP_SYN_STR("  ");
DUMP_SYN_STR("  seq exp   ,");
			break;
		case ASSIGN_EXP:
			DUMP_SYN_STR(node->line);
			DUMP_SYN_STR("  ");
DUMP_SYN_STR("  assign exp  ");
DUMP_SYN_STR(node->attr.opt_declare.opt_name);
			break;
		case LOGIC_OR_EXP:
			DUMP_SYN_STR(node->line);
			DUMP_SYN_STR("  ");
DUMP_SYN_STR("  logic or exp   ||");
			break;
		case LOGIC_AND_EXP:
			DUMP_SYN_STR(node->line);
			DUMP_SYN_STR("  ");
DUMP_SYN_STR("  logic and  exp   &&");
			break;
		case BIT_OR_EXP:
			DUMP_SYN_STR(node->line);
			DUMP_SYN_STR("  ");
DUMP_SYN_STR("  bit or exp   |");
			break;
		case BIT_XOR_EXP:
			DUMP_SYN_STR(node->line);
			DUMP_SYN_STR("  ");
DUMP_SYN_STR("  bit xor exp   ^");
			break;
		case BIT_AND_EXP:
			DUMP_SYN_STR(node->line);
			DUMP_SYN_STR("  ");
DUMP_SYN_STR("  bit and exp   &");
			break;
		case EQUAL_EXP:
			DUMP_SYN_STR(node->line);
			DUMP_SYN_STR("  ");
DUMP_SYN_STR("  equal exp  ");
DUMP_SYN_STR(node->attr.opt_declare.opt_name);
			break;
		case RELATION_EXP:
			DUMP_SYN_STR(node->line);
			DUMP_SYN_STR("  ");
DUMP_SYN_STR("  relation exp  ");
DUMP_SYN_STR(node->attr.opt_declare.opt_name);
			break;
		case SHIFT_EXP:
			DUMP_SYN_STR(node->line);
			DUMP_SYN_STR("  ");
DUMP_SYN_STR("  shift exp  ");
DUMP_SYN_STR(node->attr.opt_declare.opt_name);
			break;
		case ADDITIVE_EXP:
			DUMP_SYN_STR(node->line);
			DUMP_SYN_STR("  ");
DUMP_SYN_STR("  additive exp  ");
DUMP_SYN_STR(node->attr.opt_declare.opt_name);
			break;
		case MULTI_EXP:
DUMP_SYN_STR("  multi exp  ");
DUMP_SYN_STR(node->attr.opt_declare.opt_name);
			break;
		case CAST_EXP:
			DUMP_SYN_STR(node->line);
			DUMP_SYN_STR("  ");
DUMP_SYN_STR(" cast exp  ");
DUMP_SYN_STR(node->attr.opt_declare.opt_content);
			break;
		case UNARY_EXP:
			DUMP_SYN_STR(node->line);
			DUMP_SYN_STR("  ");
DUMP_SYN_STR("  unary exp  ");
			if(node->attr.opt_declare.opt_name){
DUMP_SYN_STR(node->attr.opt_declare.opt_name);
DUMP_SYN_STR("  ");
			}
			if(node->attr.opt_declare.opt_content){
DUMP_SYN_STR(node->attr.opt_declare.opt_content);
			}
			break;
		case ELEMENT_SELECT:
			DUMP_SYN_STR(node->line);
			DUMP_SYN_STR("  ");
DUMP_SYN_STR("  element select   []");
			break;
		case MEMBER_SELECT:
			DUMP_SYN_STR(node->line);
			DUMP_SYN_STR("  ");
DUMP_SYN_STR("  member select  ");
DUMP_SYN_STR(node->attr.opt_declare.opt_name);
DUMP_SYN_STR(" ");
DUMP_SYN_STR(node->attr.opt_declare.opt_content);
			break;
		case ARGUMENT_SPEC:
			DUMP_SYN_STR(node->line);
			DUMP_SYN_STR("  ");
DUMP_SYN_STR("  argument specific  ");
			break;
		default:
			break;
		}

DUMP_SYN_STR("\n");

	}
	break;

	case FUNC_KIND:						/*函数节点*/
DUMP_SYN_STR("---func node  ");
DUMP_SYN_STR(node->line);
DUMP_SYN_STR("  ");

	if(node->error){
		syntax_error = 1;
DUMP_SYN_STR("				!!!error\n");
		if(node->attr.content){	/*如果有具体错误消息 打印之*/
			printf("%s\n" , node->attr.content);
		}
	}else{
		if(node->type == FUNC_DECL){	/*函数声明*/
DUMP_SYN_STR("  function declare ");
		}else if(node->type == FUNC_DEFINE){
DUMP_SYN_STR("  function define ");
		}
		/*打印具体类型信息*/
		if(node->attr.type_declare.storage_class && node->error == 0){
DUMP_SYN_STR(node->attr.type_declare.storage_class);
DUMP_SYN_STR(" ");
		}
		if(node->attr.type_declare.type_qualifier && node->error == 0){
DUMP_SYN_STR(node->attr.type_declare.type_qualifier);
DUMP_SYN_STR(" ");
		}
		if(node->attr.type_declare.type_specific && node->error == 0){
DUMP_SYN_STR(node->attr.type_declare.type_specific);
DUMP_SYN_STR(" ");
		}
		if(node->attr.type_declare.nr_pointer && node->error == 0){
DUMP_SYN_STR(node->attr.type_declare.nr_pointer);
DUMP_SYN_STR(" ");
		}
		if(node->attr.type_declare.var_name && node->error == 0){
DUMP_SYN_STR(node->attr.type_declare.var_name);
		DUMP_SYN_STR(" ");
		}
		DUMP_SYN_STR("\n");
	}
	break;

	default:
		break;
	}

	if(node->first_child){
		print_syntax_tree(node->first_child);
	}
	if(node->second_child){
		print_syntax_tree(node->second_child);
	}
	if(node->third_child){
		print_syntax_tree(node->third_child);
	}
	if(node->last_child){
		print_syntax_tree(node->last_child);
	}
	if(node->sibling){
		print_syntax_tree(node->sibling);
	}

	return 0;
}



//////////////////////////PRIVATE FUNCTIONS////////////////////////////////////////
/*语句序列*/
static syntax_node *stmt(void){
	syntax_node *node;
	syntax_node *prev_child;
	syntax_node *next_child;

	node = (syntax_node *)malloc(sizeof(syntax_node));
	memset(node , 0 , sizeof(syntax_node));
	node->error = 0;
	node->kind = STMT_KIND;
	node->type = STMT;

	/*获取其第一个子语句结点如果有*/
	/*检测是否选择语句*/
	prev_child = select_stmt();
	if(prev_child){
		node->first_child = prev_child;

		if(prev_child->error){	/*如果出错那么不进行后续分析。错误节点添加到此树后。将此树标记为错误。返回*/
			node->error = 1;
			return node;
		}
		goto stmt_add_next_child;	/*如果有第一个孩子而且不出错 那么去添加其他孩子*/
	}
	/*检测是否迭代语句*/
	prev_child = iteration_stmt();
	if(prev_child){
		node->first_child = prev_child;

		if(prev_child->error){	/*如果出错那么不进行后续分析。错误节点添加到此树后。将此树标记为错误。返回*/
			node->error = 1;
			return node;
		}
		goto stmt_add_next_child;	/*如果有第一个孩子而且不出错 那么去添加其他孩子*/
	}
	/*检测是否跳转语句*/
	prev_child = jump_stmt();
	if(prev_child){
		node->first_child = prev_child;

		if(prev_child->error){	/*如果出错那么不进行后续分析。错误节点添加到此树后。将此树标记为错误。返回*/
			node->error = 1;
			return node;
		}
		goto stmt_add_next_child;	/*如果有第一个孩子而且不出错 那么去添加其他孩子*/
	}
	/*检测是否标签语句*/
	prev_child = label_stmt();
	if(prev_child){
		node->first_child = prev_child;

		if(prev_child->error){	/*如果出错那么不进行后续分析。错误节点添加到此树后。将此树标记为错误。返回*/
			node->error = 1;
			return node;
		}
		goto stmt_add_next_child;	/*如果有第一个孩子而且不出错 那么去添加其他孩子*/
	}
	/*检测是否空语句*/
	prev_child = null_stmt();
	if(prev_child){
		node->first_child = prev_child;

		if(prev_child->error){	/*如果出错那么不进行后续分析。错误节点添加到此树后。将此树标记为错误。返回*/
			node->error = 1;
			return node;
		}
		goto stmt_add_next_child;	/*如果有第一个孩子而且不出错 那么去添加其他孩子*/
	}
	/*检测是否表达式语句*/
	prev_child = exp_stmt();
	if(prev_child){
		node->first_child = prev_child;

		if(prev_child->error){	/*如果出错那么不进行后续分析。错误节点添加到此树后。将此树标记为错误。返回*/
			node->error = 1;
			return node;
		}
		goto stmt_add_next_child;	/*如果有第一个孩子而且不出错 那么去添加其他孩子*/
	}
	/*检测是否变量声明/定义语句*/
	prev_child = variable_stmt();
	if(prev_child){
		node->first_child = prev_child;

		if(prev_child->error){	/*如果出错那么不进行后续分析。错误节点添加到此树后。将此树标记为错误。返回*/
			node->error = 1;
			return node;
		}
		goto stmt_add_next_child;	/*如果有第一个孩子而且不出错 那么去添加其他孩子*/
	}


	free(node);	/*表示该语句序列为空*/
	return NULL;

stmt_add_next_child:
	while(1){	/*所有之后的孩子都添加为prev_child的兄弟*/
		/*检测是否选择语句*/
		next_child = select_stmt();
		if(next_child){
			prev_child->sibling = next_child;

			if(next_child->error == 1){	/*如果出错那么不进行后续分析。错误节点添加到此树后。将此树标记为错误。退出循环*/
				node->error = 1;
				break;
			}

			prev_child = next_child;	/*将当前孩子作为兄长。继续寻找其兄弟*/
			continue;
		}
		/*检测是否迭代语句*/
		next_child = iteration_stmt();
		if(next_child){
			prev_child->sibling = next_child;

			if(next_child->error == 1){	/*如果出错那么不进行后续分析。错误节点添加到此树后。将此树标记为错误。退出循环*/
				node->error = 1;
				break;
			}

			prev_child = next_child;	/*将当前孩子作为兄长。继续寻找其兄弟*/
			continue;
		}
		/*检测是否跳转语句*/
		next_child = jump_stmt();
		if(next_child){
			prev_child->sibling = next_child;

			if(next_child->error == 1){	/*如果出错那么不进行后续分析。错误节点添加到此树后。将此树标记为错误。退出循环*/
				node->error = 1;
				break;
			}

			prev_child = next_child;	/*将当前孩子作为兄长。继续寻找其兄弟*/
			continue;
		}
		/*检测是否标签语句*/
		next_child = label_stmt();
		if(next_child){
			prev_child->sibling = next_child;

			if(next_child->error == 1){	/*如果出错那么不进行后续分析。错误节点添加到此树后。将此树标记为错误。退出循环*/
				node->error = 1;
				break;
			}

			prev_child = next_child;	/*将当前孩子作为兄长。继续寻找其兄弟*/
			continue;
		}
		/*检测是否空语句*/
		next_child = null_stmt();
		if(next_child){
			prev_child->sibling = next_child;

			if(next_child->error == 1){	/*如果出错那么不进行后续分析。错误节点添加到此树后。将此树标记为错误。退出循环*/
				node->error = 1;
				break;
			}

			prev_child = next_child;	/*将当前孩子作为兄长。继续寻找其兄弟*/
			continue;
		}
		/*检测是否表达式语句*/
		next_child = exp_stmt();
		if(next_child){
			prev_child->sibling = next_child;

			if(next_child->error == 1){	/*如果出错那么不进行后续分析。错误节点添加到此树后。将此树标记为错误。退出循环*/
				node->error = 1;
				break;
			}

			prev_child = next_child;	/*将当前孩子作为兄长。继续寻找其兄弟*/
			continue;
		}
		/*检测是否变量声明/定义语句*/
		next_child = variable_stmt();
		if(next_child){
			prev_child->sibling = next_child;

			if(next_child->error == 1){	/*如果出错那么不进行后续分析。错误节点添加到此树后。将此树标记为错误。退出循环*/
				node->error = 1;
				break;
			}

			prev_child = next_child;	/*将当前孩子作为兄长。继续寻找其兄弟*/
			continue;
		}

		/*到这里说明为空那么退出循环*/
		break;
	}

	return node;
}
/*标记语句*/
static syntax_node *label_stmt(void){
	unsigned char flag = 0;	/*标志是否是标记语句*/
	syntax_node *node;
	char line[STR_LINE_LEN];	/*行号字符串*/

	flag = 0;
	memset(line , 0 , STR_LINE_LEN);
	sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/

	if(match(LABEL) == 0){
		flag = 1;	/*flag == 1 是普通标记语句*/
	}else if(match(CASE) == 0){
		flag = 2;	/*flag == 2 是case语句*/
	}else if(match(DEFAULT) == 0){
		flag = 3;	/*flag == 3 是default语句*/
	}

	if(flag == 0){	/*不是标记语句*/
		return NULL;
	}

	/*是标记语句*/
	node = (syntax_node *)malloc(sizeof(syntax_node));
	memset(node , 0 , sizeof(node));
	node->kind = STMT_KIND;
	node->type = LABEL_STMT;
	strcpy(node->line , line);

	if(flag == 1){	/*普通标志语句*/
		strcpy(node->attr.content , "label.");
		strcat(node->attr.content , src_buff.token_buff);
		if(match(IDENTIFIER) == -1){	/*错误的标志*/
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: illegal identifier ' ");
			strcat(node->attr.content , src_buff.token_buff);
			strcat(node->attr.content , " '");
			return node;
		}
	}else if(flag == 2){	/*case 语句*/
		strcpy(node->attr.content , "case.");
		strcat(node->attr.content , src_buff.token_buff);
		if(match(INT_VAL) == -1){

			if(match(CHAR_VAL) == -1){	/*常量错误*/
				node->error = 1;
				memset(node->attr.content , 0 , SYN_CONTENT_LEN);
				strcpy(node->attr.content , "line ");
				memset(line , 0 , STR_LINE_LEN);
				sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
				strcat(node->attr.content , line);
				strcat(node->attr.content , ": syntax error: illegal const ' ");
				strcat(node->attr.content , src_buff.token_buff);
				strcat(node->attr.content , " '");
				return node;
			}
		}

	}else if(flag == 3){	/*default 语句*/
		strcpy(node->attr.content , "default");
	}else{
		node->error = 1;/*应该永远不会到这里*/
		strcpy(node->attr.content , "worst situation happended");
		return node;
	}


	if(match(END_CONDITION) == -1){	/*匹配 : */
		node->error = 1;	/*错误*/
		memset(node->attr.content , 0 , SYN_CONTENT_LEN);
		strcpy(node->attr.content , "line ");
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcat(node->attr.content , line);
		strcat(node->attr.content , ": syntax error: token ' :");
		strcat(node->attr.content , " ' is nedded");
		return node;
	}

	if(match(LBRACE) == -1){	/*匹配 { */
		node->error = 1;	/*错误*/
		memset(node->attr.content , 0 , SYN_CONTENT_LEN);
		strcpy(node->attr.content , "line ");
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcat(node->attr.content , line);
		strcat(node->attr.content , ": syntax error: token ' {");
		strcat(node->attr.content , " ' is nedded");
		return node;
	}

	node->first_child = stmt();	/*将语句内容作为该节点的第一个孩子*/
	if(node->first_child){
		if(node->first_child->error){
			node->error = 1;
			return node;
		}
	}

	if(match(RBRACE) == -1){	/*匹配 } */
		node->error = 1;	/*错误*/
		memset(node->attr.content , 0 , SYN_CONTENT_LEN);
		strcpy(node->attr.content , "line ");
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcat(node->attr.content , line);
		strcat(node->attr.content , ": syntax error: token ' }");
		strcat(node->attr.content , " ' is nedded");
		return node;
	}


	return node;
}

/*表达式语句*/
static syntax_node *exp_stmt(void){
	syntax_node *first_child;
	syntax_node *node;
	char line[STR_LINE_LEN];	/*行号字符串*/

	memset(line , 0 , STR_LINE_LEN);
	sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/

	first_child = expression();
	if(first_child){
		node = (syntax_node *)malloc(sizeof(syntax_node));
		memset(node , 0 , sizeof(syntax_node));
		node->kind = STMT_KIND;
		node->type = EXP_STMT;
		strcpy(node->line , line);

		node->first_child = first_child;
		if(first_child->error){
			node->error = 1;
			return node;
		}

		if(match(NULL_STATE) == -1){
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' ;");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}
		return node;
	}else{
		return NULL;
	}

}
/*选择语句*/
static syntax_node *select_stmt(void){
	unsigned char flag;	/*标志是否选择语句*/
	syntax_node *node;
	char line[STR_LINE_LEN];	/*行号字符串*/

	flag = 0;
	memset(line , 0 , STR_LINE_LEN);
	sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/

	if(match(SWITCH) == 0){
		flag = 1;	/*flag == 1 表示switch*/
	}else if(match(IF) == 0){
		flag = 2;	/*flag == 2 表示if */
	}

	if(flag == 0){	/*不是选择语句*/
		return NULL;
	}

	/*if 与 switch语句开始部分都是相同的*/
	node = (syntax_node *)malloc(sizeof(syntax_node));
	memset(node , 0 , sizeof(syntax_node));
	node->kind = STMT_KIND;
	node->type = SELECT_STMT;
	strcpy(node->line , line);

	strcpy(node->attr.content , src_buff.token_buff);

	/*匹配最开始token*/
	if(flag == 1){	/*swich语句*/
		strcpy(node->attr.content , "switch");
	}else if(flag == 2){
		strcpy(node->attr.content , "if");
	}else{
		node->error = 1;/*应该永远不会到这里*/
		strcpy(node->attr.content , "worst situation happended");
		return node;
	}

	/*开始检查*/

	if(match(LPARENT) == -1){	/*缺少 ( */
		node->error = 1;	/*错误*/
		memset(node->attr.content , 0 , SYN_CONTENT_LEN);
		strcpy(node->attr.content , "line ");
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcat(node->attr.content , line);
		strcat(node->attr.content , ": syntax error: token ' (");
		strcat(node->attr.content , " ' is nedded");
		return node;
	}

	node->first_child = expression();	/*第一个孩子是表达式*/
	if(node->first_child){
		if(node->first_child->error){	/*若孩子出错该节点标记出错 不再向前。返回*/
			node->error = 1;
			return node;
		}
	}

	if(match(RPARENT) == -1){	/*缺少 ) */
		node->error = 1;	/*错误*/
		memset(node->attr.content , 0 , SYN_CONTENT_LEN);
		strcpy(node->attr.content , "line ");
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcat(node->attr.content , line);
		strcat(node->attr.content , ": syntax error: token ' )");
		strcat(node->attr.content , " ' is nedded");
		return node;
	}


	if(match(LBRACE) == -1){	/*匹配 { */
		node->error = 1;	/*错误*/
		memset(node->attr.content , 0 , SYN_CONTENT_LEN);
		strcpy(node->attr.content , "line ");
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcat(node->attr.content , line);
		strcat(node->attr.content , ": syntax error: token ' {");
		strcat(node->attr.content , " ' is nedded");
		return node;
	}

	node->second_child = stmt();	/*将主体语句内容作为该节点的第二个孩子*/
	if(node->second_child){
		if(node->second_child->error){/*若孩子出错该节点标记出错 不再向前。返回*/
			node->error = 1;
			return node;
		}
	}

	if(match(RBRACE) == -1){	/*匹配 } */
		node->error = 1;	/*错误*/
		memset(node->attr.content , 0 , SYN_CONTENT_LEN);
		strcpy(node->attr.content , "line ");
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcat(node->attr.content , line);
		strcat(node->attr.content , ": syntax error: token ' }");
		strcat(node->attr.content , " ' is nedded");
		return node;
	}

	/*主体匹配成功*/

	if(flag == 1){	/*是switch语句 到这里返回*/
		return node;
	}

	/*是if语句 需要检查else模块*/
	if(match(ELSE) == 0){
		if(match(LBRACE) == -1){	/*匹配 { */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' {");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}

		node->third_child = stmt();	/*将else语句内容作为该节点的第三个孩子*/
		if(node->third_child){
			if(node->third_child->error){/*若孩子出错该节点标记出错 不再向前。返回*/
				node->error = 1;
				return node;
			}
		}

		if(match(RBRACE) == -1){	/*匹配 } */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' }");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}

		return node;
	}

	return node;
}

/*迭代语句*/
static syntax_node *iteration_stmt(void){
	unsigned char flag;	/*标志是否迭代语句*/
	syntax_node *node;
	char line[STR_LINE_LEN];	/*行号字符串*/

	flag = 0;
	memset(line , 0 , STR_LINE_LEN);
	sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/

	if(match(WHILE) == 0){
		flag = 1;	/*flag == 1 表示while */
	}else if(match(DO) == 0){
		flag = 2;	/*flag == 2 表示do */
	}else if(match(FOR) == 0){
		flag = 3;	/*flag == 3 表示for */
	}

	if(flag == 0){/*不是迭代语句*/
		return NULL;
	}

	/*是迭代语句*/
	node = (syntax_node *)malloc(sizeof(syntax_node));
	memset(node , 0 , sizeof(syntax_node));
	node->kind = STMT_KIND;
	node->type = ITER_STMT;
	strcpy(node->line , line);

	/*while 语句*/
	if(flag == 1){
		strcpy(node->attr.content , "while");

		if(match(LPARENT) == -1){	/*缺少 ( */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' (");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}

		node->first_child = expression();	/*第一个孩子是表达式*/
		if(node->first_child){
			if(node->first_child->error){	/*若孩子出错该节点标记出错 不再向前。返回*/
				node->error = 1;
				return node;
			}
		}

		if(match(RPARENT) == -1){	/*缺少 ) */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' )");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}


		if(match(LBRACE) == -1){	/*匹配 { */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' {");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}

		node->second_child = stmt();	/*将主体语句内容作为该节点的第二个孩子*/
		if(node->second_child){
			if(node->second_child->error){/*若孩子出错该节点标记出错 不再向前。返回*/
				node->error = 1;
				return node;
			}
		}

		if(match(RBRACE) == -1){	/*匹配 } */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' }");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}

		return node;
	}

	/*是do 语句*/
	if(flag == 2){
		strcpy(node->attr.content , "do");

		if(match(LBRACE) == -1){	/*匹配 { */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' {");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}

		node->second_child = stmt();	/*将主体语句内容作为该节点的第二个孩子*/
		if(node->second_child){
			if(node->second_child->error){/*若孩子出错该节点标记出错 不再向前。返回*/
				node->error = 1;
				return node;
			}
		}

		if(match(RBRACE) == -1){	/*匹配 } */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' }");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}

		if(match(WHILE) == -1){	/*匹配while */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' while");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}

		if(match(LPARENT) == -1){	/*缺少 ( */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' (");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}

		node->first_child = expression();	/*第一个孩子是表达式*/
		if(node->first_child){
			if(node->first_child->error){	/*若孩子出错该节点标记出错 不再向前。返回*/
				node->error = 1;
				return node;
			}
		}

		if(match(RPARENT) == -1){	/*缺少 ) */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' )");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}

		if(match(NULL_STATE) == -1){	/*缺少 ; */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' ;");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}

		return node;
	}

	/*for 语句*/
	if(flag == 3){
		strcpy(node->attr.content , "for");

		if(match(LPARENT) == -1){	/*缺少 ( */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' (");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}

		node->first_child = expression();	/*第一个孩子是初始表达式*/
		if(node->first_child){
			if(node->first_child->error){	/*若孩子出错该节点标记出错 不再向前。返回*/
				node->error = 1;
				return node;
			}
		}

		if(match(NULL_STATE) == -1){	/*缺少 ; */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' ;");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}

		node->second_child = expression();	/*第二个孩子是界限表达式*/
		if(node->second_child){
			if(node->second_child->error){	/*若孩子出错该节点标记出错 不再向前。返回*/
				node->error = 1;
				return node;
			}
		}

		if(match(NULL_STATE) == -1){	/*缺少 ; */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' ;");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}

		node->third_child = expression();	/*第三个孩子是改变表达式*/
		if(node->third_child){
			if(node->third_child->error){	/*若孩子出错该节点标记出错 不再向前。返回*/
				node->error = 1;
				return node;
			}
		}

		if(match(RPARENT) == -1){	/*缺少 ) */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' )");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}


		if(match(LBRACE) == -1){	/*匹配 { */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' {");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}

		node->last_child = stmt();	/*将主体语句内容作为该节点的最后一个个孩子*/
		if(node->last_child){
			if(node->last_child->error){/*若孩子出错该节点标记出错 不再向前。返回*/
				node->error = 1;
				return node;
			}
		}

		if(match(RBRACE) == -1){	/*匹配 } */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' }");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}

		return node;
	}

	return NULL;
}

/*跳转语句*/
static syntax_node *jump_stmt(void){
	unsigned char flag;	/*标志是否跳转语句*/
	syntax_node *node;
	char line[STR_LINE_LEN];	/*行号字符串*/

	flag = 0;
	memset(line , 0 , STR_LINE_LEN);
	sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/

	if(match(BREAK) == 0){
		flag = 1;	/*flag == 1 表示break */
	}else if(match(CONTINUE) == 0){
		flag = 2;	/*flag == 2 表示continue */
	}else if(match(GOTO) == 0){
		flag = 3;	/*flag == 3 表示goto */
	}else if(match(RETURN) == 0){
		flag = 4; /*flag == 4 表示return */
	}

	if(flag == 0){/*不是迭代语句*/
		return NULL;
	}

	/*是跳转语句*/
	node = (syntax_node *)malloc(sizeof(syntax_node));
	memset(node , 0 , sizeof(syntax_node));
	node->kind = STMT_KIND;
	node->type = JUMP_STMT;
	strcpy(node->line , line);

	if(flag == 1){/*break*/
		strcpy(node->attr.content , "break");

		if(match(NULL_STATE) == -1){	/*缺少 ; */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' ;");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}

		return node;
	}

	if(flag == 2){/*continue*/
		strcpy(node->attr.content , "continue");

		if(match(NULL_STATE) == -1){	/*缺少 ; */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' ;");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}

		return node;
	}


	if(flag == 3){/*goto*/
		strcpy(node->attr.content , "goto.");
		strcat(node->attr.content , src_buff.token_buff);

		if(match(IDENTIFIER) == -1){	/*错误的标志*/
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: illegal identifier ' ");
			strcat(node->attr.content , src_buff.token_buff);
			strcat(node->attr.content , " '");
			return node;
		}

		if(match(NULL_STATE) == -1){	/*缺少 ; */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' ;");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}
		return node;
	}

	if(flag == 4){/*return*/
		strcpy(node->attr.content , "return");

		node->first_child = expression();	/*第一个孩子是初始表达式*/
		if(node->first_child){
			if(node->first_child->error){	/*若孩子出错该节点标记出错 不再向前。返回*/
				node->error = 1;
				return node;
			}
		}

		if(match(NULL_STATE) == -1){	/*缺少 ; */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' ;");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}

		return node;
	}

	return NULL;
}
/*空语句*/
static syntax_node *null_stmt(void){
	syntax_node *node;

	if(match(NULL_STATE) == -1){
		return NULL;
	}else{
		node = (syntax_node *)malloc(sizeof(syntax_node));
		memset(node , 0 , sizeof(syntax_node));
		node->kind = STMT_KIND;
		node->type = NULL_STMT;

		return node;
	}
}

/*结构体类型定义语句*/
static syntax_node *struct_stmt(void){
	syntax_node *node;
	syntax_node *tmp_node;
	syntax_node *first_child;
	char line[STR_LINE_LEN];	/*行号字符串*/
	char buff[TYPE_SPECIFIC_LEN]; /*成员类型名*/

	if(match(STRUCTDEF) == -1){	/*不是结构体类型定义语句*/
		return NULL;
	}

	memset(line , 0 , STR_LINE_LEN);
	sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/

	/*是结构体类型语句*/
	node = (syntax_node *)malloc(sizeof(syntax_node));
	memset(node , 0 , sizeof(syntax_node));
	node->kind = STMT_KIND;
	node->type = STRUCT_STMT;
	strcpy(node->line , line);

	strcpy(node->attr.content , src_buff.token_buff); /*保存结构体类型名*/
	if(match(IDENTIFIER) == -1){	/*不是结构体类型名*/
		node->error = 1;	/*错误*/
		memset(node->attr.content , 0 , SYN_CONTENT_LEN);
		strcpy(node->attr.content , "line ");
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcat(node->attr.content , line);
		strcat(node->attr.content , ": syntax error: illegal struct type identifier ' ");
		strcat(node->attr.content , src_buff.token_buff);
		strcat(node->attr.content , " '");
		return node;
	}

	if(match(LBRACE) == -1){	/*匹配 { */
		node->error = 1;	/*错误*/
		memset(node->attr.content , 0 , SYN_CONTENT_LEN);
		strcpy(node->attr.content , "line ");
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcat(node->attr.content , line);
		strcat(node->attr.content , ": syntax error: token ' {");
		strcat(node->attr.content , " ' is nedded");
		return node;
	}

	tmp_node = node;
	while(1){
		if(match(RBRACE) == 0){	/* } 成员标志结束*/
			break;
		}

		memset(buff , 0 , TYPE_SPECIFIC_LEN);
		strcpy(buff , src_buff.token_buff);
		if(match(TYPE) == -1){	/*成员类型必须是基本类型或者结构体类型*/

			if(match(STRUCT_UNION) == -1){
				node->error = 1;	/*错误*/
				memset(node->attr.content , 0 , SYN_CONTENT_LEN);
				strcpy(node->attr.content , "line ");
				memset(line , 0 , STR_LINE_LEN);
				sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
				strcat(node->attr.content , line);
				strcat(node->attr.content , ": syntax error: illegal type of member ' ");
				strcat(node->attr.content , src_buff.token_buff);
				strcat(node->attr.content , " '");
				return node;
			}else{	/*形如struct struct_identifier 结构体类型名*/
				strcat(buff , ".");
				strcat(buff , src_buff.token_buff);
				if(match(IDENTIFIER) == -1){	/*不是结构体类型名*/
					node->error = 1;	/*错误*/
					memset(node->attr.content , 0 , SYN_CONTENT_LEN);
					strcpy(node->attr.content , "line ");
					memset(line , 0 , STR_LINE_LEN);
					sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
					strcat(node->attr.content , line);
					strcat(node->attr.content , ": syntax error: illegal struct type identifier ' ");
					strcat(node->attr.content , src_buff.token_buff);
					strcat(node->attr.content , " '");
					return node;
				}
			}

		}

		first_child = (syntax_node *)malloc(sizeof(syntax_node));
		memset(first_child , 0 , sizeof(syntax_node));
		first_child->kind = STMT_KIND;
		first_child->type = STRUCT_MEMBER;
		strcpy(first_child->attr.type_declare.type_specific , buff);	/*保存具体类型名*/
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcpy(first_child->line , line);


		while(1){	/*检查是否指针类型*/
			if(match(PREFIX) == 0){	/*这里假定是$ 指针符号*/
				strcat(first_child->attr.type_declare.nr_pointer , "$"); /*将其放入指针数据中*/
				continue;
			}else{
				break;
			}
		}


		if(match(SUBSCRIPT) == 0){	/*如果是数组*/
			strcpy(first_child->attr.type_declare.array_len , src_buff.token_buff);	/*数组长度*/
			if(match(INT_VAL) == -1){	/*必须是整形常量*/
				node->error = 1;	/*错误*/
				memset(node->attr.content , 0 , SYN_CONTENT_LEN);
				strcpy(node->attr.content , "line ");
				memset(line , 0 , STR_LINE_LEN);
				sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
				strcat(node->attr.content , line);
				strcat(node->attr.content , ": syntax error: illegal length  ' ");
				strcat(node->attr.content , src_buff.token_buff);
				strcat(node->attr.content , " ' of array");

				free(first_child);
				return node;
			}

			if(match(END_SUBSCRIPT) == -1){	/*下标结束符*/
				node->error = 1;	/*错误*/
				memset(node->attr.content , 0 , SYN_CONTENT_LEN);
				strcpy(node->attr.content , "line ");
				memset(line , 0 , STR_LINE_LEN);
				sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
				strcat(node->attr.content , line);
				strcat(node->attr.content , ": syntax error:   ' ]");
				strcat(node->attr.content , src_buff.token_buff);
				strcat(node->attr.content , " ' is nedded");

				free(first_child);
				return node;
			}

		}



		strcpy(first_child->attr.type_declare.var_name , src_buff.token_buff);	/*保存变量名*/
		if(match(IDENTIFIER) == -1){
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: illegal identifier ' ");
			strcat(node->attr.content , src_buff.token_buff);
			strcat(node->attr.content , " '");

			free(first_child);
			return node;
		}

		if(match(NULL_STATE) == -1){
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' ;");
			strcat(node->attr.content , " ' is nedded");

			free(first_child);
			return node;
		}

		tmp_node->first_child = first_child;
		tmp_node = first_child;
	}

	if(match(NULL_STATE) == -1){
		node->error = 1;	/*错误*/
		memset(node->attr.content , 0 , SYN_CONTENT_LEN);
		strcpy(node->attr.content , "line ");
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcat(node->attr.content , line);
		strcat(node->attr.content , ": syntax error: token ' ;");
		strcat(node->attr.content , " ' is nedded");
		return node;
	}

	return node;
}

/*变量声明/定义语句
 * return: NULL 不是该类型语句
 * NOT NULL: 出错类别为: ERROR_KIND 出错细节保存在attr中
 *                   正确类别为: STMT_KIND 类型为 VARIABLE_STMT
*/
static syntax_node *variable_stmt(void){
	unsigned char flag;	/*表明是否匹配成功过*/
	syntax_node *start_node;
	syntax_node *node;
	syntax_node *next_node;
	char line[STR_LINE_LEN];	/*行号字符串*/


	flag = 0;
	memset(line , 0 , STR_LINE_LEN);
	sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/

	node = (syntax_node *)malloc(sizeof(syntax_node));
	memset(node , 0 , sizeof(syntax_node));
	node->kind = STMT_KIND;	/*设置类别类型*/
	node->type = VAR_STMT;
	strcpy(node->line , line);

	start_node = node;	/*记录第一个节点*/

	strcpy(node->attr.type_declare.storage_class , src_buff.token_buff); 	/*首先假设是存储类型*/
	if(match(STORAGE_CLASS) == -1){	/*表明不是storage_class 那么使用默认类型*/
		memset(node->attr.type_declare.storage_class , 0 , STORAGE_CLASS_LEN);
		strcpy(node->attr.type_declare.storage_class , "auto");
	}else{
		flag = 1;
	}

	strcpy(node->attr.type_declare.type_qualifier , src_buff.token_buff); 	/*假设是类型限定符*/
	if(match(TYPE_QUALIFIER) == -1){	/*不是type_qualifier 那么清空之前的设置*/
		memset(node->attr.type_declare.type_qualifier , 0 , TYPE_QUALIFIER_LEN);
	}else{
		flag = 1;
	}

	strcpy(node->attr.type_declare.type_specific , src_buff.token_buff);
	if(match(TYPE) == -1){	/*成员类型必须是基本类型或者结构体类型*/

		if(match(STRUCT_UNION) == -1){
			if(flag){	/*标明之前已经有匹配但是无类型声明*/
				node->error = 1;	/*错误*/
				memset(node->attr.content , 0 , SYN_CONTENT_LEN);
				strcpy(node->attr.content , "line ");
				memset(line , 0 , STR_LINE_LEN);
				sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
				strcat(node->attr.content , line);
				strcat(node->attr.content , ": syntax error: unknown type  ' ");
				strcat(node->attr.content , src_buff.token_buff);
				strcat(node->attr.content , " '");
				return node;
			}else{/*之前没有匹配表示不是此类语句*/
				free(node);
				return NULL;
			}
		}else{	/*形如struct struct_identifier 结构体类型名*/
			strcat(node->attr.type_declare.type_specific , ".");
			strcat(node->attr.type_declare.type_specific , src_buff.token_buff);
			if(match(IDENTIFIER) == -1){	/*不是结构体类型名*/
				node->error = 1;	/*错误*/
				memset(node->attr.content , 0 , SYN_CONTENT_LEN);
				strcpy(node->attr.content , "line ");
				memset(line , 0 , STR_LINE_LEN);
				sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
				strcat(node->attr.content , line);
				strcat(node->attr.content , ": syntax error: illegal struct type identifier ' ");
				strcat(node->attr.content , src_buff.token_buff);
				strcat(node->attr.content , " '");
				return node;
			}
		}

	}

	/*之前成功 进行下一步匹配*/
	flag = 1;

generate_var_node:

	while(1){	/*检查是否指针类型*/
		if(match(PREFIX) == 0){	/*这里假定是$ 指针符号*/
			strcat(node->attr.type_declare.nr_pointer , "$"); /*将其放入具体类型中*/
			continue;
		}else{
			break;
		}
	}

	strcpy(node->attr.type_declare.var_name , src_buff.token_buff);	/*匹配变量名*/
	if(match(IDENTIFIER) == -1){	/*不是标识符 出错*/
		node->error = 1;	/*错误*/
		memset(node->attr.content , 0 , SYN_CONTENT_LEN);
		strcpy(node->attr.content , "line ");
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcat(node->attr.content , line);
		strcat(node->attr.content , ": syntax error: illegal identifier ' ");
		strcat(node->attr.content , src_buff.token_buff);
		strcat(node->attr.content , " '");

		start_node->error = 1;
		return start_node;
	}


	if(match(SUBSCRIPT) == 0){	/*如果是数组*/
		strcpy(node->attr.type_declare.array_len , src_buff.token_buff);	/*数组长度*/
		if(match(INT_VAL) == -1){	/*必须是整形常量*/
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: illegal length  ' ");
			strcat(node->attr.content , src_buff.token_buff);
			strcat(node->attr.content , " ' of array");

			start_node->error = 1;
			return start_node;
		}

		if(match(END_SUBSCRIPT) == -1){	/*下标结束符*/
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error:   ' ]");
			strcat(node->attr.content , src_buff.token_buff);
			strcat(node->attr.content , " ' is nedded");

			start_node->error = 1;
			return start_node;
		}

	}

	if(match(SEQUEN) == 0){	/*如果声明多个变量就生成新节点，将其作为首节点之子*/
		next_node = (syntax_node *)malloc(sizeof(syntax_node));
		memset(next_node , 0 , sizeof(syntax_node));
		next_node->kind = STMT_KIND;
		next_node->type = VAR_STMT;

		strcpy(next_node->attr.type_declare.storage_class , node->attr.type_declare.storage_class);	/*新节点与之前节点有相同的存储类型、类型限定、具体类型*/
		strcpy(next_node->attr.type_declare.type_qualifier , node->attr.type_declare.type_qualifier);
		strcpy(next_node->attr.type_declare.type_specific , node->attr.type_declare.type_specific);

		node->first_child = next_node;
		node = next_node;
		goto generate_var_node;
	}

	if(match(NULL_STATE) == -1){	/*标志结束 ; */
		node->error = 1;	/*错误*/
		memset(node->attr.content , 0 , SYN_CONTENT_LEN);
		strcpy(node->attr.content , "line ");
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcat(node->attr.content , line);
		strcat(node->attr.content , ": syntax error: before ");
		strcat(node->attr.content , src_buff.token_buff);
		strcat(node->attr.content , " ';' is nedded");

		start_node->error = 1;
		return start_node;
	}

	/*最后成功返回节点*/
	return start_node;
}

/*函数声明/定义语句*/
static syntax_node *function_stmt(void){
	unsigned char count;	/*计算参数个数。用于防止无限循环*/
	syntax_node *start_node;
	syntax_node *node;
	syntax_node *next_node;
	char line[STR_LINE_LEN];	/*行号字符串*/

	if(match(FUNCTION) == -1){	/*首先观察是否以function开始。如果不是则标明不是函数语句*/
		return NULL;
	}

	/*是函数语句*/
	count = 0;
	memset(line , 0 , STR_LINE_LEN);
	sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/

	node = (syntax_node *)malloc(sizeof(syntax_node));
	memset(node , 0 , sizeof(syntax_node));
	node->kind = FUNC_KIND;
	strcpy(node->line , line);

	start_node = node;

	strcpy(node->attr.type_declare.storage_class , src_buff.token_buff); 	/*首先假设是存储类型*/
	if(match(STORAGE_CLASS) == -1){	/*表明不是storage_class 那么返回值是默认类型*/
		memset(node->attr.type_declare.storage_class , 0 , STORAGE_CLASS_LEN);
		strcpy(node->attr.type_declare.storage_class , "auto");
	}

	strcpy(node->attr.type_declare.type_qualifier , src_buff.token_buff); 	/*假设是类型限定符*/
	if(match(TYPE_QUALIFIER) == -1){	/*不是type_qualifier 那么清空之前的设置*/
		memset(node->attr.type_declare.type_qualifier , 0 , TYPE_QUALIFIER_LEN);
	}

	strcpy(node->attr.type_declare.type_specific , src_buff.token_buff);
	if(match(TYPE) == -1){	/*返回值类型必须是基本类型或者结构体类型*/

		if(match(STRUCT_UNION) == -1){
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: unknown type  ' ");
			strcat(node->attr.content , src_buff.token_buff);
			strcat(node->attr.content , " '");
			return node;
		}else{	/*形如struct struct_identifier 结构体类型名*/
			strcat(node->attr.type_declare.type_specific , ".");
			strcat(node->attr.type_declare.type_specific , src_buff.token_buff);
			if(match(IDENTIFIER) == -1){	/*不是结构体类型名*/
				node->error = 1;	/*错误*/
				memset(node->attr.content , 0 , SYN_CONTENT_LEN);
				strcpy(node->attr.content , "line ");
				memset(line , 0 , STR_LINE_LEN);
				sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
				strcat(node->attr.content , line);
				strcat(node->attr.content , ": syntax error: illegal struct type identifier ' ");
				strcat(node->attr.content , src_buff.token_buff);
				strcat(node->attr.content , " '");
				return node;
			}
		}
	}


	/*之前成功 进行下一步匹配*/
	while(1){	/*检查是否指针类型*/
		if(match(PREFIX) == 0){	/*这里假定是$ 指针符号*/
			strcat(node->attr.type_declare.nr_pointer , "$"); /*将其放入具体类型中*/
			continue;
		}else{
			break;
		}
	}

	strcpy(node->attr.type_declare.var_name , src_buff.token_buff);	/*匹配函数名*/
	if(match(IDENTIFIER) == -1){	/*不是标识符 出错*/
		node->error = 1;	/*错误*/
		memset(node->attr.content , 0 , SYN_CONTENT_LEN);
		strcpy(node->attr.content , "line ");
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcat(node->attr.content , line);
		strcat(node->attr.content , ": syntax error: illegal identifier ' ");
		strcat(node->attr.content , src_buff.token_buff);
		strcat(node->attr.content , " '");
		return node;
	}

	if(match(LPARENT) == -1){	/*开始匹配输入参数之左括弧*/
		node->error = 1;	/*错误*/
		memset(node->attr.content , 0 , SYN_CONTENT_LEN);
		strcpy(node->attr.content , "line ");
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcat(node->attr.content , line);
		strcat(node->attr.content , ": syntax error: function declare needs ' (");
		strcat(node->attr.content , " '");
		return node;
	}

	if(match(RPARENT) == 0){	/*参数结束标志 ) */
		/*表明没有参数*/
	}else{
		while(1){/*获得参数。每个参数作为一个var_stmt节点作为其孩子*/

			next_node = (syntax_node *)(malloc(sizeof(syntax_node)));
			memset(next_node , 0 , sizeof(syntax_node));
			next_node->kind = STMT_KIND;
			next_node->type = VAR_STMT;
			strcpy(next_node->line , line);
			node->first_child = next_node; /*将该节点作为之前节点的子节点*/

			strcpy(next_node->attr.type_declare.type_qualifier , src_buff.token_buff); 	/*假设是类型限定符*/
			if(match(TYPE_QUALIFIER) == -1){	/*不是type_qualifier 那么清空之前的设置*/
				memset(next_node->attr.type_declare.type_qualifier , 0 , TYPE_QUALIFIER_LEN);
			}

			strcpy(next_node->attr.type_declare.type_specific , src_buff.token_buff);
			if(match(TYPE) == -1){	/*返回值类型必须是基本类型或者结构体类型*/

				if(match(STRUCT_UNION) == -1){
					next_node->error = 1;	/*错误*/
					memset(next_node->attr.content , 0 , SYN_CONTENT_LEN);
					strcpy(next_node->attr.content , "line ");
					memset(line , 0 , STR_LINE_LEN);
					sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
					strcat(next_node->attr.content , line);
					strcat(next_node->attr.content , ": $$$syntax error: unknown type  ' ");
					strcat(next_node->attr.content , src_buff.token_buff);
					strcat(next_node->attr.content , " '");

					start_node->error = 1;
					return start_node;	/*注意返回的是函数节点*/
				}else{	/*形如struct struct_identifier 结构体类型名*/
					strcat(next_node->attr.type_declare.type_specific , ".");
					strcat(next_node->attr.type_declare.type_specific , src_buff.token_buff);
					if(match(IDENTIFIER) == -1){	/*不是结构体类型名*/
						next_node->error = 1;	/*错误*/
						memset(next_node->attr.content , 0 , SYN_CONTENT_LEN);
						strcpy(next_node->attr.content , "line ");
						memset(line , 0 , STR_LINE_LEN);
						sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
						strcat(next_node->attr.content , line);
						strcat(next_node->attr.content , ": syntax error: illegal struct type identifier ' ");
						strcat(next_node->attr.content , src_buff.token_buff);
						strcat(next_node->attr.content , " '");

						start_node->error = 1;
						return start_node;
					}
				}

			}

			while(1){	/*检查是否指针类型*/
				if(match(PREFIX) == 0){	/*这里假定是$ 指针符号*/
					strcat(next_node->attr.type_declare.nr_pointer , "$"); /*将其放入具体类型中*/
					continue;
				}else{
					break;
				}
			}

			strcpy(next_node->attr.type_declare.var_name , src_buff.token_buff);	/*匹配标识符名*/
			if(match(IDENTIFIER) == -1){	/*不是标识符 出错*/
				next_node->error = 1;	/*错误*/
				memset(next_node->attr.content , 0 , SYN_CONTENT_LEN);
				strcpy(next_node->attr.content , "line ");
				memset(line , 0 , STR_LINE_LEN);
				sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
				strcat(next_node->attr.content , line);
				strcat(next_node->attr.content , ": syntax error: illegal identifier ' ");
				strcat(next_node->attr.content , src_buff.token_buff);
				strcat(next_node->attr.content , " '");

				start_node->error = 1;
				return start_node;
			}

			if(match(SEQUEN) == 0){	/* , 标明多个参数*/
				node = next_node;	/*改变当前节点*/
			}else{	/*如果没有下一个参数则必须是参数结束标识符 ) */
				if(match(RPARENT) == 0){	/*参数结束标志 ) */
					break;	/*退出参数获取*/
				}else{	/*其他符号都是非法*/
					next_node->error = 1;	/*错误*/
					memset(next_node->attr.content , 0 , SYN_CONTENT_LEN);
					strcpy(next_node->attr.content , "line ");
					memset(line , 0 , STR_LINE_LEN);
					sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
					strcat(next_node->attr.content , line);
					strcat(next_node->attr.content , ": syntax error: illegal symbol ' ");
					strcat(next_node->attr.content , src_buff.token_buff);
					strcat(next_node->attr.content , " '");

					start_node->error = 1;
					return start_node;
				}
			}


		}	/*end add param*/

	}

	/*选择是函数声明还是定义*/

	if(match(NULL_STATE) == 0){	/*表明这是函数声明*/
		start_node->type = FUNC_DECL;
		return start_node;
	}

	if(match(LBRACE) == -1){	/*既不是函数声明也不是定义 出错*/
		start_node->error = 1;	/*错误*/
		memset(start_node->attr.content , 0 , SYN_CONTENT_LEN);
		strcpy(start_node->attr.content , "line ");
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcat(start_node->attr.content , line);
		strcat(start_node->attr.content , ": syntax error:   ' ;");
		strcat(start_node->attr.content , " ' is nedded");

		return start_node;
	}else{	/*标明是函数定义*/
		start_node->type = FUNC_DEFINE;
		start_node->second_child = stmt();	/*其第二孩子是函数体*/

		if(start_node->second_child){
			if(start_node->second_child->error == 1){	/*检查是否出错*/
				start_node->error = 1;
				return start_node;
			}
		}

		if(match(RBRACE) == -1){	/*匹配右大括弧*/
			start_node->error = 1;	/*错误*/
			memset(start_node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(start_node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(start_node->attr.content , line);
			strcat(start_node->attr.content , ": syntax error:  before ' }");
			strcat(start_node->attr.content , " ' is nedded");
		}

		return start_node;
	}

	return NULL;
}

/*表达式*/
static syntax_node *expression(void){
	return seq_exp();
}

/*句号表达式*/
static syntax_node *seq_exp(void){
	unsigned char flag;	/*标记是否是句号表达式*/
	syntax_node *node;	/*包含句号操作符的节点*/
	syntax_node *first_child;
	char line[STR_LINE_LEN];	/*行号字符串*/

	flag = 0;
	first_child = assign_exp();

	if(first_child){
		if(first_child->error){	/*如果子节点出错。不产生当前节点，直接返回错误节点*/
			return first_child;
		}
	}else{	/*如果第一个孩子为空那么该节点显然为空*/
		return NULL;
	}

	/*检查是否句号表达式*/
	while(1){
		if(match(SEQUEN) == 0){
			flag = 1;	/*标明是句号表达式节点*/

			node = (syntax_node *)malloc(sizeof(syntax_node));
			memset(node , 0 , sizeof(syntax_node));
			node->kind = EXP_KIND;
			node->type = SEQ_EXP;
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcpy(node->line , line);

			node->first_child = first_child;

			node->second_child = assign_exp();
			if(node->second_child){
				if(node->second_child->error){	/*子节点出错，返回当前节点*/
					node->error = 1;
					return node;
				}
			}else{	/*如果下一个孩子节点为空显然出错 二元操作符少了右边操作数 相当于出现了 xx , ?? */
				node->error = 1;
				memset(line , 0 , STR_LINE_LEN);
				memset(node->attr.content , 0 , SYN_CONTENT_LEN);
				strcpy(node->attr.content , "line ");
				sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
				strcat(node->attr.content , line);
				strcat(node->attr.content , ": syntax error: more exp is needed after ' ,");
				strcat(node->attr.content , " '" );
				return node;
			}

			first_child = node;	/*当前节点作为新的孩子节点*/
			continue;
		}

		/*没有新的句号标识*/
			break;
	}

	if(flag == 1){	/*如果是该类型表达式 返回当前节点*/
		return node;
	}
	/*不是该类型节点返回第一个孩子节点*/
	return first_child;
}

/*赋值表达式 注意赋值表达式不允许连续赋值*/
static syntax_node *assign_exp(void){
	unsigned char flag;	/*标记是否是赋值表达式*/
	syntax_node *node;	/*包含句号操作符的节点*/
	syntax_node *first_child;
	char line[STR_LINE_LEN];	/*行号字符串*/
	char opt_name[OPT_NAME_LEN];	/*操作符名字*/

	flag = 0;
	first_child = logic_or_exp();

	if(first_child){
		if(first_child->error){	/*如果子节点出错。不产生当前节点，直接返回错误节点*/
			return first_child;
		}
	}else{	/*如果第一个孩子为空那么该节点显然为空*/
		return NULL;
	}

	/*检查是否赋值表达式*/
	memset(opt_name , 0 , OPT_NAME_LEN);	/*在匹配之前先保存操作符*/
	strcpy(opt_name , src_buff.token_buff);

	if(match(ASSIGN) == 0){
		flag = 1;	/*标明是赋值表达式节点*/

		node = (syntax_node *)malloc(sizeof(syntax_node));
		memset(node , 0 , sizeof(syntax_node));
		node->kind = EXP_KIND;
		node->type = ASSIGN_EXP;
		strcpy(node->attr.opt_declare.opt_name , opt_name);
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcpy(node->line , line);


		node->first_child = first_child;

		if(first_child->kind == EXP_KIND){	/*对于赋值表达式来说，左操作数必须是一元式。因此在这里进行检测*/
			if(first_child->type != UNARY_EXP){
				node->error = 1;
				memset(line , 0 , STR_LINE_LEN);
				memset(node->attr.content , 0 , SYN_CONTENT_LEN);
				strcpy(node->attr.content , "line ");
				sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
				strcat(node->attr.content , line);
				strcat(node->attr.content , ": syntax error: unary exp is nedded before ' ");
				strcat(node->attr.content , opt_name);
				strcat(node->attr.content , " '" );
				return node;
			}
		}

		node->second_child = logic_or_exp();
		if(node->second_child){
			if(node->second_child->error){	/*子节点出错，返回当前节点*/
				node->error = 1;
				return node;
			}
		}else{	/*如果下一个孩子节点为空显然出错 二元操作符少了右边操作数 相当于出现了 xx = ?? */
			node->error = 1;
			memset(line , 0 , STR_LINE_LEN);
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: more exp is needed after ' ");
			strcat(node->attr.content , opt_name);
			strcat(node->attr.content , " '" );
			return node;
		}

	}	/*end match*/

	if(flag == 1){	/*如果是该类型表达式 返回当前节点*/
		return node;
	}
	/*不是该类型节点返回第一个孩子节点*/
	return first_child;
}

/*逻辑或表达式*/
static syntax_node *logic_or_exp(void){
	unsigned char flag;	/*标记是否是逻辑或表达式*/
	syntax_node *node;	/*包含句号操作符的节点*/
	syntax_node *first_child;
	char line[STR_LINE_LEN];	/*行号字符串*/

	flag = 0;
	first_child = logic_and_exp();

	if(first_child){
		if(first_child->error){	/*如果子节点出错。不产生当前节点，直接返回错误节点*/
			return first_child;
		}
	}else{	/*如果第一个孩子为空那么该节点显然为空*/
		return NULL;
	}

	/*检查是否逻辑或表达式*/
	while(1){
		if(match(LOGIC_OR) == 0){
			flag = 1;	/*标明是逻辑或表达式节点*/

			node = (syntax_node *)malloc(sizeof(syntax_node));
			memset(node , 0 , sizeof(syntax_node));
			node->kind = EXP_KIND;
			node->type = LOGIC_OR_EXP;
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcpy(node->line , line);

			node->first_child = first_child;

			node->second_child = logic_and_exp();
			if(node->second_child){
				if(node->second_child->error){	/*子节点出错，返回当前节点*/
					node->error = 1;
					return node;
				}
			}else{	/*如果下一个孩子节点为空显然出错 二元操作符少了右边操作数 相当于出现了 xx || ?? */
				node->error = 1;
				memset(line , 0 , STR_LINE_LEN);
				memset(node->attr.content , 0 , SYN_CONTENT_LEN);
				strcpy(node->attr.content , "line ");
				sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
				strcat(node->attr.content , line);
				strcat(node->attr.content , ": syntax error: more exp is needed after ' ||");
				strcat(node->attr.content , " '" );
				return node;
			}

			first_child = node;	/*当前节点作为新的孩子节点*/
			continue;
		}

		/*没有新的句号标识*/
			break;
	}

	if(flag == 1){	/*如果是该类型表达式 返回当前节点*/
		return node;
	}
	/*不是该类型节点返回第一个孩子节点*/
	return first_child;
}

/*逻辑与表达式*/
static syntax_node *logic_and_exp(void){
	unsigned char flag;	/*标记是否是逻辑与表达式*/
	syntax_node *node;	/*包含句号操作符的节点*/
	syntax_node *first_child;
	char line[STR_LINE_LEN];	/*行号字符串*/

	flag = 0;
	first_child = bit_or_exp();

	if(first_child){
		if(first_child->error){	/*如果子节点出错。不产生当前节点，直接返回错误节点*/
			return first_child;
		}
	}else{	/*如果第一个孩子为空那么该节点显然为空*/
		return NULL;
	}

	/*检查是否逻辑与表达式*/
	while(1){
		if(match(LOGIC_AND) == 0){
			flag = 1;	/*标明是逻辑与表达式节点*/

			node = (syntax_node *)malloc(sizeof(syntax_node));
			memset(node , 0 , sizeof(syntax_node));
			node->kind = EXP_KIND;
			node->type = LOGIC_AND_EXP;
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcpy(node->line , line);

			node->first_child = first_child;

			node->second_child = bit_or_exp();
			if(node->second_child){
				if(node->second_child->error){	/*子节点出错，返回当前节点*/
					node->error = 1;
					return node;
				}
			}else{	/*如果下一个孩子节点为空显然出错 二元操作符少了右边操作数 相当于出现了 xx && ?? */
				node->error = 1;
				memset(line , 0 , STR_LINE_LEN);
				memset(node->attr.content , 0 , SYN_CONTENT_LEN);
				strcpy(node->attr.content , "line ");
				sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
				strcat(node->attr.content , line);
				strcat(node->attr.content , ": syntax error: more exp is needed after ' &&");
				strcat(node->attr.content , " '" );
				return node;
			}

			first_child = node;	/*当前节点作为新的孩子节点*/
			continue;
		}

		/*没有新的句号标识*/
			break;
	}

	if(flag == 1){	/*如果是该类型表达式 返回当前节点*/
		return node;
	}
	/*不是该类型节点返回第一个孩子节点*/
	return first_child;
}

/*按位或表达式*/
static syntax_node *bit_or_exp(void){
	unsigned char flag;	/*标记是否是按位或表达式*/
	syntax_node *node;	/*包含句号操作符的节点*/
	syntax_node *first_child;
	char line[STR_LINE_LEN];	/*行号字符串*/

	flag = 0;
	first_child = bit_xor_exp();

	if(first_child){
		if(first_child->error){	/*如果子节点出错。不产生当前节点，直接返回错误节点*/
			return first_child;
		}
	}else{	/*如果第一个孩子为空那么该节点显然为空*/
		return NULL;
	}

	/*检查是否按位或表达式*/
	while(1){
		if(match(BIT_OR) == 0){
			flag = 1;	/*标明是按位或表达式节点*/

			node = (syntax_node *)malloc(sizeof(syntax_node));
			memset(node , 0 , sizeof(syntax_node));
			node->kind = EXP_KIND;
			node->type = BIT_OR_EXP;
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcpy(node->line , line);

			node->first_child = first_child;

			node->second_child = bit_xor_exp();
			if(node->second_child){
				if(node->second_child->error){	/*子节点出错，返回当前节点*/
					node->error = 1;
					return node;
				}
			}else{	/*如果下一个孩子节点为空显然出错 二元操作符少了右边操作数 相当于出现了 xx | ?? */
				node->error = 1;
				memset(line , 0 , STR_LINE_LEN);
				memset(node->attr.content , 0 , SYN_CONTENT_LEN);
				strcpy(node->attr.content , "line ");
				sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
				strcat(node->attr.content , line);
				strcat(node->attr.content , ": syntax error: more exp is needed after ' |");
				strcat(node->attr.content , " '" );
				return node;
			}

			first_child = node;	/*当前节点作为新的孩子节点*/
			continue;
		}

		/*没有新的句号标识*/
			break;
	}

	if(flag == 1){	/*如果是该类型表达式 返回当前节点*/
		return node;
	}
	/*不是该类型节点返回第一个孩子节点*/
	return first_child;
}

/*按位异或表达式*/
static syntax_node *bit_xor_exp(void){
	unsigned char flag;	/*标记是否是按位异或表达式*/
	syntax_node *node;	/*包含句号操作符的节点*/
	syntax_node *first_child;
	char line[STR_LINE_LEN];	/*行号字符串*/

	flag = 0;
	first_child = bit_and_exp();

	if(first_child){
		if(first_child->error){	/*如果子节点出错。不产生当前节点，直接返回错误节点*/
			return first_child;
		}
	}else{	/*如果第一个孩子为空那么该节点显然为空*/
		return NULL;
	}

	/*检查是否按位异或表达式*/
	while(1){
		if(match(BIT_XOR) == 0){
			flag = 1;	/*标明是按位异或表达式节点*/

			node = (syntax_node *)malloc(sizeof(syntax_node));
			memset(node , 0 , sizeof(syntax_node));
			node->kind = EXP_KIND;
			node->type = BIT_XOR_EXP;
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcpy(node->line , line);

			node->first_child = first_child;

			node->second_child = bit_and_exp();
			if(node->second_child){
				if(node->second_child->error){	/*子节点出错，返回当前节点*/
					node->error = 1;
					return node;
				}
			}else{	/*如果下一个孩子节点为空显然出错 二元操作符少了右边操作数 相当于出现了 xx ^ ?? */
				node->error = 1;
				memset(line , 0 , STR_LINE_LEN);
				memset(node->attr.content , 0 , SYN_CONTENT_LEN);
				strcpy(node->attr.content , "line ");
				sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
				strcat(node->attr.content , line);
				strcat(node->attr.content , ": syntax error: more exp is needed after ' ^");
				strcat(node->attr.content , " '" );
				return node;
			}

			first_child = node;	/*当前节点作为新的孩子节点*/
			continue;
		}

		/*没有新的句号标识*/
			break;
	}

	if(flag == 1){	/*如果是该类型表达式 返回当前节点*/
		return node;
	}
	/*不是该类型节点返回第一个孩子节点*/
	return first_child;
}

/*按位与表达式*/
static syntax_node *bit_and_exp(void){
	unsigned char flag;	/*标记是否是按位与表达式*/
	syntax_node *node;	/*包含句号操作符的节点*/
	syntax_node *first_child;
	char line[STR_LINE_LEN];	/*行号字符串*/

	flag = 0;
	first_child = equal_exp();

	if(first_child){
		if(first_child->error){	/*如果子节点出错。不产生当前节点，直接返回错误节点*/
			return first_child;
		}
	}else{	/*如果第一个孩子为空那么该节点显然为空*/
		return NULL;
	}

	/*检查是否按位与表达式*/
	while(1){
		if(match(BIT_AND) == 0){
			flag = 1;	/*标明是按位与表达式节点*/

			node = (syntax_node *)malloc(sizeof(syntax_node));
			memset(node , 0 , sizeof(syntax_node));
			node->kind = EXP_KIND;
			node->type = BIT_AND_EXP;
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcpy(node->line , line);

			node->first_child = first_child;

			node->second_child = equal_exp();
			if(node->second_child){
				if(node->second_child->error){	/*子节点出错，返回当前节点*/
					node->error = 1;
					return node;
				}
			}else{	/*如果下一个孩子节点为空显然出错 二元操作符少了右边操作数 相当于出现了 xx & ?? */
				node->error = 1;
				memset(line , 0 , STR_LINE_LEN);
				memset(node->attr.content , 0 , SYN_CONTENT_LEN);
				strcpy(node->attr.content , "line ");
				sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
				strcat(node->attr.content , line);
				strcat(node->attr.content , ": syntax error: more exp is needed after ' &");
				strcat(node->attr.content , " '" );
				return node;
			}

			first_child = node;	/*当前节点作为新的孩子节点*/
			continue;
		}

		/*没有新的句号标识*/
			break;
	}

	if(flag == 1){	/*如果是该类型表达式 返回当前节点*/
		return node;
	}
	/*不是该类型节点返回第一个孩子节点*/
	return first_child;
}


/*等价表达式*/
static syntax_node *equal_exp(void){
	unsigned char flag;	/*标记是否是等价表达式*/
	syntax_node *node;	/*包含句号操作符的节点*/
	syntax_node *first_child;
	char line[STR_LINE_LEN];	/*行号字符串*/
	char opt_name[OPT_NAME_LEN];	/*操作符名字*/

	flag = 0;
	first_child = relation_exp();

	if(first_child){
		if(first_child->error){	/*如果子节点出错。不产生当前节点，直接返回错误节点*/
			return first_child;
		}
	}else{	/*如果第一个孩子为空那么该节点显然为空*/
		return NULL;
	}

	/*检查是否等价表达式*/
	while(1){
		memset(opt_name , 0 , OPT_NAME_LEN);	/*在匹配之前先保存操作符*/
		strcpy(opt_name , src_buff.token_buff);

		if(match(EQUALITY) == 0){
			flag = 1;	/*标明是等价表达式节点*/

			node = (syntax_node *)malloc(sizeof(syntax_node));
			memset(node , 0 , sizeof(syntax_node));
			node->kind = EXP_KIND;
			node->type = EQUAL_EXP;
			strcpy(node->attr.opt_declare.opt_name , opt_name);
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcpy(node->line , line);

			node->first_child = first_child;

			node->second_child = relation_exp();
			if(node->second_child){
				if(node->second_child->error){	/*子节点出错，返回当前节点*/
					node->error = 1;
					return node;
				}
			}else{	/*如果下一个孩子节点为空显然出错 二元操作符少了右边操作数 相当于出现了 xx == != ?? */
				node->error = 1;
				memset(line , 0 , STR_LINE_LEN);
				memset(node->attr.content , 0 , SYN_CONTENT_LEN);
				strcpy(node->attr.content , "line ");
				sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
				strcat(node->attr.content , line);
				strcat(node->attr.content , ": syntax error: more exp is needed after ' ");
				strcat(node->attr.content , opt_name);
				strcat(node->attr.content , " '" );
				return node;
			}

			first_child = node;	/*当前节点作为新的孩子节点*/
			continue;
		}

		/*没有新的句号标识*/
			break;
	}

	if(flag == 1){	/*如果是该类型表达式 返回当前节点*/
		return node;
	}
	/*不是该类型节点返回第一个孩子节点*/
	return first_child;
}


/*关系表达式*/
static syntax_node *relation_exp(void){
	unsigned char flag;	/*标记是否是关系表达式*/
	syntax_node *node;	/*包含句号操作符的节点*/
	syntax_node *first_child;
	char line[STR_LINE_LEN];	/*行号字符串*/
	char opt_name[OPT_NAME_LEN];	/*操作符名字*/

	flag = 0;
	first_child = shift_exp();

	if(first_child){
		if(first_child->error){	/*如果子节点出错。不产生当前节点，直接返回错误节点*/
			return first_child;
		}
	}else{	/*如果第一个孩子为空那么该节点显然为空*/
		return NULL;
	}

	/*检查是否关系表达式*/
	while(1){
		memset(opt_name , 0 , OPT_NAME_LEN);	/*在匹配之前先保存操作符*/
		strcpy(opt_name , src_buff.token_buff);

		if(match(RELATION) == 0){
			flag = 1;	/*标明是关系表达式节点*/

			node = (syntax_node *)malloc(sizeof(syntax_node));
			memset(node , 0 , sizeof(syntax_node));
			node->kind = EXP_KIND;
			node->type = RELATION_EXP;
			strcpy(node->attr.opt_declare.opt_name , opt_name);
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcpy(node->line , line);

			node->first_child = first_child;

			node->second_child = shift_exp();
			if(node->second_child){
				if(node->second_child->error){	/*子节点出错，返回当前节点*/
					node->error = 1;
					return node;
				}
			}else{	/*如果下一个孩子节点为空显然出错 二元操作符少了右边操作数 相当于出现了 xx > < <= >=  ?? */
				node->error = 1;
				memset(line , 0 , STR_LINE_LEN);
				memset(node->attr.content , 0 , SYN_CONTENT_LEN);
				strcpy(node->attr.content , "line ");
				sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
				strcat(node->attr.content , line);
				strcat(node->attr.content , ": syntax error: more exp is needed after ' ");
				strcat(node->attr.content , opt_name);
				strcat(node->attr.content , " '" );
				return node;
			}

			first_child = node;	/*当前节点作为新的孩子节点*/
			continue;
		}

		/*没有新的句号标识*/
			break;
	}

	if(flag == 1){	/*如果是该类型表达式 返回当前节点*/
		return node;
	}
	/*不是该类型节点返回第一个孩子节点*/
	return first_child;
}


/*移位表达式*/
static syntax_node *shift_exp(void){
	unsigned char flag;	/*标记是否是移位表达式*/
	syntax_node *node;	/*包含句号操作符的节点*/
	syntax_node *first_child;
	char line[STR_LINE_LEN];	/*行号字符串*/
	char opt_name[OPT_NAME_LEN];	/*操作符名字*/

	flag = 0;
	first_child = additive_exp();

	if(first_child){
		if(first_child->error){	/*如果子节点出错。不产生当前节点，直接返回错误节点*/
			return first_child;
		}
	}else{	/*如果第一个孩子为空那么该节点显然为空*/
		return NULL;
	}

	/*检查是否移位表达式*/
	while(1){
		memset(opt_name , 0 , OPT_NAME_LEN);	/*在匹配之前先保存操作符*/
		strcpy(opt_name , src_buff.token_buff);

		if(match(SHIFT) == 0){
			flag = 1;	/*标明是移位表达式节点*/

			node = (syntax_node *)malloc(sizeof(syntax_node));
			memset(node , 0 , sizeof(syntax_node));
			node->kind = EXP_KIND;
			node->type = SHIFT_EXP;
			strcpy(node->attr.opt_declare.opt_name , opt_name);
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcpy(node->line , line);

			node->first_child = first_child;

			node->second_child = additive_exp();
			if(node->second_child){
				if(node->second_child->error){	/*子节点出错，返回当前节点*/
					node->error = 1;
					return node;
				}
			}else{	/*如果下一个孩子节点为空显然出错 二元操作符少了右边操作数 相当于出现了 xx >> <<  ?? */
				node->error = 1;
				memset(line , 0 , STR_LINE_LEN);
				memset(node->attr.content , 0 , SYN_CONTENT_LEN);
				strcpy(node->attr.content , "line ");
				sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
				strcat(node->attr.content , line);
				strcat(node->attr.content , ": syntax error: more exp is needed after ' ");
				strcat(node->attr.content , opt_name);
				strcat(node->attr.content , " '" );
				return node;
			}

			first_child = node;	/*当前节点作为新的孩子节点*/
			continue;
		}

		/*没有新的句号标识*/
			break;
	}

	if(flag == 1){	/*如果是该类型表达式 返回当前节点*/
		return node;
	}
	/*不是该类型节点返回第一个孩子节点*/
	return first_child;
}


/*加减表达式*/
static syntax_node *additive_exp(void){
	unsigned char flag;	/*标记是否是加减表达式*/
	syntax_node *node;	/*包含句号操作符的节点*/
	syntax_node *first_child;
	char line[STR_LINE_LEN];	/*行号字符串*/
	char opt_name[OPT_NAME_LEN];	/*操作符名字*/

	flag = 0;
	first_child = multi_exp();

	if(first_child){
		if(first_child->error){	/*如果子节点出错。不产生当前节点，直接返回错误节点*/
			return first_child;
		}
	}else{	/*如果第一个孩子为空那么该节点显然为空*/
		return NULL;
	}

	/*检查是否加减表达式*/
	while(1){
		memset(opt_name , 0 , OPT_NAME_LEN);	/*在匹配之前先保存操作符*/
		strcpy(opt_name , src_buff.token_buff);

		if(match(ADDITIVE) == 0){
			flag = 1;	/*标明是加减表达式节点*/

			node = (syntax_node *)malloc(sizeof(syntax_node));
			memset(node , 0 , sizeof(syntax_node));
			node->kind = EXP_KIND;
			node->type = ADDITIVE_EXP;
			strcpy(node->attr.opt_declare.opt_name , opt_name);
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcpy(node->line , line);

			node->first_child = first_child;

			node->second_child = multi_exp();
			if(node->second_child){
				if(node->second_child->error){	/*子节点出错，返回当前节点*/
					node->error = 1;
					return node;
				}
			}else{	/*如果下一个孩子节点为空显然出错 二元操作符少了右边操作数 相当于出现了 xx + -  ?? */
				node->error = 1;
				memset(line , 0 , STR_LINE_LEN);
				memset(node->attr.content , 0 , SYN_CONTENT_LEN);
				strcpy(node->attr.content , "line ");
				sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
				strcat(node->attr.content , line);
				strcat(node->attr.content , ": syntax error: more exp is needed after ' ");
				strcat(node->attr.content , opt_name);
				strcat(node->attr.content , " '" );
				return node;
			}

			first_child = node;	/*当前节点作为新的孩子节点*/
			continue;
		}

		/*没有新的句号标识*/
			break;
	}

	if(flag == 1){	/*如果是该类型表达式 返回当前节点*/
		return node;
	}
	/*不是该类型节点返回第一个孩子节点*/
	return first_child;
}

/*乘除取摸表达式*/
static syntax_node *multi_exp(void){
	unsigned char flag;	/*标记是否是乘除表达式*/
	syntax_node *node;	/*包含句号操作符的节点*/
	syntax_node *first_child;
	char line[STR_LINE_LEN];	/*行号字符串*/
	char opt_name[OPT_NAME_LEN];	/*操作符名字*/

	flag = 0;
	first_child = cast_exp();

	if(first_child){
		if(first_child->error){	/*如果子节点出错。不产生当前节点，直接返回错误节点*/
			return first_child;
		}
	}else{	/*如果第一个孩子为空那么该节点显然为空*/
		return NULL;
	}

	/*检查是否乘除表达式*/
	while(1){
		memset(opt_name , 0 , OPT_NAME_LEN);	/*在匹配之前先保存操作符*/
		strcpy(opt_name , src_buff.token_buff);

		if(match(MULTIPLE) == 0){
			flag = 1;	/*标明是乘除表达式节点*/

			node = (syntax_node *)malloc(sizeof(syntax_node));
			memset(node , 0 , sizeof(syntax_node));
			node->kind = EXP_KIND;
			node->type = MULTI_EXP;
			strcpy(node->attr.opt_declare.opt_name , opt_name);
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcpy(node->line , line);

			node->first_child = first_child;

			node->second_child = cast_exp();
			if(node->second_child){
				if(node->second_child->error){	/*子节点出错，返回当前节点*/
					node->error = 1;
					return node;
				}
			}else{	/*如果下一个孩子节点为空显然出错 二元操作符少了右边操作数 相当于出现了 xx * % / ?? */
				node->error = 1;
				memset(line , 0 , STR_LINE_LEN);
				memset(node->attr.content , 0 , SYN_CONTENT_LEN);
				strcpy(node->attr.content , "line ");
				sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
				strcat(node->attr.content , line);
				strcat(node->attr.content , ": syntax error: more exp is needed after ' ");
				strcat(node->attr.content , opt_name);
				strcat(node->attr.content , " '" );
				return node;
			}

			first_child = node;	/*当前节点作为新的孩子节点*/
			continue;
		}

		/*没有新的句号标识*/
			break;
	}

	if(flag == 1){	/*如果是该类型表达式 返回当前节点*/
		return node;
	}
	/*不是该类型节点返回第一个孩子节点*/
	return first_child;
}

/*类型转换表达式*/
static syntax_node *cast_exp(void){
	syntax_node *node;	/*类型操作符表达式的节点*/
	syntax_node *first_child;
	char line[STR_LINE_LEN];	/*行号字符串*/

	if(match(CAST) == 0){	/*是类型转换表达式*/
		memset(line , 0 , STR_LINE_LEN);

		node = (syntax_node *)malloc(sizeof(syntax_node));
		memset(node , 0 , sizeof(syntax_node));
		node->kind = EXP_KIND;
		node->type = CAST_EXP;
		strcpy(node->attr.opt_declare.opt_name , "cast");
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcpy(node->line , line);

		if(match(LPARENT) == -1){	/*缺少 ( */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' (");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}

		strcpy(node->attr.opt_declare.opt_content , src_buff.token_buff);
		if(match(TYPE) == -1){	/*返回值类型必须是基本类型或者结构体类型*/

			if(match(STRUCT_UNION) == -1){
				node->error = 1;	/*错误*/
				memset(node->attr.content , 0 , SYN_CONTENT_LEN);
				strcpy(node->attr.content , "line ");
				memset(line , 0 , STR_LINE_LEN);
				sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
				strcat(node->attr.content , line);
				strcat(node->attr.content , ": syntax error: unknown type  ' ");
				strcat(node->attr.content , src_buff.token_buff);
				strcat(node->attr.content , " '");
				return node;
			}else{	/*形如struct struct_identifier 结构体类型名*/
				strcat(node->attr.opt_declare.opt_content , ".");
				strcat(node->attr.opt_declare.opt_content , src_buff.token_buff);
				if(match(IDENTIFIER) == -1){	/*不是结构体类型名*/
					node->error = 1;	/*错误*/
					memset(node->attr.content , 0 , SYN_CONTENT_LEN);
					strcpy(node->attr.content , "line ");
					memset(line , 0 , STR_LINE_LEN);
					sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
					strcat(node->attr.content , line);
					strcat(node->attr.content , ": syntax error: illegal struct type identifier ' ");
					strcat(node->attr.content , src_buff.token_buff);
					strcat(node->attr.content , " '");
					return node;
				}
			}
		}


		while(1){	/*检查是否指针类型*/
			if(match(PREFIX) == 0){	/*这里假定是$ 指针符号*/
				strcat(node->attr.opt_declare.opt_content , "$"); /*将其添加到类型之后*/
				continue;
			}else{
				break;
			}
		}

		if(match(RPARENT) == -1){	/*缺少 ) */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' )");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}

		node->first_child = unary_exp();	/*一元操作符作为其孩子*/
		if(node->first_child){
			if(node->first_child->error){
				node->error = 1;
				return node;
			}
		}else{	/*没有转换对象是错误的*/
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: cast nothing");
			return node;
		}

		return node;
	}

	/*不是类型转换表达式*/
	return unary_exp();
}
/*一元表达式*/
static syntax_node *unary_exp(void){
	unsigned char flag;	/*标记是否是一元表达式*/
	syntax_node *node;	/*包含一元操作符的节点*/
	syntax_node *first_child;
	char line[STR_LINE_LEN];	/*行号字符串*/
	char opt_name[OPT_NAME_LEN];	/*操作符名字*/
	char buff[TOKEN_BUFF_LEN];	/*备份*/

	memset(opt_name , 0 , OPT_NAME_LEN);
	memset(buff , 0 , TOKEN_BUFF_LEN);

	/*首先假设是sizeof操作符*/
	if(match(SIZEOF) == 0){

		node = (syntax_node *)malloc(sizeof(syntax_node));
		memset(node , 0 , sizeof(syntax_node));
		node->kind = EXP_KIND;
		node->type = UNARY_EXP;
		strcpy(node->attr.opt_declare.opt_name , "sizeof");
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcpy(node->line , line);

		if(match(LPARENT) == -1){	/*缺少 ( */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' (");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}

		strcpy(node->attr.opt_declare.opt_content , src_buff.token_buff);
		if(match(TYPE) == -1){	/*不是类型名或者变量名或者结构体类型名将出错*/

			if(match(IDENTIFIER) == -1){

				if(match(STRUCT_UNION) == -1){
					node->error = 1;	/*错误*/
					memset(node->attr.content , 0 , SYN_CONTENT_LEN);
					strcpy(node->attr.content , "line ");
					memset(line , 0 , STR_LINE_LEN);
					sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
					strcat(node->attr.content , line);
					strcat(node->attr.content , ": syntax error: unknown type  ' ");
					strcat(node->attr.content , src_buff.token_buff);
					strcat(node->attr.content , " '");
					return node;
				}else{	/*形如struct struct_identifier 结构体类型名*/
					strcat(node->attr.opt_declare.opt_content , ".");
					strcat(node->attr.opt_declare.opt_content , src_buff.token_buff);
					if(match(IDENTIFIER) == -1){	/*不是结构体类型名*/
						node->error = 1;	/*错误*/
						memset(node->attr.content , 0 , SYN_CONTENT_LEN);
						strcpy(node->attr.content , "line ");
						memset(line , 0 , STR_LINE_LEN);
						sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
						strcat(node->attr.content , line);
						strcat(node->attr.content , ": syntax error: illegal sizeof ' ");
						strcat(node->attr.content , src_buff.token_buff);
						strcat(node->attr.content , " '");
						return node;
					}
				}

			}
		}

		if(match(RPARENT) == -1){	/*缺少 ) */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' )");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}

		return node;
	}

	/*假设是一元操作符*/
	strcpy(opt_name , src_buff.token_buff);
	if(match(PREFIX) == 0){

		node = (syntax_node *)malloc(sizeof(syntax_node));
		memset(node , 0 , sizeof(syntax_node));
		node->kind = EXP_KIND;
		node->type = UNARY_EXP;
		strcpy(node->attr.opt_declare.opt_name , opt_name);
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcpy(node->line , line);

		strcpy(node->attr.opt_declare.opt_content , src_buff.token_buff);
		if(match(IDENTIFIER) == -1){/*不是变量名将出错*/
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: illegal identifier  ' ");
			strcat(node->attr.content , src_buff.token_buff);
			strcat(node->attr.content , " '");
			return node;
		}

		node->first_child = postfix_phrase();	/*后缀短语作为当前节点的孩子*/
		if(node->first_child){
			if(node->first_child->error){
				node->error = 1;
				return node;
			}
		}

		return node;
	}


	/*假设是带小括弧的表达式*/
	if(match(LPARENT) == 0){

		node = (syntax_node *)malloc(sizeof(syntax_node));
		memset(node , 0 , sizeof(syntax_node));
		node->kind = EXP_KIND;
		node->type = UNARY_EXP;
		strcpy(node->attr.opt_declare.opt_name , "()");
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcpy(node->line , line);

		node->first_child = expression();
		if(node->first_child){
			if(node->first_child->error){
				node->error = 1;
				return node;
			}
		}else{	/*在小括弧中必须有表达式*/
			node->error = 1;
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: exprssion is needed in ' ()");
			strcat(node->attr.content , " ' ");
			return node;
		}


		if(match(RPARENT) == -1){	/*缺少 ) */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' )");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}
		return node;
	}

	/*假设是单纯的常量*/
	memset(buff , 0 , TOKEN_BUFF_LEN);
	strcpy(buff , src_buff.token_buff);
	if(match(INT_VAL) == 0){	/*整型常量*/

		node = (syntax_node *)malloc(sizeof(syntax_node));
		memset(node , 0 , sizeof(syntax_node));
		node->kind = EXP_KIND;
		node->type = UNARY_EXP;
		strcpy(node->attr.opt_declare.opt_content , "#");	/*整型常量以#开始用作标识*/
		strcat(node->attr.opt_declare.opt_content , buff);
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcpy(node->line , line);
		return node;

	}else if(match(REAL_VAL) == 0){	/*实型常量*/

		node = (syntax_node *)malloc(sizeof(syntax_node));
		memset(node , 0 , sizeof(syntax_node));
		node->kind = EXP_KIND;
		node->type = UNARY_EXP;
		strcpy(node->attr.opt_declare.opt_content , ".");	/*实型常量以.开始用作标识*/
		strcat(node->attr.opt_declare.opt_content , buff);
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcpy(node->line , line);
		return node;
	}else if(match(STR_VAL) == 0){	/*字符串常量*/

		node = (syntax_node *)malloc(sizeof(syntax_node));
		memset(node , 0 , sizeof(syntax_node));
		node->kind = EXP_KIND;
		node->type = UNARY_EXP;
		strcpy(node->attr.opt_declare.opt_content , "\"");	/*字符串常量必须以双引号开始*/
		strcat(node->attr.opt_declare.opt_content , buff);
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcpy(node->line , line);
		return node;
	}else if(match(CHAR_VAL) == 0){	/*字符型常量*/

		node = (syntax_node *)malloc(sizeof(syntax_node));
		memset(node , 0 , sizeof(syntax_node));
		node->kind = EXP_KIND;
		node->type = UNARY_EXP;
		strcpy(node->attr.opt_declare.opt_content , "'");	/*字符常量必须以单引号开始*/
		strcat(node->attr.opt_declare.opt_content , buff);
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcpy(node->line , line);
		return node;
	}else{
		;
	}

	/*假设是变量名和后缀(如果有)*/
	memset(buff , 0 , TOKEN_BUFF_LEN);
	strcpy(buff , src_buff.token_buff);
	if(match(IDENTIFIER) == 0){

		node = (syntax_node *)malloc(sizeof(syntax_node));
		memset(node , 0 , sizeof(syntax_node));
		node->kind = EXP_KIND;
		node->type = UNARY_EXP;
		strcpy(node->attr.opt_declare.opt_content , buff);
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcpy(node->line , line);

		node->first_child = postfix_phrase();	/*后缀短语作为当前节点的孩子*/
		if(node->first_child){
			if(node->first_child->error){
				node->error = 1;
				return node;
			}
		}
		return node;
	}

	return NULL;	/*以上什么都不是，返回空*/
}

/*后缀短语*/
static syntax_node *postfix_phrase(void){
	syntax_node *node;

	node = element_select();
	if(node){
		return node;
	}

	node = member_select();
	if(node){
		return node;
	}

	node = argument_spec();
	if(node){
		return node;
	}

	return NULL;
}

/*元素选择*/
static syntax_node *element_select(void){
	syntax_node *node;	/*操作符的节点*/
	syntax_node *first_child; /*子节点*/
	char line[STR_LINE_LEN];	/*行号字符串*/

	if(match(SUBSCRIPT) == 0){	/*是元素选择*/
		memset(line , 0 , STR_LINE_LEN);

		node = (syntax_node *)malloc(sizeof(syntax_node));
		memset(node , 0 , sizeof(syntax_node));
		node->kind = EXP_KIND;
		node->type = ELEMENT_SELECT;
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcpy(node->line , line);

		node->first_child = assign_exp();	/*其内部为扩展节点*/
		if(node->first_child){
			if(node->first_child->error){
				node->error = 1;
				return node;
			}
		}else{
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: selector in '[ ]' is needed");
			return node;
		}

		if(match(END_SUBSCRIPT) == -1){	/*缺少 ] */
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: token ' ]");
			strcat(node->attr.content , " ' is nedded");
			return node;
		}
		return node;
	}
	/*不是元素选择符*/
	return NULL;
}

/*成员选择*/
static syntax_node *member_select(void){
	syntax_node *node;	/*包含操作符的节点*/
	syntax_node *first_child;
	char line[STR_LINE_LEN];	/*行号字符串*/
	char opt_name[OPT_NAME_LEN];	/*操作符名字*/

	memset(line , 0 , STR_LINE_LEN);
	memset(opt_name , 0 , OPT_NAME_LEN);

	strcpy(opt_name , src_buff.token_buff);
	if(match(SELECT) == 0){	/*是成员选择*/
		node = (syntax_node *)malloc(sizeof(syntax_node));
		memset(node , 0 , sizeof(syntax_node));
		node->kind = EXP_KIND;
		node->type = MEMBER_SELECT;
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcpy(node->line , line);

		strcpy(node->attr.opt_declare.opt_name , opt_name);
		strcpy(node->attr.opt_declare.opt_content , src_buff.token_buff);
		if(match(IDENTIFIER) == -1){/*不是变量名将出错*/
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: illegal identifier  ' ");
			strcat(node->attr.content , src_buff.token_buff);
			strcat(node->attr.content , " '");
			return node;
		}
		return node;
	}
	/*不是成员选择*/
	return NULL;
}

/*参数描述*/
static syntax_node *argument_spec(void){
	syntax_node *node;	/*包含操作符的节点*/
	syntax_node *prev_child;
	syntax_node *sibling;
	char line[STR_LINE_LEN];	/*行号字符串*/

	if(match(LPARENT) == -1){	/*不是参数描述*/
		return NULL;
	}

	node = (syntax_node *)malloc(sizeof(syntax_node));
	memset(node , 0 , sizeof(syntax_node));
	node->kind = EXP_KIND;
	node->type = ARGUMENT_SPEC;
	memset(line , 0 , STR_LINE_LEN);
	sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
	strcpy(node->line , line);

	prev_child = assign_exp();	/*第一个参数*/
	node->first_child = prev_child;

	if(!prev_child){
		node->error = 1;	/*错误*/
		memset(node->attr.content , 0 , SYN_CONTENT_LEN);
		strcpy(node->attr.content , "line ");
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcat(node->attr.content , line);
		strcat(node->attr.content , ": syntax error: empty argument ");
		return node;
	}

	if(prev_child->error){	/*出错*/
		node->error = 1;
		return node;
	}

	while(1){
		if(match(SEQUEN) == -1){	/*如果不是参数分隔符 表示参数输入结束*/
			break;
		}

		sibling = assign_exp();
		if(!sibling){
			node->error = 1;	/*错误*/
			memset(node->attr.content , 0 , SYN_CONTENT_LEN);
			strcpy(node->attr.content , "line ");
			memset(line , 0 , STR_LINE_LEN);
			sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
			strcat(node->attr.content , line);
			strcat(node->attr.content , ": syntax error: empty argument ");
			return node;
		}

		if(sibling->error){
			node->error = 1;
			return node;
		}

		prev_child->sibling = sibling;
		prev_child = sibling;
	}

	if(match(RPARENT) == -1){	/*缺少 ) */
		node->error = 1;	/*错误*/
		memset(node->attr.content , 0 , SYN_CONTENT_LEN);
		strcpy(node->attr.content , "line ");
		memset(line , 0 , STR_LINE_LEN);
		sprintf(line , "%d" , src_buff.line);	/*转换行号为字符串*/
		strcat(node->attr.content , line);
		strcat(node->attr.content , ": syntax error: token ' )");
		strcat(node->attr.content , " ' is nedded");
		return node;
	}

	return node;
}
