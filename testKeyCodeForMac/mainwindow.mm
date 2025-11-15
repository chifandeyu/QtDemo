#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QKeyEvent>
#include <QTextStream>

#ifdef Q_OS_MAC
#import <Cocoa/Cocoa.h>
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // ui->textEdit->setReadOnly(true);
    // 在 macOS 上使用 NSEvent 监听键盘事件
#ifdef Q_OS_MAC
    keyDownMonitor = (void*) [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown handler:^NSEvent * (NSEvent *event) {
        unsigned short keyCode = [event keyCode];
        QString text = QString("key pressed: %1").arg(keyCode);
        qDebug() << text;
        ui->textEdit->append(text);
        if ([event modifierFlags] & NSEventModifierFlagCommand) {
            // 检查按下的键是否是 Insert 键 (macOS 没有直接的 Insert 键，通常使用 F13 代替)
            if ([[event charactersIgnoringModifiers] isEqualToString:@"\uF710"]) { // \uF710 对应 F13 键
                qDebug() << "Insert key pressed";
            }
        }
        return event;
    }];
    if (keyDownMonitor) {
      ui->textEdit->append("listen key down...");
    }

    keyUpMonitor = (void*) [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyUp handler:^NSEvent * (NSEvent *event) {
        unsigned short keyCode = [event keyCode];
        QString text = QString("key relese: %1").arg(keyCode);
        qDebug() << text;
        ui->textEdit->append(text);
        if ([event modifierFlags] & NSEventModifierFlagCommand) {
            // 检查释放的键是否是 Insert 键
            if ([[event charactersIgnoringModifiers] isEqualToString:@"\uF710"]) {
                qDebug() << "Insert key released";
            }
        }
        return event;
    }];
    if (keyUpMonitor) {
      ui->textEdit->append("listen key up...");
    }
    ui->textEdit->append("******* init");
#endif
}

MainWindow::~MainWindow()
{
    // 移除监听器
    if (keyDownMonitor) {
        [NSEvent removeMonitor:(id)keyDownMonitor];  // 将 void* 转换回 id
        keyDownMonitor = nullptr;
      ui->textEdit->append("remove listen key up...");
    }
    if (keyUpMonitor) {
        [NSEvent removeMonitor:(id)keyUpMonitor];  // 将 void* 转换回 id
        keyUpMonitor = nullptr;
      ui->textEdit->append("remove listen key up...");
    }
    delete ui;
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
  QString debugOutput;
  QDebug debug(&debugOutput);
  debug << "Qt key press event = " << event << "\n=================";
  ui->textEdit->append(debugOutput);
  return QMainWindow::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
  QString debugOutput;
  QDebug debug(&debugOutput);
  debug << "Qt key release event = " << event << "\n=================";
  ui->textEdit->append(debugOutput);
  return QMainWindow::keyReleaseEvent(event);
}
