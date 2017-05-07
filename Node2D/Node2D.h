#pragma once
#include "Node/Node.h"
#include "Node2DView.h"

class Node2DDeclear CNode2D : public CNode
{
public:
	explicit CNode2D(CNode* parent);
	~CNode2D();
	void DrawNode(DrawKit* pDrawKit);
};

class Node2DDeclear CScene2D : public CScene
{
public:
	void DrawNode(DrawKit* pDrawKit);
};

class Node2DDeclear CNode2DTextLayer : public CNode2D
{
public:
	explicit CNode2DTextLayer(CNode* parent);
	~CNode2DTextLayer();
	void DrawNode(DrawKit* pDrawKit);

	void CreateTextFormat(const std::wstring strFontName, float fontSize);
	void SetAlignment(DWRITE_TEXT_ALIGNMENT);
	void SetTextColor(D2D1::ColorF);
	const std::wstring GetText() { return m_strText; }
	void SetText(const std::wstring& text) { m_strText = text; }

protected:
	IDWriteTextFormat* m_pTextFormat;
	ID2D1SolidColorBrush* m_pTextColor;
	std::wstring m_strText;
};

