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
#include "monutils.h"
#include <VersionHelpers.h>
#include <shlobj.h>
#include <tchar.h>

//-------------------------------------------------------------------------------------
BOOL FileExists(LPCWSTR szFileName)
{
	if (wcspbrk(szFileName, L"?*") != NULL)
		return FALSE;
	WIN32_FIND_DATAW wfd;
	HANDLE hFind = FindFirstFileW(szFileName, &wfd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);
		return (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
	}
	return FALSE;
}

//-------------------------------------------------------------------------------------
BOOL FilePatternExists(LPCWSTR szFileName)
{
	BOOL bRet = FALSE;
	WIN32_FIND_DATAW wfd;
	HANDLE hFind = FindFirstFileW(szFileName, &wfd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				bRet = TRUE;
				break;
			}
		} while (FindNextFileW(hFind, &wfd));
		FindClose(hFind);
	}
	return bRet;
}

//-------------------------------------------------------------------------------------
BOOL DirectoryExists(LPCWSTR szDirName)
{
	DWORD dwAttr = GetFileAttributesW(szDirName);
	return (dwAttr != INVALID_FILE_ATTRIBUTES) &&
		((dwAttr & FILE_ATTRIBUTE_DIRECTORY) != 0);
}

//-------------------------------------------------------------------------------------
void Trim(LPWSTR szString)
{
	LPWSTR pStart = szString;
	LPWSTR pDest = szString;

	while (*pStart == L'\r' ||
		*pStart == L'\n' ||
		*pStart == L' ' ||
		*pStart == L'\t')
	{
		pStart++;
	}

	if (pStart > szString)
	{
		while (*pStart)
			*pDest++ = *pStart++;
		*pDest = L'\0';
	}

	pStart = szString + wcslen(szString) - 1;
	while (pStart >= szString && (
		*pStart == L'\r' ||
		*pStart == L'\n' ||
		*pStart == L' ' ||
		*pStart == L'\t'))
	{
		*pStart-- = L'\0';
	}
}

//-------------------------------------------------------------------------------------
void GetFileParent(LPCWSTR szFile, LPWSTR szParent, size_t count)
{
	size_t i, len;
	i = len = wcslen(szFile) - 1;
	/*go back until we encounter a colon or backslash(es)*/
	BOOL bSlashSaw = FALSE;
	while (i != 0)
	{
		if (szFile[i] == L':')
			break;
		else if (ISSLASH(szFile[i]))
		{
			if (i == 1 && ISSLASH(szFile[0]))
			{
				i = len;
				break;
			}
			else
				bSlashSaw = TRUE; //begin to eat backslashes
		}
		else if (bSlashSaw)
			break; //last backslash eaten
		i--;
	}
	if (i < count - 1)
	{
		szParent[i + 1] = L'\0';
#ifdef __GNUC__
		wmemcpy(szParent, szFile, i + 1);
#else
		wmemcpy_s(szParent, count, szFile, i + 1);
#endif
	}
	else
		szParent[0] = L'\0'; //should never occur...
}

//-------------------------------------------------------------------------------------
BOOL IsUACEnabled()
{
	BOOL bRet = FALSE;

	if (IsWindowsVistaOrGreater())
	{
		HKEY hKey;
		LONG rc;
		rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
			L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",
			0, KEY_QUERY_VALUE, &hKey);
		if (rc == ERROR_SUCCESS)
		{
			DWORD dwType;
			DWORD data = 0;
			DWORD cbData = sizeof(data);
			rc = RegQueryValueExW(hKey, L"EnableLUA", NULL, &dwType, (LPBYTE)&data, &cbData);
			if (rc == ERROR_SUCCESS)
				bRet = (data == 0x00000001);
			RegCloseKey(hKey);
		}
	}

	return bRet;
}

std::wstring GetAppDataPath(REFKNOWNFOLDERID folderType) {
  PWSTR path = NULL;
  HRESULT hr =
      SHGetKnownFolderPath(folderType,  // Ŀ¼���ͣ����·�˵����
                           0,           // ��־��ͨ��Ϊ 0��
                           NULL,        // ���ƣ���ǰ�û��� NULL��
                           &path  // ���·�������� CoTaskMemFree �ͷţ�
      );

  if (SUCCEEDED(hr) && path != NULL) {
    std::wstring result(path);
    CoTaskMemFree(path);  // �ͷ��ڴ�
    return result;
  } else {
    return L"";  // ��ȡʧ��
  }
}

std::wstring GetAppDataDir() {
	auto dir = GetAppDataPath(FOLDERID_RoamingAppData) + L"\\BYPrinter";
	if (!DirectoryExists(dir.c_str())) {
		CreateDirectory(dir.c_str(), nullptr);
	}
	return dir;
}

std::wstring GetInstallDir() {
  auto dir = GetAppDataPath(FOLDERID_ProgramFiles) + L"\\BYPrinter";
  if (!DirectoryExists(dir.c_str())) {
    CreateDirectory(dir.c_str(), nullptr);
  }
  return dir;
}

std::wstring ReadStringKey(HKEY hroot, LPCWSTR sub_key, LPCWSTR value) {
  TCHAR path[MAX_PATH] = {L'\0'};
  DWORD size = sizeof(path);
  DWORD type = 0;
  if (ERROR_SUCCESS ==
      RegGetValueW(hroot, sub_key, value, RRF_RT_REG_SZ, &type, path, &size)) {
    return path;
  } else {
    return {};
  }
}

BOOL WriteStringKey(HKEY hroot, LPCWSTR sub_key, LPCWSTR name, LPCWSTR value) {
  HKEY hKey = nullptr;
  DWORD disposition = 0;
  auto ret =
      RegCreateKeyExW(hroot, sub_key, 0, nullptr, REG_OPTION_NON_VOLATILE,
                      KEY_WRITE, nullptr, &hKey, &disposition);
  if (ret == ERROR_SUCCESS) {
    return RegSetValueExW(
               hKey, name, 0, REG_SZ, reinterpret_cast<const BYTE*>(value),
               static_cast<DWORD>((_tcslen(value)) * sizeof(wchar_t))) ==
           ERROR_SUCCESS;
  } else {
    return FALSE;
  }
}

std::wstring GetAppDataDirFromReg() {
  return ReadStringKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\BYCompany\\BYMonitor",
                       L"app_data");
}

std::wstring GetInstallDirFromReg() {
  return ReadStringKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\BYCompany\\BYMonitor",
                       L"install_dir");
}

BOOL WriteAppDataDirToReg() {
  return WriteStringKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\BYCompany\\BYMonitor",
                        L"app_data", GetAppDataDir().c_str());
}

BOOL WriteInstallDirToReg() {
  return WriteStringKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\BYCompany\\BYMonitor",
                        L"install_dir", GetInstallDir().c_str());
}