#pragma once
#include "common.h"

class NodeDeclear CTask
{
public:
	virtual void Do() = 0;
};

template<typename T, typename RT=void>
class CTask0 : public CTask
{
public:
	typedef RT (T::*F)(void);
public:
	CTask0(T* pObj, F func) :m_pObj(pObj), m_func(func)
	{
	}
	void Do()
	{
		if(m_pObj && m_func)
			(m_pObj->*m_func)();
	}
protected:
	T* m_pObj;
	F m_func;
};

template<typename T, typename ParamT1, typename RT=void>
class CTask1 :public CTask
{
public:
	typedef RT(T::*F)(ParamT1);
public:
	CTask1(T* pObj, F func, ParamT1 param1):m_pObj(pObj), m_func(func), m_param1(param1)
	{
	}
	void Do()
	{
		if (m_pObj && m_func)
			(m_pObj->*m_func)(m_param1);
	}
protected:
	T* m_pObj;
	F m_func;
	ParamT1 m_param1;
};

template<typename T, typename Param1T, typename Param2T, typename RT=void>
class CTask2 : public CTask
{
public:
	typedef RT(T::*F)(Param1T, Param2T);
public:
	CTask2(T* pObj, F func, Param1T param1, Param2T param2):
		m_pObj(pObj),
		m_func(func),
		m_param1(param1),
		m_param2(param2)
	{

	}
	void Do()
	{
		if (m_pObj && m_func)
			(m_pObj->*m_func)(m_param1, m_param2);
	}
protected:
	T* m_pObj;
	F m_func;
	Param1T m_param1;
	Param2T m_param2;
};