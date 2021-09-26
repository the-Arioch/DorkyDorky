/*
  Dokan : user-mode file system library for Windows

  Copyright (C) 2020 Google, Inc.
  Copyright (C) 2015 - 2019 Adrien J. <liryna.stark@gmail.com> and Maxime C. <maxime@islog.com>
  Copyright (C) 2007 - 2011 Hiroki Asakawa <info@dokan-dev.net>

  http://dokan-dev.github.io

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free
Software Foundation; either version 3 of the License, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DOKANC_H_
#define DOKANC_H_

#include "dokan.h"
#include <malloc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DOKAN_GLOBAL_DEVICE_NAME L"\\\\.\\Dokan_" DOKAN_MAJOR_API_VERSION

#define DOKAN_DRIVER_SERVICE L"Dokan" DOKAN_MAJOR_API_VERSION

#define DOKAN_CONTROL_OPTION_FORCE_UNMOUNT 1

#define DOKAN_CONTROL_SUCCESS 1
#define DOKAN_CONTROL_FAIL 0

#define DOKAN_SERVICE_START 1
#define DOKAN_SERVICE_STOP 2
#define DOKAN_SERVICE_DELETE 3

#define DOKAN_KEEPALIVE_TIME 3000 // in miliseconds

#define DOKAN_MAX_THREAD 63

// DokanOptions->DebugMode is ON?
extern BOOL g_DebugMode;

// DokanOptions->UseStdErr is ON?
extern BOOL g_UseStdErr;

#if defined(_MSC_VER) || (defined(__GNUC__) && !defined(__CYGWIN__))

static VOID DokanDbgPrint(LPCSTR format, ...) {
  const char *outputString = format; // fallback
  char *buffer = NULL;
  int length;
  va_list argp;

  va_start(argp, format);
  length = _vscprintf(format, argp) + 1;
  if ((length - 1) != -1) {
    buffer = (char *)_malloca(length * sizeof(char));
  }
  if (buffer && vsprintf_s(buffer, length, format, argp) != -1) {
    outputString = buffer;
  }
  if (g_UseStdErr)
    fputs(outputString, stderr);
  else
    OutputDebugStringA(outputString);
  if (buffer)
    _freea(buffer);
  va_end(argp);
  if (g_UseStdErr)
    fflush(stderr);
}

// Dokan1.DLL would often get used from multithreaded applications, like
// Windows Explorer querying "network drives", and then a debug log having
// all threads' lines interspersed and only marked with ProcessId makes it
// hardly usable, like in https://github.com/dokan-dev/dokany/issues/945
// We have to add thread's mark for filtering/sorting of the log.
#if defined(_MSC_VER) && !defined(thread_local)
  #define thread_local __declspec(thread)
#endif
static thread_local DWORD _known_thread_id = 0;
static const WCHAR *_thread_tag_template = L" <th:4294967295> "; // length == 17+1
static thread_local WCHAR _thread_tag[20] = L"";
static thread_local int _thread_tag_len = 0;

#include <assert.h>

static VOID DokanDbgPrintW(LPCWSTR format, ...) {
  const WCHAR *outputString = format; // fallback
  WCHAR *buffer = NULL;
  WCHAR *msg = NULL;
  int length;
  va_list argp;
  DWORD tid;

  va_start(argp, format);
  
  tid = GetCurrentThreadId();
  if ((tid != _known_thread_id) || (!_thread_tag[0])) {
    if ( !_thread_tag[0] ) {
       int err = wcscpy_s( _thread_tag, 20, _thread_tag_template ); 
       assert( 0 == err ); 
    }
    if ( !_ultow_s( tid, &_thread_tag[5], 20 - 5 - 2, 10) ) {
      int err = wcscat_s( _thread_tag, 20, _thread_tag_template[15] /* L"> " */ );
      if ( err ) {
        _thread_tag[0] = 0;
        _thread_tag_len = 0;
      } else {   
        _thread_tag_len = wcslen( _thread_tag );
        _known_thread_id = tid;
      }  
    }    
  }
  
  length = _vscwprintf(format, argp) + 1;
  if ((length - 1) != -1) {
    if (_thread_tag[0])
      length += _thread_tag_len;
    buffer = (WCHAR *)_malloca(length * sizeof(WCHAR));
  }
  
  if (buffer) {
    msg = buffer;  
    if (_thread_tag[0]) {
      if (!wcscpy_s(buffer, length, _thread_tag)) {
          msg += _thread_tag_len;
          length -= _thread_tag_len;
      }
    }
    if (vswprintf_s(msg, length, format, argp) != -1) {
      outputString = buffer;
    }
  }
  if (g_UseStdErr)
    fputws(outputString, stderr);
  else
    OutputDebugStringW(outputString);
  if (buffer)
    _freea(buffer);
  va_end(argp);
}

#if defined(_MSC_VER)

#define DbgPrint(format, ...)                                                  \
  do {                                                                         \
    if (g_DebugMode) {                                                         \
      DokanDbgPrint(format, __VA_ARGS__);                                      \
    }                                                                          \
  }                                                                            \
  __pragma(warning(push)) __pragma(warning(disable : 4127)) while (0)          \
      __pragma(warning(pop))

#define DbgPrintW(format, ...)                                                 \
  do {                                                                         \
    if (g_DebugMode) {                                                         \
      DokanDbgPrintW(format, __VA_ARGS__);                                     \
    }                                                                          \
  }                                                                            \
  __pragma(warning(push)) __pragma(warning(disable : 4127)) while (0)          \
      __pragma(warning(pop))

#endif // defined(_MSC_VER)

#if defined(__GNUC__)

#define DbgPrint(format, ...)                                                  \
  do {                                                                         \
    if (g_DebugMode) {                                                         \
      DokanDbgPrint(format, ##__VA_ARGS__);                                    \
    }                                                                          \
  } while (0)

#define DbgPrintW(format, ...)                                                 \
  do {                                                                         \
    if (g_DebugMode) {                                                         \
      DokanDbgPrintW(format, ##__VA_ARGS__);                                   \
    }                                                                          \
  } while (0)

#endif // defined(__GNUC__)

#endif // defined(_MSC_VER) || (defined(__GNUC__) && !defined(__CYGWIN__))

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

/**
 * \brief Output the debug logs to StdErr or not.
 *
 * Note: By default they are sent to the debugger.
 */
VOID DOKANAPI DokanUseStdErr(BOOL Status);

/**
 * \brief Enable or not the debug logs for the Library.
 */
VOID DOKANAPI DokanDebugMode(BOOL Status);

/**
 * \brief Create an start a Windows service.
 * 
 * Note: This is only used by dokanctl to install the Dokan driver.
 */
BOOL DOKANAPI DokanServiceInstall(LPCWSTR ServiceName, DWORD ServiceType,
                                  LPCWSTR ServiceFullPath);

/**
 * \brief Stop and delete a Windows service.
 *
 * Note: This is only used by dokanctl to remove the Dokan driver.
 */
BOOL DOKANAPI DokanServiceDelete(LPCWSTR ServiceName);

/**
 * \brief Install dokan network provider.
 *
 * Note: This is only used by dokanctl.
 */
BOOL DOKANAPI DokanNetworkProviderInstall();

/**
 * \brief Uninstall dokan network provider.
 *
 * Note: This is only used by dokanctl.
 */
BOOL DOKANAPI DokanNetworkProviderUninstall();

/**
 * \brief Change the current debug log level in the driver.
 * 
 * See DOKAN_DEBUG_* in util/log.h of the driver folder
 * for the different options.
 */
BOOL DOKANAPI DokanSetDebugMode(ULONG Mode);

/**
 * \brief Force the driver to cleanup mount points
 *
 * During a dirty unmount, it is possible that the driver
 * keeps the used mount point active. The function forces the driver
 * to remove them from the system for being again available for a new mount.
 */
BOOL DOKANAPI DokanMountPointsCleanUp();

#ifdef __cplusplus
}
#endif

#endif // DOKANC_H_
