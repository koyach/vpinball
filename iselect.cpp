#include "StdAfx.h"

// this table must be kept in sync with the ISelectable ItemTypeEnum table
int rgTypeStringIndex[] = {
	IDS_TB_WALL, //eItemSurface,
	IDS_TB_FLIPPER, //eItemFlipper,
	IDS_TB_TIMER, //eItemTimer,
	IDS_TB_PLUNGER, //eItemPlunger,
	IDS_TB_TEXTBOX, //eItemTextbox,
	IDS_TB_BUMPER, //eItemBumper,
	IDS_TB_TRIGGER, //eItemTrigger,
	IDS_TB_LIGHT, //eItemLight,
	IDS_TB_KICKER, //eItemKicker,
	IDS_TB_DECAL, //eItemDecal,
	IDS_TB_GATE, //eItemGate,
	IDS_TB_SPINNER, //eItemSpinner,
	IDS_TB_RAMP, //eItemRamp,
	IDS_TABLE, //eItemTable,
	IDS_TB_LIGHT, //eItemLightCenter,
	IDS_CONTROLPOINT, //eItemDragPoint,
	IDS_COLLECTION, //eItemCollection,
   IDS_TB_DISPREEL, //eItemDispReel,
	IDS_TB_LIGHTSEQ, //eItemLightSeq,
	IDS_TB_PRIMITIVE, //eItemPrimitive	
   IDS_TB_FLASHER, //eItemFlasher
	IDS_TB_LIGHTSEQ, //light seq center
	IDS_TB_COMCONTROL, //eItemComControl
    };

ISelect::ISelect()
	{
	m_fDragging = fFalse;
	m_fMarkedForUndo = fFalse;
	m_selectstate = eNotSelected;

	m_fLocked = fFalse;

	m_menuid = -1;
   layerIndex=0;
	}

void ISelect::SetObjectPos()
	{
	g_pvp->ClearObjectPosCur();
	}

void ISelect::OnLButtonDown(int x, int y)
	{
	m_fDragging = fTrue;
	m_fMarkedForUndo = fFalse; // So we will be marked when and if we are dragged
	m_ptLast.x = x;
	m_ptLast.y = y;

	SetCapture(GetPTable()->m_hwnd);

	SetObjectPos();
	}

void ISelect::OnLButtonUp(int x, int y)
	{
	m_fDragging = fFalse;

	ReleaseCapture();

	if (m_fMarkedForUndo)
		{
		m_fMarkedForUndo = fFalse;
		GetIEditable()->EndUndo();
		}
	}

void ISelect::OnRButtonDown(int x, int y, HWND hwnd)
	{
	}

void ISelect::OnRButtonUp(int x, int y)
	{
	}

void ISelect::OnMouseMove(int x, int y)
	{
	PinTable * const ptable = GetPTable();
	const float inv_zoom = 1.0f/ptable->m_zoom;

	if ((x == m_ptLast.x) && (y == m_ptLast.y))
		{
		return;
		}

	if (m_fDragging && !GetIEditable()->GetISelect()->m_fLocked) // For drag points, follow the lock of the parent
		{
		if (!m_fMarkedForUndo)
			{
			m_fMarkedForUndo = fTrue;
			GetIEditable()->BeginUndo();
			GetIEditable()->MarkForUndo();
			}
		MoveOffset((x - m_ptLast.x)*inv_zoom, (y - m_ptLast.y)*inv_zoom);
		m_ptLast.x = x;
		m_ptLast.y = y;
		SetObjectPos();
		}
	}

void ISelect::MoveOffset(const float dx, const float dy)
	{
	// Implement in child class to enable dragging
	}

void ISelect::EditMenu(HMENU hmenu)
	{
	}

void ISelect::DoCommand(int icmd, int x, int y)
{
    IEditable *piedit = GetIEditable();

    if ( (icmd & 0x0000FFFF) == ID_SELECT_ELEMENT )
    {
       const int ksshift = GetKeyState(VK_SHIFT);
       const int ksctrl = GetKeyState(VK_CONTROL);

       PinTable *currentTable = GetPTable();
       int i = (icmd & 0x00FF0000)>>16;
       ISelect * const pisel = currentTable->m_allHitElements.ElementAt(i);

       const BOOL fAdd = ((ksshift & 0x80000000) != 0);

       if (pisel == (ISelect *)currentTable && fAdd)
       {
          // Can not include the table in multi-select
          // and table will not be unselected, because the
          // user might be drawing a box around other objects
          // to add them to the selection group
          currentTable->OnLButtonDown(x,y); // Start the band select
          return;
       }

       currentTable->AddMultiSel(pisel, fAdd, fTrue);
       return;
    }
    if ( ((icmd & 0x000FFFFF) >= 0x40000 ) && ((icmd & 0x000FFFFF)<0x40020) ) 
    {
        const int ksshift = GetKeyState(VK_SHIFT);
        const int ksctrl = GetKeyState(VK_CONTROL);

        PinTable *currentTable = GetPTable();
        int i = icmd & 0x000000FF;
        currentTable->AddToCollection(i);
    }
	switch (icmd)
    {
		case ID_EDIT_DRAWINGORDER_HIT:
			g_pvp->ShowDrawingOrderDialog(false);
			break;
		case ID_EDIT_DRAWINGORDER_SELECT:
			g_pvp->ShowDrawingOrderDialog(true);
			break;
		case ID_DRAWINFRONT:
			GetPTable()->m_vedit.RemoveElement(piedit);
			GetPTable()->m_vedit.AddElement(piedit);
			GetPTable()->m_layer[ layerIndex ].RemoveElement(piedit);
			GetPTable()->m_layer[ layerIndex ].AddElement(piedit);
			GetPTable()->SetDirtyDraw();
			break;
		case ID_DRAWINBACK:
			GetPTable()->m_vedit.RemoveElement(piedit);
			GetPTable()->m_vedit.InsertElementAt(piedit, 0);
			GetPTable()->m_layer[ layerIndex ].RemoveElement(piedit);
			GetPTable()->m_layer[ layerIndex ].InsertElementAt(piedit,0);
			GetPTable()->SetDirtyDraw();
			break;
		case ID_SETASDEFAULT:
			piedit->WriteRegDefaults();
			break;
		case ID_LOCK:
			GetIEditable()->BeginUndo();
			GetIEditable()->MarkForUndo();
			m_fLocked = !m_fLocked;
			GetIEditable()->EndUndo();
			GetPTable()->SetDirtyDraw();
			break;
      case ID_ASSIGNTO_LAYER1:
         {
            GetPTable()->AssignToLayer(piedit, 0);
            break;
         }
      case ID_ASSIGNTO_LAYER2:
         {
            GetPTable()->AssignToLayer(piedit, 1);
            break;
         }
      case ID_ASSIGNTO_LAYER3:
         {
            GetPTable()->AssignToLayer(piedit, 2);
            break;
         }
      case ID_ASSIGNTO_LAYER4:
         {
            GetPTable()->AssignToLayer(piedit, 3);
            break;
         }
      case ID_ASSIGNTO_LAYER5:
         {
            GetPTable()->AssignToLayer(piedit, 4);
            break;
         }
      case ID_ASSIGNTO_LAYER6:
         {
            GetPTable()->AssignToLayer(piedit, 5);
            break;
         }
      case ID_ASSIGNTO_LAYER7:
         {
            GetPTable()->AssignToLayer(piedit, 6);
            break;
         }
      case ID_ASSIGNTO_LAYER8:
         {
            GetPTable()->AssignToLayer(piedit, 7);
            break;
         }
		/*default:
			psel->DoCommand(command, x, y);
			break;*/
    }
}

#define COLOR_LOCKED RGB(160,160,160)
//GetSysColor(COLOR_GRAYTEXT)

void ISelect::SetSelectFormat(Sur *psur)
	{
	//psur->SetFillColor(RGB(150,200,255));
	//psur->SetFillColor(RGB(128,0,0));

	DWORD color;

	if (m_fLocked)
		{
		//color = GetSysColor(COLOR_GRAYTEXT);
		color = COLOR_LOCKED;
		}
	else
		{
		color = GetSysColor(COLOR_HIGHLIGHT);
		}

	psur->SetBorderColor(color, false, 4);
	psur->SetLineColor(color, false, 4);
	}

void ISelect::SetMultiSelectFormat(Sur *psur)
	{
	DWORD color;

	if (m_fLocked)
		{
		color = COLOR_LOCKED;
		}
	else
		{
		color = GetSysColor(COLOR_HIGHLIGHT);
		}

	psur->SetBorderColor(color, false, 3);
	psur->SetLineColor(color, false, 3);
	}

void ISelect::SetLockedFormat(Sur *psur)
	{
	psur->SetBorderColor(COLOR_LOCKED, false, 1);
	psur->SetLineColor(COLOR_LOCKED, false, 1);
	}

void ISelect::FlipY(Vertex2D * const pvCenter)
	{
	GetIEditable()->MarkForUndo();

	Vertex2D vCenter;
	GetCenter(&vCenter);
	const float delta = vCenter.y - pvCenter->y;
	vCenter.y -= delta*2;
	PutCenter(&vCenter);
	}

void ISelect::FlipX(Vertex2D * const pvCenter)
	{
	GetIEditable()->MarkForUndo();

	Vertex2D vCenter;
	GetCenter(&vCenter);
	const float delta = vCenter.x - pvCenter->x;
	vCenter.x -= delta*2;
	PutCenter(&vCenter);
	}

void ISelect::Rotate(float ang, Vertex2D *pvCenter)
	{
	GetIEditable()->MarkForUndo();

	Vertex2D vCenter;
	GetCenter(&vCenter);

	const float sn = sinf(ANGTORAD(ang));
	const float cs = cosf(ANGTORAD(ang));

	const float dx = vCenter.x - pvCenter->x;
	const float dy = vCenter.y - pvCenter->y;

	vCenter.x = pvCenter->x + cs*dx - sn*dy;
	vCenter.y = pvCenter->y + cs*dy + sn*dx;
	PutCenter(&vCenter);
	}

void ISelect::Scale(float scalex, float scaley, Vertex2D *pvCenter)
	{
	GetIEditable()->MarkForUndo();

	Vertex2D vCenter;
	GetCenter(&vCenter);

	const float dx = vCenter.x - pvCenter->x;
	const float dy = vCenter.y - pvCenter->y;

	vCenter.x = pvCenter->x + dx*scalex;
	vCenter.y = pvCenter->y + dy*scaley;
	PutCenter(&vCenter);
	}

void ISelect::Translate(Vertex2D *pvOffset)
	{
	GetIEditable()->MarkForUndo();

	Vertex2D vCenter;
	GetCenter(&vCenter);

	vCenter.x += pvOffset->x;
	vCenter.y += pvOffset->y;
	PutCenter(&vCenter);
	}

HRESULT ISelect::GetTypeName(BSTR *pVal)
	{
	WCHAR wzName[128];

	LocalString ls(rgTypeStringIndex[GetItemType()]);

	MultiByteToWideChar(CP_ACP, 0, ls.m_szbuffer, -1, wzName, 128);

	*pVal = SysAllocString(wzName);

	return S_OK;
	}
BOOL ISelect::LoadToken(int id, BiffReader *pbr)
	{
	if (id == FID(LOCK))
		{
		pbr->GetBool(&m_fLocked);
		}
   if(id==FID(LAYR))
   {
      pbr->GetInt(&layerIndex);
   }
	return fTrue;
	}

HRESULT ISelect::SaveData(IStream *pstm, HCRYPTHASH hcrypthash, HCRYPTKEY hcryptkey)
	{
	BiffWriter bw(pstm, hcrypthash, hcryptkey);

	bw.WriteBool(FID(LOCK), m_fLocked);
   bw.WriteInt(FID(LAYR), layerIndex );
	return S_OK;
	}
