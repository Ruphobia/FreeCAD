#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QToolBar>
#include <QString>

#include <zip.h>

#include "version.h"
#include "sketchview.h"

static QToolBar* contextToolbar = nullptr;
static SketchView* sketchCanvas = nullptr;
static QString currentAssemblyPath;

static bool writeFCStd(const QString& path, const QString& objectType, const QString& objectName)
{
    int errCode = 0;
    zip_t* archive = zip_open(path.toUtf8().constData(), ZIP_CREATE | ZIP_TRUNCATE, &errCode);
    if (!archive)
        return false;

    QByteArray xml;
    xml.append("<?xml version='1.0' encoding='utf-8'?>\n");
    xml.append("<Document SchemaVersion=\"4\" ProgramVersion=\"1.0.0\" FileVersion=\"1\">\n");
    xml.append("  <Properties Count=\"0\"/>\n");
    xml.append("  <Objects Count=\"1\">\n");
    xml.append("    <Object type=\"");
    xml.append(objectType.toUtf8());
    xml.append("\" name=\"");
    xml.append(objectName.toUtf8());
    xml.append("\" />\n");
    xml.append("  </Objects>\n");
    xml.append("  <ObjectData Count=\"1\">\n");
    xml.append("    <Object name=\"");
    xml.append(objectName.toUtf8());
    xml.append("\">\n");
    xml.append("      <Properties Count=\"1\">\n");
    xml.append("        <Property name=\"Label\" type=\"App::PropertyString\">\n");
    xml.append("          <String value=\"");
    xml.append(objectName.toUtf8());
    xml.append("\"/>\n");
    xml.append("        </Property>\n");
    xml.append("      </Properties>\n");
    xml.append("    </Object>\n");
    xml.append("  </ObjectData>\n");
    xml.append("</Document>\n");

    zip_source_t* source = zip_source_buffer(archive, xml.constData(), xml.size(), 0);
    if (!source) {
        zip_discard(archive);
        return false;
    }

    zip_int64_t idx = zip_file_add(archive, "Document.xml", source, ZIP_FL_OVERWRITE);
    if (idx < 0) {
        zip_source_free(source);
        zip_discard(archive);
        return false;
    }

    if (zip_close(archive) < 0) {
        zip_discard(archive);
        return false;
    }

    return true;
}

static void enterSketchMode(QMainWindow* window);
static void enterAssemblyMode(QMainWindow* window);

static void newPart(QMainWindow* window)
{
    QString path = QFileDialog::getSaveFileName(window, "New Part", QString(),
                                                "FreeCAD Files (*.FCStd)");
    if (path.isEmpty())
        return;

    if (!path.endsWith(".FCStd", Qt::CaseInsensitive))
        path.append(".FCStd");

    if (!writeFCStd(path, "PartDesign::Body", QFileInfo(path).baseName())) {
        QMessageBox::critical(window, "Error", "Failed to create part file.");
        return;
    }

    enterSketchMode(window);
}

static void enterSketchMode(QMainWindow* window)
{
    if (contextToolbar) {
        window->removeToolBar(contextToolbar);
        delete contextToolbar;
    }

    // Create sketch canvas if not already present
    if (!sketchCanvas) {
        sketchCanvas = new SketchView(window);
    }
    window->setCentralWidget(sketchCanvas);

    contextToolbar = new QToolBar("Sketcher", window);

    QAction* lineAction = contextToolbar->addAction(
        QIcon(":/icons/Sketcher_CreateLine.svg"), "");
    lineAction->setToolTip("Line");
    lineAction->setCheckable(true);

    QAction* circleAction = contextToolbar->addAction(
        QIcon(":/icons/Sketcher_CreateCircle.svg"), "");
    circleAction->setToolTip("Circle");
    circleAction->setCheckable(true);

    QAction* arcAction = contextToolbar->addAction(
        QIcon(":/icons/Sketcher_CreateArc.svg"), "");
    arcAction->setToolTip("Arc");
    arcAction->setCheckable(true);

    QAction* rectAction = contextToolbar->addAction(
        QIcon(":/icons/Sketcher_CreateRectangle.svg"), "");
    rectAction->setToolTip("Rectangle");
    rectAction->setCheckable(true);

    QAction* polylineAction = contextToolbar->addAction(
        QIcon(":/icons/Sketcher_CreatePolyline.svg"), "");
    polylineAction->setToolTip("Polyline");
    polylineAction->setCheckable(true);

    QAction* pointAction = contextToolbar->addAction(
        QIcon(":/icons/Sketcher_CreatePoint.svg"), "");
    pointAction->setToolTip("Point");
    pointAction->setCheckable(true);

    contextToolbar->addSeparator();

    QAction* dimAction = contextToolbar->addAction(
        QIcon(":/icons/Constraint_Dimension.svg"), "");
    dimAction->setToolTip("Dimension (D)");
    dimAction->setCheckable(true);

    // Make tools mutually exclusive
    auto setExclusive = [=](QAction* active, SketchTool tool) {
        for (auto* a : contextToolbar->actions())
            if (a != active) a->setChecked(false);
        if (active->isChecked())
            sketchCanvas->setTool(tool);
        else
            sketchCanvas->setTool(SketchTool::None);
    };

    QObject::connect(lineAction, &QAction::triggered, [=]() {
        setExclusive(lineAction, SketchTool::Line);
    });
    QObject::connect(circleAction, &QAction::triggered, [=]() {
        setExclusive(circleAction, SketchTool::Circle);
    });
    QObject::connect(arcAction, &QAction::triggered, [=]() {
        setExclusive(arcAction, SketchTool::Arc);
    });
    QObject::connect(rectAction, &QAction::triggered, [=]() {
        setExclusive(rectAction, SketchTool::Rectangle);
    });
    QObject::connect(polylineAction, &QAction::triggered, [=]() {
        setExclusive(polylineAction, SketchTool::Polyline);
    });
    QObject::connect(pointAction, &QAction::triggered, [=]() {
        setExclusive(pointAction, SketchTool::Point);
    });
    QObject::connect(dimAction, &QAction::triggered, [=]() {
        setExclusive(dimAction, SketchTool::Dimension);
    });

    // Sync toolbar buttons when tool is changed via keyboard shortcut
    QObject::connect(sketchCanvas, &SketchView::toolChangeRequested, [=](SketchTool tool) {
        for (auto* a : contextToolbar->actions())
            a->setChecked(false);
        if (tool == SketchTool::Dimension) dimAction->setChecked(true);
    });

    window->addToolBar(contextToolbar);
    contextToolbar->show();
}

static void enterAssemblyMode(QMainWindow* window)
{
    if (contextToolbar) {
        window->removeToolBar(contextToolbar);
        delete contextToolbar;
    }

    contextToolbar = new QToolBar("Assembly", window);
    QAction* newPartAction = contextToolbar->addAction(
        QIcon(":/icons/PartDesign_Body.svg"), "");
    newPartAction->setToolTip("New Part");
    QObject::connect(newPartAction, &QAction::triggered,
                     [window]() { newPart(window); });
    window->addToolBar(contextToolbar);
    contextToolbar->show();
}

static void newAssembly(QMainWindow* window)
{
    QString path = QFileDialog::getSaveFileName(window, "New Assembly", QString(),
                                                "FreeCAD Files (*.FCStd)");
    if (path.isEmpty())
        return;

    if (!path.endsWith(".FCStd", Qt::CaseInsensitive))
        path.append(".FCStd");

    if (!writeFCStd(path, "App::Part", QFileInfo(path).baseName())) {
        QMessageBox::critical(window, "Error", "Failed to create assembly file.");
        return;
    }

    currentAssemblyPath = path;

    window->setWindowTitle(QString("FreeCAD Ruphobia Addation - Build %1 - %2")
                               .arg(BUILD_NUMBER)
                               .arg(QFileInfo(path).fileName()));

    enterAssemblyMode(window);
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle(QString("FreeCAD Ruphobia Addation - Build %1").arg(BUILD_NUMBER));
    window.resize(1024, 768);

    QMenu* fileMenu = window.menuBar()->addMenu("&File");
    QAction* newAction = fileMenu->addAction("New Assembly");
    QObject::connect(newAction, &QAction::triggered, [&window]() { newAssembly(&window); });

    window.show();

    return app.exec();
}
