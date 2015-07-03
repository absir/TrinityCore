#include "AbsirBotCreature.h"
#include "WorldSocket.h"
#include "GroupMgr.h"

static AbsirBotAI *createBotAI(AbsirBotCreature *botCreature) {
	// 
	uint8 cls = ((Creature *)botCreature)->getClass();
	AbsirBotAI *botAI = new AbsirBotAI();

	botAI->initCreature(botCreature);
	return botAI;
};

/*
class AB_BotSocket : public WorldSocket {
AB_BotSocket() : WorldSocket::WorldSocket(NULL) {
};
void Start() override {};
void SendPacket(WorldPacket const& packet) {};
};

static WorldSocket *BOT_SOCKET = new AB_BotSocket();
static std::shared_ptr<WorldSocket> BOT_SOCKET_PTR(BOT_SOCKET);
*/

class AbsirBotSession : public WorldSession {

public:
	AbsirBotSession() : WorldSession(0, NULL, SEC_PLAYER, 0, 0, LOCALE_enUS, 0, true) {
	}

	AbsirBotCreature *botCreature;

private:
};

static CharacterCreateInfo *BOT_CHARACTER = new CharacterCreateInfo();

AbsirBotCreature::AbsirBotCreature() : AbsirBotCreature(new AbsirBotSession()){
}

AbsirBotCreature::AbsirBotCreature(AbsirBotSession *botSession) : Creature(true), Player(botSession) {
	botSession->botCreature = this;
}

AbsirBotCreature::~AbsirBotCreature() {
	if (m_botAi) {
		delete m_botAi;
	}

	WorldSession *session = GetSession();
	if (session) {
		delete session;
	}
}

AbsirBotCreature *AbsirBotCreature::createBotData(Player *player, Map* map, uint32 phaseMask, uint32 entry, float x, float y, float z, float ang, CreatureData const* data, uint32 vehId)
{
	AbsirBotCreature *botCreature = new AbsirBotCreature();
	Creature *creature = botCreature;
	if (creature->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_UNIT), map, phaseMask, entry, x, y, z, ang, data, vehId)) {
		Group *group = player->GetGroup();
		if (group == NULL) {
			group = new Group();
			if (group->Create(player)) {
				sGroupMgr->AddGroup(group);
			}
			else {
				delete group;
				group = NULL;
			}
		}

		if (group != NULL) {
			if (!group->isRaidGroup() && group->GetMembersCount() >= MAXGROUPSIZE) {
				group->ConvertToRaid();
			}

			botCreature->m_owerPlayer = player;
			if (group->AddMember(botCreature->getBotPlayer())) {
				botCreature->followOwnerStats();
				creature->AddToWorld();
				return botCreature;
			}
		}
	}

	creature->RemoveFromWorld();
	delete creature;
	return NULL;
}

static int _UNIT_GUID_SIZE = sizeof(Unit) + sizeof(GridObject<Creature>);

Player *AbsirBotCreature::getBotPlayer()
{
	if (m_botPlayer == NULL) {
		// Set Bot Player
		m_botPlayer = this;
		
		// Create Player
		Player::Create(m_owerPlayer->GetGUID(), BOT_CHARACTER);
		Player::SetMap(Creature::GetMap());
		// Player::SetOwnerGUID(m_owerPlayer->GetGUID());
		Player::SetName(Creature::GetName());
		Player::GetSession()->SetPlayer(this);

		// Sync Creature Add To World
		syncCreature();
		AddToWorld();

		// Create Bot AI
		m_botAi = createBotAI(this);

		// Set Creature Owner Flag
		Creature::SetOwnerGUID(m_owerPlayer->GetGUID());
		m_owerPlayer->absirGameFlag |= AB_FLAG_HAS_BOT;
	}

	return m_botPlayer;
}

void AbsirBotCreature::followOwnerStats()
{
	Creature::SetLevel(m_owerPlayer->getLevel());
	Player::SetLevel(m_owerPlayer->getLevel());
}

void AbsirBotCreature::syncCreature()
{
	Player::Relocate(Creature::GetPosition());
}

void AbsirBotCreature::AddToWorld()
{
	bool inWold = Player::IsInWorld();
	if (!inWold) {
		sObjectAccessor->AddObject((Player *)this);
	}

	Creature::AddToWorld();
	Player::AddToWorld();
}

void AbsirBotCreature::RemoveFromWorld()
{
	bool inWold = Player::IsInWorld();
	if (inWold) {
		Group *group = GetGroup();
		if (group) {
			group->RemoveMember(Player::GetGUID(), GROUP_REMOVEMETHOD_LEAVE);
		}

		sObjectAccessor->RemoveObject((Player *)this);
	}

	Creature::RemoveFromWorld();
	Player::RemoveFromWorld();
}