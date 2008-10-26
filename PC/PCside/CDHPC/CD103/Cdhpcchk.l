-machine:i386 
-align:0x200 
-subsystem:native 
-debug:partial
-base:0x10000 
-entry:DriverEntry@8 
-out:cdhpc.mpd
-map:cdhpc.map
cdhpc.obj d:\ddk\lib\i386\checked\scsiport.lib d:\ddk\lib\i386\checked\libc.lib cdhpc.res

