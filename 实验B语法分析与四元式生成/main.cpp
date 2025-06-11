#include <iostream>
#include <vector>
#include <string>

using namespace std;
// 声明词法分析接口
extern vector<pair<string, string>> lexAnalysis();

// 声明语法分析接口（在 2.cpp 中定义）
extern void parseProgram(const vector<pair<string, string>> &tokens);

extern void outputResults();

int main()
{
    auto tokens = lexAnalysis();
    //输出tokens
    // for (const auto &token : tokens)
    // {
    //     cout << token.first << " " << token.second << endl;
    // }
    // 执行语法分析
    parseProgram(tokens); // 这里会调用 2.cpp 里的函数
    // 分析完成后输出结果
    outputResults();
    return 0;
}