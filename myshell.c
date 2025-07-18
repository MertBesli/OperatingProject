#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <signal.h>
#include <dirent.h>
#include <stdbool.h>
#include <fcntl.h>
#include <limits.h>              
#include <libgen.h>
#include <sys/wait.h>
#include <limits.h>  
#include <sys/stat.h> 
#include <string.h>
 
#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
pid_t foregroundprocesspid;

struct bookmark {
    int id;
    char *cname[41];
    struct bookmark* nextcommand;
}; 

struct backproc {
    char *cname[41];
    pid_t pid;
    //int procnumber;
    struct backproc* nextcommand;
};

void addcommandinbookmark(struct bookmark** head,char *com[]) {
    struct bookmark* newNode = (struct bookmark*)malloc(sizeof(struct bookmark));

    int i = 0;
    while (com[i] != NULL) {
        newNode->cname[i] = strdup(com[i]); 
        i++;
    }
    
    newNode->cname[i] = NULL; 
    //newNode->nextcommand = NULL;

    if (*head == NULL) {
        *head = newNode; // if the list is empty.
    } 
    else {
        struct bookmark* temp1 = *head;
        while(temp1->nextcommand != NULL){
            temp1 = temp1->nextcommand;
        }
        temp1->nextcommand = newNode;
        
    }
    int m = 0;
    struct bookmark* temp2 = *head;
    while (temp2 != NULL) {
        temp2->id = m;
        temp2 = temp2->nextcommand;
        m++;
    }
}

void deletecommandinbookmark(struct bookmark** head,int x) {
    
    if(*head == NULL){
        return;
    }
    
    struct bookmark* temp = *head;
    struct bookmark* prev = NULL;
    struct bookmark* next = NULL;
    
    if(x == 0){
        *head = temp->nextcommand;
        free(temp);
        int m = 0;
        struct bookmark* temp2 = *head;
        while (temp2 != NULL) {
            temp2->id = m;
            temp2 = temp2->nextcommand;
            m++;
        }
       
        return;
    }
    
    while(temp->id != x && temp != NULL){
        prev = temp;
        temp = temp->nextcommand;
    }
    //prev->nextcommand = temp;
    //temp->nextcommand = next;
    //free(temp);
    //next = prev->nextcommand;
    
    if(prev != NULL){
        prev->nextcommand = temp->nextcommand;
    }
    free(temp);
    
    int m = 0;
    struct bookmark* temp2 = *head;
    while (temp2 != NULL) {
        temp2->id = m;
        temp2 = temp2->nextcommand;
        m++;
    }
}

void printbookmark(struct bookmark* head) {
    struct bookmark* temp = head;
    while (temp != NULL) {
        printf("  %d ", temp->id);
        int i = 0;
        while (temp->cname[i] != NULL) {
            printf("%s ", temp->cname[i]);
            i++;
        }
        printf("\n");
        temp = temp->nextcommand;
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void addbackgroundprocess(struct backproc** head,char *com[],pid_t pid) {
    struct backproc* newNode = (struct backproc*)malloc(sizeof(struct backproc));

    int i = 0;
    while (com[i] != NULL) {
        newNode->cname[i] = strdup(com[i]); 
        i++;
    }
    
    newNode->cname[i] = NULL; 
    //newNode->nextcommand = NULL;
    newNode->pid = pid;

    if (*head == NULL) {
        *head = newNode; // if the list is empty.
    } 
    else {
        struct backproc* temp1 = *head;
        while(temp1->nextcommand != NULL){
            temp1 = temp1->nextcommand;
        }
        temp1->nextcommand = newNode;
        
    }
    /*int m = 0;
    struct backproc* temp2 = *head;
    while (temp2 != NULL) {
        temp2->procnumber = m;
        temp2 = temp2->nextcommand;
        m++;
    }*/
}

void removebackgroundprocess(struct backproc** head,pid_t x){
    
    struct backproc* temp = *head;
    struct backproc* prev = NULL;
    
    if(temp->pid == x){
        *head = temp->nextcommand;
        free(temp);
        return;
    }
    
    while(temp->pid != x && temp != NULL){
        prev = temp;
        temp = temp->nextcommand;
    }
    
    if(prev != NULL){
        prev->nextcommand = temp->nextcommand;
    }
    free(temp);
}

void checkbackgroundprocess(struct backproc** head){

    if(*head == NULL){
        return;
    }
    
    struct backproc* temp = *head;

    while(temp != NULL){
        int status;
        pid_t terminatedPID = waitpid(temp->pid,&status,WNOHANG);
        if(terminatedPID > 0){//bitmiÅŸ process kaldÄ±r
            removebackgroundprocess(head,temp->pid);
        }
        temp = temp->nextcommand;
    }
    
}

void printbackgroundprocess(struct backproc* head) {
    struct backproc* temp = head;
    while (temp != NULL) {
        printf("  %d ", temp->pid);
        int i = 0;
        while (temp->cname[i] != NULL) {
            printf("%s ", temp->cname[i]);
            i++;
        }
        printf("\n");
        temp = temp->nextcommand;
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void StopAllChilProcess(pid_t parent_pid) {
    //This function stops all child processes of foreground process.
    pid_t child_pid;
    int status;

    while ((child_pid = waitpid(parent_pid, &status, WNOHANG)) > 0) {
        //All child process terminated.
        if (kill(child_pid, SIGTERM) == -1) {
            perror("kill");
        }
    }
}

void ControlZSignalCatcher(){//This method is for ^Z signal
    if (foregroundprocesspid == 0 || waitpid(foregroundprocesspid, NULL, WNOHANG) == -1)//if no foreground process///////87987987987
    {
        printf("\nmyshell: ");
        fflush(stdout);
        return;
    }

    StopAllChilProcess(foregroundprocesspid);
    
    if(kill(foregroundprocesspid,SIGTERM) == -1){//foreground process is stopped
        perror("kill");
    }

    foregroundprocesspid = 0;
    
    printf("\n");
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//********************************************************************************************************************************
char *find_command_path(char *command) {
    //**
    if (strncmp(command, "./", 2) == 0) {
        return command;
    }
    //**
    char *path = getenv("PATH");
    if (path == NULL) {
        fprintf(stderr, "PATH environment variable not found.\n");
        return NULL;
    }

    char *path_copy = strdup(path);
    if (path_copy == NULL) {
        perror("Memory allocation error");
        return NULL;
    }

    char *token = strtok(path_copy, ":");

    while (token != NULL) {
        char *full_path = malloc(strlen(token) + strlen(command) + 2);
        if (full_path == NULL) {
            perror("Memory allocation error");
            free(path_copy);
            return NULL;
        }

        strcpy(full_path, token);
        strcat(full_path, "/");
        strcat(full_path, command);

        if (access(full_path, F_OK) != -1) {
            free(path_copy);
            return full_path;
        }

        free(full_path);
        token = strtok(NULL, ":");
    }

    free(path_copy);
    return NULL;
}
//********************************************************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */

void setup(char inputBuffer[], char *args[],int *background){

    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */
    
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

	//printf(">>%s<<",inputBuffer);
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

	//for (i = 0; i <= ct; i++)
		//printf("args %d = %s\n",i,args[i]);
} /* end of setup routine */
 
int main(void){

            char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
            int background; /* equals 1 if a command is followed by '&' */
            char *args[MAX_LINE/2 + 1]; /*command line arguments */
            
            struct bookmark* head = NULL;
            struct backproc* starptr = NULL;
            
            signal(SIGTSTP,ControlZSignalCatcher);
            
            pid_t ParentPID;
            ParentPID = getpid();    
            
            while (ParentPID == getpid()){
                        background = 0;
                        printf("myshell: ");
                        fflush(stdout);
                        /*setup() calls exit() when Control-D is entered */
                        setup(inputBuffer, args, &background);
                        
                        if(args[0] == NULL){
                            continue;
                        }
                        /*else if(strcmp(args[0], "^Z") == 0){
                            
                        }*/
                        else if(strcmp(args[0], "search") == 0){
                        
                        }
                        else if(strcmp(args[0], "bookmark") == 0){  //fprintf(stderr,"");
                            if(strcmp(args[1], "-l") == 0){
                                if(args[2] != NULL){
                                    fprintf(stderr,"The bookmark command was used incorrectly.It should be used as bookmark -l\n");
                                    continue;
                                }
                                printbookmark(head);
                                continue;
                            }
                            else if(strcmp(args[1], "-d") == 0){
                                char *endptr;
                                long int number = strtol(args[2],&endptr,10);
                                if(endptr == args[2]){
                                    fprintf(stderr,"The bookmark command was used incorrectly.Third argument should be a number.\n");
                                    continue;   
                                }
                                if(args[3] != NULL){
                                    fprintf(stderr,"The bookmark command was used incorrectly.It should be used as bookmark -d number\n");
                                    continue;
                                }
                                deletecommandinbookmark(&head,atoi(args[2]));
                                continue;
                            }
                            else if(strcmp(args[1], "-i") == 0){
                                char *endptr;
                                long int number = strtol(args[2],&endptr,10);
                                if(endptr == args[2]){
                                    fprintf(stderr,"The bookmark command was used incorrectly.Third argument should be a number.\n");
                                    continue;   
                                }
                                if(args[3] != NULL){
                                    fprintf(stderr,"The bookmark command was used incorrectly.It should be used as bookmark -i number\n");
                                    continue;
                                }
                                //run bookmark command
                                struct bookmark* temp = head;
                                while (temp != NULL) {
                                    if(temp->id == atoi(args[2])){
                                        int m = 0;
                                        while(m < 41){
                                            args[m] = NULL;
                                            m++;
                                        }
                                        int i = 0;
                                        while(temp->cname[i] != NULL){
                                            args[i] = temp->cname[i];
                                            i++;
                                        }
                                        break;
                                   }
                                   temp = temp->nextcommand;
                               }
                               printf("***%s %s %s***\n",temp->cname[0],temp->cname[1],temp->cname[2]);
                               int z = strlen(args[0]);
                               int x;
                               for(x = 0;x < z;x++){
                                   args[0][x] = args[0][x + 1];
                               }
                               int k = 0;
                               while(args[k] != NULL){
                                   k++;
                               }
                               int a = strlen(args[k - 1]);
                               args[k-1][a-1] = '\0';
                               printf("\n***%s %s %s***\n",temp->cname[0],temp->cname[1],temp->cname[2]);
                               goto label;     
                            }
                            else{
                                if(args[1][0] == '-' && !(args[1][1] == 'i' || args[1][1] == 'l' || args[1][1] == 'd')){
                                    fprintf(stderr,"The bookmark command was used incorrectly.Second argument should be -l or -d or -i\n");
                                    continue;
                                }
                                int x;
                                for(x = 0;x < 41;x++){
                                    args[x] = args[x + 1];
                                }
                                addcommandinbookmark(&head,args);
                                continue;
                            }
                        }
                        else if(strcmp(args[0], "exit") == 0){
                            checkbackgroundprocess(&starptr);//background processes are checked and finished processes are removed from the list
                            //printbackgroundprocess(starptr);
                            if(starptr == NULL){
                                exit(1);
                            }
                            else{
                                printf("There are background processes still running.\n");
                                continue;
                            }
                        }
                        else{
                            //////***********************************
                            int t = 0;
                            while(args[t] != NULL){
                                t++;
                            }//if the last character is & it removes it
                            if(strcmp(args[t - 1], "&") == 0){
		                args[t - 1] = NULL;
		            }
                            //////***********************************
                            label:
                            pid_t pid = fork();

                            if(pid < 0){
                                perror("Fork failed");
                                exit(EXIT_FAILURE);
                            }
                            else if(pid == 0){//Child process
                                if(execv(find_command_path(args[0]), args) == -1){
                                perror("Execv failed");
                                exit(EXIT_FAILURE);}
                            }
                            else{//Parent process
                                if(background == 1){
                                    addbackgroundprocess(&starptr,args,pid);
                                }
                                else{
                                    foregroundprocesspid = pid;
                                }
                                if(!background){
                                    // Wait for the child process to complete in the foreground
                                    waitpid(pid, NULL, 0);
                                }
                                // If background is true, don't wait and immediately prompt for another command
                            }
                            //printbackgroundprocess(starptr);
                        }
                                  
                        /** the steps are:
                        (1) fork a child process using fork()
                        (2) the child process will invoke execv()
			(3) if background == 0, the parent will wait,
                        otherwise it will invoke the setup() function again. */
            }
}
