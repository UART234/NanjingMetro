#pragma once
#include "MetroDef.h"
#include <vector>

class CHistoryManager
{
public:
	CHistoryManager();
	~CHistoryManager();

	// 核心业务接口
	void AddRecord(int startId, const CString& startName, int endId, const CString& endName, RouteStrategy strategy, bool isFavorite = false);
	bool IsFavorite(int startId, int endId, RouteStrategy strategy) const;
	void ToggleFavorite(int recordIndex);                     // 切换某条记录的收藏状态
	void ClearHistory();                                      // 清空非收藏的历史记录
	void ClearAllFavorites();                                 // 清空所有收藏

	// 数据获取接口
	const std::vector<UserRouteRecord>& GetAllRecords() const /* 编写者：肖博腾（4号） */ { return m_records; }
	std::vector<UserRouteRecord> GetFavorites() const;        // 仅获取收藏项
	std::vector<UserRouteRecord> GetHistoryOnly() const;      // 仅获取历史查询项

	// 文件持久化接口（自动读写）
	bool LoadFromFile(const CString& filePath = _T("user_data.txt"));
	bool SaveToFile(const CString& filePath = _T("user_data.txt"));

private:
	void NormalizeFavorites();
    void TrimHistory();
	CString m_storagePath; // 沿用 Doc 已解析的绝对路径，避免启动目录影响保存位置。
	std::vector<UserRouteRecord> m_records; // 内存中保存的记录列表
	CString GetCurrentTimeString();         // 获取当前格式化时间
};
