### Xv6 and Unix utilities
---
#### 0.要点
---
1. 多读几遍xv6的第一章。熟悉文件描述符、管道、进程概念
2. 核心函数 fork()\exec()\pipe()
3. 写lab过程中难以理解、卡得较久、出问题多的是进程和管道
4. 搞清楚fork和exec保留了什么，包括内存、管道、文件描述符的读写偏移等
#### 1. 环境配置
----
按照lab网站指南配置: https://pdos.csail.mit.edu/6.S081/2020/labs/util.html
#### 2. sleep(easy)
----
```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) { // argc是参数数量，argv是参数数组
    int n;
    if(argc < 2) { // 判断参数异常
        fprintf(2, "Usage: sleep seconds...");
        exit(1);// 异常返回
    } else {
        n = atoi(argv[1]); // char数组转int
        sleep(n); // 调用系统sleep函数
        exit(0);
    }
}
```
#### 2. pingpong(easy)
----
```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int p1[2], p2[2];
    char *buf = malloc(1); // 创建1byte空间

    pipe(p1); // 父进程向子进程通信的管道，0读1写
    pipe(p2); // 子进程向父进程通信的管道，0读1写
    
    if(fork() == 0) { // fork函数创建子进程
        // 子进程
        read(p1[0], buf, 1);
        printf("%d: received ping\n", getpid());
        write(p2[1], buf, 1);
        close(p1[0]), close(p1[1]), close(p2[0]), close(p2[1]); // 关闭管道
    } else {
        // 父进程
        write(p1[1], "1", 1);
        read(p2[0], buf, 1);
        printf("%d: received pong\n", getpid());
        close(p1[0]), close(p1[1]), close(p2[0]), close(p2[1]); // 关闭管道
    }
    exit(0);
}
```
#### 3. primes(hard)
----
不太完善，取巧控制for循环次数输出结果。这个实验涉及知识和细节比较多，包括管道使用、文件描述符、进程链、读写阻塞等。整体思路是通过for循环创建进程连，设置好各个进程间的管道，按照题目给的思想进行埃氏筛。

要注意考虑进程链第一个进程和最后一个进程，写的过程中由于管道设置问题，最后一个进程的读写都是自己，导致死循环。
```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


int main(int argc, char *argv[]) {
    int pid, p[2], left = 0, right = 0, pnum = 0, val, out;
    for(int i = 0; i < 10; ++i) { // 创建进程链
        pipe(p);
        pid = fork(); // fork函数创建子进程，子进程复制父进程内存，从当前代码行向下执行，所以结合下面的if语句，父进程循环一次就退出，子进程会进入第二次循环，创建子进程的子进程，子进程再退出循环。
        if(pid == 0) {
            sleep(1); // 控制输出顺序
            left = dup(p[0]);
            close(p[0]), close(p[1]);
        } else if(pid > 0) {
            right = dup(p[1]);
            close(p[0]), close(p[1]);
            break;
        } else {
            fprintf(2, "fork error!\n");
            break;
        }
        ++pnum; // 用于区别父进程和子进程，父进程pnum为0，子进程pnum为1，子子进程pnum为2...
    }
    // 以下代码所有进程都会执行，所以要考虑边界情况，因此通过pnum区分。
    if(!pnum) {
        val = 2; // 父进程初值手动设置
    } else {
        read(left, &val, 4); // 子进程初值从父进程处读取。
    }
    out = val;
    if(!pnum) {
        while(++val <= 35) {
            if(val % out != 0) {
                write(right, &val, 4);
            }
        }
    } else {
        while(read(left, &val, 4)) {
            if(val % out != 0) {
                write(right, &val, 4);
            }
        }
    }
    close(right);
    if(pnum) {
        close(left);
    }
    printf("prime %d\n", out);
    wait((int *)0);
    exit(0);
}
```
#### 4. find(moderate)
----
代码多一些，但是写起来简单，参考ls.c的代码编写即可。

核心是find函数，总体思路就是枚举目标目录下的所有文件/文件夹，是文件夹就递归find，是文件就比对文件名，文件名相同就输出。
```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char* get_fname(char *path) { // 由ls.c中的fmtname修改而来，输入path输出最后一个/后的内容
    static char buf[DIRSIZ+1];
    char *p;

    // Find first character after last slash.
    for(p=path+strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    // Return blank-padded name.
    if(strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p, strlen(p));
    memset(buf+strlen(p), 0, DIRSIZ-strlen(p)); // 修改处，原来填充空格，现在填充0字符
    return buf;
}

void find(char *dir, char *file) { // 第一个参数是目录，第二个参数要find的文件名。

    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(dir, 0)) < 0) {
        fprintf(2, "find: cannot open %s\n", dir);
        return;
    }
    if(fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", dir);
        close(fd);
        return;
    }
    switch(st.type) {
        case T_FILE: // 定义在fs.h中，即文件类型中的文件。
            fprintf(2, "%s is not a directory\n", dir);
            break;

        case T_DIR: // 定义在fs.h中，即文件类型中的文件夹。
            if(strlen(dir) + 1 + DIRSIZ + 1 > sizeof buf) {
                printf("find: path too long\n");
                break;
            }
            strcpy(buf, dir);
            p = buf+strlen(buf);
            *p++ = '/';
            while(read(fd, &de, sizeof(de)) == sizeof(de)) {
                if(de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
                    continue;
                }
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                if(stat(buf, &st) < 0){
                    printf("find: cannot stat %s\n", buf);
                    continue;
                }
                if(st.type == T_DIR) {
                    find(buf, file);
                } 
                char *fname = get_fname(buf);
                if(st.type == T_FILE && strcmp(fname, file) == 0) {
                    printf("%s\n", buf);
                }
            }
            break;
    }
    close(fd);
    return ;
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        fprintf(2, "find: please input parameters\n");
    } else if (argc == 2) {
        find(".", argv[1]);
    } else {
        find(argv[1], argv[2]); 
    }
    exit(0);
}
```
#### 5. xargs(moderate)
----
比较不常见的命令，了解这个命令就花了一些时间。个人理解是，这个命令是将管道前命令的输出作为管道后命令的参数。

如 

`echo hello too | xargs echo bye`

输出

`bye hello too`

即第一个echo的`hello too`被添加到了第二个echo的`bye`后面，形成命令

`echo bye hello too`

代码编写需要用到fork+exec的方法。

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

void solve(int argc, char *argv[], char *para) {
    char *subargv[MAXARG];
    for(int i = 1; i < argc; ++i) { // argv[0]是'xargs'，所以不要
        subargv[i-1] = argv[i];
    }
    subargv[argc-1] = para;
    subargv[argc] = 0;
    if(fork() == 0) { // fork子进程执行接收参数的命令，执行后退出
        exec(subargv[0], subargv);
        fprintf(2, "exec error!\n");
        exit(0);
    } else {
        wait((int*)0);
    }
}

int main(int argc, char *argv[]) {
    char ch[2], buf[64];
    char *p = buf;

    if(argc < 2) {
        fprintf(2, "xargs: please input command\n");
        exit(0);
    }
    while(read(0, ch, 1) > 0) { // 逐字符读取管道的内容
        if(ch[0] != '\n') { // 以换行符分割参数，每一个参数执行一次xargs后面的命令。
            *p = ch[0];
            ++p;
        } else {
            *p = 0; // 切断。
            solve(argc, argv, buf);
            p = buf;
        }
    }
    exit(0);
}
```