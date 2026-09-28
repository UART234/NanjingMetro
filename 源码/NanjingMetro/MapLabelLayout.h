#pragma once
#include <afxwin.h>
#include <algorithm>
#include <cmath>
#include <vector>

// 仅在屏幕坐标中排版，不修改站点数据、线网或路径结果。
namespace MapLabels
{
    inline bool Overlap(const CRect& a, const CRect& b, int spacing = 0)
    {
        return a.left < b.right + spacing && a.right + spacing > b.left &&
            a.top < b.bottom + spacing && a.bottom + spacing > b.top;
    }

    // 线段与外扩文字矩形的精确裁剪。避免斜线包围盒把两侧空白整块封死。
    inline bool HitsStroke(const CRect& box, CPoint a, CPoint b, double padding)
    {
        double lo = 0.0, hi = 1.0;
        auto slab = [&](double origin, double delta, double low, double high) {
            if (std::fabs(delta) < 1e-9) return origin >= low && origin <= high;
            double p = (low - origin) / delta, q = (high - origin) / delta;
            if (p > q) std::swap(p, q);
            lo = (std::max)(lo, p); hi = (std::min)(hi, q);
            return lo <= hi;
        };
        return slab(a.x, b.x - a.x, box.left - padding, box.right + padding) &&
            slab(a.y, b.y - a.y, box.top - padding, box.bottom + padding);
    }

    struct Label
    {
        int id = -1, priority = 4, chosen = -1;
        std::vector<CRect> candidates; // 已通过线路、站点、视口等固定障碍检查。
    };

    inline void Arrange(std::vector<Label>& labels, int spacing)
    {
        // 同优先级先安排选择少的站点，给密集区留位置；ID保证结果稳定。
        std::stable_sort(labels.begin(), labels.end(), [](const Label& a, const Label& b) {
            if (a.priority != b.priority) return a.priority < b.priority;
            if (a.candidates.size() != b.candidates.size()) return a.candidates.size() < b.candidates.size();
            return a.id < b.id;
        });
        auto free = [&](const CRect& rect, int self, int ignore) {
            for (int i = 0; i < (int)labels.size(); ++i)
                if (i != self && i != ignore && labels[i].chosen >= 0 &&
                    Overlap(rect, labels[i].candidates[labels[i].chosen], spacing)) return false;
            return true;
        };
        for (int i = 0; i < (int)labels.size(); ++i) {
            labels[i].chosen = -1;
            for (int c = 0; c < (int)labels[i].candidates.size(); ++c)
                if (free(labels[i].candidates[c], i, -1)) { labels[i].chosen = c; break; }
        }
        // 有界补位：若仅被一个文字挡住，尝试让它挪到另一合法位置；不牺牲已有标签。
        for (int i = 0; i < (int)labels.size(); ++i) {
            if (labels[i].chosen >= 0) continue;
            for (int c = 0; c < (int)labels[i].candidates.size() && labels[i].chosen < 0; ++c) {
                const CRect& candidate = labels[i].candidates[c];
                int blocker = -1; bool multiple = false;
                for (int j = 0; j < (int)labels.size(); ++j) {
                    if (j == i || labels[j].chosen < 0 || !Overlap(candidate, labels[j].candidates[labels[j].chosen], spacing)) continue;
                    if (blocker >= 0) { multiple = true; break; }
                    blocker = j;
                }
                if (multiple) continue;
                if (blocker < 0) { labels[i].chosen = c; break; }
                for (int k = 0; k < (int)labels[blocker].candidates.size(); ++k) {
                    const CRect& alternative = labels[blocker].candidates[k];
                    if (!Overlap(candidate, alternative, spacing) && free(alternative, blocker, i)) {
                        labels[blocker].chosen = k; labels[i].chosen = c; break;
                    }
                }
            }
        }
    }
}
