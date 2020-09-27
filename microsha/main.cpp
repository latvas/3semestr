#include <iostream>
#include <string>
#include <vector>
#include <dirent.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <csignal>


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

int cd(const std::string& destinationDir) {     //returns 0 if everything is OK, 1 if error
    int res;
    if (destinationDir[0] == '/'){      //absolute path
        res = chdir(destinationDir.c_str());
    } else {                            //local path
        std::string absDir = getCurrentDirectory() + "/" + destinationDir;
        res = chdir(absDir.c_str());
    }
    if (res == -1){
        fprintf(stderr, "cd: %s: Error when changing directory\n", destinationDir.c_str());
    }
    return res;
}

auto callExternalCommand(const std::vector<std::string>& data){
    pid_t pid = fork();
    if (pid == 0){ //child process
        //printf("HC: hello from child\n");
        std::vector<char *> v;
        v.reserve(data.size());
        for (const auto& word: data){
            v.push_back((char *)word.c_str());
        }
        v.push_back(nullptr);
        int res = execvp(v[0], &v[0]);
        fprintf(stderr, "%s: command not found\n", data[0].c_str());
        exit(1);
    } else { //parent process
        //printf("HP: hello from parent\n");
        wait(nullptr);
        //printf("CT: child has terminated\n");
    }
}

int commandExecutor(const std::vector<std::string> &data) { // call command by name and arguments
    if (data[0] == "cd") {   //first check if the command is internal
        if (data.size() > 2) {
            fprintf(stderr, "error: too many arguments for cd\n");
            return 1;
        }
        return cd(data[1]);
    } else if (data[0] == "pwd") {
        printf("%s\n", getCurrentDirectory().c_str());
        return 0;
    } else if (data[0] == "time") {
        printf("time\n"); // TODO Time command
        return 0;
    } else {    // call external command
        callExternalCommand(data);
    }
    return 0;
}

void printWelcomeMessage(bool root) {

    std::printf("[Pavlos %s]", getCurrentDirectory().c_str());
    if (root) {
        std::printf("! ");
    } else {
        std::printf("> ");
    }
}


[[noreturn]] int shellLoop() {
    while (true) {
        std::string inputBuffer;
        printWelcomeMessage(false);
        std::getline(std::cin, inputBuffer);
        std::vector<std::string> words = split(inputBuffer); //split input string by space and tab
        //for (const auto &word: words) printf("%s\n", word.c_str()); //print parsed data
        commandExecutor(words);
    }
}

int main(int argc, char *argv[]) {
    shellLoop();
    return 0;
}
