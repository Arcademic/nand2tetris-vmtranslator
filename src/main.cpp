#include "parser.cpp"
#include "translator.cpp"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

std::string input_type = ".vm";
std::string output_type = ".asm";

int main(int argc, char *argv[])
{
    fs::path path = argv[1];
    std::string output_file_path = path.parent_path().string();
    output_file_path += fs::path::preferred_separator;
    std::string output_file_name = path.stem().string();

    Parser p(path, input_type, output_type);
    std::stringstream cleaned_file = p.cleaned();
    
    Translator t(cleaned_file.str(), output_file_name);

    std::ofstream output_file(
        output_file_path
            .append(output_file_name)
            .append(output_type));
    output_file << t.translated();
    output_file.close();

    return 0;
}