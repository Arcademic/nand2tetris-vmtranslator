#include "parser.cpp"
#include "translator.cpp"
#include <fstream>

std::string input_type = ".vm";
std::string output_type = ".asm";

int main(int argc, char *argv[])
{
    std::string path = argv[1];
    Parser p(path, input_type, output_type);
    std::stringstream cleaned_file = p.cleaned();

    // works only with forward slash for now
    std::string output_file_path =
        path.substr(0, path.find_last_of("/") + 1);
    std::string output_file_name =
        path.substr(
                path.find_last_of("/") + 1,
                path.length() - path.find_last_of("/") - input_type.length() - 1);
            
    
    Translator t(cleaned_file.str(), output_file_name);
    std::ofstream output_file(
        output_file_path
            .append(output_file_name)
            .append(output_type));
    output_file << t.translated();
    output_file.close();

    return 0;
}