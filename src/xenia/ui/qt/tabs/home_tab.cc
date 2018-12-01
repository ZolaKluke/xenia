#include "xenia/ui/qt/tabs/home_tab.h"
#include "xenia/ui/qt/actions/action.h"
#include "xenia/ui/qt/widgets/separator.h"

namespace xe {
namespace ui {
namespace qt {

HomeTab::HomeTab() : XTab("Home", "HomeTab") {
  // TODO: buttons_ needs to be changed to allow adding XSeparator objects to it
  buttons_ = {new XSideBarButton(0xE838, "Open File"),
              new XSideBarButton(0xE8F4, "Import Folder")};
  Build();
}

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

  QVBoxLayout* sidebar_layout = new QVBoxLayout;
  sidebar_layout->setMargin(0);
  sidebar_layout->setSpacing(0);

  sidebar->setLayout(sidebar_layout);

  // Create sidebar title
  QWidget* sidebar_title = new QWidget;
  sidebar_title->setObjectName("title");

  QVBoxLayout* title_layout = new QVBoxLayout;
  title_layout->setMargin(0);
  title_layout->setContentsMargins(0, 40, 0, 0);
  title_layout->setSpacing(0);

  sidebar_title->setLayout(title_layout);

  // Title labels
  QLabel* xenia_title = new QLabel("Xenia");
  xenia_title->setObjectName("titleLabel");

  QLabel* xenia_subtitle = new QLabel("Xbox 360 Emulator");
  xenia_subtitle->setObjectName("subtitleLabel");

  title_layout->addWidget(xenia_title, 0, Qt::AlignHCenter | Qt::AlignBottom);
  title_layout->addWidget(xenia_subtitle, 0, Qt::AlignHCenter | Qt::AlignTop);

  // Title separator
  XSeparator* separator = new XSeparator;
  title_layout->addWidget(separator);

  // Add title components to sidebar
  sidebar_layout->addWidget(sidebar_title, 0, Qt::AlignHCenter | Qt::AlignTop);

  // Create Toolbar (TODO: is it worth moving to new function?)
  toolbar_ = new QToolBar;
  toolbar_->setOrientation(Qt::Vertical);
  toolbar_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

  for (const auto& btn : buttons_) {
    btn->setFixedHeight(60);
    btn->setFixedWidth(300);

    toolbar_->addWidget(btn);
  }
  toolbar_->addSeparator();

  sidebar_layout->addWidget(toolbar_, 0, Qt::AlignHCenter | Qt::AlignTop);
  sidebar_layout->addStretch(1);
  // Add sidebar to tab widget
  layout_->addWidget(sidebar, 0, Qt::AlignLeft);
}

}  // namespace qt
}  // namespace ui
}  // namespace xe
