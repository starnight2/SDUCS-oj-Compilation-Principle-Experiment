# SDUCS编译原理实验

### 注：实验A和实验B如果在windows下运行，粘贴输入后需要按"Enter"后，"Ctrl+Z+Enter"才能执行(程序在等待**输入结束标志**，Linux是"Ctrl+D")

#### 合理参考学习，可配合AI放心食用

![image-20250611205127586](C:\Users\Night\AppData\Roaming\Typora\typora-user-images\image-20250611205127586.png)



oj实验声明：

在每个实验中，你应该把代码和编译所需的文件放到名为 answer.zip 的压缩文件中并提交测评。

上传 zip 压缩文件时，请注意压缩包内不包含任何文件夹(压缩时直接全选所有代码和编译相关文件，而不是对文件夹进行压缩)。

在实验一二三中，上传的 zip 文件应该包含一个 build.sh 脚本用于编译 c++ 代码生成可执行文件 Main（注意该文件由评测服务器根据脚本生成，提交压缩包中不应包含该可执行文件）。例如：

```
#! /bin/bash
g++ -c lexer.cpp -o lexer.o -O2
g++ -c parser.cpp -o parser.o -O2
g++ -c main.cpp -o main.o -O2

g++ lexer.o parser.o main.o -o Main -lm
```

测评服务器采用 Ubuntu 系统，因此输入和输出文件都采用 LF 格式而非 Windows 系统常用的 CRLF，二者的区别是换行是用一个字符\n 还是两个字符\r\n。

在提交压缩包中最好不要包含额外子文件夹。最好不要 include 万能头文件。

编译命令最好不要使用-O2 优化，有可能会因编译时间过长导致超时。
