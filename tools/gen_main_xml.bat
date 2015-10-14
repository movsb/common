@echo off

::转换 main_orig.xml 到 main.xml
cd "%~dp0..\res"
..\tools\idxml.exe main_orig.xml resource.h main.xml

pause

