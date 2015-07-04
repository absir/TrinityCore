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

class AbsirBotCharacterCreateInfo : CharacterCreateInfo {
public:
	AbsirBotCharacterCreateInfo() {
		Name = "Bot";
		Race = 2;
		Class = 3;
		Gender = GENDER_MALE;
	};
};

AbsirBotCreature::AbsirBotCreature() : Creature(true){
}

AbsirBotCreature::~AbsirBotCreature() {
	if (m_botAi) {
		delete m_botAi;
	}
}

AbsirBotCreature *AbsirBotCreature::createBotData(Player *player, Map* map, uint32 phaseMask, uint32 entry, float x, float y, float z, float ang, CreatureData const* data, uint32 vehId)
{
	AbsirBotCreature *botCreature = new AbsirBotCreature();
	Creature *creature = botCreature;
	if (creature->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_PET), map, phaseMask, entry, x, y, z, ang, data, vehId)) {
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
				map->AddToMap(creature);
				creature->setActive(true);
				return botCreature;
			}
		}
	}

	map->RemoveFromMap(creature, true);
	creature->RemoveFromWorld();
	if (creature) {
		delete creature;
	}

	return NULL;
}

Player *AbsirBotCreature::getBotPlayer()
{
	if (m_botPlayer == NULL) {
		// Set Bot Player
		m_botPlayer = (Player *)this;

		// Set Creature Owner Flag
		SetOwnerGUID(m_owerPlayer->GetGUID());
		absirGameFlag |= AB_FLAG_IS_BOT;
		m_owerPlayer->absirGameFlag |= AB_FLAG_HAS_BOT;

		// Create Bot AI
		m_botAi = createBotAI(this);
	}

	return m_botPlayer;
}