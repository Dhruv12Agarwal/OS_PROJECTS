#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/ioctl.h>


static volatile sig_atomic_t shutdown_requested = 0;
static volatile sig_atomic_t in_handler = 0;

typedef struct Node {
  pid_t pid;
  struct Node* next;
  int wait_time;
  int runtime;

} Node;

struct process_stats {
  pid_t pid;
  int wait_time;
  int runtime;
};

int tslice;
int bytes_available;
Node* head = NULL;
Node* tail = NULL;
pid_t* cpus = NULL;
int ncpu;
int fd;
int wfd;

int* wait_times = NULL;
int* runtimes = NULL;

 Node* createNode(pid_t pid, int wait_time, int runtime) {
   Node* node = ( Node*)malloc(sizeof(Node));
  if (node == NULL){
    perror("malloc");
    exit(1);
  }
  node->pid = pid;
  node->next = NULL;
  node->wait_time = wait_time;
  node->runtime = runtime;
  return node;

}

void add(pid_t pid, int wait_time, int runtime) {
   Node* temp = createNode(pid,wait_time, runtime);

   if (head == NULL) {
     head = temp;
     tail = temp;
   } else {
     tail->next = temp;
     tail = temp;
   }
 }

pid_t pop(){
  if (head == NULL){
    return -1;
  }
  Node * temp=head;
  head = head->next;
  if (head == NULL){
    tail = NULL;
  }
  pid_t pid = temp->pid;
  free(temp);

  return pid;
}

void schedule(int signum){
   (void)signum;

   if (__sync_lock_test_and_set(&in_handler, 1)) {
     return;
   }
   if (ioctl(fd, FIONREAD, &bytes_available) == -1) {
     perror("ioctl");
     exit(1);
   }

   while (bytes_available > 0) {
     pid_t pid;
     if (read(fd, &pid, sizeof(pid)) < 0) {
       perror("read from pipe failed");
       break;
     }
     add(pid,0,0);
     if (ioctl(fd, FIONREAD, &bytes_available) == -1) {
       perror("ioctl");
       exit(1);
     }

   }

   Node* temp = head;
   while (temp != NULL){
     temp->wait_time+=1;;
     temp = temp->next;
   }

  int flag =1;
  for (int i = 0; i < ncpu; i++) {
    if(cpus[i]>0){
      runtimes[i]+=1;
      if(kill(cpus[i], SIGSTOP)==0){
        add(cpus[i],wait_times[i],runtimes[i]);
      }
      else{
        struct process_stats final_stats;
        final_stats.pid = cpus[i];
        final_stats.wait_time = wait_times[i];
        final_stats.runtime = runtimes[i];
        if (write(wfd, &final_stats, sizeof(struct process_stats)) == -1) {
          perror("Failed to write to pipe");
        }
      }
    }
    wait_times[i]=0;;
    cpus[i]=-1;
  }

  for (int i = 0; i < ncpu; i++) {
    if (head == NULL){
      break;
    }
    wait_times[i] = head->wait_time;
    runtimes[i] = head->runtime;

    pid_t pid=pop();


    if (pid!=-1){

      cpus[i]=pid;
      kill(cpus[i], SIGCONT);
    }
    else { break; }
  }

   __sync_lock_release(&in_handler);

}

void end(int signum) {
    (void)signum;
    shutdown_requested = 1;
}

int main(int argc,char** argv){

    ncpu = atoi(argv[1]);
    tslice = atoi(argv[2]);
    fd = atoi(argv[3]);
    wfd= atoi(argv[4]);

    cpus=(pid_t *)malloc(sizeof(pid_t)*ncpu);
    if (cpus == NULL){
        perror("malloc for cpus");
        exit(1);
    }
    // You also need to allocate memory for your stats arrays
    wait_times = malloc(sizeof(int) * ncpu);
    runtimes = malloc(sizeof(int) * ncpu);
    if (wait_times == NULL || runtimes == NULL) {
        perror("malloc for stats");
        exit(1);
    }


    for (int i = 0; i < ncpu; i++) {
        cpus[i] = -1;
        wait_times[i] = 0;
        runtimes[i] = 0;
    }
    signal(SIGALRM, schedule);
    signal(SIGINT, end);

    struct itimerval timer;
    timer.it_value.tv_sec = tslice / 1000;
    timer.it_value.tv_usec = (tslice % 1000) * 1000;
    timer.it_interval = timer.it_value;
    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("setitimer failed");
        exit(1);
    }


    while (!shutdown_requested) {
        pause();
    }


    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 0;
    timer.it_interval = timer.it_value;
    setitimer(ITIMER_REAL, &timer, NULL);

    for (int i = 0; i < ncpu; i++) {
        if (cpus[i] > 0) {
            kill(cpus[i], SIGKILL);


            struct process_stats final_stats;
            final_stats.pid = cpus[i];
            final_stats.wait_time = wait_times[i];
            runtimes[i]++;
            final_stats.runtime = runtimes[i];
            if (write(wfd, &final_stats, sizeof(struct process_stats)) == -1) {
                perror("Shutdown: Failed to write CPU stats to pipe");
            }
        }
    }


    while (head != NULL) {
        Node* node_to_free = head;
        kill(head->pid, SIGKILL);
        struct process_stats final_stats;
        final_stats.pid = head->pid;
        final_stats.wait_time = head->wait_time;
        final_stats.runtime = head->runtime;
        if (write(wfd, &final_stats, sizeof(struct process_stats)) == -1) {
            perror("Shutdown: Failed to write queue stats to pipe");
        }

        head = head->next;
        free(node_to_free);
    }

    close(fd);
    close(wfd);
    free(cpus);
    free(wait_times);
    free(runtimes);

    return 0;
}