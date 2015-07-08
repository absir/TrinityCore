#ifndef _ABSIRGAME_H
#define _ABSIRGAME_H

#include "Creature.h"
#include "Player.h"
#include "Pet.h"
#include "Group.h"
#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "Chat.h"

//以下是 Base64.h 的内容:
size_t AB_Base64_Decode(char *pDest, const char *pSrc, size_t srclen);
size_t AB_Base64_Encode(char *pDest, const char *pSrc, size_t srclen);

//以下是 Base64.cpp 的内容:

BYTE AB_Decode_GetByte(char c);
char AB_Encode_GetChar(BYTE num);

#define sAbsirGame AbsirGame::getInstance()

static const char *ABSIR_EMPTY_STR = "";

enum ABSIR_SCRIPT_ENTRY {
	AB_SCRIPT_NOT_ENOUGH_MONEY	= 81212001,
	AB_SCRIPT_NOT_READY			= 81212002,
	AB_SCRIPT_HIRE_ME			= 81212003,
	AB_SCRIPT_HIRE_OK			= 81212004,
	AB_SCRIPT_COST			= 81212005,
};

/*
Unit.h
int absirGameFlag = 0;
*/
enum {
	AB_FLAG_HAS_BOT	= 0x00000001,
	AB_FLAG_IS_BOT	= 0x00000002,
	AB_FLAG_IS_BOT_PET = 0x00000004,
};

class AbsirGame
{
public:
	static std::string getMoneyStr(uint32 money);
	static uint32 getFaction(Creature *creature);

	AbsirGame();
	~AbsirGame();
	static AbsirGame *getInstance();

	std::string getABScriptText(Player *player, ABSIR_SCRIPT_ENTRY id, char *defaultString);

	/*
	Pet.cpp
	bool Guardian::InitStatsForLevel(uint8 petlevel)
	SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, float(petlevel + (petlevel / 4)));
	//damage is increased afterwards as strength and pet scaling modify attack power

	sAbsirGame->setHurterPetStats(this);

	break;
	}
	*/

	void setHurterPetStats(Guardian *guardian);
	/*
	pet.cpp
	void Pet::InitLevelupSpellsForLevel()
	unlearnSpell(spellInfo->Id, true);
	// will called if level up
	else
	learnSpell(spellInfo->Id);
	}
	}

	sAbsirGame->setHurterPetSpells(this);

	}
	*/
	void setHurterPetSpells(Guardian *guardian);

	/*
	NPCHandler.cpp
	if (!sScriptMgr->OnGossipHello(_player, unit))
    {
//        _player->TalkedToCreature(unit->GetEntry(), unit->GetGUID());
        _player->PrepareGossipMenu(unit, unit->GetCreatureTemplate()->GossipMenuId, true);
		sAbsirGame->onGossipHello(NULL, _player, unit);
        _player->SendPreparedGossip(unit);
    }
	*/
	bool onGossipHello(CreatureScript *tmpscript, Player *player, Creature *creature);

	/*
	MiscHandler.cpp
	if (unit)
        {
            unit->AI()->sGossipSelect(_player, menuId, gossipListId);
			sAbsirGame->onGossipSelect(NULL, _player, unit, _player->PlayerTalkClass->GetGossipOptionSender(gossipListId), _player->PlayerTalkClass->GetGossipOptionAction(gossipListId));
            if (!sScriptMgr->OnGossipSelect(_player, unit, _player->PlayerTalkClass->GetGossipOptionSender(gossipListId), _player->PlayerTalkClass->GetGossipOptionAction(gossipListId)))
                _player->OnGossipSelect(unit, gossipListId, menuId);
        }
	*/
	bool onGossipSelect(CreatureScript *tmpscript, Player *player, Creature *creature, uint32 sender, uint32 action);

private:
	bool abWildHunt;
	bool abNpcHire;
	bool abNpcHire_Reputation;

	bool isCouldHireCreature(Player *player, Creature *creature);
};

#endif