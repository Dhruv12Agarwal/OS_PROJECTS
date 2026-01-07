#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<signal.h>
#include<sys/types.h>
#include<sys/wait.h>

int sched(int fd_,int ncpu,int tslice);//to move to scheduler
typedef struct {
    pid_t pid;
    int wait_time;
    int runtime;
}stats;
int main(int argc,char**argv){
    if(argc !=3){
        fprintf(stderr,"3 arguments expected(name,ncpu,tslice)\n");
        return 1;
    }
    int ncpu=atoi(argv[1]);
    int tslice=atoi(argv[2]);
    if(ncpu<=0||tslice<=0){
        fprintf(stderr,"ncpu and tslice should be positive\n");
        return 1;
}
signal(SIGPIPE, SIG_IGN);
int fd[2];
int f[2];
if(pipe(fd)==-1||pipe(f)==-1){
    fprintf(stderr,"pipe error\n");
    return 1;
}
pid_t sch=fork();
if(sch<0){
    fprintf(stderr,"fork error\n");
    return 1;
}
if(sch==0){
    close(fd[1]);
    close(f[0]);
    char ncpu_[10];
    char tslice_[10];
    char readend[10];
    char writeend[10];

    sprintf(ncpu_,"%d", ncpu);
    sprintf(tslice_,"%d", tslice);
    sprintf(readend,"%d", fd[0]);
    sprintf(writeend,"%d", f[1]);
    char*args[]={"./scheduler",ncpu_,tslice_,readend,writeend,NULL};
    execv("./scheduler",args);
    fprintf(stderr,"execv error\n");
    _exit(1);
}
close(fd[0]);
close(f[1]);
char cmd[200];

while(1){
    printf("SimpleShell ");
    if(!fgets(cmd,sizeof(cmd),stdin)){
        close(fd[1]);
        waitpid(sch,NULL,0);
        break;
    }
    int x=strcspn(cmd,"\n");
    cmd[x]='\0';
    if(!cmd[0]){
        continue;
    }
    if(strcmp(cmd,"exit")==0){
        close(fd[1]);
        kill(sch, SIGINT);

        printf("Printing stats");
        stats s;
        while(read(f[0],&s,sizeof(s))>0){
            printf("PID: %d, wait time: %d, runtime: %d \n",s.pid,s.wait_time*tslice,s.runtime*tslice);
        }
        close(f[0]);
        waitpid(sch,NULL,0);
        break;
    }
    if(!strncmp(cmd,"submit",6)){
        char*c=strtok(cmd," ");
        char*add=strtok(NULL," ");
        char*args[]={add,NULL};
        if(!add||strtok(NULL," ")){
            fprintf(stderr,"Provide just the address after submit\n");
            continue;
        }
        pid_t pi=fork();
        if(pi<0){
            fprintf(stderr,"fork error\n");
            continue;}
        if(pi==0){
            execv(add,args);
            fprintf(stderr,"execv error\n");
            _exit(1);
        }
        if(write(fd[1],&pi,sizeof(pi))==-1){
            fprintf(stderr,"write error\n");
            continue;
        }
        continue;
    }
    printf("Invalid command\n");
}
return 0;
}
