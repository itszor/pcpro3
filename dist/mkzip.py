#!/usr/bin/env python3
"""Zip files up for RISC OS, with their filetypes.

    mkzip.py OUT.zip NAME:TYPE:SOURCE ...

Each NAME is the file's RISC OS path in the archive, with / between the
parts ("!PC/PCBits/Core/PCMem"), TYPE its filetype in hex ("FFA") and
SOURCE the host file it's made from.  The type goes in Info-ZIP's Acorn
extra field ("ARC0"), which SparkFS and RISC OS's unzip read, with the
load and exec addresses of a typed file stamped with SOURCE's date.
"""

import os
import struct
import sys
import time
import zipfile

# Seconds from 1900-01-01, RISC OS's epoch, to 1970-01-01, the host's
EPOCH_1900 = 2208988800

ACORN_ID = 0x4341       # "AC"
ATTR_WR_R = 0x13        # owner read and write, public read
MADE_BY_ACORN = 13


def acorn_extra(filetype, mtime):
    cs = int((mtime + EPOCH_1900) * 100)
    load = 0xFFF00000 | (filetype << 8) | ((cs >> 32) & 0xFF)
    exec_ = cs & 0xFFFFFFFF
    return struct.pack("<HH4sIIII", ACORN_ID, 20, b"ARC0",
                       load, exec_, ATTR_WR_R, 0)


def main(argv):
    if len(argv) < 3:
        sys.exit(__doc__.strip())
    out, specs = argv[1], argv[2:]

    dirs = set()
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
        for spec in specs:
            name, ftype, source = spec.split(":", 2)
            # Each directory goes in before what's in it
            parts = name.split("/")[:-1]
            for i in range(1, len(parts) + 1):
                d = "/".join(parts[:i]) + "/"
                if d not in dirs:
                    dirs.add(d)
                    info = zipfile.ZipInfo(d, time.localtime()[:6])
                    info.create_system = MADE_BY_ACORN
                    info.external_attr = 0x10       # MS-DOS directory bit
                    z.writestr(info, b"")

            with open(source, "rb") as f:
                data = f.read()
            mtime = os.stat(source).st_mtime
            info = zipfile.ZipInfo(name, time.localtime(mtime)[:6])
            info.create_system = MADE_BY_ACORN
            info.external_attr = ATTR_WR_R
            info.compress_type = zipfile.ZIP_DEFLATED
            info.extra = acorn_extra(int(ftype, 16), mtime)
            z.writestr(info, data)


if __name__ == "__main__":
    main(sys.argv)
