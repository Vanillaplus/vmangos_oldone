/* Copyright (C) 2006 - 2010 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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
SDName: Boss_Halycon
SD%Complete: 100
SDComment:
SDCategory: Blackrock Spire
EndScriptData */

#include "scriptPCH.h"

#define SPELL_CROWDPUMMEL       10887
#define SPELL_MIGHTYBLOW        14099
#define SPELL_TER_HOWL          8715
#define SPELL_FRENZY            28371
#define EMOTE_GENERIC_FRENZY_KILL -1000001

struct boss_halyconAI : public ScriptedAI
{
    boss_halyconAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Summoned = false;
        Reset();
    }

    uint32 CrowdPummel_Timer;
    uint32 MightyBlow_Timer;
	uint32 TerHowl_Timer;
	uint32 Frenzy_Timer;
    bool Summoned;

    void Reset()
    {
        CrowdPummel_Timer = 14000;
        MightyBlow_Timer = 8000;
		TerHowl_Timer = urand(9000, 12000);
		Frenzy_Timer = urand(4000, 9000);
    }

    void UpdateAI(const uint32 diff)
    {
        //Return since we have no target
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

		//Frenzy_Timer
		if (Frenzy_Timer < diff)
		{
			DoCastSpellIfCan(m_creature, SPELL_FRENZY);
			DoScriptText(EMOTE_GENERIC_FRENZY_KILL, m_creature);
			Frenzy_Timer = urand(40000, 50000);
		}
		else Frenzy_Timer -= diff;

		//TerHowl_Timer
		if (TerHowl_Timer < diff)
		{
			DoCastSpellIfCan(m_creature->getVictim(), SPELL_TER_HOWL);
			TerHowl_Timer = urand(15000, 25000);
		}
		else TerHowl_Timer -= diff;

        //CrowdPummel_Timer
        if (CrowdPummel_Timer < diff)
        {
            DoCastSpellIfCan(m_creature->getVictim(), SPELL_CROWDPUMMEL);
			CrowdPummel_Timer = 14000;
        }
        else CrowdPummel_Timer -= diff;

        //MightyBlow_Timer
        if (MightyBlow_Timer < diff)
        {
            DoCastSpellIfCan(m_creature->getVictim(), SPELL_MIGHTYBLOW);
            MightyBlow_Timer = 10000;
			DoResetThreat();
        }
        else MightyBlow_Timer -= diff;

        DoMeleeAttackIfReady();
    }
    void JustDied(Unit* pKiller)
    {
        //Summon Gizrul
        if (!Summoned)
        {
            m_creature->MonsterTextEmote("Halycon lets loose a gutteral growl as her body collapses. A horrifying howl can be heard echoing through the halls of Blackrock Spire. Something is very, very angry.");
            if (Creature* pGizrul = m_creature->SummonCreature(10268, -167.58f, -382.41f, 64.401f, 1.563f, TEMPSUMMON_DEAD_DESPAWN, 0))
            {
                pGizrul->SetHomePosition(-172.633f, -324.253f, 64.401f, 4.74f);
                pGizrul->SetInCombatWithZone();
            }
            Summoned = true;
        }
    }
};
CreatureAI* GetAI_boss_halycon(Creature* pCreature)
{
    return new boss_halyconAI(pCreature);
}

void AddSC_boss_halycon()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "boss_halycon";
    newscript->GetAI = &GetAI_boss_halycon;
    newscript->RegisterSelf();
}
