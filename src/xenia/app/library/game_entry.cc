#include "xenia/app/library/game_entry.h"

namespace xe {
namespace app {

XGameEntry::XGameEntry(const GameInfo& info) { apply_info(info); };

XGameEntry::~XGameEntry(){};

bool XGameEntry::is_valid() {
  // Minimum requirements
  return file_path_.length() && file_name_.length() && title_id_ && media_id_;
}

bool XGameEntry::is_missing_data() {
  return title_.length() == 0 || icon_[0] == 0 || disc_map_.size() == 0;
  // TODO: Version
  // TODO: Base Version
  // TODO: Ratings
  // TODO: Regions
}

void XGameEntry::apply_info(const GameInfo& info) {
  auto xex = info.xex_info;
  auto nxe = info.nxe_info;

  format_ = info.format;
  file_path_ = std::string(info.path.begin(), info.path.end());
  // TODO: filename
  title_id_ = xex->header.execution_info.title_id;
  media_id_ = xex->header.execution_info.media_id;
  version_ = xex->header.execution_info.version;
  base_version_ = xex->header.execution_info.base_version;
  ratings_ = xex->header.game_ratings;
  regions_ = xex->header.loader_info.game_regions;

  /*alt_title_ids_.clear();
  for (size_t i = 0; i < xex->header.alt_title_id_count; i++) {
    alt_title_ids_.push_back(xex->header.alt_title_ids[i]);
  }*/

  if (xex->game_title.length() > 0) {
    title_ = xex->game_title;
  } else if (nxe && nxe->game_title.length() > 0) {
    title_ = nxe->game_title;
  }

  if (xex->icon_size > 0) {
    delete[] icon_;
    icon_size_ = xex->icon_size;
    icon_ = (uint8_t*)calloc(1, icon_size_);
    memcpy(icon_, xex->icon, icon_size_);
  } else if (nxe && nxe->icon_size) {
    delete[] icon_;
    icon_size_ = nxe->icon_size;
    icon_ = (uint8_t*)calloc(1, icon_size_);
    memcpy(icon_, nxe->icon, icon_size_);
  }
}

}  // namespace app
}  // namespace xe