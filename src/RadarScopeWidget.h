// RadarScopeWidget.h
#pragma once

#include <QWidget>
#include <QVector>
#include <QTimer>
#include "TrackMessage.h"

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

public slots:
    void onTrackDatagram(const QByteArray &data);

protected:
    void paintEvent(QPaintEvent *) override;
    QSize minimumSizeHint() const override { return {360, 360}; }

private:
    struct TrailPoint { QPointF pos; qint64 ms; };
    struct Trail { quint16 id; QVector<TrailPoint> points; };

    QPointF polarToPoint(float distance_m, float azimuth_deg, const QRectF &circle) const;

    float m_maxRange = 8000.0f; // 默认8km
    QVector<Trail> m_trails;    // 多目标轨迹
    QTimer m_cleanupTimer;      // 周期清理过期航迹
};
