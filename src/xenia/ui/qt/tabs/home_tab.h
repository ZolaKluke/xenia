#ifndef XENIA_UI_QT_TABS_HOME_H_
#define XENIA_UI_QT_TABS_HOME_H_

#include <QToolBar>
#include "xenia/ui/qt/widgets/sidebar.h"
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

  QVBoxLayout* layout_ = nullptr;
  XSideBar* sidebar_ = nullptr;

  QVector<XSideBarButton*> buttons_;
};

}  // namespace qt
}  // namespace ui
}  // namespace xe

#endif XENIA_UI_QT_TABS_HOME_H_
