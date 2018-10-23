
#include "xenia/ui/qt/widgets/shell.h"

namespace xe {
namespace ui {
namespace qt {

XShell::XShell(QMainWindow* window) {
  window_ = window;
  Build();
}

void XShell::Build() {
  // Build Main Layout
  layout_ = new QVBoxLayout();
  layout_->setContentsMargins(0, 0, 0, 0);
  this->setLayout(layout_);

  // Build Nav
  nav_ = new XNav();
  connect(nav_, SIGNAL(TabChanged), SLOT(TabChanged));
  layout_->addWidget(nav_, 1, Qt::AlignTop);

  // Add Contents View
  contents_ = new QWidget();
  layout_->addWidget(contents_);
}

void XShell::TabChanged(XTab* tab) {
  contents_ = tab;
  update(); // TODO: Not Working
}

}  // namespace qt
}  // namespace ui
}  // namespace xe