// RadarStatus.cpp
#include "RadarStatus.h"
#include <QtEndian>
#include <QStringList>
#include <cstring>

static quint16 rd_u16(const uchar *p) { return qFromLittleEndian<quint16>(p); }
static quint32 rd_u32(const uchar *p) { return qFromLittleEndian<quint32>(p); }
static float rd_f32(const uchar *p)
{
    quint32 bits = rd_u32(p);
    float f;
    memcpy(&f, &bits, sizeof(f));
    return f;
}
static double rd_f64(const uchar *p)
{
    quint64 bits = qFromLittleEndian<quint64>(p);
    double d;
    memcpy(&d, &bits, sizeof(d));
    return d;
}

QString RadarStatus::hwFaultText() const
{
    QStringList items;
    if ((hwFault & 0x01) != 0)
        items << QStringLiteral("天线异常");
    if ((hwFault & 0x02) != 0)
        items << QStringLiteral("伺服异常");
    if ((hwFault & 0x04) != 0)
        items << QStringLiteral("惯导异常");
    return items.isEmpty() ? QStringLiteral("无故障") : items.join(',');
}

QString RadarStatus::swFaultText() const
{
    if (swFault24 == 0)
        return QStringLiteral("无故障");
    // 列出置位的bit索引
    QStringList bits;
    for (int i = 0; i < 24; ++i)
    {
        if (swFault24 & (1u << i))
            bits << QString::number(i);
    }
    return QStringLiteral("软件故障bits:") + bits.join(',');
}

QString RadarStatus::workStateText() const
{
    // 协议未附具体枚举，先展示数字
    return QStringLiteral("状态%1").arg(workState);
}

QString RadarStatus::antPowerModeText() const
{
    switch (antPowerMode)
    {
    case 0:
        return QStringLiteral("待机");
    case 1:
        return QStringLiteral("只接收加电");
    case 2:
        return QStringLiteral("只发射加电");
    case 3:
        return QStringLiteral("收发均加电");
    default:
        return QStringLiteral("未知(%1)").arg(antPowerMode);
    }
}

bool RadarStatusParser::parseLittleEndian(const QByteArray &payload, RadarStatus &out)
{
    const int needMin = 32 /*head*/ + 4 + 1 + 1 + 2 + 1 + 1 + 1 + 1 + 8 + 8 + 4 + 4 + 4 + 4 + 8 + 21 + 32 + 4 + 1 + 16 + 8 + 16 + 2 + 2 + 12 + 2;
    if (payload.size() < needMin)
        return false;

    const uchar *p = reinterpret_cast<const uchar *>(payload.constData());
    int off = 0;

    out = RadarStatus{};
    out.frameHead = QByteArray(reinterpret_cast<const char *>(p), 32);
    off += 32;

    // 异常代码 4B: hw(1) + sw(3)
    out.hwFault = p[off];
    out.swFault24 = (quint32)p[off + 1] | ((quint32)p[off + 2] << 8) | ((quint32)p[off + 3] << 16);
    off += 4;

    out.workState = p[off++];
    out.reserved1 = p[off++];
    out.detectRange = rd_u16(p + off);
    off += 2;
    out.insValid = p[off++] == 0x01;
    out.simOn = p[off++] == 0x01;
    out.retracted = p[off++] == 0x01;
    out.driving = p[off++] == 0x01;

    out.longitude = rd_f64(p + off);
    off += 8;
    out.latitude = rd_f64(p + off);
    off += 8;
    out.altitude = rd_f32(p + off);
    off += 4;
    out.yaw = rd_f32(p + off);
    off += 4;
    out.pitch = rd_f32(p + off);
    off += 4;
    out.roll = rd_f32(p + off);
    off += 4;

    out.reserved2_8 = QByteArray(reinterpret_cast<const char *>(p + off), 8);
    off += 8;
    out.verReserved_21 = QByteArray(reinterpret_cast<const char *>(p + off), 21);
    off += 21;
    out.infoReserved_32 = QByteArray(reinterpret_cast<const char *>(p + off), 32);
    off += 32;

    out.freqGHz = rd_f32(p + off);
    off += 4;
    out.antPowerMode = p[off++];

    out.antReserved_16 = QByteArray(reinterpret_cast<const char *>(p + off), 16);
    off += 16;
    out.chanReserved_8 = QByteArray(reinterpret_cast<const char *>(p + off), 8);
    off += 8;
    out.servoReserved_16 = QByteArray(reinterpret_cast<const char *>(p + off), 16);
    off += 16;

    out.silentStart = rd_u16(p + off);
    off += 2;
    out.silentEnd = rd_u16(p + off);
    off += 2;

    out.reserved3_12 = QByteArray(reinterpret_cast<const char *>(p + off), 12);
    off += 12;
    out.checksum = rd_u16(p + off);
    off += 2;

    Q_UNUSED(off);
    return true;
}
