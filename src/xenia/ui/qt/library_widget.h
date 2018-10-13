/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2018 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_UI_QT_LIBRARY_WIDGET_H_
#define XENIA_UI_QT_LIBRARY_WIDGET_H_

#include "xenia/ui/qt/themeable_widget.h"

namespace xe {
namespace ui {
namespace qt {

class LibraryWidget : public Themeable<QWidget> {
  Q_OBJECT
 public:
   explicit LibraryWidget(QWidget* parent = nullptr);

};

}  // namespace qt
}  // namespace ui
}  // namespace xe
#endif