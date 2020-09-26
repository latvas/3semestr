#include <iostream>
#include <string>
#include <vector>
#include <dirent.h>
#include <cstdlib>
#include <cstring>



std::vector<std::string> split(const std::string &str) {
    std::vector<std::string> words;
    bool isPrevSpace = true;
    for (char i : str){
        if (i == '>' || i == '<' || i == '|'){
            words.emplace_back(1, i);
            isPrevSpace = true;
            continue;
        }
        if (!isspace(i)){
            if (isPrevSpace) {
                words.emplace_back(); // add empty string
            }
            words.back().push_back(i);
            isPrevSpace = false;

        } else {
            isPrevSpace = true;
        }
    }

    return words;
}


void printWelcomeMessage(std::string path, bool root){

    std::printf("[USERNAME %s]", path.c_str());
    if (root){
        std::printf("! ");
    } else {
        std::printf("> ");
    }
}

[[noreturn]] void shellLoop(){
    while (true){
        const std::string currentDirectory = "/";
        std::string inputBuffer;
        printWelcomeMessage(currentDirectory, false);
        std::getline(std::cin, inputBuffer);
        std::vector<std::string> words = split(inputBuffer); //split input string by space and tab
        for (const auto& word: words) printf("%s\n", word.c_str()); //print parsed data
    }
}

int main(int argc, char* argv[]) {
    shellLoop();
    return 0;
}
