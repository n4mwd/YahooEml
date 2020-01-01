
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <mem.h>
#include <ctype.h>
#include <values.h>
#include <dir.h>
#undef   isalpha



#define TRUE   1
#define FALSE  0
#define CP_1252  1252

enum {COMPACTEML=0, RAWEML, BBEML, XMLEML };

enum { ENC_7BIT=0, ENC_8BIT, ENC_BINARY, ENC_QP, ENC_BASE64, ENC_UNKNOWN};

// Content types - order here is critical
enum { CT_UNSPECIFIED=0,
       CT_TEXT_PLAIN, CT_TEXT_HTML,
       CT_MULTI_MIXED, CT_MULTI_ALT, CT_MULTI_RELATED,
       CT_APP_ZIP, CT_APP_PDF, CT_APP_MSWORD,
       CT_AUDIO_MPEG, CT_AUDIO_OGG, CT_AUDIO_MP3, CT_AUDIO_WAV,
       CT_IMAGE_PNG, CT_IMAGE_JPEG, CT_IMAGE_GIF, CT_IMAGE_BMP,
       CT_VIDEO_MP4, CT_VIDEO_AVI, CT_VIDEO_MPEG,
       CT_TEXT_UNKNOWN=50, CT_MULTI_UNKNOWN, CT_APP_UNKNOWN, CT_AUDIO_UNKNOWN,
       CT_IMAGE_UNKNOWN, CT_VIDEO_UNKNOWN, CT_MESSAGE_UNKNOWN};


// macro to determine if this is a multipart mime type
#define ISMULTIPART(x) ((x) >= CT_MULTI_MIXED && (x) <= CT_MULTI_UNKNOWN)

// This structure stores a single mime record.
// Null pointers mean object not found.

typedef struct MIMEREC
{
    char    *BodyStart;     // Points to the start of the body after the headers.
    int      BodyLen;   	// Length of body before processing and subdividing.
    unsigned ContentType;   // "text/html", "multipart/mixed", etc.
    char    *To;            // To Field
    char 	*Charset;       // "utf-8", "ISO-8859-1", etc. Part of content type.
    char 	*Boundary;      // Boundary string also from content type.
                            // this field describes the boundary string used for
                            // child mime nodes.  It will be blank if there are
                            // no children under it.
    char 	*FileName;      // "image.jpg", etc form content disposition.
    unsigned Encoding;    	// "7bit", "8bit", "binary", "quoted-printable", "base64"
    unsigned SenderIp;    	// Ip address of sender
    int      Flag;          // flag for searching
    struct   MIMEREC *Child;// First child node
    struct 	 MIMEREC *Next; // Next mime record at the same level - NULL at end
    struct   MIMEREC *Parent;  // Parent node - NULL if root node
    struct   MIMEREC *Prev;  // Previuous node at this level or NULL if first.
} MIMEREC;



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
    char *DirName;      // relative path to email directory 
} JSONHEADERS;




int UnEscapeString(char *Buf);
void ProcessJson(int MsgNum, int EmlMode, char *DirName);
int ParseJson(JSONHEADERS *jhdr, char *Buf, int BufLen);
int CpToCp1252(char *CodePageStr, char *buf);
void Base64Print(FILE *fpOut64, FILE *fpInBin);
int Base64ToBinary(char *Buf);
MIMEREC *ParseEmail(char *Buf);
void WriteCompactEml(JSONHEADERS *jHdr, MIMEREC *MimeRec, FILE *fp);
void FreeMimePartList(MIMEREC *MimeRec);
char const *GetContTypeStr(int EnumVal);
int QPToBinary(char *Buf);
void RemoveYinv(char *Buf);
int Html2BBCode(char *Buf);
int PrintBBCode(FILE *fp, char *Buf);
char *stristr(const char *haystack, const char *needle);
int PrintHtmlCode(FILE *fp, char *Buf);
int NormalizeFontColor(char *str);
int NormalizeFontSize(char *str);
char *GetHtmlColorString(int val);
void WriteBBCode(JSONHEADERS *jHdr, MIMEREC *MimeRec, FILE *fp);








