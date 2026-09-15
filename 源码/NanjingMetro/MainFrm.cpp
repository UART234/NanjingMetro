
// MainFrm.cpp: CMainFrame 类的实现
//

#include "pch.h"
#include "framework.h"
#include "NanjingMetro.h"

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
    ON_WM_GETMINMAXINFO()
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // 状态行指示器
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

// CMainFrame 构造/析构

CMainFrame::CMainFrame() noexcept // 编写者：何彦毅（1号）
{
	// TODO: 在此添加成员初始化代码
}

CMainFrame::~CMainFrame() // 编写者：何彦毅（1号）
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) // 编写者：何彦毅（1号）
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("未能创建工具栏\n");
		return -1;      // 未能创建
	}

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("未能创建状态栏\n");
		return -1;      // 未能创建
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));

	// TODO: 如果不需要可停靠工具栏，则删除这三行
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);


	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs) // 编写者：何彦毅（1号）
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	cs.cx = 1280; cs.cy = 860;
    cs.style &= ~FWS_ADDTOTITLE;
    cs.lpszName = _T("南京地铁 · 站点与出行指南");
	// TODO: 在此处通过修改
	//  CREATESTRUCT cs 来修改窗口类或样式

	return TRUE;
}

// CMainFrame 诊断

#ifdef _DEBUG
void CMainFrame::AssertValid() const // 编写者：何彦毅（1号）
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const // 编写者：何彦毅（1号）
{
	CFrameWnd::Dump(dc);
}
#endif //_DEBUG


// CMainFrame 消息处理程序


void CMainFrame::OnGetMinMaxInfo(MINMAXINFO* info) // 编写者：何彦毅（1号）
{
    CFrameWnd::OnGetMinMaxInfo(info);
    const int dpi = (int)GetDpiForWindow(m_hWnd);
    info->ptMinTrackSize = CPoint(MulDiv(1000, dpi, 96), MulDiv(680, dpi, 96));
}
