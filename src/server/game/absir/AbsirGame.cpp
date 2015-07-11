#include "AbsirGame.h"
#include "Config.h"
#include "ReputationMgr.h"
#include "ScriptedGossip.h"
#include "AbsirBotCreature.h"
#include "MySQLConnection.h"

//#include 

//===================================
//    Base64 ½âÂë
//===================================
BYTE AB_Decode_GetByte(char c)
{
	if (c == '+')
		return 62;
	else if (c == '/')
		return 63;
	else if (c <= '9')
		return (BYTE)(c - '0' + 52);
	else if (c == '=')
		return 64;
	else if (c <= 'Z')
		return (BYTE)(c - 'A');
	else if (c <= 'z')
		return (BYTE)(c - 'a' + 26);
	return 64;
}

//½âÂë
size_t AB_Base64_Decode(char *pDest, const char *pSrc, size_t srclen)
{
	BYTE input[4];
	size_t i, index = 0;
	for (i = 0; i < srclen; i += 4)
	{
		//byte[0]
		input[0] = AB_Decode_GetByte(pSrc[i]);
		input[1] = AB_Decode_GetByte(pSrc[i + 1]);
		pDest[index++] = (input[0] << 2) + (input[1] >> 4);

		//byte[1]
		if (pSrc[i + 2] != '=')
		{
			input[2] = AB_Decode_GetByte(pSrc[i + 2]);
			pDest[index++] = ((input[1] & 0x0f) << 4) + (input[2] >> 2);
		}

		//byte[2]
		if (pSrc[i + 3] != '=')
		{
			input[3] = AB_Decode_GetByte(pSrc[i + 3]);
			pDest[index++] = ((input[2] & 0x03) << 6) + (input[3]);
		}
	}

	//null-terminator
	pDest[index] = 0;
	return index;
}

//===================================
//    Base64 ±àÂë
//===================================
char AB_Encode_GetChar(BYTE num)
{
	return
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789"
		"+/="[num];
}

//±àÂë
size_t AB_Base64_Encode(char *pDest, const char *pSrc, size_t srclen)
{
	BYTE input[3], output[4];
	size_t i, index_src = 0, index_dest = 0;
	for (i = 0; i < srclen; i += 3)
	{
		//char [0]
		input[0] = pSrc[index_src++];
		output[0] = (BYTE)(input[0] >> 2);
		pDest[index_dest++] = AB_Encode_GetChar(output[0]);

		//char [1]
		if (index_src < srclen)
		{
			input[1] = pSrc[index_src++];
			output[1] = (BYTE)(((input[0] & 0x03) << 4) + (input[1] >> 4));
			pDest[index_dest++] = AB_Encode_GetChar(output[1]);
		}
		else
		{
			output[1] = (BYTE)((input[0] & 0x03) << 4);
			pDest[index_dest++] = AB_Encode_GetChar(output[1]);
			pDest[index_dest++] = '=';
			pDest[index_dest++] = '=';
			break;
		}

		//char [2]
		if (index_src < srclen)
		{
			input[2] = pSrc[index_src++];
			output[2] = (BYTE)(((input[1] & 0x0f) << 2) + (input[2] >> 6));
			pDest[index_dest++] = AB_Encode_GetChar(output[2]);
		}
		else
		{
			output[2] = (BYTE)((input[1] & 0x0f) << 2);
			pDest[index_dest++] = AB_Encode_GetChar(output[2]);
			pDest[index_dest++] = '=';
			break;
		}

		//char [3]
		output[3] = (BYTE)(input[2] & 0x3f);
		pDest[index_dest++] = AB_Encode_GetChar(output[3]);
	}
	//null-terminator
	pDest[index_dest] = 0;
	return index_dest;
}

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
	return vendor_faction ? vendor_faction->faction : 0;
}

static AbsirGame *_instance = NULL;
AbsirGame *AbsirGame::getInstance()
{
	if (_instance == NULL) {
		_instance = new AbsirGame();
	}

	return _instance;
}

//config
int8 _basefollowdist;
uint8 _maxNpcBots;
uint8 _maxClassNpcBots;
uint8 _xpReductionNpcBots;
uint8 _healTargetIconFlags;
uint32 _npcBotsCost;
bool _enableNpcBots;
bool _enableNpcBotsDungeons;
bool _enableNpcBotsRaids;
bool _enableNpcBotsBGs;
bool _enableNpcBotsArenas;
bool _enableDungeonFinder;
bool _limitNpcBotsDungeons;
bool _limitNpcBotsRaids;
bool _botPvP;
float _mult_dmg_melee;
float _mult_dmg_spell;
float _mult_healing;

AbsirGame::AbsirGame()
{
	abWildHunt = sConfigMgr->GetBoolDefault("AB_WildHunt", true);
	abNpcHire = sConfigMgr->GetBoolDefault("AB_NpcHire", true);
	abNpcHire_Reputation = sConfigMgr->GetBoolDefault("AB_NpcHire.Reputation", true);
}

AbsirGame::~AbsirGame()
{
}

std::string AbsirGame::getABScriptText(Player *player, ABSIR_SCRIPT_ENTRY id, char *defaultString)
{
	std::string value = player == NULL ? sObjectMgr->GetTrinityStringForDBCLocale(id) : sObjectMgr->GetTrinityString(id, player->GetSession()->GetSessionDbcLocale());
	if (defaultString == NULL) {
		return value;
	}

	return value.empty() || value == "<error>" ? defaultString : value;
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
	if (abNpcHire && (creature->absirGameFlag & AB_FLAG_IS_BOT) == 0) {
		uint32 faction = getFaction(creature);
		int reputation = faction == 0 ? 0 : player->GetReputation(faction);
		if (reputation >= 1000) {
			if (!abNpcHire_Reputation || reputation >= (creature->getLevel() * ReputationMgr::Reputation_Cap / 255)) {
				Group *group = player->GetGroup();
				if (group == NULL) {
					group = player->GetOriginalGroup();
				}

				if (group == NULL) {
					return true;
				}

				if (!group->isRaidGroup() || !group->IsFull()) {
					int creatureEntry = creature->GetEntry();
					const std::list<Group::MemberSlot> m_memberSlots = group->GetMemberSlots();
					for (std::list<Group::MemberSlot>::const_iterator witr = m_memberSlots.begin(); witr != m_memberSlots.end(); ++witr) {
						ObjectGuid uid = witr->guid;
						if (uid.GetHigh() == HIGHGUID_UNIT && uid.GetEntry() == creatureEntry) {
							return false;
						}
					}

					return true;
				}

				return false;
			}
		}
	}

	return false;
}

uint32 getNpcHireGold(Player *player, Creature *creature)
{
	uint32 faction = AbsirGame::getFaction(creature);
	int reputation = faction == 0 ? 0 : player->GetReputation(faction);
	float x = creature->getLevel() * (creature->GetArmor() + creature->GetMaxHealth());
	x = (x * 16000 - 95988000) * (ReputationMgr::Reputation_Cap + ReputationMgr::Reputation_Cap - reputation) / (ReputationMgr::Reputation_Cap + ReputationMgr::Reputation_Cap) / 7;
	if (x <= 0) {
		x = 1;
	}

	x = 1;
	return x;
}

#define AB_GOSSIP_SENDER_HIRE 0xABAB1001

#define AB_GOSSIP_ACTION_HIRE 0xABAB2001

bool AbsirGame::onGossipHello(CreatureScript *tmpscript, Player *player, Creature *creature) {
	bool res = false;
	if (isCouldHireCreature(player, creature)) {
		res = true;
		std::string hireStr = getABScriptText(player, AB_SCRIPT_HIRE_ME, "Hire Me");
		player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_CHAT, getABScriptText(player, AB_SCRIPT_HIRE_ME, "Hire Me"), AB_GOSSIP_SENDER_HIRE, AB_GOSSIP_ACTION_HIRE, getABScriptText(player, AB_SCRIPT_COST, "Cost"), getNpcHireGold(player, creature), false);
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
	if (sender == AB_GOSSIP_SENDER_HIRE && action == AB_GOSSIP_ACTION_HIRE) {
		std::string hireStr;
		bool res = false;
		if (isCouldHireCreature(player, creature)) {
			uint32 gold = getNpcHireGold(player, creature);
			if (player->GetMoney() >= gold) {
				AbsirBotCreature *botCreature = AbsirBotCreature::createBot(player, creature);
				if (botCreature) {
					botCreature->updateBotData();
					player->SetMoney(player->GetMoney() - gold);
					hireStr = getABScriptText(player, AB_SCRIPT_HIRE_OK, "Right now, follow you!");
					res = true;
				}
			}
			else {
				hireStr = getABScriptText(player, AB_SCRIPT_NOT_ENOUGH_MONEY, "Not enough money?");
			}
		}

		if (hireStr.empty()) {
			hireStr = getABScriptText(player, AB_SCRIPT_NOT_READY, "You are not ready?");
		}

		player->CLOSE_GOSSIP_MENU();
		creature->Say(hireStr, LANG_UNIVERSAL, player);

		return res;
	}

	return tmpscript && tmpscript->OnGossipSelect(player, creature, sender, action);
}

class AB_BotGroupScript : public GroupScript {
public:
	AB_BotGroupScript() : GroupScript("AB_BotGroupScript") {
	}

	virtual void OnRemoveMember(Group* group, ObjectGuid guid, RemoveMethod method, ObjectGuid kicker, const char* reason) {
		if (guid.GetHigh() == HIGHGUID_UNIT) {
			Creature *creature = ObjectAccessor::GetObjectInWorld(guid, (Creature*)NULL);
			if (creature && (creature->absirGameFlag & AB_FLAG_IS_BOT) != 0) {
				AbsirBotCreature::cleanUpFromWorld(creature, NULL);
			}

			return;
		}

		Player *player = ObjectAccessor::FindPlayer(guid);
		if (player == NULL) {
			return;
		}

		AbsirBotCreature::clearBotCreatures(player);
	}
};

class AB_BotUnitScript : public UnitScript {
public:
	AB_BotUnitScript() : UnitScript("AB_BotUnitScript", true){

	};

	void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override {
		int absirGameFlag = attacker->absirGameFlag;
		if ((absirGameFlag & AB_FLAG_HAS_BOT)) {
			AbsirBotCreature::attackToUnit(attacker->ToPlayer(), victim);

		} else if ((absirGameFlag & AB_FLAG_IS_BOT)) {
			AbsirBotCreature::attackToUnit(((AbsirBotCreature *)attacker)->getOwnerPlayer(), victim);
		}

		absirGameFlag = victim->absirGameFlag;
		if ((absirGameFlag & AB_FLAG_HAS_BOT)) {
			AbsirBotCreature::attackToUnit(victim->ToPlayer(), attacker);
		}
		else if ((absirGameFlag & AB_FLAG_IS_BOT)) {
			AbsirBotCreature::attackToUnit(((AbsirBotCreature *)victim)->getOwnerPlayer(), attacker);
		}
	};

	/*
	void OnHeal(Unit* healer, Unit* reciever, uint32& gain) override {
	
	};
	*/

private:
};

class AB_BotPlayerScript : public PlayerScript {
public:
	AB_BotPlayerScript() : PlayerScript("AB_BotPlayerScript"){
		
	};

	void OnLevelChanged(Player* player, uint8 olde) override {
		AbsirBotCreature::changeLevelPlayer(player);
	}

	void OnLogin(Player* player, bool firstLogin) {
		AbsirBotCreature::loadBotCreatures(player);
	}

	void OnSave(Player* player) override {
		AbsirBotCreature::saveBotCreatures(player);
	}

	void OnLogout(Player* player) override {
		AbsirBotCreature::saveBotCreatures(player);
	}

	void OnMapChanged(Player* player) override{ 
		AbsirBotCreature::changeMap(player);
	}

private:
};
/*
void AddSC_death_knight_bot();
void AddSC_druid_bot();
void AddSC_hunter_bot();
void AddSC_mage_bot();
void AddSC_paladin_bot();
void AddSC_priest_bot();
void AddSC_rogue_bot();
void AddSC_shaman_bot();
void AddSC_warlock_bot();
void AddSC_warrior_bot();
void AddSC_blademaster_bot();
*/
// void AddSC_script_bot_commands();

int _ab_initCustomeScripte() {
	if (sAbsirGame->getABNpcHire()) {
		new AB_BotGroupScript();
		new AB_BotUnitScript();
		new AB_BotPlayerScript();
		/*
		AddSC_death_knight_bot();
		AddSC_druid_bot();
		AddSC_hunter_bot();
		AddSC_mage_bot();
		AddSC_paladin_bot();
		AddSC_priest_bot();
		AddSC_rogue_bot();
		AddSC_shaman_bot();
		AddSC_warlock_bot();
		AddSC_warrior_bot();
		AddSC_blademaster_bot();
		*/
		// AddSC_script_bot_commands();
	}
	
	return 1;
}

// static int _InitScriptes = initScripte();