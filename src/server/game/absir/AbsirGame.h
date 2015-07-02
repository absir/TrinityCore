#ifndef _ABSIRGAME_H
#define _ABSIRGAME_H

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "Player.h"

#define sAbsirGame AbsirGame::getInstance()

/*
Player.h
explicit Player(WorldSession* session);
~Player();

int absirGameFlag = 0;

void CleanupsBeforeDelete(bool finalCleanup = true) override;
*/
enum {
	AB_FLAG_HAS_BOT	= 0x00000001,
	AB_FLAG_IS_BOT	= 0x00000002,
};

class AbsirGame
{
public:
	AbsirGame();
	~AbsirGame();
	static AbsirGame *getInstance();

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
	ScriptMgr.cpp
	bool ScriptMgr::OnGossipHello(CreatureScript *tmpscript, Player *player, Creature *creature)
	GET_SCRIPT_RET(CreatureScript, creature->GetScriptId(), tmpscript, false);
	player->PlayerTalkClass->ClearMenus();

	return sAbsirGame->onGossipHello(tmpscript, player, creature);

	}
	*/
	bool onGossipHello(CreatureScript *tmpscript, Player *player, Creature *creature);

	/*
	ScriptMgr.cpp
	bool ScriptMgr::OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action)
	ASSERT(creature);

	GET_SCRIPT_RET(CreatureScript, creature->GetScriptId(), tmpscript, false);

	return sAbsirGame->onGossipSelect(tmpscript, player, creature, sender, action);
	*/
	bool onGossipSelect(CreatureScript *tmpscript, Player *player, Creature *creature, uint32 sender, uint32 action);

private:
	bool abWildHunt;
	bool abNpcHire;
	bool abNpcHire_Reputation;

	bool isCouldHireCreature(Player *player, Creature *creature);
};

#endif