@if not %1k == k set lastasy=%1
tasm %lastasy%
tlink20 /m /t %lastasy%, %lastasy%.sys
del %lastasy%.obj
