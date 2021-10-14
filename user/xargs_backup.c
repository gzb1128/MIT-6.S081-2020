#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int i;
    char ch[10];
    char *str[32];
    char sr[100];
    if(!fork())
    {
        exec(argv[1],argv+1);
        exit(1);
    }
    else
    {
        str[0]=argv[1];
        int mark,num=0;
        int blk=1;
        char *p=sr;
        while((mark=read(0,&ch,1))!=0)
        {
            if(mark==-1)
                continue;
            if(ch[0]=='\n')
            {
                if(!blk)
                {
                    *p=0;
                    str[++num]=malloc(strlen(sr));
                    strcpy(str[num],sr);
                }
                str[++num]=0;
                if(!fork())
                {   
                    exec(argv[1],str);
                    exit(1);
                }
                else
                {
                    blk=1;
                    wait(0);
                    for(i=1;i<num;++i)
                        free(str[i]);
                    num=0;
                }
            }
            else if(ch[0]==' ')
            {
                if(!blk)
                {
                    *p=0;
                    str[++num]=malloc(strlen(sr));
                    strcpy(str[num],sr);
                }
                blk=1;
            }
            else
            {
                if(blk)
                {
                    p=sr;
                    blk=0;
                }
                *p++=ch[0];
            }

        }
    }
    exit(0);
}
