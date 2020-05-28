#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "disk.h"

extern unsigned char bootsector_data[];
extern unsigned char stage2_data[];

extern unsigned int bootsector_len;
extern unsigned int stage2_len;

void usage(char *argv[]);

char example_config[] = {"# This is an example configuration file\n"
                         "\n"
                         "# Change this path to the kernel image\n"
                         "filename = /boot/kernel\n"
                         "\n"
                         "# Add something to this line if there should be\n"
                         "# a command line passed to the kernel\n"
                         "cmd-line =\n"};

int main(int argc, char *argv[]) {
    assert(bootsector_len == sizeof(fat32_bootsector_t));
    assert(stage2_len > 0);

    if (argc < 3) {
        usage(argv);
        return 2;
    }

    char *filename = argv[1];
    char *root     = argv[2];

    FILE *bs_file = fopen(filename, "r+b");
    assert(bs_file != NULL);

    fat32_bootsector_t new_bootsector, bootsector_to_replace;
    memcpy(&new_bootsector, bootsector_data, bootsector_len);

    size_t read = fread(&bootsector_to_replace, sizeof(fat32_bootsector_t), 1, bs_file);
    assert(read == 1);
    assert(bootsector_to_replace.boot_signature == 0xAA55);

    memcpy(bootsector_to_replace.jmp, new_bootsector.jmp, sizeof(new_bootsector.jmp));
    memcpy(bootsector_to_replace.boot_code,
           new_bootsector.boot_code,
           sizeof(new_bootsector.boot_code));

    fseek(bs_file, 0, SEEK_SET);
    size_t written = fwrite(&bootsector_to_replace, sizeof(fat32_bootsector_t), 1, bs_file);
    assert(written == 1);

    fclose(bs_file);

    size_t root_len = strlen(root);

    char stage2_path[root_len + 7 + 1];
    strcpy(stage2_path, root);
    strcat(stage2_path, "/stage2");

    FILE *stage2_file = fopen(stage2_path, "wb");
    assert(stage2_file != NULL);

    written = fwrite(stage2_data, stage2_len, 1, stage2_file);
    assert(written == 1);

    fclose(stage2_file);

    char config_path[root_len + 5 + 10 + 1];
    strcpy(config_path, root);
    strcat(config_path, "/boot/iyasb.cfg");

    FILE *config_file = fopen(config_path, "r");
    if (config_file == NULL) {
        config_file = fopen(config_path, "wb");
        assert(config_file != NULL);

        written = fwrite(example_config, sizeof(example_config), 1, config_file);
        assert(written == 1);
    }
    fclose(config_file);

    printf("Done!\n");
    printf("Remember to sync(3) the filesystem *and* the device\n");

    return 0;
}

void usage(char *argv[]) {
    fprintf(stderr, "Usage: %s file root\n", argv[0]);
}