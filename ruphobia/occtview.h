#pragma once

#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QWheelEvent>

#include <AIS_InteractiveContext.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <TopoDS_Shape.hxx>

class OcctView : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit OcctView(QWidget* parent = nullptr);

    void displayShape(const TopoDS_Shape& shape);
    void fitAll();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void initViewer();

    Handle(V3d_Viewer) m_viewer;
    Handle(V3d_View) m_view;
    Handle(AIS_InteractiveContext) m_context;

    bool m_initialized = false;
    QPoint m_lastPos;
    Qt::MouseButton m_activeButton = Qt::NoButton;
};
