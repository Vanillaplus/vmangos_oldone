/* Copyright (C) 2006 - 2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: Boss_Illucia_Barov
SD%Complete: 100
SDComment:
SDCategory: Scholomance
EndScriptData */

#include "scriptPCH.h"
#include "scholomance.h"

#define SPELL_CURSEOFAGONY      18671
#define SPELL_SHADOWSHOCK       20603
#define SPELL_SILENCE           15487
#define SPELL_FEAR              6215
#define SPELL_MC                14515

struct boss_illuciabarovAI : public ScriptedAI
{
    boss_illuciabarovAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 CurseOfAgony_Timer;
    uint32 ShadowShock_Timer;
    uint32 Silence_Timer;
    uint32 Fear_Timer;
	uint32 MC_Timer;

    void Reset()
    {
        CurseOfAgony_Timer = 3000;
        ShadowShock_Timer = 5000;
        Silence_Timer = 10000;
        Fear_Timer = urand(10000, 15000);
		MC_Timer = 20000;
    }

    void JustDied(Unit *killer)
    {
        if (ScriptedInstance* pInstance = (ScriptedInstance*)m_creature->GetInstanceData())
            pInstance->SetData(TYPE_ILLUCIABAROV, DONE);
    }

    void UpdateAI(const uint32 diff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        //CurseOfAgony_Timer
        if (CurseOfAgony_Timer < diff)
		{
			Unit* ctarget = NULL;
			ctarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0);
			if (ctarget) DoCastSpellIfCan(ctarget, SPELL_CURSEOFAGONY);

			CurseOfAgony_Timer = urand(5000, 12000);
		}
		else CurseOfAgony_Timer -= diff;

        //ShadowShock_Timer
        if (ShadowShock_Timer < diff)
        {
            Unit* target = NULL;
            target = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0);
            if (target) DoCastSpellIfCan(target, SPELL_SHADOWSHOCK);

            ShadowShock_Timer = 6000;
        }
        else ShadowShock_Timer -= diff;

        //Silence_Timer
        if (Silence_Timer < diff)
		{
			if (Unit* sTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_SILENCE, SELECT_FLAG_NO_TOTEM | SELECT_FLAG_IN_LOS | SELECT_FLAG_POWER_MANA | SELECT_FLAG_NOT_IN_MELEE_RANGE))
			{
				if (DoCastSpellIfCan(sTarget, SPELL_SILENCE) == CAST_OK)
					Silence_Timer = 10000;
			}
		}
        else Silence_Timer -= diff;

        //Fear_Timer
        if (Fear_Timer < diff)
        {
            DoCastSpellIfCan(m_creature->getVictim(), SPELL_FEAR);
            Fear_Timer = urand(20000, 30000),
			DoResetThreat();
        }
        else Fear_Timer -= diff;

		//MC_Timer
		if (MC_Timer < diff)
		{
			if (Unit* mcTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_FEAR, SELECT_FLAG_IN_LOS))
			{
				if ((mcTarget != m_creature->getVictim()) & (DoCastSpellIfCan(mcTarget, SPELL_FEAR) == CAST_OK))
					MC_Timer = urand(18000, 25000);
			}
		}
		else MC_Timer -= diff;

        DoMeleeAttackIfReady();
    }
};
CreatureAI* GetAI_boss_illuciabarov(Creature* pCreature)
{
    return new boss_illuciabarovAI(pCreature);
}

void AddSC_boss_illuciabarov()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "boss_illucia_barov";
    newscript->GetAI = &GetAI_boss_illuciabarov;
    newscript->RegisterSelf();
}
