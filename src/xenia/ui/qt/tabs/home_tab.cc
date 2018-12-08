#include "xenia/ui/qt/tabs/home_tab.h"
#include <QGraphicsEffect>
#include "xenia/ui/qt/actions/action.h"
#include "xenia/ui/qt/widgets/separator.h"
#include "xenia/ui/qt/widgets/slider.h"

namespace xe {
namespace ui {
namespace qt {

HomeTab::HomeTab() : XTab("Home", "HomeTab") {
  Build();
}

void HomeTab::Build() {
  layout_ = new QHBoxLayout();
  layout_->setMargin(0);
  layout_->setSpacing(0);
  setLayout(layout_);

  BuildSidebar();
  BuildRecentView();
}

void HomeTab::BuildSidebar() {
  // sidebar container widget
  sidebar_ = new QWidget(this);
  sidebar_->setObjectName("sidebarContainer");
  
  QVBoxLayout* sidebar_layout = new QVBoxLayout;
  sidebar_layout->setMargin(0);
  sidebar_layout->setSpacing(0);

  sidebar_->setLayout(sidebar_layout);

  // Add drop shadow to sidebar widget
  QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect;
  effect->setBlurRadius(16);
  effect->setXOffset(4);
  effect->setYOffset(0);
  effect->setColor(QColor(0,0,0,64));

  sidebar_->setGraphicsEffect(effect);

  // Create sidebar
  sidebar_toolbar_ = new XSideBar;
  sidebar_toolbar_->setOrientation(Qt::Vertical);
  sidebar_toolbar_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  sidebar_toolbar_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);

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
  sidebar_toolbar_->addWidget(sidebar_title);

  sidebar_toolbar_->addSpacing(20);

  sidebar_toolbar_->addAction(0xE838, "Open File");
  sidebar_toolbar_->addAction(0xE8F4, "Import Folder");

  sidebar_toolbar_->addSeparator();

  sidebar_layout->addWidget(sidebar_toolbar_, 0, Qt::AlignHCenter | Qt::AlignTop);
  sidebar_layout->addStretch(1);

  // Add sidebar to tab widget
  layout_->addWidget(sidebar_, 0, Qt::AlignLeft);
}

void HomeTab::BuildRecentView() {
    // Create container widget
    QWidget *recent_container = new QWidget(this);

    QVBoxLayout *recent_layout = new QVBoxLayout(this);
    recent_layout->setContentsMargins(0, 0, 0, 0);
    recent_layout->setSpacing(0);

    recent_container->setLayout(recent_layout);

    // Setup toolbar
    auto toolbar = recent_toolbar_;
    toolbar = new XToolBar(this);

    QLabel *title = new QLabel("Recent Games");
    title->setFont(QFont("Segoe UI", 24));
    title->setStyleSheet("color: white;");

    toolbar->addWidget(title);

    toolbar->addSeparator();

    // TODO: handle button clicks

    toolbar->addAction(new XAction(QChar(0xEDB5), "Play"));
    toolbar->addAction(new XAction(QChar(0xEBE8), "Debug"));
    toolbar->addAction(new XAction(QChar(0xE946), "Info"));

    toolbar->addSeparator();

    toolbar->addAction(new XAction(QChar(0xE8FD), "List"));
    toolbar->addAction(new XAction(QChar(0xF0E2), "Grid"));

    // TODO: hide slider unless "Grid" mode is selected

    auto *slider = new XSlider(Qt::Horizontal, this);
    slider->setRange(48, 96);
    slider->setFixedWidth(100);
    toolbar->addWidget(slider);

    recent_layout->addWidget(toolbar);

    list_view_ = new XGameListView(this);
    recent_layout->addWidget(list_view_);

    layout_->addWidget(recent_container);

    // Lower the widget to prevent overlap with sidebar's shadow
    recent_container->lower();
}

}  // namespace qt
}  // namespace ui
}  // namespace xe
