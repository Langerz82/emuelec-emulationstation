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

/*
template<class T>
std::vector<T> explode_dir(T const & path, T const & delims = "/\\")
{
  std::vector<T> sections;
  T section = path;
  while(true)
  {
    int count = path.find_first_of(delims);
    if (count == -1) {
      sections.push_back(section);
      return sections;
    }  
    sections.push_back(section.substr(0,count));
    section = section.substr(count+1);
  }
}
*/

template<class T>
T parent_dir(T const & path, T const & delims = "/\\")
{
  int count = path.find_last_of(delims);
  std::string subpath = path.substr(0,count);
  count = subpath.find_last_of(delims);
  return subpath.substr(count+1);
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
  mWindow(window),
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

  std::vector<FolderData*> fds = getChildFolders(game->getParent());
  
  auto emuelec_folderopt_def = std::make_shared< OptionListComponent<std::string> >(mWindow, "CHOOSE FOLDER", false);

  auto folderoptionsS = SystemConf::getInstance()->get("folder_option");
  
  if (mGame->getParent()->getParent() != nullptr) {
    std::string basePath = mGame->getSystem()->getRootFolder()->getPath();
    emuelec_folderopt_def->add(basePath, basePath, (fds.size() > 0) ? folderoptionsS == basePath : true);
  }

  for (auto it = fds.begin(); it != fds.end(); it++) {
    FolderData* fd = *it;
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
	auto createName = std::make_shared<TextComponent>(window, _("CREATE FOLDER"), theme->Text.font, theme->Text.color);
	row.addElement(createName, true);
  auto updateFN = [this, window, game](const std::string& newVal)
	{
		if (newVal.empty()) return;

    std::string path = base_path<std::string>(game->getPath()) + "/" + newVal;
		if (Utils::FileSystem::exists(path.c_str())) {
      window->pushGui(new GuiMsgBox(window, _("FOLDER EXISTS"), _("OK"), nullptr));
      return;
    }
    createFolder(game, path);
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

  FolderData* fd = file->getParent();
  std::string destDir = path.c_str();
  mWindow->pushGui(new GuiMsgBox(mWindow, destDir+" != "+file->getSystem()->getRootFolder()->getPath(), _("OK"), nullptr));
  if (destDir != file->getSystem()->getRootFolder()->getPath()) {
    mWindow->pushGui(new GuiMsgBox(mWindow, base_name<std::string>(path.c_str()), _("OK"), nullptr));
    fd = getFolderData(file->getParent(), base_name<std::string>(path.c_str()));
  }
  if (fd != nullptr) {
    mWindow->pushGui(new GuiMsgBox(mWindow, fd->getPath(), _("OK"), nullptr));
    std::string newPath = fd->getPath()+"/"+base_name<std::string>(file->getPath());
    mWindow->pushGui(new GuiMsgBox(mWindow, newPath, _("OK"), nullptr));
    FileData* newFile = new FileData(GAME, newPath, file->getSystem());

    fd->addChild(newFile);

  	if (view != nullptr) {
      //auto system = file->getSystem();
  		view.get()->remove(sourceFile);
  	}
  	else
  	{
  		sys->getRootFolder()->removeFromVirtualFolders(sourceFile);
  	}
    //ViewController::get()->reloadGameListView(sys);
  }
}

std::vector<FolderData*> GuiMoveToFolder::getChildFolders(FolderData* folder)
{
  std::vector<FolderData*> fds;
  std::vector<FileData*> children = folder->getChildren();
  for (auto it = children.begin(); it != children.end(); it++) {
    if ((*it)->getType() == FOLDER)
      fds.push_back(dynamic_cast<FolderData*>(*it));
  }
  return fds;
}

FolderData* GuiMoveToFolder::getFolderData(FolderData* folder, const std::string& name)
{
  std::vector<FileData*> children = folder->getChildren();
  for (auto it = children.begin(); it != children.end(); it++) {
    if ((*it)->getType() == FOLDER && (*it)->getName() == name)
      return dynamic_cast<FolderData*>(*it);
  }
  return nullptr;
}

void GuiMoveToFolder::createFolder(FileData* file, const std::string& path)
{
  auto sourceFile = file->getSourceFileData();
  //std::string folderName = remove_extension(base_name(path));

  auto sys = sourceFile->getSystem();
	if (sys->isGroupChildSystem())
		sys = sys->getParentGroupSystem();
  auto view = ViewController::get()->getGameListView(sys, false);

  if (!Utils::FileSystem::exists(path.c_str())) {
		Utils::FileSystem::createDirectory(path.c_str());

    std::string showFoldersMode = Settings::getInstance()->getString("FolderViewMode");
    if (showFoldersMode == "always")
      mGame->getParent()->addChild(new FolderData(path.c_str(), sys, false));
      ViewController::get()->reloadGameListView(sys);
	}
}
