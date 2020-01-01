
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
    char *sptr, *dptr;
    unsigned x;
    char ch, *endptr;

    if (!Buf) return(0);
    
    for (sptr = dptr = Buf; (ch = *sptr++) != 0; )
    {
        if (ch == '\\')   // C style escape sequence
        {
            ch = *sptr++;   // get actual escape char

            // is it one of the common ones?
            for (x = 0; C_escTable[x][0]; x++)
            {
                if (ch == C_escTable[x][0]) break;
            }

            if (C_escTable[x][0])
            {
                // found one
                *dptr++ = C_escTable[x][1];
                continue;
            }

            // Not one of the common most ones
            endptr = NULL;
            if (ch == 'x')
            {
                x = strtoul(sptr, &endptr, 16);
            }
            else if (ch >= '0' && ch <= '7')   // octal
            {
                // sIdx has already been incremented to the 2nd char
                x = strtoul(sptr - 1, &endptr, 8);
            }

            // we can't un-escape a zero so re-escape that if necessary
            x &= 0xFF;

            if (x && endptr)
            {
	            // either octal or hex if here
    	        sptr = endptr;    // point to the char that stoppped the scan
	            *dptr++ = (char) x;   // save numeric escape char
            }
            else     // unsupported escape char - just store as is
            {
                *dptr++ = '\\';
                *dptr++ = ch;
            }
   	        continue;

        }   // if() \\
        else if (ch == '&')   // HTML style sequence
        {
            // First, Check for &#nnnn; (decimal) or   &#xhhhh; (hex).
            if (*sptr == '#')  // yes it is
            {
                ch = *(sptr + 1);  // get 1st char after '#'
                if (ch == 'x' || ch == 'X')   // hex version
                    x = strtoul(sptr + 2, &endptr, 16);
                else x = strtoul(sptr + 1, &endptr, 10);

                if (*endptr == ';' && x != 0)   // we don't decode a zero
                {
                    *dptr++ = (char) (x & 0xFF);
                    sptr = endptr + 1;
                    continue;
                }
            }

            // If here, we could not decode a '#' escape code
            // check for standard HTML escape sequences

            for (x = 0; HtmlEscTable[x]; x++)
            {
                if (memicmp(sptr, HtmlEscTable[x] + 1, strlen(HtmlEscTable[x] + 1)) == 0) break;
            }

            if (HtmlEscTable[x])
            {
                // found one
                *dptr++ = HtmlEscTable[x][0];
                sptr += strlen(HtmlEscTable[x] + 1);
                continue;
            }

            // We didn't recognize the escape sequence so just leave it as is
            *dptr++ = '&';
            continue;
        }
        else     // not a recognized escape sequence
        {
            // just copy as is
            *dptr++ = ch;
        }
    }

    *dptr = 0;   // null terminate

    return(strlen(Buf));
}







