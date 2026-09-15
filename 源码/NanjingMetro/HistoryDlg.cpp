
// HistoryDlg.cpp: 历史记录与收藏夹对话框的实现（何彦毅 - 1号模块）
//

#include "pch.h"
#include "framework.h"
#include "HistoryDlg.h"
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CHistoryDlg 对话框

CHistoryDlg::CHistoryDlg(CHistoryManager& historyMgr, ViewMode mode, CWnd* pParent)
	: CDialogEx(IDD_DIALOG_HISTORY, pParent)
	, m_historyMgr(historyMgr)
	, m_mode(mode) // 编写者：何彦毅（1号）
{
}

void CHistoryDlg::DoDataExchange(CDataExchange* pDX) // 编写者：何彦毅（1号）
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HISTORY_LIST, m_listRecords);
}

BEGIN_MESSAGE_MAP(CHistoryDlg, CDialogEx)
	ON_BN_CLICKED(IDC_HISTORY_TOGGLE_FAV, &CHistoryDlg::OnToggleFavorite)
	ON_BN_CLICKED(IDC_HISTORY_QUERY, &CHistoryDlg::OnQueryRoute)
	ON_BN_CLICKED(IDC_HISTORY_CLEAR, &CHistoryDlg::OnClearHistory)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_HISTORY_LIST, &CHistoryDlg::OnSelectionChanged)
	ON_NOTIFY(NM_DBLCLK, IDC_HISTORY_LIST, &CHistoryDlg::OnItemDoubleClick)
END_MESSAGE_MAP()

// CHistoryDlg 消息处理程序

BOOL CHistoryDlg::OnInitDialog() // 编写者：何彦毅（1号）
{
	CDialogEx::OnInitDialog();

	// 列表控件: 报表模式 + 整行选中 + 网格线
	m_listRecords.SetExtendedStyle(m_listRecords.GetExtendedStyle()
		| LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

	// 列头: 时间 | 起点 | 终点 | 规划方式 | 收藏状态
	m_listRecords.InsertColumn(0, _T("时间"), LVCFMT_LEFT, 135);
	m_listRecords.InsertColumn(1, _T("起点"), LVCFMT_LEFT, 90);
	m_listRecords.InsertColumn(2, _T("终点"), LVCFMT_LEFT, 90);
	m_listRecords.InsertColumn(3, _T("规划方式"), LVCFMT_LEFT, 95);
	m_listRecords.InsertColumn(4, _T("收藏状态"), LVCFMT_LEFT, 80);

	// 标题随模式变化
	if (m_mode == MODE_FAVORITES)
		SetWindowText(_T("我的收藏夹"));

	GetDlgItem(IDC_HISTORY_CLEAR)->ShowWindow(m_mode == MODE_FAVORITES ? SW_HIDE : SW_SHOW);
    CRect listRect; m_listRecords.GetClientRect(listRect);
    int width = listRect.Width() - GetSystemMetrics(SM_CXVSCROLL);
    const int percentages[] = {24,24,24,16,12};
    for (int i=0;i<5;++i) m_listRecords.SetColumnWidth(i, width*percentages[i]/100);
    RefreshList();
    m_listRecords.SetFocus();
    return FALSE;
}

void CHistoryDlg::RefreshList() // 编写者：何彦毅（1号）
{
	// 记住当前选中项对应的记录下标，刷新后尽量恢复
	int selectedIndex = GetSelectedRecordIndex();
    int previousRow = m_listRecords.GetNextItem(-1, LVNI_SELECTED);
    m_refreshing = true;

	m_listRecords.SetRedraw(FALSE);
	m_listRecords.DeleteAllItems();

	const std::vector<UserRouteRecord>& all = m_historyMgr.GetAllRecords();
	int visibleCount = 0;
	for (size_t i = 0; i < all.size(); ++i)
	{
		const UserRouteRecord& rec = all[i];
		// 收藏模式只显示收藏项
		if (m_mode == MODE_FAVORITES && !rec.isFavorite)
			continue;

		LVITEM item = { 0 };
		item.mask = LVIF_TEXT | LVIF_PARAM;
		item.iItem = visibleCount;
		item.iSubItem = 0;
		item.pszText = (LPTSTR)(LPCTSTR)rec.queryTime;
		item.lParam = (LPARAM)(int)i;   // 保存管理器中的真实下标

		int row = m_listRecords.InsertItem(&item);
		m_listRecords.SetItemText(row, 1, rec.startStationName);
		m_listRecords.SetItemText(row, 2, rec.endStationName);
		m_listRecords.SetItemText(row, 3, FormatStrategy(rec.strategy));
		m_listRecords.SetItemText(row, 4, FormatFavorite(m_historyMgr.IsFavorite(rec.startStationId, rec.endStationId, rec.strategy)));

		if ((int)i == selectedIndex)
			m_listRecords.SetItemState(row, LVIS_SELECTED, LVIS_SELECTED);

		++visibleCount;
	}

	if (m_listRecords.GetNextItem(-1, LVNI_SELECTED) < 0 && visibleCount > 0) {
        int row = previousRow < 0 ? 0 : (std::min)(previousRow, visibleCount-1);
        m_listRecords.SetItemState(row, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
        m_listRecords.EnsureVisible(row, FALSE);
    }
    m_refreshing = false;
    m_listRecords.SetRedraw(TRUE);
	m_listRecords.Invalidate();
    UpdateButtonState();
}

int CHistoryDlg::GetSelectedRecordIndex() const // 编写者：何彦毅（1号）
{
	int nItem = m_listRecords.GetNextItem(-1, LVNI_SELECTED);
	if (nItem < 0)
		return -1;
	return (int)m_listRecords.GetItemData(nItem);
}

CString CHistoryDlg::FormatStrategy(RouteStrategy strategy) const // 编写者：何彦毅（1号）
{
	switch (strategy)
	{
	case STRATEGY_SHORTEST_DIST: return _T("最短距离");
	case STRATEGY_MIN_STATIONS:  return _T("最少站点");
	case STRATEGY_MIN_TRANSFERS: return _T("最少换乘");
	default:                     return _T("未知");
	}
}

CString CHistoryDlg::FormatFavorite(bool isFavorite) const // 编写者：何彦毅（1号）
{
	return isFavorite ? _T("★ 已收藏") : _T("未收藏");
}

void CHistoryDlg::UpdateButtonState() // 编写者：何彦毅（1号）
{
    int index = GetSelectedRecordIndex();
    const auto& all = m_historyMgr.GetAllRecords();
    bool selected = index >= 0 && index < (int)all.size();
    GetDlgItem(IDC_HISTORY_TOGGLE_FAV)->EnableWindow(selected);
    GetDlgItem(IDC_HISTORY_QUERY)->EnableWindow(selected);
    bool hasHistory = false; for (const auto& rec : all) if (!rec.isFavorite) hasHistory = true;
    GetDlgItem(IDC_HISTORY_CLEAR)->EnableWindow(hasHistory && m_mode == MODE_HISTORY);
    CString hint;
    if (selected) {
        const auto& rec = all[index];
        bool favorite = m_historyMgr.IsFavorite(rec.startStationId,rec.endStationId,rec.strategy);
        SetDlgItemText(IDC_HISTORY_TOGGLE_FAV, favorite ? _T("取消收藏(&F)") : _T("收藏选中路线(&F)"));
        hint.Format(_T("已选：%s → %s（%s）。点击“查询选中路线”，或双击此行查看乘车方案。"),
            rec.startStationName.GetString(),rec.endStationName.GetString(),FormatStrategy(rec.strategy).GetString());
    } else {
        SetDlgItemText(IDC_HISTORY_TOGGLE_FAV, m_mode == MODE_FAVORITES ? _T("取消收藏(&F)") : _T("收藏选中路线(&F)"));
        hint = m_listRecords.GetItemCount() ? _T("请先单击选中一条路线，再使用下方按钮；双击路线可直接查询。") :
            (m_mode == MODE_FAVORITES ? _T("暂无收藏。先在主界面查询路线，再点击方案底部的“收藏路线”。") : _T("暂无历史记录。先在主界面选择起终点并查询路线。"));
    }
    SetDlgItemText(IDC_HISTORY_HINT,hint);
}

void CHistoryDlg::OnSelectionChanged(NMHDR*, LRESULT* result) // 编写者：何彦毅（1号）
{
    *result = 0;
    if (!m_refreshing) UpdateButtonState();
}

BOOL CHistoryDlg::PreTranslateMessage(MSG* message) // 编写者：何彦毅（1号）
{
    if (message->message == WM_KEYDOWN && message->wParam == VK_RETURN && GetFocus() == &m_listRecords) {
        OnQueryRoute(); return TRUE;
    }
    return CDialogEx::PreTranslateMessage(message);
}

void CHistoryDlg::OnToggleFavorite() // 编写者：何彦毅（1号）
{
	int index = GetSelectedRecordIndex();
	if (index < 0)
		return;

	m_historyMgr.ToggleFavorite(index);
	if (!m_historyMgr.SaveToFile(_T("user_history.txt"))) {
        m_historyMgr.ToggleFavorite(index);
        AfxMessageBox(_T("收藏未能保存，请检查程序目录是否可写。"), MB_ICONWARNING);
    }
    RefreshList();
	UpdateButtonState();
}

void CHistoryDlg::OnQueryRoute() // 编写者：何彦毅（1号）
{
	int index = GetSelectedRecordIndex();
	if (index < 0)
		return;

	m_queryRecord = m_historyMgr.GetAllRecords()[index];
	EndDialog(RESULT_QUERY_ROUTE);
}

void CHistoryDlg::OnClearHistory() // 编写者：何彦毅（1号）
{
	if (m_listRecords.GetItemCount() <= 0)
		return;

	if (AfxMessageBox(_T("确定要清空全部历史记录吗？收藏的路线会保留。"),
		MB_YESNO | MB_ICONQUESTION) == IDYES)
	{
		m_historyMgr.ClearHistory();
		m_historyMgr.SaveToFile(_T("user_history.txt"));
		RefreshList();
		UpdateButtonState();
	}
}

void CHistoryDlg::OnItemDoubleClick(NMHDR* pNMHDR, LRESULT* pResult) // 编写者：何彦毅（1号）
{
	// 仅双击实际条目时查询，空白区双击不会误触发。
    if (reinterpret_cast<NMITEMACTIVATE*>(pNMHDR)->iItem >= 0) OnQueryRoute();
	*pResult = 0;
}
