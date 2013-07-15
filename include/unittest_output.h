#pragma once

#include "unittest.h"
#include "lock.h"
#include <vector>
#include <Windows.h>
#include <CommCtrl.h>
#include <process.h>
#include <string>

namespace tp
{
    /* TestOutput class that display test result in list view control
    The UI is created in separate thread
    */
    class ListTestOutput : public tp::TestOutput
    {
        struct UIThreadParam
        {
            ListTestOutput* lto;
            HANDLE readyevent;
        };
        typedef std::vector<tp::TestResult> results_t;

    public:
        ListTestOutput(LPCWSTR title) : m_hwnd(NULL), m_list(NULL), m_uithread(NULL), m_title(title), m_defaultBK(0)
        {
            UIThreadParam up;
            up.lto = this;
            up.readyevent = ::CreateEventW(NULL, FALSE, FALSE, NULL);
            m_uithread = (HANDLE)::_beginthreadex(NULL, 0, &ListTestOutput::UIThread, &up, 0, NULL);
            if (up.readyevent)
            {
                ::WaitForSingleObject(up.readyevent, INFINITE);
                ::CloseHandle(up.readyevent);
            }
            else
            {
                ::Sleep(1000);
            }
        }
        ~ListTestOutput()
        {
        }

        virtual void BlockBegin(const TestBlock & /*block*/)
        {
        }
        virtual void OutputResult(const TestResult& res)
        {
            size_t count = 0;

            m_csdata.lock();
            m_result.push_back(res);
            count = m_result.size();
            m_csdata.unlock();

            ::PostMessage(m_list, LVM_SETITEMCOUNT, count, 0);
        }
        virtual void TestEnd(int total, int succeeded)
        {
            wchar_t buffer[256];
            _snwprintf_s(buffer, _TRUNCATE, L" -- TOTAL:%d, FAILED:%d", total, total - succeeded);
            std::wstring title = m_title + buffer;
            ::SetWindowText(m_hwnd, title.c_str());
        }

        void WaitUIExit()
        {
            ::WaitForSingleObject(m_uithread, INFINITE);
            ::CloseHandle(m_uithread);
        }

    private:
        HWND m_hwnd;
        HWND m_list;
        HANDLE m_uithread;

        std::wstring m_title;

        results_t m_result;
        tp::critical_section_lock m_csdata;

        TestResult m_dummyResult;
        COLORREF m_defaultBK;
        COLORREF m_defaultFG;

    private:
        static unsigned int __stdcall UIThread(void* param)
        {
            UIThreadParam* up = static_cast<UIThreadParam*>(param);
            return up->lto->InternalUIThread(up->readyevent);
        }

        unsigned int InternalUIThread(HANDLE readyevent)
        {
            CreateWnd();
            ::SetEvent(readyevent);
            DoMessageLoop();
            return 0;
        }

        const TestResult& GetTestResult(int index)
        {
            size_t i = static_cast<size_t>(index);
            if (i < m_result.size()) return m_result[i];
            return m_dummyResult;
        }

        static void CopyTextToClipboard(LPCWSTR text)
        {
            const size_t len = (wcslen(text) + 1) * sizeof(wchar_t) ;
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
            if (hMem != NULL)
            {
                LPVOID addr = GlobalLock(hMem);
                if (addr != NULL)
                {
                    memcpy(addr, text, len);
                    GlobalUnlock(hMem);
                    OpenClipboard(0);
                    EmptyClipboard();
                    SetClipboardData(CF_UNICODETEXT, hMem);
                    CloseClipboard();
                }

                GlobalFree(hMem);
            }
        }

        void CreateWnd()
        {
            WNDCLASSW wc = { 0 };
            wc.style = CS_DBLCLKS | CS_ENABLE | CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = &ListTestOutput::WndProc;
            wc.lpszClassName = L"tplib_unittest_output";
            ::RegisterClassW(&wc);
            m_hwnd = ::CreateWindowW(wc.lpszClassName, m_title.c_str(), WS_VISIBLE | WS_OVERLAPPEDWINDOW, 0, 0, 1000, 500, NULL, NULL, NULL, this);
            ::ShowWindow(m_hwnd, SW_SHOW);
        }
        void CreateList(HWND hwnd)
        {
            DWORD style = LVS_REPORT | LVS_OWNERDATA;
            DWORD exstyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES;
            m_list = ::CreateWindowW(L"SysListView32", L"tput_list", WS_CHILD | WS_VISIBLE | WS_BORDER | style, 0, 0, 400, 300, hwnd, NULL, NULL, NULL);
            ::SendMessage(m_list, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, (LPARAM) exstyle);

            struct
            {
                LPCWSTR name;
                int width;
            }columnInfo [] =
            {
                L"result", 50,
                L"group", 150,
                L"test case", 500,
                L"comment", 200
            };
            for (size_t i = 0; i < _countof(columnInfo); i++)
            {
                LVCOLUMN column = { 0 };
                column.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
                column.pszText = (LPWSTR) columnInfo[i].name;
                column.fmt = LVCFMT_LEFT;
                column.cx = columnInfo[i].width;
                ::SendMessage(m_list, LVM_INSERTCOLUMN, i, (LPARAM)&column);
            }
        }
        void DoMessageLoop()
        {
            MSG msg;
            while (::GetMessage(&msg, NULL, 0, 0))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
            }
        }
        void ResizeList(int cx, int cy)
        {
            ::MoveWindow(m_list, 0, 0, cx, cy, TRUE);
        }
        void OnListKeyDown(LPNMLVKEYDOWN kd)
        {
            bool ctrldown = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
            if (ctrldown && kd->wVKey == L'C')
            {
                // control+C: copy test case description
                int index = -1;
                std::wstring text;
                for (;;)
                {
                    index = ListView_GetNextItem(m_list, index, LVNI_SELECTED);
                    if (index < 0) break;
                    m_csdata.lock();
                    text += GetTestResult(index).operation;
                    text += L"\r\n";
                    m_csdata.unlock();
                }
                if (text.length() > 0)
                {
                    CopyTextToClipboard(text.c_str());
                }
            }
            if (ctrldown && kd->wVKey == L'A')
            {
                int count = ListView_GetItemCount(m_list);
                for (int i = 0; i < count; i++)
                {
                    ListView_SetItemState(m_list, i, LVIS_SELECTED, LVIS_SELECTED);
                }
            }
        }
        void OnListGetDispInfo(LPNMLVDISPINFOW info)
        {
            int index = info->item.iItem;
            int subindex = info->item.iSubItem;
            size_t textlen = static_cast<size_t>(info->item.cchTextMax);
            tp::TestResult tr;
            m_csdata.lock();
            tr = GetTestResult(index);
            m_csdata.unlock();
            if (info->item.mask & LVIF_TEXT)
            {
                switch (subindex)
                {
                case 0:
                    wcsncpy_s(info->item.pszText, textlen, tr.success ? L"OK" : L"FAIL", _TRUNCATE);
                    break;
                case 1:
                    wcsncpy_s(info->item.pszText, textlen, tr.block->name, _TRUNCATE);
                    break;
                case 2:
                    wcsncpy_s(info->item.pszText, textlen, tr.operation.c_str(), _TRUNCATE);
                    break;
                case 3:
                    wcsncpy_s(info->item.pszText, textlen, tr.comment.c_str(), _TRUNCATE);
                    break;
                }
            }
            if (info->item.mask & LVIF_IMAGE)
            {
                info->item.iImage = 0;
            }
        }
        LRESULT OnCustomDrawList(LPNMLVCUSTOMDRAW cd)
        {
            if (cd->nmcd.dwDrawStage == CDDS_PREPAINT)
            {
                return CDRF_NOTIFYITEMDRAW;
            }
            if (cd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
            {
                m_defaultBK = cd->clrTextBk;
                m_defaultFG = cd->clrText;
                return CDRF_NOTIFYSUBITEMDRAW;
            }
            if (cd->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_ITEM | CDDS_SUBITEM))
            {
                if (cd->iSubItem == 0)
                {
                    int index = static_cast<int>(cd->nmcd.dwItemSpec);
                    const tp::TestResult& tr = GetTestResult(index);
                    cd->clrText = tr.success ? RGB(0, 192, 0) : RGB(255, 0, 0);
                }
                else
                {
                    cd->clrText = m_defaultFG;
                    cd->clrTextBk = m_defaultBK;
                }
            }
            return CDRF_DODEFAULT;
        }

        static LRESULT CALLBACK WndProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
        {
            if (msg == WM_NCCREATE)
            {
                CREATESTRUCT* cs = (CREATESTRUCT*) lp;
                ::SetWindowLongPtrW(wnd, GWLP_USERDATA, (LONG) cs->lpCreateParams);
            }

            ListTestOutput* pThis = (ListTestOutput*) ::GetWindowLongPtrW(wnd, GWLP_USERDATA);
            if (pThis)
            {
                return pThis->MyWndProc(wnd, msg, wp, lp);
            }
            else
            {
                // assert
                return ::DefWindowProcW(wnd, msg, wp, lp);
            }
        }

        LRESULT MyWndProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
        {
            switch (msg)
            {
            case WM_CREATE:
                CreateList(wnd);
                break;
            case WM_DESTROY:
                ::PostQuitMessage(0);
                break;
            case WM_SIZE:
                ResizeList(LOWORD(lp), HIWORD(lp));
                break;
            case WM_NOTIFY:
                {
                    LPNMHDR nmhdr = (LPNMHDR) lp;
                    if (nmhdr->hwndFrom == m_list)
                    {
                        if (nmhdr->code == LVN_GETDISPINFO)
                        {
                            OnListGetDispInfo((LPNMLVDISPINFOW) lp);
                        }
                        if (nmhdr->code == NM_CUSTOMDRAW)
                        {
                            return OnCustomDrawList((LPNMLVCUSTOMDRAW) lp);
                        }
                        if (nmhdr->code == LVN_KEYDOWN)
                        {
                            OnListKeyDown((LPNMLVKEYDOWN) lp);
                        }
                    }
                }
                break;
            }
            return ::DefWindowProcW(wnd, msg, wp, lp);
        }
    };
}
