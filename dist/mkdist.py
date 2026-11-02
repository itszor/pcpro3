#!/usr/bin/env python3
"""Make !PC's release zip: everything a Risc PC with a PC card needs.

    mkdist.py OUT.zip PC308.zip PC307.zip

PC308.zip is the 3.08 release ("PC (FR) 3.08"), whose !PC and !PCConfig
are the base, with this tree's builds in place of the files they replace.
PC307.zip is the 3.07 release ("PC (FR) 3.07"), the last with Aleph One's
installer, for what 3.08 took to be installed already: GuardDD and PCSleep
from its !Boot additions, the user guide, and the PC-side drivers from its
boot disc, put on a floppy image of their own without the disc's DOS.
Run from the top of the tree, after make all and make pcconfig.
"""

import os
import shutil
import subprocess
import sys
import tempfile
import time
import zipfile

from mkzip import RiscOSZip, open_acorn_zip

BIN = "build/bin"

# This tree's files, in place of the 3.08 release's: (name, type, source)
OURS = [
    ("ReadMe", 0xFFF, "dist/ReadMe"),
    ("!PC/!RunImage", 0xFF8, f"{BIN}/!RunImageG"),
    ("!PC/PCBits/Core/PCMem", 0xFFA, f"{BIN}/PCMEM"),
    ("!PC/PCBits/Netlinks/PCNAT", 0xFFA, f"{BIN}/PCNAT"),
    ("!PC/PCBits/Netlinks/PCNE2", 0xFFA, "PC/Bin/PCNE2"),
    ("!PC/PCBits/Netlinks/Startup", 0xFEB, "PC/PCBits/Netlinks/Startup"),
    ("!PC/PCBits/Netlinks/Shutdown", 0xFEB, "PC/PCBits/Netlinks/Shutdown"),
    ("!PCConfig/!RunImage", 0xFF8, "build/pcconfig/bin/!RunImage"),
    ("!PCConfig/Res", 0xFAE, "PCconfig/!PCConfig/Res"),
]

# From the 3.07 installer's Boot archive, into Boot/ for merging into !Boot
BOOT_EXTRAS = ("Choices/Boot/PreDesk/!GuardDD/", "Choices/Boot/PreDesk/PCSleep/")

# Aleph One's drivers from the 3.07 boot disc.  Left out: DR-DOS (IBMBIO,
# IBMDOS, COMMAND, HIMEM, XCOPY, ATTRIB, CHOICE, NWCDEX), Microsoft's
# KEYB622, ukkeyb, awk and VBETEST, and A. Walter's ASPI drivers, none of
# them Aleph One's; and SETUP.BAT and the AUTOEXEC/CONFIG that need them.
DRIVERS = [
    "ReadMe.txt",
    "PCP3_Drv/cdrom/HPC_CD.SYS", "PCP3_Drv/cdrom/readme.txt",
    "PCP3_Drv/mouse/AMOUSE.COM", "PCP3_Drv/mouse/readme.txt",
    "Win3Vid/ARMDRV.DRV", "Win3Vid/oemsetup.inf",
    "win95/cdrom/CDHPC.MPD", "win95/cdrom/cdhpc.inf", "win95/cdrom/readme.txt",
    "win95/video/ARMDRV.DRV", "win95/video/ArmDX.drv", "win95/video/armdrv.inf",
    "WinSound/SBE16AUX.DRV", "WinSound/SBE16SND.DRV", "WinSound/VSBPD.386",
    "WinSound/sbe.inf", "WinSound/sndblst.ini",
    "utils/quit.com", "utils/multit.com", "utils/singlet.com",
    "utils/synctime.exe", "utils/readme.txt",
]


def find(zf, suffix):
    for i in zf.infolist():
        if i.filename.endswith(suffix):
            return i
    sys.exit(f"{zf.filename}: no {suffix}")


def driver_image(bootdisc):
    """A blank 1.44MB DOS floppy holding DRIVERS from the boot disc image."""
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "System")
        img = os.path.join(tmp, "PCDrivers")
        with open(src, "wb") as f:
            f.write(bootdisc)
        run = lambda *a: subprocess.run(a, check=True, capture_output=True)
        run("mformat", "-C", "-i", img, "-f", "1440", "-v", "PC_DRIVERS", "::")
        made = set()
        for name in DRIVERS:
            parts = name.split("/")
            for i in range(1, len(parts)):
                d = "/".join(parts[:i])
                if d not in made:
                    made.add(d)
                    run("mmd", "-i", img, "::" + d)
            out = os.path.join(tmp, "f")
            run("mcopy", "-n", "-o", "-m", "-i", src, "::" + name, out)
            run("mcopy", "-o", "-m", "-i", img, out, "::" + name)
            os.unlink(out)
        with open(img, "rb") as f:
            return f.read()


def main(argv):
    if len(argv) != 4:
        sys.exit(__doc__.strip())
    out, pc308, pc307 = argv[1:]
    for tool in ("mformat", "mcopy", "mmd"):
        if not shutil.which(tool):
            sys.exit(f"mkdist: needs {tool} (mtools)")

    z = RiscOSZip(out)
    ours = {name for name, _, _ in OURS}
    for name, ftype, source in OURS:
        with open(source, "rb") as f:
            z.add(name, ftype, f.read(), os.stat(source).st_mtime)

    r308 = zipfile.ZipFile(pc308)
    for i in r308.infolist():
        top = i.filename.split("/")[0]
        if i.is_dir() or top not in ("!PC", "!PCConfig") or i.filename in ours:
            continue
        z.copy(i.filename, r308, i)

    r307 = zipfile.ZipFile(pc307)
    boot = open_acorn_zip(r307.read(find(r307, "!InstallPC/Resources/Boot")))
    for i in boot.infolist():
        if not i.is_dir() and i.filename.startswith(BOOT_EXTRAS):
            z.copy("Boot/" + i.filename, boot, i)
    # The user guide is a zip of its HTML and pictures: unzipped, to read
    guide = open_acorn_zip(r307.read(find(r307, "Release/UserGuide")))
    for i in guide.infolist():
        if not i.is_dir():
            z.copy("Docs/UserGuide/" + i.filename, guide, i)

    software = open_acorn_zip(r307.read(find(r307, "!InstallPC/Resources/Software")))
    system = find(software, "!MkBootDsc/System")
    z.add("PCDrivers", 0xFC8, driver_image(software.read(system)), time.time())
    z.close()


if __name__ == "__main__":
    main(sys.argv)
