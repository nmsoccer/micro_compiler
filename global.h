/*
 * global.h
 *
 *  Created on: 2011-5-16
 *      Author: leiming
 */

#ifndef GLOBAL_H_
#define GLOBAL_H_


#include <stdio.h>



/*条件编译*/
#define DEVELOP
//#define TRACE_LEX	/*是否跟踪词法分析*/
//#define TRACE_SYN	/*是否跟踪语法分析*/
#define TRACE_SEM	/*是否跟踪语义分析*/

#define DUMP_ERR(e) printf("%s" , e);


#ifdef DEVELOP
#define DUMP_STR(e) printf("%s" , e);
#else
#define DUMP_STR(e)
#endif

#ifdef TRACE_LEX
#define DUMP_INFO(no_line , e) printf("line %d:   %s\n" , no_line , e);
#define DUMP_LEX_STR(e) printf("%s" , e);
#else
#define DUMP_INFO(no_line , e)
#define DUMP_LEX_STR(e)
#endif


#ifdef TRACE_SYN
#define DUMP_SYN(no_line , e) printf("line %d:   %s\n" , no_line , e);
#define DUMP_SYN_STR(e) printf("%s" , e);
#else
#define DUMP_SYN(e)
#define DUMP_SYN_STR(e)
#endif

#ifdef TRACE_SEM
#define DUMP_SEM_STR(e) printf("%s" , e);
#else
#define DUMP_SEM_STR(e)
#endif


/*在实际机器中个类型所占字节*/
#define CHAR_SIZE 		1
#define SHORT_SIZE		2
#define INT_SIZE			4
#define LONG_SIZE		4
#define FLOAT_SIZE		4
#define DOUBLE_SIZE	8

#define ALIGN_BYTE		4	/*对齐的字节*/

/*想要编译的文件名长度*/
#define COMPILE_FILE_NAME_LEN 24
extern char compile_file_name[COMPILE_FILE_NAME_LEN];






#endif /* GLOBAL_H_ */
