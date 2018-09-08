#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>


int main(int argc, char* argv[])
{
    while (true) {
        
        system("sysnc");
        system("echo 3 > /proc/sys/vm/drop_caches");
        sleep(5);
    }
    return 0;
}