#pragma once

//模拟视频流  
typedef struct _tagFrame
{  
	Uint32  pixformat;	//像素格式  
	int     w, h;		//宽高  
	Uint8   *data;		//像素数据  
}FRAME, *LPFRAME;  


class CSdlWnd
{
public:
	enum E_STATUS
	{
		E_INIT	= 0,
		E_REQ	= 1,
		E_STOP	= 2
	};

public:
	CSdlWnd(void);
	~CSdlWnd(void);

	int Init(void);
	void Quit(void);

	SDL_Window*		CreateWnd(HWND hWnd);	//	创建窗口
	SDL_Renderer*	CreateRender();			//	创建渲染器
	SDL_Texture*	CreateTexture();		//	创建纹理

	//模拟像素数据  
	void FillColorRgba8888();

	//视频显示线程  
	//int SDLCALL video_process(void * data);

public:
	E_STATUS		m_emStatus;
	int				m_nScreenWidth;
	int				m_nScreenHeight;
	FRAME			m_frame;
	SDL_Window*		m_pWnd;
	SDL_Texture*	m_pTexture;
	SDL_Renderer*	m_pRender;
};

