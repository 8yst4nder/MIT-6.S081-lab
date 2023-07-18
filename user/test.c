#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

void solve(int argc, char *argv[], char *para) {
    char *subargv[MAXARG];
    for(int i = 1; i < argc; ++i) {
        subargv[i-1] = argv[i];
    }
    subargv[argc-1] = para;
    subargv[argc] = 0;
    if(fork() == 0) {
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
    while(read(0, ch, 1) > 0) {
        if(ch[0] != '\n') {
            *p = ch[0];
            ++p;
        } else {
            *p = 0;
            // printf("%s\n", buf);
            solve(argc, argv, buf);
            p = buf;
        }
    }
    exit(0);
}
//sh < xargstest.sh