#ifndef FTL_SHAREPTR_H
#define FTL_SHAREPTR_H
#pragma once

//#include <ftlThread.h>

//#if defined(WIN32) || defined(WIN64)
    FTLINLINE void FTLInterlockedIncrement(LONG *pLong) //, FTL::CFCriticalSection *)
	{
		::InterlockedIncrement(pLong);
	}
	FTLINLINE LONG FTLInterlockedDecrement(LONG *pLong) //, FTL::CFCriticalSection *)
	{
		return ::InterlockedDecrement(pLong);
	}
//#else
//	FTLINLINE void FTLInterlockedIncrement(LONG *pLong, FTL::CFCriticalSection *pCS)
//	{
//		CFAutoLock<CFLockObject> lock(pCS);
//		++(*pLong);
//	}
//	FTLINLINE LONG FTLInterlockedDecrement(LONG *pLong, FTL::CFCriticalSection *pCS)
//	{
//		CFAutoLock<CFLockObject> lock(pCS);
//		return --(*pLong);
//	}
//#endif


namespace CFSharedPtrDetail
{
//#if defined(WIN32) || defined(WIN64)
//#define FTL_SHARED_PTR_CRITICAL_SECTION(var)
//#define FTL_SHARED_PTR_CRITICAL_SECTION_PTR(var)	0
//#else
//#define FTL_SHARED_PTR_CRITICAL_SECTION(var)		CRITICAL_SECTION	var
//#define FTL_SHARED_PTR_CRITICAL_SECTION_PTR(var)	&var
//#endif

	class _CFSharedCounterBase
	{
	public:
		_CFSharedCounterBase()
		: useCount(1), weakCount(1)
		{
		}
		virtual ~_CFSharedCounterBase()
		{
		}
		void addRef()
		{
			FTLInterlockedIncrement(&useCount); //, FTL_SHARED_PTR_CRITICAL_SECTION_PTR(cs));
		}
		virtual void release() = 0;
		void weakAddRef()
		{
			FTLInterlockedIncrement(&weakCount); //, FTL_SHARED_PTR_CRITICAL_SECTION_PTR(cs));
		}
		virtual void weakRelease() = 0;
		LONG use_count() const
		{
			return useCount;
		}
	protected:
		LONG				useCount;
		LONG				weakCount;
		//FTL_SHARED_PTR_CRITICAL_SECTION(cs);
	};
	template <class T>
	class _FTLSharedCounter :
		public _CFSharedCounterBase
	{
	public:
		_FTLSharedCounter(T* p)
		: ptr(p)
		{
		}
		virtual void release()
		{
			LONG _useCount = FTLInterlockedDecrement(&useCount); //, FTL_SHARED_PTR_CRITICAL_SECTION_PTR(cs));
			if (_useCount == 0)
			{
				delete ptr;
				ptr = 0;
				weakRelease();
			}
		}
		virtual void weakRelease()
		{
			LONG _weakCount = FTLInterlockedDecrement(&weakCount);//, FTL_SHARED_PTR_CRITICAL_SECTION_PTR(cs));
			if (_weakCount == 0)
			{
				FTLASSERT(useCount <= 1);
				delete this;
			}
		}
	private:
		T*					ptr;
	};
	struct tagStaticCast {};
	struct tagDynamicCast {};

#undef FTL_SHARED_PTR_CRITICAL_SECTION
#undef FTL_SHARED_PTR_CRITICAL_SECTION_PTR
}

template <class T> class FTLWeakPtr;

template <class T>
class CFSharePtr
{
public:
	CFSharePtr()
	: ptr(0), ref(new CFSharedPtrDetail::_FTLSharedCounter<T>(0))
	{
	}
	explicit CFSharePtr(T* p) : ptr(p), ref(new CFSharedPtrDetail::_FTLSharedCounter<T>(p))
	{
	}
	CFSharePtr(CFSharePtr const & p)
	: ptr(p.ptr), ref(p.ref)
	{
		ref->addRef();
	}
	template <class Y>
	CFSharePtr(CFSharePtr<Y> const & p)
	: ptr(p.ptr), ref(p.ref)
	{
		ref->addRef();
	}
	template <class Y>
	CFSharePtr(CFSharePtr<Y> const & p, CFSharedPtrDetail::tagStaticCast)
	: ptr(static_cast<T*>(p.ptr)), ref(p.ref)
	{
		ref->addRef();
	}
	template <class Y>
	CFSharePtr(CFSharePtr<Y> const & p, CFSharedPtrDetail::tagDynamicCast)
	: ptr(dynamic_cast<T*>(p.ptr)), ref(p.ref)
	{
		if (ptr == 0)
		{
			ref = new CFSharedPtrDetail::_FTLSharedCounter<T>(0);
		}
		else
		{
			ref->addRef();
		}
	}
	template <class Y>
	explicit CFSharePtr(FTLWeakPtr<Y> const & p)
	: ptr(p.ptr), ref(p.ref)
	{
		if (ptr == 0)
		{
			ref = new CFSharedPtrDetail::_FTLSharedCounter<T>(0);
		}
		else
		{
			ref->addRef();
		}
	}
	~CFSharePtr()
	{
		ref->release();
	}
/*	
	CFSharePtr& operator =(T* p)
	{
		ptr = p;
		ref->release();
		ref = new CFSharedPtrDetail::_FTLSharedCounter<T>(p);
		return *this;
	}
*/
	CFSharePtr& operator =(CFSharePtr const & p)
	{
		ptr = p.ptr;
		CFSharedPtrDetail::_CFSharedCounterBase* tmp = p.ref;
		tmp->addRef();
		ref->release();
		ref = tmp;
		return *this;
	}

	template <class Y>
	CFSharePtr& operator =(CFSharePtr<Y> const & p)
	{
		ptr = p.ptr;
		CFSharedPtrDetail::_CFSharedCounterBase* tmp = p.ref;
		tmp->addRef();
		ref->release();
		ref = tmp;
		return *this;
	}

	T* operator ->() const
	{
		return ptr;
	}
	T& operator *() const
	{
		return *ptr;
	}
	operator bool() const
	{
		return ptr != 0;
	}
	bool operator !() const
	{
		return ptr == 0;
	}

	bool unique() const
	{
		return ref->use_count() == 1;
	}
    void detach()
    {
        ptr = 0;
        ref = new CFSharedPtrDetail::_FTLSharedCounter<T>(0);
    }
	void reset()
	{
		ref->release();
		ptr = 0;
		ref = new CFSharedPtrDetail::_FTLSharedCounter<T>(0);
	}
	T* get() const
	{
		return ptr;
	}

private:
	template<class Y>
	friend class CFSharePtr;
	template<class Y>
	friend class FTLWeakPtr;

	T*											ptr;
	CFSharedPtrDetail::_CFSharedCounterBase*	ref;
};

template<class T, class U>
bool operator ==(CFSharePtr<T> const & a, CFSharePtr<U> const & b)
{
    return a.get() == b.get();
}

template<class T, class Y>
CFSharePtr<T> ftlshared_static_cast(CFSharePtr<Y> const & p)
{
	return CFSharePtr<T>(p, CFSharedPtrDetail::tagStaticCast());
}

template<class T, class Y>
CFSharePtr<T> ftlshared_dynamic_cast(CFSharePtr<Y> const & p)
{
	return CFSharePtr<T>(p, CFSharedPtrDetail::tagDynamicCast());
}

template <class T>
class FTLWeakPtr
{
public:
	FTLWeakPtr()
	: ptr(0), ref(new CFSharedPtrDetail::_FTLSharedCounter<T>(0))
	{
	}
	FTLWeakPtr(FTLWeakPtr const & p)
	: ptr(p.ptr), ref(p.ref)
	{
		if (ref)
		{
			ref->weakAddRef();
		}
	}
	template <class Y>
	FTLWeakPtr(FTLWeakPtr<Y> const & p)
	{
		ptr = p.lock().get();
		if (ptr)
		{
			ref = ptr.ref;
			if (ref)
			{
				ref->weakAddRef();
			}
		}
		else
		{
			ref = new CFSharedPtrDetail::_FTLSharedCounter<T>(0);
		}
	}
	template <class Y>
	FTLWeakPtr(CFSharePtr<Y> const & p)
	: ptr(p.ptr), ref(p.ref)
	{
		if (ref)
		{
			ref->weakAddRef();
		}
	}
	template <class Y>
	FTLWeakPtr& operator =(FTLWeakPtr<Y> const & p)
	{
		ptr = p.lock().get();
		if (ptr)
		{
			CFSharedPtrDetail::_CFSharedCounterBase* tmp = p.ref;
			tmp->weakAddRef();
			ref->weakRelease();
			ref = tmp;
		}
		else
		{
			ref->weakRelease();
			ref = new CFSharedPtrDetail::_FTLSharedCounter<T>(0);
		}
		return *this;
	}
	template <class Y>
	FTLWeakPtr& operator =(CFSharePtr<Y> const & p)
	{
		ptr = p.ptr;
		if (ptr)
		{
			CFSharedPtrDetail::_CFSharedCounterBase* tmp = p.ref;
			tmp->weakAddRef();
			ref->weakRelease();
			ref = tmp;
		}
		else
		{
			ref->weakRelease();
			ref = new CFSharedPtrDetail::_FTLSharedCounter<T>(0);
		}
		return *this;
	}
	CFSharePtr<T> lock() const
	{
		if (ref->use_count() == 0)
		{
			return CFSharePtr<T>();
		}
		else
		{
			return CFSharePtr<T>(*this);
		}
	}
	~FTLWeakPtr()
	{
		ref->weakRelease();
	}

private:
	template<class Y>
	friend class CFSharePtr;
	template<class Y>
	friend class FTLWeakPtr;

	T*											ptr;
	CFSharedPtrDetail::_CFSharedCounterBase*	ref;
};

#endif
