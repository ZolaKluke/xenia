#include "xenia/ui/qt/tabs/home_tab.h"

namespace xe {
namespace ui {
namespace qt {

HomeTab::HomeTab() : XTab("Home", "HomeTab") { Build(); }

void HomeTab::Build() {
  layout_ = new QVBoxLayout();
  layout_->setMargin(0);
  layout_->setSpacing(0);
  setLayout(layout_);

  BuildSidebar();
}

void HomeTab::BuildSidebar() {
  // Create sidebar
  QWidget* sidebar = new QWidget;
  sidebar->setObjectName("sidebar");
  sidebar->setStyleSheet("background: #232323");
  sidebar->setFixedWidth(300);

  QVBoxLayout* sidebar_layout = new QVBoxLayout;
  sidebar_layout->setMargin(0);
  sidebar_layout->setSpacing(0);

  sidebar->setLayout(sidebar_layout);

  // Create sidebar title
  QWidget* sidebar_title = new QWidget;
  sidebar_title->setObjectName("title");
  sidebar_title->setFixedHeight(200);

  QVBoxLayout* title_layout = new QVBoxLayout;
  title_layout->setMargin(0);
  title_layout->setContentsMargins(0, 0, 0, 0);
  title_layout->setSpacing(0);

  sidebar_title->setLayout(title_layout);

  QLabel* xenia_title = new QLabel("Xenia");
  xenia_title->setObjectName("titleLabel");

  QLabel* xenia_subtitle = new QLabel("Xbox 360 Emulator");
  xenia_subtitle->setObjectName("subtitleLabel");

  title_layout->addWidget(xenia_title, 0, Qt::AlignCenter | Qt::AlignBottom);
  title_layout->addWidget(xenia_subtitle, 0, Qt::AlignCenter | Qt::AlignTop);

  sidebar_layout->addWidget(sidebar_title, 0, Qt::AlignTop);

  // Add sidebar to tab widget
  layout_->addWidget(sidebar, 0, Qt::AlignLeft);
}

}  // namespace qt
}  // namespace ui
}  // namespace xe
