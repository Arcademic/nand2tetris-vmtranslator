#ifndef PARSER_CPP
#define PARSER_CPP

#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

using namespace std;

static const string COMMENT_STR = "//";

class Parser
{
public:
    Parser(string path, string input_type, string output_type)
    {
        // check for correct file type
        if (path.substr(path.find_last_of(".")) != input_type)
        {
            throw invalid_argument("Invalid file format. Expected .vm file.");
        }
        raw_file.open(path);
    }

    stringstream cleaned()
    {
        stringstream clean_file;
        string raw_line;
        string clean_line;

        while (getline(raw_file, raw_line))
        {
            
            // remove comments
            auto comm_pos = raw_line.find(COMMENT_STR);
            if (comm_pos != string::npos)
            {
                raw_line = raw_line.substr(0, comm_pos);
            }
            // trim leading and trailing whitespace
            raw_line.erase(0, raw_line.find_first_not_of(" \t"));
            raw_line.erase(raw_line.find_last_not_of(" \t") + 1);

            // trim newline characters
            raw_line.erase(remove(raw_line.begin(), raw_line.end(), '\n'), raw_line.end());
            raw_line.erase(remove(raw_line.begin(), raw_line.end(), '\r'), raw_line.end());

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
    ifstream raw_file;
};

#endif // PARSER_CPP