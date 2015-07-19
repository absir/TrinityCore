#ifndef AB_BOT_CREATURE
#define AB_BOT_CREATURE

#include "AbsirGame.h"
#include "Pet.h"

class AbsirBotCreature;
class AbsirBotSession;

typedef struct _AbsirBotData {
	bool follow = true;
	float distance = PET_FOLLOW_DIST;
	float angle = 0;

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
//npcbot - check if trying to add bot
if (player->GetGUID().GetHigh() == HIGHGUID_PLAYER)
{
//end npcbot
// insert into the table if we're not a battleground group
if (!isBGGroup() && !isBFGroup())
{
PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_GROUP_MEMBER);

stmt->setUInt32(0, m_dbStoreId);
stmt->setUInt32(1, member.guid.GetCounter());
stmt->setUInt8(2, member.flags);
stmt->setUInt8(3, member.group);
stmt->setUInt8(4, member.roles);

CharacterDatabase.Execute(stmt);
}
//npcbot
}
//end npcbot

@6
if (m_memberMgr.getSize() < ((isLFGGroup() || isBGGroup()) ? 1u : 2u))
if (guid.GetHigh() != HIGHGUID_UNIT || m_memberSlots.size <= 1)
Disband();
*/
class AbsirBotCreature : public Minion
{
public:
	static AbsirBotCreature *createBot(Player *player, Unit *unit) {
		return createBotData(player, unit->GetMap(), PHASEMASK_NORMAL, unit->GetEntry(), unit->GetPositionX(), unit->GetPositionY(), unit->GetPositionZ(), unit->GetOrientation());
	};
	static AbsirBotCreature *createBotData(Player *player, Map* map, uint32 phaseMask, uint32 entry, float x, float y, float z, float ang, CreatureData const* data = nullptr, uint32 vehId = 0);
	static void cleanUpFromWorld(Creature *creature, Group *group);

	static void saveBotCreatures(Player *player);
	static void loadBotCreatures(Player *player);
	static void clearBotCreatures(Player *player);
	static void changeLevelPlayer(Player *player);
	static void attackToUnit(Player *player, Unit *unit);
	static void changeMap(Player *player);

	AbsirBotCreature(Player* owner);
	~AbsirBotCreature();
	Player *getOwnerPlayer() { return m_owerPlayer; };
	Player *getBotPlayer();
	AbsirBotData getBotData() { return m_botData; };

	void updateOwnerData();
	void updateBotData();
	void teleportToPosition(Position &position);
	void UnSummon(uint32 msTime = 0) override;
	void Update(uint32 time) override;

private:
	Player *m_owerPlayer = NULL;
	Player *m_botPlayer = NULL;
	AbsirBotData m_botData;
	AbsirBotAI *m_botAI = NULL;
};

#endif