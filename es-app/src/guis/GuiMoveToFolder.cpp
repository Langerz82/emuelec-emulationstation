#include "GuiMoveToFolder.h"
#include "CollectionSystemManager.h"
#include "SystemConf.h"
#include "SystemData.h"
#include "platform.h"

#include "views/gamelist/IGameListView.h"
#include "views/gamelist/ISimpleGameListView.h"
#include "views/ViewController.h"
#include "components/OptionListComponent.h"

GuiMoveToFolder::GuiMoveToFolder(Window* window, FileData* game) : GuiSettings(window, _("MOVE TO FOLDER").c_str())
{
  addEntry(_("MOVE GAME TO FOLDER"), true, [this, game]
	{
    std::string folderOption = SystemConf::getInstance()->get("folder_option");
    if (!folderOption.empty())
      moveToFolderGame(game, folderOption);
	});

  SystemData* system = game->getSystem();

  std::shared_ptr<IGameListView> gameList = ViewController::get()->getGameListView(system);
  std::vector<FileData*> folderoptions;
  ISimpleGameListView* view = dynamic_cast<ISimpleGameListView*>(gameList.get());
  if (view != nullptr)
  {
    std::vector<FileData*> gameListFiles = view->getFileDataEntries();
    for (auto it = gameListFiles.begin(); it != gameListFiles.end(); it++) {
      if ((*it)->getType() == FOLDER)
        folderoptions.push_back(*it);
    }
  }

  auto emuelec_folderopt_def = std::make_shared< OptionListComponent<std::string> >(mWindow, "CHOOSE FOLDER", false);

  auto folderoptionsS = SystemConf::getInstance()->get("folder_option");
  if (folderoptionsS.empty() && !folderoptions.size() > 0)
    folderoptionsS = folderoptions[0]->getFullPath();

  for (auto it = folderoptions.cbegin(); it != folderoptions.cend(); it++) {
    emuelec_folderopt_def->add(it->getPath(), it->getFullPath(), folderoptionsS == it->getFullPath());
  }
  
  addWithLabel(_("CHOOSE FOLDER"), emuelec_folderopt_def);
  addSaveFunc([emuelec_folderopt_def, game] {
    if (emuelec_folderopt_def->changed()) {
      std::string selectedfolder = emuelec_folderopt_def->getSelected();
      SystemConf::getInstance()->set("folder_option", selectedfolder);
      SystemConf::getInstance()->saveSystemConf();
    }
  });
}

void GuiMoveToFolder::moveToFolderGame(FileData* file, const std::string& path)
{
	if (file->getType() != GAME)
		return;

	auto sourceFile = file->getSourceFileData();

	auto sys = sourceFile->getSystem();
	if (sys->isGroupChildSystem())
		sys = sys->getParentGroupSystem();

	CollectionSystemManager::get()->deleteCollectionFiles(sourceFile);

	auto view = ViewController::get()->getGameListView(sys, false);

	char cmdMvFile[1024];
  snprintf(cmdMvFile, sizeof(cmdMvFile), "mv \"%s\" \"%s\"", sourceFile->getFullPath().c_str(), path.c_str());
  std::string strMvFile = cmdMvFile;
	LOG(LogInfo) << "strMvFile:" << strMvFile.c_str();
	system(strMvFile.c_str());

	if (view != nullptr) {
		view.get()->remove(sourceFile);
		ViewController::get()->reloadGameListView(view.get());
	}
	else
	{
		sys->getRootFolder()->removeFromVirtualFolders(sourceFile);
	}
}
