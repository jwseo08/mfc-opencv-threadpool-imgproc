#pragma once

#include <afxwin.h>

// mfc 기능을 상속받아 라디오 버튼 기능을 제공하는 클래스
// BS_RADIOBUTTON 혹은 BS_AUTORADIOBUTTON 리소스에 연결해서 사용
class CModRadioBtn : public CButton
{
public:
    CModRadioBtn();

    void SetTextColor(COLORREF color, BOOL redraw = TRUE);
    void SetDisabledTextColor(COLORREF color, BOOL redraw = TRUE);
    void SetBackgroundColor(COLORREF color, BOOL redraw = TRUE);
    COLORREF GetTextColor() const noexcept { return m_textColor; }

protected:
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* dc);
    afx_msg void OnEnable(BOOL enable);
    afx_msg void OnSetFocus(CWnd* oldWnd);
    afx_msg void OnKillFocus(CWnd* newWnd);
    afx_msg void OnLButtonDown(UINT flags, CPoint point);
    afx_msg void OnLButtonUp(UINT flags, CPoint point);
    afx_msg LRESULT OnSetCheck(WPARAM check, LPARAM unused);
    afx_msg LRESULT OnSetText(WPARAM unused, LPARAM text);
    DECLARE_MESSAGE_MAP()

private:
    void RedrawIfNeeded(BOOL redraw);

    COLORREF m_textColor;
    COLORREF m_disabledTextColor;
    COLORREF m_backgroundColor;
};
