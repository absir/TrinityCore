#ifndef AB_BOT_CREATURE
#define AB_BOT_CREATURE

#include "AbsirGame.h"

class AbsirBotCreature;
class AbsirBotSession;

class AbsirBotAI  {
public:
	void initCreature(AbsirBotCreature *botCreature) {
		m_botCreature = botCreature;
	};
	
private:
	AbsirBotCreature *m_botCreature;
};

/*
Creature.h
class Creature : public virtual Unit, public virtual GridObject<Creature>, public MapObject
Player.h
class Player : public virtual Unit, public virtual GridObject<Player>
*/
class AbsirBotCreature : public Creature, public Player
{
public:
	static AbsirBotCreature *createBot(Player *player, Unit *unit) {
		return createBotData(player, unit->GetMap(), PHASEMASK_NORMAL, unit->GetEntry(), unit->GetPositionX(), unit->GetPositionY(), unit->GetPositionZ(), unit->GetOrientation());
	};
	static AbsirBotCreature *createBotData(Player *player, Map* map, uint32 phaseMask, uint32 entry, float x, float y, float z, float ang, CreatureData const* data = nullptr, uint32 vehId = 0);
	
	AbsirBotCreature();
	~AbsirBotCreature();
	Player *getOwnerPlayer() { return m_owerPlayer; };
	AbsirBotAI *getBotAI() { return m_botAi; };
	Player *getBotPlayer();

	void followOwnerStats();
	void syncCreature();

	void AddToWorld() override;
	void RemoveFromWorld() override;
	
	void SetObjectScale(float scale) {
		Creature::SetObjectScale(scale);
	};
	void Update(uint32 time) {
		Creature::Update(time);
		syncCreature();
	};
	void SetMap(Map *map) {
		Creature::SetMap(map);
		Creature::SetMap(map);
	};

	// ovrride virtual method
	bool CanAlwaysSee(WorldObject const* obj) const {
		return Creature::CanAlwaysSee(obj);
	};
	uint32 GetShieldBlockValue() const {
		return Creature::GetShieldBlockValue();
	};
	bool HasSpell(uint32 spellID) const {
		return Creature::HasSpell(spellID);
	};
	bool SetDisableGravity(bool disable, bool packetOnly = false) {
		return Creature::SetDisableGravity(disable, packetOnly);
	};
	bool SetCanFly(bool enable, bool packetOnly = false) {
		return Creature::SetCanFly(enable, packetOnly);
	};
	bool SetWaterWalking(bool enable, bool packetOnly = false) {
		return Creature::SetWaterWalking(enable, packetOnly);
	};
	bool SetFeatherFall(bool enable, bool packetOnly = false) {
		return Creature::SetFeatherFall(enable, packetOnly);
	};
	bool SetHover(bool enable, bool packetOnly = false) {
		return Creature::SetHover(enable, packetOnly);
	};
	void setDeathState(DeathState s) {
		return Player::setDeathState(s);
	};
	bool UpdateStats(Stats stat) {
		return Creature::UpdateStats(stat);
	};
	bool UpdateAllStats() {
		return Creature::UpdateAllStats();
	};
	void UpdateResistances(uint32 school) {
		return Creature::UpdateResistances(school);
	};
	void UpdateArmor() {
		return Creature::UpdateArmor();
	};
	void UpdateMaxHealth() {
		return Creature::UpdateMaxHealth();
	};
	void UpdateMaxPower(Powers power) {
		return Creature::UpdateMaxPower(power);
	};
	void UpdateAttackPowerAndDamage(bool ranged = false) {
		Creature::UpdateAttackPowerAndDamage(ranged);
	};
	void CalculateMinMaxDamage(WeaponAttackType attType, bool normalized, bool addTotalPct, float& minDamage, float& maxDamage) {
		Creature::CalculateMinMaxDamage(attType, normalized, addTotalPct, minDamage, maxDamage);
	};
	bool CanFly() const {
		return Creature::CanFly();
	};
	void SetTarget(ObjectGuid guid) {
		Creature::SetTarget(guid);
	};

protected:
	AbsirBotCreature(AbsirBotSession *botSession);

private:
	Player *m_owerPlayer = NULL;
	AbsirBotAI *m_botAi = NULL;
	Player *m_botPlayer = NULL;
};

#endif