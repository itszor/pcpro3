# Cross-build of !PC's ARM side on a host, with Acorn C/C++ 5's tools run
# under riscose.  See BUILDING.md.
#
#   make RISCOSE=/path/to/riscose ACORN_C=/path/to/acornc5
#
# RISCOSE  the riscose binary (default: riscose, on the PATH)
# ACORN_C  a directory holding Acorn C/C++ 5's Library (cc, link, objasm,
#          cmhg) and Libraries (clib, tboxlibs) directories
#
# Builds into build/bin: !RunImageG, !RunImageD (the demo), and the modules
# PCSupport, PC_IO, PCNE2, PCMEM and PCDevHelp.  PC/Bin, the release's
# binaries, is left alone.  PC/MakeFile is the original, for amu on RISC OS.

RISCOSE ?= riscose
ACORN_C ?=
WIMPSLOT ?= 4194304

# A leading ~ is expanded here, as not every shell does so after the = of
# make's VAR=value arguments (zsh doesn't).  The tools run in $(SRC), so a
# path is made absolute; a bare RISCOSE is looked for on the PATH.
home      = $(patsubst ~/%,$(HOME)/%,$(patsubst ~,$(HOME),$(1)))
ACORN_DIR := $(abspath $(call home,$(ACORN_C)))
RISCOSE_BIN := $(if $(findstring /,$(RISCOSE)),$(abspath $(call home,$(RISCOSE))),$(RISCOSE))

ifneq ($(MAKECMDGOALS),clean)
ifeq ($(strip $(ACORN_C)),)
$(error Set ACORN_C to the directory holding Acorn C/C++ 5's Library and Libraries)
endif
ifeq ($(wildcard $(ACORN_DIR)/Library/cc),)
$(error ACORN_C: no Library/cc in $(ACORN_DIR))
endif
endif

SRC   := PC
BUILD := build
OBJ   := $(BUILD)/o
BIN   := $(BUILD)/bin

# The tools run in $(SRC), with RISC OS names relative to it ("sys.h.sys",
# "^.build.o.main"), as the sources expect; riscose matches names to the
# host's files ignoring case.  Library directories are named through
# build/AcornC, since a RISC OS tool can't be given a host's absolute path
# (cc appends ".h.stdio" to an include directory).
ro      = $(subst /,.,$(1))
FROMSRC = ^.$(call ro,$(1))
RUN     = cd $(SRC) && $(RISCOSE_BIN) -w $(WIMPSLOT)
TOOL    = $(ACORN_DIR)/Library

LIBS    := ^.$(BUILD).AcornC.Libraries
CLIB    := $(LIBS).clib
TBOXLIB := $(LIBS).tboxlibs
INCLUDE := -I@,$(CLIB),$(TBOXLIB)
STUBS   := $(CLIB).o.Stubs

# The original mc and mcm's flags (-throwback aside: there's no desktop to
# throw back to).  mc defines PC_<variant>, PC_ for the normal build.
CFLAGS  := -g -c -fah $(INCLUDE)
MFLAGS  := -c -zM -fah $(INCLUDE)

# Two files pass -1 for a pointer, which this compiler rejects (-Ec allows).
CFLAGS_palette := -Ec
CFLAGS_wfebits := -Ec

.DELETE_ON_ERROR:
# build/AcornC is remade each time, in case ACORN_C has changed; it's only
# ever an order-only prerequisite, so it doesn't make anything rebuild.
.PHONY: all clean $(BUILD)/AcornC

all: $(BIN)/!RunImageG $(BIN)/!RunImageD \
     $(BIN)/PCSupport $(BIN)/PC_IO $(BIN)/PCNE2 $(BIN)/PCMEM $(BIN)/PCDevHelp

$(BUILD)/AcornC: | $(BUILD)
	ln -sfn $(ACORN_DIR) $@

$(BUILD) $(OBJ) $(BIN):
	mkdir -p $@

# ---- objects ------------------------------------------------------------

# $(call c_object,object,source,variant)
define c_object
$(OBJ)/$(1): $(SRC)/$(2) | $(OBJ) $(BUILD)/AcornC
	$$(RUN) $$(TOOL)/cc $$(CFLAGS) $$(CFLAGS_$$(notdir $(2))) -DPC_$(3) \
	    $(call ro,$(2)) -o $$(call FROMSRC,$$@)
endef

# $(call s_object,object,source)
define s_object
$(OBJ)/$(1): $(SRC)/$(2) | $(OBJ)
	$$(RUN) $$(TOOL)/objasm -from $(call ro,$(2)) -to $$(call FROMSRC,$$@) -quit
endef

C_SOURCES := \
    main:SYS/c/main          veneers:SYS/c/Veneers    sys:SYS/c/Sys \
    config:SYS/c/Config      WFEbits:SYS/c/wfebits    ext:SYS/c/Ext \
    cpu_g:CPU/c/cpu_g        hpc:CPU/c/HPC            dma:CPU/c/DMA \
    ports:VID/c/ports        scroll:VID/c/Scroll      modes:VID/c/modes \
    windrv:VID/c/Windrv      winblt:VID/c/Winblt      tables:VID/c/Tables \
    cursor:VID/c/Cursor      palette:VID/c/palette    mouse:VID/c/Mouse \
    rtc:DEV/c/RTC            hdd_hpc:DEV/c/HDD_HPC    ideem:DEV/c/IDEEm \
    fdd_hpc:DEV/c/Fdd_hpc    fddem:DEV/c/FddEm        kbd:DEV/c/Kbd \
    printer:DEV/c/Printer    ne2000:DEV/c/NE2000      cd_hpc:DEV/c/CD_HPC \
    multiio:DEV/c/MultiIO

DEMO_SOURCES := cpu_gDEMO:CPU/c/cpu_g windrvDEMO:VID/c/Windrv

S_SOURCES := \
    cpus_g:CPU/s/Cpus_g      vgas:VID/s/Vgas          vids:VID/s/Vids \
    wins:VID/s/Wins          veneerss:SYS/s/VeneersS  syss:SYS/s/SYSs \
    wfes:SYS/s/WFEs          cd_hpcs:DEV/s/CD_HPCs    turbolut:DEV/s/TurboLUT

pair = $(subst :, ,$(1))
$(foreach p,$(C_SOURCES),$(eval $(call c_object,$(word 1,$(call pair,$(p))),$(word 2,$(call pair,$(p))),)))
$(foreach p,$(DEMO_SOURCES),$(eval $(call c_object,$(word 1,$(call pair,$(p))),$(word 2,$(call pair,$(p))),DEMO)))
$(foreach p,$(S_SOURCES),$(eval $(call s_object,$(word 1,$(call pair,$(p))),$(word 2,$(call pair,$(p))))))

# ---- !RunImage ----------------------------------------------------------

# In LinkFileG's order (LinkFileD's for the demo, with the DEMO objects).
RUNIMAGE_G := main veneers veneerss sys cpu_g cpus_g vgas ports vids hpc \
    rtc scroll modes windrv winblt wins tables cursor palette hdd_hpc \
    ideem fdd_hpc fddem kbd mouse config syss WFEbits wfes printer ne2000 \
    cd_hpc cd_hpcs multiio dma turbolut ext
RUNIMAGE_D := $(subst windrv,windrvDEMO,$(subst cpu_g,cpu_gDEMO,$(RUNIMAGE_G)))
RUNIMAGE_LIBS := $(STUBS) $(TBOXLIB).o.eventlib $(TBOXLIB).o.toolboxlib \
    $(TBOXLIB).o.wimplib

# Linked through a via file, as lnk does, with a link map beside it.
# $(call runimage,variant)
define runimage
$(BIN)/!RunImage$(1): $(addprefix $(OBJ)/,$(RUNIMAGE_$(1))) | $(BIN) $(BUILD)/AcornC
	printf '%s\n' $$(foreach o,$$(RUNIMAGE_$(1)),^.$(BUILD).o.$$(o)) \
	    $$(RUNIMAGE_LIBS) > $(BUILD)/LinkFile$(1)
	$$(RUN) $$(TOOL)/link -o $$(call FROMSRC,$$@) -via ^.$(BUILD).LinkFile$(1) \
	    -LIST ^.$(BUILD).linkMap$(1) -MAP -XREF
endef
$(eval $(call runimage,G))
$(eval $(call runimage,D))

# ---- modules ------------------------------------------------------------

# PCSupport and PC_IO: assembled and linked as a binary at 0, as msm does.
$(OBJ)/PCSupport: $(SRC)/Module/s/PCSupport | $(OBJ)
$(OBJ)/PC_IO: $(SRC)/Module/s/PC_IO | $(OBJ)
$(OBJ)/PCSupport $(OBJ)/PC_IO:
	$(RUN) $(TOOL)/objasm -from Module.s.$(notdir $@) -to $(call FROMSRC,$@) \
	    -stamp -quit

$(BIN)/PCSupport $(BIN)/PC_IO: $(BIN)/%: $(OBJ)/% | $(BIN)
	$(RUN) $(TOOL)/link -bin -base 0 -o $(call FROMSRC,$@) $(call FROMSRC,$<)

# PCNE2, PCMEM and PCDevHelp: C modules, with cmhg headers.
$(OBJ)/%_MH: $(SRC)/Module/mh/%_MH | $(OBJ)
	$(RUN) $(TOOL)/cmhg Module.mh.$*_MH -o $(call FROMSRC,$@)

$(OBJ)/PCNE2 $(OBJ)/PCMEM $(OBJ)/PCDevHelp: | $(OBJ) $(BUILD)/AcornC
$(OBJ)/PCNE2: $(SRC)/Module/c/PCNE2
$(OBJ)/PCMEM: $(SRC)/Module/c/PCMEM
$(OBJ)/PCDevHelp: $(SRC)/Module/c/PCDevHelp
$(OBJ)/PCNE2 $(OBJ)/PCMEM $(OBJ)/PCDevHelp:
	$(RUN) $(TOOL)/cc $(MFLAGS) $(call ro,$(patsubst $(SRC)/%,%,$<)) \
	    -o $(call FROMSRC,$@)

$(OBJ)/PCMems: $(SRC)/Module/s/PCMemS | $(OBJ)
	$(RUN) $(TOOL)/objasm -from Module.s.PCMemS -to $(call FROMSRC,$@) -stamp -quit

$(BIN)/PCNE2: $(OBJ)/PCNE2_MH $(OBJ)/PCNE2
$(BIN)/PCMEM: $(OBJ)/PCMEM_MH $(OBJ)/PCMEM $(OBJ)/PCMems
$(BIN)/PCDevHelp: $(OBJ)/PCDVH_MH $(OBJ)/PCDevHelp
$(BIN)/PCNE2 $(BIN)/PCMEM $(BIN)/PCDevHelp: | $(BIN) $(BUILD)/AcornC
	$(RUN) $(TOOL)/link -m -o $(call FROMSRC,$@) \
	    $(foreach o,$^,$(call FROMSRC,$(o))) $(STUBS)

clean:
	rm -rf $(BUILD)
