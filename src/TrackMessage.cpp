// TrackMessage.cpp
#include "TrackMessage.h"
#include <QtEndian>
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

bool TrackParser::parseLittleEndian(const QByteArray &payload, TrackMessage &out)
{
    // 计算最小长度：32(head)+1+8+8+4 + trackInfo(2+8+8+4+4+4+4+4+4+4+4+1*?)+16+2
    // 逐字段推进避免算错时越界。
    if (payload.size() < 32 + 1 + 8 + 8 + 4 + 2 + 8 + 8 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 1 + 1 + 1 + 1 + 1 + 1 + 4 + 4 + 4 + 3 + 16 + 2)
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
    return true;
}
