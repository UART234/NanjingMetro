# 南京地铁换乘查询 (NanjingMetro)

一个基于 **MFC（Microsoft Foundation Classes）** 开发的南京地铁线路换乘查询桌面程序，支持站点搜索、最短/最优换乘路径计算、首末班车时刻表查询以及历史记录管理。

## 功能特性

- **线路与站点管理**：内置南京地铁线网数据（`metro_data.txt`），支持多线路、换乘站建模。
- **站点搜索**：按名称快速检索地铁站点（`StationSearchDlg`）。
- **路径规划**：基于图结构（`MetroGraph`）的换乘策略（`RouteStrategy`），计算两站之间的最优乘车方案，并展示换乘次数、经过站点与预计耗时（`PathResultDlg`）。
- **首末班车时刻表**：查询各线路首班车 / 末班车时间（`ServiceTimetable`）。
- **历史记录**：保存与回看历史查询记录（`HistoryManager` / `HistoryDlg`）。
- **集成测试**：内置 `IntegrationTests` 工程，可校验路径计算与数据一致性。

## 目录结构

```
南京地铁_V06/
├── 构建.ps1              # 一键构建脚本（需要 Visual Studio 2022 + MFC）
├── 可运行程序/           # 已编译的可执行程序与数据文件
│   ├── NanjingMetro.exe
│   ├── metro_data.txt
│   └── service_times.txt
└── 源码/
    └── NanjingMetro/     # MFC 工程源码（.vcxproj）
```

## 环境要求

- Windows 10 / 11
- **Visual Studio 2022**（含“使用 C++ 的桌面开发”与 **MFC** 组件）
- PowerShell 5.1+

## 构建

在项目根目录（`南京地铁_V06`）下以 PowerShell 运行：

```powershell
# Release 构建（默认）
.\构建.ps1

# Debug 构建
.\构建.ps1 -Configuration Debug

# 构建并运行集成测试
.\构建.ps1 -Test
```

构建产物默认输出到 `可运行程序/`，日志与缓存位于 `临时文件/`、`构建产物/`、`构建缓存/`（已被 `.gitignore` 忽略）。

## 直接运行

若不想自行编译，可直接使用 `可运行程序/NanjingMetro.exe`（需运行在 Windows 环境）。

## 许可

本项目以源代码形式开源，具体许可条款请在发布仓库中补充（如添加 `LICENSE` 文件）。
