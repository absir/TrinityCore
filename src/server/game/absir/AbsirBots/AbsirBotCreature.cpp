#include "AbsirBotCreature.h"
#include "WorldSocket.h"
#include "GroupMgr.h"

static AbsirBotAI *createBotAI(AbsirBotCreature *botCreature) {
	// 
	uint8 cls = botCreature->getClass();
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

class AB_BotSession : public WorldSession {

public:
	AB_BotSession() : WorldSession(0, NULL, SEC_PLAYER, 0, 0, LOCALE_enUS, 0, true) {
	}

	AbsirBotCreature *botCreature;

private:
};

static CharacterCreateInfo *BOT_CHARACTER = new CharacterCreateInfo();

AbsirBotCreature::AbsirBotCreature() : Unit(false), Creature(), Player(new AB_BotSession()) {
	AB_BotSession *botSession = (AB_BotSession*)GetSession();
	botSession->SetPlayer(this);
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

Player *AbsirBotCreature::getBotPlayer()
{
	if (m_botPlayer == NULL) {
		m_botPlayer = this;
		m_botPlayer->Create(m_owerPlayer->GetGUID(), BOT_CHARACTER);
		m_botPlayer->AddToWorld();
		SetOwnerGUID(m_owerPlayer->GetGUID());
		m_owerPlayer->absirGameFlag |= AB_FLAG_HAS_BOT;
		m_botAi = createBotAI(this);
	}

	return m_botPlayer;
}

void AbsirBotCreature::followOwnerStats()
{
	SetLevel(m_owerPlayer->getLevel());
}

void AbsirBotCreature::AddToWorld()
{
	bool inWold = IsInWorld();
	if (!inWold) {
		sObjectAccessor->AddObject((Player *)this);
	}

	Creature::AddToWorld();
	Player::AddToWorld();
}

void AbsirBotCreature::RemoveFromWorld()
{
	bool inWold = IsInWorld();
	if (inWold) {
		Group *group = GetGroup();
		if (group) {
			group->RemoveMember(GetGUID(), GROUP_REMOVEMETHOD_LEAVE);
		}

		sObjectAccessor->RemoveObject((Player *)this);
	}

	Creature::RemoveFromWorld();
	Player::RemoveFromWorld();
}