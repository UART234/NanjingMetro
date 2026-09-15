
// NanjingMetroView.cpp: CNanjingMetroView 类的实现
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS 可以在实现预览、缩略图和搜索筛选器句柄的
// ATL 项目中进行定义，并允许与该项目共享文档代码。
#ifndef SHARED_HANDLERS
#include "NanjingMetro.h"
#endif

#include "NanjingMetroDoc.h"
#include "NanjingMetroView.h"
#include "HistoryDlg.h"
#include "MetroGraph.h"
#include "PathResultDlg.h"
#include "StationSearchDlg.h"
#include "ServiceTimetable.h"
#include "MapLayout.h"
#include "MapDrawing.h"
#include <algorithm>
#include <set>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CNanjingMetroView

IMPLEMENT_DYNCREATE(CNanjingMetroView, CView)

BEGIN_MESSAGE_MAP(CNanjingMetroView, CView)
	ON_COMMAND(ID_ROUTE_SHORTEST, &CNanjingMetroView::OnRouteShortest)
	ON_COMMAND(ID_ROUTE_MIN_STATION, &CNanjingMetroView::OnRouteMinStation)
	ON_COMMAND(ID_ROUTE_MIN_TRANSFER, &CNanjingMetroView::OnRouteMinTransfer)
	ON_COMMAND(ID_QUERY_STATION, &CNanjingMetroView::OnQueryStation)
	ON_COMMAND(ID_QUERY_LANDMARK, &CNanjingMetroView::OnQueryLandmark)
	ON_COMMAND(ID_FAV_VIEW, &CNanjingMetroView::OnFavView)
	ON_COMMAND(ID_HISTORY_LIST, &CNanjingMetroView::OnHistoryList)
	ON_COMMAND(ID_HISTORY_CLEAR, &CNanjingMetroView::OnHistoryClear)
	ON_COMMAND(ID_VIEW_RESET_MAP, &CNanjingMetroView::OnViewResetMap)
	ON_COMMAND(ID_VIEW_THEME_TOGGLE, &CNanjingMetroView::OnViewThemeToggle)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_CAPTURECHANGED()
	ON_WM_SETCURSOR()
	ON_WM_MOUSEWHEEL()
	ON_WM_ERASEBKGND()
    ON_WM_SIZE()
    ON_WM_VSCROLL()
    ON_COMMAND_RANGE(5001, 5010, &CNanjingMetroView::OnUiAction)
    ON_CBN_SELCHANGE(5020, &CNanjingMetroView::OnEndpointChanged)
    ON_CBN_SELCHANGE(5021, &CNanjingMetroView::OnEndpointChanged)
END_MESSAGE_MAP()

// CNanjingMetroView 构造/析构

CNanjingMetroView::CNanjingMetroView() noexcept
	: m_zoom(1.0), m_panOffset(0, 0), m_dragStartPoint(0, 0), m_dragStartPan(0, 0),
	m_dragging(false), m_dragMoved(false), m_darkTheme(false),
	m_selectedStationId(-1), m_hoverStationId(-1),
	m_routeStartStationId(-1), m_routeEndStationId(-1), m_drawScale(1.0),
	m_minDataX(0.0), m_minDataY(0.0), m_drawOriginX(0), m_drawOriginY(0) // 编写者：何彦毅（1号）
{
	// TODO: 在此处添加构造代码

}

CNanjingMetroView::~CNanjingMetroView() // 编写者：何彦毅（1号）
{
}

BOOL CNanjingMetroView::PreCreateWindow(CREATESTRUCT& cs) // 编写者：何彦毅（1号）
{
	// TODO: 在此处通过修改
	//  CREATESTRUCT cs 来修改窗口类或样式

	cs.style |= WS_CLIPCHILDREN;
	return CView::PreCreateWindow(cs);
}

// CNanjingMetroView 绘图

void CNanjingMetroView::OnDraw(CDC* pDC) // 编写者：刘子瑜（3号）
{
	CNanjingMetroDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	CRect rcClient;
	GetClientRect(&rcClient);

	const CMetroData& data = pDoc->m_metroData;
	if (data.m_stations.empty())
	{
		pDC->FillSolidRect(rcClient, RGB(255, 255, 255));
		return;
	}

	// 所有内容先绘制到内存位图，最后一次性复制到窗口，避免拖动和缩放时闪烁。
	CDC* screenDC = pDC;
	CDC memoryDC;
	CBitmap backBuffer;
	CBitmap* oldBackBuffer = nullptr;
	const bool useBuffer = !pDC->IsPrinting() && rcClient.Width() > 0 && rcClient.Height() > 0 &&
		memoryDC.CreateCompatibleDC(pDC) &&
		backBuffer.CreateCompatibleBitmap(pDC, rcClient.Width(), rcClient.Height());
	if (useBuffer)
	{
		oldBackBuffer = memoryDC.SelectObject(&backBuffer);
		pDC = &memoryDC;
	}

	// 1. 计算全网包围盒
	auto itFirst = data.m_stations.begin();
	int minX = itFirst->second.pos.x;
	int minY = itFirst->second.pos.y;
	int maxX = minX;
	int maxY = minY;
	for (const auto& pair : data.m_stations)
	{
		const StationNode& station = pair.second;
		minX = __min(minX, station.pos.x);
		minY = __min(minY, station.pos.y);
		maxX = __max(maxX, station.pos.x);
		maxY = __max(maxY, station.pos.y);
	}

	const COLORREF pageColor = m_darkTheme ? RGB(24, 29, 36) : RGB(241, 245, 249);
	const COLORREF mapColor = m_darkTheme ? RGB(34, 41, 49) : RGB(255, 255, 255);
	pDC->FillSolidRect(rcClient, pageColor);
	pDC->SetBkMode(TRANSPARENT);

	// 4号：地图和侧栏共享布局计算，控件与绘制区域不会相互覆盖。
	pDC->FillSolidRect(m_mapViewport, mapColor);

	const int margin = Ui(38);
	const int drawW = __max(m_mapViewport.Width() - margin * 2, 1);
	const int drawH = __max(m_mapViewport.Height() - margin * 2, 1);
	const double rangeX = __max(maxX - minX, 1);
	const double rangeY = __max(maxY - minY, 1);
	const double baseScale = __min((double)drawW / rangeX, (double)drawH / rangeY);
	m_drawScale = baseScale * m_zoom;
	m_minDataX = minX;
	m_minDataY = minY;
	m_drawOriginX = m_mapViewport.left + (m_mapViewport.Width() - (int)(rangeX * m_drawScale)) / 2 + m_panOffset.x;
	m_drawOriginY = m_mapViewport.top + (m_mapViewport.Height() - (int)(rangeY * m_drawScale)) / 2 + m_panOffset.y;

	auto toScreen = [&](const CPoint& pos) -> CPoint
	{
		return StationToScreen(pos);
	};

	const int savedDC = pDC->SaveDC();
	pDC->IntersectClipRect(m_mapViewport);

	DrawGeography(pDC);
    // 底图和规划高亮共用官方示意折点，反向查询自动反转折点顺序。
    auto drawSegment = [&](int from, int to, COLORREF color, float width) {
        auto a=data.m_stations.find(from), b=data.m_stations.find(to);
        if(a==data.m_stations.end() || b==data.m_stations.end())return;
        auto points=MapSegmentPoints(from,to,a->second.pos,b->second.pos);
        for(auto& point:points)point=toScreen(point);
        DrawMapStroke(pDC,points,color,width);
    };
    for(const auto& line:data.m_lines)
        for(size_t i=1;i<line.stationIds.size();++i)
            drawSegment(line.stationIds[i-1],line.stationIds[i],line.color,(float)Ui(4));
    for(size_t i=1;i<m_highlightStationIds.size();++i)
        drawSegment(m_highlightStationIds[i-1],m_highlightStationIds[i],RGB(255,108,24),(float)Ui(6));

	// 4. 第三层：绘制站点标记
	for (const auto& pair : data.m_stations)
	{
		const StationNode& station = pair.second;
		CPoint pt = toScreen(station.pos);
		bool highlighted = std::find(m_highlightStationIds.begin(),
			m_highlightStationIds.end(), station.id) != m_highlightStationIds.end();
		const bool selected = station.id == m_selectedStationId;
		const bool hovered = station.id == m_hoverStationId;
		const bool routeStart = station.id == m_routeStartStationId;
		const bool routeEnd = station.id == m_routeEndStationId;
		const int radius = Ui((routeStart || routeEnd) ? 7 :
			(selected ? 6 : (station.isTransfer ? 4 : (hovered ? 4 : 2))));
		const COLORREF fillColor = routeStart ? RGB(218, 248, 229) :
			(routeEnd ? RGB(255, 226, 226) : (selected ? RGB(255, 241, 214) :
				(highlighted ? RGB(255, 184, 72) : (hovered ? RGB(255, 236, 145) : mapColor))));
		const COLORREF ringColor = routeStart ? RGB(18, 145, 78) :
			(routeEnd ? RGB(209, 55, 55) : (selected ? RGB(239, 112, 24) :
				(highlighted ? RGB(220, 100, 0) :
					(m_darkTheme ? RGB(235, 239, 244) : RGB(40, 48, 56)))));

		CBrush stationBrush(fillColor);
		CPen stationPen(PS_SOLID, (selected || routeStart || routeEnd) ? 3 :
			(station.isTransfer ? 2 : 1), ringColor);
		CBrush* pOldBrush = pDC->SelectObject(&stationBrush);
		CPen* pOldPen = pDC->SelectObject(&stationPen);
		pDC->Ellipse(pt.x - radius, pt.y - radius, pt.x + radius + 1, pt.y + radius + 1);
		pDC->SelectObject(pOldPen);
		pDC->SelectObject(pOldBrush);
		if (routeStart || routeEnd)
		{
			CFont* previous = pDC->SelectObject(&m_uiFont);
			pDC->SetTextColor(ringColor);
			CRect pin(pt.x - Ui(10), pt.y - Ui(30), pt.x + Ui(10), pt.y - Ui(10));
			pDC->FillSolidRect(pin, mapColor);
			pDC->DrawText(routeStart ? _T("起") : _T("终"), pin, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			pDC->SelectObject(previous);
		}
	}

	// 5. 第四层：站名避让与渲染
	std::map<int, int> lineOrder;
	std::map<int, bool> isHorizontalStation;

	for (const MetroLine& line : data.m_lines)
	{
		const size_t count = line.stationIds.size();
		for (size_t j = 0; j < count; ++j)
		{
			const int id = line.stationIds[j];
			if (lineOrder.find(id) == lineOrder.end())
				lineOrder[id] = (int)j;

			auto it = data.m_stations.find(id);
			if (it == data.m_stations.end())
				continue;

			int dx = 0, dy = 0;
			if (j + 1 < count)
			{
				auto nextIt = data.m_stations.find(line.stationIds[j + 1]);
				if (nextIt != data.m_stations.end())
				{
					dx += abs(nextIt->second.pos.x - it->second.pos.x);
					dy += abs(nextIt->second.pos.y - it->second.pos.y);
				}
			}
			if (j > 0)
			{
				auto prevIt = data.m_stations.find(line.stationIds[j - 1]);
				if (prevIt != data.m_stations.end())
				{
					dx += abs(it->second.pos.x - prevIt->second.pos.x);
					dy += abs(it->second.pos.y - prevIt->second.pos.y);
				}
			}
			if (dx > dy * 1.2) // 显著水平走向
				isHorizontalStation[id] = true;
		}
	}

	struct StationObstacle
	{
		int stationId;
		CRect rect;
	};
	std::vector<StationObstacle> obstacles;
	obstacles.reserve(data.m_stations.size());
	for (const auto& pair : data.m_stations)
	{
		CPoint pt = toScreen(pair.second.pos);
		int r = Ui(pair.second.isTransfer ? 4 : 2);
		obstacles.push_back({ pair.first, CRect(pt.x - r - 2, pt.y - r - 2, pt.x + r + 3, pt.y + r + 3) });
		if (pair.first == m_routeStartStationId || pair.first == m_routeEndStationId)
			obstacles.push_back({ -1, CRect(pt.x - Ui(10), pt.y - Ui(30), pt.x + Ui(10), pt.y - Ui(10)) });
	}

	std::vector<CRect> placedBoxes{CRect(m_mapViewport.right-Ui(104),m_mapViewport.bottom-Ui(56),m_mapViewport.right,m_mapViewport.bottom)};
	// 官方图端点色签，S1/S3 共用南京南站的一端省略，避免枢纽重复堆叠。
	CFont badgeFont;
	badgeFont.CreateFont(-Ui(12),0,0,0,FW_BOLD,FALSE,FALSE,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,_T("Microsoft YaHei UI"));
	CFont* beforeBadgeFont=pDC->SelectObject(&badgeFont);
	for(const auto& line:data.m_lines) {
		if(line.stationIds.empty())continue;
		CString number=line.lineName;number.Replace(_T("号线"),_T(""));
		for(int end=0;end<2;++end) {
			if(end==0 && (line.lineId==11 || line.lineId==13))continue;
			int id=end?line.stationIds.back():line.stationIds.front();
			CPoint pt=toScreen(data.m_stations.at(id).pos);
			int width=Ui(number.GetLength()>1?27:20),height=Ui(20);
			CRect badge(pt.x-width/2,pt.y+Ui(10),pt.x-width/2+width,pt.y+Ui(10)+height);
			if((end==0&&line.lineId==1) || (end==1&&(line.lineId==2||line.lineId==4)))badge.OffsetRect(0,-height-Ui(20));
			else if(end==0)badge=CRect(pt.x-Ui(10)-width,pt.y-height/2,pt.x-Ui(10),pt.y-height/2+height);
			pDC->FillSolidRect(badge,line.color);pDC->SetTextColor(RGB(255,255,255));
			pDC->DrawText(number,badge,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
			badge.InflateRect(Ui(3),Ui(3));placedBoxes.push_back(badge);
		}
	}
	pDC->SelectObject(beforeBadgeFont);
	std::set<int> terminalStationIds;
	for (const MetroLine& line : data.m_lines)
	{
		if (!line.stationIds.empty())
		{
			terminalStationIds.insert(line.stationIds.front());
			terminalStationIds.insert(line.stationIds.back());
		}
	}
	// 初始视图也尝试显示普通站名；空间不足时避让，放大后补显。

	auto intersects = [](const CRect& a, const CRect& b) -> bool {
		return a.left < b.right && a.right > b.left && a.top < b.bottom && a.bottom > b.top;
	};

	auto checkFit = [&](const CRect& rc, int selfId) -> bool {
		if (rc.left < m_mapViewport.left || rc.top < m_mapViewport.top ||
			rc.right > m_mapViewport.right || rc.bottom > m_mapViewport.bottom)
			return false;

		for (const auto& obs : obstacles)
		{
			if (obs.stationId != selfId && intersects(rc, obs.rect))
				return false;
		}
		for (const auto& placed : placedBoxes)
		{
			CRect pad = placed;
			pad.InflateRect(2, 1);
			if (intersects(rc, pad))
				return false;
		}
		return true;
	};

	CFont labelFont;
	labelFont.CreatePointFont(85, _T("微软雅黑"), pDC);
	CFont* pOldFont = pDC->SelectObject(&labelFont);
	pDC->SetBkMode(TRANSPARENT);
	pDC->SetTextColor(m_darkTheme ? RGB(224, 230, 236) : RGB(40, 40, 40));

    std::vector<std::pair<int, StationNode>> labelStations(data.m_stations.begin(), data.m_stations.end());
    auto priority = [&](int id) {
        if (id == m_hoverStationId || id == m_selectedStationId) return 0;
        if (id == m_routeStartStationId || id == m_routeEndStationId) return 1;
        if (std::find(m_highlightStationIds.begin(), m_highlightStationIds.end(), id) != m_highlightStationIds.end()) return 2;
        if(data.m_stations.at(id).isTransfer || terminalStationIds.count(id))return 3;
        return 4;
    };
    std::stable_sort(labelStations.begin(), labelStations.end(), [&](const auto& a, const auto& b) { return priority(a.first) < priority(b.first); });
	for (const auto& pair : labelStations)
	{
		const StationNode& station = pair.second;
		const int id = pair.first;
		const CPoint pt = toScreen(station.pos);
		CSize ext = pDC->GetTextExtent(station.name);
		const int w = ext.cx;
		const int h = ext.cy;

		std::vector<CRect> candidates;

		// --- 规则一：核心密集枢纽强制人工定向优先 ---
		if (id == 8) // 南京站：向右上方引导
		{
			candidates.push_back(CRect(pt.x + 10, pt.y - h - 4, pt.x + 10 + w, pt.y - 4));
		}
		else if (id == 21) // 南京南站：向右下方引导
		{
			candidates.push_back(CRect(pt.x + 12, pt.y + 4, pt.x + 12 + w, pt.y + 4 + h));
		}
		else if (id == 13) // 新街口：向左侧引导
		{
			candidates.push_back(CRect(pt.x - w - 10, pt.y - h / 2, pt.x - 10, pt.y + h / 2));
		}
		// --- 规则二：S1 号线（机场线）平行南下，统一靠左排布，彻底避开右侧的 3 号线 ---
		else if (id >= 57 && id <= 64)
		{
			candidates.push_back(CRect(pt.x - w - 8, pt.y - h / 2, pt.x - 8, pt.y + h / 2));
			candidates.push_back(CRect(pt.x - w - 8, pt.y - h - 2, pt.x - 8, pt.y - 2));
		}
		// --- 规则三：东西向水平线路（S3段、江北段、大学城段），按次序交错上下排布 ---
		else if (isHorizontalStation.find(id) != isHorizontalStation.end())
		{
			int order = lineOrder.count(id) ? lineOrder[id] : id;
			bool top = (order % 2 == 0);
			if (top)
			{
				candidates.push_back(CRect(pt.x - w / 2, pt.y - h - 6, pt.x - w / 2 + w, pt.y - 6));
				candidates.push_back(CRect(pt.x - w / 2, pt.y + 8, pt.x - w / 2 + w, pt.y + 8 + h));
			}
			else
			{
				candidates.push_back(CRect(pt.x - w / 2, pt.y + 8, pt.x - w / 2 + w, pt.y + 8 + h));
				candidates.push_back(CRect(pt.x - w / 2, pt.y - h - 6, pt.x - w / 2 + w, pt.y - 6));
			}
		}
		// --- 规则四：普通南北向站点默认靠右，退避方案包含左、上、下 ---
		else
		{
			candidates.push_back(CRect(pt.x + 8, pt.y - h / 2, pt.x + 8 + w, pt.y + h / 2));
			candidates.push_back(CRect(pt.x - w - 8, pt.y - h / 2, pt.x - 8, pt.y + h / 2));
			candidates.push_back(CRect(pt.x - w / 2, pt.y - h - 6, pt.x - w / 2 + w, pt.y - 6));
			candidates.push_back(CRect(pt.x - w / 2, pt.y + 8, pt.x - w / 2 + w, pt.y + 8 + h));
		}

		// 补齐其余防碰撞退避方向
		candidates.push_back(CRect(pt.x + 8, pt.y - h - 2, pt.x + 8 + w, pt.y - 2));
		candidates.push_back(CRect(pt.x + 8, pt.y + 4, pt.x + 8 + w, pt.y + 4 + h));
		candidates.push_back(CRect(pt.x - w - 8, pt.y - h - 2, pt.x - 8, pt.y - 2));
		candidates.push_back(CRect(pt.x - w - 8, pt.y + 4, pt.x - 8, pt.y + 4 + h));

		CRect finalRect = candidates[0];
		bool found = false;
		for (const auto& cand : candidates)
		{
			if (checkFit(cand, id))
			{
				finalRect = cand;
				found = true;
				break;
			}
		}

		// 无可用位置时隐藏标签；悬停/选中站优先，放大后重新布局。
		if (!found) continue;

		pDC->SetTextColor(m_darkTheme ? RGB(224, 230, 236) : RGB(40, 48, 56));
		pDC->DrawText(station.name, finalRect, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
		placedBoxes.push_back(finalRect);
	}

	pDC->SelectObject(pOldFont);
	pDC->RestoreDC(savedDC);
	DrawInteractionHeader(pDC, rcClient);
	DrawStationDetails(pDC, rcClient);

	if (useBuffer)
	{
		screenDC->BitBlt(0, 0, rcClient.Width(), rcClient.Height(), pDC, 0, 0, SRCCOPY);
		memoryDC.SelectObject(oldBackBuffer);
	}
}

CPoint CNanjingMetroView::StationToScreen(const CPoint& position) const // 编写者：刘子瑜（3号）
{
	return CPoint(
		m_drawOriginX + (int)((position.x - m_minDataX) * m_drawScale + 0.5),
		m_drawOriginY + (int)((position.y - m_minDataY) * m_drawScale + 0.5));
}

int CNanjingMetroView::HitTestStation(const CPoint& point) const // 编写者：刘子瑜（3号）
{
	if (!m_mapViewport.PtInRect(point))
		return -1;
	CNanjingMetroDoc* pDoc = GetDocument();
	if (!pDoc)
		return -1;

	int closestId=-1, closestDistance=Ui(12)*Ui(12)+1;
	for (auto it = pDoc->m_metroData.m_stations.rbegin();
		it != pDoc->m_metroData.m_stations.rend(); ++it)
	{
		const CPoint center = StationToScreen(it->second.pos);
		const int dx = point.x - center.x;
		const int dy = point.y - center.y;
		int distance=dx*dx+dy*dy;
		if (distance < closestDistance) { closestDistance=distance;closestId=it->first; }
	}
	return closestId;
}

void CNanjingMetroView::SelectStation(int stationId, bool centerOnStation) // 编写者：肖博腾（4号）
{
	CNanjingMetroDoc* pDoc = GetDocument();
	if (!pDoc || pDoc->m_metroData.GetStationById(stationId) == nullptr)
		return;
	m_selectedStationId = stationId;
    m_showRoute = false; m_detailOffset = 0;
	LayoutControls();
	if (centerOnStation)
		CenterOnStation(stationId);
	Invalidate(FALSE);
}

void CNanjingMetroView::CenterOnStation(int stationId) // 编写者：刘子瑜（3号）
{
	CNanjingMetroDoc* pDoc = GetDocument();
	if (!pDoc)
		return;
	StationNode* station = pDoc->m_metroData.GetStationById(stationId);
	if (!station)
		return;
	const CPoint current = StationToScreen(station->pos);
	const CPoint target = m_mapViewport.CenterPoint();
	m_panOffset += target - current;
}

void CNanjingMetroView::SetRouteEndpoint(int stationId, bool asStart) // 编写者：肖博腾（4号）
{
	CNanjingMetroDoc* pDoc = GetDocument();
	if (!pDoc || !pDoc->m_metroData.GetStationById(stationId))
		return;

	if (asStart)
		m_routeStartStationId = stationId;
	else
		m_routeEndStationId = stationId;

	m_selectedStationId = stationId;
    ClearRoute();
    SyncEndpoints();
	Invalidate(FALSE);
}

void CNanjingMetroView::QuerySelectedRoute(RouteStrategy strategy) // 编写者：肖博腾（4号）
{
	CNanjingMetroDoc* pDoc = GetDocument();
	if (!pDoc)
		return;

	StationNode* start = pDoc->m_metroData.GetStationById(m_routeStartStationId);
	StationNode* end = pDoc->m_metroData.GetStationById(m_routeEndStationId);
	if (!start || !end)
	{
		AfxMessageBox(_T("请在顶部选择起终点，或点击站点后使用右侧的设站按钮。"),
			MB_OK | MB_ICONINFORMATION);
		return;
	}

	QueryRoute(start->id, start->name, end->id, end->name, strategy);
}

CString CNanjingMetroView::GetStationLineNames(const StationNode& station) const // 编写者：肖博腾（4号）
{
	CNanjingMetroDoc* pDoc = GetDocument();
	CString result;
	for (size_t i = 0; pDoc && i < station.lineIds.size(); ++i)
	{
		CString lineName;
		for (const MetroLine& line : pDoc->m_metroData.m_lines)
			if (line.lineId == station.lineIds[i])
			{
				lineName = line.lineName;
				break;
			}
		if (lineName.IsEmpty())
			lineName.Format(_T("线路%d"), station.lineIds[i]);
		if (!result.IsEmpty())
			result += _T(" / ");
		result += lineName;
	}
	return result.IsEmpty() ? CString(_T("暂无线信息")) : result;
}

CString CNanjingMetroView::GetLandmarkTypeName(LandmarkType type) const // 编写者：肖博腾（4号）
{
	switch (type)
	{
	case LANDMARK_HOTEL: return _T("酒店");
	case LANDMARK_SCENERY: return _T("景点");
	case LANDMARK_HOSPITAL: return _T("医院");
	case LANDMARK_SCHOOL: return _T("学校");
	case LANDMARK_MALL: return _T("商场");
	default: return _T("周边");
	}
}

// CNanjingMetroView 诊断

#ifdef _DEBUG
void CNanjingMetroView::AssertValid() const // 编写者：何彦毅（1号）
{
	CView::AssertValid();
}

void CNanjingMetroView::Dump(CDumpContext& dc) const // 编写者：何彦毅（1号）
{
	CView::Dump(dc);
}

CNanjingMetroDoc* CNanjingMetroView::GetDocument() const // 非调试版本是内联的 // 编写者：何彦毅（1号）
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNanjingMetroDoc)));
	return (CNanjingMetroDoc*)m_pDocument;
}
#endif //_DEBUG


// CNanjingMetroView 消息处理程序


void CNanjingMetroView::OnRouteShortest() // 编写者：何彦毅（1号）
{
	QuerySelectedRoute(STRATEGY_SHORTEST_DIST);
}


void CNanjingMetroView::OnRouteMinStation() // 编写者：何彦毅（1号）
{
	QuerySelectedRoute(STRATEGY_MIN_STATIONS);
}


void CNanjingMetroView::OnRouteMinTransfer() // 编写者：何彦毅（1号）
{
	QuerySelectedRoute(STRATEGY_MIN_TRANSFERS);
}


void CNanjingMetroView::OnQueryStation() // 编写者：肖博腾（4号）
{
	CNanjingMetroDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc) return;

	std::vector<StationNode> stations;
	stations.reserve(pDoc->m_metroData.m_stations.size());
	for (const auto& pair : pDoc->m_metroData.m_stations)
		stations.push_back(pair.second);

	CStationSearchDlg dialog(stations, this);
	if (dialog.DoModal() == IDOK)
		SelectStation(dialog.m_selectedStationId, true);
}


void CNanjingMetroView::OnQueryLandmark() // 编写者：肖博腾（4号）
{
	// 搜索对话框右侧已经按选中站点同步展示分类周边设施。
	OnQueryStation();
}


void CNanjingMetroView::OnFavView() // 编写者：何彦毅（1号）
{
	CNanjingMetroDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc) return;

	CHistoryDlg dlg(pDoc->m_historyMgr, CHistoryDlg::MODE_FAVORITES, this);
	if (dlg.DoModal() == CHistoryDlg::RESULT_QUERY_ROUTE)
	{
		const UserRouteRecord& rec = dlg.m_queryRecord;
		QueryRoute(rec.startStationId, rec.startStationName,
			rec.endStationId, rec.endStationName, rec.strategy);
	}
	LayoutControls();
	Invalidate(FALSE);
}


void CNanjingMetroView::OnHistoryList() // 编写者：何彦毅（1号）
{
	CNanjingMetroDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc) return;

	CHistoryDlg dlg(pDoc->m_historyMgr, CHistoryDlg::MODE_HISTORY, this);
	if (dlg.DoModal() == CHistoryDlg::RESULT_QUERY_ROUTE)
	{
		const UserRouteRecord& rec = dlg.m_queryRecord;
		QueryRoute(rec.startStationId, rec.startStationName,
			rec.endStationId, rec.endStationName, rec.strategy);
	}
	LayoutControls();
	Invalidate(FALSE);
}


void CNanjingMetroView::OnHistoryClear() // 编写者：何彦毅（1号）
{
	CNanjingMetroDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc) return;

	if (AfxMessageBox(_T("确定要清空全部历史记录吗？收藏的路线会保留。"),
		MB_YESNO | MB_ICONQUESTION) == IDYES)
	{
		pDoc->m_historyMgr.ClearHistory();
		pDoc->m_historyMgr.SaveToFile(_T("user_history.txt"));
		AfxMessageBox(_T("历史记录已清空。"), MB_ICONINFORMATION);
	}
}


void CNanjingMetroView::QueryRoute(int startId, const CString& startName,
	int endId, const CString& endName, RouteStrategy strategy) // 编写者：何彦毅（1号）
{
	CNanjingMetroDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc) return;

	if (startId == endId)
	{
		AfxMessageBox(_T("起点和终点不能相同！"), MB_ICONWARNING);
		ClearRoute();
		Invalidate(FALSE);
		return;
	}

	m_routeStartStationId = startId; m_routeEndStationId = endId;
    SyncEndpoints();
	// 1. 记录本次查询到历史
	pDoc->m_historyMgr.AddRecord(startId, startName, endId, endName, strategy, false);
	pDoc->m_historyMgr.SaveToFile(_T("user_history.txt"));

	// 2. 调用图算法获取真实 PathResult
	PathResult result = g_graph.FindRoute(startId, endId, strategy);
	result.startStationName = startName;
	result.endStationName = endName;
	result.strategy = strategy;

	if (!result.isFound)
	{
		m_highlightStationIds.clear();
		ShowRouteResult(result);
		Invalidate();
		return;
	}

	// 3. 缓存路径站点序列用于地图高亮
	m_highlightStationIds = result.stationSequence;
	m_selectedStationId = endId;
	m_zoom = 1.0; m_panOffset = CPoint(0, 0);

	// 4. 统一格式化并展示
	ShowRouteResult(result);
	Invalidate();
}


void CNanjingMetroView::ShowRouteResult(const PathResult& result) // 编写者：何彦毅（1号）
{
    m_lastRoute = result;
    m_showRoute = true;
    m_detailOffset = 0;
    m_routeNotice = result.isFound ? _T("") : _T("未找到可行路线，请重新选择起终点。");
    LayoutControls();
    Invalidate(FALSE);
}

void CNanjingMetroView::OnViewResetMap() // 编写者：刘子瑜（3号）
{
	// 只还原地图视角，保留当前站点、起终点和乘车方案。
	m_dragging = false;
	m_dragMoved = false;
	if (GetCapture() == this) ReleaseCapture();
	m_hoverStationId = -1;
	m_zoom = 1.0;
	m_panOffset = CPoint(0, 0);
	Invalidate(FALSE);
}


void CNanjingMetroView::OnViewThemeToggle() // 编写者：刘子瑜（3号）
{
	m_darkTheme = !m_darkTheme;
	Invalidate(FALSE);
}

void CNanjingMetroView::OnLButtonDown(UINT nFlags, CPoint point) // 编写者：刘子瑜（3号）
{
	if (m_mapViewport.PtInRect(point))
	{
		SetFocus();
		m_dragging = true;
		m_dragMoved = false;
		m_dragStartPoint = point;
		m_dragStartPan = m_panOffset;
		SetCapture();
		::SetCursor(::LoadCursor(nullptr, IDC_SIZEALL));
	}
	CView::OnLButtonDown(nFlags, point);
}

void CNanjingMetroView::OnLButtonUp(UINT nFlags, CPoint point) // 编写者：刘子瑜（3号）
{
	if (m_dragging)
	{
		const bool wasMoved = m_dragMoved;
		m_dragging = false;
		m_dragMoved = false;
		if (GetCapture() == this)
			ReleaseCapture();
		if (!wasMoved)
		{
			const int stationId = HitTestStation(point);
			if (stationId >= 0)
				SelectStation(stationId, false);
		}
	}
	CView::OnLButtonUp(nFlags, point);
}

void CNanjingMetroView::OnRButtonUp(UINT nFlags, CPoint point) // 编写者：刘子瑜（3号）
{
	const int stationId = HitTestStation(point);
	if (stationId < 0)
	{
		CView::OnRButtonUp(nFlags, point);
		return;
	}

	CNanjingMetroDoc* pDoc = GetDocument();
	StationNode* station = pDoc ? pDoc->m_metroData.GetStationById(stationId) : nullptr;
	if (!station)
		return;

	SelectStation(stationId, false);

	enum : UINT
	{
		MENU_SET_START = 1,
		MENU_SET_END,
		MENU_QUERY_SHORTEST,
		MENU_CLEAR_ENDPOINTS
	};

	CMenu menu;
	menu.CreatePopupMenu();
	CString item;
	item.Format(_T("设为起点：%s"), station->name.GetString());
	menu.AppendMenu(MF_STRING, MENU_SET_START, item);
	item.Format(_T("设为终点：%s"), station->name.GetString());
	menu.AppendMenu(MF_STRING, MENU_SET_END, item);
	menu.AppendMenu(MF_SEPARATOR);

	StationNode* start = pDoc->m_metroData.GetStationById(m_routeStartStationId);
	StationNode* end = pDoc->m_metroData.GetStationById(m_routeEndStationId);
	CString status;
	status.Format(_T("当前起点：%s"), start ? start->name.GetString() : _T("未设置"));
	menu.AppendMenu(MF_STRING | MF_GRAYED, 0, status);
	status.Format(_T("当前终点：%s"), end ? end->name.GetString() : _T("未设置"));
	menu.AppendMenu(MF_STRING | MF_GRAYED, 0, status);

	const bool canQuery = start && end && start->id != end->id;
	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING | (canQuery ? MF_ENABLED : MF_GRAYED),
		MENU_QUERY_SHORTEST, _T("按最短距离查询路线"));
	menu.AppendMenu(MF_STRING, MENU_CLEAR_ENDPOINTS, _T("清除起点和终点"));

	CPoint screenPoint = point;
	ClientToScreen(&screenPoint);
	const UINT command = (UINT)menu.TrackPopupMenu(
		TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
		screenPoint.x, screenPoint.y, this);

	switch (command)
	{
	case MENU_SET_START:
		SetRouteEndpoint(stationId, true);
		break;
	case MENU_SET_END:
		SetRouteEndpoint(stationId, false);
		break;
	case MENU_QUERY_SHORTEST:
		QuerySelectedRoute(STRATEGY_SHORTEST_DIST);
		break;
	case MENU_CLEAR_ENDPOINTS:
		m_routeStartStationId = -1;
		m_routeEndStationId = -1;
        ClearRoute(); SyncEndpoints();
		Invalidate(FALSE);
		break;
	default:
		break;
	}

	CView::OnRButtonUp(nFlags, point);
}

void CNanjingMetroView::OnMouseMove(UINT nFlags, CPoint point) // 编写者：刘子瑜（3号）
{
	if (m_dragging && (nFlags & MK_LBUTTON) != 0)
	{
		const CPoint delta = point - m_dragStartPoint;
		if (abs(delta.x) > 3 || abs(delta.y) > 3)
			m_dragMoved = true;
		if (m_dragMoved)
		{
			m_panOffset = m_dragStartPan + delta;
			Invalidate(FALSE);
		}
	}
	else
	{
		const int stationId = HitTestStation(point);
		if (stationId != m_hoverStationId)
		{
			m_hoverStationId = stationId;
			Invalidate(FALSE);
		}
	}
	CView::OnMouseMove(nFlags, point);
}

void CNanjingMetroView::OnCaptureChanged(CWnd* pWnd) // 编写者：刘子瑜（3号）
{
	m_dragging = false;
	m_dragMoved = false;
	CView::OnCaptureChanged(pWnd);
}

BOOL CNanjingMetroView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) // 编写者：刘子瑜（3号）
{
	if (nHitTest == HTCLIENT)
	{
		::SetCursor(::LoadCursor(nullptr,
			m_dragging ? IDC_SIZEALL : (m_hoverStationId >= 0 ? IDC_HAND : IDC_ARROW)));
		return TRUE;
	}
	return CView::OnSetCursor(pWnd, nHitTest, message);
}

BOOL CNanjingMetroView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) // 编写者：刘子瑜（3号）
{
	CPoint clientPoint = pt;
	ScreenToClient(&clientPoint);
	if (m_panelRect.PtInRect(clientPoint)) {
        m_detailOffset = (std::max)(0, (std::min)(m_detailMax, m_detailOffset + (zDelta > 0 ? -Ui(72) : Ui(72))));
        Invalidate(FALSE); return TRUE;
    }
    if (!m_mapViewport.PtInRect(clientPoint))
		return CView::OnMouseWheel(nFlags, zDelta, pt);

	const double oldZoom = m_zoom;
	m_zoom = zDelta > 0 ? (std::min)(3.0, m_zoom * 1.15) : (std::max)(0.65, m_zoom / 1.15);
	if (m_zoom != oldZoom)
	{
		const double ratio = m_zoom / oldZoom;
		const CPoint center = m_mapViewport.CenterPoint();
		m_panOffset.x = (int)(m_panOffset.x * ratio + (clientPoint.x - center.x) * (1.0 - ratio));
		m_panOffset.y = (int)(m_panOffset.y * ratio + (clientPoint.y - center.y) * (1.0 - ratio));
		Invalidate(FALSE);
	}
	return TRUE;
}

BOOL CNanjingMetroView::OnEraseBkgnd(CDC* pDC) // 编写者：刘子瑜（3号）
{
	UNREFERENCED_PARAMETER(pDC);
	return TRUE;
}
