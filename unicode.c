
#include "yahooeml.h"



// Convert a codepage string into a code page number.
// Sadly, there doesn't seem to be a windows function to do this.

int GetCodePageNumberFromString(char *CpStr)
{
    int i;
    static char *CpTbl[] =
    {
        "us-ascii\0 20127",                 // US-ASCII (7-bit)
        "utf-8\0 65001",                    // Unicode (UTF-8)
        "iso-8859-1\0 28591",               // ISO 8859-1 Latin 1; Western European (ISO)
        "iso-8859-2\0 28592",               // ISO 8859-2 Central European; Central European (ISO)
        "iso-8859-3\0 28593",               // ISO 8859-3 Latin 3
        "iso-8859-4\0 28594",               // ISO 8859-4 Baltic
        "iso-8859-5\0 28595",               // ISO 8859-5 Cyrillic
        "iso-8859-6\0 28596",               // ISO 8859-6 Arabic
        "iso-8859-7\0 28597",               // ISO 8859-7 Greek
        "iso-8859-8\0 28598",               // ISO 8859-8 Hebrew; Hebrew (ISO-Visual)
        "iso-8859-9\0 28599",               // ISO 8859-9 Turkish
        "iso-8859-13\0 28603",              // ISO 8859-13 Estonian
        "iso-8859-15\0 28605",              // ISO 8859-15 Latin 9
        "windows-1250\0 1250",              // ANSI Central European; Central European (Windows)
        "windows-1251\0 1251",              // ANSI Cyrillic; Cyrillic (Windows)
        "windows-1252\0 1252",              // ANSI Latin 1; Western European (Windows)
        "windows-1253\0 1253",              // ANSI Greek; Greek (Windows)
        "windows-1254\0 1254",              // ANSI Turkish; Turkish (Windows)
        "windows-1255\0 1255",              // ANSI Hebrew; Hebrew (Windows)
        "windows-1256\0 1256",              // ANSI Arabic; Arabic (Windows)
        "windows-1257\0 1257",              // ANSI Baltic; Baltic (Windows)
        "windows-1258\0 1258",              // ANSI/OEM Vietnamese; Vietnamese (Windows)
        "IBM037\0 37",                      // IBM EBCDIC US-Canada
        "IBM437\0 437",                     // OEM United States
        "IBM500\0 500",                     // IBM EBCDIC International
        "ASMO-708\0 708",                   // Arabic (ASMO 708)
        "DOS-720\0 720",                    // Arabic (Transparent ASMO); Arabic (DOS)
        "ibm737\0 737",                     // OEM Greek (formerly 437G); Greek (DOS)
        "ibm775\0 775",                     // OEM Baltic; Baltic (DOS)
        "ibm850\0 850",                     // OEM Multilingual Latin 1; Western European (DOS)
        "ibm852\0 852",                     // OEM Latin 2; Central European (DOS)
        "IBM855\0 855",                     // OEM Cyrillic (primarily Russian)
        "ibm857\0 857",                     // OEM Turkish; Turkish (DOS)
        "IBM00858\0 858",                   // OEM Multilingual Latin 1 + Euro symbol
        "IBM860\0 860",                     // OEM Portuguese; Portuguese (DOS)
        "ibm861\0 881",                     // OEM Icelandic; Icelandic (DOS)
        "DOS-862\0 862",                    // OEM Hebrew; Hebrew (DOS)
        "IBM863\0 863",                     // OEM French Canadian; French Canadian (DOS)
        "IBM864\0 864",                     // OEM Arabic; Arabic (864)
        "IBM865\0 865",                     // OEM Nordic; Nordic (DOS)
        "cp866\0 866",                      // OEM Russian; Cyrillic (DOS)
        "ibm869\0 869",                     // OEM Modern Greek; Greek, Modern (DOS)
        "IBM870\0 870",                     // IBM EBCDIC Multilingual/ROECE (Latin 2);
        "windows-874\0 874",                // ANSI/OEM Thai (ISO 8859-11); Thai (Windows)
        "cp875\0 875",                      // IBM EBCDIC Greek Modern
        "shift_jis\0 932",                  // ANSI/OEM Japanese; Japanese (Shift-JIS)
        "gb2312\0 936",                     // ANSI/OEM Simplified Chinese (PRC, Singapore); (GB2312)
        "ks_c_5601-1987\0 949",             // ANSI/OEM Korean (Unified Hangul Code)
        "big5\0 950",                       // ANSI/OEM Traditional Chinese (Big5)
        "IBM1026\0 1026",                   // IBM EBCDIC Turkish (Latin 5)
        "IBM01047\0 1047",                  // IBM EBCDIC Latin 1/Open System
        "IBM01140\0 1140",                  // IBM EBCDIC US-Canada (037 + Euro symbol);
        "IBM01141\0 1141",                  // IBM EBCDIC Germany (20273 + Euro symbol);
        "IBM01142\0 1142",                  // IBM EBCDIC Denmark-Norway (20277 + Euro symbol);
        "IBM01143\0 1143",                  // IBM EBCDIC Finland-Sweden (20278 + Euro symbol);
        "IBM01144\0 1144",                  // IBM EBCDIC Italy (20280 + Euro symbol);
        "IBM01145\0 1145",                  // IBM EBCDIC Latin America-Spain (20284 + Euro symbol);
        "IBM01146\0 1146",                  // IBM EBCDIC United Kingdom (20285 + Euro symbol);
        "IBM01147\0 1147",                  // IBM EBCDIC France (20297 + Euro symbol);
        "IBM01148\0 1148",                  // IBM EBCDIC International (500 + Euro symbol);
        "IBM01149\0 1149",                  // IBM EBCDIC Icelandic (20871 + Euro symbol);
        "utf-16\0 1200",                    // Unicode UTF-16, little endian (BMP of ISO 10646);
        "unicodeFFFE\0 1201",               // Unicode UTF-16, big endian
        "Johab\0 1361",                     // Korean (Johab)
        "macintosh\0 10000",                // MAC Roman; Western European (Mac)
        "x-mac-japanese\0 10001",           // Japanese (Mac)
        "x-mac-chinesetrad\0 10002",        // MAC Traditional Chinese (Big5); (Mac)
        "x-mac-korean\0 10003",             // Korean (Mac)
        "x-mac-arabic\0 10004",             // Arabic (Mac)
        "x-mac-hebrew\0 10005",             // Hebrew (Mac)
        "x-mac-greek\0 10006",              // Greek (Mac)
        "x-mac-cyrillic\0 10007",           // Cyrillic (Mac)
        "x-mac-chinesesimp\0 10008",        // MAC Simplified Chinese (GB 2312); (Mac)
        "x-mac-romanian\0 10010",           // Romanian (Mac)
        "x-mac-ukrainian\0 10017",          // Ukrainian (Mac)
        "x-mac-thai\0 10021",               // Thai (Mac)
        "x-mac-ce\0 10029",                 // MAC Latin 2; Central European (Mac)
        "x-mac-icelandic\0 10079",          // Icelandic (Mac)
        "x-mac-turkish\0 10081",            // Turkish (Mac)
        "x-mac-croatian\0 10082",           // Croatian (Mac)
        "utf-32\0 12000",                   // Unicode UTF-32, little endian byte order;
        "utf-32BE\0 12001",                 // Unicode UTF-32, big endian byte order;
        "x-Chinese_CNS\0 20000",            // CNS Taiwan; Chinese Traditional (CNS)
        "x-cp20001\0 20001",                // TCA Taiwan
        "x_Chinese-Eten\0 20002",           // Eten Taiwan; Chinese Traditional (Eten)
        "x-cp20003\0 20003",                // IBM5550 Taiwan
        "x-cp20004\0 20004",                // TeleText Taiwan
        "x-cp20005\0 20005",                // Wang Taiwan
        "x-IA5\0 20105",                    // IA5 (IRV International Alphabet No. 5, 7-bit);
        "x-IA5-German\0 20106",             // IA5 German (7-bit)
        "x-IA5-Swedish\0 20107",            // IA5 Swedish (7-bit)
        "x-IA5-Norwegian\0 20108",          // IA5 Norwegian (7-bit)
        "x-cp20261\0 20261",                // T.61
        "x-cp20269\0 20269",                // ISO 6937 Non-Spacing Accent
        "IBM273\0 20273",                   // IBM EBCDIC Germany
        "IBM277\0 20277",                   // IBM EBCDIC Denmark-Norway
        "IBM278\0 20278",                   // IBM EBCDIC Finland-Sweden
        "IBM280\0 20280",                   // IBM EBCDIC Italy
        "IBM284\0 20284",                   // IBM EBCDIC Latin America-Spain
        "IBM285\0 20285",                   // IBM EBCDIC United Kingdom
        "IBM290\0 20290",                   // IBM EBCDIC Japanese Katakana Extended
        "IBM297\0 20297",                   // IBM EBCDIC France
        "IBM420\0 20420",                   // IBM EBCDIC Arabic
        "IBM423\0 20423",                   // IBM EBCDIC Greek
        "IBM424\0 20424",                   // IBM EBCDIC Hebrew
        "x-EBCDIC-KoreanExtended\0 20833",  // IBM EBCDIC Korean Extended
        "IBM-Thai\0 20838",                 // IBM EBCDIC Thai
        "koi8-r\0 20866",                   // Russian (KOI8-R); Cyrillic (KOI8-R)
        "IBM871\0 20871",                   // IBM EBCDIC Icelandic
        "IBM880\0 20880",                   // IBM EBCDIC Cyrillic Russian
        "IBM905\0 20905",                   // IBM EBCDIC Turkish
        "IBM00924\0 20924",                 // IBM EBCDIC Latin 1/Open System (1047 + Euro symbol)
        "EUC-JP\0 20932",                   // Japanese (JIS 0208-1990 and 0212-1990)
        "x-cp20936\0 20936",                // Simplified Chinese ((GB2312-80)
        "x-cp20949\0 20949",                // Korean Wansung
        "cp1025\0 21025",                   // IBM EBCDIC Cyrillic Serbian-Bulgarian
        "koi8-u\0 21866",                   // Ukrainian (KOI8-U); Cyrillic (KOI8-U)
        "x-Europa\0 29001",                 // Europa 3
        "iso-8859-8-i\0 38598",             // ISO 8859-8 Hebrew; Hebrew (ISO-Logical)
        "iso-2022-jp\0 50220",              // ISO 2022 Japanese with no halfwidth Katakana;
        "csISO2022JP\0 50221",              // ISO 2022 Japanese with halfwidth Katakana; (
        "iso-2022-jp\0 50222",              // ISO 2022 Japanese JIS X 0201-1989; 
        "iso-2022-kr\0 50225",              // ISO 2022 Korean
        "x-cp50227\0 50227",                // ISO 2022 Simplified Chinese; C
        "euc-jp\0 51932",                   // EUC Japanese
        "EUC-CN\0 51936",                   // EUC Simplified Chinese;
        "euc-kr\0 51949",                   // EUC Korean
        "hz-gb-2312\0 52936",               // HZ-GB2312 Simplified Chinese;
        "GB18030\0 54936",                  // Windows XP and later: GB18030 Simplified Chinese (4 byte);
        "x-iscii-de\0 57002",               // ISCII Devanagari
        "x-iscii-be\0 57003",               // ISCII Bangla
        "x-iscii-ta\0 57004",               // ISCII Tamil
        "x-iscii-te\0 57005",               // ISCII Telugu
        "x-iscii-as\0 57006",               // ISCII Assamese
        "x-iscii-or\0 57007",               // ISCII Odia
        "x-iscii-ka\0 57008",               // ISCII Kannada
        "x-iscii-ma\0 57009",               // ISCII Malayalam
        "x-iscii-gu\0 57010",               // ISCII Gujarati
        "x-iscii-pa\0 57011",               // ISCII Punjabi
        "utf-7\0 65000",                    // Unicode (UTF-7)
        NULL
    };

    for (i = 0; CpTbl[i]; i++)
    {
        if (strcmpi(CpStr, CpTbl[i]) == 0)
        {
            // found A match
            return(atoi(CpTbl[i] + strlen(CpTbl[i]) + 1));
        }
    }



    return(20127);  // US-ASCII default

}


// UTF-8 example"  "S√¥n b√¥n de magn√  el v√©der, el me fa minga mal."
// C_1252 example: "SÙn bÙn de magn‡ el vÈder, el me fa minga mal."

// Convert from UTF-8 (or other code page) to Windows-1252
// Return in the same buffer so make sure it is big enough.


int CpToCp1252(char *CodePageStr, char *buf)
{
    int size, CodePage;
    LPWSTR wBuf;

    CodePage = GetCodePageNumberFromString(CodePageStr);

    // This function works by first converting to unicode and then back to CP_1252

    // First convert charset to unicode
    size = MultiByteToWideChar(CodePage, 0, buf, -1, 0, 0);   // get num Wchars
    wBuf = calloc(size, sizeof(WCHAR));
    if (!wBuf) return(1);
    MultiByteToWideChar(CodePage, 0, buf, -1, wBuf, size);

    // convert unicode back to CP_1252
    WideCharToMultiByte(CP_1252, 0, wBuf, -1, buf, size, 0, 0);

    free(wBuf);
    return(0);
}


// Convert a binary buffer to Base64 and output it as text to the file.
// Line breaks every 19 octets (76 chars) and at end.  Padding if necessary.
// fp = open text file, Buf = binary input buffer, Len = Length of Binary buffer.

void Base64Print(FILE *fpOut64, FILE *fpInBin)
{
    int j, pads, CCnt, Len;
    unsigned work;
    unsigned char OutBuf[5], Buf[16];
    static const char Base64Tbl[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    CCnt = 0;
    memset(Buf, 0, 4);
    while ((Len = fread(Buf, 1, 3, fpInBin)) != 0)
    {
        // Load 3 bytes into working variable
        work = 0;
        for (j = 0; j < Len; j++)
            work = (work << 8) | Buf[j];
        pads = 3 - j;
        work <<= 8 * pads;

        // copy octets to buffer
        memset(OutBuf, 0, sizeof(OutBuf));
        for (j = 3; j >= 0; j--)
        {
            OutBuf[j] = (char)((pads-- > 0) ? '=' : Base64Tbl[work & 0x3F]);
            work >>= 6;
        }

        // print result
        fprintf(fpOut64, "%s", OutBuf);
        if (++CCnt == 19)
        {
            fprintf(fpOut64, "\r\n");
            CCnt = 0;
        }

        memset(Buf, 0, 4);
    }


}


// Perform an in-place conversion of Base64 text to binary.
// Converts Base64 text pointed to by Buf until an illegal character is found.
// Stores Binary output in Buf.  Returns number of bytes written.

int Base64ToBinary(char *Buf)
{
    int j, done = 0, wc = 0, val64, ByteCount = 0;
    char ch, *OutPtr = Buf;
    unsigned work = 0;

    while (!done)
    {
        ch = *Buf++;
        if (isspace(ch)) continue;   // ignore spaces, tabs, newlines

        if (ch == '=') val64 = 0;   // padding
        else if (isupper(ch)) val64 = ((ch - 'A') & 0x3F);
        else if (islower(ch)) val64 = ((ch - 'a' + 26) & 0x3F);
        else if (isdigit(ch)) val64 = ((ch - '0' + 52) & 0x3F);
        else if (ch == '+') val64 = 62;
        else if (ch == '/') val64 = 63;
        else   // illegal char stops the conversion
        {
            if (wc != 0)    // some nut didn't write all his pad chars
            {
            	while (wc++ != 4)
                	work <<= 6;    // shift until we have 4 characters
                done = 1;
                goto WriteBytes;
            }
            break;
        }

        wc++;
        work = (work << 6) | val64;   // shift in the 6 bit value

        if (wc == 4)   // we have a full triple
        {
            // write our three bytes
WriteBytes:
            wc = 0;
            for (j = 2; j >= 0; j--)
            {
                OutPtr[j] = (char)(work & 0xFF);
                work >>= 8;
                ByteCount++;
            }
            OutPtr += 3;
        }
    }

    return(ByteCount);
}


// Hex2Val -- return value of a hexadecimal digit (0-9,A-F)
// Note that hex digit must be valid and uppercase or else it returns 0xFF.

unsigned char Hex2Val(unsigned char ch)
{
    if (!isxdigit(ch) || islower(ch)) return(-1);
    return((unsigned char)(((ch >= '0' && ch <= '9') ? ch - '0' : 10 + ch - 'A') & 0x0F));
}


// Perform an in-place conversion of quoted printable text to binary.
// Converts null terminated QP text string pointed to by Buf.
// Stores Binary output in Buf.  Returns number of bytes written.
// Note that it is possible for a \0 to be encoded and returned here.

int QPToBinary(char *Buf)
{
    int NumWritten = 0;
    char *OutBuf;
    unsigned char ch, hex1, hex2, x1, x2;

    if (!Buf) return(0);

    OutBuf = Buf;

    while ((ch = *Buf++) != 0)
    {
//if (ch == '<' && *Buf == 'a')
//printf("");

        if (ch != '=')   // just copy the char
        {
            *OutBuf++ = ch;
            NumWritten++;
        }
        else   // decode an equals sign
        {
            hex1 = *Buf++;
            hex2 = *Buf++;
            x1 = Hex2Val(hex1);
            x2 = Hex2Val(hex2);

            if ((hex1 == '\r' || hex1 == ' ') && hex2 == '\n') continue;  // soft line break - ignore
            if (hex1 == '\n') Buf--;    // half soft line break
            else if (x1 == 0xFF || x2 == 0xFF)   // non-compliant
            {
                // invalid escapement - just copy it as is
                // note that there is no official way to handle this
                *OutBuf++ = '=';
                *OutBuf++ = hex1;
                *OutBuf++ = hex2;
                NumWritten += 3;
            }
            else // convert hex1 and hex2 to a single byte
            {
                ch = (unsigned char)((x1 << 4) | x2);
                *OutBuf++ = ch;
                NumWritten++;
            }

        }
    }

    return(NumWritten);
}


// case insensitive strstr()

char *stristr(const char *haystack, const char *needle)
{
    int i;
    int c = tolower((unsigned char)*needle);

    if (c == '\0')
        return (char *)haystack;

    for (; *haystack; haystack++)
    {
        if (tolower((unsigned char)*haystack) == c)
        {
            for (i = 0;;)
            {
                if (needle[++i] == '\0')
                    return (char *)haystack;

                if (tolower((unsigned char)haystack[i]) != tolower((unsigned char)needle[i]))
                    break;
            }
        }
    }

    return NULL;
}







