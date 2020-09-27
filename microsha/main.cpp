#include <iostream>
#include <string>
#include <vector>
#include <dirent.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>


std::vector<std::string> split(const std::string &str) {
    std::vector<std::string> words;
    bool isPrevSpace = true;
    for (char i : str) {
        if (i == '>' || i == '<' || i == '|') {
            words.emplace_back(1, i);
            isPrevSpace = true;
            continue;
        }
        if (!isspace(i)) {
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

auto getCurrentDirectory() {
    char *path = (char *) malloc(PATH_MAX * sizeof(char)); //allocating memory
    char *res = getcwd(path, PATH_MAX);
    std::string path_string(path);
    free(path);
    return path_string;
}

void printWelcomeMessage(bool root) {

    std::printf("[USERNAME %s]", getCurrentDirectory().c_str());
    if (root) {
        std::printf("! ");
    } else {
        std::printf("> ");
    }
}

int shellLoop() {
    while (true) {
        std::string inputBuffer;
        printWelcomeMessage(false);
        std::getline(std::cin, inputBuffer);
        std::vector<std::string> words = split(inputBuffer); //split input string by space and tab
        for (const auto &word: words) printf("%s\n", word.c_str()); //print parsed data
        return 0;
    }
}

int main(int argc, char *argv[]) {
    shellLoop();
    return 0;
}
