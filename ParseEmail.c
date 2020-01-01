
#include "YahooEml.h"

// Content type table - order must match enums
static char const *ContTypeTbl[] =
{
    "", "text/plain", "text/html",
    "multipart/mixed", "multipart/alternative", "multipart/related",
    "application/zip","application/pdf","application/msword",
    "audio/mpeg","audio/ogg","audio/mp3","audio/wav",
    "image/png","image/jpeg","image/gif", "image/bmp",
    "video/mp4","video/avi","video/mpeg",
    NULL
};

// Content type table - order must match enums
static char const *ContTypeTblMain[] =
{
    "text/", "multipart/", "application/", "audio/",
    "image/", "video/", "message/", NULL
};

char const *GetContTypeStr(int EnumVal)
{
    static char RetBuf[20];  // used in special cases

    if (EnumVal >= CT_UNSPECIFIED && EnumVal <= CT_VIDEO_MPEG)
        return(ContTypeTbl[EnumVal]);        // well known content type

    if (EnumVal >= CT_TEXT_UNKNOWN && EnumVal <= CT_MESSAGE_UNKNOWN)
    {
        strcpy(RetBuf, ContTypeTblMain[EnumVal - CT_TEXT_UNKNOWN]);
        strcat(RetBuf, "unknown");
        return(RetBuf);
    }

    return(NULL);   // Not a defined content type enum
}



// Return a pointer to a line.
// Parameter Str points to the start of the line and will be used for
// further processing.
//
// If IsHeader is true, null terminates the Str at the newline then return
// the number of characters in the line.
// If IsHeader is false, do not modify input Str and return TRUE if Str
// Points to a valid string (not null terminated).
//
// On Exit, Parameter EndPtr is set to the start of the next line and will be
// used for the next call as the Str parameter.
//
// CR's are converted to spaces and LF's are converted to nulls.  However,
// if the line is being processed as a header line, and the next line starts
// with a space, the previous Lf is instead changed to a space so the line
// appears as one long line.  This is folding white space which is allowed in
// headers.
//
// Returns number of chars in line not counting CR or LF's.
// On EOF, returns -1 and Str is invalid.


int GetEmailLine(char *Str, char **EndPtr, int IsHeader)
{
    char ch;
    int LineChars = 0, NumChars = 0;

    if (!Str || !EndPtr) return(0);

//if (memcmp(Str, "X-Rocket-Track:", 15) == 0)
//printf("");

    while (LineChars < 1024)
    {
        ch = *Str;
//printf("%c", ch);
        if (ch == 0) //  end of email
        {
            if (NumChars == 0) return(-1);  // EOF
            else
            {
                *EndPtr = Str;     // signal error point to null char
                return(NumChars);
            }
        }
        else if (ch == '\r') *Str = ' ';  // Convert returns to spaces
        else if (ch == '\n')   // newline
        {
            LineChars = 0;
            if (IsHeader && NumChars && (Str[1] == ' ' || Str[1] == '\t'))  // folding white space
            {
                *Str = ' ';     // remove LF
                Str[1] = ' ';  // Force FWS to be a space
            }
            else   // end of line
            {
                if (IsHeader) *Str = 0;    // null terminate only for header
                *EndPtr = Str + 1;
                return(NumChars);
            }
        }
        else
        {
            NumChars++;    // count all other chars
            LineChars++;
        }

        Str++;
    }

    printf("Illegal line length in %s, skipping to next line.\n>%.20s\n",
            IsHeader ? "header" : "body", *EndPtr);

    *EndPtr = strchr(Str, '\n');
    if (IsHeader) *Str = 0;    // null terminate only for header
    if (!*EndPtr) return(-1);     // EOF

    (*EndPtr)++;    // go past \n
    LineChars = *EndPtr - Str;
    NumChars += LineChars;

    return(NumChars);

}


// Look for an IP address in str.
// Return IP address or 0 if not found.

unsigned GetStrIp(char *str)
{
    unsigned result = 0, bnum, dotcnt = 0, i;
    char *endptr;

    // Get rid of leading spaces
    while (*str && isspace(*str)) str++;  // No other spaces allowed except at end

    for (i = 0; i < 4; i++)
    {
        result <<= 8;
        if (!isdigit(*str)) return(0);   	  // must start with a digit
        bnum = strtoul(str, &endptr, 10);
        if (bnum > 255) return(0);      // must be less than 256

        result |= (unsigned char) bnum;

        // validate endptr
        if (i == 3)    // last octet
        {
            if (*endptr != ' ' && *endptr != 0) break;
        }
        else
        {
            if (*endptr == '.')
            {
                dotcnt++;
        		str = endptr + 1;
            }
            else
                break;   // illegal digit terminator
        }
    }

    if (result == 0 || dotcnt != 3 || i != 4)
        return(0);  // must have exactly 3 dots and 4 numbers

    // eliminate special  and private addresses
    if ((result & 0xFF000000) == 0x7F000000 ||   // loopback 127.0.0.0/8
        (result & 0xFFFF0000) == 0xC0A80000 ||   // private address 192.168.0.0/16
        (result & 0xFF000000) == 0x0A000000 ||   // private address 10.0.0.0/8
        (result & 0xFF0C0000) == 0x64400000 ||   // private address 100.64.0.0/10
        (result & 0xFFF00000) == 0xAC100000 ||   // private address 172.16.0.0/12
        (result & 0xFFFFFF00) == 0xC0000000 ||   // private address 192.0.0.0/24
        (result & 0xFFFE0000) == 0xC6120000 ||   // private address 198.18.0.0/15
        (result & 0xFF000000) == 0x00000000)     // this host address 0.0.0.0/8
            return(0);

    return(result);     // looks good
}


// Process a recieved header
// Look for an IP address and save it in MimeRec if its not 127.0.0.0

void ParseRecievedHeader(char *Params[], int ParamCnt, MIMEREC *MimeRec)
{
    int i, j, From, NotFrom;
    unsigned IpAddr;
    char ch;

    // Look for "from"
    for (From = 0; From < ParamCnt; From++)
        if (strcmpi(Params[From], "from") == 0) break;

    if (From < ParamCnt)
    {
        // Look for "by", "with", "via" or "for"
        for (NotFrom = From + 1; NotFrom < ParamCnt; NotFrom++)
            if (strcmpi(Params[NotFrom], "by") == 0 ||
                strcmpi(Params[NotFrom], "with") == 0 ||
                strcmpi(Params[NotFrom], "via") == 0 ||
                strcmpi(Params[NotFrom], "for") == 0) break;

        for (i = From + 1; i < NotFrom; i++)
        {
            // Look for an IP address

            // Convert non-IP chars to spaces
            for (j = 0; (ch = Params[i][j]) != 0; j++)
            {
                if (!isdigit(ch) && ch != '.') Params[i][j] = ' ';
            }

            // Note that the folowing does not break the loop which means that
            // only the final IP address is returned.
            IpAddr = GetStrIp(Params[i]);   // get an IP address
            if (IpAddr) MimeRec->SenderIp = IpAddr;   // store only if valid

        }
    }

}



// Input is a null terminated header string.
// Scan the header to see if its one we are interested in and process accordingly.
// MimeRec fields are modified if appropriate.
// We are concerned about the following headers:
// Received: and X_Received:
// Content-Type:
// Content-Transfer-Encoding:
// Content-Disposition:


void ParseHeaderString(char *Hdr, MIMEREC *MimeRec)
{
    char *ptr, *ptr2, *HdrName, *Params[10], ch;
    int i, t, ParamCnt = 0;


    HdrName = strtok(Hdr, ": ");
//if (strcmpi(HdrName, "Content-Transfer-Encoding") == 0)
//printf("");
    // Get the next 10 paramters in the table.
    memset(Params, 0, sizeof(Params));
    while (ParamCnt < (sizeof(Params) / sizeof(Params[0])) &&
           (ptr = strtok(NULL, "; ")) != NULL)
    {
        Params[ParamCnt++] = ptr;
    }

    // look for headers we are interested in
    if (strcmpi(HdrName, "Content-Type") == 0)   // Found Content-Type
    {
        // get content type
        t = CT_UNSPECIFIED;
        ptr = Params[0];
        for (i = 1; ContTypeTbl[i]; i++)
        {
            if (strcmpi(ptr, ContTypeTbl[i]) == 0)
                t = i;
        }

        if (t == CT_UNSPECIFIED)  // wasn't found
        {
            for (i = 0; ContTypeTblMain[i]; i++)
            {
                if (memicmp(ptr, ContTypeTblMain[i], strlen(ContTypeTblMain[i])) == 0)
                    t = i + CT_TEXT_UNKNOWN;
            }
        }

        MimeRec->ContentType = t;

        // look for other important parameters
        for (i = 1; i < ParamCnt; i++)
        {
             ptr = strtok(Params[i], "= \"");
             ptr2 = strtok(NULL, "\"");
             if (strcmpi(ptr, "charset") == 0) MimeRec->Charset = ptr2;
             else if (strcmpi(ptr, "boundary") == 0) MimeRec->Boundary = ptr2;
             else if (strcmpi(ptr, "filename") == 0) MimeRec->FileName = ptr2;
        }
    }

    else if (strcmpi(HdrName, "Content-Transfer-Encoding") == 0)
    {
        // Value can be "7bit", "8bit", "binary", "quoted-printable" or "base64"
        ptr = Params[0];
        if (strcmpi(ptr, "7bit") == 0) MimeRec->Encoding = ENC_7BIT;
        else if (strcmpi(ptr, "8bit") == 0) MimeRec->Encoding = ENC_8BIT;
        else if (strcmpi(ptr, "binary") == 0) MimeRec->Encoding = ENC_BINARY;
        else if (strcmpi(ptr, "quoted-printable") == 0) MimeRec->Encoding = ENC_QP;
        else if (strcmpi(ptr, "base64") == 0) MimeRec->Encoding = ENC_BASE64;
    }

    else if (strcmpi(HdrName, "Content-Disposition") == 0)
    {
        // The only parameter we care about here is the filename
        for (i = 0; i < ParamCnt; i++)
        {
            if (strncmpi(Params[i], "filename=", 9) == 0)
            {
                ptr = Params[i] + 9;   // src ptr
                ptr2 = ptr;            // dst ptr
                MimeRec->FileName = ptr;

                // remove quotes
                while ((ch = *ptr++) != NULL)
                {
                    if (ch != '"')
                        *ptr2++ = ch;
                }
                *ptr2 = 0;
                break;
            }
        }

    }

    else if (strcmpi(HdrName, "Received") == 0 || strcmpi(HdrName, "X-Received") == 0)
    {
        ParseRecievedHeader(Params, ParamCnt, MimeRec);
    }

    else if (strcmpi(HdrName, "To") == 0)
    {
        // Stitch email parts back together
//        int x;

//        for (x = 1; x < ParamCnt; x++)
//        {
//            Params[x - 1][strlen(Params[x - 1])] = ' ';
//        }
 //       printf("%s\n", Params[0]);
        MimeRec->To = Params[0];
    }

    // All other headers are ignored
    
}



// Parse the headers starting at Buf.
// Sets the values in MIMEREC if found.
// Returns pointer to blank line (end) or NULL on error.

char *ParseHeaders(char *Buf, MIMEREC *MimeRec)
{
    char *NextHdr;
    int rt;

    if (!Buf || !MimeRec) return(NULL);

    while (Buf)
    {
        rt = GetEmailLine(Buf, &NextHdr, TRUE);
        if (rt == 0) return(NextHdr);    // return if blank line
        else if (rt == -1) break;   // EOF

//printf("[%s]\n", Buf);   // debug
        // If here, Buf should point top the header string that we need to process
        ParseHeaderString(Buf, MimeRec);


        Buf = NextHdr;
    }

    // Buf is NULL if here.  This means there was an unexpected EOF.

    return(NULL);


}

// Find Boundary
// Recursively search the node list for a boundary match.
// On entry, Buf points to the first char in the line of the suspected boundary
// string after the leading "--".
// Returns a pointer to the MIME node if found or NULL if not.
// No other processing is done.

MIMEREC *FindBoundaryNode(char *Buf, MIMEREC *MimeRoot)
{
    while (MimeRoot)
    {
        // Check this node
        if (MimeRoot->Boundary)  // only check if a boundary is present
        {
            int Len;

            Len = strlen(MimeRoot->Boundary);
            if (Len && memcmp(Buf, MimeRoot->Boundary, Len) == 0)
            {
                return(MimeRoot);
            }
        }

        // Check children
        if (MimeRoot->Child)
        {
            MIMEREC *Tmp;

            Tmp = FindBoundaryNode(Buf, MimeRoot->Child);
            if (Tmp) return(Tmp);
        }

        MimeRoot = MimeRoot->Next;

    }

    return(NULL);    // Not found
}

// Find Last Sibling
// Given a parent Node, find the last child currently in the list.
// Return NULL if there are no children.

MIMEREC *FindLastChild(MIMEREC *Root)
{
    if (!Root || !Root->Child) return(NULL);
    Root = Root->Child;
    while (Root->Next) Root = Root->Next;

    return(Root);
}





// Search the MimeRec list for a boundary match.
// On entry, Buf points to the first char in the line.
// Returns a pointer to the newly created and empty MIMEREC.
// Returns NULL if not found.

// This function recursively searches the Mime node list for a matching
// boundary.  If a match is found, it is the start of a child node or the
// terminating boundary.
// Except for the first child node, the previous BodyEnd pointer is set.
// Except for the terminal node, the function returns with a pointer to the
// newly created and empty node.  The MIME parameters and BodyStart are not set
// at this point.  Otherwise, it returns NULL.

MIMEREC *SearchBoundary(char *Buf, MIMEREC *MimeRoot)
{
    MIMEREC *Parent, *LastChild, *Tmp;

    if (memcmp(Buf, "--", 2) != 0)      // Boundaries must start with "--"
        return(NULL);                   //   This is not a boundary line

    Parent = FindBoundaryNode(Buf + 2, MimeRoot);
    if (!Parent) return(NULL);   // Not found

    // Find last child or NULL
    LastChild = FindLastChild(Parent);

    // Null terminate last child at byte prior to the previous CR-LF
    if (LastChild)
    {
        *(Buf-2) = 0;  // null terminate last child
        LastChild->BodyLen = strlen(LastChild->BodyStart);
    }    

    // now determine if this boundary string a terminal boundary
    // If so, no further processing is done.
    if (memcmp(Buf + 2 + strlen(Parent->Boundary), "--", 2) == 0)
        return(NULL);       // terminal boundaries return NULL

    // If here, this is a new child node
    // Create new node
    Tmp = malloc(sizeof(MIMEREC));
    if (!Tmp)   // not enough memory
    {
        printf("Insuffient memory to add new MIME node.\n");
        return(NULL);
    }
    memset(Tmp, 0, sizeof(MIMEREC));
    if (LastChild)    // other children already exist
    {
        LastChild->Next = Tmp;
    }
    else    // This is the first child
    {
        Parent->Child = Tmp;
    }
    Tmp->Prev = LastChild;
    Tmp->Parent = Parent;
    Tmp->Encoding = Parent->Encoding;   // inherit default encoding from parent

    // New node has been created
    // Return a pointer to the new, but empty, node.
    return(Tmp);
}


void FreeMimePartList(MIMEREC *MimeRec)
{
    MIMEREC *ptr;

    while (MimeRec)
    {
        // Children First
        if (MimeRec->Child) FreeMimePartList(MimeRec->Child);

        // Free this node then move to next sibling
        ptr = MimeRec->Next;
        free(MimeRec);
        MimeRec = ptr;
    }
}


// Parse the data pointed to by Buf as an email.
// Buf is a null terminated buffer with an email in it.

MIMEREC *ParseEmail(char *Buf)
{
    MIMEREC *MimeRec;
    MIMEREC *TmpRec; //, *LastTmpRec;
    char *ptr, *endptr;
    int rt;

    // Create top node
    MimeRec = malloc(sizeof(MIMEREC));
    if (!MimeRec)   // not enough memory
    {
        printf("Insuffient memory to add new MIME node.\n");
        return(NULL);
    }
    memset(MimeRec, 0, sizeof(MIMEREC));

    ptr = ParseHeaders(Buf, MimeRec);
    if (!ptr)
    {
         printf("Parse Error while parsing email headers.\n");
         free(MimeRec);
         return(NULL);
    }

    // If here, ptr should point to the blank line after the headers.
    MimeRec->BodyStart = ptr;
    MimeRec->BodyLen = strlen(ptr);

    if (!ISMULTIPART(MimeRec->ContentType))   // Not a multipart-mime
    {
        return(MimeRec);
    }

    // If here, we must process a multipart type email

    while (1)
    {
        // Get a null terminated line
        rt = GetEmailLine(ptr, &endptr, FALSE);
        if (rt == -1) break;   // end of buffer reached or error

        // check for boundaries
        TmpRec = SearchBoundary(ptr, MimeRec);
        if (TmpRec)   // found new non-terminal boundary
        {
            // TempRec is the new empty Mime node
            // Parse the new headers if there
            ptr = ParseHeaders(endptr, TmpRec);
            if (!ptr)
            {
        		printf("Parse Error while parsing Mime headers.\n");
         		return(NULL);
    		}

            TmpRec->BodyStart = ptr;
            endptr = ptr;    // endptr must point to start of next line
        }

        // Just go to next line
        ptr = endptr;
    }

//    if (LastTmpRec->BodyEnd == 0)
//        LastTmpRec->BodyEnd = MimeRec->BodyEnd;
    return(MimeRec);
}






#if 0

X-eGroups-Remote-IP: 66.218.66.71


For formalists, the syntax of quoted-printable data is described by
   the following grammar:

     quoted-printable := qp-line *(CRLF qp-line)

     qp-line := *(qp-segment transport-padding CRLF)
                qp-part transport-padding

     qp-part := qp-section
                ; Maximum length of 76 characters

     qp-segment := qp-section *(SPACE / TAB) "="
                   ; Maximum length of 76 characters

     qp-section := [*(ptext / SPACE / TAB) ptext]

     ptext := hex-octet / safe-char

     safe-char := <any octet with decimal value of 33 through
                  60 inclusive, and 62 through 126>
                  ; Characters not listed as "mail-safe" in
                  ; RFC 2049 are also not recommended.

     hex-octet := "=" 2(DIGIT / "A" / "B" / "C" / "D" / "E" / "F")
                  ; Octet must be used for characters > 127, =,
                  ; SPACEs or TABs at the ends of lines, and is
                  ; recommended for any character not listed in
                  ; RFC 2049 as "mail-safe".

     transport-padding := *LWSP-char
                          ; Composers MUST NOT generate
                          ; non-zero length transport
                          ; padding, but receivers MUST
                          ; be able to handle padding
                          ; added by message transports.

   IMPORTANT:  The addition of LWSP between the elements shown in this
   BNF is NOT allowed since this BNF does not specify a structured
   header field.



#endif


