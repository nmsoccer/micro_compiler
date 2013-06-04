/*
 * lexical_analy.h
 *
 *  Created on: 2011-5-15
 *      Author: leiming
 */

#ifndef LEXICAL_ANALY_H_
#define LEXICAL_ANALY_H_


/*TINYC中主要标记类型为标识符(字符)、关键字、常量(数字)、操作符、标点
 * 这些只是用于说明标记的逻辑划分，不因此判别类型。具体类型的划分要待更上一层次说明
 * token 标记
    → identifier | keyword | literal-constant | operator | punctuation
 *
 * * *identifier 标识符
    → ( letter | _ ) ( letter | digit | _ ) *
 *
 *keyword 关键字
    → auto | else | register | union | break | enum | return | unsigned | case | extern | short | void | char | float | signed | volatile
    | const | for | sizeof | while | continue | goto | static | default | label | if | struct | do | int | switch | double | long | label | function
 *
 *
 *literal-constant 常量
    → integer-literal 整型常量    → nonzero-digit digit *
    | real-literal	实型常量 → digits . | . digits | digits . digits
    | character-literal	字符型常量  → ' letter | digit | c-symbol | escape-sequence '
    | string-literal	字符串常量 →" s-character * "
 *
 *
 *operator 操作符
 * Subscript 		[ ]
 * Select			. ->
 *Unary			Postfix 	++ --
 *						Prefix 	++ -- + - ! ~ * & () sizeof
 *Arithmetic 	Multiplicative 	* / %
 *						Additive 	+ -
 *Shift 				>> <<
 *Comparison	Relational 	< <= > >=
 *Equality 		== !=
 *Bitwise 			Bitwise AND 	&
 *						Bitwise XOR 	^
 *						Bitwise OR 	|
 *Boolean 		Boolean AND 	&&
 *						Boolean OR 	||
 *Conditional 	? :
 *Assignment 	= += -= *= /= %= &= |= ^= <<= >>=
 *Sequence 		,

 *punctuation
 *→ ; { } ,  :
 *************************************************************
 *digits
    → digit digit *
 *
 *
 *
 *c-symbol
    → " | q-symbol
 *
 *s-character
    → letter | digit | s-symbol | escape-sequence
   s-symbol
    → ' | q-symbol

 *
 *
 *letter 字母
    → a | b | c | d | e | f | g | h | i | j | k | l | m
    | n | o | p | q | r | s | t | u | v | w | x | y | z
    | A | B | C | D | E | F | G | H | I | J | K | L | M
    | N | O | P | Q | R | S | T | U | V | W | X | Y | Z
 *
 *digit	数字
    → 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9
 *

 *q-symbol
    → space | ! | # | $ | % | & | ( | ) | * | + | , | - | . | / | : | : | < | = | > | @ | [ | ] | ^ | _ | ` | { | '|' | } | ~
 *escape-sequence
    → \a | \b | \f | \n | \r | \t | \v | \" | \' | \\ | \?
 *
 */


/*
 * 根据每个token的作用来进行分类。
 * 有些类别只有一个值，比如IF："if";
 * 大多数类别有多个值，比如TYPE：void char short int long float double等。
 *
 */

typedef enum _token_type{
	//空记号
	NULL_TOKEN ,

	//标识符 	主要用于标识变量名字 variable
	IDENTIFIER ,

	//常量		主要用于标识各种常量 num string char
	INT_VAL /*整型常量*/, REAL_VAL /*实型常量*/, CHAR_VAL /*字符常量*/, STR_VAL /*字符串常量*/,

	//关键字	主要用于类型声明与语句 declare statement 除了sizeof
	STORAGE_CLASS /*存储类型 auto extern register static*/ , TYPE_QUALIFIER /*类型限定符号 const volatile*/ ,
	TYPE /*数据类型void char  short  int  long  float  double */ , STRUCT_UNION /*结构体和联合体 sruct union*/ , ENUM /*enum*/ ,
	STRUCTDEF /*结构体类型定义structdef */ , IF	/*if*/ , ELSE	/*else*/ , DO /*do*/ , WHILE /*while*/ , SWITCH	/*switch*/ , CASE	/*case*/ , DEFAULT	/*default*/ , LABEL /*label*/ ,
	FOR	/*for*/ , CONTINUE	/*continue*/ , BREAK /*break*/ , GOTO /*goto*/ , RETURN /*return*/ , FUNCTION /*function*/ , NULL_STATE /*;*/ ,

	//操作符	主要用于表达式expression 我们取消相同的符号代表不同的操作意义这种情况 比如 &既是取指又是按位与
	//我们将取地址符由 & 改为 $  , 将解除引用由* 修改为 !$
	SUBSCRIPT /*数组下标操作 [ 优先级15*/ , SELECT /*选择成员符号 . -> 优先级15 */ , PREFIX /*单目前缀 ++ --  ~ ! !$ $   优先级14*/ ,
	SIZEOF /*类型长度操作符 前缀 优先级14*/ , CAST /*类型转换符 优先14*/ ,
	MULTIPLE /*算术乘除取模 * % /  优先级13*/ ,  ADDITIVE /*算术加减 + -  优先级12*/ , SHIFT /*移位 << >> 优先级11*/ ,
	RELATION	/*大小关系 < > <= >= 优先级10*/ , EQUALITY /*相等关系 == != 优先级9*/ ,
	BIT_AND /*安位与 & 优先级8*/ , BIT_XOR /*安位异或 ^ 优先级7*/ , BIT_OR /*按位或 | 优先级6*/ ,
	LOGIC_AND /*逻辑与 && 优先级5*/ ,  LOGIC_OR /*逻辑或 || 优先级4*/ ,
	CONDITION /*条件 ?: 优先级3*/ , ASSIGN /*赋值 = += -= /= *= %=  &= ^= |= 优先级2*/ , SEQUEN /*逗号操作符 , 优先级1*/ ,

	//重要标记
	END_SUBSCRIPT /*右中括弧 】 左中括弧用作下标操作符*/ , END_CONDITION /*条件操作符结束标志 : */ ,
	LPARENT /*左小括弧 ( */ , RPARENT /*右小括弧 ) */ , 	LBRACE /*左大括弧 { */ , RBRACE /*右大括弧 } */ ,
	QUOTE /*单引号 ' */ , DBQUOTE /*双引号 “ */ , CTRL /*回车换行 \n*/ ,  END_FILE	/*文件末尾*/ ,

	//类型 主要和前面的结合一起用于词法分析阶段之后的处理过程
	COMMENT	 /*注释：// 或者 /*   */ , ERROR /*错误*/

}token_type;


/*处理源文件输入流的数据结构*/
#define LINE_BUFF_LEN 	512	/*一行最多字符数*/
#define TOKEN_BUFF_LEN	32	/*一个token的最多字符*/

struct _src_buff{
	FILE *file;						/*源文件句柄*/
	token_type token;		/*当前token_buff对应的token类型*/
//	char *current;

	unsigned short line;	/*行数*/

	unsigned short index_line;	/*指向line_buff当前指针*/
	unsigned short index_token;	/*指向token_buff当前指针*/

//	unsigned short real_chars;	/*该行实际字符数目*/

	char line_buff[LINE_BUFF_LEN];
	char token_buff[TOKEN_BUFF_LEN];

};

extern struct _src_buff src_buff;

/*进行词法分析的主要函数*/
extern int get_token(void);
extern int match(token_type token);


#endif /* LEXICAL_ANALY_H_ */
