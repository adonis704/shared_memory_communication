/*
 * 上层接口
 */

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <linux/types.h>
#include "regex.h"
#include "communicate.h"

/*
 * 读共享内存，返回成功读取的字节数
 * name:设备名称
 * buf：读缓冲区
 */
ssize_t read_shared_mem(const char* name, char buf[BUFFSIZE]){
    int fd;
    ssize_t rst;

    fd = open(name, O_RDWR);
    rst = read (fd, buf, BUFFSIZE);

    close(fd);
    return rst;
}

/*
 * 写共享内存，返回成功写入的字节数
 * name：设备名称
 * buf：读缓冲区
 */
ssize_t write_shared_mem(const char* name, char content[BUFFSIZE]){
    int fd;
    ssize_t rst;

    fd = open (name, O_RDWR);
    rst = write(fd, content, BUFFSIZE);
    printf("write content \"%s\" to device %s\n", content, name);

    close(fd);
    return rst;
}

/*
 * 将进程名转化为进程ID
 * pid：存放pid的指针
 * task_name：进程名
 */
void get_pid_by_name(pid_t *pid, const char *task_name){
    DIR *dir;   //存放进程信息的目录
    struct dirent *ptr;
    FILE *fp;
    char filepath[MAX_COMMAND_LENGTH];
    char cur_task_name[MAX_COMMAND_LENGTH];
    char buf[BUFFSIZE];

    dir = opendir("/proc");
    if (NULL != dir)
    {
        while ((ptr = readdir(dir)) != NULL) //循环读取/proc下的每一个文件/文件夹
        {
            //如果读取到的是"."或者".."则跳过，读取到的不是文件夹名字也跳过
            if ((strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0))
                continue;
            if (DT_DIR != ptr->d_type)
                continue;

            sprintf(filepath, "/proc/%s/status", ptr->d_name);//生成要读取的文件的路径
            fp = fopen(filepath, "r");
            if (NULL != fp)
            {
                if( fgets(buf, BUFFSIZE-1, fp)== NULL ){
                    fclose(fp);
                    continue;
                }
                sscanf(buf, "%*s %s", cur_task_name);

                //如果文件内容满足要求则打印路径的名字（即进程的PID）
                if (!strcmp(task_name, cur_task_name)){
                    sscanf(ptr->d_name, "%d", pid);
                }
                fclose(fp);
            }
        }
        closedir(dir);
    }
}



/*
 * 找出已经使用的设备，成功返回0
 * used：用于存放设备信息的数组，0表示未使用，1表示已经使用
 * match_pattern：正则表达式
 */
int match(int used[MAX_DEVICES], char match_pattern[MAX_COMMAND_LENGTH]){
    DIR *dir;
    struct dirent *ptr;

    dir = opendir("/dev");
    int status;
    int c_flags = REG_EXTENDED;
    regmatch_t pmatch[1];
    const size_t nmatch = 1;
    regex_t reg;
    char *pattern = match_pattern;
    regcomp(&reg, pattern, c_flags);    //编译正则表达式

    while((ptr = readdir(dir)) != NULL){
        status = regexec(&reg, ptr->d_name, nmatch, pmatch, 0); //执行正则表达式
        if(status == 0){
            for(int i = pmatch[0].rm_so; i < pmatch[0].rm_eo; i++){
                if(ptr->d_name[i] >= '0' && ptr->d_name[i] <= '9'){ //ptr->d_name为待匹配的字符串
                    int id = ptr->d_name[i] - '0';
                    used[id] = 1;
                    printf("device%d has been used.\n", id);
                    break;  //读到的第一个数字即为设备号，直接中断即可
                }
            }
        }
    }
    regfree(&reg);  //释放正则表达式
    closedir(dir);

    return 0;
}

/*
 * 格式化设备id与两个进程的pid，格式为“次设备号：发送者进程设备号：接收者进程设备号”
 * content：保存格式化后的信息
 * id：次设备号
 * major_id：发送者进程ID
 * minor_pid：接收者进程ID
 */
void format(char content[MAX_COMMAND_LENGTH], int id, pid_t major_pid, pid_t minor_pid){
    char tmp[MAX_PID_BITS];

    snprintf(tmp, MAX_PID_BITS, "%d", id);
    strcpy(content, tmp);   //第一次写先覆盖，后面拼接
    strcat(content, ":");
    snprintf(tmp, MAX_PID_BITS, "%d", major_pid);
    strcat(content, tmp);
    strcat(content, ":");
    snprintf(tmp, MAX_PID_BITS, "%d", minor_pid);
    strcat(content, tmp);
}

/*
 * 建立连接，返回占用的设备号，失败返回-1
 * major：发送者进程ID
 * minor：接收者进程ID
 */
int connect(pid_t major, pid_t minor){
    int used[MAX_DEVICES] = {0};
    char command[MAX_COMMAND_LENGTH];   //总命令
    char *command1 = "mknod /dev/shared";
    char *command2 = " c 666 ";
    char command3[MAX_COMMAND_LENGTH];  //格式化的“次设备号：写进程pid：读进程pid”
    char command4[MAX_PID_BITS];    //id的字符串格式
    char *command5 = "mknod /dev/sharedtmp";
    int id = -1;
    int err;
    pid_t major_pid = major;
    pid_t minor_pid = minor;

    match(used, "shared[0-9]");

    //找到第一个未被使用的设备
    for(int i = 0; i < MAX_DEVICES; i++){
        if(used[i] == 0){
            id = i;
            printf("device%d is available.\n", id);
            break;
        }
    }
    if(id == -1){   //无设备可用
        printf("no devices available.\n");
        return -1;
    }

    //先创建临时文件以确保有权限
    format(command3, id, major_pid, minor_pid);
    snprintf(command4, MAX_PID_BITS, "%d", id);
    strcpy(command, command5);
    strcat(command, command4);
    strcat(command, command2);
    strcat(command, command4);
    err = system(command);
    if(err){    //发生冲突，返回-1
        printf("can't build connection, please try again.\n");
        return -1;
    }
    printf("exec:%s\n", command);

    //有权限后再创建节点
    strcpy(command, command1);
    strcat(command, command3);
    strcat(command, command2);
    strcat(command, command4);
    err = system(command);
    if(err){
        printf("can't build connection from %d to %d.\n", major_pid, minor_pid);
        return -1;
    }

    //写入初始信息（可以不用）
    char dev_name[MAX_COMMAND_LENGTH];
    strcpy(dev_name, "/dev/shared");
    strcat(dev_name, command);
    write_shared_mem(dev_name, "initial message");
    printf("exec:%s\n", command);
    printf("connect pid %d to pid %d\n", major_pid, minor_pid);

    return id;
}

/*
 * 断开连接，成功返回0，失败返回-1
 * major：发送者进程ID
 * minor：接收者进程ID
 */
int disconnect(pid_t major, pid_t minor){
    char command[MAX_COMMAND_LENGTH];
    char content[MAX_COMMAND_LENGTH];
    int used[MAX_DEVICES] = {0};
    int id = -1;
    int err;
    pid_t major_pid = major;
    pid_t minor_pid = minor;

    //拼接pattern并进行正则匹配
    char pattern1[MAX_COMMAND_LENGTH];
    char pattern2[MAX_COMMAND_LENGTH];
    char pattern3[MAX_COMMAND_LENGTH];

    strcpy(pattern1, "shared[0-9]:");
    snprintf(pattern2, MAX_COMMAND_LENGTH, "%d", major_pid);
    snprintf(pattern3, MAX_COMMAND_LENGTH, "%d", minor_pid);
    strcat(pattern1, pattern2);
    strcat(pattern1, ":");
    strcat(pattern1, pattern3);
    match(used, pattern1);

    //找到占用的设备
    for(int i = 0; i < MAX_DEVICES; i++){
        if(used[i] == 1){
            id = i;
            break;
        }
    }
    if(id == -1){   //没有符合要求的设备
        printf("no connection between %d and %d\n", major_pid, minor_pid);
        return -1;
    }

    //删除节点
    strcpy(command, "rm -f /dev/shared");
    format(content, id, major_pid, minor_pid);
    strcat(command, content);
    err = system(command);
    if(err){
        printf("can't cut the connection between %d and %d\n", major_pid, minor_pid);
        return -1;
    }
    printf("exec:%s\n", command);
    printf("disconnect pid %d to pid %d\n", major_pid, minor_pid);

    //解除对临时文件的占用
    char id_str[MAX_PID_BITS];
    snprintf(id_str, MAX_PID_BITS, "%d", id);
    strcpy(command, "rm -f /dev/sharedtmp");
    strcat(command, id_str);
    err = system(command);
    if(err){
        printf("can't release devices sources.\n");
        return -1;
    }
    printf("exec:%s\n", command);

    return 0;
}

/*
 * 发送信息，成功返回0，失败返回-1
 * name：接收进程名称
 * buf：缓冲区
 */
int send_message(const char *name, char buf[BUFFSIZE]){
    int used[MAX_DEVICES] = {0};
    int id = -1;
    pid_t major_pid;
    pid_t minor_pid = 0;

    //获取接收者进程的pid
    major_pid = getpid();
    get_pid_by_name(&minor_pid, name);
    if(minor_pid == 0){ //找不到接收者进程
        printf("no process named %s.\n", name);
        return -1;
    }

    //拼接pattern并进行正则匹配
    char pattern1[MAX_COMMAND_LENGTH];
    char pattern2[MAX_COMMAND_LENGTH];
    char pattern3[MAX_COMMAND_LENGTH];

    strcpy(pattern1, "shared[0-9]:");
    snprintf(pattern2, MAX_COMMAND_LENGTH, "%d", major_pid);
    snprintf(pattern3, MAX_COMMAND_LENGTH, "%d", minor_pid);
    strcat(pattern1, pattern2);
    strcat(pattern1, ":");
    strcat(pattern1, pattern3);
    match(used, pattern1);

    //找到占用的设备
    for(int i = 0; i < MAX_DEVICES; i++){
        if(used[i] == 1){
            id = i;
            break;
        }
    }
    if(id == -1){   //如果没有占用的设备则先建立连接
        id = connect(major_pid, minor_pid);
        if(id < 0){
            printf("can't connect %s\n", name);
            return -1;
        }
    }

    //格式化设备文件名并将缓冲区数据写入对应设备
    char dev_name[MAX_COMMAND_LENGTH];
    char dev_no[MAX_COMMAND_LENGTH];
    strcpy(dev_name, "/dev/shared");
    format(dev_no, id, major_pid, minor_pid);
    strcat(dev_name, dev_no);
    write_shared_mem(dev_name, buf);

    return 0;
}

/*
 * 接收信息，成功返回0，失败返回-1
 * name：发送者进程名称
 * buf：缓冲区
 */
int recv_message(const char *name, char buf[BUFFSIZE]){
    int used[MAX_DEVICES] = {0};
    int id = -1;
    pid_t major_pid = 0;
    pid_t minor_pid;

    //获取发送者进程的pid
    minor_pid = getpid();
    get_pid_by_name(&major_pid, name);
    if(major_pid == 0){
        printf("no process named %s.\n", name);
        return -1;
    }

    //拼接pattern并进行正则匹配
    char pattern1[MAX_COMMAND_LENGTH];
    char pattern2[MAX_COMMAND_LENGTH];
    char pattern3[MAX_COMMAND_LENGTH];

    strcpy(pattern1, "shared[0-9]:");
    snprintf(pattern2, MAX_COMMAND_LENGTH, "%d", major_pid);
    snprintf(pattern3, MAX_COMMAND_LENGTH, "%d", minor_pid);
    strcat(pattern1, pattern2);
    strcat(pattern1, ":");
    strcat(pattern1, pattern3);
    match(used, pattern1);

    //找到占用的设备
    for(int i = 0; i < MAX_DEVICES; i++){
        if(used[i] == 1){
            id = i;
            break;
        }
    }
    if(id == -1){   //如果没有占用的设备则建立连接
        id = connect(major_pid, minor_pid);
        if(id < 0){
            printf("can't connect %s\n", name);
            return -1;
        }
    }

    //格式化设备文件名并从缓冲区读入信息
    char dev_name[MAX_COMMAND_LENGTH];
    char dev_no[MAX_COMMAND_LENGTH];
    strcpy(dev_name, "/dev/shared");
    format(dev_no, id, major_pid, minor_pid);
    strcat(dev_name, dev_no);
    read_shared_mem(dev_name, buf);

    return 0;
}

