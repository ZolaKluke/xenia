#ifndef XENIA_UI_QT_TABS_HOME_H_
#define XENIA_UI_QT_TABS_HOME_H_

#include <QToolBar>
#include "xenia/ui/qt/widgets/sidebar.h"
#include "xenia/ui/qt/widgets/toolbar.h"
#include "xenia/ui/qt/widgets/sidebar_button.h"
#include "xenia/ui/qt/widgets/tab.h"

namespace xe {
namespace ui {
namespace qt {

class HomeTab : public XTab {
  Q_OBJECT
 public:
  explicit HomeTab();

 private:
  void Build();
  void BuildSidebar();
  void BuildRecentView();

  QHBoxLayout* layout_ = nullptr;
  QWidget* sidebar_ = nullptr;
  XSideBar* sidebar_toolbar_ = nullptr;
  XToolBar* recent_toolbar_ = nullptr;
  QVector<XSideBarButton*> buttons_;
};

}  // namespace qt
}  // namespace ui
}  // namespace xe

#endif XENIA_UI_QT_TABS_HOME_H_
