
#include "YahooEml.h"


enum { ENC_7BIT=0, ENC_8BIT, ENC_BINARY, ENC_QP, ENC_BASE64, ENC_UNKNOWN};
enum { CT_TEXT_PLAIN=0, CT_TEXT_HTML, CT_TEXT_UNKNOWN,
       CT_MULTI_MIXED, CT_MULTI_ALT, CT_MULTI_RELATED, CT_MULTI_UNKNOWN,
       CT_APPLICATION, CT_AUDIO, CT_IMAGE, CT_MESSAGE, CT_VIDEO,
       CT_UNSUPPORTED=-1 };

// macro to determine if this is a multipart mime type
#define ISMULTIPART(x) ((x) >= CT_MULTI_MIXED && (x) <= CT_MULTI_UNKNOWN)

// This structure stores a single mime record.
// Null pointers mean object not found.

typedef struct MIMEREC
{
    char    *BodyStart;     // Points to the start of the body after the headers.
    char    *BodyEnd;       // Points to end of area defined by this mime node.
                            // This is mostly the byte preceeding the "boundary--".
                            // For the root node, this is the last byte in the file.
    unsigned ContentType;   // "text/html", "multipart/mixed", etc.
    char 	*Charset;       // "utf-8", "ISO-8859-1", etc. Part of content type.
    char 	*Boundary;      // Boundary string also from content type.
                            // this field describes the boundary string used for
                            // child mime nodes.  It will be blank if there are
                            // no children under it.
    char 	*FileName;      // "image.jpg", etc form content disposition.
    unsigned Encoding;    	// "7bit", "8bit", "binary", "quoted-printable", "base64"
    unsigned SenderIp;    	// Ip address of sender
    struct   MIMEREC *Child;// First child node
    struct 	 MIMEREC *Next; // Next mime record at the same level
} MIMEREC;



// Get the next header line as a null terminated string.
// Parameter Str points to the start of the line and will be used for
// further processing.
// Parameter EndPtr is set to the start of the next line and will be used
// for the next call as the Str parameter.
// The IsHeader parameter is TRUE if the line is a header.
// CR's are converted to spaces and LF's are converted to nulls.  However,
// if the line is being processed as a header line, and the next line starts
// with a space, the previous Lf is instead changed to a space so the line
// appears as one long line.  This is folding white space which is allowed in
// headers.
// Returns number of chars in line not counting CR or LF's.
// If EndPtr is returned NULL, then an unexpected EOF was detected, however
// this does not mean the HdrStr value is invalid.


int GetEmailLine(char *Str, char **EndPtr, int IsHeader)
{
    char ch;
    int LineChars = 0, NumChars = 0;

    if (!Str || !EndPtr) return(0);

    while (LineChars < 1024)
    {
        ch = *Str;
        if (ch == 0) //  end of email
        {
            *EndPtr = NULL;     // signal error
            return(NumChars);
        }
        else if (ch == '\r') *Str = ' ';  // Convert returns to spaces
        else if (ch == '\n')   // newline
        {
            LineChars = 0;
            if (IsHeader && (Str[1] == ' ' || Str[1] == '\t'))  // folding white space
            {
                *Str = ' ';     // remove LF
                Str[1] = ' ';  // Force FWS to be a space
            }
            else   // end of line
            {
                *Str = 0;    // null terminate
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

    *EndPtr = NULL;
    return(0);
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
//return;
#if 1
    char *ptr, *ptr2, *HdrName, *Params[10];
    int i, t, ParamCnt = 0;


    HdrName = strtok(Hdr, ": ");

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
        // the type should be the first parameter
        ptr = strtok(Params[0], " /");     // Ex: "multipart"
        ptr2 = strtok(NULL, " ");          // Ex: "mixed"

        if (strcmpi(ptr, "multipart") == 0)
        {
            t = CT_MULTI_UNKNOWN;
            if (strcmpi(ptr2, "mixed") == 0) t = CT_MULTI_MIXED;
            else if (strcmpi(ptr2, "alternative") == 0) t = CT_MULTI_ALT;
            else if (strcmpi(ptr2, "related") == 0) t = CT_MULTI_RELATED;

        }
        else if (strcmpi(ptr, "text") == 0)
        {
            t = CT_TEXT_UNKNOWN;
            if (strcmpi(ptr2, "plain") == 0) t = CT_TEXT_PLAIN;
            else if (strcmpi(ptr2, "html") == 0) t = CT_TEXT_HTML;
        }
        else if (strcmpi(ptr, "application") == 0) t = CT_APPLICATION;
        else if (strcmpi(ptr, "audio") == 0) t = CT_AUDIO;
        else if (strcmpi(ptr, "image") == 0) t = CT_IMAGE;
        else if (strcmpi(ptr, "message") == 0) t = CT_MESSAGE;
        else if (strcmpi(ptr, "video") == 0) t = CT_VIDEO;

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
                MimeRec->FileName = Params[i] + 9;
                break;
            }
        }

    }

    else if (strcmpi(HdrName, "Received") == 0 || strcmpi(HdrName, "X-Received") == 0)
    {
        ParseRecievedHeader(Params, ParamCnt, MimeRec);
    }

    // All other headers are ignored
    
#endif

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
if (memcmp(Buf, "From:", 5) ==0)
printf("");
        rt = GetEmailLine(Buf, &NextHdr, TRUE);
        if (rt == 0)
            return(NextHdr);    // return if blank line

        // If here, Buf should point top the header string that we need to process
printf("[%s]\n", Buf);
        ParseHeaderString(Buf, MimeRec);

        Buf = NextHdr;
    }

    // Buf is NULL if here.  This meads there was an unexpected EOF.

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

    }

    return(NULL);    // Not found
}

// Find Last Sibling
// Given a parent Node, find the last child currently in the list.
// Return NULL if there are no children.

MIMEREC *FindLastNode(MIMEREC *Root)
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
    LastChild = FindLastNode(Parent);

    // Set BodyEnd of last child to point to byte prior to the previous CR-LF
    if (LastChild) LastChild->BodyEnd = Buf - 2;

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
    if (LastChild) LastChild->Next = Tmp;
    else Parent->Child = Tmp;

    // New node has been created
    // Return a pointer to the new, but empty, node.
    return(Tmp);
}


void ProcessNonMulti(MIMEREC *MimeRec)
{
    if (MimeRec) return;
}


void testparseheaderfunc(char *ptr, MIMEREC *MimeRec)
{
	int Len;
    char *endptr;
    MIMEREC *TempRec;
    char *encodings[] = {"default", "7bit", "8bit", "binary", "quoted-printable", "base64"};
    char *ct[] =
    {
        "unsupported",
        "multipart-unknown", "multipart-mixed", "multipart-alternative", "multipart-related",
    	"text-unknown", "text-plain", "text-html",
    	"application", "audio", "image", "message", "video"
    };

    printf("Content-Type: %s\n", MimeRec->ContentType < 14 ? ct[MimeRec->ContentType] : "Invalid Encoding");
    printf("Charset: %s\n", MimeRec->Charset ? MimeRec->Charset : "Unknown");
    printf("Boundary: %s\n", MimeRec->Boundary ? MimeRec->Boundary : "Not Speciofied");
    printf("FileName: %s\n", MimeRec->FileName ? MimeRec->FileName : "Not Specified");
    printf("Encoding: %s\n", MimeRec->Encoding < 6 ? encodings[MimeRec->Encoding] : "Invalid encoding");
    printf("SenderIP: %u.%u.%u.%u\n", MimeRec->SenderIp >> 24,
        (MimeRec->SenderIp >> 16) & 0xFF,
        (MimeRec->SenderIp >> 8) & 0xFF,
        MimeRec->SenderIp & 0xFF);

    printf("\n\n\n");

    endptr = NULL;
    while (1)
    {
        Len = GetEmailLine(ptr, &endptr, FALSE);
        if (!endptr) break;

        TempRec = SearchBoundary(ptr, MimeRec);

        if (TempRec)   // found boundary
        {
            printf("\n\n***FOUND BOUNDARY: %s\n\n", TempRec->Boundary);
        }
        else printf("%d:%s\n", Len,ptr);

        ptr = endptr;
    }

}

// Parse the data pointed to by Buf as an email.
// Buf is a null terminated buffer with an email in it.

void ParseEmail(char *Buf)
{
    MIMEREC MimeRec, *TmpRec;
    char *ptr, *endptr;

    memset(&MimeRec, 0, sizeof(MimeRec));
    ptr = ParseHeaders(Buf, &MimeRec);
    if (!ptr)
    {
         printf("Parse Error.\n");
         return;
    }

    // If here, ptr should point to the blank line after the headers.
    GetEmailLine(ptr, &ptr, FALSE);  // get blank line
    MimeRec.BodyStart = ptr;
    MimeRec.BodyEnd = MimeRec.BodyStart + strlen(MimeRec.BodyStart);

    if (!ISMULTIPART(MimeRec.ContentType))   // Not a multipart-mime
    {
        ProcessNonMulti(&MimeRec);
        return;
    }

    // If here, we must process a multipart type email

    while (1)
    {
        // Get a null terminated line
        GetEmailLine(ptr, &endptr, FALSE);
        if (!endptr) break;   // end of buffer reached or error

        // check for boundaries
        TmpRec = SearchBoundary(ptr, &MimeRec);
        if (TmpRec)   // found new non-terminal boundary
        {
            // TempRec is the new empty Mime node
            // Parse the new headers if there
            ptr = ParseHeaders(endptr, TmpRec);
            if (!ptr)
            {
        		printf("Parse Error.\n");
         		return;
    		}

            // ptr should point to the blank line after the headers.
            GetEmailLine(ptr, &ptr, FALSE);  // get blank line
            TmpRec->BodyStart = ptr;
            endptr = ptr;    // endptr must point to start of next line
        }

        // Just go to next line
        ptr = endptr;
    }
}






#if 0

X-eGroups-Remote-IP: 66.218.66.71

// Parse the headers.  Save values for
//MIME-Version: 1.0
//Content-Type: multipart/alternative;
//  boundary="----=_Part_223176_1800133257.1505487319309"
//Content-Type: text/plain; charset=utf-8; format=flowed
//Content-Transfer-Encoding: 8bit
//--------------26D1C33446561D3F6AF764BC
//Content-Type: text/html; charset=utf-8
//Content-Transfer-Encoding: 8bit
//Content-Type: text/plain; charset="ISO-8859-1"
//Content-Transfer-Encoding: quoted-printable


    application
    audio
    font
    example
    image
    message
    model
    multipart
    text
    video

    application/zip
    application/pdf
    application/msword

    audio/mpeg
    audio/ogg
    audio/mp3

    multipart/mixed
    multipart/alternative

    text/css
    text/html
    text/xml
    text/csv
    text/plain

    image/png
    image/jpeg
    image/gif

    video/mp4

Content-Disposition:
disposition := "Content-Disposition" ":"
                    disposition-type
                    *(";" disposition-parm)

     disposition-type := "inline"
                       / "attachment"
                       / extension-token
                       ; values are not case-sensitive

     disposition-parm := filename-parm
                       / creation-date-parm
                       / modification-date-parm
                       / read-date-parm
                       / size-parm
                       / parameter

     filename-parm := "filename" "=" value

     creation-date-parm := "creation-date" "=" quoted-date-time

     modification-date-parm := "modification-date" "=" quoted-date-time

     read-date-parm := "read-date" "=" quoted-date-time

     size-parm := "size" "=" 1*DIGIT

     quoted-date-time := quoted-string
                      ; contents MUST be an RFC 822 `date-time'
                      ; numeric timezones (+HHMM or -HHMM) MUST be used

=============

5.1.  Syntax of the Content-Type Header Field

   In the Augmented BNF notation of RFC 822, a Content-Type header field
   value is defined as follows:

     content := "Content-Type" ":" type "/" subtype
                *(";" parameter)
                ; Matching of media type and subtype
                ; is ALWAYS case-insensitive.

     type := discrete-type / composite-type

     discrete-type := "text" / "image" / "audio" / "video" /
                      "application" / extension-token

     composite-type := "message" / "multipart" / extension-token

     extension-token := ietf-token / x-token

     ietf-token := <An extension token defined by a
                    standards-track RFC and registered
                    with IANA.>

     x-token := <The two characters "X-" or "x-" followed, with
                 no intervening white space, by any token>

     subtype := extension-token / iana-token

     iana-token := <A publicly-defined extension token. Tokens
                    of this form must be registered with IANA
                    as specified in RFC 2048.>

     parameter := attribute "=" value

     attribute := token
                  ; Matching of attributes
                  ; is ALWAYS case-insensitive.

     value := token / quoted-string

     token := 1*<any (US-ASCII) CHAR except SPACE, CTLs,
                 or tspecials>

     tspecials :=  "(" / ")" / "<" / ">" / "@" /
                   "," / ";" / ":" / "\" / <">
                   "/" / "[" / "]" / "?" / "="
                   ; Must be in quoted-string,
                   ; to use within parameter values

==========

6.1.  Content-Transfer-Encoding Syntax

   The Content-Transfer-Encoding field's value is a single token
   specifying the type of encoding, as enumerated below.  Formally:

     encoding := "Content-Transfer-Encoding" ":" mechanism

     mechanism := "7bit" / "8bit" / "binary" /
                  "quoted-printable" / "base64" /
                  ietf-token / x-token

======================


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


====================

8.  Content-Description Header Field

   The ability to associate some descriptive information with a given
   body is often desirable.  For example, it may be useful to mark an
   "image" body as "a picture of the Space Shuttle Endeavor."  Such text
   may be placed in the Content-Description header field.  This header
   field is always optional.

     description := "Content-Description" ":" *text

   The description is presumed to be given in the US-ASCII character
   set, although the mechanism specified in RFC 2047 may be used for
   non-US-ASCII Content-Description values.


==============




The syntax of a boundary is:

     boundary := 0*69<bchars> bcharsnospace
     bchars := bcharsnospace / " "
     bcharsnospace := DIGIT / ALPHA / "'" / "(" / ")" /
                      "+" / "_" / "," / "-" / "." /
                      "/" / ":" / "=" / "?"

And the body of a multipart entity has the syntax (only the important parts):

     multipart-body := [preamble CRLF]
                       dash-boundary transport-padding CRLF
                       body-part *encapsulation
                       close-delimiter transport-padding
                       [CRLF epilogue]
     dash-boundary := "--" boundary
     encapsulation := delimiter transport-padding
                      CRLF body-part
     delimiter := CRLF dash-boundary
     close-delimiter := delimiter "--"

The preceeding -- is mandatory for every boundary used in the message and the trailing -- is mandatory for the closing boundary (close-delimiter). So a multipart body with three body-parts with boundary as boundary can look like this:

--boundary
1. body-part
--boundary
2. body-part
--boundary
3. body-part
--boundary--



#endif


