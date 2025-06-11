# 题目概述

在本实验中，你需要依靠实验一中完成的词法分析器对输入进行预处理得到Token流，利用LR（1）方法进行语法分析并进行语法制导翻译生成符号表和四元式，并将其输出。该实验的输出对后续实验中的目标代码生成同样有着重要作用。为保证输出结果唯一，语法生成式和相应的制导翻译规则在后面给出。

# 程序输入

输入为一段c语言风格的程序，需要利用实验一的词法分析器作预处理。如果是一段正确的程序，则一定能被后文中所给出的文法推导产生。

# 使用文法

## 文法中出现的所有终结符

int    double     scanf     printf     if     then     while    do    ,     ;    +    -     *    /    =    ==   !=    <    <=    >    >=    (   )    {    }    !    &&    ||  id  UINT UFLOAT
其中UINT,UFLOAT，id是在词法阶段识别完成的标识符，整数和浮点数常量。
这里不包含空字符和结束字符，可以根据自己的代码实现加入。

## 文法中出现的所有非终结符

PROG SUBPROG M N VARIABLES  STATEMENT VARIABLE T ASSIGN SCANF PRINTF L
B  EXPR ORITEM ANDITEM RELITEM NOITEM
ITEM FACTOR BORTERM BANDTERM PLUS_MINUS MUL_DIV REL SCANF_BEGIN PRINTF_BEGIN ID

注意区分ID（非终结符）和id（终结符）。

每个非终结符可能具有如下属性中的一个或多个：

quad,name,nextlist,truelist,falselist,type,width,op,place。

## 语法产生式和制导翻译规则

每个产生式后面的中括号斜体文字为其对应的制导翻译规则，在LR1分析法的规约步骤中，通过该规则帮助生成对应的四元式。

### 总程序部分

PROG -> SUBPROG  {}

SUBPROG -> M VARIABLES  STATEMENT

*{backpatch(STATEMENT.nextlist,nxq);gen(End,-,-,-);}*

M -> ^ *{OFFSET=0}*

N -> ^ *{N.quad=nxq}*

### 变量声明部分

VARIABLES -> VARIABLES VARIABLE ; *{}*

VARIABLES -> VARIABLE ; *{}*

T -> int *{T.type=int;T.width=4;}*

T -> double *{T.type=double;T.width=8;}*

ID -> id *{ID.name=id}*

VARIABLE ->T ID

*{enter(ID.name,type=T.type,offset=OFFSET);OFFSET+=T.width;VARIABLE.type=T.type;VARIABLE.width=T.width}*

VARIABLE -> VARIABLE_1 , ID

*{enter(ID.name,type=VARIABLE_1.type,offset=OFFSET);OFFSET+=VARIABLE_1.width;VARIABLE.type=VARIABLE_1.type;VARIABLE.width=VARIABLE_1.width;}*

### 语句部分(赋值、读写、if-then,do-while,复合语句

STATEMENT -> ASSIGN *{STATEMENT.nextlist=mklist()}*

STATEMENT -> SCANF
*{STATEMENT.nextlist=mklist()}*

STATEMENT -> PRINTF *{STATEMENT.nextlist=mklist()}*

STATEMENT -> ^ *{STATEMENT.nextlist=mklist()}*

STATEMENT -> { L ; } *{STATEMENT.nextlist=L.nextlist}*

STATEMENT -> while N_1 B do N_2 STATEMENT_1

*{backpatch(STATEMENT_1.nextlist,N_1.quad);backpatch(B.truelist,N_2.quad);STATEMENT.nextlist=B.falselist;gen(j,-,-,N_1.quad)}*

STATEMENT -> if B then N STATEMENT_1

*{backpatch(B.truelist,N.quad),STATEMENT.nextlist=merge(B.falselist,STATEMENT_1.nextlist)}*

*ASSIGN -> ID = EXPR
{p=lookup(ID.name);gen(=,EXPR.place,-,p) }*

L -> L_1 ; N STATEMENT *{backpatch(L1.nextlist,N.quad),L.nextlist=STATEMENT.nextlist}*

L -> STATEMENT *{L.nextlist=STATEMENT.nextlist}*

### 数值表达式部分

EXPR ->EXPR_1 || ORITEM

*{EXPR.place=newtemp(int);EXPR.type=int;gen(||,EXPR_1.place,ORITEM.place,EXPR.place)}*

EXPR -> ORITEM
*{EXPR.place=ORITEM.place;EXPR.type=ORITEM.type}*

ORITEM -> ORITEM_1 && ANDITEM

*{ORITEM.place=newtemp(int);ORITEM.type=int;gen(&&,ORITEM_1.place,ANDITEM.place,ORITEM.place)}*

ORITEM -> ANDITEM
*{ORITEM.place=ANDITEM.place;ORITEM.type=ANDITEM.type}*

ANDITEM -> NOITEM
*{ANDITEM.place=NOITEM.place;ANDITEM.type=NOITEM.type;}*

ANDITEM -> ! NOITEM

*{ANDITEM=newtemp(int);ANDITEM.type=int;gen(!,NOITEM.place,-,ANDITEM.place)}*

NOITEM -> NOITEM_1 REL RELITEM

*{NOITEM.place=newtemp(int);NOITEM.type=int;gen(REL.op,NOITEM_1.place,RELITEM.place,NOITEM.place)}*

NOITEM -> RELITEM *{NOITEM.place=RELITEM.place;NOITEM.type=RELITEM.type}*

RELITEM -> RELITEM_1 PLUS_MINUS ITEM

*{RELITEM.place=newtemp(RELITEM_1.type);RELITEM.type=RELITEM_1.type;gen(PLUS_MINUS.op,RELITEM_1.place,ITEM.place,RELITEM.place)}*

RELITEM -> ITEM *{RELITEM.place=ITEM.place;RELITEM.type=ITEM.type}*

ITEM -> FACTOR  *{ITEM.place=FACTOR.place;ITEM.type=FACTOR.type}*

ITEM -> ITEM MUL_DIV FACTOR

*{ITEM.place=newtemp(FACTOR.type);ITEM.type=FACTOR.type;gen(MUL_DIV.op,ITEM_1.place,FACTOR.place,ITEM.place)}*

FACTOR -> ID  *{FACTOR.place=lookup(ID.name);FACTOR.type=lookup_type(ID.name)}*

FACTOR -> UINT

*{FACTOR.place=newtemp(int);FACTOR.type=int;gen(=,UINT,-,FACTOR.place)}*

FACTOR -> UFLOAT

*{FACTOR.place=newtemp(double);FACTOR.type=double;gen(=,UFLOAT,-,FACTOR.place)}*

FACTOR -> ( EXPR ) *{FACTOR.place=EXPR.place;FACTOR.type=EXPR.type}*

FACTOR -> PLUS_MINUS FACTOR_1

*{FACTOR.place=newtemp(FACTOR_1.type);FACTOR.type=FACTOR_1.type;gen(PLUS_MINUS.op,0,FACTOR_1.place,FACTOR.place)}*

### 作为条件控制的表达式

B -> B_1 || N BORTERM

*{backpatch(B_1.falselist,N.quad);B.truelist=merge(B_1.truelist,BORTERM.truelist);B.falselist=BORTERM.falselist}*

B -> BORTERM
*{B.truelist=BORTERM.truelist;B.falselist=BORTERM.falselist}*

BORTERM -> BORTERM_1 && N
BANDTERM

*{backpatch(BORTERM_1.truelist,N.quad);BORTERM.falselist=merge(BORTERM_1.falselist,BANDTERM.falselist;BORTERM.truelist=BANDTERM.truelist)}*

BORTERM -> BANDTERM

*{BORTERM.truelist=BANDTERM.truelist;BORTERM.falselist=BANDTERM.falselist}*

BANDTERM -> ( B )

{BANDTERM.truelist=B.truelist;BANDTERM.falselist=B.falselist}

BANDTERM ->  ! BANDTERM_1

*{BANDTERM.truelist=BANDTERM_1.falselist;BANDTERM.falselist=BANDTERM_1.truelist}*

BANDTERM -> BFACTOR_1 REL BFACTOR_2

*{BANDTERM.truelist=mklist(nxq);BANDTERM.falselist=mklist(nxq+1);gen(j+REL.op,BFACTOR_1.place,BFACTOR_2.place,0);gen(j,-,-,0);}*

BANDTERM -> BFACTOR

*{BANDTERM.truelist=mklist(nxq);BANDTERM.falselist=mklist(nxq+1);gen(jnz,BFACTOR.place,-,0);gen(j,-,-,0)}*

BFACTOR -> UINT

*{BFACTOR.place=newtemp(int);BFACTOR.type=int;gen(=,UINT,-,FACTOR.place)}*

BFACTOR -> UFLOAT

*{BFACTOR.place=newtemp(double);BFACTOR.type=double;gen(=,UFLOAT,-,BFACTOR.place)}*

BFACTOR -> ID
*{BFACTOR.place=lookup(ID.name);BFACTOR.type=lookup_type(ID.name)}*

### 运算符

PLUS_MINUS -> + *{PLUS_MINUS.op='+'}*

PLUS_MINUS -> -  *{ PLUS_MINUS.op='-'}*

MUL_DIV -> \*  *{MUL_DIV.op='\*'}*

MUL_DIV -> /  *{ MUL_DIV.op='/'}*

REL -> == *{REL.op='=='}*

REL -> !=  *{ REL.op='!='}}*

REL -> <  *{ REL.op='<'}}*

REL -> <= *{ REL.op='<='}*

REL -> >  *{ REL.op='>'}}*

REL -> >= *{ REL.op='>='}*

### 读写语句

SCANF -> SCANF_BEGIN ) *{}*

SCANF_BEGIN -> SCANF_BEGIN , ID *{p=lookup(ID.name);gen(R,-,-,p)}*

SCANF_BEGIN -> scanf ( ID       *{p=lookup(ID.name);gen(R,-,-,p)}*

PRINTF -> PRINTF_BEGIN ) *{}*

PRINTF_BEGIN -> printf ( ID       *{p=lookup(ID.name);gen(W,-,-,p)}*

PRINTF_BEGIN -> PRINTF_BEGIN , ID {p=lookup(ID.name);gen(W,-,-,p)}

## 补充说明：

1.假定UINT和id，UFLOAT的识别已经在词法分析步骤中完成，故语法分析中认为其是终结符，但需注意大写的ID为非终结符。
2. 实验二中之考虑简单的声明语句，不涉及带作用域的过程声明和调用，因此只使用一张全局符号表。
3.翻译规则中的lookup(name)方法可以理解为返回了指出变量在符号表位置的指针用于给非终结符的place属性赋值，例如若符号表中按顺序有变量A,B,x,y。
则lookup('A')='TB0',lookup('B')='TB1',lookup('x')='TB2',lookup('y')='TB3'。
4.翻译规则中的lookup_type(name)方法则根据输入的变量名，查找并返回符号表中对应变量的类型。
5. 对于newtemp（type）方法，type为类型参数，可以为int,double。同样可以理解为返回一个指针字符川，从'T0_i','T1_d'，‘T2_i’...序号逐渐递增，下划线后的字母表示数据类型int或double,以便实验三生成x86目标代码。
6.enter（name,type,offset）方法为向符号便中添加名称为name,类型为type，偏移量为offset的变量，不可与符号表中已有变量重名。符号表中的变量按照插入的先后顺序从前往后排列。
7.符号表每一行包含如下属性，name:变量或常量名,type:int/double ,value: 初始声明时为null，offset：全局偏移量。在计算偏移量时，一个int类型变量占4个字节，double类型变量占8个字节。
8. ^ 表示空字符。

9. 首条指令起始标号nxq=0。

## 语法错误类型

实验二中的输入不会出现词法错误，但有可能出现语法错误，此时需要你输出“Syntax
Error”。

语法错误包含如下几种类型：

1.程序中使用未定义的变量标识符，或是使用虽然定义但未赋值的变量参与表达式右值的计算。

2.重复定义同名称变量。

3.LR(1)分析中出现错误，即输入程序不满足文法要求。

## 算符优先级从高到低排序：

1.*， / ，表正负数而非二元运算符时的+和-

2.+ ，-

3.关系运算符：== | != |
< | <= | > | >=

4.！

5.&&

6.||

当表达式用于条件和循环语句控制时，作了一定的简化和限制，包括不使用算术运算符，括号只针对已经产生truelist和falselist属性的布尔表达式使用。且所有二元运算符均为左结合。

后面给出的文法已经考虑到了算符优先级、结合性和二义文法的消除，本节只用作提醒。

## 四元式

生成的四元式根据第一个参数算符的不同分为如下几类：

(=,lhs/UINT/UFLOAT,-,dest)： 赋值语句，其参数可能为（临时）变量地址，也可能为整数/浮点数字面值，对于UFLOAT，其在四元式中的输出保留六位小数（即std::tostring(double value)函数的默认保留位数）。

(j,-,-,dest) :无条件跳转。

(jnz,lhs,-,dest): 非零时跳转。

(jθ,lhs,rhs,dest): 关系运算符条件跳转，θ可以为== | != | < | <= | > | >= ，关系运算为真时跳转 ,lhs和rhs必须为同一种类型变量或临时变量。

(op,lhs,rhs,dest): 用于计算，op可以为 + | - | * | /  或者二元关系运算符，lhs和rhs可以为变量或临时变量的地址或常数 ,其中lhs和rhs必须为同一种类型变量或临时变量。对于算出运算，dest类型必须与lhr和rhs相同。对于关系运算，dest类型必须未int。

(&&,lhs,rhs,dest): 与运算，lhs,rhs和dest类型必须为int，此时定义0为True，非0为False。

(||,lhs,rhs,dest) :或运算，同上

(!,lhs,-,dest) :非运算，同上

(R,-,-,dest) ：输入语句，dest为待读入的变量。

(W,-,-,dest) ：输出写语句，dest为待输出的变量。

(End,-,-,-) 整个程序结束，退出。

# 输出要求

## 假如LR1分析成功

1.输出符号表：

第一行输出符号表长度（变量个数）

接着符号表中每个变量输出一行，属性中间以空格分隔：

name type value
offset

对于type属性，若为int则输出0，对于value属性，由于该语言设计中不在编译时进行赋值计算，故统一输出为字符串null。

2.下一行输出所使用的临时变量总个数，即调用newtemp方法的总次数。

3..紧接着下一行开始输出序号和四元式

第一行输出生成四元式总个数

接着每行输出“序号： 对应四元式“。冒号和四元式之间空一格，四元式内部参数用逗号分隔，无空格。

四元式中出现浮点数字面值的，保留六位小数（即std::to_string(double val)的默认输出格式）。

## 假如分析过程中发现任何语法错误

输出一行：Syntax Error。

## 输入输出样例

### 样例1

输入：

```
int a,b,c;
double d,e,f;
int j;
{
    d=2.4;
    e=-3.6*5;
    f=!(d*e)/((d+e)<=d);
    a=4;
    b=+3;
    c=111;
    while a>=0 do
        {
            a=a-1;
            d=d*e;
            while (b!=0 && !c>0)|| !a>0 do
                {
                    b=b-1 <c || a==0;
                    printf(b,d);
                };
            if a==1 then
                printf(c,a,b);
        };
}
```

输出：

```
7
a 0 null 0
b 0 null 4
c 0 null 8
d 1 null 12
e 1 null 20
f 1 null 28
j 0 null 36
28
55
0: (=,2.400000,-,T0_d)
1: (=,T0_d,-,TB3)
2: (=,3.600000,-,T1_d)
3: (-,0,T1_d,T2_d)
4: (=,5,-,T3_i)
5: (*,T2_d,T3_i,T4_i)
6: (=,T4_i,-,TB4)
7: (*,TB3,TB4,T5_d)
8: (+,TB3,TB4,T6_d)
9: (<=,T6_d,TB3,T7_i)
10: (/,T5_d,T7_i,T8_i)
11: (!,T8_i,-,T9_i)
12: (=,T9_i,-,TB5)
13: (=,4,-,T10_i)
14: (=,T10_i,-,TB0)
15: (=,3,-,T11_i)
16: (+,0,T11_i,T12_i)
17: (=,T12_i,-,TB1)
18: (=,111,-,T13_i)
19: (=,T13_i,-,TB2)
20: (=,0,-,T14_i)
21: (j>=,TB0,T14_i,23)
22: (j,-,-,54)
23: (=,1,-,T15_i)
24: (-,TB0,T15_i,T16_i)
25: (=,T16_i,-,TB0)
26: (*,TB3,TB4,T17_d)
27: (=,T17_d,-,TB3)
28: (=,0,-,T18_i)
29: (j!=,TB1,T18_i,31)
30: (j,-,-,34)
31: (=,0,-,T19_i)
32: (j>,TB2,T19_i,34)
33: (j,-,-,37)
34: (=,0,-,T20_i)
35: (j>,TB0,T20_i,47)
36: (j,-,-,37)
37: (=,1,-,T21_i)
38: (-,TB1,T21_i,T22_i)
39: (<,T22_i,TB2,T23_i)
40: (=,0,-,T24_i)
41: (==,TB0,T24_i,T25_i)
42: (||,T23_i,T25_i,T26_i)
43: (=,T26_i,-,TB1)
44: (W,-,-,TB1)
45: (W,-,-,TB3)
46: (j,-,-,28)
47: (=,1,-,T27_i)
48: (j==,TB0,T27_i,50)
49: (j,-,-,20)
50: (W,-,-,TB2)
51: (W,-,-,TB0)
52: (W,-,-,TB1)
53: (j,-,-,20)
54: (End,-,-,-)
```

### 样例2

下面是语法分析出错误时的输出，错误原因是使用未声明变量。

输入：

```
int a,b;
{
a=0;
b=1;
c=2;
}
```

输出：

```
Syntax Error
```

### 样例3

输入：

```
int a,b,c;

{
    scanf(a);
    b=0;
    while a>=0 do
        {
            a=a-1;
            c=a-(a/2)*2;
            if c!=0 then
                b=b+a;
        };
    printf(b);
}
```

输出

```
3
a 0 null 0
b 0 null 4
c 0 null 8
11
23
0: (R,-,-,TB0)
1: (=,0,-,T0_i)
2: (=,T0_i,-,TB1)
3: (=,0,-,T1_i)
4: (j>=,TB0,T1_i,6)
5: (j,-,-,21)
6: (=,1,-,T2_i)
7: (-,TB0,T2_i,T3_i)
8: (=,T3_i,-,TB0)
9: (=,2,-,T4_i)
10: (/,TB0,T4_i,T5_i)
11: (=,2,-,T6_i)
12: (*,T5_i,T6_i,T7_i)
13: (-,TB0,T7_i,T8_i)
14: (=,T8_i,-,TB2)
15: (=,0,-,T9_i)
16: (j!=,TB2,T9_i,18)
17: (j,-,-,3)
18: (+,TB1,TB0,T10_i)
19: (=,T10_i,-,TB1)
20: (j,-,-,3)
21: (W,-,-,TB1)
22: (End,-,-,-)
```

### 样例4

输入：

```
int a,b,c,d,e,f;
{
    a=0;
    b=1;
    c=2;
    d=0;
    e=0;
    f=0;
    if !(!(a && d || (e<5)))  then
        d=b+c;
    if ! (a<=b) then
        e=b*c;

    printf(d,e,f);
}
```

输出：

```
6
a 0 null 0
b 0 null 4
c 0 null 8
d 0 null 12
e 0 null 16
f 0 null 20
9
29
0: (=,0,-,T0_i)
1: (=,T0_i,-,TB0)
2: (=,1,-,T1_i)
3: (=,T1_i,-,TB1)
4: (=,2,-,T2_i)
5: (=,T2_i,-,TB2)
6: (=,0,-,T3_i)
7: (=,T3_i,-,TB3)
8: (=,0,-,T4_i)
9: (=,T4_i,-,TB4)
10: (=,0,-,T5_i)
11: (=,T5_i,-,TB5)
12: (jnz,TB0,-,14)
13: (j,-,-,16)
14: (jnz,TB3,-,19)
15: (j,-,-,16)
16: (=,5,-,T6_i)
17: (j<,TB4,T6_i,19)
18: (j,-,-,21)
19: (+,TB1,TB2,T7_i)
20: (=,T7_i,-,TB3)
21: (j<=,TB0,TB1,25)
22: (j,-,-,23)
23: (*,TB1,TB2,T8_i)
24: (=,T8_i,-,TB4)
25: (W,-,-,TB3)
26: (W,-,-,TB4)
27: (W,-,-,TB5)
28: (End,-,-,-)
```

### 样例5

输入：

```
int a,b,c;
double x,y;
int d,e;
double z;

```

输出：

```
8
a 0 null 0
b 0 null 4
c 0 null 8
x 1 null 12
y 1 null 20
d 0 null 28
e 0 null 32
z 1 null 36
0
1
0: (End,-,-,-)

```

# 代码实现思路

建议同学们在完成实验二时，分为如下几步：

1.实现针对任意给定文法的LR1分析表的生成模块（根据指定文法生成LR1表的代码网上有不少，可以参考借鉴），文法可以以资源文件的形式存储供该模块读取。

2.实现基于1中生成的LR1分析表和词法分析得到的token序列进行LR1分析的分析栈。

前两步中可以先不考虑四元式的生成，首先的目标是确保源代码输入可以正确归约至ACC状态。

3.按照题目所给的语法规则，添加归约生成四元式的功能。

在调试1，2，3模块时，可以先使用简单程序调试文法中的一小部分，确定代码编写思路正确后再测试全部的完整文法。

上述思路供大家参考，也欢迎同学们按照自己的想法完成该实验。

其他注意事项：正常情况下输出末尾会有一个换行，Syntax Error情况下输出末尾没有换行。
