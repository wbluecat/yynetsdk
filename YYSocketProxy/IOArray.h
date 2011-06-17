#pragma once
#include <vector>
using namespace std;

template<typename T>
class CIOArray
{
private:
	vector<T> m_ioArray;
public:
	void	Clear()
	{
		m_ioArray.clear();
	}

	int		Size()
	{
		return m_ioArray.size();
	}

	T*		GetFirst()
	{
		if (m_ioArray.empty() || m_ioArray.size() < sizeof(CMsgHead))
		{
			return NULL;
		}
		return &m_ioArray[0];
	}

	void	PushFront(T*data,int dataLen)
	{
		m_ioArray.insert(m_ioArray.begin(),data,data+dataLen);
	}

	void	PushBack(T*data,int dataLen)
	{
		m_ioArray.insert(m_ioArray.end(),data,data+dataLen);
	}

	void	PopFront(T*data,int dataLen)
	{
		if (dataLen > m_ioArray.size() || m_ioArray.empty())
		{
			printf("ioarray popFront err\n");
			return;
		}

		//for (int i=0;i<dataLen;i++)
		//{
		//	data[i] = m_ioArray[i];
		//}
		memcpy(data,&m_ioArray[0],dataLen);

		m_ioArray.erase(m_ioArray.begin(),m_ioArray.begin()+dataLen);
	}

	void	PopBack(T*data,int dataLen)
	{
		if (dataLen > m_ioArray.size())
		{
			return;
		}

		//for (int i=0;i<dataLen;i++)
		//{
		//	data[i] = m_ioArray[m_ioArray.size() - dataLen + i];
		//}

		memcpy(data,&m_ioArray[m_ioArray.size() - dataLen],dataLen);

		m_ioArray.erase(m_ioArray.end() - dataLen,m_ioArray.end());
	}
public:
	void TestPrintf()
	{
		for (vector<T>::iterator iter = m_ioArray.begin();iter!=m_ioArray.end();++iter)
		{
			printf("%d\n",*iter);
		}
	}
protected:
private:
};