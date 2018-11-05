#ifndef XENIA_APP_SCANNER_UTILS_H_
#define XENIA_APP_SCANNER_UTILS_H_

#include "xenia/base/filesystem.h"
#include "xenia/base/string_util.h"
#include "xenia/vfs/device.h"
#include "xenia/vfs/devices/disc_image_device.h"
#include "xenia/vfs/devices/host_path_device.h"
#include "xenia/vfs/devices/stfs_container_device.h"
#include "xenia/vfs/file.h"

#include <string>

namespace xe {
namespace app {
using std::wstring;
using vfs::Device;
using vfs::File;

enum XGameFormat {
  kIso,
  kStfs,
  kXex,
};

inline const wstring GetFileExtension(const wstring& path) {
  auto index = path.find_last_of('.');

  if (index < 0) {
    return L"";
  }

  return path.substr(index + 1);
}

inline const wstring GetParentDirectory(const wstring& path) {
  auto index = path.find_last_of('/');
  return path.substr(0, index);
}

inline uint8_t* Read(File* file, size_t offset = 0, size_t length = 0) {
  if (!length) {
    length = file->entry()->size();
  }

  uint8_t* data = (uint8_t*)calloc(1, length);
  size_t bytes_read;
  file->ReadSync(data, length, offset, &bytes_read);

  return data;
}

inline const XGameFormat ResolveFormat(const wstring& path) {
  const std::wstring& extension = GetFileExtension(path);

  // TODO: Case Insensitivity
  if (extension.compare(L"iso") == 0) return XGameFormat::kIso;
  if (extension.compare(L"xex") == 0) return XGameFormat::kXex;
  // TODO: Stfs Container

  return XGameFormat::kIso;  // TODO
}

inline Device* CreateDevice(const wstring& path) {
  std::string mount_path = "\\SCAN";
  XGameFormat format = ResolveFormat(path);

  switch (format) {
    case XGameFormat::kIso:
      return new vfs::DiscImageDevice(mount_path, path);
    default:
      return nullptr;
  }
}

template <typename T>
inline T Read(File* file, size_t offset) {
  uint8_t* data = Read(file, offset, sizeof(T));
  T swapped = xe::load_and_swap<T>(data);

  delete[] data;
  return swapped;
}

}  // namespace app
}  // namespace xe

#endif  // !define XENIA_APP_SCANNER_UTILS_H_
