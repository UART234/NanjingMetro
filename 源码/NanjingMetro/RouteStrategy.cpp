//由邓博文编写
// RouteStrategy.cpp: 寻路算法实现（BFS / Dijkstra / 最少换乘）

#include "pch.h"
#include "RouteStrategy.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <queue>
#include <set>

using namespace std;

// ----------------------------------------------------------
// ----------------------------------------------------------
// 1. 最少站数：站数优先；站数相同时取换乘最少
//    实现：对状态(站点, 当前线路)做字典序代价搜索
//    代价 = (乘车站段数, 换乘次数)
// ----------------------------------------------------------
PathResult FindFewestStations(const MetroGraph& graph, int startStationId, int endStationId,
	vector<RouteSegmentInfo>& segments) // 编写者：邓博文（2号）
{
	PathResult result;
	const auto& adj = graph.GetAdjacency();
	if (adj.find(startStationId) == adj.end() || adj.find(endStationId) == adj.end())
		return result;

	// 统计每个站点途经的线路集合
	map<int, set<int>> linesAtStation;
	for (auto it = adj.begin(); it != adj.end(); ++it)
	{
		int station = it->first;
		const vector<MetroEdge>& edges = it->second;
		for (size_t i = 0; i < edges.size(); ++i)
			linesAtStation[station].insert(edges[i].lineId);
	}

	// 状态：(站点, 当前线路)。代价 (站段数, 换乘次数)，字典序比较。
	typedef pair<int, int> State;      // (stationId, lineId)
	typedef pair<int, int> Cost;       // (segments, transfers)
	map<State, Cost> cost;
	map<State, State> prev;
	map<State, RouteSegmentInfo> rideSeg;
	typedef pair<Cost, State> PQItem;
	priority_queue<PQItem, vector<PQItem>, greater<PQItem>> pq;

	// 起点站可乘任意经过它的线路，初始代价 (0, 0)
	const set<int>& startLines = linesAtStation[startStationId];
	for (set<int>::const_iterator it = startLines.begin(); it != startLines.end(); ++it)
	{
		State st(startStationId, *it);
		cost[st] = Cost(0, 0);
		pq.push(make_pair(Cost(0, 0), st));
	}

	State bestEnd(-1, -1);
	bool reached = false;
	while (!pq.empty())
	{
		Cost curCost = pq.top().first;
		State cur = pq.top().second;
		pq.pop();

		map<State, Cost>::iterator cIt = cost.find(cur);
		if (cIt == cost.end() || curCost != cIt->second)
			continue;

		// 第一次弹出终点站状态时，(站数, 换乘) 已字典序最小
		if (cur.first == endStationId)
		{
			bestEnd = cur;
			reached = true;
			break;
		}

		// (a) 同线乘车：站段数 +1
		map<int, vector<MetroEdge>>::const_iterator adjIt = adj.find(cur.first);
		if (adjIt != adj.end())
		{
			const vector<MetroEdge>& edges = adjIt->second;
			for (size_t i = 0; i < edges.size(); ++i)
			{
				const MetroEdge& edge = edges[i];
				if (edge.lineId != cur.second)
					continue;
				State next(edge.toStationId, cur.second);
				Cost nd(curCost.first + 1, curCost.second);
				map<State, Cost>::iterator nIt = cost.find(next);
				if (nIt == cost.end() || nd < nIt->second)
				{
					cost[next] = nd;
					prev[next] = cur;
					RouteSegmentInfo seg;
					seg.fromStationId = cur.first;
					seg.toStationId = edge.toStationId;
					seg.lineId = cur.second;
					seg.distanceKm = edge.distanceKm;
					rideSeg[next] = seg;
					pq.push(make_pair(nd, next));
				}
			}
		}

		// (b) 站内换线：换乘次数 +1
		const set<int>& linesHere = linesAtStation[cur.first];
		for (set<int>::const_iterator it = linesHere.begin(); it != linesHere.end(); ++it)
		{
			if (*it == cur.second)
				continue;
			State next(cur.first, *it);
			Cost nd(curCost.first, curCost.second + 1);
			map<State, Cost>::iterator nIt = cost.find(next);
			if (nIt == cost.end() || nd < nIt->second)
			{
				cost[next] = nd;
				prev[next] = cur;
				rideSeg.erase(next);
				pq.push(make_pair(nd, next));
			}
		}
	}
	if (!reached)
		return result;   // 不可达

	// 回溯状态链（终点 -> 起点）
	vector<State> chain;
	for (State s = bestEnd;; s = prev[s])
	{
		chain.push_back(s);
		if (prev.find(s) == prev.end())
			break;
	}

	// 生成站点序列与分段信息（chain 是终点在前，从后往前走）
	vector<int> path;
	path.push_back(startStationId);
	segments.clear();
	for (int i = (int)chain.size() - 2; i >= 0; --i)
	{
		const State& before = chain[i + 1];   // 更靠近起点
		const State& after = chain[i];        // 更靠近终点
		if (before.second == after.second)
		{
			// 同线乘车：before.station -> after.station
			RouteSegmentInfo seg = rideSeg[after];
			path.push_back(after.first);
			segments.push_back(seg);
		}
		// 换线：站点不变，只改变 lineId，不产生新站点
	}

	result.isFound = true;
	result.stationSequence = path;
	return result;
}

// ----------------------------------------------------------
// 2. 最短距离：Dijkstra
// ----------------------------------------------------------
PathResult FindShortestDistance(const MetroGraph& graph, int startStationId, int endStationId,
	vector<RouteSegmentInfo>& segments) // 编写者：邓博文（2号）
{
	PathResult result;
	const auto& adj = graph.GetAdjacency();
	if (adj.find(startStationId) == adj.end() || adj.find(endStationId) == adj.end())
		return result;

	map<int, double> dist;
	map<int, int> prev;
	map<int, int> prevLine;
	map<int, double> prevDist;
	set<int> done;

	typedef pair<double, int> PQItem;
	priority_queue<PQItem, vector<PQItem>, greater<PQItem>> pq;

	dist[startStationId] = 0.0;
	pq.push(make_pair(0.0, startStationId));

	while (!pq.empty())
	{
		int cur = pq.top().second;
		double curDist = pq.top().first;
		pq.pop();
		if (done.count(cur) != 0)
			continue;
		done.insert(cur);
		if (cur == endStationId)
			break;

		auto it = adj.find(cur);
		if (it == adj.end())
			continue;
		for (size_t i = 0; i < it->second.size(); ++i)
		{
			const MetroEdge& edge = it->second[i];
			double nd = curDist + edge.distanceKm;
			if (dist.count(edge.toStationId) == 0 || nd < dist[edge.toStationId])
			{
				dist[edge.toStationId] = nd;
				prev[edge.toStationId] = cur;
				prevLine[edge.toStationId] = edge.lineId;
				prevDist[edge.toStationId] = edge.distanceKm;
				pq.push(make_pair(nd, edge.toStationId));
			}
		}
	}

	if (done.count(endStationId) == 0)
		return result;   // 不可达

	// 回溯得到站点序列
	vector<int> path;
	for (int cur = endStationId; cur != startStationId; cur = prev[cur])
		path.push_back(cur);
	path.push_back(startStationId);
	reverse(path.begin(), path.end());

	result.isFound = true;
	result.stationSequence = path;

	segments.clear();
	for (size_t i = 1; i < path.size(); ++i)
	{
		RouteSegmentInfo seg;
		seg.fromStationId = path[i - 1];
		seg.toStationId = path[i];
		seg.lineId = prevLine[path[i]];
		seg.distanceKm = prevDist[path[i]];
		segments.push_back(seg);
	}
	return result;
}

// ----------------------------------------------------------
// 3. 最少换乘：对 (站点, 当前线路) 状态做 Dijkstra
//    同线乘车换乘次数 +0，站内换线 +1
// ----------------------------------------------------------
struct TransferState
{
	int stationId;
	int lineId;

	TransferState() : stationId(-1), lineId(-1) /* 编写者：邓博文（2号） */ {}
	TransferState(int s, int l) : stationId(s), lineId(l) /* 编写者：邓博文（2号） */ {}

	bool operator<(const TransferState& other) const // 编写者：邓博文（2号）
	{
		if (stationId != other.stationId)
			return stationId < other.stationId;
		return lineId < other.lineId;
	}
};

PathResult FindFewestTransfers(const MetroGraph& graph, int startStationId, int endStationId,
	vector<RouteSegmentInfo>& segments) // 编写者：邓博文（2号）
{
	PathResult result;
	const auto& adj = graph.GetAdjacency();
	if (adj.find(startStationId) == adj.end() || adj.find(endStationId) == adj.end())
		return result;

	// 统计每个站点途经的线路集合
	map<int, set<int>> linesAtStation;
	for (auto it = adj.begin(); it != adj.end(); ++it)
	{
		int station = it->first;
		const vector<MetroEdge>& edges = it->second;
		for (size_t i = 0; i < edges.size(); ++i)
			linesAtStation[station].insert(edges[i].lineId);
	}

	map<TransferState, int> dist;              // 状态 -> 最少换乘次数
	map<TransferState, TransferState> prev;    // 状态 -> 前一个状态
	map<TransferState, RouteSegmentInfo> rideSeg; // 乘车进入该状态的那一段（换乘进入则为空）

	typedef pair<int, TransferState> PQItem;   // (换乘次数, 状态)
	priority_queue<PQItem, vector<PQItem>, greater<PQItem>> pq;

	// 起点站可乘坐任意经过它的线路，换乘次数为 0
	set<int> startLines = linesAtStation[startStationId];
	for (set<int>::iterator it = startLines.begin(); it != startLines.end(); ++it)
	{
		TransferState st(startStationId, *it);
		dist[st] = 0;
		pq.push(make_pair(0, st));
	}

	while (!pq.empty())
	{
		TransferState cur = pq.top().second;
		int curCost = pq.top().first;
		pq.pop();

		map<TransferState, int>::iterator dIt = dist.find(cur);
		if (dIt == dist.end() || curCost > dIt->second)
			continue;

		// 第一次弹出终点站状态时，换乘次数已最少
		if (cur.stationId == endStationId)
			break;

		// (a) 同线乘车：沿当前线路前进，换乘次数不变
		map<int, vector<MetroEdge>>::const_iterator adjIt = adj.find(cur.stationId);
		if (adjIt != adj.end())
		{
			const vector<MetroEdge>& edges = adjIt->second;
			for (size_t i = 0; i < edges.size(); ++i)
			{
				const MetroEdge& edge = edges[i];
				if (edge.lineId != cur.lineId)
					continue;
				TransferState next(edge.toStationId, cur.lineId);
				if (dist.count(next) == 0 || curCost < dist[next])
				{
					dist[next] = curCost;
					prev[next] = cur;
					RouteSegmentInfo seg;
					seg.fromStationId = cur.stationId;
					seg.toStationId = edge.toStationId;
					seg.lineId = cur.lineId;
					seg.distanceKm = edge.distanceKm;
					rideSeg[next] = seg;
					pq.push(make_pair(curCost, next));
				}
			}
		}

		// (b) 站内换线：换乘次数 +1
		const set<int>& linesHere = linesAtStation[cur.stationId];
		for (set<int>::iterator it = linesHere.begin(); it != linesHere.end(); ++it)
		{
			if (*it == cur.lineId)
				continue;
			TransferState next(cur.stationId, *it);
			int nd = curCost + 1;
			if (dist.count(next) == 0 || nd < dist[next])
			{
				dist[next] = nd;
				prev[next] = cur;
				rideSeg.erase(next);   // 换乘进入的状态没有乘车分段
				pq.push(make_pair(nd, next));
			}
		}
	}

	// 取到达终点站、换乘次数最少的状态
	TransferState bestEnd;
	int bestCost = (numeric_limits<int>::max)();
	const set<int>& endLines = linesAtStation[endStationId];
	for (set<int>::iterator it = endLines.begin(); it != endLines.end(); ++it)
	{
		TransferState st(endStationId, *it);
		map<TransferState, int>::iterator dIt = dist.find(st);
		if (dIt != dist.end() && dIt->second < bestCost)
		{
			bestCost = dIt->second;
			bestEnd = st;
		}
	}
	if (bestCost == (numeric_limits<int>::max)())
		return result;   // 不可达

	// 回溯状态链（终点 -> 起点）
	vector<TransferState> chain;
	for (TransferState s = bestEnd;; s = prev[s])
	{
		chain.push_back(s);
		if (prev.find(s) == prev.end())
			break;
	}

	// 生成站点序列与分段信息（chain 是终点在前，从后往前走）
	vector<int> path;
	path.push_back(startStationId);
	segments.clear();
	for (int i = (int)chain.size() - 2; i >= 0; --i)
	{
		const TransferState& before = chain[i + 1];   // 更靠近起点
		const TransferState& after = chain[i];        // 更靠近终点
		if (before.lineId == after.lineId)
		{
			// 同线乘车：before.station -> after.station
			RouteSegmentInfo seg = rideSeg[after];
			path.push_back(after.stationId);
			segments.push_back(seg);
		}
		// 换线：站点不变，只改变 lineId，不产生新站点
	}

	result.isFound = true;
	result.stationSequence = path;
	return result;
}

// ----------------------------------------------------------
// 4. 结果统计：站数 / 里程 / 票价
// ----------------------------------------------------------
void CalculateRouteInfo(const MetroGraph& graph, PathResult& result,
	const vector<RouteSegmentInfo>& segments) // 编写者：邓博文（2号）
{
	// 站数：不含起点、含终点（即乘车段数）
	result.totalStations = result.stationSequence.empty()
		? 0 : static_cast<int>(result.stationSequence.size()) - 1;

	// 总里程：本次所选路径的里程
	result.totalDistanceKm = 0.0;
	for (size_t i = 0; i < segments.size(); ++i)
		result.totalDistanceKm += segments[i].distanceKm;

	// 票价：按“线网最短距离”计价（与选择哪种模式无关）
	result.ticketPrice = 0;
	if (!result.stationSequence.empty())
	{
		int startId = result.stationSequence.front();
		int endId = result.stationSequence.back();
		double shortestKm = ComputeShortestDistanceKm(graph, startId, endId);
		if (shortestKm >= 0.0)
			result.ticketPrice = CalcTicketPrice(shortestKm);
	}

	// 不统计预估耗时
	result.estimatedTimeMin = 0;
}

// ----------------------------------------------------------
// 5. 换乘站：线路变化处的前一段终点站
// ----------------------------------------------------------
void FindTransferStations(const MetroGraph& graph, PathResult& result,
	const vector<RouteSegmentInfo>& segments) // 编写者：邓博文（2号）
{
	result.transferStations.clear();
	for (size_t i = 1; i < segments.size(); ++i)
	{
		if (segments[i].lineId != segments[i - 1].lineId)
		{
			int transferStation = segments[i - 1].toStationId;
			if (result.transferStations.empty()
				|| result.transferStations.back() != transferStation)
				result.transferStations.push_back(transferStation);
		}
	}
}

// ----------------------------------------------------------
// 6. 乘车指引：按线路把连续分段合并为 PathSegment
// ----------------------------------------------------------
void BuildTransferGuides(const MetroGraph& graph, PathResult& result,
	const vector<RouteSegmentInfo>& segments) // 编写者：邓博文（2号）
{
	result.transferGuides.clear();
	if (segments.empty())
		return;

	size_t i = 0;
	while (i < segments.size())
	{
		int lineId = segments[i].lineId;
		size_t j = i;
		while (j < segments.size() && segments[j].lineId == lineId)
			++j;

		PathSegment seg;
		seg.fromLineId = lineId;
		const MetroLine* line = graph.FindLine(lineId);
		seg.lineName = line ? line->lineName : CString(_T(""));
		seg.startStationId = segments[i].fromStationId;
		seg.endStationId = segments[j - 1].toStationId;
		seg.passStationCount = static_cast<int>(j - i);
		// 4号UI适配：保留新算法分段，补齐组长结果展示所需的名称与方向。
        const StationNode* start = graph.FindStation(seg.startStationId);
        const StationNode* end = graph.FindStation(seg.endStationId);
        if (start) seg.startStationName = start->name;
        if (end) seg.endStationName = end->name;
        if (line && !line->stationIds.empty()) {
            auto a = std::find(line->stationIds.begin(), line->stationIds.end(), seg.startStationId);
            auto b = std::find(line->stationIds.begin(), line->stationIds.end(), seg.endStationId);
            if (a != line->stationIds.end() && b != line->stationIds.end()) {
                const StationNode* terminal = graph.FindStation(a < b ? line->stationIds.back() : line->stationIds.front());
                if (terminal) seg.directionStationName = terminal->name;
            }
        }
        result.transferGuides.push_back(seg);

		i = j;
	}
}

// ----------------------------------------------------------
// 7. 线网最短距离（票价计价依据）
// ----------------------------------------------------------
double ComputeShortestDistanceKm(const MetroGraph& graph, int startStationId, int endStationId)
{
	const auto& adj = graph.GetAdjacency();
	if (adj.find(startStationId) == adj.end() || adj.find(endStationId) == adj.end())
		return -1.0;

	map<int, double> dist;
	set<int> done;

	typedef pair<double, int> PQItem;
	priority_queue<PQItem, vector<PQItem>, greater<PQItem>> pq;

	dist[startStationId] = 0.0;
	pq.push(make_pair(0.0, startStationId));

	while (!pq.empty())
	{
		int cur = pq.top().second;
		double curDist = pq.top().first;
		pq.pop();
		if (done.count(cur) != 0)
			continue;
		done.insert(cur);
		if (cur == endStationId)
			return curDist;

		map<int, vector<MetroEdge>>::const_iterator it = adj.find(cur);
		if (it == adj.end())
			continue;
		for (size_t i = 0; i < it->second.size(); ++i)
		{
			const MetroEdge& edge = it->second[i];
			double nd = curDist + edge.distanceKm;
			if (dist.count(edge.toStationId) == 0 || nd < dist[edge.toStationId])
			{
				dist[edge.toStationId] = nd;
				pq.push(make_pair(nd, edge.toStationId));
			}
		}
	}
	return -1.0;
}

// ----------------------------------------------------------
// 8. 票价：南京地铁分段计价（2019-03-31 起实施）
//    起步价 2 元可乘 4 公里；之后每增加 1 元，晋级里程依次为
//    5、5、7、7、9、11、13、15 公里，61 公里以上每 1 元可乘 15 公里。
// ----------------------------------------------------------
int CalcTicketPrice(double distanceKm) // 编写者：邓博文（2号）
{
	if (distanceKm <= 0.0)
		return 0;   // 同站

	// 线网里程是各区间小数的累加值，不同路径的累加顺序会产生 1e-12 级浮点误差，
	// 使本应正好等于分档边界的距离变成 13.999999999999998 或 14.000000000000002，
	// 导致同一段行程因累加顺序不同而算出不同票价。
	// 这里把落在边界容差（1e-6 公里，即 1 毫米）内的距离吸附到边界值，保证计价稳定。
	static const double kBounds[] = { 4.0, 9.0, 14.0, 21.0, 28.0, 37.0, 48.0, 61.0, 76.0, 91.0 };
	for (int i = 0; i < 10; ++i)
	{
		if (fabs(distanceKm - kBounds[i]) < 1e-6)
		{
			distanceKm = kBounds[i];
			break;
		}
	}

	if (distanceKm <= 4.0)
		return 2;

	double remaining = distanceKm - 4.0;
	int fare = 2;
	static const double kSteps[] = { 5.0, 5.0, 7.0, 7.0, 9.0, 11.0, 13.0, 15.0 };
	for (int i = 0; i < 8; ++i)
	{
		if (remaining <= kSteps[i])
			return fare + 1;
		remaining -= kSteps[i];
		++fare;
	}
	return fare + static_cast<int>(ceil(remaining / 15.0));
}