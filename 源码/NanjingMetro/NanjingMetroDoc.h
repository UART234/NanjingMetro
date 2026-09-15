
// NanjingMetroDoc.h: CNanjingMetroDoc 类的接口
//


#pragma once
#include "HistoryManager.h"
#include "MetroData.h"
class CNanjingMetroDoc : public CDocument
{
protected: // 仅从序列化创建
	CNanjingMetroDoc() noexcept;
	DECLARE_DYNCREATE(CNanjingMetroDoc)

// 特性
public:
	CHistoryManager m_historyMgr; // 全局历史与收藏管理器
	CMetroData m_metroData;       // global metro data (loaded from metro_data.txt)
// 操作
public:

// 重写
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

// 实现
public:
	virtual ~CNanjingMetroDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	bool m_historyInitialized = false;

// 生成的消息映射函数
protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// 用于为搜索处理程序设置搜索内容的 Helper 函数
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
public:
	virtual void DeleteContents();
};
