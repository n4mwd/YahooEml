
#include "YahooEml.h"


// Un-escape Text String In Place
// Decoding is done in place since escape chars are bigger than the
// characters they represent.  Unrecognized sequences are left alone.

// Conversions:
//   \a 	07 	Alert (Beep, Bell) (added in C89)[1]
//   \b 	08 	Backspace
//   \e 	1B 	Escape character
//   \f 	0C 	Formfeed Page Break
//   \n 	0A 	Newline (Line Feed); see notes below
//   \r 	0D 	Carriage Return
//   \t 	09 	Horizontal Tab
//   \v 	0B 	Vertical Tab
//   \\ 	5C 	Backslash
//   \' 	27 	Apostrophe or single quotation mark
//   \" 	22 	Double quotation mark
//   \? 	3F 	Question mark (used to avoid trigraphs)
//   \nnn 	any 	An octal byte repesented by nnn
//   \xhh 	any 	A hex byte represented by hh
//   \uhhhh 	none 	Unicode code point below 10000 hexadecimal
//   \Uhhhhhhhh 	none 	Unicode code point where h is a hexadecimal digit

// A numeric character reference in HTML refers to a character by its Universal
// Character Set/Unicode code point, and uses the format
//    &#nnnn; (decimal) or   &#xhhhh; (hex)

// XML encodings
//    &amp;  & (ampersand, U+0026)
//    &lt;  < (less-than sign, U+003C)
//    &gt;  > (greater-than sign, U+003E)
//    &quot;  " (quotation mark, U+0022)
//    &apos;  ' (apostrophe, U+0027)

// HTML Encoding
//    &nbsp;  No breakable space








char C_escTable[][2] =
{
    { 'a',  0x07 },     // Alert (Beep, Bell)
    { 'b', 	0x08 },     // Backspace
    { 'e', 	0x1B },     // Escape character
    { 'f', 	0x0C },     // Formfeed Page Break
    { 'n', 	0x0A },     // Newline (Line Feed); see notes below
    { 'r', 	0x0D },     // Carriage Return
    { 't', 	0x09 },     // Horizontal Tab
    { 'v', 	0x0B },     // Vertical Tab
    { '\\', 0x5C },     // Backslash
    { '\'', 0x27 },     // Apostrophe or single quotation mark
    { '"', 	0x22 },     // Double quotation mark
    { '?', 	0x3F },     // Question mark (used to avoid trigraphs)
    { NULL, NULL }
};


char *HtmlEscTable[] =
{
    "&amp;",     // & (ampersand, U+0026)
    "<lt;",      // < (less-than sign, U+003C)
    ">gt;",      // > (greater-than sign, U+003E)
    "\"quot;",   // " (quotation mark, U+0022)
    "'apos;",    // ' (apostrophe, U+0027)
    " nbsp;",    // Non breakable space
    NULL
};

// Un-escape a string in place.
// Return 0 on success else error number.

int UnEscapeString(char *Buf)
{
    int sIdx, dIdx;
    unsigned x;
    char ch, *endptr;


    for (sIdx = dIdx = 0; (ch = Buf[sIdx++]) != 0; )
    {
        if (ch == '\\')   // C style escape sequence
        {
            ch = Buf[sIdx++];   // escape char

            // is it one of the common ones?
            for (x = 0; C_escTable[x][0]; x++)
            {
                if (ch == C_escTable[x][0]) break;
            }

            if (C_escTable[x][0])
            {
                // found one
                Buf[dIdx++] = C_escTable[x][1];
                continue;
            }

            // Not one of the common most ones
            if (ch == 'x')
            {
                x = strtoul(&Buf[sIdx], &endptr, 16);

            }
            else if (ch >= '0' && ch <= '8')   // octal
            {
                // sIdx has already been incremented to the 2nd char
                x = strtoul(&Buf[sIdx - 1], &endptr, 8);

            }
            else   // unsupported escape char - just store as is
            {
                Buf[dIdx++] = '\\';
                Buf[dIdx++] = ch;
                continue;
            }

            // either octal or hex if here
            sIdx = endptr - Buf;   // point to the char that stoppped the scan

            // we can't en-escape a zero so re-escape that if necessary
            x &= 0xFF;
            if (x == 0)
            {
                Buf[dIdx++] = '\\';
                Buf[dIdx++] = '0';
                continue;
            }

            Buf[dIdx++] = (char) x;   // save numeric escape char
            continue;

        }   // if() \\
        else if (ch == '&')   // HTML style sequence
        {
            char tmpstr[16];

            // copy suspected escape sequence into string
            for (x = 0; x < (sizeof(tmpstr) - 1); x++)
            {
                ch = Buf[sIdx++];
                tmpstr[x] = ch;
                if (ch == ';')
                {
                    x++;
                    break;
                }
            }
            tmpstr[x] = 0;   // null terminate

            // check for standard HTML escape sequences
            for (x = 0; HtmlEscTable[x]; x++)
            {
                if (strcmp(tmpstr, HtmlEscTable[x] + 1) == 0) break;
            }

            if (HtmlEscTable[x])
            {
                // found one
                Buf[dIdx++] = HtmlEscTable[x][0];
                continue;
            }
            
            // if here, it wasn't in the table but could still be a numberic
            // sequence  Check for &#nnnn; (decimal) or   &#xhhhh; (hex).

            if (tmpstr[0] == '#')  // yes it is
            {
                if (tmpstr[1] == 'x' || tmpstr[1] == 'X')
                    x = strtoul(&tmpstr[2], NULL, 16);
                else x = strtoul(&tmpstr[1], NULL, 10);

                if (x != 0)   // we don't decode a zero
                {
                    Buf[dIdx++] = (char) (x & 0xFF);
                    continue;
                }
            }

            // We didn't recognize the escape sequence so just leave it as is
            Buf[dIdx++] = '&';
            for (x = 0; tmpstr[x]; x++)
            {
                Buf[dIdx++] = tmpstr[x];
            }

            continue;
        }
        else     // not a recognized escape sequence
        {
            // just copy as is
            Buf[dIdx++] = ch;
        }
    }

    Buf[dIdx] = 0;
    
    return(dIdx);
}







