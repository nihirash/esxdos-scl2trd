#include <stdio.h>
#include <arch/zx/esxdos.h>
#include <errno.h>
#include <string.h>

unsigned char iStream;
unsigned char oStream;
unsigned freeTrack = 1;
unsigned freeSec = 0;
unsigned char count;

void openFile()
{
    unsigned char *name[ESX_FILENAME_MAX];
    unsigned char *inputName[ESX_FILENAME_MAX + 1];
    unsigned char *outputName[ESX_FILENAME_MAX + 1];
    printf("Enter filename(without ext):\n");
    scanf("%s", name);
    sprintf(inputName, "%s.SCL", name);
    sprintf(outputName, "%s.TRD", name);
    iStream = esx_f_open(inputName, ESX_MODE_READ | ESX_MODE_OPEN_EXIST);
    if (!errno) 
        oStream = esx_f_open(outputName, ESX_MODE_WRITE | ESX_MODE_OPEN_CREAT_TRUNC);
    if (errno) {
        esx_f_close(iStream);
        esx_f_close(oStream);
        printf("\n Failed to open streams!\n");
        exit(0);
    }
}

void validateScl()
{
    unsigned char *header[9] = {0,0,0,0,0,0,0,0,0};
    unsigned char *expected[9] = "SINCLAIR";
    esx_f_read(iStream, header, 8);
    if (strcmp(header, expected)) {
        printf("Wrong file header: %s", header);
        exit(1);
    }
}
/**
 *  In SCL file absent info about starting sector/track. We read headers as is and just append absent info 
 */
void importCatalog()
{
    unsigned char trdHeader[16];
    unsigned char name[10] = {0,0,0,0,0,0,0,0,0,0};
    unsigned char i;
    esx_f_read(iStream, &count, 1);
    printf("Contains %i files\n", count);
    for (i=0;i<count;i++) {
        esx_f_read(iStream, &trdHeader, 14);
        strncpy(name, trdHeader, 9);
        printf("%s ", name);
        trdHeader[14] = freeSec;
        trdHeader[15] = freeTrack;
        freeSec += trdHeader[0xd];
        freeTrack += freeSec / 16;
        freeSec = freeSec % 16;
        esx_f_write(oStream, &trdHeader, 16);
    }
    for (i=0;i<16;i++) trdHeader[i] = 0;
    for (i=count;i<128;i++) esx_f_write(oStream, &trdHeader, 16);
}

/**
 * This function writes only really need to run info
 * 
 * Cause TRD will be cutted - it will be read only and I don't write "free sectors" info.
 */
void writeDiskInfo()
{
    unsigned char data[255];
    unsigned char i;
    for (i=0;i<255;data[i++]=0) ;
    data[0xe3] = 0x16; // IMPORTANT! 80 track double sided
    data[0xe4] = count;
    data[0xe1] = freeSec;
    data[0xe2] = freeTrack;
    data[0xe7] = 0x10;
    data[0xea] = 32;
    data[0xf5] = 's';
    data[0xf6] = 'c';
    data[0xf7] = 'l';
    data[0xf8] = '2';
    data[0xf9] = 't';
    data[0xfa] = 'r';
    data[0xfb] = 'd';
    esx_f_write(oStream, &data, 256);
    for (i=0;i<255;i++) data[i] = 0;
    esx_f_write(oStream, 0, 1792);
}

/**
 * In SCL data already written by sectors - just copy as is!
 */
void writeData()
{
    unsigned char buf[256];
    int r = esx_f_read(iStream, &buf, 255);
    while (r > 4 && !errno) {
        esx_f_write(oStream, &buf, r);
        r = esx_f_read(iStream, &buf, 255);
    }
}

void showSclFiles()
{
    char *ext = ".SCL";
    unsigned char dirDescr;
    unsigned char cwd[ESX_PATHNAME_MAX+1];
    struct esx_dirent file;

    esx_f_getcwd(cwd);
    dirDescr = esx_f_opendir_ex(cwd, ESX_DIR_USE_HEADER);
    printf("Current directory %s.\nYou may convert:\n", cwd);

    while ((esx_f_readdir(dirDescr, &file)) && (errno == 0))
    {
        if (strcmp(&file.name[9], ext) == 0) printf("%s\n", file.name);
    }
}

void main(void) 
{
    printf("scl2trd converter\n(L) 2019 Alexander Sharikhin\n\n");
    showSclFiles();
    while (1) {
        openFile();
        validateScl();
        importCatalog();
        writeDiskInfo();
        writeData();
        esx_f_close(oStream);
        esx_f_close(iStream);
        printf("\nReady\n");
    }
}