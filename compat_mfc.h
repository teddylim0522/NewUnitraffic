// Compatibility shim when MFC is not available in the build environment.
#pragma once

#include <string>
#include <cstring>

// Provide a minimal CString substitute based on std::string to avoid
// requiring MFC headers for non-GUI builds. This is intentionally small;
// convert to real CString (MFC) if MFC is available.

// Simple alias — many project files use CString as a string container.
typedef std::string CString;

// CA2W macro in original code converts ANSI to wide; provide a simple
// passthrough that returns std::string when MFC isn't present.
#define CA2W(x) (std::string(x))

// _T macro passthrough
#ifndef _T
#define _T(x) x
#endif

// Minimal definitions used in the code
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

