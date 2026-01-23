#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsPathItem>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPen>
#include <QPointF>
#include <vector>

enum class SketchTool {
    None,
    Line,
    Circle,
    Arc,
    Rectangle,
    Polyline,
    Point,
    Dimension
};

class SketchView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit SketchView(QWidget* parent = nullptr);

    void setTool(SketchTool tool);
    SketchTool currentTool() const { return m_tool; }

signals:
    void toolChangeRequested(SketchTool tool);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override;

private:
    void finishCurrentOperation();
    QPointF snapToGrid(const QPointF& pt) const;

    QGraphicsScene* m_scene;
    SketchTool m_tool = SketchTool::None;

    // Drawing state
    bool m_drawing = false;
    QPointF m_startPoint;
    QPointF m_currentPoint;
    int m_clickCount = 0;

    // Temporary items for visual feedback during drawing
    QGraphicsLineItem* m_tempLine = nullptr;
    QGraphicsEllipseItem* m_tempCircle = nullptr;
    QGraphicsRectItem* m_tempRect = nullptr;
    QGraphicsPathItem* m_tempPath = nullptr;
    QGraphicsEllipseItem* m_tempArcCircle = nullptr;

    // Polyline state
    std::vector<QPointF> m_polylinePoints;
    QPainterPath m_polylinePath;

    // Arc state
    QPointF m_arcCenter;
    double m_arcRadius = 0;
    double m_arcStartAngle = 0;

    // Dimension state
    QPointF m_dimStart;
    QGraphicsLineItem* m_tempDimLine = nullptr;

    // Pens
    QPen m_sketchPen;
    QPen m_previewPen;
    QPen m_dimensionPen;

    // Grid
    double m_gridSize = 10.0;
};
