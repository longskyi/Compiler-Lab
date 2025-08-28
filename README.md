# Compiler-Lab
C++23项目，你需要尽可能新的GCC MSVC编译器，以及CMake。

项目内容远超课程要求，仅供参考。

readme内容待完善

```
LCCompiler
用法:
  1. SLR语法分析:
    LCCompiler -slr <grammar文件> <terminal文件> [<SLR冲突解决文件>] -o=<输出文件名>

  2. 生成AST,若使用外置参数文件则生成通用AST结点:
    LCCompiler -ast -i=<输入文件> [-p=<参数文件>] -o=<输出文件名>

  3. 生成汇编:
    LCCompiler -s -i=<输入文件> -o=<输出文件名>

  4. 帮助:
    LCCompiler -h 或 LCCompiler --help

选项说明:
  -slr     执行SLR语法分析
  -ast     执行AST处理
  -s       执行S处理
  -i       指定输入文件
  -p       指定参数文件(可选)
  -o       指定输出文件名(不含后缀)
  -h/--help 显示帮助信息
```
