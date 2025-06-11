#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cctype>

using namespace std;

int main()
{
    string source, line;
    while (getline(cin, line))
        source += line + '\n';

    unordered_map<string, string> keywords = 
    {
        {"int", "INTSYM"},
        {"double", "DOUBLESYM"},
        {"scanf", "SCANFSYM"},
        {"printf", "PRINTFSYM"},
        {"if", "IFSYM"},
        {"then", "THENSYM"},
        {"while", "WHILESYM"},
        {"do", "DOSYM"}
    };

    vector<pair<string, string>> answer; // 存储token和类型

    size_t n = source.size();
    size_t i = 0;

    auto is_ident_char = [](char c) {
        return isalnum(c);
    };

    while (i < n)
    {
        // 跳过空白和注释
        while (i < n)
        {
            if (isspace(source[i]))
                ++i;
            else if (source[i] == '/' && i + 1 < n && source[i + 1] == '/')
            {
                i += 2;
                while (i < n && source[i] != '\n')
                    ++i;
            }
            else if (source[i] == '/' && i + 1 < n && source[i + 1] == '*')
            {
                i += 2;
                bool closed = false;
                while (i + 1 < n)
                {
                    if (source[i] == '*' && source[i + 1] == '/')
                    {
                        i += 2;
                        closed = true;
                        break;
                    }
                    ++i;
                }
                if (!closed) // 注释未闭合，跳到末尾
                    i = n;
            }
            else
                break;
        }
        if (i >= n)
            break;

        char c = source[i];

        // 识别标识符或关键字
        if (isalpha(c))
        {
            size_t start = i++;
            while (i < n && is_ident_char(source[i]))
                ++i;
            string token = source.substr(start, i - start);
            if (keywords.count(token))
                answer.emplace_back(token, keywords[token]);
            else
                answer.emplace_back(token, "IDENT");
            continue;
        }

        // 识别数字（整数或浮点数）
        if (isdigit(c) || (c == '.' && i + 1 < n && isdigit(source[i + 1])))
        {
            size_t start = i;
            bool has_dot = false;
            if (source[i] == '.')
            {
                has_dot = true;
                ++i;
            }
            // 连续读取数字和最多一个小数点
            while (i < n)
            {
                if (isdigit(source[i]))
                    ++i;
                else if (source[i] == '.')
                {
                    if (has_dot)
                    {
                        ++i;
                        break; // 第二个小数点，结束数字
                    }
                    has_dot = true;
                    ++i;
                }
                else
                    break;
            }

            string token = source.substr(start, i - start);

            // 检查多余小数点
            int dot_count = 0;
            for (char ch : token)
                if (ch == '.')
                    ++dot_count;
            if (dot_count > 1)
            {
                cout << "Malformed number: More than one decimal point in a floating point number.\n";
                return 0;
            }

            // 小数点不能在开头或结尾
            if (token.front() == '.' || token.back() == '.')
            {
                cout << "Malformed number: Decimal point at the beginning or end of a floating point number.\n";
                return 0;
            }

            // 整数部分不能有前导零（除了单个0）
            size_t dot_pos = token.find('.');
            string int_part = dot_pos == string::npos ? token : token.substr(0, dot_pos);
            if (int_part.size() > 1 && int_part[0] == '0')
            {
                cout << "Malformed number: Leading zeros in an integer.\n";
                return 0;
            }

            answer.emplace_back(token, has_dot ? "DOUBLE" : "INT");
            continue;
        }

        // 识别双字符运算符
        if (i + 1 < n)
        {
            string two = source.substr(i, 2);
            if (two == "==" || two == ">=" || two == "<=" || two == "!=")
            {
                answer.emplace_back(two, "RO");
                i += 2;
                continue;
            }
            if (two == "&&" || two == "||")
            {
                answer.emplace_back(two, "LO");
                i += 2;
                continue;
            }
        }

        // 单字符运算符及分隔符
        switch (c)
        {
        case '=': answer.emplace_back("=", "AO"); break;
        case '>': answer.emplace_back(">", "RO"); break;
        case '<': answer.emplace_back("<", "RO"); break;
        case '!': answer.emplace_back("!", "LO"); break;
        case '+': answer.emplace_back("+", "PLUS"); break;
        case '-': answer.emplace_back("-", "MINUS"); break;
        case '*': answer.emplace_back("*", "TIMES"); break;
        case '/': answer.emplace_back("/", "DIVISION"); break;
        case ',': answer.emplace_back(",", "COMMA"); break;
        case '(': answer.emplace_back("(", "BRACE"); break;
        case ')': answer.emplace_back(")", "BRACE"); break;
        case '{': answer.emplace_back("{", "BRACE"); break;
        case '}': answer.emplace_back("}", "BRACE"); break;
        case ';': answer.emplace_back(";", "SEMICOLON"); break;

        // 以下字符单独出现即为错误
        case '&':
        case '|':
        case '.':
        default:
            cout << "Unrecognizable characters.\n";
            return 0;
        }
        ++i;
    }

    // 无错误则输出所有token
    for (auto &p : answer)
        cout << p.first << " " << p.second << "\n";

    return 0;
}
