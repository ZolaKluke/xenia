#ifndef XENIA_APP_SCANNER_UTILS_H_
#define XENIA_APP_SCANNER_UTILS_H_

#include "xenia/base/filesystem.h"
#include "xenia/base/string_util.h"
#include "xenia/vfs/device.h"
#include "xenia/vfs/devices/disc_image_device.h"
#include "xenia/vfs/devices/host_path_device.h"
#include "xenia/vfs/devices/stfs_container_device.h"
#include "xenia/vfs/file.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

namespace xe {
namespace app {
using std::wstring;
using vfs::Device;
using vfs::File;

enum XGameFormat {
  kUnknown,
  kIso,
  kStfs,
  kXex,
};

inline wstring AppendToPath(const wstring& left, const wstring& right) {
  wchar_t separator = xe::kWPathSeparator;
  wstring path(left);

  // Add path separator if not present
  if (path[path.length() - 1] != separator) {
    path = path + separator;
  }

  return path.append(right);
}

inline const bool CompareCaseInsensitive(const wstring& left,
                                         const wstring& right) {
  // Copy strings for transform
  wstring a(left);
  wstring b(right);

  std::transform(a.begin(), a.end(), a.begin(), toupper);
  std::transform(b.begin(), b.end(), b.begin(), toupper);

  return a == b;
}

inline const wstring GetFileExtension(const wstring& path) {
  wstring path_(path);

  // Chop off all but filename
  size_t index = path_.find_last_of(xe::kWPathSeparator);
  if (index != wstring::npos) {
    path_ = path.substr(index);
  }

  // Find the index of the file extension
  index = path_.find_last_of('.');
  if (index == wstring::npos) {
    return L"";
  }

  return path_.substr(index + 1);
}

inline const wstring GetFileName(const wstring& path) {
  size_t index = path.find_last_of(xe::kWPathSeparator);
  if (index == wstring::npos) {
    return path;
  }

  return path.substr(index + 1);
}

inline const wstring GetParentDirectory(const wstring& path) {
  auto index = path.find_last_of(xe::kWPathSeparator);
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

inline std::string ReadFileMagic(const wstring& path, size_t length = 4) {
  char* buffer = new char[length];

  std::ifstream file(path, std::ios::binary);
  if (file.is_open()) {
    file.seekg(0, std::ios::beg);
    file.read(buffer, length);
    file.close();
  }

  std::string magic(buffer);

  delete[] buffer;
  return magic;
}

inline const XGameFormat ResolveFormat(const wstring& path) {
  const std::wstring& extension = GetFileExtension(path);

  if (CompareCaseInsensitive(extension, L"iso")) return XGameFormat::kIso;
  if (CompareCaseInsensitive(extension, L"xex")) return XGameFormat::kXex;

  // STFS Container
  if (extension.length() == 0) {
    std::string magic = ReadFileMagic(path);

    if (magic.compare("LIVE") == 0) return XGameFormat::kStfs;
  }

  return XGameFormat::kUnknown;
}

inline Device* CreateDevice(const wstring& path) {
  std::string mount_path = "\\SCAN";
  XGameFormat format = ResolveFormat(path);

  switch (format) {
    case XGameFormat::kIso:
      return new vfs::DiscImageDevice(mount_path, path);
    case XGameFormat::kXex:
      return new vfs::HostPathDevice(mount_path, GetParentDirectory(path),
                                     true);
    case XGameFormat::kStfs:
      // TODO: Load GOD Container when supported
      return nullptr;  // new vfs::StfsContainerDevice(mount_path, path);
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
