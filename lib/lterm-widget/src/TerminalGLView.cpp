// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#include "TerminalGLView.hpp"
#include "TerminalWidget.hpp"

#include <QOpenGLFunctions>
#include <QPainter>

namespace LTerm {

TerminalGLView::TerminalGLView(TerminalWidget* owner)
    : QOpenGLWidget(owner), _owner(owner)
{
    // Make sure mouse events pass through to the scroll area.
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setFocusPolicy(Qt::NoFocus);
}

void TerminalGLView::initializeGL()
{
    // No custom GL state needed — we use QPainter inside paintGL().
    auto* f = QOpenGLContext::currentContext()->functions();
    f->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void TerminalGLView::resizeGL(int /*w*/, int /*h*/)
{
    // TerminalWidget::resizeEvent handles dimension recalculation.
}

void TerminalGLView::paintGL()
{
    // Delegate all painting to TerminalWidget's rendering logic.
    // QPainter on a QOpenGLWidget uses Qt's OpenGL paint engine.
    _owner->_paintGL(this);
}

} // namespace LTerm
