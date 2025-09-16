// RadarScopeWidget.h
#pragma once

#include <QWidget>
#include <QVector>
#include <QTimer>
#include <QPointF>
#include "TrackMessage.h"
#include <QString>

// 简单的圆形雷达显示器：
// - 以正北向上，顺时针为正角；
// - 支持设置显示半径（米）；
// - 接收 TrackMessage 实时绘点与航迹；
// - 显示最近若干条轨迹的折线和末端点。
class RadarScopeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RadarScopeWidget(QWidget *parent = nullptr);

    void setMaxRangeMeters(float r);
    float maxRangeMeters() const { return m_maxRange; }

    // 轨迹保留策略（默认：保留5分钟，单条轨迹最多2000点）
    void setTrailRetentionMs(qint64 ms) { m_trailKeepMs = qMax<qint64>(1000, ms); }
    qint64 trailRetentionMs() const { return m_trailKeepMs; }
    void setMaxTrailPoints(int n) { m_maxTrailPoints = qMax(10, n); }
    int maxTrailPoints() const { return m_maxTrailPoints; }

    // 事件提示（仅显示关键信息，如“发现新目标”）
    void setShowNotices(bool on) { m_showNotices = on; }
    bool showNotices() const { return m_showNotices; }
    void setNoticeKeepMs(qint64 ms) { m_noticeKeepMs = qMax<qint64>(500, ms); }
    qint64 noticeKeepMs() const { return m_noticeKeepMs; }

signals:
    // notify that a target has been destroyed (so other UI can remove it)
    void targetHit(quint16 id);

public slots:
    void onTrackDatagram(const QByteArray &data);
    void highlightTarget(quint16 id);
    // 请求锁定（界面变色）
    void lockTarget(quint16 id);
    // 请求发起打击（id）
    void engageTarget(quint16 id);
    // 开/关搜索扫描线
    void setSearchActive(bool on);
    void setSweepSpeedDegPerSec(float degPerSec) { m_sweepSpeed = qBound(1.0f, degPerSec, 360.0f); }
    // 清空当前显示的目标轨迹
    void clearTrails();

protected:
    void paintEvent(QPaintEvent *) override;
    QSize minimumSizeHint() const override { return {360, 360}; }

private:
    struct Attack
    {
        enum Type
        {
            Laser,
            SlowMissile,
            FastMissile
        } type;
        quint16 targetId;
        QPointF pos; // current position of the attack
        qint64 startMs;
        bool finished{false};
        // for missiles: velocity pixels per second
        float speed{0.0f};
        // for laser: lifetime ms
    };
    QVector<Attack> m_attacks;
    // locked target id -> show red halo
    quint16 m_lockedId{0};
    struct TrailPoint
    {
        QPointF pos;
        qint64 ms;
    };
    struct Trail
    {
        quint16 id;
        QVector<TrailPoint> points;
        quint8 targetType{0};
        quint8 targetSize{0};
        float lastSpeed{0.0f};
        float lastDistance{0.0f};
        bool identityKnown{false};
    };
    struct Notice
    {
        QString text;
        qint64 ms;
    };

    // helper to convert type/size to short label
    static QString typeSizeLabel(int type, int size);
    // 计算威胁分（0..1）
    float computeThreatScore(const Trail &t) const;

    // 可调参数（默认值按需求）
    float m_weightDistance = 0.3f;
    float m_weightSpeed = 0.3f;
    float m_weightType = 0.2f;
    float m_weightIdentity = 0.2f;
    float m_maxSpeed = 100.0f; // m/s，用于速度归一化

    QPointF polarToPoint(float distance_m, float azimuth_deg, const QRectF &circle) const;

    float m_maxRange = 5000.0f; // 默认5km
    QVector<Trail> m_trails;    // 多目标轨迹
    quint16 m_highlightId{0};
    QTimer m_cleanupTimer; // 周期清理过期航迹
    QTimer m_attackTimer;  // 更新攻击行为（导弹移动、激光寿命）

    // 减少默认保留时长与点数以降低内存与绘制负担
    qint64 m_trailKeepMs = 60ll * 1000ll; // 保留60秒
    int m_maxTrailPoints = 500;           // 每条轨迹最大采样点
    bool m_showNotices = true;
    qint64 m_noticeKeepMs = 3000; // 提示保留3s
    QVector<Notice> m_notices;    // 左上角提示

    // 扫描线（搜索模式）
    bool m_sweepOn = false;
    QTimer m_sweepTimer;        // 动画定时器
    float m_sweepAngle = 0.0f;  // 0..360，北为0，顺时针增加
    float m_sweepSpeed = 60.0f; // deg/s
};
