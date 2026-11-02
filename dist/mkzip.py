"""Zip files up for RISC OS, with their filetypes.

Each file's RISC OS type goes in Info-ZIP's Acorn extra field ("ARC0"),
which SparkFS and RISC OS's unzip read, with the load and exec addresses
of a typed file stamped with its date.  Names have / between the parts
("!PC/PCBits/Core/PCMem").
"""

import io
import struct
import time
import zipfile

# Seconds from 1900-01-01, RISC OS's epoch, to 1970-01-01, the host's
EPOCH_1900 = 2208988800

ACORN_ID = 0x4341       # "AC"
ATTR_WR_R = 0x13        # owner read and write, public read
MADE_BY_ACORN = 13
# external_attr holds MS-DOS attributes in its low byte (where RISC OS's
# 0x13 would read as read-only, hidden and a directory) and Unix mode bits
# in its top half: a plain file, rw-------, as Info-ZIP on RISC OS writes
FILE_ATTR = 0o100600 << 16
DIR_ATTR = (0o040700 << 16) | 0x10      # rwx------, and MS-DOS's directory bit


def acorn_extra(filetype, mtime):
    cs = int((mtime + EPOCH_1900) * 100)
    load = 0xFFF00000 | (filetype << 8) | ((cs >> 32) & 0xFF)
    exec_ = cs & 0xFFFFFFFF
    return struct.pack("<HH4sIIII", ACORN_ID, 20, b"ARC0",
                       load, exec_, ATTR_WR_R, 0)


class RiscOSZip:
    """A zip being written, which puts each directory in before what's in
    it, and takes files either typed afresh or as another zip has them."""

    def __init__(self, path):
        self.zip = zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED)
        self.dirs = set()

    def _dirs_for(self, name):
        parts = name.split("/")[:-1]
        for i in range(1, len(parts) + 1):
            d = "/".join(parts[:i]) + "/"
            if d not in self.dirs:
                self.dirs.add(d)
                info = zipfile.ZipInfo(d, time.localtime()[:6])
                info.create_system = MADE_BY_ACORN
                info.external_attr = DIR_ATTR
                self.zip.writestr(info, b"")

    def add(self, name, filetype, data, mtime):
        """A file of the given type, dated mtime (host seconds)."""
        self._dirs_for(name)
        info = zipfile.ZipInfo(name, time.localtime(mtime)[:6])
        info.create_system = MADE_BY_ACORN
        info.external_attr = FILE_ATTR
        info.compress_type = zipfile.ZIP_DEFLATED
        info.extra = acorn_extra(filetype, mtime)
        self.zip.writestr(info, data)

    def copy(self, name, src_zip, src_info):
        """A file from another RISC OS zip, with its type and date."""
        self._dirs_for(name)
        info = zipfile.ZipInfo(name, src_info.date_time)
        info.create_system = MADE_BY_ACORN
        info.external_attr = src_info.external_attr
        info.compress_type = zipfile.ZIP_DEFLATED
        info.extra = acorn_field(src_info.extra)
        self.zip.writestr(info, src_zip.read(src_info))

    def close(self):
        self.zip.close()


def open_acorn_zip(data):
    """A ZipFile of data, an Acorn zip whose central directory may give its
    Acorn fields more length than their entries' extra data holds, which
    Python's zipfile won't read: each such length is cut to fit."""
    data = bytearray(data)
    i = data.find(b"PK\1\2")
    while i != -1:
        name_len, extra_len, comment_len = struct.unpack_from("<HHH", data, i + 28)
        x = i + 46 + name_len
        tp, ln = struct.unpack_from("<HH", data, x) if extra_len >= 4 else (0, 0)
        if tp == ACORN_ID and ln > extra_len - 4:
            struct.pack_into("<H", data, x + 2, extra_len - 4)
        i = data.find(b"PK\1\2", x + extra_len + comment_len)
    return zipfile.ZipFile(io.BytesIO(bytes(data)))


def acorn_field(extra):
    """The Acorn field of a zip entry's extra data, in the 20-byte form
    Info-ZIP writes."""
    i = extra.find(b"ARC0")
    if i < 4 or struct.unpack_from("<H", extra, i - 4)[0] != ACORN_ID:
        raise ValueError("no Acorn extra field")
    load, exec_, attr = struct.unpack_from("<III", extra, i + 4)
    return struct.pack("<HH4sIIII", ACORN_ID, 20, b"ARC0", load, exec_, attr, 0)
