#include "pch.h"
#include "ServiceTimetable.h"
#include <fstream>
#include <sstream>
#include <set>

namespace {
CString FromUtf8(const std::string& text) // 编写者：肖博腾（4号）
{
    int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), (int)text.size(), nullptr, 0);
    if (length <= 0) return CString();
    CString result;
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), (int)text.size(), result.GetBuffer(length), length);
    result.ReleaseBuffer(length);
    return result;
}
bool ValidTime(const CString& t) // 编写者：肖博腾（4号）
{
    if (t == _T("--:--")) return true;
    return t.GetLength() == 5 && t[2] == ':' &&
        t[0] >= '0' && t[0] <= '2' && t[1] >= '0' && t[1] <= '9' &&
        t[3] >= '0' && t[3] <= '5' && t[4] >= '0' && t[4] <= '9' && _ttoi(t.Left(2)) < 24;
}
}
bool CServiceTimetable::Load(const CString& path) // 编写者：肖博腾（4号）
{
    m_rows.clear();
    std::ifstream input(path.GetString(), std::ios::binary);
    if (!input) return false;
    std::string line;
    std::set<std::wstring> keys;
    while (std::getline(input, line))
    {
        if (line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        std::istringstream stream(line);
        std::vector<CString> fields;
        std::string value;
        while (std::getline(stream, value, '|')) fields.push_back(FromUtf8(value));
        if ((fields.size() != 6 && fields.size() != 9) || fields[0].IsEmpty() || fields[2].IsEmpty()) continue;
        int id = _ttoi(fields[1]);
        CString canonical; canonical.Format(_T("%d"), id);
        // 允许全部 14 条已开通线路的线路 ID（1/2/3/4/5/7/10/S1/S2/S3/S6/S7/S8/S9）
        static const int kValidLineIds[] = { 1, 2, 3, 4, 5, 7, 10, 11, 12, 13, 14, 15, 16, 17 };
        bool validId = false;
        for (int k = 0; k < (int)(sizeof(kValidLineIds) / sizeof(kValidLineIds[0])); ++k)
            if (id == kValidLineIds[k]) { validId = true; break; }
        if (canonical != fields[1] || !validId) continue;
        if (!ValidTime(fields[3]) || !ValidTime(fields[4]) ||
            (fields[5] != _T("amap-2026-09-09") && fields[5] != _T("njmetro-2025-12-19") &&
             fields[5] != _T("njmetro-2026-09-19") && fields[5] != _T("official-2026-09-19"))) continue;
        if(fields.size()==9 && (!ValidTime(fields[6]) ||
            ((fields[7]==_T("-")) != (fields[8]==_T("-"))) ||
            (fields[8]!=_T("-") && !ValidTime(fields[8])))) continue;
        CString key = fields[0] + _T("|") + fields[1] + _T("|") + fields[2];
        if (!keys.insert(std::wstring(key.GetString())).second) continue;
        ServiceTime row;
        row.stationName = fields[0]; row.lineId = id; row.direction = fields[2];
        row.first = fields[3]; row.last = fields[4];
        row.lastFriSat = fields.size()==9 ? fields[6] : row.last;
        if(fields.size()==9 && fields[7]!=_T("-")){row.extraDirectionSunThu=fields[7];row.extraLastSunThu=fields[8];}
        m_rows.push_back(row);
    }
    return !m_rows.empty();
}
std::vector<ServiceTime> CServiceTimetable::ForStation(const CString& name, int lineId) const // 编写者：肖博腾（4号）
{
    std::vector<ServiceTime> result;
    for (const auto& row : m_rows)
        if (row.stationName == name && (lineId == 0 || row.lineId == lineId)) result.push_back(row);
    return result;
}
const CServiceTimetable& CServiceTimetable::Instance() // 编写者：肖博腾（4号）
{
    static CServiceTimetable table;
    static const bool initialized = [&]() {
        wchar_t path[32768] = {};
        DWORD length = GetModuleFileNameW(nullptr, path, _countof(path));
        if (length == 0 || length >= _countof(path)) return false;
        CString file(path); file = file.Left(file.ReverseFind('\\') + 1) + _T("service_times.txt");
        return table.Load(file);
    }();
    UNREFERENCED_PARAMETER(initialized);
    return table;
}
CString CServiceTimetable::DisplayTime(const CString& value) // 编写者：肖博腾（4号）
{
    if(value.IsEmpty() || value == _T("--:--")) return _T("待核实");
    return value.Left(3)==_T("00:") ? _T("次日 ")+value : value;
}
CString CServiceTimetable::Supplement(const ServiceTime& row) // 编写者：肖博腾（4号）
{
    CString text;
    if(row.lastFriSat!=row.last) text=_T("周五、周六末班：")+DisplayTime(row.lastFriSat);
    if(!row.extraDirectionSunThu.IsEmpty()){
        if(!text.IsEmpty())text+=_T("\r\n");
        text+=_T("周日至周四另有往")+row.extraDirectionSunThu+_T("区间末班：")+DisplayTime(row.extraLastSunThu);
    }
    return text;
}
CString CServiceTimetable::Notice() // 编写者：肖博腾（4号）
{
    return _T("非实时运营表");
}
