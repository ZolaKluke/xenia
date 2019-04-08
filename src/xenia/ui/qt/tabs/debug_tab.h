#ifndef XENIA_UI_QT_TABS_DEBUG_H_
#define XENIA_UI_QT_TABS_DEBUG_H_

// This tab should not be shown in xenia releases
#ifdef DEBUG

#include <QHBoxLayout>
#include "xenia/ui/qt/widgets/sidebar.h"
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
  void BuildSidebar();

  QWidget* CreateSliderGroup();
  QWidget* CreateCheckboxGroup();
  QWidget* CreateRadioButtonGroup();
  QWidget* CreateGroupBoxGroup();

  QHBoxLayout* layout_ = nullptr;
  QWidget* sidebar_container_ = nullptr;
  XSideBar* sidebar_ = nullptr;
};

}  // namespace qt
}  // namespace ui
}  // namespace xe

#endif

#endif