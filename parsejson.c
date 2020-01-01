
#include "YahooEml.h"

// This file accepts a json file in a memory buffer.
// The output is a JSONHEADER sructure.
// The input buffer is modified.


// Parse JSON File
// Buf is a buffer with the entire file contents.
// BufLen is the length of the file.
// Returns 0 if file successfully parsed, else 1 for error, 2 for fatal error.

int ParseJson(JSONHEADERS *jhdr, char *Buf, int BufLen)
{
    int i;
    char ch;
    int state, DataType, DataNum;
    char *KeyStr, *DataStr;

    state = 0;
    memset(jhdr, 0, sizeof(JSONHEADERS));
    
    for (i = 0; i < BufLen; i++)
    {
        ch = Buf[i];
        if (ch == '\\')    // skip escaped chars
        {
            i++;
            continue;
        }

        if (state == 0)         // 0=outside quotes, key side
        {
            KeyStr = DataStr = NULL;   // clear pointers
            DataType = DataNum = 0;
            if (ch == '"')      // found the quote
            {
                state = 1;
                KeyStr = &Buf[i+1];  // Set Key pointer
            }
        }
        else if (state == 1)    // 1=inside key quote
        {
            if (ch == '"')      // found ending quote
            {
                state = 2;
                Buf[i] = 0;     // Null terminate key
            }
        }
        else if (state == 2)    // 2=between key and colon
        {
            if (ch == ':')      // Found colon
                state = 3;
        }
        else if (state == 3)    // looking for data
        {
            if (ch == '"')      // found starting quote of data
            {
                state = 4;      // look for end quote state
                DataStr = &Buf[i + 1];
                DataType = 2;    // string type
            }
            else if (ch == 't')   // start of 'true'
            {
                state = 5;      // just advance to final data state
                DataType = 1;
                DataNum = 1;
            }
            else if (ch == 'f')   // start of 'false'
            {
                state = 5;      // just advance to final data state
                DataType = 1;
                DataNum = 0;
            }
            else if (ch == '-' || isdigit(ch))   // start of digit
            {
                state = 5;      // just advance to final data state
                DataType = 1;
                DataNum = atoi(&Buf[i]);
            }
            else if (ch == '{' || ch == '[')   // start of another block
            {
                DataType = 3;
                state = 5;
            }
            else if (ch == '}' || ch == ']' || ch == ',')   // End of block found
            {
                // We shouldn't be here
                state = 0;
            }
        }
        else if (state == 4)    // Look for end of quoted data
        {
            if (ch == '"')      // ending quote of data
            {
                state = 5;
                Buf[i] = 0;     // Null terminate data
            }
        }
        else if (state == 5)    // Finalize Node processing
        {
            if (DataType == 2)  // string
            {
                // check to see if string can be catagorized as a number or boolean
                if (strcmp(DataStr, "false") == 0)
                {
                    DataType = 1;  // Number
                    DataNum = 0;
                }
                else if (strcmp(DataStr, "true") == 0)
                {
                    DataType = 1;  // Number
                    DataNum = 1;
                }
                else if (DataStr[0] == 0)    // Null
                {
                    DataType = 0;
                    DataNum = 0;
                }
                else if (strspn(DataStr, "-0123456789") == strlen(DataStr))
                {
                    // data is likely a number
                    DataType = 1;  // Number
                    DataNum = atoi(DataStr);
                }
            }
            state = 0;     // back to start again

            // Store headers
            if (strcmp(KeyStr, "msgId") == 0) jhdr->MsgNum = DataNum;
            else if (strcmp(KeyStr, "postDate") == 0) jhdr->PostDate = (unsigned) DataNum;
            else if (strcmp(KeyStr, "topicId") == 0) jhdr->TopicId = DataNum;
            else if (strcmp(KeyStr, "nextInTopic") == 0) jhdr->NextInTopic = DataNum;
            else if (strcmp(KeyStr, "prevInTopic") == 0) jhdr->PrevInTopic = DataNum;
            else if (strcmp(KeyStr, "subject") == 0) jhdr->Subject = DataStr;
            else if (strcmp(KeyStr, "from") == 0) jhdr->SenderEmail = DataStr;
            else if (strcmp(KeyStr, "profile") == 0) jhdr->YahooId = DataStr;
            else if (strcmp(KeyStr, "rawEmail") == 0) jhdr->RawEmail = DataStr;
        }
        else break;   // terminate for loop if state invalid
    }

    return(0);
}



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





void ProcessJson(int MsgNum, int EmlMode, char *DirName)
{
    char *Buf;
    char Tmpstr[256];
    unsigned BufLen;
    JSONHEADERS jhdr;
    char *ptr;


    // Fabricate file name and read file
    sprintf(Tmpstr, "%s/%d_raw.json", DirName, MsgNum);
    Buf = ReadJsonFile(Tmpstr, &BufLen);
    if (Buf == NULL) return;     // Quietly return if file not found

    // Parse JSON file into record of keys and data.
    if (ParseJson(&jhdr, Buf, BufLen))
        printf("Error processing JSON file: %s\n", Tmpstr);

    printf("Message Number: %d\n", jhdr.MsgNum);
    jhdr.DirName = DirName;

    ptr = jhdr.RawEmail;
    if (ptr)
    {
        FILE *fp;
        MIMEREC *MimeRec;
        int NewLen;

        NewLen = UnEscapeString(ptr);
        if (EmlMode == RAWEML)   // just output the raw email
        {
            sprintf(Tmpstr, "%s/eml/%d_raw.eml", DirName, MsgNum);
            fp = fopen(Tmpstr, "wb");
            if (fp)
            {
                fwrite(ptr, NewLen, 1, fp);
                fclose(fp);
            }

        }
        else    // Output has to be processed
        {
			MimeRec = ParseEmail(ptr);
            if (!MimeRec) return;

            if (EmlMode == COMPACTEML)
            {
                // output as compact .eml and re-attach attachments
                sprintf(Tmpstr, "%s/eml/%d.eml", DirName, MsgNum);
                fp = fopen(Tmpstr, "wb");
                if (fp)
                {
                    WriteCompactEml(&jhdr, MimeRec, fp);
                    fclose(fp);
                }

            }
            else if (EmlMode == BBEML)
            {
                // Output as BBCode file (.bb) with attachments
                sprintf(Tmpstr, "%s/eml/%d.bb", DirName, MsgNum);
                fp = fopen(Tmpstr, "wb");
                if (fp)
                {
                    WriteBBCode(&jhdr, MimeRec, fp);
                    fclose(fp);
                }
            }
            else if (EmlMode == XMLEML)
            {
                // Output single XML file (.xml) as BBCode and all attachments
            }

            // Free the mime part list
            FreeMimePartList(MimeRec);

    	}
    }

    free(Buf);
}







