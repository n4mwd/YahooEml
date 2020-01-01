#include "YahooEml.h"

typedef struct COLORREC
{
    char *ColorStr;
    int  ColorIdx;
} COLORREC;



static COLORREC HtmlColors[] =
{
    { "Black",                 0x000000 },
    { "White",                 0xFFFFFF },
    { "IndianRed",             0xCD5C5C },
    { "LightCoral",            0xF08080 },
    { "Salmon",                0xFA8072 },
    { "DarkSalmon",            0xE9967A },
    { "LightSalmon",           0xFFA07A },
    { "Crimson",               0xDC143C },
    { "Red",                   0xFF0000 },
    { "FireBrick",             0xB22222 },
    { "DarkRed",               0x8B0000 },
    { "Pink",                  0xFFC0CB },
    { "LightPink",             0xFFB6C1 },
    { "HotPink",               0xFF69B4 },
    { "DeepPink",              0xFF1493 },
    { "MediumVioletRed",       0xC71585 },
    { "PaleVioletRed",         0xDB7093 },
    { "LightSalmon",           0xFFA07A },
    { "Coral",                 0xFF7F50 },
    { "Tomato",                0xFF6347 },
    { "OrangeRed",             0xFF4500 },
    { "DarkOrange",            0xFF8C00 },
    { "Orange",                0xFFA500 },
    { "Gold",                  0xFFD700 },
    { "Yellow",                0xFFFF00 },
    { "LightYellow",           0xFFFFE0 },
    { "LemonChiffon",          0xFFFACD },
    { "LightGoldenrodYellow",  0xFAFAD2 },
    { "PapayaWhip",            0xFFEFD5 },
    { "Moccasin",              0xFFE4B5 },
    { "PeachPuff",             0xFFDAB9 },
    { "PaleGoldenrod",         0xEEE8AA },
    { "Khaki",                 0xF0E68C },
    { "DarkKhaki",             0xBDB76B },
    { "Lavender",              0xE6E6FA },
    { "Thistle",               0xD8BFD8 },
    { "Plum",                  0xDDA0DD },
    { "Violet",                0xEE82EE },
    { "Orchid",                0xDA70D6 },
    { "Fuchsia",               0xFF00FF },
    { "Magenta",               0xFF00FF },
    { "MediumOrchid",          0xBA55D3 },
    { "MediumPurple",          0x9370DB },
    { "RebeccaPurple",         0x663399 },
    { "BlueViolet",            0x8A2BE2 },
    { "DarkViolet",            0x9400D3 },
    { "DarkOrchid",            0x9932CC },
    { "DarkMagenta",           0x8B008B },
    { "Purple",                0x800080 },
    { "Indigo",                0x4B0082 },
    { "SlateBlue",             0x6A5ACD },
    { "DarkSlateBlue",         0x483D8B },
    { "MediumSlateBlue",       0x7B68EE },
    { "GreenYellow",           0xADFF2F },
    { "Chartreuse",            0x7FFF00 },
    { "LawnGreen",             0x7CFC00 },
    { "Lime",                  0x00FF00 },
    { "LimeGreen",             0x32CD32 },
    { "PaleGreen",             0x98FB98 },
    { "LightGreen",            0x90EE90 },
    { "MediumSpringGreen",     0x00FA9A },
    { "SpringGreen",           0x00FF7F },
    { "MediumSeaGreen",        0x3CB371 },
    { "SeaGreen",              0x2E8B57 },
    { "ForestGreen",           0x228B22 },
    { "Green",                 0x008000 },
    { "DarkGreen",             0x006400 },
    { "YellowGreen",           0x9ACD32 },
    { "OliveDrab",             0x6B8E23 },
    { "Olive",                 0x808000 },
    { "DarkOliveGreen",        0x556B2F },
    { "MediumAquamarine",      0x66CDAA },
    { "DarkSeaGreen",          0x8FBC8B },
    { "LightSeaGreen",         0x20B2AA },
    { "DarkCyan",              0x008B8B },
    { "Teal",                  0x008080 },
    { "Aqua",                  0x00FFFF },
    { "Cyan",                  0x00FFFF },
    { "LightCyan",             0xE0FFFF },
    { "PaleTurquoise",         0xAFEEEE },
    { "Aquamarine",            0x7FFFD4 },
    { "Turquoise",             0x40E0D0 },
    { "MediumTurquoise",       0x48D1CC },
    { "DarkTurquoise",         0x00CED1 },
    { "CadetBlue",             0x5F9EA0 },
    { "SteelBlue",             0x4682B4 },
    { "LightSteelBlue",        0xB0C4DE },
    { "PowderBlue",            0xB0E0E6 },
    { "LightBlue",             0xADD8E6 },
    { "SkyBlue",               0x87CEEB },
    { "LightSkyBlue",          0x87CEFA },
    { "DeepSkyBlue",           0x00BFFF },
    { "DodgerBlue",            0x1E90FF },
    { "CornflowerBlue",        0x6495ED },
    { "MediumSlateBlue",       0x7B68EE },
    { "RoyalBlue",             0x4169E1 },
    { "Blue",                  0x0000FF },
    { "MediumBlue",            0x0000CD },
    { "DarkBlue",              0x00008B },
    { "Navy",                  0x000080 },
    { "MidnightBlue",          0x191970 },
    { "Cornsilk",              0xFFF8DC },
    { "BlanchedAlmond",        0xFFEBCD },
    { "Bisque",                0xFFE4C4 },
    { "NavajoWhite",           0xFFDEAD },
    { "Wheat",                 0xF5DEB3 },
    { "BurlyWood",             0xDEB887 },
    { "Tan",                   0xD2B48C },
    { "RosyBrown",             0xBC8F8F },
    { "SandyBrown",            0xF4A460 },
    { "Goldenrod",             0xDAA520 },
    { "DarkGoldenrod",         0xB8860B },
    { "Peru",                  0xCD853F },
    { "Chocolate",             0xD2691E },
    { "SaddleBrown",           0x8B4513 },
    { "Sienna",                0xA0522D },
    { "Brown",                 0xA52A2A },
    { "Maroon",                0x800000 },
    { "Snow",                  0xFFFAFA },
    { "HoneyDew",              0xF0FFF0 },
    { "MintCream",             0xF5FFFA },
    { "Azure",                 0xF0FFFF },
    { "AliceBlue",             0xF0F8FF },
    { "GhostWhite",            0xF8F8FF },
    { "WhiteSmoke",            0xF5F5F5 },
    { "SeaShell",              0xFFF5EE },
    { "Beige",                 0xF5F5DC },
    { "OldLace",               0xFDF5E6 },
    { "FloralWhite",           0xFFFAF0 },
    { "Ivory",                 0xFFFFF0 },
    { "AntiqueWhite",          0xFAEBD7 },
    { "Linen",                 0xFAF0E6 },
    { "LavenderBlush",         0xFFF0F5 },
    { "MistyRose",             0xFFE4E1 },
    { "Gainsboro",             0xDCDCDC },
    { "LightGray",             0xD3D3D3 },
    { "Silver",                0xC0C0C0 },
    { "DarkGray",              0xA9A9A9 },
    { "Gray",                  0x808080 },
    { "DimGray",               0x696969 },
    { "LightSlateGray",        0x778899 },
    { "SlateGray",             0x708090 },
    { "DarkSlateGray",         0x2F4F4F },
    { NULL,                    NULL }
};


// converts shorthand color "#F6F" to "#FF66FF".
// Makes sure that color is a valid HTML color.
// Converts rgb(r,g,b) to #xxxxxx.
// Then finally finds the coloest color namme if not already given.
// Str is assumes to not have a null terminator.
// Returns an index to the Htmlcolors table.

int NormalizeFontColor(char *str)
{
    unsigned int newdiff, mindiff, mindiffidx;
    char tmpstr[32];
    int i;
    int r, g, b, r1, g1, b1, val;

    if (*str == '#')   // numeric
    {
        int ln;

        str++;
        ln = min(strspn(str, "0123456789ABCDEFabcdef"), sizeof(tmpstr));
        memcpy(tmpstr, str, ln);
        tmpstr[ln] = 0;

        if (ln < 3)     // too short - add leading zeros
        {
            strrev(tmpstr);
            strcat(tmpstr, "000");
            tmpstr[3] = 0;
            strrev(tmpstr);
            ln = 3;
        }

        if (ln == 3)   // expand shorthand
        {
            strcat(tmpstr, tmpstr);
            tmpstr[1] = tmpstr[3];
            tmpstr[2] = tmpstr[4];
            tmpstr[3] = tmpstr[4];
            tmpstr[4] = tmpstr[5];
            ln = 6;
        }

        if (ln > 6)
        {
            tmpstr[6] = 0;   // truncate
//            ln = 6;
        }

        // convert to RGB
        val = strtoul(tmpstr, NULL, 16);

    }
    else if (memicmp(str, "rgb", 3) == 0)
    {
        char *ptr;

        memcpy(tmpstr, str + 3, sizeof(tmpstr));
        ptr = strtok(tmpstr, "(), \t\r\n");
        r = strtoul(ptr, NULL, 0) & 0xFF;
        ptr = strtok(NULL, "(), \t\r\n");
        g = strtoul(ptr, NULL, 0) & 0xFF;
        ptr = strtok(NULL, "(), \t\r\n");
        b = strtoul(ptr, NULL, 0) & 0xFF;

        val = (r << 16) | (g << 8) | b;
    }
    else    // it must be a color
    {
        // copy color to tmpstr
        memset(tmpstr, 0, sizeof(tmpstr));
        for (i = 0; isalpha(str[i]) && i < sizeof(tmpstr) - 1; i++)
            tmpstr[i] = str[i];

        // make sure it is in table
        for (i = 0; HtmlColors[i].ColorStr; i++)
        {
            if (strcmpi(tmpstr, HtmlColors[i].ColorStr) == 0)
                return(i);
        }
        // wasn't a legal color if here
        return(0);     // black
    }

    //convert to RGB and search table
    r = (val >> 16) & 0xFF;
    g = (val >> 8) & 0xFF;
    b = val & 0xFF;
    mindiff = -1;
    mindiffidx = 0;

    for (i = 0; HtmlColors[i].ColorStr; i++)
    {
        // split components
        val = HtmlColors[i].ColorIdx;
	    r1 = (val >> 16) & 0xFF;
    	g1 = (val >> 8) & 0xFF;
	    b1 = val & 0xFF;

        // get differences
        r1 -= r;
        g1 -= g;
        b1 -= b;

        // square
        newdiff = r1 * r1 + g1 * g1 + b1 * b1;
        if (newdiff < mindiff)
        {
            mindiff = newdiff;
            mindiffidx = i;
        }

    }

    return(mindiffidx);

}


// Convert html font size into pixels
// 1=10px, 2=13px, 3=16px, 4=18px, 5=24px, 6=32px, 7=48px (10 is off the chart - max is 7)
// "EM" and "%" are percentages of 16px so:
//     "1em" = "100%" = "16px"
//     "2em" = "200%  = "32px"
//     "0.5em" = "50%" = "8px"
//
// Take a string with the suffixes "px", "%", "em" or nothing and return the
// pixel size.  If no suffix, then html-size is assumed.
// Input string does not have to be null terminated but must start with a digit.
// It ends when an unrecognized char is read.


int NormalizeFontSize(char *str)
{
//    static char htmlsizes[8] = { 0, 10, 13, 16, 18, 24, 32, 48 };  // in px
    static int htmlsizes[8] = { 0, 63, 81, 100, 113, 150, 200, 300 };    // in %
    float val;
    char *endptr, sign;
    int ival;

    sign = *str;      // get possible size
    val = strtod(str, &endptr);
    if (memicmp(endptr, "em", 2) == 0)  // process as EM
        return((int)(100.0 * val));
    if (*endptr == '%')    				// process as percent
        return((int)(val));
    if (memicmp(endptr, "px", 2) == 0)  // process as PX
        return((int)(val / 0.16));

    // if here, we must assume its an html size and look it up in the table
    ival = (int) val;

    // handle any relative signs
    if (sign == '-' || sign == '+') ival += 3;   // relative to size = 3

    // correct bounds
    if (ival < 1) ival = 1;
    if (ival > 7) ival = 7;

    return(htmlsizes[ival]);   // look it up and return
}


char *GetHtmlColorString(int val)
{
    if (val >= sizeof(HtmlColors) / sizeof(HtmlColors[0])) val = 0;
    return(HtmlColors[val].ColorStr);
}
