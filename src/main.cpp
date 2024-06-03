#include "parser.cpp"
#include "code_writer.cpp"
#include <fstream>
#include <filesystem>

using namespace std;
namespace fs = filesystem;

string input_type = ".vm";
string output_type = ".asm";

bool init = false;

int main(int argc, char *argv[])
{
    fs::path path = argv[1];
    if (path.string().back() == '/') {
        path = path.string().substr(0, path.string().size() - 1);
    }
    string output_file_path = path.parent_path().string();
    string output_file_name = path.stem().string();

    if (fs::is_directory(path)) {
        clog << "Input is a directory." << endl;
        output_file_path
            .append(1, fs::path::preferred_separator)
            .append(output_file_name)
            .append(1, fs::path::preferred_separator)
            .append(output_file_name)
            .append(output_type);
        ofstream output_file(output_file_path);

        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.path().extension() == input_type) {
                clog << "Found VM file: " << entry.path().filename() << endl;
                Parser p(entry.path(), input_type, output_type);
                stringstream cleaned_file = p.cleaned();
                
                CodeWriter cw(cleaned_file.str(), entry.path().stem().string());
                if (!init) {
                    cw.write_init();
                    init = true;
                }

                output_file << cw.translated();
            }
        }
        output_file.close();
        clog << "Output file: " << output_file_path << endl;
    } else {
        clog << "Input is a single file." << endl;
        Parser p(path, input_type, output_type);
        stringstream cleaned_file = p.cleaned();
        
        CodeWriter cw(cleaned_file.str(), output_file_name);

        output_file_path
                .append(1, fs::path::preferred_separator)
                .append(output_file_name)
                .append(output_type);
        ofstream output_file(output_file_path);
        output_file << cw.translated();
        output_file.close();
        clog << "Output file: " << output_file_path << endl;
    }

    return 0;
}