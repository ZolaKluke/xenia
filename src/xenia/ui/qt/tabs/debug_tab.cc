#include "xenia/ui/qt/tabs/debug_tab.h"

#ifdef DEBUG

namespace xe {
namespace ui {
namespace qt {

DebugTab::DebugTab() : XTab("Debug", "DebugTab") { Build(); }

void DebugTab::Build() {
  layout_ = new QVBoxLayout();
  layout_->setContentsMargins(0, 0, 0, 0);
  layout_->setSpacing(0);
  setLayout(layout_);

  BuildCard();
}

void DebugTab::BuildCard() {
  card_ = new XCard("Debug");

  layout_->addSpacing(100);
  layout_->addWidget(card_, 0, Qt::AlignHCenter);
}

}  // namespace qt
}  // namespace ui
}  // namespace xe

#endif