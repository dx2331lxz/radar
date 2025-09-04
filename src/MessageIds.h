// MessageIds.h
#pragma once

#include <QtGlobal>

namespace ProtocolIds
{

    // 表2 协议通用报文
    static constexpr quint16 GeneralCmdAck = 0xF000;        // 指挥中心命令应答报文 (雷达->指挥中心)
    static constexpr quint16 QueryRadarStatus = 0xF001;     // 查询雷达状态报文 (指挥中心->雷达)
    static constexpr quint16 HealthMonitor = 0xF002;        // 健康监测报文 (双向)
    static constexpr quint16 GeneralReservedBegin = 0xF003; // ~0xFFFF 系统保留
    static constexpr quint16 GeneralReservedEnd = 0xFFFF;

    // 表3 指挥中心命令报文（指挥中心->雷达）
    static constexpr quint16 CmdInit = 0x1001;          // 初始化任务
    static constexpr quint16 CmdCalibration = 0x1002;   // 自校准任务
    static constexpr quint16 CmdStandby = 0x1003;       // 雷达待机任务
    static constexpr quint16 CmdSearch = 0x1004;        // 雷达搜索任务
    static constexpr quint16 CmdTrack = 0x1005;         // 雷达跟踪任务
    static constexpr quint16 CmdSimulation = 0x1006;    // 雷达模拟任务
    static constexpr quint16 CmdPower = 0x1007;         // 雷达上下电任务
    static constexpr quint16 CmdDeploy = 0x1008;        // 雷达展开撤收任务
    static constexpr quint16 CmdServo = 0x1009;         // 伺服转停任务
    static constexpr quint16 CmdReservedBegin = 0x100A; // ~0x10FF 指挥中心功能保留
    static constexpr quint16 CmdReservedEnd = 0x10FF;

    // 表4 指挥中心命令报文（内部，雷达系统内部使用）
    static constexpr quint16 InternalCtrlBegin = 0x1101; // 0x1101-0x1FFF
    static constexpr quint16 InternalCtrlEnd = 0x1FFF;

    // 表5 雷达工作参数配置报文
    static constexpr quint16 CfgRadarPosition = 0x2011;    // 雷达位置配置 (指挥中心->雷达)
    static constexpr quint16 CfgRadarPositionAck = 0x2012; // 雷达位置反馈 (雷达->指挥中心)
    static constexpr quint16 CfgRadarIp = 0x2081;          // 雷达IP配置 (指挥中心->雷达)
    static constexpr quint16 CfgRadarIpAck = 0x2082;       // 雷达IP反馈 (雷达->指挥中心)
    static constexpr quint16 CfgSilentZone = 0x2091;       // 静默区配置 (指挥中心->雷达)
    static constexpr quint16 CfgSilentZoneAck = 0x2092;    // 静默区反馈 (雷达->指挥中心)
    static constexpr quint16 CfgReservedBegin = 0x2093;    // ~0x2FFF 系统保留
    static constexpr quint16 CfgReservedEnd = 0x2FFF;

} // namespace ProtocolIds
