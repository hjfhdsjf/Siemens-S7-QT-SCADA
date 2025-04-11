#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include "s7_base.h"

QT_BEGIN_NAMESPACE
namespace Ui { class S7Tester; }
QT_END_NAMESPACE

// 测试用结构体（保持1字节对齐）
#pragma pack(push, 1)
struct TestStruct {
    qint16 id;
    float value;
    bool status;
    quint8 checksum;
};
#pragma pack(pop)

class S7Tester : public QWidget
{
    Q_OBJECT

public:
    explicit S7Tester(QWidget *parent = nullptr);
    ~S7Tester();

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void testBoolOperations();
    void testIntOperations();
    void testFloatOperations();
    void testStringOperations();
    void testStructOperations();
    void testBatchOperations();

private:
    Ui::S7Tester *ui;
    S7_BASE *s7;
    void log(const QString &message);
    bool checkConnection();
};

#endif // MAINWINDOW_H
