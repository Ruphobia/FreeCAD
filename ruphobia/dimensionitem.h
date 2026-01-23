#pragma once

#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QPen>
#include <QPointF>
#include <QFont>

class DimensionItem : public QGraphicsItem
{
public:
    // Which type of geometry this dimension is attached to
    enum class GeomType { Line, RectEdge, Circle };

    DimensionItem(const QPointF& p1, const QPointF& p2, const QPen& pen,
                  QGraphicsItem* sourceItem, GeomType geomType, int edgeIndex = 0,
                  QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

    void setDimensionValue(double value);
    double dimensionValue() const { return m_value; }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

private:
    void recalculate();
    void applyValueToGeometry(double newValue);

    QPointF m_p1, m_p2;
    double m_value;
    double m_offset = 20.0;
    QPen m_pen;
    QFont m_font;

    // Source geometry reference
    QGraphicsItem* m_sourceItem;
    GeomType m_geomType;
    int m_edgeIndex; // for rect edges: 0=top, 1=bottom, 2=left, 3=right

    // Drag state
    bool m_dragging = false;

    // Calculated geometry
    QPointF m_normal;
    QRectF m_bounds;
};
