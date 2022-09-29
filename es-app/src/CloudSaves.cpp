#include "CloudSaves.h"

#include "guis/GuiLoading.h"
#include "guis/GuiMsgBox.h"
#include "SaveStateRepository.h"
#include "platform.h"

void CloudSaves::load(Window* window, FileData *game, GuiSaveState* guiSaveState, const std::function<void(void)>& callback)
{
  guiSaveState->setVisible(false)
	auto loading = new GuiLoading<bool>(window, _("LOADING PLEASE WAIT"),
	[this, window, game, guiSaveState, callback](auto gui) {
		std::string sysName = game->getSourceFileData()->getSystem()->getName();
		int exitCode = runSystemCommand("ra_rclone.sh get \""+sysName+"\" \""+game->getPath()+"\"", "", nullptr);
		if (exitCode != 0)
			window->pushGui(new GuiMsgBox(window, _("ERROR LOADING FROM CLOUD"), _("OK")));
		else
			window->pushGui(new GuiMsgBox(window, _("LOADED FROM CLOUD"), _("OK")));
    guiSaveState->setVisible(false)
    callback();
		return true;
	});
	window->pushGui(loading);
}

void CloudSaves::save(Window* window, FileData* game)
{
	auto loading = new GuiLoading<bool>(window, _("SAVING PLEASE WAIT"),
	[this, window, game](auto gui) {
		std::string sysName = game->getSourceFileData()->getSystem()->getName();
		int exitCode = runSystemCommand("ra_rclone.sh set \""+sysName+"\" \""+game->getPath()+"\"", "", nullptr);
		if (exitCode != 0)
			window->pushGui(new GuiMsgBox(window, _("ERROR SAVING TO CLOUD"), _("OK")));
		else
			window->pushGui(new GuiMsgBox(window, _("SAVED TO CLOUD"), _("OK")));
		return true;
	});
	window->pushGui(loading);
}

bool CloudSaves::isSupported(FileData* game)
{
  SystemData* system = game->getSourceFileData()->getSystem();
	bool canCloudSave = system->isFeatureSupported(
		game->getEmulator(true),
		game->getEmulator(true),
		EmulatorFeatures::cloudsave);
	canCloudSave = canCloudSave && SaveStateRepository::isEnabled(game);
  return canCloudSave;
}