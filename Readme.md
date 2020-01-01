# **YAHOOEML**

If you are trying to get your data from yahoo groups into a format that can be used by a web board such as phpBB, then you have come to the right place.

This program will read JSON files written by [Yahoo Group Downloader](https://github.com/IgnoredAmbience/yahoo-group-archiver) written by Ignored Ambiance.  You must use that python script to download your RAW JSON files from Yahoo before using this program.  This program will then write raw .EML files, simplified .EML files, .BB files or an .XML file containing all of the yahoo group posts in a single file.

The current state of this file is a work in progress.  Currently, only RAW mode works well.  COMPACT and BBCODE modes work fair.  XML format does not work at all.  Image recompression does not work either.  Needless to say that you should not delete your source JSON files.  I am working on this as fast as I can trying to do a bunch of stuff all at once.

The source files are written in C and will compile with Borland C v5.02\. It has no dependencies.  The executable is a 32 bit Windows console program.  A precompiled executable is provided here (about 90K on disk).

### How to Run YAHOOEML

YahooEml is a Windows command prompt program.  You will need to copy it to the parent folder where you ran the python script.  That is the folder with all your groups listed as sub-folders.

To start the Windows Command Prompt, go to START,  then enter "cmd" in the search box and press enter.  The command prompt should start.  Navigate to the parent folder described above.  Type "YahooEml -h" then press enter and it will give you a list of the options.

The basic command line format is:

YahooEml (optional flags) GroupName

where optional flags are shown below. The GroupName is the name of the group folder where your downloaded group files are stored.

Optional Flags:  
-h Help - prints a more detailed Help page.

-r Generate raw EML files - no attachments. The raw EML files are straight from Yahoo and have had their attachments stripped off into separate files. This option is usefull if you want to preserve all the original headers and formatting.

-c Generate compact messages with attachements. This option (default) strips away junk headers and leaves only the bare essential ones. If both Text and HTML versions are present, it keeps only the HTML version, but simplifies it so that only basic formatting is preserved. Attachments are re-attached to the basic email. An attempt is made to preserve inline photos, but not guaranteed.

-b Generate BBCode files - with attachments. BBCode files are described below.

-x Generate a single XML file with all messages and attachments. The XML file is threaded so that messages appear as threads rather than a list of messages.  XML file format is described below.  **Not currently working.**

-m[range] Process only messages within specified range. "[range]" is a variable format, but must not contain any spaces. "[range]" is a message number range consisting of an optional start, an optional '-' separator and optional end.

Examples:  
-m500 Process only message 500.  
-m200-400 Process messages from 200 to 400.  
-m-1000 Process from the beginning up to 1000.  
-m5000- Process 5000 to the end.

-j Recompress GIF and JPG attachments to save space. For some reason, some cameras and scanners save their JPG images in nearly uncompressed formats. Likewise, some people upload photos in BMP and GIF formats which take a huge amount of space. This option will recompress images into JPG formats, but only if the resulting JPG file is  smaller than the original.  **Not Currently working.**

The **CURRENT WORKING DIRECTORY IS CRITICAL**. The program must be run from the group's parent folder as described above.  Under the group's folder, the program will look for an "EMAIL" folder for the JSON files.  For example, PARENT/GroupName/EMAIL. Currently, this program only reads the *_RAW.JSON files.  When run, the program generates all of its output files in a new EML subdirectory, PARENT/GroupName/EMAIL/EML.

###   
BBCode File Format

The BBCode format has a header, similar to regular email files, but the manner in which it handles attachments is different.  Also, the charset is fixed to "UTF-8".  The message itself is in 8bit plain text.  However, the attachments are in Base64.  Message sections and attachments are separated by a "\r\n0xFF\r\n" byte sequence.  Sample headers are shown below:

From: John Smith <smithman@aol.com>  
To: Super_Forum@yahoogroups.com  
Subject: High cost of snails in restaurants  
Date: Mon, 18 May 2015 13:41:25 00  
X-Originating-IP: 166.34.121.22  
X-User-Id: smithman99  
X-Msg-Id: 4076  
X-Topic-Id: 4065  
X-Topic-Next: 4077  
X-Topic-Prev: 4075  
X-Charset: UTF-8  
X-Attach: "image1.jpeg" ID=1  
X-Attach: "image2.jpeg" ID=2  
X-Attach: "image3.jpeg" ID=3  
X-Attach: "image4.jpeg" ID=4

This is the [b]message[/b] separated by a [u]single[/u] blank line.

Notice that the attachments are all defined in the one and only header in the order they appear in the file.  As said before, the boundary between sections is always "\r\n0xFF\r\n".  The text portion of the message is always first in 8bit plain text using the UTF-8 charset, followed by any attachments in Base64 format.

### **XML File Format**

Not defined yet.  However the format will be thread based to that the topics are grouped together.  This is to make it better suited for web boards like phpBB.

### **Future Possibilities**

Yahoo also provided a GetMyData tool that returned most of the messages in MBOX format.  Most attachments were missing.  As such, one possible upgrade is to make the program also accept MBOX files.  Another upgrade would be to make the program less restrictivbe when it comes to what folder it must be run in.  If there is a demand, I might even add a GUI interface for those who are less technical.
