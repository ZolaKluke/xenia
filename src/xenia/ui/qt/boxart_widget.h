/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2018 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_UI_QT_BOXART_WIDGET_H_
#define XENIA_UI_QT_BOXART_WIDGET_H_

#include <QImage>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

#include "themeable_widget.h"
#include "xenia/app/game_entry.h"

namespace xe {
namespace ui {
namespace qt {

using xe::app::GameEntry;

class BoxArtWidget : public Themeable<QWidget> {
  Q_OBJECT
 public:
  BoxArtWidget(GameEntry* game, QWidget* parent = nullptr);

  void SetArt(QImage art);

 private:
  GameEntry* game_;
  QImage art_;
  QLabel* artlabel_;
  QVBoxLayout* layout_;
};

}  // namespace qt
}  // namespace ui
}  // namespace xe

#endif