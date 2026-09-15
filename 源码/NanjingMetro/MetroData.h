#pragma once
#include "MetroDef.h"
#include <map>
#include <vector>

class CMetroData
{
public:
	CMetroData();
	~CMetroData();

	// 加载文件核心接口
	bool LoadDataFromFile(const CString& filePath);
	bool ValidateData(CString* error = nullptr) const;
	// 新格式数据无坐标时，按线路拓扑自动生成布局
	void LayoutNetwork();

	// 基础查询辅助函数
	StationNode* GetStationById(int stationId);
	StationNode* GetStationByName(const CString& name);
	MetroLine* GetLineById(int lineId);

public:
	std::map<int, StationNode> m_stations; // 站点查找表: ID -> StationNode
	std::vector<MetroLine> m_lines;         // 线路列表
};
