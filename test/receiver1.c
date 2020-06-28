/*
 * 接收特定进程的信息
 */

#include "communicate.c"

int main(int argc, const char* argv[]){
    const char *name = argv[1]; //发送者进程名称
    char buf[BUFFSIZE];
    int times = 10;

    //每隔5s获取一次信息
    for(int i = 0; i < times; i++){
        recv_message(name, buf);
        printf("recv1 %d times:%s\n", i, buf);
        sleep(5);
    }

    printf("recv1 finished.\n");
    return 0;
}
