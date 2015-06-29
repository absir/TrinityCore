#include "AbsirGame.h"
#include "Pet.h"

AbsirGame::AbsirGame()
{
}

AbsirGame::~AbsirGame()
{
}

static AbsirGame *_instance = NULL;
AbsirGame *AbsirGame::getInstance()
{
	if (_instance == NULL) {
		_instance = new AbsirGame();
	}

	return _instance;
}

void AbsirGame::setHurterPetStats(Guardian *guardian)
{
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