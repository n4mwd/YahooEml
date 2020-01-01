

#include "YahooEml.h"

// Count raw JSON files
// Set the globals MinJson and MaxJson.
// Return count of files or -1 on error.

int CountJsonFiles(char *DirName, int *Start, int *Stop)
{
    struct dirent *ent;  // Pointer for directory entry
    DIR *dir;            // Directory handle
    int MsgNum;
    char *ptr;
    int MinJson, MaxJson, NumMsgs;

    if (!Start || !Stop) return(-1);   // validate parameter

    MinJson = MAXINT;
    MaxJson = 0;
    NumMsgs = 0;
    // opendir() returns a pointer of DIR type.
    dir = opendir(DirName);
    if (dir == NULL)  // opendir returns NULL if couldn't open directory
    {
        printf("Could not open directory '%s'", DirName);
        return(-1);
    }

    while ((ent = readdir(dir)) != NULL)
    {
        MsgNum = strtol(ent->d_name, &ptr, 10);
        if (stricmp("_RAW.JSON", ptr) == 0)
        {
            // If here, we found a raw json file, but is it in range
            if (MsgNum >= *Start && MsgNum <= *Stop)
            {
                // If here, it is one that we are processing
                if (MsgNum < MinJson) MinJson = MsgNum;
                if (MsgNum > MaxJson) MaxJson = MsgNum;
                NumMsgs++;
            }
        }
    }
    //printf("Processing %d messages.\n", NumMsgs);

    closedir(dir);

    *Start = MinJson;
    *Stop = MaxJson;

    return(NumMsgs);
}

void main(int argc, char *argv[])
{
    char DirName[256], OutDirName[256], *ptr;
    int i, Start, Stop, NumMsgs, EmlMode;

    Start = 0;             // defaults
    Stop = MAXINT;
    EmlMode = COMPACTEML;
    DirName[0] = 0;

    if (argc < 2)
    {
        printf("usage: YahooEml GroupName\n\tFor help: YahooEml -h");
        exit(1);
    }

    printf("\n");
    for (i = 1; i < argc; i++)  // scan parameters
    {
        if (argv[i][0] == '-')    // parameter
        {
            switch(argv[i][1])
            {
                case 'h':    // help
                    printf("YahooEml (optional flags) GroupName\n\n");

                    printf("This program is to compliment the yahoo downloader program written by\n"
                           "IgnoredAmbiance.  You must use that python script to download your\n"
                           "files from Yahoo before using this program.\n\n");

                    printf("GroupName is the name of the directory where the group's data files are\n"
                           "stored.  There must be a directory under that called 'email' which must\n"
                           "contain all the JSON message files and attachments to be processed.\n\n");

                    printf("This program will read all the JSON files for the specified group and\n"
                           "convert them to '.eml' files in the 'GroupName\\email\\eml' directory.\n"
                           "Files already existing in the 'eml' directory may be overwritten.\n\n");

                    printf("Optional flags:\n");
                    printf(" -h\tThis help screen.\n");
                    printf(" -r\tGenerate raw EML files - no attachments.\n");
                    printf(" -c\tGenerate compact messages with attachements. (default)\n");
                    printf(" -b\tGenerate BBCode files - no attachments.\n");
                    printf(" -x\tGenerate a single XML file with all messages and attachments.\n");
                    printf(" -m[range] Process only messages within specified range.\n"
                           " \tWhere [range] describes a message number range in the format \"Start#-End#\".\n"
                           " \tNo spaces are permitted.  Default values used if one or more parameters are\n"
                           " \tomitted.  So \"-m400-900\", \"-m500\", \"-m25-\" and \"-m-700\" are legal.\n");
                    printf(" -j Recompress GIF and JPG attachments to save space.\n");

                    printf("\nSee ReadMe.txt for more detailed help including file formats.\n");

                    printf("\n");
                    exit(1);
                    break;

                case 'r':
                    EmlMode = RAWEML;
                    break;

                case 'c':
                    EmlMode = COMPACTEML;
                    break;

                case 'b':
                    EmlMode = BBEML;
                    break;

                case 'x':
                    EmlMode = XMLEML;
                    printf("XML function is not working yet.\n");
                    break;

                case 'j':
                    printf("Image compression mode is not working yet.\n");
                    break;

                case 'm':     // Message Range
                    ptr = &argv[i][2];
                    if (isdigit(*ptr))   // starts with number
                    {
                        Start = strtol(ptr, &ptr, 10);
                    }
                    else if (*ptr != '-') goto ParamErr;  // error

                    if (*ptr == '-')   // has possible 2nd parameter
                    {
                        ptr++;  // point to possible second number
                        if (isdigit(*ptr)) Stop = strtol(ptr, NULL, 10);
                    }
                    else if (*ptr == 0)  // only one number specified
                    {
                        Stop = Start;
                    }
                    else goto ParamErr;   // error
                    break;



                default:
ParamErr:
                    printf("Invalid parameter: %s\n", argv[i]);
                    break;
            }
        }
        else
        {
            // copy group name
            if (strlen(argv[i]) > 200)  // abort if filename too long
            {
                printf("Group path too long.\n'%s'\n"
                       "Try again from a short directory path.\n", argv[i]);
                exit(1);
            }
            if (DirName[0])
            {
                printf("Error: Group Name has already been set to '%s'\n"
                       "Parameter '%s' will be ignored.\n"
                       "Use quotes if group name has spaces.\n",
                       DirName, argv[i]);
            }
            strcpy(DirName, argv[i]);
            strcat(DirName, "/email");
        }

    }   // end for()


    NumMsgs = CountJsonFiles(DirName, &Start, &Stop);
    if (NumMsgs < 0) return;
    else printf("Processing %d messages from %d to %d.\nin direectory: %s\n",
                 NumMsgs, Start, Stop, DirName);

    // Create output directory
    strcpy(OutDirName, DirName);
    strcat(OutDirName, "/eml");
    mkdir(OutDirName);

    // process all files
    for (i = Start; i <= Stop; i++)
    {
        ProcessJson(i, EmlMode, DirName);
    }


    exit(0);
}


