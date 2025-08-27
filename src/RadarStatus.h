// RadarStatus.h
#pragma once

#include <QtGlobal>
#include <QByteArray>
#include <QString>

// 依据“表24 雷达状态报文”定义的解析结果（假定小端字节序，帧头32字节）。
// 若后续协议明确了校验与消息ID，再完善对应校验与判别。
struct RadarStatus
{
    // 原始帧头（32字节，暂不细分字段）
    QByteArray frameHead; // size==32

    // 异常代码
    quint8 hwFault{};    // [0] 硬件故障位：bit0 天线, bit1 伺服, bit2 惯导
    quint32 swFault24{}; // [1:3] 软件故障24bit，低字节为索引1

    quint8 workState{};    // 雷达工作状态
    quint8 reserved1{};    // 预留
    quint16 detectRange{}; // 米 0~65535
    bool insValid{};       // 惯导有效标志
    bool simOn{};          // 雷达模拟状态
    bool retracted{};      // 撤收状态 true=已撤收
    bool driving{};        // 行驶状态 true=动态行车

    double longitude{}; // 度 -180~180，东正西负
    double latitude{};  // 度 -90~90，北正南负
    float altitude{};   // 米
    float yaw{};        // 0~360 度
    float pitch{};      // -90~90 度
    float roll{};       // -90~90 度

    QByteArray reserved2_8;     // 8B
    QByteArray verReserved_21;  // 21B
    QByteArray infoReserved_32; // 32B

    float freqGHz{};       // 频点 GHz（默认15.80）
    quint8 antPowerMode{}; // 天线上电模式 0~3

    QByteArray antReserved_16;   // 16B
    QByteArray chanReserved_8;   // 8B
    QByteArray servoReserved_16; // 16B

    quint16 silentStart{}; // 静默区起点 [0..36000] (0.01deg)
    quint16 silentEnd{};   // 静默区终点 [0..36000]

    QByteArray reserved3_12; // 12B
    quint16 checksum{};      // 校验位（未校验）

    // 简易可读文本
    QString hwFaultText() const;
    QString swFaultText() const;
    QString workStateText() const; // 未知表，先以数值呈现
    QString antPowerModeText() const;
};

namespace RadarStatusParser
{
    // 成功返回true；若长度不足或字段异常返回false。
    bool parseLittleEndian(const QByteArray &payload, RadarStatus &out);
}
