# Canon CanoScan FS4000US Driver Project Page

> [Last updated on January 2, 2006]
> 
> <div class="alertdiv">
> 
> I'd intended to keep working on this over time and refining it but with my interest in film waning and now having a full-time job again, I'll defer to two others who took my initial work and ran with it:
> 
> [Click here](http://www.wildestdreams.nl/pages/fs4000us_main.php) for Michael Haaksman's site. He made a nice GUI and did a lot of work to get scratch & dust removal working.
> 
> [Click here](http://home.exetel.com.au/phelum/fs4.htm) for Steven Saunderson's site. Steven did a lot of nice work getting the low-level calibration and exposure settings working well. He also got the USB interface working.
> 
> </div>
> 
> This page is a work-in-progress and contains the latest notes, code, etc. related to creating a custom driver for the Canon CanoScan FS4000US scanner.

## Building the code

First of all, in case there are problems, it's best to make sure we're starting from similar configurations. Here's mine:

### Hardware

*   Windows XP Service Pack 2
*   CanoScan FS4000 with bios version 1.07 attached with an Adaptec SCSI card. Host adapter id 3, target #4\. No other SCSI devices.

### Software

*   Visual Studio .NET 2003
*   FilmGet 1.04
*   VueScan 8.0.1
*   ASPI SDK 4.71.2 from Adaptec (see link below)
*   libTIFF 3.6.1 (see link below)

### Build instructions

1.  Install the ASPI SDK per Adaptec's instructions. (If you already use VueScan, you probably don't need to do this since ASPI is on your machine.)
2.  Unzip and build libtiff per the enclosed instructions. I built it as a static library instead of a DLL to keep things simplest.
3.  Unzip my code into some directory (I called mine fs4000).
4.  You'll need to edit the make file to point the LIBTIFFDIR macro to your local path.
5.  cd to the fs4000 directory, type 'nmake' and cross your fingers. My build works without warning or error. Hopefully it will for you too.

## Build output

The make file currently generates 3 executables:

**avgscans.exe**: Doesn't even touch the scanner. I wrote this to test the value of multi-pass scanning. Takes several input files of the same image and outputs a new file with averaged values. Pretty simple. No command-line options: the input is hard-coded.

**xlate-fglog.exe**: Short for "translate FilmGet log" although I later added the ability to translate VueScan logs as well. The idea is to take as input a FilmGet or VueScan log (see below for how to generate these) and output a normalized and easy-to-read version of the log that can be compared to others with a diff tool. This tool used with diff is critical to understanding what is going on with the scanner and how these other two programs drive it. As I understood more and more about the scanner, I improve the output info of this and can regenerate processed logs from originals, possibly revealing something I didn't know about an old log..

> <pre>usage: xlate-fglog.exe -[h?] -v [-i <inputfile>] [-o <outputfile>]
> 
> -i uses <inputfile> instead of stdin
> -o uses <outputfile> instead of stdout
> -v reads a VueScan log instead of FilmGet
> -?, -h this message</pre>

By default, xlate-fglog acts like a Unix filter, taking input from stdin and writing output to stdout. I use two aliases with my command shell (a Windows port of Unix's tcsh [found here](http://www.blarg.net/~amol/)) to make this tool even easier to use:

> <pre>xfgl ("C:/Documents and Settings/Dave/My Documents/Visual Studio Projects/fs4000/xlate-fglog.exe" -i !^ > !^.proc)
> xvsl ("C:/Documents and Settings/Dave/My Documents/Visual Studio Projects/fs4000/xlate-fglog.exe" -v -i !^ > !^.proc)</pre>

Usage with these becomes: "xfgl foo" or "xvsl foo" which outputs foo.proc. Once in a rare while, xlate-fglog will choke on a FilmGet log because the FilmGet log has a mangled line that my code doesn't understand. The log looks like two threads were writing to it at once. Since this is rare and a bit unpredictable, I don't handle this in the code. I just tweak the original log so that it will parse correctly and then rerun xlate-fglog.

**fs4000test.exe**: this is my test driver program where I put what I've learned with xlate-fglog to use. You'll see a simple main() which initiates control of the scanner and then calls one of many small test routines which are described in comments in the code.

## Roadmap to the source files

**Makefile** - very simple at this point. Only builds a debug version of everything.

**fs4000-scsi.c**<br/>
**fs4000-scsi.h** - these two files are the heart of all this and where the most effort has gone. All FS4000 commands and order in which they should be used are defined, described, etc. here. Rather than duplicate doc from the .h file on a web page, I'll just refer you to the .h file to understand it all. There is a lot here to absorb - to know the FS4000 well means knowing these files. These files are used both by fs4000test and by xlate-fglog (although xlate-fglog doesn't operate the scanner, it does use the printing routines here for uniformity of output).

**fs4000test.c** - the main file for test routines.

**xlate-fglog.c** - the main file for xlate-fglog. Contains all the code that reads and normalizes original log files.

**avgscans.c** - only source file for avgscans.exe described above.

**aspi_wrappers.c  
scsi_wrappers.h** - these are simple wrappers around ASPI routines that make it pretty easy to send commands/data to and from the scanner. You'll need to modify do_scsi_command() for your SCSI host adapter ID (experiment for this. It's likely to be something like 0, 1, 2, or 3) and target # (what the dial on the bottom of your scanner is set to). Otherwise, I don't think we'll ever need to mess with this until it's time to make a more robust thing that's not intended for experimentation.

**wnaspi32.h** - to be honest, I'm not sure where this came from but it covers the routines in ASPI just fine. I think I got it from an ASPI tutorial site that made this from the header files included with the ASPI SDK. Pretty shoddy software engineering on my part but I hand-checked the contents of this against the ASPI headers and it's fine.

**getopt.c**<br />
**getopt.h** - a public domain version of getopt() used in xlate-fglog.

## Generating log files from FilmGet and VueScan:

Turning on logging in VueScan is easy: on the Output tab, just check the box named "Log File". A file named vuescan.log is generated at "C:\Documents and Settings\Dave\" (at least on my machine). VueScan keeps this file open for sole use so, although you can read it any time, you can't delete it or rename it without quitting out of VueScan.

Turning on logging in FilmGet requires the Registry Editor. In HKEY_LOCAL_MACHINE\SOFTWARE\Canon\FilmGet FS 1.0, create a new REG_SZ value named "Log". Create another named "LogScsiData". For the most detail (I rarely use this since it logs image data too), create another named "LogScsiRead". The data for these values doesn't matter. They work as long as they just exist. When these exist and FilmGet is started, a file named "FilmGet.log" is created at "C:/Documents and Settings/dave/Local Settings/Temp" (at least on my machine). Unlike VueScan, FilmGet opens and closes this file every time it writes so it is easy to move, delete, rename, etc. while FilmGet is running.

## Useful Links

[SCSI-2 draft standard document](http://www.t10.org/ftp/t10/drafts/s2/s2-r10l.pdf) - I used this a lot to learn fundamentals and understand the standard scanner commands, especially chapters 7, 8, & 15.

[SANE](http://www.sane-project.org/) - a project to create open-source scanner drivers/back-ends for Linux variants. The mailing list is useful to subscribe to. Most popular scanners are supported but not the FS4000\. See their [info page for the Canon FS4000](http://www.sane-project.org/unsupported/canon-fs4000.html). Note that Eric Bachard is referenced on this page as being interested in this scanner but I've contacted him and he no longer is.

[Home page for SANE back-end for Canon scanners](http://www.rzg.mpg.de/~mpd/sane/) - a little dated and seems inactive but email contact for the FS2710 scanner (closest match to the FS4000) was valid and Ulrich was helpful. I gleaned some useful tips for the FS4000 from his code for the FS2710 back-end (downloadable from this page).

[ASPI Spy utility](http://www.cdrlabs.com/articles/index.php?articleid=4&page=1) - not used but I saved this link in case I ever felt that I needed to know exactly how another app used ASPI.

[PLScsi](http://members.aol.com/plscsi/) - code available here to control the Windows SCSI driver directly without ASPI by calling DeviceIOControl. I never used it but saved this in case ASPI didn't work out.

[Raw Digital Photo Decoding in Linux](http://www.cybercom.net/~dcoffin/dcraw/) - home of dcraw, Dave Coffin's code for decoding camera RAW formats. I thought I might need this before I understood that scanner sensor data is not raw in that sense.

[LittleCms, Great color at small footprint](http://www.littlecms.com/) - an open-source CMS project. I currently plan on using this to apply tonal curves to scanned data rather than guessing at the correct gamma value.

[Gamma Correction](http://research.microsoft.com/~hollasch/cgindex/color/gamma.html) - nice backgrounder for understanding gamma.

[CGSD - Gamma Correction Home Page](http://www.cgsd.com/papers/gamma.html) - another nice backgrounder for understanding gamma.

[LibTIFF - TIFF Library and Utilities](http://www.libtiff.org/) - the standard open-source TIFF library. I incorporated this when I got sick of opening raw image data in Photoshop.

[TIFF 6.0 Specification](http://partners.adobe.com/asn/developer/PDFS/TN/TIFF6.pdf) - useful background for using libtiff, especially for the tag fields.

[Adaptec Driver Windows ASPI drivers version v4.71.2 Downloads Detail](http://www.adaptec.com/worldwide/support/driverdetail.jsp?sess=no&language=English+US&cat=/Product/ASPI-4.70&filekey=aspi_471a2.exe) - the version of the ASPI SDK that I used.

[Digital Light & Color](http://www.dl-c.com/Temp/) - makers of Profile Mechanic - Scanner which I've used with calibrated targets to generate profiles of my scanner's tonal curves.

[Cheap IT 8.7 (ISO 12641) Scanner Color Calibration Targets](http://www.targets.coloraid.de/) - Maker of the calibrated targets I used with Profile Mechanic (although I bought the targets through DL & C). I've also used these targets without Profile Mechanic to test exposure values and ranges.