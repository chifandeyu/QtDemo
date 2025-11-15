#include "snapslider.h"

SnapSlider::SnapSlider(QWidget *parent) : QSlider(Qt::Horizontal, parent) {
    setRange(1, 20);        // 范围1-20
    setSingleStep(1);
    setPageStep(1);
    setValue(1);           // 初始值
    setFixedHeight(24);
    setMouseTracking(true);
    m_isHovered = false;
    m_grooveCornerRadius = 2; // 轨道圆角半径（可调整，默认2px）
    m_handlerColor = QColor("#0078d7");
    m_handleHoverColor = QColor("#005a9e");
    m_selectedColor = QColor("#0078d7");
    m_unselectedColor = QColor("#e0e0e0");
    m_grooveHeight = 4;
    m_grooveLeftPadding = 0;
    m_handleRadius = 8;
    m_recommended = -1;
    m_tickColor = QColor("#D8D8D8");

    // 初始化独立提示框（父控件为滑块的顶层窗口，确保悬浮）
    m_tooltipWidget = new TooltipWidget(window());
    m_tooltipWidget->setBottomMargin(12);
    m_tooltipWidget->setHPadding(5);
    m_tooltipWidget->setVPadding(3);
    m_tooltipWidget->setTipsColor(QColor("#666666"));
    m_strUnit = "m";

    // 关键：初始化独立建议值图标部件（父控件设为滑块的顶层窗口，确保悬浮）
    m_recIconWidget = new RecommendIconWidget(window());
    m_recIconWidget->resize(9, 8);
    m_recIconWidget->setIconColor(QColor("#0072FF"));
    m_recIconWidget->setCustomIcon(QPixmap(":/img/bandwidth_arrow.png"));
    m_recIconWidget->hide();

    // 初始化提示框自动隐藏计时器
    m_tooltipAutoHideTimer = new QTimer(this);
    m_tooltipAutoHideTimer->setSingleShot(true); // 单次触发（超时后只执行一次）
    m_tooltipAutoHideTimer->setInterval(m_tooltipAutoHideMs);
    // 连接计时器超时信号到隐藏函数
    connect(m_tooltipAutoHideTimer, &QTimer::timeout, this, &SnapSlider::onTooltipAutoHideTimeout);
}

void SnapSlider::setHandleColor(const QColor &color, const QColor &hoverColor) {
    m_handlerColor = color;
    m_handleHoverColor = hoverColor;
}

void SnapSlider::setSelectColor(const QColor &selectedColor, const QColor &unselectColor)
{
    m_selectedColor = selectedColor;
    m_unselectedColor = unselectColor;
}

void SnapSlider::setRecommendedPixmap(const QPixmap &pixmap)
{
    m_recIconWidget->setCustomIcon(pixmap);
    update();
}

void SnapSlider::setRecommended(const int recommended)
{
    m_recommended = recommended;
    if (m_recommended == -1) {
        // 无建议值，隐藏图标
        m_recIconWidget->hide();
    } else {
        // 有建议值，计算图标位置并显示
        QPoint iconPos = calculateRecommendIconPos(m_recommended);
        m_recIconWidget->move(iconPos);
        m_recIconWidget->show();
    }

    update();
}

void SnapSlider::setRecommendColor(const QColor &color) {
    m_recIconWidget->setIconColor(color);
}

void SnapSlider::setUnit(const QString &unit)
{
    m_strUnit = unit;
    m_tooltipWidget->setTooltipInfo(getTipsStr(), getHandlerCenter());
}

void SnapSlider::updateRecommendPos()
{
    if (m_recommended != -1) {
        QPoint iconPos = calculateRecommendIconPos(m_recommended);
        m_recIconWidget->move(iconPos);
    }
}

void SnapSlider::hideRecommend()
{
    if (m_recIconWidget->isVisible()) {
        m_recIconWidget->hide();
    }
}

void SnapSlider::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        int value = getSnappedValue(event->pos().x());
        setValue(value);
        // 显示并更新提示框
        m_tooltipWidget->setTooltipInfo(getTipsStr(), getHandlerCenter());
        m_tooltipWidget->show();
        setSliderDown(true);
        event->accept();
        return;
    }
    QSlider::mousePressEvent(event);
}

void SnapSlider::mouseMoveEvent(QMouseEvent *event) {
    if (isSliderDown()) {
        // 同时停止计时器（避免拖动时隐藏）
        int value = getSnappedValue(event->pos().x());
        setValue(value);
        QString tips = QString("%1m").arg(value);
        m_tooltipWidget->setTooltipInfo(tips, getHandlerCenter());
        m_tooltipWidget->show();
        m_tooltipAutoHideTimer->stop(); // 拖动时不自动隐藏
        event->accept();
        return;
    }

    QRect handleRect = getHandleRect();
    bool isHoverHandle = handleRect.contains(event->pos());
    if (isHoverHandle) {
        // 鼠标悬浮到手柄上，显示提示框并停止计时器
        QString tips = QString("%1m").arg(value());
        m_tooltipWidget->setTooltipInfo(tips, getHandlerCenter());
        m_tooltipWidget->show();
        m_tooltipAutoHideTimer->stop(); // 悬浮时不自动隐藏
    } else {
        // 鼠标在滑块内但不在手柄上，隐藏提示框
        m_tooltipWidget->hide();
    }

    m_isHovered = isHoverHandle;
    update();
    QSlider::mouseMoveEvent(event);
}

void SnapSlider::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && isSliderDown()) {
        setSliderDown(false);
        // 释放时隐藏提示框
        m_tooltipWidget->hide();
        event->accept();
        return;
    }
    m_isHovered = false;
    m_tooltipWidget->hide(); // 意外释放也隐藏
    update();
    QSlider::mouseReleaseEvent(event);
}

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
void SnapSlider::enterEvent( QEnterEvent * event)
#else
void SnapSlider::enterEvent(QEvent *event)
#endif
{
    // 鼠标进入滑块时，检查是否初始位置就在手柄上
    QRect handleRect = getHandleRect();
    QPoint mousePos = mapFromGlobal(QCursor::pos()); // 获取鼠标在滑块内的位置
    m_isHovered = handleRect.contains(mousePos);

    if (m_isHovered) {
        // 初始就在手柄上，直接显示提示框
        m_tooltipWidget->setTooltipInfo(getTipsStr(), getHandlerCenter());
        m_tooltipWidget->show();
    }

    update();
    QSlider::enterEvent(event);
}

void SnapSlider::leaveEvent(QEvent *event) {
    m_isHovered = false;
    m_tooltipWidget->hide(); // 离开滑块隐藏提示框
    m_tooltipAutoHideTimer->stop(); // 离开时停止计时器
    update();
    QSlider::leaveEvent(event);
}

void SnapSlider::hideEvent(QHideEvent *event)
{
    m_recIconWidget->hide();
    QWidget::hideEvent(event);
}

void SnapSlider::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); // 抗锯齿：确保圆角边缘平滑

    QRect grooveRect = getAdjustedGrooveRect();
//    painter.fillRect(this->rect(), Qt::yellow);
    // 1. 绘制未选中轨道（浅灰 + 圆角）
    QRect tempRect = QRect(grooveRect.x(), grooveRect.y(), grooveRect.width() - m_grooveLeftPadding, grooveRect.height());
    QPainterPath unselectedPath;
    unselectedPath.addRoundedRect(tempRect, m_grooveCornerRadius, m_grooveCornerRadius);
    painter.fillPath(unselectedPath, m_unselectedColor);

    // 2. 绘制选中轨道（蓝色 + 圆角）
    double ratio = getValueRatio(value());
    int selectedWidth = grooveRect.width() * ratio;
    QRect selectedRect = grooveRect.adjusted(0, 0, selectedWidth - grooveRect.width(), 0);
    QPainterPath selectedPath;
    selectedPath.addRoundedRect(selectedRect, m_grooveCornerRadius, m_grooveCornerRadius);
    painter.fillPath(selectedPath, m_selectedColor);

    // 3. 绘制滑条下方的刻度（每个步长1个，共20个）
    drawTicks(painter, grooveRect);

    // 4. 绘制圆形滑块
    QRect handleRect = getHandleRect();
    QColor handleColor = m_isHovered ? m_handleHoverColor : m_handlerColor;
    painter.setBrush(handleColor);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(handleRect);
}

void SnapSlider::keyPressEvent(QKeyEvent *event) {
    // 先判断是否是上下左右方向键
    bool isArrowKey = (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right
                       || event->key() == Qt::Key_Up || event->key() == Qt::Key_Down);
    if (isArrowKey) {
        // 保存当前值，用于判断是否真的变化
        int oldValue = value();

        // 调用父类按键处理逻辑（自动改变滑块值）
        QSlider::keyPressEvent(event);

        // 只有值发生变化时，才更新提示框
        if (value() != oldValue) {
            QString tips = QString("%1m").arg(value());
            m_tooltipWidget->setTooltipInfo(tips, getHandlerCenter());
            m_tooltipWidget->show(); // 显示提示框

            // 重启计时器（每次按键都重置1.5秒倒计时）
            m_tooltipAutoHideTimer->start();
        }
        event->accept();
        return;
    }

    // 其他按键按默认逻辑处理
    QSlider::keyPressEvent(event);
}

QPoint SnapSlider::calculateRecommendIconPos(int recommendedValue)
{
    // 1. 校验依赖对象，避免空指针导致坐标异常
    if (!window() || m_recIconWidget == nullptr) {
        return QPoint(0, 0); // 异常时返回默认位置，避免崩溃
    }

    // 2. 计算建议值对应的刻度在滑块控件内的局部坐标（X+Y都要精准）
    QRect grooveRect = getAdjustedGrooveRect();
    int totalTicks = maximum() - minimum() + 1;
    double tickInterval = (double)grooveRect.width() / (totalTicks - 1);
    int tickIndex = recommendedValue - minimum();
    int tickLocalX = grooveRect.left() + tickIndex * tickInterval; // 刻度在滑块内的X坐标

    // 关键修正1：刻度的局部Y坐标 = 滑块轨道底部 + 刻度间距（控件内相对位置）
    int tickLocalY = grooveRect.bottom() + m_tickSpacing;
    QPoint tickLocalPos(tickLocalX, tickLocalY); // 刻度在滑块控件内的完整局部坐标

    // 3. 转换为屏幕绝对坐标（核心步骤：滑块内局部坐标 → 屏幕全局坐标）
    // 注意：mapToGlobal 是将「当前控件内的点」转换为屏幕坐标，必须用滑块自身的 mapToGlobal
        QPoint tickGlobalPos = this->mapToGlobal(tickLocalPos);

        // 4. 计算图标在屏幕上的绝对位置（对齐刻度下方）
        QSize iconSize = m_recIconWidget->size();
        int iconGlobalX = tickGlobalPos.x() - iconSize.width() / 2; // 水平居中刻度
        int iconGlobalY = tickGlobalPos.y() + m_recIconVerticalSpacing; // 垂直在刻度下方
        QPoint iconGlobalPos(iconGlobalX, iconGlobalY);

        return iconGlobalPos;
}

void SnapSlider::onTooltipAutoHideTimeout()
{
    // 只有鼠标不在滑块上时，才隐藏提示框（避免鼠标悬浮时误隐藏）
    QRect sliderRect = this->rect();
    QPoint mousePos = mapFromGlobal(QCursor::pos());
    if (!sliderRect.contains(mousePos)) {
        m_tooltipWidget->hide();
    }
}

double SnapSlider::getValueRatio(int value) {
    return (double)(value - minimum()) / (maximum() - minimum());
}

QRect SnapSlider::getAdjustedGrooveRect() {
    QStyleOptionSlider option;
    initStyleOption(&option);
    QRect grooveRect = style()->subControlRect(
                QStyle::CC_Slider, &option, QStyle::SC_SliderGroove, this
                );
    grooveRect.setHeight(m_grooveHeight);
    grooveRect.moveCenter(QPoint(grooveRect.center().x(), height()/2));

    // 左侧右侧预留滑块半径
    grooveRect.adjust(m_handleRadius, 0, -m_handleRadius, 0);
    return grooveRect;
}

QRect SnapSlider::getHandleRect() {
    QRect grooveRect = getAdjustedGrooveRect();
    int handleSize = m_handleRadius * 2;
    QRect handleRect(0, 0, handleSize, handleSize);

    double ratio = getValueRatio(value());
    // 计算X坐标（按比例精准定位，19和20各有对应位置）
    int x = grooveRect.left() + grooveRect.width() * ratio;
    // 滑块的x坐标
    x = qMin(x, grooveRect.right());

    handleRect.moveCenter(QPoint(x, height()/2));
    return handleRect;
}

int SnapSlider::getSnappedValue(int x) {
    QRect grooveRect = getAdjustedGrooveRect();

    // 1. 仅最右侧2px范围触发最大值
    if (x >= grooveRect.right() - 2) {
        return maximum();
    }
    // 2. 仅最左侧2px范围触发最小值
    if (x <= grooveRect.left() + 2) {
        return minimum();
    }

    // 3. 计算原始比例值
    double ratio = (double)(x - grooveRect.left()) / grooveRect.width();
    double rawValue = minimum() + (maximum() - minimum()) * ratio;

    // 4. 四舍五入吸附
    int snappedValue = qRound(rawValue);
    return qBound(minimum(), snappedValue, maximum());
}

void SnapSlider::drawTicks(QPainter &painter, const QRect &grooveRect) {
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#999999")); // 刻度颜色（浅灰，可调整）

    int totalTicks = maximum() - minimum() + 1; // 总刻度数
    double tickInterval = (double)grooveRect.width() / (totalTicks - 1); // 刻度间的水平间距

    // 遍历每个刻度（min-max）
    for (int i = 0; i < totalTicks; ++i) {
        // 计算每个刻度的X坐标（与对应值的位置对齐）
        int tickX = grooveRect.left() + i * tickInterval;
        // 计算刻度的Y坐标：滑条底部 + 间距5px
        int tickY = grooveRect.bottom() + m_tickSpacing;

        // 1. 计算当前刻度对应的数值
        int currentValue = minimum() + i;

        // 绘制刻度矩形（宽1px，高2px）
        double padding = (double)m_tickWidth / 2;
        QRect tickRectF(tickX - padding, tickY, m_tickWidth, m_tickHeight);
        painter.fillRect(tickRectF, m_tickColor);
    }
}

QString SnapSlider::getTipsStr()
{
    return QString("%1%2").arg(value()).arg(m_strUnit);
}

QPoint SnapSlider::getHandlerCenter()
{
    // 计算滑块中心在窗口中的绝对位置（关键：提示框需要窗口坐标）
    QRect handleRect = getHandleRect();
    QRect valRect = QRect(mapToGlobal(handleRect.topLeft()), handleRect.size());
    QPoint handleCenter = mapTo(this, valRect.center());
    return handleCenter;
}

RecommendIconWidget::RecommendIconWidget(QWidget *parent) : QWidget(parent) {
    // 关键：设置为顶层悬浮部件，脱离父控件布局限制
    setWindowFlags( Qt::FramelessWindowHint | Qt::Tool | Qt::WindowMinimizeButtonHint);
    setAttribute(Qt::WA_TransparentForMouseEvents); // 鼠标穿透，不影响滑块操作
    setAttribute(Qt::WA_TranslucentBackground);     // 透明背景
    setAttribute(Qt::WA_ShowWithoutActivating);     // 显示时不抢焦点
    // 默认三角形颜色（醒目红色，可自定义）
    m_iconColor = QColor("#FF6B6B");
}

void RecommendIconWidget::setIconColor(const QColor &color) { m_iconColor = color; update(); }

void RecommendIconWidget::setCustomIcon(const QPixmap &pixmap) { m_customPixmap = pixmap; update(); }

void RecommendIconWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 1. 优先绘制自定义图标
    if (!m_customPixmap.isNull()) {
        painter.drawPixmap(rect(), m_customPixmap.scaled(
                               rect().size(), Qt::KeepAspectRatio, Qt::SmoothTransformation
                               ));
        return;
    }

    // 2. 绘制默认三角形（朝上，指向刻度）
    QPainterPath trianglePath;
    // 三角形三个顶点：上中点（指向刻度）、左下、右下
    trianglePath.moveTo(rect().center().x(), rect().top());
    trianglePath.lineTo(rect().left(), rect().bottom());
    trianglePath.lineTo(rect().right(), rect().bottom());
    trianglePath.closeSubpath();

    painter.setBrush(m_iconColor);
    painter.setPen(Qt::NoPen);
    painter.drawPath(trianglePath);
}
