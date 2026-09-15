#pragma once
#include <afxwin.h>
#include <string>
#include <vector>
#include <map>

// ==========================================
// 1. 周边地标类型枚举
// ==========================================
enum LandmarkType {
	LANDMARK_HOTEL = 0,    // 宾馆酒店
	LANDMARK_SCENERY,      // 景点/打卡地
	LANDMARK_HOSPITAL,     // 医院
	LANDMARK_SCHOOL,       // 学校
	LANDMARK_MALL          // 商场/商圈
};

// ==========================================
// 2. 地标结构体 (肖博腾 - 4号模块)
// ==========================================
struct Landmark {
	CString name;          // 地标名称
	LandmarkType type;     // 地标类别
	CString description;   // 简介/出口提示(如"1号口出直行200米")
};

// ==========================================
// 3. 地铁站点结构体 (刘子瑜 - 3号模块)
// ==========================================
struct StationNode {
	int id;                          // 站点唯一ID (从 0 或 1 开始自增)
	CString name;                    // 站点名称 (如 "新街口")
	CPoint pos;                      // 在地图上的逻辑像素坐标 (X, Y)
	std::vector<int> lineIds;        // 所属线路ID列表 (若大小 > 1 则为换乘站)
	std::vector<Landmark> landmarks; // 周边地标列表
	bool isTransfer;                 // 是否为换乘站 (方便快速判断)

	StationNode() : id(-1), pos(0, 0), isTransfer(false) /* 编写者：刘子瑜（3号） */ {}
};

// ==========================================
// 4. 地铁线路结构体 (刘子瑜 - 3号模块)
// ==========================================
struct MetroLine {
	int lineId;                      // 线路号 (如 1, 2, 3...)
	CString lineName;                // 线路名 (如 "1号线", "S1号线")
	COLORREF color;                  // 线路在地图上的绘制颜色 (如 RGB(0, 150, 214))
	std::vector<int> stationIds;     // 沿途站点ID的有序列表 (按行车先后顺序排列)
	std::vector<double> distances;   // 相邻站点间的距离(km)列表 (大小通常为 stationIds.size() - 1)

	MetroLine() : lineId(0), color(RGB(0, 0, 0)) /* 编写者：刘子瑜（3号） */ {}
};

// ==========================================
// 5. 图的邻接边结构 (邓博文 - 2号算法模块)
// ==========================================
struct MetroEdge {
	int toStationId;                 // 目标站点ID
	int lineId;                      // 连接两站的所属线路ID
	int durationMin = 0;             // 可选运行耗时
	double distanceKm;               // 两站间物理距离 (公里)
};

// ==========================================
// 6. 寻路策略与换乘指示 (邓博文 - 2号算法模块)
// ==========================================
enum RouteStrategy {
	STRATEGY_SHORTEST_DIST = 0,      // 最短距离优先 (Dijkstra)
	STRATEGY_MIN_STATIONS,           // 最少站点优先 (BFS)
	STRATEGY_MIN_TRANSFERS           // 最少换乘优先
};

// 单段行程乘车/换乘指引
struct PathSegment {
	int fromLineId;                  // 当前乘坐线路ID
	CString lineName;                // 线路名称 (如 "1号线")
	int startStationId;              // 该段上车站ID
	int endStationId;                // 该段下车站ID
	int passStationCount;            // 乘车经过的站数
	CString startStationName;        // 可选: 该段上车站名称 (由算法填充, 用于文案展示)
	CString endStationName;          // 可选: 该段下车站名称
	CString directionStationName;    // 可选: 该段行车方向终点站名 (如 "迈皋桥"), 用于“往XX方向”文案
};

// 完整规划结果结构体
struct PathResult {
	int estimatedTimeMin = 0;                 // 算法未提供时由UI单独估算
	bool isFound;                            // 是否成功找到路径
	std::vector<int> stationSequence;        // 途经全部站点ID的有序序列
	std::vector<int> transferStations;       // 途径换乘站ID列表
	std::vector<PathSegment> transferGuides; // 具体乘车/换乘指引步骤
	int totalStations;                       // 途经总站数
	double totalDistanceKm;                  // 总里程数 (km)
	int ticketPrice;                         // 计算出的票价 (元)
	CString startStationName;                // 可选: 起点站名称
	CString endStationName;                  // 可选: 终点站名称
	RouteStrategy strategy;                  // 本次查询使用的策略 (供历史记录使用)

	PathResult() : isFound(false), totalStations(0), totalDistanceKm(0.0), ticketPrice(0),
		strategy(STRATEGY_SHORTEST_DIST) /* 编写者：邓博文（2号） */ {}
};

// ==========================================
// 7. 用户历史记录与收藏路线 (肖博腾 - 4号模块)
// ==========================================
struct UserRouteRecord {
	CString recordId;                // 记录唯一标识/时间戳
	int startStationId;              // 起点站ID
	CString startStationName;        // 起点站名
	int endStationId;                // 终点站ID
	CString endStationName;          // 终点站名
	RouteStrategy strategy;          // 使用的规划策略
	CString queryTime;               // 查询/收藏时间 (如 "2026-08-31 10:00")
	bool isFavorite;                 // 是否为用户收藏
};
