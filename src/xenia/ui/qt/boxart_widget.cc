/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2018 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "boxart_widget.h"
#include <QBitmap>

namespace xe {
namespace ui {
namespace qt {

BoxArtWidget::BoxArtWidget(GameEntry* game, QWidget* parent) {
  game_ = game;

  setAutoFillBackground(true);
  setFixedSize(120, 168);

  layout_ = new QVBoxLayout();
  layout_->setMargin(0);
  setLayout(layout_);

  QImage placeholder(":box-art-placeholder.png");
  placeholder = placeholder.scaled(size());
  placeholder.convertToFormat(QImage::Format_ARGB32);

  QPalette palette;
  palette.setColor(QPalette::Background, Qt::transparent);

  artlabel_ = new QLabel;
  artlabel_->setBackgroundRole(QPalette::Base);
  artlabel_->setPalette(palette);
  artlabel_->setScaledContents(true);
  artlabel_->setPixmap(QPixmap::fromImage(placeholder));
  artlabel_->setMask(QPixmap::fromImage(placeholder).mask());

  layout_->addWidget(artlabel_);
}

void BoxArtWidget::SetArt(QImage art) {
  art_ = art;
  artlabel_->setPixmap(QPixmap::fromImage(art));
}

}  // namespace qt
}  // namespace ui
}  // namespace xe