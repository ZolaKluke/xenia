#ifndef XENIA_APP_GAME_ENTRY_H_
#define XENIA_APP_GAME_ENTRY_H_

#include "xenia/app/library/game_scanner.h"
#include "xenia/kernel/util/xex2_info.h"

#include <map>
#include <vector>

namespace xe {
namespace app {

class XGameEntry final {
 public:
  static XGameEntry* from_game_info(const GameInfo* info);
  ~XGameEntry();

  bool is_valid();
  bool is_missing_data();
  bool apply_info(const GameInfo* info);

  const XGameFormat& format() const { return format_; }
  const std::wstring& file_path() const { return file_path_; }
  const std::wstring& file_name() const { return file_name_; }
  const std::map<std::wstring, uint32_t> launch_paths() const {
    return launch_paths_;
  }
  const std::map<uint32_t, std::wstring> default_launch_paths() const {
    return default_launch_paths_;
  }

  const std::string& title() const { return title_; }
  const uint8_t* icon() const { return icon_; }
  const size_t& icon_size() const { return icon_size_; }
  const uint32_t title_id() const { return title_id_; }
  const uint32_t media_id() const { return media_id_; }
  const std::vector<uint32_t>& alt_title_ids() const { return alt_title_ids_; }
  const std::vector<uint32_t>& alt_media_ids() const { return alt_media_ids_; }
  const std::map<uint8_t, uint32_t>& disc_map() const { return disc_map_; }
  const XGameVersion& version() const { return version_; }
  const XGameVersion& base_version() const { return base_version_; }
  const XGameRatings& ratings() const { return ratings_; }
  const XGameRegions& regions() const { return regions_; }
  const std::string& genre() const { return genre_; }
  const std::string& build_date() const { return build_date_; }
  const std::string& release_date() const { return release_date_; }
  const uint8_t& player_count() const { return player_count_; }

 private:
  explicit XGameEntry(){};

  // File Info
  XGameFormat format_;
  std::wstring file_path_;
  std::wstring file_name_;
  std::map<std::wstring, uint32_t> launch_paths_;          // <Path, MediaId>
  std::map<uint32_t, std::wstring> default_launch_paths_;  // <MediaId, Path>

  // Game Metadata
  std::string title_;
  uint8_t* icon_ = nullptr;
  size_t icon_size_ = 0;
  uint32_t title_id_ = 0;
  uint32_t media_id_ = 0;
  std::vector<uint32_t> alt_title_ids_;
  std::vector<uint32_t> alt_media_ids_;
  std::map<uint8_t, uint32_t> disc_map_;  // <Disc #, MediaID>
  XGameVersion version_;
  XGameVersion base_version_;
  XGameRatings ratings_;
  XGameRegions regions_;
  std::string build_date_;
  std::string genre_;
  std::string release_date_;
  uint8_t player_count_ = 0;
};

}  // namespace app
}  // namespace xe

#endif