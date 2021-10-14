#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int call_prime(int rd_fd,int ab_fd)
{
    int prime;
    int child_exist=0;
    close(ab_fd);
    read(rd_fd,&prime,4);
    fprintf(1,"prime %d\n",prime);

    int n;
    int p[2];
    while(read(rd_fd,&n,4)!=0)
    {
        if((n%prime))
        {
            if(child_exist)
                write(p[1],&n,4);
            else
            {
                pipe(p);
                if(!fork())//child process
                {
                    close(rd_fd);
                    call_prime(p[0],p[1]);
                }
                else//parent
                {
                    close(p[0]);
                    child_exist=1;
                    write(p[1],&n,4);
                }
            } 
        }
    }
    close(rd_fd);
    if(child_exist) close(p[1]);
    wait(0);
    exit(0);
}

int
main(int argc, char *argv[])
{
    int p[2];
    pipe(p);
    if(fork())//main
    {
        close(p[0]);
        int i;
        for(i=2;i<=35;++i)
            write(p[1],&i,4);
        close(p[1]);
        wait(0);
    }
    else//child
    call_prime(p[0],p[1]);
    exit(0);
}
