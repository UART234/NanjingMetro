#include "pch.h"
#include "MetroData.h"
#include "MetroGraph.h"
#include "RouteStrategy.h"
#include "ServiceTimetable.h"
#include "HistoryManager.h"
#include "NanjingMetroDoc.h"
#include "MapLayout.h"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <queue>
#include <fstream>
#include <set>

CWinApp testApp;
int checks=0;
void Check(bool ok,const char* name)/* 编写者：肖博腾（4号） */ {++checks;if(!ok){std::cerr<<"FAIL "<<name<<std::endl;exit(1);}}
int main(int argc,char**) // 编写者：肖博腾（4号）
{
    UNREFERENCED_PARAMETER(argc);
    Check(AfxWinInit(GetModuleHandle(nullptr),nullptr,GetCommandLine(),0)!=FALSE,"MFC init");
    CMetroData data;Check(data.LoadDataFromFile(_T("metro_data.txt")),"data load");Check(data.ValidateData(),"data valid");
    Check(data.m_stations.size()==129 && data.m_lines.size()==6,"129 stations / 6 lines");
    MetroGraph graph;graph.BuildGraph(data);
    const int n=130;const double inf=1e20;
    std::vector<std::vector<double>> dist(n,std::vector<double>(n,inf));
    std::vector<std::vector<int>> hops(n,std::vector<int>(n,9999));
    std::map<int,std::set<int>> stationLines;
    std::map<int,int> lineIndex;int index=0;for(auto& line:data.m_lines)lineIndex[line.lineId]=index++;
    int lineDistance[6][6];for(int i=0;i<6;++i)for(int j=0;j<6;++j)lineDistance[i][j]=i==j?0:9999;
    size_t edgeCount=0;
    for(const auto& line:data.m_lines){for(int id:line.stationIds)stationLines[id].insert(line.lineId);
        for(size_t i=1;i<line.stationIds.size();++i){int a=line.stationIds[i-1],b=line.stationIds[i];double d=line.distances[i-1];Check(d>0.05 && d<20,"kilometres not metres");dist[a][b]=dist[b][a]=(std::min)(dist[a][b],d);hops[a][b]=hops[b][a]=1;++edgeCount;}}
    Check(edgeCount==133,"133 undirected intervals");
    int bentEdges=0;
    for(const auto& line:data.m_lines)for(size_t i=1;i<line.stationIds.size();++i){
        int a=line.stationIds[i-1],b=line.stationIds[i];
        auto points=MapSegmentPoints(a,b,data.m_stations.at(a).pos,data.m_stations.at(b).pos);
        auto reverse=MapSegmentPoints(b,a,data.m_stations.at(b).pos,data.m_stations.at(a).pos);
        Check(points.front()==data.m_stations.at(a).pos&&points.back()==data.m_stations.at(b).pos,"map interval remains anchored to clickable stations");
        std::reverse(reverse.begin(),reverse.end());Check(points==reverse,"reverse route uses identical map geometry");
        if(points.size()>2)++bentEdges;
    }
    Check(bentEdges==15,"all 15 official-map bends belong to real graph intervals");
    for(const auto& s:stationLines)for(int a:s.second)for(int b:s.second)if(a!=b)lineDistance[lineIndex[a]][lineIndex[b]]=1;
    for(int k=0;k<6;++k)for(int i=0;i<6;++i)for(int j=0;j<6;++j)lineDistance[i][j]=(std::min)(lineDistance[i][j],lineDistance[i][k]+lineDistance[k][j]);
    for(int i=1;i<n;++i){dist[i][i]=0;hops[i][i]=0;}
    for(int k=1;k<n;++k)for(int i=1;i<n;++i)for(int j=1;j<n;++j){dist[i][j]=(std::min)(dist[i][j],dist[i][k]+dist[k][j]);hops[i][j]=(std::min)(hops[i][j],hops[i][k]+hops[k][j]);}
    // Independent Floyd-Warshall station distances and line-intersection transfer oracle.
    int routes=0;
    for(int a=1;a<n;++a)for(int b=1;b<n;++b){
        int transfers=9999;for(int l:stationLines[a])for(int m:stationLines[b])transfers=(std::min)(transfers,lineDistance[lineIndex[l]][lineIndex[m]]);
        for(int strategy=0;strategy<3;++strategy){auto r=graph.FindRoute(a,b,(RouteStrategy)strategy);++routes;
            Check(r.isFound,"all pairs reachable");Check(r.stationSequence.front()==a&&r.stationSequence.back()==b,"route endpoints");
            Check(r.totalStations==(int)r.stationSequence.size()-1,"station count");
            Check(r.ticketPrice==CalcTicketPrice(dist[a][b]),"fare uses shortest distance for all strategies");
            if(strategy==0)Check(std::abs(r.totalDistanceKm-dist[a][b])<1e-8,"Dijkstra vs Floyd");
            if(strategy==1)Check(r.totalStations==hops[a][b],"minimum stations vs Floyd");
            if(strategy==2)Check((int)r.transferStations.size()==transfers,"minimum transfers vs line graph");
            double sum=0;int segCount=0;
            for(const auto& seg:r.transferGuides){Check(!seg.startStationName.IsEmpty()&&!seg.endStationName.IsEmpty()&&!seg.directionStationName.IsEmpty(),"UI guide names and direction");segCount+=seg.passStationCount;}
            Check(segCount==r.totalStations,"guide covers route");
            for(size_t i=1;i<r.stationSequence.size();++i){int u=r.stationSequence[i-1],v=r.stationSequence[i];Check(hops[u][v]==1,"route adjacency");sum+=dist[u][v];}
            Check(std::abs(sum-r.totalDistanceKm)<1e-8,"route distance sum");
        }
    }
    Check(!graph.FindRoute(-1,-1,STRATEGY_SHORTEST_DIST).isFound,"invalid same station rejected");
    Check(!graph.FindRoute(1,999,STRATEGY_SHORTEST_DIST).isFound,"unknown destination");
    Check(!graph.FindRoute(1,1,(RouteStrategy)99).isFound,"unknown strategy");
    auto interval=graph.FindRoute(1,2,STRATEGY_SHORTEST_DIST);Check(std::abs(interval.totalDistanceKm-1.370)<1e-8 && interval.ticketPrice==2,"known interval 1.370 km not 0.001370 km");
    auto direct=graph.FindRoute(13,21,STRATEGY_SHORTEST_DIST);std::cout<<"129-station network: Xinjiekou to Nanjing South "<<direct.totalDistanceKm<<" km, fare "<<direct.ticketPrice<<std::endl;
    const double bounds[]={4,9,14,21,28,37,48,61,76,91};
    Check(CalcTicketPrice(0)==0,"zero fare");for(int i=0;i<10;++i){Check(CalcTicketPrice(bounds[i])==i+2,"fare boundary");Check(CalcTicketPrice(bounds[i]+0.001)==i+3,"fare beyond boundary");}
    // New algorithm regression: equal two-stop paths, one path transfers, the other stays on line 3.
    MetroData tiny;for(int i=1;i<=5;++i){StationNode s;s.id=i;s.name.Format(_T("S%d"),i);tiny.stations.push_back(s);}
    auto add=[&](int id,std::vector<int> ids){MetroLine l;l.lineId=id;l.lineName.Format(_T("L%d"),id);l.stationIds=ids;l.distances.assign(ids.size()-1,1);tiny.lines.push_back(l);};
    add(1,{1,2});add(2,{2,4});add(3,{1,3,4});
    MetroGraph tinyGraph;tinyGraph.BuildGraph(tiny);auto tie=tinyGraph.FindRoute(1,4,STRATEGY_MIN_STATIONS);
    Check(tie.totalStations==2&&tie.transferStations.empty(),"new minimum stations tie-break");
    Check(!tinyGraph.FindRoute(1,5,STRATEGY_MIN_TRANSFERS).isFound,"disconnected station");
    CServiceTimetable table;Check(table.Load(_T("service_times.txt")),"timetable load");Check(table.Count()==266,"266 unique timetable records");
    auto south=table.ForStation(_T("南京南站"));Check(south.size()==6,"Nanjing South six departures");
    bool first=false;for(auto& row:south)if(row.lineId==1&&row.direction==_T("中国药科大学"))first=row.first==_T("06:17")&&row.last==_T("23:54")&&row.lastFriSat==_T("00:16");Check(first,"official Nanjing South weekday and weekend times");
    size_t expectedTimes=0, extras=0;
    for(const auto& line:data.m_lines){
        const CString firstName=data.m_stations.at(line.stationIds.front()).name,lastName=data.m_stations.at(line.stationIds.back()).name;
        for(size_t i=0;i<line.stationIds.size();++i){
            auto rows=table.ForStation(data.m_stations.at(line.stationIds[i]).name,line.lineId);
            size_t expected=(i>0?1:0)+(i+1<line.stationIds.size()?1:0);expectedTimes+=expected;
            Check(rows.size()==expected,"every station has all outbound directions, no terminal self-departures");
            std::set<std::wstring> actual;
            for(const auto& row:rows){
                actual.insert(row.direction.GetString());
                Check(row.first!=_T("--:--")&&row.last!=_T("--:--")&&row.lastFriSat!=_T("--:--"),"all first and weekday/weekend last times known");
                if(!row.extraDirectionSunThu.IsEmpty())++extras;
            }
            if(i>0)Check(actual.count(firstName.GetString())==1,"departure towards first terminus");
            if(i+1<line.stationIds.size())Check(actual.count(lastName.GetString())==1,"departure towards last terminus");
        }
    }
    Check(expectedTimes==table.Count()&&extras==28,"266 directions and 28 additional weekday short turns");
    Check(CServiceTimetable::DisplayTime(_T("00:16"))==_T("次日 00:16"),"midnight day label");
    Check(table.ForStation(_T("不存在")).empty(),"missing station");
    Check(CServiceTimetable::DisplayTime(_T("--:--"))==_T("待核实"),"missing not fabricated");
    Check(!table.Load(_T("missing_file.txt"))&&table.Count()==0,"missing file clears stale data");
    {std::ofstream out("test_times.txt");out<<"A|1|B|23:10|00:32|amap-2026-09-09\nA|1|B|23:10|00:32|amap-2026-09-09\nA|1|C|25:00|23:00|amap-2026-09-09\nA|99|D|06:00|23:00|amap-2026-09-09\n";}
    Check(table.Load(_T("test_times.txt"))&&table.Count()==1,"malformed and duplicate rejected; midnight retained");
    Check(table.ForStation(_T("A"))[0].last==_T("00:32"),"cross-midnight time retained");
    TCHAR historyPath[MAX_PATH] = {};
    GetFullPathName(_T("test_history_location.txt"),MAX_PATH,historyPath,nullptr);
    CHistoryManager history;history.LoadFromFile(historyPath);history.ClearHistory();
    history.AddRecord(21,_T("南京南站"),45,_T("大行宫"),STRATEGY_SHORTEST_DIST,true);
    Check(history.SaveToFile(_T("ignored_relative_history.txt")),"save to remembered absolute path");
    CHistoryManager reopened;Check(reopened.LoadFromFile(historyPath),"reload history from expected location");
    Check(!reopened.GetFavorites().empty()&&reopened.GetFavorites().front().endStationId==45&&reopened.GetFavorites().front().startStationName==_T("南京南站")&&reopened.GetFavorites().front().endStationName==_T("大行宫"),"favorite Chinese names round trip");
    // Favorites are unique by ordered endpoints and strategy, including legacy files.
    CHistoryManager favorites;
    favorites.AddRecord(1,_T("A"),2,_T("B"),STRATEGY_SHORTEST_DIST,false);
    for(int i=0;i<10;++i) favorites.AddRecord(1,_T("A"),2,_T("B"),STRATEGY_SHORTEST_DIST,true);
    Check(favorites.GetFavorites().size()==1 && favorites.GetAllRecords().size()==1,"repeated favorite promotes existing query without duplicates");
    favorites.AddRecord(1,_T("A"),2,_T("B"),STRATEGY_SHORTEST_DIST,false);
    favorites.AddRecord(1,_T("A"),2,_T("B"),STRATEGY_SHORTEST_DIST,true);
    Check(favorites.GetFavorites().size()==1 && favorites.GetAllRecords().size()==2,"requery then favorite stays unique");
    favorites.ToggleFavorite(0);
    Check(!favorites.IsFavorite(1,2,STRATEGY_SHORTEST_DIST),"cancel via duplicate history row cancels route favorite");
    favorites.ToggleFavorite(0);
    Check(favorites.GetFavorites().size()==1,"toggle restores one favorite");
    favorites.AddRecord(2,_T("B"),1,_T("A"),STRATEGY_SHORTEST_DIST,true);
    favorites.AddRecord(1,_T("A"),2,_T("B"),STRATEGY_MIN_STATIONS,true);
    Check(favorites.GetFavorites().size()==3,"reverse direction and strategy remain distinct");
    for(int i=0;i<100;++i) favorites.AddRecord(3,_T("C"),4,_T("D"),STRATEGY_SHORTEST_DIST,false);
    Check(favorites.GetFavorites().size()==3 && favorites.GetAllRecords().size()==53,"history cap never removes favorites");
    favorites.ClearHistory();
    Check(favorites.GetAllRecords().size()==3,"clear history retains all favorites");
    Check(favorites.SaveToFile(_T("test_favorites.txt")),"favorites save");
    CHistoryManager loadedFavorites; Check(loadedFavorites.LoadFromFile(_T("test_favorites.txt")),"favorites reload");
    Check(loadedFavorites.GetFavorites().size()==3,"favorites survive restart");
    {std::ofstream out("test_duplicate_favorites.txt");
        out<<"1\tA\t2\tB\t0\t2026-09-09 12:00\t1\n1\tA\t2\tB\t0\t2026-09-09 11:00\t1\n1\tA\t2\tB\t0\t2026-09-09 10:00\t1\n";}
    CHistoryManager legacy;Check(legacy.LoadFromFile(_T("test_duplicate_favorites.txt")),"legacy favorites load");
    Check(legacy.GetFavorites().size()==1 && legacy.GetAllRecords().size()==3,"legacy duplicate favorites normalize without losing query history");
    legacy.ToggleFavorite(2);
    Check(legacy.GetFavorites().empty(),"cancel legacy favorite through historical duplicate");
    legacy.ToggleFavorite(1);
    Check(legacy.GetFavorites().size()==1,"refavorite legacy route stays unique");
    Check(legacy.SaveToFile(_T("test_duplicate_favorites.txt")),"save normalized legacy favorites");
    CHistoryManager legacyReload;Check(legacyReload.LoadFromFile(_T("test_duplicate_favorites.txt")) && legacyReload.GetFavorites().size()==1,"normalized favorites persist");
    legacyReload.ToggleFavorite(-1);legacyReload.ToggleFavorite(999);
    Check(legacyReload.GetFavorites().size()==1,"invalid selection leaves favorites intact");
    legacyReload.ClearAllFavorites();Check(legacyReload.GetFavorites().empty(),"empty favorites supported");
    // Exercise the actual MFC document lifecycle, not just manager serialization.
    // The test executable has its own output directory; preserve any preexisting local history.
    TCHAR testModule[MAX_PATH]={}; GetModuleFileName(nullptr,testModule,MAX_PATH);
    CString docHistoryPath(testModule);docHistoryPath=docHistoryPath.Left(docHistoryPath.ReverseFind(_T('\\'))+1)+_T("user_history.txt");
    std::ifstream oldHistory(docHistoryPath.GetString(),std::ios::binary);
    const bool existed=oldHistory.is_open();
    std::string oldBytes((std::istreambuf_iterator<char>(oldHistory)),std::istreambuf_iterator<char>());oldHistory.close();
    {std::ofstream out(docHistoryPath.GetString(),std::ios::binary|std::ios::trunc);out<<"21\tA\t45\tB\t0\t2026-09-09 12:00\t1\n21\tA\t45\tB\t0\t2026-09-09 11:00\t1\n";}
    auto document=static_cast<CNanjingMetroDoc*>(RUNTIME_CLASS(CNanjingMetroDoc)->CreateObject());
    Check(document && document->OnNewDocument(),"MFC document initial load");
    Check(document->m_historyMgr.GetFavorites().size()==1,"startup preserves and normalizes existing favorites");
    Check(document->OnNewDocument() && document->m_historyMgr.GetFavorites().size()==1,"new document preserves favorites");
    document->DeleteContents();delete document;
    auto restarted=static_cast<CNanjingMetroDoc*>(RUNTIME_CLASS(CNanjingMetroDoc)->CreateObject());
    Check(restarted && restarted->OnNewDocument() && restarted->m_historyMgr.GetFavorites().size()==1,"fresh document reloads saved favorites");
    restarted->DeleteContents();delete restarted;
    if(existed){std::ofstream out(docHistoryPath.GetString(),std::ios::binary|std::ios::trunc);out.write(oldBytes.data(),(std::streamsize)oldBytes.size());}
    else DeleteFile(docHistoryPath);
    std::cout<<"PASS "<<checks<<" checks, "<<routes<<" routes (129 x 129 x 3), 266 timetable records; favorites and MFC lifecycle regressions passed."<<std::endl;
    return 0;
}
