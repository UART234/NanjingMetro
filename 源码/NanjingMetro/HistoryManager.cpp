#include "pch.h"
#include "HistoryManager.h"
#include <fstream>
#include <sstream>
#include <set>
#include <tuple>

CHistoryManager::CHistoryManager() // 编写者：肖博腾（4号）
{
}

CHistoryManager::~CHistoryManager() // 编写者：肖博腾（4号）
{
}

CString CHistoryManager::GetCurrentTimeString() // 编写者：肖博腾（4号）
{
	CTime now = CTime::GetCurrentTime();
	return now.Format(_T("%Y-%m-%d %H:%M"));
}

void CHistoryManager::AddRecord(int startId, const CString& startName, int endId, const CString& endName, RouteStrategy strategy, bool isFavorite) // 编写者：肖博腾（4号）
{
	// 收藏按有向起终点与规划策略唯一；普通查询仍保留历史。
    if (startId <= 0 || endId <= 0 || startId == endId) return;
    if (isFavorite) {
        if (IsFavorite(startId, endId, strategy)) return;
        for (auto& existing : m_records) {
            if (existing.startStationId == startId && existing.endStationId == endId && existing.strategy == strategy) {
                existing.isFavorite = true;
                return;
            }
        }
    }

	UserRouteRecord rec;
	rec.recordId.Format(_T("%lld"), CTime::GetCurrentTime().GetTime());
	rec.startStationId = startId;
	rec.startStationName = startName;
	rec.endStationId = endId;
	rec.endStationName = endName;
	rec.strategy = strategy;
	rec.queryTime = GetCurrentTimeString();
	rec.isFavorite = isFavorite;

	// 最新记录插入到最前面
	m_records.insert(m_records.begin(), rec);

	TrimHistory(); // 收藏不因普通历史超过50条而丢失
}

bool CHistoryManager::IsFavorite(int startId, int endId, RouteStrategy strategy) const // 编写者：肖博腾（4号）
{
    for (const auto& rec : m_records)
        if (rec.isFavorite && rec.startStationId == startId && rec.endStationId == endId && rec.strategy == strategy) return true;
    return false;
}

void CHistoryManager::NormalizeFavorites() // 编写者：肖博腾（4号）
{
    std::set<std::tuple<int,int,int>> seen;
    for (auto& rec : m_records)
        if (rec.isFavorite && !seen.emplace(rec.startStationId,rec.endStationId,(int)rec.strategy).second)
            rec.isFavorite = false; // 旧重复收藏转回普通历史，保留查询信息。
}

void CHistoryManager::TrimHistory() // 编写者：肖博腾（4号）
{
    size_t historyCount = 0;
    for (auto it = m_records.begin(); it != m_records.end(); ) {
        if (!it->isFavorite && ++historyCount > 50) it = m_records.erase(it);
        else ++it;
    }
}

void CHistoryManager::ToggleFavorite(int recordIndex) // 编写者：肖博腾（4号）
{
    if (recordIndex < 0 || recordIndex >= (int)m_records.size()) return;
    const auto selected = m_records[recordIndex];
    const bool enabled = !IsFavorite(selected.startStationId,selected.endStationId,selected.strategy);
    for (auto& rec : m_records)
        if (rec.startStationId == selected.startStationId && rec.endStationId == selected.endStationId && rec.strategy == selected.strategy)
            rec.isFavorite = false;
    m_records[recordIndex].isFavorite = enabled;
}

void CHistoryManager::ClearHistory() // 编写者：肖博腾（4号）
{
	// 只保留被收藏的项，清除普通历史记录
	std::vector<UserRouteRecord> kept;
	for (const auto& rec : m_records) {
		if (rec.isFavorite) {
			kept.push_back(rec);
		}
	}
	m_records = kept;
}

void CHistoryManager::ClearAllFavorites() // 编写者：肖博腾（4号）
{
	for (auto& rec : m_records) {
		rec.isFavorite = false;
	}
}

std::vector<UserRouteRecord> CHistoryManager::GetFavorites() const // 编写者：肖博腾（4号）
{
	std::vector<UserRouteRecord> favs;
	for (const auto& rec : m_records) {
		if (rec.isFavorite) {
			favs.push_back(rec);
		}
	}
	return favs;
}

std::vector<UserRouteRecord> CHistoryManager::GetHistoryOnly() const // 编写者：肖博腾（4号）
{
	return m_records;
}


// 明确以UTF-8读写，避免默认C locale下中文在CStdioFile中被截断。
bool CHistoryManager::SaveToFile(const CString& filePath) // 编写者：肖博腾（4号）
{
    CString target = filePath;
    if (filePath.Find(_T('\\')) < 0 && filePath.Find(_T('/')) < 0 && !m_storagePath.IsEmpty())
        target = m_storagePath;
    std::string bytes("\xEF\xBB\xBF");
    for (const auto& rec : m_records) {
        CString line;
        line.Format(_T("%d\t%s\t%d\t%s\t%d\t%s\t%d\n"),rec.startStationId,rec.startStationName.GetString(),
            rec.endStationId,rec.endStationName.GetString(),(int)rec.strategy,rec.queryTime.GetString(),rec.isFavorite?1:0);
        int count=WideCharToMultiByte(CP_UTF8,0,line.GetString(),line.GetLength(),nullptr,0,nullptr,nullptr);
        if(count<=0)return false;
        std::string encoded(count,'\0');
        WideCharToMultiByte(CP_UTF8,0,line.GetString(),line.GetLength(),&encoded[0],count,nullptr,nullptr);
        bytes+=encoded;
    }
    // 同目录临时文件提交，写入失败时保留原记录。
    CString temporary=target+_T(".tmp");
    std::ofstream output(temporary.GetString(),std::ios::binary|std::ios::trunc);
    if(!output)return false;
    output.write(bytes.data(),(std::streamsize)bytes.size());output.close();
    if(!output)return false;
    return MoveFileEx(temporary,target,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;
}

bool CHistoryManager::LoadFromFile(const CString& filePath) // 编写者：肖博腾（4号）
{
    m_storagePath=filePath;m_records.clear();
    std::ifstream input(filePath.GetString(),std::ios::binary);
    if(!input)return false;
    std::string bytes;
    while(std::getline(input,bytes)){
        if(bytes.compare(0,3,"\xEF\xBB\xBF")==0)bytes.erase(0,3);
        if(!bytes.empty()&&bytes.back()=='\r')bytes.pop_back();
        if(bytes.empty())continue;
        UINT page=CP_UTF8;
        int count=MultiByteToWideChar(page,MB_ERR_INVALID_CHARS,bytes.data(),(int)bytes.size(),nullptr,0);
        if(!count){page=CP_ACP;count=MultiByteToWideChar(page,0,bytes.data(),(int)bytes.size(),nullptr,0);}
        if(!count)continue;
        CString line;MultiByteToWideChar(page,0,bytes.data(),(int)bytes.size(),line.GetBuffer(count),count);line.ReleaseBuffer(count);
        CString fields[7];bool valid=true;
        for(int i=0;i<7;++i)if(!AfxExtractSubString(fields[i],line,i,'\t'))valid=false;
        if(!valid||fields[1].IsEmpty()||fields[3].IsEmpty())continue;
        UserRouteRecord rec;rec.startStationId=_ttoi(fields[0]);rec.startStationName=fields[1];
        rec.endStationId=_ttoi(fields[2]);rec.endStationName=fields[3];
        int strategy=_ttoi(fields[4]);if(rec.startStationId<=0||rec.endStationId<=0||strategy<0||strategy>2)continue;
        rec.strategy=(RouteStrategy)strategy;rec.queryTime=fields[5];rec.isFavorite=fields[6]==_T("1");
        rec.recordId.Format(_T("loaded-%d"),(int)m_records.size());m_records.push_back(rec);
    }
    NormalizeFavorites();
    TrimHistory();
    return !input.bad();
}
