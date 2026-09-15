#pragma once
#include "MetroDef.h"

// 4号扩展模块：独立的站点/线路/方向时刻快照，不改变3号 StationNode 接口。
struct ServiceTime
{
    CString stationName;
    int lineId = 0;
    CString direction;
    CString first;
    CString last;
    CString lastFriSat;
    CString extraDirectionSunThu;
    CString extraLastSunThu;
};

class CServiceTimetable
{
public:
    bool Load(const CString& path);
    std::vector<ServiceTime> ForStation(const CString& name, int lineId = 0) const;
    static const CServiceTimetable& Instance();
    static CString Notice();
    static CString DisplayTime(const CString& value);
    static CString Supplement(const ServiceTime& row);
    size_t Count() const /* 编写者：肖博腾（4号） */ { return m_rows.size(); }
private:
    std::vector<ServiceTime> m_rows;
};
