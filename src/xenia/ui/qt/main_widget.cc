/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2018 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <QApplication>
#include <QLabel>
#include <QLayout>
#include <QMenuBar>
#include <QStyle>
#include <QUrl>

#include "boxart_widget.h"
#include "main_widget.h"
#include "main_window.h"
#include "sidebar.h"
#include "theme_manager.h"
#include "xenia/app/game_library.h"

namespace xe {
namespace ui {
namespace qt {

MainWidget::MainWidget(QWidget* parent)
    : Themeable<QWidget>("MainWidget", parent) {
  window_ = static_cast<MainWindow*>(parent);

  QHBoxLayout* layout = new QHBoxLayout();
  layout->setContentsMargins(0, 0, 0, 0);
  setLayout(layout);

  Sidebar* sidebar = new Sidebar();
  layout->addWidget(sidebar, 0, Qt::AlignLeft);

  QWidget* main_container = new QWidget();
  main_container->setStyleSheet("background: rgb(40,40,40)");
  QGridLayout* main_container_layout = new QGridLayout();
  main_container_layout->setSpacing(15);
  main_container->setObjectName("main_container");

  std::shared_ptr<app::GameLibrary> library = app::GameLibrary::Instance();
  GameEntry* game = library->games()[0];

  QImage PLACEHOLDER(":box-art.jpg");
  PLACEHOLDER.convertToFormat(QImage::Format_ARGB32);

  for (int i = 0; i < 12; i++) {
    BoxArtWidget* art = new BoxArtWidget(game);
    if( i == 0 )
      art->SetArt(PLACEHOLDER);
    main_container_layout->addWidget(art, i / 4, i % 4);
  }
  main_container->setLayout(main_container_layout);

  layout->addWidget(main_container, 1);
}

void MainWidget::keyPressEvent(QKeyEvent* e) {
  if (e->key() == Qt::Key_Alt) {
    if (window_) {
      window_->menuBar()->show();
    }
  }
}

void MainWidget::keyReleaseEvent(QKeyEvent* e) {
  if (e->key() == Qt::Key_Alt) {
    if (window_) {
      window_->menuBar()->hide();
    }
  }
}

}  // namespace qt
}  // namespace ui
}  // namespace xe
