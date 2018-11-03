#include "xenia/ui/qt/delegates/game_listview_delegate.h"
#include "xenia/ui/qt/models/game_library_model.h"

#include <QBitmap>
#include <QImage>
#include <QPainter>

namespace xe {
namespace ui {
namespace qt {

XGameListViewDelegate::XGameListViewDelegate(QWidget* parent)
    : QStyledItemDelegate(parent) {
  QImage mask_image(":resources/graphics/GameIconMask.png");
  icon_mask_ = QPixmap::fromImage(mask_image.createAlphaMask());
}

void XGameListViewDelegate::paint(QPainter* painter,
                                  const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const {
  // Override options
  auto options = QStyleOptionViewItem(option);
  options.state &= (~QStyle::State_HasFocus);

  switch (index.column()) {
    case XGameLibraryModel::kIconColumn: {
      QStyledItemDelegate::paint(painter, options, index);
      auto pixmap = index.data().value<QPixmap>();
      paintIcon(pixmap, painter, options, index);
    } break;
    default:
      QStyledItemDelegate::paint(painter, options, index);
  }
}

void XGameListViewDelegate::paintIcon(QPixmap& icon, QPainter* painter,
                                      const QStyleOptionViewItem& options,
                                      const QModelIndex& index) const {
  // Get the column bounds
  auto width = options.rect.width();
  auto height = options.rect.height();
  auto icon_size = options.rect.height() * 0.78;

  // Scale the Pixmap and mask
  auto scaled_icon = icon.scaled(icon_size, icon_size, Qt::KeepAspectRatio,
                                 Qt::SmoothTransformation);
  auto scaled_mask = icon_mask_.scaled(
      icon_size, icon_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  scaled_icon.setMask(scaled_mask);

  // Calculate the Icon position
  QRectF icon_rect = scaled_icon.rect();
  auto shift_x = (width - icon_size) / 2;
  auto shift_y = (height - icon_size) / 2 + height * index.row();
  icon_rect.translate(shift_x, shift_y);

  painter->setRenderHint(QPainter::Antialiasing);
  painter->drawPixmap(icon_rect, scaled_icon, scaled_icon.rect());
}

QSize XGameListViewDelegate::sizeHint(const QStyleOptionViewItem& option,
                                      const QModelIndex& index) const {
  switch (index.column()) {
    case XGameLibraryModel::kIconColumn:
      return QSize(58, 48);
    default:
      return QSize();
  }
}

}  // namespace qt
}  // namespace ui
}  // namespace xe