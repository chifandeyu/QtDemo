#include "snapslidertest.h"
#include "ui_snapslidertest.h"

SnapSliderTest::SnapSliderTest(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SnapSliderTest)
{
    ui->setupUi(this);
    ui->m_pSlider->setValue(1); // 默认值
    QColor handleColor = QColor("#0072FF");
    ui->m_pSlider->setHandleColor(handleColor, handleColor);
    ui->m_pSlider->setSelectColor(handleColor, QColor("#F5F5F5"));
}

SnapSliderTest::~SnapSliderTest()
{
    delete ui;
}

