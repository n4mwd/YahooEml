
#include "YahooEml.h"


// Read file into a malloc()ed buffer.
// On return, BufLen is the file length
// On error, return NULL and set BufLen to error code.
// BufLen == 0 if file could not be opened, 1 = if no memory

char *ReadJsonFile(char *FileName, unsigned *BufLen)
{
    FILE *fp;
    unsigned size;
    char *Buf;

    fp = fopen(FileName, "rb"); //binary mode
    if (fp == NULL)
    {
        if (BufLen) *BufLen = 0;
        return(NULL);
    }

    if (fseek(fp, 0, SEEK_END))
    {
        fclose(fp);
        if (BufLen) *BufLen = 0;
        return(NULL);
    }

    size = ftell(fp);
    Buf = malloc(size + 128);
    if (!Buf)
    {
        fclose(fp);
        if (BufLen) *BufLen = 1;
        return(NULL);
    }

    fseek(fp, 0, SEEK_SET);   // rewind

    fread(Buf, 1, size, fp);

    fclose(fp);
    *BufLen = size;

    return(Buf);
}





void ProcessJson(int MsgNum, int EmlMode, char *OutDirName)
{
    char *Buf;
    char Tmpstr[256];
    unsigned BufLen;
    JSONHEADERS jhdr;
    char *ptr;


    // Fabricate file name and read file
    sprintf(Tmpstr, "%s/%d_raw.json", OutDirName, MsgNum);
    Buf = ReadJsonFile(Tmpstr, &BufLen);
    if (Buf == NULL) return;     // Quietly return if file not found

    // Parse JSON file into record of keys and data.
    if (ParseJson(&jhdr, Buf, BufLen))
        printf("Error processing JSON file: %s\n", Tmpstr);

    printf("Message Number: %d\n", jhdr.MsgNum);


    ptr = jhdr.RawEmail;
    if (ptr)
    {
        UnEscapeString(ptr);

        if (EmlMode == RAWEML)   // just output the raw email
        {
            FILE *fp;

            sprintf(Tmpstr, "%s/eml/%d_raw.eml", OutDirName, MsgNum);
            fp = fopen(Tmpstr, "wb");
            if (fp)
            {
                fwrite(ptr, strlen(ptr), 1, fp);
                fclose(fp);
            }

        }
        else
        {
            // Output has to be processed
			ParseEmail(ptr);
    	}
    }

    free(Buf);
}



