#pragma once

#include "MetroDef.h"
#include <map>
#include <vector>
using namespace std;

class CMetroData;

// ==========================================
// 1. 地铁全量数据（文档中 m_metroData 的类型）
// ==========================================
struct MetroData
{
	vector<StationNode> stations;	// 全部站点
	vector<MetroLine> lines;		// 全部线路（含 stationIds 与 distances）
};

// ==========================================
// 2. 图模块：建图 + 寻路入口
// ==========================================
class MetroGraph
{
public:
	// 由线路数据构建邻接拓扑（双向边）
	void BuildGraph(const MetroData& data);
	// 由文档持有的 CMetroData 构建邻接拓扑
	void BuildGraph(const CMetroData& data);

	// 寻路总入口：按策略返回 PathResult，isFound 表示是否找到
	PathResult FindRoute(int startStationId, int endStationId, RouteStrategy strategy) const;

	// 供算法模块（RouteStrategy.cpp）读取图数据
	const map<int, vector<MetroEdge>>& GetAdjacency() const /* 编写者：邓博文（2号） */ { return m_adj; }
	const MetroData& GetData() const /* 编写者：邓博文（2号） */ { return m_data; }

	// 按 ID 查找站点 / 线路（找不到返回 nullptr）
	const StationNode* FindStation(int stationId) const;
	const MetroLine* FindLine(int lineId) const;

private:
	MetroData m_data;							// 站点 + 线路数据
	map<int, vector<MetroEdge>> m_adj;	// 站ID -> 邻接边列表
};

// 全局图实例（初始化时调用 g_graph.BuildGraph(pDoc->m_metroData)）
extern MetroGraph g_graph;
