
// HistoryDlg.h: 历史记录与收藏夹对话框（何彦毅 - 1号模块）
//

#pragma once

#include "resource.h"
#include "HistoryManager.h"

class CHistoryDlg : public CDialogEx
{
public:
	// 展示模式: 全部历史记录 / 仅收藏
	enum ViewMode
	{
		MODE_HISTORY = 0,
		MODE_FAVORITES
	};

	// DoModal() 的返回码: 用户点击了“直接以此路线查询”
	static const INT_PTR RESULT_QUERY_ROUTE = IDC_HISTORY_QUERY;

	CHistoryDlg(CHistoryManager& historyMgr, ViewMode mode = MODE_HISTORY, CWnd* pParent = nullptr);

	// 当 DoModal() 返回 RESULT_QUERY_ROUTE 时，读取此记录进行后续查询
	UserRouteRecord m_queryRecord;

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_HISTORY };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
    virtual BOOL PreTranslateMessage(MSG* message);

protected:
	CListCtrl          m_listRecords;   // 报表模式历史列表
	CHistoryManager&   m_historyMgr;    // 引用文档级历史/收藏管理器
	ViewMode           m_mode;          // 当前展示模式

	// 内部辅助
	void RefreshList();                              // 按当前模式重建列表
	int  GetSelectedRecordIndex() const;             // 选中行对应的管理器记录下标
	CString FormatStrategy(RouteStrategy strategy) const;
	CString FormatFavorite(bool isFavorite) const;
	void UpdateButtonState();
    bool m_refreshing = false;

	// 消息处理
	afx_msg void OnSelectionChanged(NMHDR* header, LRESULT* result);
    afx_msg void OnToggleFavorite();
	afx_msg void OnQueryRoute();
	afx_msg void OnClearHistory();
	afx_msg void OnItemDoubleClick(NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()
};
