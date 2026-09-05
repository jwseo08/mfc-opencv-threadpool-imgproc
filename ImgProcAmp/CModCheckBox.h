#pragma once

#include <afxwin.h>

// mfc 클래스를 상속받아 체크 박스 기능 제공 클래스
class CModCheckBox : public CButton
{
    DECLARE_DYNAMIC(CModCheckBox)

public:
    CModCheckBox();
    virtual ~CModCheckBox() = default;

    void SetTextColor(COLORREF color, BOOL redraw = TRUE);
    void SetDisabledTextColor(COLORREF color, BOOL redraw = TRUE);
    void SetBackgroundColor(COLORREF color, BOOL redraw = TRUE);

    COLORREF GetTextColor() const { return m_textColor; }
    COLORREF GetDisabledTextColor() const { return m_disabledTextColor; }
    COLORREF GetBackgroundColor() const { return m_backgroundColor; }

protected:
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* dc);
    afx_msg void OnEnable(BOOL enable);
    afx_msg void OnSetFocus(CWnd* oldWnd);
    afx_msg void OnKillFocus(CWnd* newWnd);
    afx_msg void OnMouseMove(UINT flags, CPoint point);
    afx_msg LRESULT OnMouseLeave(WPARAM wParam, LPARAM lParam);
    afx_msg void OnLButtonDown(UINT flags, CPoint point);
    afx_msg void OnLButtonUp(UINT flags, CPoint point);
    afx_msg LRESULT OnSetCheck(WPARAM check, LPARAM unused);
    afx_msg LRESULT OnSetText(WPARAM unused, LPARAM text);
    afx_msg LRESULT OnSetFont(WPARAM font, LPARAM redraw);

    DECLARE_MESSAGE_MAP()

private:
    void RedrawIfNeeded(BOOL redraw);
    int GetThemeState(bool disabled, bool pressed, bool hot, int check) const;

    COLORREF m_textColor;
    COLORREF m_disabledTextColor;
    COLORREF m_backgroundColor;
    bool m_trackingMouse;
    bool m_mouseOver;
};
