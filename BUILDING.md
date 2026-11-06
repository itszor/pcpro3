# Building !PC on a host

`PC/MakeFile` builds !PC with amu on RISC OS, as it always has. The
`Makefile` here builds the ARM side of !PC 3.08 on another machine instead,
running Acorn's own tools under [riscose](https://github.com/itszor/riscose):

    make RISCOSE=/path/to/riscose ACORN_C=/path/to/acornc5

- `RISCOSE`: the riscose binary (default `riscose`, on the `PATH`). It needs
  the fixes on the `acorn-c-toolchain` branch of
  [itszor/riscose](https://github.com/itszor/riscose), which is jmb202's
  riscose with them added (an FPA emulator, unaligned loads, the Shared C
  Library's stdio, `_swix` and so on, and names looked up ignoring case).
  See *Building riscose*, below.
- `ACORN_C`: a directory holding Acorn C/C++ 5's tools and libraries, laid out
  as they are installed, with no `,xxx` filetype suffixes:

      Library/cc, link, objasm, cmhg     (cc 5.09, link 5.08: the C++ module
                                          update's; 5.0x's others)
      Libraries/clib/h/..., clib/o/Stubs (the update's Stubs)
      Libraries/tboxlibs/h/..., tboxlibs/o/eventlib, toolboxlib, wimplib

Both may be set in the environment instead.  Everything is built in `build/`:

| `build/bin/` | from |
|---|---|
| `!RunImageG` | the application (`PC/LinkFileG`'s objects) |
| `!RunImageD` | the demo version (`PC/LinkFileD`'s) |
| `PCSupport`, `PC_IO` | `PC/Module/s`, assembled with objasm and linked as a binary at 0 |
| `PCNE2`, `PCMEM`, `PCDevHelp` | `PC/Module/c` and their cmhg headers in `PC/Module/mh` |
| `PCNAT` | `PC/Module/c/PCNAT`, `PC/Module/s/PCNATs` and slirp in `PC/Module/slirp` (new: see *PCNAT*, below) |

with the link maps (`build/linkMapG`, `linkMapD`) beside them.  The files
have no filetypes: `!RunImage*` are Absolute (`&FF8`), the rest Modules
(`&FFA`).  `PC/Bin`, the release's modules, is left alone, so the two can
be compared; 3.08's `!RunImage` is in its binary release, not here.  The x86
side (the PC's BIOS, `PC/Bin/ROM`, and its drivers) and `PC/Bin/Messages`
aren't built.

`make pcconfig`, with the same `RISCOSE` and `ACORN_C`, builds !PCConfig's
`!RunImage` from `PCconfig` into `build/pcconfig/bin`.  It isn't part of the
default target: !PCConfig is an application of its own, installed beside !PC
with the `Res`, `Messages` and sprites it already has.  The original linked
James Bye's file library as `C:o.files`; `PCconfig/c/File` is its source.

## The release

    make RISCOSE=... ACORN_C=... PC308=".../PC (FR) 3.08 ()().zip" \
        PC307=".../PC (FR) 3.07 ()().zip" all pcconfig dist

makes `build/dist/PC309.zip` (named for `PC/SYS/h/Version`), everything a
Risc PC with a PC card needs, with its RISC OS filetypes in Info-ZIP's
Acorn extra field for SparkFS or RISC OS's unzip to set.  `dist/mkdist.py`
puts it together from:

- the 3.08 release's !PC and !PCConfig, with their filetypes, but for the
  files built here: !PC's `!RunImage`, `PCBits.Core.PCMem` and
  `PCBits.Netlinks`, and !PCConfig's `!RunImage` and `Res`;
- the 3.07 release, the last with Aleph One's installer, for what 3.08
  takes to be installed already: GuardDD and PCSleep from its `!Boot`
  additions, into `Boot`; its user guide, unzipped into `Docs.UserGuide`;
  and Aleph One's PC-side drivers from its boot disc, on a DOS floppy image
  of their own (`PCDrivers`, made with mtools) without the disc's DR-DOS
  and others' files;
- `dist/ReadMe`.

`PCconfig/!PCConfig/Res` is the 3.08 release's, which differs from the
source release's, with "Middle button to PC" added to the Adv window.

## What differs from the 3.08 release

Built this way, the binaries do what the release's do, but aren't
byte-for-byte the same:

- `!RunImageG`: each of its 127 areas is the size it is in `PC/linkMap`, the
  release's own link map, but for the C library's stubs: the update's are
  newer than the release's, and 112 bytes longer.  Relocations aside, the
  code is the release's but for two words and the Toolbox event library.
  The words are one stack frame in Main, `SYS_UpdateSizeMapping`'s, 32
  bytes bigger here: one `WimpIcon`, so these tboxlibs' `wimp.h` evidently
  makes `WimpGetWindowInfoBlock` an icon longer than the release's did.
  The event library (`tbevent`) is the same size, but a different version.
- The modules and `!RunImage`s are about 3.5K shorter: this linker leaves
  their zero-initialised data out of the file.
- `PCSupport` and `PC_IO` differ from the release's only in equivalent
  encodings (`LDR rd, [rn]` pre-indexed, where AAsm's was post-indexed with
  `#0`; ADRL's two immediates in the other order), and padding to a word.
- The C modules' help strings carry the date they're built.

## Changes made for this

Each is a commit of its own on top of the release:

- PCSupport and PC_IO are assembled with objasm, not AAsm (which isn't to be
  had): an `AREA` takes the place of AAsm's `LEADR`, and `Hdr.Proc`'s `ENTRY`
  macro is called `Entry`, as objasm has an `ENTRY` directive.
- `fe_toolbox.h` defines two Toolbox show types that Acorn C/C++ 5's
  `toolbox.h` predates.
- Two files are compiled with `-Ec`, as they pass `-1` for a pointer, which
  this compiler won't allow otherwise.

## PCNAT

`PCNAT` is new: a network interface for the PC with no card behind it.  It is
a DCI4 driver, `pcnat0`, for PCNE2 to use in place of a real card.  Behind it
is slirp (QEMU's, as RPCEmu has it, ported to Norcroft and to the Internet
module's sockets), which gives the PC a private network behind RISC OS: DHCP
gives the PC 10.0.2.15, with the gateway at 10.0.2.2 and DNS at 10.0.2.3 (the
first of RISC OS's `Inet$Resolvers`), and each TCP connection or UDP flow the
PC makes becomes a socket of RISC OS's own.  RISC OS keeps its address and
its TCP/IP, which a PC sharing a card with it through PCNE2 can't do: both
would claim IP's frame types.  It needs Internet 5 and the Mbuf manager.

`PC/PCBits/Netlinks` is the `PCBits` extension that loads it: `Startup` loads
PCNAT, and then PCNE2 with `-F`, so that PCNE2 reads `NE2000Basic` from the
configuration (`-pTCPIP` for TCP/IP), and sets `Diva$SuggestedInterface` to
`pcnat0` unless something else has.  Install it as `!PC.PCBits.Netlinks`,
with PCNAT and `PC/Bin/PCNE2` beside the two Obey files.  `*PCNATInfo` says
what it's doing.  Its SWI chunk, `&5A4C0`, is a development one: it needs an
allocation before it's released.

The cmhg file has no list of service calls, as building the table under
riscose fails, so PCNAT's handler sees every service call.

## The NE2000's fast path

`!RunImage` from this branch gives the NE2000 a fast path for a PC driver
that knows of it (pccard-linux's apcnet): frames go through rings in memory
the PC and the ARM share, rather than the 8390's ports.  See `FASTPATH.md`,
which also records what was found about the Gemini card's shared memory, L2
cache and write FIFO, and what !PCConfig's shared memory settings do.

## Building riscose

    git clone -b acorn-c-toolchain https://github.com/itszor/riscose.git
    cd riscose
    autoreconf -if
    ./configure CFLAGS='-g -O2 -std=gnu17 '
    make

The binary is `src/riscose`.  It needs libgdbm's headers (Debian's
`libgdbm-dev`), ncurses and readline.  Three things to know:

- `-std=gnu17`: GCC's default is now C23, in which the ARMulator's
  `unsigned (*func)()` takes no arguments, and it won't compile.
- The space at the end of `CFLAGS` matters: `configure.ac` appends `-Wall`
  to it without one.
- Don't build with `make -j`: the generated OSLib glue in `src/osapi` is
  written through one temporary file.

Its own tests need the C locale (`LC_ALL=C make check`): `tests/armas`
writes bytes with gawk, which encodes them in a UTF-8 locale.
