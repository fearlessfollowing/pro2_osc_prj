#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CPU_TEMP_PATH   "/sys/class/thermal/thermal_zone2/temp"
#define GPU_TEMP_PATH   "/sys/class/thermal/thermal_zone1/temp"


int main(int argc, char* argv[])
{
    char line[512] = {0};
    unsigned int temp = 0;

    FILE* fp = fopen(CPU_TEMP_PATH, "r");
    if (fp) {
        fgets(line, sizeof(line), fp);
        line[strlen(line) -1] = '\0';

        printf("line: %s\n", line);
        temp = atoi(line);
        printf("temp = %f\n", temp / 1000.0f);
    }
    fclose(fp);

    return 0;
}