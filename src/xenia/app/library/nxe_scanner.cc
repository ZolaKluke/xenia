#include "xenia/app/library/nxe_scanner.h"
#include "xenia/app/library/scanner_utils.h"
#include "xenia/vfs/devices/stfs_container_entry.h"

namespace xe {
namespace app {
using vfs::StfsHeader;

const NxeInfo* NxeScanner::ScanNxe(File* file) {
  NxeInfo* info = new NxeInfo();

  // Read Header
  uint8_t* data = Read(file);
  StfsHeader header;
  header.Read(data);

  // Read Title
  std::wstring title(header.title_name);
  info->game_title = std::string(title.begin(), title.end());

  // Read Icon
  info->icon_size = header.title_thumbnail_image_size;
  info->icon = (uint8_t*)calloc(1, info->icon_size);
  memcpy(info->icon, header.title_thumbnail_image, info->icon_size);

  // TODO: Read nxebg.jpg
  // TODO: Read nxeslot.jpg
  //   How can we open the file with a StfsContainerDevice?

  delete[] data;
  return info;
}

}  // namespace app
}  // namespace xe