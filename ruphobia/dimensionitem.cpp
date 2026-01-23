#include "dimensionitem.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QInputDialog>
#include <QApplication>
#include <QtMath>
#include <QCursor>

DimensionItem::DimensionItem(const QPointF& p1, const QPointF& p2, const QPen& pen,
                             QGraphicsItem* sourceItem, GeomType geomType, int edgeIndex,
                             QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_p1(p1)
    , m_p2(p2)
    , m_pen(pen)
    , m_font("Sans", 8)
    , m_sourceItem(sourceItem)
    , m_geomType(geomType)
    , m_edgeIndex(edgeIndex)
{
    m_value = QLineF(p1, p2).length();
    setFlags(ItemIsSelectable);
    setAcceptHoverEvents(true);
    setCursor(Qt::SizeAllCursor);
    recalculate();
}

void DimensionItem::recalculate()
{
    prepareGeometryChange();

    QPointF dir = m_p2 - m_p1;
    double len = qSqrt(dir.x() * dir.x() + dir.y() * dir.y());
    m_normal = QPointF(-dir.y(), dir.x());
    if (len > 0)
        m_normal /= len;

    // Bounding rect covers geometry points, dimension line, and text
    QPointF d1 = m_p1 + m_normal * m_offset;
    QPointF d2 = m_p2 + m_normal * m_offset;

    double margin = 30.0;
    QRectF geomRect = QRectF(m_p1, m_p2).normalized();
    QRectF dimRect = QRectF(d1, d2).normalized();
    m_bounds = geomRect.united(dimRect).adjusted(-margin, -margin, margin, margin);
}

QRectF DimensionItem::boundingRect() const
{
    return m_bounds;
}

void DimensionItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setRenderHint(QPainter::Antialiasing);

    QPointF d1 = m_p1 + m_normal * m_offset;
    QPointF d2 = m_p2 + m_normal * m_offset;

    QPen pen = m_pen;
    pen.setCosmetic(true);

    // Extension lines (dotted, from geometry to dimension line)
    QPen extPen = pen;
    extPen.setStyle(Qt::DotLine);
    painter->setPen(extPen);
    painter->drawLine(m_p1, d1 + m_normal * 3);
    painter->drawLine(m_p2, d2 + m_normal * 3);

    // Main dimension line
    painter->setPen(pen);
    painter->drawLine(d1, d2);

    // Arrowheads
    double arrowSize = 6.0;
    QPointF lineDir = d2 - d1;
    double lineLen = qSqrt(lineDir.x() * lineDir.x() + lineDir.y() * lineDir.y());
    if (lineLen > 0)
        lineDir /= lineLen;
    QPointF lineNorm(-lineDir.y(), lineDir.x());

    // Arrow at d1
    painter->drawLine(d1, d1 + lineDir * arrowSize + lineNorm * (arrowSize * 0.4));
    painter->drawLine(d1, d1 + lineDir * arrowSize - lineNorm * (arrowSize * 0.4));

    // Arrow at d2
    painter->drawLine(d2, d2 - lineDir * arrowSize + lineNorm * (arrowSize * 0.4));
    painter->drawLine(d2, d2 - lineDir * arrowSize - lineNorm * (arrowSize * 0.4));

    // Distance text
    QPointF midpoint = (d1 + d2) / 2.0;
    QString text = QString::number(m_value, 'f', 1);

    painter->setFont(m_font);
    QFontMetricsF fm(m_font);
    double textWidth = fm.horizontalAdvance(text);
    double textHeight = fm.height();

    // Rotate text to follow the dimension line
    double angle = qAtan2(lineDir.y(), lineDir.x());
    if (angle > M_PI / 2 || angle < -M_PI / 2)
        angle += M_PI;

    painter->save();
    painter->translate(midpoint);
    painter->rotate(qRadiansToDegrees(angle));
    painter->drawText(QRectF(-textWidth / 2, -textHeight - 2, textWidth, textHeight),
                      Qt::AlignCenter, text);
    painter->restore();

    // Selection highlight
    if (isSelected()) {
        QPen selPen(QColor(100, 150, 255), 1.0, Qt::DashDotLine);
        selPen.setCosmetic(true);
        painter->setPen(selPen);
        painter->setBrush(Qt::NoBrush);
        QRectF dimBounds = QRectF(d1, d2).normalized().adjusted(-5, -15, 5, 5);
        painter->drawRect(dimBounds);
    }
}

void DimensionItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        event->accept();
    } else {
        QGraphicsItem::mousePressEvent(event);
    }
}

void DimensionItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_dragging) {
        // Project cursor position onto the normal axis to get new offset
        QPointF scenePos = event->scenePos();
        // Compute signed distance from cursor to the p1-p2 line
        QPointF toPoint = scenePos - m_p1;
        double newOffset = toPoint.x() * m_normal.x() + toPoint.y() * m_normal.y();

        // Minimum offset so dimension doesn't sit on the geometry
        if (qAbs(newOffset) < 8.0)
            newOffset = (newOffset >= 0) ? 8.0 : -8.0;

        m_offset = newOffset;
        recalculate();
        update();
        event->accept();
    } else {
        QGraphicsItem::mouseMoveEvent(event);
    }
}

void DimensionItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    } else {
        QGraphicsItem::mouseReleaseEvent(event);
    }
}

void DimensionItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    Q_UNUSED(event);

    bool ok = false;
    double newValue = QInputDialog::getDouble(
        QApplication::activeWindow(),
        "Edit Dimension",
        "Value:",
        m_value,
        0.001,
        1e9,
        1,
        &ok);

    if (ok && newValue > 0 && qAbs(newValue - m_value) > 0.001) {
        applyValueToGeometry(newValue);
    }
}

void DimensionItem::applyValueToGeometry(double newValue)
{
    if (!m_sourceItem)
        return;

    double oldValue = m_value;
    double scale = newValue / oldValue;

    switch (m_geomType) {
    case GeomType::Line: {
        auto* lineItem = dynamic_cast<QGraphicsLineItem*>(m_sourceItem);
        if (!lineItem) break;

        QLineF line = lineItem->line();
        // Keep p1 fixed, scale p2
        QPointF dir = line.p2() - line.p1();
        double len = qSqrt(dir.x() * dir.x() + dir.y() * dir.y());
        if (len > 0) {
            dir /= len;
            QPointF newP2 = line.p1() + dir * newValue;
            lineItem->setLine(QLineF(line.p1(), newP2));
            // Update our reference points
            m_p2 = newP2;
        }
        break;
    }

    case GeomType::RectEdge: {
        auto* rectItem = dynamic_cast<QGraphicsRectItem*>(m_sourceItem);
        if (!rectItem) break;

        QRectF r = rectItem->rect();
        switch (m_edgeIndex) {
        case 0: // top edge (horizontal)
        case 1: // bottom edge (horizontal)
            r.setWidth(newValue);
            break;
        case 2: // left edge (vertical)
        case 3: // right edge (vertical)
            r.setHeight(newValue);
            break;
        }
        rectItem->setRect(r);

        // Update reference points to match new geometry
        QLineF edges[4] = {
            QLineF(r.topLeft(), r.topRight()),
            QLineF(r.bottomLeft(), r.bottomRight()),
            QLineF(r.topLeft(), r.bottomLeft()),
            QLineF(r.topRight(), r.bottomRight())
        };
        m_p1 = edges[m_edgeIndex].p1();
        m_p2 = edges[m_edgeIndex].p2();
        break;
    }

    case GeomType::Circle: {
        auto* ellipseItem = dynamic_cast<QGraphicsEllipseItem*>(m_sourceItem);
        if (!ellipseItem) break;

        QRectF r = ellipseItem->rect();
        QPointF center = r.center();
        double newRadius = newValue;
        ellipseItem->setRect(center.x() - newRadius, center.y() - newRadius,
                             newRadius * 2, newRadius * 2);
        // Update reference points
        m_p1 = center;
        m_p2 = QPointF(center.x() + newRadius, center.y());
        break;
    }
    }

    m_value = newValue;
    recalculate();
    update();
}

void DimensionItem::setDimensionValue(double value)
{
    applyValueToGeometry(value);
}
