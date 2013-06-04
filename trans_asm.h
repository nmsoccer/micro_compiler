/*
 * trans_asm.h
 *
 *  Created on: 2011-6-23
 *      Author: leiming
 */

#ifndef TRANS_ASM_H_
#define TRANS_ASM_H_


/*
 * 内存对齐策略。
 * 4字节对齐。
 *
 * 普通变量在定义时按照大小，如果不足4字节补足4字节，占用4字节空间。如果大于4字节(double)则8字节
 * 结构体定义时成员与普通变量定义一样，小于4字节补足4字节。最终结构体大小一定是4字节对齐的
 * 数组成员也是4字节对齐
 *
 *
 */
/*
 * 翻译汇编代码策略
 *
 * 表达式产生的值都是通过eax返回
 *
 */

/*
 * 翻译成汇编文件
 * @param0 src_file: 源文件名
 */
extern int trans_asm(char *src_file);



#endif /* TRANS_ASM_H_ */
