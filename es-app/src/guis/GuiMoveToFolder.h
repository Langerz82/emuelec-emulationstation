#pragma once

#include "GuiSettings.h"
#include "FileData.h"

class Window;

class GuiMoveToFolder : public GuiSettings
{
public:
	GuiMoveToFolder(Window* window, FileData* game);

  void moveToFolderGame(FileData* file, const std::string& path);
  FolderData* getFolderData(const std::string& name);
  void createFolder(const std::string& path);

protected:
  FileData* mGame;

};
