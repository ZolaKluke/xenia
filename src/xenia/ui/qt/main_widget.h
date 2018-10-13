/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2018 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_UI_QT_WIDGET_H_
#define XENIA_UI_QT_WIDGET_H_

#include <QKeyEvent>
#include <QMainWindow>
#include <QPainter>
#include <QPushButton>
#include <QToolBar>
#include "themeable_widget.h"

namespace xe {
namespace ui {
namespace qt {

class MainWindow;

class MainWidget : public Themeable<QWidget> {
  Q_OBJECT
 public:
  explicit MainWidget(QWidget* parent = nullptr);

 protected:
  void keyPressEvent(QKeyEvent* e) override;
  void keyReleaseEvent(QKeyEvent* e) override;

 private:
  MainWindow* window_;

 signals:

 public slots:
};

}  // namespace qt
}  // namespace ui
}  // namespace xe

#endif  // WIDGET_H
