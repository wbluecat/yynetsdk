#include <vector>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>


template<class Obj>
class BasePool
{
	typedef boost::shared_ptr<Obj> Obj_ptr;
	typedef vector<Obj_ptr> Objs;
public:
	~BasePool()
	{
		Clear();
	}

	//也可以不调用Init自行生成对象池
	void Init(int n=10)
	{
		if(!m_objs.empty())
		{
			ERROR_DUMP("Init pool,but isn't empty.");
			return;
		}
		creat(n);
	}

	//获取池中的对象
	Obj_ptr Get()
	{
		boost::mutex::scoped_lock lock(m_mtx);

		if(m_objs.size()<3)
		{
			creat(10);
			ERROR_DUMP("Creat obj.\n");
		}
		Obj_ptr obj_ptr = m_objs[m_objs.size()-1];
		m_objs.pop_back();
		return obj_ptr;
	}

	//检测连接池
	void ChkConnect()
	{
		boost::mutex::scoped_lock lock(m_mtx);
		for(size_t i=0;i<m_objs.size();++i)
		{
			Obj_ptr obj_ptr = m_objs[i];
			try
			{
				obj_ptr->dbsize();
			}
			catch (std::exception& e)
			{
				ERROR_DUMP("ChkConnect Redis connect pool error.\n");
				obj_ptr->SetClosed();
				obj_ptr.reset(new Obj,bind(&BasePool::Put,this,_1));
				m_objs.pop_back();
				m_objs.push_back(obj_ptr);
			}
		}
		//cout << "chking!\n";
	}

	void Del()
	{
		boost::mutex::scoped_lock lock(m_mtx);

		Obj_ptr obj_ptr = m_objs[m_objs.size()-1];
		obj_ptr->SetClosed();
		cout << "reset 1\n";
		obj_ptr.reset(new Obj,bind(&BasePool::Put,this,_1));
		m_objs.pop_back();
		m_objs.push_back(obj_ptr);
		cout << "reset 2\n";
	}

	void Put(Obj * obj)
	{
		if(obj->IsClosed())
		{
			//cout << "socket closed..\n";
			delete obj;
			return;
		}
		//cout << "gc..  " <<"\n";

		Obj_ptr obj_ptr(obj,bind(&BasePool::Put,this,_1));
		boost::mutex::scoped_lock lock(m_mtx);
		m_objs.push_back(obj_ptr);
	}

	void Clear()
	{
		boost::mutex::scoped_lock lock(m_mtx);
		while(!m_objs.empty())
		{
			Obj_ptr &obj_ptr = m_objs[m_objs.size()-1];
			obj_ptr->SetClosed();
			m_objs.pop_back();
		}
	}

	//提取空闲对象池数量
	int GetPoolSize()
	{
		return m_objs.size();
	}

	//优化与整理对象池，清理多余的对象，只留下n个对象
	void Optimize(size_t n)
	{
		boost::mutex::scoped_lock lock(m_mtx);
		for(;n<m_objs.size();n++)
		{
			Obj_ptr &obj_ptr = m_objs[m_objs.size()-1];
			obj_ptr->SetClosed();
			m_objs.pop_back();
		}
	}
private:
	//创建对象
	void creat(int n)
	{
		for(int i=0;i<n;++i)
		{
			Obj_ptr obj_ptr(new Obj,bind(&BasePool::Put,this,_1));//&BasePool::instance()
			m_objs.push_back(obj_ptr);
		}
	}
private:
	boost::mutex m_mtx;
	Objs m_objs;
};