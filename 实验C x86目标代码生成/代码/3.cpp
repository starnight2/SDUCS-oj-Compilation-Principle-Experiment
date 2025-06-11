#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <set>
#include <sstream>
#include <algorithm>
#include <map>
#include <functional>
#include <bits/stdc++.h>
using namespace std;

// 四元式元素
struct QuadElement
{
    string val;
    int nextUse = -1;
    bool live = false;
    QuadElement(string val) : val(val) {}
    QuadElement() {}
};

// 四元式
struct Quad
{
    QuadElement op;
    QuadElement arg1;
    QuadElement arg2;
    QuadElement result;
    Quad(QuadElement op, QuadElement arg1, QuadElement arg2, QuadElement result) : op(op), arg1(arg1), arg2(arg2), result(result) {}
    Quad() {}
};

// 符号表项
struct SymbolTableEntry
{
    string name = "", type = "", result = "";
    int offset = -1;
    int S_nextUse = -1;
    bool isTemp = true;
    bool live = false;
    SymbolTableEntry() {}
    SymbolTableEntry(bool isTemp) : isTemp(isTemp) {}
};

// 地址描述符
struct AddressDescriptor
{
    set<string> registerSet;
    set<string> memory;
};

vector<vector<int>> basicBlocks;           //  基本块列表
vector<set<string>> liveVars;              // 活跃变量集合
vector<string> registers;                  // 寄存器列表
vector<Quad> quads;                        // 四元式列表
vector<int> labelFlags;                    // 标签标志，记录每个四元式是否为标签
map<string, SymbolTableEntry> symbolTable; // 符号表
map<string, QuadElement> historyInfo;      // 历史信息，用于记录变量的使用情况
map<string, AddressDescriptor> addrDesc;   // 地址描述符，记录变量在寄存器或内存中的状态
map<string, set<string>> regDesc;          // 寄存器描述符，记录寄存器中存储的变量

int initOffset = 0; // 初始偏移量，用于栈帧分配
int offset = 0;     // 当前偏移量，用于栈帧分配

// 操作符到汇编指令的映射
map<string, string> opMap =
{
        // 算术运算
        {"+", "add"},  // 加法
        {"-", "sub"},  // 减法
        {"*", "mul"},  // 乘法
        {"/", "div"},  // 除法
        // 逻辑运算
        {"&&", "and"}, // 逻辑与
        {"||", "or"},  // 逻辑或
        {"!", "not"},  // 逻辑非
        // 比较运算
        {">", "cmp"},  // 大于
        {">=", "cmp"}, // 大于等于
        {"==", "cmp"}, // 等于
        {"<", "cmp"},  // 小于
        {"<=", "cmp"}, // 小于等于
        {"!=", "cmp"}, // 不等于
        // 赋值和移动
        {"=", "mov"},  // 赋值
};

// 比较指令映射
map<string, string> cmpMap =
{
        // 比较操作
        {">", "setg"},   // 设置大于
        {">=", "setge"}, // 设置大于等于
        {"==", "sete"},  // 设置等于
        {"<", "setl"},   // 设置小于
        {"<=", "setle"}, // 设置小于等于
        {"!=", "setne"}  // 设置不等于
};

// 条件跳转映射
map<string, string> JumpMap =
{
        // 条件跳转
        {"j>", "jg"},   // 大于跳转
        {"j>=", "jge"}, // 大于等于跳转
        {"j==", "je"},  // 等于跳转
        {"j<", "jl"},   // 小于跳转
        {"j<=", "jle"}, // 小于等于跳转
        {"j!=", "jne"}  // 不等于跳转
};

// 判断是否为逻辑运算
bool isLogicOp(const Quad &q)
{
    if (q.arg2.val == "-")
        return false;
    char op = q.op.val[0];
    return op == '+' || op == '-' || op == '*' || op == '/' ||
           op == '=' || op == '<' || op == '>' || op == '!' ||
           op == '&' || op == '|';
}

// 判断是否为读写指令
bool isRW(const Quad &q)
{
    return q.op.val == "W" || q.op.val == "R";
}

// 判断是否为一元/赋值
bool isUnaryOrAssign(const Quad &q)
{
    return (q.op.val == "=" || q.op.val == "!") && q.arg2.val == "-";
}

// 判断是否为数字
bool isNumber(const string &item)
{
    return !item.empty() && item[0] >= '0' && item[0] <= '9';
}

// 判断是否为无条件跳转
bool isUncondJump(const Quad &q)
{
    return q.op.val == "j";
}

// 判断是否为条件跳转
bool isCondJump(const Quad &q)
{
    return q.op.val[0] == 'j' && q.op.val != "j" && q.op.val != "jnz";
};

// 判断是否为jnz跳转
bool isJnz(const Quad &q)
{
    return q.op.val == "jnz";
}

// 判断是否为结束
bool isEnd(const Quad &q)
{
    return q.op.val == "End";
}

// 辅助函数：处理变量在向前扫描中的初始化逻辑
void markLive(const string &name, set<string> &liveSet)
{
    if (name.empty() || name[0] != 'T')
        return;

    symbolTable[name].S_nextUse = -1;

    if (!symbolTable[name].isTemp)
    {
        symbolTable[name].live = true;
        liveSet.insert(name); // set 不会插入重复元素，无需再 find 检查
    }
}

// 更新某个变量的 nextUse 和活跃状态
void updateNextUse(QuadElement &elem, const string &name, int quadIndex, bool isDefined)
{
    if (name.empty() || name[0] != 'T')
        return; // 只处理临时变量 T 开头的

    // 赋值目标（如 result）是定义位置：kill 原信息
    if (isDefined)
    {
        elem.nextUse = symbolTable[name].S_nextUse;
        elem.live = symbolTable[name].live;

        symbolTable[name].S_nextUse = -1;
        symbolTable[name].live = false;
    }
    else // 使用变量（如 arg1、arg2）：读取并更新符号表
    {
        elem.nextUse = symbolTable[name].S_nextUse;
        elem.live = symbolTable[name].live;

        symbolTable[name].S_nextUse = quadIndex;
        symbolTable[name].live = true;
    }
}

// 分析活跃变量
set<string> analyzeLiveness(vector<int> &block)
{
    set<string> liveSet;
    // 向前扫描初始化
    for (int index : block)
    {
        markLive(quads[index].arg1.val, liveSet);
        markLive(quads[index].arg2.val, liveSet);
        markLive(quads[index].result.val, liveSet);
    }
    // 反向扫描：更新每个元素的 nextUse 与 live 信息
    for (int i = int(block.size()) - 1; i >= 0; i--)
    {
        int idx = block[i];
        Quad &q = quads[idx];

        updateNextUse(q.result, q.result.val, idx, true);
        updateNextUse(q.arg1, q.arg1.val, idx, false);
        updateNextUse(q.arg2, q.arg2.val, idx, false);
    }
    return liveSet;
}

// 根据变量名获取在栈上的地址表示（"[ebp-偏移]"）
// 若传入的 var 已经是 "[...]" 形式，直接返回原串
string getMemoryAddress(string var)
{
    // 如果已经是内存地址格式，直接返回
    if (var[0] == '[')
        return var;

    // 如果符号表中已存在该变量且已分配过偏移量，直接复用
    if (symbolTable.find(var) != symbolTable.end() && symbolTable[var].offset != -1)
        return "[ebp-" + to_string(symbolTable[var].offset) + "]";

    // 新变量首次分配：根据变量类型（后缀 'd' 或 'i'）分配不同大小的空间
    if (var.back() == 'd')
    {
        // 'd' 类型（假设 double 或 8 字节），先分配当前 offset + 8
        symbolTable[var].offset = offset + 8;
        offset += 8; // 更新全局偏移，避免重叠
    }
    else if (var.back() == 'i')
    {
        // 'i' 类型（假设 int 或 4 字节），先分配当前 offset + 4
        symbolTable[var].offset = offset + 4;
        offset += 4; // 更新全局偏移
    }

    // 返回栈帧中该变量的地址
    return "[ebp-" + to_string(symbolTable[var].offset) + "]";
}

// 寄存器分配选择替换寄存器
string selectSpillRegister(vector<string> &cand_reg, int spill_reg)
{
    string selected;
    int latestUse = INT32_MIN;

    // 内联 getBasicBlockIndex
    int blockIndex = -1;
    for (int i = 0; i < basicBlocks.size(); ++i)
    {
        if (find(basicBlocks[i].begin(), basicBlocks[i].end(), spill_reg) != basicBlocks[i].end())
        {
            blockIndex = i;
            break;
        }
    }

    if (blockIndex == -1)
        return ""; // 未找到基本块，返回空字符串

    for (auto &reg : cand_reg)
    {
        bool found = false;
        for (int i = spill_reg + 1; i <= basicBlocks[blockIndex].back(); i++)
        {
            if (regDesc[reg].find(quads[i].arg1.val) != regDesc[reg].end() ||
                regDesc[reg].find(quads[i].arg2.val) != regDesc[reg].end())
            {
                found = true;
                if (i > latestUse)
                {
                    latestUse = i;
                    selected = reg;
                }
                break;
            }
        }

        if (!found)
        {
            selected = reg;
            break;
        }
    }

    return selected;
}

// 分配目标寄存器
string allocateTargetRegister(int quadIndex)
{
    Quad quad = quads[quadIndex];
    string arg1 = quad.arg1.val;
    string arg2 = quad.arg2.val;
    string result = quad.result.val;
    if (!isNumber(arg1) && arg1 != "-")
        for (auto &reg : addrDesc[arg1].registerSet)
            if (regDesc[reg] == set<string>{arg1} && (arg1 == result || !quad.arg1.live))
                return reg;

    for (auto &reg : registers)
        if (regDesc[reg].empty())
            return reg;

    vector<string> usedReg;
    for (auto &reg : registers)
        if (!regDesc[reg].empty())
            usedReg.push_back(reg);

    if (usedReg.empty())
        usedReg = registers;

    string selectedReg;

    bool allInMemory = true;
    for (auto &reg : usedReg)
    {
        allInMemory = true;
        for (auto &var : regDesc[reg])
        {
            if (addrDesc[var].memory.find(var) == addrDesc[var].memory.end())
            {
                allInMemory = false;
                break;
            }
        }
        if (allInMemory)
        {
            selectedReg = reg;
            break;
        }
    }
    if (!allInMemory)
        selectedReg = selectSpillRegister(usedReg, quadIndex);

    for (auto &var : regDesc[selectedReg])
    {
        if (addrDesc[var].memory.find(var) == addrDesc[var].memory.end() && var != result)
            cout << "mov " << getMemoryAddress(var) << ", " << selectedReg << "\n";
        if (var == arg1 || (var == arg2 && regDesc[selectedReg].find(arg1) != regDesc[selectedReg].end()))
        {
            addrDesc[var].memory = {var};
            addrDesc[var].registerSet = {selectedReg};
        }
        else
        {
            addrDesc[var].memory = {var};
            addrDesc[var].registerSet = {};
        }
    }
    regDesc[selectedReg].clear();
    return selectedReg;
}

// 活跃变量处理
void handleDeadVariable(string var, set<string> &liveVars)
{
    if (liveVars.find(var) == liveVars.end())
    {
        for (auto &reg : addrDesc[var].registerSet)
            regDesc[reg].erase(var);
        addrDesc[var].registerSet.clear();
    }
}

// 处理一元运算或赋值四元式
void generateUnaryOrAssignAssembly(int quadIndex, int blockIndex)
{
    Quad quad = quads[quadIndex];
    string arg1 = quad.arg1.val;
    string result = quad.result.val;
    // 分配目标寄存器
    auto targetReg = allocateTargetRegister(quadIndex);
    string src;

    // 如果 arg1 是数字，直接使用它作为源
    if (isNumber(arg1))
    {
        // 直接把立即数加载到目标寄存器
        src = arg1;
        cout << "mov " << targetReg << ", " << src << "\n";
    }
    else
    {
        // 如果目标寄存器中当前没有保存 arg1 的值，需要先把 arg1 加载到目标寄存器
        if (regDesc[targetReg].find(arg1) == regDesc[targetReg].end())
        {
            // 检查 arg1 当前是否已在某个寄存器中,如果不在寄存器，则从内存加载
            if (addrDesc[arg1].registerSet.empty())
                src = getMemoryAddress(arg1);
            else
                src = *addrDesc[arg1].registerSet.begin();
            cout << "mov " << targetReg << ", " << src << "\n";
        }

        // 如果操作不是简单赋值，则输出对应的算术/逻辑指令
        if (quad.op.val != "=")
            cout << opMap[quad.op.val] << " " << targetReg << "\n";

        // 对于非立即数的源操作数，判定其是否“死掉”以便释放寄存器
        if (!isNumber(arg1))
            handleDeadVariable(arg1, liveVars[blockIndex]);
    }

    regDesc[targetReg].insert(result);              // 更新寄存器描述符：目标寄存器现在存放 result
    historyInfo[result] = quad.result;              // 记录 result 的最新四元式信息，用于历史查询
    addrDesc[result].registerSet.insert(targetReg); // 更新地址描述符：result 现在也在目标寄存器里
    addrDesc[result].memory.clear();                // 并清除其内存中保存的位置（已在寄存器中）
}

// 取得操作数的源字符串：立即数、寄存器或内存
string getOperandValue(const string &arg)
{
    if (arg == "-" || isNumber(arg))
        return arg;
    if (addrDesc[arg].registerSet.empty())
        return getMemoryAddress(arg);
    return *addrDesc[arg].registerSet.begin();
}

// 输出算术/逻辑指令，并处理 cmp 的特殊情况
void emitBinaryOp(const string &opKey, const string &destReg, const string &src)
{
    cout << opMap[opKey] << " " << destReg << ", " << src << "\n";
    if (opMap[opKey] == "cmp")
        cout << cmpMap[opKey] << " " << destReg << "\n";
}

// 更新结果变量在寄存器和符号表中的状态
void recordResult(const string &result, const string &targetReg, int quadIndex)
{
    // 更新 regDesc：目标寄存器现在只保存 result
    regDesc[targetReg].clear();
    regDesc[targetReg].insert(result);

    // 记录新的四元式历史信息
    historyInfo[result] = quads[quadIndex].result;

    // 更新地址描述符：result 只在 targetReg 中
    addrDesc[result].registerSet.clear();
    addrDesc[result].registerSet.insert(targetReg);
    addrDesc[result].memory.clear();
}

// 处理一个二元运算四元式的汇编生成
void generateBinaryAssembly(int quadIndex, int blockIndex)
{
    const Quad &quad = quads[quadIndex];
    string arg1 = quad.arg1.val;
    string arg2 = quad.arg2.val;
    string result = quad.result.val;
    string targetReg = allocateTargetRegister(quadIndex);

    // 1. 准备两个源操作数
    string src1 = getOperandValue(arg1);
    string src2 = getOperandValue(arg2);

    // 2. 生成指令
    if (src1 == targetReg)
    {
        // 目标寄存器已含 arg1，直接用 destReg 操作 src2
        emitBinaryOp(quad.op.val, targetReg, src2);
        addrDesc[arg1].registerSet.erase(targetReg);
    }
    else
    {
        // 先 mov src1 → destReg，再执行 op
        cout << "mov " << targetReg << ", " << src1 << "\n";
        emitBinaryOp(quad.op.val, targetReg, src2);
    }

    // 如果 src2 恰好是目标寄存器，且不是立即数，要清理旧绑定
    if (src2 == targetReg && !isNumber(arg2))
        addrDesc[arg2].registerSet.erase(targetReg);

    // 3. 记录结果到寄存器和符号表
    recordResult(result, targetReg, quadIndex);

    // 4. 处理死变量：arg1, arg2 如果不再被使用，则可释放
    if (!isNumber(arg1))
        handleDeadVariable(arg1, liveVars[blockIndex]);
    if (!isNumber(arg2))
        handleDeadVariable(arg2, liveVars[blockIndex]);
}

// 标记基本块入口点（入口点既是函数入口、分支目标，也包括条件跳转后的下一条指令、读／写内存指令）
// 返回一个和 quads 等长的布尔数组，true 表示该下标是一个基本块的入口
vector<bool> markBlockEntries()
{
    int n = quads.size();
    vector<bool> entry(n, false);

    // 第 0 条总是入口
    entry[0] = true;

    for (int i = 0; i < n; ++i)
    {
        const auto &q = quads[i];

        // 条件跳转：目标和下一条都是入口
        if (isCondJump(q) || isJnz(q))
        {
            int target = stoi(q.result.val);
            entry[target] = true;
            if (i + 1 < n)
                entry[i + 1] = true;

            // 标记标签已被引用
            if (labelFlags[target] == 0)
                labelFlags[target] = 1;
        }
        // 无条件跳转：目标是入口
        else if (isUncondJump(q))
        {
            int target = stoi(q.result.val);
            entry[target] = true;
            if (labelFlags[target] == 0)
                labelFlags[target] = 1;
        }
        // 遇到 End：最后一条也算一个新块的入口
        else if (q.op.val == "End")
            entry[n - 1] = true;
        // 读/写内存：自成一个基本块
        else if (isRW(q))
            entry[i] = true;
    }

    return entry;
}

// 2. 根据入口点构造基本块列表
set<vector<int>> buildBlocks(const vector<bool> &entry)
{
    int n = entry.size();
    set<vector<int>> blocks;

    int i = 0;
    while (i < n)
    {
        if (!entry[i])
        {
            ++i;
            continue;
        }

        // 从 i 开始向后找下一个入口或跳点结束当前块
        int start = i, end = i;
        for (int j = i + 1; j < n; ++j)
        {
            if (entry[j])
            {
                end = j - 1;
                break;
            }
            const auto &q = quads[j];
            // 如果碰到跳转、ret、End，也算块尾
            if (q.op.val[0] == 'j' || q.op.val == "ret" || q.op.val == "End")
            {
                end = j;
                break;
            }
        }

        // 收集从 start 到 end 的指令索引
        vector<int> blk(end - start + 1);
        iota(blk.begin(), blk.end(), start);
        blocks.insert(blk);

        // 下一次从 end+1 或第一个下一个入口开始
        i = end + 1;
    }

    return blocks;
}

// 划分基本块写回 global
void divideBasicBlocks()
{
    auto entry = markBlockEntries();
    auto blocks = buildBlocks(entry);

    // 转为 vector 存入全局 basicBlocks
    basicBlocks.assign(blocks.begin(), blocks.end());
}

// 打印基本块标签（如果需要）
void emitBlockLabel(int blockIndex)
{
    int label = basicBlocks[blockIndex].front();
    if (labelFlags[label] == 1)
        cout << "?" << label << ":\n";
}

// 处理块中间的四元式：算术/逻辑、一元/赋值、读写
void emitBlockQuads(int blockIndex)
{
    for (int qi : basicBlocks[blockIndex])
    {
        const Quad &q = quads[qi];

        if (isLogicOp(q))
            generateBinaryAssembly(qi, blockIndex);
        else if (isUnaryOrAssign(q))
            generateUnaryOrAssignAssembly(qi, blockIndex);
        else if (isRW(q))
        {
            // 读写跳转
            if (q.op.val != "W")
                cout << "jmp ?read";
            else
                cout << "jmp ?write";

            cout << "(" << getMemoryAddress(q.result.val) << ")\n";

            if (!isNumber(q.result.val))
                handleDeadVariable(q.result.val, liveVars[blockIndex]);
        }
    }
}

// 将活跃变量写回内存
void emitSpillLiveVars(int blockIndex)
{
    for (auto &var : liveVars[blockIndex])
    {
        // 如果还没写回内存，但寄存器里有值，就 spill
        if (addrDesc[var].memory.find(var) == addrDesc[var].memory.end() && !addrDesc[var].registerSet.empty() && historyInfo[var].live)
            for (auto &reg : addrDesc[var].registerSet)
                cout << "mov " << getMemoryAddress(var) << ", " << reg << "\n";
    }
}

// 处理基本块最后一条四元式：跳转、比较、结束
void emitBlockTerminator(int blockIndex)
{
    const Quad &last = quads[basicBlocks[blockIndex].back()];

    if (isUncondJump(last))
        cout << "jmp ?" << last.result.val << "\n";
    else if (isCondJump(last))
    {
        // 有条件跳转
        string s1 = !addrDesc[last.arg1.val].registerSet.empty()? *addrDesc[last.arg1.val].registerSet.begin(): last.arg1.val;
        string s2 = !addrDesc[last.arg2.val].registerSet.empty()? *addrDesc[last.arg2.val].registerSet.begin(): getMemoryAddress(last.arg2.val);

        if (s1 == last.arg1.val)
        {
            s1 = allocateTargetRegister(basicBlocks[blockIndex].back());
            cout << "mov " << s1 << ", " << getMemoryAddress(last.arg1.val) << "\n";
        }
        cout << "cmp " << s1 << ", " << s2 << "\n";
        cout << JumpMap[last.op.val] << " ?" << last.result.val << "\n";
    }
    else if (isJnz(last))
    {
        // 非零跳转
        string s1 = !addrDesc[last.arg1.val].registerSet.empty() ? *addrDesc[last.arg1.val].registerSet.begin(): last.arg1.val;

        if (s1 == last.arg1.val)
        {
            s1 = allocateTargetRegister(basicBlocks[blockIndex].back());
            cout << "mov " << s1 << ", " << getMemoryAddress(last.arg1.val) << "\n";
        }
        cout << "cmp " << s1 << ", 0\n";
        cout << "jne" << " ?" << last.result.val << "\n";
    }
    else if (isEnd(last))
        cout << "halt\n";
    
}

// 清理寄存器和地址描述符，准备下一个基本块
void clearDescriptors()
{
    regDesc.clear();
    addrDesc.clear();
}

// 汇编代码生成主过程
void generateAssembly()
{
    for (int b = 0; b < basicBlocks.size(); ++b)
    {
        emitBlockLabel(b);      // 输出基本块标签（如果需要）
        emitBlockQuads(b);      // 输出块内四元式对应的汇编指令
        emitSpillLiveVars(b);   // 将活跃变量写回内存
        emitBlockTerminator(b); // 处理块的最后一条四元式（跳转、比较、结束）
        clearDescriptors();     // 清理寄存器和地址描述符，准备下一个块
    }
}




// 初始化可用寄存器列表
void initRegisters()
{
    registers = {"R0", "R1", "R2"};
}

// 读取符号表部分，返回 false 时表示 Syntax Error 已经处理并退出
bool readSymbolTable()
{
    string line;
    getline(cin, line);
    if (line == "Syntax Error")
    {
        cout << "halt\n";
        return false;
    }

    int symbolCount = stoi(line);
    int maxOffset = 0;

    for (int i = 0; i < symbolCount; ++i)
    {
        getline(cin, line);
        stringstream ss(line);
        SymbolTableEntry entry(false);
        ss >> entry.name >> entry.type >> entry.result >> entry.offset;

        maxOffset = max(maxOffset, entry.offset);
        symbolTable["TB" + to_string(i)] = entry;
    }
    initOffset = maxOffset;

    // 读一行空行（符号表与四元式之间的空隔行）
    getline(cin, line);
    return true;
}

// 读取四元式序列
void readQuads()
{
    string line;
    // 读取 quadCount
    getline(cin, line);
    int quadCount = stoi(line);

    quads.resize(quadCount);
    for (int i = 0; i < quadCount; ++i)
    {
        getline(cin, line);
        // 剥离前缀 “<idx>: ( … )”
        line = line.substr(line.find(':') + 2);
        line = line.substr(1, line.find(')') - 1);

        // 按逗号拆成 4 项
        stringstream ss(line);
        vector<string> tok(4);
        for (int j = 0; j < 4; ++j)
            getline(ss, tok[j], ',');

        quads[i].op     = tok[0];
        quads[i].arg1   = tok[1];
        quads[i].arg2   = tok[2];
        quads[i].result = tok[3];
    }
}

// 初始化分析前所需数据结构
void initializeAnalysis()
{
    offset     = initOffset;
    labelFlags.assign(quads.size(), 0);

    divideBasicBlocks();
    liveVars.resize(basicBlocks.size());
}

// 对每个基本块做活跃变量分析
void LivenessAnalysis()
{
    for (int i = 0; i < basicBlocks.size(); ++i)
    {
        liveVars[i] = analyzeLiveness(basicBlocks[i]);
    }
}


int main()
{
    initRegisters();        // 初始化寄存器列表

    // 读取并初始化符号表，遇到 “Syntax Error” 则直接 halt
    if (!readSymbolTable())
        return 0;

    readQuads();            // 读取四元式流
    initializeAnalysis();   // 分析前准备：偏移初始化、标签数组 resize
    LivenessAnalysis();     // 进行活跃变量分析
    generateAssembly();     // 生成最终汇编并输出

    return 0;
}