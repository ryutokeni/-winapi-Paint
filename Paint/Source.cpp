#include <windows.h>
#include <windowsx.h>
#include <string>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <queue>
#include <CommCtrl.h>
#include "resource.h"
#pragma comment(lib, "comctl32.lib")
using namespace std;


struct Shape //struct hình dùng để lưu lại thông tin của hình đã vẽ
{
	int type; //0 ligne, 1 rectangle, 2 elipse, 3 carre
	POINT First, Last;
	int thickness;
	COLORREF bColor = NULL;
	COLORREF fColor = NULL;
};

HWND hWndBorderToolBar; //border toolbar
HWND hWndFillToolBar; //fill toolbar
static TCHAR szWindowClass[] = _T("win32app");
static TCHAR szTitle[] = _T("Paint");
static bool moved = false;
static POINT ptFirst; //điểm đầu
static POINT ptLast; //điểm cuối
static queue<Shape> List; //Hàng đợi lưu danh sách các hình đã vẽ
static int shape; // 0: ligne, 1: rectangle, 2: elipse, 3: carre
static COLORREF bColor = NULL; //màu của border
static COLORREF fColor = NULL; //màu nền
static bool bShow = false; //kiểm tra xem border toolbar có đang hiện ko
static bool fShow = false; //kiểm tra xem fill toolbar có đang hiện ko
static int thickness = 1; //độ dày của border
HINSTANCE hInst;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDR_MENU2);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

	if (!RegisterClassEx(&wcex))
	{
		MessageBox(NULL,
			_T("Call to RegisterClassEx failed!"),
			_T("Win32 Guided Tour"),
			NULL);

		return 1;
	}

	hInst = hInstance;
	HWND hWnd = CreateWindow(
		szWindowClass,
		szTitle,
		WS_OVERLAPPEDWINDOW&~WS_MAXIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT,
		500, 500,
		NULL,
		NULL,
		hInstance,
		NULL
		);

	if (!hWnd)
	{
		MessageBox(NULL,
			_T("Call to CreateWindow failed!"),
			_T("Win32 Guided Tour"),
			NULL);

		return 1;
	}

	ShowWindow(hWnd,
		nCmdShow);
	UpdateWindow(hWnd);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}


int Min(int a, int b)
{
	if (a > b)
		return b;
	return a;
}

//Hàm lưu hình đã vẽ
void saveShape(int type, POINT First, POINT Last, COLORREF bColor, COLORREF fColor, int thickness) 
{
	Shape newShape;
	newShape.First = First;
	newShape.Last = Last;
	newShape.type = type;
	newShape.bColor = bColor;
	newShape.fColor = fColor;
	newShape.thickness = thickness;
	List.push(newShape);
}

HPEN Pen;

//hàm in lại các hình đã vẽ
void loadShape(HDC hdc)
{
	
	queue<Shape> tempList = List;
	while (!tempList.empty())
	{
		HPEN hPen;
		Shape temp = tempList.front();
		tempList.pop();
		//chuẩn bị brush phù hợp
		if (temp.fColor == NULL)
			SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
		else
		{
			SelectObject(hdc, GetStockObject(DC_BRUSH));
			SetDCBrushColor(hdc, temp.fColor);
		}

		//chuẩn bị pen phù hợp
		if (temp.bColor == NULL)
			hPen = CreatePen(PS_SOLID, temp.thickness, RGB(0, 0, 0));
		else
		{
			hPen = CreatePen(PS_SOLID, temp.thickness, temp.bColor);
			SelectObject(hdc, hPen);
		}

		//vẽ lại hình
		switch (temp.type)
		{
		case 0:
			MoveToEx(hdc, temp.First.x, temp.First.y, NULL);
			LineTo(hdc, temp.Last.x, temp.Last.y);
			break;
		case 1:
			MoveToEx(hdc, temp.First.x, temp.First.y, NULL);
			Rectangle(hdc, temp.First.x, temp.First.y, temp.Last.x, temp.Last.y);
			break;
		case 2:
			MoveToEx(hdc, temp.First.x, temp.First.y, NULL);
			Ellipse(hdc, temp.First.x, temp.First.y, temp.Last.x, temp.Last.y);
			break;
		case 3:
			MoveToEx(hdc, temp.First.x, temp.First.y, NULL);
			int t = Min((temp.Last.x - temp.First.x), (temp.Last.y - temp.First.y));
			if (temp.First.x <= temp.Last.x)
			{
				if (temp.First.y <= temp.Last.y)
					Rectangle(hdc, temp.First.x, temp.First.y, temp.First.x + t, temp.First.y + t);
				else
					Rectangle(hdc, temp.First.x, temp.First.y + t, temp.First.x - t, temp.First.y);
			}
			else
			{
				if (temp.First.y <= temp.Last.y)
					Rectangle(hdc, temp.First.x, temp.First.y - t, temp.First.x + t, temp.First.y);
				else
					Rectangle(hdc, temp.First.x, temp.First.y, temp.First.x + t, temp.First.y + t);
			}
			break;
		}
		DeleteObject(hPen); // xóa pen đã tạo, brush dùng GetStockObject nên không cần xóa
	}
}

//hàm chuẩn bị pen và brush để thực hiện thao tác vẽ
void checkOption(HDC hdc)
{
	if (!bShow)
		Pen = CreatePen(PS_SOLID, thickness, RGB(0, 0, 0));
	else
		Pen = CreatePen(PS_SOLID, thickness, bColor);
	SelectObject(hdc, Pen);

	SelectObject(hdc, GetStockObject(DC_BRUSH));
	if (!fShow)
		SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
	else
		SetDCBrushColor(hdc, fColor);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int x, y;
	PAINTSTRUCT ps;
	HDC hdc;
	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		loadShape(hdc); //in lại những hình đã vẽ
		checkOption(hdc); //chuẩn bị pen và brush
		switch (shape) //vẽ hình mới
		{
		case 0:
			MoveToEx(hdc, ptFirst.x, ptFirst.y, NULL);
			LineTo(hdc, ptLast.x, ptLast.y);
			break;
		case 1:
			MoveToEx(hdc, ptFirst.x, ptFirst.y, NULL);
			Rectangle(hdc, ptFirst.x, ptFirst.y, ptLast.x, ptLast.y);
			break;
		case 2:
			MoveToEx(hdc, ptFirst.x, ptFirst.y, NULL);
			Ellipse(hdc, ptFirst.x, ptFirst.y, ptLast.x, ptLast.y);
			break;
		case 3:
			MoveToEx(hdc, ptFirst.x, ptFirst.y, NULL);
			int t = Min((ptLast.x - ptFirst.x), (ptLast.y - ptFirst.y));
			if (ptFirst.x <= ptLast.x)
			{
				if (ptFirst.y <= ptLast.y)
					Rectangle(hdc, ptFirst.x, ptFirst.y, ptFirst.x + t, ptFirst.y + t);
				else
					Rectangle(hdc, ptFirst.x, ptFirst.y + t, ptFirst.x - t, ptFirst.y);
			}
			else
			{
				if (ptFirst.y <= ptLast.y)
					Rectangle(hdc, ptFirst.x, ptFirst.y - t, ptFirst.x + t, ptFirst.y);
				else
					Rectangle(hdc, ptFirst.x, ptFirst.y, ptFirst.x + t, ptFirst.y + t);
			}
			break;
		}
		DeleteObject(Pen);
		EndPaint(hWnd, &ps);
		break;

	case WM_CREATE: //tạo 2 toolbar
		TBADDBITMAP tbab;
		TBBUTTON tbb[21];
		TBADDBITMAP tbab1;
		TBBUTTON tbb1[15];
		hWndBorderToolBar = CreateWindowEx(
						  0,
						  TOOLBARCLASSNAME,
						  (LPCWSTR)NULL,
						  WS_CHILD,
						  0, 0,
						  0, 0,
						  hWnd,
						  (HMENU)IDR_TOOLBAR1,
						  (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
						  NULL);

		if (!hWndBorderToolBar)
		{
			MessageBox(NULL, (LPCWSTR)L"Toolbar Failed.", (LPCWSTR)L"Error", MB_OK | MB_ICONERROR);
			return 0;
		}
		SendMessage(hWndBorderToolBar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
		SendMessage(hWndBorderToolBar, TB_SETBITMAPSIZE, (WPARAM)0, (LPARAM)MAKELONG(15, 15));
		tbab.hInst = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
		tbab.nID = IDB_TOOLBITMAP1;
		SendMessage(hWndBorderToolBar, TB_ADDBITMAP, (WPARAM)2, (LPARAM)&tbab);
		ZeroMemory(tbb, sizeof(tbb));

		tbb[0].iBitmap = 0;
		tbb[0].idCommand = ID_BORDER_BLACK;
		tbb[0].fsState = TBSTATE_ENABLED;
		tbb[0].fsStyle = TBSTYLE_BUTTON;

		tbb[1].iBitmap = 1;
		tbb[1].idCommand = ID_BORDER_DRED;
		tbb[1].fsState = TBSTATE_ENABLED;
		tbb[1].fsStyle = TBSTYLE_BUTTON;
		
		tbb[2].iBitmap = 2;
		tbb[2].idCommand = ID_BORDER_DGREEN;
		tbb[2].fsState = TBSTATE_ENABLED;
		tbb[2].fsStyle = TBSTYLE_BUTTON;

		tbb[3].iBitmap = 3;
		tbb[3].idCommand = ID_BORDER_DYELLOW;
		tbb[3].fsState = TBSTATE_ENABLED;
		tbb[3].fsStyle = TBSTYLE_BUTTON;

		tbb[4].iBitmap = 4;
		tbb[4].idCommand = ID_BORDER_DBLUE;
		tbb[4].fsState = TBSTATE_ENABLED;
		tbb[4].fsStyle = TBSTYLE_BUTTON;

		tbb[5].iBitmap = 5;
		tbb[5].idCommand = ID_BORDER_PURPLE;
		tbb[5].fsState = TBSTATE_ENABLED;
		tbb[5].fsStyle = TBSTYLE_BUTTON;

		tbb[6].iBitmap = 6;
		tbb[6].idCommand = ID_BORDER_TEAL;
		tbb[6].fsState = TBSTATE_ENABLED;
		tbb[6].fsStyle = TBSTYLE_BUTTON;

		tbb[7].iBitmap = 7;
		tbb[7].idCommand = ID_BORDER_GRAY;
		tbb[7].fsState = TBSTATE_ENABLED;
		tbb[7].fsStyle = TBSTYLE_BUTTON;

		tbb[8].iBitmap = 8;
		tbb[8].idCommand = ID_BORDER_RED;
		tbb[8].fsState = TBSTATE_ENABLED;
		tbb[8].fsStyle = TBSTYLE_BUTTON;

		tbb[9].iBitmap = 9;
		tbb[9].idCommand = ID_BORDER_GREEN;
		tbb[9].fsState = TBSTATE_ENABLED;
		tbb[9].fsStyle = TBSTYLE_BUTTON;

		tbb[10].iBitmap = 10;
		tbb[10].idCommand = ID_BORDER_YELLOW;
		tbb[10].fsState = TBSTATE_ENABLED;
		tbb[10].fsStyle = TBSTYLE_BUTTON;

		tbb[11].iBitmap = 11;
		tbb[11].idCommand = ID_BORDER_BLUE;
		tbb[11].fsState = TBSTATE_ENABLED;
		tbb[11].fsStyle = TBSTYLE_BUTTON;

		tbb[12].iBitmap = 12;
		tbb[12].idCommand = ID_BORDER_PINK;
		tbb[12].fsState = TBSTATE_ENABLED;
		tbb[12].fsStyle = TBSTYLE_BUTTON;

		tbb[13].iBitmap = 13;
		tbb[13].idCommand = ID_BORDER_MINT;
		tbb[13].fsState = TBSTATE_ENABLED;
		tbb[13].fsStyle = TBSTYLE_BUTTON;

		tbb[14].iBitmap = 14;
		tbb[14].idCommand = ID_BORDER_WHITE;
		tbb[14].fsState = TBSTATE_ENABLED;
		tbb[14].fsStyle = TBSTYLE_BUTTON;

		tbb[15].iBitmap = 15;
		tbb[15].idCommand = ID_BORDER_1;
		tbb[15].fsState = TBSTATE_ENABLED;
		tbb[15].fsStyle = TBSTYLE_BUTTON;

		tbb[16].iBitmap = 16;
		tbb[16].idCommand = ID_BORDER_2;
		tbb[16].fsState = TBSTATE_ENABLED;
		tbb[16].fsStyle = TBSTYLE_BUTTON;

		tbb[17].iBitmap = 17;
		tbb[17].idCommand = ID_BORDER_5;
		tbb[17].fsState = TBSTATE_ENABLED;
		tbb[17].fsStyle = TBSTYLE_BUTTON;

		tbb[18].iBitmap = 18;
		tbb[18].idCommand = ID_BORDER_10;
		tbb[18].fsState = TBSTATE_ENABLED;
		tbb[18].fsStyle = TBSTYLE_BUTTON;

		tbb[19].iBitmap = 19;
		tbb[19].idCommand = ID_BORDER_15;
		tbb[19].fsState = TBSTATE_ENABLED;
		tbb[19].fsStyle = TBSTYLE_BUTTON;

		tbb[20].iBitmap = 20;
		tbb[20].idCommand = ID_BORDER_20;
		tbb[20].fsState = TBSTATE_ENABLED;
		tbb[20].fsStyle = TBSTYLE_BUTTON;

		SendMessage(hWndBorderToolBar, TB_ADDBUTTONS, 21, (LPARAM)&tbb);


		hWndFillToolBar = CreateWindowEx(0,
						TOOLBARCLASSNAME,
						(LPCWSTR)NULL,
						WS_CHILD | CCS_NOPARENTALIGN,
						0, 15,
						500, 15,
						hWnd,
						(HMENU)IDR_TOOLBAR2,
						(HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
						NULL);

		if (!hWndFillToolBar)
		{
			MessageBox(NULL, (LPCWSTR)L"Toolbar Failed.", (LPCWSTR)L"Error", MB_OK | MB_ICONERROR);
			return 0;
		}
		SendMessage(hWndFillToolBar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
		SendMessage(hWndFillToolBar, TB_SETBITMAPSIZE, (WPARAM)0, (LPARAM)MAKELONG(15, 15));
		tbab1.hInst = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
		tbab1.nID = IDB_TOOLBITMAP2;
		SendMessage(hWndFillToolBar, TB_ADDBITMAP, (WPARAM)2, (LPARAM)&tbab1);
		ZeroMemory(tbb1, sizeof(tbb1));

		tbb1[0].iBitmap = 0;
		tbb1[0].idCommand = ID_FILL_BLACK;
		tbb1[0].fsState = TBSTATE_ENABLED;
		tbb1[0].fsStyle = TBSTYLE_BUTTON;

		tbb1[1].iBitmap = 1;
		tbb1[1].idCommand = ID_FILL_DRED;
		tbb1[1].fsState = TBSTATE_ENABLED;
		tbb1[1].fsStyle = TBSTYLE_BUTTON;

		tbb1[2].iBitmap = 2;
		tbb1[2].idCommand = ID_FILL_DGREEN;
		tbb1[2].fsState = TBSTATE_ENABLED;
		tbb1[2].fsStyle = TBSTYLE_BUTTON;

		tbb1[3].iBitmap = 3;
		tbb1[3].idCommand = ID_FILL_DYELLOW;
		tbb1[3].fsState = TBSTATE_ENABLED;
		tbb1[3].fsStyle = TBSTYLE_BUTTON;

		tbb1[4].iBitmap = 4;
		tbb1[4].idCommand = ID_FILL_DBLUE;
		tbb1[4].fsState = TBSTATE_ENABLED;
		tbb1[4].fsStyle = TBSTYLE_BUTTON;

		tbb1[5].iBitmap = 5;
		tbb1[5].idCommand = ID_FILL_PURPLE;
		tbb1[5].fsState = TBSTATE_ENABLED;
		tbb1[5].fsStyle = TBSTYLE_BUTTON;

		tbb1[6].iBitmap = 6;
		tbb1[6].idCommand = ID_FILL_TEAL;
		tbb1[6].fsState = TBSTATE_ENABLED;
		tbb1[6].fsStyle = TBSTYLE_BUTTON;

		tbb1[7].iBitmap = 7;
		tbb1[7].idCommand = ID_FILL_GRAY;
		tbb1[7].fsState = TBSTATE_ENABLED;
		tbb1[7].fsStyle = TBSTYLE_BUTTON;

		tbb1[8].iBitmap = 8;
		tbb1[8].idCommand = ID_FILL_RED;
		tbb1[8].fsState = TBSTATE_ENABLED;
		tbb1[8].fsStyle = TBSTYLE_BUTTON;

		tbb1[9].iBitmap = 9;
		tbb1[9].idCommand = ID_FILL_GREEN;
		tbb1[9].fsState = TBSTATE_ENABLED;
		tbb1[9].fsStyle = TBSTYLE_BUTTON;

		tbb1[10].iBitmap = 10;
		tbb1[10].idCommand = ID_FILL_YELLOW;
		tbb1[10].fsState = TBSTATE_ENABLED;
		tbb1[10].fsStyle = TBSTYLE_BUTTON;

		tbb1[11].iBitmap = 11;
		tbb1[11].idCommand = ID_FILL_BLUE;
		tbb1[11].fsState = TBSTATE_ENABLED;
		tbb1[11].fsStyle = TBSTYLE_BUTTON;

		tbb1[12].iBitmap = 12;
		tbb1[12].idCommand = ID_FILL_PINK;
		tbb1[12].fsState = TBSTATE_ENABLED;
		tbb1[12].fsStyle = TBSTYLE_BUTTON;

		tbb1[13].iBitmap = 13;
		tbb1[13].idCommand = ID_FILL_MINT;
		tbb1[13].fsState = TBSTATE_ENABLED;
		tbb1[13].fsStyle = TBSTYLE_BUTTON;

		tbb1[14].iBitmap = 14;
		tbb1[14].idCommand = ID_FILL_WHITE;
		tbb1[14].fsState = TBSTATE_ENABLED;
		tbb1[14].fsStyle = TBSTYLE_BUTTON;

		SendMessage(hWndFillToolBar, TB_ADDBUTTONS, 15, (LPARAM)&tbb1);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_COMMAND: //xửa lý command
		switch (LOWORD(wParam))
		{
			case ID_LIGNE:
				shape = 0;
				break;
			case ID_RECTANGLE:
				shape = 1;
				break;
			case ID_ELIPSE:
				shape = 2;
				break;
			case ID_CARRE:
				shape = 3;
				break;
			case ID_BORDURE:
				if (!bShow)
				{
					bShow = true;
					ShowWindow(hWndBorderToolBar, SW_SHOW);
				}
				else
				{
					bShow = false;
					ShowWindow(hWndBorderToolBar, SW_HIDE);
				}
				break;
			case ID_REMPLIR:
				if (!fShow)
				{
					fShow = true;
					ShowWindow(hWndFillToolBar, SW_SHOW);
				}
				else
				{
					fShow = false;
					ShowWindow(hWndFillToolBar, SW_HIDE);
				}
				break;
			case ID_BORDER_BLACK:
				bColor = RGB(0, 0, 0);
				break;
			case ID_BORDER_DRED:
				bColor = RGB(136, 0, 21);
				break;
			case ID_BORDER_DGREEN:
				bColor = RGB(37, 177, 28);
				break;
			case ID_BORDER_DYELLOW:
				bColor = RGB(140, 140, 0);
				break;
			case ID_BORDER_DBLUE:
				bColor = RGB(28, 33, 104);
				break;
			case ID_BORDER_PURPLE:
				bColor = RGB(170, 0, 170);
				break;
			case ID_BORDER_TEAL:
				bColor = RGB(0, 126, 126);
				break;
			case ID_BORDER_GRAY:
				bColor = RGB(192, 192, 192);
				break;
			case ID_BORDER_RED:
				bColor = RGB(255, 0, 0);
				break;
			case ID_BORDER_GREEN:
				bColor = RGB(0, 255, 0);
				break;
			case ID_BORDER_YELLOW:
				bColor = RGB(255, 255, 0);
				break;
			case ID_BORDER_BLUE:
				bColor = RGB(0, 0, 255);
				break;
			case ID_BORDER_PINK:
				bColor = RGB(255, 0, 255);
				break;
			case ID_BORDER_MINT:
				bColor = RGB(0, 255, 255);
				break;
			case ID_BORDER_WHITE:
				bColor = RGB(255, 255, 255);
				break;
			case ID_BORDER_1:
				thickness = 1;
				break;
			case ID_BORDER_2:
				thickness = 2;
				break;
			case ID_BORDER_5:
				thickness = 5;
				break;
			case ID_BORDER_10:
				thickness = 10;
				break;
			case ID_BORDER_15:
				thickness = 15;
				break;
			case ID_BORDER_20:
				thickness = 20;
				break;
			case ID_FILL_BLACK:
				fColor = RGB(0, 0, 0);
				break;
			case ID_FILL_DRED:
				fColor = RGB(136, 0, 21);
				break;
			case ID_FILL_DGREEN:
				fColor = RGB(37, 177, 28);
				break;
			case ID_FILL_DYELLOW:
				fColor = RGB(140, 140, 0);
				break;
			case ID_FILL_DBLUE:
				fColor = RGB(28, 33, 104);
				break;
			case ID_FILL_PURPLE:
				fColor = RGB(170, 0, 170);
				break;
			case ID_FILL_TEAL:
				fColor = RGB(0, 126, 126);
				break;
			case ID_FILL_GRAY:
				fColor = RGB(192, 192, 192);
				break;
			case ID_FILL_RED:
				fColor = RGB(255, 0, 0);
				break;
			case ID_FILL_GREEN:
				fColor = RGB(0, 255, 0);
				break;
			case ID_FILL_YELLOW:
				fColor = RGB(255, 255, 0);
				break;
			case ID_FILL_BLUE:
				fColor = RGB(0, 0, 255);
				break;
			case ID_FILL_PINK:
				fColor = RGB(255, 0, 255);
				break;
			case ID_FILL_MINT:
				fColor = RGB(0, 255, 255);
				break;
			case ID_FILL_WHITE:
				fColor = RGB(255, 255, 255);
				break;
			case ID_NEW:
				while (!List.empty())
					List.pop();
				break;
			case ID_EXIT:
				PostQuitMessage(0);
				break;
		}
		break;

	case WM_LBUTTONUP:
		if (moved) //kiểm tra chỉ khi LBUTTONDOWN và MOVE thì mới lưu hình, tránh tình trạng lưu từng điểm rời rạc
		{
			if (!fShow)
				fColor = NULL;
			if (!bShow)
				bColor = NULL;
			saveShape(shape, ptFirst, ptLast, bColor, fColor, thickness);
		}
		moved = false;
		break;

	case WM_LBUTTONDOWN:
		x = LOWORD(lParam);
		y = HIWORD(lParam);
		ptFirst.x = x;
		ptFirst.y = y;
		hdc = GetDC(hWnd);
		MoveToEx(hdc, ptFirst.x, ptFirst.y, NULL);
		ReleaseDC(hWnd, hdc);
		break;

	case WM_MOUSEMOVE:
		if (wParam & MK_LBUTTON)
		{
			moved = true;
			x = GET_X_LPARAM(lParam);
			y = GET_Y_LPARAM(lParam);
			ptLast.x = x;
			ptLast.y = y;
			InvalidateRect(hWnd, NULL, TRUE);
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return 0;
}
