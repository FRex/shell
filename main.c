#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define LINE_LEN 1000
#define BUFFER_SIZE ((LINE_LEN) + 10)
static char line[BUFFER_SIZE];

static size_t skipline()
{
    size_t ret = 0;

    while(1)
    {
        int c = fgetc(stdin);
        if(c == '\n' || c == EOF)
            return ret;

        ++ret;
    }

    return ret;
}

static int getline()
{
    size_t linelen;

    fputs("> ", stdout);
    fflush(stdout);
    if(fgets(line, BUFFER_SIZE, stdin) == NULL)
        return 0;

    linelen = strlen(line);
    if(linelen == 0)
        return 0;

    linelen -= 1;
    if(line[linelen] != '\n')
        linelen += skipline();

    if(linelen > LINE_LEN)
    {
        fprintf(stderr, "Line too long: %u, max is %u\n", (unsigned)linelen, (unsigned)LINE_LEN);
        return 0;
    }

    return 1;
}

static int mustquit(int * ret)
{
    if(feof(stdin))
    {
        puts("Got EOF - bye!");
        *ret = 0;
        return 1;
    }

    if(ferror(stdin))
    {
        puts("True returned from ferror(stdin) - quitting.\n");
        *ret = 1;
        return 1;
    }

    return 0;
}

static int isbuiltin(char * arg0, char * arg1)
{
    if(0 == strcmp("cd", arg0))
    {
        if(arg1 != NULL)
        {
            if(chdir(arg1))
                fprintf(stderr, "chdir(\"%s\") failed: %s\n", arg1, strerror(errno));
        }
        else
        {
            fprintf(stderr, "builtin 'cd' requires an argument\n");
        }
        return 1;
    }

    if(0 == strcmp("exit", arg0))
    {
        puts("Bye!");
        if(arg1 == NULL)
            arg1 = "0";

        exit(atoi(arg1) % 256);
    }

    return 0;
}

static void runcmd()
{
    char * args[BUFFER_SIZE];
    char * c;
    int intoken, argcount;
    pid_t pid;

    argcount = 0;
    intoken = 0;
    c = line;
    while(*c)
    {
        if(intoken && isspace(*c))
        {
            intoken = 0;
            *c = '\0';
        }
        else if(!intoken && !isspace(*c))
        {
            args[argcount] = c;
            ++argcount;
            intoken = 1;
        }

        ++c;
    } /* while *c */

    args[argcount] = NULL;
    if(argcount == 0)
        return;

    if(isbuiltin(args[0], args[1]))
        return;

    pid = fork();
    if(pid == -1)
    {
        perror("fork() failed");
        return;
    }

    if(pid == 0)
    {
        execvp(args[0], args);
        printf("execvp(%s, [%d args], failed: %s\n", args[0], argcount, strerror(errno));
        exit(1);
    }

    if(pid > 0)
        waitpid(pid, NULL, 0);
}

int main(void)
{
    while(1)
    {
        if(getline())
        {
            runcmd();
        }
        else
        {
            int ret;
            if(mustquit(&ret))
                return ret;
        }
    } /* while 1 */

    return 0;
}
