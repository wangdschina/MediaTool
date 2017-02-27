#pragma once

#include <d3d9.h>
#include <list>
#include <queue>


const int MAX_MEMORY_POOL = 24;

typedef struct _tagUserData
{
	long lPos;
	unsigned long ulWidth;
	unsigned long ulHeight;
	unsigned long ulYStride;
	unsigned long ulUVStride;
	char* pszData;

	_tagUserData()
	{
		lPos = 0;
		ulWidth = 0;
		ulHeight = 0;
		ulYStride = 0;
		ulUVStride = 0;
		pszData = nullptr;
	}
}USERDATA, *LPUSERDATA;

class CDirect3d
{
public:
	CDirect3d(void);
	~CDirect3d(void);

	int InitD3D( HWND hwnd, unsigned long lWidth, unsigned long lHeight );

	bool Render();

	void Cleanup();
	
	long GetPos();

	int Size(void);

	void Push(USERDATA userData);
	void Clear(void);
	void Front(USERDATA& userData);

private:
	long					m_lPos;
	list<USERDATA>			m_lstBlcokPool;
	queue<USERDATA>			m_queBlockPool;
	

public:
	int	m_nPixWidth;
	int m_nPixHeitht;

private:
	CRITICAL_SECTION	m_critial;

	IDirect3D9*			m_pDirect3D9;
	IDirect3DDevice9*	m_pDirect3DDevice;
	IDirect3DSurface9*	m_pDirect3DSurfaceRender;

	RECT				m_rtViewport;
};

