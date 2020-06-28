/*
 * 向特定进程发送信息
 */

#include "communicate.c"

int main(int argc, const char* argv[]) {
    const char *name = argv[1]; //接收者进程名称
    char content[BUFFSIZE];
    char tmp[MAX_COMMAND_LENGTH];
    int times = 10;
    pid_t receiver_pid = 0;

    //等待receiver上线
    get_pid_by_name(&receiver_pid, name);
    while(receiver_pid == 0){
        sleep(5);
        get_pid_by_name(&receiver_pid, name);
    }

    //每隔5s发送一次消息，一共发送times次
    for(int i = 0; i < times; i++){
        strcpy(content, argv[2]);   //要发送的内容
        snprintf(tmp, MAX_COMMAND_LENGTH, "%d", i);
        strcat(content, tmp);
        send_message(name, content);
        sleep(5);
    }

    printf("sender1 finished!\n");

    return 0;
}
