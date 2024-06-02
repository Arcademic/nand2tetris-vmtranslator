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
    if (path.string().back() == '/') {
        path = path.string().substr(0, path.string().size() - 1);
    }
    std::string output_file_path = path.parent_path().string();
    std::string output_file_name = path.stem().string();

    if (fs::is_directory(path)) {
        std::clog << "Input is a directory." << std::endl;
        output_file_path
            .append(1, fs::path::preferred_separator)
            .append(output_file_name)
            .append(1, fs::path::preferred_separator)
            .append(output_file_name)
            .append(output_type);
        std::ofstream output_file(output_file_path);
                    
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.path().extension() == input_type) {
                std::clog << "Found VM file: " << entry.path().filename() << std::endl;
                Parser p(entry.path(), input_type, output_type);
                std::stringstream cleaned_file = p.cleaned();
                
                Translator t(cleaned_file.str(), entry.path().stem().string());

                output_file << t.translated();
            }
        }
        output_file.close();
        std::clog << "Output file: " << output_file_path << std::endl;
    } else {
        std::clog << "Input is a single file." << std::endl;
        Parser p(path, input_type, output_type);
        std::stringstream cleaned_file = p.cleaned();
        
        Translator t(cleaned_file.str(), output_file_name);

        output_file_path
                .append(1, fs::path::preferred_separator)
                .append(output_file_name)
                .append(output_type);
        std::ofstream output_file(output_file_path);
        output_file << t.translated();
        output_file.close();
        std::clog << "Output file: " << output_file_path << std::endl;
    }

    return 0;
}
