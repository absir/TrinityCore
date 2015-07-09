#include "AbsirBotCreature.h"
#include "WorldSocket.h"
#include "GroupMgr.h"
#include "MySQLConnection.h"
#include "bot_ai.h"

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

static size_t AB_BOT_DATA_SIZE = sizeof(AbsirBotData);

void AbsirBotCreature::saveBotCreatures(Player *player)
{
	if ((player->absirGameFlag & AB_FLAG_HAS_BOT) != 0) {
		Group *group = player->GetGroup();
		if (group != NULL) {
			uint32 guid = player->GetGUID();
			CharacterDatabase.PExecute("UPDATE ab_character_bot SET sequ = sequ - 64 WHERE guid = %u", guid);
			const std::list<Group::MemberSlot> m_memberSlots = group->GetMemberSlots();
			// CharacterDatabase.PExecute("DELETE FROM ab_character_bot WHERE guid = ? AND sequ >= 0", guid);
			int8 sequ = 0;
			for (std::list<Group::MemberSlot>::const_iterator witr = m_memberSlots.begin(); witr != m_memberSlots.end(); ++witr) {
				ObjectGuid uid = witr->guid;
				if (uid.GetHigh() == HIGHGUID_UNIT) {
					Creature *member = ObjectAccessor::GetObjectInWorld(uid, (Creature*)NULL);
					if (member && member->GetOwnerGUID() == guid) {
						AbsirBotData data = ((AbsirBotCreature *)member)->getBotData();
						char *encodeChr = new char[AB_BOT_DATA_SIZE * 2 + 4];
						try{
							size_t len = AB_Base64_Encode(encodeChr, (char *)&data, AB_BOT_DATA_SIZE);
							encodeChr[len] = 0;
							std::string dataStr = encodeChr;
							CharacterDatabase.PExecute("INSERT INTO ab_character_bot (guid, entry, sequ, phaseMask, x, y, z, ang, sdata) VALUES (%u, %u, %d, %u, %f, %f, %f, %f, %s)", guid, member->GetEntry(), ++sequ, member->GetPhaseMask(), member->GetPositionX(), member->GetPositionY(), member->GetPositionZ(), member->GetOrientation(), dataStr);
							delete encodeChr;
						}
						catch(double e) {
							delete encodeChr;
						}
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
		QueryResult result = CharacterDatabase.PQuery("SELECT entry, sequ, phaseMask, x, y, z, ang, sdata FROM ab_character_bot WHERE guid = %u ORDER BY sequ", guid);
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
				float x = fields->GetFloat();
				float y = fields->GetFloat();
				float z = fields->GetFloat();
				float ang = fields->GetFloat();
				AbsirBotCreature *creature = AbsirBotCreature::createBotData(player, player->GetMap(), phaseMask, entry, x, y, z, ang);
				if (creature) {
					// Decode Base64
					std::string dataStr = fields->GetString();
					const char *chr = dataStr.c_str();
					size_t len = strlen(chr);
					char *decodeChr = new char[len];
					try{
						len = AB_Base64_Decode(decodeChr, chr, len);
						AbsirBotData botData;
						if (len > AB_BOT_DATA_SIZE) {
							len = AB_BOT_DATA_SIZE;
						}

						_memccpy(&botData, decodeChr, 0, len);
						creature->m_botData = botData;
						delete decodeChr;
					}
					catch (double e) {
						delete decodeChr;
					}
				}

			} while (result->NextRow());
		}

		if (saveError) {
			CharacterDatabase.PExecute("DELETE FROM ab_character_bot WHERE guid = %u AND sequ >= 0", guid);
			CharacterDatabase.PExecute("UPDATE ab_character_bot SET sequ = sequ + 64 WHERE guid = %u", guid);
		}
	}
}

AbsirBotCreature::AbsirBotCreature() : Creature(true){
}

AbsirBotCreature::~AbsirBotCreature() {
	if (botAI) {
		botAI->UnsummonAll();
	}

	if (m_botAi) {
		delete m_botAi;
	}

	if (botMap) {
		delete botMap;
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
		// m_botAi = createBotAI(this);
	}

	return m_botPlayer;
}