
#include "YahooEml.h"


#define MAXTAGLEN   31   // max length of a tag to store
#define MAXATTRS    20   // max number of attribute pairs

static char G_CurTagStr[MAXTAGLEN + 1];   // global area to store current tag
static char G_CodeBreaker[MAXTAGLEN + 2]; // tag to end preformatted text
//static char *G_CurAttrTbl[MAXATTRS][2];   // Table of pointers to attributes and values
static char G_EmailBuf[1024];             // Temp storage for href attribute as defined by A tag.
static char G_StyleBuf[1024];             // temp storage for style attribute
//static char G_FontFace[MAXTAGLEN + 1];    // Font face as defined by FONT tag.
static char G_FontColor[MAXTAGLEN + 1];   // Font color as defined by FONT tag.
static char G_FontSize[MAXTAGLEN + 1];    // Font size as defined by FONT tag.
static char G_ImgSrc[256];                // Url to image
static int  G_ImgWidth;                   // Image width as defined by IMG tag.
static int  G_ImgHeight;                  // Image height as defined by IMG tag.
static char G_InPre;                      // Boolean if in preformatted text

// BBCode  enums
// These are the BBcodes that we support and match the string table.
// NOTE: <set> tags and odd numbers, </unset> tags are even numbers,

enum {  BB_BOLD=1,      BB_END_BOLD,
		BB_ITALIC,      BB_END_ITALIC,
        BB_UNDER,       BB_END_UNDER,
        BB_STRIKE,      BB_END_STRIKE,
        BB_SUP,         BB_END_SUP,
        BB_SUB,         BB_END_SUB,
        BB_LEFT,        BB_END_LEFT,
        BB_CENTER,      BB_END_CENTER,
        BB_RIGHT,       BB_END_RIGHT,
        BB_QUOTE,       BB_END_QUOTE,
        BB_LIST,        BB_END_LIST,
        BB_LIST2,       BB_END_LIST2,
        BB_LI,          BB_END_LI,
        BB_MOVE,        BB_END_MOVE,
        BB_TABLE,       BB_END_TABLE,
        BB_TR,          BB_END_TR,
        BB_TD,          BB_END_TD,
        BB_TH,          BB_END_TH,
        BB_NEWLINE,     BB_END_NEWLINE,
        BB_NEWLINE2,    BB_END_NEWLINE2,
        BB_HR,          BB_MAXSIMPLE,
        BB_CODE,        BB_END_CODE,
        BB_SPECIAL,     BB_END_SPECIAL,
        BB_SPECIAL_A,   BB_END_SPECIAL_A,
        BB_SPECIAL_IMG, BB_END_SPECIAL_IMG,
        BB_SPECIAL_FONT, BB_END_SPECIAL_FONT,
        BB_URL,         BB_END_URL,
        BB_EMAIL,       BB_END_EMAIL,
        BB_IMG,         BB_END_IMG,
        BB_IMG2,        BB_END_IMG2,
        BB_SIZE,        BB_END_SIZE,
        BB_COLOR,       BB_END_COLOR,
        BB_MAXCODES };

// BBCode table
static char *BBCodes[] =
{
    "",
	"[b]",          "[/b]",
    "[i]",          "[/i]",
    "[u]",          "[/u]",
    "[s]",          "[/s]",
    "[sup]",        "[/sup]",
    "[sub]",        "[/sub]",
    "[left]",       "[/left]",
    "[center]",     "[/center]",
    "[right]",      "[/right]",
    "[quote]",      "[/quote]",
    "[list]",       "[/list]",
    "[list=1]",     "[/list]",
    "[li]",         "[/li]",
    "[move]",       "[/move]",
    "[table]",      "[/table]",
    "[tr]",         "[/tr]",
    "[td]",         "[/td]",
    "[th]",         "[/th]",
    "\r\n",         "",
    "\r\n\r\n",     "",
    "\r\n---------------\r\n", "endsimple",
    "[code]",       "[/code]",
    "",             "",         // special and end special
    "",             "",         // special A and end special A
    "",             "",         // Special img and end special img
    "",             "",         // Special font and end special font
    "[url=",        "[/url]",
    "[email=",      "[/email]",
    "[img]",        "[/img]",
    "[img=",        "[/img]",
    "[size=",       "[/size]",
    "[color=",      "[/color]",
    NULL
};

// BBCode table
static char *HtmlPrint[] =
{
    "",
	"<b>",          "</b>",
    "<i>",          "</i>",
    "<u>",          "</u>",
    "<s>",          "</s>",
    "<sup>",        "</sup>",
    "<sub>",        "</sub>",
    "<left>",       "</left>",
    "<center>",     "</center>",
    "<right>",      "</right>",
    "<blockquote>", "</blockquote>",
    "<list>",       "</list>",
    "<list=1>",     "</list>",
    "<li>",         "</li>",
    "<marquee>",    "</marquee>",
    "<table>",      "</table>",
    "<tr>",         "</tr>",
    "<td>",         "</td>",
    "<th>",         "</th>",
    "<br>",         "</br>",
    "<p>",          "</p>",
    "<hr>",         "endsimple",
    "<pre>",        "</pre>",
    "",             "",         // special and end special
    "",             "",         // special A and end special A
    "",             "",         // Special img and end special img
    "",             "",         // Special font and end special font
    "<a href=\"%s\">", "</a>",  // link
    "<a href=\"mailto:=", "</a>", // email
    "<img src=\"%s\">", "",
    "<img width=%d height=%d src=\"%s\">", "",
    "<span style=font size", "</span>",
    "<font color=", "</font>",
    NULL
};

// end tags that break (stop) attributes - can be any tag
enum { ST_BOLD=0, ST_ITALIC, ST_UNDER, ST_STRIKE,
       ST_SIZE, ST_COLOR, /* ST_FACE, */
       ST_LEFT, ST_CENTER, ST_RIGHT, ST_MAXSTYLE };

// This array records which tag was used to start the font style.
// since ANY tag, not just the ones supported here, can have a Style
// attribute, we must store the actual tag and not its reference.

static char G_BrkAttrs[ST_MAXSTYLE][MAXTAGLEN + 2];       // ends the current font attribute

// Codes output when a BrkAttr[] is matched.
static int G_BrkCodes[ST_MAXSTYLE] =
{
    BB_END_BOLD, BB_END_ITALIC, BB_END_UNDER, BB_END_STRIKE,
    BB_END_SIZE, BB_END_COLOR, /* BB_END_FONT, */
    BB_END_LEFT, BB_END_CENTER, BB_END_RIGHT
};



typedef struct HTMLREC
{
    char *html;
    int  bbidx;
} HTMLREC;

// This table is the simple one to one html codes.
static HTMLREC HtmlCodes[] =
{
    { "",			0 },			// unused code

    // These are simple codes with no parameters
    { "HR", 		BB_HR },
    { "H1", 		BB_BOLD },
    { "/H1", 		BB_END_BOLD },
    { "H2", 		BB_BOLD },
    { "/H2", 		BB_END_BOLD },
    { "H3", 		BB_BOLD },
    { "/H3", 		BB_END_BOLD },
    { "H4", 		BB_BOLD },
    { "/H4", 		BB_END_BOLD },
    { "H5", 		BB_BOLD },
    { "/H5", 		BB_END_BOLD },
    { "H6", 		BB_BOLD },
    { "/H6", 		BB_END_BOLD },
    { "BLOCKQUOTE", BB_QUOTE },
    { "/BLOCKQUOTE",BB_END_QUOTE },
    { "UL", 		BB_LIST },
    { "/UL", 		BB_END_LIST },
    { "OL", 		BB_LIST2 },
    { "/OL", 		BB_END_LIST },
    { "LI", 		BB_LI },
    { "/LI", 		BB_END_LI },
    { "B", 			BB_BOLD },
    { "/B", 		BB_END_BOLD },
    { "BIG", 		BB_BOLD },
    { "/B", 		BB_END_BOLD },
    { "U", 			BB_UNDER },
    { "/U", 		BB_END_UNDER },
    { "INS", 		BB_UNDER },
    { "/INS", 		BB_END_UNDER },
    { "I", 			BB_ITALIC },
    { "/I", 	   	BB_END_ITALIC },
    { "EM", 		BB_ITALIC },
    { "/EM", 	   	BB_END_ITALIC },
    { "STRONG", 	BB_BOLD },
    { "/STRONG",	BB_END_BOLD },
    { "LEFT", 		BB_LEFT },
    { "/LEFT", 		BB_END_LEFT },
    { "CENTER", 	BB_CENTER },
    { "/CENTER", 	BB_END_CENTER },
    { "RIGHT", 		BB_RIGHT },
    { "/RIGHT",		BB_END_RIGHT },
    { "STRIKE", 	BB_STRIKE },
    { "/STRIKE", 	BB_END_STRIKE },
    { "S",       	BB_STRIKE },
    { "/S", 		BB_END_STRIKE },
    { "DEL", 		BB_STRIKE },
    { "/DEL", 		BB_END_STRIKE },
    { "MARQUEE", 	BB_MOVE },
    { "/MARQUEE", 	BB_END_MOVE },
    { "TABLE", 		BB_TABLE },
    { "/TABLE",		BB_END_TABLE },
    { "TR", 		BB_TR },
    { "/TR", 		BB_END_TR },
    { "TD", 		BB_TD },
    { "/TD", 		BB_END_TD },
    { "TH", 		BB_TH },
    { "/TH", 		BB_END_TH },
    { "SUP", 		BB_SUP },
    { "/SUP", 		BB_END_SUP },
    { "SUB", 		BB_SUB },
    { "/SUB", 		BB_END_SUB },

    // These tags have no end tags
    { "BR", 		BB_NEWLINE },
    { "P", 			BB_NEWLINE2 },

    // Preformatted text areas - content between markers is not modified.
    { "TEXTAREA", 	BB_CODE },      // text block
    { "/TEXTAREA", 	BB_END_CODE },  // end text block
    { "PRE", 		BB_CODE },      // preformatted
    { "/PRE", 		BB_END_CODE },  // end preformatted
    { "TT", 		BB_CODE },      // teletype
    { "/TT", 		BB_END_CODE },  // end teletype
    { "CODE", 		BB_CODE },      // code
    { "/CODE", 		BB_END_CODE },  // end code

    // These tags require special processing.
    { "A",          BB_SPECIAL_A },
    { "/A",         BB_END_SPECIAL_A },
    { "IMG",   		BB_SPECIAL_IMG },
    { "/IMG",  		BB_END_IMG },
    { "FONT",		BB_SPECIAL_FONT },
    { "/FONT",		BB_END_SPECIAL_FONT },

    { NULL, 		NULL }
};





// Strips yahoo CSS junk from email
// Look for "#yiv" followed by at least one digit.  If found, then look
// for a "{" then a "}".  If that sequence is found, delete it.

void RemoveYinv(char *Buf)
{
    char ch, *OutBuf;
    char *lptr;
    int level;

    if (!Buf) return;

    OutBuf = Buf;

    while ((ch = *Buf++) != 0)
    {
        if (ch == '#')    // possible match
        {
            if (memcmp(Buf, "yiv", 3) == 0 && isdigit(Buf[3]))
            {
                Buf += 4;
                while (!isspace(*Buf)) Buf++;   // skip over "#yiv234234534"

                // scan for opening bracket
                lptr = strchr(Buf + 4, '{');
                if (lptr)
                {
                	// scan to closing bracket
	                Buf = lptr + 1;
	                level = 1;
	                while ((ch = *Buf++) != 0)
	                {
	                    if (ch == '{')
	                        level++;
	                    else if (ch == '}')
	                        level--;
	                    if (level == 0) break;
	                }
                }

                // remove trailing white space

                while (isspace(*Buf)) Buf++;
            }
            else *OutBuf++ = ch;

        }
        else   // regular chars
        {
            *OutBuf++ = ch;
        }


    }

    *OutBuf = 0;    // null terminate
}


/*

TAG CONVERSIONS EQUAL OR SHORTER LENGTH
HTML Tag 						Converts to:
<BR> or <BR />					Carriage Return
<H1> to <H6>  </H1> to </H6>	[b][/b]
<BLOCKQUOTE></BLOCKQUOTE>		[quote][/quote]  [quote="author"]quoted text[/quote]
<LI>							[*]
<IMG SRC="pic.jpg">				[img]pic.jpg[/img]
<A HREF="http://webpage.com/">Web Page</A>		[url=http://webpage.com]Web Page[/url]
<A HREF="mailto:me@address.com">Email me</A>	[email=me@addres.com]Email me[/email]
<BIG></BIG>						[b][/b]
<B></B>							[b][/b]
<U></U>	or <ins></ins>			[u][/u]
<I></I> or <EM></EM>			[i][/i]
<FONT FACE="arial"></FONT>		[font=arial][/font]
<FONT COLOR="red"></FONT>		[color=red][/color]
<FONT SIZE="5"></FONT>			[size=5][/size]
<STRONG></STRONG>				[b][/b]
<TEXTAREA></TEXTAREA> 			[code][/code] (Note: All code within textarea is not converted.)
<SCRIPT></SCRIPT> 				(ignored)
<LEFT></LEFT>					[left][/left]
<CENTER></CENTER>				[center][/center]
<RIGHT></RIGHT>					[right][/right]
<STRIKE></STRIKE>               [s][/s]
<del></del>                     [s][/s]
<MARQUEE></MARQUEE>     		[move][/move]
<HR>                    		[hr]
<TABLE></TABLE>      			[table][/table]
<TR></TR>	         			[tr][/tr]
<TD></TD>	         			[td][/td]
<TH></TH>	         			[th][/th]
&nbsp;	             			(Space)
<SUP></SUP>             		[sup][/sup]
<SUB></SUB>             		[sub][/sub]

TAG CONVERSIONS RESULTING IN GREATER LENGTH
HTML Tag 						Converts to:
<P>								2 Carriage Returns  \r\n\r\n  4 chars
<UL></UL>						[list][/list]
<OL></OL>						[list=1][/list]
<FONT FACE="arial" COLOR="red" SIZE="5"></FONT>		[font=arial][color=red][size=5][/font][/color][/size]
<PRE></PRE>						[code][/code]
<TT></TT>               		[code][/code]

*/




// Check the tag in G_CurTagStr to see if it was
// used to close a style or font attribute
// Writes encoded bytes to OutBuf.  Clears G_BrkAttrs[] if used.
// Returns the number of chars written to OutBuf.

int CloseStyle(char *OutBuf)
{
    int i, cnt = 0;

    if (G_CurTagStr[0] == '/')
    {
        for (i = 0; i < ST_MAXSTYLE; i++)
	    {
			if (strcmpi(G_CurTagStr + 1, G_BrkAttrs[i]) == 0)
	        {
	            G_BrkAttrs[i][0] = 0;
	            *OutBuf++ = 0xFF;
	            *OutBuf++ = (char) G_BrkCodes[i];
	            cnt += 2;
	        }
    	}
    }

    return(cnt);
}



/*
// styles - can appear in any html tag including <HTML>
<span style="font-weight: bold;">bolded text</span>                      [b][/b]
<span style="font-style: italic;">italicized text</span>                 [i][/i]
<span style="text-decoration: underline;">underlined text</span>         [u][/u]
<span style="text-decoration: line-through;">strikethrough text</span>   [s][/s]
<span style="font-size:30">Large Text</span>         [size=30]Large Text[/size]
<span style="color:fuchsia;">Text in fuchsia</span>  [color=fuchsia]Text in fuchsia[/color]
<span style="color:#FF00FF;">Text in fuchsia</span>  [color=#FF00FF]Text in fuchsia[/color]
<p style="font-family:courier;">This is a paragraph.</p>
*/



// HTML Style codes
// A "style" code can be in ANY HTML tag so ALL tags need to be seached.
// General format: <tag style="param:value *[; param2:value][;] "> some text </tag>


static HTMLREC HtmlStyles[ST_MAXSTYLE] =
{
    { "font-weight:bold", 		  		BB_BOLD },
    { "font-style:italic", 				BB_ITALIC },
    { "text-decoration:underline", 		BB_UNDER },
    { "text-decoration:line-through", 	BB_STRIKE },
    { "font-size:", 					BB_SIZE },
    { "color:", 						BB_COLOR },
//    { "font-family:",					BB_FONT },
    { "text-align:left", 				BB_LEFT },
    { "text-align:center", 				BB_CENTER },
    { "text-align:right", 				BB_RIGHT }
};


// Parse style string and output corresponding values.
// style syntax: <tagname style="property:value;">
// General format: <tag style="param:value *[; param2:value][;] "> some text </tag>
// If a supported style was detected, write the code to OutBuf, copy the
// appropriate break tag, and return number of bytes written to OutStr.

int ProcessStyle(char *OutBuf)
{
    int i, Count;
    char *sptr, *dptr;
    char ch;

    // make sure there is a style and this is not an end tag
    if (G_StyleBuf[0] == 0 || G_CurTagStr[0] == '/') return(0);

    // remove spaces
    sptr = dptr = G_StyleBuf;
    while ((ch = *sptr++) != 0) if (!isspace(ch)) *dptr++ = ch;
    *dptr = 0;

    // parse styles
    Count = 0;
    sptr = strtok(G_StyleBuf, ";");
    while (sptr)
    {
        for (i = 0; i < ST_MAXSTYLE; i++)
        {
            char *param;
            int   LenParam;

            param = HtmlStyles[i].html;
            LenParam = strlen(param);
            if (memicmp(sptr, param, LenParam) == 0)
            {
                int bbcode;

                // found a match - copy break tag
                strcpy(G_BrkAttrs[i], G_CurTagStr);

                // output code
                bbcode = HtmlStyles[i].bbidx;
                *OutBuf++ = 0xFF;
                *OutBuf++ = (char) bbcode;
                Count += 2;

                // special case for size wich has a parameter
                if (bbcode == BB_SIZE)   // add size or color or font
                {
                    int rt;

                    rt = sprintf(OutBuf, "%x;", NormalizeFontSize(sptr + LenParam));
                    OutBuf += rt;
                    Count += rt;
                }
                // special case for size and color which have a parameter
                if (bbcode == BB_COLOR)   // add size or color or font
                {
                    int rt;

                    rt = sprintf(OutBuf, "%x;", NormalizeFontColor(sptr + LenParam));
                    OutBuf += rt;
                    Count += rt;
                }
				break;    // for loop
            }
        }

        sptr = strtok(NULL, ";");
    }

    G_StyleBuf[0] = 0;

    return(Count);
}


// Search for HTML Tag
// Tag must be null terminated string with no leading space.
// Return the BB enum value or 0 if not found.

int SearchTag(char *Tag)
{
    int i;

    if (!Tag) return(0);

    for (i = 0; HtmlCodes[i].html; i++)
 	{
        if (strcmpi(HtmlCodes[i].html, Tag) == 0)  // found it
            return(HtmlCodes[i].bbidx);
    }

    return(0);
}

// Process email string to make sure its compatable with BBCODE

void ExtractEmail(char *Ebuf)
{
    char *ptr, *pdomain;
    // enforce a text '@' text email.

    ptr = strchr(Ebuf, '@');
    if (!ptr)
    {
        Ebuf[0] = 0;
        return;
    }
    *ptr = 0;  // temp
    pdomain = ptr + 1;
    strrev(Ebuf);    // reverse order

    ptr = strpbrk(Ebuf, "?&<>()';:");
    if (ptr) *ptr = 0;
    strrev(Ebuf);    // correct order

    ptr = strpbrk(pdomain, "?&;:<>()'");
    if (ptr) *ptr = 0;

    ptr = strchr(Ebuf, 0);
    *ptr++ = '@';
    strcpy(ptr, pdomain);

}

// Process an URL so that it is a valid link URL
// Returns length of url in chars or 0 if invalid
// Requires the "http://" or "https://" protocol in the string.
// href="http://www.example.com/default.htm" - allowed
// href="https://www.example.com/default.htm" - allowed
// href="www.example.com"  - not allowed
// href="default.htm" - not allowed
// href="#top" - not allowed
// ftp://, file: - not allowed
// href="javascript:alert('Hello');" - not allowed

int  ExtractUrl(char *Ubuf)
{
    if (memicmp(Ubuf, "http://", 7) == 0 || memicmp(Ubuf, "https://", 8) == 0)
    {
        // good enough
        return(strlen(Ubuf));
    }
    return(0);
}



// Process the HTML tag and its attributes appropriately
// Returns the number of bytes written to outbuf.

int ProcessHtmlTag(char *OutBuf)
{
    int rt;
    static int G_Special_A_Type = 0;

    rt = SearchTag(G_CurTagStr);
    if (rt == 0) return(0);     // tag not handled here
    else if (rt < BB_MAXSIMPLE)   // simple tag
    {
        // with simple tags, we only have top save the value
        *OutBuf       = 0xFF;
        *(OutBuf + 1) = (unsigned char) rt;
        return(2);      // wrote 2 bytes
    }
    else if (rt == BB_CODE)    // special area that doesn't get processed
    {
        // Copy all text verbatim until an END_CODE is encountered.
        G_InPre = TRUE;          // Inside preformatted text
        G_CodeBreaker[0] = '/';
        strcpy(G_CodeBreaker + 1, G_CurTagStr);  // curtagstr is known to be smaller
        *OutBuf = 0xFF;
        *(OutBuf + 1) = BB_CODE;
        return(2);         // wrote 2 bytes
    }
    else if (rt == BB_END_CODE)
    {
        G_InPre = FALSE;
        *OutBuf = 0xFF;
        *(OutBuf + 1) = BB_END_CODE;
        return(2);         // wrote 2 bytes

    }
    else if (rt == BB_SPECIAL_A)   // special processing for anchor tags
    {
        // could be a link or could be a mailto
        // Examine the HREF attricute.  If the first chars of the HREF value
        // are "mailto:" then its processed as an email.
        // Otherwise, its processed as a link.
        G_Special_A_Type = 0;
        if (memicmp(G_EmailBuf, "mailto:", 7) == 0)  // Mailto
        {
            // The email address is extracted and the following
            // code is generated: BB_EMAIL {bare email address} ';'
            // which gets translated to [email={addr}]

            G_Special_A_Type = BB_END_EMAIL;
            ExtractEmail(G_EmailBuf);

            return(sprintf(OutBuf, "%c%c%s]", 0xFF, BB_EMAIL, G_EmailBuf));
        }
        else    // Link
        {
            // Generate: BB_URL {url} ';' which gets translated to: [url={url}]

            G_Special_A_Type = BB_END_URL;
            ExtractUrl(G_EmailBuf);
            return(sprintf(OutBuf, "%c%c%s]", 0xFF, BB_URL, G_EmailBuf));
        }
    }
    else if (rt == BB_END_SPECIAL_A)   // special processing for anchor tags
    {
        // Close tag for link or mailto

        if (G_Special_A_Type)
            return(sprintf(OutBuf, "%c%c", 0xFF, G_Special_A_Type));
    }
    else if (rt == BB_SPECIAL_IMG)  // special processing for image tags
    {
        // check paramters for src, Width and Height
        // HTML does not normally close an IMG tag, but XHTML and BBcode do
        // This generates [img]{src}[/img] or [img={width)x{height}] {src} [/img]

        if (!G_ImgSrc[0]) return(0);    // No image provided

        if (G_ImgWidth && G_ImgHeight)  // must have both to count
        {
            return(sprintf(OutBuf, "%c%c%dx%d]%s%c%c",
                           0xFF, BB_IMG2, // indicates paramters
                           G_ImgWidth, G_ImgHeight, G_ImgSrc, 0xFF, BB_END_IMG));
        }
        else   // only the image itself
        {
            return(sprintf(OutBuf, "%c%c%s%c%c",
                           0xFF, BB_IMG, // indicates no paramters
                           G_ImgSrc, 0xFF, BB_END_IMG));
        }
    }
    else if (rt == BB_SPECIAL_FONT)  // special processing for FONT tags
    {
        int len;

        // check parameters for color, face and size
        // If these parameters are included, Generates code
        // [font={face}], [color={color}] and [size={size}]

        // The closing "</FONT> is handled by the CloseStyle() function.
        len = 0;
//        if (G_FontFace[0])
//        {
//            rt = sprintf(OutBuf, "%c%c%s]", 0xFF, BB_FONT, G_FontFace);
//            OutBuf += rt;
//            len += rt;
//            strcpy(G_BrkAttrs[ST_FACE], "font");
//        }

        if (G_FontColor[0])
        {
            rt = sprintf(OutBuf, "%c%c%x;", 0xFF, BB_COLOR, NormalizeFontColor(G_FontColor));
            OutBuf += rt;
            len += rt;
            strcpy(G_BrkAttrs[ST_COLOR], "font");
        }

        if (G_FontSize[0])
        {
            rt = sprintf(OutBuf, "%c%c%x;", 0xFF, BB_SIZE, NormalizeFontSize(G_FontSize));
            // OutBuf += rt;
            len += rt;
            strcpy(G_BrkAttrs[ST_SIZE], "font");
        }

        return(len);

    }

    return(0);
}

// return a pointer to the next char in string that is not a whitespace char

char *SkipSpaces(char *str)
{
    int len;

    len = strspn(str, " \r\n\t\f");   // skip whitespace

    return(str + len);
}



// Parse hTML tag
// Given a pointer to a tag string starting with '<'.
// Tag string is not null terminated.
// Returns number of chars in buffer that caller should advance.  Usually
// this will point to the that after the '>'.
// Copies results to CurTagStr, G_EmailBuf, G_FontFace, G_FontColor,
// G_FontSize, G_ImgWidth and G_ImageHeight.
//
// HTML Tag Syntax * = zero or more, + = 1 or more, [ ] = optional:
//  htmltag := '<' tagname ( <attrdef>* | (whitespc)* ) ['/'] '>'
//  attrdef := (whitespc)+ AttributeName (whitespc)* [ '=' (whitespc)* <ValueStr> ] (whitespc)*
//  ValueStr := ( '"' value '"') | value


int ParseHtmlTag(char *TagStr)
{
    char *ptr, *ptr2;
    int len;
    char ch;
    char inquote;
    int retval;
    char tmpattr[MAXTAGLEN + 2];

    // Initialize variables for new tag
    memset(G_CurTagStr, 0, MAXTAGLEN + 1);
    memset(G_EmailBuf, 0, sizeof(G_EmailBuf));
    memset(G_StyleBuf, 0, sizeof(G_StyleBuf));
//    memset(G_FontFace, 0, sizeof(G_FontFace));
    memset(G_FontColor, 0, sizeof(G_FontColor));
    memset(G_FontSize, 0, sizeof(G_FontSize));
    memset(G_ImgSrc, 0, sizeof(G_ImgSrc));
    G_ImgWidth = 0;
    G_ImgHeight = 0;

    // Do we really have a tag?
    if (!TagStr || *TagStr != '<') return(0);  // Not a tag

    // Tag or '/' must be first char after '<' or its a syntax error
    TagStr++;   // point to first char in tag
    ptr = TagStr;
    if (*ptr == '/') ptr++;
    if (!isalpha(*ptr)) return(0);

    // Make sure there are no more '<' chars before a '>' char
    inquote = 0;
    retval = ptr - TagStr + 1;    // count chars in tag while we're at it.
    while ((ch = *ptr++) != 0)
    {
        retval++;

        if (ch == '"' || ch == '\'')
        {
            if (inquote == ch)   // end quote
            {
                inquote = 0;
                continue;
            }
            else if (inquote == 0)   // start quote
            {
                inquote = ch;
                continue;
            }
        }

        if (inquote) continue;   // ignore stuff in quotes

        if (ch == '<') return(0);   // this should not be here
        else if (ch == '>') break;  // found end of tag
    }
    if (ch == 0) return(0);         // If we hit the end, then it wasn't a real one.

    // When here, we are reasonably certain that this is a valid tag
    // and not something like "A < B"

    // Get tag
    len = strcspn(TagStr, " \r\n\t\f>");  // get len of tag
    if (len == 0) return(0);     // No tag found

    memcpy(G_CurTagStr, TagStr, min(len, MAXTAGLEN));   // copy tag

    ptr = SkipSpaces(TagStr + len);  // skip trailing white space
//    if (*ptr == '/' || *ptr == '>') return(retval);   // nothing but a tag

    // ptr should now point to first attribute

    // get attributes
    while (*ptr)
    {
        // get length of attribute
        len = strcspn(ptr, "= \r\n\t\f/>");
        if (len == 0) return(retval);     // no more attributes found

        // copy to tmpstr
        memset(tmpattr, 0, sizeof(tmpattr));
        memcpy(tmpattr, ptr, min(len, MAXTAGLEN));
        ptr += len;  // point to char that stopped the strcspn()

        if (*ptr != '=')   // we haven't found the '=' yet
        {
            ptr = SkipSpaces(ptr);   // skip any preceeding spaces
            if (*ptr != '=') continue; // no '=' means no value
        }

        // If here, ptr is pointing to the equal
        ptr = SkipSpaces(ptr + 1);     // skip any trailing spaces

        // When here, ptr is pointing to the first char in the value
        // This could be a dbl-quote, single-quote or a non-space

        ch = *ptr;
        if (ch == '\'' || ch == '"')
        {
            ptr++;                        // point to the first char in quote
            ptr2 = strchr(ptr, ch);       // get pointer to terminal quote
            if (!ptr2 || ptr == ptr2)
                break;  // missing terminal quote or empty string = no value
            len = ptr2 - ptr;     // length wiothout quotes
            ptr2++;    // point to char after closing quote
        }
        else   // unquoted value
        {
            len = strcspn(ptr, " \r\n\t\f/'`\"=<>");
            if (len == 0) return(retval);     // syntax error - value can't be null after '='
            ptr2 = ptr + len;    // point to char that stopped the scan
        }

        // when here, ptr is start of value, Len is the length, and
        // ptr2 is the next char after the value

        if (strcmpi(tmpattr, "style") == 0)    // is it a style attribute
        {
            memcpy(G_StyleBuf, ptr, min(sizeof(G_StyleBuf) - 1, len));
        }
        else if (strcmpi(G_CurTagStr, "font") == 0)    // FONT tag
        {
            if (strcmpi(tmpattr, "color") == 0)  // font color
 				memcpy(G_FontColor, ptr, min(sizeof(G_FontColor) - 1, len));
            else if (strcmpi(tmpattr, "size") == 0)  // font size
 				memcpy(G_FontSize, ptr, min(sizeof(G_FontSize) - 1, len));
//            else if (strcmpi(tmpattr, "face") == 0)  // font face
// 				memcpy(G_FontFace, ptr, min(sizeof(G_FontFace) - 1, len));
        }
        else if (strcmpi(G_CurTagStr, "img") == 0)    // IMG tag
        {
            if (strcmpi(tmpattr, "width") == 0)  // img width
                G_ImgWidth = atoi(ptr);
            else if (strcmpi(tmpattr, "height") == 0)  // img height
                G_ImgHeight = atoi(ptr);
            else if (strcmpi(tmpattr, "src") == 0)  // img src
 				memcpy(G_ImgSrc, ptr, min(sizeof(G_ImgSrc) - 1, len));
		}
        else if (strcmpi(G_CurTagStr, "a") == 0)    // Anchor tag
        {
            if (strcmpi(tmpattr, "href") == 0)  // either Mailto or link
 				memcpy(G_EmailBuf, ptr, min(sizeof(G_EmailBuf) - 1, len));
		}

        ptr = SkipSpaces(ptr2);   // point to start of next attribute
    }

    return(retval);
}


// Checks to see if tag is to be ignored.
// Modifies Buf if found.
// Returns 0 if start not found, 1 if start and stop found, -1 if start found but not stop.

int IgnoreStuff(char **Buf, char *Start, char *Stop)
{
    // Look for start string
    if (memicmp(*Buf, Start, strlen(Start)) == 0)   // special comment
    {
        char *ptr;

        // ignore everything until we get to stop string
        ptr = stristr(*Buf, Stop);
        if (ptr)
        {
            ptr = strchr(ptr, '>');   // move to closing '>'
            if (ptr)
            {
                // Found closing '>', return pointer to next char
                *Buf = ptr + 1;
                return(1);
            }
        }

        // EOF - move BUF to EOF
        *Buf = strchr(*Buf, 0);
        return(-1);   // EOF error

    }

    return(0);    // no match found
}



// 1. Convert HTML file to BBCode in place.  Known tags are replaced with
//    0xFF followed by a code byte.  If 0xFF is in the source, then it will
//    be followed by another 0xFF.
// 2. Trim white space to a single space.  "&nbsp;" is converted to a
//    real space, but not trimmed.
// 3. When a '<' is found and another is found before a '>', then
//    the text from the first '<' up to the second is treated like regular text.
// 4. If a tag is not syntactly correct, it is treated like regular text.
// When the file is printed, the actual BBCodes are substituted.

int Html2BBCode(char *Buf)
{
    char ch, *dptr;
    int  len;


    memset(G_BrkAttrs, 0, sizeof(G_BrkAttrs));   // clear attribute break array


    dptr = Buf;

    while ((ch = *Buf) != 0)
    {

        if (ch == (char) 0xFF)   // special escape char
        {
            // check to see if there is room for the escape code
            if (dptr - Buf >= 2)  // plenty of room
            {
                *dptr++ = 0xFF;
                *dptr++ = 0xFF;
            }
            else *dptr++ = ' ';    // wasn't enough room - discard char

            Buf++;
            continue;
        }

        if (ch == '<')
        {
            // ignore comments
            if (IgnoreStuff(&Buf, "<!--", "-->")) continue;

            // ignore scripts
            if (IgnoreStuff(&Buf, "<script", "</script")) continue;

            // ignore head section
            if (IgnoreStuff(&Buf, "<head", "</head")) continue;

            // ignore head section
            if (IgnoreStuff(&Buf, "<!doctype", ">")) continue;

            len = ParseHtmlTag(Buf);
            if (len)
            {
                // Found a valid tag if here
/*                if (G_InPre)    // we are in preformatted text
                {
                    // see if this was the matching terminal tag
                    if (strcmpi(G_CodeBreaker, G_CurTagStr) == 0)
                    {
                        // This is the only tag we process
                        // Cancel preformatted text
                        G_InPre = FALSE;
                        *dptr++ = (char) 0xFF;
                        *dptr++ = BB_END_CODE;
                        Buf += len;               // point to next char
                    }
                    else  // This was not an END tag - just copy the '<'
                    {
                        *dptr++ = '<';    // not an end tag
                        Buf++;
                    }
                }
                else   // not in preformatted text - process tag normally
  */              {
                    dptr += ProcessHtmlTag(dptr);  // process a handled tag
                    dptr += ProcessStyle(dptr);    // process style string
                    dptr += CloseStyle(dptr);      // close style attributes
                    Buf += len;                    // point to next char
                }
            }
            else   // zero len returned - not a syntactly correct tag
            {
//                char *ptr;

               // Zero length means it wasn't a valid HTML tag
//               ptr = strchr(Buf, '>');  // find end of tag
//               if (ptr) Buf = ptr + 1;
                 Buf++;
               *dptr++ = '<';    // not an start char
            }
            continue;
        }
        else if (ch == '&' /* && !G_InPre */ )
        {
            if (memicmp(Buf, "&nbsp;", 6) == 0)
            {
                *dptr++ = 0xA0;
                Buf += 6;
                continue;
            }
        }
        else if (ch == '\\' /* && !G_InPre */ )
        {
            if (memicmp(Buf, "\\u00a0", 6) == 0)
            {
                *dptr++ = 0xA0;
                Buf += 6;
                continue;
            }
        }

        // Just a regular char if here - save char as is
        *dptr++ = ch;
        Buf++;

        // compress spaces
        if (/* !G_InPre && */ isspace(ch))
        {
            *(dptr - 1) = ' ';   // force to a real space
            while (isspace(*Buf)) Buf++;
        }

    }
    *dptr = 0;   // null terminate last char

    return(0);
}


// Search for and remove improper tags
// [b][b]text[/b][/b]  -->  [b]text[/b]   // duplicate tags
// [b][/b] --> ""    // empty tags
// [size=12][size=16]  --> [size=16]      // empty tags
// [size=12]text[size=16]  --> [size=12]text[/size][size=16] // missing end tag
// We do this by keeping a table of flags of common attributes.  The an
// attribute is activated, the flag for that attribute is set to TRUE, but no
// command is output.  When an actual printable text (non-space) is encountered,
// then all the attribute commands are output at once.  If a second valued tag
// (color, size, etc) of the same kind is encountered before actual text, then
// the new value superceeds the previous tag.
// If a new valued tag of the same type is seen *after* printable text, then we
// generate an <end> tag for it followed by the new tag with the new value.
// So when we encounter tags in the source, we set the flag to 1.  When we
// output the command for it, we set the flag to 2.  when we output the <end>
// tag for it, we clear the flag to 0.
// At the end of the buffer, if there are any flags that are 2, then we outout
// the <end> tags for them.

void BBCleanup(char *sBuf)
{
    char *dptr, *dBuf, *sptr;
    int len;
    unsigned int val, Flags[BB_MAXCODES];
    unsigned char ch;

    len = strlen(sBuf);
    if (len == 0) return;    // nothing to do

    dBuf = malloc(len * 2);   // Temp buffer twice as big
    if (!dBuf) return;        // not enough memory

    memset(Flags, 0, sizeof(Flags));    // clear flags

    dptr = dBuf;
    sptr = sBuf;

    while ((ch = *sptr++) != 0)
    {
        if (ch == 0xFF)  // command char
        {
            ch = *sptr++;
            if (ch == 0xFF)  // escape char
            {
                *dptr++ = ch;     // just copy as is where is
                *dptr++ = ch;
            }
            else if (ch < BB_MAXCODES)// command flag
            {
                int state = Flags[ch] & 0x3;
                if (state == 0)      // not previously set
                {
                    val = 0;
                    if (ch == BB_COLOR || state == BB_SIZE)
                    {
                        // Read the value and save it to the upper 16 bits of flag
                        val = strtoul(sptr, &sptr, 16);  // stops on ';'
                        sptr++;                          // point to char after ';'
                    }

                    // mark the flag as having been read, but not output
                    Flags[ch] = ((val << 16) & 0xFFFF0000) | 0x0001;
                }
                else if (state == 1)
                {

                }
                else if (state == 3)
                {
                }

            }
        }


    }


    // copy over new buffer
    *dptr = 0;
    strcpy(sBuf, dBuf);
    free(dBuf);
}



int PrintBBCode(FILE *fp, char *Buf)
{
    unsigned char ch;

    while ((ch = (unsigned char) *Buf++) != 0)
    {
        if (ch == 0xFF)   // encoded BBCODE
        {
            ch = *Buf++;   // get next char
            if (ch == 0xFF) fputc(0xFF, fp);
            else if (ch > 0 && ch < (sizeof(BBCodes) / sizeof(BBCodes[0])))
            {
           		fprintf(fp, "%s", BBCodes[ch]);
                if (ch == BB_COLOR)   // special case for colors
                {
                    int val = strtol(Buf, &Buf, 16);

                    fprintf(fp, "%s]", GetHtmlColorString(val));
                    Buf++;
                }
                else if (ch == BB_SIZE)   // special case for colors
                {
                    int val = strtol(Buf, &Buf, 16);

                    fprintf(fp, "%d]", val);
                    Buf++;
                }
            }
//            else fprintf(fp, "[INVALID: %02X %02X]", 0xFF, ch);
            // anything else not in table is not printed
        }
        else   // just a regular char
        {
            fputc(ch, fp);
        }

    }

    return(0);

}


int PrintHtmlCode(FILE *fp, char *Buf)
{
    unsigned char ch;
    char *ptr;

    while ((ch = (unsigned char) *Buf++) != 0)
    {
        if (ch == 0xFF)   // encoded BBCODE
        {
            ch = *Buf++;   // get next char
            if (ch == 0xFF) fputc(0xFF, fp);
            else if (ch == BB_URL)
            {
                ptr = strchr(Buf, ']');
                *ptr = 0;
                fprintf(fp, HtmlPrint[BB_URL], Buf);
                Buf = ptr + 1;
            }
            else if (ch == BB_EMAIL)
            {
                fprintf(fp, "");
            }
            else if (ch == BB_IMG)
            {
                fprintf(fp, "<img src=\"");
                while ((ch = *Buf++) != 0xFF && ch != 0)
                   fputc(ch, fp);
                fprintf(fp, "\">");
                Buf--;
            }
            else if (ch == BB_IMG2)
            {
                fprintf(fp, "");

            }
            else if (ch == BB_SIZE)
            {
                int val = strtol(Buf, &Buf, 16);

                fprintf(fp, "<span style=\"font-size:%d%\">", val);
                Buf++;      // skip over ';'
            }
            else if (ch == BB_COLOR)
            {
                int val = strtol(Buf, &Buf, 16);

                fprintf(fp, "<font color=\"%s\">", GetHtmlColorString(val));
                Buf++;    // skip over ';'
            }
            else if (ch > 0 && ch < (sizeof(BBCodes) / sizeof(BBCodes[0])))
            {
                fprintf(fp, "%s", HtmlPrint[ch]);
            }
            // anything else not in table is not printed
        }
        else   // just a regular char
        {
            fputc(ch, fp);
        }

    }

    return(0);

}


