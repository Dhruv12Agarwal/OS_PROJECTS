#include <iostream>
#include <list>
#include <functional>
#include <stdlib.h>
#include <cstring>
#include<time.h>
#include<pthread.h>

typedef struct{
int low;
int high;
std::function<void(int)>f;
}args1D;
typedef struct{
int low1;
int high1;
int low2;
int high2;
std::function<void(int,int)>f;
}args2D;
static void* th_func1d(void*para){//function performed (1d)
  args1D* args=(args1D*)para;
  for(int i=args->low;i<args->high;i++){
    args->f(i);//calls lambda
  }
  return nullptr;
}
static void* th_func2d(void*para){//function performed (2d)
  args2D* args=(args2D*)para;
  for(int i=args->low1;i<args->high1;i++) {
    for(int j=args->low2;j<args->high2;j++) {
    args->f(i,j);//calls lambda
  }
}
  return nullptr;
}
static void range_cal(int low,int high,int nth,int*start,int*end){
  if(nth<=0){
    nth=1;
  }
  int t=high-low;
  if(t>0){
    int base=t/nth;
    int r=t%nth;
    int x=low;
    for(int i=0;i<nth;i++){
      int z;
      if(i<r){
        z=1;
      }
      else{
        z=0;
      }
      int len=base+z;
      start[i]=x;
      end[i]=x+len;
      x+=len;
    }}
  if(t<=0){
    for(int i=0;i<nth;i++){
      start[i]=low;
      end[i]=low;
    }
    return;
  }
}
static long long time_(){
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC,&ts);
  return (long long)ts.tv_sec*1000LL +(ts.tv_nsec/1000000LL);
}
void parallel_for(int low,int high,std::function<void(int)>&&lambda,int nth){
  if(nth<1){
    nth=1;
  }
  if(low>=high){
    return;
  }
  int*start=new int[nth];
  int*end=new int[nth];
  range_cal(low,high,nth,start,end);
  args1D*args=new args1D[nth];
  for(int i=0;i<nth;i++){
    args[i].low=start[i];
    args[i].high=end[i];
    args[i].f=lambda;
  }
  pthread_t*ids=nullptr;
  if(nth>1){
    ids=new pthread_t[nth-1];
  }
  long long ts=time_();
  for(int i=1;i<nth;i++){
    int th=pthread_create(&ids[i-1],NULL,th_func1d,(void*)&args[i]);
    if(th!=0){//th=0 upon successful
      std::cerr<<"pthread_create error";
      for(int j=1;j<i;j++){
        pthread_join(ids[j-1],NULL);//pichle ke liye ruko then start
      }
      delete[]start;
      delete[]end;
      delete[]args;
      if(ids!=nullptr){
        delete[]ids;
      }
      return;
    }
  }
  th_func1d((void*)&args[0]);
  for(int i=1;i<nth;i++){
    pthread_join(ids[i-1],NULL);
  }
  long long te=time_();
  std::cout<<"Time: "<<(te-ts)<<"ms using "<<nth<<" threads\n";
    delete[]start;
    delete[]end;
    delete[]args;
    if(ids!=nullptr){
      delete[]ids;
  }
}
void parallel_for(int low1,int high1,int low2, int high2,std::function<void(int,int)>&&lambda,int nth){
  if(nth<1){
    nth=1;
  }
  if(high1<=low1){
    return;
  }
  if(high2<=low2){
    return;
  }
   int*start=new int[nth];
  int*end=new int[nth];
  range_cal(low1,high1,nth,start,end);
  args2D*args=new args2D[nth];
  for(int i=0;i<nth;i++){
    args[i].low1=start[i];
    args[i].high1=end[i];
    args[i].low2=low2;
    args[i].high2=high2;
    args[i].f=lambda;
  }
  pthread_t*ids=nullptr;
  if(nth>1){
    ids=new pthread_t[nth-1];
  }
  long long ts=time_();
  for(int i=1;i<nth;i++){
    int th=pthread_create(&ids[i-1],NULL,th_func2d,(void*)&args[i]);
    if(th!=0){//th=0 upon successful
      std::cerr<<"pthread_create error";
      for(int j=1;j<i;j++){
        pthread_join(ids[j-1],NULL);//pichle ke liye ruko then start
      }
      delete[]start;
      delete[]end;
      delete[]args;
      if(ids!=nullptr){
        delete[]ids;
      }
      return;
    }
  }
  th_func2d((void*)&args[0]);
  for(int i=1;i<nth;i++){
    pthread_join(ids[i-1],NULL);
  }
  long long te=time_();
  std::cout<<"Time: "<<(te-ts)<<"ms using "<<nth<<" threads\n";
    delete[]start;
    delete[]end;
    delete[]args;
    if(ids!=nullptr){
      delete[]ids;
  }
}
int user_main(int argc, char **argv);
/* Demonstration on how to pass lambda as parameter.
 * "&&" means r-value reference. You may read about it online.
 */
void demonstration(std::function<void()> && lambda) {
  lambda();
}

int main(int argc, char **argv) {
  /*
   * Declaration of a sample C++ lambda function
   * that captures variable 'x' by value and 'y'
   * by reference. Global variables are by default
   * captured by reference and are not to be supplied
   * in the capture list. Only local variables must be
   * explicity captured if they are used inside lambda.
   */
  int x=5,y=1;
  // Declaring a lambda expression that accepts void type parameter
  auto /*name*/ lambda1 = /*capture list*/[/*by value*/ x, /*by reference*/ &y](void) {
    /* Any changes to 'x' will throw compilation error as x is captured by value */
    y = 5;
    std::cout<<"====== Welcome to Assignment-"<<y<<" of the CSE231(A) ======\n";
    /* you can have any number of statements inside this lambda body */
  };
  // Executing the lambda function
  demonstration(lambda1); // the value of x is still 5, but the value of y is now 5

  int rc = user_main(argc, argv);

  auto /*name*/ lambda2 = [/*nothing captured*/]() {
    std::cout<<"====== Hope you enjoyed CSE231(A) ======\n";
    /* you can have any number of statements inside this lambda body */
  };
  demonstration(lambda2);
  return rc;
}

#define main user_main


// time lambda err hand