#ifndef SNAPSLIDER_H
#define SNAPSLIDER_H

#include <QSlider>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOptionSlider>
#include <QPainterPath> // 用于绘制圆角矩形
#include <QTimer>
#include "tooltipwidget.h"

// 独立的顶层悬浮部件：绘制建议值三角形图标
class RecommendIconWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RecommendIconWidget(QWidget *parent = nullptr);
    // 设置图标颜色
    void setIconColor(const QColor &color);
    // 设置自定义图标（优先显示自定义pixmap，无则显示三角形）
    void setCustomIcon(const QPixmap &pixmap);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor m_iconColor;
    QPixmap m_customPixmap;
};

class SnapSlider : public QSlider
{
    Q_OBJECT
public:
    explicit SnapSlider(QWidget *parent = nullptr);
    void setHandleColor(const QColor& color, const QColor& hoverColor);
    void setSelectColor(const QColor& selectedColor, const QColor& unselectColor);
    void setRecommendedPixmap(const QPixmap& pixmap);
    void setRecommended(const int recommended);
    void setRecommendColor(const QColor &color);
    void setUnit(const QString& unit);
    void updateRecommendPos();
    void hideRecommend();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    void enterEvent(QEnterEvent* event) override;
#else
    void enterEvent(QEvent* event) override;
#endif
    void leaveEvent(QEvent *event) override;
    void hideEvent(QHideEvent* event) override;
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

    // 计算建议值图标的位置（刻度正下方，脱离滑块区域）
    QPoint calculateRecommendIconPos(int recommendedValue);

private slots:
    // 计时器超时后隐藏提示框
    void onTooltipAutoHideTimeout();

private:
    bool m_isHovered;
    int m_grooveCornerRadius; // 存储轨道圆角半径
    // 计算当前值与范围的比例（0.0-1.0）
    double getValueRatio(int value);
    // 调整轨道区域（确保右侧有足够空间）
    QRect getAdjustedGrooveRect();
    // 计算滑块位置
    QRect getHandleRect();
    // 获取滑块中心点(绝对位置)
    QPoint getHandlerCenter();
    // 核心：根据鼠标位置计算并吸附到最近值
    int getSnappedValue(int x);
    // 绘制刻度（每个步长1个，宽1px、高2px，与滑条间距5px）
    void drawTicks(QPainter &painter, const QRect &grooveRect);
    // 绘制悬浮提示框
    void drawTooltip(QPainter &painter);

    QString getTipsStr();
private:
    QColor m_handlerColor;
    QColor m_handleHoverColor;
    QColor m_selectedColor;
    QColor m_unselectedColor;
    int m_grooveHeight;
    int m_grooveLeftPadding;
    int m_handleRadius;

    int m_tickWidth = 1;//：刻度宽度
    int m_tickHeight = 4;//：刻度高度
    int m_tickSpacing = 6;//：刻度与滑条的垂直间距 6px
    QColor m_tickColor;
    int m_recommended;
    QPixmap m_recommendedPixmap;
    // 独立建议值图标部件
    RecommendIconWidget *m_recIconWidget;
    // 图标与刻度的垂直间距（控制图标在刻度下方的距离，如5px）
    int m_recIconVerticalSpacing = 1;

    // 悬浮提示
    TooltipWidget *m_tooltipWidget;
    QString m_strUnit;
    // 提示框自动隐藏计时器
    QTimer *m_tooltipAutoHideTimer;
    // 提示框自动隐藏时长（1500ms = 1.5秒，可调整）
    int m_tooltipAutoHideMs = 1500;
};

#endif // SNAPSLIDER_H
