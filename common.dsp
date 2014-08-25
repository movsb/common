# Microsoft Developer Studio Project File - Name="common" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=common - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "common.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "common.mak" CFG="common - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "common - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "common - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "common - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "common___Win32_Release"
# PROP BASE Intermediate_Dir "common___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "output\Release_vc6"
# PROP Intermediate_Dir "output\Release_vc6"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "C:\Users\YangTao\Desktop\SdkLayout\SdkLayout" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 sdklayout.lib kernel32.lib user32.lib gdi32.lib setupapi.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386 /out:"bin/common_vc6.exe" /libpath:"C:\Users\YangTao\Desktop\SdkLayout\bin" /opt:nowin98
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "common - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "common___Win32_Debug"
# PROP BASE Intermediate_Dir "common___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "output\Debug_vc6"
# PROP Intermediate_Dir "output\Debug_vc6"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "C:\Users\YangTao\Desktop\SdkLayout\SdkLayout" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 sdklayout_d.lib kernel32.lib user32.lib gdi32.lib setupapi.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"bin/common_debug_vc6.exe" /pdbtype:sept /libpath:"C:\Users\YangTao\Desktop\SdkLayout\bin" /opt:nowin98
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "common - Win32 Release"
# Name "common - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;asm"
# Begin Source File

SOURCE=.\sources\about.c
# End Source File
# Begin Source File

SOURCE=.\sources\asctable.c
# End Source File
# Begin Source File

SOURCE=.\sources\comm.c
# End Source File
# Begin Source File

SOURCE=.\sources\common.c
# End Source File
# Begin Source File

SOURCE=.\sources\deal.c
# End Source File
# Begin Source File

SOURCE=.\sources\debug.c
# End Source File
# Begin Source File

SOURCE=.\sources\GetBaudRate.cpp
# End Source File
# Begin Source File

SOURCE=.\sources\GetBaudRate.h
# End Source File
# Begin Source File

SOURCE=.\sources\msg.c
# End Source File
# Begin Source File

SOURCE=.\sources\pinctrl.c
# End Source File
# Begin Source File

SOURCE=.\sources\send_cmd.cpp
# End Source File
# Begin Source File

SOURCE=.\sources\send_cmd.h
# End Source File
# Begin Source File

SOURCE=.\sources\str2hex.c
# End Source File
# Begin Source File

SOURCE=.\sources\timeouts.c
# End Source File
# Begin Source File

SOURCE=.\sources\utils.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;inc"
# Begin Source File

SOURCE=.\sources\about.h
# End Source File
# Begin Source File

SOURCE=.\sources\asctable.h
# End Source File
# Begin Source File

SOURCE=.\sources\cmd_dlg.h
# End Source File
# Begin Source File

SOURCE=.\sources\comm.h
# End Source File
# Begin Source File

SOURCE=.\sources\common.h
# End Source File
# Begin Source File

SOURCE=.\sources\deal.h
# End Source File
# Begin Source File

SOURCE=.\sources\debug.h
# End Source File
# Begin Source File

SOURCE=.\sources\expr.h
# End Source File
# Begin Source File

SOURCE=.\sources\monitor.h
# End Source File
# Begin Source File

SOURCE=.\sources\msg.h
# End Source File
# Begin Source File

SOURCE=.\sources\pinctrl.h
# End Source File
# Begin Source File

SOURCE=.\sources\stack.h
# End Source File
# Begin Source File

SOURCE=.\sources\str2hex.h
# End Source File
# Begin Source File

SOURCE=.\sources\timeouts.h
# End Source File
# Begin Source File

SOURCE=.\sources\utils.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\100.ico
# End Source File
# Begin Source File

SOURCE=.\res\common.rc
# End Source File
# Begin Source File

SOURCE=.\res\main.xml
# End Source File
# Begin Source File

SOURCE=.\res\xps.manifest
# End Source File
# End Group
# Begin Group "drivers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\sources\load_driver\com_drv.h
# End Source File
# Begin Source File

SOURCE=.\sources\load_driver\load_driver.c
# End Source File
# Begin Source File

SOURCE=.\sources\load_driver\load_driver.h
# End Source File
# End Group
# Begin Group "struct"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\sources\struct\Config.cpp
# End Source File
# Begin Source File

SOURCE=.\sources\struct\Config.h
# End Source File
# Begin Source File

SOURCE=.\sources\struct\list.c
# End Source File
# Begin Source File

SOURCE=.\sources\struct\list.h
# End Source File
# Begin Source File

SOURCE=.\sources\struct\memory.c
# End Source File
# Begin Source File

SOURCE=.\sources\struct\memory.h
# End Source File
# Begin Source File

SOURCE=.\sources\struct\parse_cmd.c
# End Source File
# Begin Source File

SOURCE=.\sources\struct\parse_cmd.h
# End Source File
# Begin Source File

SOURCE=.\sources\struct\Thunk.cpp
# End Source File
# Begin Source File

SOURCE=.\sources\struct\Thunk.h
# End Source File
# End Group
# End Target
# End Project
