#pragma once

#include "GuiSettings.h"
#include "FileData.h"

class Window;

class GuiMoveToFolder : public GuiSettings
{
public:
	GuiMoveToFolder(Window* window, FileData* game);

  void moveToFolderGame(FileData* file, const std::string& path);
};
