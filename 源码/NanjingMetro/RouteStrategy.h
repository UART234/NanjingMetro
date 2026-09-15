//由邓博文编写
#pragma once
#include "MetroGraph.h"
#include <vector>
using namespace std;

// ==========================================
// 路径上一段的乘车信息（算法内部使用，供结果统计）
// ==========================================
struct RouteSegmentInfo
{
	int fromStationId = -1;   // 本段起点站ID
	int toStationId = -1;     // 本段终点站ID
	int lineId = -1;          // 本段所在线路ID
	double distanceKm = 0.0;  // 本段距离（km）
};

// 三种策略算法：返回 PathResult，并把路径分段信息写入 segments
PathResult FindFewestStations(const MetroGraph& graph, int startStationId, int endStationId,
	vector<RouteSegmentInfo>& segments);

PathResult FindShortestDistance(const MetroGraph& graph, int startStationId, int endStationId,
	vector<RouteSegmentInfo>& segments);

PathResult FindFewestTransfers(const MetroGraph& graph, int startStationId, int endStationId,
	vector<RouteSegmentInfo>& segments);

// 结果统计 / 换乘站 / 乘车指引
void CalculateRouteInfo(const MetroGraph& graph, PathResult& result,
	const vector<RouteSegmentInfo>& segments);

void FindTransferStations(const MetroGraph& graph, PathResult& result,
	const vector<RouteSegmentInfo>& segments);

void BuildTransferGuides(const MetroGraph& graph, PathResult& result,
	const vector<RouteSegmentInfo>& segments);

// 线网最短距离（票价计价依据）与票价计算
double ComputeShortestDistanceKm(const MetroGraph& graph, int startStationId, int endStationId);
int CalcTicketPrice(double distanceKm);