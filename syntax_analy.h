/*
 * syntax_analy.h
 *
 *
 *进行语法分析。构成一棵语法树
 *
 *  Created on: 2011-5-18
 *      Author: leiming
 */

#ifndef SYNTAX_ANALY_H_
#define SYNTAX_ANALY_H_

#include "global.h"
#include "lexical_analy.h"
/*
 * tinyC的EBNF文法。大部分借鉴标准C的文法，有些地方有所改变。
 * 语句statement是构成源文件的基本结构。
 *
 * 开始<start>	::= <variable-statement> 变量声明/定义语句
 * 								| <struct-statement> 结构体类型定义语句
 * 								|<function-statement> 	函数声明/定义语句
 * 								*
 * 语句<statement>	::=  <labeled-statement>标记语句
									| <expression-statement>表达式语句
									| <selection-statement> 选择语句
									| <iteration-statement> 迭代语句
									| <jump-statement> 跳转语句
									| <null-statement > 空语句
									| <variable-statement> 变量声明/定义语句
									*


 * 标记语句<labeled-statement>	::=  	label identifier : {<statement>}
																| case <constant-expression> : {<statement>}
																| default : {<statement>}

 * 表达式语句<expression-statement>	::=  <expression>;
 * 选择语句<selection-statement>	::=  	<if-statement> | <switch-statement>
 * if语句<if-statement>	::=  	if ( <expression> ) {<statement>}
												| if ( <expression> ) {<statement>} else {<statement>}
 * switch语句<switch-statement>	::=  	switch ( <expression> ) {<statement>}
 * 迭代语句<iteration-statement>	::=  	<while-statement> | <do-statement> | <for-statement>
 * while语句<while-statement>	::=  	while ( <expression> ) {<statement>}
 * do语句<do-statement>	::=  	do {<statement>} while (<expression>) ;
 * for语句<for-statement>	::=  	for ( <expression>|ε ; <expression>|ε ; <expression>| ε ) {<statement>}
 * 跳转语句<jump-statement>	::= <goto-statment> | <continue-statement> | <break-statement> | <return-statement>
 *  goto语句<goto-statement>	::=  goto identifier ;
 * continue语句<continue-statement>	::= continue ;
 * break语句<break-statement>	::=  break ;
 * return语句<return-statement> ::=  return (<expression>|ε ) ;
 * 结构体类型定义<struct-statement> ::= structdef struct_identifier{ type|struct struct_identifier $* identifier (; type|struct struct_identifier $* identifier) };
 * 变量声明/定义语句<variable-statement> ::= <type-specification> <variable-declarators>
 * 类型说明<type-specification>	::= storage_class|ε type_qualifier|ε  type|struct struct_identifer
 * 变量群描述<variable-declarators>	::=<variable-declarator>;
 * 																|<variable-declarator> , <variable-declarators>
 * 变量描述<variable-declarator>	::= identifier
 * 															 | identifier[<const-expression>]
    														 | $* type_qualifier|ε <variable-declarator>
 *函数声明/定义语句<function-statement>	::=  function <type-secification> $*|ε  identifier(<formal-arguments>);
 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 |function <type-specification> $*|ε identifier(<formal-arguments>){<staement>}

 * 参数列表<formal-arguments>	::= <formal-argument>
 * 														| <formal-argument> , <formal-arguments>
 *
 * 参数<formal-argument>	::= type_qualifier|ε type|struct struct_identifier $*|ε identifier
 *
 *
 *
 *表达式是由操作符连接起来的有意义的元结构
 *以下排列亦按照优先级从低到高
 * 表达式<expression>		::= <sequence-expression>
 *
 * 二元
 * 句号<sequence-expression>					::= <assignment-expression> ( , <assignment-expression> ) *
 * 赋值<assignment-expression>				::=  <unary-expression>  assignment-operator   <logical-or-expression> 不允许连接赋值
 * 逻辑或<logical-or-expression>				::= <logical-and-expression> ( || <logical-and-expression> ) *
 * 逻辑与<logical-and-expression>				::= <bitwise-or-expression> ( && <bitwise-or-expression> ) *
 * 按位或<bitwise-or-expression>				::= <bitwise-xor-expression> ( | <bitwise-xor-expression> ) *
 * 按位异或<bitwise-xor-expression>			::= <bitwise-and-expression> ( ^ <bitwise-and-expression> ) *
 * 按位与<bitwise-and-expression>			::= <equality-expression> ( & <equality-expression> ) *
 * 等价<equality-expression>						::= <relational-expression> ( equality-operator <relational-expression> ) *
 * 关系<relational-expression>					::= <shift-expression> ( relational-operator <shift-expression> ) *
 * 移位<shift-expression>							::= <additive-expression> ( shift-operator <additive-expression> ) *
 * 加减<additive-expression>						::= <multiplicative-expression> ( additive-operator <multiplicative-expression> ) *
 * 乘除取模<multiplicative-expression>		::=<cast-expression>( multiplicative-operator <cast-expression> ) *
 *类型转换<cast-expression>						::= cast(type|struct struct_identifier $*) <unary-expression>
 *
 *注意以上运算符都是是左结合(即左子更深)
 *
 * 一元表达式<unary-expression>		::= 	 prefix-operator  identifier <postfix-phrase>|ε	有操作符一定后面跟标识符
 * 																	|sizeof (type|identifier|struct struct_identifier)	sizeof后面一定跟着类型或者变量名
 * 																	|literal-constant													没有操作符那么可能是常量
 * 																	|identifier <postfix-phrase>|ε								没有操作符可能是标识符+后缀
 * 																	|(<expression>)													以上都没有则是括弧短语

 * 后缀短语<postfix-phrase>				::= 	<element-selector>
																	|  <member-selector>
																	|  <argument-specifier>
 *元素选择<element-selector>		::= [ <assignment-expression> ]
 *成员选择<member-selector>		::= 	. identifier
																|  -> identifier
 *参数描述<argument-specifier>		::=  (<assignment-expression> (, <assignment-expression>)*)
 *
 *二元操作符节点的结构比较简单，节点opt_name标明操作符文字，其first_child为其第一个操作数second_child为其第二个操作数
 *
 *对于一元操作符节点prefix-operator或者sizeof则保存在节点的opt_name中，其修饰的内容放入opt_content中
 *如果只是单纯的标识符或者常量那么保存在节点的opt_content中且设置opt_name为空
 *因为(<expression>)与<postfix-phrase>不同时存在因此，只要有其一作为当前节点的first_child放入
 *注意参数描述节点的构造为第一个expression作为其first_child其他作为first_child的兄弟节点连入
 *
 *
 *说明一元表达式的节点存储内容：
 *如果是sizeof 则在 node->attr.opt_declare.opt_name放入sizeof 在node->attr.opt_declare.opt_content放入type|identifer|struct.type_identifier
 *如果是一元操作符 则将type放入opt_name中 identifier放入opt_content中 如果有后缀放入节点node->first_child中
 *如果是()操作符 则将()放入opt_name中 exp放入first_child中
 *如果是无操作符则有两种可能：常量:放入opt_content中 其中字符与字符串型前面有" 用来和标识符坐区别
 *													标识符加后缀:标识符放入opt_content后缀放入first_child(如果有)
 */

/*
 * 语句节点与表达式节点产生机制有一定区别：
 *
 *对于语句节点来说，首先判断是否该类型语句，如果不是，返回NULL表示不是该类型节点
 *如果是则产生父节点再产生相应的子节点。
 *因此语句错误传递机制是如果任意父节点的子节点出现错误，那么该父节点标记为错误。
 *向上传递
 *
 * 对于表达式节点来说在判定表达式类型之前已经会生成一个子节点，而当前类型表达式节点会之后判定表达式类型之后才生成，
 * 如果不是该类型表达式，就将之前生成的子节点向上传递，不生成父节点也不返回NULL
 * 否则如果是该类型表达式，那么生成该类型表达式节点并建立之前子节点与当前节点的相互关系，然后迭代生成下一个孩子。
 * 返回当前节点。
 * 因此，表达式错误传递机制是，如果发现子节点出错，且未生成父节点时，而是直接将子节点向上传递。而不再生成当前节点
 * 如果在生成当前节点之后发现错误，那么机制与语句节点传递一致了。
 */

/*
 * 语法树的结构:
 * 采用三孩子一兄弟的指针域
 *
 *  函数节点：其first_child为参数，second_child为函数体(如果有)
 *
 * 语句中只有<start>节点、<statement>节点的所有子节点之间互相使用兄弟指针链接因为可能会有多个孩子。其first-child指向第一个孩子.
 * 其余类型节点都将其孩子节点放入孩子指针,其孩子节点之间不使用兄弟指针。
 * 所有节点本身的兄弟指针不由自己设置而是由父<start>,<statemen>节点设置
 *
 *表达式节点与以上性质相同的是一元后缀的函数参数节点。第一个参数是ARGUMENT_SPEC的first_child，其余参数是第一个参数的兄弟节点
 *
 *另外<struct-statement>所有的成员节点依次链接到节点的first_child中, <var-statment>使用一句声明多个变量也会依次形成first_child链
 *而在表达式中具有以上性质的是<argument-specific>节点
 *
 *
 *
 */

#define STR_LINE_LEN 8	/*行号转换成字符串缓冲最大长度*/

#define SYN_CONTENT_LEN 		80	/*属性内容长度*/

#define STORAGE_CLASS_LEN 	8		/*存储类型文字长度*/
#define TYPE_QUALIFIER_LEN 	8		/*限定符文字长度*/
#define TYPE_SPECIFIC_LEN		24	/*具体类型名称文字长度包括结构体类型名*/
#define POINTER_LEN				8		/*指针文字长度*/
#define ARRAY_LEN					8		/*可能的数组长度*/
#define 	VAR_NAME_LEN 			24	/*变量名长度*/

#define OPT_NAME_LEN				16	/*操作符名字长度*/
#define OPT_CONTENT_LEN		64 /*操作符内容长度*/


typedef enum _node_kind{	/*结点大类别*/
 START_KIND/*文法分析开始*/ , STMT_KIND /*语句类型*/ , FUNC_KIND /*函数类型*/ , EXP_KIND /*表达式类型*/
}node_kind;

typedef enum _node_type{	/*结点在大类别下具体的类型*/
 //属于START_KIND的类型
 START /*文法开始*/ ,

 //属于STMT_KIND的类型
 STMT /*语句序列*/, LABEL_STMT /*标记语句*/, EXP_STMT /*表达式语句*/, SELECT_STMT /*选择语句*/, ITER_STMT /*迭代语句*/ ,
 JUMP_STMT /*跳转语句*/ , VAR_STMT /*变量声明/定义语句*/ , NULL_STMT /*空语句*/ , STRUCT_STMT /*结构体类型定义语句*/ ,
 STRUCT_MEMBER/*结构体成员*/ ,

 //属于FUNC_KIND的类型
 FUNC_DECL /*函数声明*/ , FUNC_DEFINE /*函数定义*/ ,

 //属于EXP_KIND的类型
 SEQ_EXP /*句号表达式*/ , ASSIGN_EXP /*赋值表达式*/ , LOGIC_OR_EXP /*逻辑或表达式*/ , LOGIC_AND_EXP /*逻辑与表达式*/ ,
 BIT_OR_EXP /*按位或表达式*/ , BIT_XOR_EXP /*按位异或表达式*/ , BIT_AND_EXP /*按位与表达式*/ , EQUAL_EXP /*等价表达式*/ ,
 RELATION_EXP /*关系表达式*/ , SHIFT_EXP /*移位表达式*/ , ADDITIVE_EXP /*加减表达式*/ , MULTI_EXP /*乘除取摸表达式*/ ,
 CAST_EXP /*类型转换表达式*/ , UNARY_EXP /*一元表达式*/ ,
 ELEMENT_SELECT /*元素选择*/ , MEMBER_SELECT /*成员选择*/ , ARGUMENT_SPEC /*函数参数*/


}node_type;


typedef struct _syntax_node{
	unsigned char error;					/*是否是错误节点*/
	node_kind kind;							/*结点类别*/
	node_type type;							/*结点类型*/
	char line[STR_LINE_LEN];		/*行号字符串*/

	struct _syntax_node *first_child;
	struct _syntax_node *second_child;
	struct _syntax_node *third_child;
	struct _syntax_node *last_child;	/*最多四个孩子*/
	struct _syntax_node *sibling;	/*兄弟*/

	union {
		char content[SYN_CONTENT_LEN];	/*结点的文字内容*/
		struct{
			char storage_class[STORAGE_CLASS_LEN];	/*存储类型*/
			char type_qualifier[TYPE_QUALIFIER_LEN];		/*类型限定*/
			char type_specific[TYPE_SPECIFIC_LEN];		/*具体类型*/
			char nr_pointer[POINTER_LEN];						/*记录有多少层指针*/
			char array_len[ARRAY_LEN];							/*数组长度*/
			char var_name[VAR_NAME_LEN];					/*变量名字*/
		}type_declare;	/*声明的类型*/
		struct{
			char opt_name[OPT_NAME_LEN];					/*操作符名字*/
			char opt_content[OPT_CONTENT_LEN];			/*操作内容*/
		}opt_declare;	/*操作符描述*/


	}attr;

}syntax_node;

extern unsigned char syntax_error;	/*标志是否出现语法错误*/

extern syntax_node *syntax_tree;	/*一棵语法解析树*/


extern int delete_syntax_tree(syntax_node *node);	/*删除所构建的语法分析树*/
extern int delete_syntax_tree(syntax_node *node);	/*删除语法树*/
extern int print_syntax_tree(syntax_node *node);/*打印构成的语法树信息*/
extern int syntax_analy(void); /*语法分析 建立语法树*/

#endif /* SYNTAX_ANALY_H_ */
