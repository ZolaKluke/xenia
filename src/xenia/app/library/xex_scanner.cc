#include "xenia/app/library/xex_scanner.h"
#include "third_party/crypto/TinySHA1.hpp"
#include "third_party/crypto/rijndael-alg-fst.h"
#include "xenia/app/library/scanner_utils.h"
#include "xenia/cpu/lzx.h"

namespace xe {
namespace app {

void aes_decrypt_buffer(const uint8_t* session_key, const uint8_t* input_buffer,
                        const size_t input_size, uint8_t* output_buffer,
                        const size_t output_size) {
  uint32_t rk[4 * (MAXNR + 1)];
  uint8_t ivec[16] = {0};
  int32_t Nr = rijndaelKeySetupDec(rk, session_key, 128);
  const uint8_t* ct = input_buffer;
  uint8_t* pt = output_buffer;
  for (size_t n = 0; n < input_size; n += 16, ct += 16, pt += 16) {
    // Decrypt 16 uint8_ts from input -> output.
    rijndaelDecrypt(rk, Nr, ct, pt);
    for (size_t i = 0; i < 16; i++) {
      // XOR with previous.
      pt[i] ^= ivec[i];
      // Set previous.
      ivec[i] = ct[i];
    }
  }
}

void aes_decrypt_inplace(const uint8_t* session_key, const uint8_t* buffer,
                         const size_t size) {
  uint32_t rk[4 * (MAXNR + 1)];
  uint8_t ivec[0x10] = {0};
  int32_t Nr = rijndaelKeySetupDec(rk, session_key, 128);
  for (size_t n = 0; n < size; n += 0x10) {
    uint8_t* in = (uint8_t*)buffer + n;
    uint8_t out[0x10] = {0};
    rijndaelDecrypt(rk, Nr, in, out);
    for (size_t i = 0; i < 0x10; i++) {
      // XOR with previous.
      out[i] ^= ivec[i];
      // Set previous.
      ivec[i] = in[i];
    }

    // Fast copy
    *(size_t*)in = *(size_t*)out;
    *(size_t*)(in + 0x8) = *(size_t*)(out + 0x8);
  }
}

inline void ReadXexAltTitleIds(uint8_t* data, XexInfo* info) {
  uint32_t length = xe::load_and_swap<uint32_t>(data);
  uint32_t count = (length - 0x04) / 0x04;

  info->alt_title_ids_count = count;
  info->alt_title_ids = (uint32_t*)calloc(length, sizeof(uint32_t));

  uint8_t* cursor = data + 0x04;
  for (uint32_t i = 0; i < length; i++, cursor += 0x04) {
    info->alt_title_ids[i] = xe::load_and_swap<uint32_t>(cursor);
  }
}

inline void ReadXexExecutionInfo(uint8_t* data, XexInfo* info) {
  uint32_t length = sizeof(xex2_opt_execution_info);
  memcpy(&info->execution_info, data, length);
}

inline void ReadXexFileFormatInfo(uint8_t* data, XexInfo* info) {
  uint32_t length = xe::load_and_swap<uint32_t>(data);  // TODO
  info->file_format_info = (xex2_opt_file_format_info*)calloc(1, length);
  memcpy(info->file_format_info, data, length);
}

inline void ReadXexGameRatings(uint8_t* data, XexInfo* info) {
  uint32_t length = 0xC;
  memcpy(&info->game_ratings, data, 0xC);
}

inline void ReadXexMultiDiscMediaIds(uint8_t* data, XexInfo* info) {
  uint32_t entry_size = sizeof(xex2_multi_disc_media_id_t);
  uint32_t length = xe::load_and_swap<uint32_t>(data);
  uint32_t count = (length - 0x04) / 0x10;

  info->multi_disc_media_ids_count = count;
  info->multi_disc_media_ids =
      (xex2_multi_disc_media_id_t*)calloc(count, entry_size);

  uint8_t* cursor = data + 0x04;
  for (uint32_t i = 0; i < count; i++, cursor += 0x10) {
    auto id = &info->multi_disc_media_ids[i];
    memcpy(id->hash, cursor, 0x0C);
    id->media_id = xe::load_and_swap<uint32_t>(cursor + 0x0C);
  }
}

inline void ReadXexOriginalPeName(uint8_t* data, XexInfo* info) {
  uint32_t length = xe::load_and_swap<uint32_t>(data);

  info->original_pe_name = (xex2_opt_original_pe_name*)calloc(1, length);
  memcpy(info->original_pe_name, data, length);
}

inline void ReadXexResourceInfo(uint8_t* data, XexInfo* info) {
  uint32_t size = sizeof(xex2_opt_resource_info);
  uint32_t length = xe::load_and_swap<uint32_t>(data);
  uint32_t count = (length - 0x04) / 0x10;

  info->resources_count = count;
  info->resources = (xex2_resource*)calloc(count, size);
  memcpy(info->resources, data + 0x4, count * size);

  /*uint8_t* cursor = data + 0x04;
  for (uint32_t i = 0; i < count; i++, cursor += 0x10) {
    auto resource_info = &info->resources[i];
    memcpy(resource_info->name, cursor, 0x08);
    resource_info->address = xe::load_and_swap<uint32_t>(cursor + 0x08);
    resource_info->size = xe::load_and_swap<uint32_t>(cursor + 0x0C);
  }*/
}

inline void ReadXexOptHeader(xex2_opt_header* entry, uint8_t* data,
                             XexInfo* info) {
  switch (entry->key) {
    case XEX_HEADER_ALTERNATE_TITLE_IDS:
      ReadXexAltTitleIds(data + entry->offset, info);
      break;
    case XEX_HEADER_EXECUTION_INFO:
      ReadXexExecutionInfo(data + entry->offset, info);
      break;
    case XEX_HEADER_FILE_FORMAT_INFO:
      ReadXexFileFormatInfo(data + entry->offset, info);
      break;
    case XEX_HEADER_GAME_RATINGS:
      ReadXexGameRatings(data + entry->offset, info);
      break;
    case XEX_HEADER_MULTIDISC_MEDIA_IDS:
      ReadXexMultiDiscMediaIds(data + entry->offset, info);
      break;
    case XEX_HEADER_RESOURCE_INFO:
      ReadXexResourceInfo(data + entry->offset, info);
      break;
    case XEX_HEADER_ORIGINAL_PE_NAME:
      ReadXexOriginalPeName(data + entry->offset, info);
      break;
    case XEX_HEADER_IMAGE_BASE_ADDRESS:
      info->base_address = entry->value;
      break;
    case XEX_HEADER_SYSTEM_FLAGS:
      info->system_flags =
          (xex2_system_flags)xe::byte_swap<uint32_t>(entry->value.value);
      break;
  }
}

X_STATUS ReadXexHeaderSecurityInfo(File* file, XexInfo* info) {
  uint32_t length = 0x180;
  uint8_t* data = Read(file, info->security_offset, length);

  memcpy(&info->security_info, data, sizeof(xex2_security_info));
  delete[] data;

  // Check to see if the XEX is already decrypted
  const uint16_t PE_MAGIC = 0x4D5A;  // "MZ"
  uint8_t* magic_block = Read(file, info->header_size, 0x10);
  uint16_t found_magic = xe::load_and_swap<uint16_t>(magic_block);

  // XEX is still encrypted. Derive the session key.
  if (found_magic != PE_MAGIC) {
    auto aes_key = reinterpret_cast<uint8_t*>(info->security_info.aes_key);
    uint8_t session_key[0x10];
    uint8_t decrypted_block[0x10];

    // Test retail key
    aes_decrypt_buffer(xex2_retail_key, aes_key, 0x10, session_key, 0x10);
    aes_decrypt_buffer(session_key, magic_block, 0x10, decrypted_block, 0x10);

    // TODO: Proper key detection. This is currently hacked to assume retail
    // key.
    memcpy(info->session_key, session_key, 0x10);
    delete[] magic_block;
    return X_STATUS_SUCCESS;

    found_magic = xe::load_and_swap<uint16_t>(decrypted_block);
    if (found_magic == PE_MAGIC) {
      memcpy(info->session_key, session_key, 0x10);
      delete[] magic_block;
      return X_STATUS_SUCCESS;
    }

    // Test devkit key
    aes_decrypt_buffer(xex2_devkit_key, aes_key, 0x10, session_key, 0x10);
    aes_decrypt_buffer(session_key, magic_block, 0x10, decrypted_block, 0x10);
    found_magic = xe::load_and_swap<uint16_t>(decrypted_block);
    if (found_magic == PE_MAGIC) {
      memcpy(info->session_key, session_key, 0x10);
      delete[] magic_block;
      return X_STATUS_SUCCESS;
    }

    delete[] magic_block;
    return X_STATUS_UNSUCCESSFUL;
  }

  delete[] magic_block;
  return X_STATUS_SUCCESS;
}

X_STATUS ReadXexHeaderSectionInfo(File* file, XexInfo* info) {
  uint32_t offset = info->security_offset;
  uint32_t count = Read<uint32_t>(file, offset);
  uint32_t size = sizeof(xex2_page_descriptor);
  uint32_t length = count * size;
  uint8_t* data = Read(file, offset, length);

  info->page_descriptors_count = count;
  info->page_descriptors = (xex2_page_descriptor*)calloc(count, size);

  if (!info->page_descriptors || !count) {
    return X_STATUS_UNSUCCESSFUL;
  }

  uint8_t* cursor = data + 0x04;
  for (uint32_t i = 0; i < count; i++) {
    auto section = &info->page_descriptors[i];

    section->value = xe::load<uint32_t>(cursor + 0x00);
    memcpy(section->data_digest, cursor + 0x04, sizeof(section->data_digest));
    cursor += 0x04 + sizeof(section->data_digest);
  }

  delete[] data;
  return X_STATUS_SUCCESS;
}

X_STATUS ReadXexHeader(File* file, XexInfo* info) {
  uint32_t header_size = Read<uint32_t>(file, 0x8);
  uint8_t* data = Read(file, 0x0, header_size);

  // Read Main Header Data
  info->module_flags =
      (xex2_module_flags)xe::load_and_swap<uint32_t>(data + 0x04);
  info->header_size = xe::load_and_swap<uint32_t>(data + 0x08);
  info->security_offset = xe::load_and_swap<uint32_t>(data + 0x10);
  info->header_count = xe::load_and_swap<uint32_t>(data + 0x14);

  // Read Optional Headers
  uint8_t* cursor = data + 0x18;
  for (uint32_t i = 0; i < info->header_count; i++, cursor += 0x8) {
    auto entry = reinterpret_cast<xex2_opt_header*>(cursor);
    ReadXexOptHeader(entry, data, info);
  }

  if (XFAILED(ReadXexHeaderSecurityInfo(file, info)))
    return X_STATUS_UNSUCCESSFUL;
  if (XFAILED(ReadXexHeaderSectionInfo(file, info)))
    return X_STATUS_UNSUCCESSFUL;

  delete[] data;
  return X_STATUS_SUCCESS;
}

X_STATUS ReadXexImageUncompressed(File* file, XexInfo* info, size_t offset,
                                  uint32_t length, uint8_t*& out_data) {
  size_t file_size = file->entry()->size();
  auto format = info->file_format_info;

  const size_t exe_size = file_size - info->header_size;

  switch (format->encryption_type) {
    case XEX_ENCRYPTION_NONE: {
      out_data = Read(file, info->header_size + offset, length);
      return X_STATUS_SUCCESS;
    }
    case XEX_ENCRYPTION_NORMAL: {
      out_data = Read(file, info->header_size + offset, length);
      aes_decrypt_inplace(info->session_key, out_data, length);
      return X_STATUS_SUCCESS;
    }
    default:
      return X_STATUS_UNSUCCESSFUL;
  }
}

X_STATUS ReadXexImageBasicCompressed(File* file, XexInfo* info, size_t offset,
                                     uint32_t length, uint8_t*& out_data) {
  auto file_size = file->entry()->size();
  auto format = info->file_format_info;
  auto compression = &format->compression_info.basic;
  auto encryption = format->encryption_type;

  // Find proper block
  uint32_t i;
  uint32_t compressed_position = 0;
  uint32_t uncompressed_position = 0;
  uint32_t block_count = (format->info_size - 8) / 8;
  for (i = 0; i < block_count; i++) {
    const uint32_t data_size = compression->blocks[i].data_size;
    const uint32_t zero_size = compression->blocks[i].zero_size;
    const uint32_t total_size = data_size + zero_size;
    if (uncompressed_position + total_size > offset) break;

    compressed_position += data_size;
    uncompressed_position += total_size;
  }

  // For some reason the AES IV is screwing up the first 0x10 bytes,
  // so in the meantime, we're just shifting back 0x10 and skipping
  // the garbage data.
  auto block = compression->blocks[i];
  uint32_t block_size = block.data_size + 0x10;
  uint32_t block_address = info->header_size + compressed_position - 0x10;
  uint8_t* data = Read(file, block_address, block_size);

  if (encryption == XEX_ENCRYPTION_NORMAL) {
    aes_decrypt_inplace(info->session_key, data, block_size);
  }

  // Get the requested data
  auto remaining_offset = offset - uncompressed_position;
  out_data = (uint8_t*)malloc(length);
  memcpy(out_data, data + remaining_offset + 0x10, length);

  delete[] data;
  return X_STATUS_SUCCESS;
}

X_STATUS ReadXexImageNormalCompressed(File* file, XexInfo* info, size_t offset,
                                      uint32_t length, uint8_t*& out_data) {
  auto encryption_type = info->file_format_info->encryption_type;
  auto exe_length = info->security_info.image_size;
  auto exe_buffer = Read(file, info->header_size, exe_length);

  uint8_t* compress_buffer = NULL;
  const uint8_t* p = NULL;
  uint8_t* d = NULL;

  // Decrypt (if needed).
  const uint8_t* input_buffer = exe_buffer;
  size_t input_size = exe_length;

  switch (encryption_type) {
    case XEX_ENCRYPTION_NONE:
      // No-op.
      break;
    case XEX_ENCRYPTION_NORMAL:
      aes_decrypt_inplace(info->session_key, exe_buffer, exe_length);
      break;
    default:
      assert_always();
      return X_STATUS_UNSUCCESSFUL;
  }

  const auto* compression_info = &info->file_format_info->compression_info;
  const xex2_compressed_block_info* cur_block =
      &compression_info->normal.first_block;

  compress_buffer = (uint8_t*)calloc(1, exe_length);

  p = input_buffer;
  d = compress_buffer;

  while (cur_block->block_size) {
    const uint8_t* pnext = p + cur_block->block_size;
    const auto* next_block = (const xex2_compressed_block_info*)p;

    // skip block info
    p += 4;
    p += 20;

    while (true) {
      const size_t chunk_size = (p[0] << 8) | p[1];
      p += 2;
      if (!chunk_size) {
        break;
      }

      memcpy(d, p, chunk_size);
      p += chunk_size;
      d += chunk_size;
    }

    p = pnext;
    cur_block = next_block;
  }

  uint32_t uncompressed_size = info->security_info.image_size;

  uint8_t* buffer = (uint8_t*)calloc(1, uncompressed_size);

  // Decompress
  lzx_decompress(
      compress_buffer, d - compress_buffer, buffer, uncompressed_size,
      compression_info->normal.window_size, nullptr, 0);

  out_data = (uint8_t*)malloc(length);
  memcpy(out_data, buffer + offset, length);

  delete[] buffer;
  delete[] compress_buffer;
  delete[] exe_buffer;
  return X_STATUS_SUCCESS;
}

X_STATUS ReadXexImage(File* file, XexInfo* info, size_t offset, uint32_t length,
                      uint8_t*& out_data) {
  auto format = info->file_format_info;

  switch (format->compression_type) {
    case XEX_COMPRESSION_NONE:
      return ReadXexImageUncompressed(file, info, offset, length, out_data);
    case XEX_COMPRESSION_BASIC:
      return ReadXexImageBasicCompressed(file, info, offset, length, out_data);
    case XEX_COMPRESSION_NORMAL:
      return ReadXexImageNormalCompressed(file, info, offset, length, out_data);
    default:
      return X_STATUS_UNSUCCESSFUL;
  }
}

X_STATUS ReadXexResources(File* file, XexInfo* info) {
  auto resources = info->resources;

  for (size_t i = 0; i < info->resources_count; i++) {
    auto resource = &resources[i];

    uint32_t title_id = info->execution_info.title_id;
    uint32_t name =
        xe::string_util::from_string<uint32_t>(resource->name, true);

    // Game resources are listed as the TitleID
    if (name == title_id) {
      uint32_t offset = resource->address - info->base_address;

      uint8_t* data;
      if (XFAILED(ReadXexImage(file, info, offset, resource->size, data))) {
        return X_STATUS_UNSUCCESSFUL;
      }

      auto xbdf_data = XdbfGameData(data, resource->size);

      if (!xbdf_data.is_valid()) {
        delete[] data;
        return X_STATUS_UNSUCCESSFUL;
      }

      // Extract Game Title
      info->game_title = xbdf_data.title();

      // Extract Game Icon
      auto icon = xbdf_data.icon();
      info->icon_size = icon.size;
      info->icon = (uint8_t*)calloc(1, icon.size);
      memcpy(info->icon, icon.buffer, icon.size);

      // TODO: Extract Achievements

      delete[] data;
      return X_STATUS_SUCCESS;
    }
  }

  return X_STATUS_SUCCESS;
}

X_STATUS XexScanner::ScanXex(File* file, XexInfo* out_info) {
  if (XFAILED(ReadXexHeader(file, out_info))) return X_STATUS_UNSUCCESSFUL;
  if (XFAILED(ReadXexResources(file, out_info))) return X_STATUS_UNSUCCESSFUL;

  return X_STATUS_SUCCESS;
}

}  // namespace app
}  // namespace xe