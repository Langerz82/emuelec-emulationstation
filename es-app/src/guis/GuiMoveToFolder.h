#pragma once

#include "GuiSettings.h"
#include "FileData.h"
#include "GuiComponent.h"

#include <vector>

class Window;

class GuiMoveToFolder : public GuiSettings
{
public:
	GuiMoveToFolder(Window* window, FileData* file);
  void makeFolderList(FileData* file, GuiComponent* optionList);
  void moveToFolderGame(FileData* file, const std::string& path);
  void createFolder(FileData* file, const std::string& path);
  FolderData* getFolderData(FolderData* folder, const std::string& name);
  std::vector<FolderData*> getChildFolders(FolderData* folder);

protected:
  FileData* mFile;
  Window* mWindow;
  GuiComponent* mMenu;

};
