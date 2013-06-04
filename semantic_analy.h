/*
 * semantic_analy.h
 *
 *	主要对生成的语法树进行语义分析。
 * 包括建立符号表、类型检查等
 *
 *  Created on: 2011-6-1
 *      Author: leiming
 */

#ifndef SEMANTIC_ANALY_H_
#define SEMANTIC_ANALY_H_

#include "syntax_analy.h"

/*类型宏定义*/
#define TYPE_MASK		 	0x00FF		//类型掩码
#define NO_TYPE				0x0000		//无类型		0000 0000 0000 0000
#define VOID_TYPE		 	0x0001		//空类型		0000 0000 0000 0001
#define CHAR_TYPE		 	0x0002		//字符型		0000 0000 0000 0010
#define SHORT_TYPE		 	0x0004		//短整型		0000 0000 0000 0100
#define INT_TYPE			 	0x0008		//整型			0000 0000 0000 1000
#define LONG_TYPE			0x0010		//长整型		0000 0000 0001 0000
#define FLOAT_TYPE	 	 	0x0020		//浮点类型	0000 0000 0010 0000
#define DOUBLE_TYPE	 	0x0040		//双精度		0000 0000 0100 0000
#define STRUCT_TYPE		0x0080		//结构体类型0000 0000 1000 0000

#define QUALIFIER_MASK	0x0300		//类型限定掩码
#define STATIC_QF			0x0100		//静态限定	0000 0001 0000 0000
#define CONST_QF			0x0200		//常量限定	0000 0010	0000 0000

#define ERROR_MASK			0x0800		//错误掩码
#define TYPE_ERROR			0x0800			//类型错误	0000 1000 0000 0000

#define POINTER_MASK		0xF000		//指针层次掩码
#define POINTER_1L			0x1000		//一级指针	0001 0000 0000 0000
#define POINTER_2L			0x2000		//二级指针	0010 0000 0000 0000
#define POINTER_3L			0x4000		//三级指针	0100 0000 0000 0000
#define POINTER_4L			0x8000		//四级指针	1000 0000 0000 0000

/*计算符号表杂凑函数公式
 * 对于任意字符串，设任一字符值为c(i)
 * 有h(i) = [coeff * h(i - 1) + c(i)] % SIZE 设h(0) = 0; 1<= i <= n;
 * 最终得到h(n) % SIZE
 */
#define SYM_TB_COEFF 16	/*乘积因子*/
#define SYM_TB_SIZE	41	/*哈希表大小。素数*/

/*分别对应符号表的表项类型*/
#define VAR_ENTRY 		0
#define FUNC_ENTRY		1
#define STRUCT_ENTRY	2
#define ARRAY_ENTRY	3
#define CONST_ENTRY 	4

/*在生成的汇编文件对应的标记名长度*/
#define ASM_LABEL_LEN VAR_NAME_LEN + COMPILE_FILE_NAME_LEN



/*每个杂凑符号表的表项。用于存储每个标识符*/
typedef struct _sym_tb_entry{
	unsigned char entry_type;	/*该项目储存的符号类型 VARIABLE、FUNCTION 、STRUCT、ARRAY*/
	unsigned long src_addr; /*在源文件中的相对地址*/
	unsigned int size;	/*该项目变量所占字节 对于变量/结构体是该变量或结构体的长度，若是数组则是单个成员长度*/
	unsigned int align_size;	/*在4字节对齐之后大小*/
	char name[VAR_NAME_LEN];	/*变量名*/
	char storage_class[STORAGE_CLASS_LEN];	/*存储类型*/
	char type_qualifier[TYPE_QUALIFIER_LEN];		/*类型限定*/
	char type_specific[TYPE_SPECIFIC_LEN];		/*具体类型*/
	char nr_pointer[POINTER_LEN];						/*记录有多少层指针*/

	char asm_label[ASM_LABEL_LEN];	/*在汇编文件中相应的名字*/

	union {
		char array_len[ARRAY_LEN];						/*如果是数组记录数组长度*/
		void *struct_table;											/*如果是结构体指向结构体定义符号表 类型为 sym_table *  */
		struct {
			void *param_table;								/*参数表   类型为 sym_table *   */
			void *block_table;								/*实现表   类型为 sym_table *   */
		}function_table;										/*如果是函数指向函数体定义表 */
	}attr;

	struct _sym_tb_entry *next;
}sym_tb_entry;

/*每个杂凑符号表的结构
 * 注意符号表的嵌套深度最多两层，最外层为根符号表内层为函数符号表或者结构体定义表。因为函数不允许嵌套定义
 */
typedef struct _sym_table{
	sym_tb_entry *parent_entry;	/*该表所在的父entry项；root表该值为空*/
	sym_tb_entry *entry_heads[SYM_TB_SIZE];
	unsigned int total_size;	/*该表所包含的总大小*/
	unsigned int total_align_size;	/*对齐之后的总大小*/
	unsigned short src_addr; /*名字在源文件的相对地址 同时可以由此获得该表中表项数目*/
}sym_table;

extern sym_table *root_table;	/*根符号表。记录公共变量、结构体与函数. */
extern sym_table *const_table;	/*记录常量的符号表*/

////////////////////////////////FUNCTIONS////////////////////////////////////
extern int semantic_analy(void);
extern int delete_sym_table(sym_table *table);	/*删除符号表及其表项*/
extern sym_tb_entry *get_entry(sym_table *table , char *key);	/*根据名字获取其在符号表里的表项指针。如果失败返回空*/









#endif /* SEMANTIC_ANALY_H_ */
