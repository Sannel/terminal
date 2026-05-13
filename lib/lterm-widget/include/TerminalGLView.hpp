// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#pragma once

#include <QOpenGLWidget>

namespace LTerm {

class TerminalWidget;

/**
 * TerminalGLView — OpenGL viewport for TerminalWidget.
 *
 * Installed via QAbstractScrollArea::setViewport(). All cell rendering
 * runs inside paintGL() using QPainter, which Qt6 routes through the
 * OpenGL paint engine for GPU-accelerated output.
 */
class TerminalGLView : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit TerminalGLView(TerminalWidget* owner);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    TerminalWidget* _owner;
};

} // namespace LTerm
