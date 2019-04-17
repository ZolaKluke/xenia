#ifndef XENIA_UI_QT_TABS_DEBUG_H_
#define XENIA_UI_QT_TABS_DEBUG_H_

// This tab should not be shown in xenia releases
#ifdef DEBUG

#include <QHBoxLayout>
#include <QStackedLayout>

#include "xenia/ui/qt/widgets/sidebar.h"
#include "xenia/ui/qt/widgets/tab.h"

namespace xe {
namespace ui {
namespace qt {

// TODO: should this be in its own file for reusability?
// Represents a sidebar item and a widget that is shown when the item is clicked
struct SidebarItem {
  QChar glyph;
  const char* name;
  QWidget* widget;
};

class DebugTab : public XTab {
  Q_OBJECT
 public:
  explicit DebugTab();

 private:
  void Build();
  void BuildSidebar();

  QWidget* CreateComponentsTab();
  QWidget* CreateNavigationTab();
  QWidget* CreateThemeTab();
  QWidget* CreateLibraryTab();

  // create widgets for "components" tab
  QWidget* CreateSliderGroup();
  QWidget* CreateCheckboxGroup();
  QWidget* CreateRadioButtonGroup();

  QHBoxLayout* layout_ = nullptr;
  QWidget* sidebar_container_ = nullptr;
  XSideBar* sidebar_ = nullptr;
  QStackedLayout* content_layout_ = nullptr;
  QList<SidebarItem> sidebar_items_;
};

}  // namespace qt
}  // namespace ui
}  // namespace xe

#endif

#endif