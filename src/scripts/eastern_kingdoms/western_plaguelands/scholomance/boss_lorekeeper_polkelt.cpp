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
SDName: Boss_Lorekeeper_Polkelt
SD%Complete: 100
SDComment:
SDCategory: Scholomance
EndScriptData */

#include "scriptPCH.h"
#include "scholomance.h"

#define SPELL_VOLATILEINFECTION      24928
#define SPELL_DARKPLAGUE_AURA        12038
#define SPELL_CORROSIVEACID          8245
#define SPELL_NOXIOUSCATALYST        18151
#define SPELL_CORROSIVEACIDBREATH    16359
#define SPELL_DISEASEBURST           17689

struct boss_lorekeeperpolkeltAI : public ScriptedAI
{
    boss_lorekeeperpolkeltAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 VolatileInfection_Timer;
    uint32 Darkplague_Timer;
    uint32 CorrosiveAcid_Timer;
    uint32 NoxiousCatalyst_Timer;
	uint32 CorrosiveAcidBreath_Timer;
	bool perc80;
	bool perc60;
	bool perc40;
	bool perc20;
	bool perc2;

    void Reset()
    {
        VolatileInfection_Timer = 25000;
        CorrosiveAcid_Timer = 15000;
        NoxiousCatalyst_Timer = 35000;
		CorrosiveAcidBreath_Timer = 7000;
		perc80 = false;
		perc60 = false;
		perc40 = false;
		perc20 = false;
		perc2 = false;
    }

    void Aggro(Unit* /*pWho*/)
    {
        DoCastSpellIfCan(m_creature, SPELL_DARKPLAGUE_AURA, CF_TRIGGERED | CF_AURA_NOT_PRESENT);        
    }
    
    void JustDied(Unit *killer)
    {
        if (ScriptedInstance* pInstance = (ScriptedInstance*)m_creature->GetInstanceData())
            pInstance->SetData(TYPE_POLKELT, DONE);
    }

    void UpdateAI(const uint32 diff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

		//Disease Burst every 20% and 2%
		if (!perc80 && m_creature->GetHealthPercent() < 81.0f)
		{
			DoCastSpellIfCan(m_creature, SPELL_DISEASEBURST);
			perc80 = true;
		}

		if (!perc60 && m_creature->GetHealthPercent() < 61.0f)
		{
			DoCastSpellIfCan(m_creature, SPELL_DISEASEBURST);
			perc60 = true;
		}

		if (!perc40 && m_creature->GetHealthPercent() < 41.0f)
		{
			DoCastSpellIfCan(m_creature, SPELL_DISEASEBURST);
			perc40 = true;
		}

		if (!perc20 && m_creature->GetHealthPercent() < 21.0f)
		{
			DoCastSpellIfCan(m_creature, SPELL_DISEASEBURST);
			perc20 = true;
		}

		if (!perc2 && m_creature->GetHealthPercent() < 3.0f)
		{
			DoCastSpellIfCan(m_creature, SPELL_DISEASEBURST);
			perc2 = true;
		}


        //VolatileInfection_Timer
        if (VolatileInfection_Timer < diff)
        {
			Unit* target = NULL;
			target = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0);
			if (target) DoCastSpellIfCan(target, SPELL_VOLATILEINFECTION);
			VolatileInfection_Timer = urand(12000, 25000);
        }
        else VolatileInfection_Timer -= diff;

        //CorrosiveAcid_Timer
        if (CorrosiveAcid_Timer < diff)
        {
            DoCastSpellIfCan(m_creature->getVictim(), SPELL_CORROSIVEACID);
            CorrosiveAcid_Timer = 25000;
        }
        else CorrosiveAcid_Timer -= diff;

		//CorrosiveAcidBreath_Timer
		if (CorrosiveAcidBreath_Timer < diff)
		{
			DoCastSpellIfCan(m_creature->getVictim(), SPELL_CORROSIVEACIDBREATH);
			CorrosiveAcidBreath_Timer = urand(10000,25000);
		}
		else CorrosiveAcidBreath_Timer -= diff;

        //NoxiousCatalyst_Timer
        if (NoxiousCatalyst_Timer < diff)
        {
			Unit* ntarget = NULL;
			ntarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0);
			if (ntarget) DoCastSpellIfCan(ntarget, SPELL_NOXIOUSCATALYST);
			NoxiousCatalyst_Timer = urand(12000, 25000);
        }
        else NoxiousCatalyst_Timer -= diff;

        DoMeleeAttackIfReady();
    }
};
CreatureAI* GetAI_boss_lorekeeperpolkelt(Creature* pCreature)
{
    return new boss_lorekeeperpolkeltAI(pCreature);
}

void AddSC_boss_lorekeeperpolkelt()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "boss_lorekeeper_polkelt";
    newscript->GetAI = &GetAI_boss_lorekeeperpolkelt;
    newscript->RegisterSelf();
}
