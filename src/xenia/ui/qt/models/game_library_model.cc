#include "xenia/ui/qt/models/game_library_model.h"
#include "xenia/base/string_util.h"

#include <QIcon>
#include <QLabel>

namespace xe {
namespace ui {
namespace qt {

XGameLibraryModel::XGameLibraryModel(QObject* parent) {
  library_ = XGameLibrary::Instance();
}

QVariant XGameLibraryModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }

  std::shared_ptr<XGameEntry> entry = library_->games()[index.row()];

  switch (index.column()) {
    case kIconColumn:
      if(role == Qt::DisplayRole) {
        QPixmap pixmap;
        pixmap.loadFromData(entry->icon(), (int)entry->icon_size());
        return pixmap;
      }
      break;
    case kTitleColumn:
      if (role == Qt::DisplayRole) {
        return QString::fromStdString(entry->title());
      }
      break;
    case kTitleIdColumn:
      if (role == Qt::DisplayRole) {
        auto title_id = xe::string_util::to_hex_string(entry->title_id());
        return QString::fromStdString(title_id);
      }
      break;
    case kMediaIdColumn:
      if (role == Qt::DisplayRole) {
        auto media_id = xe::string_util::to_hex_string(entry->media_id());
        return QString::fromStdString(media_id);
      }
      break;
    case kPathColumn:
      if (role == Qt::DisplayRole) {
        return QString::fromStdString(entry->file_path());
      }
      break;
    case kVersionColumn:
      if (role == Qt::DisplayRole) {
        auto version = entry->version();
        QString version_str;
        version_str.sprintf("v%d.%d.%d", version.major, version.minor,
                            version.build);
        return version_str;
      }
    case kGenreColumn:
      if (role == Qt::DisplayRole) {
        return QString::fromStdString(entry->genre());
      }
      break;
    case kReleaseDateColumn:
      if (role == Qt::DisplayRole) {
        return QString::fromStdString(entry->release_date());
      }
      break;
    case kBuildDateColumn:
      if (role == Qt::DisplayRole) {
        return QString::fromStdString(entry->build_date());
      }
      break;
    case kLastPlayedColumn:
      return QVariant();  // TODO
    case kTimePlayedColumn:
      return QVariant();  // TODO
    case kAchievementsUnlockedColumn:
      return QVariant();  // TODO
    case kGamerscoreUnlockedColumn:
      return QVariant();  // TODO
    case kGameRatingColumn:
      return QVariant();  // TODO
    case kGameRegionColumn:
      if (role == Qt::DisplayRole) {
        auto region = RegionStringMap.find(entry->regions());
        if(region != RegionStringMap.end()) {
          return region->second;
        }
      }
      break;
    case kCompatabilityColumn:
      return QVariant();  // TODO
    case kPlayerCountColumn:
      if (role == Qt::DisplayRole) {
        return QString::number(entry->player_count());
      }
      break;
  }

  return QVariant();
}

QVariant XGameLibraryModel::headerData(int section, Qt::Orientation orientation,
                                    int role) const {
  if (orientation == Qt::Vertical || role != Qt::DisplayRole) {
    return QVariant();
  }

  switch (section) {
    case kIconColumn:
      return tr("");
    case kTitleColumn:
      return tr("Title");
    case kTitleIdColumn:
      return tr("Title ID");
    case kMediaIdColumn:
      return tr("Media ID");
    case kPathColumn:
      return tr("Path");
    case kVersionColumn:
      return tr("Version");
    case kGenreColumn:
      return tr("Genre");
    case kReleaseDateColumn:
      return tr("Release Date");
    case kBuildDateColumn:
      return tr("Build Date");
    case kLastPlayedColumn:
      return tr("Last Played");
    case kTimePlayedColumn:
      return tr("Time Played");
    case kAchievementsUnlockedColumn:
      return tr("Achievements");
    case kGamerscoreUnlockedColumn:
      return tr("Gamerscore");
    case kGameRatingColumn:
      return tr("Rating");
    case kGameRegionColumn:
      return tr("Region");
    case kCompatabilityColumn:
      return tr("Compatibility");
    case kPlayerCountColumn:
      return tr("# Players");
    default:
      return QVariant();  // Should not be seeing this
  }
}

int XGameLibraryModel::rowCount(const QModelIndex& parent) const {
  if (parent.isValid()) {  // TODO
    return 0;
  }
  return library_->size();
}

int XGameLibraryModel::columnCount(const QModelIndex& parent) const {
  return kColumnCount;
}

}  // namespace qt
}  // namespace ui
}  // namespace xe