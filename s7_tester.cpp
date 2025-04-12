/******************************************************************************
 * @file    s7_tester.cpp
 * @brief   测试程序，用于测试s7_base实现的对西门子1200、1500系列PLC读写操作
 *
 * @details
 * 功能描述：
 *    - 支持西门子PLC的STRING、INT、BOOL、CHAR、FLOAT数据读写
 *    - 支持西门子PLC的IP、机架、槽号的设置
 *    - 支持选择DB/I/Q/M存储区
 *    - 支持日志功能，对应的操作会输出在日志输入栏，日志支持不同颜色提示
 *    - 支持循环读写功能，可以设定不同区域、不同数据类型、采集间隔
 *
 * @author  Magic
 * @date    2024-03-10 创建
 * @date    2024-03-12 最后修改
 *
 * @note
 * - 修改记录：
 *   2025-4-10 实现基础功能 V1.0
 *   2025-4-12 增加停止plc后自动清除所有任务 V1.0.1
 *
 *         .--,       .--,
 *       ( (  \.---./  ) )
 *        '.__/o   o\__.'
 *           {=  ^  =}
 *            >  -  <
 *           /       \
 *          //       \\
 *         //|   .   |\\
 *         "'\       /'"_.-~^`'-.
 *            \  _  /--'         `
 *           ___)( )(___
 *          (((__) (__)))
 *  高山仰止,景行行止.虽不能至,心向往之。
 *****************************************************************************/

#include "s7_tester.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QIntValidator>
#include <QDoubleValidator>
#include <QDateTime>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QLabel>

// 设置中文编码，防止乱码
#pragma execution_character_set("utf-8")

//==========================================================
// TaskWorker子线程循环读实现
//==========================================================
TaskWorker::TaskWorker(int taskId, S7_BASE *s7Ptr, int areaCode, int dbNumber, int startByte, int bitOffset,
                       DataType dt, int interval, QObject *parent)
    : QObject(parent),
    m_taskId(taskId),
    s7(s7Ptr),
    area(areaCode),
    dbNum(dbNumber),
    startAddr(startByte),
    bitOffset(bitOffset),
    dataType(dt),
    intervalMs(interval)
{
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &TaskWorker::doRead);
}

TaskWorker::~TaskWorker()
{

}
//启动线程
void TaskWorker::start()
{
    timer->start(intervalMs);
}
//停止线程
void TaskWorker::stop()
{
    timer->stop();
    emit finished();
}

//线程工作
void TaskWorker::doRead()
{
    QString result;
    switch (dataType) {
    case DT_Int:
        result = QString("Int类型-偏移量:%1  获取值：%2").arg(startAddr).arg(s7->ReadInt(area, dbNum, startAddr));
        break;
    case DT_Bool: {
        bool b = s7->ReadBool(area, dbNum, startAddr, bitOffset);
        result = QString("Bool类型-偏移量:%1.%2  获取值：%3").arg(startAddr).arg(bitOffset).arg(b ? "TRUE" : "FALSE");
        break;
    }
    case DT_Float:
        result = QString("Float类型-偏移量:%1  获取值：%2").arg(startAddr).arg(s7->ReadFloat(area, dbNum, startAddr));
        break;
    case DT_String:
        result = QString("String类型-偏移量:%1  获取值：%2").arg(startAddr).arg(s7->ReadString(area, dbNum, startAddr, 20));
        break;
    case DT_Char: {
        char ch = s7->ReadChar(area, dbNum, startAddr);
        result = QString("Char类型-偏移量:%1  获取值：%2").arg(startAddr).arg(ch);
        break;
    }
    default:
        result = "未知数据类型";
        break;
    }
    emit newData(m_taskId, result);
}

//==========================================================
// S7_Tester 实现
//==========================================================
S7_Tester::S7_Tester(QWidget *parent)
    : QMainWindow(parent),
    s7(new S7_BASE),
    infoLogCount(0),
    taskLogCount(0)
{
    createUI();
    setWindowTitle(tr("S7助手_V1.0_by_Magic"));

    // 初始化可用任务编号为1到10
    for (int i = 1; i <= 10; ++i) {
        availableTaskIds.append(i);
    }
}

S7_Tester::~S7_Tester()
{
    // 停止并清理所有循环读任务
    for (auto &item : taskList) {
        if (item.worker)
            item.worker->stop();
        if (item.thread) {
            item.thread->quit();
            item.thread->wait();
            delete item.thread;
        }
    }
    delete s7;
}

void S7_Tester::createUI()
{
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout;

    // 左侧布局：包含原有控件和任务配置
    QVBoxLayout *leftLayout = new QVBoxLayout;

    // =========连接控件=========
    QGroupBox *grpConnection = new QGroupBox(tr("PLC参数设置"));
    QHBoxLayout *layoutConn = new QHBoxLayout;
    editIp = new QLineEdit;
    editIp->setPlaceholderText(tr("IP地址"));
    editIp->setMinimumWidth(120);
    editIp->setText("192.168.0.16");
    editRack = new QLineEdit;
    editRack->setPlaceholderText(tr("机架"));
    editRack->setText("0");
    editRack->setValidator(new QIntValidator(0, 100, this));
    editSlot = new QLineEdit;
    editSlot->setPlaceholderText(tr("插槽"));
    editSlot->setText("1");
    editSlot->setValidator(new QIntValidator(0, 100, this));
    btnConnect = new QPushButton(tr("连接"));
    btnDisconnect = new QPushButton(tr("断开"));
    layoutConn->addWidget(new QLabel(tr("IP:")));
    layoutConn->addWidget(editIp);
    layoutConn->addWidget(new QLabel(tr("Rack:")));
    layoutConn->addWidget(editRack);
    layoutConn->addWidget(new QLabel(tr("Slot:")));
    layoutConn->addWidget(editSlot);
    layoutConn->addWidget(btnConnect);
    layoutConn->addWidget(btnDisconnect);
    grpConnection->setLayout(layoutConn);

    // =========区域参数设置=========
    QGroupBox *grpArea = new QGroupBox(tr("区域参数设置"));
    QHBoxLayout *layoutArea = new QHBoxLayout;
    comboArea = new QComboBox;
    comboArea->addItem("DB");
    comboArea->addItem("Q");
    comboArea->addItem("I");
    comboArea->addItem("M");
    editDbNumber = new QLineEdit;
    editDbNumber->setPlaceholderText(tr("DB地址"));
    editDbNumber->setValidator(new QIntValidator(0, 9999, this));
    editStartByte = new QLineEdit;
    QRegularExpression regExp("^\\d+(\\.\\d+)?$");
    editStartByte->setValidator(new QRegularExpressionValidator(regExp, this));
    editStartByte->setPlaceholderText(tr("偏移量（如18.5）"));
    layoutArea->addWidget(new QLabel(tr("区域:")));
    layoutArea->addWidget(comboArea);
    layoutArea->addWidget(editDbNumber);
    layoutArea->addWidget(editStartByte);
    grpArea->setLayout(layoutArea);

    // =========string 操作控件=========
    QGroupBox *grpString = new QGroupBox(tr("string 读写"));
    QHBoxLayout *layoutString = new QHBoxLayout;
    editStringValue = new QLineEdit;
    editStringValue->setPlaceholderText(tr("写入字符串"));
    btnReadString = new QPushButton(tr("读 string"));
    btnWriteString = new QPushButton(tr("写 string"));
    labelStringResult = new QLabel;
    layoutString->addWidget(editStringValue);
    layoutString->addWidget(btnReadString);
    layoutString->addWidget(btnWriteString);
    grpString->setLayout(layoutString);

    // =========int 操作控件=========
    QGroupBox *grpInt = new QGroupBox(tr("int 读写"));
    QHBoxLayout *layoutInt = new QHBoxLayout;
    editIntValue = new QLineEdit;
    editIntValue->setPlaceholderText(tr("写入整数"));
    editIntValue->setValidator(new QIntValidator(this));
    btnReadInt = new QPushButton(tr("读 int"));
    btnWriteInt = new QPushButton(tr("写 int"));
    labelIntResult = new QLabel;
    layoutInt->addWidget(editIntValue);
    layoutInt->addWidget(btnReadInt);
    layoutInt->addWidget(btnWriteInt);
    grpInt->setLayout(layoutInt);

    // =========bool 操作控件=========
    QGroupBox *grpBool = new QGroupBox(tr("bool 读写"));
    QHBoxLayout *layoutBool = new QHBoxLayout;
    checkBoolValue = new QCheckBox(tr("勾选为TRUE"));
    btnReadBool = new QPushButton(tr("读 bool"));
    btnWriteBool = new QPushButton(tr("写 bool"));
    labelBoolResult = new QLabel;
    layoutBool->addWidget(checkBoolValue);
    layoutBool->addWidget(btnReadBool);
    layoutBool->addWidget(btnWriteBool);
    grpBool->setLayout(layoutBool);

    // =========char 操作控件=========
    QGroupBox *grpChar = new QGroupBox(tr("char 读写"));
    QHBoxLayout *layoutChar = new QHBoxLayout;
    editCharValue = new QLineEdit;
    editCharValue->setPlaceholderText(tr("写入 char"));
    editCharValue->setMaxLength(1);
    btnReadChar = new QPushButton(tr("读 char"));
    btnWriteChar = new QPushButton(tr("写 char"));
    labelCharResult = new QLabel;
    layoutChar->addWidget(editCharValue);
    layoutChar->addWidget(btnReadChar);
    layoutChar->addWidget(btnWriteChar);
    grpChar->setLayout(layoutChar);

    // =========float 操作控件=========
    QGroupBox *grpFloat = new QGroupBox(tr("float 读写"));
    QHBoxLayout *layoutFloat = new QHBoxLayout;
    editFloatValue = new QLineEdit;
    editFloatValue->setPlaceholderText(tr("写入 float"));
    editFloatValue->setValidator(new QDoubleValidator(this));
    btnReadFloat = new QPushButton(tr("读 float"));
    btnWriteFloat = new QPushButton(tr("写 float"));
    labelFloatResult = new QLabel;
    layoutFloat->addWidget(editFloatValue);
    layoutFloat->addWidget(btnReadFloat);
    layoutFloat->addWidget(btnWriteFloat);
    grpFloat->setLayout(layoutFloat);

    // 将原有各组控件依次加入左侧布局
    leftLayout->addWidget(grpConnection);
    leftLayout->addWidget(grpArea);
    leftLayout->addWidget(grpString);
    leftLayout->addWidget(grpInt);
    leftLayout->addWidget(grpBool);
    leftLayout->addWidget(grpChar);
    leftLayout->addWidget(grpFloat);

    // =========循环读任务控件=========
    QGroupBox *grpTask = new QGroupBox(tr("循环任务设定"));
    QVBoxLayout *layoutTask = new QVBoxLayout;
    // 上排任务配置区域
    QHBoxLayout *layoutTaskConfig = new QHBoxLayout;
    comboTaskArea = new QComboBox;
    comboTaskArea->addItem("DB");
    comboTaskArea->addItem("Q");
    comboTaskArea->addItem("I");
    comboTaskArea->addItem("M");
    // 新增任务用 DB号输入控件，仅 DB 区有效
    editTaskDbNumber = new QLineEdit;
    editTaskDbNumber->setPlaceholderText(tr("DB号"));
    editTaskDbNumber->setValidator(new QIntValidator(0, 9999, this));
    if(comboTaskArea->currentText() == "DB")
        editTaskDbNumber->setEnabled(true);
    else {
        editTaskDbNumber->setEnabled(false);
        editTaskDbNumber->clear();
    }
    editTaskStartByte = new QLineEdit;
    editTaskStartByte->setPlaceholderText(tr("偏移量（如18.5）"));
    comboTaskDataType = new QComboBox;
    comboTaskDataType->addItem("int");
    comboTaskDataType->addItem("bool");
    comboTaskDataType->addItem("float");
    comboTaskDataType->addItem("string");
    comboTaskDataType->addItem("char");
    editTaskInterval = new QLineEdit;
    editTaskInterval->setPlaceholderText(tr("间隔(ms)"));
    editTaskInterval->setValidator(new QIntValidator(1, 100000, this));
    layoutTaskConfig->addWidget(new QLabel(tr("区域:")));
    layoutTaskConfig->addWidget(comboTaskArea);
    layoutTaskConfig->addWidget(new QLabel(tr("DB号:")));
    layoutTaskConfig->addWidget(editTaskDbNumber);
    layoutTaskConfig->addWidget(editTaskStartByte);
    layoutTaskConfig->addWidget(new QLabel(tr("数据类型:")));
    layoutTaskConfig->addWidget(comboTaskDataType);
    layoutTaskConfig->addWidget(editTaskInterval);

    // 下排操作按钮和任务列表
    QHBoxLayout *layoutTaskOp = new QHBoxLayout;
    btnAddTask = new QPushButton(tr("添加任务"));
    btnStopTask = new QPushButton(tr("停止任务"));
    layoutTaskOp->addWidget(btnAddTask);
    layoutTaskOp->addWidget(btnStopTask);

    listTask = new QListWidget;
    listTask->setSelectionMode(QAbstractItemView::SingleSelection);

    layoutTask->addLayout(layoutTaskConfig);
    layoutTask->addLayout(layoutTaskOp);
    layoutTask->addWidget(listTask);
    grpTask->setLayout(layoutTask);

    leftLayout->addWidget(grpTask);
    leftLayout->addStretch();

    // =========日志输出（右侧）=========
    // 右侧布局修改为上下分割
    QVBoxLayout *rightLayout = new QVBoxLayout;

    // =========信息日志=========
    QGroupBox *grpInfoLog = new QGroupBox(tr("信息日志"));
    QVBoxLayout *layoutInfoLog = new QVBoxLayout;

    // 顶部布局（标题行右侧）
    QHBoxLayout *infoHeaderLayout = new QHBoxLayout;
    infoHeaderLayout->addStretch(); // 占位符推动元素到右侧
    grpInfoLog->setContentsMargins(10, -100, 10, 10);
    infoCountLabel = new QLabel(tr("总数: 0"));
    btnClearInfoLog = new QPushButton(tr("清空"));
    infoHeaderLayout->addWidget(infoCountLabel);
    infoHeaderLayout->addWidget(btnClearInfoLog);

    textLog = new QTextEdit;
    textLog->setReadOnly(true);
    layoutInfoLog->addLayout(infoHeaderLayout);
    layoutInfoLog->addWidget(textLog);
    grpInfoLog->setLayout(layoutInfoLog);

    // =========任务日志=========
    QGroupBox *grpTaskLog = new QGroupBox(tr("任务日志"));
    QVBoxLayout *layoutTaskLog = new QVBoxLayout;

    QHBoxLayout *taskHeaderLayout = new QHBoxLayout;
    taskHeaderLayout->addStretch();
    grpTaskLog->setContentsMargins(10, -100, 10, 10);
    taskCountLabel = new QLabel(tr("总数: 0"));
    btnClearTaskLog = new QPushButton(tr("清空"));
    taskHeaderLayout->addWidget(taskCountLabel);
    taskHeaderLayout->addWidget(btnClearTaskLog);

    taskLog = new QTextEdit;
    taskLog->setReadOnly(true);
    layoutTaskLog->addLayout(taskHeaderLayout);
    layoutTaskLog->addWidget(taskLog);
    grpTaskLog->setLayout(layoutTaskLog);

    // 设置上下比例（关键：拉伸因子2:1）
    rightLayout->addWidget(grpInfoLog, 2);
    rightLayout->addWidget(grpTaskLog, 1);

    // 修改主布局（保持左右1:1比例）
    mainLayout->addLayout(leftLayout);
    mainLayout->addLayout(rightLayout);


    mainLayout->setStretch(0, 1);
    mainLayout->setStretch(1, 1);

    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);

    // 信号与槽连接（原有部分）
    connect(btnConnect, &QPushButton::clicked, this, &S7_Tester::onConnectClicked);
    connect(btnDisconnect, &QPushButton::clicked, this, &S7_Tester::onDisconnectClicked);
    connect(btnReadString, &QPushButton::clicked, this, &S7_Tester::onReadStringClicked);
    connect(btnWriteString, &QPushButton::clicked, this, &S7_Tester::onWriteStringClicked);
    connect(btnReadInt, &QPushButton::clicked, this, &S7_Tester::onReadIntClicked);
    connect(btnWriteInt, &QPushButton::clicked, this, &S7_Tester::onWriteIntClicked);
    connect(btnReadBool, &QPushButton::clicked, this, &S7_Tester::onReadBoolClicked);
    connect(btnWriteBool, &QPushButton::clicked, this, &S7_Tester::onWriteBoolClicked);
    connect(btnReadChar, &QPushButton::clicked, this, &S7_Tester::onReadCharClicked);
    connect(btnWriteChar, &QPushButton::clicked, this, &S7_Tester::onWriteCharClicked);
    connect(btnReadFloat, &QPushButton::clicked, this, &S7_Tester::onReadFloatClicked);
    connect(btnWriteFloat, &QPushButton::clicked, this, &S7_Tester::onWriteFloatClicked);
    connect(comboArea, &QComboBox::currentTextChanged, this, &S7_Tester::onAreaChanged);

    // 任务相关信号连接
    connect(btnAddTask, &QPushButton::clicked, this, &S7_Tester::onAddTaskClicked);
    connect(btnStopTask, &QPushButton::clicked, this, &S7_Tester::onStopTaskClicked);
    connect(comboTaskArea, &QComboBox::currentTextChanged, this, &S7_Tester::onTaskAreaChanged);

    // 连接清空按钮信号槽
    connect(btnClearInfoLog, &QPushButton::clicked, this, &S7_Tester::onClearInfoLogClicked);
    connect(btnClearTaskLog, &QPushButton::clicked, this, &S7_Tester::onClearTaskLogClicked);
}

//————————————————————————————
// 区域映射
static int mapArea(const QString &areaStr)
{
    if(areaStr == "DB")
        return 0x84;
    else if(areaStr == "Q")
        return 0x82;
    else if(areaStr == "I")
        return 0x81;
    else if(areaStr == "M")
        return 0x83;
    return 0;
}

//————————————————————————————
// 信息日志输出函数：追加日志并加时间前缀
void S7_Tester::logMessage(const QString &msg, LogType type) {
    infoLogCount++; // 增加计数
    infoCountLabel->setText(tr("总数: %1").arg(infoLogCount));
    QString color;
    switch(type) {
    case Success: color = "#009900"; break;  // 绿色
    case Warning: color = "#FF6600"; break; // 橙色
    case Error: color = "#FF0000"; break;   // 红色
    default: color = "#000000";             // 黑色
    }

    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss");
    textLog->append(QString("<span style='color:%1'>[%2]%3</span> ")
                        .arg(color).arg(timeStr).arg(msg));
}

//————————————————————————————
// 任务日志输出函数：追加日志并加时间前缀
void S7_Tester::TaskMessage(const QString &msg, LogType type) {
    taskLogCount++; // 增加计数
    taskCountLabel->setText(tr("总数: %1").arg(taskLogCount));
    QString color;
    switch(type) {
    case Success: color = "#009900"; break;  // 绿色
    default: color = "#666666";              // 灰色
    }

    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss");
    taskLog->append(QString("<span style='color:%1'>[%2]%3</span> ")
                        .arg(color).arg(timeStr).arg(msg));
}

//————————————————————————————
// 日志输出总数
void S7_Tester::onClearInfoLogClicked()
{
    textLog->clear();
    infoLogCount = 0;
    infoCountLabel->setText(tr("总数: %1").arg(infoLogCount));
}

void S7_Tester::onClearTaskLogClicked()
{
    taskLog->clear();
    taskLogCount = 0;
    taskCountLabel->setText(tr("总数: %1").arg(taskLogCount));
}

//————————————————————————————
// 地址解析函数：允许小数点时将整数部分视为字节地址，小数部分为位偏移（0~7）
bool S7_Tester::parseAddress(const QString &address, int &byteAddr, int &bitOffset, bool allowBit)
{
    QString addr = address.trimmed();
    if (allowBit && !addr.contains('.')) {
        logMessage(tr("【错误】bool 类型地址必须包含小数点以指定位偏移，例如：1.7"),Error);
        return false;
    }
    if (addr.contains('.')) {
        if (!allowBit) {
            logMessage(tr("【错误】该数据类型地址不能包含小数点"),Error);
            return false;
        }
        QStringList parts = addr.split('.');
        if (parts.size() != 2 || parts[1].isEmpty()) {
            logMessage(tr("【错误】地址格式无效，示例：1.7"),Error);
            return false;
        }
        bool ok1, ok2;
        byteAddr = parts[0].toInt(&ok1);
        bitOffset = parts[1].toInt(&ok2);
        if (!ok1 || !ok2 || bitOffset < 0 || bitOffset > 7) {
            logMessage(tr("【错误】地址解析失败（字节范围0-N，位范围0-7）"),Error);
            return false;
        }
    } else {
        bool ok;
        byteAddr = addr.toInt(&ok);
        if (!ok) {
            logMessage(tr("【错误】无效的地址格式"),Error);
            return false;
        }
        bitOffset = 0;
    }
    return true;
}

//————————————————————————————
// PLC 连接槽函数
void S7_Tester::onConnectClicked()
{
    if (!isConnectClicked()){
        logMessage(tr("【提示】PLC已连接！！！"),Warning);
        return;
    }
    QString ip = editIp->text().trimmed();
    int rack = editRack->text().toInt();
    int slot = editSlot->text().toInt();
    if(s7->Connect(ip, rack, slot))
        logMessage(tr("【提示】PLC连接成功！"),Success);
    else
        logMessage(tr("【提示】PLC连接失败！"),Warning);
}

void S7_Tester::onDisconnectClicked()
{
    if (isConnectClicked()) { // 检查PLC是否未连接
        logMessage(tr("【提示】PLC未连接！"), Warning);
        return;
    }

    // 停止并清理所有循环读任务
    for (auto &item : taskList) {
        if (item.worker) {
            item.worker->stop();
        }
        if (item.thread) {
            item.thread->quit();
            item.thread->wait();
            delete item.thread;
            item.thread = nullptr;
        }
    }
    // 清空任务列表和界面列表
    taskList.clear();
    listTask->clear();

    // 重置可用任务ID为1-10
    availableTaskIds.clear();
    for (int i = 1; i <= 10; ++i) {
        availableTaskIds.append(i);
    }

    // 断开PLC连接
    s7->Disconnect();
    logMessage(tr("【提示】PLC已断开连接，所有任务已停止并清除！"), Warning);
}

bool S7_Tester::isConnectClicked(){
    return s7->isConnected();
}

//————————————————————————————
// string 读写槽函数
void S7_Tester::onReadStringClicked()
{

    if (isConnectClicked()){
        logMessage(tr("【提示】请先连接PLC！！！"),Warning);
        return;
    }
    int areaCode = mapArea(comboArea->currentText());
    int dbNumber = (comboArea->currentText() == "DB") ? editDbNumber->text().toInt() : 0;

    // 调用 parseAddress() 检查输入是否合法。对于非 bool 类型，不允许含小数点
    int byteAddr = 0, bitOffset = 0;
    if (!parseAddress(editStartByte->text(), byteAddr, bitOffset, false)) return;

    QString result = s7->ReadString(areaCode, dbNumber, byteAddr, 20);
    logMessage(tr("【提示】读 string：%1").arg(result),Info);
}

void S7_Tester::onWriteStringClicked()
{

    if (isConnectClicked()){
        logMessage(tr("【提示】请先连接PLC！！！"),Warning);
        return;
    }
    int areaCode = mapArea(comboArea->currentText());
    int dbNumber = (comboArea->currentText() == "DB") ? editDbNumber->text().toInt() : 0;

    // 调用 parseAddress() 检查输入是否合法。对于非 bool 类型，不允许含小数点
    int byteAddr = 0, bitOffset = 0;
    if (!parseAddress(editStartByte->text(), byteAddr, bitOffset, false)) return;

    QString value = editStringValue->text();
    if(s7->WriteString(areaCode, dbNumber, byteAddr, value, 20))
        logMessage(tr("【提示】写 string 成功，值：%1").arg(value),Info);
    else
        logMessage(tr("【提示】写 string 失败，值：%1").arg(value),Warning);
}

//————————————————————————————
// int 读写槽函数
void S7_Tester::onReadIntClicked()
{

    if (isConnectClicked()){
        logMessage(tr("【提示】请先连接PLC！！！"),Warning);
        return;
    }
    int areaCode = mapArea(comboArea->currentText());
    int dbNumber = (comboArea->currentText() == "DB") ? editDbNumber->text().toInt() : 0;

    // 调用 parseAddress() 检查输入是否合法。对于非 bool 类型，不允许含小数点
    int byteAddr = 0, bitOffset = 0;
    if (!parseAddress(editStartByte->text(), byteAddr, bitOffset, false)) return;

    int value = s7->ReadInt(areaCode, dbNumber, byteAddr);
    logMessage(tr("【提示】读 int：%1").arg(value),Info);
}

void S7_Tester::onWriteIntClicked()
{

    if (isConnectClicked()){
        logMessage(tr("【提示】请先连接PLC！！！"),Warning);
        return;
    }
    int areaCode = mapArea(comboArea->currentText());
    int dbNumber = (comboArea->currentText() == "DB") ? editDbNumber->text().toInt() : 0;

    // 调用 parseAddress() 检查输入是否合法。对于非 bool 类型，不允许含小数点
    int byteAddr = 0, bitOffset = 0;
    if (!parseAddress(editStartByte->text(), byteAddr, bitOffset, false)) return;

    int value = editIntValue->text().toInt();
    if(s7->WriteInt(areaCode, dbNumber, byteAddr, value))
        logMessage(tr("【提示】写 int 成功，值：%1").arg(value),Info);
    else
        logMessage(tr("【提示】写 int 失败，值：%1").arg(value),Warning);
}

//————————————————————————————
// bool 读写槽函数
void S7_Tester::onReadBoolClicked()
{

    if (isConnectClicked()){
        logMessage(tr("【提示】请先连接PLC！！！"),Warning);
        return;
    }
    int areaCode = mapArea(comboArea->currentText());
    int dbNumber = (comboArea->currentText() == "DB") ? editDbNumber->text().toInt() : 0;

    int startByte, bitPos;

    if (!parseAddress(editStartByte->text(), startByte, bitPos, true)) return;

    bool value = s7->ReadBool(areaCode, dbNumber, startByte, bitPos);
    logMessage(tr("【提示】读 bool：%1").arg(value ? "TRUE" : "FALSE"),Info);
}

void S7_Tester::onWriteBoolClicked()
{

    if (isConnectClicked()){
        logMessage(tr("【提示】请先连接PLC！！！"),Warning);
        return;
    }
    int areaCode = mapArea(comboArea->currentText());
    int dbNumber = (comboArea->currentText() == "DB") ? editDbNumber->text().toInt() : 0;
    int startByte, bitPos;

    if (!parseAddress(editStartByte->text(), startByte, bitPos, true)) return;

    bool value = checkBoolValue->isChecked();
    if(s7->WriteBool(areaCode, dbNumber, startByte, bitPos, value))
        logMessage(tr("【提示】写 bool 成功，值：%1").arg(value ? "TRUE" : "FALSE"),Info);
    else
        logMessage(tr("【提示】写 bool 失败"),Warning);
}

//————————————————————————————
// char 读写槽函数
void S7_Tester::onReadCharClicked()
{

    if (isConnectClicked()){
        logMessage(tr("【提示】请先连接PLC！！！"),Warning);
        return;
    }
    int areaCode = mapArea(comboArea->currentText());
    int dbNumber = (comboArea->currentText() == "DB") ? editDbNumber->text().toInt() : 0;

    // 调用 parseAddress() 检查输入是否合法。对于非 bool 类型，不允许含小数点
    int byteAddr = 0, bitOffset = 0;
    if (!parseAddress(editStartByte->text(), byteAddr, bitOffset, false)) return;

    char ch = s7->ReadChar(areaCode, dbNumber, byteAddr);
    logMessage(tr("【提示】读 char：%1").arg(ch),Info);
}

void S7_Tester::onWriteCharClicked()
{

    if (isConnectClicked()){
        logMessage(tr("【提示】请先连接PLC！！！"),Warning);
        return;
    }
    int areaCode = mapArea(comboArea->currentText());
    int dbNumber = (comboArea->currentText() == "DB") ? editDbNumber->text().toInt() : 0;

    // 调用 parseAddress() 检查输入是否合法。对于非 bool 类型，不允许含小数点
    int byteAddr = 0, bitOffset = 0;
    if (!parseAddress(editStartByte->text(), byteAddr, bitOffset, false)) return;

    QString str = editCharValue->text();
    if(str.isEmpty()) {
        logMessage(tr("【错误】字符不能为空"),Error);
        return;
    }
    char ch = str.at(0).toLatin1();
    if(s7->WriteChar(areaCode, dbNumber, byteAddr, ch))
        logMessage(tr("【提示】写 char 成功，值：%1").arg(ch),Info);
    else
        logMessage(tr("【提示】写 char 失败，值：%1").arg(ch),Warning);
}

//————————————————————————————
// float 读写槽函数
void S7_Tester::onReadFloatClicked()
{

    if (isConnectClicked()){
        logMessage(tr("【提示】请先连接PLC！！！"),Warning);
        return;
    }
    int areaCode = mapArea(comboArea->currentText());
    int dbNumber = (comboArea->currentText() == "DB") ? editDbNumber->text().toInt() : 0;

    // 调用 parseAddress() 检查输入是否合法。对于非 bool 类型，不允许含小数点
    int byteAddr = 0, bitOffset = 0;
    if (!parseAddress(editStartByte->text(), byteAddr, bitOffset, false)) return;

    float value = s7->ReadFloat(areaCode, dbNumber, byteAddr);
    logMessage(tr("【提示】读 float：%1").arg(value),Info);
}

void S7_Tester::onWriteFloatClicked()
{

    if (isConnectClicked()){
        logMessage(tr("【提示】请先连接PLC！！！"),Warning);
        return;
    }
    int areaCode = mapArea(comboArea->currentText());
    int dbNumber = (comboArea->currentText() == "DB") ? editDbNumber->text().toInt() : 0;

    // 调用 parseAddress() 检查输入是否合法。对于非 bool 类型，不允许含小数点
    int byteAddr = 0, bitOffset = 0;
    if (!parseAddress(editStartByte->text(), byteAddr, bitOffset, false)) return;

    float value = editFloatValue->text().toFloat();
    if(s7->WriteFloat(areaCode, dbNumber, byteAddr, value))
        logMessage(tr("【提示】写 float 成功，值：%1").arg(value),Info);
    else
        logMessage(tr("【提示】写 float 失败，值：%1").arg(value),Warning);
}

//————————————————————————————
// 当主界面区域选择变化时：若为 DB 则启用 DB 号输入框，否则禁用
void S7_Tester::onAreaChanged(const QString &text)
{
    if(text == "DB") {
        editDbNumber->setEnabled(true);
        editDbNumber->setPlaceholderText(tr("DB号"));
    } else {
        editDbNumber->setEnabled(false);
        editDbNumber->clear();
    }
}

//————————————————————————————
// 当任务区域选择变化时，调整任务专用 DB 号输入框（仅 DB 区启用）
void S7_Tester::onTaskAreaChanged(const QString &text)
{
    if(text == "DB") {
        editTaskDbNumber->setEnabled(true);
        editTaskDbNumber->setPlaceholderText(tr("DB号"));
    } else {
        editTaskDbNumber->setEnabled(false);
        editTaskDbNumber->clear();
    }
}

//————————————————————————————
// 循环读任务：添加任务槽函数，禁止添加相同任务
void S7_Tester::onAddTaskClicked()
{
    if (isConnectClicked()){
        logMessage(tr("【提示】请先连接PLC！！！"),Warning);
        return;
    }

    if(taskList.size() >= 10) {
        QMessageBox::warning(this, tr("警告"), tr("最多允许添加10个循环读任务"));
        return;
    }

    // 获取任务区域
    QString areaStr = comboTaskArea->currentText();
    int areaCode = mapArea(areaStr);

    // 对于 DB 区，使用任务专用 DB 号输入，否则 dbNumber 为 0
    int dbNumber = (areaStr == "DB") ? editTaskDbNumber->text().toInt() : 0;

    // 判断数据类型
    DataType dt;
    QString typeStr = comboTaskDataType->currentText();
    if(typeStr == "int")
        dt = DT_Int;
    else if(typeStr == "bool")
        dt = DT_Bool;
    else if(typeStr == "float")
        dt = DT_Float;
    else if(typeStr == "string")
        dt = DT_String;
    else
        dt = DT_Char;

    // 解析任务起始地址，允许小数点仅在 bool 类型时
    int byteAddr = 0, bitOffset = 0;
    bool allowBit = (dt == DT_Bool);
    if(!parseAddress(editTaskStartByte->text(), byteAddr, bitOffset, allowBit))
        return;
    int interval = editTaskInterval->text().toInt();
    if (interval<1){
        logMessage(tr("【警告】时间间隔范围1ms-100000ms之间！"),Error);
        return;
    }

    // 检查是否已存在相同任务（区域、数据类型、起始字节、位偏移；对于 DB 区还要 DB 号相同）
    for(const TaskItem &item : taskList) {
        TaskWorker *worker = item.worker;
        if(worker) {
            if(worker->dataType == dt &&
                worker->area == areaCode &&
                worker->startAddr == byteAddr &&
                worker->bitOffset == bitOffset)
            {
                if(areaStr != "DB" || worker->dbNum == dbNumber) {
                    QMessageBox::warning(this, tr("警告"), tr("不能添加相同的任务"));
                    return;
                }
            }
        }
    }

    // 检查是否还有可用编号
    if (availableTaskIds.isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("最多允许添加10个循环读任务"));
        return;
    }

    // 取出最小可用编号
    int taskId = availableTaskIds.takeFirst();

    // 创建新的任务对象，并传入解析后的起始地址和位偏移
    TaskWorker *worker = new TaskWorker(taskId, s7, areaCode, dbNumber, byteAddr, bitOffset, dt, interval);
    QThread *thread = new QThread;
    worker->moveToThread(thread);
    connect(thread, &QThread::started, worker, &TaskWorker::start);
    connect(worker, &TaskWorker::newData, this,
            [this](int taskId, const QString &msg) {
                onTaskNewData(taskId, msg);
            });
    connect(worker, &TaskWorker::finished, this, &S7_Tester::onTaskFinished);
    connect(worker, &TaskWorker::finished, worker, &TaskWorker::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();

    TaskItem item;
    item.thread = thread;
    item.worker = worker;
    item.taskId = taskId;
    item.areaStr = areaStr;
    item.dbNumber = dbNumber;
    item.startByteStr = editTaskStartByte->text();
    item.typeStr = typeStr;
    item.interval = interval;
    item.executionCount = 0; // 初始次数为0
    taskList.append(item);

    // 生成带次数的任务描述并添加到列表
    QString taskDesc = QString("【任务%1】区域:%2").arg(taskId).arg(areaStr);
    if (areaStr == "DB")
        taskDesc.append(QString(" DB号:%1").arg(dbNumber));
    taskDesc.append(QString(" 起始:%1").arg(editTaskStartByte->text()));
    taskDesc.append(QString(" 类型:%1 间隔:%2ms").arg(typeStr).arg(interval));
    taskDesc.append(QString(" 已执行次数：0")); // 初始次数

    QListWidgetItem *listItem = new QListWidgetItem(taskDesc);
    listItem->setData(Qt::UserRole, taskId); // 存储taskId到项的data中
    listTask->addItem(listItem);

    logMessage(tr("【提示】添加任务成功：%1").arg(taskDesc),Info);
}

//————————————————————————————
// 循环读任务：停止任务槽函数
void S7_Tester::onStopTaskClicked()
{
    int currentRow = listTask->currentRow();
    if(currentRow < 0 || currentRow >= taskList.size()){
        QMessageBox::warning(this, tr("提示"), tr("请选择要停止的任务"));
        return;
    }
    TaskItem item = taskList.takeAt(currentRow);
    availableTaskIds.append(item.taskId);
    std::sort(availableTaskIds.begin(), availableTaskIds.end()); // 保持编号有序
    if(item.worker)
        item.worker->stop();
    if(item.thread) {
        item.thread->quit();
        item.thread->wait();
    }
    delete listTask->takeItem(currentRow);
    logMessage(tr("【提示】任务%1 已停止").arg(item.taskId),Info);
}

//————————————————————————————
// 循环读任务：任务新数据槽函数
void S7_Tester::onTaskNewData(int taskId, const QString &msg)
{
    // 查找对应的TaskItem
    for (int i = 0; i < taskList.size(); ++i) {
        TaskItem &item = taskList[i];
        if (item.taskId == taskId) {
            item.executionCount++; // 递增次数

            // 更新列表项的文本
            for (int j = 0; j < listTask->count(); ++j) {
                QListWidgetItem *listItem = listTask->item(j);
                if (listItem->data(Qt::UserRole).toInt() == taskId) {
                    // 重新生成带次数的描述
                    QString newDesc = QString("【任务%1】区域:%2").arg(taskId).arg(item.areaStr);
                    if (item.areaStr == "DB")
                        newDesc.append(QString("  DB地址:%1").arg(item.dbNumber));
                    newDesc.append(QString("  偏移量:%1").arg(item.startByteStr));
                    newDesc.append(QString("  类型:%1 间隔:%2ms").arg(item.typeStr).arg(item.interval));
                    newDesc.append(QString("  执行次数：%1").arg(item.executionCount));
                    listItem->setText(newDesc);
                    break;
                }
            }

            TaskMessage(tr("任务%1: %2").arg(taskId).arg(msg), Info);
            break;
        }
    }
}

//————————————————————————————
// 循环读任务：任务结束槽函数
void S7_Tester::onTaskFinished()
{
    logMessage(tr("【提示】任务结束"),Info);
}

