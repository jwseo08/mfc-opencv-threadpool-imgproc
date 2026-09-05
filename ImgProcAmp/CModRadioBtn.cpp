#include "pch.h" // PCH를 쓰지 않는 프로젝트에서는 삭제하세요.
#include "CModRadioBtn.h"

#include <uxtheme.h>
#include <vsstyle.h>

#pragma comment(lib, "UxTheme.lib")

BEGIN_MESSAGE_MAP(CModRadioBtn, CButton)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_ENABLE()
    ON_WM_SETFOCUS()
    ON_WM_KILLFOCUS()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_MESSAGE(BM_SETCHECK, &CModRadioBtn::OnSetCheck)
    ON_MESSAGE(WM_SETTEXT, &CModRadioBtn::OnSetText)
END_MESSAGE_MAP()

CModRadioBtn::CModRadioBtn()
    : m_textColor(::GetSysColor(COLOR_BTNTEXT))
    , m_disabledTextColor(::GetSysColor(COLOR_GRAYTEXT))
    , m_backgroundColor(::GetSysColor(COLOR_3DFACE))
{
}

void CModRadioBtn::SetTextColor(COLORREF color, BOOL redraw)
{
    m_textColor = color;
    RedrawIfNeeded(redraw);
}

void CModRadioBtn::SetDisabledTextColor(COLORREF color, BOOL redraw)
{
    m_disabledTextColor = color;
    RedrawIfNeeded(redraw);
}

void CModRadioBtn::SetBackgroundColor(COLORREF color, BOOL redraw)
{
    m_backgroundColor = color;
    RedrawIfNeeded(redraw);
}

void CModRadioBtn::OnPaint()
{
    CPaintDC paintDC(this);
    CRect client;
    GetClientRect(&client);
    if (client.IsRectEmpty()) return;

    CDC bufferDC;
    bufferDC.CreateCompatibleDC(&paintDC);
    CBitmap bitmap;
    bitmap.CreateCompatibleBitmap(&paintDC, client.Width(), client.Height());
    CBitmap* oldBitmap = bufferDC.SelectObject(&bitmap);

    bufferDC.FillSolidRect(client, m_backgroundColor);
    bufferDC.SetBkMode(TRANSPARENT);
    CFont* font = GetFont();
    CFont* oldFont = font != nullptr ? bufferDC.SelectObject(font) : nullptr;

    const UINT buttonState = GetState();
    const bool checked = GetCheck() != BST_UNCHECKED;
    const bool disabled = IsWindowEnabled() == FALSE;
    const bool pressed = (buttonState & BST_PUSHED) != 0;

    int themeState = RBS_UNCHECKEDNORMAL;
    if (disabled)
    {
        themeState = checked ? RBS_CHECKEDDISABLED : RBS_UNCHECKEDDISABLED;
    }
    else if (pressed)
    {
        themeState = checked ? RBS_CHECKEDPRESSED : RBS_UNCHECKEDPRESSED;
    }
    else if (checked)
    {
        themeState = RBS_CHECKEDNORMAL;
    }

    const int dpi = bufferDC.GetDeviceCaps(LOGPIXELSX);
    CSize glyphSize(::MulDiv(13, dpi, 96), ::MulDiv(13, dpi, 96));
    HTHEME theme = ::OpenThemeData(m_hWnd, L"BUTTON");
    if (theme != nullptr)
    {
        SIZE size{};
        if (SUCCEEDED(::GetThemePartSize(theme, bufferDC.GetSafeHdc(),
            BP_RADIOBUTTON, themeState, nullptr, TS_DRAW, &size)))
        {
            glyphSize = CSize(size.cx, size.cy);
        }
    }

    int glyphTop = (client.Height() - glyphSize.cy) / 2;
    if (glyphTop < 0) glyphTop = 0;

    CRect glyph(client.left, glyphTop, client.left + glyphSize.cx, glyphTop + glyphSize.cy);

    if (theme != nullptr)
    {
        ::DrawThemeBackground(theme, bufferDC.GetSafeHdc(), BP_RADIOBUTTON, themeState, &glyph, nullptr);
        ::CloseThemeData(theme);
    }
    else
    {
        UINT frameState = DFCS_BUTTONRADIO;
        if (checked)  frameState |= DFCS_CHECKED;
        if (disabled) frameState |= DFCS_INACTIVE;
        if (pressed)  frameState |= DFCS_PUSHED;
        bufferDC.DrawFrameControl(&glyph, DFC_BUTTON, frameState);
    }

    CString text;
    GetWindowText(text);
    const int gap = ::MulDiv(4, dpi, 96);
    CRect textRect(glyph.right + gap, client.top, client.right, client.bottom);
    bufferDC.SetTextColor(disabled ? m_disabledTextColor : m_textColor);

    UINT format = DT_LEFT | DT_VCENTER | DT_SINGLELINE;
    if ((SendMessage(WM_QUERYUISTATE) & UISF_HIDEACCEL) != 0) format |= DT_HIDEPREFIX;
    bufferDC.DrawText(text, textRect, format);

    if (GetFocus() == this &&
        (SendMessage(WM_QUERYUISTATE) & UISF_HIDEFOCUS) == 0)
    {
        CRect focusRect = textRect;
        bufferDC.DrawText(text, focusRect, format | DT_CALCRECT);
        focusRect.InflateRect(1, 1);
        bufferDC.DrawFocusRect(focusRect);
    }

    paintDC.BitBlt(0, 0, client.Width(), client.Height(), &bufferDC, 0, 0, SRCCOPY);

    if (oldFont != nullptr) bufferDC.SelectObject(oldFont);
    bufferDC.SelectObject(oldBitmap);
}

BOOL CModRadioBtn::OnEraseBkgnd(CDC* /*dc*/)
{
    return TRUE;
}

void CModRadioBtn::OnEnable(BOOL enable)
{
    CButton::OnEnable(enable);
    Invalidate(FALSE);
}

void CModRadioBtn::OnSetFocus(CWnd* oldWnd)
{
    CButton::OnSetFocus(oldWnd);
    Invalidate(FALSE);
}

void CModRadioBtn::OnKillFocus(CWnd* newWnd)
{
    CButton::OnKillFocus(newWnd);
    Invalidate(FALSE);
}

void CModRadioBtn::OnLButtonDown(UINT flags, CPoint point)
{
    CButton::OnLButtonDown(flags, point);
    Invalidate(FALSE);
}

void CModRadioBtn::OnLButtonUp(UINT flags, CPoint point)
{
    CButton::OnLButtonUp(flags, point);
    Invalidate(FALSE);
}

LRESULT CModRadioBtn::OnSetCheck(WPARAM /*check*/, LPARAM /*unused*/)
{
    const LRESULT result = Default();
    Invalidate(FALSE);
    return result;
}

LRESULT CModRadioBtn::OnSetText(WPARAM /*unused*/, LPARAM /*text*/)
{
    const LRESULT result = Default();
    Invalidate(FALSE);
    return result;
}

void CModRadioBtn::RedrawIfNeeded(BOOL redraw)
{
    if (redraw && GetSafeHwnd() != nullptr) Invalidate(FALSE);
}
