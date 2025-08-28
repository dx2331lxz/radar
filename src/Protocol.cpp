#include "Protocol.h"
#include <QDateTime>
#include <QtEndian>

namespace Protocol
{

    static void wr_u16(QByteArray &ba, quint16 v)
    {
        quint16 le = qToLittleEndian(v);
        ba.append(reinterpret_cast<const char *>(&le), sizeof(le));
    }
    static void wr_u32(QByteArray &ba, quint32 v)
    {
        quint32 le = qToLittleEndian(v);
        ba.append(reinterpret_cast<const char *>(&le), sizeof(le));
    }
    static void wr_u64(QByteArray &ba, quint64 v)
    {
        quint64 le = qToLittleEndian(v);
        ba.append(reinterpret_cast<const char *>(&le), sizeof(le));
    }

    quint16 checksumSum16(const QByteArray &data)
    {
        quint32 sum = 0;
        for (unsigned char c : data)
            sum = (sum + c) & 0xFFFFu;
        return quint16(sum);
    }

    quint16 checksumCrc16IBM(const QByteArray &data)
    {
        quint16 crc = 0xFFFF;
        for (unsigned char b : data)
        {
            crc ^= b;
            for (int i = 0; i < 8; ++i)
            {
                if (crc & 1)
                    crc = (crc >> 1) ^ 0xA001; // 0xA001 = reverse(0x8005)
                else
                    crc >>= 1;
            }
        }
        return crc;
    }

    static QByteArray buildFrameHead(const HeaderConfig &cfg, quint16 totalBytes, quint8 seq, quint32 count)
    {
        QByteArray h;
        h.reserve(32);
        // 1. 协议帧头 "HRGK"
        h.append('H');
        h.append('R');
        h.append('G');
        h.append('K');
        // 2. 字节总数 uint16
        wr_u16(h, totalBytes);
        // 3. 设备型号（雷达） uint16
        wr_u16(h, cfg.deviceModel);
        // 4. UTC时戳 uint64 ms since epoch
        const quint64 ms = quint64(QDateTime::currentMSecsSinceEpoch());
        wr_u64(h, ms);
        // 5. 报文ID（雷达） uint16
        wr_u16(h, cfg.msgIdRadar);
        // 6. 报文ID（外部） uint16
        wr_u16(h, cfg.msgIdExternal);
        // 7. 设备ID（雷达） uint16
        wr_u16(h, cfg.deviceIdRadar);
        // 8. 设备ID（外部） uint16
        wr_u16(h, cfg.deviceIdExternal);
        // 9. 保留 uint16 0
        wr_u16(h, 0);
        // 10. 校验方式 uint8
        h.append(char(cfg.checkMethod));
        // 11. 报文序号 uint8
        h.append(char(seq));
        // 12. 报文计数 uint32
        wr_u32(h, count);
        // 32 bytes expected
        if (h.size() != 32)
            h.resize(32);
        return h;
    }

    QByteArray buildSearchTaskPacket(const HeaderConfig &cfg, quint8 taskType)
    {
        // Body: [1]任务类型 + [16]保留 + [2]校验，构建时先不含校验，计算后填入
        const int bodyNoCkLen = 1 + 16;         // 17
        const int total = 32 + bodyNoCkLen + 2; // 51 bytes

        static quint8 s_seq = 0; // 简易序号
        static quint32 s_count = 0;
        const quint8 seq = s_seq++;
        const quint32 cnt = ++s_count;

        QByteArray packet;
        packet.reserve(total);
        // 先占位帧头（总长度需要，先用total计算）
        QByteArray head = buildFrameHead(cfg, quint16(total), seq, cnt);
        packet.append(head);

        // 任务体
        packet.append(char(taskType));
        packet.append(QByteArray(16, '\0'));

        // 按校验方式在末尾追加2字节小端
        quint16 ck = 0;
        if (cfg.checkMethod == 1)
            ck = checksumSum16(packet);
        else if (cfg.checkMethod == 2)
            ck = checksumCrc16IBM(packet);
        else
            ck = 0;
        quint16 le = qToLittleEndian(ck);
        packet.append(reinterpret_cast<const char *>(&le), sizeof(le));

        return packet;
    }

} // namespace Protocol
