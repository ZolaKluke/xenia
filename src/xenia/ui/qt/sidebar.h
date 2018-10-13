/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2018 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */
#ifndef XENIA_UI_QT_SIDEBAR_H_
#define XENIA_UI_QT_SIDEBAR_H_

#include <QAction>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#include "themeable_widget.h"

namespace xe {
namespace ui {
namespace qt {

class Sidebar : public Themeable<QWidget> {
 public:
  Sidebar(QWidget* parent = nullptr);

 private:
  QVBoxLayout* layout_;
};

}  // namespace qt
}  // namespace ui
}  // namespace xe

#endif  // XENIA_UI_QT_SIDEBAR_H_
