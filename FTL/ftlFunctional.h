///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   ftlFunctional.h
/// @brief  Fishjam Template Library Functional Header File.
/// @author fujie
/// @version 0.6 
/// @date 03/30/2008
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef FTL_FUNCTIONAL_H
#define FTL_FUNCTIONAL_H
#pragma once

#include <iosfwd>
#include <list>
#include <sstream>
#include "ftlTypes.h"
#include <atlstr.h>
//#include <WinSock2.h>
namespace FTL
{
/*****************************************************************************************************
* 定义类中的成员函数指针
*   定义时：typedef 返回值 (T::* MyFunProc)(参数列表);
*   使用时：(pObj->*pMyFunProc)(参数);
*   常见的使用场景：
*     a.成员函数指针数组(一般用于映射调用)；
*     b.C++ 中实现 Delegate(参见 DuiLib 中的 UIDelegate.h) 等
*     c.类似 MFC 中的消息映射时，通过一个有各种类型函数指针的 union(MessageMapFunctions) 进行函数调用方式转换
*
* 语法
*   operator bool();   -- bool 操作符调用，比如 if( (bool) *pMyObj) { ... }
*   bool operator()(); -- 返回值为 bool 的函数调用，参数个数为0， 如 bRet = （*pMyObj)(); 
*****************************************************************************************************/

    //STL中没有实现 copy_if -- 为什么？
    //这个是书上写的，一个正确实现的copy_if
    template<typename InputIterator,typename OutputIterator,typename Predicate>
    OutputIterator copy_if(InputIterator begin,InputIterator end,OutputIterator destBegin, Predicate p)
    {
        while (begin != end) 
        {
            if (p(*begin))*destBegin++ = *begin;
            ++begin;
        }
        return destBegin;
    }

    //通常用在 set<T*,UnreferenceLess<T*> > 中，否者默认会成为比较指针的地址
    template <typename T>
    struct UnreferenceLess : public std::binary_function<T, T, bool>
    {
        bool operator()(const T& _Left, const T& _Right) const
        {
            return (*_Left < *_Right);
        }
    };

    //使用 for_each 删除容器中保存的对象指针的类(结构) -- 参见 Effective STL 中的条款7
    template <typename T>
    struct ObjectDeleter
    {
        //注意：不要用 T* ptr, 否则 PairDeleter 无法定义为和容器一样的类型进行删除
        void operator()(const T& ptr) const  //通过指针的类型自动实例化一个operator
        {
            delete ptr;
        }
    };

    //空删除标识，一般用于 pair 的 KEY
    template <typename T>
    struct NullDeleter
    {
        void operator()(const T& /*ptr*/) const
        {
            //Do Nothing
        }
    };

    //usage: for_each(map.begin(), map.end(), PairDeleter...) 
    template <
        typename KEY,
        typename VALUE,
        typename KeyDeleter = NullDeleter<KEY>,
        typename ValueDeleter = ObjectDeleter<VALUE>
    >
    struct PairDeleter
    {
        void operator()(const std::pair<KEY,VALUE>& pa) const
        {
            KeyDeleter()(pa.first);
            ValueDeleter()(pa.second);
        }
    };

    //Longest Common Sequence -- 计算最大公共子序列(两个字符串的最长公共序列)
    //http://blog.csdn.net/hackbuteer1/article/details/6686931
    //http://www.codeproject.com/Articles/3697/V-Diff-A-File-Comparer-with-visual-output
    //INT CalculateLongestCommonSequence(LPCTSTR pszX, LPCTSTR pszY, LPTSTR* pStartPos);    

#if 0
#pragma TODO(尝试编写一个带迭代器的容器基类)
	template <typename T, typename C>
	class CFContainerIter
	{
	public:
		CFContainerIter(C& c)
			:m_rContainer(c)
		{

		}
		//BOOL	SetContainer(C& c);
		BOOL	MoveToFirst()
		{
			m_iterator = m_rContainer.begin();
		}
		BOOL	MoveToNext()
		{
			if (m_iterator != m_rContainer.end())
			{
				++m_iterator;
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}
	private:
		C&				m_rContainer;
		C::const_iter	m_iterator;
	};
#endif

    //之前使用了VS中特有的 static _Kfn 函数，但在linux上不通用，因此自己写一个
    template <class K>
    const K& ftl_Kfn(const K& Val)
    {
        return (Val);
    }

    template <class K, class V>
    const K& ftl_Kfn(const std::pair<const K,V> & Val)
    {
        return (Val.first);
    }

    template <class K>
    const K& ftl_Vfn(const K& Val)
    {
        return (Val);
    }

    template <class K, class V>
    const V& ftl_Vfn(const std::pair<const K,V> & Val)
    {
        return (Val.second);
    }

    //引入高效的“添加或更新”功能的模版函数 -- 参见 Effective STL 规则 24
    //使用：
    // iterator affectedPair =	efficientAddOrUpdate(m, k, v);
    //   如果键k不在map m中；高效地把pair(k, v)添加到m中；
    //   否则高效地把和k关联的值更新为v。返回一个指向添加或修改的pair的迭代器
    template<typename MapType,typename KeyArgType,typename ValueArgtype>
    typename MapType::iterator efficientAddOrUpdate(MapType& m,const KeyArgType& k, const ValueArgtype& v)
    {
        //需要能找出k的值是否在map中；如果是这样，那它在哪里；如果不是，它该被插入哪里 -- 使用low_bound的原因
        typename MapType::iterator Ib = m.lower_bound(k);  // 找到k在或应该在哪里；

        if(Ib != m.end() && !(m.key_comp()(k, Ib->first))) // 如果Ib指向一个pair它的键等价于k
        {
            Ib->second = v;	// 更新这个pair的值
            return Ib;      // 返回指向pair的迭代器
        }
        else
        {   //增加
            typedef typename MapType::value_type MVT;
            //使用了insert的“提示”形式 -- 指出需要插入的位置Ib,可以将插入时间分摊为常数时间 -- 而非对数时间
            return m.insert(Ib, MVT(k, v));	// 把pair(k, v)添加到m并返回指向新map元素的迭代器
        }
    }

    //在set或map中根据key查找最接近的元素(复杂度为 O(logN))，通过传入的仿函数(F func)来判断哪个更接近想要的对象
    //如果找到满足条件且最接近的，返回true,且 retIter 为最接近元素的迭代器；如果找不到，返回false
    // F 必须是如下结构的函数指针或仿函数
    //   int operator()(const key_type* pPre, const key_type* pWant, const key_type* pNext)
    //   注意：
    //     1.返回值的意义 : 小于0 表示 pPre 更接近， 等于 0 表示两个都不接近， 大于0 表示后一个接近
    //     2.pPre 和 pNext 都可能为NULL，但 pWant 不可能为NULL

    template<typename C, typename F>
    bool find_nearest(const C& container, 
        const typename C::key_type& want, 
        F func,
        typename C::const_iterator& retIter)
    {
        if(container.empty())
        {
            return false;
        }

        retIter = container.end();
        bool bResult = false;

        //查看是否有想要的数据，或者该插入的位置(相等的或后一个)
        typename C::const_iterator nextIter = container.lower_bound(want);
        typename C::const_iterator preIter = nextIter;

        int retCmp = 0; 
        const typename C::key_type* pPreValue = NULL;
        const typename C::key_type* pNextValue = NULL;

        //找到中间
        if (nextIter != container.end())
        {
            //直接找到等价的
            if(!container.key_comp()(want, ftl_Kfn(*nextIter)))
            {
                bResult = true;
                retIter = nextIter;
                return bResult;
            }

            preIter--;
            if(preIter != container.end())
            {
                pPreValue = &ftl_Kfn(*preIter);
            }
            pNextValue = &ftl_Kfn(*nextIter);
        }
        else
        {
            //在最后，说明没有数据，或者想要的是最大的值
            nextIter = (--container.end());
            pNextValue = &ftl_Kfn(*nextIter);
        }

        retCmp = func(pPreValue,&want,pNextValue);
        if(retCmp < 0)
        {
            bResult = true;
            retIter = preIter;
        }
        else if(retCmp > 0)
        {
            bResult = true;
            retIter = nextIter;
        }
        //else -- 为0,表明没有找到，retIter 和 bResult 之前已经赋好值了

        return bResult;
    }

    ////STL中树的遍历 -- 利用模板参数，可以控制先根、先广等不同的算法
    //template <typename Predicate, template Functor>
    //int ForSubTree(const HATIterater& iRoot, Predicate pred, Functor f){}

    template<typename T>
    FTLINLINE T& ToRef(T * p)
    {
        FTLASSERT(NULL != p);
        return *p;
    }

    template<typename T>
    T& ToRef(const std::auto_ptr<T>& sp)
    {
        FTLASSERT(NULL != sp.get());
        return *(sp.get());
    }

    template<typename T>
    T* ToPtr(T &obj)
    {
        return &obj;
    }


	//删除字符串中的空格(前部分,后部分)
	FTLINLINE std::string& TrimLeft(std::string& text)
	{
		if (text.empty())   
		{  
			return text;  
		}
		text.erase(0, text.find_first_not_of(" "));  
		return text;  
	}

	FTLINLINE std::string& TrimRight(std::string& text)
	{
		if (text.empty())   
		{  
			return text;  
		}  
		text.erase(text.find_last_not_of(" ") + 1);  
		return text;  
	}

	FTLINLINE std::string& Trim(std::string& text)
	{
		TrimLeft(text);
		TrimRight(text);
		return text;
	}

    FTLINLINE size_t Split(const tstring& text, 
        const tstring& delimiter,
        bool bWithDelimeter,
        std::list<tstring>& tokens)
    {
        size_t len = text.length();
        size_t start = text.find_first_not_of(delimiter); //找到第一个不是分隔符的
        size_t stop = 0;
        while ( (start >=0) && (start < len))
        {
            stop = text.find_first_of(delimiter, start); //找到这之后的第一个分隔符
            if( (stop < 0) || stop > len)
            {
                stop = len;
            }
            if (bWithDelimeter && start > 0)
            {
                tokens.push_back(text.substr(start - 1, stop - start + 1));
            }
            else
            {
                tokens.push_back(text.substr(start, stop - start));
            }
            start = text.find_first_not_of(delimiter, stop + 1);
        }
        return tokens.size();
    }

    FTLINLINE size_t Split(const ATL::CAtlString& text, 
        const ATL::CAtlString& delimiter,
        bool bWithDelimeter,
        std::list<ATL::CAtlString>& tokens)
    {
        int len = text.GetLength();
        int start = 0;
        int stop = text.Find(delimiter, start);
        while ((stop = (text.Find(delimiter, start))) == start)
        {
            //ignore init repeat delimiter
            start = stop + 1;
        }

        while (-1 != stop)
        {
            if (bWithDelimeter && start > 0)
            {
                tokens.push_back(text.Mid(start - 1, stop - start + 1));
            }
            else
            {
                tokens.push_back(text.Mid(start, stop - start));
            }
            start = stop + 1;
            while ((stop = (text.Find(delimiter, start))) == start)
            {
                //ignore repeat delimiter
                start = stop + 1;
            }

        }
        if (start >= 0 && start < len)
        {
            tokens.push_back(text.Mid(start));
        }

        return tokens.size();
    }

    FTLINLINE int GetRandomArray(int from, int to, std::vector<int>& result)
    {
        FTLASSERT(from <= to);
        int size = to - from + 1;
        if (size <= 0)
        {
            return 0;
        }

        std::vector<int> tmpVector;
        tmpVector.resize(size);

        //初始化
        for (int i = from; i <= to; i++)
        {
            tmpVector[i - from] = i;
        }
        result.resize(size);

        //time_t now = 0;
        srand(GetTickCount());

        for (int i = 0; i < size; i++)
        {
            int index = rand() % (size-i);
            result[i] = tmpVector[index];
            tmpVector[index] = tmpVector[size - i - 1];
        }
        return size;
    }

	//注意：目前只写了缩小(ZoomOut)的算法，是否应该分为 ZoomIn 和 ZoomOut ?
	template <typename T>
	BOOL ZoomScaleArray(T* pNumberArray, int nSize, T nMaxSize, T nMinValue, T nMaxDeviation = nMinValue / 2, BOOL bFill = TRUE)
	{
        FTLASSERT(nMaxSize >= nMinValue * nSize);
        if (nMaxSize < nMinValue * nSize)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

		T nSrcSize = 0;
		for (int i = 0; i < nSize; i++)
		{
			nSrcSize = nSrcSize + pNumberArray[i];
		}

		T nRemainSize = nMaxSize;
		int nLoopCount = 0;
		do 
		{
			nRemainSize = nMaxSize;
			double ratio = (double)(nMaxSize) / (double)(nSrcSize);

			//int nTargetIndex = 0;
			nSrcSize = 0;
			for (int i = 0; i < nSize; i++)
			{
                pNumberArray[i] = pNumberArray[i] * ratio;
				if (pNumberArray[i] < nMinValue)
				{
					pNumberArray[i] = nMinValue;
				}
				nRemainSize -= pNumberArray[i];
				
				nSrcSize = nSrcSize + pNumberArray[i];
			}
			if (nRemainSize < 0)
			{
				nSrcSize = nSrcSize + nMaxDeviation; //(nMinValue / 2); //误差最多是 MinValue 的一半
			}
			nLoopCount++;
		} while (nRemainSize < 0);

		//ATLASSERT(nLoopCount <= 2);
		FTLTRACE(TEXT("ZoomScaleArray LoopCount=%d\n"), nLoopCount);
		
		if (bFill)
		{
			pNumberArray[nSize-1] += (nMaxSize - nSrcSize);
		}

		return TRUE;
	}
    
    //比较数据的绝对值
    template<typename T>
    struct abs_comparator {
        bool operator() (T i, T j) { 
            return abs(i) < abs(j); 
        }
    };

    //一个序列数生成器，用于 std::generate, 使用方法见 test_generate
    //  std::generate(intVect.begin(), intVect.end(), sequence_generator<int>(1,1));
    //  
    template<typename T>
    struct sequence_generator
    {
    public:
        sequence_generator(const T& _start, const T& _step)
            :m_start(_start)
            ,m_step(_step)
        {

        }
        T operator()()
        {
            T e = m_start;
            m_start += m_step;
            return e;
        }
    private:
        T m_start;
        T m_step;
    };
    template <typename InType, typename OutType, typename Fun>
    struct sequence_generator_ex
    {
    public:
        sequence_generator_ex(const InType& _start, const InType& _step)
            :m_start(_start)
            ,m_step(_step)
        {

        }
        OutType operator()()
        {
            OutType e = Fun(m_start);
            m_start+= m_step;
            return e;
        }
    private:
        InType m_start;
        InType m_step;
    };

    /************************************************************************
    * 信用卡的Luhn算法
    *   数字从右向左每偶数位乘2,未乘2的数字和刚才的结果相加。
    *   一位数字的直接相加，两位数字则分别相加(如 7x2后为14，则相加为 1+4=5，
    *   最后的综合可被 10 整除，即有效
    ************************************************************************/
    FTLINLINE int LuhnCalc(const std::string& strInput)
    {
        UNREFERENCED_PARAMETER(strInput);
        FTLASSERT(FALSE);
        return 0;
    }

    //二进制流输入输出 -- 从 google 的代码改造而来
    class binarystream
    {
    public:
        explicit binarystream(std::ios_base::openmode mode = std::ios_base::out |std::ios_base::in)
            : m_stream(mode) {}
        explicit binarystream(const std::string &str,  
            std::ios_base::openmode mode = std::ios_base::out|std::ios_base::in)  
            : m_stream(str, mode) {}  
        explicit binarystream(const char *str, size_t size,
            std::ios_base::openmode mode = std::ios_base::out|std::ios_base::in)
            : m_stream(std::string(str, size), mode) {}

        binarystream &operator>>(std::string &str)
        {
            u_int32_t length;
            *this >> length;
            if (eof())
            {
                return *this;
            }
            if (length == 0) {
                str.clear();
                return *this;
            }
            std::vector<char> buffer(length);
            m_stream.read(&buffer[0], length);
            if (!eof())
            {
                str.assign(&buffer[0], length);
            }
            return *this;
        }
        binarystream &operator>>(u_int8_t &u8)
        {
            m_stream.read((char *)&u8, 1);
            return *this;
        }
        binarystream &operator>>(u_int16_t &u16)
        {
            u_int16_t temp;
            m_stream.read((char *)&temp, 2);
            if (!eof())
            {
                u16 = temp;
            }
            return *this;
        }
        binarystream &operator>>(u_int32_t &u32)
        {
            u_int32_t temp;
            m_stream.read((char *)&temp, 4);
            if (!eof())
            {
                u32 = temp;
            }
            return *this;
        }
        binarystream &operator>>(u_int64_t &u64)
        {
            u_int32_t lower, upper;
            *this >> lower >> upper;
            if (!eof())
            {
                u64 = static_cast<u_int64_t>(lower) | (static_cast<u_int64_t>(upper) << 32);
            }
            return *this;
        }

        binarystream &operator<<(const std::string &str)
        {
            u_int32_t length = (u_int32_t)(str.length());
            *this <<  length;
            m_stream.write(str.c_str(), length);
            return *this;
        }
        binarystream &operator<<(u_int8_t u8)
        {
            m_stream.write((const char*)&u8, 1);
            return *this;
        }
        binarystream &operator<<(u_int16_t u16)
        {
            m_stream.write((const char*)&u16, 2);
            return *this;
        }
        binarystream &operator<<(u_int32_t u32)
        {
            m_stream.write((const char*)&u32, 4);
            return *this;
        }
        binarystream &operator<<(u_int64_t u64)
        {
            // write 64-bit ints as two 32-bit ints, so we can byte-swap them easily
            u_int32_t lower = static_cast<u_int32_t>(u64 & 0xFFFFFFFF);
            u_int32_t upper = static_cast<u_int32_t>(u64 >> 32);
            *this << lower << upper;
            return *this;
        }

        bool eof() const { return m_stream.eof(); }
        void clear() { m_stream.clear(); }
        std::string str() const { return m_stream.str(); }
        void str(const std::string &s){ m_stream.str(s); }
    
        // Seek both read and write pointers to the beginning of the stream.
        void rewind(){
            m_stream.seekg (0, std::ios::beg);
            m_stream.seekp (0, std::ios::beg);
        }
    private:
        std::stringstream m_stream;
    };

    //布隆过滤器 -- http://googlechinablog.com/2007/07/bloom-filter.html
    //  在巨量的Hash集合中，通过多个不同的 hash 函数制作信息指纹，来是判断一个元素是否存在，可以节约为 1/8~1/4 的空间大小
    //  很长的二进制向量和一系列随机映射函数
    //好处：快速、省空间
    //问题：决不会漏掉，但有极小的可能误判，常见的补救办法为 建立一个白名单
    template <typename T>
    class BloomFilter
    {
    public:
    };
}//namespace FTL

#endif //FTL_FUNCTIONAL_H