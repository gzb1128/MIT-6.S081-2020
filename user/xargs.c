#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int i;
    char ch[10];
    char *str[32];
    char sr[256];

    if(argc>32)
    {
        fprintf(1,"xargs:argument number is too big\n");
        exit(1);
    }
    for(i=1;i<argc;++i)
    str[i-1]=argv[i];

    int mark;
    str[argc]=0;

    char *p=sr;
    while((mark=read(0,&ch,1))!=0)
    {
        if(mark==-1)
            continue;
        if(ch[0]=='\n')
        {
            *p=0;
            str[argc-1]=sr;
            if(!fork())
            {   
                exec(argv[1],str);
                exit(0);
            }
            else
            {
                wait(0);
                p=sr;
            }
        }
        else
            *p++=ch[0];
    }
    exit(0);
}
