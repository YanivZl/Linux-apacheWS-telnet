#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>

#define INDEX_DIR "/var/www/html/index.html"

void reset_index(char* dir_name)
{
    FILE* f = fopen(INDEX_DIR, "w");
        if(f == NULL)
        {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
    fprintf(f, "<h2>Appache server - File System Monitor<br>Watched Directory: %s", dir_name);
    fclose(f);

}

void print_to_index(char* message)
{
    FILE* f = fopen(INDEX_DIR, "a");
    if(f == NULL)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
        
    }
    fprintf(f, "<br>%s" ,message);
    fclose(f);
}