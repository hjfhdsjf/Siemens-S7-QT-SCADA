/******************************************************************************
 * @file    s7_base.cpp
 * @brief   S7通信协议基础功能模块 对西门子1200、1500系列PLC数据的读写
 *
 * @details
 * 功能描述：
 *    - 支持西门子1200、1500系列PLC的STRING、INT、BOOL、CHAR、FLOAT数据读写
 *    - 支持西门子1200、1500系列PLC的连接
 *    - 支持DB/I/Q/M等存储区操作
 *
 * @author  Magic
 * @date    2024-03-10 创建
 * @date    2024-03-11 最后修改
 *
 * @note
 * - 修改记录：
 *   2025-4-10 实现基础功能V1.0
 *
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

#include "s7_base.h"
#include <QByteArray>
#include <QtEndian>
#include <QDebug>

S7_BASE::S7_BASE()
{
    connected = false;
    client = Cli_Create();
}

S7_BASE::~S7_BASE()
{
    Disconnect();
    if(client) {
        Cli_Destroy(&client);
    }
}

// 建立连接
bool S7_BASE::Connect(const QString &ip, int rack, int slot)
{
    if(!client) return false;

    // 使用ConnectTo进行连接
    int result = Cli_ConnectTo(client,
                               ip.toLatin1().constData(),
                               rack,
                               slot);

    connected = (result == 0);
    return connected;
}

// 断开连接
void S7_BASE::Disconnect()
{
    if(client && connected) {
        Cli_Disconnect(client);
        connected = false;
    }
}

//连接状态判断
bool S7_BASE::isConnected(){
    return (connected == 0);
}

// 基础字节读写实现，支持所有区域（I, Q, M, DB等）
bool S7_BASE::ReadBytes(int area, int dbNumber, int startByte, quint8 *buffer, size_t size)
{
    if(!client || !connected) return false;

    return Cli_ReadArea(client,
                        area,
                        dbNumber,
                        startByte,
                        static_cast<int>(size),
                        S7WLByte,
                        buffer) == 0;
}

bool S7_BASE::WriteBytes(int area, int dbNumber, int startByte, const quint8 *buffer, size_t size)
{
    if(!client || !connected) return false;

    return Cli_WriteArea(client,
                         area,
                         dbNumber,
                         startByte,
                         static_cast<int>(size),
                         S7WLByte,
                         const_cast<quint8*>(buffer)) == 0;
}

// 读取bool
bool S7_BASE::ReadBool(int area, int dbNumber, int startByte, int bitPosition)
{
    quint8 buffer;
    if(ReadBytes(area, dbNumber, startByte, &buffer, 1)) {
        return (buffer & (1 << bitPosition)) != 0;
    }
    return false;
}

// 写入bool
bool S7_BASE::WriteBool(int area, int dbNumber, int startByte, int bitPosition, bool value)
{
    quint8 buffer;
    if(!ReadBytes(area, dbNumber, startByte, &buffer, 1))
        return false;

    if(value)
        buffer |= (1 << bitPosition);
    else
        buffer &= ~(1 << bitPosition);

    return WriteBytes(area, dbNumber, startByte, &buffer, 1);
}

// 读取int
int S7_BASE::ReadInt(int area, int dbNumber, int startByte)
{
    qint16 buffer;
    if(ReadBytes(area, dbNumber, startByte, reinterpret_cast<quint8*>(&buffer), 2)) {
        return qFromBigEndian(buffer);  // 西门子使用大端字节序
    }
    return 0;
}

// 写入int
bool S7_BASE::WriteInt(int area, int dbNumber, int startByte, int value)
{
    qint16 val = qToBigEndian(static_cast<qint16>(value));
    return WriteBytes(area, dbNumber, startByte, reinterpret_cast<quint8*>(&val), 2);
}

// 读取float
float S7_BASE::ReadFloat(int area, int dbNumber, int startByte)
{
    quint32 buffer;
    if(ReadBytes(area, dbNumber, startByte, reinterpret_cast<quint8*>(&buffer), 4)) {
        buffer = qFromBigEndian(buffer);
        return *reinterpret_cast<float*>(&buffer);
    }
    return 0.0f;
}

// 写入float
bool S7_BASE::WriteFloat(int area, int dbNumber, int startByte, float value)
{
    quint32 buffer = qToBigEndian(*reinterpret_cast<quint32*>(&value));
    return WriteBytes(area, dbNumber, startByte, reinterpret_cast<quint8*>(&buffer), 4);
}

//读string
QString S7_BASE::ReadString(int area, int dbNumber, int startByte, quint16 maxLength)
{
    QByteArray buffer(maxLength + 2, 0);
    if(ReadBytes(area, dbNumber, startByte, reinterpret_cast<quint8*>(buffer.data()), buffer.size())) {
        quint16 currentLength = static_cast<quint8>(buffer[1]); // 从第二个字节获取当前长度
        return QString::fromLatin1(buffer.constData() + 2, qMin(currentLength, maxLength));
    }
    return QString();
}

// 写string
bool S7_BASE::WriteString(int area, int dbNumber, int startByte, const QString &value, quint16 maxLength)
{
    QByteArray buffer(maxLength + 2, 0);
    buffer[0] = static_cast<char>(maxLength);   // 第一个字节为最大长度
    buffer[1] = static_cast<char>(value.length()); // 第二个字节为当前长度
    QByteArray strData = value.left(maxLength).toLatin1();
    memcpy(buffer.data() + 2, strData.constData(), strData.size());
    return WriteBytes(area, dbNumber, startByte, reinterpret_cast<quint8*>(buffer.data()), buffer.size());
}

// 读取char
char S7_BASE::ReadChar(int area, int dbNumber, int startByte)
{
    quint8 buffer;
    if(ReadBytes(area, dbNumber, startByte, &buffer, 1)) {
        return static_cast<char>(buffer);
    }
    return 0;
}

// 写入char
bool S7_BASE::WriteChar(int area, int dbNumber, int startByte, char value)
{
    quint8 buffer = static_cast<quint8>(value);
    return WriteBytes(area, dbNumber, startByte, &buffer, 1);
}

