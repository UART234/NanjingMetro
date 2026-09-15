#pragma once
#include <algorithm>
#include <vector>
#include <gdiplus.h>
#include <cmath>
#pragma comment(lib, "gdiplus.lib")

// 站点中心保持精确，圆角仅作用于站间辅助折点。底图与查询路线共用。
inline void DrawMapStroke(CDC* dc, const std::vector<CPoint>& points, COLORREF color, float width) // 编写者：刘子瑜（3号）
{
    struct Runtime {
        ULONG_PTR token = 0;
        Runtime() /* 编写者：刘子瑜（3号） */ { Gdiplus::GdiplusStartupInput input; Gdiplus::GdiplusStartup(&token, &input, nullptr); }
        ~Runtime() /* 编写者：刘子瑜（3号） */ { if (token) Gdiplus::GdiplusShutdown(token); }
    };
    static Runtime runtime;
    if (points.size() < 2) return;
    Gdiplus::Graphics graphics(dc->GetSafeHdc());
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    Gdiplus::Pen pen(Gdiplus::Color(255, GetRValue(color), GetGValue(color), GetBValue(color)), width);
    pen.SetStartCap(Gdiplus::LineCapRound); pen.SetEndCap(Gdiplus::LineCapRound);
    pen.SetLineJoin(Gdiplus::LineJoinRound);
    auto p = [](CPoint a) { return Gdiplus::PointF((float)a.x, (float)a.y); };
    Gdiplus::GraphicsPath path;
    Gdiplus::PointF previous = p(points.front());
    for (size_t i = 1; i + 1 < points.size(); ++i) {
        auto a = p(points[i-1]), b = p(points[i]), c = p(points[i+1]);
        float incoming = std::hypot(b.X-a.X, b.Y-a.Y), outgoing = std::hypot(c.X-b.X, c.Y-b.Y);
        if (incoming < 0.5f || outgoing < 0.5f) continue;
        float radius = (std::min)(8.0f, (std::min)(incoming, outgoing) * 0.4f);
        Gdiplus::PointF before(b.X+(a.X-b.X)*radius/incoming, b.Y+(a.Y-b.Y)*radius/incoming);
        Gdiplus::PointF after(b.X+(c.X-b.X)*radius/outgoing, b.Y+(c.Y-b.Y)*radius/outgoing);
        path.AddLine(previous, before);
        path.AddBezier(before, Gdiplus::PointF(before.X+(b.X-before.X)*0.667f,before.Y+(b.Y-before.Y)*0.667f),
            Gdiplus::PointF(after.X+(b.X-after.X)*0.667f,after.Y+(b.Y-after.Y)*0.667f), after);
        previous = after;
    }
    path.AddLine(previous, p(points.back()));
    graphics.DrawPath(&pen, &path);
}
