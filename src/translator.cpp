#ifndef VMTRANSL_CPP
#define VMTRANSL_CPP

#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <regex>

#include <iostream>

static const std::regex PUSH_CONST_VM(R"(^\s*push\s+constant\s+(\d+)\s*$)");
static const std::string PUSH_CONST_ASM[] = {"@", "\nD=A\n@SP\nA=M\nM=D\n@SP\nM=M+1\n"};

static const std::regex PUSH_SGMT_VM(R"(^\s*push\s+(local|argument|this|that)\s+(\d+)\s*$)");
static const std::string PUSH_SGMT_ASM[] = {
    "@", "\nD=A\n@",
    "\nA=M+D\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n"};

static const std::regex PUSH_TMP_VM(R"(^\s*push\s+temp\s+(\d+)\s*$)");
static const std::string PUSH_TMP_ASM[] = {"@", "\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n"};

static const std::regex PUSH_STATIC_VM(R"(^\s*push\s+static\s+(\d+)\s*$)");
static const std::string PUSH_STATIC_ASM[] = {"@", "\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n"};

static const std::regex PUSH_PTR_VM(R"(^\s*push\s+pointer\s+(\d+)\s*$)");
static const std::string PUSH_PTR_ASM[] = {"@", "\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n"};

static const std::regex POP_SGMT_VM(R"(^\s*pop\s+(local|argument|this|that)\s+(\d+)\s*$)");
static const std::string POP_SGMT_ASM[] = {
    "@SP\nAM=M-1\nD=M\n@R13\nM=D\n@", "\nD=A\n@",
    "\nD=M+D\n@R14\nM=D\n@R13\nD=M\n@R14\nA=M\nM=D\n"};

static const std::regex POP_TMP_VM(R"(^\s*pop\s+temp\s+(\d+)\s*$)");
static const std::string POP_TMP_ASM[] = {"@SP\nAM=M-1\nD=M\n@", "\nM=D\n"};

static const std::regex POP_STATIC_VM(R"(^\s*pop\s+static\s+(\d+)\s*$)");
static const std::string POP_STATIC_ASM[] = {"@SP\nAM=M-1\nD=M\n@", "\nM=D\n"};

static const std::regex POP_PTR_VM(R"(^\s*pop\s+pointer\s+(\d+)\s*$)");
static const std::string POP_PTR_ASM[] = {"@SP\nAM=M-1\nD=M\n@", "\nM=D\n"};

static const std::regex BINARY_OP_VM(R"(^\s*add|sub|and|or\s*$)");
static const std::string BINARY_OP_ASM[] = {"@SP\nAM=M-1\nD=M\nA=A-1\nM=M", "D\n"};

static const std::regex UNARY_OP_VM(R"(^\s*neg|not\s*$)");
static const std::string UNARY_OP_ASM[] = {"@SP\nA=M-1\nM=", "M\n"};

static const std::regex BOOL_OP_VM(R"(^\s*eq|gt|lt\s*$)");
static const std::string BOOL_OP_ASM[] = {
    "@SP\nAM=M-1\nD=M\nA=A-1\nD=M-D\n@COND", "\nD;",
    "\n@SP\nA=M-1\nM=0\n@END", "\n0;JMP\n(COND",
    ")\n@SP\nA=M-1\nM=-1\n(END", ")\n"};

static const std::regex LABEL_VM(R"(^\s*label\s+(.*)\s*$)");
static const std::string LABEL_ASM[] = {"(", ")\n"};

static const std::regex IF_GOTO_VM(R"(^\s*if-goto\s+(.*)\s*$)");
static const std::string IF_GOTO_ASM[] = {"@SP\nAM=M-1\nD=M\n@", "\nD;JNE"};

static const std::regex GOTO_VM(R"(^\s*goto\s+(.*)\s*$)");
static const std::string GOTO_ASM[] = {"@", "\n0;JMP\n"};

static const std::regex CALL_VM(R"(^\s*call\s+(.*)\s+(\d+)\s*$)");

static const std::regex FUNC_VM(R"(^\s*function\s+(.*)\s+(\d+)\s*$)");

static const std::regex RET_VM(R"(^\s*return\s*$)");

static int LABEL_COUNT = 0;

static const std::unordered_map<std::string, std::string> SGMT_MAP =
    {
        {"local",    "LCL"},
        {"argument", "ARG"},
        {"this",     "THIS"},
        {"that",     "THAT"},
};
static const std::unordered_map<std::string, std::string> OPERATOR_MAP =
    {
        {"add", "+"},
        {"sub", "-"},
        {"and", "&"},
        {"or",  "|"},
        {"neg", "-"},
        {"not", "!"},
};
static const std::unordered_map<std::string, std::string> JUMP_MAP =
    {
        {"eq", "JEQ"},
        {"gt", "JGT"},
        {"lt", "JLT"},
};

class Translator
{
public:
    Translator(const std::string &input, const std::string &name)
    {
        input_stream << input;
        class_name = name;
    }

    std::string translated()
    {
        std::string line;
        std::smatch regex_match_results;

        while (std::getline(input_stream, line))
        {
            // add comments to output
            output_stream << "// " << line << '\n';

            // handle push command
            if (std::regex_match(line, regex_match_results, PUSH_CONST_VM))
            {
                std::string arg = regex_match_results[1];
                write_push_const(arg);
                continue;
            }
            if (std::regex_match(line, regex_match_results, PUSH_SGMT_VM))
            {
                auto seg = regex_match_results[1].str();
                auto arg = std::stoi(regex_match_results[2].str());
                write_push_seg(arg, SGMT_MAP.at(seg));
                continue;
            }
            if (std::regex_match(line, regex_match_results, PUSH_TMP_VM))
            {
                int arg = std::stoi(regex_match_results[1].str());
                write_push_temp(arg);
                continue;
            }
            if (std::regex_match(line, regex_match_results, PUSH_STATIC_VM))
            {
                int arg = std::stoi(regex_match_results[1].str());
                write_push_static(arg);
                continue;
            }
            if (std::regex_match(line, regex_match_results, PUSH_PTR_VM))
            {
                int arg = std::stoi(regex_match_results[1].str());
                write_push_pointer(arg);
                continue;
            }

            // handle pop command
            if (std::regex_match(line, regex_match_results, POP_SGMT_VM))
            {
                auto seg = regex_match_results[1].str();
                auto arg = std::stoi(regex_match_results[2].str());
                write_pop_seg(SGMT_MAP.at(seg), arg);
                continue;
            }
            if (std::regex_match(line, regex_match_results, POP_TMP_VM))
            {
                int arg = std::stoi(regex_match_results[1].str());
                write_pop_temp(arg);
                continue;
            }
            if (std::regex_match(line, regex_match_results, POP_STATIC_VM))
            {
                int arg = std::stoi(regex_match_results[1].str());
                write_pop_static(arg);
                continue;
            }
            if (std::regex_match(line, regex_match_results, POP_PTR_VM))
            {
                int arg = std::stoi(regex_match_results[1].str());
                write_pop_pointer(arg);
                continue;
            }

            // handle binary operation
            if (std::regex_match(line, regex_match_results, BINARY_OP_VM))
            {
                write_binary_op(OPERATOR_MAP.at(line));
                continue;
            }

            // handle unary operation
            if (std::regex_match(line, regex_match_results, UNARY_OP_VM))
            {
                write_unary_op(OPERATOR_MAP.at(line));
                continue;
            }

            // handle boolean commands
            if (std::regex_match(line, regex_match_results, BOOL_OP_VM))
            {
                write_boolean_op(JUMP_MAP.at(line));
                continue;
            }

            // handle branching
            if (std::regex_match(line, regex_match_results, LABEL_VM))
            {
                std::string label = regex_match_results[1];
                write_label(class_name + '.' + label);
                continue;
            }
            if (std::regex_match(line, regex_match_results, IF_GOTO_VM))
            {
                std::string label = regex_match_results[1];
                write_if_goto(class_name + '.' + label);
                continue;
            }
            if (std::regex_match(line, regex_match_results, GOTO_VM))
            {
                std::string label = regex_match_results[1];
                write_goto(class_name + '.' + label);
                continue;
            }

            // handle functions
            if (std::regex_match(line, regex_match_results, CALL_VM))
            {
                std::string function = regex_match_results[1];
                int nArgs = std::stoi(regex_match_results[2]);
                write_call(function, nArgs);
                continue;
            }
            if (std::regex_match(line, regex_match_results, FUNC_VM))
            {
                std::string function = regex_match_results[1];
                int nVars = std::stoi(regex_match_results[2]);
                write_function(function, nVars);
                continue;
            }
            if (std::regex_match(line, regex_match_results, RET_VM))
            {
                write_return();
                continue;
            }
        }

        return output_stream.str();
    }

private:
    std::stringstream input_stream;
    std::stringstream output_stream;
    std::string class_name;

    std::stringstream &write_push_const(std::string i)
    {
        output_stream
            << PUSH_CONST_ASM[0]
            << i
            << PUSH_CONST_ASM[1];
        return output_stream;
    }

    std::stringstream &write_push_seg(int i, std::string seg)
    {
        output_stream
            << PUSH_SGMT_ASM[0]
            << std::to_string(i)
            << PUSH_SGMT_ASM[1]
            << seg
            << PUSH_SGMT_ASM[2];
        return output_stream;
    }

    std::stringstream &write_push_temp(int i)
    {
        output_stream
            << PUSH_TMP_ASM[0]
            << std::to_string(i+5)
            << PUSH_TMP_ASM[1];
        return output_stream;
    }

    std::stringstream &write_push_static(int i)
    {
        output_stream
            << PUSH_STATIC_ASM[0]
            << class_name << '.' << std::to_string(i)
            << PUSH_STATIC_ASM[1];
        return output_stream;
    }

    std::stringstream &write_push_pointer(int i)
    {
        output_stream
            << PUSH_PTR_ASM[0]
            << std::to_string(i+3)
            << PUSH_PTR_ASM[1];
        return output_stream;
    }

    std::stringstream &write_pop_seg(std::string seg, int i)
    {
        output_stream
            << POP_SGMT_ASM[0]
            << std::to_string(i)
            << POP_SGMT_ASM[1]
            << seg
            << POP_SGMT_ASM[2];
        return output_stream;
    }

    std::stringstream &write_pop_temp(int i)
    {
        output_stream
            << POP_TMP_ASM[0]
            << std::to_string(i+5)
            << POP_TMP_ASM[1];
        return output_stream;
    }

    std::stringstream &write_pop_static(int i)
    {
        output_stream
            << POP_STATIC_ASM[0]
            << class_name << '.' << std::to_string(i)
            << POP_STATIC_ASM[1];
        return output_stream;
    }

    std::stringstream &write_pop_pointer(int i)
    {
        output_stream
            << POP_PTR_ASM[0]
            << std::to_string(i+3)
            << POP_PTR_ASM[1];
        return output_stream;
    }

    std::stringstream &write_binary_op(std::string operation)
    {
        output_stream
            << BINARY_OP_ASM[0]
            << operation
            << BINARY_OP_ASM[1];
        return output_stream;
    }

    std::stringstream &write_unary_op(std::string operation)
    {
        output_stream
            << UNARY_OP_ASM[0]
            << operation
            << UNARY_OP_ASM[1];
        return output_stream;
    }

    std::stringstream &write_boolean_op(std::string cond)
    {
        output_stream
            << BOOL_OP_ASM[0]
            << LABEL_COUNT
            << BOOL_OP_ASM[1]
            << cond
            << BOOL_OP_ASM[2]
            << LABEL_COUNT
            << BOOL_OP_ASM[3]
            << LABEL_COUNT
            << BOOL_OP_ASM[4]
            << LABEL_COUNT
            << BOOL_OP_ASM[5];

        ++LABEL_COUNT;
        return output_stream;
    }

    std::stringstream &write_label(std::string label)
    {
        output_stream
            << LABEL_ASM[0]
            << label
            << LABEL_ASM[1];
        return output_stream;
    }

    std::stringstream &write_if_goto(std::string label)
    {
        output_stream
            << IF_GOTO_ASM[0]
            << label
            << IF_GOTO_ASM[1];
        return output_stream;
    }

    std::stringstream &write_goto(std::string label)
    {
        output_stream
            << GOTO_ASM[0]
            << label
            << GOTO_ASM[1];
        return output_stream;
    }

    std::stringstream &write_call(std::string function, int nArgs)
    {
        write_push_const("return") << LABEL_COUNT;
        write_push_addr("LCL");
        write_push_addr("ARG");
        write_push_addr("THIS");
        write_push_addr("THAT")
            << "@SP"
            << "D=M"
            << "@5"
            << "D=D-A"
            << "@" << nArgs
            << "D=D-A"
            << "@ARG"
            << "M=D"
            << "@SP"
            << "D=M"
            << "@LCL"
            << "M=D";
        write_goto(function)
            << '('
            << "return" << LABEL_COUNT
            << ')';
        
        ++LABEL_COUNT;
        return output_stream;
    }

    std::stringstream &write_function(std::string function, int nVars)
    {
        write_label(function);
        for (int i=0; i<nVars; ++i) {
            write_push_const("0");
        }
        return output_stream;
    }

    std::stringstream &write_return()
    {
        output_stream
            << "@LCL\n"
            << "D=M\n"
            << "@R15\n"
            << "M=D\n";
        write_pop_seg("ARG", 0)
            << "@ARG\n"
            << "D=M+1\n"
            << "@SP\n"
            << "M=D\n";
        write_restore_addr("THAT");
        write_restore_addr("THIS");
        write_restore_addr("ARG");
        write_restore_addr("LCL")
            << "@R13\n"
            << "A=M-1;JMP\n";
        return output_stream;
    }

    std::stringstream &write_push_addr(std::string addr)
    {
        output_stream
            << "@" << addr
            << "D=M"
            << "@SP"
            << "A=M"
            << "M=D"
            << "@SP"
            << "M=M+1";
        return output_stream;
    }

    std::stringstream &write_restore_addr(std::string addr)
    {
        output_stream
            << "@R15\n"
            << "AM=M-1\n"
            << "D=M\n"
            << "@" << addr << '\n'
            << "M=D\n";
        return output_stream;
    }
};

#endif // VMTRANSL_CPP