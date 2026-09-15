#include "pch.h"
#include "framework.h"
#include "NanjingMetro.h"
#include "StationSearchDlg.h"
#include "ServiceTimetable.h"

#include <algorithm>

IMPLEMENT_DYNAMIC(CStationSearchDlg, CDialogEx)

CStationSearchDlg::CStationSearchDlg(const std::vector<StationNode>& stations, CWnd* pParent)
	: CDialogEx(IDD_STATION_SEARCH_DLG, pParent),
	m_selectedStationId(-1), m_stations(stations), m_updating(false) // 编写者：肖博腾（4号）
{
}

void CStationSearchDlg::DoDataExchange(CDataExchange* pDX) // 编写者：肖博腾（4号）
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_STATION_SEARCH, m_editKeyword);
	DDX_Control(pDX, IDC_LIST_STATION_RESULTS, m_listResults);
	DDX_Control(pDX, IDC_LIST_STATION_DETAILS, m_listDetails);
	DDX_Control(pDX, IDC_STATIC_SEARCH_HINT, m_staticHint);
	DDX_Control(pDX, IDC_BUTTON_SELECT_STATION, m_buttonSelect);
}

BEGIN_MESSAGE_MAP(CStationSearchDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_STATION_SEARCH, &CStationSearchDlg::OnBnClickedSearch)
	ON_BN_CLICKED(IDC_BUTTON_SELECT_STATION, &CStationSearchDlg::OnOK)
	ON_EN_CHANGE(IDC_EDIT_STATION_SEARCH, &CStationSearchDlg::OnEnChangeKeyword)
	ON_LBN_SELCHANGE(IDC_LIST_STATION_RESULTS, &CStationSearchDlg::OnLbnSelChangeResults)
	ON_LBN_DBLCLK(IDC_LIST_STATION_RESULTS, &CStationSearchDlg::OnLbnDblClkResults)
END_MESSAGE_MAP()

BOOL CStationSearchDlg::OnInitDialog() // 编写者：肖博腾（4号）
{
	CDialogEx::OnInitDialog();
	std::sort(m_stations.begin(), m_stations.end(),
		[](const StationNode& left, const StationNode& right) { return left.id < right.id; });
	m_buttonSelect.EnableWindow(FALSE);
    m_listResults.SetHorizontalExtent(700);
	RefreshResults();
	m_editKeyword.SetFocus();
	return FALSE;
}

BOOL CStationSearchDlg::PreTranslateMessage(MSG* pMsg) // 编写者：肖博腾（4号）
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	{
		if (GetFocus() == &m_listResults && GetSelectedResultId() >= 0)
			OnOK();
		else if (GetFocus() == &m_editKeyword)
			RefreshResults();
		else
			return CDialogEx::PreTranslateMessage(pMsg);
		return TRUE;
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}

CString CStationSearchDlg::BuildPinyinInitials(const CString& text) const // 编写者：肖博腾（4号）
{
	// GBK 一级汉字区间法：足以覆盖地铁站名，英文与数字原样参与匹配。
	static const int thresholds[] = {
		0xB0A1, 0xB0C5, 0xB2C1, 0xB4EE, 0xB6EA, 0xB7A2, 0xB8C1,
		0xB9FE, 0xBBF7, 0xBFA6, 0xC0AC, 0xC2E8, 0xC4C3, 0xC5B6,
		0xC5BE, 0xC6DA, 0xC8BB, 0xC8F6, 0xCBFA, 0xCDDA, 0xCEF4,
		0xD1B9, 0xD4D1, 0xD7FA
	};
	static const wchar_t initials[] = L"abcdefghjklmnopqrstwxyz";
	CString result;

	for (int i = 0; i < text.GetLength(); ++i)
	{
		const wchar_t value = text[i];
		if ((value >= L'A' && value <= L'Z') || (value >= L'a' && value <= L'z') ||
			(value >= L'0' && value <= L'9'))
		{
			result.AppendChar(static_cast<wchar_t>(towlower(value)));
			continue;
		}

		char gbk[3] = {};
		const int length = WideCharToMultiByte(936, 0, &value, 1, gbk, 2, nullptr, nullptr);
		if (length != 2)
			continue;
		const int code = (static_cast<unsigned char>(gbk[0]) << 8) |
			static_cast<unsigned char>(gbk[1]);
		for (int range = 0; range < 23; ++range)
		{
			if (code >= thresholds[range] && code < thresholds[range + 1])
			{
				result.AppendChar(initials[range]);
				break;
			}
		}
	}
	return result;
}

int CStationSearchDlg::GetMatchRank(const StationNode& station, const CString& keyword) const // 编写者：肖博腾（4号）
{
	if (keyword.IsEmpty())
		return 0;

	CString name = station.name;
	CString query = keyword;
	name.MakeLower();
	query.MakeLower();
	const CString initials = BuildPinyinInitials(station.name);

	if (name == query) return 0;
	if (name.Find(query) == 0) return 1;
	if (initials == query || initials.Find(query) == 0) return 2;
	if (name.Find(query) >= 0) return 3;
	if (initials.Find(query) >= 0) return 4;
    for (const auto& landmark : station.landmarks) {
        CString name = landmark.name; name.MakeLower();
        if (name == query) return 5;
        if (name.Find(query) >= 0 || BuildPinyinInitials(landmark.name).Find(query) >= 0) return 6;
    }
	return -1;
}

void CStationSearchDlg::RefreshResults() // 编写者：肖博腾（4号）
{
	if (m_updating)
		return;
	m_updating = true;
	CString keyword;
	m_editKeyword.GetWindowText(keyword);
	keyword.Trim();

	struct Match
	{
		int rank;
		const StationNode* station;
	};
	std::vector<Match> matches;
	for (const StationNode& station : m_stations)
	{
		const int rank = GetMatchRank(station, keyword);
		if (rank >= 0)
			matches.push_back(Match{ rank, &station });
	}
	std::sort(matches.begin(), matches.end(), [](const Match& left, const Match& right)
	{
		return left.rank < right.rank ||
			(left.rank == right.rank && left.station->id < right.station->id);
	});

	m_listResults.ResetContent();
	for (const Match& match : matches)
	{
		CString item;
		item.Format(_T("%s    [%s]"), match.station->name.GetString(),
			GetLineNames(*match.station).GetString());
		const int index = m_listResults.AddString(item);
		m_listResults.SetItemData(index, static_cast<DWORD_PTR>(match.station->id));
	}

	CString hint;
	if (keyword.IsEmpty())
		hint.Format(_T("未输入关键字：已显示全部 %d 个站点。支持站名、地标及拼音首字母。"),
			static_cast<int>(matches.size()));
	else if (matches.empty())
		hint.Format(_T("没有找到“%s”，请尝试站名、地标或拼音首字母。"), keyword.GetString());
	else
		hint.Format(_T("找到 %d 个结果；单击查看周边，双击直接选中。"), static_cast<int>(matches.size()));
	m_staticHint.SetWindowText(hint);
	m_listDetails.SetWindowText(_T(""));
	if (matches.empty())
		m_listDetails.SetWindowText(_T("暂无可显示的站点详情。"));
	m_buttonSelect.EnableWindow(FALSE);
	m_updating = false;
}

void CStationSearchDlg::OnBnClickedSearch() // 编写者：肖博腾（4号）
{
	RefreshResults();
}

void CStationSearchDlg::OnEnChangeKeyword() // 编写者：肖博腾（4号）
{
	RefreshResults();
}

int CStationSearchDlg::GetSelectedResultId() const // 编写者：肖博腾（4号）
{
	const int selection = m_listResults.GetCurSel();
	if (selection < 0)
		return -1;
	return static_cast<int>(m_listResults.GetItemData(selection));
}

const StationNode* CStationSearchDlg::FindStation(int stationId) const // 编写者：肖博腾（4号）
{
	for (const StationNode& station : m_stations)
		if (station.id == stationId)
			return &station;
	return nullptr;
}

CString CStationSearchDlg::GetLineNames(const StationNode& station) const // 编写者：肖博腾（4号）
{
	CString result;
	for (size_t i = 0; i < station.lineIds.size(); ++i)
	{
		CString name;
		if (station.lineIds[i] == 11) name = _T("S1号线");
		else if (station.lineIds[i] == 13) name = _T("S3号线");
		else name.Format(_T("%d号线"), station.lineIds[i]);
		if (!result.IsEmpty()) result += _T(" / ");
		result += name;
	}
	return result.IsEmpty() ? CString(_T("线路待接入")) : result;
}

CString CStationSearchDlg::GetLandmarkTypeName(LandmarkType type) const // 编写者：肖博腾（4号）
{
	switch (type)
	{
	case LANDMARK_HOTEL: return _T("酒店");
	case LANDMARK_SCENERY: return _T("景点");
	case LANDMARK_HOSPITAL: return _T("医院");
	case LANDMARK_SCHOOL: return _T("学校");
	case LANDMARK_MALL: return _T("商场");
	default: return _T("其他");
	}
}

void CStationSearchDlg::RefreshDetails() // 编写者：肖博腾（4号）
{
	m_listDetails.SetWindowText(_T(""));
	const StationNode* station = FindStation(GetSelectedResultId());
	if (station == nullptr)
	{
		m_buttonSelect.EnableWindow(FALSE);
		return;
	}

    CString details = station->name + (station->isTransfer ? _T("  [换乘车站]\r\n") : _T("  [普通车站]\r\n"));
    details += _T("途经线路：") + GetLineNames(*station) + _T("\r\n\r\n首末班车（按方向，周日至周四）\r\n");
    for (int lineId : station->lineIds) {
        CString lineName; if (lineId == 11) lineName = _T("S1号线"); else if (lineId == 13) lineName = _T("S3号线"); else lineName.Format(_T("%d号线"), lineId);
        details += lineName + _T("\r\n");
        auto rows = CServiceTimetable::Instance().ForStation(station->name, lineId);
        if (rows.empty()) details += _T("  暂无时刻记录，请以车站公告为准。\r\n");
        for (const auto& row : rows) {
            details += _T("  开往 ") + row.direction + _T("\r\n  首 ") + CServiceTimetable::DisplayTime(row.first) + _T("   末 ") + CServiceTimetable::DisplayTime(row.last) + _T("\r\n");
            CString extra=CServiceTimetable::Supplement(row);if(!extra.IsEmpty())details+=_T("  ")+extra+_T("\r\n");
        }
        details += _T("\r\n");
    }
    details += CServiceTimetable::Notice() + _T("\r\n\r\n周边地标与出站导向\r\n");
    if (station->landmarks.empty()) details += _T("暂无周边地标记录。\r\n");
    for (const auto& landmark : station->landmarks) details += _T("[") + GetLandmarkTypeName(landmark.type) + _T("] ") + landmark.name + _T("\r\n  出站指引：") + landmark.description + _T("\r\n\r\n");
    m_listDetails.SetWindowText(details);
    CString hint;
    hint.Format(_T("已选中：%s · 双击或点击“定位到地图”继续。"), station->name.GetString());
    m_staticHint.SetWindowText(hint);
	m_buttonSelect.EnableWindow(TRUE);
}

void CStationSearchDlg::OnLbnSelChangeResults() // 编写者：肖博腾（4号）
{
	RefreshDetails();
}

void CStationSearchDlg::OnLbnDblClkResults() // 编写者：肖博腾（4号）
{
	OnOK();
}

void CStationSearchDlg::OnOK() // 编写者：肖博腾（4号）
{
	const int stationId = GetSelectedResultId();
	if (stationId < 0)
	{
		AfxMessageBox(_T("请先在搜索结果中选择一个站点。"), MB_OK | MB_ICONINFORMATION);
		return;
	}
	m_selectedStationId = stationId;
	CDialogEx::OnOK();
}
