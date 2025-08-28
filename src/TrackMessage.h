// TrackMessage.h
#pragma once

#include <QByteArray>
#include <QtGlobal>

// 6.1 雷达航迹报文（假定小端字节序，帧头32字节）
// 本文件仅解析协议字段到内存结构，未做校验位验证。
struct TrackInfo
{
    quint16 trackId{};     // 航迹批号
    double tgtLon{};       // 目标经度 (deg)
    double tgtLat{};       // 目标纬度 (deg)
    float tgtAlt{};        // 目标海拔 (m)
    float distance{};      // 距离 (m)
    float azimuth{};       // 方位角 (deg, 0~360, 北为0 顺时针)
    float elevation{};     // 俯仰角 (deg)
    float speed{};         // 速度 (m/s)
    float course{};        // 航向 (deg)
    float strength{};      // dB
    quint8 reserved4[4]{}; // 预留

    quint8 targetType{}; // 0未知 1旋翼无人机 2固定翼 3直升机 4民航 5车
    quint8 targetSize{}; // 0小 1中 2大 3特大
    quint8 pointType{};  // 0检测点 1外推点
    quint8 trackType{};  // 0模拟 1搜索 2跟踪
    quint8 lostCount{};  // 连续丢失次数
    quint8 quality{};    // 0-100

    float rawDistance{};  // 原始距离 (m)
    float rawAzimuth{};   // 原始方位角 (deg)
    float rawElevation{}; // 原始俯仰角 (deg)
    quint8 reserved3[3]{};
};

struct TrackMessage
{
    QByteArray frameHead; // 32B
    bool insValid{};      // 惯导有效标志 0x01 有效
    double radarLon{};    // 雷达经度
    double radarLat{};    // 雷达纬度
    float radarAlt{};     // 雷达海拔

    TrackInfo info;       // 1个航迹

    quint8 reserved16[16]{}; // 预留
    quint16 checksum{};      // 校验（未验）
};

namespace TrackParser
{
    // 成功返回 true；否则 false。不会越界访问。
    bool parseLittleEndian(const QByteArray &payload, TrackMessage &out);
}
