
// PathResultDlg.h: 路线规划结果详情对话框（何彦毅 - 1号模块，结果展示）
//

#pragma once

#include "resource.h"
#include "MetroDef.h"
#include "HistoryManager.h"

class CPathResultDlg : public CDialogEx
{
public:
	// summaryText: 可换行的行程概览文案（由 View 的 ShowRouteResult 拼装）
	// guideText:   分步换乘指引文本
	// result:      原始规划结果（用于收藏时回填历史记录）
	CPathResultDlg(const CString& summaryText, const CString& guideText,
		const PathResult& result, CHistoryManager* pHistoryMgr = nullptr,
		CWnd* pParent = nullptr);

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_PATH_RESULT };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

protected:
	CString           m_summaryText;   // 行程概览
	CString           m_guideText;     // 分步指引
	PathResult        m_result;        // 原始结果
	CHistoryManager*  m_pHistoryMgr;   // 可选: 用于“收藏此路线”

	afx_msg void OnFavorite();

	DECLARE_MESSAGE_MAP()
};
