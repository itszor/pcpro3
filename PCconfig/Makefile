# This makefile made by Makatic2



ObjectFiles	=		\
			o.adv	\
			o.aspi	\
			o.booting	\
			o.Discs	\
			o.file \
			o.LoadConfig	\
			o.main	\
			o.memory	\
			o.multicfg	\
			o.network	\
			o.printing	\
			o.SaveConf	\
			o.Serial	\
			o.switching	\
			o.Utils	\
			o.Windows	\
			o.miscfile	\


ProjectName		=	PCconfig



Librarys	=					\
			C:o.Stubs			\
			C:o.toolboxlib			\
			C:o.wimplib			\
			C:o.eventlib
#			C:o.files
#			C:o.debugit

Target		=	<PC_Config$Dir>.!RunImage

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
o.memory:	c.memory
o.memory:	C:h.kernel
o.memory:	C:h.stdio
o.memory:	C:h.stdlib
o.memory:	C:h.toolbox
o.memory:	C:h.wimp
o.memory:	C:h.event
o.memory:	C:h.gadgets
o.memory:	C:h.swis
o.memory:	h.utils
o.memory:	C:h.kernel
o.memory:	C:h.toolbox
o.memory:	h.vars
o.memory:	C:h.toolbox
o.memory:	h.types
o.memory:	h.debugit
o.memory:	h.memory
o.memory:	h.miscfile
o.network:	c.network
o.network:	C:h.toolbox
o.network:	C:h.kernel
o.network:	C:h.wimp
o.network:	C:h.event
o.network:	C:h.gadgets
o.network:	h.utils
o.network:	C:h.kernel
o.network:	C:h.toolbox
o.network:	h.vars
o.network:	C:h.toolbox
o.network:	h.types
o.network:	h.network
o.printing:	c.printing
o.printing:	C:h.toolbox
o.printing:	C:h.kernel
o.printing:	C:h.wimp
o.printing:	C:h.event
o.printing:	C:h.gadgets
o.printing:	h.vars
o.printing:	C:h.toolbox
o.printing:	h.types
o.printing:	h.printing
o.printing:	C:h.swis
o.printing:	h.utils
o.printing:	C:h.kernel
o.printing:	C:h.toolbox
o.SaveConf:	c.SaveConf
o.SaveConf:	C:h.stdlib
o.SaveConf:	C:h.stdio
o.SaveConf:	C:h.string
o.SaveConf:	C:h.kernel
o.SaveConf:	C:h.toolbox
o.SaveConf:	C:h.wimp
o.SaveConf:	h.types
o.SaveConf:	h.vars
o.SaveConf:	C:h.toolbox
o.SaveConf:	h.types
o.SaveConf:	h.windows
o.SaveConf:	h.types
o.SaveConf:	h.saveconf
o.SaveConf:	h.file
o.SaveConf:	h.utils
o.SaveConf:	C:h.kernel
o.SaveConf:	C:h.toolbox
o.SaveConf:	C:h.window
o.SaveConf:	C:h.gadgets
o.SaveConf:	h.multicfg
o.SaveConf:	C:h.toolbox
o.SaveConf:	h.types
o.Serial:	c.Serial
o.Serial:	C:h.toolbox
o.Serial:	C:h.kernel
o.Serial:	C:h.wimp
o.Serial:	C:h.event
o.Serial:	C:h.gadgets
o.Serial:	h.utils
o.Serial:	C:h.kernel
o.Serial:	C:h.toolbox
o.Serial:	h.vars
o.Serial:	C:h.toolbox
o.Serial:	h.types
o.Serial:	h.serial
o.switching:	c.switching
o.switching:	C:h.toolbox
o.switching:	C:h.kernel
o.switching:	C:h.wimp
o.switching:	C:h.event
o.switching:	C:h.gadgets
o.switching:	h.vars
o.switching:	C:h.toolbox
o.switching:	h.types
o.switching:	h.switching
o.switching:	h.utils
o.switching:	C:h.kernel
o.switching:	C:h.toolbox
o.Utils:	c.Utils
o.Utils:	C:h.swis
o.Utils:	C:h.kernel
o.Utils:	C:h.kernel
o.Utils:	C:h.toolbox
o.Utils:	C:h.wimp
o.Utils:	C:h.gadgets
o.Utils:	C:h.stdarg
o.Utils:	C:h.wimplib
o.Utils:	h.vars
o.Utils:	C:h.toolbox
o.Utils:	h.types
o.Utils:	h.utils
o.Utils:	C:h.kernel
o.Utils:	C:h.toolbox
o.Utils:	C:h.window
o.Utils:	C:h.string
o.Windows:	c.Windows
o.Windows:	C:h.stdlib
o.Windows:	h.debugit
o.Windows:	C:h.toolbox
o.Windows:	C:h.kernel
o.Windows:	C:h.wimp
o.Windows:	C:h.event
o.Windows:	C:h.gadgets
o.Windows:	C:h.swis
o.Windows:	C:h.stdio
o.Windows:	C:h.string
o.Windows:	h.utils
o.Windows:	C:h.kernel
o.Windows:	C:h.toolbox
o.Windows:	h.vars
o.Windows:	C:h.toolbox
o.Windows:	h.types
o.Windows:	h.windows
o.Windows:	h.types
o.adv:	c.adv
o.adv:	C:h.toolbox
o.adv:	C:h.kernel
o.adv:	C:h.wimp
o.adv:	C:h.event
o.adv:	C:h.gadgets
o.adv:	h.utils
o.adv:	C:h.kernel
o.adv:	C:h.toolbox
o.adv:	h.vars
o.adv:	C:h.toolbox
o.adv:	h.types
o.adv:	h.adv
o.aspi:	c.aspi
o.aspi:	C:h.ctype
o.aspi:	C:h.toolbox
o.aspi:	C:h.kernel
o.aspi:	C:h.wimp
o.aspi:	C:h.event
o.aspi:	C:h.gadgets
o.aspi:	h.utils
o.aspi:	C:h.kernel
o.aspi:	C:h.toolbox
o.aspi:	h.vars
o.aspi:	C:h.toolbox
o.aspi:	h.types
o.aspi:	h.aspi
o.aspi:	h.miscfile
o.booting:	c.booting
o.booting:	C:h.toolbox
o.booting:	C:h.kernel
o.booting:	C:h.wimp
o.booting:	C:h.event
o.booting:	C:h.gadgets
o.booting:	h.vars
o.booting:	C:h.toolbox
o.booting:	h.types
o.booting:	h.switching
o.booting:	h.booting
o.booting:	h.utils
o.booting:	C:h.kernel
o.booting:	C:h.toolbox
o.Discs:	c.Discs
o.Discs:	C:h.stdio
o.Discs:	C:h.string
o.Discs:	C:h.toolbox
o.Discs:	C:h.kernel
o.Discs:	C:h.wimp
o.Discs:	C:h.event
o.Discs:	C:h.gadgets
o.Discs:	C:h.swis
o.Discs:	h.utils
o.Discs:	C:h.kernel
o.Discs:	C:h.toolbox
o.Discs:	C:h.window
o.Discs:	C:h.wimplib
o.Discs:	h.discs
o.Discs:	h.vars
o.Discs:	C:h.toolbox
o.Discs:	h.types
o.Discs:	h.miscfile
o.Discs:	h.debugit
o.LoadConfig:	c.LoadConfig
o.LoadConfig:	C:h.stdio
o.LoadConfig:	C:h.string
o.LoadConfig:	C:h.kernel
o.LoadConfig:	C:h.toolbox
o.LoadConfig:	C:h.wimp
o.LoadConfig:	h.types
o.LoadConfig:	h.vars
o.LoadConfig:	C:h.toolbox
o.LoadConfig:	h.types
o.LoadConfig:	h.windows
o.LoadConfig:	h.types
o.LoadConfig:	C:h.wimplib
o.LoadConfig:	h.file
o.LoadConfig:	h.utils
o.LoadConfig:	C:h.kernel
o.LoadConfig:	C:h.toolbox
o.LoadConfig:	h.debugit
o.LoadConfig:	C:h.ctype
o.LoadConfig:	h.loadconfig
o.LoadConfig:	C:h.gadgets
o.multicfg:	c.multicfg
o.multicfg:	C:h.dcs
o.multicfg:	C:h.kernel
o.multicfg:	C:h.toolbox
o.multicfg:	C:h.wimp
o.multicfg:	C:h.window
o.multicfg:	C:h.gadgets
o.multicfg:	C:h.toolbox
o.multicfg:	C:h.iconbar
o.multicfg:	C:h.gadgets
o.multicfg:	C:h.event
o.multicfg:	C:h.wimplib
o.multicfg:	h.miscfile
o.multicfg:	h.multicfg
o.multicfg:	C:h.toolbox
o.multicfg:	h.types
o.multicfg:	h.types
o.multicfg:	h.utils
o.multicfg:	C:h.kernel
o.multicfg:	C:h.toolbox
o.multicfg:	h.vars
o.multicfg:	C:h.toolbox
o.multicfg:	h.types
o.multicfg:	h.saveconf
o.multicfg:	h.loadconfig
o.file:	c.file
o.file:	C:h.kernel
o.file:	h.file
o.main:	c.main
o.main:	h.debugit
o.main:	C:h.wimp
o.main:	C:h.toolbox
o.main:	C:h.kernel
o.main:	C:h.gadgets
o.main:	C:h.event
o.main:	C:h.swis
o.main:	C:h.window
o.main:	C:h.menu
o.main:	C:h.wimplib
o.main:	C:h.dcs
o.main:	C:h.proginfo
o.main:	h.Discs
o.main:	h.Windows
o.main:	h.types
o.main:	h.Utils
o.main:	C:h.kernel
o.main:	C:h.toolbox
o.main:	h.Memory
o.main:	h.printing
o.main:	h.serial
o.main:	h.multicfg
o.main:	C:h.toolbox
o.main:	h.types
o.main:	h.network
o.main:	h.aspi
o.main:	h.switching
o.main:	h.booting
o.main:	h.saveconf
o.main:	h.loadconfig
o.main:	h.adv
o.main:	h.miscfile
o.main:	h.vars
o.main:	C:h.toolbox
o.main:	h.types
o.main:	h.types
