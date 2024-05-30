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
    "@SP\nAM=M-1\nD=M\nA=A-1\nD=M-D\n@", "\nD;",
    "\n@SP\nA=M-1\nM=0\n@", "\n0;JMP\n(",
    ")\n@SP\nA=M-1\nM=-1\n(", ")\n"};
static const std::string COND_LABEL = "COND";
static const std::string END_LABEL = "END";


// add filename to LABELS
static const std::regex LABEL_VM(R"(^\s*label\s+(string_regex)\s*$)"); // add string regex

static const std::regex IF_GOTO_VM(R"(^\s*if-goto\s+(string_regex)\s*$)");

static const std::regex GOTO_VM(R"(^\s*goto\s+(string_regex)\s*$)");

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
                int arg = std::stoi(regex_match_results[1].str());
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
                write_pop_seg(arg, SGMT_MAP.at(seg));
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
        }

        return output_stream.str();
    }

private:
    std::stringstream input_stream;
    std::stringstream output_stream;
    std::string class_name;

    std::stringstream &write_push_const(int i)
    {
        output_stream
            << PUSH_CONST_ASM[0]
            << std::to_string(i)
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

    std::stringstream &write_pop_seg(int i, std::string seg)
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
        auto label_count_str = std::to_string(LABEL_COUNT);
        output_stream
            << BOOL_OP_ASM[0]
            << COND_LABEL << label_count_str
            << BOOL_OP_ASM[1]
            << cond
            << BOOL_OP_ASM[2]
            << END_LABEL << label_count_str
            << BOOL_OP_ASM[3]
            << COND_LABEL << label_count_str
            << BOOL_OP_ASM[4]
            << END_LABEL << label_count_str
            << BOOL_OP_ASM[5];

        ++LABEL_COUNT;
        return output_stream;
    }
};

#endif // VMTRANSL_CPP