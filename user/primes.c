#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


int main(int argc, char *argv[]) {
    int pid, p[2], left = 0, right = 0, pnum = 0, val, out;
    for(int i = 0; i < 10; ++i) {
        // printf("%d\n", i);
        pipe(p);
        pid = fork();
        if(pid == 0) {
            sleep(1);
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
        ++pnum;
    }
    if(!pnum) {
        val = 2;
    } else {
        read(left, &val, 4);
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