#include "stdafx.h"
#include "SdlWnd.h"


CSdlWnd::CSdlWnd(void) : 
	m_pWnd(nullptr),
	m_pTexture(nullptr),
	m_pRender(nullptr)
{
	
}


CSdlWnd::~CSdlWnd(void)
{
	this->Quit();
}

void CSdlWnd::Quit( void )
{
	//	清理
	if (m_pTexture)
	{
		SDL_DestroyTexture(m_pTexture);
	}
	if (m_pRender)
	{
		SDL_DestroyRenderer(m_pRender);
	}
	if (m_pWnd)
	{
		SDL_DestroyWindow(m_pWnd);
	}
	if (m_frame.data)
	{
		free(m_frame.data);
	}
	SDL_Quit();
}

//Refresh Event
#define REFRESH_EVENT  (SDL_USEREVENT + 1)

int thread_exit=0;

int refresh_video(void *opaque){
	while (thread_exit==0) {
		SDL_Event event;
		event.type = REFRESH_EVENT;
		SDL_PushEvent(&event);
		SDL_Delay(40);
	}
	return 0;
}

int CSdlWnd::Init( void )
{
	m_nScreenWidth = 500;
	m_nScreenHeight = 500;
	m_frame.w = 320;
	m_frame.h = 180;
	m_frame.pixformat = SDL_PIXELFORMAT_RGBA8888;	//	像素格式 SDL_PIXELFORMAT_IYUV
	m_frame.data = (Uint8 *)calloc(m_frame.w*m_frame.h*4, 1); //	一个像素颜色值需要4个字节（32位：r8，g8，b8，a8）

	return SDL_Init(SDL_INIT_VIDEO);
}

void CSdlWnd::FillColorRgba8888()
{
	//r，g，b，a分别占8位，也就是一个字节  
	static Uint8 r = 0, g = 0, b = 0, a = 0;
	//模拟RGBA变化  
	r += 10, g += 15, b += 20, a += 5;
	//将模拟的值赋值给像素数据  
	Uint32 *pdst = (Uint32 *)m_frame.data;
	Uint32 color = (r << 24) | (g << 16) | (b << 8) | a;
	//每个像素一个颜色值  
	for (int i = 0; i < m_frame.w*m_frame.h; i++)
	{  
		*pdst++ = color;
	}  
}

SDL_Window* CSdlWnd::CreateWnd( HWND hWnd )
{
	if (hWnd == INVALID_HANDLE_VALUE)
	{
		return nullptr;
	}

	if (m_pWnd)
	{
		return m_pWnd;
	}

	m_pWnd = SDL_CreateWindowFrom((void*)hWnd);

	return m_pWnd;
}

SDL_Renderer* CSdlWnd::CreateRender()
{
	if (m_pRender == nullptr)
	{
		m_pRender = SDL_CreateRenderer(m_pWnd, -1, 0);
	}
	
	return m_pRender;
}

SDL_Texture* CSdlWnd::CreateTexture()
{
	if (m_pTexture == nullptr)
	{
		m_pTexture = SDL_CreateTexture(m_pRender, m_frame.pixformat, SDL_TEXTUREACCESS_STREAMING, m_frame.w, m_frame.h);
	}

	return m_pTexture;
}
