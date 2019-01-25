LIBTIFFDIR = "C:/Documents and Settings/Dave/My Documents/Visual Studio Projects/tiff-3.6.1-2-src/libtiff"

LINK = link.exe

CFLAGS = -nologo -Zi -MDd -I$(LIBTIFFDIR)
LFLAGS = -nologo -debug -incremental:no kernel32.lib

all: fs4000test.exe xlate-fglog.exe avgscans.exe

fs4000test.exe: fs4000-scsi.obj aspi_wrappers.obj fs4000test.obj
    $(LINK) $(LFLAGS) -out:$@ $** $(LIBTIFFDIR)/libtiff.lib

xlate-fglog.exe: fs4000-scsi.obj aspi_wrappers.obj getopt.obj xlate-fglog.obj
    $(LINK) $(LFLAGS) -out:$@ $** 

avgscans.exe: avgscans.obj
    $(LINK) $(LFLAGS) -out:$@ $**

clean:
    del *.obj *.exe *.pdb *.ilk

fs4000test.obj fs4000-scsi.obj : fs4000-scsi.h scsi_wrappers.h
xlat-fglog.obj : fs4000-scsi.h
