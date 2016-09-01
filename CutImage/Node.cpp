#include "Node.h"
#include <WindowsX.h>
#pragma comment(lib, "msimg32.lib")

CNode::CNode(CNode* pParent) :
	m_bNeedInit(true),
	m_bNeedUpdateRect(true),
	m_bNeedSortChild(false),
	m_bVisible(true),
	m_iData(0),
	m_iTag(0),
	m_iZOrder(0),
	m_iNCHitTest(HTCLIENT),
	m_pParent(NULL),
	m_pairAnchor(0.5f, 0.5f),
	m_pairMinSize(1.0f, 1.0f),
	m_pairMaxSize(5000.0f, 5000.0f),
	m_sizePolicy(SizePolicyFixed),
	m_bLBDown(false),
	m_bRBDown(false),
	m_bMouseIN(false),
	m_pCurrentNode(NULL),
	m_iFindIndex(-1)
{
	if (pParent)
	{
		pParent->AddChild(this);
	}
}

CNode::~CNode()
{
	if (m_pParent)
	{
		m_pParent->RemoveChild(this, false);
	}

	CNode* pNode;
	for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter)
	{
		pNode = *iter;
		if (pNode)
		{
			pNode->SetParent(NULL);
			delete pNode;
		}
	}
	TRACE("delete CNode\n");
}

CNode::CNode(CNode&& rr)
{
	m_bNeedInit = rr.m_bNeedInit;
	m_bNeedUpdateRect = rr.m_bNeedUpdateRect;
	m_bNeedSortChild = rr.m_bNeedSortChild;
	m_bVisible = rr.m_bVisible;
	m_iData = rr.m_iData;
	m_iTag = rr.m_iTag;
	m_iZOrder = rr.m_iZOrder;
	m_iNCHitTest = rr.m_iNCHitTest;
	m_pParent = rr.m_pParent;
	rr.m_pParent = NULL;
	m_rect = rr.m_rect;
	m_pairAnchor = rr.m_pairAnchor;
	m_pairSize = rr.m_pairSize;
	m_pairPos = rr.m_pairPos;
	m_Children = std::move(rr.m_Children);
	m_pairMaxSize = rr.m_pairMaxSize;
	m_pairMinSize = rr.m_pairMinSize;
	m_sizePolicy = rr.m_sizePolicy;

	m_pointLast = rr.m_pointLast;
	m_bLBDown = rr.m_bLBDown;
	m_bRBDown = rr.m_bRBDown;
	m_bMouseIN = rr.m_bMouseIN;
	m_pCurrentNode = rr.m_pCurrentNode;
}

void CNode::SetVisible(bool b)
{
	m_bVisible = true;
}

bool CNode::IsVisible()const
{
	return m_bVisible;
}

void CNode::SetData(int data)
{
	m_iData = data;
}

int  CNode::GetData()const
{
	return m_iData;
}

void CNode::SetTag(int tag)
{
	m_iTag = tag;
}

int  CNode::GetTag()const
{
	return m_iTag;
}

void CNode::SetZOrder(int order)
{
	if (order != m_iZOrder)
	{
		m_iZOrder = order;
		auto p = GetParent();
		if (p)
			p->NeedUpdate(UpdateFlagReSortchild);
	}
}

int  CNode::GetZOrder()const
{
	return m_iZOrder;
}

void CNode::SetNCHitTest(int value)
{
	m_iNCHitTest = value;
}

int  CNode::GetNCHitTest()const
{
	if (m_pCurrentNode)
	{
		return m_pCurrentNode->GetNCHitTest();
	}
	return m_iNCHitTest;
}

void CNode::SetRect(const RECT& rect)
{
	if (m_pParent)
	{
		RECT r = m_pParent->GetRect();
		int w = rect.right - rect.left;
		int h = rect.bottom - rect.top;
		m_pairPos.first = rect.left - r.left + w / 2.0f;
		m_pairPos.second = rect.top - r.top + h / 2.0f;
		m_pairSize.first = w*1.0f;
		m_pairSize.second = h*1.0f;
	}
	else
		m_rect = rect;
	NeedUpdate(UpdateFlagReSize);
}

RECT CNode::GetRect()
{
	if (m_bNeedUpdateRect)
	{
		m_bNeedUpdateRect = false;
		if (m_pParent)
		{
			RECT r = m_pParent->GetRect();
			float fx = r.left + m_pairPos.first;
			float fy = r.top + m_pairPos.second;
			fx -= m_pairSize.first * m_pairAnchor.first;
			fy -= m_pairSize.second * m_pairAnchor.second;

			m_rect.left = static_cast<int>(fx + 0.5f);
			m_rect.top = static_cast<int>(fy + 0.5f);
			m_rect.right = static_cast<int>(fx + m_pairSize.first + 0.5f);
			m_rect.bottom = static_cast<int>(fy + m_pairSize.second + 0.5f);
		}
	}
	return m_rect;
}

void CNode::SetParent(CNode* p)
{
	m_pParent = p;
}

CNode* CNode::GetParent()const
{
	return m_pParent;
}

void CNode::ChangeParent(CNode* pNew)
{
	if (pNew == m_pParent)
		return;
	if (!m_pParent)
	{
		pNew->AddChild(this);
	}
	else
	{
		m_pParent->RemoveChild(this, false);
		pNew->AddChild(this);
	}
}

void CNode::SetAnchor(float x, float y)
{
	m_pairAnchor.first = x;
	m_pairAnchor.second = y;
	NeedUpdate(UpdateFlagReLocation);
}

NodePair CNode::GetAnchor()const
{
	return m_pairAnchor;
}

void CNode::SetSize(float w, float h)
{
	if (w<m_pairMinSize.first || w>m_pairMaxSize.first || h<m_pairMinSize.second || h>m_pairMaxSize.second)
		return;
	m_pairSize.first = w;
	m_pairSize.second = h;
	NeedUpdate(UpdateFlagReSize);
}

void CNode::SetSize(int w, int h)
{
	SetSize(1.0f*w, 1.0f*h);
}

const NodePair& CNode::GetSize()const
{
	return m_pairSize;
}

void CNode::SetMinSize(float x, float y)
{
	if (x < 1.0f || y < 1.0f || x>m_pairMaxSize.first || y>m_pairMaxSize.second)
		return;
	if (x == m_pairMinSize.first || y == m_pairMinSize.second)
		return;
	m_pairMinSize.first = x;
	m_pairMinSize.second = y;

	bool b1 = m_pairSize.first < x;
	bool b2 = m_pairSize.second < y;
	if (b1 && b2)
	{
		SetSize(x, y);
	}
	else if (b1)
	{
		SetSize(x, m_pairSize.second);
	}
	else if (b2)
	{
		SetSize(m_pairSize.second, y);
	}

	auto p = GetParent();
	if (p)
		p->NeedUpdate(UpdateFlagReLayout);
}

void CNode::SetMaxSize(float x, float y)
{
	if (x < 1.0f || y < 1.0f || x < m_pairMinSize.first || y < m_pairMinSize.second)
		return;
	if (x == m_pairMaxSize.first || y == m_pairMaxSize.second)
		return;
	m_pairMaxSize.first = x;
	m_pairMaxSize.second = y;

	if (m_pairSize.first > x && m_pairSize.second > y)
	{
		SetSize(x, y);
	}
	else if (m_pairSize.first > x)
	{
		SetSize(x, m_pairSize.second);
	}
	else if (m_pairSize.second > y)
	{
		SetSize(m_pairSize.second, y);
	}

	auto p = GetParent();
	if (p)
		p->NeedUpdate(UpdateFlagReLayout);
}

const NodePair& CNode::GetMinSize()const
{
	return m_pairMinSize;
}

const NodePair& CNode::GetMaxSize()const
{
	return m_pairMaxSize;
}

void CNode::SetSizePolicy(NodeSizePolicy policy)
{
	if (policy != m_sizePolicy)
	{
		m_sizePolicy = policy;
		auto p = GetParent();
		if (p)
			p->NeedUpdate(UpdateFlagReLayout);
	}
}

NodeSizePolicy CNode::GetSizePolicy()const
{
	return m_sizePolicy;
}

void CNode::SetPos(float px, float py)
{
	m_pairPos.first = px;
	m_pairPos.second = py;
	NeedUpdate(UpdateFlagReLocation);
}

void CNode::SetPos(int x, int y)
{
	SetPos(1.0f*x, 1.0f*y);
}

const NodePair& CNode::GetPos()const
{
	return m_pairSize;
}

void CNode::NeedUpdate(NodeUpdateFlag flag)
{
	if (flag == UpdateFlagReSize)
	{
		m_bNeedUpdateRect = true;

		CNode* pNode;
		for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter)
		{
			pNode = *iter;
			if (pNode)
				pNode->NeedUpdate(flag);
		}
	}
	else if (flag == UpdateFlagReSortchild)
	{
		m_bNeedSortChild = true;
	}
	else if (flag == UpdateFlagClean)
	{
		m_bNeedSortChild = false;
		m_bNeedUpdateRect = false;
	}
}

bool CNode::IsNeedUpdateRect()const
{
	return m_bNeedUpdateRect;
}

bool CNode::AddChild(CNode* pNode)
{
	if (pNode &&
		(pNode->GetParent() == NULL || pNode->GetParent() == this) &&
		std::find(m_Children.begin(), m_Children.end(), pNode) == m_Children.end())
	{
		if (!m_Children.empty() && m_Children.back()->GetZOrder() > pNode->GetZOrder())
		{
			NeedUpdate(UpdateFlagReSortchild);
		}
		m_Children.push_back(pNode);
		pNode->SetParent(this);
		return true;
	}

	return false;
}

bool CNode::AddChild(CNode* pNode, int order)
{
	if (AddChild(pNode))
	{
		pNode->SetZOrder(order);
		return true;
	}
	return false;
}

bool CNode::RemoveChild(CNode* pNode, bool bDelete)
{
	if (pNode && pNode->GetParent() == this)
	{
		auto iter = std::find(m_Children.begin(), m_Children.end(), pNode);
		if (iter != m_Children.end())
		{
			if (m_iFindIndex != -1)
			{
				auto dis = std::distance(m_Children.begin(), iter);
				if (dis <= m_iFindIndex)
					--m_iFindIndex;
			}

			m_Children.erase(iter);
			if (bDelete)
			{
				pNode->Destroy();
				pNode->SetParent(NULL);
				delete pNode;
			}
			else
			{
				pNode->SetParent(NULL);
			}
			return true;
		}
	}
	return false;
}

bool CNode::RemoveChildAll(bool bDelete)
{
	CNode* pNode;

	if (bDelete)
	{
		for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter)
		{
			pNode = *iter;
			if (pNode)
			{
				pNode->Destroy();
				pNode->SetParent(NULL);
				delete pNode;
			}
		}
	}
	else
	{
		for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter)
		{
			pNode = *iter;
			if (pNode)
				pNode->SetParent(NULL);
		}
	}

	m_Children.clear();
	return true;
}

void CNode::SortChild()
{
	if (!m_bNeedSortChild)
	{
		return;
	}

	std::stable_sort(m_Children.begin(), m_Children.end(), [](CNode* one, CNode* two)->bool
	{
		if (one && two)
			return one->GetZOrder() < two->GetZOrder();
		else
			return false;
	});

	m_bNeedSortChild = false;
}

CNode* CNode::GetChildByTag(int tag)
{
	CNode* pNode(NULL);
	for (auto iter = m_Children.rbegin(); iter != m_Children.rend(); ++iter)
	{
		pNode = *iter;
		if (pNode && pNode->GetTag() == tag)
		{
			return pNode;
		}
	}

	return NULL;
}

CNode* CNode::GetChildByFinder(NodeFinder  func)
{
	auto iter = std::find_if(m_Children.begin(), m_Children.end(), func);
	if (iter != m_Children.end())
	{
		return *iter;
	}
	return NULL;
}

CNode* CNode::FindFirstNode()
{
	if (!m_Children.empty())
	{
		m_iFindIndex = 0;
		return m_Children[0];
	}
	return NULL;
}

CNode* CNode::FindNextNode()
{
	if (m_iFindIndex < (int)m_Children.size() - 1)
	{
		++m_iFindIndex;
		return m_Children[m_iFindIndex];
	}

	m_iFindIndex = -1;
	return NULL;
}

bool CNode::Init()
{
	if (!m_bNeedInit)
	{
		return true;
	}

	CNode* pNode;
	for (auto iter = this->m_Children.begin(); iter != m_Children.end(); ++iter)
	{
		pNode = *iter;
		if (pNode)
		{
			pNode->Init();
		}
	}
	SortChild();
	m_bNeedInit = false;
	return true;
}

bool CNode::Destroy()
{
	CNode* pNode;
	for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter)
	{
		pNode = *iter;
		if (pNode)
			pNode->Destroy();
	}

	TRACE("destroy CNode\n");
	return true;
}

void CNode::DrawNode()
{
	CNode* pNode;

	SortChild();
	for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter)
	{
		pNode = *iter;
		if (pNode && pNode->IsVisible())
			pNode->DrawNode();
	}
}

void CNode::RefreshNode()
{
	auto p = GetView();
	if (p)
	{
		p->Refresh();
	}
}

CScene* CNode::GetScene()
{
	//assert(m_pParent != NULL);
	return m_pParent ? m_pParent->GetScene() : NULL;
}

CGDIView* CNode::GetView()
{
	CScene* pScene = GetScene();
	return pScene ? pScene->GetView() : NULL;
}

CDirector* CNode::GetDirector()
{
	CScene* pScene = GetScene();
	return pScene ? pScene->GetDirector() : NULL;
}

const std::wstring& CNode::GetNodeClassName()const
{
	static std::wstring name = L"CNode";
	return name;
}

void CNode::MouseEnter()
{
	TRACE1("MouseEnter:0x%x\n", this);
	m_bMouseIN = true;
}

void CNode::MouseLeave()
{
	TRACE1("MouseLeave:0x%x\n", this);
	m_bMouseIN = false;
}

void CNode::MouseTravel(POINT point, unsigned int flag)
{
	m_pointLast = point;
	if (m_Children.empty())
	{
		return;
	}

	CNode* pNode(NULL);
	auto iter = m_Children.rbegin();
	for (; iter != m_Children.rend(); ++iter)
	{
		pNode = *iter;
		if (pNode && pNode->IsPointINNode(point))
		{
			break;
		}
	}
	if (iter == m_Children.rend())
	{
		pNode = NULL;
	}

	if (pNode != m_pCurrentNode)
	{
		if (m_pCurrentNode)
			m_pCurrentNode->MouseLeave();
		m_pCurrentNode = pNode;
		if (m_pCurrentNode)
			m_pCurrentNode->MouseEnter();
	}
	else if (m_pCurrentNode)
	{
		m_pCurrentNode->MouseTravel(point, flag);
	}
}

void CNode::MouseDown(POINT point, unsigned int flag, bool l)
{
	if (l)
	{
		m_bLBDown = true;
		TRACE1("MouseDown left:0x%x\n", this);
	}
	else
	{
		m_bRBDown = true;
		TRACE1("MouseDown right:0x%x\n", this);
	}

	if (m_pCurrentNode)
	{
		m_pCurrentNode->MouseDown(point, flag, l);
	}
}

void CNode::MouseUp(POINT point, unsigned int flag, bool l)
{
	if (l)
	{
		m_bLBDown = false;
		TRACE1("MouseUp left:0x%x\n", this);
	}
	else
	{
		m_bRBDown = false;
		TRACE1("MouseUp right:0x%x\n", this);
	}

	if (m_pCurrentNode)
	{
		m_pCurrentNode->MouseUp(point, flag, l);
	}
}

bool CNode::IsLBDown()const
{
	return m_bLBDown;
}

bool CNode::IsRBDown()const
{
	return  m_bRBDown;
}

bool CNode::IsMouseIN()const
{
	return m_bMouseIN;
}

bool CNode::IsPointINNode(POINT point)
{
	return PtInRect(&GetRect(), point) != 0;
}

POINT CNode::GetLastPoint()const
{
	return m_pointLast;
}

void CNode::SetLastPoint(POINT p)
{
	m_pointLast = p;
}

CNode* CNode::GetCurrentNode()
{
	return m_pCurrentNode;
}


CHLayout::CHLayout(CNode* parent) :
	CNode(parent),
	m_iMarginLeft(0),
	m_iMarginTop(0),
	m_iMarginRight(0),
	m_iMarginBottom(0),
	m_iSpacing(1),
	m_bNeedReLayout(true)
{
	if (parent)
		m_bUsedAsNode = false;
	else
		m_bUsedAsNode = true;
}

bool CHLayout::AddChild(CNode* pNode)
{
	bool bRes = CNode::AddChild(pNode);
	if (bRes)
	{
		NeedUpdate(UpdateFlagReLayout);
	}
	return bRes;
}

bool CHLayout::RemoveChild(CNode* pNode, bool bDelete)
{
	bool bRes = CNode::RemoveChild(pNode, bDelete);
	if (bRes && !Child().empty())
	{
		NeedUpdate(UpdateFlagReLayout);
	}
	return bRes;
}

void CHLayout::NeedUpdate(NodeUpdateFlag flag)
{
	if (flag == UpdateFlagReLayout)
	{
		m_bNeedReLayout = true;
	}
	else if(flag == UpdateFlagReSize)
	{
		m_bNeedReLayout = true;
		CNode::NeedUpdate(flag);
	}
	else if (flag == UpdateFlagReSortchild)
	{
	}
	else
	{
		CNode::NeedUpdate(flag);
	}
}

RECT CHLayout::GetRect()
{
	RECT rect;
	CNode* parent = GetParent();

	if (m_bUsedAsNode || !parent)
	{
		rect = CNode::GetRect();
	}
	else
	{
		rect = parent->GetRect();
		if(IsNeedUpdateRect())
		{
			SetRect(rect);
			NeedUpdate(UpdateFlagClean);
		}
	}
	return rect;
}

void CHLayout::ReLayout()
{
	auto& children = Child();
	const int nodesize = children.size();
	std::vector<float> nodes(nodesize);
	float t;
	int i;
	float content(0);

	auto size = GetSize();
	float w = size.first - m_iMarginLeft - m_iMarginRight - m_iSpacing*(nodesize - 1);
	float h = size.second - m_iMarginTop - m_iMarginBottom;

	if (w < 0) w = 0;
	if (h < 0) h = 0;

	for (i = 0; i < nodesize; ++i)
	{
		t = children[i]->GetSize().first;
		if (t < 0.5f)
			t = 0.5f;
		nodes[i] = t;
		content += t;
	}

	std::list<std::pair<int, float>> expanding, prefered, fixed;
	float sum, sum1;
	float arrange;

	if (content < w)
	{
		for (i = 0; i < nodesize; ++i)
		{
			if (children[i]->GetSizePolicy() == SizePolicyFixed)
				fixed.push_back(std::make_pair(i, children[i]->GetMaxSize().first - nodes[i]));
			else if (children[i]->GetSizePolicy() == SizePolicyPrefered)
				prefered.push_back(std::make_pair(i, children[i]->GetMaxSize().first - nodes[i]));
			else if (children[i]->GetSizePolicy() == SizePolicyExpanding)
				expanding.push_back(std::make_pair(i, children[i]->GetMaxSize().first - nodes[i]));
		}
		std::list<std::pair<int, float>>& group = (!expanding.empty() ? expanding : (!prefered.empty() ? prefered : fixed));
		arrange = w - content;
		while (!group.empty())
		{
			sum = 0;
			for (auto iter = group.begin(); iter != group.end(); ++iter)
			{
				sum += nodes[iter->first];
			}
			sum1 = 0;
			for (auto iter = group.begin(); iter != group.end(); )
			{
				t = nodes[iter->first] / sum*arrange;
				if (t < iter->second)
				{
					nodes[iter->first] += t;
					sum1 += t;
					++iter;
				}
				else
				{
					nodes[iter->first] += iter->second;
					sum1 += iter->second;
					iter = group.erase(iter);
				}
			}
			arrange -= sum1;
			if (arrange < 0.5f)
				break;
		}
	}
	else if (content > w)
	{
		for (i = 0; i < nodesize; ++i)
		{
			if (children[i]->GetSizePolicy() == SizePolicyFixed)
				fixed.push_back(std::make_pair(i, nodes[i] - children[i]->GetMinSize().first));
			else if (children[i]->GetSizePolicy() == SizePolicyPrefered)
				prefered.push_back(std::make_pair(i, nodes[i] - children[i]->GetMinSize().first));
			else if (children[i]->GetSizePolicy() == SizePolicyExpanding)
				expanding.push_back(std::make_pair(i, nodes[i] - children[i]->GetMinSize().first));
		}
		std::list<std::pair<int, float>>& group = (!expanding.empty() ? expanding : (!prefered.empty() ? prefered : fixed));
		arrange = content - w;
		while (!group.empty())
		{
			sum = 0;
			for (auto iter = group.begin(); iter != group.end(); ++iter)
			{
				sum += nodes[iter->first];
			}
			sum1 = 0;
			for (auto iter = group.begin(); iter != group.end(); )
			{
				t = nodes[iter->first] / sum*arrange;
				if (t < iter->second)
				{
					nodes[iter->first] -= t;
					sum1 += t;
					++iter;
				}
				else
				{
					nodes[iter->first] -= iter->second;
					sum1 += iter->second;
					iter = group.erase(iter);
				}
			}
			arrange -= sum1;
			if (arrange < 0.5f)
				break;
		}
	}

	i = 0;
	CNode* pNode = FindFirstNode();
	float x = (float)m_iMarginLeft;
	float y = (float)m_iMarginTop + h / 2.0f;
	while (pNode)
	{
		pNode -> SetSize(nodes[i], h);
		pNode -> SetPos(x + nodes[i] / 2.0f, y);
		pNode = FindNextNode();
		x += (nodes[i] + m_iSpacing);
		++i;
	}
}

void CHLayout::DrawNode()
{
	GetRect();
	if(m_bNeedReLayout)
	{
		m_bNeedReLayout = false;
		if (!Child().empty())
			ReLayout();
	}
	CNode::DrawNode();
}

void CHLayout::SetContentMargin(int l, int t, int r, int b)
{
	if (l < 0 || t < 0 || r < 0 || b < 0)
		return;
	m_iMarginLeft = l;
	m_iMarginTop = t;
	m_iMarginRight = r;
	m_iMarginBottom = b;
	NeedUpdate(UpdateFlagReLayout);
}

void CHLayout::SetSpacing(int spacing)
{
	if (spacing < 0)
		return;
	m_iSpacing = spacing;
	NeedUpdate(UpdateFlagReLayout);
}

void CVLayout::ReLayout()
{
}

CScene::CScene():
	m_pView(NULL),
	m_pDir(NULL),
	m_bCustomNCHitTest(true)
{
}

CScene* CScene::GetScene()
{
	return this;
}

CGDIView* CScene::GetView()
{
	return m_pView;
}

void CScene::SetView(CGDIView* view)
{
	assert(view != NULL);
	m_pView = view;
	auto rect = view->GetWndRect();

	float w, h;
	w = (float)(rect.right - rect.left);
	h = (float)(rect.bottom - rect.top);
	//SetSize(w, h);
	//SetPos(w / 2.0f, h / 2.0f);
	SetRect(rect);
}

CDirector* CScene::GetDirector()
{
	return m_pDir;
}

void CScene::SetDirector(CDirector* pDir)
{
	assert(pDir != NULL);
	m_pDir = pDir;
}

void CScene::DrawScene()
{
	//TRACE("WM_PAINT\n");
	assert(m_pView != NULL);
	assert(m_pDir != NULL);

	m_pView->ClearBuffer();
	DrawNode();
	m_pView->SwapBuffer();
}

bool CScene::EnableCustomNCHitTest(bool value)
{
	bool oldvalue=m_bCustomNCHitTest;
	m_bCustomNCHitTest=value;
	return oldvalue;
}

LRESULT CScene::MessageProc(UINT message, WPARAM wParam, LPARAM lParam, bool& bProcessed)
{
	if (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST)
	{
		POINT po;
		unsigned int flag;

		po.x = GET_X_LPARAM(lParam);
		po.y = GET_Y_LPARAM(lParam);
		flag = wParam;

		switch (message)
		{
		case WM_MOUSEMOVE:
			MouseTravel(po, flag);
			break;
		case WM_LBUTTONDOWN:
			SetCapture(GetView()->GetWnd());
			MouseDown(po, flag, true);
			break;
		case WM_LBUTTONUP:
			ReleaseCapture();
			MouseUp(po, flag, true);
			break;
		case WM_RBUTTONDOWN:
			SetCapture(GetView()->GetWnd());
			MouseDown(po, flag, false);
			break;
		case WM_RBUTTONUP:
			ReleaseCapture();
			MouseUp(po, flag, false);
			break;
		}
		goto End;
	}

	switch (message)
	{
	case WM_NCHITTEST:
		if(m_bCustomNCHitTest)
		{
			bProcessed = true;
			return GetNCHitTest();
		}
		break;
	case WM_ERASEBKGND:
		bProcessed = true;
		return 1;
		break;
	case WM_PAINT:
		DrawScene();
		break;
	case WM_SIZING:
		break;
	case WM_SIZE:
		// 更新窗口尺寸
		GetView()->WndRectChanged();
		SetView(GetView());
		break;
	case WM_DESTROY:
		Destroy();
		break;
	case WM_NCMOUSEMOVE:
	{
		POINT po;
		po.x = GET_X_LPARAM(lParam);
		po.y = GET_Y_LPARAM(lParam);
		ScreenToClient(GetView()->GetWnd(), &po);
		MouseTravel(po, wParam);
	}
	break;
	}
End:
	GetView()->DoRefresh();
	return 0;
}

CShadowScene::CShadowScene(int shadowSize) :m_iShadowSize(shadowSize)
{
}

void CShadowScene::SetView(CGDIView* view)
{
	//assert(dynamic_cast<CGDIViewAlpha* >(view) != NULL);
	CScene::SetView(view);

	auto rect = view->GetWndRect();
	rect.left += m_iShadowSize;
	rect.top += m_iShadowSize;
	rect.right -= m_iShadowSize;
	rect.bottom -= m_iShadowSize;

	int w = rect.right - rect.left;
	int h = rect.bottom - rect.top;
	SetSize(w, h);
	SetPos(w / 2.0f, h / 2.0f);
	SetRect(rect);
}

void CShadowScene::DrawScene()
{
	ClearScene();
	DrawNode();
	DrawShadow();
	SwapScene();
}

void CShadowScene::ClearScene()
{
	auto pView = GetView();
	auto pData = pView->GetBitmapData();
	auto rect = pView->GetScreenRect();
	memset(pData, 255, (rect.right - rect.left)*(rect.bottom - rect.top) * 4);
}

void CShadowScene::SwapScene()
{
	unsigned char *p1, *p2;
	int j, i;
	int w, h, sw, sh;

	auto pView = GetView();
	auto rect1 = pView->GetScreenRect();
	auto rect2 = pView->GetWndRect();
	auto pData = pView->GetBitmapData();

	sw = rect1.right - rect1.left;
	sh = rect1.bottom - rect1.top;
	w = m_iShadowSize + (int)GetSize().first;
	h = m_iShadowSize + (int)GetSize().second;

	for (j = m_iShadowSize; j < h; ++j)
	{
		p1 = pData + (sh - 1 - j) * sw * 4;
		for (i = m_iShadowSize; i < w; ++i)
		{
			p2 = p1 + i * 4;
			p2[3] = 255;
		}
	}

	pView->SwapBuffer();
}

void CShadowScene::DrawShadow()
{
	unsigned char *p1, *p2;
	int j, i;
	int w, h, sw, sh;
	unsigned char a[4] = { 0 };

	auto pView = GetView();
	auto rect1 = pView->GetScreenRect();
	auto rect2 = pView->GetWndRect();
	auto pData = pView->GetBitmapData();

	sw = rect1.right - rect1.left;
	sh = rect1.bottom - rect1.top;
	w = rect2.right - rect2.left;
	h = rect2.bottom - rect2.top;

	for (j = 0; j < m_iShadowSize; ++j)
	{
		a[3] = (unsigned char)(10 + j * 100 / m_iShadowSize);

		p1 = pData + (sh - 1 - j)*sw * 4;
		p2 = pData + (sh - h + j)*sw * 4;
		for (i = j; i < w - j; ++i)
		{
			// 上
			memcpy(p1 + i * 4, a, 4);
			// 下
			memcpy(p2 + i * 4, a, 4);
		}

		for (i = j; i < h - j; ++i)
		{
			p1 = pData + (sh - 1 - i)*sw * 4;
			// 左
			memcpy(p1 + j * 4, a, 4);
			// 右
			memcpy(p1 + (w - 1 - j) * 4, a, 4);
		}
	}
}

CDirector::CDirector(CGDIView* view) :m_pCurrentView(view),
m_pCurrentScene(NULL)
{
}

CDirector::~CDirector()
{
	if (m_pCurrentScene)
	{
		delete m_pCurrentScene;
	}

	if (m_pCurrentView)
	{
		m_pCurrentView->Clear();
		delete m_pCurrentView;
	}
}

void CDirector::RunScene(CScene* scene)
{
	if (!scene)
	{
		assert(0 && "scene pointer is null");
		return;
	}

	scene->SetView(m_pCurrentView);
	scene->SetDirector(this);
	if (scene->Init())
	{
		m_pCurrentScene = scene;
	}
	else
	{
		assert(0 && "failed to init scene");
		delete scene;
	}
}

LRESULT CDirector::MessageProc(UINT message, WPARAM wParam, LPARAM lParam, bool& bProcessed)
{
	//assert(m_pCurrentScene != NULL);
	auto res = m_pCurrentScene->MessageProc(message, wParam, lParam, bProcessed);

	return res;
}

CImageLayer::CImageLayer(CNode* pParent) :CNode(pParent),
m_pData(NULL),
m_hDc(NULL),
m_hBitmap(NULL)
{
}

CImageLayer::~CImageLayer()
{
	if (m_hBitmap)
	{
		DeleteObject(m_hBitmap);
	}
	if (m_hDc)
	{
		DeleteDC(m_hDc);
	}
}

bool CImageLayer::CreateImageLayerByFile(const std::wstring& sFileName)
{
	std::unique_ptr<Gdiplus::Bitmap> pBitmap(Gdiplus::Bitmap::FromFile(sFileName.c_str()));
	if (pBitmap->GetLastStatus() != Gdiplus::Ok)
	{
		return false;
	}

	Gdiplus::PixelFormat format = pBitmap->GetPixelFormat();
	UINT size = Gdiplus::GetPixelFormatSize(format);
	Gdiplus::Rect rect(0, 0, pBitmap->GetWidth(), pBitmap->GetHeight());
	Gdiplus::BitmapData data;
	bool bRes;

	if (size < 24)
	{
		format = PixelFormat24bppRGB;
		size = 24;
	}
	pBitmap->LockBits(&rect, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite, format, &data);
	if (data.Stride > 0)
	{
		// data.Scan0指向图像左上角,颠倒过来
		std::unique_ptr<unsigned char[]> pLine(new unsigned char[data.Stride]);
		unsigned char *p1, *p2;
		for (unsigned int i = 0; i < data.Height / 2; ++i)
		{
			p1 = (unsigned char*)data.Scan0 + data.Stride*i;
			p2 = (unsigned char*)data.Scan0 + data.Stride*(data.Height - 1 - i);
			memcpy(pLine.get(), p1, data.Stride);
			memcpy(p1, p2, data.Stride);
			memcpy(p2, pLine.get(), data.Stride);
		}
	}
	bRes = CreateImageLayerByData((unsigned char*)data.Scan0, pBitmap->GetWidth(), pBitmap->GetHeight(), size);
	pBitmap->UnlockBits(&data);

	return bRes;
}

bool CImageLayer::CreateImageLayerByData(unsigned char* pData, int w, int h, int bitcount, bool bUseImageSizeAsNodeSize)
{
	assert(pData != NULL && h > 0 && w > 0 && bitcount >= 24);

	BITMAPINFO bi;
	int stride;
	HDC hRealDC;

	if (GetScene() == NULL)
	{
		hRealDC = GetDC(NULL);
	}
	else
	{
		hRealDC = GetView()->GetRealDC();
	}

	stride = w * bitcount / 8;
	if (stride % 4 != 0)
	{
		stride = stride / 4 * 4 + 4;
	}
	memset(&m_Info, 0, sizeof(m_Info));
	m_Info.biSize = sizeof(BITMAPINFOHEADER);
	m_Info.biPlanes = 1;
	m_Info.biHeight = h;
	m_Info.biWidth = w;
	m_Info.biBitCount = bitcount;
	m_Info.biCompression = BI_RGB;
	m_Info.biSizeImage = h*stride;
	bi.bmiHeader = m_Info;

	if (m_hBitmap)
	{
		DeleteObject(m_hBitmap);
	}
	//m_hBitmap=::CreateDIBitmap(hRealDC,
	//	&bi,
	//	CBM_INIT,
	//	pData,
	//	&bi,
	//	DIB_RGB_COLORS);
	m_hBitmap = CreateDIBSection(hRealDC,
		&bi,
		DIB_RGB_COLORS,
		(void**)&m_pData,
		NULL,
		0);
	memcpy(m_pData, pData, m_Info.biSizeImage);

	if (!m_hDc)
	{
		m_hDc = CreateCompatibleDC(hRealDC);
	}
	SelectObject(m_hDc, m_hBitmap);

	if (bUseImageSizeAsNodeSize)
	{
		SetSize(w, h);
	}

	if (GetScene() == NULL)
	{
		ReleaseDC(NULL, hRealDC);
	}

	return true;
}

bool CImageLayer::CreateImageLayerByColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	unsigned char data[16 * 16 * 4];
	unsigned char* p;
	for (int i = 0; i < 16 * 16; ++i)
	{
		p = data + i * 4;
		*p = b;
		*(p + 1) = g;
		*(p + 2) = r;
		*(p + 3) = a;
	}

	return CreateImageLayerByData(data, 16, 16, 32);
}

void CImageLayer::DrawImage(int dest_leftup_x, int dest_leftup_y, int dest_w, int dest_h, unsigned char opacity)
{
	if (!m_hBitmap)
	{
		return;
	}
	HDC hMemDC = GetView()->GetMemDC();

	if (255 != opacity || m_Info.biBitCount == 32)
	{
		BLENDFUNCTION ftn = { 0 };
		if (m_Info.biBitCount == 32)
		{
			ftn.BlendOp = AC_SRC_OVER;
			ftn.AlphaFormat = AC_SRC_ALPHA;
			ftn.BlendFlags = 0; // must be zero
			ftn.SourceConstantAlpha = 255; // must be 255
		}
		else
		{
			ftn.BlendOp = AC_SRC_OVER;
			ftn.AlphaFormat = 0; // not AC_SRC_ALPHA
			ftn.BlendFlags = 0; // must be zero
			ftn.SourceConstantAlpha = opacity;
		}
		AlphaBlend(hMemDC,
			dest_leftup_x, dest_leftup_y, dest_w, dest_h,
			m_hDc, 0, 0, m_Info.biWidth, m_Info.biHeight, ftn);
	}
	else
	{
		if (dest_h != m_Info.biWidth || dest_w != m_Info.biHeight)
		{
			StretchBlt(hMemDC,
				dest_leftup_x, dest_leftup_y, dest_w, dest_h,
				m_hDc, 0, 0, m_Info.biWidth, m_Info.biHeight, SRCCOPY);
		}
		else
		{
			BitBlt(hMemDC,
				dest_leftup_x, dest_leftup_y, dest_w, dest_h,
				m_hDc, 0, 0, SRCCOPY);
		}
	}
}

void CImageLayer::DrawNode()
{
	RECT r = GetRect();
	DrawImage(r.left, r.top, r.right - r.left, r.bottom - r.top);
}

const std::wstring& CImageLayer::GetNodeClassName()const
{
	static std::wstring name = L"CImageLayer";
	return name;
}

CTextLayer::CTextLayer(CNode* pParent) :
	CNode(pParent),
	m_hFont(NULL),
	m_dwColor(RGB(1, 1, 1)),
	m_dwAlignment(DT_CENTER | DT_SINGLELINE | DT_VCENTER)
{
	m_hFont = CreateFont(20, 0, 0, 0, FW_THIN, FALSE, FALSE, FALSE,
		GB2312_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY,
		FF_MODERN,
		L"微软雅黑");
}

CTextLayer::~CTextLayer()
{
	if (m_hFont)
	{
		DeleteObject(m_hFont);
	}
}

void CTextLayer::SetFont(const LOGFONT& f)
{
	if (m_hFont)
	{
		DeleteObject(m_hFont);
	}
	m_hFont = CreateFont(f.lfHeight,
		f.lfWidth,
		f.lfEscapement,
		f.lfOrientation,
		f.lfWeight,
		f.lfItalic,
		f.lfUnderline,
		f.lfStrikeOut,
		f.lfCharSet,
		f.lfOutPrecision,
		f.lfClipPrecision,
		f.lfQuality,
		f.lfPitchAndFamily,
		f.lfFaceName);
}

void CTextLayer::SetFont(HFONT h)
{
	if (h && h != m_hFont)
	{
		DeleteObject(m_hFont);
		m_hFont = h;
	}
}

HFONT CTextLayer::GetFont()
{
	return m_hFont;
}

void CTextLayer::SetTextColor(COLORREF color)
{
	if (color == RGB(0, 0, 0))
	{
		color = RGB(1, 1, 1);
	}
	m_dwColor = color;
}

COLORREF CTextLayer::GetTextColor()const
{
	return m_dwColor;
}

void CTextLayer::SetAlignment(DWORD align)
{
	m_dwAlignment = align;
}

DWORD CTextLayer::GetAlignment()const
{
	return m_dwAlignment;
}

void CTextLayer::SetText(const std::wstring& sText, bool bUseTextSizeAsNodeSize)
{
	m_szText = sText;
	if (bUseTextSizeAsNodeSize)
	{
		SIZE size = GetTextSize();
		SetSize(size.cx, size.cy);
	}
}

const std::wstring& CTextLayer::GetText()const
{
	return m_szText;
}

SIZE CTextLayer::GetTextSize()
{
	SIZE size;
	HDC hMemDC = NULL;

	if (GetScene())
	{
		hMemDC = GetView()->GetMemDC();
	}
	else
	{
		hMemDC = GetDC(NULL);
	}

	SelectObject(hMemDC, m_hFont);
	GetTextExtentPoint32(hMemDC, m_szText.c_str(), m_szText.length(), &size);

	if (!GetScene())
	{
		int res = ReleaseDC(NULL, hMemDC);
		TRACE1("ReleaseDC res:%d", res);
	}

	return size;
}

void CTextLayer::DrawNode()
{
	HDC hMemDC = GetView()->GetMemDC();
	RECT r = GetRect();
	::SetTextColor(hMemDC, m_dwColor);
	SelectObject(hMemDC, m_hFont);

	if (!m_szText.empty())
	{
		DrawText(hMemDC,
			m_szText.c_str(),
			m_szText.length(),
			&r,
			m_dwAlignment);

		//BYTE rr=GetRValue(m_dwColor);
		//BYTE gg=GetGValue(m_dwColor);
		//BYTE bb=GetBValue(m_dwColor);
		//Gdiplus::Graphics g(hMemDC);
		//g.DrawString(m_szText.c_str(),
		//	m_szText.length(),
		//	&Gdiplus::Font(hMemDC),
		//	Gdiplus::RectF(r.left, r.top, r.right-r.left, r.bottom-r.top),
		//	&Gdiplus::StringFormat(),
		//	&Gdiplus::SolidBrush(Gdiplus::Color(255, rr, gg, bb)));
	}
}

CStaticImageNode::CStaticImageNode(int showType, CNode* pParent) :CNode(pParent),
m_presentType((PresentType)showType),
m_sw(1.0f),
m_sh(1.0f)
{
}

void CStaticImageNode::SetImageLayer(CImageLayer* pImage)
{
	CNode* pNode = GetChildByFinder([pImage](CNode* pNode)
	{
		return pNode->GetNodeClassName() == pImage->GetNodeClassName();
	});

	if (pNode != NULL)
	{
		// 删除前一张图片
		if (pNode != pImage)
			RemoveChild(pNode, true);
		else
			pNode->Init();
	}
	else
	{
		AddChild(pImage);
		pImage->Init();
	}

	auto size = GetSize();
	m_sw = pImage->GetSize().first / size.first;
	m_sh = pImage->GetSize().second / size.second;
	if (m_presentType == PresentCenter)
	{
		m_sw = m_sw > m_sh ? (1.0f > m_sw ? 1.0f : m_sw) : (1.0f > m_sh ? 1.0f : m_sh);
		m_sh = m_sw;
		pImage->SetSize(pImage->GetSize().first / m_sh, pImage->GetSize().second / m_sh);
	}
	else if (m_presentType == PresentFillNode)
	{
		pImage->SetSize(size.first, size.second);
	}
	pImage->SetPos(size.first / 2, size.second / 2);
	pImage->SetTag((int)this);
}

CImageLayer* CStaticImageNode::GetImageLayer()
{
	return dynamic_cast<CImageLayer*>(GetChildByTag((int)this));
}

CButtonNode::CButtonNode(CNode* pParent) :CTextLayer(pParent)
{
	m_bgNormal = Gdiplus::Color(255, 220, 220, 220);
	m_bgHighLight = Gdiplus::Color(255, 240, 240, 240);
	m_iBorderWidth = 1;
	m_borderNormal = Gdiplus::Color(255, 50, 50, 50);
	m_borderHighLight = Gdiplus::Color(255, 80, 80, 80);
}

void CButtonNode::SetCallback(ButtonCallback ck)
{
	m_callback = ck;
}

void CButtonNode::SetBgColor(const Gdiplus::Color& normal, const Gdiplus::Color& highlight)
{
	m_bgNormal = normal;
	m_bgHighLight = highlight;
}

void CButtonNode::SetBorderColor(const Gdiplus::Color& normal, const Gdiplus::Color& highlight)
{
	m_borderNormal = normal;
	m_borderHighLight = highlight;
}

void CButtonNode::SetBorderWidth(int width)
{
	m_iBorderWidth = width;
}

void CButtonNode::DrawNode()
{
	RECT r = GetRect();
	HDC hMemDC = GetView()->GetMemDC();
	Gdiplus::Graphics g(hMemDC);
	Gdiplus::SolidBrush brush(IsMouseIN() ? m_bgHighLight : m_bgNormal);
	Gdiplus::Pen pen((IsMouseIN() ? m_borderHighLight : m_borderNormal), (float)m_iBorderWidth);

	g.FillRectangle(&brush, r.left, r.top, r.right - r.left, r.bottom - r.top);
	CTextLayer::DrawNode();
	g.DrawRectangle(&pen, r.left, r.top, r.right - r.left, r.bottom - r.top);
}

void CButtonNode::MouseEnter()
{
	RefreshNode();
	CNode::MouseEnter();
}

void CButtonNode::MouseLeave()
{
	RefreshNode();
	CNode::MouseLeave();
}

void CButtonNode::MouseUp(POINT point, unsigned int flag, bool l)
{
	if (l && IsMouseIN() && m_callback)
	{
		m_callback(this);
	}
	CNode::MouseUp(point, flag, l);
}