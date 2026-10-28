# This makefile made by Makatic2



ObjectFiles	=		\
			o.choosecfg	\
			o.misc	\


ProjectName		=	Choosecfg



Librarys	=					\
			C:o.Stubs			\
			C:o.toolboxlib			\
			C:o.wimplib			\
			C:o.eventlib

Target		=	<Diva$Dir>.!ChooseCfg.ChooseCfg

ASMFlags	=	$(ASMExtra) -Stamp -NoCache -CloseExec -Quit -throwback
CCFlags		=	$(CCExtra) -fahi -depend !Depend -throwback -I,C:
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
o.choosecfg:	c.choosecfg
o.choosecfg:	C:h.toolbox
o.choosecfg:	C:h.kernel
o.choosecfg:	C:h.wimp
o.choosecfg:	C:h.event
o.choosecfg:	C:h.gadgets
o.choosecfg:	C:h.iconbar
o.choosecfg:	C:h.menu
o.choosecfg:	C:h.wimplib
o.choosecfg:	C:h.window
o.choosecfg:	h.misc
