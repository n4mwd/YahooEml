
#include "YahooEml.h"
#include <time.h>

enum {HASATTACH=0x01, HASPLAIN=0x02, HASHTML=0x04 }


// Return a set of flags showing what elements are present in this email

unsigned FindParts(MIMEREC *MimeRec)
{
    unsigned res = 0;

    while (MimeRec)
    {
        if (MimeRec->Child) res |= FindParts(MimeRec->Child);

        if (MimeRec->FileName) res |= HASATTACH;
        else if (MimeRec->ContentType == CT_TEXT_HTML) res |= HASHTML;
        else if (MimeRec->ContentType == CT_TEXT_PLAIN) res|= HASPLAIN;

        MimeRec = MimeRec->Next;
    }

    return(res);
}

int CountAttach(MIMEREC *MimeRec)
{
    int Count = 0;

    while (MimeRec)
    {
        Count += CountAttach(MimeRec->Child);
        if (MimeRec->FileName) Count++;
        MimeRec->Flag = FALSE;                   // clear flag
        MimeRec = MimeRec->Next;
    }

    return(Count);   
}


// Search for the next MimeRec with a matching type.
// Returns NULL if no more records found.
// Ctype can be HASATTACH, HASTEXT,  or HASHTML

MIMEREC *GetNextRec(MIMEREC *MimeRec, int Ctype)
{
    while (MimeRec)
    {
        if (MimeRec->Child)
        {
            MIMEREC *ptr;

            ptr = GetNextRec(MimeRec->Child, Ctype);
            if (ptr) return(ptr);
        }

        if (!MimeRec->Flag)    // ignore if we have already returned it
        {
            if ((Ctype == HASATTACH && MimeRec->FileName) ||
                (Ctype == HASPLAIN && MimeRec->ContentType == CT_TEXT_PLAIN) ||
                (Ctype == HASHTML && MimeRec->ContentType == CT_TEXT_HTML))
            {
                MimeRec->Flag = TRUE;   // mark as returned
                return(MimeRec);
            }
        }

        MimeRec = MimeRec->Next;
    }

    return(NULL);   // not found
}


char *GetMessageAscii(MIMEREC *TextRec)
{
    int i;
    char *retstr;

    if (!TextRec->BodyStart /* || !TextRec->BodyEnd */)
        return(NULL);

    retstr = TextRec->BodyStart;

    switch(TextRec->Encoding)
    {
        case ENC_7BIT:      // take bytes as is
        case ENC_8BIT:
        case ENC_BINARY:
        case ENC_UNKNOWN:
            break;

        case ENC_QP:        // decode quoted printable into binary.
            i = QPToBinary(TextRec->BodyStart);
            TextRec->BodyStart[i] = 0;     // null terminate output
            break;

        case ENC_BASE64:    // decode Base64 into binary
            i = Base64ToBinary(TextRec->BodyStart);
            TextRec->BodyStart[i] = 0;   // null terminate
            break;

        default:
            retstr = NULL;
            break;
    }

    if (retstr && TextRec->Charset)   // convert charset if necessary
    {

        CpToCp1252(TextRec->Charset, retstr);
    }

    return(retstr);
}




// convert an "encoded word" string to US_ASCII

char *UnEncodeWord(char *Str)
{

    return(Str);
}

// convert unix time to ascii
// Proper date format is: "Date: Thu, 28 Nov 2019 13:04:06 -0500"
                 

char *UnixTime2Ascii(time_t t)
{
    static char str[80];

    strftime(str, sizeof(str), "%a, %d %b %Y %H:%M:%S %z", localtime(&t));

    return(str);
}


// Convert IP in unsigned to dotted quad string

char *Ip2Ascii(unsigned ip)
{
    static char str[16];

    sprintf(str, "%d.%d.%d.%d", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
            (ip >> 8) & 0xFF, ip & 0xFF);

    return(str);
}


// search for and open the attachment if found

FILE *OpenAttachment(char *DirName, int MsgNum, char *fname)
{
    char TmpStr[512];
    FILE *fp;

    if (!fname || strlen(fname) > sizeof(TmpStr) - 64) return(NULL);

    sprintf(TmpStr, "%s/%d-%s", DirName, MsgNum, fname);
    fp = fopen(TmpStr, "rb");

    return(fp);
}


void WriteCompactEml(JSONHEADERS *jHdr, MIMEREC *MimeRec, FILE *fp)
{
    unsigned parts, ctype;
    char *Boundary = ",,Yahoo/Eml/Converter:=?";
    char *Unk = "Unknown";
    char *ptr, *TextPtr;
    MIMEREC *TextRec, *AttachRec;
    //int OriginalLength;

    // write headers

    // Write FROM:
    UnEscapeString(jHdr->SenderEmail);
    fprintf(fp, "From: %s\r\n", (jHdr->SenderEmail) ? UnEncodeWord(jHdr->SenderEmail) : Unk);

    // Write TO:
    UnEscapeString(MimeRec->To);
    fprintf(fp, "To: %s\r\n", MimeRec->To ? UnEncodeWord(MimeRec->To) : Unk);

    // Write SUBJECT:
    UnEscapeString(jHdr->Subject);
    fprintf(fp, "Subject: %s\r\n", jHdr->Subject ? UnEncodeWord(jHdr->Subject) : Unk);

    // Write DATE:
    fprintf(fp, "Date: %s\r\n", UnixTime2Ascii((time_t) jHdr->PostDate));

    // Write X-Originating-IP:
    if (MimeRec->SenderIp)
        fprintf(fp, "X-Originating-IP: %s\r\n", Ip2Ascii(MimeRec->SenderIp));

    // Write X-Yahoo-Profile: (Poster User ID)
    if (jHdr->YahooId)
        fprintf(fp, "X-Yahoo-Profile: %s\r\n", UnEncodeWord(jHdr->YahooId));

    // Write X-Msg-Id:    -- message number
    fprintf(fp, "X-Msg-Id: %d\r\n", jHdr->MsgNum);

    // Write X-Topic-Id:  -- message number of first topic
    fprintf(fp, "X-Topic-Id: %d\r\n", jHdr->TopicId);

    // Write X-Topic-Next: -- message number of next in thread
    fprintf(fp, "X-Topic-Next: %d\r\n", jHdr->NextInTopic);

    // Write X-Topic-Prev: -- Message Number of previous in thread
    fprintf(fp, "X-Topic-Prev: %d\r\n", jHdr->PrevInTopic);

    // Write MIME-Version: 1.0
    fprintf(fp, "MIME-Version: 1.0\r\n");

    parts = FindParts(MimeRec);
    if (parts & HASATTACH)
    {
        // multipart/mixed because there are attachments
        fprintf(fp, "Content-Type: multipart/mixed; boundary=%s\r\n", Boundary);
        fprintf(fp, "\r\n");   // end of headers marker
        fprintf(fp, "\r\n--%s\r\n", Boundary);   // boundary marker

    }

    fprintf(fp, "Content-Type: text/%s; ", (parts & HASHTML) ? "html" : "plain");
    fprintf(fp, "charset=Windows-1252;\r\n");
    fprintf(fp, "Content-Transfer-Encoding: 8bit\r\n");
    fprintf(fp, "\r\n");   // end of headers marker

    ctype = parts & HASHTML;
    if (!ctype) ctype = HASPLAIN;

    TextPtr = NULL;
    while ((TextRec = GetNextRec(MimeRec, ctype)) != NULL)
    {
        if (TextPtr) fprintf(fp, "\r\n=====-\r\n");  // Been here before
        //OriginalLength = strlen(TextRec->BodyStart);
        TextPtr = GetMessageAscii(TextRec);
        //UnEscapeString(TextPtr);
        RemoveYinv(TextPtr);    // remove yahoo css
        if (TextRec->ContentType == CT_TEXT_HTML)
        {
            Html2BBCode(TextPtr);
            //PrintBBCode(fp, TextPtr);
            PrintHtmlCode(fp, TextPtr);

            // if html out, we reconvert it to html here
        }
        else
        {
	        // output the text
    	    if (TextPtr)
        	{
            	for (ptr = TextPtr; *ptr; ptr++)
		        {
    		        fputc(*ptr, fp);
        		}
            }
	    }
    }


    if (parts & HASATTACH)
    {
        FILE *fpAt;

        while ((AttachRec = GetNextRec(MimeRec, HASATTACH)) != NULL)
        {
            fprintf(fp, "\r\n--%s\r\n", Boundary);   // boundary marker

            // print attachment boundary header stuff
            fprintf(fp, "Content-Type: %s; name=\"%s\"\r\n",
                GetContTypeStr(AttachRec->ContentType),
                AttachRec->FileName);
            fprintf(fp, "Content-Disposition: attachment; filename=\"%s\"\r\n",
                AttachRec->FileName);
            fprintf(fp, "Content-Transfer-Encoding: base64\r\n");
            fprintf(fp, "\r\n");   // end of headers marker

            // locate attachment file and base64 it here
            fpAt = OpenAttachment(jHdr->DirName, jHdr->MsgNum, AttachRec->FileName);
            if (fpAt)
            {
                Base64Print(fp, fpAt);
                fclose(fpAt);
            }
            else
            {
                fprintf(fp, "[ Attachment could not be found ] \n");
            }

        }

        fprintf(fp, "\r\n--%s--\r\n", Boundary);   // final boundary marker
    }


    // Content-Type:
    //Content-Type: multipart/alternative;
//  boundary="----=_Part_223176_1800133257.1505487319309"
//Content-Type: text/plain; charset=utf-8; format=flowed
//Content-Transfer-Encoding: 8bit
//--------------26D1C33446561D3F6AF764BC
//Content-Type: text/html; charset=utf-8
//Content-Transfer-Encoding: 8bit
//Content-Type: text/plain; charset="ISO-8859-1"
//Content-Transfer-Encoding: quoted-printable



}



void WriteBBCode(JSONHEADERS *jHdr, MIMEREC *MimeRec, FILE *fp)
{
    unsigned parts, ctype;
    char *Unk = "Unknown";
    char *ptr, *TextPtr;
    MIMEREC *TextRec, *AttachRec;
    int i;

    // write headers

    // Write FROM:
    UnEscapeString(jHdr->SenderEmail);
    fprintf(fp, "From: %s\r\n", (jHdr->SenderEmail) ? UnEncodeWord(jHdr->SenderEmail) : Unk);

    // Write TO:
    UnEscapeString(MimeRec->To);
    fprintf(fp, "To: %s\r\n", MimeRec->To ? UnEncodeWord(MimeRec->To) : Unk);

    // Write SUBJECT:
    UnEscapeString(jHdr->Subject);
    fprintf(fp, "Subject: %s\r\n", jHdr->Subject ? UnEncodeWord(jHdr->Subject) : Unk);

    // Write DATE:
    fprintf(fp, "Date: %s\r\n", UnixTime2Ascii((time_t) jHdr->PostDate));

    // Write X-Originating-IP:
    if (MimeRec->SenderIp)
        fprintf(fp, "X-Originating-IP: %s\r\n", Ip2Ascii(MimeRec->SenderIp));

    // Write X-Yahoo-Profile: (Poster User ID)
    if (jHdr->YahooId)
        fprintf(fp, "X-User-Id: %s\r\n", UnEncodeWord(jHdr->YahooId));

    // Write X-Msg-Id:    -- message number
    fprintf(fp, "X-Msg-Id: %d\r\n", jHdr->MsgNum);

    // Write X-Topic-Id:  -- message number of first topic
    fprintf(fp, "X-Topic-Id: %d\r\n", jHdr->TopicId);

    // Write X-Topic-Next: -- message number of next in thread
    fprintf(fp, "X-Topic-Next: %d\r\n", jHdr->NextInTopic);

    // Write X-Topic-Prev: -- Message Number of previous in thread
    fprintf(fp, "X-Topic-Prev: %d\r\n", jHdr->PrevInTopic);

    // Write MIME-Version: 1.0
    fprintf(fp, "X-Charset: UTF-8\r\n");

    parts = FindParts(MimeRec);
    if (parts & HASATTACH)
    {
        // Print attachment headers
        i = 0;
        while ((AttachRec = GetNextRec(MimeRec, HASATTACH)) != NULL)
        {
            fprintf(fp, "X-Attach: \"%s\" ID=%d\r\n", AttachRec->FileName, ++i);
        }
        CountAttach(MimeRec);    // reset all flags
    }

    fprintf(fp, "\r\n");   // end of headers marker

    // Now go through and print all HTML if its there else print all text email
    ctype = parts & HASHTML;
    if (!ctype) ctype = HASPLAIN;

    TextPtr = NULL;
    while ((TextRec = GetNextRec(MimeRec, ctype)) != NULL)
    {
        if (TextPtr) fprintf(fp, "\r\n======\r\n");  // Been here before
        //OriginalLength = strlen(TextRec->BodyStart);
        TextPtr = GetMessageAscii(TextRec);
        //UnEscapeString(TextPtr);
        RemoveYinv(TextPtr);    // remove yahoo css
        if (TextRec->ContentType == CT_TEXT_HTML)    // converting HTML
        {
            Html2BBCode(TextPtr);           // convert to tokenized BBCode
            // BBCleanup(TextPtr);  -- not working yet
            PrintBBCode(fp, TextPtr);
        }
        else   // text only - no conversion
        {
	        // output the text
    	    if (TextPtr)
        	{
            	for (ptr = TextPtr; *ptr; ptr++)
		        {
    		        fputc(*ptr, fp);
        		}
            }
	    }
    }

    // Now we process the attachments

    if (parts & HASATTACH)
    {
        FILE *fpAt;

        while ((AttachRec = GetNextRec(MimeRec, HASATTACH)) != NULL)
        {
            // This boundary marker is illegal in UTF-8 so it makes a good
            // universal boundary marker.
            fprintf(fp, "\r\n%c\r\n", 0xFF);   // boundary marker

            // In BB files, there is no header here, just raw Base64 code

            // locate attachment file and base64 it here
            fpAt = OpenAttachment(jHdr->DirName, jHdr->MsgNum, AttachRec->FileName);
            if (fpAt)
            {
                Base64Print(fp, fpAt);
                fclose(fpAt);
            }
            else
            {
                fprintf(fp, "[ Attachment could not be found ] \n");
            }

        }
    }
}



