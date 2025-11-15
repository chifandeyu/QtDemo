#include "tooltipwidget.h"

TooltipWidget::TooltipWidget(QWidget *parent) : QWidget(parent) {
    // 设置为顶层部件（不依赖父控件，悬浮在窗口之上）
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents); // 鼠标穿透，不影响滑块操作
    setAttribute(Qt::WA_ShowWithoutActivating);     // 显示时不抢夺焦点
    m_font.setBold(true);
    m_font.setPointSize(12);
    m_iRadius = 6;
    m_iBottomMargin = 10;
    m_tipsColor = QColor("#222222");
}

void TooltipWidget::setTooltipInfo(const QString& tips, const QPoint &targetPos) {
    m_strTips = tips;
    // 计算提示框位置：目标点（滑块中心）正上方 + 15px间距（脱离滑块）
    QFontMetrics fm(m_font);
    int textWidth = fm.horizontalAdvance(tips) + 12;  // 左右内边距6px
    int textHeight = fm.height() + 3; // 上下内边距3px

    // 提示框居中对齐目标点，顶部在目标点上方15px
    int x = targetPos.x() - textWidth / 2;
    int y = targetPos.y() - textHeight - m_iBottomMargin;          // 关键：15px间距，脱离滑块
    move(x, y);
    resize(textWidth, textHeight);
    update();
}

void TooltipWidget::setBottomMargin(int margin)
{
    m_iBottomMargin = margin;
    update();
}

void TooltipWidget::setTipsColor(const QColor &color)
{
    m_tipsColor = color;
    update();
}

void TooltipWidget::setHPadding(int padding)
{
    m_iHPadding = padding;
}

void TooltipWidget::setVPadding(int padding)
{
    m_iVPadding = padding;
}

void TooltipWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 1. 绘制提示框背景（Win11风格：白色圆角 + 淡阴影）
    painter.setBrush(Qt::white);
    painter.setPen(QPen(QColor(0, 0, 0, 30), 1)); // 淡灰边框（模拟阴影）
    painter.drawRoundedRect(rect(), m_iRadius, m_iRadius);        // 圆角6px

    // 2. 绘制提示文本（深灰、居中）
    painter.setPen(m_tipsColor);
    painter.setFont(m_font);
    painter.drawText(rect(), Qt::AlignCenter, m_strTips);

    /// TODO:3. 绘制箭头（兼容低版本Qt：手动计算底部中心）
    ///
}
