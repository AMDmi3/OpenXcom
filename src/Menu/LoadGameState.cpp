/*
 * Copyright 2010-2014 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "LoadGameState.h"
#include <sstream>
#include "../Engine/Logger.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Engine/Game.h"
#include "../Engine/Action.h"
#include "../Engine/Exception.h"
#include "../Engine/Options.h"
#include "../Engine/CrossPlatform.h"
#include "../Engine/Screen.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"
#include "../Interface/Text.h"
#include "../Geoscape/GeoscapeState.h"
#include "ErrorMessageState.h"
#include "../Battlescape/BattlescapeState.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Load Game screen.
 * @param game Pointer to the core game.
 * @param origin Game section that originated this state.
 * @param filename Name of the save file without extension.
 * @param palette Parent state palette.
 */
LoadGameState::LoadGameState(OptionsOrigin origin, const std::string &filename, SDL_Color *palette) : _firstRun(0), _origin(origin), _filename(filename)
{
	buildUi(palette);
}

/**
 * Initializes all the elements in the Load Game screen.
 * @param game Pointer to the core game.
 * @param origin Game section that originated this state.
 * @param type Type of auto-load being used.
 * @param palette Parent state palette.
 */
LoadGameState::LoadGameState(OptionsOrigin origin, SaveType type, SDL_Color *palette) : _firstRun(0), _origin(origin)
{
	switch (type)
	{
	case SAVE_QUICK:
		_filename = SavedGame::QUICKSAVE;
		break;
	case SAVE_AUTO_GEOSCAPE:
		_filename = SavedGame::AUTOSAVE_GEOSCAPE;
		break;
	case SAVE_AUTO_BATTLESCAPE:
		_filename = SavedGame::AUTOSAVE_BATTLESCAPE;
		break;
	default:
		// can't auto-load ironman games
		break;
	}
	
	buildUi(palette);
}

/**
 *
 */
LoadGameState::~LoadGameState()
{

}

/**
 * Builds the interface.
 * @param palette Parent state palette.
 */
void LoadGameState::buildUi(SDL_Color *palette)
{
	_screen = false;

	// Create objects
	_txtStatus = new Text(320, 17, 0, 92);

	// Set palette
	setPalette(palette);

	add(_txtStatus);

	centerAllSurfaces();

	// Set up objects
	_txtStatus->setColor(Palette::blockOffset(8)+5);
	_txtStatus->setBig();
	_txtStatus->setAlign(ALIGN_CENTER);
	_txtStatus->setText(tr("STR_LOADING_GAME"));

	if (_origin == OPT_BATTLESCAPE)
	{
		_txtStatus->setColor(Palette::blockOffset(1)-1);
		_txtStatus->setHighContrast(true);
	}
}

/**
 * Ignore quick loads without a save available.
 */
void LoadGameState::init()
{
	State::init();
	if (_filename == SavedGame::QUICKSAVE && !CrossPlatform::fileExists(Options::getUserFolder() + _filename))
	{
		_game->popState();
		return;
	}
}

/**
 * Loads the specified save.
 */
void LoadGameState::think()
{
	State::think();
	// Make sure it gets drawn properly
	if (_firstRun < 10)
	{
		_firstRun++;
	}
	else
	{
		_game->popState();

		// Load the game
		SavedGame *s = new SavedGame();
		try
		{
			s->load(_filename, _game->getRuleset());
			_game->setSavedGame(s);
			Options::baseXResolution = Options::baseXGeoscape;
			Options::baseYResolution = Options::baseYGeoscape;
			_game->getScreen()->resetDisplay(false);
			_game->setState(new GeoscapeState);
			if (_game->getSavedGame()->getSavedBattle() != 0)
			{
				_game->getSavedGame()->getSavedBattle()->loadMapResources(_game);
				Options::baseXResolution = Options::baseXBattlescape;
				Options::baseYResolution = Options::baseYBattlescape;
				_game->getScreen()->resetDisplay(false);
				BattlescapeState *bs = new BattlescapeState;
				_game->pushState(bs);
				_game->getSavedGame()->getSavedBattle()->setBattleState(bs);
			}
		}
		catch (Exception &e)
		{
			Log(LOG_ERROR) << e.what();
			std::wostringstream error;
			error << tr("STR_LOAD_UNSUCCESSFUL") << L'\x02' << Language::fsToWstr(e.what());
			if (_origin != OPT_BATTLESCAPE)
				_game->pushState(new ErrorMessageState(error.str(), _palette, Palette::blockOffset(8) + 10, "BACK01.SCR", 6));
			else
				_game->pushState(new ErrorMessageState(error.str(), _palette, Palette::blockOffset(0), "TAC00.SCR", -1));

			if (_game->getSavedGame() == s)
				_game->setSavedGame(0);
			else
				delete s;
		}
		catch (YAML::Exception &e)
		{
			Log(LOG_ERROR) << e.what();
			std::wostringstream error;
			error << tr("STR_LOAD_UNSUCCESSFUL") << L'\x02' << Language::fsToWstr(e.what());
			if (_origin != OPT_BATTLESCAPE)
				_game->pushState(new ErrorMessageState(error.str(), _palette, Palette::blockOffset(8) + 10, "BACK01.SCR", 6));
			else
				_game->pushState(new ErrorMessageState(error.str(), _palette, Palette::blockOffset(0), "TAC00.SCR", -1));

			if (_game->getSavedGame() == s)
				_game->setSavedGame(0);
			else
				delete s;
		}
		CrossPlatform::flashWindow();
	}
}

}
