#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char* get_fname(char *path) {
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
    memset(buf+strlen(p), 0, DIRSIZ-strlen(p));
    return buf;
}

void find(char *dir, char *file) {

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
        case T_FILE:
            fprintf(2, "%s is not a directory\n", dir);
            break;

        case T_DIR:
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