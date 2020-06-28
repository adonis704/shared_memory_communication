/*
 * 读取特定设备的内容
 */

#include "communicate.c"

int main(int argc, const char* argv[]){
    const char *name = argv[1]; //设备名称
    char *buf[BUFFSIZE];

    read_shared_mem(name, buf);
    printf("the content is:%s\n", buf);
    return 0;
}
