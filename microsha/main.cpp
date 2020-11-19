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
#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <sys/resource.h>


std::vector<std::vector<std::string>> split(const std::string &str) {
    std::vector<std::vector<std::string>> commands;
    bool isPrevSpace = true;
    bool isPrevSeparator = true;
    commands.emplace_back();
    for (char i : str) {
        if (i == '|') {
            commands.emplace_back(); //add empty command
            isPrevSeparator = true;
            isPrevSpace = true;
            continue;
        }
        if (i == '>' || i == '<') {
            commands[commands.size() - 1].emplace_back(1, i);
            isPrevSpace = true;
            continue;
        }
        if (!isspace(i)) {
            if (isPrevSpace) {
                commands[commands.size() - 1].emplace_back(); // add empty string
            }
            commands.back().back().push_back(i);
            isPrevSpace = false;

        } else {
            isPrevSpace = true;
        }
    }
    return commands;
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

std::vector<std::string> listOfFilesInDirectory(const std::string &dirName) {
    std::vector<std::string> listOfFiles;
    DIR *dir = opendir(dirName.c_str());
    if (dir == nullptr) return listOfFiles;
    for (dirent *d = readdir(dir); d != nullptr; d = readdir(dir)) {
        if ((d->d_type == DT_REG) || (d->d_type == DT_DIR)) {
            listOfFiles.emplace_back(d->d_name);
        }
    }
    return listOfFiles;
}

std::vector<std::string> listOfDirInDirectory(const std::string &dirName) {
    std::vector<std::string> listOfDir;
    DIR *dir = opendir(dirName.c_str());
    if (dir == nullptr) return listOfDir;
    for (dirent *d = readdir(dir); d != nullptr; d = readdir(dir)) {
        if (d->d_type == DT_DIR) {
            listOfDir.emplace_back(d->d_name);
        }
    }
    return listOfDir;
}

bool matchStrings(char const *pattern, char const *filename) {
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

std::vector<std::string> filesMatchingPattern(std::string pattern) {
    //Я знаю, что реализация не лучшая, уже просто хочется побыстрее закончить
    std::vector<std::string> ans;
    int currentPosition = 0;
    if (pattern[0] == '/') { //abs path
        currentPosition = 1;
        ans.emplace_back("/");
    } else {
        pattern = "./" + pattern;
        currentPosition = 2;
        ans.emplace_back("./");
    }

    int slash = pattern.find('/', currentPosition);
    while (slash != std::string::npos) {    //Здесь обрабатываются директории
        std::string subPattern = pattern.substr(currentPosition, slash - currentPosition);
        std::vector<std::string> newAns;
        for (auto &an : ans) {
            std::vector<std::string> dirs = listOfDirInDirectory(an);
            if (dirs.empty()) {
                //ans.erase(i);
                //dirs = listOfDirInDirectory((*i));
                continue;
            }
            for (const auto &dir: dirs) {
                if (matchStrings(subPattern.c_str(), dir.c_str())) {
                    newAns.push_back(an + dir + "/");
                }
            }
        }
        ans = newAns;
        currentPosition = slash + 1;
        slash = pattern.find('/', currentPosition);
    }

    char last = *(pattern.end() - 1);
    if (last == '/') {    //если это директория, то возвращаем ответ
        return ans;
    }

    std::string subPattern = pattern.substr(currentPosition); // а вот здесь обрабатываем имя файла.
    std::vector<std::string> newAns;
    for (auto &an : ans) {
        std::vector<std::string> files = listOfFilesInDirectory(an);
        if (files.empty()) {
            //curDirectory = ans.erase(curDirectory);
            //files = listOfFilesInDirectory((*curDirectory));
            continue;
        }
        for (const auto &file: files) {
            if (matchStrings(subPattern.c_str(), file.c_str())) {
                newAns.push_back(an + file);
            }
        }
    }
    if (newAns.empty()) {
        newAns.push_back(pattern);
        return newAns;
    }
    return newAns;
}

int callExternalCommand(std::vector<std::string> &data) {
    std::vector<char *> v;
    v.reserve(data.size());
    for (const auto &word: data) {
        v.push_back((char *) word.c_str());
    }
    v.push_back(nullptr);
    prctl(PR_SET_PDEATHSIG, SIGINT);    //make child after parent exits
    int res = execvp(v[0], &v[0]);
    fprintf(stderr, "%s: command not found\n", data[0].c_str());
    exit(1);
}

void commandExecutor(std::vector<std::string> &data) { // call command by name and arguments

    int outPos = findFirstStringInVector(data, ">");
    if (outPos >= 0 && (outPos + 1) < data.size()) {    //redirect output
        FILE *outRes = freopen(data[outPos + 1].c_str(), "wt", stdout);
        if (outRes == nullptr) {
            fprintf(stderr, "%s: can't open output file\n", data[outPos + 1].c_str());
            exit(EXIT_FAILURE);
        }
        data.erase(data.begin() + outPos);
        data.erase(data.begin() + outPos);
    }

    int inPos = findFirstStringInVector(data, "<");     //redirect input
    if (inPos >= 0 && (inPos + 1) < data.size()) {
        FILE *inRes = freopen(data[inPos + 1].c_str(), "rt", stdin);
        if (inRes == nullptr) {
            fprintf(stderr, "%s: can't open input file\n", data[inPos + 1].c_str());
            exit(EXIT_FAILURE);
        }
        data.erase(data.begin() + inPos);
        data.erase(data.begin() + inPos);
    }

    //Подставляем имена вместо шаблонов
    for (int i = 1; i < data.size(); i++) {
        if ((data[i].find('*') != std::string::npos) || (data[i].find('?') != std::string::npos)) {
            std::string pattern = data[i];
            std::vector<std::string> filenames = filesMatchingPattern(pattern);
            data.insert(data.erase(data.begin() + i), filenames.begin(), filenames.end());
            //data.erase(data.begin() + i);
        }
    }

    //CD not work here because it runs not in main process...so, cd not works in pipeline
    if (data[0] == "cd") {   //first check if the command is internal
        if (data.size() > 2) {
            fprintf(stderr, "error: too many arguments for cd\n");
            exit(EXIT_FAILURE);
        } else if (data.size() == 1) {
            if (cd(getHomeDirectory()) == 0) {
                exit(EXIT_SUCCESS);
            } else {
                exit(EXIT_FAILURE);
            }
        }
        cd(data[1]);
        exit(EXIT_SUCCESS);
    } else if (data[0] == "pwd") {
        printf("%s\n", getCurrentDirectory().c_str());
        exit(EXIT_SUCCESS);
    } else if (data[0] == "set") {
        for (auto par = environ; *par != 0; ++par) {
            printf("%s\n", *par);
        }
        exit(EXIT_SUCCESS);
    } else {    // call external command
        callExternalCommand(data);

    }
}

void printWelcomeMessage() {
    struct passwd *pw = getpwuid(getuid());
    std::string userName(pw->pw_name);
    bool isRoot = false;
    if (pw->pw_uid == 0) {
        isRoot = true;
    }
    std::printf("[%s %s]", userName.c_str(), getCurrentDirectory().c_str());
    if (isRoot) {
        std::printf("! ");
    } else {
        std::printf("> ");
    }
}


int callPipeline(std::vector<std::vector<std::string>> &commands) {     //return 0 if OK, >0 if not OK
    std::vector<int> timepos = findAllStringInVector(commands[0], "time");
    if (timepos.size() > 1 || (timepos.size() == 1 && timepos[0] != 0)) {
        fprintf(stderr, "error: time can only be the first command\n");
        return 4;
    }
    for (int number = 0; number < commands.size(); ++number) {
        if ((number == 0 && findAllStringInVector(commands[number], "<").size() > 1) ||
            ((number != 0 && !findAllStringInVector(commands[number], "<").empty()))) {
            fprintf(stderr, "error: can't read from file\n");
            return 1;
        }
        if ((number == commands.size() - 1 && findAllStringInVector(commands[number], ">").size() > 1) ||
            ((number != commands.size() - 1 && !findAllStringInVector(commands[number], ">").empty()))) {
            fprintf(stderr, "error: can't write to file\n");
            return 2;
        }
        if (commands.size() > 1 && !findAllStringInVector(commands[0], "cd").empty()) {
            //fprintf(stderr, "cd not works in pipeline\n");
            //perror("cd not works in pipeline\n");
            printf("cd not works in pipeline\n");
            return 3;
        }
        timepos = findAllStringInVector(commands[number], "time");
        if (number > 0 && !timepos.empty()) {
            fprintf(stderr, "error: time can only be the first command\n");
            return 4;
        }
    }

    if (commands[0][0] == "time") {      //time command
        commands[0].erase(commands[0].begin());
        rusage start, stop;
        timeval start_real_time, stop_real_time;
        getrusage(RUSAGE_CHILDREN, &start);
        gettimeofday(&start_real_time, nullptr);

        callPipeline(commands);

        gettimeofday(&stop_real_time, nullptr);
        getrusage(RUSAGE_CHILDREN, &stop);
        auto userTimeMilliseconds = stop.ru_utime.tv_usec - start.ru_utime.tv_usec;
        auto userTimeSeconds = stop.ru_utime.tv_sec - start.ru_utime.tv_sec;
        auto systemTimeMilliseconds = stop.ru_stime.tv_usec - start.ru_stime.tv_usec;
        auto systemTimeSeconds = stop.ru_stime.tv_sec - start.ru_stime.tv_sec;
        auto realTimeMilliseconds = stop_real_time.tv_usec - start_real_time.tv_usec;
        auto realTimeSeconds = stop_real_time.tv_sec - start_real_time.tv_sec;

        printf("\n"
               "real: %lld,%lld s\n"
               "user: %lld,%lld s\n"
               "system: %lld,%lld s\n",
               realTimeSeconds, realTimeMilliseconds,
               userTimeSeconds, userTimeMilliseconds,
               systemTimeSeconds, systemTimeMilliseconds);

        return 0;
    }

    if (commands.size() == 1) {  // one command
        if (commands[0][0] == "cd") {
            if (commands[0].size() > 2) {
                fprintf(stderr, "error: too many arguments for cd\n");
                return -1;
            } else if (commands[0].size() == 1) {
                if (cd(getHomeDirectory()) == 0) {
                    return 0;
                } else {
                    return -1;
                }
            }
            cd(commands[0][1]);
            return 0;
        }

        pid_t childPid = fork();
        if (childPid == -1) {
            fprintf(stderr, "error: fork error\n");
            exit(EXIT_FAILURE);
        }
        if (childPid == 0) {     //child process
            commandExecutor(commands[0]);
        } else {
            wait(nullptr);
            return 0;
        }
    }

    std::vector<int[2]> pipefd(commands.size() - 1); // creating vector of pipe file descriptors

    for (int number = 0; number < commands.size() - 1; ++number) {
        if (pipe2(pipefd[number], O_CLOEXEC) == -1) {
            fprintf(stderr, "error: pipe error\n");
            return 3;
        }
    }

    for (int number = 0; number < commands.size(); ++number) {
        pid_t childPid = fork();
        if (childPid == -1) {
            fprintf(stderr, "error: fork error\n");
            exit(EXIT_FAILURE);
        }
        if (childPid == 0) {     //child process
            if (number == 0) {   //first command.
                dup2(pipefd[0][1], STDOUT_FILENO);  //redirect output from stdio to pipe
                close(pipefd[0][0]);    //close pipe input
                //fprintf(stderr, "PROCESS NUM %d\n", number);
                commandExecutor(commands[0]);
            } else if ((0 < number) && (number < commands.size() - 1)) {
                dup2(pipefd[number - 1][0], STDIN_FILENO);    //redirect input from previous pipe to stdin
                close(pipefd[number - 1][1]);     //close this pipe output
                dup2(pipefd[number][1], STDOUT_FILENO);     //redirect output from stdout to pipe
                close(pipefd[number][0]);       //close pipe input
                //fprintf(stderr, "PROCESS NUM %d\n", number);
                commandExecutor(commands[number]);
            } else if (number == commands.size() - 1) {  //last command
                dup2(pipefd[number - 1][0], STDIN_FILENO);  //same actions
                close(pipefd[number - 1][1]);
                //fprintf(stderr, "PROCESS NUM %d\n", number);
                commandExecutor(commands[number]);
            }
        } else {    //parent
            if (number == commands.size() - 1) {
                for (auto &i : pipefd) {    //close all pipes in parent
                    if (i[0] != -1) {
                        close(i[0]);
                    }
                    if (i[1] != -1) {
                        close(i[1]);
                    }

                }
                //wait(nullptr);

                for (int i = 0; i < commands.size(); ++i) {
                    wait(nullptr);
                }
            }
        }
    }
    return 0;
}

void sigHandler(int signal) {
    printf("\n");
}

int shellLoop() {
    signal(SIGINT, sigHandler);
    while (true) {
        std::string inputBuffer;
        printWelcomeMessage();
        //listOfFilesInDir(getCurrentDirectory());
        std::getline(std::cin, inputBuffer);
        if (std::cin.eof()) {
            return 1;
        }
        auto commands = split(inputBuffer);
        if (commands.empty() || commands[0].empty()) {
            continue;
        }
        callPipeline(commands);
    }
}

int main(int argc, char *argv[]) {
    cd(getHomeDirectory());
    shellLoop();
    printf("\nEOF: program terminated because input stream closed.\n");
    return 0;
}
