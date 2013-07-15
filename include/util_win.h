#pragma once

#include "exception.h"
#include "auto_release.h"
#include <vector>
#include <string>
#include <windows.h>

namespace tp
{
    namespace oswin
    {
        struct file
        {
            std::wstring abs_path;
            std::wstring rela_path;
            FILETIME create_time;
            FILETIME modify_time;
            FILETIME access_time;
            UINT64 length;
            DWORD attribute;
        };
        class iniconf
        {
            std::wstring m_path;
            std::wstring m_defsec;
        public:
            iniconf(const std::wstring&path, const std::wstring& default_section)
                : m_path(path), m_defsec(default_section)
            {
            }

            void savestr(const std::wstring& key, const std::wstring& val)
            {
                BOOL bRet = ::WritePrivateProfileStringW(m_defsec.c_str(), key.c_str(), val.c_str(), m_path.c_str());
                throw_winerr_when(!bRet);
            }
            void saveint(const std::wstring& key, int val)
            {
                wchar_t buf[1024];
                _itow_s(val, buf, 10);
                savestr(key, buf);
            }
            void savebool(const std::wstring& key, bool val)
            {
                saveint(key, val? 1 : 0);
            }
            std::wstring loadstr(const std::wstring& key, const std::wstring& defval)
            {
                wchar_t buf[1024];
                ::GetPrivateProfileStringW(m_defsec.c_str(), key.c_str(), defval.c_str(), buf, _countof(buf), m_path.c_str());
                return buf;
            }
            int loadint(const std::wstring& key, int defval)
            {
                return ::GetPrivateProfileIntW(m_defsec.c_str(), key.c_str(), defval, m_path.c_str());
            }
            bool loadbool(const std::wstring& key, bool defval)
            {
                int intval = loadint(key, defval?1:0);
                return (intval == 0? false : true);
            }
        };
        static int get_osverion(int* minor_ver)
        {
            OSVERSIONINFOW ovi;
            ovi.dwOSVersionInfoSize = sizeof(ovi);
            if (GetVersionExW(&ovi))
            {
                if (minor_ver) *minor_ver = ovi.dwMinorVersion;
                return ovi.dwMajorVersion;
            }
            return 0;
        }

        static std::wstring get_module_path(HMODULE m)
        {
            wchar_t buffer[4096];
            GetModuleFileNameW(m, buffer, _countof(buffer));
            return buffer;
        }

        static bool file_exists(const std::wstring& file)
        {
            return _waccess_s(file.c_str(), 0) == 0;
        }
        static UINT64 file_size(const std::wstring& file)
        {
            HANDLE hFile = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            ON_LEAVE_1(CloseHandle(hFile), HANDLE, hFile);
            DWORD dwSizeHIGH = 0;
            DWORD dwSizeLOW = ::GetFileSize(hFile, &dwSizeHIGH);
            return (UINT64)dwSizeHIGH << 32 | dwSizeLOW;
        }

        /** VISTA/WIN7: detect whether current process is elevated
         *  
         *  \ret TRUE current process is elevated, or UAC is turned off
         */
        static bool is_elevated()
        {
            if (get_osverion(0) < 6)
            {
                // XP
                return true;
            }

            HANDLE hToken;
            BOOL bRet = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
            throw_winerr_when(bRet != TRUE);
            ON_LEAVE_1(CloseHandle(hToken), HANDLE, hToken);

            TOKEN_ELEVATION te = {};
            DWORD dwReturnLength = 0;
            bRet = GetTokenInformation(hToken, TokenElevation, &te, sizeof(te), &dwReturnLength);
            throw_winerr_when(bRet != TRUE);

            return te.TokenIsElevated? true : false;
        }

        void reg_deletekey(HKEY root, const std::wstring& path, const std::wstring& name)
        {
            HKEY hkey;
            LONG ret = RegOpenKeyW(root, path.c_str(), &hkey);
            throw_winerr_when(ret != ERROR_SUCCESS);
            ON_LEAVE_1(RegCloseKey(hkey), HKEY, hkey);

            ret = RegDeleteKeyW(hkey, name.c_str());
            throw_winerr_when(ret != ERROR_SUCCESS);
        }

        void reg_deletekey(HKEY root, const std::wstring& pathname)
        {
            size_t pos = pathname.find_last_of(L'\\');
            throw_when(pos == std::wstring::npos, L"invalid reg path");

            reg_deletekey(root, pathname.substr(0, pos), pathname.substr(pos + 1));
        }

        static void reg_write_string(HKEY root, const std::wstring& path, const std::wstring& name, const std::wstring& value)
        {
            HKEY hkey;
            LONG ret = RegCreateKeyExW(root, path.c_str(), 0, NULL, 0, KEY_ALL_ACCESS, 0, &hkey, NULL);
            throw_winerr_when(ret != ERROR_SUCCESS);
            ON_LEAVE_1(RegCloseKey(hkey), HKEY, hkey);

            ret = RegSetValueExW(hkey, name.c_str(), 0, REG_SZ, (CONST BYTE*)value.c_str(), (value.length() + 1) * 2);
            throw_winerr_when(ret != ERROR_SUCCESS);
        }

        void reg_write_dword(HKEY root, LPCWSTR path, LPCWSTR name, DWORD value)
        {
            HKEY hkey;
            LONG ret = RegCreateKeyExW(root, path, 0, NULL, 0, KEY_ALL_ACCESS, 0, &hkey, NULL);
            throw_winerr_when(ret != ERROR_SUCCESS);
            ON_LEAVE_1(RegCloseKey(hkey), HKEY, hkey);

            ret = RegSetValueExW(hkey, name, 0, REG_DWORD, (CONST BYTE*)&value, 4);
            throw_winerr_when(ret != ERROR_SUCCESS);
        }

        void reg_read_value(HKEY root, const std::wstring& path, const std::wstring& name, DWORD& type, LPBYTE buffer, LPDWORD buflen)
        {
            HKEY hkey;
            LSTATUS ret;
            ret = RegOpenKeyW(root, path.c_str(), &hkey);
            throw_winerr_when(ret != ERROR_SUCCESS);
            ON_LEAVE_1(RegCloseKey(hkey), HKEY, hkey);

            ret = RegQueryValueExW(hkey, name.c_str(), NULL, &type, buffer, buflen);
            throw_winerr_when(ret != ERROR_SUCCESS);
        }

        std::wstring reg_read_string(HKEY root, const std::wstring& path, const std::wstring& name, const std::wstring& defval)
        {
            WCHAR buffer[1024] = {};
            DWORD buffer_len = sizeof(buffer);
            DWORD type = 0;
            reg_read_value(root, path, name, type, (LPBYTE)buffer, &buffer_len);

            return buffer;
        }

        DWORD reg_read_dword(HKEY root, const std::wstring& path, const std::wstring& name, const DWORD defval)
        {
            DWORD buffer = 0;
            DWORD buffer_len = sizeof(buffer);
            DWORD type = 0;
            reg_read_value(root, path, name, type, (LPBYTE)buffer, &buffer_len);

            return buffer;
        }

        std::vector<std::wstring> reg_get_subkeys(HKEY root, const std::wstring& path)
        {
            std::vector<std::wstring> keys;

            HKEY hkey;
            LSTATUS ret;
            ret = RegOpenKeyW(HKEY_LOCAL_MACHINE, path.c_str(), &hkey);
            throw_winerr_when(ret != ERROR_SUCCESS);
            ON_LEAVE_1(RegCloseKey(hkey), HKEY, hkey);

            WCHAR subKey[1024];
            for (int i = 0; ; i++)
            {
                ret = RegEnumKeyW(hkey, i, subKey, _countof(subKey));
                if (ret != ERROR_SUCCESS) break;

                keys.push_back(subKey);
            }

            return keys;
        }

        void reg_copykey(HKEY src, HKEY dest)
        {
            DWORD dwMaxSubkeyLen = 255;
            DWORD dwMaxValueNameLen = 255;
            DWORD dwMaxValueLen = 16383;
            LSTATUS ret;
            ret = RegQueryInfoKeyW(src, NULL, NULL, NULL, NULL, &dwMaxSubkeyLen, NULL, NULL, &dwMaxValueNameLen, &dwMaxValueLen, NULL, NULL);
            throw_winerr_when(ret != ERROR_SUCCESS);

            wchar_t* key = new wchar_t[dwMaxSubkeyLen + 1];
            ON_LEAVE_1(delete [] key, wchar_t*, key);
            wchar_t* vname = new wchar_t[dwMaxValueNameLen + 1];
            ON_LEAVE_1(delete[] vname, wchar_t*, vname);
            BYTE* value = new BYTE[dwMaxValueLen + 1];
            ON_LEAVE_1(delete[] value, BYTE*, value);

            for (DWORD index = 0;;index++)
            {
                DWORD dwValueNameLen = dwMaxValueNameLen + 1;
                DWORD dwValueLen = dwMaxValueLen + 1;
                DWORD dwType;
                ret = RegEnumValueW(src, index, vname, &dwValueNameLen, NULL, &dwType, value, &dwValueLen);
                if (ret == ERROR_NO_MORE_ITEMS) break;
                throw_winerr_when(ret != ERROR_SUCCESS);
                ret = RegSetValueExW(dest, vname, NULL, dwType, value, dwValueLen);
                throw_winerr_when(ret != ERROR_SUCCESS);
            }

            for (DWORD index = 0;;index++)
            {
                DWORD dwKeyLen = dwMaxSubkeyLen + 1;
                ret = RegEnumKeyExW(src, index, key, &dwKeyLen, NULL, NULL, NULL, NULL);
                if (ret == ERROR_NO_MORE_ITEMS) break;
                throw_winerr_when(ret != ERROR_SUCCESS);
                HKEY newsrc;
                HKEY newdst;
                ret = RegOpenKeyExW(src, key, NULL, KEY_READ, &newsrc);
                throw_winerr_when(ret != ERROR_SUCCESS);
                ret = RegCreateKeyExW(dest, key, NULL, NULL, 0, KEY_READ|KEY_WRITE, NULL, &newdst, NULL);
                throw_winerr_when(ret != ERROR_SUCCESS);

                reg_copykey(newsrc, newdst);
            }
        }

        static inline void list_dir(std::vector<file>& result, const wchar_t* path, const wchar_t* pattern = NULL, bool include_file = true, bool include_dir = false, bool recursive = false)
        {
            if (!pattern) pattern = L"*";
            if (!path || !*path) path = L".";
            std::wstring matcher = path;
            matcher += L'\\';
            matcher += pattern;

            WIN32_FIND_DATA wfd;
            HANDLE h = ::FindFirstFileW(matcher.c_str(), &wfd);
            if (h == INVALID_HANDLE_VALUE) return;
            ON_LEAVE_1(::FindClose(h), HANDLE, h);
            std::wstring root = path;
            file f;
            do {
                if (wfd.cFileName[0] == L'.' && !wfd.cFileName[1] ||
                    wfd.cFileName[0] == L'.' && wfd.cFileName[1] == L'.' && !wfd.cFileName[2])
                {
                    continue;
                }
                bool isdir = wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
                if (include_file && !isdir || include_dir && isdir)
                {
                    f.rela_path = root + L"\\" + wfd.cFileName;
                    f.attribute = wfd.dwFileAttributes;
                    f.length = (UINT64)wfd.nFileSizeHigh << 32 + wfd.nFileSizeLow;
                    f.access_time = wfd.ftLastAccessTime;
                    f.modify_time = wfd.ftLastWriteTime;
                    f.create_time = wfd.ftCreationTime;
                    result.push_back(f);
                }
                if (isdir && recursive)
                {
                    list_dir(result, f.rela_path.c_str(), pattern, include_file, include_dir, recursive);
                }
            } while (::FindNextFileW(h, &wfd));
        }
    };
}

