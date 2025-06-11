#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <iomanip>
#include <cstdlib>
#include <cctype>

using namespace std;

void outputResults();
int getFinalTarget(int idx);

// 符号表项
struct Symbol
{
    string name, type;
    int offset;
};

// 四元式的操作数类型
enum ArgKind
{
    NONE,
    VAR,
    TEMP,
    CONST
};

// Arg结构体增加type
struct Arg
{
    ArgKind kind = NONE;
    int varIndex = -1;   // 符号表下标（变量）
    int tempId = -1;     // 临时变量编号
    double constVal = 0; // 常数值
    string type;         // "int" 或 "double"
};

// 四元式结构
struct Quad
{
    string op;
    Arg arg1, arg2, result;
};



// 全局变量：符号表和四元式列表
vector<Symbol> symTable;
unordered_map<string, int> symIndex; // 变量名→符号表下标
int nextOffset = 0;
vector<Quad> quads;
int tempCount = -1;


struct JumpChain 
{
    vector<int> trueList;
    vector<int> falseList;
};

void backpatch(std::vector<int> &list, int target) 
{
    for (int idx : list) 
        quads[idx].result = {CONST, -1, -1, (double)target, "int"};
}

std::vector<int> merge(const std::vector<int> &a, const std::vector<int> &b) 
{
    vector<int> res = a;
    res.insert(res.end(), b.begin(), b.end());
    return res;
}





// 获取新临时变量操作数
Arg newTemp(const string& type)
{
    tempCount++;
    Arg a;
    a.kind = TEMP;
    a.tempId = tempCount;
    a.type = type;
    return a;
}

// 产生四元式并保存
void GEN(const string &op, Arg a1, Arg a2, Arg res)
{
    Quad q;
    q.op = op;
    q.arg1 = a1;
    q.arg2 = a2;
    q.result = res;
    quads.push_back(q);

    // 输出四元式
    // cout <<"\n++"<< setw(5) << quads.size() - 1 << ": " 
    //      << op << " " 
    //      << (a1.kind == NONE ? "_" : (a1.kind == VAR ? symTable[a1.varIndex].name : ("T" + to_string(a1.tempId)))) << " "
    //      << (a2.kind == NONE ? "_" : (a2.kind == VAR ? symTable[a2.varIndex].name : ("T" + to_string(a2.tempId)))) << " "
    //      << (res.kind == NONE ? "_" : (res.kind == VAR ? symTable[res.varIndex].name : ("T" + to_string(res.tempId)))) 
    //      << endl;
    //cout<<"++"<<endl;
    //outputResults();
}

// 错误处理：输出并终止
void syntaxError()
{
    cout << "Syntax Error";
    exit(0);
}

// 获取当前令牌
pair<string, string> peek(const vector<pair<string, string>> &tokens, int cur)
{
    if (cur < tokens.size())
        return tokens[cur];
    else
    {
        syntaxError();
        return {"", ""}; // 不会执行到此
    }
}

// 读取下一个令牌
pair<string, string> nextToken(const vector<pair<string, string>> &tokens, int &cur)
{
    if (cur < tokens.size())
        return tokens[cur++];
    else
    {
        syntaxError();
        return {"", ""}; // 不会执行到此
    }
}

// 解析函数声明的标识符列表： id {, id}
void parseIdList(const vector<pair<string, string>> &tokens, int &cur, const string &type)
{
    while (true)
    {
        auto tok = nextToken(tokens, cur);
        string name = tok.first;
        // 检查标识符已定义
        if (symIndex.count(name))
            syntaxError();
        // 插入符号表
        symTable.push_back({name, type, nextOffset});
        symIndex[name] = symTable.size() - 1;
        // 根据类型累加offset
        if (type == "int")
            nextOffset += 4;
        else if (type == "double")
            nextOffset += 8;
        else
            syntaxError();
        // 查看下一个符号
        if (peek(tokens, cur).first == ",")
            nextToken(tokens, cur);
        else
            break;
    }
}

// 解析声明语句： type idList ;
void parseDecl(const vector<pair<string, string>> &tokens, int &cur)
{
    auto tok = nextToken(tokens, cur);
    string type = tok.first; // "int" 或 "double"
    //cout<< "Declaring type: " << type << endl;

    parseIdList(tokens, cur, type);
    // Expect ';'
    if (peek(tokens, cur).first != ";")
        syntaxError();
    nextToken(tokens, cur);
}


// 前向声明表达式解析
Arg parseExpr(const vector<pair<string, string>> &, int &);
Arg parseFullExpr(const vector<pair<string, string>> &, int &);
void parseStmt(const vector<pair<string, string>> &tokens, int &cur);

// 前向声明
JumpChain parseLogicJump(const vector<pair<string, string>> &tokens, int &cur);

// 解析因子： '(' Expr ')' | id | number | '-' Factor
Arg parseFactor(const vector<pair<string, string>> &tokens, int &cur)
{
    auto p = peek(tokens, cur);
    // 一元正号
    if (p.first == "+")
    {
        nextToken(tokens, cur); // 读取 '+'
        Arg f = parseFactor(tokens, cur);
        Arg temp = newTemp(f.type);
        GEN("+", Arg{CONST, -1, -1, 0, "int"}, f, temp);
        return temp;
    }
    // 一元负号
    if (p.first == "-")
    {
        nextToken(tokens, cur); // 读取 '-'
        Arg f = parseFactor(tokens, cur);
        Arg temp = newTemp(f.type);
        GEN("-", Arg{CONST, -1, -1, 0, "int"}, f, temp);
        return temp;
    }
    // 括号
    if (p.first == "(")
    {
        nextToken(tokens, cur); // '('
        Arg res = parseFullExpr(tokens, cur);
        if (peek(tokens, cur).first == ")")
            nextToken(tokens, cur);
        else
            syntaxError();
        return res;
    }
    // 标识符
    if (p.second == "IDENT")
    {
        nextToken(tokens, cur);
        string name = p.first;
        if (!symIndex.count(name))
            syntaxError();
        int idx = symIndex[name];
        return {VAR, idx, -1, 0, symTable[idx].type};
    }
    // 整数常量
    if (p.second == "INT")
    {
        nextToken(tokens, cur);
        Arg temp = newTemp("int");
        GEN("=", {CONST, -1, -1, stod(p.first), "int"}, Arg(), temp);
        return temp;
    }
    // 浮点常量
    if (p.second == "DOUBLE")
    {
        nextToken(tokens, cur);
        Arg temp = newTemp("double");
        GEN("=", {CONST, -1, -1, stod(p.first), "double"}, Arg(), temp);
        return temp;
    }
    syntaxError();
    return Arg();
}

Arg parseTerm(const vector<pair<string, string>> &tokens, int &cur)
{
    Arg left = parseFactor(tokens, cur);
    while (true)
    {
        auto p = peek(tokens, cur);
        if (p.first == "*" || p.first == "/")
        {
            string op = p.first;
            nextToken(tokens, cur);
            Arg right = parseFactor(tokens, cur);
            //string resultType = (left.type == "double" || right.type == "double") ? "double" : "int";
            // 结果类型和右操作数一致
            string resultType = right.type;
            Arg temp = newTemp(resultType);
            GEN(op, left, right, temp);
            left = temp;
        }
        else
            break;
    }
    return left;
}

// 解析表达式： Term {('+'|'-') Term}
Arg parseExpr(const vector<pair<string, string>> &tokens, int &cur)
{
    Arg left = parseTerm(tokens, cur);
    while (true)
    {
        auto p = peek(tokens, cur);
        if (p.first == "+" || p.first == "-")
        {
            string op = p.first;
            nextToken(tokens, cur);
            Arg right = parseTerm(tokens, cur);
            //string resultType = (left.type == "double" || right.type == "double") ? "double" : "int";
            // 结果类型和右操作数一致
            string resultType = right.type;
            Arg temp = newTemp(resultType);
            GEN(op, left, right, temp);
            left = temp;
        }
        else
            break;
    }
    return left;
}

struct RelResult {
    bool isRel;      // 是否为关系表达式
    string op;       // 关系操作符
    Arg left, right; // 两边操作数
    Arg place;       // 普通表达式结果
};

// 关系表达式
RelResult parseRelExpr(const vector<pair<string, string>> &tokens, int &cur, bool forJump = false)
{
    Arg left = parseExpr(tokens, cur);
    auto p = peek(tokens, cur);
    if (p.first == "==" || p.first == "!=" || p.first == "<" || p.first == "<=" || p.first == ">" || p.first == ">=")
    {
        string op = p.first;
        nextToken(tokens, cur);
        Arg right = parseExpr(tokens, cur);
        if (left.type != right.type) syntaxError();
        if (forJump) {
            // 只返回关系表达式信息，不生成四元式
            return {true, op, left, right, Arg()};
        } else {
            // 普通表达式，生成关系运算四元式
            Arg temp = newTemp("int");
            GEN(op, left, right, temp);
            return {true, op, left, right, temp};
        }
    }
    return {false, "", Arg(), Arg(), left};
}

// 关系表达式跳转链
JumpChain parseRelJump(const vector<pair<string, string>> &tokens, int &cur) 
{
    // 只处理关系表达式
    auto p = peek(tokens, cur);
    if (p.first == "(") {
        nextToken(tokens, cur);
        JumpChain jc = parseLogicJump(tokens, cur);
        if (peek(tokens, cur).first != ")") syntaxError();
        nextToken(tokens, cur);
        return jc;
    }
    Arg left = parseFactor(tokens, cur); // 只允许单因子
    p = peek(tokens, cur);
    if (p.first == "==" || p.first == "!=" || p.first == "<" || p.first == "<=" || p.first == ">" || p.first == ">=") {
        string op = p.first;
        nextToken(tokens, cur);
        Arg right = parseFactor(tokens, cur); // 只允许单因子
        if (left.type != right.type) syntaxError();
        int trueIdx = quads.size();
        GEN("j" + op, left, right, Arg()); // 目标待回填
        int falseIdx = quads.size();
        GEN("j", Arg(), Arg(), Arg()); // 无条件跳转，目标待回填
        return {{trueIdx}, {falseIdx}};
    }
    // 普通表达式，非零为真
    int trueIdx = quads.size();
    GEN("jnz", left, Arg(), Arg());
    int falseIdx = quads.size();
    GEN("j", Arg(), Arg(), Arg());
    return {{trueIdx}, {falseIdx}};
}

// 逻辑非
Arg parseNotExpr(const vector<pair<string, string>> &tokens, int &cur)
{
    auto p = peek(tokens, cur);
    if (p.first == "!")
    {
        nextToken(tokens, cur);
        Arg f = parseNotExpr(tokens, cur);
        Arg temp = newTemp("int");
        GEN("!", f, Arg(), temp);
        return temp;
    }
    // 只取 place 字段
    return parseRelExpr(tokens, cur).place;
}

// 逻辑非
JumpChain parseNotJump(const vector<pair<string, string>> &tokens, int &cur) {
    auto p = peek(tokens, cur);
    if (p.first == "!") {
        nextToken(tokens, cur);
        JumpChain jc = parseNotJump(tokens, cur);
        return {jc.falseList, jc.trueList}; // 交换真假链
    }
    return parseRelJump(tokens, cur);
}

// 支持逻辑非、关系、算术、逻辑与或的完整表达式
Arg parseFullExpr(const vector<pair<string, string>> &tokens, int &cur)
{
    Arg left = parseNotExpr(tokens, cur);
    while (true)
    {
        auto p = peek(tokens, cur);
        if (p.first == "||")
        {
            nextToken(tokens, cur);
            Arg right = parseNotExpr(tokens, cur);
            Arg temp = newTemp("int");
            GEN("||", left, right, temp);
            left = temp;
        }
        else if (p.first == "&&")
        {
            nextToken(tokens, cur);
            Arg right = parseNotExpr(tokens, cur);
            Arg temp = newTemp("int");
            GEN("&&", left, right, temp);
            left = temp;
        }
        else break;
    }
    return left;
}

// 逻辑与或
JumpChain parseLogicJump(const vector<pair<string, string>> &tokens, int &cur) {
    JumpChain left = parseNotJump(tokens, cur);
    while (true) {
        auto p = peek(tokens, cur);
        if (p.first == "&&") {
            nextToken(tokens, cur);
            backpatch(left.trueList, quads.size());
            JumpChain right = parseNotJump(tokens, cur);
            left.falseList = merge(left.falseList, right.falseList);
            left.trueList = right.trueList;
        } else if (p.first == "||") {
            nextToken(tokens, cur);
            backpatch(left.falseList, quads.size());
            JumpChain right = parseNotJump(tokens, cur);
            left.trueList = merge(left.trueList, right.trueList);
            left.falseList = right.falseList;
        } else break;
    }
    return left;
}

// 解析语句块 { StmtList }
void parseBlock(const vector<pair<string, string>> &tokens, int &cur)
{
    if (peek(tokens, cur).first != "{") syntaxError();
    nextToken(tokens, cur);
    while (peek(tokens, cur).first != "}")
        parseStmt(tokens, cur);
    nextToken(tokens, cur);
}

// 赋值语句
void parseAssign(const vector<pair<string, string>> &tokens, int &cur)
{
    auto p = nextToken(tokens, cur);
    if (p.second != "IDENT") syntaxError();
    if (!symIndex.count(p.first)) syntaxError();
    int varIdx = symIndex[p.first];
    if (peek(tokens, cur).first != "=") syntaxError();
    nextToken(tokens, cur);
    Arg rhs = parseFullExpr(tokens, cur);
    if (peek(tokens, cur).first != ";") syntaxError();
    nextToken(tokens, cur);
    // 类型检查
    //if (rhs.type != symTable[varIdx].type) syntaxError();
    GEN("=", rhs, Arg(), {VAR, varIdx, -1, 0, symTable[varIdx].type});
}

// while语句
void parseWhile(const vector<pair<string, string>> &tokens, int &cur)
{
    nextToken(tokens, cur); // while
    int condPos = quads.size();
    JumpChain jc = parseLogicJump(tokens, cur);
    if (peek(tokens, cur).second != "DOSYM") syntaxError();
    nextToken(tokens, cur); // do
    int bodyStart = quads.size();
    parseStmt(tokens, cur); // 循环体
    GEN("j", Arg(), Arg(), {CONST, -1, -1, (double)condPos, "int"});
    backpatch(jc.trueList, bodyStart);
    backpatch(jc.falseList, quads.size());
}

// if语句
void parseIf(const vector<pair<string, string>> &tokens, int &cur)
{
    // nextToken(tokens, cur); // if
    // int curCopy = cur;
    // RelResult cond = parseRelExpr(tokens, curCopy, true);

    nextToken(tokens, cur); // if
    JumpChain jc = parseLogicJump(tokens, cur);
    if (peek(tokens, cur).second != "THENSYM") syntaxError();
    nextToken(tokens, cur); // then
    int thenStart = quads.size();
    parseStmt(tokens, cur);
    backpatch(jc.trueList, thenStart); // 条件成立跳到then
    backpatch(jc.falseList, quads.size()); // 条件不成立跳到then后

    // int jmpIdx = quads.size();
    // int jmpFalseIdx;
    // if (cond.isRel) {
    //     cur = curCopy;
    //     GEN("j" + cond.op, cond.left, cond.right, Arg()); // 目标待回填
    //     jmpFalseIdx = quads.size();
    //     GEN("j", Arg(), Arg(), Arg()); // 无条件跳转，目标待回填
    // } else {
    //     Arg condExpr = parseFullExpr(tokens, cur);
    //     GEN("jnz", condExpr, Arg(), Arg());
    //     jmpFalseIdx = quads.size();
    //     GEN("j", Arg(), Arg(), Arg());
    // }
    // if (peek(tokens, cur).second != "THENSYM") syntaxError();
    // nextToken(tokens, cur); // then
    // parseStmt(tokens, cur);
    // // 回填
    // quads[jmpFalseIdx].result = {CONST, -1, -1, (double)quads.size(), "int"}; // 跳出if
    // quads[jmpIdx].result = {CONST, -1, -1, (double)(jmpFalseIdx + 1), "int"}; // 跳到then
}

// printf语句
void parsePrintf(const vector<pair<string, string>> &tokens, int &cur)
{
    nextToken(tokens, cur); // printf
    if (peek(tokens, cur).first != "(") syntaxError();
    nextToken(tokens, cur);
    vector<Arg> args;
    while (true)
    {
        auto p = peek(tokens, cur);
        if (p.second == "IDENT")
        {
            nextToken(tokens, cur);
            if (!symIndex.count(p.first)) syntaxError();
            int idx = symIndex[p.first];
            args.push_back({VAR, idx, -1, 0, symTable[idx].type});
        }
        else if (p.second == "INT")
        {
            nextToken(tokens, cur);
            args.push_back({CONST, -1, -1, stod(p.first), "int"});
        }
        else if (p.second == "DOUBLE")
        {
            nextToken(tokens, cur);
            args.push_back({CONST, -1, -1, stod(p.first), "double"});
        }
        else break;
        if (peek(tokens, cur).first == ",")
            nextToken(tokens, cur);
        else break;
    }
    if (peek(tokens, cur).first != ")") syntaxError();
    nextToken(tokens, cur);
    if (peek(tokens, cur).first != ";") syntaxError();
    nextToken(tokens, cur);
    for (auto &a : args)
        GEN("W", Arg(), Arg(), a);
}

// scanf语句
void parseScanf(const vector<pair<string, string>> &tokens, int &cur)
{
    nextToken(tokens, cur); // scanf
    if (peek(tokens, cur).first != "(") syntaxError();
    nextToken(tokens, cur);
    vector<Arg> args;
    while (true)
    {
        auto p = peek(tokens, cur);
        if (p.second == "IDENT")
        {
            nextToken(tokens, cur);
            if (!symIndex.count(p.first)) syntaxError();
            int idx = symIndex[p.first];
            args.push_back({VAR, idx, -1, 0, symTable[idx].type});
        }
        else break;
        if (peek(tokens, cur).first == ",")
            nextToken(tokens, cur);
        else break;
    }
    if (peek(tokens, cur).first != ")") syntaxError();
    nextToken(tokens, cur);
    if (peek(tokens, cur).first != ";") syntaxError();
    nextToken(tokens, cur);
    for (auto &a : args)
        GEN("R", Arg(), Arg(), a);
}



// 单条语句
void parseStmt(const vector<pair<string, string>> &tokens, int &cur)
{
    auto p = peek(tokens, cur);
    if (p.second == "IDENT")
        parseAssign(tokens, cur);
    else if (p.second == "WHILESYM")
        parseWhile(tokens, cur);
    else if (p.second == "IFSYM")
        parseIf(tokens, cur);
    else if (p.second == "PRINTFSYM")
        parsePrintf(tokens, cur);
    else if (p.second == "SCANFSYM")
        parseScanf(tokens, cur); // 新增scanf分支
    else if (p.first == "{")
        parseBlock(tokens, cur);
    else if (p.first == ";")
        nextToken(tokens, cur); // 允许空语句
    else
        syntaxError();
}

// 主程序
void parseProgram(const vector<pair<string, string>> &tokens)
{
    int cur = 0;
    while (cur < tokens.size())
    {
        auto p = peek(tokens, cur);
        if (p.second == "INTSYM" || p.second == "DOUBLESYM")
            parseDecl(tokens, cur);
        else
            break;
    }
    // 处理所有剩余语句
    while (cur < tokens.size())
        parseStmt(tokens, cur);
    GEN("End", Arg(), Arg(), Arg());
}

// 四元式输出
void outputResults()
{
    // 符号表输出
    cout << symTable.size() << "\n";
    for (auto &s : symTable)
        cout << s.name << " " << (s.type == "int" ? 0 : 1) << " null " << s.offset << "\n";
    // 临时变量总数
    cout << tempCount +1 << "\n";

    // 跳转目标优化
    vector<Quad> optimizedQuads = quads;
    for (int i = 0; i < optimizedQuads.size(); ++i) {
        Quad& q = optimizedQuads[i];
        if ((q.op == "j" || q.op.substr(0,1) == "j") && q.result.kind == CONST) 
        {
            int target = (int)q.result.constVal;
            int finalTarget = getFinalTarget(target);
            if (finalTarget != target)
                q.result.constVal = finalTarget;
        }
    }

    cout << optimizedQuads.size() << "\n";
    for (int i = 0; i < optimizedQuads.size(); ++i)
    {
        cout << i << ": (" << optimizedQuads[i].op << ",";
        auto printArg = [](Arg a)
        {
            if (a.kind == VAR)
                cout << "TB" << a.varIndex;
            else if (a.kind == TEMP)
                cout << "T" << a.tempId << (a.type == "double" ? "_d" : "_i");
            else if (a.kind == CONST)
            {
                if (a.type == "double")
                    cout << fixed << setprecision(6) << a.constVal;
                else
                    cout << (int)a.constVal;
            }
            else
                cout << "-";
        };
        printArg(optimizedQuads[i].arg1);
        cout << ",";
        printArg(optimizedQuads[i].arg2);
        cout << ",";
        printArg(optimizedQuads[i].result);
        cout << ")\n";
    }
}

int getFinalTarget(int idx) 
{
    // 防止死循环，最多跳100次
    for (int cnt = 0; cnt < 100; ++cnt) 
    {
        if (idx < 0 || idx >= quads.size()) break;
        const Quad& q = quads[idx];
        // 只优化无条件跳转
        if (q.op == "j" && q.result.kind == CONST) 
        {
            int next = (int)q.result.constVal;
            if (next == idx) break; // 防止自跳
            // 如果目标也是跳转语句，继续找
            if (next >= 0 && next < quads.size()  ) 
            {
                idx = next;
                if(quads[next].op == "j"|| quads[next].op.substr(0,1) == "j") 
                    continue; // 继续寻找最终目标
            }
        }
        break;
    }
    return idx;
}

