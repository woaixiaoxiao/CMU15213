/*
 * CS:APP Data Lab
 *
 * <Please put your name and userid here>
 *
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting if the shift amount
     is less than 0 or greater than 31.


EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implement floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants. You can use any arithmetic,
logical, or comparison operations on int or unsigned data.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operations (integer, logical,
     or comparison) that you are allowed to use for your implementation
     of the function.  The max operator count is checked by dlc.
     Note that assignment ('=') is not counted; you may use as many of
     these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */

#endif
// 1
/*
 * bitXor - x^y using only ~ and &
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y) {
  // 两个1的位置是0，否则是1
  int a = ~(x & y);
  // 两个0的位置是0，否则是1
  int b = ~(~x & ~y);
  // 因此将a和b按位&之后，两个1或者两个0的位置都是0，其他位置是1
  return a & b;
}
/*
 * tmin - return minimum two's complement integer
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) { return 1 << 31; }
// 2
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 1
 */
int isTmax(int x) {
  // 0xffffffff，这一步不能用异或
  int a = x + x + 1;
  // 0x00000000，如果b为0，那么b要么是0x3fffffff，要么是0xffffffff
  int b = ~a;
  // 排除x为0xffffffff，如果x是0xffffffff，那么c就是1，否则c就是0
  int c = !(x + 1);
  // 只有b和c都是0，才返回1
  return !(b | c);
}
/*
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   where bits are numbered from 0 (least significant) to 31 (most significant)
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x) {
  // 构造出奇数位均为1的数
  int a = (0xaa << 24) + (0xaa << 16) + (0xaa << 8) + 0xaa;
  // 将a看做掩码，取出x中所有奇数位
  int b = x & a;
  // 判断a和b是否相同，只有当a和b相同，异或才为0，那么取！后才为1
  return !(a ^ b);
}
/*
 * negate - return -x
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) { return (~x) + 1; }
// 3
/*
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0'
 * to '9') Example: isAsciiDigit(0x35) = 1. isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int isAsciiDigit(int x) {
  // 0x 0011 0000
  // 0x 0011 1001
  // 首先，右移4位之后，应该是0x3，即a=1
  int a = !((x >> 4) ^ (0x3));
  // 低4位，要么第4位为0，要么就只能是1001或者1000
  // b为1，代表第4位为0
  int b = !((x >> 3) & 1);
  // c为1，代表为1001或者1000，即第1位和第4位无所谓，但是第2和第3位必须是0
  int c = !(x & 0x6);
  // printf("-----------------\n%x\n%d\n%d\n%d\n%d\n",x,a,b,c,a&(b|c));
  return a & (b | c);
}
/*
 * conditional - same as x ? y : z
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z) {
  // 如果x不为0，则得到一个全1的数，如果x为0，则得到一个全0的数
  // 如果x为0，则得到0，如果x非0，则得到1
  int a = !!x;
  // 如果a为0，则b为0，如果a为1，则b为-1，即全1
  int b = ~a + 1;
  // c和d一定有一个为0
  int c = b & y;
  int d = ~b & z;
  return c | d;
}
/*
 * isLessOrEqual - if x <= y  then return 1, else return 0
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
  // 如果符号不同，则正数更大，如果符号相同，则看差值，还要特判一下x是否和y相同
  int x_flag = x >> 31 & 1;
  int y_flag = y >> 31 & 1;
  // 如果flag_not_same为1，则代表符号不同，如果为0，则代表符号相同
  int flag_not_same = x_flag ^ y_flag;
  // 还要结合差值的正负来看,x-y，即x+(y的补码)
  int y_ = ~y + 1;
  int sub_flag = (x + y_) >> 31 & 1;
  // 如果符号不同并且x为负，即a=1，即x_flag=1，并且flag_not_same=1，
  int a = flag_not_same & x_flag;
  // 如果符号相同，并且差值为负，即b=1，即flag_same=0,sub_flag=1;
  int b = !(flag_not_same | (!sub_flag));
  // 如果两个数相同，则c为1，否则c为0
  int c = !(x ^ y);
  return a | b | c;
}
// 4
/*
 * logicalNeg - implement the ! operator, using all of
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4
 */
int logicalNeg(int x) {
  // 先得到最大的32位有符号数
  int T_max = ~(1 << 31);
  // 得到x的符号
  int x_sign = x >> 31 & 1;
  // 将x和T_max相加，除了0，其他的数加上去之后一定是一个负数
  int a = x + T_max;
  int a_sign = a >> 31 & 1;
  // 只有当x和a的sign都是0时，才返回1，否则返回0
  return (x_sign ^ 1) & (a_sign ^ 1);
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {
  // 如果是正数，那么直接求x的最高位1，如果是负数，则是要求最高位的0
  // 假设这个最高位为第x位，则答案案为x+1位，因为正数需要加上符号0，负数需要加上符号1
  // 对于负数，先预处理，将所有的1变成0，所有的0变成1
  // 如果x为正数，则help为0，如果x为负数，则help为0xffffffff
  int ans = 0;
  int help = x >> 31;
  // 通过help将x的1变成0,0变成1。若help为0，则x不变，若help为全1，则完成转换的任务
  x = x ^ help;
  // 下面就统一为了计算最高位的1所在的位置
  // 如果高16位不为0，则has_high_16为1，否则为0
  int has_high_16 = !!(x >> 16);
  // 如果高16位存在，即has_high_16为1，那么说明低16位肯定跑不掉了，正好就是has_high_16<<4
  // 如果高16位不存在，has_high_16为0，低16位就不一定都要，此时左移4位正好是0
  int add_bits = has_high_16 << 4;
  ans += add_bits;
  // 又是一个很巧妙的操作，如果add_bits不为0，说明低16位肯定是需要的，那么就不用管低16位，直接移位
  // 如果add__bits为0，说明高16位肯定不需要，低16位可能需要，那么此时右移0位，接下来正常处理低16位
  x >>= add_bits;

  int has_high_8 = !!(x >> 8);
  add_bits = has_high_8 << 3;
  ans += add_bits;
  x >>= add_bits;

  int has_high_4 = !!(x >> 4);
  add_bits = has_high_4 << 2;
  ans += add_bits;
  x >>= add_bits;

  int has_high_2 = !!(x >> 2);
  add_bits = has_high_2 << 1;
  ans += add_bits;
  x >>= add_bits;

  int has_high_1 = !!(x >> 1);
  add_bits = has_high_1 << 0;
  ans += add_bits;
  x >>= add_bits;

  // x可能现在是1
  ans += x;

  return ans + 1;
}
// float
/*
 * floatScale2 - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned floatScale2(unsigned uf) {
  // 分别取出符号，指数，以及有效数位，
  // 其中有效的取出比较tricky，是对sign和e的对应位置进行异或，即将高9位都置为0，剩下的就是有效数位了
  unsigned sign = (uf >> 31) & 1;
  unsigned e = (uf >> 23) & 0xff;
  unsigned f = uf ^ (sign << 31) ^ (e << 23);
  // 如果uf为NaN等特殊值，即指数为全1，直接返回这个特殊值本身
  if (!(e ^ 0xff)) {
    return uf;
  }
  // 如果uf为非规格化的数，即指数为0，直接将f*2即可
  // 这里感觉有点问题，如果这个非规格数*2后达到了规格数的范围了，是不是要额外处理？
  if (!e) {
    return (sign << 31) | (f << 1);
  }
  // 如果uf为规格化的数
  return (sign << 31) | ((e + 1) << 23) | (f);
}
/*
 * floatFloat2Int - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int floatFloat2Int(unsigned uf) {
  // 分别取出符号，指数，以及有效数位，
  // 其中有效位的取出比较tricky，是对sign和e的对应位置进行异或，即将高9位都置为0，剩下的就是有效数位了
  unsigned sign = (uf >> 31) & 1;
  unsigned e = (uf >> 23) & 0xff;
  unsigned f = uf ^ (sign << 31) ^ (e << 23);
  // 如果指数大于等于31了，因为要返回的值是int类型，1<<31位直接爆int了，所以返回0x80000000u
  int E = e - 127;
  if (E >= 31) {
    return 0x80000000;
  }
  // 如果指数小于0了，那么肯定是返回0，因为需要对小数部分除2，那么对int来说，就是0
  if (E < 0) {
    return 0;
  }
  // 真正的小数部分，是有一个隐藏的1在最前面的，这里不用考虑非规格化数，因为它已经在前面的E<0里给淘汰了
  int frac = f | 0x800000;
  // 这个小数部分是用整数来表示的，即默认左移了23位，那么当前的移位应该减去23
  int real_f = (E > 23) ? (frac << (E - 23)) : (frac >> (23 - E));
  return sign ? -real_f : real_f;
}
/*
 * floatPower2 - Return bit-level equivalent of the expression 2.0^x
 *   (2.0 raised to the power x) for any 32-bit integer x.
 *
 *   The unsigned value that is returned should have the identical bit
 *   representation as the single-precision floating-point number 2.0^x.
 *   If the result is too small to be represented as a denorm, return
 *   0. If too large, return +INF.
 *
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. Also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned floatPower2(int x) {
  // 得到阶码
  int e = x + 127;
  // 非规格数，直接返回0
  if (e <= 0) {
    return 0;
  }
  // 无穷
  if (e >= 0xff) {
    return 0x7f800000;
  }
  // 规格数，符号位0，小数部分也是0
  return e << 23;
}
