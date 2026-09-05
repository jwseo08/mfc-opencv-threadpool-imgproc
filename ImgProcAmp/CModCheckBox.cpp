#include "pch.h"
#include "CModCheckBox.h"

#include "pch.h" // PCH를 사용하지 않는 프로젝트라면 이 줄을 삭제하세요.
#include "CModCheckBox.h"

#include <uxtheme.h>
#include <vsstyle.h>

#pragma comment(lib, "UxTheme.lib")

IMPLEMENT_DYNAMIC(CModCheckBox, CButton)

BEGIN_MESSAGE_MAP(CModCheckBox, CButton)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_ENABLE()
    ON_WM_SETFOCUS()
    ON_WM_KILLFOCUS()
    ON_WM_MOUSEMOVE()
    ON_MESSAGE(WM_MOUSELEAVE, &CModCheckBox::OnMouseLeave)
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_MESSAGE(BM_SETCHECK, &CModCheckBox::OnSetCheck)
    ON_MESSAGE(WM_SETTEXT, &CModCheckBox::OnSetText)
    ON_MESSAGE(WM_SETFONT, &CModCheckBox::OnSetFont)
END_MESSAGE_MAP()

CModCheckBox::CModCheckBox()
    : m_textColor(::GetSysColor(COLOR_BTNTEXT))
    , m_disabledTextColor(::GetSysColor(COLOR_GRAYTEXT))
    , m_backgroundColor(::GetSysColor(COLOR_3DFACE))
    , m_trackingMouse(false)
    , m_mouseOver(false)
{
}

void CModCheckBox::SetTextColor(COLORREF color, BOOL redraw)
{
    m_textColor = color;
    RedrawIfNeeded(redraw);
}

void CModCheckBox::SetDisabledTextColor(COLORREF color, BOOL redraw)
{
    m_disabledTextColor = color;
    RedrawIfNeeded(redraw);
}

void CModCheckBox::SetBackgroundColor(COLORREF color, BOOL redraw)
{
    m_backgroundColor = color;
    RedrawIfNeeded(redraw);
}

int CModCheckBox::GetThemeState(bool disabled, bool pressed,
    bool hot, int check) const
{
    const bool mixed = check == BST_INDETERMINATE;
    const bool checked = check == BST_CHECKED;

    if (mixed)
    {
        if (disabled) return CBS_MIXEDDISABLED;
        if (pressed) return CBS_MIXEDPRESSED;
        if (hot) return CBS_MIXEDHOT;
        return CBS_MIXEDNORMAL;
    }

    if (checked)
    {
        if (disabled) return CBS_CHECKEDDISABLED;
        if (pressed) return CBS_CHECKEDPRESSED;
        if (hot) return CBS_CHECKEDHOT;
        return CBS_CHECKEDNORMAL;
    }

    if (disabled) return CBS_UNCHECKEDDISABLED;
    if (pressed) return CBS_UNCHECKEDPRESSED;
    if (hot) return CBS_UNCHECKEDHOT;
    return CBS_UNCHECKEDNORMAL;
}

void CModCheckBox::OnPaint()
{
    CPaintDC paintDC(this);

    CRect client;
    GetClientRect(&client);
    if (client.IsRectEmpty()) return;

    // Paint once into an off-screen bitmap to prevent background flicker.
    CDC bufferDC;
    if (!bufferDC.CreateCompatibleDC(&paintDC)) return;

    CBitmap bitmap;
    if (!bitmap.CreateCompatibleBitmap(&paintDC, client.Width(), client.Height())) return;

    CBitmap* oldBitmap = bufferDC.SelectObject(&bitmap);
    bufferDC.FillSolidRect(client, m_backgroundColor);
    bufferDC.SetBkMode(TRANSPARENT);

    CFont* font = GetFont();
    CFont* oldFont = font != nullptr ? bufferDC.SelectObject(font) : nullptr;

    const UINT buttonState = GetState();
    const int check = GetCheck();
    const bool disabled = IsWindowEnabled() == FALSE;
    const bool pressed = (buttonState & BST_PUSHED) != 0;
    const int themeState = GetThemeState(disabled, pressed, m_mouseOver, check);

    const int dpi = bufferDC.GetDeviceCaps(LOGPIXELSX);
    CSize glyphSize(::MulDiv(13, dpi, 96), ::MulDiv(13, dpi, 96));

    HTHEME theme = ::OpenThemeData(m_hWnd, L"BUTTON");
    if (theme != nullptr)
    {
        SIZE size{};
        if (SUCCEEDED(::GetThemePartSize(theme, bufferDC.GetSafeHdc(),
            BP_CHECKBOX, themeState, nullptr, TS_DRAW, &size)))
        {
            glyphSize = CSize(size.cx, size.cy);
        }
    }

    const DWORD style = GetStyle();
    const bool glyphOnRight = (style & BS_LEFTTEXT) != 0;
    int glyphTop = client.top + (client.Height() - glyphSize.cy) / 2;
    if (glyphTop < client.top) glyphTop = client.top;

    CRect glyph;
    if (glyphOnRight)
    {
        glyph.SetRect(client.right - glyphSize.cx, 
            glyphTop, 
            client.right, 
            glyphTop + glyphSize.cy);
    }
    else
    {
        glyph.SetRect(client.left, 
            glyphTop,
            client.left + glyphSize.cx, 
            glyphTop + glyphSize.cy);
    }

    if (theme != nullptr)
    {
        ::DrawThemeBackground(theme, bufferDC.GetSafeHdc(), BP_CHECKBOX, themeState, &glyph, nullptr);
        ::CloseThemeData(theme);
    }
    else
    {
        UINT frameState = (check == BST_INDETERMINATE) ? DFCS_BUTTON3STATE : DFCS_BUTTONCHECK;
        if (check != BST_UNCHECKED) frameState |= DFCS_CHECKED;
        if (disabled) frameState |= DFCS_INACTIVE;
        if (pressed) frameState |= DFCS_PUSHED;
        if (m_mouseOver) frameState |= DFCS_HOT;
        bufferDC.DrawFrameControl(&glyph, DFC_BUTTON, frameState);
    }

    CString caption;
    GetWindowText(caption);
    const int gap = ::MulDiv(4, dpi, 96);
    CRect textRect = client;
    if (glyphOnRight)
    {
        textRect.right = glyph.left - gap;
        if (textRect.right < textRect.left) textRect.right = textRect.left;
    }
    else
    {
        textRect.left = glyph.right + gap;
        if (textRect.left > textRect.right) textRect.left = textRect.right;
    }

    bufferDC.SetTextColor(disabled ? m_disabledTextColor : m_textColor);

    UINT format = DT_VCENTER | DT_SINGLELINE;
    format |= glyphOnRight ? DT_RIGHT : DT_LEFT;
    if ((SendMessage(WM_QUERYUISTATE) & UISF_HIDEACCEL) != 0) format |= DT_HIDEPREFIX;
    bufferDC.DrawText(caption, &textRect, format);

    if (GetFocus() == this && 
        (SendMessage(WM_QUERYUISTATE) & UISF_HIDEFOCUS) == 0 &&
        !caption.IsEmpty())
    {
        CRect focusRect = textRect;
        bufferDC.DrawText(caption, &focusRect, format | DT_CALCRECT);
        focusRect.OffsetRect(0, (textRect.Height() - focusRect.Height()) / 2);
        focusRect.InflateRect(1, 1);
        bufferDC.DrawFocusRect(&focusRect);
    }

    paintDC.BitBlt(0, 0, client.Width(), client.Height(), &bufferDC, 0, 0, SRCCOPY);

    if (oldFont != nullptr) bufferDC.SelectObject(oldFont);

    bufferDC.SelectObject(oldBitmap);
}

BOOL CModCheckBox::OnEraseBkgnd(CDC* /*dc*/)
{
    return TRUE;
}

void CModCheckBox::OnEnable(BOOL enable)
{
    CButton::OnEnable(enable);
    Invalidate(FALSE);
}

void CModCheckBox::OnSetFocus(CWnd* oldWnd)
{
    CButton::OnSetFocus(oldWnd);
    Invalidate(FALSE);
}

void CModCheckBox::OnKillFocus(CWnd* newWnd)
{
    CButton::OnKillFocus(newWnd);
    Invalidate(FALSE);
}

void CModCheckBox::OnMouseMove(UINT flags, CPoint point)
{
    CButton::OnMouseMove(flags, point);

    if (!m_trackingMouse)
    {
        TRACKMOUSEEVENT track{};
        track.cbSize = sizeof(track);
        track.dwFlags = TME_LEAVE;
        track.hwndTrack = m_hWnd;
        m_trackingMouse = ::TrackMouseEvent(&track) != FALSE;
    }

    if (!m_mouseOver)
    {
        m_mouseOver = true;
        Invalidate(FALSE);
    }
}

LRESULT CModCheckBox::OnMouseLeave(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    m_trackingMouse = false;
    m_mouseOver = false;
    Invalidate(FALSE);
    return 0;
}

void CModCheckBox::OnLButtonDown(UINT flags, CPoint point)
{
    CButton::OnLButtonDown(flags, point);
    Invalidate(FALSE);
}

void CModCheckBox::OnLButtonUp(UINT flags, CPoint point)
{
    CButton::OnLButtonUp(flags, point);
    Invalidate(FALSE);
}

LRESULT CModCheckBox::OnSetCheck(WPARAM /*check*/, LPARAM /*unused*/)
{
    const LRESULT result = Default();
    Invalidate(FALSE);
    return result;
}

LRESULT CModCheckBox::OnSetText(WPARAM /*unused*/, LPARAM /*text*/)
{
    const LRESULT result = Default();
    Invalidate(FALSE);
    return result;
}

LRESULT CModCheckBox::OnSetFont(WPARAM /*font*/, LPARAM redraw)
{
    const LRESULT result = Default();
    RedrawIfNeeded(static_cast<BOOL>(redraw));
    return result;
}

void CModCheckBox::RedrawIfNeeded(BOOL redraw)
{
    if (redraw && GetSafeHwnd() != nullptr) Invalidate(FALSE);
}
