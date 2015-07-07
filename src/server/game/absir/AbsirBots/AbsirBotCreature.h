#ifndef AB_BOT_CREATURE
#define AB_BOT_CREATURE

#include "AbsirGame.h"

class AbsirBotCreature;
class AbsirBotSession;

typedef struct _AbsirBotData {



} AbsirBotData;

class AbsirBotAI  {
public:
	void initCreature(AbsirBotCreature *botCreature) {
		m_botCreature = botCreature;
	};
	
private:
	AbsirBotCreature *m_botCreature;
};

/*
Group.cpp
@1
SubGroupCounterIncrease(subGroup);

//npcbot - check if trying to add bot
if (player->GetGUID().GetHigh() == HIGHGUID_PLAYER)
{
//end npcbot

player->SetGroupInvite(NULL);

@2
player->m_InstanceValid = true;

//npcbot
}
//end npcbot

if (!isRaidGroup())

@3
sScriptMgr->OnGroupAddMember(this, player->GetGUID());

//npcbot - check 2
if (player->GetGUID().GetHigh() == HIGHGUID_PLAYER)
{
//end npcbot

@4
m_maxEnchantingLevel = player->GetSkillValue(SKILL_ENCHANTING);

//npcbot
}
//end npcbot

return true;

@5
Player.cpp
m_achievementMgr->CheckAllAchievementCriteria();

_LoadEquipmentSets(holder->GetPreparedResult(PLAYER_LOGIN_QUERY_LOAD_EQUIPMENT_SETS));

AbsirBotCreature::saveBotCreatures(this);

return true;

@6
Player.cpp
else
{
AbsirBotCreature::saveBotCreatures(this);

// Update query
stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHARACTER);
stmt->setString(index++, GetName());

*/
class AbsirBotCreature : public Creature
{
public:
	static AbsirBotCreature *createBot(Player *player, Unit *unit) {
		return createBotData(player, unit->GetMap(), PHASEMASK_NORMAL, unit->GetEntry(), unit->GetPositionX(), unit->GetPositionY(), unit->GetPositionZ(), unit->GetOrientation());
	};
	static AbsirBotCreature *createBotData(Player *player, Map* map, uint32 phaseMask, uint32 entry, float x, float y, float z, float ang, CreatureData const* data = nullptr, uint32 vehId = 0);
	static void cleanUpFromWorld(Creature *creature, Group *group);

	static void saveBotCreatures(Player *player);
	static void loadBotCreatures(Player *player);

	AbsirBotCreature();
	~AbsirBotCreature();
	Player *getOwnerPlayer() { return m_owerPlayer; };
	AbsirBotAI *getBotAI() { return m_botAi; };
	Player *getBotPlayer();
	AbsirBotData getBotData() { return m_botData; };

private:
	Player *m_owerPlayer = NULL;
	AbsirBotAI *m_botAi = NULL;
	Player *m_botPlayer = NULL;
	AbsirBotData m_botData;
};

#endif