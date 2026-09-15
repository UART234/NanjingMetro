
// NanjingMetroDoc.cpp: CNanjingMetroDoc 类的实现
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS 可以在实现预览、缩略图和搜索筛选器句柄的
// ATL 项目中进行定义，并允许与该项目共享文档代码。
#ifndef SHARED_HANDLERS
#include "NanjingMetro.h"
#endif

#include "NanjingMetroDoc.h"
#include "MetroGraph.h"

namespace
{
CString GetRuntimeFilePath(LPCTSTR fileName) // 编写者：何彦毅（1号）
{
	TCHAR modulePath[MAX_PATH] = {};
	const DWORD length = ::GetModuleFileName(nullptr, modulePath, MAX_PATH);
	if (length == 0 || length >= MAX_PATH)
		return CString(fileName);

	CString path(modulePath);
	const int separator = path.ReverseFind(_T('\\'));
	if (separator >= 0)
		path = path.Left(separator + 1);
	else
		path.Empty();

	path += fileName;
	return path;
}
}

#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CNanjingMetroDoc

IMPLEMENT_DYNCREATE(CNanjingMetroDoc, CDocument)

BEGIN_MESSAGE_MAP(CNanjingMetroDoc, CDocument)
END_MESSAGE_MAP()


// CNanjingMetroDoc 构造/析构

CNanjingMetroDoc::CNanjingMetroDoc() noexcept // 编写者：何彦毅（1号）
{
	// TODO: 在此添加一次性构造代码

}

CNanjingMetroDoc::~CNanjingMetroDoc() // 编写者：何彦毅（1号）
{
}

BOOL CNanjingMetroDoc::OnNewDocument() // 编写者：何彦毅（1号）
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// 运行时文件始终放在可执行文件旁，避免因启动目录不同导致地图空白。
	m_historyMgr.LoadFromFile(GetRuntimeFilePath(_T("user_history.txt")));
	m_historyInitialized = true;
	const CString dataPath = GetRuntimeFilePath(_T("metro_data.txt"));
	if (!m_metroData.LoadDataFromFile(dataPath))
	{
		CString message;
		message.Format(_T("无法加载地铁数据文件：\n%s"), dataPath.GetString());
		AfxMessageBox(message, MB_OK | MB_ICONERROR);
		return FALSE;
	}
	g_graph.BuildGraph(m_metroData);

	return TRUE;
}




// CNanjingMetroDoc 序列化

void CNanjingMetroDoc::Serialize(CArchive& ar) // 编写者：何彦毅（1号）
{
	if (ar.IsStoring())
	{
		// TODO: 在此添加存储代码
	}
	else
	{
		// TODO: 在此添加加载代码
	}
}

#ifdef SHARED_HANDLERS

// 缩略图的支持
void CNanjingMetroDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds) // 编写者：何彦毅（1号）
{
	// 修改此代码以绘制文档数据
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// 搜索处理程序的支持
void CNanjingMetroDoc::InitializeSearchContent() // 编写者：何彦毅（1号）
{
	CString strSearchContent;
	// 从文档数据设置搜索内容。
	// 内容部分应由“;”分隔

	// 例如:     strSearchContent = _T("point;rectangle;circle;ole object;")；
	SetSearchContent(strSearchContent);
}

void CNanjingMetroDoc::SetSearchContent(const CString& value) // 编写者：何彦毅（1号）
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = nullptr;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != nullptr)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CNanjingMetroDoc 诊断

#ifdef _DEBUG
void CNanjingMetroDoc::AssertValid() const // 编写者：何彦毅（1号）
{
	CDocument::AssertValid();
}

void CNanjingMetroDoc::Dump(CDumpContext& dc) const // 编写者：何彦毅（1号）
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CNanjingMetroDoc 命令


void CNanjingMetroDoc::DeleteContents() // 编写者：何彦毅（1号）
{
	// 程序退出前自动保存历史记录和收藏到本地文件。
	// MFC 首次 OnNewDocument 也会调用 DeleteContents；读入前不能覆盖已有收藏。
	if (m_historyInitialized)
		m_historyMgr.SaveToFile(GetRuntimeFilePath(_T("user_history.txt")));
	CDocument::DeleteContents();
}
