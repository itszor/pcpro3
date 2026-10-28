# Building !PC on a host

`PC/MakeFile` builds !PC with amu on RISC OS, as it always has. The
`Makefile` here builds the ARM side of !PC 3.08 on another machine instead,
running Acorn's own tools under [riscose](https://github.com/jmb202/riscose):

    make RISCOSE=/path/to/riscose ACORN_C=/path/to/acornc5

- `RISCOSE`: the riscose binary (default `riscose`, on the `PATH`). It needs
  the fixes on the `acorn-c-toolchain` branch of jmb202's riscose (an FPA
  emulator, unaligned loads, the Shared C Library's stdio, `_swix` and so
  on, and names looked up ignoring case).
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

with the link maps (`build/linkMapG`, `linkMapD`) beside them.  The files
have no filetypes: `!RunImage*` are Absolute (`&FF8`), the rest Modules
(`&FFA`).  `PC/Bin`, the release's modules, is left alone, so the two can
be compared; 3.08's `!RunImage` is in its binary release, not here.  The x86
side (the PC's BIOS, `PC/Bin/ROM`, and its drivers) and `PC/Bin/Messages`
aren't built.

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
