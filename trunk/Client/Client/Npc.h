//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#pragma once /* Npc.h */
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "Role.h"
#include "Monster.h"
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

class CNpc : public CMonster
{
public:
	CNpc();
	~CNpc();
	// ----
	virtual int  GetObjType		(){return MAP_ROLE;};
};
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------