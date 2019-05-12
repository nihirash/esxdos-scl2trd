#include <input.h>
#include <stdlib.h>
#include <string.h>

#include "lib/textUtils.h"
#include "lib/esxdos.h"

#define PATH_SIZE ( 200 )
uint8_t filePath[ PATH_SIZE ];
uint16_t drive;
uint16_t iStream;
uint16_t oStream;
uint8_t buff[256];
unsigned freeTrack = 1;
unsigned freeSec = 0;
unsigned char count;

void cleanBuffer()
{
    for (uint16_t i=0;i<256;buff[i++] = 0); 
}

void showMessage(char *e) 
{
    textUtils_cls();
    textUtils_println( e );
    textUtils_println( "Press any key..." );
    waitKeyPress();
} 

void writeDiskData()
{
    uint16_t r = ESXDOS_fread(&buff, 255, iStream);
    while (r > 4) {
        ESXDOS_fwrite(&buff, r, oStream);
        r = ESXDOS_fread(&buff, 255, iStream);
    }
    iferror {
        showMessage("Issue with writing disk data to TRD-file");
        return;
    }

    ESXDOS_fclose(iStream);
    ESXDOS_fclose(oStream);
    textUtils_println(" * All data written!");
    textUtils_println("");
    textUtils_println(" TRD file is stored and ready to use!");
    textUtils_println("");
    textUtils_println(" Press any key to convert other SCL-file");
    waitKeyPress();
}

void writeDiskInfo()
{
    cleanBuffer();
    buff[0xe3] = 0x16; // IMPORTANT! 80 track double sided
    buff[0xe4] = count;
    buff[0xe1] = freeSec;
    buff[0xe2] = freeTrack;
    buff[0xe7] = 0x10;
    buff[0xea] = 32;
    buff[0xf5] = 's';
    buff[0xf6] = 'c';
    buff[0xf7] = 'l';
    buff[0xf8] = '2';
    buff[0xf9] = 't';
    buff[0xfa] = 'r';
    buff[0xfb] = 'd';
    ESXDOS_fwrite(&buff, 256, oStream);
    ESXDOS_fwrite(0, 1792, oStream); // Any dirt is ok
    iferror {
        showMessage("Issue with writing disk info to TRD-file");
        return;
    }
    textUtils_println(" * Disk information written");
    writeDiskData();
}

void writeCatalog()
{
    uint8_t i;

    oStream = ESXDOS_fopen(filePath, ESXDOS_FILEMODE_WRITE_CREATE, drive);
    iferror {
        showMessage("Can't open output file");
        return ;
    }

    ESXDOS_fread(&count, 1, iStream);
    for (i=0;i<count; i++) {
        ESXDOS_fread(&buff, 14, iStream);
        buff[14] = freeSec;
        buff[15] = freeTrack;
        freeSec += buff[0xd];
        freeTrack += freeSec / 16;
        freeSec = freeSec % 16;
        ESXDOS_fwrite(&buff, 16, oStream);
    }
    cleanBuffer();

    for (i = count;i<128;i++) ESXDOS_fwrite(&buff, 16, oStream);
    iferror {
        showMessage("Issue with writing catalog to TRD-file");
        return;
    }
    textUtils_println(" * Disk catalog written");
    writeDiskInfo();
}


void validateScl()
{
    unsigned char *expected[9] = "SINCLAIR";
    drive = ESXDOS_getDefaultDrive();
    iStream = ESXDOS_fopen(filePath, ESXDOS_FILEMODE_READ, drive );
    iferror {
        showMessage("Can't open input file");
        return ;
    }

    cleanBuffer();
    ESXDOS_fread(&buff, 8, iStream);
    if (strcmp(expected, &buff)) {
        showMessage("Wrong file! Select only SCL files");
        return;
    }
    sprintf(strstr(filePath, ".SCL"), ".TRD");
    textUtils_println(" * File is valid SCL");
    writeCatalog();
}

void selectFile() 
{
    textUtils_cls();
    textUtils_setAttributes( INK_BLUE | PAPER_BLACK );
    fileDialogpathUpOneDir( filePath );
 
    while ( openFileDialog( "Open SCL file", filePath, PATH_SIZE, INK_BLUE | PAPER_WHITE, INK_WHITE | PAPER_BLACK ) == false ) {
        showMessage("Didn't select a file.");
    }
    textUtils_cls();
    textUtils_println("");
    textUtils_println(" Converting file!");

    validateScl();
}

void main(void)
{
    sprintf( filePath, "/" );
    textUtils_64ColumnsMode();
    
    zx_border( INK_BLACK );
    while (1) 
    {
        selectFile();
        ESXDOS_fclose(iStream);
        ESXDOS_fclose(oStream);
    }
}