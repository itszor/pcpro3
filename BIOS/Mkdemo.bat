@echo off
 
@REM  A bit of code that needs to be assembled in 32-bit mode, but tasm
@REM  won't change in mid-segment so we have to assemble it, then feed the
@REM  assembled instructions byte by byte as an include file in the main code.
tasm /t %1 bios32
tlink20 /x bios32
execonv bios32.exe bios32.bin
bin2asm bios32.bin bios32.inc
 
tasm /dPC_DEMO /t %1 bios
tlink20 /x bios
execonv bios.exe bios.bin
 
mkrom bios.bin romDEMO
 
del bios32.bin
del bios32.exe
del bios32.obj
del bios32.inc
del bios.bin
del bios.exe
del bios.obj
 
REM C:\drivers\3rdparty\armedit\putfile ADFS::4.$.PCs.!PC.ROM
REM copy rom e:\home\mattbee\rom
 
