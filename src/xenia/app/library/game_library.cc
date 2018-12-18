#include "xenia/app/library/game_library.h"

#include <algorithm>

namespace xe {
namespace app {

XGameLibrary* XGameLibrary::Instance() {
  static XGameLibrary* instance = new XGameLibrary;
  return instance;
}

bool XGameLibrary::add_path(const std::wstring& path) {
  auto existing = std::find(paths_.begin(), paths_.end(), path);
  if (existing != paths_.end()) {
    return false;  // Path already exists.
  }

  paths_.push_back(path);
  return true;
}

bool XGameLibrary::remove_path(const std::wstring& path) {
  auto existing = std::find(paths_.begin(), paths_.end(), path);
  if (existing == paths_.end()) {
    return false;  // Path does not exist.
  }

  paths_.erase(existing);
  return true;
}

void XGameLibrary::scan_paths() {
  clear();

  std::vector<GameInfo> results;
  for (auto path : paths_) {
    auto found = XGameScanner::ScanPath(path);
    results.insert(results.end(), found.begin(), found.end());
  }

  for (auto result : results) {
    uint32_t title_id = result.xex_info->header->execution_info.title_id;

    if (!games_.count(title_id)) {
      // Create a new XGameEntry with the factory constructor.
      // If a nullptr is returned, the scan info is invalid.
      auto new_entry = XGameEntry::from_game_info(result);
      if (new_entry == nullptr) continue;

      games_.emplace(std::make_pair(title_id, new_entry));
    } else {
      auto existing = games_.at(title_id);
      existing->apply_info(result);
    }
  }
}

const XGameEntry* XGameLibrary::game(const uint32_t title_id) const {
  return games_.at(title_id);
}

const std::vector<XGameEntry*> XGameLibrary::games() const {
  std::vector<XGameEntry*> games;

  for (auto game : games_) {
    games.push_back(game.second);
  }

  return games;
}

const size_t XGameLibrary::size() const { return games_.size(); }

void XGameLibrary::clear() {
  for (auto game : games_) {
    auto game_ptr = game.second;
    delete game_ptr;
  }

  games_.clear();
}

bool XGameLibrary::load() {
  // TODO
  return false;
}

bool XGameLibrary::save() {
  // TODO
  return false;
}

}  // namespace app
}  // namespace xe
