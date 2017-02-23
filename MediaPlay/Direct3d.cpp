#include "stdafx.h"
#include "Direct3d.h"
#include "MainWnd.h"


//Width, Height
const int pixel_w = 1920, pixel_h = 1080;


const int bpp = 12;
unsigned char buffer[pixel_w*pixel_h*bpp/8];

CDirect3d::CDirect3d(void)
{
	m_pDirect3D9 = nullptr;
	m_pDirect3DDevice = nullptr;
	m_pDirect3DSurfaceRender = nullptr;

	m_lPos = 0;
}


CDirect3d::~CDirect3d(void)
{
	this->Cleanup();
	this->Clear();
}

int CDirect3d::InitD3D( HWND hwnd, unsigned long lWidth, unsigned long lHeight )
{
	InitializeCriticalSection(&m_critial);
	Cleanup();

	m_pDirect3D9 = Direct3DCreate9( D3D_SDK_VERSION );
	if( m_pDirect3D9 == NULL )
		return -1;

	D3DPRESENT_PARAMETERS d3dpp; 
	ZeroMemory( &d3dpp, sizeof(d3dpp) );
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;	
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

	
	GetClientRect(hwnd, &m_rtViewport);

	HRESULT lRet = m_pDirect3D9->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp, &m_pDirect3DDevice );
	if(FAILED(lRet))
		return -1;

	lRet = m_pDirect3DDevice->CreateOffscreenPlainSurface(
		lWidth, lHeight,
		(D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2'),
		D3DPOOL_DEFAULT,
		&m_pDirect3DSurfaceRender,
		NULL);


	if(FAILED(lRet))
		return -1;

	return 0;
}

bool CDirect3d::Render()
{
	HRESULT lRet;
	//Read Data
	//RGB
	//if (fp == nullptr)
	//{
	//	return -1;
	//}
	//if (fread(buffer, 1, pixel_w*pixel_h*bpp/8, fp) != pixel_w*pixel_h*bpp/8)
	//{
	//	// Loop
	//	fseek(fp, 0, SEEK_SET);
	//	fread(buffer, 1, pixel_w*pixel_h*bpp/8, fp);
	//}

	FastMutex::ScopedLock lock(g_pMainWndDlg->m_mciMutex);
	/*list<USERDATA>::iterator it = m_lstBlcokPool.begin();
	if (it == m_lstBlcokPool.end())
	{
		return -1;
	}*/
	if (this->Size() <= 0)
	{
		return -1;
	}
	USERDATA it = m_queBlockPool.front();
	memcpy(buffer, it.pszData, pixel_h*pixel_w*bpp/8);
	delete [] it.pszData;
	m_queBlockPool.pop();
	//delete [] it->pszData;
	//m_lstBlcokPool.erase(it);

	if(m_pDirect3DSurfaceRender == NULL)
		return -1;
	D3DLOCKED_RECT d3d_rect;
	lRet = m_pDirect3DSurfaceRender->LockRect(&d3d_rect, NULL, D3DLOCK_DONOTWAIT);
	if(FAILED(lRet))
		return -1;

	byte *pSrc = buffer;
	byte * pDest = (BYTE *)d3d_rect.pBits;
	int stride = d3d_rect.Pitch;
	unsigned long i = 0;

	//Copy Data
	for(i = 0; i < pixel_h; i++)
	{
		memcpy(pDest + i * stride, pSrc + i * pixel_w, pixel_w);
	}
	for(i = 0; i < pixel_h/2; i++)
	{
		memcpy(pDest + stride * pixel_h + i * stride / 2, pSrc + pixel_w * pixel_h + pixel_w * pixel_h / 4 + i * pixel_w / 2, pixel_w / 2);
	}
	for(i = 0; i < pixel_h/2; i++)
	{
		memcpy(pDest + stride * pixel_h + stride * pixel_h / 4 + i * stride / 2, pSrc + pixel_w * pixel_h + i * pixel_w / 2, pixel_w / 2);
	}

	lRet = m_pDirect3DSurfaceRender->UnlockRect();
	if(FAILED(lRet))
		return -1;

	if (m_pDirect3DDevice == NULL)
		return -1;

	m_pDirect3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0 );
	m_pDirect3DDevice->BeginScene();
	IDirect3DSurface9 * pBackBuffer = NULL;

	m_pDirect3DDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
	m_pDirect3DDevice->StretchRect(m_pDirect3DSurfaceRender, NULL, pBackBuffer, &m_rtViewport, D3DTEXF_LINEAR);
	m_pDirect3DDevice->EndScene();
	m_pDirect3DDevice->Present( NULL, NULL, NULL, NULL );


	return true;
}

void CDirect3d::Cleanup()
{
	EnterCriticalSection(&m_critial);
	if(m_pDirect3DSurfaceRender)
		m_pDirect3DSurfaceRender->Release();
	if(m_pDirect3DDevice)
		m_pDirect3DDevice->Release();
	if(m_pDirect3D9)
		m_pDirect3D9->Release();
	LeaveCriticalSection(&m_critial);
}

long CDirect3d::GetPos(  )
{
	if (this->Size() > 0)
	{
		//return m_lstBlcokPool.begin()->lPos;
		return m_queBlockPool.front().lPos;
	}

	return 0;
}

int CDirect3d::Size( void )
{
	FastMutex::ScopedLock lock(g_pMainWndDlg->m_mciMutex);

	//return std::distance(m_lstBlcokPool.begin(), m_lstBlcokPool.end());
	return m_queBlockPool.size();
}

void CDirect3d::Push( USERDATA userData )
{
	FastMutex::ScopedLock lock(g_pMainWndDlg->m_mciMutex);

	//m_lstBlcokPool.push_back(userData);

	m_queBlockPool.push(userData);
}


void CDirect3d::Clear( void )
{
	FastMutex::ScopedLock lock(g_pMainWndDlg->m_mciMutex);

	/*for (auto it : m_lstBlcokPool)
	{
	delete [] it.pszData;
	}

	m_lstBlcokPool.clear();*/

	while (!m_queBlockPool.empty())
	{
		USERDATA userData = m_queBlockPool.front();
		delete [] userData.pszData;
		m_queBlockPool.pop();
	}
}
