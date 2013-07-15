#pragma once

#include <vector>
#include <string>
#include <algorithm>

/// composite design pattern, with serizlization/compare/sort support.

class component;

class component_creator
{
public:
    virtual component* create(const std::wstring& classname) = 0;
    virtual const component* get_prototype(const std::wstring& classname) const = 0;
    virtual bool configure(component* c) = 0;
    virtual ~component_creator() {}
};

class serializer
{
public:
    virtual std::wstring get_property(const std::wstring& name) const = 0;
    virtual std::wstring get_name() const = 0;
    virtual serializer* get_first_child() const = 0;
    virtual serializer* get_next_sibling() const = 0;

    virtual bool set_property(const std::wstring& name, const std::wstring& val) = 0;
    virtual serializer* add_child(const std::wstring& name) = 0;
    virtual ~serializer() {}
};

class component
{
public:
    virtual component* clone(bool deep = true) const = 0;

    virtual std::wstring classname() const = 0;
    virtual std::wstring name() const = 0;
    virtual std::wstring desc() const = 0;

    virtual int compare(const component* c) const = 0;
    virtual component* simplify() const = 0;

    virtual size_t child_count() const = 0;
    virtual component* get_child(size_t index) const = 0;
    virtual bool add_child(component* child) = 0;
    virtual bool del_child(size_t index) = 0;
    virtual bool del_child(component* child) = 0;
    virtual bool can_add_child(component* child) const = 0;
    virtual bool can_del_child(size_t index) const = 0;

    virtual bool load(component_creator* cc, serializer* s) = 0;
    virtual bool save(component_creator* cc, serializer* s) const = 0;

    virtual ~component() {}
};

struct component_less
{
    bool operator()(const component* a, const component* b) const
    {
        return a->compare(b) < 0;
    }
};
struct component_equal
{
    bool operator()(const component* a, const component* b) const
    {
        return a->compare(b) == 0;
    }
};

class composite : public component
{
public:
    virtual size_t child_count() const
    {
        return m_childs.size();
    }
    virtual component* get_child(size_t index) const
    {
        if (index >= m_childs.size()) return 0;
        return m_childs[index];
    }
    virtual bool add_child(component* child)
    {
        if (!can_add_child(child))
        {
            return false;
        }
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
    virtual bool del_child(size_t index)
    {
        if (can_del_child(index) && index < m_childs.size())
        {
            component* c = m_childs[index];
            m_childs.erase(m_childs.begin() + static_cast<int>(index));
            delete c;
            return true;
        }
        return false;
    }
    virtual bool del_child(component* child)
    {
        for (size_t i = 0; i < child_count(); i++)
        {
            if (m_childs[i] == child)
            {
                return del_child(i);
            }
        }

        return false;
    }

    virtual bool can_add_child(component* /*child*/) const
    {
        return true;
    }
    virtual bool can_del_child(size_t /*index*/) const
    {
        return true;
    }
    virtual bool load(component_creator* cc, serializer* s)
    {
        serializer* child_serializer = s->get_first_child();
        while (child_serializer)
        {
            component* child = cc->create(child_serializer->get_name());
            if (child)
            {
                child->load(cc, child_serializer);
                if (can_add_child(child))
                {
                    add_child(child);
                }
            }
            serializer* next_childs = child_serializer->get_next_sibling();
            delete child_serializer;
            child_serializer = next_childs;
        }
        return true;
    }
    virtual bool save(component_creator* cc, serializer* s) const
    {
        serializer* child_serializer = s->add_child(classname());
        for (size_t i = 0; i < child_count(); i++)
        {
            component* c = get_child(i);
            c->save(cc, child_serializer);
        }
        delete child_serializer;
        return true;
    }

    virtual int compare(const component* c) const
    {
        if (c == static_cast<const component*>(this)) return 0;

        const composite* comp = dynamic_cast<const composite*>(c);
        if (!comp)
        {
            // composite > leaf
            return 1;
        }

        int r = name().compare(comp->name());
        if (r != 0) return r;

        return compare_composite(comp);
    }

    virtual int compare_composite(const composite* c) const
    {
        size_t n1 = child_count();
        size_t n2 = c->child_count();
        if (n1 < n2) return -1;
        if (n1 > n2) return 1;

        for (size_t i = 0; i < child_count(); i++)
        {
            component* c1 = get_child(i);
            component* c2 = c->get_child(i);
            int rr = c1->compare(c2);
            if (rr < 0) return -1;
            if (rr > 0) return 1;
        }

        return 0;
    }

    virtual void sort()
    {
        std::sort(m_childs.begin(), m_childs.end(), component_less());
    }
    virtual void uniq()
    {
        for (size_t i = 0;;)
        {
            if (i + 1 >= child_count()) break;

            if (get_child(i)->compare(get_child(i+1)) == 0)
            {
                del_child(i+1);
            }
            else
            {
                i++;
            }
        }
    }

    virtual component* simplify() const
    {
        composite* c = dynamic_cast<composite*>(clone(false));

        // child first
        for (childs_t::const_iterator it = m_childs.begin(); it != m_childs.end(); ++it)
        {
            component* child = (*it)->simplify();
            if (child) c->add_child(child);
        }

        //
        if (c->child_count() == 0)
        {
            delete c;
            return NULL;
        }

        // pass direct
        if (c->child_count() == 1 && c->equal_to_child())
        {
            component* ret = c->get_child(0)->clone();
            delete c;
            return ret;
        }

        // 
        if (compare(c) != 0)
        {
            component* ret = c->simplify();
            delete c;
            return ret;
        }

        return c;
    }

    ~composite()
    {
        for (size_t i = 0; i < child_count(); i++)
        {
            delete get_child(i);
        }
    }

protected:
    typedef std::vector<component*> childs_t;
    childs_t m_childs;

    void clone_childs(childs_t* childs) const
    {
        if (!childs) return;

        childs->clear();
        for (childs_t::const_iterator it = m_childs.begin(); it != m_childs.end(); ++it)
        {
            childs->push_back((*it)->clone());
        }
    }

    virtual bool equal_to_child() const
    {
        return (child_count() == 1);
    }
};

class leaf : public component
{
public:
    virtual size_t child_count() const                      { return 0;     }
    virtual component* get_child(size_t /*index*/) const    { return NULL;  }
    virtual bool add_child(component* /*child*/)            { return false; }
    virtual bool del_child(size_t /*index*/)                { return false; }
    virtual bool del_child(component* /*child*/)            { return false; }
    virtual bool can_add_child(component* /*child*/) const  { return false; }
    virtual bool can_del_child(size_t /*index*/) const      { return false; }

    virtual int compare(const component* c) const
    {
        const leaf* l = dynamic_cast<const leaf*>(c);
        if (!l)
        {
            // leaf < composite
            return -1;
        }

        int r = name().compare(l->name());
        if (r != 0) return r;

        return compare(l);
    }

private:
    virtual int compare(const leaf* l) const = 0;
};
