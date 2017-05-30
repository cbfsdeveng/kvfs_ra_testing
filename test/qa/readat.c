
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

//       off_t lseek(int fd, off_t offset, int whence);
long argval(char* cp)
{
    return strtol(cp, NULL, 0);
}
main(int argc, char **argv)
{
    char* buf = (char*)calloc(1, argval(argv[2]));
    lseek(0, argval(argv[1]), 0);
    read(0, buf, argval(argv[2]));
    write(1, buf, argval(argv[2]));
}
