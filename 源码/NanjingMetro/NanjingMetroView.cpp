
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
#include "MapStyle.h"
#include "MapLabelLayout.h"
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
    ON_COMMAND_RANGE(5001, 5012, &CNanjingMetroView::OnUiAction)
    ON_CBN_SELCHANGE(5020, &CNanjingMetroView::OnEndpointChanged)
    ON_CBN_SELCHANGE(5021, &CNanjingMetroView::OnEndpointChanged)
END_MESSAGE_MAP()

// CNanjingMetroView 构造/析构

CNanjingMetroView::CNanjingMetroView() noexcept
	: m_zoom(1.0), m_panOffset(0, 0), m_dragStartPoint(0, 0), m_dragStartPan(0, 0),
	m_dragging(false), m_dragMoved(false), m_darkTheme(false),
	m_selectedStationId(-1), m_hoverStationId(-1),
	m_routeStartStationId(-1), m_routeEndStationId(-1), m_drawScale(1.0),
	// 出行指南版式保持等比例投影，让左侧地图保留参考图中的舒展留白。
	// 该比例仅影响屏幕绘制，不改变任何站点坐标、线路拓扑或查询数据。
	m_drawAspectX(1.0), m_drawAspectY(1.0),
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
	const MetroLayout::Layout& metro = Metro();
	CPoint firstPos=metro.MarkerPos(data.m_stations.begin()->first);
	int minX=firstPos.x, minY=firstPos.y;
	int maxX = minX;
	int maxY = minY;
	for (const auto& pair : data.m_stations)
	{
		CPoint pos=metro.MarkerPos(pair.first);
		minX=__min(minX,pos.x);minY=__min(minY,pos.y);maxX=__max(maxX,pos.x);maxY=__max(maxY,pos.y);
	}

	const COLORREF pageColor = m_darkTheme ? RGB(24, 29, 36) : RGB(241, 245, 249);
	const COLORREF mapColor = m_darkTheme ? RGB(34, 41, 49) : RGB(255, 255, 255);
	const COLORREF paperColor = mapColor; // 线路底色描边与站符号底色
	pDC->FillSolidRect(rcClient, pageColor);
	pDC->SetBkMode(TRANSPARENT);

	// 4号：地图和侧栏共享布局计算，控件与绘制区域不会相互覆盖。
	pDC->FillSolidRect(m_mapViewport, mapColor);
	// 官方线路图的细边框，让画布与页面底色分层。
	pDC->Draw3dRect(m_mapViewport, m_darkTheme ? RGB(51, 61, 72) : RGB(227, 233, 240),
		m_darkTheme ? RGB(51, 61, 72) : RGB(227, 233, 240));

	const int margin = Ui(44);
	const int drawW = __max(m_mapViewport.Width() - margin * 2, 1);
	const int drawH = __max(m_mapViewport.Height() - margin * 2, 1);
	const double rangeX = __max(maxX - minX, 1);
	const double rangeY = __max(maxY - minY, 1);
	const double baseScale = __min((double)drawW / (rangeX * m_drawAspectX),
		(double)drawH / (rangeY * m_drawAspectY));
	m_drawScale = baseScale * m_zoom;
	m_minDataX = minX;
	m_minDataY = minY;
	m_drawOriginX = m_mapViewport.left + (m_mapViewport.Width() - (int)(rangeX * m_drawScale * m_drawAspectX)) / 2 + m_panOffset.x;
	m_drawOriginY = m_mapViewport.top + (m_mapViewport.Height() - (int)(rangeY * m_drawScale * m_drawAspectY)) / 2 + m_panOffset.y;

	auto toScreen = [&](const CPoint& pos) -> CPoint
	{
		return StationToScreen(pos);
	};

	const int savedDC = pDC->SaveDC();
	pDC->IntersectClipRect(m_mapViewport);

	DrawGeography(pDC);
	// 线网几何：直接使用用户原图描摹的显示坐标与折点，站点业务数据零改动。
	// 每条线先描一道底色再描线路色，交叉处形成干净的分层，而不是互相糊在一起。
	auto strokePolyline = [&](const std::vector<CPoint>& source, COLORREF color, float width) {
		if (source.size() < 2) return;
		std::vector<CPoint> points;
		points.reserve(source.size());
		for (const CPoint& point : source) points.push_back(toScreen(point));
		DrawMapStroke(pDC, points, paperColor, width + UiF(1.8f));
		DrawMapStroke(pDC, points, color, width);
	};
	std::vector<MetroLine> drawLines=data.m_lines;
    std::stable_sort(drawLines.begin(),drawLines.end(),[&](const MetroLine& a,const MetroLine& b){return (a.lineId==m_selectedLineId)<(b.lineId==m_selectedLineId);});
    for (const auto& line : drawLines)
    {
        const std::vector<CPoint>* pts = metro.LinePoints(line.lineId);
		COLORREF color=line.color;
        if((m_selectedLineId>=0 && line.lineId!=m_selectedLineId) || !m_highlightStationIds.empty())
            color=m_darkTheme?RGB(73,81,91):RGB(217,225,233);
        if (pts) strokePolyline(*pts, color, UiF(4.0f));
	}
	// 规划高亮：沿所属线路的绘制折线走，换乘处回落到两点直连。
	for (size_t i = 1; i < m_highlightStationIds.size(); ++i)
	{
		const int from = m_highlightStationIds[i - 1];
		const int to = m_highlightStationIds[i];
		int lineId = -1;
		for (const auto& line : data.m_lines)
		{
			for (size_t k = 1; k < line.stationIds.size(); ++k)
			{
				if ((line.stationIds[k - 1] == from && line.stationIds[k] == to) ||
					(line.stationIds[k - 1] == to && line.stationIds[k] == from))
				{ lineId = line.lineId; break; }
			}
			if (lineId >= 0) break;
		}
		std::vector<CPoint> segment = metro.SegmentPoints(lineId, from, to);
		if (segment.size() < 2)
		{
			auto a = data.m_stations.find(from), b = data.m_stations.find(to);
			if (a == data.m_stations.end() || b == data.m_stations.end()) continue;
			segment.clear();
			segment.push_back(metro.MarkerPos(from));
			segment.push_back(metro.MarkerPos(to));
		}
		COLORREF routeColor=RGB(35,111,229);for(const auto& line:data.m_lines)if(line.lineId==lineId)routeColor=line.color;
		strokePolyline(segment, routeColor, UiF(5.0f));
	}

	// 4. 所有车站使用同尺寸白底圆点；状态只改变边框颜色，不改变形状和大小。
	const COLORREF inkColor = m_darkTheme ? RGB(192, 202, 214) : RGB(95, 107, 119);
    const float stationRadius=UiF(2.0f),stationOutline=UiF(0.7f);

	for (const auto& pair : data.m_stations)
	{
		const StationNode& station = pair.second;
		const CPoint pt = toScreen(metro.MarkerPos(station.id));
		if (std::find(m_highlightStationIds.begin(), m_highlightStationIds.end(), station.id) != m_highlightStationIds.end())
			continue; // 路线高亮站点在下方单独绘制，不被官方符号盖住
		MapStyle::StrokeRing(pDC,pt,stationRadius,inkColor,stationOutline,paperColor,true);
	}
	// 路径上的站点与其他站点使用相同圆点，线路颜色负责显示路线。
	for (int id : m_highlightStationIds)
	{
		auto found = data.m_stations.find(id);
		if (found == data.m_stations.end())
			continue;
		const CPoint pt = toScreen(metro.MarkerPos(id));
		MapStyle::StrokeRing(pDC,pt,stationRadius,inkColor,stationOutline,paperColor,true);
	}
	// 选中 / 悬停 / 起终点：统一圆点仅改变边框颜色。
	for (const auto& pair : data.m_stations)
	{
		const StationNode& station = pair.second;
		const bool selected = station.id == m_selectedStationId;
		const bool hovered = station.id == m_hoverStationId;
		const bool routeStart = station.id == m_routeStartStationId;
		const bool routeEnd = station.id == m_routeEndStationId;
		if (!selected && !hovered && !routeStart && !routeEnd)
			continue;

		const CPoint pt = toScreen(metro.MarkerPos(station.id));
		const COLORREF ringColor = routeStart ? RGB(18, 145, 78) :
			(routeEnd ? RGB(209, 55, 55) : (selected ? RGB(239, 112, 24) : RGB(206, 160, 26)));
        MapStyle::StrokeRing(pDC,pt,stationRadius,ringColor,stationOutline,paperColor,true);

		if (routeStart || routeEnd)
		{
			CFont* previous = pDC->SelectObject(&m_uiFont);
			pDC->SetTextColor(ringColor);
			CRect pin(pt.x - Ui(10), pt.y - Ui(34), pt.x + Ui(10), pt.y - Ui(14));
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
					dx += abs(metro.MarkerPos(nextIt->first).x - metro.MarkerPos(id).x);
					dy += abs(metro.MarkerPos(nextIt->first).y - metro.MarkerPos(id).y);
				}
			}
			if (j > 0)
			{
				auto prevIt = data.m_stations.find(line.stationIds[j - 1]);
				if (prevIt != data.m_stations.end())
				{
					dx += abs(metro.MarkerPos(id).x - metro.MarkerPos(prevIt->first).x);
					dy += abs(metro.MarkerPos(id).y - metro.MarkerPos(prevIt->first).y);
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
	obstacles.reserve(data.m_stations.size() * 2);
	for (const auto& pair : data.m_stations)
	{
		CPoint pt = toScreen(metro.MarkerPos(pair.first));
        // 避让框同步采用圆点尺寸，不再为换乘站保留椭圆占位。
        const int rx=(int)std::ceil(stationRadius+stationOutline/2)+Ui(1),ry=rx;
        obstacles.push_back({pair.first,CRect(pt.x-rx,pt.y-ry,pt.x+rx+1,pt.y+ry+1)});
		if (pair.first == m_routeStartStationId || pair.first == m_routeEndStationId)
			obstacles.push_back({ -1, CRect(pt.x - Ui(10), pt.y - Ui(34), pt.x + Ui(10), pt.y - Ui(14)) });
	}

    struct StrokeObstacle { CPoint a,b; double padding; };
    std::vector<StrokeObstacle> strokes;
    for (const auto& line:data.m_lines) {
        auto* points=metro.LinePoints(line.lineId); if(!points)continue;
        for(size_t i=1;i<points->size();++i)
            strokes.push_back({toScreen((*points)[i-1]),toScreen((*points)[i]),UiF(3.5f)});
    }
    // 高亮更宽，也按实际区间几何参与文字避让。
    for(size_t i=1;i<m_highlightStationIds.size();++i) {
        int from=m_highlightStationIds[i-1],to=m_highlightStationIds[i];
        for(const auto& line:data.m_lines) {
            bool adjacent=false;
            for(size_t k=1;k<line.stationIds.size();++k)
                if((line.stationIds[k-1]==from&&line.stationIds[k]==to)||(line.stationIds[k-1]==to&&line.stationIds[k]==from))adjacent=true;
            if(!adjacent)continue;
            auto points=metro.SegmentPoints(line.lineId,from,to);
            for(size_t k=1;k<points.size();++k)strokes.push_back({toScreen(points[k-1]),toScreen(points[k]),UiF(4.5f)});
            break;
        }
    }
    auto touchesLine=[&](const CRect& box) {
        for(const auto& stroke:strokes)
            if(MapLabels::HitsStroke(box,stroke.a,stroke.b,stroke.padding))return true;
        return false;
    };

	// 障碍桶格：绘制期反复做命中测试，用格子索引保证平移/缩放仍然跟手。
	struct ObstacleGrid
	{
		enum { kCell = 96 }; // 局部类不能有静态数据成员，用枚举常量
		std::map<long long, std::vector<int> > buckets;
		static int CellOf(int value)
		{
			return value >= 0 ? value / kCell : -((-value + kCell - 1) / kCell);
		}
		void Add(int index, const CRect& rect)
		{
			for (int cx = CellOf(rect.left); cx <= CellOf(rect.right); ++cx)
				for (int cy = CellOf(rect.top); cy <= CellOf(rect.bottom); ++cy)
					buckets[(long long)(((unsigned long long)(unsigned int)cx << 32) | (unsigned int)cy)].push_back(index);
		}
		void Query(const CRect& rect, std::vector<int>& out) const
		{
			out.clear();
			for (int cx = CellOf(rect.left); cx <= CellOf(rect.right); ++cx)
				for (int cy = CellOf(rect.top); cy <= CellOf(rect.bottom); ++cy)
				{
					auto it = buckets.find((long long)(((unsigned long long)(unsigned int)cx << 32) | (unsigned int)cy));
					if (it == buckets.end()) continue;
					out.insert(out.end(), it->second.begin(), it->second.end());
				}
			std::sort(out.begin(), out.end());
			out.erase(std::unique(out.begin(), out.end()), out.end());
		}
	};
	ObstacleGrid obstacleGrid;
	for (size_t i = 0; i < obstacles.size(); ++i)
		obstacleGrid.Add((int)i, obstacles[i].rect);

	std::vector<CRect> placedBoxes{CRect(m_mapViewport.right-Ui(104),m_mapViewport.bottom-Ui(138),m_mapViewport.right,m_mapViewport.bottom)};
	std::vector<int> gridHits;

	auto overlaps = [](const CRect& a, const CRect& b) -> bool {
		return a.left < b.right && a.right > b.left && a.top < b.bottom && a.bottom > b.top;
	};

	// 某个矩形是否既不出图幅、又不压站点、不压线路、不压已放好的文字。
	auto boxFree = [&](const CRect& rc) -> bool {
		if (rc.left < m_mapViewport.left || rc.top < m_mapViewport.top ||
			rc.right > m_mapViewport.right || rc.bottom > m_mapViewport.bottom)
			return false;
		for (const CRect& placed : placedBoxes)
		{
			CRect pad = placed;
			pad.InflateRect(Ui(1), Ui(1));
			if (overlaps(rc, pad))
				return false;
		}
		obstacleGrid.Query(rc, gridHits);
		for (int index : gridHits)
		{
			if (overlaps(rc, obstacles[index].rect))
				return false;
		}
		return !touchesLine(rc);
	};

	// 官方图端点色签，S1/S3 共用南京南站的一端省略，避免枢纽重复堆叠。
	CFont badgeFont;
	badgeFont.CreateFont(-Ui(12),0,0,0,FW_BOLD,FALSE,FALSE,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,_T("Microsoft YaHei UI"));
	CFont* beforeBadgeFont=pDC->SelectObject(&badgeFont);
	for(const auto& line:data.m_lines) {
		if(line.stationIds.empty())continue;
		CString number=line.lineName;number.Replace(_T("号线"),_T(""));
		for(int end=0;end<2;++end) {
			if(!m_highlightStationIds.empty() || (m_selectedLineId>=0 && line.lineId!=m_selectedLineId))continue;
            if(end==0 && (line.lineId==11 || line.lineId==13))continue;
			const int id=end?line.stationIds.back():line.stationIds.front();
			const int neighbor=end?line.stationIds[line.stationIds.size()-2]:line.stationIds[1];
			CPoint pt=toScreen(metro.LineStationPos(line.lineId,id));
			int width=Ui(number.GetLength()>1?27:20),height=Ui(20);

			// 终点朝外的方向：色签摆在终点之外，避免落进走廊和站名堆里。
			double ux=1.0,uy=0.0;
			std::vector<CPoint> tail=metro.SegmentPoints(line.lineId,neighbor,id);
			if(tail.size()>=2) {
				const CPoint a=toScreen(tail[tail.size()-2]),b=toScreen(tail.back());
				const double dx=(double)(b.x-a.x),dy=(double)(b.y-a.y),L=std::sqrt(dx*dx+dy*dy);
				if(L>1e-6){ux=dx/L;uy=dy/L;}
			}
			auto centered=[&](int cx,int cy){return CRect(cx-width/2,cy-height/2,cx-width/2+width,cy-height/2+height);};
			std::vector<CRect> candidates;
			if(std::fabs(ux)>=std::fabs(uy)) {
				const int sx=ux>=0.0?1:-1;
				candidates.push_back(centered(pt.x+sx*(Ui(12)+width/2),pt.y));
				candidates.push_back(centered(pt.x,pt.y-Ui(13)-height/2));
				candidates.push_back(centered(pt.x,pt.y+Ui(13)+height/2));
				candidates.push_back(centered(pt.x-sx*(Ui(12)+width/2),pt.y));
			} else {
				const int sy=uy>=0.0?1:-1;
				candidates.push_back(centered(pt.x,pt.y+sy*(Ui(13)+height/2)));
				candidates.push_back(centered(pt.x+Ui(13)+width/2,pt.y));
				candidates.push_back(centered(pt.x-Ui(13)-width/2,pt.y));
				candidates.push_back(centered(pt.x,pt.y-sy*(Ui(13)+height/2)));
			}
			CRect badge; bool badgeFound=false;
            for(const CRect& cand:candidates) { if(boxFree(cand)){badge=cand;badgeFound=true;break;} }
            if(!badgeFound)continue;
			MapStyle::FillRoundRect(pDC,badge,Ui(5),line.color,mapColor,UiF(1.0f));
			pDC->SetTextColor(RGB(255,255,255));
			pDC->DrawText(number,badge,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
			badge.InflateRect(Ui(2),Ui(2));placedBoxes.push_back(badge);
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

	auto checkFit = [&](const CRect& rc, int selfId) -> bool {
		if (rc.left < m_mapViewport.left || rc.top < m_mapViewport.top ||
			rc.right > m_mapViewport.right || rc.bottom > m_mapViewport.bottom)
			return false;

		obstacleGrid.Query(rc, gridHits);
		for (int index : gridHits)
		{
			UNREFERENCED_PARAMETER(selfId);
			if (overlaps(rc, obstacles[index].rect))
				return false;
		}
		for (const CRect& placed : placedBoxes)
		{
			CRect pad = placed;
			pad.InflateRect(Ui(1), Ui(1));
			if (overlaps(rc, pad))
				return false;
		}
		return !touchesLine(rc);
	};

	CFont labelFont;
	labelFont.CreatePointFont(68, _T("微软雅黑"), pDC); // 按协作要求保留 6.8pt 字号，空间不足时隐藏而不压线
	CFont* pOldFont = pDC->SelectObject(&labelFont);
	pDC->SetBkMode(TRANSPARENT);
	pDC->SetTextColor(m_darkTheme ? RGB(224, 230, 236) : RGB(40, 40, 40));

    std::vector<std::pair<int, StationNode>> labelStations(data.m_stations.begin(), data.m_stations.end());
    auto priority = [&](int id) {
        const auto& memberships=data.m_stations.at(id).lineIds;
        if(m_selectedLineId>=0 && std::find(memberships.begin(),memberships.end(),m_selectedLineId)==memberships.end())return 20;
        if (id == m_hoverStationId || id == m_selectedStationId) return 0;
        if (id == m_routeStartStationId || id == m_routeEndStationId) return 1;
        if (std::find(m_highlightStationIds.begin(), m_highlightStationIds.end(), id) != m_highlightStationIds.end()) return 2;
        if(data.m_stations.at(id).isTransfer || terminalStationIds.count(id))return 3;
        return 4;
    };
    std::stable_sort(labelStations.begin(), labelStations.end(), [&](const auto& a, const auto& b) { return priority(a.first) < priority(b.first); });
    std::vector<MapLabels::Label> labelChoices;
	for (const auto& pair : labelStations)
	{
		const StationNode& station = pair.second;
		const int id = pair.first;
		const CPoint pt = toScreen(metro.MarkerPos(id));
		CSize ext = pDC->GetTextExtent(station.name);
		const int w = ext.cx;
		const int h = ext.cy;

		std::vector<CRect> candidates;
		const int gap = Ui(5); // 站名与站点/线路之间的最小空隙

		// --- 规则一：核心密集枢纽强制人工定向优先 ---
		if (id == 8) // 南京站：向右上方引导
		{
			candidates.push_back(CRect(pt.x + Ui(10), pt.y - h - gap, pt.x + Ui(10) + w, pt.y - gap));
		}
		else if (id == 21) // 南京南站：向右下方引导
		{
			candidates.push_back(CRect(pt.x + Ui(12), pt.y + gap, pt.x + Ui(12) + w, pt.y + gap + h));
		}
		else if (id == 13) // 新街口：向左侧引导
		{
			candidates.push_back(CRect(pt.x - w - Ui(10), pt.y - h / 2, pt.x - Ui(10), pt.y + h / 2));
		}
		// --- 规则二：S1 号线（机场线）平行南下，统一靠左排布，彻底避开右侧的 3 号线 ---
		else if ((id >= 57 && id <= 64) || (id >= 193 && id <= 207) || (id >= 246 && id <= 250))
		{
			candidates.push_back(CRect(pt.x - w - gap, pt.y - h / 2, pt.x - gap, pt.y + h / 2));
			candidates.push_back(CRect(pt.x - w - gap, pt.y - h - Ui(2), pt.x - gap, pt.y - Ui(2)));
		}
		// --- 规则三：东西向水平线路（S3段、江北段、大学城段），按次序交错上下排布 ---
		else if (isHorizontalStation.find(id) != isHorizontalStation.end())
		{
			int order = lineOrder.count(id) ? lineOrder[id] : id;
			bool top = (order % 2 == 0);
			if (top)
			{
				candidates.push_back(CRect(pt.x - w / 2, pt.y - h - gap, pt.x - w / 2 + w, pt.y - gap));
				candidates.push_back(CRect(pt.x - w / 2, pt.y + gap, pt.x - w / 2 + w, pt.y + gap + h));
			}
			else
			{
				candidates.push_back(CRect(pt.x - w / 2, pt.y + gap, pt.x - w / 2 + w, pt.y + gap + h));
				candidates.push_back(CRect(pt.x - w / 2, pt.y - h - gap, pt.x - w / 2 + w, pt.y - gap));
			}
		}
		// --- 规则四：普通南北向站点默认靠右，退避方案包含左、上、下 ---
		else
		{
			candidates.push_back(CRect(pt.x + gap, pt.y - h / 2, pt.x + gap + w, pt.y + h / 2));
			candidates.push_back(CRect(pt.x - w - gap, pt.y - h / 2, pt.x - gap, pt.y + h / 2));
			candidates.push_back(CRect(pt.x - w / 2, pt.y - h - gap, pt.x - w / 2 + w, pt.y - gap));
			candidates.push_back(CRect(pt.x - w / 2, pt.y + gap, pt.x - w / 2 + w, pt.y + gap + h));
		}

		// 补齐其余防碰撞退避方向
		candidates.push_back(CRect(pt.x + gap, pt.y - h - Ui(2), pt.x + gap + w, pt.y - Ui(2)));
		candidates.push_back(CRect(pt.x + gap, pt.y + Ui(2), pt.x + gap + w, pt.y + Ui(2) + h));
		candidates.push_back(CRect(pt.x - w - gap, pt.y - h - Ui(2), pt.x - gap, pt.y - Ui(2)));
		candidates.push_back(CRect(pt.x - w - gap, pt.y + Ui(2), pt.x - gap, pt.y + Ui(2) + h));

		// 系统化补位：四个斜向。密集区只剩斜角有空档，站名宁可稍微斜放，
		// 也不再叠到线路上或别的站名上；但距离要克制，否则读者认不出它属于哪一站。
		{
			const int distance = gap + (h * 2) / 3;
			candidates.push_back(CRect(pt.x + distance, pt.y - h - distance, pt.x + distance + w, pt.y - distance));
			candidates.push_back(CRect(pt.x + distance, pt.y + distance, pt.x + distance + w, pt.y + distance + h));
			candidates.push_back(CRect(pt.x - w - distance, pt.y - h - distance, pt.x - distance, pt.y - distance));
			candidates.push_back(CRect(pt.x - w - distance, pt.y + distance, pt.x - distance, pt.y + distance + h));
		}

        // 少量近距离补位，不把名称挪到远离站点的位置。
        for (int shift : {-Ui(5), Ui(5)}) {
            candidates.push_back(CRect(pt.x+gap,pt.y-h/2+shift,pt.x+gap+w,pt.y-h/2+shift+h));
            candidates.push_back(CRect(pt.x-gap-w,pt.y-h/2+shift,pt.x-gap,pt.y-h/2+shift+h));
            candidates.push_back(CRect(pt.x-w/2+shift,pt.y-gap-h,pt.x-w/2+shift+w,pt.y-gap));
            candidates.push_back(CRect(pt.x-w/2+shift,pt.y+gap,pt.x-w/2+shift+w,pt.y+gap+h));
        }
        // 沿文字长边微调锚点，为横向密集站和较长名称提供紧邻站心的空档。
        for(int offset : {-w/3,w/3}) {
            candidates.push_back(CRect(pt.x-w/2+offset,pt.y-gap-h,pt.x-w/2+offset+w,pt.y-gap));
            candidates.push_back(CRect(pt.x-w/2+offset,pt.y+gap,pt.x-w/2+offset+w,pt.y+gap+h));
        }
        for(int clearance : {Ui(9),Ui(13)}) {
            candidates.push_back(CRect(pt.x+clearance,pt.y-h/2,pt.x+clearance+w,pt.y-h/2+h));
            candidates.push_back(CRect(pt.x-clearance-w,pt.y-h/2,pt.x-clearance,pt.y-h/2+h));
            candidates.push_back(CRect(pt.x-w/2,pt.y-clearance-h,pt.x-w/2+w,pt.y-clearance));
            candidates.push_back(CRect(pt.x-w/2,pt.y+clearance,pt.x-w/2+w,pt.y+clearance+h));
        }
        MapLabels::Label label;label.id=id;label.priority=priority(id);
        for(const auto& candidate:candidates)
            if(checkFit(candidate,id) && std::find(label.candidates.begin(),label.candidates.end(),candidate)==label.candidates.end())
                label.candidates.push_back(candidate);
        labelChoices.push_back(label);
    }
    MapLabels::Arrange(labelChoices,Ui(1));
    m_visibleLabelCount=0;
    for(const auto& label:labelChoices) {
        if(label.chosen<0)continue;
        CRect rect=label.candidates[label.chosen];
        const auto& memberships=data.m_stations.at(label.id).lineIds;
        bool dim=(m_selectedLineId>=0 && std::find(memberships.begin(),memberships.end(),m_selectedLineId)==memberships.end()) || (!m_highlightStationIds.empty() && std::find(m_highlightStationIds.begin(),m_highlightStationIds.end(),label.id)==m_highlightStationIds.end());
        pDC->SetTextColor(dim?(m_darkTheme?RGB(103,115,128):RGB(173,182,192)):(m_darkTheme?RGB(224,230,236):RGB(40,48,56)));
        pDC->DrawText(data.m_stations.at(label.id).name,rect,DT_LEFT|DT_SINGLELINE|DT_NOPREFIX);
        ++m_visibleLabelCount;
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
		m_drawOriginX + (int)((position.x - m_minDataX) * m_drawScale * m_drawAspectX + 0.5),
		m_drawOriginY + (int)((position.y - m_minDataY) * m_drawScale * m_drawAspectY + 0.5));
}

// 线网版式只在首次绘制时构建一次；站点数据在运行期不变。
const MetroLayout::Layout& CNanjingMetroView::Metro() const
{
	if (!m_layout.Ready())
	{
		CNanjingMetroDoc* pDoc = GetDocument();
		if (pDoc)
			m_layout.Build(pDoc->m_metroData.m_stations, pDoc->m_metroData.m_lines);
	}
	return m_layout;
}

int CNanjingMetroView::HitTestStation(const CPoint& point) const // 编写者：刘子瑜（3号）
{
	if (!m_mapViewport.PtInRect(point))
		return -1;
	CNanjingMetroDoc* pDoc = GetDocument();
	if (!pDoc)
		return -1;

	const MetroLayout::Layout& metro = Metro();
	int closestId=-1, closestDistance=Ui(12)*Ui(12)+1;
	for (auto it = pDoc->m_metroData.m_stations.rbegin();
		it != pDoc->m_metroData.m_stations.rend(); ++it)
	{
		const CPoint center = StationToScreen(metro.MarkerPos(it->first));
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
    if(centerOnStation)m_selectedLineId=-1;
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
	const CPoint current = StationToScreen(Metro().MarkerPos(stationId));
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
    m_selectedLineId=-1;
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
    m_selectedLineId=-1;
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
	// 控件不是 OnDraw 的一部分，主题切换时需同步刷新其颜色。
	LayoutControls();
	Invalidate(FALSE);
}

void CNanjingMetroView::OnLButtonDown(UINT nFlags, CPoint point) // 编写者：刘子瑜（3号）
{
    for(const auto& item:m_lineLegend)if(item.first.PtInRect(point)){
        m_selectedLineId=item.second;ClearRoute();LayoutControls();Invalidate(FALSE);return;
    }
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
