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

bool replace_words        = false; // Чи треба робити заміну слів. -r
bool split_text           = false; // Чи треба ділити розділ на ділянки певної відстані. -t
bool split_text_to_files  = false; // Чи треба нарізати файли за розміром. -T
bool split_text_by_length = false; // Чи треба ділити текст за розміром і копіювати в буфер обміну. -E
bool clipboard_split_copy = false; // Чи треба копіювати текст файлу в буфер обміну. -e
bool fix_endings          = false; // Чи треба виправляти закінчення речень. -d
bool repare_structure     = false; // Чи треба виправляти структуру тексту. -D
bool fix_begin		  = false; // Чи треба виправляти початок тексту. -b

std::string chapter_delimeter = "\nРозділ ";

const std::string wl_copy  = "wl-copy ";
const std::string wl_paste = "wl-paste ";

void save_to_file(int index, std::string_view str){
    std::ofstream file;
    file.open(std::to_string(index), std::ofstream::binary | std::ofstream::out);
    file.write(str.data(), str.length());
    file.flush();
    file.close();
}

void save_to_file(std::string_view fname, std::string_view str){
    std::ofstream file;
    file.open(fname.data(), std::ofstream::binary | std::ofstream::out);
    file.write(str.data(), str.length());
    file.flush();
    file.close();
}

void read_file(std::string_view fname, std::string &str){
    std::ifstream file;
    file.open(fname.data(), std::ifstream::in);
    file.seekg (0, file.end);

    int length = file.tellg();
    if(length == 0){ file.close(); return; }

    str.resize(length);

    file.seekg (0, file.beg);
    file.read(str.data(), length);
    file.close();
}

std::vector<std::string> split(const std::string& str, const std::string& delim){
    std::vector<std::string> result;
    size_t start = 0;
    for (size_t found = str.find(delim); found != std::string::npos; found = str.find(delim, start)){
        result.emplace_back(str.begin() + start - delim.size() - 1, str.begin() + found);
        start = found + delim.size();
    }
    if (start != str.size())
        result.emplace_back(str.begin() + start - delim.size(), str.end());

    return result;
}

std::vector<std::string> split_by_length(const std::string& str, int length){
    std::vector<std::string> result;
    size_t start = 0;

    for(size_t index = 0; index + length < str.length(); ){//поділ розділу на ділянки за кількостю
            index += length;

            index = str.rfind('\n', index);
            if(index == std::string::npos) break;
		

	    if(result.size() == 50) break;
            result.emplace_back(str.begin() + start, str.begin() + index);
            start = index;
    }

    if (start != str.size())
        result.emplace_back(str.begin() + start, str.end());
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

static void replaceAll(std::string& data, const std::string_view& from, const std::string_view& to){
    for(size_t pos = 0; (pos = data.find(from, pos)) != std::string::npos; pos += to.size())
        data.replace(pos, from.size(), to);
}

void fix_structure(std::string &str){
    std::string result;

    std::istringstream iss(str);

    for (std::string line; std::getline(iss, line); )
    {


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

        auto count = std::count(line.begin(), line.end(), '"');

        bool find_opened = false;
        bool is_opened   = false;

        auto change = [&](int index, std::string s){
                line[index] = ' ';
                line.insert(index, s);
        };
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
}

void fix_strings_ending(std::string &str){
	if(fix_endings){
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

    if(split_text){//поділ розділу на ділянки
        for(size_t index = 0; index + split_length < str.length(); ){
            index += split_length;

            index = str.rfind('\n', index);
            if(index == std::string::npos) break;

            str.insert(index, 1, '\n');
        }
    }
}

int main(int argc, char** argv){
    if(argc < 2){
        std::cerr << "usage: fix [-rtTdDe] [-n length] [-c chapter] [-C delimeter] file ..." << std::endl;
        return 1;
    }

    if(!std::filesystem::exists("res/")) std::filesystem::create_directory("res");

    for(int arg = 1; arg < argc; arg++){
	if(std::filesystem::is_directory(argv[arg])) continue;

        if (argc >= 3){
            if(std::string("-n") == argv[arg]){ split_length      = std::atoi(argv[arg + 1]);   arg++; continue; }
            if(std::string("-c") == argv[arg]){ from_chapter      = std::atoi(argv[arg + 1]);   arg++; continue; }
            if(std::string("-C") == argv[arg]){
                chapter_delimeter = std::string(argv[arg + 1]);
                replaceAll(chapter_delimeter, "\\n", "\n"); replaceAll(chapter_delimeter, "\\r", "\r");
                arg++; continue;
            }
        }

        if     (std::string("-r") == argv[arg]){ replace_words        = true;  continue; }
        if     (std::string("-t") == argv[arg]){ split_text           = true;  continue; }
        if     (std::string("-T") == argv[arg]){ split_text_to_files  = true;  continue; }
        if     (std::string("-e") == argv[arg]){ clipboard_split_copy = true;  continue; }
        if     (std::string("-E") == argv[arg]){ split_text_by_length = clipboard_split_copy = true;  continue; }
        if     (std::string("-d") == argv[arg]){ fix_endings          = true; continue; }
        if     (std::string("-D") == argv[arg]){ repare_structure     = true;  continue; }
        if     (std::string("-b") == argv[arg]){ fix_begin	      = true;  continue; }

        if(!std::filesystem::exists(argv[arg])){
                std::cerr << "file [" << argv[arg] << "] not exist, skip" << std::endl;
                continue;
        }

        std::string str = "";
        read_file(argv[arg], str);

        if(str.length() == 0){
            std::cerr << "file [" << argv[arg] << "] is empty, skip" << std::endl;
            continue;
        }



   if(fix_begin){
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
            index = str.find('\n', index);
            if(index == std::string::npos)
            {
                std::cout << "bad file[" << argv[arg] << "]" << std::endl;
                continue;
            }
            const char *dell = "\n \n";
            str.insert(index, 1, *dell);
*/
            save_to_file("res/" + std::string(argv[arg]), str);

    }
else
        if(replace_words){
            //str.erase(remove_if(str.begin(), str.end(), [](char c){ return !((c >= 'а' && c <= 'я') || (c >= 'a' && c <= 'z') || (c >= 0 && c <= 128));}), str.end());

       		std::string replace = "";
	        read_file("replace.txt", replace);

            auto v = split(replace, "|||");
	
	        for(int i = 0; i < v.size(); i += 2){
                replaceAll(str, v.at(i), v.at(i + 1));
	        }
            save_to_file("res/" + std::string(argv[arg]) + std::string("_mod"), str);
	}
else if(repare_structure){
        fix_structure(str);

        save_to_file("res/" + std::string(argv[arg]) + std::string("_mod"), str);
}
else if(split_text_to_files){
	int  file_num = 1;
   //	fix_strings_ending(str);
	std::vector<std::string> lines = split_by_length(str, split_length);

	for(auto item : lines){
		save_to_file("res/" + std::to_string(file_num++), item);
	}

	continue;
}
else if(clipboard_split_copy){

   		replaceAll(str, "\"", "\\\"");
		replaceAll(str, "'", "\'");
		replaceAll(str, "`", "\\`");

    	//	fix_strings_ending(str);

   		std::vector<std::string> v;
		if(split_text_by_length)
			v = split_by_length(str, split_length);
		else
			v = split(str, chapter_delimeter);

   		int index = 0;
		std::string s = " ";
       		str = "";

		bool copy = false, rerun = false;
		while(index < v.size()){
			if(copy){
				exec_cmd(wl_paste.data(), str);
				copy = false;
			}

			std::system(std::string(wl_copy + "\"" + v.at(index) + "\"").c_str());

			std::cout << argv[arg] << " [" << index + 1 << ":" << v.size() << "]";

			std::getline(std::cin, s);
			if(s == "/"){//Переміщення по розділам
                if(index != 0) index--;//переміщення по розділу
                else if(arg >= 2) { arg -= 2; rerun = true; str = ""; break; }//повернення до попереднього розділу
            }
			else if(s.length() == 0) { index++; s = " "; }
			else continue;

			copy = true;
		}
		std::cout << std::endl;
        if(!rerun){
            if(copy) exec_cmd(wl_paste.data(), str);

            save_to_file("res/" + std::string(argv[arg]), str);
        }
}
else{//поділ роздлів тексту на файли
    // fix_strings_ending(str);
    auto v = split(str, chapter_delimeter);

    for(auto item : v){
        save_to_file("res/" + std::to_string(from_chapter++), item);
    }
}

// 	replaceAll(str, "”", "\"");
 //	replaceAll(str, "“", "\"");
       // replaceAll(str, ". ", ".");
/*        replaceAll(str, ", ", ",");
        replaceAll(str, " \"", "\"");
        replaceAll(str, "\" ", "\"");
        replaceAll(str, "' ", "'");
        replaceAll(str, ": ", ":");
        replaceAll(str, "; ", ";");
        replaceAll(str, "? ", "?");
        replaceAll(str, "! ", "!");
        replaceAll(str, "  ", " ");
        replaceAll(str, "  ", " ");
        replaceAll(str, "  ", " ");*/
//        replaceAll(str, "”", "");
//        replaceAll(str, "’", "'");
//        replaceAll(str, "‘", "'");
//        replaceAll(str, "“", "");
//        replaceAll(str, "−", "");
//        replaceAll(str, "—", "");
//        replaceAll(str, "“", "");

    }
    return 0;
}


