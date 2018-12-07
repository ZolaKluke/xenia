#include "xenia/ui/qt/main_window.h"
#include <QVBoxLayout>
#include "build/version.h"
#include "xenia/ui/qt/widgets/status_bar.h"

namespace xe {
namespace ui {
namespace qt {

MainWindow::MainWindow() : Themeable<QMainWindow>("MainWindow") {
  // Custom Frame Border
  // Disable for now until windows aero additions are added
  // setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

  shell_ = new XShell(this);
  this->setCentralWidget(shell_);

  XStatusBar* status_bar = new XStatusBar(this);
  this->setStatusBar(status_bar);

  QLabel* build_label = new QLabel;
  build_label->setObjectName("buildLabel");
  build_label->setText(QStringLiteral("Xenia: %1 / %2 / %3")
                           .arg(XE_BUILD_BRANCH)
                           .arg(XE_BUILD_COMMIT_SHORT)
                           .arg(XE_BUILD_DATE));
  // build_label->setFont(QFont("Segoe UI", 10));
  status_bar->addPermanentWidget(build_label);
};

}  // namespace qt
}  // namespace ui
}  // namespace xe