#ifndef XENIA_UI_QT_TABS_DEBUG_H_
#define XENIA_UI_QT_TABS_DEBUG_H_

// This tab should not be shown in xenia releases
#ifdef DEBUG

#include <QGridLayout>
#include "xenia/ui/qt/widgets/card.h"
#include "xenia/ui/qt/widgets/tab.h"

namespace xe {
namespace ui {
namespace qt {

class DebugTab : public XTab {
  Q_OBJECT
 public:
  explicit DebugTab();

 private:
  void Build();
  void BuildCard();

  QGridLayout* layout_ = nullptr;
  XCard* card_ = nullptr;
};

}  // namespace qt
}  // namespace ui
}  // namespace xe

#endif

#endif