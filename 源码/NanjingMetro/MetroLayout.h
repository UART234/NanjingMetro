#pragma once
// 显示位置、站点命中和路线高亮统一使用原图描摹版式。
#include "MetroDef.h"
#include "OfficialMapLayout.h"
#include <map>
#include <vector>
#include <algorithm>
#include <cmath>
namespace MetroLayout {
inline CPoint DisplayPosition(CPoint p) {return p;}
class Layout {
    struct Line {std::vector<CPoint> points;std::map<int,size_t> stations;};
    std::map<int,Line> m_lines;
    std::map<int,CPoint> m_positions,m_directions;
    std::map<int,int> m_members;
    bool m_ready=false;
public:
    bool Ready() const {return m_ready;}
    int Membership(int id) const {auto i=m_members.find(id);return i==m_members.end()?0:i->second;}
    bool IsMultiLine(int id) const {return Membership(id)>1;}
    void Build(const std::map<int,StationNode>& stations,const std::vector<MetroLine>& lines) {
        m_ready=false;m_lines.clear();m_positions.clear();m_directions.clear();m_members.clear();
        for(const auto& station:stations)m_positions[station.first]=OfficialMap::Position(station.first,station.second.pos);
        for(const auto& line:lines){
            Line geo;
            for(size_t i=0;i<line.stationIds.size();++i){
                int id=line.stationIds[i];if(!m_positions.count(id))continue;
                ++m_members[id];
                if(geo.points.empty())geo.points.push_back(m_positions.at(id));
                else {
                    int from=line.stationIds[i-1];
                    auto segment=OfficialMap::Segment(from,id,m_positions.at(from),m_positions.at(id));
                    geo.points.insert(geo.points.end(),segment.begin()+1,segment.end());
                }
                geo.stations[id]=geo.points.size()-1;
            }
            if(!geo.points.empty())m_lines[line.lineId]=geo;
        }
        for(const auto& station:m_positions){
            CPoint dir(1000,0);bool found=false;
            for(const auto& entry:m_lines){
                const auto& geo=entry.second;auto it=geo.stations.find(station.first);if(it==geo.stations.end())continue;
                size_t k=it->second;CPoint a=k>0?geo.points[k-1]:geo.points[k];
                CPoint b=k+1<geo.points.size()?geo.points[k+1]:geo.points[k];
                double dx=b.x-a.x,dy=b.y-a.y,length=std::hypot(dx,dy);if(length<0.5)continue;
                CPoint candidate((int)std::lround(dx/length*1000),(int)std::lround(dy/length*1000));
                bool axis=std::abs(candidate.x)<30 || std::abs(candidate.y)<30;
                if(!found || axis){dir=candidate;found=true;}if(axis)break;
            }
            m_directions[station.first]=dir;
        }
        m_ready=!m_lines.empty();
    }
    const std::vector<CPoint>* LinePoints(int lineId) const {
        auto i=m_lines.find(lineId);return i==m_lines.end()?nullptr:&i->second.points;
    }
    CPoint MarkerPos(int id) const {auto i=m_positions.find(id);return i==m_positions.end()?CPoint(0,0):i->second;}
    CPoint MarkerDir(int id) const {auto i=m_directions.find(id);return i==m_directions.end()?CPoint(1000,0):i->second;}
    CPoint LineStationPos(int lineId,int id) const {
        auto line=m_lines.find(lineId);if(line==m_lines.end())return MarkerPos(id);
        auto i=line->second.stations.find(id);return i==line->second.stations.end()?MarkerPos(id):line->second.points[i->second];
    }
    std::vector<CPoint> SegmentPoints(int lineId,int from,int to) const {
        std::vector<CPoint> result;auto line=m_lines.find(lineId);if(line==m_lines.end())return result;
        const auto& geo=line->second;auto a=geo.stations.find(from),b=geo.stations.find(to);
        if(a==geo.stations.end() || b==geo.stations.end())return result;
        int start=(int)a->second,end=(int)b->second,step=start<=end?1:-1;
        for(int i=start;;i+=step){result.push_back(geo.points[i]);if(i==end)break;}return result;
    }
};
}
