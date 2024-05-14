#ifndef PARSER_CPP
#define PARSER_CPP

#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

static const std::string COMMENT_STR = "//";

class Parser
{
public:
    Parser(std::string path, std::string input_type, std::string output_type)
    {
        // check for correct file type
        if (path.substr(path.find_last_of(".")) != input_type)
        {
            throw std::invalid_argument("Invalid file format. Expected .vm file.");
        }
        raw_file.open(path);
    }

    std::stringstream cleaned()
    {
        std::stringstream clean_file;
        std::string raw_line;
        std::string clean_line;

        while (std::getline(raw_file, raw_line))
        {
            // remove comments
            auto comm_pos = raw_line.find(COMMENT_STR);
            if (comm_pos != std::string::npos)
            {
                raw_line = raw_line.substr(0, comm_pos);
            }
            // trim leading and trailing whitespace
            raw_line.erase(0, raw_line.find_first_not_of(" \t"));
            raw_line.erase(raw_line.find_last_not_of(" \t") + 1);

            // trim newline characters
            raw_line.erase(std::remove(raw_line.begin(), raw_line.end(), '\n'), raw_line.end());
            raw_line.erase(std::remove(raw_line.begin(), raw_line.end(), '\r'), raw_line.end());

            if (raw_line.empty() || raw_line[0] == '\r' || raw_line[0] == '\n')
            {
                continue;
            }

            clean_line = raw_line;
            clean_file << clean_line << '\n';
        }
        return clean_file;
    }

private:
    std::ifstream raw_file;
};

#endif // PARSER_CPP