#include <cctype>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
#include <array>
#include <locale>

int  from_chapter         = 1;	   // Який номер дати першому збереженому файлу
int  split_length         = 5000;  // Скільки символів мають містити поділені файли
int  file_num             = 1;	   // Номер першого файлу коли зберігаємо ділянки файлів

enum what_to_do {
    NONE = 0,
    REPLACE_WORDS,                        // виконати заміну слів. -r
    SPLIT_TEXT,                           // поділити розділ на ділянки певної відстані. -t
    SPLIT_TEXT_TO_FILES,                  // нарізати розділ на файли відповідного розміру. -T
    CLIPBOARD_COPY_SPLIT_TEXT_BY_LENGTH,  // поділити текст за розміром і копіювати в буфер обміну. -E
    CLIPBOARD_COPY_SPLIT,                 // копіювати текст файлу в буфер обміну. -e
    FIX_ENDINGS,                          // виправити закінчення речень. -d
    REPARE_STRUCTURE,                     // виправити структуру тексту. -D
    FIX_BEGIN,                            // виправити початок тексту. -b
};

std::string chapter_delimeter = "\nРозділ ";

const std::string wl_copy  = "wl-copy ";
const std::string wl_paste = "wl-paste ";

std::string str = "";

int arg = 0;


void save_to_file(std::string_view fname, std::string &what){
    std::ofstream file;
    file.open(std::string("res/" + std::string(fname)).data(), std::ofstream::binary | std::ofstream::out);
    file.write(what.data(), what.length());
    file.flush();
    file.close();
}

void save_to_file(int index, std::string &what){
    save_to_file(std::to_string(index), what);
}

void read_file(std::string_view fname, std::string &res){
    std::ifstream file;
    file.open(fname.data(), std::ifstream::in);
    file.seekg (0, file.end);

    int length = file.tellg();
    if(length == 0){ file.close(); res = ""; return; }

    res.resize(length);

    file.seekg (0, file.beg);
    file.read(res.data(), length);
    file.close();
}

std::vector<std::string> split(const std::string& data, const std::string& delim){
    std::vector<std::string> result;
    size_t start = 0;
    for (size_t found = data.find(delim); found != std::string::npos; found = data.find(delim, start)){
        result.emplace_back(data.begin() + start - delim.size() - 1, data.begin() + found);
        start = found + delim.size();
    }
    if (start != data.size())
        result.emplace_back(data.begin() + start - delim.size(), data.end());

    return result;
}

//поділ розділу на ділянки за кількостю символів
std::vector<std::string> split_by_length(const std::string& data){
    std::vector<std::string> result;
    size_t start = 0;

    for(size_t index = 0; index + split_length < data.length(); ){
        index += split_length;

        index = data.rfind('\n', index);
        if(index == std::string::npos) break;

        //if(result.size() == 50) break;

        result.emplace_back(data.begin() + start, data.begin() + index);
        start = index;
    }

    if (start != data.size())
        result.emplace_back(data.begin() + start, data.end());
    return result;
}

void exec_cmd(const char* cmd, std::string &result) {
    std::array<char, 128> buffer;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
	    result = "popen() failed!";
	    return;
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
}

inline void replaceAll(std::string& data, const std::string_view& from, const std::string_view& to){
    for(size_t pos = 0; (pos = data.find(from, pos)) != std::string::npos; pos += to.size())
        data.replace(pos, from.size(), to);
}

inline void task_split_text(char** argv){
    for(size_t index = 0; index + split_length < str.length(); ){
        index += split_length;

        index = str.rfind('\n', index);
        if(index == std::string::npos) break;

        str.insert(index, 1, '\n');
    }

    save_to_file(std::string(argv[arg]), str);
}

inline void task_split_text_to_files(){
	auto lines = split_by_length(str);

	for(auto item : lines){
		save_to_file(file_num++, item);
	}
}

inline void task_clipboard_copy(char** argv, int argc, what_to_do how){
    replaceAll(str, "\"", "\\\"");
    replaceAll(str, "'", "\'");
    replaceAll(str, "`", "\\`");

    auto v = how == what_to_do::CLIPBOARD_COPY_SPLIT ?
                                split(str, chapter_delimeter) :
                                split_by_length(str);

    int index = 0;
    std::string c = " ";
    str = "";

    bool copy = false, rerun = false;
    while(index < v.size()){
        if(copy){
            exec_cmd(wl_paste.data(), str);
            copy = false;
        }

        std::system(std::string(wl_copy + "\"" + v.at(index) + "\"").c_str());

        std::cout << argv[arg] << " [" << index + 1 << ":" << v.size() << "]";

        std::getline(std::cin, c);
        if(c == "/"){//Переміщення по розділам
            if(index != 0) index--;//переміщення по розділу
            else if(arg >= 2) { arg -= 2; rerun = true; str = ""; break; }//повернення до попереднього розділу
        }
        else if(c.length() == 0) { index++; c = " "; }
        else continue;

        copy = true;
    }
    std::cout << std::endl;
    if(!rerun){
        if(copy) exec_cmd(wl_paste.data(), str);

        save_to_file(std::string(argv[arg]), str);
    }
}

inline void task_replace(char** argv){
    //str.erase(remove_if(str.begin(), str.end(), [](char c){ return !((c >= 'а' && c <= 'я') || (c >= 'a' && c <= 'z') || (c >= 0 && c <= 128));}), str.end());

    std::string replace = "";
    read_file("replace.txt", replace);

    std::istringstream iss(replace);

    for (std::string line; std::getline(iss, line); ){
        auto v = split(line, "|||");
        if(v.size() != 2){
            std::cout << "bad replace pattern [" << line << "]" << std::endl;
            continue;
        }

        replaceAll(str, v.at(0), v.at(1));

        save_to_file(std::string(argv[arg]) + "_mod", str);
    }
}

inline void task_fix_endings(){
    for(size_t index = 0; index < str.length(); index++){//fix ending
            index = str.find('\n', index);

            if(index == std::string::npos) break;

            auto c = str.at(--index);

            if(c == '.' || c == '?' || c == '!'){ index += 2; continue; }// "**" **.

            if((c >= 'a' && c <= 'z') || c == ',' /*|| c == '"'*/ || (c >= 'а' && c <= 'я') ){//**c\nc**
                str[index + 1] = ' ';
                index += 1;
                continue;
            }
            else index++;
    }
}

inline void task_repare_structure(){
    //fix_structure(str);
   /* std::string result;

    std::istringstream iss(str);

    for (std::string line; std::getline(iss, line); )
    {
*/

        /*
         *
         *
         *  UTF8 CPP library:

    char* str = (char*)text.c_str();    // utf-8 string
    char* str_i = str;                  // string iterator
    char* end = str+strlen(str)+1;      // end iterator

    do
    {
        uint32_t code = utf8::next(str_i, end); // get 32 bit code of a utf-8 symbol
        if (code == 0)
            continue;

        unsigned char[5] symbol = {0};
        utf8::append(code, symbol); // copy code to symbol

        // ... do something with symbol
    }
    while ( str_i < end );

        */
/*
        auto count = std::count(line.begin(), line.end(), '"');

        bool find_opened = false;
        bool is_opened   = false;

        auto change = [&](int index, std::string s){
                line[index] = ' ';
                line.insert(index, s);
        };
        */
/*
        std::cout << "co[" << count_o << "] cc[" << count_c << "]" << std::endl;

        if(count_o != count_c){
            if(count_o == 1){
                auto c = line.at(line.length() - 1);

                std::cout << "line [" + line + "] last [" << c << "]" << std::endl;

                if(c == '.'){
                    change(line.length() - 1, ".»");
                }
                else if(c == '!'){
                    change(line.length() - 1, "!»");
                }
                else if(c == '?'){
                    change(line.length() - 1, "?»");
                }
            }
        }
*/



/*
        if(count == 2 && line[0] == '"'){//звичайне речення "say it."
            change(0, "— ");
            replaceAll(line, "\"", "");
        }
        else if(count % 2 == 0){//кількість парна
                if(line[0] = '"'){//початок промови
                    change(0, "— ");

                    for(int i = 2; i < line.length(); i++){//початок заміни
                        if(line[i] == '"'){
                            int j = i + 1;
                            bool is_singel_word = true;// "word"
                            for(; j < line.length(); j++)
                            {
                                auto c = line[j];
                                if(isalpha(c)){}
                                else if(c == '"') break;
                                else { is_singel_word = false; break; }
                            }
                            if(is_singel_word){
                                change(i, "«");
                                change(j, "»");
                                i = j + 1;
                                continue;
                            }

                            if(find_opened){
                                change(i, "»");
                                find_opened = false;
                            }
                            else{
                                change(i, "«");
                                find_opened = true;
                            }
                        }
                    }
                }
                else{//десь у середині
                        for(int i = 1; i < line.length(); i++){//початок заміни
                            if(line[i] == '"'){
                                int j = i + 1;
                                bool is_singel_word = true;// "word"
                                for(; j < line.length(); j++)
                                {
                                    auto c = line[j];
                                    if(isalpha(c)){}
                                    else if(c == '"') break;
                                    else { is_singel_word = false; break; }
                                }
                                if(is_singel_word){
                                    change(i, "«");
                                    change(j, "»");
                                    i = j + 1;
                                    continue;
                                }

                                if(i - 2 > 0){
                                    if(line[i - 2] == ':'){//початок промови
                                        change(i, "«");// : ["]cc
                                        find_opened = false;
                                    }
                                    else change(i, "— ");// cc. —
                                }
                                else {
                                    if(!find_opened){
                                        change(i, "«");
                                        find_opened = true;
                                    }
                                    else{
                                        change(i, "«");
                                        find_opened = false;
                                    }
                                }
                            }
                        }
                }
        }
        // "Ескадрилья відкриття вогню!"
        //"Ракета "Альфа" не влучила. є якийсь спосіб?"
        //з іншого боку. "Мета місії: знищити "Юніон"!"
        else replaceAll(line, "\"", "«");

        result += line + "\n";
    }
    str = result;

    save_to_file(std::string(argv[arg]) + std::string("_mod"), str);

    */
}

inline void task_fix_begin(char** argv){
    //    replaceAll(str, "\n\n", "");

	auto remove = [&](int index, int count = 1){
		str.erase(index, count);
	};

        int index = 0;

        auto change = [&](int index, char c){
                str[index] = c;
//                str.insert(index, s);
        };
	//	change(15, '.');
	remove(0);
/*

*/
    save_to_file(std::string(argv[arg]), str);
}


int main(int argc, char** argv){
    if(argc < 2){
        std::cerr << "usage: fix [-rtTdDeEb] [-n length] [-c chapter] [-C delimeter] file ..." << std::endl;
        return 1;
    }

    if(!std::filesystem::exists("res/")) std::filesystem::create_directory("res");

    what_to_do task = what_to_do::NONE;
    for(arg = 1; arg < argc; arg++){
        if(std::filesystem::is_directory(argv[arg])) continue;

        if (argc >= 3){
            if(std::string("-n") == argv[arg]){
                split_length = std::atoi(argv[arg + 1]);
                arg++;
                if(split_length < 1000)
                    std :: cout << "Warning! Smalll split distance may loop forewer, incrise by -n num, current distance [" << split_length << "]" << std::endl;
                continue;
            }
            if(std::string("-c") == argv[arg]){ from_chapter = std::atoi(argv[arg + 1]); arg++; continue; }
            if(std::string("-C") == argv[arg]){
                chapter_delimeter = std::string(argv[arg + 1]);
                replaceAll(chapter_delimeter, "\\n", "\n"); replaceAll(chapter_delimeter, "\\r", "\r");
                arg++; continue;
            }
        }

        if(std::string("-r") == argv[arg]){ task = what_to_do::REPLACE_WORDS;                       continue; }
        if(std::string("-t") == argv[arg]){ task = what_to_do::SPLIT_TEXT;                          continue; }
        if(std::string("-T") == argv[arg]){ task = what_to_do::SPLIT_TEXT_TO_FILES;                 continue; }
        if(std::string("-e") == argv[arg]){ task = what_to_do::CLIPBOARD_COPY_SPLIT;                continue; }
        if(std::string("-E") == argv[arg]){ task = what_to_do::CLIPBOARD_COPY_SPLIT_TEXT_BY_LENGTH; continue; }
        if(std::string("-d") == argv[arg]){ task = what_to_do::FIX_ENDINGS;                         continue; }
        if(std::string("-D") == argv[arg]){ task = what_to_do::REPARE_STRUCTURE;                    continue; }
        if(std::string("-b") == argv[arg]){ task = what_to_do::FIX_BEGIN;                           continue; }

        if(!std::filesystem::exists(argv[arg])){
                std::cerr << "file [" << argv[arg] << "] not exist, skip" << std::endl;
                continue;
        }

        read_file(argv[arg], str);

        if(str.length() == 0){
            std::cerr << "file [" << argv[arg] << "] is empty, skip" << std::endl;
            continue;
        }

        switch(task){
            case REPLACE_WORDS: // заміна слів
            { task_replace(argv); break; }

            case SPLIT_TEXT: // поділ розділу на ділянки
            { task_split_text(argv); break; }

            case SPLIT_TEXT_TO_FILES: // поділ файлів за кількістю символів
            { task_split_text_to_files(); break; }

            case CLIPBOARD_COPY_SPLIT_TEXT_BY_LENGTH: // поділити файл і додати до буфіру обміну
            { task_clipboard_copy(argv, argc, CLIPBOARD_COPY_SPLIT_TEXT_BY_LENGTH); break; }

            case CLIPBOARD_COPY_SPLIT: // додати текст до буфіру обміну
            { task_clipboard_copy(argv, argc, CLIPBOARD_COPY_SPLIT); break; }

            case  FIX_ENDINGS: // виправити закінчення
            { task_fix_endings(); break; }

            case  REPARE_STRUCTURE: // виправити структуру файлу
            { task_repare_structure(); break; }

            case FIX_BEGIN: // виправити текст за шаблоном
            { task_fix_begin(argv); break; }

            default: { // поділ тексту на файли за ключовим словом
                auto v = split(str, chapter_delimeter);

                for(auto item : v){
                    save_to_file(from_chapter++, item);
                }
            }
        }

/* 	replaceAll(str, {"”", "\"" },
        { "“", "\""} ,
        { ". ", "." },
        { ", ", "," },
        { " \"", "\"" },
        { "\" ", "\"" },
        { "' ", "'" },
        { ": ", ":" },
        { "; ", ";" },
        { "? ", "?" },
        { "! ", "!" },
        { "  ", " " },
        { "  ", " " },
        { "  ", " " },
        { "”", "" },
        { "’", "'" },
        { "‘", "'" },
        { "“", "" },
        { "−", "" },
        { "—", "" },
        { "“", "" });
*/

    }
    return 0;
}
