
#include <windows.h>
#include "yahooeml.h"

// UTF-8 example"  "S√¥n b√¥n de magn√  el v√©der, el me fa minga mal."
// C_1252 example: "SÙn bÙn de magn‡ el vÈder, el me fa minga mal."

// Convert from UTF-8 (or other code page) to Windows-1252
// Return in the same buffer so make sure it is big enough.

int CpToCp1252(int CodePage, char *buf)
{
    int size;
    LPWSTR wBuf;

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

void Base64Print(FILE *fp, char *Buf, int Len)
{
    int i, j, pads, CCnt;
    unsigned work;
    char OutBuf[5];
    static const char Base64Tbl[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    CCnt = 0;
    OutBuf[4] = 0;
    for (i = 0; i < Len; i += 3)
    {
        // Load 3 bytes into working variable
        work = 0;
        for (j = 0; j < 3 && i + j < Len; j++)
            work = (work << 8) | *Buf++;
        pads = 3 - j;
        work <<= 8 * pads;

        // copy octets to buffer
        for (j = 3; j >= 0; j--)
        {
            OutBuf[j] = (char)((pads-- > 0) ? '=' : Base64Tbl[work & 0x3F]);
            work >>= 6;
        }

        // print result
        fprintf(fp, "%s", OutBuf);
        if (++CCnt == 19)
        {
            printf("\r\n");
            CCnt = 0;
        }
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

#if 0

// Test function for base64 functions
void TestFunc64(void)
{
	char *Buf64 = "QmFzZTY0IGVuY29kaW5nIHNjaGVtZXMgYXJlIGNvbW1vbmx5IHVzZWQgd2hlbiB0aGVyZSBpcyBh"
	"IG5lZWQgdG8gZW5jb2RlIGJpbmFyeSBkYXRhIHRoYXQgbmVlZHMgYmUgc3RvcmVkIGFuZCB0cmFu"
	"c2ZlcnJlZCBvdmVyIG1lZGlhIHRoYXQgYXJlIGRlc2lnbmVkIHRvIGRlYWwgd2l0aCB0ZXh0dWFs"
	"IGRhdGEuIFRoaXMgaXMgdG8gZW5zdXJlIHRoYXQgdGhlIGRhdGEgcmVtYWlucyBpbnRhY3Qgd2l0"
	"aG91dCBtb2RpZmljYXRpb24gZHVyaW5nIHRyYW5zcG9ydC4gQmFzZTY0IGlzIHVzZWQgY29tbW9u"
	"bHkgaW4gYSBudW1iZXIgb2YgYXBwbGljYXRpb25zIGluY2x1ZGluZyBlbWFpbCB2aWEgTUlNRSwg"
	"ICBhbmQgc3RvcmluZyBjb21wbGV4IGRhdGEgaW4gWE1MLg==";

	char *ibuf = "Base64 encoding schemes are commonly used when there is a need"
	" to encode binary data that needs be stored and transferred over media that"
	" are designed to deal with textual data. This is to ensure that the data"
	" remains intact without modification during transport. Base64 is used"
	" commonly in a number of applications including email via MIME,   and"
	" storing complex data in XML.";

    Base64Print(stdout, ibuf, strlen(ibuf));
	printf("\n%d bytes written.\n", Base64ToBinary(Buf64));
	printf("%s\n", Buf64);
}
#endif



