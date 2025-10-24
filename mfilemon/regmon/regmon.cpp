/*
MFILEMON - print to file with automatic filename assignment
Copyright (C) 2007-2023 Lorenzo Monti

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "stdafx.h"
#include "../common/monutils.h"

static const LPCWSTR pMonitorName = (LPWSTR)L"BYPortMonitor";

static LPCTSTR szGPL =
    _T("MFILEMON - print to file with automatic filename assignment\n")
    _T("Copyright (C) 2007-2023 Lorenzo Monti\n")
    _T("\n")
    _T("This program is free software; you can redistribute it and/or\n")
    _T("modify it under the terms of the GNU General Public License\n")
    _T("as published by the Free Software Foundation; either version 3\n")
    _T("of the License, or (at your option) any later version.\n")
    _T("\n")
    _T("This program is distributed in the hope that it will be useful,\n")
    _T("but WITHOUT ANY WARRANTY; without even the implied warranty of\n")
    _T("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n")
    _T("GNU General Public License for more details.\n")
    _T("\n")
    _T("You should have received a copy of the GNU General Public License\n")
    _T("along with this program; if not, write to the Free Software\n")
    _T("Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  ")
    _T("02110-1301, USA.");

static LPCTSTR szUsage =
    _T("**************************************\n")
    _T("Usage: regmon [-r | -d]\n")
    _T("       -r: register monitor\n")
    _T("       -d: deregister monitor\n")
    _T("       -l: list registered monitors\n")
    _T("**************************************");

static BOOL ListRegisteredMonitors() {
  DWORD pcbNeeded = 0;
  DWORD pcReturned = 0;

  EnumMonitors(NULL, 2, NULL, 0, &pcbNeeded, &pcReturned);
  if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
    //_tprintf(_T("%s\n"), "ERROR_INSUFFICIENT_BUFFER");
    return FALSE;
  }

  LPBYTE pPorts = new BYTE[pcbNeeded];

  if (!pPorts)
    return FALSE;

  ZeroMemory(pPorts, pcbNeeded);

  BOOL result =
      EnumMonitors(NULL, 2, pPorts, pcbNeeded, &pcbNeeded, &pcReturned);
  if (!result) {
    DWORD dwErr = GetLastError();
    delete[] pPorts;
    SetLastError(dwErr);
    return FALSE;
  }

  MONITOR_INFO_2* pinfo = reinterpret_cast<MONITOR_INFO_2*>(pPorts);

  for (; pcReturned-- > 0; pinfo++) {
    _tprintf(_T("%s\n"), pinfo->pName);
  }

  delete[] pPorts;

  return TRUE;
}

BOOL CopyFileToSystem32(LPCTSTR file_name) { 
    wchar_t system_dir[MAX_PATH];
    GetSystemDirectoryW(system_dir, MAX_PATH);
    std::wstring dst_path = system_dir;
    dst_path += L"\\";
    dst_path += file_name;
    TCHAR appDir[MAX_PATH] = _T("\0");
    GetModuleFileName(NULL, appDir, MAX_PATH);
    *_tcsrchr(appDir, _TCHAR('\\')) = _TCHAR('\0');
    std::wstring src_path = appDir;
    src_path += L"\\";
    src_path += file_name;
    return CopyFileW(src_path.c_str(), dst_path.c_str(), FALSE);
}

BOOL CopyFileToInstallDir(LPCTSTR file_name) {
  std::wstring dst_path = GetInstallDirFromReg();
  dst_path += L"\\";
  dst_path += file_name;
  TCHAR appDir[MAX_PATH] = _T("\0");
  GetModuleFileName(NULL, appDir, MAX_PATH);
  *_tcsrchr(appDir, _TCHAR('\\')) = _TCHAR('\0');
  std::wstring src_path = appDir;
  src_path += L"\\";
  src_path += file_name;
  return CopyFileW(src_path.c_str(), dst_path.c_str(), FALSE);
}

int _tmain(int argc, _TCHAR* argv[]) {
  MONITOR_INFO_2 minfo = {0};
  LPTSTR szAction = NULL;
  int ret;

  minfo.pName = (LPWSTR) _T("BYPortMonitor");
  minfo.pEnvironment = NULL;
  minfo.pDLLName = (LPWSTR) _T("BYPortMonitor.dll");

  if (argc > 1 && _tcsicmp(argv[1], _T("-r")) == 0) {
    do 
    {
      ret = WriteAppDataDirToReg();
      if (!ret) {
        szAction = (LPWSTR) _T("WriteAppDataDirToReg");
        break;
      }

      ret = WriteInstallDirToReg();
      if (!ret) {
        szAction = (LPWSTR) _T("WriteAppDataDirToReg");
        break;
      }

      ret = CopyFileToSystem32(L"BYPortMonitor.dll");
      if (!ret) {
        szAction = (LPWSTR) _T("CopyFileToSystem32");
        break;
      }
      ret = CopyFileToSystem32(L"BYPortMonitorUI.dll");
      if (!ret) {
        szAction = (LPWSTR) _T("CopyFileToSystem32");
        break;
      }
      ret = CopyFileToInstallDir(L"gsdll64.dll");
      if (!ret) {
        szAction = (LPWSTR) _T("CopyFileToInstallDir");
        break;
      }
      ret = CopyFileToInstallDir(L"gswin64c.exe");
      if (!ret) {
        szAction = (LPWSTR) _T("CopyFileToInstallDir");
        break;
      }
      szAction = (LPWSTR) _T("AddMonitor");
      ret = AddMonitor(NULL, 2, (LPBYTE)&minfo);
      break;
    } while (true);
  } else if (argc > 1 && _tcsicmp(argv[1], _T("-d")) == 0) {
    szAction = (LPWSTR) _T("DeleteMonitor");
    ret = DeleteMonitor(NULL, NULL, minfo.pName);
  } else if (argc > 1 && _tcsicmp(argv[1], _T("-l")) == 0) {
    szAction = (LPWSTR) _T("ListMonitors");
    ret = ListRegisteredMonitors();
  } else {
    return 1;
  }

  if (ret == FALSE) {
    DWORD dwErr = GetLastError();
    LPTSTR pMsgBuf = NULL;
    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_FROM_SYSTEM |
                          FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL, dwErr, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                      reinterpret_cast<LPTSTR>(&pMsgBuf), 0, NULL)) {
      _tprintf(_T("The following error occurred:\n%d\n%s\n"), dwErr,
               pMsgBuf);
      LocalFree(pMsgBuf);
    }
  }

  _tprintf(_T("%s %s\n"), szAction,
           ret == FALSE ? _T("FAILED!!!") : _T("SUCCEEDED!!!"));

  return (ret == FALSE);
}
