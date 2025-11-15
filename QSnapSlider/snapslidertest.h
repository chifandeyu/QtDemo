#ifndef SNAPSLIDERTEST_H
#define SNAPSLIDERTEST_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class SnapSliderTest; }
QT_END_NAMESPACE

class SnapSliderTest : public QWidget
{
    Q_OBJECT

public:
    SnapSliderTest(QWidget *parent = nullptr);
    ~SnapSliderTest();

private:
    Ui::SnapSliderTest *ui;
};
#endif // SNAPSLIDERTEST_H
