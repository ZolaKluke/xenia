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
  // sidebar container widget
  QWidget* sidebar_widget = new QWidget;
  sidebar_widget->setObjectName("sidebarContainer");

  // Create sidebar
  sidebar_ = new XSideBar;
  sidebar_->setOrientation(Qt::Vertical);
  sidebar_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  sidebar_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);

  QVBoxLayout* sidebar_layout = new QVBoxLayout;
  sidebar_layout->setMargin(0);
  sidebar_layout->setSpacing(0);

  sidebar_widget->setLayout(sidebar_layout);

  // Create sidebar title
  QWidget* sidebar_title = new QWidget;
  sidebar_title->setObjectName("sidebarTitle");

  QVBoxLayout* title_layout = new QVBoxLayout;
  title_layout->setMargin(0);
  title_layout->setContentsMargins(0, 40, 0, 0);
  title_layout->setSpacing(0);

  sidebar_title->setLayout(title_layout);

  // Title labels
  QLabel* xenia_title = new QLabel("Xenia");
  xenia_title->setObjectName("sidebarTitleLabel");

  QLabel* xenia_subtitle = new QLabel("Xbox 360 Emulator");
  xenia_subtitle->setObjectName("sidebarSubtitleLabel");

  title_layout->addWidget(xenia_title, 0, Qt::AlignHCenter | Qt::AlignBottom);
  title_layout->addWidget(xenia_subtitle, 0, Qt::AlignHCenter | Qt::AlignTop);

  // Title separator
  auto separator = new XSeparator;
  title_layout->addWidget(separator, 0, Qt::AlignHCenter);

  // Setup Sidebar toolbar
  sidebar_->addWidget(sidebar_title);

  sidebar_->addAction(0xE838, "Open File");
  sidebar_->addAction(0xE8F4, "Import Folder");
  sidebar_->addSeparator();

  sidebar_layout->addWidget(sidebar_, 0, Qt::AlignHCenter | Qt::AlignTop);
  sidebar_layout->addStretch(1);

  // Add sidebar to tab widget
  layout_->addWidget(sidebar_widget, 0, Qt::AlignLeft);
}

}  // namespace qt
}  // namespace ui
}  // namespace xe
