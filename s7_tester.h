#ifndef S7_TESTER_H
#define S7_TESTER_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QComboBox>
#include <QTextEdit>
#include <QThread>
#include <QTimer>
#include <QListWidget>
#include "s7_base.h"



// 支持的数据类型枚举
enum DataType {
    DT_Int,
    DT_Bool,
    DT_Float,
    DT_String,
    DT_Char
};

// 任务工作类，用于在独立线程中循环读取数据
class TaskWorker : public QObject
{
    Q_OBJECT
public:
    // 构造函数用于 bool 类型任务
    TaskWorker(int taskId, S7_BASE *s7Ptr, int areaCode, int dbNumber, int startByte, int bitOffset,
               DataType dt, int interval, QObject *parent = nullptr);
    ~TaskWorker();
    void start();
    void stop();
    static int mapArea(const QString &areaStr);

    // 用于任务重复判断
    int area;
    int dbNum;
    int startAddr;
    int bitOffset;
    DataType dataType;

signals:
    void newData(int taskId, const QString &msg);
    void finished();

private slots:
    void doRead();

private:
    int m_taskId;
    S7_BASE *s7;
    int intervalMs;
    QTimer *timer;
};

// 存储任务对象及对应线程信息
struct TaskItem {
    QThread *thread;
    TaskWorker *worker;
    int taskId;
    QString areaStr;      // 区域字符串，如"DB"
    int dbNumber;         // DB号（仅DB区域有效）
    QString startByteStr; // 起始地址字符串，如"18.5"
    QString typeStr;      // 数据类型字符串，如"int"
    int interval;         // 间隔时间（毫秒）
    int executionCount;   // 执行次数
};

class S7_Tester : public QMainWindow
{
    Q_OBJECT
public:
    explicit S7_Tester(QWidget *parent = nullptr);
    ~S7_Tester();

    //日志颜色枚举
    enum LogType {
        Info,       // 默认信息（黑色）
        Success,    // 成功（绿色）
        Warning,    // 警告（橙色）
        Error,      // 错误（红色）
    };

private slots:
    void onClearInfoLogClicked();
    void onClearTaskLogClicked();
    // 连接与断开PLC
    void onConnectClicked();
    void onDisconnectClicked();
    bool isConnectClicked();
    // string 的读写
    void onReadStringClicked();
    void onWriteStringClicked();
    // int 的读写
    void onReadIntClicked();
    void onWriteIntClicked();
    // bool 的读写
    void onReadBoolClicked();
    void onWriteBoolClicked();
    // char 的读写
    void onReadCharClicked();
    void onWriteCharClicked();
    // float 的读写
    void onReadFloatClicked();
    void onWriteFloatClicked();

    // 区域选择变化时调整主界面 DB 号输入框是否可编辑
    void onAreaChanged(const QString &text);

    // 循环读任务相关槽
    void onAddTaskClicked();
    void onStopTaskClicked();
    void onTaskNewData(int taskId, const QString &msg);
    void onTaskFinished();

    // 当任务区域选择变化时，调整任务专用 DB 号输入框（仅 DB 区启用）
    void onTaskAreaChanged(const QString &text);

private:
    void createUI();
    void logMessage(const QString &msg, LogType type);
    void S7_Tester::TaskMessage(const QString &msg, LogType type);
    // 地址解析：允许小数点时返回字节地址和位偏移
    bool parseAddress(const QString &address, int &byteAddr, int &bitOffset, bool allowBit = false);

    S7_BASE *s7;

    QList<int> availableTaskIds; // 可用任务编号池（1-10）

    // 连接相关控件
    QLineEdit   *editIp;
    QLineEdit   *editRack;
    QLineEdit   *editSlot;
    QPushButton *btnConnect;
    QPushButton *btnDisconnect;

    // 主界面区域设置参数
    QComboBox   *comboArea;
    QLineEdit   *editDbNumber;
    QLineEdit   *editStartByte;

    // string 操作控件
    QLineEdit   *editStringValue;
    QPushButton *btnReadString;
    QPushButton *btnWriteString;
    QLabel      *labelStringResult;

    // int 操作控件
    QLineEdit   *editIntValue;
    QPushButton *btnReadInt;
    QPushButton *btnWriteInt;
    QLabel      *labelIntResult;

    // bool 操作控件
    QCheckBox   *checkBoolValue;
    QPushButton *btnReadBool;
    QPushButton *btnWriteBool;
    QLabel      *labelBoolResult;

    // char 操作控件
    QLineEdit   *editCharValue;
    QPushButton *btnReadChar;
    QPushButton *btnWriteChar;
    QLabel      *labelCharResult;

    // float 操作控件
    QLineEdit   *editFloatValue;
    QPushButton *btnReadFloat;
    QPushButton *btnWriteFloat;
    QLabel      *labelFloatResult;

    // 日志信息输出控件
    QTextEdit   *textLog;
    QTextEdit   *taskLog;    //任务日志

    // 循环读任务控件
    QComboBox   *comboTaskArea;
    QLineEdit   *editTaskDbNumber; // 仅当任务区域为 DB 时有效
    QLineEdit   *editTaskStartByte;
    QComboBox   *comboTaskDataType;
    QLineEdit   *editTaskInterval;
    QPushButton *btnAddTask;
    QPushButton *btnStopTask;
    QListWidget *listTask;

    // 存储任务对象（最多允许10个任务）
    QList<TaskItem> taskList;

    //
    int infoLogCount;
    int taskLogCount;
    QLabel *infoCountLabel;
    QLabel *taskCountLabel;
    QPushButton *btnClearInfoLog;
    QPushButton *btnClearTaskLog;

};

#endif
