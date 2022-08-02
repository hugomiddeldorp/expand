/************************************************
* 
* Expand: Takes shorthand teaching notes and expands them
* with modular pre-written explanations. Saves writing time
* for lesson notes by avoiding repetition and increases
* quality of explanations by standardising them.
*
* Currently matches:
* - [[name_of_module]] for modular explanations
* - Lilypond block code to image, syntax is ``l [...] l``
* - export // comments to another file
*
* TO-DO:
* - None
* 
* Author: Hugo Middeldorp
* 
************************************************/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>


struct config {
    FILE *file_in;
    FILE *file_out;
    FILE *file_lily;
    FILE *file_comments;
    char *file_name;
    char file_out_name[128];
    int image_count;
    int inside_lilypond;
} config;


/* Create the image using lilypond system call and replace the full image
 * with the cropped image */
void createImage() {

    time_t current_time;
    current_time = time(NULL);

    char image_path[128];
    
    /* Don't add .png to image path now because lilypond 
     * doesn't take it as an extension */
    char image_path_temp[125];

    sprintf(image_path, "img/%ld_%d", current_time, config.image_count);
    sprintf(image_path_temp, "img/%ld_%d.cropped.png",
            current_time, config.image_count);

    char system_command[256];
    sprintf(system_command,
            "lilypond -fpng -dresolution=120 -dcrop -o %s temp.ly",
            image_path);
    printf("%s\n", system_command);
    system(system_command);

    /* Add the extension here for all future use */
    sprintf(image_path, "img/%ld_%d.png", current_time, config.image_count);

    rename(image_path_temp, image_path);
    remove("temp.ly");

    fprintf(config.file_out, "![](%s)\n", image_path);
}


/* Replaces the current buffer with the corresponding module */
int outModule(char buffer[], int length) {

    int i;
    int j = 0;
    char module_name[64];
    char module_path[160];
    
    for (i = 0; i < length; ++i) {
        if (buffer[i] != '[' && buffer[i] != ']') {
            module_name[j] = buffer[i];
            j++;
        }
    }

    module_name[j] = '\0';

    sprintf(module_path, "modules/%s.md", module_name);

    FILE *module_file;

    if (!(module_file = fopen(module_path, "r"))) {
        printf("Could not find module.\n");
        return 1;
    }

    char c;

    while ((c = getc(module_file)) != EOF) {
        putc(c, config.file_out);
    }

    return 0;
}


void openLily() {
    config.file_lily = fopen("temp.ly", "w");
    config.inside_lilypond = 1;
    ++config.image_count;
}


void closeLily() {
    fclose(config.file_lily);
    createImage();
    config.inside_lilypond = 0;
}


void inspectBuffer(char buffer[], int length) {
    int i;
    int special_buffer = 0;


    for (i = 0; i < length-1; ++i) {
        // End of Lily code block
        if (buffer[i] == 'l' && buffer[i+1] == '`' && buffer[i+2] == '`') {
            closeLily();
            special_buffer = 1;
            break;
        }
        // Start of Lily code block
        else if (buffer[i] == '`' && buffer[i+1] == '`' && buffer[i+2] == 'l') {
            openLily();
            special_buffer = 1;
            break;
        }
        // Module
        else if (buffer[i] == '[' && buffer[i+1] == '[') {
            printf("Module found!\n");
            outModule(buffer, length);
            special_buffer = 1;
            break;
        }
        // Comment
        else if (buffer[i] == '/' && buffer[i+1] == '/') {
            fprintf(config.file_comments, "%s %s\n", config.file_name, buffer);
            special_buffer = 1;
            break;
        }
    }

    if (!special_buffer) {
        if (config.inside_lilypond) {
            printf("%s\n", buffer);
            fprintf(config.file_lily, "%s\n", buffer);
        }
        // Normal text
        else { 
            fprintf(config.file_out, "%s\n", buffer);
        }
    }
}


void init(char *file_name) {

    sprintf(config.file_out_name, "out_%s", file_name);

    config.file_in = fopen(file_name, "r");
    config.file_out = fopen(config.file_out_name, "w");
    config.file_comments = fopen("comments.txt", "a");

    config.file_name = file_name;
    config.image_count = 0;
    config.inside_lilypond = 0;
}


void closeFiles() {
    fclose(config.file_in);
    fclose(config.file_out);
    fclose(config.file_comments);
}


void processText() {

    char c;
    int buffer_index = 0;
    char buffer[512];       // Contains one line

    while ((c = getc(config.file_in)) != EOF) {
        if (c != '\n') {
            buffer[buffer_index] = c;
            ++buffer_index;
        }
        else {
            buffer[buffer_index] = '\0';
            inspectBuffer(buffer, buffer_index);
            buffer_index = 0;
        }
    }
}


int main(int argc, char *argv[]) {

    if (argc <= 1) {
        printf("No file given.\n");
        return 1;
    }
    else {
        init(argv[1]);
        processText();
        closeFiles();
        rename(config.file_out_name, config.file_name);
    }

	return 0;
}

