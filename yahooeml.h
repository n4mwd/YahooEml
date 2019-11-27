
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <mem.h>
#include <ctype.h>
#include <values.h>
#include <dir.h>



#define TRUE   1
#define FALSE  0
#define CP_1252  1252
enum {COMPACTEML=0, RAWEML, BBEML, XMLEML };


typedef struct
{
    int MsgNum;         // current mesage number
    unsigned PostDate;  // Post date in unix format
    int TopicId;        // The first message number of this topic
    int NextInTopic;    // Message number of next message in thread
    int PrevInTopic;    // Message number of previous message in thread
    char *Subject;      // Pointer to subject string
    char *SenderEmail;  // email address of poster
    char *YahooId;      // Yahoo ID of sender
    char *RawEmail;     // Ponter to full email text
} JSONHEADERS;




int UnEscapeString(char *Buf);
void ProcessJson(int MsgNum, int EmlMode, char *OutDirName);
int ParseJson(JSONHEADERS *jhdr, char *Buf, int BufLen);
int CpToCp1252(int CodePage, char *buf);
void Base64Print(FILE *fp, char *Buf, int Len);
int Base64ToBinary(char *Buf);
void ParseEmail(char *Buf);



