---------------- This is Electric, Version 7.00 ----------------

This README file contains installation instructions for all 3 of the platforms
supported by Electric: UNIX, Macintosh, and Windows.

---------------- Requirements:

UNIX:
  Electric runs on most UNIX variants, including SunOS, Solaris, BSD, HPUX, AIX,
  and (of course) GNU/Linux.

Macintosh:
  Macintosh users must run System 7 or later. Electric compiles best under
  Metrowerks, although it has been built with MPW and THINK_C.

Windows:
  Electric runs under Windows 95/98/ME, NT 4.0, 2000, or XP.
  The system compiles with Visual C++ 5.0 or later.
  
---------------- Distribution:

The Electric distribution is a single file in UNIX "tar" format, GNU-zipped
(see http://www.gzip.org for more information).
> Macintosh users: You will have to locate a program that can read this
  file ("MacGzip", available at www.gzip.org, will unzip the file; MacTar,
  available at www.strout.net/macsoft/mactar, will extract the files).
  If you use some other method of extraction, beware of Macintosh line-feed
  conventions, which are different from those on other operating systems.
  You may need to set the "Convert Newlines" option in the "tar" program
  before extracting the files.  Also, if you use "Internet Config", check
  to be sure its "Change Newline" setting set.  
  To be sure that the extraction has worked properly, examine the file "cadrc",
  which is at the top level (just inside of the "Electric" directory).
  This file should have about 10 lines of text.  If the file appears as a
  single line, or if there are spurious unprintable characters at the start
  or end of each line, then the text conversion has been done incorrectly.
> Windows users: You can use "WinZip" to extract this file.  Make sure that
  the "TAR File Smart CR/LF Conversion" box is checked in the "Configuration..."
  dialog of the "Options" menu.

When extracted, there will be a directory called "electric-VERSION" (where
the VERSION is the particular version number).  This directory will have
four subdirectories: "src", "lib", "examples", and "html".

The "src" directory contains the source code.  It is hierarchically
organized by function.

The "lib" directory has library files (more on that later).

The "examples" directory has some sample files.

The "html" directory contains one subdirectory, "manual", which is
the user's manual in HTML format.  To see the document, point your
browser to the file "index.html" inside of the "manual" directory.

---------------- Building (UNIX):

To configure for your system, go into the top-level directory and type:
        ./configure
This will examine the system and create a file called "Makefile".
To build Electric you now only need to use type:
        make
This compiles Electric and creates the executable file "electric"
in the top level directory.
Type "./electric" to run the system.

On Solaris, when using the Forte C compiler, uncomment the line that starts with "FORTECFLAGS"
in "Makefile".

Electric has the option of using TrueType fonts.  If you want this, follow these instructions:
> Obtain Rainer Menzner's "T1Lib" here:
    ftp://sunsite.unc.edu/pub/Linux/libs/graphics 
  Once you have it, unpack it (it will create a directory called "T1-1.1.0"), go into that
  directory, type "./configure" and then type "make".  You can then install with "make install".
> Now go to the Electric directory and edit the "Makefile".  Near the top are comments labeled
  "T1LIB TRUETYPE".  Uncomment the two lines (remove the "#" from the beginning of the lines)
  and change the lines so that they point into the T1Lib folder that you have just installed.
  For example, if the T1Lib libraries installed into "/usr/local/lib" and the T1Lib headers
  installed into "/usr/local/include", then you want these lines to read:
    TRUETYPE_LIBS = /usr/local/lib/libt1.a
    TRUETYPECFLAGS = -DTRUETYPE=1 -I/usr/local/include
> Next (this is the most painful part), you have to set the environment variable T1LIB_CONFIG
  to point to the file "t1lib.config".  This file may be installed in "/usr/local/share/t1lib".
  There will certainly be a copy in the "examples" folder of the distribution.  The catch here is that
  this file has relative path names in it which must be converted to absolute.  So, if you have
  extracted the T1Lib distribution into the folder "/home/strubin/T1-1.1.0", then the file should
  look like this:
    This is a configuration file for t1lib
    FONTDATABASE=/home/strubin/T1-1.1.0/examples/FontDataBase
    ENCODING=/home/strubin/T1-1.1.0/Fonts/enc:.
    AFM=/home/strubin/T1-1.1.0/Fonts/afm:.
    TYPE1=/home/strubin/T1-1.1.0/Fonts/type1:.
> Finally, rebuild Electric with the TrueType library.  When you run it, you will get a warning if
  any of the TrueType initialization fails (in which case it will revert to the non-TrueType code).
  Otherwise, you have it.
> Note that Electric uses the first font in the database by default.  To change the font that
  Electric uses, set the environment variable ELECTRIC_TRUETYPE_FONT to the desired font name.
  You can see a list of available fonts by setting this environment variable to an unknown name,
  in which case Electric will show all fonts in its error message.

To add Java, follow these instructions:
> Download the Java Development Kit (JDK) from http://java.sun.com.  Install it.
  For the purposes of these instructions, assume that it is installed into
        /usr/java/jdk
  If you install it elsewhere, adjust these instructions accordingly.
> After configuration, but before making Electric, edit the "Makefile" and
  uncomment the lines near the top that enable Java.
  Change the definition of LANGJAVA_DIR to point to the installed JDK location.
> On Solaris, add this string to the environment variable LD_LIBRARY_PATH:
        :/usr/java/jdk/jre/lib/sparc:/usr/java/jdk/jre/lib/sparc/classic:/usr/java/jdk/jre/lib/sparc/native_threads
> On GNU/Linux, add this string to the environment variable LD_LIBRARY_PATH:
        :/usr/java/jdk/jre/lib/i386:/usr/java/jdk/jre/lib/i386/classic:/usr/java/jdk/jre/lib/i386/native_threads
> Be sure to export "LD_LIBRARY_PATH" if your shell requires it.

To add the TCL interpreter, download ActiveTcl from "http://www.tcl.tk" and install it.
Then edit "Makefile" and you will find the instructions necessary to enable the interpreter.

---------------- Building (Macintosh):

For System 7, 8, and 9, there is a Metrowerks project (called "Electric.xml").
Run Metrowerks, import this file, and save it in the top level,
alongside the "src" directory.  Due to the size of the code that is being built,
you may have to increase the size of the Metrowerks partition.

For System 10, there are two ways to go: Qt or ProjectBuilder.  Qt is the only fully-working
solution, but unfortunately it is not free on the Macintosh (it is actually quite expensive).
Also, you need Qt release 3.1.0 or later.

To build with "Qt", use a terminal window and type "./configure" to generate a "Makefile".
Edit the "Makefile" and switch to Qt widgets (uncomment the Qt part, comment the Motif part,
and in the Qt section, change comments to switch to "Qt on Macintosh").

The alternative to Qt is ProjectBuilder (the files "Electric.pbproj" and "English.lproj" are included).
Note that the ProjectBuilder solution is not fully debugged, so use with care.

After compilation, you will have the application "Electric".  Double-click it to run the system.

To add the TCL interpreter, follow these instructions:
> Download ActiveTcl from "http://www.tcl.tk" and install it.
> If using Qt/System 10, edit "Makefile" and add TCL.  Otherwise:
  > In the compiler, add an include path to the installed TCL "include" directory.
  > Also in the compiler, add the appropriate TCL library to the project.
> Edit the appropriate "mac" include file in "src/include" (for example, "macsfsheaders.h")
  and uncomment the definition of "FORCETCL".
  
To add a Java interpreter (System 10 only) follow these instructions:
> Download Java from "http://java.sun.com" and install it.
> If using Qt, edit "Makefile" and add Java.  Otherwise:
  > In the compiler, add an include path to the installed Java "include" directory.
  > Also in the compiler, add the appropriate Java library to the project.
> Edit the appropriate "mac" include file in "src/include" (for example, "macsfsheaders.h")
  and uncomment the definition of "FORCEJAVA".

---------------- Building (Windows):

For users of Visual C++ 5.0 or 6.0, open the workspace file "Electric.dsw"
(both it and the associated file "Electric.dsp" are in the top level, alongside
the "src" directory).  Visual Studio .NET users can open "Electric.vcproj".
If you have trouble with any of these files, use the MAKE file
"Electric.mak".  Compile Electric.  This will create a new directory in the
top level called "Debug", which will contain all of the object files.

Inside of the "Debug" directory, you will find the executable file "Electric".
Move this file out of the "Debug" directory and place it in the top-level
directory.  Double-click the "Electric" executable to run the system.

To add the Java interpreter, follow these instructions:
> Download the Java Development Kit (JDK) from http://java.sun.com.  Install it.
  The standard location is "C:\Program Files\JavaSDK" (or whatever the version is), and this path
  will be used here.  If you install it elsewhere, adjust these instructions accordingly.
> Edit the environment variables in the "System" Control Panel.
  On some systems, you click on the "Environment" tab; on others, click on the "Advanced"
  tab and then click the "Environment Variables" button.
  Under "System variables", select "Path" and in the "Value:" area,
  add this string to the end:
        ;C:\Program Files\JavaSDK\jre\bin\classic;C:\Program Files\JavaSDK\bin
  On some newer versions of the Java Development Kit, you may also have to include this path:
        ;<CODE>;C:\Program Files\JavaSDK\bin\client
  On Windows 95 and Windows 98 systems, you may have to edit C:\AUTOEXE.BAT and append
  this to the PATH variable.  You must restart your computer after making this change.
> In Visual C++ 5.0 or 6.0, use the "Settings" command of the "Project" menu.  Select the
  "C/C++" tab and the "Preprocessor" category.  In the "Preprocessor definitions"
  area, add this to the end:
          ,FORCEJAVA=1
  In the "Additional include directories" area add this to the end:
          ,C:\Program Files\JavaSDK\include,C:\Program Files\JavaSDK\include\win32
  Select the "Link" tab and the "General" category.  In the "Object/library modules"
  area, enter this:
          jvm.lib
  Select the "Link" tab and the "Input" category.  In the "Additional library path"
  area, enter this:
          C:\Program Files\JavaSDK\lib
> In Visual Studio .NET, right-click on the "Electric" solution and choose "Properties".
  Select "C/C++" on the left and choose the "General" category under it.
  In the "Additional Include Directories" area, add this to the end:
          ;C:\Program Files\JavaSDK\include,C:\Program Files\JavaSDK\include\win32
  Next choose the "Preprocessor" category of "C/C++" and in the "Preprocessor Definitions"
  area add this to the end:
          ;FORCEJAVA=1
  Select "Linker" on the left and choose the "General" category under it.
  In the "Additional Library Directories" area, enter this:
          ;C:\Program Files\JavaSDK\lib
  Next choose the "Input" category of "Linker" and in the "Additional Dependencies" area
  enter this:
          jvm.lib
> Once Java is installed, you must compile the ROM generator.  In a command window, change
  directories to the "lib\java" directory and run the command:
    javac romgen.java

To add the TCL interpreter, follow these instructions:
> Download ActiveTcl from "http://www.tcl.tk" and install it.
> In the compiler, edit the Project Settings and find the field "Additional include directories"
  (under "C/C++").  Add a new path to the installed TCL Includes
  (typically "C:\Program Files\Tcl\include").
> Also in the compiler, edit the Project Settings and find the field "Additional library path"
  (under "Linker").  Add a new path to the installed TCL Libraries
  (typically "C:\Program Files\Tcl\lib").
> Edit the file "src/include/config.h" and make sure that the constant "TCLLIBDIR" points
  to the proper location of the initialization files ("init.tcl" and others).
  This is typically "C:\Program Files\Tcl\lib\tcl8.3"
  (note that each backslash is doubled in this file, and you should follow this convention).

---------------- Installing:

Once compiled, Electric will run properly from the directory where it was
built.  However, if you wish to install the system, you must move files
carefully.  This is because Electric makes use of a collection of "support
files".  The main support file is called "cadrc" (on UNIX, it has a dot in
front: ".cadrc").  In addition, Electric needs to find the "lib" and "html"
directories.  If these support files cannot be found, Electric will not be
able to initialize its graphical user interface (just type "-quit" to exit
the program if this happens).

On Windows and the Macintosh, it is sufficient to move the support files,
along with the executable, to a public location.  Then make an alias
(shortcut) to the executable and place that anywhere you like.  When the
alias is run, the directory with the executable will become the current
directory, and all of the needed support files will be found.

On UNIX, the "make install" command will place the executable and the support
files in a public location, but they may not be together.  For example, it is
not uncommon for the executable to be placed in "/usr/local/bin", but the
support files in "/usr/local/lib/electric".  When this happens, the executable
needs to know where the support files are located.  There are three ways to
do this:
 (1) You can set the ELECTRIC_LIBDIR environment variable to point to the
     location of the support files.
 (2) You can change the #define of "LIBDIR" in "src/include/config.h" to point
     to the location of the support files.
 (3) You can keep a local copy of ".cadrc" file (this file can be in your
     home directory or in the current directory).  Inside of the ".cadrc"
     file, change the "electric library default-path" command to point to the
     remaining support files (the "lib" and "html" directories).

UNIX systems also offer "make install.html" which installs the online manual
into a public place (typically "/usr/local/share/doc/electric/html").  Be sure
that the #define of "DOCDIR" in "src/include/config.h" agrees with this path
or else the "See Manual" command will not work.

---------- Additional Details (UNIX):

Electric uses "widget libraries" to control the windows on the display.
The default widget library is Motif (see http://www.opennc.org/openmotif).
You can use LessTif (see http://www.lesstif.org), but has some bugs (you will
have to remove the "XtDestroyWidget()" call in "DiaDoneDialog()").
You can also use Qt (see http://www.trolltech.com) by editing the "Makefile"
after running "configure" (comments near the top explain what to do).
When installing these packages on your system, be sure to get both the libraries
and the "devel" package that contains the compiler header files.  Also note that
many systems use shared libraries for these widget packages, and this may require
some additional steps when installing.  This is because the libraries get
installed in a place that the shared library  system doesn't know about.
If you have superuser access, you can use "ldconfig" to tell the system where
to find the libraries.  Otherwise, you can use the LD_LIBRARY_PATH environment
variable (on AIX use LIBPATH and on HP-UP use SHLIB_PATH).  This variable is
a colon-separated list of paths to be searched for shared libraries.  For example,
this setting will work on many systems:
   LD_LIBRARY_PATH = /usr/X11R6/lib/
   export LD_LIBRARY_PATH

Electric has two ways to control the display.  By default, the system runs
on any depth monitor, but is slow on older machines and must be run
locally (that is, the client and the server must be on the same computer).
The alternate method of display is faster and can run over the network,
but it can only support displays that are set to 8bpp (8 bits per pixel).
In addition, this alternate method will suffer from "colormap flashing"
when the cursor enters and leaves the Electric windows.  To switch to
this alternate method, edit the "Makefile" after running "configure"
(comments near the top explain what to do).  Note also that Motif and
Lesstif do not work well with this alternate display method, so you will
also have to switch to using the Athena widgets.

Electric makes use of external programs for simulation.  The location of
these programs can be found in the various #defines in the file
"src/include/config.h", which can be overridden with the following
variables in your ".cshrc" file:
   setenv ELECTRIC_SPICELOC   /usr/local/bin/spice
   setenv ELECTRIC_ESIMLOC    /usr/local/bin/esim
   setenv ELECTRIC_RSIMLOC    /usr/local/bin/rsim
   setenv ELECTRIC_PRESIMLOC  /usr/local/bin/presim
   setenv ELECTRIC_RNLLOC     /usr/local/bin/rnl

There are two command-line arguments that can be given which will control
the X-windows display.  If you use the "-f" option, Electric
will start with a full-screen graphics window.  If you use the
"-geom WxH+X+Y", it will set the graphics window to be "W" wide,
"H" high, and with its corner at (X, Y).  Additional X-Windows options
can be typed into the file ".Xdefaults".  The resources "Electric.font0"
through "Electric.font8" set the font to use for point sizes 4, 6, 8, 10,
12, 14, 16, 18, and 20.  The resource "Electric.fontmenu" controls the
text used in the component menu, and the resource "Electric.fontedit"
controls the text used in the text editor.  Here is a sample line from
the file:
   Electric.font5: -misc-fixed-medium-r-normal-*-*-140-*-*-*-*-*-*
To see what all of these fonts look like, load the library "samples.txt"
(with the "Readable Dump" subcommand of the "Import" command of the "File"
menu) and edit the cell "tech-Artwork".  The top part of the cell shows
text in sizes 4 through 20.

---------- Additional Details (Windows):

Electric must be run with the Display Settings set to "65536 Colors" or
"True Color".  Anything less will cause the colors to appear wrong.

If you have trouble reading the cursor or icon files (".cur" or ".ico") you can
find a text-encoded version of these binary files in \src\graph\graphpc.uue.
Use "WinZip" to extract the files into the same directory.

---------- Internationalizing Electric:

Electric is able to interact in other languages than English.
Currently, there is a French translation.

To use this facility on GNU/Linux, edit the "Makefile" and follow the instructions for
"Internationalization".  You must then set the environment variable "LANGUAGE" to the
proper language ("fr" for French).  On Solaris, you must also set the environment variable
"NLSPATH" to point to Electric's "lib/international" directory.

To use this facility on Windows or the Macintosh, you must obtain the Static Free Software
extras and build the "International" version of Electric.  Before compiling, set the
desired language by changing the routine "elanguage()" (on Windows, it is in
"graph/graphpccode.cpp"; on the Macintosh it is in "graph/graphmac.c").

At any time, you can disable the foreign language and return to English by moving the
translation files.  These files are in the "lib/international" folder, with a subfolder
that has the language name (for example, French translations are in "lib/international/fr").
Beneath that is a folder called "LC_MESSAGES" and inside of that are the translation files.


If you wish to translate Electric yourself, look in the appropriate folder for that
language.  There you will find two types files: the translation file and its binary equivalent.
The binary file is named "electric.mo" and the translation file ends in the language name
("electric.fr" for French).
Note also that the Macintosh uses different characters than UNIX and Windows, so there
will also be a second set of files ("macelectric.mo" and "macelectric.fr").

When repairing or augmenting a translation, edit the appropriate translation file.  It must
then be converted to/from Macintosh encoding, and then both files must be converted to the
binary format.

When creating a new translation, you can start with the file "lib/international/messages.po"
which has all of Electric's messages but no translations.  Rename this file appropriately
and edit it.


If you are using GNU/Linux, you can edit this file with EMACS and use "po-mode", which
assists in translation.  If you do not have access to this, you can use Electric's built-in
translation editor.  Use the "General Options" subcommand of the "User Interface" command of
the "Info" menu; click "Advanced" and then click "Language Translation".


If you edit the translation file by hand, you will see groups of lines that look like this:
	#: src/usr/usrcomtv.c:233
	#, c-format
	msgid "Technology %s is still in use"
	msgstr ""
The line that begins with "#:" is the location in the source code of the string.
The line that begins with "#," contains flags that are unimportant.
The line that begins with "msgid" is the string in English.
The line that begins with "msgstr" is an empty string, and it is where you type the translation.


In some cases, the first line is many lines (if the string appears in many places in the code).
For example:
	#: src/io/iocifin.c:208 src/io/iodefi.c:117 src/io/iodxf.c:487
	#: src/io/ioedifi.c:810 src/io/iogdsi.c:1955 src/io/iolefi.c:113
	#: src/io/iosdfi.c:157 src/io/iosuei.c:238 src/io/iotexti.c:375
	#: src/io/iovhdl.c:57
	#, c-format
	msgid "File %s not found"
	msgstr ""


Be aware that many of these strings have formatting control in them.  An example is shown above.
The string is "File %s not found", and the "%s" part of the string will be replaced with the
name of a file.  When you translate it, leave the "%s" so the substitution will work.
In French, that string will be "Fichier %s introuvable".

The common substitutions that you will find are "%s" for string and "%d" or "%ld" for numbers.
There might even be a substitution that has digits in it such as "%3d"  Be warned that the
order of the substitutions must be preserved, and so must the exact sequence of characters in
the substitution.  For example, the string:
    "Unknown section name (%s) at line %ld"
has two substitutions, and the translation must have the "%s" before the "%ld"
(even if the sentence is awkward).


At some point in the translation process, you may see the keyword "fuzzy" on the flags line
(the line that begins with "#,").  This indicates that some program guessed at the translation
because of similarity.  No human did a translation for this string, and it may be wrong.
Verify the translation and remove the word "fuzzy".


Some lines begin with "#~", for example:
	#~ msgid "File Format:"
	#~ msgstr "Format fichier"
This indicates messages that were translated but are no longer used in Electric.  The
translations are kept around in case the messages reappear in a later version of Electric.
None of the lines that begin with "#~" should require translation, so you can safely ignore them.


Some translations span two lines, for example:
	#: src/db/dbchange.c:1114
	#, c-format
	msgid "Warning: Could not find index %ld of attribute %s on object %s"
	msgstr ""
	" Attention: Impossible de trouver l'index %ld de l'attribut %s de l'objet %s"
This is standard format for the translation package.  It is permitted for the translation
to span more than 1 line in the file, but in all cases these strings are concatenated together
to form 1 string.


Sometimes the English contains the character "&" where it makes no sense (for example, the
word "P&rint").  These are markers for "quick keys" in pulldown menus.  When the "&" is in
front of the letter "X", it means that typing "X" will invoke that menu entry.  However, each
menu must have the "&" in front of a unique letter.  And since you cannot tell, when you are
translating, which strings are grouped into a single menu, it is best if you simply ignore the
"&" character and translate the string without it.  Note, however, that the character should
be preserved where it makes sense, such as in the expression "black&white".


An alternate "quick key" marker is the string "/X" at the end of a phrase.  This indicates that
the Control-X key will invoke that menu entry.  These letters are not part of the phrase, and
so they do not need to be translated (just left alone).  Ambitious translators will want to
rearrange these control-keys so that they make sense for the language.


Some phrases end with an elipsis ("...").  These are used in pulldown menu entries that
invoke dialogs.  Keep the elipses when translating.


Here are some terms that may be confusing:
	"jog" describes a wire that makes two bends.  Like a "Z", it turns in one direction
		and then back again.  Also called a "zig-zag" in English.
	"R-Tree" is a technical term...you can translate "tree" (as in "tree structure",
		something that branches), and leave the letter "R".
	"high" refers to a signal level (high voltage, low voltage).
	"trans" is an abbreviation for "transistor"
	"diff" is an abbreviation for "diffusion" (the active layer on an IC).
	"gate" is the polysilicon layer on a chip that "gates" (controls) the transistor.
	"deck" refers to a disk file with netlist information.
	"prim" is an abbreviation for "primitive", a type of node (the other type
		of node is the cell instance).
	"serpentine" describes a transistor with a complex gate path, winding its way through
		the active region.  Thus, it looks like a serpent.
	"arch" is an abbreviation for Architecture, and is used to describe FGPAs.
	"pip" is another FPGA term: it is a "program point" on the chip.
	"block" refers to a main section of a FPGA chip, similar to a "cell".
		The "block net" is a network in a block.  A "block net segment" is a part of that network.
	"well" refers to an implant layer on a chip (p-well and n-well are the two types).
	"negated arc" is one with a negating bubble on it.
	"reversed arc" is one where the head and tail are swapped.

---------------- Discussion:

There are two GNU mailing lists devoted to Electric:
    bug-gnu-electric (subscribe at http://mail.gnu.org/mailman/listinfo/bug-gnu-electric)
	discuss-gnu-electric (subscribe at http://mail.gnu.org/mailman/listinfo/discuss-gnu-electric)

In addition, you can send mail to:
	info@staticfreesoft.com


