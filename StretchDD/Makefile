# This makefile made by Makatic2



ObjectFiles	=		\
			o.stretchdd	\
			o.stretchdds	\


ProjectName		=	StretchDD



Librarys	=					\
			C:o.Stubs			\
			C:o.toolboxlib			\
			C:o.eventlib			\
			C:o.wimplib

Target		=	!StretchDD.!Runimage

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
o.stretchdd:	c.stretchdd
o.stretchdd:	C:h.toolbox
o.stretchdd:	C:h.kernel
o.stretchdd:	C:h.wimp
o.stretchdd:	C:h.event
o.stretchdd:	C:h.gadgets
o.stretchdd:	C:h.iconbar
o.stretchdd:	C:h.menu
o.stretchdd:	C:h.wimplib
o.stretchdd:	C:h.window
o.stretchdd:	h.stretchdds
