#include "AbsirBotCreature.h"
#include "WorldSocket.h"
#include "GroupMgr.h"
#include "MySQLConnection.h"

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

void AbsirBotCreature::cleanUpFromWorld(Creature *creature, Group *group)
{
	if ((creature->absirGameFlag & AB_FLAG_IS_BOT) != 0) {
		creature->absirGameFlag &= ~AB_FLAG_IS_BOT;
	}

	/*
	if (group) {
		group->RemoveMember(creature->GetGUID());
	}
	*/

	creature->GetMap()->RemoveFromMap(creature, true);
}

void AbsirBotCreature::saveBotCreatures(Player *player)
{
	if ((player->absirGameFlag & AB_FLAG_HAS_BOT) != 0) {
		Group *group = player->GetGroup();
		if (group != NULL) {
			uint32 guid = player->GetGUID();
			CharacterDatabase.PExecute("UPDATE ab_character_bot SET sequ = sequ - 64 WHERE guid = ?", guid);
			const std::list<Group::MemberSlot> m_memberSlots = group->GetMemberSlots();
			// CharacterDatabase.PExecute("DELETE FROM ab_character_bot WHERE guid = ? AND sequ >= 0", guid);
			int8 sequ = 0;
			for (std::list<Group::MemberSlot>::const_iterator witr = m_memberSlots.begin(); witr != m_memberSlots.end(); ++witr) {
				ObjectGuid uid = witr->guid;
				if (uid.GetHigh() == HIGHGUID_UNIT) {
					Creature *member = ObjectAccessor::GetObjectInWorld(uid, (Creature*)NULL);
					if (member && member->GetOwnerGUID() == guid) {
						AbsirBotData data = ((AbsirBotCreature *)member)->getBotData();
						CharacterDatabase.PExecute("INSERT INTO ab_character_bot SET(guid, entry, sequ, phaseMask, data) VALUES (?, ?, ?, ?, ?)", guid, member->GetEntry(), ++sequ, member->GetPhaseMask(), data);
					}
				}
			}
		}
	}
}

void AbsirBotCreature::loadBotCreatures(Player *player)
{
	if ((player->absirGameFlag & AB_FLAG_HAS_BOT) == 0) {
		uint32 guid = player->GetGUID();
		bool saveError = false;
		QueryResult result = CharacterDatabase.PQuery("SELECT entry, sequ, phaseMask, data FROM ab_character_bot WHERE guid = ? ORDER BY sequ", guid);
		if (result)
		{
			do{
				Field *fields = result->Fetch();
				uint32 entry = fields->GetUInt32();
				int8 sequ = fields->GetInt8();
				if (sequ < 0) {
					saveError = true;
				
				}else if (saveError) {
					break;
				}

				uint32 phaseMask = fields->GetUInt32();
				std::string dataStr = fields->GetString();
				AbsirBotData botData;
				AbsirBotCreature *creature = AbsirBotCreature::createBotData(player, player->GetMap(), phaseMask, entry, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetOrientation());
				// Base64
				if (creature) {
					creature->m_botData = botData;
				}

			} while (result->NextRow());
		}

		if (saveError) {
			CharacterDatabase.PExecute("DELETE FROM ab_character_bot WHERE guid = ? AND sequ >= 0", guid);
			CharacterDatabase.PExecute("UPDATE ab_character_bot SET sequ = sequ + 64 WHERE guid = ?", guid);
		}
	}
}

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