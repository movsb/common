#pragma once
#ifndef __stdafx_120937012387901247_h__
#define __stdafx_120937012387901247_h__

#include <functional>
#include <iostream>
#include <vector>
#include <map>
#include <list>
#include <fstream>
#include <string>
#include <sstream>
#include <memory>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <process.h>

#include <windows.h>
#include <Richedit.h>
#include <WindowsX.h>
#include <Dbt.h>
#include <tchar.h>
#include <SetupAPI.h>
#include <devguid.h>
#include <WinInet.h>

#include "Sdklayout/SdkLayout.h"
#include "tinyxml2/tinyxml2.h"


#include "debug.h"
#include "utils.h"
#include "struct/list.h"
#include "struct/Config.h"
#include "struct/memory.h"
#include "struct/Thunk.h"

#include "Windows/Timer.h"
#include "Windows/theApp.h"
#include "Windows/WindowManager.h"
#include "Windows/Wnd.h"
#include "Windows/TextEditor.h"
#include "Windows/FileDialog.h"
#include "Windows/DialogBuilder.h"
#include "Windows/FileOperation.h"
#include "Windows/InputBox.h"

extern Common::CComConfig* comcfg;

#endif//!__stdafx_h__
