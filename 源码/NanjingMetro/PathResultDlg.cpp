
// PathResultDlg.cpp: 路线规划结果详情对话框的实现（何彦毅 - 1号模块）
//

#include "pch.h"
#include "framework.h"
#include "PathResultDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CPathResultDlg 对话框

CPathResultDlg::CPathResultDlg(const CString& summaryText, const CString& guideText,
	const PathResult& result, CHistoryManager* pHistoryMgr, CWnd* pParent)
	: CDialogEx(IDD_DIALOG_PATH_RESULT, pParent)
	, m_summaryText(summaryText)
	, m_guideText(guideText)
	, m_result(result)
	, m_pHistoryMgr(pHistoryMgr) // 编写者：何彦毅（1号）
{
}

void CPathResultDlg::DoDataExchange(CDataExchange* pDX) // 编写者：何彦毅（1号）
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_RESULT_SUMMARY, m_summaryText);
	DDX_Text(pDX, IDC_RESULT_GUIDE, m_guideText);
}

BEGIN_MESSAGE_MAP(CPathResultDlg, CDialogEx)
	ON_BN_CLICKED(IDC_RESULT_FAV, &CPathResultDlg::OnFavorite)
END_MESSAGE_MAP()

// CPathResultDlg 消息处理程序

BOOL CPathResultDlg::OnInitDialog() // 编写者：何彦毅（1号）
{
	CDialogEx::OnInitDialog();

	// 让只读多行文本框自动换行显示，并避免它作为首个 Tab 控件时全选正文。
	CEdit* pGuide = (CEdit*)GetDlgItem(IDC_RESULT_GUIDE);
	if (pGuide)
	{
		pGuide->SetMargins(4, 4);
		pGuide->SetSel(0, 0);
		pGuide->LineScroll(-pGuide->GetLineCount());
	}

	// 初始焦点放在“关闭”按钮，正文保持普通阅读状态。
	CWnd* pClose = GetDlgItem(IDOK);
	if (pClose)
	{
		pClose->SetFocus();
		return FALSE;
	}

	return TRUE;
}

void CPathResultDlg::OnFavorite() // 编写者：何彦毅（1号）
{
	if (!m_pHistoryMgr)
		return;

	CString startName = m_result.startStationName;
	CString endName = m_result.endStationName;
	if (startName.IsEmpty())
		startName = _T("起点");
	if (endName.IsEmpty())
		endName = _T("终点");

	int startId = m_result.stationSequence.empty() ? -1 : m_result.stationSequence.front();
	int endId = m_result.stationSequence.empty() ? -1 : m_result.stationSequence.back();
	if (startId == endId)
	{
		AfxMessageBox(_T("该路线起终点相同，无法收藏。"), MB_ICONINFORMATION);
		return;
	}

	m_pHistoryMgr->AddRecord(startId, startName, endId, endName,
		m_result.strategy, true);
	m_pHistoryMgr->SaveToFile(_T("user_history.txt"));

	AfxMessageBox(_T("已收藏该路线，可在“我的收藏”中查看。"), MB_ICONINFORMATION);
}
