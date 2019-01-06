#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define CREATE_FLAGS_OUT (O_WRONLY | O_CREAT | O_TRUNC)
#define CREATE_FLAGS_OUT2 (O_WRONLY | O_CREAT | O_APPEND)
#define CREATE_FLAGS_IN (O_RDONLY)
#define CREATE_MODE_OUT (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define CREATE_MODE_IN (S_IRUSR | S_IRGRP | S_IROTH)
int ct;

/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */
struct Node { //A linked list node for storing the alias list
    char cmd_name[255];
    char *real_cmd[255];
    struct Node *next;
};

//Appends a new node at the end of linked list
void append(struct Node** head_ref, char fake[], char *real[]){
    //Allocate node
    struct Node* new_node = (struct Node*) malloc(sizeof(struct Node));
    struct Node *last = *head_ref;  //used in step 5

    //Put in the alias name
    new_node->cmd_name[0]='\0';
    strcpy(new_node->cmd_name, fake);

    //Put in the alias command
    int c;
    for(c=0;real[c]!=NULL;c++){
        new_node->real_cmd[c]=(char*)malloc(sizeof(char)*50);
        new_node->real_cmd[c][0]='\0';
        strcpy(new_node->real_cmd[c], real[c]);
    }
    new_node->real_cmd[c]=NULL;

    //Make next of the added node NULL
    new_node->next = NULL;

    //If the linked list is empty, then make the new node is head
    if (*head_ref == NULL){
        *head_ref = new_node;
        return;
    }

    //Else traverse till the last node
    while (last->next != NULL)
        last = last->next;

    //Change the next of last node
    last->next = new_node;
}

char **getRealCmds(struct Node** head, char *nickname){
    //If the linked list is empty, then return NULL
    if (*head == NULL)
        return NULL;

    //Create a pointer to travel on the linked list
    struct Node *iterPtr = *head;

    //Travel on the linked list and find the node which has that alias name
    while (iterPtr != NULL) {
        if (!strcmp(iterPtr->cmd_name, nickname)) {
            return iterPtr->real_cmd;
        }
        iterPtr = iterPtr->next;
    }
    return NULL;
}

//Function for splitting a string according to a particular deliminator
char** str_split(char* a_str, const char a_delim) {

    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_delim = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    //Count how many elements will be extracted
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_delim = tmp;
        }
        tmp++;
    }

    //Add space for trailing token
    count += last_delim< (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result) {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);
        while (token){
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }
    return result;
}

void setup(char inputBuffer[], char *args[], int *background) {
    int length, /* # of characters in the command line */
            i,      /* loop index for accessing inputBuffer array */
            start;  /* index where beginning of next command parameter is */
     //       ct;     /* index of where to place the next parameter into args[] */

    ct = 0;

    /* read what the user enters on the command line */
        length = read(STDIN_FILENO,inputBuffer,MAX_LINE);


    /* 0 is the system predefined file descriptor for stdin (standard input),
       which is the user's screen in this case. inputBuffer by itself is the
       same as &inputBuffer[0], i.e. the starting address of where to store
       the command that is read, and length holds the number of characters
       read in. inputBuffer is not a null terminated C-string. */

    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */

/* the signal interrupted the read system call */
/* if the process is in the read() system call, read returns -1
  However, if this occurs, errno is set to EINTR. We can check this  value
  and disregard the -1 value */
    if ( (length < 0) && (errno != EINTR) ) {
        perror("error reading the command");
        exit(-1);           /* terminate with error code of -1 */
    }

    printf(">>%s<<",inputBuffer);
    for (i=0;i<length;i++){ /* examine every character in the inputBuffer */

        switch (inputBuffer[i]){
            case ' ':
            case '\t' :               /* argument separators */
                if(start != -1){
                    args[ct] = &inputBuffer[start];    /* set up pointer */
                    ct++;
                }
                inputBuffer[i] = '\0'; /* add a null char; make a C string */
                start = -1;
                break;

            case '\n':                 /* should be the final char examined */
                if (start != -1){
                    args[ct] = &inputBuffer[start];
                    ct++;
                }
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
                break;

            default :             /* some other character */
                if (start == -1)
                    start = i;
                if (inputBuffer[i] == '&'){
                    *background  = 1;
                    inputBuffer[i-1] = '\0';
                }
        } /* end of switch */
    }    /* end of for */
    args[ct] = NULL; /* just in case the input line was > 80 */

    for (i = 0; i <= ct; i++)
        printf("args %d = %s\n",i,args[i]);
} /* end of setup routine */

//Function for removing a particular character in a string
void RemoveChars(char *s, char c) {
    int writer = 0, reader = 0;

    while (s[reader]) {
        if (s[reader]!=c) {
            s[writer++] = s[reader];
        }
        reader++;
    }
    s[writer]=0;
}

//Function for calling execl()
void run(char *args[], int background, int backgroundpid[]){
    //Check if the process is background
    if(strcmp(args[ct-1],"&")==0){
        args[ct-1]= NULL;
        background=1;
    }

    pid_t childpid = 0;
    int j = 0;
    char filePath[50];
    char tokens_copy[100];
    char **tokens;
    char s[100];
    s[0] = '\0';

    //Find the file path for the calling process
    strcpy(s, getenv("PATH"));
    tokens = str_split(s, ':');
    fflush(stdin);
    fflush(stdout);

    if (tokens) {
        for (int t = 0; *(tokens + t); t++) {
            memset(&tokens_copy[0], 0, sizeof(tokens_copy));
            memset(&filePath[0], 0, sizeof(filePath));
            strcpy(tokens_copy, *(tokens + t));
            strcat(tokens_copy, "/");
            strcat(tokens_copy, args[0]);

            if (access(tokens_copy, F_OK) != -1) {
                strcpy(filePath, tokens_copy);
                break;
            }
            free(*(tokens + t));
        }
        printf("\n");
        free(tokens);
    }

    //Create a child process for executing the commands
    childpid = fork();
    if (childpid == -1) {
        perror("Fork failed");
    }
    else if (childpid == 0) {
        execl(filePath, args[0], args[1], args[2], args[3], args[4], NULL);
        perror("Child failed when running execl");
        exit(1);

    }
    else { //If there is a background process, store their pids in an array
        if (background == 0) {
            if (waitpid(childpid,NULL,0)<0) {
                perror("Parent failed to wait due to signal or error");
            }
        }
        else {
            while (j < 100) {
                if (backgroundpid[j] == -1) {
                    backgroundpid[j] = childpid;
                    printf("%d\n",backgroundpid[j]);
                    break;
                }
                j++;
            }
        }
    }
}

void delete(struct Node **head_ref, char *nickname){
    //Store head node
    struct Node* temp = *head_ref;
    struct Node *prev = NULL;

    //If head node itself holds the key to be deleted
    if (temp != NULL && strcmp(temp->cmd_name, nickname)==0)
    {
        *head_ref = temp->next;   // Changed head
        free(temp);               // free old head
        return;
    }

    // Search for the key to be deleted, keep track of the
    // previous node as we need to change 'prev->next'
    while (temp != NULL && strcmp(temp->cmd_name, nickname)!=0)
    {
        prev = temp;
        temp = temp->next;
    }

    // If key was not present in linked list
    if (temp == NULL)
        return;

    prev->next = temp->next;

    free(temp);  // Free memory
}
void display(struct Node **start) {
    struct Node *temp;
    temp = *start;

    //Display the alias list
    while(temp!=NULL) {
        printf("%s  ",temp->cmd_name);
        printf("\"");
        for(int r=0;temp->real_cmd[r]!=NULL;r++){
            printf("%s",temp->real_cmd[r]);
        }
        printf("\"\n");
        temp=temp->next;
    }
}

void redirection(char *redirectArgs[], int background, int backgroundpid[], int flag, char fileName[], char fileName2[]){
    if (flag==1) { //For redirecting the INPUT
        int fd;
        fd = open(fileName, CREATE_FLAGS_IN, CREATE_MODE_IN);
        if (fd == -1) {
            perror("Failed to open file");
            return;
        }
        if (dup2(fd, STDIN_FILENO) == -1) {
            perror("Failed to redirect standard input");
            return;
        }
        if (close(fd) == -1) {
            perror("Failed to close the file");
            return;
        }

    } else if (flag==2) { //For redirecting the OUTPUT and truncat the file
        int fd;
        fd = open(fileName, CREATE_FLAGS_OUT, CREATE_MODE_OUT);
        if (fd == -1) {
            perror("Failed to open file");
            return;
        }
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("Failed to redirect standard output");
            return;
        }
        if (close(fd) == -1) {
            perror("Failed to close the file");
            return;
        }
    } else if (flag==3) { //For redirecting the OUTPUT and append to the file
        int fd;
        fd = open(fileName, CREATE_FLAGS_OUT2, CREATE_MODE_OUT);
        if (fd == -1) {
            perror("Failed to open file");
            return;
        }
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("Failed to redirect standard output");
            return;
        }
        if (close(fd) == -1) {
            perror("Failed to close the file");
            return;
        }
    } else if (flag==4) { //For redirecting the ERROR MESSAGE
        int fd;
        fd = open(fileName, CREATE_FLAGS_OUT, CREATE_MODE_OUT);
        if (fd == -1) {
            perror("Failed to open file");
            return;
        }
        if (dup2(fd, STDERR_FILENO) == -1) {
            perror("Failed to redirect standard error");
            return;
        }
        if (close(fd) == -1) {
            perror("Failed to close the file");
            return;
        }
    } else if (flag==5) { //For redirecting the INPUT and OUTPUT both
                int fdin, fdout;
                fdin = open(fileName, CREATE_FLAGS_IN, CREATE_MODE_IN);
                fdout = open(fileName2, CREATE_FLAGS_OUT, CREATE_MODE_OUT);
                if (fdin == -1 || fdout == -1) {
                    perror("Failed to open file");
                    return;
                }
                if (dup2(fdin, STDIN_FILENO) == -1) {
                    perror("Failed to redirect standard input");
                    return;
                }
                if (dup2(fdout, STDOUT_FILENO) == -1) {
                    perror("Failed to redirect standard output");
                    return;
                }
                if (close(fdin) == -1 || close(fdout) == -1) {
                    perror("Failed to close the file");
                    return;
                }
            }
    run(redirectArgs, background, backgroundpid); //Run the process to redirection
}
int main(void) {
    char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
    int background; /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE/2 + 1]; /*command line arguments */

    char nickname[255];
    char *redirectArgs[255];
    char fileName[255];
    char fileName2[255];
    pid_t childpid2 = 0;
    struct Node* head = NULL;
    int b = 0;
    int backgroundpid[100];

    while (b<100){
        backgroundpid[b]=-1;
        b++;
    }

    while (1) {
        memset(inputBuffer, '\0', sizeof(inputBuffer));
        background = 0;
        printf("myshell: ");
        fflush(stdin);
        fflush(stdout);

        /*setup() calls exit() when Control-D is entered */
        setup(inputBuffer, args, &background);

        char *between_quotes[255];
        memset(between_quotes, '\0', sizeof(between_quotes));
        char *commands[255];
        memset(commands, '\0', sizeof(commands));
        char fileIn[255];
        memset(fileIn, '\0', sizeof(fileIn));
        char fileOut[255];
        memset(fileOut, '\0', sizeof(fileOut));
        fflush(stdin);
        fflush(stdout);

        int flag=0;
        int indexIn=0;
        fileName[0]='\0';
        fileName2[0]='\0';

        //Checking the command line for redirection
        for (int l = 0; args[l]!=NULL; l++){
            if((strcmp(args[l], "<") == 0)){ //For INPUT redirection
                flag=1;
                indexIn=l;
                strcpy(fileName,args[ct-1]);
                for (int k = l; k < ct; k++){
                    if((strcmp(args[k], ">") == 0)){ //For INPUT and OUTPUT redirection both
                        flag=5;
                        strcpy(fileName,args[indexIn+1]);
                        strcpy(fileName2,args[ct-1]);
                        break;
                    }
                }
                break;
            }
            else if((strcmp(args[l], ">") == 0)){ //For OUTPUT redirection with truncation
                flag=2;
                strcpy(fileName,args[ct-1]);
                break;
            }
            else if((strcmp(args[l], ">>") == 0)){ //For OUTPUT redirection with append
                flag=3;
                strcpy(fileName,args[ct-1]);
                break;
            }
            else if((strcmp(args[l], "2>") == 0)){ //For ERROR redirection
                flag=4;
                strcpy(fileName,args[ct-1]);
                break;
            }
            //Commands for redirection
            redirectArgs[l] = (char *) malloc(sizeof(char) * 255);
            redirectArgs[l][0] = '\0';
            strcpy(redirectArgs[l],args[l]);
        }

        //If alias is entered
        if (strcmp(args[0], "alias") == 0){
            //Displaying the alias list
            if(strcmp(args[1], "-l") == 0){
                display(&head);
            }
            else{
                for (int n = 0; n < ct; n++) {
                    RemoveChars(args[n], '"');
                    fflush(stdin);
                    fflush(stdout);
                }
                int k = 0;
                for (int j = 1; j < (ct - 1); j++) {
                    between_quotes[k] = (char *) malloc(sizeof(char) * 50);
                    between_quotes[k][0] = '\0';
                    strcpy(between_quotes[k], args[j]);
                    k++;
                }
                between_quotes[k] = NULL;
                strcpy(nickname, args[ct - 1]);

                append(&head, nickname, between_quotes);
                continue;
            }
        }
        //If unalias is entered
        else if (strcmp(args[0], "unalias") == 0) {
            delete(&head,args[1]);
        }
        //If clr is entered
        else if (strcmp(args[0], "clr") == 0) {
            system("clear");

        }
        //If fg is entered
        else if (strcmp(args[0], "fg") == 0) {
            for(int i=0;backgroundpid[i]!=-1;i++){
                if(waitpid(backgroundpid[i], NULL, 0)>0)
                backgroundpid[i] = -1;
            }
        }
        //If exit is entered
        else if (strcmp(args[0], "exit") == 0) {
            //Checking for the background processes
            if(backgroundpid[0]==-1){
                exit(0);
            }
            else
                perror("There are background processes still running. Cannot terminate.\n");
        }
        //If there is a redirection, call the 'redirection' function in a child process
        else if(flag!=0){
            childpid2 = fork();
            if (childpid2 == -1) {
                perror("Fork failed");
            } else if (childpid2 == 0) {
                redirection(redirectArgs,background,backgroundpid,flag, fileName, fileName2);
                wait(NULL);
                exit(0);
            }
        }
        //If normal commands or alias fake name had entered
        else{
            char **returnVal = getRealCmds(&head, args[0]);
            if (returnVal != NULL) {
                run(returnVal, background, backgroundpid);
            }
            else {
                run(args, background, backgroundpid);
        }
    }


        /** the steps are:
        (1) fork a child process using fork()
        (2) the child process will invoke execv()
        (3) if background == 0, the parent will wait,
        otherwise it will invoke the setup() function again. */
    }
}
