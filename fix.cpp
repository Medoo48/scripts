#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
#include <array>

int  split_length = 5000;  // Скільки символів мають містити поділені файли
int  file_num     = 1;	   // Номер першого файлу коли зберігаємо результат
bool add_file_ext = true;
std::string file_ext = ".txt";


enum what_to_do {
    NONE = 0,
    SPLIT_TEXT,                           // поділити розділ на ділянки певної відстані. -t
    SPLIT_TEXT_TO_FILES,                  // нарізати розділ на файли відповідного розміру. -T
    CLIPBOARD_COPY_SPLIT_TEXT_BY_LENGTH,  // поділити текст за розміром і копіювати в буфер обміну. -E
    CLIPBOARD_COPY_SPLIT,                 // копіювати текст файлу в буфер обміну. -e
    FIX_ENDINGS,                          // виправити закінчення речень. -d
};

std::string chapter_delimeter = "\nРозділ ";

#ifdef __linux__ 
    const std::string wl_copy  = "wl-copy ";
    const std::string wl_paste = "wl-paste ";
#elif __APPLE__
    const std::string mac_copy  = "pbcopy";
    const std::string mac_paste = "pbpaste";
#elif _WIN32
    #include <windows.h>
    #include <conio.h>
#else
#error Схоже ваша операційна система не підтримується, зверніться до автора програми за допомогою
#endif

std::string str = "";

int arg = 0;

bool ends_with(std::string_view str, std::string_view suffix)
{
    return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

void save_to_file(std::string_view fname, std::string &what){
    std::ofstream file;
    std::string file_name = "res/" + std::string(fname) + (add_file_ext && !ends_with(fname, file_ext) ? file_ext : "");
    file.open(file_name.data(), std::ofstream::binary | std::ofstream::out);
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

    size_t last = 0;
    size_t found = 0;

    while(data.at(last) == '\n') last++;

    while ((found = data.find(delim, last)) != std::string::npos) {
        result.emplace_back(data.substr(last, found - last));
        last = found + 1;
    }

    if (last != data.size())
        result.emplace_back(data.substr(last));
  
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
#if defined(__linux__) || defined(__APPLE__)
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
#endif
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

void clipboard_set_text(const std::string *text){
#ifdef __linux__
        std::system(std::string(wl_copy + "\"" + *text + "\"").c_str());
#elif __APPLE__
        std::system(std::string(mac_copy + "\"" + *text + "\"").c_str());
#elif _WIN32
    OpenClitboard(0);
    EmptyClipboard();
    HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, text->length());
    if(!h) { CloseClipboard(); }
    memcpy(GlobalLock(h), text->c_str(), text->length());
    GlobalUnlock(h);
    SetClipboardData(CF_TEXT, h);
    CloseClipboard();
    GlobalFree(h)
#else
#error Схоже ваша операційна система не підтримується, зверніться до автора програми за допомогою
#endif
}

void clipboard_get_text(std::string &result){
#ifdef __linux__
    exec_cmd(wl_paste.data(), result);
#elif __APPLE__
    exec_cmd(mac_paste.data(), result);
#elif _WIN32
    OpenClitboard(0);
    HANDLE in = GetClipboardData(CF_TEXT);
    result += std::string((char *)in);
#else
#error Схоже ваша операційна система не підтримується, зверніться до автора програми за допомогою
#endif
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
            clipboard_get_text(str);
            copy = false;
        }

        clipboard_set_text(&v.at(index));

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
    std::cout << "arg [" << arg << "/" << argc << "], type '/' for going to previous chapter" << std::endl;
    if(!rerun){
        if(copy) clipboard_get_text(str);

        save_to_file(std::string(argv[arg]), str);
    }
}

inline void task_fix_endings(){
    for(size_t index = 0; index < str.length(); index++){//fix ending
            index = str.find('\n', index);

            if(index == std::string::npos) break;

            auto c = str.at(--index);

            if(c == '.' || c == '?' || c == '!'){ index += 2; continue; }// "**" **.

            if((c >= 'a' && c <= 'z') || c == ',' /*|| c == '"' || (c >= 'а' && c <= 'я')*/ ){//**c\nc**
                str[index + 1] = ' ';
                index += 1;
                continue;
            }
            else index++;
    }
}

int main(int argc, char** argv){
    if(argc < 2){
        std::cerr << "usage: fix [-adeEtT] [-n length] [-c chapter] [-C delimeter] file ..." << std::endl;
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
            if(std::string("-a") == argv[arg]){ file_ext = argv[arg + 1]; arg++; continue; }
            if(std::string("-c") == argv[arg]){ file_num = std::atoi(argv[arg + 1]); arg++; continue; }
            if(std::string("-C") == argv[arg]){
                chapter_delimeter = std::string(argv[arg + 1]);
                replaceAll(chapter_delimeter, "\\n", "\n"); replaceAll(chapter_delimeter, "\\r", "\r");
                arg++; continue;
            }
        }
        if(std::string("-A") == argv[arg]){ add_file_ext = false; continue; }

        if(std::string("-t") == argv[arg]){ task = what_to_do::SPLIT_TEXT;                          continue; }
        if(std::string("-T") == argv[arg]){ task = what_to_do::SPLIT_TEXT_TO_FILES;                 continue; }
        if(std::string("-e") == argv[arg]){ task = what_to_do::CLIPBOARD_COPY_SPLIT;                continue; }
        if(std::string("-E") == argv[arg]){ task = what_to_do::CLIPBOARD_COPY_SPLIT_TEXT_BY_LENGTH; continue; }
        if(std::string("-d") == argv[arg]){ task = what_to_do::FIX_ENDINGS;                         continue; }

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
            case SPLIT_TEXT: // поділ розділу на ділянки
            { task_split_text(argv); break; }

            case SPLIT_TEXT_TO_FILES: // поділ файлів за кількістю символів
            { task_split_text_to_files(); break; }

            case CLIPBOARD_COPY_SPLIT_TEXT_BY_LENGTH: // поділити файл і додати до буфіру обміну
            { task_clipboard_copy(argv, argc, CLIPBOARD_COPY_SPLIT_TEXT_BY_LENGTH); break; }

            case CLIPBOARD_COPY_SPLIT: // додати текст до буфіру обміну
            { task_clipboard_copy(argv, argc, CLIPBOARD_COPY_SPLIT); break; }

            case FIX_ENDINGS: // виправити закінчення
            { task_fix_endings(); break; }

            default: { // поділ тексту на файли за ключовим словом
                auto v = split(str, chapter_delimeter);

                for(auto item : v){
                    save_to_file(file_num++, item);
                }
            }
        }
    }
    return 0;
}
