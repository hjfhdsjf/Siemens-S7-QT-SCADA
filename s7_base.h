#ifndef S7_BASE_H
#define S7_BASE_H

#include <QString>
#include <QByteArray>
#include <Lib/snap7.h>

class S7_BASE
{
public:
    S7_BASE();
    ~S7_BASE();

    bool Connect(const QString &ip, int rack, int slot);
    void Disconnect();
    bool isConnected();

    // 支持多区域操作
    bool ReadBytes(int area, int dbNumber, int startByte, quint8 *buffer, size_t size);
    bool WriteBytes(int area, int dbNumber, int startByte, const quint8 *buffer, size_t size);

    // 对应不同数据类型的读写
    bool ReadBool(int area, int dbNumber, int startByte, int bitPosition);
    bool WriteBool(int area, int dbNumber, int startByte, int bitPosition, bool value);

    int ReadInt(int area, int dbNumber, int startByte);
    bool WriteInt(int area, int dbNumber, int startByte, int value);

    float ReadFloat(int area, int dbNumber, int startByte);
    bool WriteFloat(int area, int dbNumber, int startByte, float value);

    QString ReadString(int area, int dbNumber, int startByte, quint16 maxLength);
    bool WriteString(int area, int dbNumber, int startByte, const QString &value, quint16 maxLength);

    char ReadChar(int area, int dbNumber, int startByte);
    bool WriteChar(int area, int dbNumber, int startByte, char value);

private:
    S7Object client;  // S7客户端对象
    bool connected;  // 是否已连接
};

#endif

