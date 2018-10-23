#include "xenia/ui/qt/main_window.h"

#include <QVBoxLayout>

namespace xe {
namespace ui {
namespace qt {

MainWindow::MainWindow() {
  this->setStyleSheet("background-color: #1f1f1f;");
  // Custom Frame Border
  setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

  shell_ = new XShell(this);
  this->setCentralWidget(shell_);

};

}  // namespace qt
}  // namespace ui
}  // namespace xe