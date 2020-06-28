#ifndef LINUXDRIVER_COMMUNICATE_H
#define LINUXDRIVER_COMMUNICATE_H

#endif //LINUXDRIVER_COMMUNICATE_H

#ifndef NULL
#define NULL 0
#endif

#define BUFFSIZE 1024   //缓冲区大小
#define MAX_DEVICES 10  //支持的最大设备数量
#define MAX_PID_BITS 10 //pid的最大位数
#define MAX_COMMAND_LENGTH 100  //指令的最大长度

ssize_t read_shared_mem(const char* name, char buf[BUFFSIZE]);
ssize_t write_shared_mem(const char* name, char content[BUFFSIZE]);
void get_pid_by_name(pid_t *pid, const char *task_name);
int match(int used[MAX_DEVICES], char match_pattern[MAX_COMMAND_LENGTH]);
void format(char content[MAX_COMMAND_LENGTH], int id, pid_t major_pid, pid_t minor_pid);
int connect(pid_t major, pid_t minor);
int disconnect(pid_t major, pid_t minor);
int send_message(const char *name, char buf[BUFFSIZE]);
int recv_message(const char *name, char buf[BUFFSIZE]);
