

#include "xenia/app/game_scanner.h"
#include "third_party/crypto/rijndael-alg-fst.h"
#include "xenia/base/string_util.h"
#include "xenia/kernel/util/xex2_info.h"
#include "xenia/vfs/devices/disc_image_device.h"
#include "xenia/vfs/devices/host_path_device.h"
#include "xenia/vfs/devices/stfs_container_device.h"

#include <QDir>
#include <QDirIterator>

namespace xe {
namespace app {

vector<QString> GameScanner::ScanPaths(const vector<QString>& paths) {
  vector<QString> candidates;

  for (const QString path : paths) {
    vector<QString> path_candidates = ScanPath(path);
    candidates.insert(candidates.end(), path_candidates.begin(),
                      path_candidates.end());
  }

  return candidates;
}

vector<QString> GameScanner::ScanPath(const QString& path) {
  vector<QString> candidates;

  QDir dir(path);
  dir.setNameFilters({"*.xex", "*.iso"});

  if (!dir.exists()) {
    // TODO: Warn about nonexistent path
    return candidates;
  }

  auto flags = QDirIterator::Subdirectories | QDirIterator::FollowSymlinks;
  QDirIterator dir_iterator(dir, flags);

  while (dir_iterator.hasNext()) {
    dir_iterator.next();
    // TODO Filter non-xbox isos here?
    candidates.push_back(dir_iterator.filePath());
  }

  return candidates;
}

vector<GameEntry*> GameScanner::ScanFiles(const vector<QString>& filepaths) {
  vector<GameEntry*> games;

  for (const QString filepath : filepaths) {
    GameEntry* game = ScanFile(filepath);
    games.push_back(game);
  }
}

GameEntry* GameScanner::ScanFile(const QString& filepath) {
  auto ext = QFileInfo(filepath).suffix().toLower();

  if (ext == "iso") return ScanIso(filepath);
  if (ext == "xex") return ScanXex(filepath);
  throw 0x1;  // TODO: File format not supported
}

GameEntry* GameScanner::ScanIso(const QString& filepath) {
  QFileInfo info(filepath);
  auto mount_path = "\\Device\\iso";
  auto file_path = info.absoluteFilePath().toStdWString();
  auto device = std::make_unique<vfs::DiscImageDevice>(mount_path, file_path);
  if (!device->Initialize()) {
    throw 0x2;  // TODO: Could not initialize DiscImageDevice
  }

  auto xex_entry = device->ResolvePath("default.xex");
  if (!xex_entry) {
    throw 0x3;  // TODO: ISO does not contain a default.xex file
  }

  vfs::File* xex_file = nullptr;
  X_STATUS result = xex_entry->Open(vfs::FileAccess::kFileReadData, &xex_file);
  if (XFAILED(result)) {
    throw 0x4;  // TODO: Could not open ISO default.xex for reading.
  }

  Xex* xex = ReadXex(xex_file);
  // TODO: Set path

  xex_file->Destroy();     // Destruct the file handle
  device.reset();          // Destruct the device
  return new GameEntry();  // TODO
}

GameEntry* GameScanner::ScanXex(const QString& filepath) {
  QFileInfo info(filepath);
  auto mount_path = "\\Device\xex";
  auto file_path = info.absolutePath().toStdWString();
  auto device =
      std::make_unique<vfs::HostPathDevice>(mount_path, file_path, true);
  if (!device->Initialize()) {
    throw 0x5;  // TODO: Could not open HostPathDevice
  }

  auto xex_entry = device->ResolvePath("default.xex");
  if (!xex_entry) {
    throw 0x6;  // TODO: Path does not contain a default.xex file
  }

  vfs::File* xex_file = nullptr;
  X_STATUS result = xex_entry->Open(vfs::FileAccess::kFileReadData, &xex_file);
  if (XFAILED(result)) {
    throw 0x7;  // TODO: Could not open default.xex for reading
  }

  Xex* xex = ReadXex(xex_file);
  // TODO: Set path

  xex_file->Destroy();     // Destruct the file handle
  device.reset();          // Destruct the device
  return new GameEntry();  // TODO
}

///////////////////////////////////////////////////////////////////////////////

Xex* GameScanner::ReadXex(vfs::File* xex_file) {
  Xex* xex = new Xex();
  xex->file = xex_file;
  xex->size = xex_file->entry()->size();

  if (XFAILED(XexVerifyMagic(xex))) {
    throw 0x8;  // TODO: XEX magic does not match
  }

  XexReadHeader(xex);

  return nullptr;  // TODO
}

X_STATUS GameScanner::XexVerifyMagic(Xex* xex) {
  uint32_t magic = xex->Read<uint32_t>(0x0);
  return magic == XEX2_MAGIC ? X_STATUS_SUCCESS : X_STATUS_UNSUCCESSFUL;
}

X_STATUS GameScanner::XexReadHeader(Xex* xex) {
  uint32_t header_size = xex->Read<uint32_t>(0x8);
  uint8_t* data = xex->Read(0x0, header_size);

  // Read Primary Header Data
  auto header = &xex->header;
  header->xex2 = xe::load_and_swap<uint32_t>(data + 0x00);
  header->module_flags =
      (xe_xex2_module_flags)xe::load_and_swap<uint32_t>(data + 0x04);
  header->exe_offset = xe::load_and_swap<uint32_t>(data + 0x08);
  header->unknown0 = xe::load_and_swap<uint32_t>(data + 0x0C);
  header->certificate_offset = xe::load_and_swap<uint32_t>(data + 0x10);
  header->header_count = xe::load_and_swap<uint32_t>(data + 0x14);

  // Read Optional Headers
  uint8_t* cursor = data + 0x18;
  for (int i = 0; i < header->header_count; i++, cursor += 0x8) {
    header->headers[i] = XexReadOptionalHeader(xex, cursor);
  }

  XexReadLoaderInfo(xex, header->certificate_offset);
  XexReadSectionInfo(xex, header->certificate_offset + 0x180);
  XexDecryptHeaderKey(xex);

  delete[] data;
  return X_STATUS_SUCCESS;
}

X_STATUS GameScanner::XexReadLoaderInfo(Xex* xex, uint32_t offset) {
  uint32_t length = 0x180;
  uint8_t* data = xex->Read(offset, length);

  auto security = &xex->header.loader_info;
  security->header_size = xe::load_and_swap<uint32_t>(data + 0x000);
  security->image_size = xe::load_and_swap<uint32_t>(data + 0x004);
  memcpy(security->rsa_signature, data + 0x008, 0x100);
  security->unklength = xe::load_and_swap<uint32_t>(data + 0x108);
  security->image_flags =
      (xe_xex2_image_flags)xe::load_and_swap<uint32_t>(data + 0x10C);
  security->load_address = xe::load_and_swap<uint32_t>(data + 0x110);
  memcpy(security->section_digest, data + 0x114, 0x14);
  security->import_table_count = xe::load_and_swap<uint32_t>(data + 0x128);
  memcpy(security->import_table_digest, data + 0x12C, 0x14);
  memcpy(security->media_id, data + 0x140, 0x10);
  memcpy(security->file_key, data + 0x150, 0x10);
  security->export_table = xe::load_and_swap<uint32_t>(data + 0x160);
  memcpy(security->header_digest, data + 0x164, 0x14);
  security->game_regions =
      (xe_xex2_region_flags)xe::load_and_swap<uint32_t>(data + 0x178);
  security->media_flags =
      (xe_xex2_media_flags)xe::load_and_swap<uint32_t>(data + 0x17C);

  delete[] data;
  return X_STATUS_SUCCESS;
}

X_STATUS GameScanner::XexReadSectionInfo(Xex* xex, uint32_t offset) {
  uint32_t count = xex->Read<uint32_t>(offset);
  uint32_t size = sizeof(xe_xex2_section_t);
  uint32_t length = count * size;
  uint8_t* data = xex->Read(offset, length);

  auto header = &xex->header;
  header->section_count = count;
  header->sections = (xe_xex2_section_t*)calloc(count, size);

  if (!header->sections || !count) {
    return X_STATUS_UNSUCCESSFUL;
  }

  uint8_t* cursor = data + 0x04;
  for (uint i = 0; i < count; i++) {
    auto section = &header->sections[i];

    // Determine page size (4kb/64kb)
    uint32_t image_flags = header->loader_info.image_flags;
    uint32_t page_size_4kb_flag = xe_xex2_image_flags::XEX_IMAGE_PAGE_SIZE_4KB;
    bool is_4kb = image_flags & page_size_4kb_flag;
    section->page_size = is_4kb ? 0x1000 : 0x10000;

    section->info.value = xe::load_and_swap<uint32_t>(cursor + 0x00);
    memcpy(section->digest, cursor + 0x04, sizeof(section->digest));
    cursor += 0x04 + sizeof(section->digest);
  }

  delete[] data;
  return X_STATUS_SUCCESS;
}

X_STATUS GameScanner::XexDecryptHeaderKey(Xex* xex) {
  auto Decrypt = [](Xex* xex, const uint8_t* key) {
    auto header = &xex->header;
    uint32_t rk[4 * (MAXNR + 1)];
    uint32_t Nr = rijndaelKeySetupDec(rk, key, 128);
    rijndaelDecrypt(rk, Nr, header->loader_info.file_key, header->session_key);

    // Check key validity by searching for PE Magic
    uint8_t enc_buffer[0x10] = {0};
    uint8_t dec_buffer[0x10] = {0};
    uint8_t* block = xex->Read(header->exe_offset, 0x10);

    memcpy(enc_buffer, block, 0x10);

    Nr = rijndaelKeySetupDec(rk, header->session_key, 128);
    rijndaelDecrypt(rk, Nr, enc_buffer, dec_buffer);

    const uint16_t PE_MAGIC = 0x4D5A;  // "MZ"
    uint16_t magic = xe::load_and_swap<uint16_t>(dec_buffer);

    return magic == PE_MAGIC;
  };

  return Decrypt(xex, xe_xex2_retail_key)
             ? X_STATUS_SUCCESS
             : Decrypt(xex, xe_xex2_devkit_key) ? X_STATUS_SUCCESS
                                                : X_STATUS_UNSUCCESSFUL;
}

XexOptHeader GameScanner::XexReadOptionalHeader(Xex* xex, uint8_t* cursor) {
  const uint32_t key = xe::load_and_swap<uint32_t>(cursor + 0x00);
  const uint32_t value = xe::load_and_swap<uint32_t>(cursor + 0x04);

  switch (key) {
    case XEX_HEADER_ALTERNATE_TITLE_IDS:
      return XexReadAlternateTitleIds(xex, value);
    case XEX_HEADER_DEFAULT_FILESYSTEM_CACHE_SIZE:
      return XexReadDefaultFsCacheSize(xex, value);
    case XEX_HEADER_DEFAULT_HEAP_SIZE:
      return XexReadDefaultHeapSize(xex, value);
    case XEX_HEADER_DEFAULT_STACK_SIZE:
      return XexReadDefaultStackSize(xex, value);
    case XEX_HEADER_DEVICE_ID:
      return XexReadDeviceId(xex, cursor + 0x04);
      // TODO: WHAT IS THIS: not working?
    case XEX_HEADER_EXECUTION_INFO:
      return XexReadExecutionInfo(xex, value);
    case XEX_HEADER_FILE_FORMAT_INFO:
      return XexReadFileFormatInfo(xex, value);
    case XEX_HEADER_GAME_RATINGS:
      return XexReadGameRatings(xex, value);
    case XEX_HEADER_LAN_KEY:
      return XexReadLanKey(xex, value);
    case XEX_HEADER_MULTIDISC_MEDIA_IDS:
      return XexReadMultiDiscMediaIds(xex, value);
    case XEX_HEADER_RESOURCE_INFO:
      return XexReadResourceInfo(xex, value);
    case XEX_HEADER_ORIGINAL_BASE_ADDRESS:
      return XexReadOriginalBaseAddress(xex, value);
    case XEX_HEADER_ORIGINAL_PE_NAME:
      return XexReadOriginalPeName(xex, value);
    case XEX_HEADER_IMAGE_BASE_ADDRESS:
      return XexReadImageBaseAddress(xex, value);
    case XEX_HEADER_SYSTEM_FLAGS:
      return XexReadSystemFlags(xex, value);
  }

  // Return unparsed header if not implemented
  XexOptHeader unknown_opt_header;
  unknown_opt_header.key = key;
  unknown_opt_header.value = value;
  return unknown_opt_header;
}

XexOptHeader GameScanner::XexReadAlternateTitleIds(Xex* xex, uint32_t offset) {
  uint32_t length = xex->Read<uint32_t>(offset);
  uint32_t count = (length - 0x04) / 0x04;
  uint8_t* data = xex->Read(offset, length);

  XexOptHeader opt_header;
  opt_header.key = XEX_HEADER_ALTERNATE_TITLE_IDS;
  opt_header.offset = offset;
  opt_header.length = length;

  auto header = &xex->header;
  header->alt_title_id_count = count;
  header->alt_title_ids = (uint32_t*)calloc(length, sizeof(uint32_t));

  uint8_t* cursor = data + 0x04;
  for (uint i = 0; i < length; i++, cursor += 0x04) {
    xex->header.alt_title_ids[i] = xe::load_and_swap<uint32_t>(cursor);
  }

  delete[] data;
  return opt_header;
}

XexOptHeader GameScanner::XexReadDefaultFsCacheSize(Xex* xex, uint32_t value) {
  XexOptHeader opt_header;
  opt_header.key = XEX_HEADER_DEFAULT_FILESYSTEM_CACHE_SIZE;
  opt_header.value = value;

  xex->header.default_filesystem_cache_size = xe::byte_swap<uint32_t>(value);

  return opt_header;
}

XexOptHeader GameScanner::XexReadDefaultHeapSize(Xex* xex, uint32_t value) {
  XexOptHeader opt_header;
  opt_header.key = XEX_HEADER_DEFAULT_HEAP_SIZE;
  opt_header.value = value;

  xex->header.default_heap_size = xe::byte_swap<uint32_t>(value);

  return opt_header;
}

XexOptHeader GameScanner::XexReadDefaultStackSize(Xex* xex, uint32_t value) {
  XexOptHeader opt_header;
  opt_header.key = XEX_HEADER_DEFAULT_STACK_SIZE;
  opt_header.value = value;

  xex->header.default_stack_size = xe::byte_swap<uint32_t>(value);

  return opt_header;
}

XexOptHeader GameScanner::XexReadDeviceId(Xex* xex, uint8_t* data) {
  XexOptHeader opt_header;
  opt_header.key = XEX_HEADER_DEVICE_ID;

  // TODO

  return opt_header;
}

XexOptHeader GameScanner::XexReadExecutionInfo(Xex* xex, uint32_t offset) {
  uint32_t length = sizeof(xe_xex2_execution_info_t);
  uint8_t* data = xex->Read(offset, length);

  XexOptHeader opt_header;
  opt_header.key = XEX_HEADER_EXECUTION_INFO;
  opt_header.offset = offset;
  opt_header.length = length;

  auto info = &xex->header.execution_info;
  info->media_id = xe::load_and_swap<uint32_t>(data + 0x00);
  info->version.value = xe::load_and_swap<uint32_t>(data + 0x04);
  info->base_version.value = xe::load_and_swap<uint32_t>(data + 0x08);
  info->title_id = xe::load_and_swap<uint32_t>(data + 0x0c);
  info->platform = xe::load_and_swap<uint8_t>(data + 0x10);
  info->executable_table = xe::load_and_swap<uint8_t>(data + 0x11);
  info->disc_number = xe::load_and_swap<uint8_t>(data + 0x12);
  info->disc_count = xe::load_and_swap<uint8_t>(data + 0x13);
  info->savegame_id = xe::load_and_swap<uint8_t>(data + 0x14);

  delete[] data;
  return opt_header;
}

XexOptHeader GameScanner::XexReadFileFormatInfo(Xex* xex, uint32_t offset) {
  uint32_t length = xex->Read<uint32_t>(offset);  // TODO
  uint8_t* data = xex->Read(offset, length);

  XexOptHeader opt_header;
  opt_header.key = XEX_HEADER_FILE_FORMAT_INFO;
  opt_header.offset = offset;
  opt_header.length = length;

  uint8_t* cursor = data;
  auto info = &xex->header.file_format_info;
  info->encryption_type =
      (xe_xex2_encryption_type)xe::load_and_swap<uint16_t>(cursor + 0x04);
  info->compression_type =
      (xe_xex2_compression_type)xe::load_and_swap<uint16_t>(cursor + 0x06);

  switch (info->compression_type) {
    case XEX_COMPRESSION_BASIC: {
      auto compression = &info->compression_info.basic;
      uint32_t region_size = xe::load_and_swap<uint32_t>(cursor + 0x00);
      uint32_t block_count = (region_size - 0x08) / 0x08;
      size_t block_size = sizeof(xe_xex2_file_basic_compression_block_t);

      compression->block_count = block_count;
      compression->blocks = (xe_xex2_file_basic_compression_block_t*)calloc(
          block_count, block_size);

      cursor += 0x08;
      for (uint i = 0; i < block_count; i++) {
        auto block = &compression->blocks[i];
        block->data_size = xe::load_and_swap<uint32_t>(cursor + 0x00);
        block->zero_size = xe::load_and_swap<uint32_t>(cursor + 0x04);
        cursor += 0x08;
      }
    } break;
    case XEX_COMPRESSION_NORMAL: {
      auto compression = &info->compression_info.normal;
      uint32_t window_size = xe::load_and_swap<uint32_t>(cursor + 0x08);
      uint32_t window_bits = 0x00;

      for (int i = 0; i < 32; i++, window_bits++) {
        window_size >>= 1;
        if (window_size == 0x00) {
          break;
        }
      }

      compression->window_size = window_size;
      compression->window_bits = window_bits;
      compression->block_size = xe::load_and_swap<uint32_t>(cursor + 0x0C);
      memcpy(compression->block_hash, cursor + 0x10, 0x14);

    } break;
  }

  delete[] data;
  return opt_header;
}

XexOptHeader GameScanner::XexReadGameRatings(Xex* xex, uint32_t offset) {
  uint32_t length = 0xC;
  uint8_t* data = xex->Read(offset, length);

  XexOptHeader opt_header;
  opt_header.key = XEX_HEADER_GAME_RATINGS;
  opt_header.offset = offset;
  opt_header.length = length;

  auto ratings = &xex->header.game_ratings;
  ratings->esrb =
      (xe_xex2_rating_esrb_value)xe::load_and_swap<uint8_t>(data + 0x00);
  ratings->pegi =
      (xe_xex2_rating_pegi_value)xe::load_and_swap<uint8_t>(data + 0x01);
  ratings->pegifi =
      (xe_xex2_rating_pegi_fi_value)xe::load_and_swap<uint8_t>(data + 0x02);
  ratings->pegipt =
      (xe_xex2_rating_pegi_pt_value)xe::load_and_swap<uint8_t>(data + 0x03);
  ratings->bbfc =
      (xe_xex2_rating_bbfc_value)xe::load_and_swap<uint8_t>(data + 0x04);
  ratings->cero =
      (xe_xex2_rating_cero_value)xe::load_and_swap<uint8_t>(data + 0x05);
  ratings->usk =
      (xe_xex2_rating_usk_value)xe::load_and_swap<uint8_t>(data + 0x06);
  ratings->oflcau =
      (xe_xex2_rating_oflc_au_value)xe::load_and_swap<uint8_t>(data + 0x07);
  ratings->oflcnz =
      (xe_xex2_rating_oflc_nz_value)xe::load_and_swap<uint8_t>(data + 0x08);
  ratings->kmrb =
      (xe_xex2_rating_kmrb_value)xe::load_and_swap<uint8_t>(data + 0x09);
  ratings->brazil =
      (xe_xex2_rating_brazil_value)xe::load_and_swap<uint8_t>(data + 0x0A);
  ratings->fpb =
      (xe_xex2_rating_fpb_value)xe::load_and_swap<uint8_t>(data + 0x0B);

  delete[] data;
  return opt_header;
}

XexOptHeader GameScanner::XexReadImageBaseAddress(Xex* xex, uint32_t value) {
  XexOptHeader opt_header;
  opt_header.key = XEX_HEADER_IMAGE_BASE_ADDRESS;
  opt_header.value = value;

  xex->header.exe_address = value;

  return opt_header;
}

XexOptHeader GameScanner::XexReadLanKey(Xex* xex, uint32_t offset) {
  uint32_t length = 0x10;
  uint8_t* data = xex->Read(offset, length);

  XexOptHeader opt_header;
  opt_header.key = XEX_HEADER_LAN_KEY;
  opt_header.offset = offset;
  opt_header.length = length;

  for (uint i = 0; i < 16; i++) {
    xex->header.lan_key[i] = *(data + i);
  }

  delete[] data;
  return opt_header;
}

XexOptHeader GameScanner::XexReadMultiDiscMediaIds(Xex* xex, uint32_t offset) {
  uint32_t length = xex->Read<uint32_t>(offset);
  uint32_t count = (length - 0x04) / 0x10;
  uint32_t entry_size = sizeof(xe_xex2_multi_disc_media_id_t);
  uint8_t* data = xex->Read(offset, length);

  XexOptHeader opt_header;
  opt_header.key = XEX_HEADER_MULTIDISC_MEDIA_IDS;
  opt_header.offset = offset;
  opt_header.length = length;

  auto header = &xex->header;
  header->multi_disc_media_id_count = count;
  header->multi_disc_media_ids =
      (xe_xex2_multi_disc_media_id_t*)calloc(count, entry_size);

  uint8_t* cursor = data + 0x04;
  for (uint i = 0; i < count; i++, cursor += 0x10) {
    auto id = &xex->header.multi_disc_media_ids[i];
    memcpy(id->hash, cursor, 0x0C);
    id->media_id = xe::load_and_swap<uint32_t>(cursor + 0x0C);
  }

  delete[] data;
  return opt_header;
}

XexOptHeader GameScanner::XexReadOriginalBaseAddress(Xex* xex, uint32_t value) {
  XexOptHeader opt_header;
  opt_header.key = XEX_HEADER_ORIGINAL_PE_NAME;
  opt_header.value = value;

  xex->header.original_base_address = xe::byte_swap<uint32_t>(value);

  return opt_header;
}

XexOptHeader GameScanner::XexReadOriginalPeName(Xex* xex, uint32_t offset) {
  uint32_t length = xex->Read<uint32_t>(offset);
  uint8_t* data = xex->Read(offset, length);

  XexOptHeader opt_header;
  opt_header.key = XEX_HEADER_ORIGINAL_PE_NAME;
  opt_header.offset = offset;
  opt_header.length = length;

  uint8_t* cursor = data + 0x04;
  xex->header.original_pe_name = (uint8_t*)calloc(length, 0x01);
  memcpy(xex->header.original_pe_name, cursor, length - 0x04);

  delete[] data;
  return opt_header;
}

XexOptHeader GameScanner::XexReadResourceInfo(Xex* xex, uint32_t offset) {
  uint32_t length = xex->Read<uint32_t>(offset);
  uint32_t count = (length - 0x04) / 0x10;
  uint32_t size = sizeof(xe_xex2_resource_info_t);
  uint8_t* data = xex->Read(offset, length);

  XexOptHeader opt_header;
  opt_header.key = XEX_HEADER_RESOURCE_INFO;
  opt_header.offset = offset;
  opt_header.length = length;

  auto header = &xex->header;
  header->resource_info_count = count;
  header->resource_infos = (xe_xex2_resource_info_t*)calloc(count, size);

  uint8_t* cursor = data + 0x04;
  for (uint i = 0; i < count; i++, cursor += 0x10) {
    auto info = &xex->header.resource_infos[i];
    memcpy(info->name, cursor, 0x08);
    info->address = xe::load_and_swap<uint32_t>(cursor + 0x08);
    info->size = xe::load_and_swap<uint32_t>(cursor + 0x0C);
  }

  delete[] data;
  return opt_header;
}

XexOptHeader GameScanner::XexReadSystemFlags(Xex* xex, uint32_t offset) {
  uint32_t length = sizeof(xe_xex2_system_flags);
  uint8_t* data = xex->Read(offset, length);

  XexOptHeader opt_header;
  opt_header.key = XEX_HEADER_SYSTEM_FLAGS;
  opt_header.offset = offset;
  opt_header.length = length;

  xex->header.system_flags = (xe_xex2_system_flags)*data;

  delete[] data;
  return opt_header;
}

}  // namespace app
}  // namespace xe