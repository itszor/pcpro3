# This makefile made by Makatic2



ObjectFiles	=		\
			o.create	\
			o.frontend	\
			o.misc	\


ProjectName		=	MKDOSdisc



Librarys	=					\
			C:o.Stubs			\
			C:o.wimplib

Target		=	<PC_Config$Dir>.MkDOSDisc

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
o.frontend:	c.frontend
o.frontend:	C:h.kernel
o.frontend:	C:h.wimplib
o.frontend:	C:h.wimp
o.frontend:	h.misc
o.frontend:	h.create
o.create:	c.create
o.create:	C:h.kernel
o.create:	C:h.swis
o.create:	h.create
o.create:	h.frontend
