#include "error.h"
#include "modules.h"

#include "stdio.h"
#include <unistd.h>
#include <termios.h>



char t_getchar()
{
    struct termios old_tio, new_tio;
    unsigned char c;

    tcgetattr(STDIN_FILENO,&old_tio);
    new_tio=old_tio;
    /* disable canonical mode (buffered i/o) and local echo */
    new_tio.c_lflag &=(~ICANON & ~ECHO);
    tcsetattr(STDIN_FILENO,TCSANOW,&new_tio);

    c=getchar();

    /* restore the former settings */
    tcsetattr(STDIN_FILENO,TCSANOW,&old_tio);

    return c;
}


int grant_tty(Grant *g)
{
    FILE *tty = fopen("/dev/tty", "a+");
    if (!tty) return GrantSkip;

    fprintf(tty,
            "\033[1;31mGrant elevated execution\n"
            "   of: %s %s \n"
            "   to: %s %s [ pid=%i CWD='%s' ]"
            "\033[0m\n"
           ,qPrintable(g->program)
           ,qPrintable(g->arguments.join(" "))
           ,qPrintable(g->parent)
           ,qPrintable(g->parentargs.join(" "))
           ,g->ppid
           ,qPrintable(g->pcwd)
    );

    int r = GrantAgain;
    for (;;) {
        fputs("\r", tty);
        if (g->constraints & GrantTrace)
            fputs("\033[1;32m(T)race\033[0m, ", tty);
        else
            fputs("(T)race, ", tty);

        if (g->constraints & GrantIsolate)
            fputs("\033[1;32m(I)solate\033[0m, ", tty);
        else
            fputs("(I)solate, ", tty);

        if (r == GrantPass) {
            fputs("\033[1;32m(G)rant\033[0m, (A)bort \n", tty);
            return r;
        } else if (r == GrantFail) {
            fputs("(G)rant, \033[1;32m(A)bort\033[0m \n", tty);
            return r;
        }

        fputs("(G)rant, (A)bort ... ", tty);
        fflush(tty);
        char c = t_getchar();
        if (c == '\n')
            c = 'G';

        switch (c) {
            case 'A':
            case 'a':
                r = GrantFail;
                break;
            case 'G':
            case 'g':
                r = GrantPass;
                break;
            case 'T':
            case 't':
                g->constraints ^= GrantTrace;
                break;
            case 'I':
            case 'i':
                g->constraints ^= GrantIsolate;
                break;
        }
        putc(c, tty);
        fflush(tty);
    }
}

