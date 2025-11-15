#ifndef TOOLTIPWIDGET_H
#define TOOLTIPWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QFont>

// 独立的顶层悬浮提示框（脱离滑块控件）
class TooltipWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TooltipWidget(QWidget *parent = nullptr);
    // 设置提示框内容和位置
    void setTooltipInfo(const QString& tips, const QPoint &targetPos);
    void setBottomMargin(int margin);
    void setTipsColor(const QColor& color);
    void setHPadding(int padding);
    void setVPadding(int padding);
protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_strTips;
    QFont m_font;
    int m_iRadius;
    int m_iBottomMargin;
    int m_iHPadding;
    int m_iVPadding;
    QColor m_tipsColor;
};

#endif // TOOLTIPWIDGET_H
