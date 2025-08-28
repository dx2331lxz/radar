// TrackMessage.cpp
#include "TrackMessage.h"
#include <QtEndian>
#include <cstring>
#include <QtGlobal>

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

bool TrackParser::parseLittleEndian(const QByteArray &payload, TrackMessage &out)
{
    // 计算最小长度：32(head)+1+8+8+4 + trackInfo(2+8+8+4+4+4+4+4+4+4+4+1*?)+16+2
    // 逐字段推进避免算错时越界。
    // 最小长度精确为 142 字节：
    // 32(head) + 1(ins) + 8(lon) + 8(lat) + 4(alt)
    // + trackInfo(2 + 8 + 8 + 4 + 6*4 + 4 + 6 + 3*4 + 3 = 71)
    // + 16(reserved) + 2(crc)
    if (payload.size() < 142)
        return false;

    const uchar *p = reinterpret_cast<const uchar *>(payload.constData());
    int off = 0;

    out = TrackMessage{};
    out.frameHead = QByteArray(reinterpret_cast<const char *>(p), 32);
    off += 32;

    out.insValid = p[off++] == 0x01;
    out.radarLon = rd_f64(p + off);
    off += 8;
    out.radarLat = rd_f64(p + off);
    off += 8;
    out.radarAlt = rd_f32(p + off);
    off += 4;

    TrackInfo ti{};
    ti.trackId = rd_u16(p + off);
    off += 2;
    ti.tgtLon = rd_f64(p + off);
    off += 8;
    ti.tgtLat = rd_f64(p + off);
    off += 8;
    ti.tgtAlt = rd_f32(p + off);
    off += 4;
    ti.distance = rd_f32(p + off);
    off += 4;
    ti.azimuth = rd_f32(p + off);
    off += 4;
    ti.elevation = rd_f32(p + off);
    off += 4;
    ti.speed = rd_f32(p + off);
    off += 4;
    ti.course = rd_f32(p + off);
    off += 4;
    ti.strength = rd_f32(p + off);
    off += 4;

    // 预留4字节
    memcpy(ti.reserved4, p + off, 4);
    off += 4;

    ti.targetType = p[off++];
    ti.targetSize = p[off++];
    ti.pointType = p[off++];
    ti.trackType = p[off++];
    ti.lostCount = p[off++];
    ti.quality = p[off++];

    ti.rawDistance = rd_f32(p + off);
    off += 4;
    ti.rawAzimuth = rd_f32(p + off);
    off += 4;
    ti.rawElevation = rd_f32(p + off);
    off += 4;
    memcpy(ti.reserved3, p + off, 3);
    off += 3;

    out.info = ti;

    memcpy(out.reserved16, p + off, 16);
    off += 16;
    out.checksum = rd_u16(p + off);
    off += 2;

    Q_UNUSED(off);

    // 基本合法性校验（根据协议注释的范围）：
    auto inRange = [](double v, double lo, double hi)
    { return v >= lo && v <= hi; };
    if (!inRange(out.radarLon, -180.0, 180.0))
        return false;
    if (!inRange(out.radarLat, -90.0, 90.0))
        return false;
    if (!inRange(double(out.info.tgtLon), -180.0, 180.0))
        return false;
    if (!inRange(double(out.info.tgtLat), -90.0, 90.0))
        return false;
    if (!inRange(double(out.info.azimuth), 0.0, 360.0))
        return false;
    if (!inRange(double(out.info.elevation), -90.0, 90.0))
        return false;
    if (!inRange(double(out.info.course), 0.0, 360.0))
        return false;
    if (!inRange(double(out.info.rawAzimuth), 0.0, 360.0))
        return false;
    if (!inRange(double(out.info.rawElevation), -90.0, 90.0))
        return false;
    if (out.info.quality > 100)
        return false;
    // 有些设备distance<0表示异常，过滤
    if (out.info.distance < 0 || out.info.rawDistance < 0)
        return false;

    return true;
}

bool TrackParser::hasReadableMagic(const QByteArray &payload)
{
    if (payload.size() < 4)
        return false;
    const uchar *p = reinterpret_cast<const uchar *>(payload.constData());
    // 允许字母数字和空格的可读头，或特定示例中出现的 "H R G K"/其他大写
    bool asciiLike = true;
    for (int i = 0; i < 4; ++i)
    {
        uchar c = p[i];
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == ' '))
        {
            asciiLike = false;
            break;
        }
    }
    return asciiLike;
}
