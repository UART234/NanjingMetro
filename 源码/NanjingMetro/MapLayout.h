#pragma once
// 15 处官方图弯道；折点坐标已随站点坐标重排按相对位置同步重算。
inline std::vector<CPoint> MapSegmentPoints(int from,int to,CPoint start,CPoint end) /* 编写者：刘子瑜（3号） */ {
    struct Bend {int a,b;std::vector<CPoint> points;};
    static const std::vector<Bend> bends={
        {8,9,{CPoint(567,579),CPoint(552,585)}}, // 南京站 - 新模范马路
        {27,28,{CPoint(720,1106)}}, // 竹山路 - 天印大道
        {29,30,{CPoint(830,1117)}}, // 龙眠大道 - 南医大·江苏经贸学院
        {38,39,{CPoint(476,457)}}, // 柳洲东路 - 上元门
        {8,42,{CPoint(604,595)}}, // 南京站 - 南京林业大学·新庄
        {91,69,{CPoint(434,1035)}}, // 螺塘路 - 油坊桥
        {92,93,{CPoint(418,900)}}, // 雨润大街 - 元通
        {69,92,{CPoint(458,944)}}, // 油坊桥 - 雨润大街
        {96,97,{CPoint(435,745)}}, // 集庆门大街 - 云锦路
        {97,98,{CPoint(441,737)}}, // 云锦路 - 莫愁湖
        {124,125,{CPoint(924,644)}}, // 汇通路 - 灵山
        {127,128,{CPoint(1014,565)}}, // 孟北 - 西岗桦墅
        {21,57,{CPoint(545,1007)}}, // 南京南站 - 翠屏山
        {70,71,{CPoint(384,977)}}, // 永初路 - 平良大街
        {75,76,{CPoint(252,1123),CPoint(215,1140)}}, // 刘村 - 马骡圩
    };
    std::vector<CPoint> result{start};
    for(const auto& b:bends){
        if(b.a==from&&b.b==to)result.insert(result.end(),b.points.begin(),b.points.end());
        else if(b.b==from&&b.a==to)result.insert(result.end(),b.points.rbegin(),b.points.rend());
    }
    result.push_back(end);return result;
}
