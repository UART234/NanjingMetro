#pragma once
// 官方《南京地铁运营线路图》风格的绘制原语：圆角矩形、江道色带、
// 站点横杠、换乘同心圆、状态圆环。全部基于 GDI+，与 MapDrawing.h 的
// 线网描边共用同一套抗锯齿外观。
#include <algorithm>
#include <cmath>
#include <vector>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

namespace MapStyle
{
    struct GdiPlusHost
    {
        ULONG_PTR token = 0;
        GdiPlusHost() { Gdiplus::GdiplusStartupInput input; Gdiplus::GdiplusStartup(&token, &input, nullptr); }
        ~GdiPlusHost() { if (token) Gdiplus::GdiplusShutdown(token); }
    };
    inline GdiPlusHost& Host() { static GdiPlusHost host; return host; }

    inline Gdiplus::Color ToColor(COLORREF color, BYTE alpha = 255)
    {
        return Gdiplus::Color(alpha, GetRValue(color), GetGValue(color), GetBValue(color));
    }

    // 官方图的线路端点色签与图例色块：圆角矩形 + 可选同色描边。
    inline void FillRoundRect(CDC* dc, const CRect& rect, int radius, COLORREF fill, COLORREF border, float borderWidth)
    {
        if (rect.IsRectEmpty()) return;
        Host();
        const int limit = (std::min)(rect.Width(), rect.Height()) / 2;
        const int r = (std::max)(0, (std::min)(radius, limit));
        Gdiplus::Graphics graphics(dc->GetSafeHdc());
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        Gdiplus::GraphicsPath path;
        if (r <= 0)
        {
            path.AddRectangle(Gdiplus::Rect(rect.left, rect.top, rect.Width(), rect.Height()));
        }
        else
        {
            const int d = r * 2;
            path.AddArc(rect.left, rect.top, d, d, 180.0f, 90.0f);
            path.AddArc(rect.right - d, rect.top, d, d, 270.0f, 90.0f);
            path.AddArc(rect.right - d, rect.bottom - d, d, d, 0.0f, 90.0f);
            path.AddArc(rect.left, rect.bottom - d, d, d, 90.0f, 90.0f);
            path.CloseFigure();
        }
        Gdiplus::SolidBrush brush(ToColor(fill));
        graphics.FillPath(&brush, &path);
        if (borderWidth > 0.0f)
        {
            Gdiplus::Pen pen(ToColor(border), borderWidth);
            graphics.DrawPath(&pen, &path);
        }
    }

    // 粗细均匀的折线描边（圆角转折）：官方图的长江色带即由此描出。
    inline void StrokeThickPolyline(CDC* dc, const std::vector<CPoint>& points, COLORREF color, float width)
    {
        if (points.size() < 2) return;
        Host();
        std::vector<Gdiplus::PointF> scaled;
        scaled.reserve(points.size());
        for (const CPoint& point : points)
            scaled.push_back(Gdiplus::PointF((float)point.x, (float)point.y));
        Gdiplus::Graphics graphics(dc->GetSafeHdc());
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        Gdiplus::Pen pen(ToColor(color), width);
        pen.SetLineJoin(Gdiplus::LineJoinRound);
        pen.SetStartCap(Gdiplus::LineCapFlat);
        pen.SetEndCap(Gdiplus::LineCapFlat);
        graphics.DrawLines(&pen, scaled.data(), (INT)scaled.size());
    }

    // 圆环：换乘站的同心圆符号、以及选中/悬停的状态环都由它组合而成。
    inline void StrokeRing(CDC* dc, const CPoint& center, float radius, COLORREF ring, float ringWidth, COLORREF fill, bool filled)
    {
        if (radius <= 0.0f) return;
        Host();
        Gdiplus::Graphics graphics(dc->GetSafeHdc());
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        const Gdiplus::RectF bounds((float)center.x - radius, (float)center.y - radius, radius * 2.0f, radius * 2.0f);
        if (filled)
        {
            Gdiplus::SolidBrush brush(ToColor(fill));
            graphics.FillEllipse(&brush, bounds);
        }
        if (ringWidth > 0.0f)
        {
            Gdiplus::Pen pen(ToColor(ring), ringWidth);
            graphics.DrawEllipse(&pen, bounds);
        }
    }

    // 官方图普通车站符号：在线路上横切一道底色的短横杠，形成“缺口”。
    inline void StationTick(CDC* dc, const CPoint& center, double directionX, double directionY,
        COLORREF paper, int halfLength, int thickness)
    {
        double length = std::sqrt(directionX * directionX + directionY * directionY);
        if (length < 1e-6) { directionX = 0.0; directionY = 1.0; length = 1.0; }
        directionX /= length; directionY /= length;
        const double normalX = -directionY, normalY = directionX;
        const int dx = (int)std::lround(normalX * halfLength);
        const int dy = (int)std::lround(normalY * halfLength);
        CPen pen(PS_SOLID, (std::max)(1, thickness), paper);
        CPen* previous = dc->SelectObject(&pen);
        dc->MoveTo(center.x + dx, center.y + dy);
        dc->LineTo(center.x - dx, center.y - dy);
        dc->SelectObject(previous);
    }

    // 官方图普通车站符号：压在线路上的白色圆点，线路因此保持连续、不被切成虚线。
    inline void StationDot(CDC* dc, const CPoint& center, COLORREF paper, float radius)
    {
        if (radius <= 0.0f) return;
        StrokeRing(dc, center, radius, paper, 0.0f, paper, true);
    }

    // 官方图换乘车站符号：白底同心圆（保留给高亮路线等场景）。
    inline void TransferMark(CDC* dc, const CPoint& center, COLORREF paper, COLORREF ink,
        float outerRadius, float outerWidth, float innerRadius, float innerWidth)
    {
        StrokeRing(dc, center, outerRadius, ink, outerWidth, paper, true);
        if (innerRadius > 0.0f && innerWidth > 0.0f)
            StrokeRing(dc, center, innerRadius, ink, innerWidth, paper, false);
    }

    // 官方图换乘车站符号：沿线路走向的白色胶囊 + 细描边。
    // 官方图整张图只用这一个换乘符号，比同心圆轻得多，也不会把线路打成“靶心”。
    inline void TransferPill(CDC* dc, const CPoint& center, double directionX, double directionY,
        int longHalf, int shortHalf, COLORREF paper, COLORREF ink, float outlineWidth)
    {
        Host();
        const float w=(float)(std::max)(2,longHalf*2),h=(float)(std::max)(2,shortHalf*2);
        Gdiplus::Graphics g(dc->GetSafeHdc());g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        g.TranslateTransform((float)center.x,(float)center.y);
        g.RotateTransform((float)(std::atan2(directionY,directionX)*180.0/3.141592653589793));
        Gdiplus::GraphicsPath path;
        path.AddArc(-w/2,-h/2,h,h,90,180);path.AddArc(w/2-h,-h/2,h,h,270,180);path.CloseFigure();
        Gdiplus::SolidBrush fill(ToColor(paper));g.FillPath(&fill,&path);
        if(outlineWidth>0){Gdiplus::Pen pen(ToColor(ink),outlineWidth);g.DrawPath(&pen,&path);}
    }
}
