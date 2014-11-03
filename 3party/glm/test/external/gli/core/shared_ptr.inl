///////////////////////////////////////////////////////////////////////////
// Interstate Gangs : smart_ptr.inl
///////////////////////////////////////////////////////////////////////////
// This file is under GPL licence
///////////////////////////////////////////////////////////////////////////
// CHANGELOG
// Groove - 13/06/2005 :
// - Create file
///////////////////////////////////////////////////////////////////////////

namespace gli
{
	template <typename T>
	util::CSmartPtr<T>::CSmartPtr()
	{
		m_pPointer = 0;
	}

	template <typename T>
	util::CSmartPtr<T>::CSmartPtr(const util::CSmartPtr<T> & SmartPtr)
	{
		m_pReference = SmartPtr.m_pReference;
		m_pPointer = SmartPtr.m_pPointer;
		(*m_pReference)++;
	}

	template <typename T>
	util::CSmartPtr<T>::CSmartPtr(T* pPointer)
	{
		m_pReference = new int;
		m_pPointer = pPointer;
		(*m_pReference) = 1;
	}

	template <typename T>
	util::CSmartPtr<T>::~CSmartPtr()
	{
		if(!m_pPointer)
			return;

		(*m_pReference)--;
		if(*m_pReference <= 0)
		{
			delete m_pReference;
			delete m_pPointer;
		}
	}

	template <typename T>
	util::CSmartPtr<T>& util::CSmartPtr<T>::operator=(const util::CSmartPtr<T> & SmartPtr)
	{
		if(m_pPointer)
		{
			(*m_pReference)--;
			if(*m_pReference <= 0)
			{
				delete m_pReference;
				delete m_pPointer;
			}
		}

		m_pReference = SmartPtr.m_pReference;
		m_pPointer = SmartPtr.m_pPointer;
		(*m_pReference)++;

		return *this;
	}

	template <typename T>
	util::CSmartPtr<T>& util::CSmartPtr<T>::operator=(T* pPointer)
	{
		if(m_pPointer)
		{
			(*m_pReference)--;
			if(*m_pReference <= 0)
			{
				delete m_pReference;
				delete m_pPointer;
			}
		}

		m_pReference = new int;
		m_pPointer = pPointer;
		(*m_pReference) = 1;

		return *this;
	}

	template <typename T>
	bool util::CSmartPtr<T>::operator==(const util::CSmartPtr<T> & SmartPtr) const
	{
		return m_pPointer == SmartPtr.m_pPointer;
	}

	template <typename T>
	bool util::CSmartPtr<T>::operator!=(const util::CSmartPtr<T> & SmartPtr) const
	{
		return m_pPointer != SmartPtr.m_pPointer;
	}

	template <typename T>
	T& util::CSmartPtr<T>::operator*()
	{
		return *m_pPointer;
	}

	template <typename T>
	T* util::CSmartPtr<T>::operator->()
	{
		return m_pPointer;
	}

	template <typename T>
	const T& util::CSmartPtr<T>::operator*() const
	{
		return *m_pPointer;
	}

	template <typename T>
	const T* util::CSmartPtr<T>::operator->() const
	{
		return m_pPointer;
	}

}//namespace gli
