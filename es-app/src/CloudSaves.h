#pragma once
#ifndef ES_APP_SERVICES_CLOUDSAVE_SERVICE_H
#define ES_APP_SERVICES_CLOUDSAVE_SERVICE_H

#include "Window.h"
#include "FileData.h"
#include "guis/GuiSaveState.h"

class CloudSaves
{
  public:
    static CloudSaves& getInstance()
    {
      static CloudSaves instance;
      return instance;
    }
  private:
      CloudSaves() {}
      CloudSaves(CloudSaves const&);
      void operator=(CloudSaves const&);
  public:
      //CloudSaves(CloudSaves const&) = delete;
      //void operator=(CloudSaves const&) = delete;
			void save(Window* window, FileData* game);
			void load(Window* window, FileData* game, GuiSaveState* guiSaveState, const std::function<void(void)>& callback);
			bool isSupported(FileData* game);
};

#endif
