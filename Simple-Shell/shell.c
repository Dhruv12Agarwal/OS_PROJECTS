#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/time.h>
#include<time.h>
#include<signal.h>

char*his[100];
int count=0;
struct timeval start_time[100];
long duration[100];
pid_t pid[100];
void print(){
    printf("Summary:\n");
    for(int i=0;i<count;i++){
        char*t=ctime(&start_time[i].tv_sec);
        t[strlen(t)-1]='\0';
    printf("pid= %d  start= %s  duration= %ld ms command= %s\n",(int)pid[i],t,duration[i],his[i]);
}
}
void ctrlcExit(int sig){
    printf("\n");
    print();
    exit(0);
}

int main(){
    char cmd[256];
    char kill[]="^C";

    signal(SIGINT,ctrlcExit);
    do{
        printf("dhruv-abhishek-shell> ");
        fgets(cmd,sizeof(cmd),stdin);
        size_t l=strlen(cmd);
        if(l>0&&cmd[l-1]=='\n'){
            cmd[l-1]='\0';
        }
        if(cmd[0]=='\0'){
            continue;
        }
        int index=-1;
        if(count<100){
            size_t len=strlen(cmd)+1;
            his[count]=(char*)malloc(len);
            if (his[count] == NULL) { printf("malloc"); continue; }
            if(his[count]!=NULL){
                strcpy(his[count],cmd);
                index=count;
                count++;
            }
        }
        struct timeval s,e;
        pid_t cpid=-1;

        if (strcmp(cmd,kill)==0){
            break;
        }
        if(strcmp(cmd,"exit")==0){
            break;}
        if(strcmp(cmd,"history")==0){
            if(index!=-1){
                free(his[index]);
                count--;
            }
            for(int i=0;i<count;i++){
            printf("%s\n",his[i]);
            }
            continue;
        }
        else if(strchr(cmd,'"') ||strchr(cmd,'\\')) {
            printf("Invalid Character detected\n");
            free(his[index]);
            count--;
            continue;
        }

        char *pip=strchr(cmd,'|');
        if (pip!=NULL){
            pid_t last=-1;
            if(index!=-1){
                gettimeofday(&s,NULL);
            }
            char *commands[11];
            int ncmds=0;
            char *p=cmd;
            while (*p &&ncmds<10) {
                while(*p==' ') {
                    p++;}
                if (*p=='\0') {
                    break;
                }
                commands[ncmds++] =p;
                while (*p &&*p !='|') {
                    p++;
                }
                if (*p =='|') {
                    *p='\0';
                    p++;
                }
            }
            commands[ncmds] =NULL;
            int prev_fd[2]={-1,-1};
            int fd[2];
            int ind =0;

            while (commands[ind]!=NULL) {
                if (commands[ind+1] !=NULL) {
                    pipe(fd);
                }
                cpid=fork();
                if (cpid==-1) {
                    free(his[index]);
                    count--;
                    printf("fork error");
                    exit(1);
                }
                if(cpid==0){
                    if(ind>0){

                        if (dup2(prev_fd[0], STDIN_FILENO)==-1) {
                            printf("dup2");
                            exit(1);
                        }
                        close(prev_fd[0]);
                    }
                    if(commands[ind+1] !=NULL) {
                        dup2(fd[1],STDOUT_FILENO);
                        close(fd[0]);
                        close(fd[1]);
                    }
                    char *args[256];
                    int k=0;
                    int i=0;
                    while(commands[ind][i]!='\0') {
                        while(commands[ind][i]==' '){
                            i++;
                        }
                        if(commands[ind][i]=='\0') {
                            break;
                        }
                        char *start = &commands[ind][i];
                        while (commands[ind][i]!=' ' &&commands[ind][i]!='\0') {
                            i++;}
                        if(commands[ind][i]!='\0') {
                            commands[ind][i]= '\0';
                            i++;
                        }
                        args[k++]=start;
                    }
                    args[k]=NULL;
                    execvp(args[0],args);
                    printf("execv error");
                    exit(1);
                }
                if(ind>0) {
                    close(prev_fd[0]);
                }
                if (commands[ind+1]!=NULL) {
                    close(fd[1]);
                    prev_fd[0]=fd[0];
                }
                if(commands[ind+1]==NULL){
                    last=cpid;
                }
                ind++;}
            if(prev_fd[0]!=-1){
                close(prev_fd[0]);
            }
            for (int j=0; j< ind;j++) {
                wait(NULL);
            }
            if(index!=-1){
                gettimeofday(&e,NULL);
                long t=(e.tv_sec-s.tv_sec)*1000;
                start_time[index]=s;
                duration[index]=t;
                pid[index]=last;
            }
            continue;
        }
        char* args[256];

        int k=0;
        int i=0;
        while (cmd[i]!='\0') {
            while (cmd[i]== ' ') {
                i++;
            }
            if (cmd[i]=='\0'){
                break;}
            char *start = &cmd[i];
            while (cmd[i]!= ' '&& cmd[i] !='\0') {
                i++;}
            if (cmd[i] != '\0') {
                cmd[i] = '\0';
                i++;
            }
            args[k++]=start;
        }
        args[k]=NULL;
        if(index!=-1){
            gettimeofday(&s,NULL);}
            cpid=fork();
            if (cpid == -1){
                free(his[index]);
                count--;
                printf("fork erroe");
                continue;
                }
            if(cpid==0){
                execvp(args[0],args);
                printf("execv error");
                exit(1);
            }
            else{
                wait(NULL);
                if(index!=-1){
                    gettimeofday(&e,NULL);
                    long t=(e.tv_sec-s.tv_sec)*1000;
                    start_time[index]=s;
                    duration[index]=t;
                    pid[index]=cpid;
                }
            }
        }
     while(1);
       print();
    for(int i=0;i<count;i++){
        free(his[i]);
    }
    return 0;
}
