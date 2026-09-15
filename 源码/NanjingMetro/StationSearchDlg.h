#pragma once

#include "MetroDef.h"

// 可独立交付的单站搜索弹窗。
// 调用方负责传入站点快照；本类不依赖 MetroStub、CMetroData 或寻路模块。
class CStationSearchDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CStationSearchDlg)

public:
	CStationSearchDlg(const std::vector<StationNode>& stations, CWnd* pParent = nullptr);
	enum { IDD = IDD_STATION_SEARCH_DLG };

	// DoModal() 返回 IDOK 后，由调用方读取。
	int m_selectedStationId;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedSearch();
	afx_msg void OnEnChangeKeyword();
	afx_msg void OnLbnSelChangeResults();
	afx_msg void OnLbnDblClkResults();

private:
	void RefreshResults();
	void RefreshDetails();
	int GetSelectedResultId() const;
	int GetMatchRank(const StationNode& station, const CString& keyword) const;
	CString BuildPinyinInitials(const CString& text) const;
	CString GetLineNames(const StationNode& station) const;
	CString GetLandmarkTypeName(LandmarkType type) const;
	const StationNode* FindStation(int stationId) const;

	CEdit m_editKeyword;
	CListBox m_listResults;
	CEdit m_listDetails;
	CStatic m_staticHint;
	CButton m_buttonSelect;
	std::vector<StationNode> m_stations;
	bool m_updating;
};
