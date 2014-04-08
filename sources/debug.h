#ifndef __DEBUGOUT_H__
#define __DEBUGOUT_H__

#ifdef _DEBUG
#define debug_out(x) DebugOut##x
#else
#define debug_out(x) 
#endif

void DebugOut(char* fmt, ...);

#endif//!DEBUGOUT_INC_
