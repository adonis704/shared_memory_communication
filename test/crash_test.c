/*
 * 冲突测试
 */

#include <pthread.h>

#include "communicate.c"
#define THREAD_NUM 3	// 创建三个线程
pthread_t tid[THREAD_NUM];

/*
 * 建立与进程12580的连接
 */
void *build_connect(void *arg){
    pid_t major_pid = getpid();
    pid_t minor_pid = 12580;
    connect(major_pid, minor_pid);
    return NULL;
}

int main(void){
    int i = 0;
    int err;

    while(i < THREAD_NUM){
        err = pthread_create(&(tid[i]), NULL, &build_connect, NULL);
        if(err != 0) printf("can't create thread:[%s]\n", strerror(err));
        else printf("Thread%d created successfully\n", i);
        i++;
    }

    sleep(5);
    return 0;
}
