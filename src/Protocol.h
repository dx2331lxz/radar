#pragma once

#include <QByteArray>
#include <QtGlobal>

namespace Protocol
{
    // 帧头配置（除时间戳、总长、序号/计数外其余从UI给定）
    struct HeaderConfig
    {
        quint16 deviceModel = 6000;   // 设备型号（雷达）
        quint16 msgIdRadar = 0x0001;  // 报文ID（雷达）——假定值，待对接文档时可调整
        quint16 msgIdExternal = 0;    // 报文ID（外部）
        quint16 deviceIdRadar = 0;    // 设备ID（雷达）
        quint16 deviceIdExternal = 0; // 设备ID（外部）
        quint8 checkMethod = 1;       // 校验方式：0无校验，1和校验，2 CRC16
    };

    // 构造“雷达搜索任务”完整数据包：
    // 包含32B帧头 + 1B任务类型 + 16B保留(全0) + 2B校验
    QByteArray buildSearchTaskPacket(const HeaderConfig &cfg, quint8 taskType = 0x01);

    // 构造“雷达待机任务”完整数据包：
    // 含32B帧头 + 1B任务类型(0x00) + 16B保留(全0) + 2B校验
    QByteArray buildStandbyTaskPacket(const HeaderConfig &cfg, quint8 taskType = 0x00);

    // 计算和校验与CRC16（LE）
    quint16 checksumSum16(const QByteArray &data);    // 对data全体求和16位
    quint16 checksumCrc16IBM(const QByteArray &data); // CRC-16/IBM (poly 0xA001), 初值0xFFFF

} // namespace Protocol
