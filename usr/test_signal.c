#include <param.h>
#include <file.h>
#include <unistd.h>
#include <signal.h>

int a=3;

int strlen(char *str) {
    char* sp;
    for(sp=str; *sp!='\0'; sp++);
    return sp-str;
}

int hwsig(int n){
    int i;
    char str[] = "sig handled\n";

    printf("n: %x\n", n);
    for(i=0; i<n; i++) {
        write(1, str, sizeof(str));
    }
    return 0;
}

int main(int argc, char **argv) {
    int fd, i, pid;
    char str[] = "hello, world\n";
    struct sigaction sa;

    sa.sa_handler = hwsig;
    if ((pid=fork())==0) {
        sigaction(SIGINT, &sa, NULL);
        write(1, str, sizeof(str));
        for(;;);
    }
    kill(pid, SIGINT);
    return 0;
}
