#include "pch.h"
#include "MetroData.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace
{
	CString Utf8ToCString(const std::string& utf8) // 编写者：刘子瑜（3号）
	{
		if (utf8.empty())
			return CString();

		const int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
		if (wideLen <= 0)
			return CString();

		CStringW wide;
		const int written = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, wide.GetBuffer(wideLen), wideLen);
		if (written <= 0)
			return CString();
		wide.ReleaseBuffer(written - 1);
		return CString(wide);
	}

	COLORREF LineColorForName(const CString& lineName, size_t index) // 编写者：刘子瑜（3号）
	{
		// 注意：必须用精确匹配，避免 "1号线" 误匹配 "10号线"、"S1号线" 等。
		if (lineName == _T("1号线"))  return RGB(0, 160, 201);   // 天蓝色
		if (lineName == _T("2号线"))  return RGB(198, 0, 86);    // 枣红色
		if (lineName == _T("3号线"))  return RGB(0, 140, 86);    // 薄绿色
		if (lineName == _T("4号线"))  return RGB(121, 103, 166); // 青紫色
		if (lineName == _T("5号线"))  return RGB(242, 218, 81);  // 鹅黄色
		if (lineName == _T("7号线"))  return RGB(0, 90, 60);     // 墨绿色
		if (lineName == _T("10号线")) return RGB(212, 168, 74);  // 香槟色
		if (lineName == _T("S1号线")) return RGB(108, 189, 181); // 青绿色
		if (lineName == _T("S2号线")) return RGB(196, 84, 108);  // 胭脂色
		if (lineName == _T("S3号线")) return RGB(185, 155, 194); // 粉紫色
		if (lineName == _T("S6号线")) return RGB(167, 120, 199); // 丁香色
		if (lineName == _T("S7号线")) return RGB(233, 140, 160); // 薄红梅色
		if (lineName == _T("S8号线")) return RGB(243, 168, 60);  // 橙黄色
		if (lineName == _T("S9号线")) return RGB(216, 200, 50);  // 藤黄色
		static const COLORREF palette[] = {
			RGB(255, 120, 0), RGB(220, 40, 60),
			RGB(60, 120, 200), RGB(150, 80, 190)
		};
		return palette[index % 4];
	}

	bool LoadSegmentFormat(CMetroData& data, const std::vector<std::string>& rawLines) // 编写者：刘子瑜（3号）
	{
		std::map<std::wstring, int> stationIds;
		MetroLine* currentLine = nullptr;

		auto stationIdFor = [&](const CString& name) -> int
		{
			std::wstring key((LPCTSTR)name);
			std::map<std::wstring, int>::iterator it = stationIds.find(key);
			if (it != stationIds.end())
				return it->second;

			int id = (int)data.m_stations.size() + 1;
			StationNode station;
			station.id = id;
			station.name = name;
			data.m_stations[id] = station;
			stationIds[key] = id;
			return id;
		};

		for (size_t i = 0; i < rawLines.size(); ++i)
		{
			const std::string& raw = rawLines[i];
			if (raw.empty() || raw[0] == '#' || raw[0] == '\r')
				continue;

			CString text = Utf8ToCString(raw);
			text.TrimRight();
			text.TrimLeft();
			if (text.IsEmpty())
				continue;

			// 线路头，例如 "1号线： 距离单位：米"
			if (text.Find(_T("号线")) > 0)
			{
				int colon = text.Find(_T('：'));
				CString lineName = (colon > 0) ? text.Left(colon) : text;
				lineName.Trim();
				if (lineName.IsEmpty())
					continue;

				MetroLine line;
				line.lineName = lineName;
				data.m_lines.push_back(line);
				currentLine = &data.m_lines.back();
				continue;
			}

			if (!currentLine)
				continue;

			int tab = text.Find(_T('\t'));
			if (tab <= 0)
				continue;

			CString segment = text.Left(tab);
			CString distanceText = text.Mid(tab + 1);
			distanceText.Trim();
			if (distanceText.IsEmpty())
				continue;

			int dash = segment.Find(_T("—"));
			if (dash <= 0)
				continue;

			CString nameA = segment.Left(dash);
			CString nameB = segment.Mid(dash + 1);
			nameA.Trim();
			nameB.Trim();
			if (nameA.IsEmpty() || nameB.IsEmpty())
				continue;

			int idA = stationIdFor(nameA);
			int idB = stationIdFor(nameB);

			if (currentLine->stationIds.empty() || currentLine->stationIds.back() != idA)
				currentLine->stationIds.push_back(idA);
			currentLine->stationIds.push_back(idB);
			currentLine->distances.push_back(_ttof(distanceText) / 1000.0); // 原分段格式使用米，统一转公里
		}

		for (size_t i = 0; i < data.m_lines.size(); )
		{
			if (data.m_lines[i].stationIds.empty())
				data.m_lines.erase(data.m_lines.begin() + i);
			else
				++i;
		}

		for (size_t i = 0; i < data.m_lines.size(); ++i)
		{
			MetroLine& line = data.m_lines[i];
			line.lineId = (int)i + 1;
			line.color = LineColorForName(line.lineName, i);

			for (size_t j = 0; j < line.stationIds.size(); ++j)
			{
				std::map<int, StationNode>::iterator it = data.m_stations.find(line.stationIds[j]);
				if (it == data.m_stations.end())
					continue;
				std::vector<int>& lineIds = it->second.lineIds;
				if (std::find(lineIds.begin(), lineIds.end(), line.lineId) == lineIds.end())
					lineIds.push_back(line.lineId);
			}
		}

		for (std::map<int, StationNode>::iterator it = data.m_stations.begin();
			it != data.m_stations.end(); ++it)
		{
			it->second.isTransfer = it->second.lineIds.size() > 1;
		}

		data.LayoutNetwork();
		return true;
	}

	bool LoadLegacyFormat(CMetroData& data, const std::vector<std::string>& rawLines) // 编写者：刘子瑜（3号）
	{
		std::string section;
		size_t idx = 0;
		while (idx < rawLines.size())
		{
			const std::string& line = rawLines[idx++];
			if (line.empty() || line[0] == '#' || line[0] == '\r')
				continue;

			if (line[0] == '[')
			{
				section = line;
				continue;
			}

			std::stringstream ss(line);

			if (section == "[STATIONS]")
			{
				StationNode station;
				int isTransferInt = 0, landmarkCount = 0;
				std::string nameStr;

				ss >> station.id >> nameStr >> station.pos.x >> station.pos.y >> isTransferInt >> landmarkCount;
				station.name = Utf8ToCString(nameStr);
				station.isTransfer = (isTransferInt == 1);

				for (int i = 0; i < landmarkCount; ++i)
				{
					std::string lmToken;
					ss >> lmToken;
					if (lmToken.empty())
						continue;

					std::stringstream lmSS(lmToken);
					std::string typeStr, lmName, desc;
					std::getline(lmSS, typeStr, '|');
					std::getline(lmSS, lmName, '|');
					std::getline(lmSS, desc, '|');

					Landmark lm;
					lm.type = (LandmarkType)std::stoi(typeStr);
					lm.name = Utf8ToCString(lmName);
					lm.description = Utf8ToCString(desc);
					station.landmarks.push_back(lm);
				}

				data.m_stations[station.id] = station;
			}
			else if (section == "[LINES]")
			{
				std::string tag;
				ss >> tag;
				if (tag != "LINE")
					continue;

				MetroLine metroLine;
				std::string lineNameStr;
				int r = 0, g = 0, b = 0, stationCount = 0;

				ss >> metroLine.lineId >> lineNameStr >> r >> g >> b >> stationCount;
				metroLine.lineName = Utf8ToCString(lineNameStr);
				metroLine.color = RGB(r, g, b);

				if (idx < rawLines.size())
				{
					std::stringstream ssStations(rawLines[idx++]);
					for (int i = 0; i < stationCount; ++i)
					{
						int sid = 0;
						ssStations >> sid;
						metroLine.stationIds.push_back(sid);
						if (data.m_stations.find(sid) != data.m_stations.end())
						{
							data.m_stations[sid].lineIds.push_back(metroLine.lineId);
							if (data.m_stations[sid].lineIds.size() > 1)
								data.m_stations[sid].isTransfer = true;
						}
					}
				}

				if (idx < rawLines.size())
				{
					std::stringstream ssDistances(rawLines[idx++]);
					for (int i = 0; i < stationCount - 1; ++i)
					{
						double dist = 0.0;
						ssDistances >> dist;
						metroLine.distances.push_back(dist);
					}
				}

				data.m_lines.push_back(metroLine);
			}
		}
		return true;
	}
}

CMetroData::CMetroData() // 编写者：刘子瑜（3号）
{
}

CMetroData::~CMetroData() // 编写者：刘子瑜（3号）
{
}

StationNode* CMetroData::GetStationById(int stationId) // 编写者：刘子瑜（3号）
{
	auto it = m_stations.find(stationId);
	if (it != m_stations.end())
		return &(it->second);
	return nullptr;
}

StationNode* CMetroData::GetStationByName(const CString& name) // 编写者：刘子瑜（3号）
{
	for (auto& pair : m_stations)
	{
		if (pair.second.name == name)
			return &(pair.second);
	}
	return nullptr;
}

MetroLine* CMetroData::GetLineById(int lineId) // 编写者：刘子瑜（3号）
{
	for (auto& line : m_lines)
	{
		if (line.lineId == lineId)
			return &line;
	}
	return nullptr;
}

bool CMetroData::LoadDataFromFile(const CString& filePath) // 编写者：刘子瑜（3号）
{
	std::ifstream file(filePath.GetString()); // 中文工作区路径使用宽字符，避免系统代码页转换丢失。
	if (!file.is_open())
		return false;

	std::vector<std::string> rawLines;
	std::string line;
	while (std::getline(file, line))
		rawLines.push_back(line);
	file.close();

	bool legacy = false;
	for (size_t i = 0; i < rawLines.size(); ++i)
	{
		if (rawLines[i].find("[STATIONS]") != std::string::npos)
		{
			legacy = true;
			break;
		}
	}

	m_stations.clear();
	m_lines.clear();

	bool ok = legacy ? LoadLegacyFormat(*this, rawLines) : LoadSegmentFormat(*this, rawLines);
	if (ok && !ValidateData(nullptr)) { m_stations.clear(); m_lines.clear(); return false; }
	return ok;
}

bool CMetroData::ValidateData(CString* error) const // 编写者：刘子瑜（3号）
{
	for (std::map<int, StationNode>::const_iterator s=m_stations.begin(); s!=m_stations.end(); ++s)
		if (s->first <= 0 || s->second.name.IsEmpty()) { if(error)*error=_T("站点 ID 或站名非法"); return false; }
	for (size_t i=0;i<m_lines.size();++i) { const MetroLine& l=m_lines[i]; if (l.stationIds.size()<2 || l.distances.size()!=l.stationIds.size()-1) { if(error)*error=_T("线路站点数与距离数不匹配"); return false; } for(size_t j=0;j<l.stationIds.size();++j) if(m_stations.find(l.stationIds[j])==m_stations.end()) { if(error)*error=_T("线路引用了不存在的站点 ID"); return false; } }
	return true;
}

void CMetroData::LayoutNetwork() // 编写者：刘子瑜（3号）
{
	const int kViewW = 1600;
	const int kViewH = 1400;
	const double kMargin = 80.0;
	const int n = (int)m_stations.size();
	if (n == 0)
		return;

	std::vector<int> ids;
	ids.reserve(n);
	std::map<int, int> indexOf;
	for (std::map<int, StationNode>::iterator it = m_stations.begin(); it != m_stations.end(); ++it)
	{
		indexOf[it->first] = (int)ids.size();
		ids.push_back(it->first);
	}

	std::vector<std::vector<int> > adj(n);
	for (size_t li = 0; li < m_lines.size(); ++li)
	{
		const MetroLine& line = m_lines[li];
		for (size_t j = 0; j + 1 < line.stationIds.size(); ++j)
		{
			std::map<int, int>::iterator ia = indexOf.find(line.stationIds[j]);
			std::map<int, int>::iterator ib = indexOf.find(line.stationIds[j + 1]);
			if (ia == indexOf.end() || ib == indexOf.end())
				continue;
			int a = ia->second;
			int b = ib->second;
			if (std::find(adj[a].begin(), adj[a].end(), b) == adj[a].end())
				adj[a].push_back(b);
			if (std::find(adj[b].begin(), adj[b].end(), a) == adj[b].end())
				adj[b].push_back(a);
		}
	}

	std::vector<double> xs(n, 0.0), ys(n, 0.0);
	std::vector<int> coverCount(n, 0);
	const double pi = 3.14159265358979323846;
	const size_t lineCount = m_lines.size();

	// 先沿每条线路的弧线放置站点，换乘站在多条线之间取平均。
	for (size_t li = 0; li < lineCount; ++li)
	{
		const MetroLine& line = m_lines[li];
		const double baseAngle = pi / 2.0 + (2.0 * pi * li) / (lineCount > 0 ? lineCount : 1);
		const double spread = 1.7;
		const size_t stationCount = line.stationIds.size();
		for (size_t j = 0; j < stationCount; ++j)
		{
			const double t = (stationCount <= 1) ? 0.5 : (double)j / (stationCount - 1);
			const double angle = baseAngle - spread / 2.0 + spread * t;
			const double radius = kViewW * 0.34 + (li % 4) * 70.0;
			std::map<int, int>::iterator it = indexOf.find(line.stationIds[j]);
			if (it == indexOf.end())
				continue;
			const int idx = it->second;
			xs[idx] += kViewW / 2.0 + std::cos(angle) * radius;
			ys[idx] += kViewH / 2.0 + std::sin(angle) * radius;
			coverCount[idx] += 1;
		}
	}

	for (int i = 0; i < n; ++i)
	{
		if (coverCount[i] > 0)
		{
			xs[i] /= coverCount[i];
			ys[i] /= coverCount[i];
		}
		else
		{
			const double angle = 2.0 * pi * i / n;
			xs[i] = kViewW / 2.0 + std::cos(angle) * kViewW * 0.3;
			ys[i] = kViewH / 2.0 + std::sin(angle) * kViewH * 0.3;
		}
	}

	// Fruchterman-Reingold 风格的松弛布局，让站点均匀铺开。
	const double area = (kViewW * 0.7) * (kViewH * 0.7);
	const double k = std::sqrt(area / n);
	double temperature = kViewW * 0.22;
	for (int iter = 0; iter < 900; ++iter)
	{
		std::vector<double> dx(n, 0.0), dy(n, 0.0);
		for (int i = 0; i < n; ++i)
		{
			for (int j = i + 1; j < n; ++j)
			{
				double diffx = xs[i] - xs[j];
				double diffy = ys[i] - ys[j];
				double dist = std::sqrt(diffx * diffx + diffy * diffy);
				if (dist < 1.0) dist = 1.0;
				const double force = (k * k) / dist;
				dx[i] += diffx / dist * force;
				dy[i] += diffy / dist * force;
				dx[j] -= diffx / dist * force;
				dy[j] -= diffy / dist * force;
			}
		}

		for (int i = 0; i < n; ++i)
		{
			for (size_t j = 0; j < adj[i].size(); ++j)
			{
				const int b = adj[i][j];
				if (b <= i)
					continue;
				double diffx = xs[b] - xs[i];
				double diffy = ys[b] - ys[i];
				double dist = std::sqrt(diffx * diffx + diffy * diffy);
				if (dist < 1.0) dist = 1.0;
				const double force = (dist * dist) / k;
				dx[i] += diffx / dist * force;
				dy[i] += diffy / dist * force;
				dx[b] -= diffx / dist * force;
				dy[b] -= diffy / dist * force;
			}
		}

		for (int i = 0; i < n; ++i)
		{
			dx[i] += (kViewW / 2.0 - xs[i]) * 0.015;
			dy[i] += (kViewH / 2.0 - ys[i]) * 0.015;
			double len = std::sqrt(dx[i] * dx[i] + dy[i] * dy[i]);
			if (len > temperature)
			{
				dx[i] = dx[i] / len * temperature;
				dy[i] = dy[i] / len * temperature;
			}
			xs[i] += dx[i];
			ys[i] += dy[i];
			xs[i] = (std::max)(20.0, (std::min)((double)kViewW - 20.0, xs[i]));
			ys[i] = (std::max)(20.0, (std::min)((double)kViewH - 20.0, ys[i]));
		}
		temperature *= 0.995;
	}

	// 归一化到逻辑视口，并保留边距。
	double minX = xs[0], maxX = xs[0], minY = ys[0], maxY = ys[0];
	for (int i = 1; i < n; ++i)
	{
		minX = (std::min)(minX, xs[i]);
		maxX = (std::max)(maxX, xs[i]);
		minY = (std::min)(minY, ys[i]);
		maxY = (std::max)(maxY, ys[i]);
	}

	const double rangeX = (std::max)(maxX - minX, 1.0);
	const double rangeY = (std::max)(maxY - minY, 1.0);
	const double scale = (std::min)(
		(kViewW - kMargin * 2.0) / rangeX,
		(kViewH - kMargin * 2.0) / rangeY);
	const double cx = (minX + maxX) / 2.0;
	const double cy = (minY + maxY) / 2.0;

	for (int i = 0; i < n; ++i)
	{
		std::map<int, StationNode>::iterator it = m_stations.find(ids[i]);
		if (it == m_stations.end())
			continue;
		it->second.pos.x = (int)(kViewW / 2.0 + (xs[i] - cx) * scale + 0.5);
		it->second.pos.y = (int)(kViewH / 2.0 + (ys[i] - cy) * scale + 0.5);
	}
}
