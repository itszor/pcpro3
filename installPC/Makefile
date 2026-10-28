# This makefile made by Makatic2



ObjectFiles	=		\
			o.init	\
			o.installer	\
			o.installpc	\
			o.messages	\
			o.misc	\


ProjectName		=	installPC



Librarys	=					\
			C:o.Stubs			\
			C:o.toolboxlib			\
			C:o.wimplib			\
			C:o.eventlib			\
			DivaSrc:o.DivaSerial

Target		=	!InstallPC.!Runimage

ASMFlags	=	$(ASMExtra) -Stamp -NoCache -CloseExec -Quit -throwback
CCFlags		=	$(CCExtra) -fahi -g -depend !Depend -throwback -I,C:
CPPFlags	=	$(CPPExtra) -depend !Depend -throwback -I,C:
LinkFlags	=	$(LinkExtra)

ASM		=	objasm $(ASMFlags)
CC		=	cc -c $(CCFlags)
CPP		=	c++ -c $(CPPFlags)
LINK		=	Link -aif -c++ $(LinkFlags)



# Don't alter anything below this...


.SUFFIXES:	.o .s .c .c++

$(Target):	$(ObjectFiles) $(Librarys)
	$(LINK) -o $@ $(ObjectFiles) $(Librarys)

.s.o:
	$(ASM) -from $< -to $@

.c.o:
	$(CC) -o $@ $<

.c++.o:
	$(CPP) -o $@ $<


# Dynamic dependencies:
o.newconfig:	c.newconfig
o.newconfig:	C:h.wimplib
o.newconfig:	C:h.kernel
o.newconfig:	C:h.wimp
o.newconfig:	h.InstallPC
o.newconfig:	C:h.kernel
o.newconfig:	h.misc
o.init:	c.init
o.init:	C:h.wimplib
o.init:	C:h.kernel
o.init:	C:h.wimp
o.init:	h.init
o.init:	h.installer
o.init:	h.InstallPC
o.init:	C:h.kernel
o.init:	h.misc
o.init:	h.messages
o.init:	C:h.toolbox
o.installer:	c.installer
o.installer:	C:h.wimplib
o.installer:	C:h.kernel
o.installer:	C:h.wimp
o.installer:	h.init
o.installer:	h.InstallPC
o.installer:	C:h.kernel
o.installer:	h.installer
o.installer:	h.misc
o.installer:	h.messages
o.installer:	C:h.toolbox
o.installer:	h.installer
o.installpc:	c.installpc
o.installpc:	C:h.toolbox
o.installpc:	C:h.kernel
o.installpc:	C:h.wimp
o.installpc:	C:h.event
o.installpc:	C:h.gadgets
o.installpc:	C:h.iconbar
o.installpc:	C:h.menu
o.installpc:	C:h.saveas
o.installpc:	C:h.window
o.installpc:	C:h.wimplib
o.installpc:	C:h.window
o.installpc:	h.init
o.installpc:	h.installer
o.installpc:	h.installpc
o.installpc:	C:h.kernel
o.installpc:	h.messages
o.installpc:	C:h.toolbox
o.installpc:	h.misc
o.installpc:	DivaSrc:sys.h.divaserial
