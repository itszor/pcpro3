@if not %1k == k set lastasm=%1
tasm %lastasm%
tlink20 /m /t %lastasm%
del %lastasm%.obj
