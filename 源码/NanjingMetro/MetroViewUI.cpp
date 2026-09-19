// 4号模块：布局、站点详情、方向时刻卡、内嵌路线及查询联动。
#include "pch.h"
#include "framework.h"
#include "NanjingMetro.h"
#include "NanjingMetroDoc.h"
#include "NanjingMetroView.h"
#include "ServiceTimetable.h"
#include <algorithm>

namespace {
enum { Search=5001, Swap, Plan, SetStart, SetEnd, StationTab, RouteTab, CopyRoute, Favorite, Reset };
void Text(CDC* dc, const CString& text, CRect rect, COLORREF color, UINT flags = DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS) // 编写者：何彦毅（1号）
{
    dc->SetTextColor(color); dc->DrawText(text, rect, flags | DT_NOPREFIX);
}
}
int CNanjingMetroView::Ui(int value) const /* 编写者：何彦毅（1号） */ { return MulDiv(value, (int)GetDpiForWindow(m_hWnd), 96); }

void CNanjingMetroView::OnInitialUpdate() // 编写者：何彦毅（1号）
{
    CView::OnInitialUpdate();
    if (m_startCombo.GetSafeHwnd()) {
        m_selectedStationId=21;m_routeStartStationId=m_routeEndStationId=-1;
        m_zoom=1.0;m_panOffset=CPoint(0,0);ClearRoute();SyncEndpoints();Invalidate(FALSE);return;
    }
    m_uiFont.CreateFont(-Ui(14),0,0,0,FW_NORMAL,FALSE,FALSE,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,_T("Microsoft YaHei UI"));
    const DWORD comboStyle=WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL;
    m_startCombo.Create(comboStyle, CRect(0,0,100,300),this,5020);
    m_endCombo.Create(comboStyle, CRect(0,0,100,300),this,5021);
    m_strategyCombo.Create(comboStyle, CRect(0,0,100,200),this,5022);
    m_startCombo.AddString(_T("选择起点")); m_startCombo.SetItemData(0,(DWORD_PTR)-1);
    m_endCombo.AddString(_T("选择终点")); m_endCombo.SetItemData(0,(DWORD_PTR)-1);
    for(const auto& pair:GetDocument()->m_metroData.m_stations) {
        int a=m_startCombo.AddString(pair.second.name), b=m_endCombo.AddString(pair.second.name);
        m_startCombo.SetItemData(a,pair.first); m_endCombo.SetItemData(b,pair.first);
    }
    m_startCombo.SetCurSel(0); m_endCombo.SetCurSel(0);
    m_strategyCombo.AddString(_T("最短距离")); m_strategyCombo.AddString(_T("最少站数")); m_strategyCombo.AddString(_T("最少换乘")); m_strategyCombo.SetCurSel(0);
    auto button=[&](CButton& b,LPCTSTR title,UINT id) { b.Create(title,WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,CRect(0,0,10,10),this,id); };
    button(m_searchButton,_T("搜索站点 / 地标"),Search); button(m_swapButton,_T("互换"),Swap); button(m_planButton,_T("查询路线"),Plan);
    m_searchButton.m_bDontUseWinXPTheme=TRUE;
    m_searchButton.m_nFlatStyle=CMFCButton::BUTTONSTYLE_FLAT;
    m_searchButton.SetFaceColor(RGB(0,119,163));
    m_searchButton.SetTextColor(RGB(255,255,255));
    m_searchButton.SetTextHotColor(RGB(255,255,255));
    button(m_startButton,_T("设为起点"),SetStart); button(m_endButton,_T("设为终点"),SetEnd);
    button(m_stationTab,_T("站点 · 首末车"),StationTab); button(m_routeTab,_T("乘车方案"),RouteTab);
    button(m_copyButton,_T("复制指引"),CopyRoute); button(m_favoriteButton,_T("收藏路线"),Favorite);
    button(m_resetMapButton,_T("还原"),Reset);
    m_detailScroll.Create(WS_CHILD|WS_VISIBLE|SBS_VERT,CRect(0,0,10,10),this,5030);
    for(CWnd* child=GetWindow(GW_CHILD);child;child=child->GetNextWindow()) child->SetFont(&m_uiFont);
    m_selectedStationId=21;
    LayoutControls();
}

void CNanjingMetroView::LayoutControls() // 编写者：何彦毅（1号）
{
    CRect r; GetClientRect(r);
    int side=Ui(354), gap=Ui(16), top=Ui(164);
    m_panelRect=CRect(r.right-gap-side,top,r.right-gap,r.bottom-Ui(34));
    m_mapViewport=CRect(gap,top,m_panelRect.left-gap,r.bottom-Ui(34));
    m_detailArea=CRect(m_panelRect.left+Ui(18),top+Ui(124),m_panelRect.right-Ui(28),m_panelRect.bottom-Ui(62));
    if(!m_startCombo.GetSafeHwnd())return;
    auto move=[](CWnd& w,CRect rect){ CRect old;w.GetWindowRect(old);w.GetParent()->ScreenToClient(old);if(old!=rect)w.MoveWindow(rect,TRUE); };
    int y=Ui(119),h=Ui(30), x=gap+Ui(16), available=m_mapViewport.Width()-Ui(32);
    move(m_searchButton,CRect(x,y-Ui(2),x+Ui(180),y+h+Ui(2)));
    x+=Ui(192); available-=Ui(192);
    int combo=(available-Ui(74))/2;
    move(m_startCombo,CRect(x,y,x+combo,y+Ui(290)));
    move(m_swapButton,CRect(x+combo+Ui(10),y,x+combo+Ui(64),y+h));
    move(m_endCombo,CRect(x+combo+Ui(74),y,m_mapViewport.right-Ui(16),y+Ui(290)));
    x=m_panelRect.left; move(m_strategyCombo,CRect(x,y,x+Ui(128),y+Ui(200)));
    move(m_planButton,CRect(x+Ui(140),y,m_panelRect.right,y+h));
    move(m_resetMapButton,CRect(m_mapViewport.right-Ui(96),m_mapViewport.bottom-Ui(48),m_mapViewport.right-Ui(16),m_mapViewport.bottom-Ui(14)));
    int mid=m_panelRect.CenterPoint().x;
    move(m_stationTab,CRect(m_panelRect.left+Ui(12),top+Ui(12),mid-Ui(4),top+Ui(44)));
    move(m_routeTab,CRect(mid+Ui(4),top+Ui(12),m_panelRect.right-Ui(12),top+Ui(44)));
    CRect left(m_panelRect.left+Ui(18),m_panelRect.bottom-Ui(46),mid-Ui(6),m_panelRect.bottom-Ui(12));
    CRect right(mid+Ui(6),left.top,m_panelRect.right-Ui(18),left.bottom);
    move(m_startButton,left);move(m_copyButton,left);move(m_endButton,right);move(m_favoriteButton,right);
    m_startButton.ShowWindow(m_showRoute?SW_HIDE:SW_SHOW);m_endButton.ShowWindow(m_showRoute?SW_HIDE:SW_SHOW);
    m_copyButton.ShowWindow(m_showRoute?SW_SHOW:SW_HIDE);m_favoriteButton.ShowWindow(m_showRoute?SW_SHOW:SW_HIDE);
    bool selected=GetDocument() && GetDocument()->m_metroData.GetStationById(m_selectedStationId);
    m_startButton.EnableWindow(selected);m_endButton.EnableWindow(selected);
    m_copyButton.EnableWindow(m_lastRoute.isFound);
    bool favorite = m_lastRoute.isFound && GetDocument()->m_historyMgr.IsFavorite(m_routeStartStationId,m_routeEndStationId,m_lastRoute.strategy);
    m_favoriteButton.SetWindowText(favorite ? _T("已收藏") : _T("收藏路线"));
    m_favoriteButton.EnableWindow(m_lastRoute.isFound && !favorite);
    m_planButton.EnableWindow(m_routeStartStationId>0 && m_routeEndStationId>0 && m_routeStartStationId!=m_routeEndStationId);
    move(m_detailScroll,CRect(m_panelRect.right-Ui(18),m_detailArea.top,m_panelRect.right-Ui(4),m_detailArea.bottom));
}
void CNanjingMetroView::OnSize(UINT type,int cx,int cy) /* 编写者：何彦毅（1号） */ { CView::OnSize(type,cx,cy);LayoutControls();RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN); }
void CNanjingMetroView::SyncEndpoints() // 编写者：肖博腾（4号）
{
    if(!m_startCombo.GetSafeHwnd())return;
    for(int i=0;i<m_startCombo.GetCount();++i)if((int)m_startCombo.GetItemData(i)==m_routeStartStationId)m_startCombo.SetCurSel(i);
    for(int i=0;i<m_endCombo.GetCount();++i)if((int)m_endCombo.GetItemData(i)==m_routeEndStationId)m_endCombo.SetCurSel(i);
    LayoutControls();
}
void CNanjingMetroView::ClearRoute() /* 编写者：肖博腾（4号） */ { m_highlightStationIds.clear();m_lastRoute=PathResult();m_showRoute=false;m_detailOffset=0;m_routeNotice.Empty(); }
void CNanjingMetroView::OnEndpointChanged() // 编写者：肖博腾（4号）
{
    m_routeStartStationId=(int)m_startCombo.GetItemData(m_startCombo.GetCurSel());
    m_routeEndStationId=(int)m_endCombo.GetItemData(m_endCombo.GetCurSel());
    ClearRoute();LayoutControls();Invalidate(FALSE);
}
BOOL CNanjingMetroView::PreTranslateMessage(MSG* message) // 编写者：肖博腾（4号）
{
    if(message->message==WM_KEYDOWN && message->wParam==VK_RETURN &&
        (GetFocus()==&m_startCombo||GetFocus()==&m_endCombo||GetFocus()==&m_strategyCombo)) {
        QuerySelectedRoute((RouteStrategy)m_strategyCombo.GetCurSel());return TRUE;
    }
    if(message->message==WM_KEYDOWN && message->wParam==VK_TAB) {
        CWnd* next=GetNextDlgTabItem(GetFocus(),(GetKeyState(VK_SHIFT)&0x8000)!=0);if(next){next->SetFocus();return TRUE;}
    }
    return CView::PreTranslateMessage(message);
}
void CNanjingMetroView::OnUiAction(UINT id) // 编写者：肖博腾（4号）
{
    switch(id) {
    case Search:OnQueryStation();break;
    case Reset:OnViewResetMap();break;
    case Swap:std::swap(m_routeStartStationId,m_routeEndStationId);ClearRoute();SyncEndpoints();break;
    case Plan:QuerySelectedRoute((RouteStrategy)m_strategyCombo.GetCurSel());break;
    case SetStart:SetRouteEndpoint(m_selectedStationId,true);break;
    case SetEnd:SetRouteEndpoint(m_selectedStationId,false);break;
    case StationTab:m_showRoute=false;m_detailOffset=0;break;
    case RouteTab:m_showRoute=true;m_detailOffset=0;break;
    case CopyRoute: {
        CString text=RouteText();if(text.IsEmpty()||!OpenClipboard())break;
        HGLOBAL block=GlobalAlloc(GMEM_MOVEABLE,(text.GetLength()+1)*sizeof(wchar_t));
        if(block){void* memory=GlobalLock(block);if(memory){memcpy(memory,text.GetString(),(text.GetLength()+1)*sizeof(wchar_t));GlobalUnlock(block);EmptyClipboard();if(SetClipboardData(CF_UNICODETEXT,block)){m_routeNotice=_T("乘车指引已复制");block=nullptr;}}if(block)GlobalFree(block);}
        CloseClipboard();break;
    }
    case Favorite:if(m_lastRoute.isFound){
        auto& manager=GetDocument()->m_historyMgr;
        const auto before=manager;
        manager.AddRecord(m_routeStartStationId,m_lastRoute.startStationName,m_routeEndStationId,m_lastRoute.endStationName,m_lastRoute.strategy,true);
        if(manager.SaveToFile(_T("user_history.txt"))) m_routeNotice=_T("已加入收藏，可从收藏菜单再次查询");
        else {manager=before;m_routeNotice=_T("收藏未能保存，请检查程序目录是否可写后重试");}
    }break;
    }
    LayoutControls();Invalidate(FALSE);
}
void CNanjingMetroView::OnVScroll(UINT code,UINT pos,CScrollBar* bar) // 编写者：肖博腾（4号）
{
    if(bar!=&m_detailScroll){CView::OnVScroll(code,pos,bar);return;}
    switch(code){case SB_LINEUP:m_detailOffset-=Ui(28);break;case SB_LINEDOWN:m_detailOffset+=Ui(28);break;
    case SB_PAGEUP:m_detailOffset-=m_detailArea.Height();break;case SB_PAGEDOWN:m_detailOffset+=m_detailArea.Height();break;
    case SB_THUMBTRACK:case SB_THUMBPOSITION:m_detailOffset=(int)pos;break;case SB_TOP:m_detailOffset=0;break;case SB_BOTTOM:m_detailOffset=m_detailMax;break;}
    m_detailOffset=(std::max)(0,(std::min)(m_detailMax,m_detailOffset));Invalidate(FALSE);
}

void CNanjingMetroView::DrawInteractionHeader(CDC* dc,const CRect& r) // 编写者：何彦毅（1号）
{
    COLORREF ink=m_darkTheme?RGB(230,237,246):RGB(27,48,67), muted=m_darkTheme?RGB(159,177,194):RGB(103,122,138);
    CRect header(Ui(16),Ui(12),r.right-Ui(16),Ui(104));dc->FillSolidRect(header,m_darkTheme?RGB(42,49,58):RGB(255,255,255));
    CFont title;title.CreateFont(-Ui(26),0,0,0,FW_BOLD,FALSE,FALSE,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,_T("Microsoft YaHei UI"));
    CFont* old=dc->SelectObject(&title);
    Text(dc,_T("南京地铁 · 出行指南"),CRect(Ui(32),Ui(19),r.right-Ui(224),Ui(54)),ink);
    dc->SelectObject(&m_uiFont);
    Text(dc,_T("站点查询 / 周边导向 / 首末班车"),CRect(Ui(33),Ui(56),r.right-Ui(224),Ui(79)),muted);
    const std::vector<MetroLine>& lines=GetDocument()->m_metroData.m_lines;
    int x=Ui(33),y=Ui(90);
    const int legendLimit=r.right-Ui(20);
    for(const auto& line:lines){
        if(x+Ui(74)>legendLimit){x=Ui(33);y+=Ui(20);} // 线路过多时自动换行
        dc->FillSolidRect(x,y,Ui(20),Ui(4),line.color);Text(dc,line.lineName,CRect(x+Ui(27),y-Ui(9),x+Ui(90),y+Ui(13)),ink);x+=Ui(93);
    }
    CString status;status.Format(_T("%d 条线路 · %d 站    |    缩放 %d%%    |    拖动平移 · 滚轮缩放 · Ctrl+F 搜索"),(int)lines.size(),(int)GetDocument()->m_metroData.m_stations.size(),(int)(m_zoom*100));
    Text(dc,status,CRect(Ui(20),r.bottom-Ui(28),r.right-Ui(20),r.bottom-Ui(5)),muted);
    dc->SelectObject(old);
}
void CNanjingMetroView::DrawGeography(CDC* dc) // 编写者：刘子瑜（3号）
{
    // 示意长江，与柳洲东路—上元门、刘村—马骡圩过江段相交，不作为精确地理底图。
    CPoint river[]={CPoint(210,1450),CPoint(210,750),CPoint(505,377),CPoint(1150,377),CPoint(1150,418),CPoint(533,418),CPoint(265,770),CPoint(265,1450)};
    for(auto& pt:river)pt=StationToScreen(pt);
    CBrush brush(m_darkTheme?RGB(36,64,81):RGB(233,244,250));CBrush* old=dc->SelectObject(&brush);CPen* pen=(CPen*)dc->SelectStockObject(NULL_PEN);dc->Polygon(river,_countof(river));dc->SelectObject(pen);dc->SelectObject(old);
    CFont* font=dc->SelectObject(&m_uiFont);int x=m_mapViewport.left+Ui(22),y=m_mapViewport.top+Ui(18);
    Text(dc,_T("N"),CRect(x-Ui(5),y,x+Ui(16),y+Ui(20)),RGB(114,141,161));
    CPen arrow(PS_SOLID,Ui(2),RGB(114,141,161));pen=dc->SelectObject(&arrow);dc->MoveTo(x+Ui(5),y+Ui(47));dc->LineTo(x+Ui(5),y+Ui(24));dc->LineTo(x,y+Ui(32));dc->MoveTo(x+Ui(5),y+Ui(24));dc->LineTo(x+Ui(10),y+Ui(32));dc->SelectObject(pen);
    Text(dc,_T("长江 / 示意"),CRect(x,y+Ui(54),x+Ui(94),y+Ui(78)),RGB(130,158,178));dc->SelectObject(font);
}

CString CNanjingMetroView::RouteText() const // 编写者：何彦毅（1号）
{
    if(!m_lastRoute.isFound)return CString();
    CString result;result.Format(_T("%s → %s\r\n%d站 · %.3f公里 · 参考票价%d元 · %d次换乘\r\n"),m_lastRoute.startStationName.GetString(),m_lastRoute.endStationName.GetString(),m_lastRoute.totalStations,m_lastRoute.totalDistanceKm,m_lastRoute.ticketPrice,(int)m_lastRoute.transferStations.size());
    for(const auto& seg:m_lastRoute.transferGuides){CString row;row.Format(_T("乘%s（往%s）：%s → %s，%d站\r\n"),seg.lineName.GetString(),seg.directionStationName.GetString(),seg.startStationName.GetString(),seg.endStationName.GetString(),seg.passStationCount);result+=row;}
    result+=_T("里程为课程数据；首末班车请查看各上车站时刻卡，换乘接驳以车站公告为准。");return result;
}
void CNanjingMetroView::DrawStationDetails(CDC* dc,const CRect& client) // 编写者：何彦毅（1号）
{
    UNREFERENCED_PARAMETER(client);
    COLORREF ink=m_darkTheme?RGB(230,237,246):RGB(27,48,67), muted=m_darkTheme?RGB(159,177,194):RGB(103,122,138);
    COLORREF paper=m_darkTheme?RGB(42,49,58):RGB(255,255,255),rule=m_darkTheme?RGB(64,77,89):RGB(228,235,240);
    dc->FillSolidRect(m_panelRect,paper);CFont* old=dc->SelectObject(&m_uiFont);
    CRect indicator=m_showRoute?CRect(m_panelRect.CenterPoint().x+Ui(4),m_panelRect.top+Ui(48),m_panelRect.right-Ui(12),m_panelRect.top+Ui(51)):CRect(m_panelRect.left+Ui(12),m_panelRect.top+Ui(48),m_panelRect.CenterPoint().x-Ui(4),m_panelRect.top+Ui(51));
    dc->FillSolidRect(indicator,RGB(0,145,204));
    StationNode* station=GetDocument()->m_metroData.GetStationById(m_selectedStationId);
    CFont title;title.CreateFont(-Ui(23),0,0,0,FW_BOLD,FALSE,FALSE,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,_T("Microsoft YaHei UI"));
    dc->SelectObject(&title);
    Text(dc,m_showRoute?_T("乘车方案"):(station?station->name:_T("探索一座车站")),CRect(m_panelRect.left+Ui(18),m_panelRect.top+Ui(60),m_panelRect.right-Ui(18),m_panelRect.top+Ui(92)),ink);
    dc->SelectObject(&m_uiFont);
    CString sub=m_showRoute?(m_lastRoute.isFound?_T("地图已高亮 · 向下滚动查看完整指引"):_T("选好起终点，开始规划")):(station?(station->isTransfer?_T("换乘站 · 非实时运营表"):_T("普通站 · 非实时运营表")):_T("点击地图，或搜索站名 / 地标"));
    if(!m_routeNotice.IsEmpty()&&m_showRoute)sub=m_routeNotice;
    Text(dc,sub,CRect(m_panelRect.left+Ui(18),m_panelRect.top+Ui(96),m_panelRect.right-Ui(18),m_panelRect.top+Ui(118)),muted);
    int saved=dc->SaveDC();dc->IntersectClipRect(m_detailArea);
    int y=m_detailArea.top-m_detailOffset,x=m_detailArea.left,w=m_detailArea.Width();
    auto paragraph=[&](const CString& text,COLORREF color,int spacing=8){CRect rect(x,y,x+w,y);dc->DrawText(text,rect,DT_LEFT|DT_WORDBREAK|DT_CALCRECT|DT_NOPREFIX);rect.bottom=(std::max)(rect.bottom,rect.top+Ui(21));Text(dc,text,rect,color,DT_LEFT|DT_WORDBREAK);y=rect.bottom+Ui(spacing);};
    auto section=[&](const CString& text){y+=Ui(8);dc->FillSolidRect(x,y,w,1,rule);y+=Ui(13);paragraph(text,ink,10);};
    if(m_showRoute){
        if(!m_lastRoute.isFound)paragraph(_T("在顶部选择起点与终点，然后点击“查询路线”。可切换最短距离、最少站数和最少换乘。"),muted);
        else{
            paragraph(m_lastRoute.startStationName+_T(" → ")+m_lastRoute.endStationName,ink);
            CString metrics;metrics.Format(_T("%d 站   /   %d 次换乘   /   %d 元\r\n%.3f km   ·   估算约 %d 分钟"),m_lastRoute.totalStations,(int)m_lastRoute.transferStations.size(),m_lastRoute.ticketPrice,m_lastRoute.totalDistanceKm,m_lastRoute.totalStations*2+(int)m_lastRoute.transferStations.size()*2);paragraph(metrics,RGB(0,145,204));
            paragraph(_T("里程/票价基于课程数据；用时按每站2分钟、每次换乘2分钟估算。"),muted);
            section(_T("乘车指引"));
            for(size_t i=0;i<m_lastRoute.transferGuides.size();++i){const auto& seg=m_lastRoute.transferGuides[i];CString step;step.Format(_T("%d  %s · 往%s\r\n%s → %s（%d站）"),(int)i+1,seg.lineName.GetString(),seg.directionStationName.GetString(),seg.startStationName.GetString(),seg.endStationName.GetString(),seg.passStationCount);paragraph(step,ink,12);
                auto times=CServiceTimetable::Instance().ForStation(seg.startStationName,seg.fromLineId);bool matched=false;
                for(const auto& row:times)if(row.direction==seg.directionStationName){paragraph(_T("周日至周四：首 ")+CServiceTimetable::DisplayTime(row.first)+_T(" / 末 ")+CServiceTimetable::DisplayTime(row.last),muted);CString extra=CServiceTimetable::Supplement(row);if(!extra.IsEmpty())paragraph(extra,muted);matched=true;}
                if(!matched)paragraph(_T("该方向时刻待核实，请查看上车站首末车卡。"),muted);
            }
            section(_T("沿途车站（含起终点）"));
            for(size_t i=0;i<m_lastRoute.stationSequence.size();++i){auto* s=GetDocument()->m_metroData.GetStationById(m_lastRoute.stationSequence[i]);if(s){CString row;row.Format(_T("%02d  %s"),(int)i+1,s->name.GetString());paragraph(row,ink,3);}}
            section(_T("出行提示"));paragraph(CServiceTimetable::Notice(),muted);paragraph(_T("末班车接驳需分别核对各换乘站，当前路线规划不判断当日是否赶得上末班车。"),muted);
        }
    }else if(station){
        paragraph(_T("途经线路  ")+GetStationLineNames(*station),ink);
        section(_T("首末班车 · 周日至周四"));
        for(const auto& line:GetDocument()->m_metroData.m_lines){if(std::find(station->lineIds.begin(),station->lineIds.end(),line.lineId)==station->lineIds.end())continue;
            paragraph(line.lineName,line.color,4);auto rows=CServiceTimetable::Instance().ForStation(station->name,line.lineId);
            if(rows.empty())paragraph(_T("暂无本站时刻，请以车站公告为准。"),muted);
            for(const auto& row:rows){paragraph(_T("开往 ")+row.direction,ink,3);int half=(w-Ui(8))/2;
                CRect first(x,y,x+half,y+Ui(28)),last(x+half+Ui(8),y,x+w,y+Ui(28));dc->FillSolidRect(first,RGB(237,99,76));dc->FillSolidRect(last,RGB(73,150,107));
                Text(dc,_T("首 ")+CServiceTimetable::DisplayTime(row.first),first,RGB(255,255,255),DT_CENTER|DT_VCENTER|DT_SINGLELINE);
                Text(dc,_T("末 ")+CServiceTimetable::DisplayTime(row.last),last,RGB(255,255,255),DT_CENTER|DT_VCENTER|DT_SINGLELINE);y+=Ui(38);
                CString extra=CServiceTimetable::Supplement(row);if(!extra.IsEmpty())paragraph(extra,muted);
            }y+=Ui(4);
        }
        paragraph(CServiceTimetable::Notice(),muted);
        section(_T("周边地标与出站导向"));
        if(station->landmarks.empty())paragraph(_T("暂无周边地标记录，可搜索其他站点。"),muted);
        for(const auto& land:station->landmarks){paragraph(_T("[")+GetLandmarkTypeName(land.type)+_T("] ")+land.name,m_darkTheme?RGB(118,212,185):RGB(23,123,99),3);paragraph(_T("  出站指引：")+land.description,muted,12);}
    }else paragraph(_T("试试搜索“总统府”“德基广场”或“njz”。选中车站后可查看周边与首末班车，并一键设为起点或终点。"),muted);
    int height=y+m_detailOffset-m_detailArea.top;
    dc->RestoreDC(saved);m_detailMax=(std::max)(0,height-m_detailArea.Height());
    if(m_detailOffset>m_detailMax){m_detailOffset=m_detailMax;Invalidate(FALSE);}
    SCROLLINFO si={sizeof(SCROLLINFO),SIF_RANGE|SIF_PAGE|SIF_POS,0,(std::max)(0,height-1),(UINT)(std::max)(1,m_detailArea.Height()),m_detailOffset,0};
    if(m_detailScroll.GetSafeHwnd())m_detailScroll.SetScrollInfo(&si,TRUE);
    dc->FillSolidRect(m_panelRect.left+Ui(18),m_panelRect.bottom-Ui(57),m_panelRect.Width()-Ui(36),1,rule);
    dc->SelectObject(old);
}
