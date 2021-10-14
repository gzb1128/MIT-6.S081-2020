#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int p[2],q[2];
    pipe(p);
    pipe(q);
    int pid;
    char str[10];

    if(fork())//parent
    {
        pid=getpid();
        close(p[0]);
        close(q[1]);
        write(p[1],"ping",4);
        close(p[1]);

        read(q[0],str,4);
        close(q[0]);
        fprintf(1,"%d: received %s\n",pid,str);
        exit(0);
    }
    else//child
    {
        pid=getpid();
        close(p[1]);
        close(q[0]);

        read(p[0],str,4);
        close(p[0]);
        fprintf(1,"%d: received %s\n",pid,str);

        write(q[1],"pong",4);
        close(q[1]);
        exit(0);
    }
}
