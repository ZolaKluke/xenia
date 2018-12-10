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
      QImage icon = index.data().value<QImage>();
      QPixmap pixmap = QPixmap::fromImage(icon);

      pixmap.setDevicePixelRatio(painter->device()->devicePixelRatioF());
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
  double width = options.rect.width();
  double height = options.rect.height();
  double icon_size = options.rect.height() * 0.8;

  double dpr = painter->device()->devicePixelRatioF();

  // Scale the Pixmap and mask
  QPixmap scaled_icon = icon.scaled(icon_size, icon_size, Qt::KeepAspectRatio,
                                    Qt::SmoothTransformation);
  scaled_icon.setDevicePixelRatio(dpr);

  QPixmap scaled_mask = icon_mask_.scaled(
      icon_size, icon_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  scaled_icon.setMask(scaled_mask);

  // Calculate the Icon position
  QRectF icon_rect = scaled_icon.rect();
  double shift_x = (width - icon_size) / 2;
  double shift_y = (height - icon_size) / 2 + options.rect.y();
  icon_rect.translate(shift_x, shift_y);

  painter->setRenderHints(QPainter::SmoothPixmapTransform);
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