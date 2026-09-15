
// NanjingMetroView.h: CNanjingMetroView 类的接口
//

#pragma once

#include "MetroDef.h"
#include <vector>

class CNanjingMetroView : public CView
{
protected: // 仅从序列化创建
	CNanjingMetroView() noexcept;
	DECLARE_DYNCREATE(CNanjingMetroView)

// 特性
public:
	CNanjingMetroDoc* GetDocument() const;
	// Station IDs of the last planned route, drawn as a highlighted path.
	std::vector<int> m_highlightStationIds;

// 操作
public:
	// 结果展示: 将 PathResult 拼装为友好文本并弹窗呈现（何彦毅 - 1号模块）
	void ShowRouteResult(const PathResult& result);

	// 统一下发查询: 写入历史并调用结果展示（2号算法接入后在此替换寻路实现）
	void QueryRoute(int startId, const CString& startName,
		int endId, const CString& endName, RouteStrategy strategy);

// 重写
public:
	virtual void OnDraw(CDC* pDC);  // 重写以绘制该视图
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:

// 实现
public:
	virtual ~CNanjingMetroView();
    virtual void OnInitialUpdate();
    virtual BOOL PreTranslateMessage(MSG* message);
    afx_msg void OnSize(UINT type, int cx, int cy);
    afx_msg void OnVScroll(UINT code, UINT pos, CScrollBar* bar);
    afx_msg void OnUiAction(UINT id);
    afx_msg void OnEndpointChanged();
    int Ui(int value) const;
    void LayoutControls();
    void SyncEndpoints();
    void ClearRoute();
    void DrawGeography(CDC* dc);
    CString RouteText() const;
    CFont m_uiFont;
    CComboBox m_startCombo, m_endCombo, m_strategyCombo;
    CMFCButton m_searchButton;
    CButton m_resetMapButton;
    CButton m_swapButton, m_planButton, m_startButton, m_endButton;
    CButton m_stationTab, m_routeTab, m_copyButton, m_favoriteButton;
    CScrollBar m_detailScroll;
    CRect m_panelRect, m_detailArea;
    int m_detailOffset = 0, m_detailMax = 0;
    bool m_showRoute = false;
    PathResult m_lastRoute;
    CString m_routeNotice;
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	double m_zoom;
	CPoint m_panOffset;
	CPoint m_dragStartPoint;
	CPoint m_dragStartPan;
	bool m_dragging;
	bool m_dragMoved;
	bool m_darkTheme;
	int m_selectedStationId;
	int m_hoverStationId;
	int m_routeStartStationId;
	int m_routeEndStationId;
	double m_drawScale;
	double m_minDataX;
	double m_minDataY;
	int m_drawOriginX;
	int m_drawOriginY;
	CRect m_mapViewport;

	CPoint StationToScreen(const CPoint& position) const;
	int HitTestStation(const CPoint& point) const;
	void SelectStation(int stationId, bool centerOnStation);
	void CenterOnStation(int stationId);
	void SetRouteEndpoint(int stationId, bool asStart);
	void QuerySelectedRoute(RouteStrategy strategy);
	CString GetStationLineNames(const StationNode& station) const;
	CString GetLandmarkTypeName(LandmarkType type) const;
	void DrawInteractionHeader(CDC* pDC, const CRect& clientRect);
	void DrawStationDetails(CDC* pDC, const CRect& clientRect);

// 生成的消息映射函数
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnRouteShortest();
	afx_msg void OnRouteMinStation();
	afx_msg void OnRouteMinTransfer();
	afx_msg void OnQueryStation();
	afx_msg void OnQueryLandmark();
	afx_msg void OnFavView();
	afx_msg void OnHistoryList();
	afx_msg void OnHistoryClear();
	afx_msg void OnViewResetMap();
	afx_msg void OnViewThemeToggle();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnCaptureChanged(CWnd* pWnd);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};

#ifndef _DEBUG  // NanjingMetroView.cpp 中的调试版本
inline CNanjingMetroDoc* CNanjingMetroView::GetDocument() const // 编写者：何彦毅（1号）
   { return reinterpret_cast<CNanjingMetroDoc*>(m_pDocument); }
#endif

