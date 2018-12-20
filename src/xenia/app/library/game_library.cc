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

  std::vector<GameInfo*> results;
  for (auto path : paths_) {
    auto found = XGameScanner::ScanPath(path);
    results.insert(results.end(), found.begin(), found.end());
  }

  for (auto result : results) {
    uint32_t title_id = result->xex_info.execution_info.title_id;
    if (!games_map_.count(title_id)) {
      // Create a new XGameEntry with the factory constructor.
      // If a nullptr is returned, the scan info is invalid.
      auto new_entry = XGameEntry::from_game_info(result);
      delete result;

      if (new_entry == nullptr) continue;

      games_.push_back(new_entry);
      games_map_.emplace(std::make_pair(title_id, new_entry));
    } else {
      auto existing = games_map_.at(title_id);
      existing->apply_info(result);
      delete result;
    }
  }
}

const XGameEntry* XGameLibrary::game(const uint32_t title_id) const {
  return games_.at(title_id);
}

const std::vector<XGameEntry*> XGameLibrary::games() const { return games_; }

const size_t XGameLibrary::size() const { return games_.size(); }

void XGameLibrary::clear() {
  for (auto game : games_) {
    delete game;
  }

  games_.clear();
  games_map_.clear();
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
