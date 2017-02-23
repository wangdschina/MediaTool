#pragma once

#include <memory.h>
#include <malloc.h>
#include <assert.h>

#define BYTESTREAM_STACK_ALLOCATION_SIZE 256

class CByteStream
{
public:
	CByteStream::CByteStream()
	{
		m_unByteSizeUsed = 0;
		m_unByteSizeAllocated = BYTESTREAM_STACK_ALLOCATION_SIZE;
		m_unByteReadOffset = 0;
		m_pszData = (unsigned char*)m_szStackData;
		m_bCopyData = true;
	}

	CByteStream::CByteStream(const CByteStream& bitStream)
	{
		m_unByteSizeUsed = bitStream.m_unByteSizeUsed;
		m_unByteSizeAllocated = bitStream.m_unByteSizeAllocated;
		m_unByteReadOffset = 0;
		m_bCopyData = true;
		if (m_unByteSizeAllocated > 0)
		{
			if (m_unByteSizeAllocated <= BYTESTREAM_STACK_ALLOCATION_SIZE)
			{
				m_pszData = (unsigned char*)m_szStackData;
				m_unByteSizeAllocated = BYTESTREAM_STACK_ALLOCATION_SIZE;
			}
			else
			{
				m_pszData = (unsigned char*)malloc(m_unByteSizeAllocated);
				assert(m_pszData != nullptr);
			}
			memcpy(m_pszData, bitStream.m_pszData, m_unByteSizeUsed);
		}
		else
		{
			m_pszData = nullptr;
		}
	}

	CByteStream::CByteStream(unsigned char* pszData, unsigned int unByteSize, bool bCopyData)
	{
		m_unByteSizeUsed = unByteSize;
		m_unByteSizeAllocated = unByteSize;
		m_unByteReadOffset = 0;
		m_bCopyData = bCopyData;

		if (m_bCopyData)
		{
			if (unByteSize > 0)
			{
				if (unByteSize <= BYTESTREAM_STACK_ALLOCATION_SIZE)
				{
					m_pszData = (unsigned char*)m_szStackData;
					m_unByteSizeAllocated = BYTESTREAM_STACK_ALLOCATION_SIZE;
				}
				else
				{
					m_pszData = (unsigned char*)malloc(unByteSize);
					assert(m_pszData != nullptr);
				}
				memcpy(m_pszData, pszData, unByteSize);
			}
			else
			{
				m_pszData = nullptr;
			}
		}
		else
		{
			m_pszData = (unsigned char*)pszData;
		}
	}

	CByteStream::~CByteStream()
	{
		if (m_bCopyData && (m_unByteSizeAllocated > BYTESTREAM_STACK_ALLOCATION_SIZE) && m_pszData)
		{
			free(m_pszData);
		}
	}

	void Reset()
	{
		if (m_bCopyData && (m_unByteSizeAllocated > BYTESTREAM_STACK_ALLOCATION_SIZE) && m_pszData)
		{
			free(m_pszData);
		}

		m_unByteSizeUsed = 0;
		m_unByteSizeAllocated = BYTESTREAM_STACK_ALLOCATION_SIZE;
		m_unByteReadOffset = 0;
		m_pszData = (unsigned char*)m_szStackData;
		m_bCopyData = true;
	}

	void Write(const unsigned char* pszInputByteArray, unsigned int unByteSize)
	{
		if (unByteSize == 0 || pszInputByteArray == nullptr)
		{
			return;
		}

		this->AddBytesAndReallocate(unByteSize);

		memcpy(m_pszData + m_unByteSizeUsed, pszInputByteArray, unByteSize);
		m_unByteSizeUsed += unByteSize;
	}

	bool Read(unsigned char* pszOutByteArray, unsigned int unByteSize)
	{
		if (unByteSize <= 0)
		{
			return false;
		}

		if ((m_unByteReadOffset + unByteSize) > m_unByteSizeUsed)
		{
			return false;
		}

		memcpy(pszOutByteArray, m_pszData + m_unByteReadOffset, unByteSize);
		m_unByteReadOffset += unByteSize;
		return true;
	}

	unsigned char* GetData( void ) const 
	{
		return m_pszData;
	}

	unsigned int GetNumberOfBytesUsed() const
	{
		return m_unByteSizeUsed;
	}

	unsigned int GetReadOffset() const
	{
		return m_unByteReadOffset;
	}

	void SetReadOffset(unsigned int unNewByteReadOffset)
	{
		m_unByteReadOffset = unNewByteReadOffset;
	}

	// 把任意整形类写入bitStream
	template <class T>
	void Write(const T& inTemplateVar)
	{
#ifdef _MSC_VER
#pragma warning(disable:4127)   // conditional expression is constant
#endif
		if (sizeof(inTemplateVar) == 1)
		{
			Write((unsigned char*)&inTemplateVar, sizeof(T));
		}
		else
		{
			if (IsNetworkOrder())
			{
				Write((unsigned char*)&inTemplateVar, sizeof(T));
			}
			else
			{
				int  t = sizeof(T);
				unsigned char szOutput[sizeof(T)];
				ReverseBytes((unsigned char*)&inTemplateVar, szOutput, sizeof(T));
				Write((unsigned char*)szOutput, sizeof(T));
			}
		}
	}

	// 从bitStream读出任意整形类
	template <class T>
	bool Read(T& outTemplateVar)
	{
#ifdef _MSC_VER
#pragma warning(disable:4127)   // conditional expression is constant
#endif
		if (sizeof(outTemplateVar) == 1)
		{
			return Read((unsigned char*)&outTemplateVar, sizeof(T));
		}
		else
		{
#ifdef _MSC_VER
#pragma warning(disable:4244)   // '=' : conversion from 'unsigned long' to 'unsigned short', possible loss of data
#endif
			if (IsNetworkOrder())
			{
				return Read((unsigned char*)&outTemplateVar, sizeof(T));
			}
			else
			{
				unsigned char szOutput[sizeof(T)];
				if (Read((unsigned char*)szOutput, sizeof(T)))
				{
					ReverseBytes(szOutput, (unsigned char*)&outTemplateVar, sizeof(T));
					return true;
				}
				return false;
			}
		}
	}

private:
	void AddBytesAndReallocate(unsigned int unByteSizeToWrite)
	{
		if (unByteSizeToWrite <= 0)
		{
			return;
		}

		unsigned int unNewByteSizeAllocated = unByteSizeToWrite + m_unByteSizeUsed;

		if (unNewByteSizeAllocated > 0 && m_unByteSizeAllocated < unNewByteSizeAllocated)
		{
			// Less memory efficient but saves on news and deletes
			// Cap to 1 meg buffer to save on huge allocations
			unNewByteSizeAllocated = unNewByteSizeAllocated * 2;
			if ((unNewByteSizeAllocated - (unByteSizeToWrite + m_unByteSizeUsed)) > 131072)
			{
				unNewByteSizeAllocated = unByteSizeToWrite + m_unByteSizeUsed + 131072;
			}

			if (m_pszData == (unsigned char*)m_szStackData)
			{
				if (unNewByteSizeAllocated > BYTESTREAM_STACK_ALLOCATION_SIZE)
				{
					m_pszData = (unsigned char*)malloc(unNewByteSizeAllocated);
					assert(m_pszData != nullptr);
					memcpy(m_pszData, m_szStackData, m_unByteSizeAllocated);
				}
			}
			else
			{
				m_pszData = (unsigned char*)realloc(m_pszData, unNewByteSizeAllocated);
				assert(m_pszData != nullptr);
			}
		}

		if (unNewByteSizeAllocated > m_unByteSizeAllocated)
		{
			m_unByteSizeAllocated = unNewByteSizeAllocated;
		}
	}

	bool IsNetworkOrderInit()
	{
		union UNSHORT_ORDER
		{  
			unsigned short value;  
			unsigned char bytes[2];  
		};

		UNSHORT_ORDER unshortOrder;
		unshortOrder.value = 0x1234;  

		if (unshortOrder.bytes[0] == 0x12 && unshortOrder.bytes[1] == 0x34)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	bool IsNetworkOrder()
	{
		static const bool isNetworkOrder = IsNetworkOrderInit();
		return isNetworkOrder;
	}

	void ReverseBytes(unsigned char* pszInByteArray, unsigned char* pszInOutByteArray, unsigned int unByteSize)
	{
		for (unsigned int i = 0; i < unByteSize; i++)
		{
			pszInOutByteArray[i] = pszInByteArray[unByteSize - 1 - i];
		}
	}

	unsigned int m_unByteSizeUsed;
	unsigned int m_unByteSizeAllocated;
	unsigned int m_unByteReadOffset;
	unsigned char* m_pszData;
	unsigned char m_szStackData[BYTESTREAM_STACK_ALLOCATION_SIZE];
	bool m_bCopyData;
};