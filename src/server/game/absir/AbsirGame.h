#ifndef _ABSIRGAME_H
#define _ABSIRGAME_H

#include "ObjectMgr.h"

#define sAbsirGame AbsirGame::getInstance()

class AbsirGame
{
public:
	AbsirGame();
	~AbsirGame();
	static AbsirGame *getInstance();

	void setHurterPetStats(Guardian *guardian);
	void setHurterPetSpells(Guardian *guardian);

private:
	
};

#endif