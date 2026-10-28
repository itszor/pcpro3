# This makefile made by Makatic2



ObjectFiles	=		\
			o.header	\
			o.DivaCode	\
			o.divacodes	\


ProjectName		=	divacode



Librarys	=					\
			C:o.Stubs			\
			C:o.toolboxlib			\
			C:o.wimplib			\
			C:o.eventlibm			\
			DivaSrc:o.divaserial

Target		=	!DivaCode.DivaCode

ASMFlags	=	$(ASMExtra) -Stamp -NoCache -CloseExec -Quit -throwback
CCFlags		=	$(CCExtra) -fahi -zM -depend !Depend -throwback -I,C:
CPPFlags	=	$(CPPExtra) -depend !Depend -throwback -I,C:
LinkFlags	=	$(LinkExtra) -rmf
cmhgFlags       =       $(cmhgExtra)

ASM		=	objasm $(ASMFlags)
CC		=	cc -c $(CCFlags)
CPP		=	c++ -c $(CPPFlags)
LINK		=	Link -c++ $(LinkFlags)
CMHG            =       cmhg $(cmhgFlags)



# Don't alter anything below this...


.SUFFIXES:	.o .s .c .c++ .cmhg

$(Target):	$(ObjectFiles) $(Librarys)
	$(LINK) -o $@ $(ObjectFiles) $(Librarys)

.s.o:
	$(ASM) -from $< -to $@

.c.o:
	$(CC) -o $@ $<

.c++.o:
	$(CPP) -o $@ $<
	
# .cmhg.o: $(CMHG) $< $@	


# Dynamic dependencies:
o.DivaCode:	c.DivaCode
o.DivaCode:	C:h.toolbox
o.DivaCode:	C:h.kernel
o.DivaCode:	C:h.wimp
o.DivaCode:	C:h.event
o.DivaCode:	C:h.gadgets
o.DivaCode:	C:h.iconbar
o.DivaCode:	C:h.menu
o.DivaCode:	C:h.wimplib
o.DivaCode:	C:h.window
o.DivaCode:	C:h.swis
o.DivaCode:	DivaSrc:sys.h.DivaSerial
