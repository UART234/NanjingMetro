#pragma once
// 由文档/生成官方示意布局.cjs生成。数据坐标源自用户提供的官方图。
inline std::vector<CPoint> MapSegmentPoints(int from,int to,CPoint start,CPoint end) /* 编写者：刘子瑜（3号） */ {
    struct Bend {int a,b;std::vector<CPoint> points;};
    static const std::vector<Bend> bends={
        {8,9,{CPoint(570,586),CPoint(551,605)}}, // 南京站 - 新模范马路
        {27,28,{CPoint(721,1118)}}, // 竹山路 - 天印大道
        {29,30,{CPoint(857,1118)}}, // 龙眠大道 - 南医大·江苏经贸学院
        {38,39,{CPoint(442,438)}}, // 柳洲东路 - 上元门
        {8,42,{CPoint(605,604)}}, // 南京站 - 南京林业大学·新庄
        {91,69,{CPoint(460,1044)}}, // 螺塘路 - 油坊桥
        {92,93,{CPoint(420,897)}}, // 雨润大街 - 元通
        {69,92,{CPoint(460,941)}}, // 油坊桥 - 雨润大街
        {96,97,{CPoint(420,721)}}, // 集庆门大街 - 云锦路
        {97,98,{CPoint(434,715)}}, // 云锦路 - 莫愁湖
        {124,125,{CPoint(952,653)}}, // 汇通路 - 灵山
        {127,128,{CPoint(1052,574)}}, // 孟北 - 西岗桦墅
        {21,57,{CPoint(526,1065)}}, // 南京南站 - 翠屏山
        {70,71,{CPoint(357,954)}}, // 永初路 - 平良大街
        {75,76,{CPoint(357,1106),CPoint(139,1106)}}, // 刘村 - 马骡圩
    };
    std::vector<CPoint> result{start};
    for(const auto& b:bends){
        if(b.a==from&&b.b==to)result.insert(result.end(),b.points.begin(),b.points.end());
        else if(b.b==from&&b.a==to)result.insert(result.end(),b.points.rbegin(),b.points.rend());
    }
    result.push_back(end);return result;
}
