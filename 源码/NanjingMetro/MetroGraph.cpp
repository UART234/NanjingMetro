// MetroGraph.cpp: 地铁图构建与寻路入口
//

#include "pch.h"
#include "MetroGraph.h"
#include "RouteStrategy.h"
#include "MetroData.h"
#include <cmath>
using namespace std;

// 全局图实例
MetroGraph g_graph;

// 4号整合适配：CMetroData 的公里距离原样传入，不重复除以1000。
void MetroGraph::BuildGraph(const CMetroData& data) // 编写者：邓博文（2号）
{
    MetroData snapshot;
    for (const auto& pair : data.m_stations) snapshot.stations.push_back(pair.second);
    snapshot.lines = data.m_lines;
    BuildGraph(snapshot);
}

void MetroGraph::BuildGraph(const MetroData& data) // 编写者：邓博文（2号）
{
	m_data = data;
	m_adj.clear();

	// 遍历每条线路，把相邻两站连成双向边
	for (size_t i = 0; i < m_data.lines.size(); ++i)
	{
		const MetroLine& line = m_data.lines[i];
		for (size_t j = 0; j + 1 < line.stationIds.size(); ++j)
		{
			int a = line.stationIds[j];
			int b = line.stationIds[j + 1];

			// 统一约定：distances 单位为公里。
			double distKm = 0.0;
			if (j < line.distances.size())
				distKm = line.distances[j];

			if (!FindStation(a) || !FindStation(b) || !std::isfinite(distKm) || distKm <= 0.0) continue;
			MetroEdge edgeAB;
			edgeAB.toStationId = b;
			edgeAB.lineId = line.lineId;
			edgeAB.distanceKm = distKm;
			edgeAB.durationMin = 0;		// 本项目不统计耗时

			MetroEdge edgeBA = edgeAB;
			edgeBA.toStationId = a;

			m_adj[a].push_back(edgeAB);
			m_adj[b].push_back(edgeBA);
		}
	}
}

PathResult MetroGraph::FindRoute(int startStationId, int endStationId, RouteStrategy strategy) const // 编写者：邓博文（2号）
{
	PathResult result;
	if (m_adj.empty() || !FindStation(startStationId) || !FindStation(endStationId) ||
        strategy < STRATEGY_SHORTEST_DIST || strategy > STRATEGY_MIN_TRANSFERS)
		return result;

	// 同站：无需乘车
	if (startStationId == endStationId)
	{
		result.isFound = true;
		result.stationSequence.push_back(startStationId);
		result.totalStations = 0;
		result.totalDistanceKm = 0.0;
		result.ticketPrice = 0;
		return result;
	}

	// 按策略调用对应算法，并收集路径分段信息
	vector<RouteSegmentInfo> segments;
	switch (strategy)
	{
	case STRATEGY_MIN_STATIONS:
		result = FindFewestStations(*this, startStationId, endStationId, segments);
		break;
	case STRATEGY_SHORTEST_DIST:
		result = FindShortestDistance(*this, startStationId, endStationId, segments);
		break;
	case STRATEGY_MIN_TRANSFERS:
		result = FindFewestTransfers(*this, startStationId, endStationId, segments);
		break;
	default:
		return result;
	}

	if (result.isFound)
	{
		CalculateRouteInfo(*this, result, segments);
		FindTransferStations(*this, result, segments);
		BuildTransferGuides(*this, result, segments);
	}
	return result;
}

const StationNode* MetroGraph::FindStation(int stationId) const // 编写者：邓博文（2号）
{
	for (size_t i = 0; i < m_data.stations.size(); ++i)
	{
		if (m_data.stations[i].id == stationId)
			return &m_data.stations[i];
	}
	return nullptr;
}

const MetroLine* MetroGraph::FindLine(int lineId) const // 编写者：邓博文（2号）
{
	for (size_t i = 0; i < m_data.lines.size(); ++i)
	{
		if (m_data.lines[i].lineId == lineId)
			return &m_data.lines[i];
	}
	return nullptr;
}