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
    std::string path_string(path);
    free(path);
    return path_string;
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

int commandExecutor(std::vector<std::string> &data) { // call command by name and arguments
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
        printf("time\n"); // TODO Time command
        return 0;
    } else {    // call external command
        std::vector<int> separators = findAllStringInVector(data, "|");
        if (separators.empty()){ //call single command
            callExternalSingleCommand(data);
        } else{
            // TODO Сделать конвейер
        }

    }
    return 0;
}

void printWelcomeMessage(bool root, const std::string &userName) {

    std::printf("[%s %s]", userName.c_str(), getCurrentDirectory().c_str());
    if (root) {
        std::printf("! ");
    } else {
        std::printf("> ");
    }
}


[[noreturn]] int shellLoop(bool isRoot, const std::string &userName) {
    while (true) {
        std::string inputBuffer;
        printWelcomeMessage(isRoot, userName);
        std::getline(std::cin, inputBuffer);
        std::vector<std::string> words = split(inputBuffer); //split input string by space and tab
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
    return 0;
}
