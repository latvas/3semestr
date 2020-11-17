#include <iostream>
#include <string>
#include <vector>
#include <dirent.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <csignal>
#include <pwd.h>

// TODO Сделать перехват сигналов, чтобы  на ctrl+С программа не закрывалась
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

int findFirstStringInVector(const std::vector<std::string> &v, const std::string &t) {
    int ans = -1;
    for (size_t i = 0; i < v.size(); ++i) {
        if (v[i] == t) {
            ans = i;
            return ans;
        }
    }
    return ans;
}

std::vector<int> findAllStringInVector(const std::vector<std::string> &v, const std::string &t) {
    std::vector<int> ans;
    for (size_t i = 0; i < v.size(); ++i) {
        if (v[i] == t) {
            ans.push_back(i);
        }
    }
    return ans;
}

auto getCurrentDirectory() {
    char *path = (char *) malloc(PATH_MAX * sizeof(char)); //allocating memory
    char *res = getcwd(path, PATH_MAX);
    std::string pathString(path);
    free(path);
    return pathString;
}

auto getHomeDirectory() {
    struct passwd *pw = getpwuid(getuid());
    std::string homeDir(pw->pw_dir);
    return homeDir;
}

int cd(const std::string &destinationDir) {     //returns 0 if everything is OK, 1 if error
    int res;
    if (destinationDir[0] == '/') {      //absolute path
        res = chdir(destinationDir.c_str());
    } else {                            //local path
        std::string absDir = getCurrentDirectory() + "/" + destinationDir;
        res = chdir(absDir.c_str());
    }
    if (res == -1) {
        fprintf(stderr, "cd: %s: Error when changing directory\n", destinationDir.c_str());
    }
    return res;
}

std::vector<std::string> listOfFilesAndDirInDirectory(const std::string &dirName) {
    std::vector<std::string> listOfFiles;
    DIR *dir = opendir(dirName.c_str());
    if (dir == nullptr) return listOfFiles;
    for (dirent *d = readdir(dir); d != nullptr; d = readdir(dir)) {
        if ((d->d_type == DT_REG) || (d->d_type == DT_DIR)) {
            // printf("TYPE=%d, IS FILE=%d, NAME=%s\n", d->d_type, d->d_type==DT_REG, d->d_name);
            listOfFiles.emplace_back(d->d_name);
        }
    }
    return listOfFiles;
}

bool matchStrings(char const *pattern, char const *filename) {// TODO Сделать подстановку имен вместо шаблонов
    for (; pattern[0] != '\0'; pattern++) {
        switch (pattern[0]) {
            case '?': {
                if (filename[0] == '\0') {   //is empty
                    return false;
                }
                filename++; //next symbol
                break;
            }
            case '*': {
                if (pattern[1] == '\0') {    //if * is the last symbol, then any expression suits us
                    return true;
                }
                size_t n = strlen(filename);
                for (int i = 0; i < n; ++i) {
                    if (matchStrings(pattern + 1, filename + i)) {
                        return true;
                    }
                }
                return false;
                break;
            }
            default:    // not * and not ?
                //printf("%c\n", pattern[0]);
                if (filename[0] != pattern[0]) {
                    return false;
                }
                filename++;    //next symbol
        }
    }
    return filename[0] == '\0';     // is string empty
}


int callExternalSingleCommand(std::vector<std::string> &data) {
    pid_t pid = fork();
    if (pid == 0) {                                          //child process
        int outPos = findFirstStringInVector(data, ">");
        if (outPos >= 0 && (outPos + 1) < data.size()) {
            FILE *outRes = freopen(data[outPos + 1].c_str(), "wt", stdout);
            if (outRes == nullptr) {
                fprintf(stderr, "%s: can't open output file\n", data[outPos + 1].c_str());
                return -1;
            }
            data.erase(data.begin() + outPos);
            data.erase(data.begin() + outPos);
        }

        int inPos = findFirstStringInVector(data, "<");
        if (inPos >= 0 && (inPos + 1) < data.size()) {
            FILE *inRes = freopen(data[inPos + 1].c_str(), "rt", stdin);
            if (inRes == nullptr) {
                fprintf(stderr, "%s: can't open input file\n", data[inPos + 1].c_str());
                return -1;
            }
            data.erase(data.begin() + inPos);
            data.erase(data.begin() + inPos);
        }

        //for (const auto &word: data) {fprintf(stderr,"%s\n", word.c_str());}

        std::vector<char *> v;
        v.reserve(data.size());
        for (const auto &word: data) {
            v.push_back((char *) word.c_str());
        }
        v.push_back(nullptr);
        int res = execvp(v[0], &v[0]);
        fprintf(stderr, "%s: command not found\n", data[0].c_str());
        exit(1);
    } else {                                                //parent process
        //printf("HP: hello from parent\n");
        wait(nullptr);
        //printf("CT: child has terminated\n");
    }
    return 0;
}

int callPipeline(std::vector<std::string> &data) {
    //TODO pipeline
}

int commandExecutor(std::vector<std::string> &data) { // call command(pipeline) by name and arguments
    // TODO Сделать комманду SET
    if (data[0] == "cd") {   //first check if the command is internal
        if (data.size() > 2) {
            fprintf(stderr, "error: too many arguments for cd\n");
            return 1;
        } else if (data.size() == 1) {
            return cd(getHomeDirectory());
        }
        return cd(data[1]);
    } else if (data[0] == "pwd") {
        printf("%s\n", getCurrentDirectory().c_str());
        return 0;
    } else if (data[0] == "time") {
        printf("time coming soon\n"); // TODO Time command
        return 0;
    } else {    // call external command
        std::vector<int> separators = findAllStringInVector(data, "|");
        if (separators.empty()) { //call single command
            callExternalSingleCommand(data);
        } else {
            // TODO Сделать конвейер
        }

    }
    return 0;
}

void printWelcomeMessage(bool isRoot, const std::string &userName) {

    std::printf("[%s %s]", userName.c_str(), getCurrentDirectory().c_str());
    if (isRoot) {
        std::printf("! ");
    } else {
        std::printf("> ");
    }
}

int shellLoop(bool isRoot, const std::string &userName) {
    while (true) {
        std::string inputBuffer;
        printWelcomeMessage(isRoot, userName);
        //listOfFilesInDir(getCurrentDirectory());
        std::getline(std::cin, inputBuffer);
        if (std::cin.eof()) {
            return 1;
        }
        std::vector<std::string> words = split(inputBuffer);
        if (words.empty()) {
            continue;
        }
        //split input string by space and tab
        // for (const auto &word: words) printf("%s\n", word.c_str()); //print parsed data
        commandExecutor(words);
    }
}

int main(int argc, char *argv[]) {
    struct passwd *pw = getpwuid(getuid());
    std::string userName(pw->pw_name);
    bool isRoot = false;
    if (pw->pw_uid == 0) {
        isRoot = true;
    }
    std::string homeDir(pw->pw_dir);
    cd(homeDir);
    shellLoop(isRoot, userName);
    printf("\nEOF: program terminated because input stream closed.\n");
    return 0;
}
