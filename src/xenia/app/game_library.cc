/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2018 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <algorithm>

#include "game_library.h"

namespace xe {
namespace app {

GameLibrary::GameLibrary(){

  //DEBUG MOCK, REMOVE THIS
  GameEntry* entry = new GameEntry;
  entry->path = "TEST PATH";
  entry->title_id = "FFFFFFFF";
  entry->media_id = "FFFFFFFF";
  AddEntry(entry);

};

int GameLibrary::AddEntry(GameEntry* entry) { 
  games_.push_back(entry);
  return 0;
}

int GameLibrary::RemoveEntry(GameEntry* entry) {
  auto found_entry = std::find(games_.begin(), games_.end(), entry);
  if(found_entry != games_.end()) {
    games_.erase(found_entry);
    return 0;
  }
  return 1;
}

}  // namespace app
}  // namespace xe