#pragma once
#include <boost/shared_ptr.hpp>
#include <list>
using namespace std;
#include <boost/ptr_container/ptr_list.hpp>

template<typename T>
class CPool
{
public:
	typedef T* PT;
	typedef boost::ptr_list<T> ptrTList;
public:
	CPool()
	{

	}
	~CPool()
	{

	}
	PT Allocal()
	{
		PT pt = NULL;

		if (m_list.empty())
		{
			pt = new T;
		}
		else
		{
			pt = &m_list.front();
			m_list.pop_front();
		}

		return pt;
	}

	void Release(PT pt)
	{
		m_list.push_back(pt);
	}

protected:
private:
	ptrTList m_list;
};

//CPoolEx

#include <boost/pool/object_pool.hpp>

using namespace boost;

template<class T>
class CPoolEx
{
	typedef T* PT;
public:
	PT Allocal()
	{
		void * mem = m_pool.malloc();
		T*obj = new(mem)T();
		return obj;
	}
	void Release(PT obj)
	{
		m_pool.free(obj);
	}
protected:
private:
	object_pool<T>m_pool;
};