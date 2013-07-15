#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <stdio.h>

struct process_info
{
    std::wstring name;
    std::wstring image_path;
    std::wstring command_line;
    DWORD pid;
    DWORD ppid;
};
typedef std::vector<process_info> process_list_t;

#define RETURN_LASTERROR_WHEN(cond) \
    if (!!(cond)) { DWORD err_ = ::GetLastError(); return HRESULT_FROM_WIN32(err_); }

#define PI_TMP_JS L"PI_3e4df88y.js"
#define PI_TMP L"PI_3e4df88y.tmp"

struct pinfo
{
    static HRESULT get_process_list(process_list_t& pl)
    {
        WCHAR buffer[MAX_PATH];
        ::GetTempPath(_countof(buffer), buffer);
        std::wstring tmpdir = buffer;
        std::wstring jsfile = tmpdir + L"\\" PI_TMP_JS;
        std::wstring pifile = tmpdir + L"\\" PI_TMP;

        generate_js_file(jsfile);

        SHELLEXECUTEINFOW si = {};
        si.cbSize = sizeof(si);
        si.lpFile = L"wscript.exe";
        si.lpParameters = PI_TMP_JS;
        si.lpDirectory = tmpdir.c_str();
        si.fMask = SEE_MASK_NOCLOSEPROCESS|SEE_MASK_FLAG_NO_UI;
        si.nShow = SW_HIDE;
        si.lpVerb = L"open";
        ::ShellExecuteExW(&si);

        RETURN_LASTERROR_WHEN(!si.hProcess);

        DWORD script_pid = ::GetProcessId(si.hProcess);
        ::WaitForSingleObject(si.hProcess, 5000);
        ::CloseHandle(si.hProcess);

        FILE* fp;
        errno_t err = _wfopen_s(&fp, pifile.c_str(), L"rt,ccs=UNICODE");
        RETURN_LASTERROR_WHEN(err != 0);

        WCHAR line[4096];
        while (fgetws(line, _countof(line), fp))
        {
            LPCWSTR p = line;
            process_info pi;
            pi.name = fetch_next_field(&p);
            pi.pid = _wtoi(fetch_next_field(&p).c_str());
            if (pi.name.length() > 0 && pi.pid != script_pid)
            {
                pi.ppid = _wtoi(fetch_next_field(&p).c_str());
                pi.image_path = fetch_next_field(&p);
                pi.command_line = fetch_next_field(&p);
                pl.push_back(pi);
            }
        }

        fclose(fp);

        return S_OK;
    }

private:
    static std::wstring fetch_next_field(LPCWSTR* p)
    {
        std::wstring ret;
        LPCWSTR q = wcschr(*p, L'|');
        if (!q) return ret;
        ret.assign(*p, q - *p);
        *p = q + 1;
        return ret;
    }
    static HRESULT generate_js_file(const std::wstring& path)
    {
        WCHAR jscode[] = L"                                                                               \n\
            var wmi = GetObject('winmgmts:\\\\\\\\.\\\\root\\\\CIMV2');                                   \n\
            var fso = new ActiveXObject('Scripting.FileSystemObject');                                    \n\
            var colItems = wmi.ExecQuery('SELECT * FROM Win32_Process', 'WQL', 0x10 | 0x20);              \n\
            var enumItems = new Enumerator(colItems);                                                     \n\
            var result = '';                                                                              \n\
            for (; !enumItems.atEnd(); enumItems.moveNext()) {                                            \n\
                var p = enumItems.item();                                                                 \n\
                                                                                                          \n\
                result += [                                                                               \n\
                    p.Caption, p.ProcessId, p.ParentProcessId, p.ExecutablePath, p.CommandLine,           \n\
                        p.CreationDate, p.Priority, p.SessionId, p.ThreadCount, p.HandleCount,            \n\
                        p.KernelModeTime, p.UserModeTime,                                                 \n\
                        0].join('|') + '\\n';                                                             \n\
            }                                                                                             \n\
            fso.OpenTextFile('" PI_TMP L"', 2, true, -1).Write(result);                                \n\
        ";
        
        FILE* fp = NULL;
        errno_t err = _wfopen_s(&fp, path.c_str(), L"wt,ccs=UNICODE");
        RETURN_LASTERROR_WHEN(err != 0);

        fwprintf_s(fp, L"%s", jscode);
        fclose(fp);
    }
};
