#pragma once

#include "unittest.h"
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
        ListTestOutput(LPCWSTR title) : m_hwnd(NULL), m_list(NULL), m_uithread(NULL), m_title(title)
		{
			::InitializeCriticalSection(&m_csdata);

			UIThreadParam up;
			up.lto = this;
			up.readyevent = ::CreateEventW(NULL, FALSE, FALSE, NULL);
			m_uithread = (HANDLE)::_beginthreadex(NULL, 0, &ListTestOutput::UIThread, &up, 0, NULL);
			::WaitForSingleObject(up.readyevent, INFINITE);
			::CloseHandle(up.readyevent);
		}
		~ListTestOutput()
		{
			::DeleteCriticalSection(&m_csdata);
		}

		virtual void BlockBegin(const TestBlock& block)
		{
			
		}
		virtual void OutputResult(const TestResult& res)
		{
			size_t count = 0;
			::EnterCriticalSection(&m_csdata);
			m_result.push_back(res);
			count = m_result.size();
			::LeaveCriticalSection(&m_csdata);
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
		CRITICAL_SECTION m_csdata;

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

        static void CopyTextToClipboard(LPCWSTR text)
        {
            const size_t len = (wcslen(text) + 1) * sizeof(wchar_t);
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
            memcpy(GlobalLock(hMem), text, len);
            GlobalUnlock(hMem);
            OpenClipboard(0);
            EmptyClipboard();
            SetClipboardData(CF_UNICODETEXT, hMem);
            CloseClipboard();
        }

		void CreateWnd()
		{
			WNDCLASSW wc = {0};
			wc.style = CS_DBLCLKS|CS_ENABLE|CS_HREDRAW|CS_VREDRAW;
			wc.lpfnWndProc = &ListTestOutput::WndProc;
			wc.lpszClassName = L"tplib_unittest_output";
			::RegisterClassW(&wc);
            m_hwnd = ::CreateWindowW(wc.lpszClassName, m_title.c_str(), WS_VISIBLE | WS_OVERLAPPEDWINDOW, 0, 0, 1000, 500, NULL, NULL, NULL, this);
			::ShowWindow(m_hwnd, SW_SHOW);
		}
		void CreateList(HWND hwnd)
		{
			DWORD style = LVS_REPORT|LVS_OWNERDATA;
			DWORD exstyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES;
			m_list = ::CreateWindowW(L"SysListView32", L"tput_list", WS_CHILD|WS_VISIBLE|WS_BORDER|style, 0, 0, 400, 300, hwnd, NULL, NULL, NULL);
			::SendMessage(m_list, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, exstyle);

			struct
			{
				LPCWSTR name;
				int width;
			}columnInfo[] =
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
				column.pszText = (LPWSTR)columnInfo[i].name;
				column.fmt = LVCFMT_LEFT;
				column.cx = columnInfo[i].width;
				::SendMessage(m_list, LVM_INSERTCOLUMN,	i, (LPARAM)&column);
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
                    ::EnterCriticalSection(&m_csdata);
                    text += m_result[index].operation;
                    text += L"\r\n";
                    ::LeaveCriticalSection(&m_csdata);
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
			size_t textlen = info->item.cchTextMax;
			tp::TestResult tr;
			::EnterCriticalSection(&m_csdata);
			tr = m_result[index];
			::LeaveCriticalSection(&m_csdata);
			wchar_t cvtbuf[1024];
			if (info->item.mask & LVIF_TEXT)
			{
				switch (subindex)
				{
				case 0:
					wcsncpy_s(info->item.pszText, textlen, tr.success?L"OK":L"FAIL", _TRUNCATE);
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
				int index = static_cast<int>(cd->nmcd.dwItemSpec);
				const tp::TestResult& tr = m_result[index];
				if (!tr.success)
				{
					cd->clrTextBk = RGB(255, 128, 128);
					//return CDRF_NOTIFYSUBITEMDRAW;
				}
				else
				{
					cd->clrTextBk = RGB(128, 255, 128);
				}
			}
			if (cd->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT|CDDS_ITEM|CDDS_SUBITEM))
			{
				if (cd->iSubItem == 4)
				{
					cd->clrTextBk = RGB(0, 255, 0);
				}
			}
			return CDRF_DODEFAULT;
		}

		static LRESULT CALLBACK WndProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
		{
			if (msg == WM_NCCREATE)
			{
				CREATESTRUCT* cs = (CREATESTRUCT*)lp;
				::SetWindowLongPtrW(wnd, GWLP_USERDATA, (LONG)cs->lpCreateParams);
			}

			ListTestOutput* pThis = (ListTestOutput*)::GetWindowLongPtrW(wnd, GWLP_USERDATA);
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
					LPNMHDR nmhdr = (LPNMHDR)lp;
					if (nmhdr->hwndFrom == m_list)
					{
						if (nmhdr->code == LVN_GETDISPINFO)
						{
							OnListGetDispInfo((LPNMLVDISPINFOW)lp);
						}
						if (nmhdr->code == NM_CUSTOMDRAW)
						{
							return OnCustomDrawList((LPNMLVCUSTOMDRAW)lp);
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

