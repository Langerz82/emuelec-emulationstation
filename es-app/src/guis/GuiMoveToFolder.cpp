#include "GuiMoveToFolder.h"
#include "CollectionSystemManager.h"
#include "SystemConf.h"
#include "SystemData.h"
#include "platform.h"

#include "views/gamelist/IGameListView.h"
#include "views/gamelist/ISimpleGameListView.h"
#include "views/ViewController.h"
#include "components/OptionListComponent.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "guis/GuiTextEditPopup.h"

template<class T>
T parent_dir(T const & path, T const & delims = "/\\")
{
  int count = path.find_last_of(delims);
  std::string subpath = path.substr(0,count);
  count = path.find_last_of(delims);
  return subpath.substr(count);
}

template<class T>
T base_path(T const & path, T const & delims = "/\\")
{
  return path.substr(0,path.find_last_of(delims));
}

template<class T>
T base_name(T const & path, T const & delims = "/\\")
{
  return path.substr(path.find_last_of(delims) + 1);
}

template<class T>
T remove_extension(T const & filename)
{
  typename T::size_type const p(filename.find_last_of('.'));
  return p > 0 && p != T::npos ? filename.substr(0, p) : filename;
}

GuiMoveToFolder::GuiMoveToFolder(Window* window, FileData* game) : 
  mGame(game), 
  GuiSettings(window, _("MOVE TO FOLDER").c_str())
{
  auto theme = ThemeData::getMenuTheme();

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
  //if (folderoptionsS.empty() && !folderoptions.size() > 0)
    //folderoptionsS = folderoptions[0]->getFullPath();
  std::string basePath = mGame->getParent()->getPath();
  emuelec_folderopt_def->add(basePath, basePath, folderoptionsS == basePath);
  for (auto it = folderoptions.begin(); it != folderoptions.end(); it++) {
    FileData* fd = *it;
    emuelec_folderopt_def->add(fd->getPath(), fd->getPath(), folderoptionsS == fd->getPath());
  }
  
  addWithLabel(_("CHOOSE FOLDER"), emuelec_folderopt_def);
  const std::function<void()> saveFunc([emuelec_folderopt_def] {
    if (emuelec_folderopt_def->changed()) {
      std::string selectedfolder = emuelec_folderopt_def->getSelected();
      SystemConf::getInstance()->set("folder_option", selectedfolder);
      SystemConf::getInstance()->saveSystemConf();
    }
  });
  addSaveFunc(saveFunc);
  emuelec_folderopt_def->setSelectedChangedCallback([emuelec_folderopt_def, saveFunc] (std::string val) { 
    saveFunc();
  });

	ComponentListRow row;
	auto createName = std::make_shared<TextComponent>(window, _("CREATE FOLDER NAME"), theme->Text.font, theme->Text.color);
	row.addElement(createName, true);
  auto updateFN = [this, window, game](const std::string& newVal)
	{
		if (newVal.empty()) return;

    std::string path = game->getPath() + "/" + newVal;
		if (Utils::FileSystem::exists(path.c_str())) {
      window->pushGui(new GuiMsgBox(window, _("FOLDER EXISTS"), _("OK"), nullptr));
      return;
    }

    createFolder(path);
	};

  row.makeAcceptInputHandler([this, window, game, updateFN]
	{
		if (Settings::getInstance()->getBool("UseOSK"))
			window->pushGui(new GuiTextEditPopupKeyboard(window, _("FOLDER NAME"), "", updateFN, false));
		else
			window->pushGui(new GuiTextEditPopup(window, _("FOLDER NAME"), "", updateFN, false));
	});

  addRow(row);
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
    std::string dir = parent_dir<std::string>(path.c_str());
    FileData* fd = getFolderData(dir);
    if (fd != nullptr)
      fd->getFolder()->addChild(mGame);
	}
	else
	{
		sys->getRootFolder()->removeFromVirtualFolders(sourceFile);
	}
}

FileData* GuiMoveToFolder::getFolderData(const std::string& name)
{
  SystemData* system = mGame->getSystem();

  std::shared_ptr<IGameListView> gameList = ViewController::get()->getGameListView(system);
  std::vector<FileData*> folderoptions;
  ISimpleGameListView* view = dynamic_cast<ISimpleGameListView*>(gameList.get());
  if (view != nullptr)
  {
    std::vector<FileData*> gameListFiles = view->getFileDataEntries();
    for (auto it = gameListFiles.begin(); it != gameListFiles.end(); it++) {
      if ((*it)->getType() == FOLDER && (*it)->getName() == name)
        return *it;
    }
  }
  return nullptr;
}

void GuiMoveToFolder::createFolder(const std::string& path)
{
  auto sourceFile = mGame->getSourceFileData();
  std::string folderName = remove_extension(base_name(path));

  auto sys = sourceFile->getSystem();
	if (sys->isGroupChildSystem())
		sys = sys->getParentGroupSystem();
  auto view = ViewController::get()->getGameListView(sys, false);

  if (!Utils::FileSystem::exists(path.c_str())) {
		Utils::FileSystem::createDirectory(path.c_str());
		/*FileData* newFolder = new FileData(FOLDER, folderName, sourceFile->getSystem());
		mGame->getParent()->addChild(newFolder);
		if (view != nullptr) {
				view.get()->repopulate();
		}*/
	}
}
