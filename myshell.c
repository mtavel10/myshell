#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/stat.h>

enum redirect_type {
    NONE, NORMAL, ADVANCED
};

void myPrint(char *msg) {
    write(STDOUT_FILENO, msg, strlen(msg));
}

void myErrorPrint() {
    char error_message[30] = "An error has occurred\n";
    write(STDOUT_FILENO, error_message, strlen(error_message));
}

int arrLen(char** arr) {
	int len = 0;
	while (*arr) {
		len++;
		arr++;
	}
	return len;
}

// Get rid of whitespace and ret, returns an array of char* args
char** getCmd(char *msg) {
    // Loop over to find the number of args in this command
    int arg_count = 0;
    bool in_arg = false;
    char* pmsg = msg;
    while (*pmsg){
        // Condition for a new argument
        if (!in_arg && *pmsg != ' ' && *pmsg != '\n' && *pmsg != '\t'){
            in_arg = true;
            arg_count++;
        }
        else if (*pmsg == ' ' || *pmsg == '\n' || *pmsg == '>' 
                                               || *pmsg == '\t'){
            // Counting '>' or '>+' as arguments
            if (*pmsg == '>'){
                arg_count++;
            }
            in_arg = false;
        }
        pmsg++;
    }
    
    // Allocate an array of arguments (i.e. one command)
    char **cmd = (char**)malloc(sizeof(char*) * (arg_count + 1));
    
    // i tracks which argument we're on
    int i = 0; 
    pmsg = msg;
    in_arg = false;
    while (*pmsg) {
        // Need to reset to the beginning of the arg
        if (*pmsg == '>'){
            in_arg = false; 
        }
        // take out the spaces, tabs, skip over +
        if (!in_arg && *pmsg != ' ' && *pmsg != '+' && *pmsg != '\t') { 
            in_arg = true;
            int arg_len;
            char* pcmd;
            
            // if this arg is a redirect command, parses differently
            if (*pmsg == '>'){
                if (*(pmsg + 1) == '+'){
                    arg_len = 2; // ['>', '+']
                }
                else {
                    arg_len = 1; // ['>']
                    // after parsing this, want to reset to a new arg
                    in_arg = false; 
                }
            }
            else {
                // find the length of this argument
                pcmd = pmsg;
                while (*pcmd != ' ' && *pcmd != '\0' && *pcmd != '\n' 
                                    && *pcmd != '>' && *pcmd != '\t') {
                    pcmd++;
                }
                arg_len = pcmd - pmsg;
            }
            // create char array for this argument
            char* arg = (char*)malloc(sizeof(char) * arg_len + 1);
            pcmd = pmsg;
            int j;
            for (j = 0; j < arg_len; j++) {
				arg[j] = *pcmd;
				pcmd++;
            }

            // add the argument to the command array
            cmd[i] = arg;
            i++;
        }
        else if (*pmsg == ' ' || *pmsg == '+' ){
            in_arg = false;
        }
        pmsg++;
    }
    
    cmd[i] = '\0';

    return cmd;
}


// Extracts multiple commands from an input message
// Sets outparameter to the number of commands in the input message
char*** getCmds(char* msg, int* num_cmds){
    int cmd_count = 0;
    char* pmsg = msg;
    bool in_cmd = false;

    while (*pmsg && *pmsg != '\n') { // doesn't parse return
        // We are at the first character of a command 
        // A command being the raw string message, only delimited by semi-colons
        if (!in_cmd && *pmsg != ';' && *pmsg != '\n') {
            in_cmd = true;
            // need to check if the command has contents
            bool has_contents = true; 
            char* pcopy = pmsg;
            while (*pcopy && *pcopy != '\n' && *pcopy != ';'){
                if (*pcopy != ' ' && *pcopy != '\t'){ 
                    has_contents = false;
                }
                pcopy++;
            }
            if (!has_contents){
                cmd_count++;
            }
        }
        else if (*pmsg == ';' || *pmsg == '\n') {
            in_cmd = false;
        }
        pmsg++;
    }
    (*num_cmds) = cmd_count;

    char*** cmds = (char***)malloc(sizeof(char**) * (cmd_count+ 1));

    pmsg = msg; // start at beginning of full message
    unsigned int i;
    
    for (i = 0; i < cmd_count; i++){
        // need to skip semi-colons and blank commands
        while (*pmsg == ';' || *pmsg == ' ' || *pmsg == '\t'){
            pmsg++;
        }
        // parse each command, one at a time
        char* command_string;
        unsigned int str_len = 0; // how long is command string? 
        
        //printf("char at: %c\n", *pmsg);
        while (*pmsg && *pmsg != '\n' && *pmsg != ';') {
            str_len++;
            pmsg++;
        }
        
        command_string = (char*)malloc(sizeof(char) * (str_len + 1));
        
        pmsg = pmsg - str_len; // sets pointer to beginning of command
        //printf("message pointer is at %c\n", *pmsg);
        unsigned int j;
        for (j = 0; j < str_len; j++){
            command_string[j] = pmsg[j];
        }
        command_string[j] = '\0';
        cmds[i] = getCmd(command_string);

        pmsg = pmsg + str_len + 1; // sets pointer to start of next command
        
        free(command_string);
    }

    cmds[i] = '\0';
    return cmds;
}


int main(int argc, char *argv[]) {
    bool batch_mode;
    FILE *file;

    // File check
    if (argc == 1){
        batch_mode = false;
    }
    else {
        file = fopen(argv[1], "r");
        if (!file){
            myErrorPrint();
            exit(1);
        }
        else {
            batch_mode = true;
        }
    }

    char cmd_buff[5000];
    char *pinput;
    
    while (1) {
        if (batch_mode){
            // Read commands from the file
            pinput = fgets(cmd_buff, 5000, file);
            // Don't print tabs, blank lines, returns
            bool blank = true;
            unsigned int i = 0;
            while (cmd_buff[i]){
                char curr = cmd_buff[i];
                if (curr != ' ' && curr != '\t' && curr != '\n'){
                    blank = false;
                }
                i++;
            }
            if (!blank){
                myPrint(cmd_buff);
            }
        }
        else {
            // Read commands from standard input
            myPrint("myshell> ");
            pinput = fgets(cmd_buff, 5000, stdin);
        }
        // Exits if reached end of file or fgets error
        if (!pinput) {
            exit(0);
        }

        // parses a single command and its args
        int num_cmds;
        char ***cmds = getCmds(cmd_buff, &num_cmds);

        if (strlen(cmd_buff) > 514) { 
            myErrorPrint();
            continue;
        }

        // Loop through every command in the cmds array
        unsigned int j; 
        for (j = 0; j < num_cmds; j++){
            char** cmd = cmds[j];
            int cmd_len = arrLen(cmd);
    
            // Built-in commands: exit, cd, pwd
            if(!strcmp("exit", cmd[0])) {
                if (cmd_len > 1){
                    myErrorPrint();
                    continue;
                }
                exit(0);
            }
            else if (!strcmp("cd", cmd[0])) {
                if (cmd_len == 1){
                    chdir(getenv("HOME"));
                    continue;
                }
                // Check to make sure we're passing in valid arguments
                else if (!strcmp(">", cmd[1])) {
                    myErrorPrint();
                    continue;
                }
                // Checking this format
                else if (!strcmp("/", cmd[1])){
                    if (cmd_len > 2){
                        if (!strcmp("/", cmd[2]) || !strcmp(">", cmd[2])){
                            myErrorPrint();
                            continue;
                        }
                        else if (access(cmd[2], F_OK)){
                            myErrorPrint();
                            continue;
                        }
                    }
                }

                // Checks to see if the path exists before changing directory
                if (!access(cmd[1], F_OK)){
                    chdir(cmd[1]);
                }
                else {
                    myErrorPrint();
                    continue;
                }
            }
            else if (!strcmp("pwd", cmd[0])) {
                if (cmd_len > 1){
                    if (strcmp(cmd[1], "-L") || strcmp(cmd[1], "-P")){
                        myErrorPrint();
                        continue;
                    }
                }
                char cwd_buff[514];
                char* pcwd = getcwd(cwd_buff, sizeof(cwd_buff));
                if (!pcwd){
                    myErrorPrint();
                    continue;
                }
                myPrint(cwd_buff);
                myPrint("\n");
            }
            // WE ARE EXECUTING A PROGRAM!
            else  {
                // Check if we are redirecting output to a file
                enum redirect_type redirect_type = NONE;
                char *filename;
                char **redirect_cmd;
                unsigned int redirect_index;
                int outputFD;

                // used only in advanced redirect
                int tempFD;
                char file_buff[512];
                int bytes_read;

                bool error = false;
                unsigned int k = 0;
                while (cmd[k]) {
                    // Advanced redirection
                    if (!strcmp(cmd[k], ">+")) {
                        // Error if more than one redirect command
                        if (redirect_type != NONE){
                            myErrorPrint();
                            error = true;
                            break;
                        }

                        filename = cmd[k+1];

                        if (access(filename, F_OK) == 0) {
                            // if file exists, we are doing advanced redirect
                            redirect_type = ADVANCED;
                        }
                        else{
                            // otherwise, treating it as normal
                            redirect_type = NORMAL;
                        }

                        redirect_index = k;
                        redirect_cmd = (char**)malloc(sizeof(char*) * (redirect_index + 1));
                        for (k = 0; k < redirect_index; k++){
                            redirect_cmd[k] = cmd[k];
                        }
                        redirect_cmd[k] = '\0';
                    }

                    // Normal redirection
                    if (!strcmp(cmd[k], ">")) {
                        if (redirect_type != NONE){
                            myErrorPrint();
                            error = true;
                            break;
                        }

                        redirect_type = NORMAL;
                        filename = cmd[k+1];
                        
                        // If file already exists, error
                        if (access(filename, F_OK) == 0) {
                            // myPrint("LINE 396: ");
                            myErrorPrint();
                            error = true;
                            break;
                        }

                        redirect_index = k;
                        redirect_cmd = (char**)malloc(sizeof(char*) * (redirect_index + 1));
                        for (k = 0; k < redirect_index; k++){
                            redirect_cmd[k] = cmd[k];
                        }
                        redirect_cmd[k] = '\0';
                    }
                    k++;
                }
                if (error){
                    // move onto the next command if this one produces an error
                    continue;
                }


                int ret;
                ret = fork();
                
                // Child process (i.e. the program)
                if (ret == 0) {
                    int prog;

                    if (redirect_type == NORMAL){
                        outputFD = creat(filename, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                        int dup_ret = dup2(outputFD, STDOUT_FILENO);

                        if (dup_ret < 0){
                            myErrorPrint();
                            exit(1);
                        }

                        prog = execvp(redirect_cmd[0], redirect_cmd);
                        if (prog < 0){
                            myErrorPrint();
                            exit(1);
                        }
                    }
                    else if (redirect_type == ADVANCED){
                        outputFD = open(filename, O_RDONLY);
                        if (outputFD < 0) {
                            myPrint("LINE 438: ");
                            myErrorPrint();
                            exit(1);
                        }
                        
                        // copy og file contents to the temp
                        tempFD = creat("temp", S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                        close(tempFD);
                        tempFD = open("temp", O_RDWR, O_APPEND);
                        if (tempFD < 0){
                            myErrorPrint();
                            exit(1);
                        }

                        int bytes_written;
                        while ((bytes_read = read(outputFD, file_buff, 512)) > 0) {

                            bytes_written = write(tempFD, file_buff, bytes_read);

                            if (bytes_written < 0){
                                myErrorPrint();
                                exit(1);
                            }
                        }

                        close(tempFD);
                        close(outputFD);

                        // zero out the contents of the file
                        fclose(fopen(filename, "w+"));
                        open(filename, O_WRONLY);

                        int dup_ret = dup2(outputFD, STDOUT_FILENO);
                        if (dup_ret < 0){
                            myErrorPrint();
                            exit(1);
                        }
                    
                        prog = execvp(redirect_cmd[0], redirect_cmd);
                        if (prog < 0){
                            myErrorPrint();
                            exit(1);
                        }
                    }
                    else{ // No redirection
                        prog = execvp(cmd[0], cmd);
                        
                        if (prog < 0){
                            myErrorPrint();
                            exit(1);
                        }
                    }
                }
                else if (ret > 0) { // ret = child's pid
                    int *child_status = 0;
                    waitpid(ret, child_status, WUNTRACED); 

                    if (redirect_type == ADVANCED){
                        outputFD = open(filename, O_RDWR, O_APPEND);
                        int l = lseek(outputFD, 0, SEEK_END);
                        if (l < 0){
                            myErrorPrint();
                            exit(1);
                        }
                        tempFD = open("temp", O_RDWR);

                        // append the temp file to the end (adding old data)
                        int written;
                        while ((bytes_read = read(tempFD, file_buff, 512)) > 0){
                            written = write(outputFD, file_buff, bytes_read);
                            if (written < 0){
                                myErrorPrint();
                                exit(1);
                            }
                        }

                        close(tempFD);
                        close(outputFD);

                        int r = remove("temp");
                        if (r < 0){
                            myErrorPrint();
                            exit(1);
                        }
                    }
                }
                else { // If fork() failed, ret<0
                    myErrorPrint();
                    exit(1);
                }
            
            }
        }
    }
}