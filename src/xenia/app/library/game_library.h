#ifndef XENIA_APP_GAME_LIBRARY_H_
#define XENIA_APP_GAME_LIBRARY_H_

#include "xenia/app/library/game_entry.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace xe {
namespace app {

class XGameLibrary {
 public:
  XGameLibrary(XGameLibrary const&) = delete;
  XGameLibrary& operator=(XGameLibrary const&) = delete;
  static XGameLibrary* Instance();

  bool add_path(const std::wstring& path);
  bool remove_path(const std::wstring& path);
  void scan_paths();

  const XGameEntry* game(const uint32_t title_id) const;
  const std::vector<XGameEntry*> games() const;
  const size_t size() const;

  void clear();
  bool load();
  bool save();

 private:
  XGameLibrary(){};

  std::map<uint32_t, XGameEntry*> games_;
  std::vector<std::wstring> paths_;
};

}  // namespace app
}  // namespace xe

#endif