@echo off
set CDROMpath=c:\ih\cdrom\
set CDROMtest=\\chewypc\chewydrvd\win95\system\iosubsys\
REM del %CDROMpath%cdhpc.pdb
REM del %CDROMpath%cdhpc.obj
REM del %CDROMpath%cdhpc.sym
REM del %CDROMpath%cdhpc.map
del %CDROMpath%cdhpc.mpd

call nmake

if exist %CDROMpath%cdhpc.mpd copy %CDROMpath%cdhpc.mpd %CDROMtest%cdhpc.mpd
