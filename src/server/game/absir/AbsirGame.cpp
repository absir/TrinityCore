#include "AbsirGame.h"
#include "Config.h"
#include "Pet.h"
#include "ScriptedGossip.h"
#include "Group.h"
#include "AbsirBotCreature.h"

AbsirGame::AbsirGame()
{
	abWildHunt = sConfigMgr->GetBoolDefault("AB_WildHunt", true);
	abNpcHire = sConfigMgr->GetBoolDefault("AB_NpcHung", true);
	abNpcHire_Reputation = sConfigMgr->GetBoolDefault("AB_NpcHung.Reputation", true);

	//ScriptRegistry
}

AbsirGame::~AbsirGame()
{
}

static AbsirGame *_instance = NULL;
AbsirGame *AbsirGame::getInstance()
{
	if (_instance == NULL) {
		_instance = new AbsirGame();
	}

	return _instance;
}

void AbsirGame::setHurterPetStats(Guardian *guardian)
{
	if (!abWildHunt) {
		return;
	}

	Pet *pet = dynamic_cast<Pet *>(guardian);
	if (pet != NULL && pet->getPetType() == HUNTER_PET) {
		const CreatureTemplate *cInfo = pet->GetCreatureTemplate();
		pet->SetAttackTime(BASE_ATTACK, cInfo->BaseAttackTime);
		pet->SetAttackTime(OFF_ATTACK, cInfo->BaseAttackTime);
		pet->SetAttackTime(RANGED_ATTACK, cInfo->RangeAttackTime);

		CreatureBaseStats const* stats = sObjectMgr->GetCreatureBaseStats(pet->getLevel(), cInfo->unit_class);
		float armor = (float)stats->GenerateArmor(cInfo); /// @todo Why is this treated as uint32 when it's a float?
		pet->SetModifierValue(UNIT_MOD_ARMOR, BASE_VALUE, armor);
		pet->SetModifierValue(UNIT_MOD_RESISTANCE_HOLY, BASE_VALUE, float(cInfo->resistance[SPELL_SCHOOL_HOLY]));
		pet->SetModifierValue(UNIT_MOD_RESISTANCE_FIRE, BASE_VALUE, float(cInfo->resistance[SPELL_SCHOOL_FIRE]));
		pet->SetModifierValue(UNIT_MOD_RESISTANCE_NATURE, BASE_VALUE, float(cInfo->resistance[SPELL_SCHOOL_NATURE]));
		pet->SetModifierValue(UNIT_MOD_RESISTANCE_FROST, BASE_VALUE, float(cInfo->resistance[SPELL_SCHOOL_FROST]));
		pet->SetModifierValue(UNIT_MOD_RESISTANCE_SHADOW, BASE_VALUE, float(cInfo->resistance[SPELL_SCHOOL_SHADOW]));
		pet->SetModifierValue(UNIT_MOD_RESISTANCE_ARCANE, BASE_VALUE, float(cInfo->resistance[SPELL_SCHOOL_ARCANE]));

		pet->SetCanModifyStats(true);
		pet->UpdateAllStats();
	}
}

void AbsirGame::setHurterPetSpells(Guardian *guardian)
{
	if (!abWildHunt) {
		return;
	}

	Pet *pet = dynamic_cast<Pet *>(guardian);
	if (pet != NULL && pet->getPetType() == HUNTER_PET) {
		for (uint8 i = 0; i < CREATURE_MAX_SPELLS; ++i) {
			const CreatureTemplate *cInfo = pet->GetCreatureTemplate();
			UINT32 spell = cInfo->spells[i];
			if (spell != 0) {
				pet->learnSpell(spell);
			}
		}
	}
}

bool AbsirGame::isCouldHireCreature(Player *player, Creature *creature) {
	if (abNpcHire) {
		int reputation = player->GetReputation(creature->getFaction());
		if (reputation > 0) {
			if (!abNpcHire_Reputation || reputation >= (creature->getLevel() * 999 / 255)) {
				Group *group = player->GetGroup();
				if (group == NULL) {
					group = player->GetOriginalGroup();
				}

				if (group == NULL) {
					return true;
				}

				if (group->isRaidGroup()) {
					for (int i = 0; i < MAX_RAID_SUBGROUPS; i++) {
						if (group->HasFreeSlotSubGroup(i)) {
							return true;
						}
					}
				}
				else {
					return group->GetMembersCount() < MAXGROUPSIZE;
				}
			}
		}
	}

	return false;
}

int getNpcHireGold(Player *player, Creature *creature)
{
	int reputation = player->GetReputation(creature->getFaction());
	return creature->getLevel() * (2000 - reputation) / 2000 * (creature->GetArmor() + creature->GetMaxHealth());
}

#define GOSSIP_SENDER_HIRE 0xABAB1001

#define GOSSIP_ACTION_HIRE 0xABAB2001

bool AbsirGame::onGossipHello(CreatureScript *tmpscript, Player *player, Creature *creature) {
	bool res = false;
	if (isCouldHireCreature(player, creature)) {
		res = true;
		std::string hireStr;
		sprintf(hireStr, "Hire Me? Give Me %d !!!", getNpcHireGold(player, creature));
		player->ADD_GOSSIP_ITEM(/*IconId*/GOSSIP_ICON_CHAT, /*NameOfTheGossipMenu*/hireStr, GOSSIP_SENDER_HIRE, /*NameOfTheCase*/GOSSIP_ACTION_HIRE);
	}

	if (tmpscript->OnGossipHello(player, creature)) {
		res = true;
	}

	if (!player->PlayerTalkClass->GetGossipMenu().Empty()) {
		player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
	}

	return res;
}

bool AbsirGame::onGossipSelect(CreatureScript *tmpscript, Player *player, Creature *creature, uint32 sender, uint32 action) {
	if (sender == GOSSIP_SENDER_HIRE && action == GOSSIP_ACTION_HIRE) {
		std::string hireStr;
		bool res;
		if (isCouldHireCreature(player, creature)) {
			int gold = getNpcHireGold(player, creature);
			if (player->GetMoney() >= gold) {
				if (AbsirBotCreature::createBot(player, creature)) {
					player->SetMoney(player->GetMoney() - gold);
					hireStr = "从现在起，追随你！";
					player->CLOSE_GOSSIP_MENU();
					res = true;
				}
			}
			else {
				hireStr = "你的钱不够？";
			}
		}

		if (hireStr.empty()) {
			hireStr = "你还没有准备好？";
		}

		player->Whisper(hireStr, LANG_COMMON, player, true);
		return res;
	}

	return tmpscript->OnGossipSelect(player, creature, sender, action);
}


class AB_BotGroupScript : public GroupScript {
public:
	AB_BotGroupScript() : GroupScript("AB_BotGroupScript") {

	}

	virtual void OnRemoveMember(Group* group, ObjectGuid guid, RemoveMethod method, ObjectGuid kicker, const char* reason) {
		Player *player = ObjectAccessor::FindPlayer(guid);
		if ((player->absirGameFlag & AB_FLAG_HAS_BOT != 0)) {
			const std::list<Group::MemberSlot> m_memberSlots = group->GetMemberSlots();
			for (std::list<Group::MemberSlot>::const_iterator witr = m_memberSlots.begin(); witr != m_memberSlots.end(); ++witr) {
				Player *member = ObjectAccessor::FindPlayer(witr->guid);
				if ((member->absirGameFlag & AB_FLAG_IS_BOT) != 0) {
					AbsirBotPlayer *botPlayer = (AbsirBotPlayer*)botPlayer;
					if (botPlayer->getBotCreature()->getOwnerPlayer() == player) {
						botPlayer->RemoveFromWorld();
					}
				}
			}

			player->absirGameFlag &= ~AB_FLAG_HAS_BOT;
		}
	}
};

static int initScripte() {
	new AB_BotGroupScript();
}

static int _InitScriptes = initScripte();