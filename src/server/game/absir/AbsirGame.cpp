#include "AbsirGame.h"
#include "Config.h"
#include "ReputationMgr.h"
#include "ScriptedGossip.h"
#include "AbsirBotCreature.h"
#include "MySQLConnection.h"

std::string AbsirGame::getMoneyStr(uint32 money) {
	std::ostringstream moneyStr;
	uint32 gold = uint32(money / 10000);
	money -= (gold * 10000);
	uint32 silver = uint32(money / 100);
	money -= (silver * 100);

	if (gold != 0)
		moneyStr << gold << " |TInterface\\Icons\\INV_Misc_Coin_01:8|t";
	if (silver != 0)
		moneyStr << silver << " |TInterface\\Icons\\INV_Misc_Coin_03:8|t";
	if (money)
		moneyStr << money << " |TInterface\\Icons\\INV_Misc_Coin_05:8|t";

	return moneyStr.str();
}

uint32 AbsirGame::getFaction(Creature *creature)
{
	FactionTemplateEntry const* vendor_faction = creature->GetFactionTemplateEntry();
	return vendor_faction ? 0 : vendor_faction->faction;
}

static AbsirGame *_instance = NULL;
AbsirGame *AbsirGame::getInstance()
{
	if (_instance == NULL) {
		_instance = new AbsirGame();
	}

	return _instance;
}

AbsirGame::AbsirGame()
{
	abWildHunt = sConfigMgr->GetBoolDefault("AB_WildHunt", true);
	abNpcHire = sConfigMgr->GetBoolDefault("AB_NpcHung", true);
	abNpcHire_Reputation = sConfigMgr->GetBoolDefault("AB_NpcHung.Reputation", true);
}

AbsirGame::~AbsirGame()
{
}

const char *AbsirGame::getABScriptText(ABSIR_SCRIPT_ENTRY id, const char *defaultString)
{
	std::map<int, const char *>::iterator it = scriptTextMap.find(id);
	const char *value = it == scriptTextMap.end ? NULL : it->second;
	if (it == scriptTextMap.end) {
		QueryResult result = WorldDatabase.PQuery("SELECT content_default FROM db_script_text WHERE entry = ?", id);
		if (result) {
			const char *value = result->Fetch()[0].GetCString();
			if (value) {
				scriptTextMap.insert(std::map<int, const char *>::value_type(id, value));
				return value;
			}
		}

		return defaultString;
	}
	else {
		const char *value = it->second;
		return value == ABSIR_EMPTY_STR ? defaultString : value;
	}
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
		uint32 faction = getFaction(creature);
		int reputation = faction == 0 ? 0 : player->GetReputation(faction);
		if (reputation >= 3000) {
			if (!abNpcHire_Reputation || reputation >= (creature->getLevel() * ReputationMgr::Reputation_Cap / 255)) {
				Group *group = player->GetGroup();
				if (group == NULL) {
					group = player->GetOriginalGroup();
				}

				if (group == NULL) {
					return true;
				}

				return !group->IsFull();
			}
		}
	}

	return false;
}

uint32 getNpcHireGold(Player *player, Creature *creature)
{
	uint32 faction = AbsirGame::getFaction(creature);
	int reputation = faction == 0 ? 0 : player->GetReputation(faction);
	return creature->getLevel() * (creature->GetArmor() + creature->GetMaxHealth()) * (ReputationMgr::Reputation_Cap + ReputationMgr::Reputation_Cap - reputation) / (ReputationMgr::Reputation_Cap + ReputationMgr::Reputation_Cap) / 1000;
}

#define AB_GOSSIP_SENDER_HIRE 0xABAB1001

#define AB_GOSSIP_ACTION_HIRE 0xABAB2001
#define AB_GOSSIP_ACTION_HIRE_CONFIRM 0xABAB2002

bool AbsirGame::onGossipHello(CreatureScript *tmpscript, Player *player, Creature *creature) {
	bool res = false;
	if (isCouldHireCreature(player, creature)) {
		res = true;
		// std::string hireStr;
		const char *hireFormat = getABScriptText(AB_SCRIPT_HIRE_ME, "Hire Me");
		player->ADD_GOSSIP_ITEM(/*IconId*/GOSSIP_ICON_CHAT, /*NameOfTheGossipMenu*/getABScriptText(AB_SCRIPT_HIRE_ME, "Hire Me"), AB_GOSSIP_SENDER_HIRE, /*NameOfTheCase*/AB_GOSSIP_ACTION_HIRE);
	}

	if (tmpscript && tmpscript->OnGossipHello(player, creature)) {
		res = true;
	}

	if (tmpscript != NULL && !player->PlayerTalkClass->GetGossipMenu().Empty()) {
		player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
	}

	return res;
}

bool AbsirGame::onGossipSelect(CreatureScript *tmpscript, Player *player, Creature *creature, uint32 sender, uint32 action) {
	if (sender == AB_GOSSIP_SENDER_HIRE) {
		if (action == AB_GOSSIP_ACTION_HIRE) {
			player->ADD_GOSSIP_ITEM_EXTENDED(0, 0, AB_GOSSIP_SENDER_HIRE, AB_GOSSIP_ACTION_HIRE_CONFIRM, 0, (int)getNpcHireGold(player, creature), false);
			player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
		}
		else if (action == AB_GOSSIP_ACTION_HIRE_CONFIRM) {
			std::string hireStr;
			bool res = false;
			if (isCouldHireCreature(player, creature)) {
				uint32 gold = getNpcHireGold(player, creature);
				if (player->GetMoney() >= gold) {
					if (AbsirBotCreature::createBot(player, creature)) {
						player->SetMoney(player->GetMoney() - gold);
						hireStr = getABScriptText(AB_SCRIPT_HIRE_OK, "Right now, follow you!");
						res = true;
					}
				}
				else {
					hireStr = getABScriptText(AB_SCRIPT_NOT_ENOUGH_MONEY, "Not enough money?");
				}
			}

			if (hireStr.empty()) {
				hireStr = getABScriptText(AB_SCRIPT_NOT_READY, "You are not ready?");
			}

			player->CLOSE_GOSSIP_MENU();
			creature->Say(hireStr, LANG_UNIVERSAL, player);

			return res;
		}
	}
	return tmpscript && tmpscript->OnGossipSelect(player, creature, sender, action);
}


class AB_BotGroupScript : public GroupScript {
public:
	AB_BotGroupScript() : GroupScript("AB_BotGroupScript") {
	}

	virtual void OnRemoveMember(Group* group, ObjectGuid guid, RemoveMethod method, ObjectGuid kicker, const char* reason) {
		Player *player = ObjectAccessor::FindPlayer(guid);
		int absirGameFlag = player->absirGameFlag;
		if ((player->absirGameFlag & AB_FLAG_HAS_BOT) != 0) {
			const std::list<Group::MemberSlot> m_memberSlots = group->GetMemberSlots();
			std::list<Creature> removeMembers;
			for (std::list<Group::MemberSlot>::const_iterator witr = m_memberSlots.begin(); witr != m_memberSlots.end(); ++witr) {
				Creature *member = ObjectAccessor::FindCreature(witr->guid);
				if (member && (member->absirGameFlag & AB_FLAG_IS_BOT) != 0) {
					if (member->GetOwner() == player) {
						removeMembers.push_back(*member);
					}
				}
			}

			for (std::list<Creature>::iterator witr = removeMembers.begin(); witr != removeMembers.end(); ++witr) {
				witr->RemoveFromWorld();
			}

			player->absirGameFlag &= ~AB_FLAG_HAS_BOT;
		}
		else if ((absirGameFlag & AB_FLAG_IS_BOT) != 0) {
			player->RemoveFromWorld();
		}
	}
};

static int initScripte() {
	new AB_BotGroupScript();
	return 1;
}

static int _InitScriptes = initScripte();