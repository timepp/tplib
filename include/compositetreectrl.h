#pragma once

/** sys treeview control for viewing and editing general composite
    supported operations:
	    - add/delete/modify/configuration
	    - copy/cut/paste
	    - drap/drop
*/

#include "composite.h"
#include <atlbase.h>
#include <atlwin.h>
#include <atlapp.h>
#include <atlctrls.h>
#include <atlcrack.h>
#include <atlmisc.h>

namespace tp
{
    class CRootComposite : public composite
    {
    public:
        virtual std::wstring name() const { return L""; }
        virtual std::wstring desc() const { return L""; }
        virtual component* clone(bool deep = true) const
        {
            CRootComposite* c = new CRootComposite;
            if (deep) clone_childs(&c->m_childs);
            return c;
        }
        virtual bool can_add_child(component * /*child*/) const
        {
            return child_count() == 0;
        }
        virtual bool add_child(component* child)
        {
            for (size_t i = 0; i < child_count(); i++)
            {
                if (m_childs[i] == child)
                {
                    return false;
                }
            }
            m_childs.push_back(child);
            return true;
        }
        virtual bool load(component_creator * /*cc*/, serializer * /*s*/) { return false; }
        virtual bool save(component_creator * /*cc*/, serializer * /*s*/) const { return false; }
        virtual std::wstring classname() const { return L"CRootComposite"; }
    };

    struct LocaleTranslater
    {
        virtual ~LocaleTranslater(){}
        virtual std::wstring Translate(int id) = 0;
    };

    class CCompositeTreeCtrl : public CWindowImpl<CCompositeTreeCtrl, CTreeViewCtrlEx>
    {
    public:

        CCompositeTreeCtrl() : m_clipboard(NULL), m_fc(NULL), m_lb_down(false), m_dragging(false), m_translater(NULL)
        {
        }
        ~CCompositeTreeCtrl()
        {
        }

        void Create(HWND hWndParent, HIMAGELIST imglist)
        {
            CWindowImpl<CCompositeTreeCtrl, CTreeViewCtrlEx>::Create(
                hWndParent, NULL, NULL,
                CControlWinTraits::GetWndStyle(0) | WS_BORDER | WS_TABSTOP | TVS_DISABLEDRAGDROP | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_INFOTIP
                );
            m_imgList = SetImageList(imglist, TVSIL_NORMAL);
        }

        void SetTranslater(LocaleTranslater* translater)
        {
            m_translater = translater;
        }

        void SetComponentCreator(component_creator* fc)
        {
            m_fc = fc;
        }

        void AddComponentName(const std::wstring& cname)
        {
            m_cnames.push_back(cname);
        }

        void SetComponent(const component* c)
        {
            CTreeItem item = GetRootItem();
            if (!IsNormalItem(item))
            {
                if (c)
                    InsertComponent(TVI_ROOT, TVI_LAST, c->clone());
            }
            else
            {
                if (c)
                    InsertComponent(TVI_ROOT, item, c->clone());
                DeleteComponent(item);
            }
        }

        component* GetComponent() const
        {
            if (m_root.child_count() == 0)
            {
                return NULL;
            }
            return m_root.get_child(0);
        }

    private:

        BEGIN_MSG_MAP(CCompositeTreeCtrl)
            MSG_WM_RBUTTONDOWN(OnRbuttonDown)
            MSG_WM_LBUTTONDBLCLK(OnLbuttonDbclk)
            MSG_WM_LBUTTONDOWN(OnLButtonDown)
            MSG_WM_MOUSEMOVE(OnMouseMove)
            MSG_WM_LBUTTONUP(OnLButtonUp)
            MSG_WM_COMMAND(OnCommand)
            MSG_WM_KEYDOWN(OnKeyDown)
            MSG_WM_CONTEXTMENU(OnContextMenu)
        END_MSG_MAP()

        enum
        {
            MENUID_COPY = 1001,
            MENUID_PASTE = 1002,
            MENUID_PASTE_AS_CHILD = 1003,
            MENUID_CUT = 1004,
            MENUID_DELETE = 1005,
            MENUID_ADD_COMP_BEGIN = 1100,
            MENUID_ADD_COMP_END = 1199,
            MENUID_CVT_COMP_BEGIN = 1200,
            MENUID_CVT_COMP_END = 1299,
            MENUID_ADD_COMP_AS_PARENT_BEGIN = 1300,
            MENUID_ADD_COMP_AS_PARENT_END = 1399
        };

        CRootComposite m_root;
        component* m_clipboard;
        component_creator* m_fc;
        std::vector<std::wstring> m_cnames;
        CImageList m_dragImgList;
        CImageList m_imgList;
        LocaleTranslater* m_translater;
        std::wstring m_transbuffer;

        // status
        bool m_lb_down;
        bool m_dragging;
        CPoint m_dragpt;
        CTreeItem m_dragging_source;
        CTreeItem m_dragging_target;

        static bool KeyIsDown(int vk)
        {
            return (GetKeyState(vk) & 0x8000) != 0;
        }
        const wchar_t* GetMenuItemText(int menuID, LPCWSTR postfix)
        {
	        m_transbuffer = m_translater->Translate(menuID) + postfix;
	        return m_transbuffer.c_str();
        }
        void PopupContextMenu(CPoint pt)
        {
            CMenu muAdd;
            CMenu muAddAsParent;
            CMenu muCvt;
            muAdd.CreatePopupMenu();
            muAddAsParent.CreatePopupMenu();
            muCvt.CreatePopupMenu();
            for (size_t i = 0; i < m_cnames.size(); i++)
            {
                if (m_cnames[i].length() == 0)
                {
                    muAdd.AppendMenu(MF_SEPARATOR);
                    muCvt.AppendMenu(MF_SEPARATOR);
                }
                else
                {
                    const component* proto = m_fc->get_prototype(m_cnames[i]);
                    muAdd.AppendMenu(MF_STRING, MENUID_ADD_COMP_BEGIN + i, proto->name().c_str());
                    muCvt.AppendMenu(MF_STRING, MENUID_CVT_COMP_BEGIN + i, proto->name().c_str());
                    if (proto->can_add_child(NULL))
                    {
                        muAddAsParent.AppendMenu(MF_STRING, MENUID_ADD_COMP_AS_PARENT_BEGIN + i, proto->name().c_str());
                    }
                }
            }

            CMenu mu;
            mu.CreatePopupMenu();
            mu.AppendMenu(MF_POPUP, muAdd.m_hMenu, GetMenuItemText(MENUID_ADD_COMP_BEGIN, L"(&A)"));
            mu.AppendMenu(MF_POPUP, muAddAsParent.m_hMenu, GetMenuItemText(MENUID_ADD_COMP_AS_PARENT_BEGIN, L"(&Q)"));
            mu.AppendMenu(MF_POPUP, muCvt.m_hMenu, GetMenuItemText(MENUID_CVT_COMP_BEGIN, L"(&S)"));
            mu.AppendMenu(MF_SEPARATOR);
            mu.AppendMenu(MF_STRING, MENUID_COPY, GetMenuItemText(MENUID_COPY, L"(&C)\tCtrl+C"));
            mu.AppendMenu(MF_STRING, MENUID_CUT, GetMenuItemText(MENUID_CUT, L"(&X)\tCtrl+X"));
            mu.AppendMenu(MF_STRING, MENUID_PASTE, GetMenuItemText(MENUID_PASTE, L"(&V)\tCtrl+V"));
            mu.AppendMenu(MF_STRING, MENUID_PASTE_AS_CHILD, GetMenuItemText(MENUID_PASTE_AS_CHILD, L"(&B)"));
            mu.AppendMenu(MF_STRING, MENUID_DELETE, GetMenuItemText(MENUID_DELETE, L"(&D)\tDel"));

            CTreeItem item = GetSelectedItem();
            component* c = GetItemData(item);
            if (!IsNormalItem(item))
            {
                mu.EnableMenuItem(MENUID_COPY, MF_DISABLED);
                mu.EnableMenuItem(MENUID_CUT, MF_DISABLED);
                mu.EnableMenuItem(MENUID_DELETE, MF_DISABLED);
                mu.EnableMenuItem((UINT) muCvt.m_hMenu, MF_DISABLED);
                mu.EnableMenuItem((UINT) muAddAsParent.m_hMenu, MF_DISABLED);
                mu.EnableMenuItem(MENUID_PASTE, MF_DISABLED);
            }
            else
            {
                CTreeItem parentItem = item.GetParent();
                component* pc = GetItemData(parentItem);
                if (!pc->can_add_child(NULL))
                {
                    mu.EnableMenuItem(MENUID_PASTE, MF_DISABLED);
                }
            }

            if (!c->can_add_child(NULL))
            {
                mu.EnableMenuItem((UINT) muAdd.m_hMenu, MF_DISABLED);
                mu.EnableMenuItem(MENUID_PASTE_AS_CHILD, MF_DISABLED);
            }

            if (!m_clipboard)
            {
                mu.EnableMenuItem(MENUID_PASTE, MF_DISABLED);
                mu.EnableMenuItem(MENUID_PASTE_AS_CHILD, MF_DISABLED);
            }

            ClientToScreen(&pt);
            mu.TrackPopupMenu(0, pt.x, pt.y, m_hWnd);
        }

        std::wstring GetComponentDisplayText(const component* c)
        {
            if (dynamic_cast<const composite*>(c))
            {
                return c->name() + L" : " + c->desc();
            }
            else if (c)
            {
                return c->desc();
            }
            return L"";
        }

        void UpdateDesc(CTreeItem item)
        {
            while (IsNormalItem(item))
            {
                component* c = GetItemData(item);
                item.SetText(GetComponentDisplayText(c).c_str());
                item = item.GetParent();
            }
        }

        component* CreateAndConfigure(const std::wstring name)
        {
            component* c = m_fc->create(name);
            if (!m_fc->configure(c))
            {
                delete c;
                c = NULL;
            }
            return c;
        }

        void OnRbuttonDown(UINT /*code*/, CPoint pt)
        {
            SetFocus();
            CTreeItem item = HitTest(pt, NULL);
            Select(item, TVGN_CARET);
        }

        void OnContextMenu(HWND /*hWnd*/, CPoint pt)
        {
            if (pt.x == -1 || pt.y == -1)
            {
                CTreeItem item = GetSelectedItem();
                if (!item.IsNull())
                {
                    CRect rc;
                    item.GetRect(&rc, TRUE);
                    PopupContextMenu(CPoint(rc.left, rc.bottom));
                }
            }
            else
            {
                ScreenToClient(&pt);
                PopupContextMenu(pt);
            }
        }

        void OnLbuttonDbclk(UINT /*code*/, CPoint pt)
        {
            CTreeItem item = HitTest(pt, NULL);
            component* c = GetItemData(item);

            m_fc->configure(c);
            UpdateDesc(item);

            SetMsgHandled(FALSE);
        }

        component* GetItemData(CTreeItem item)
        {
            if (!IsNormalItem(item))
            {
                return &m_root;
            }
            return reinterpret_cast<component*>(item.GetData());
        }

        bool IsNormalItem(CTreeItem item)
        {
            return !item.IsNull() && item != TVI_ROOT;
        }

        void DeleteComponent(CTreeItem item)
        {
            CTreeItem parentItem = item.GetParent();
            GetItemData(parentItem)->del_child(GetItemData(item));
            DeleteItem(item);
            UpdateDesc(parentItem);
        }

        void CopyToClipboard(component* c)
        {
            if (c)
            {
                if (m_clipboard) delete m_clipboard;
                m_clipboard = c->clone();
            }
        }

        int GetComponentIconIndex(const component* c)
        {
            std::wstring name = c->classname();
            for (size_t i = 0; i < m_cnames.size(); i++)
            {
                if (m_cnames[i] == name) return static_cast<int>(i);
            }
            return -1;
        }

        void InsertComponent(CTreeItem parentItem, CTreeItem item, component* c)
        {
            int index = GetComponentIconIndex(c);
            CTreeItem newItem = InsertItem(GetComponentDisplayText(c).c_str(), index, index, parentItem, item);
            newItem.SetData((DWORD_PTR) c);

            GetItemData(parentItem)->add_child(c);

            for (size_t i = 0; i < c->child_count(); i++)
            {
                InsertComponent(newItem, TVI_LAST, c->get_child(i));
            }

            if (IsNormalItem(parentItem))
            {
                parentItem.Expand();
                UpdateDesc(parentItem);
            }
        }

        void OnCommand(UINT /*code*/, int id, HWND /*hWnd*/)
        {
            if (id >= MENUID_ADD_COMP_BEGIN && id <= MENUID_ADD_COMP_END)
            {
                component* c = CreateAndConfigure(m_cnames[static_cast<size_t>(id - MENUID_ADD_COMP_BEGIN)]);
                if (!c) return;
                CTreeItem item = GetSelectedItem();
                InsertComponent(item, TVI_LAST, c);
            }
            else if (id >= MENUID_CVT_COMP_BEGIN && id <= MENUID_CVT_COMP_END)
            {
                component* c = CreateAndConfigure(m_cnames[static_cast<size_t>(id - MENUID_CVT_COMP_BEGIN)]);
                if (!c) return;
                CTreeItem item = GetSelectedItem();
                CTreeItem parentItem = item.GetParent();
                component* oldc = GetItemData(item);
                for (size_t i = 0; i < oldc->child_count(); i++)
                {
                    c->add_child(oldc->get_child(i)->clone());
                }
                InsertComponent(parentItem, item, c);
                DeleteComponent(item);
            }
            else if (id >= MENUID_ADD_COMP_AS_PARENT_BEGIN && id <= MENUID_ADD_COMP_AS_PARENT_END)
            {
                component* c = CreateAndConfigure(m_cnames[static_cast<size_t>(id - MENUID_ADD_COMP_AS_PARENT_BEGIN)]);
                if (!c) return;
                CTreeItem item = GetSelectedItem();
                CTreeItem parentItem = item.GetParent();
                component* me = GetItemData(item);
                c->add_child(me->clone());
                InsertComponent(parentItem, item, c);
                DeleteComponent(item);
            }
            else if (id == MENUID_DELETE)
            {
                CTreeItem item = GetSelectedItem();
                DeleteComponent(item);
            }
            else if (id == MENUID_CUT)
            {
                CTreeItem item = GetSelectedItem();
                if (!item.IsNull())
                {
                    CopyToClipboard(GetItemData(item));
                    DeleteComponent(item);
                }
            }
            else if (id == MENUID_COPY)
            {
                CTreeItem item = GetSelectedItem();
                if (!item.IsNull())
                {
                    CopyToClipboard(GetItemData(item));
                }
            }
            else if (id == MENUID_PASTE)
            {
                CTreeItem item = GetSelectedItem();
                CTreeItem parentItem = item.GetParent();
                if (m_clipboard && GetItemData(parentItem)->can_add_child(NULL))
                {
                    InsertComponent(parentItem, item, m_clipboard->clone());
                }
            }
            else if (id == MENUID_PASTE_AS_CHILD)
            {
                CTreeItem item = GetSelectedItem();
                if (m_clipboard && GetItemData(item)->can_add_child(NULL))
                {
                    InsertComponent(item, TVI_LAST, m_clipboard->clone());
                }
            }
        }

        void SelectItemIfValid(CTreeItem item)
        {
            if (!item.IsNull()) item.Select(TVGN_CARET);
        }

        void OnKeyDown(UINT ch, UINT /*cnt*/, UINT /*flags*/)
        {
            bool bControlKeyIsDown = KeyIsDown(VK_CONTROL);
            bool bShiftKeyIsDown = KeyIsDown(VK_SHIFT);
            if (ch == VK_DELETE)
            {
                SendMessage(WM_COMMAND, MENUID_DELETE, 0);
            }
            else if (ch == L'C' && bControlKeyIsDown)
            {
                SendMessage(WM_COMMAND, MENUID_COPY, 0);
            }
            else if (ch == L'X' && bControlKeyIsDown)
            {
                SendMessage(WM_COMMAND, MENUID_CUT, 0);
            }
            else if (ch == L'V' && bControlKeyIsDown)
            {
                SendMessage(WM_COMMAND, (WPARAM) (bShiftKeyIsDown ? MENUID_PASTE_AS_CHILD : MENUID_PASTE), 0);
            }
            else if (ch == VK_LEFT)
            {
                CTreeItem item = GetSelectedItem();
                if (item.HasChildren() && item.GetState(TVIS_EXPANDED))
                {
                    item.Expand(TVE_COLLAPSE);
                }
                else
                {
                    SelectItemIfValid(item.GetParent());
                }
            }
            else if (ch == VK_RIGHT)
            {
                CTreeItem item = GetSelectedItem();
                if (item.HasChildren() && !item.GetState(TVIS_EXPANDED))
                {
                    item.Expand(TVE_EXPAND);
                }
                else
                {
                    SelectItemIfValid(item.GetChild());
                }
            }
            else if (ch == VK_DOWN)
            {
                SelectItemIfValid(GetSelectedItem().GetNextVisible());
            }
            else if (ch == VK_UP)
            {
                SelectItemIfValid(GetSelectedItem().GetPrevVisible());
            }
        }

        int Distance(CPoint pt1, CPoint pt2)
        {
            return (pt1.x - pt2.x) * (pt1.x - pt2.x) + (pt1.y - pt2.y) * (pt1.y - pt2.y);
        }

        bool IsDescendant(CTreeItem t, CTreeItem p)
        {
            if (t.IsNull() || p.IsNull()) return false;
            do
            {
                t = t.GetParent();
                if (t == p) return true;
            } while (!t.IsNull());
            return false;
        }

        void DragComponent(CTreeItem src, CTreeItem dest, bool copy)
        {
            if (src == dest || IsDescendant(dest, src))
            {
                return;
            }

            if (GetItemData(dest)->can_add_child(NULL))
            {
                InsertComponent(dest, TVI_LAST, GetItemData(src)->clone());
                if (!copy)
                {
                    DeleteComponent(src);
                }
            }
        }

        void OnLButtonDown(UINT /*code*/, CPoint pt)
        {
            m_lb_down = true;
            UINT flag;
            CTreeItem item = HitTest(pt, &flag);
            if (IsNormalItem(item))
            {
                Select(item, TVGN_CARET);
                SetMsgHandled(FALSE);
                if (flag & TVHT_ONITEM)
                {
                    m_dragging_source = item;
                    m_dragging_target = NULL;
                    m_dragpt = pt;
                }
            }
        }

        void OnMouseMove(UINT /*code*/, CPoint pt)
        {
            if (m_dragging)
            {
                UINT flag;
                CTreeItem item = HitTest(pt, &flag);
                if (item != m_dragging_target)
                {
                    m_dragImgList.DragShowNolock(FALSE);
                    Select(item, TVGN_CARET);
                    m_dragImgList.DragShowNolock(TRUE);
                    m_dragging_target = item;
                }
                m_dragImgList.DragMove(pt);
                return;
            }
            if (m_lb_down && !m_dragging_source.IsNull() && Distance(pt, m_dragpt) >= 100)
            {
                m_dragging = true;
                m_dragImgList = CreateDragImage(m_dragging_source);
                m_dragImgList.BeginDrag(0, CPoint(0, 0));
                m_dragImgList.DragEnter(m_hWnd, pt);
                SetCapture();
            }
        }

        void OnLButtonUp(UINT /*code*/, CPoint /*pt*/)
        {
            m_lb_down = false;
            ReleaseCapture();
            if (m_dragging)
            {
                m_dragImgList.DragLeave(m_hWnd);
                m_dragImgList.EndDrag();
                if (!m_dragging_source.IsNull() && !m_dragging_target.IsNull())
                {
                    DragComponent(m_dragging_source, m_dragging_target, KeyIsDown(VK_CONTROL));
                }
            }
            m_dragging = false;
            m_dragging_source = NULL;
        }
    };
}