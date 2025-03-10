#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using filesystem::path;

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}

bool ProcessInclude(const path& file, ofstream& out, const vector<path>& include_directories, 
                    const path& parent_directory);

bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories) {
    ifstream in(in_file);
    if (!in) {
        return false;
    }

    ofstream out(out_file);
    if (!out) {
        return false;
    }

    path parent_directory = in_file.parent_path();
    
    return ProcessInclude(in_file, out, include_directories, parent_directory);
}

bool ProcessInclude(const path& file, ofstream& out, const vector<path>& include_directories, 
                    const path& parent_directory) {
    ifstream in(file);
    if (!in) {
        return false;
    }

    const regex local_include_pattern(R"/(\s*#\s*include\s*"([^"]*)"\s*)/");
    const regex global_include_pattern(R"/(\s*#\s*include\s*<([^>]*)>\s*)/");
    
    string line;
    int line_counter = 0;
    
    while (getline(in, line)) {
        line_counter++;
        smatch match;

        if (regex_match(line, match, local_include_pattern)) {
            string include_file = match[1].str();
            
            path include_path = parent_directory / include_file;
            ifstream included_file(include_path);
            
            if (!included_file) {
                bool found = false;
                for (const auto& dir : include_directories) {
                    include_path = dir / include_file;
                    included_file = ifstream(include_path);
                    if (included_file) {
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    cout << "unknown include file " << include_file 
                         << " at file " << file.string() 
                         << " at line " << line_counter << endl;
                    return false;
                }
            }
            
            if (!ProcessInclude(include_path, out, include_directories, include_path.parent_path())) {
                return false;
            }
        } 

        else if (regex_match(line, match, global_include_pattern)) {
            string include_file = match[1].str();
            
            bool found = false;
            path include_path;
            
            for (const auto& dir : include_directories) {
                include_path = dir / include_file;
                ifstream included_file(include_path);
                if (included_file) {
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                cout << "unknown include file " << include_file 
                     << " at file " << file.string() 
                     << " at line " << line_counter << endl;
                return false;
            }
            
            if (!ProcessInclude(include_path, out, include_directories, include_path.parent_path())) {
                return false;
            }
        } else {
            out << line << endl;
        }
    }
    
    return true;
}
