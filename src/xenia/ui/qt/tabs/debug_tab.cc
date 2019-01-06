#include "xenia/ui/qt/tabs/debug_tab.h"

#ifdef DEBUG

namespace xe {
namespace ui {
namespace qt {

DebugTab::DebugTab() : XTab("Debug", "DebugTab") { Build(); }

void DebugTab::Build() {
  layout_ = new QGridLayout();
  layout_->setContentsMargins(0, 0, 0, 0);
  layout_->setSpacing(0);
  setLayout(layout_);

  BuildCard();
}

void DebugTab::BuildCard() {
  // this is a placeholder widget to make sure the grid layout is correct
  // TODO: possibly move this to the card class?
  QWidget* card_container = new QWidget(this);

  card_ = new XCard("Debug", card_container);

  layout_->addWidget(card_container, 0, 0, 10, 10);
  layout_->addWidget(card_, 1, 1, 9, 8);
}

}  // namespace qt
}  // namespace ui
}  // namespace xe

#endif