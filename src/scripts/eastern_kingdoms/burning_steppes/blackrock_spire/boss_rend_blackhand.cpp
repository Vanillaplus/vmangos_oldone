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
SDName: Boss_Rend_Blackhand
SD%Complete: 100
SDComment: Intro event NYI
SDCategory: Blackrock Spire
EndScriptData */

#include "scriptPCH.h"
#include "blackrock_spire.h"

/*
In DB : change creature_template et creature_equip_template
*/
enum
{
    NPC_RENDBLACKHAND     = 10429,
    SPELL_MORTALSTRIKE    = 24573, //16856
    SPELL_CLEAVE          = 15284,
    SPELL_WHIRLWIND       = 26083, //13736
	TAUNT_IMMUNE          = 26602,
    DISARM_IMMUNE         = 7219,
	SPELL_FEAR            = 19134,
	SPELL_DISARM          = 13534,
	SPELL_EXECUTE         = 34428,
    SPELL_FRENZY          = 8269,

    REND_X                = 170,
    REND_Y                = -422,
    REND_Z                = 120
};

struct boss_rend_blackhandAI : public ScriptedAI
{
    boss_rend_blackhandAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (instance_blackrock_spire*)pCreature->GetInstanceData();
        Reset();
    }

    instance_blackrock_spire* m_pInstance;
    uint32 m_uiMortalStrikeTimer;
    uint32 m_uiCleaveTimer;
    uint32 m_uiFrenzyTimer;
    uint32 m_uiWhirlWindTimer;
	uint32 m_uiWhirlWindEndTimer;
	uint32 m_uiAggroResetTimer;
	uint32 m_uiFearTimer;
	uint32 m_uiDisarmTimer;
	uint32 m_uiExeTimer;
    uint32 m_uiEvadeRangeCheckTimer;

    void Reset()
    {
        m_uiMortalStrikeTimer       = 10000;
        m_uiCleaveTimer             = 8000;
        m_uiFrenzyTimer             = 0;
        m_uiWhirlWindTimer          = urand(20000, 28000);
		m_uiWhirlWindEndTimer       = 0;
		m_uiAggroResetTimer         = urand(5000, 7500);
		m_uiFearTimer               = 3000;
		m_uiDisarmTimer             = urand(4000, 10000);
		m_uiExeTimer                = 3000;
        m_uiEvadeRangeCheckTimer    = 10000;
    }

	void AssignRandomThreat()
	{
		if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
		{
			if (m_creature->IsWithinDist(pTarget, VISIBLE_RANGE))
			{
				DoResetThreat();
				m_creature->getThreatManager().addThreatDirectly(pTarget, urand(500, 800));
			}
		}
	}

    void JustReachedHome()
    {
        m_creature->RespawnNearCreaturesByEntry(10339, 100.0f);
        m_creature->DespawnOrUnsummon();
    }

    void JustDied(Unit* pKiller)
    {
        if (m_pInstance)
            if (m_pInstance->GetData(TYPE_GYTH) == IN_PROGRESS)
            {
                Creature *gyth = nullptr;
                gyth = GetClosestCreatureWithEntry(m_creature, 10339, VISIBLE_RANGE);
                if (gyth)
                    /*gyth->CastSpell(gyth, 5, true);*/
                m_creature->CastSpell(gyth, 5, true);
                m_pInstance->SetData(TYPE_GYTH, DONE);
            }

    }

    void UpdateAI(const uint32 uiDiff)
    {
        m_creature->SetInCombatWithZone();

        // Return since we have no target
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        // WhirlWind Check
        if (m_uiWhirlWindEndTimer)                          // Is in Whirlwind
        {
            // While in whirlwind, switch to random targets often
            if (m_uiAggroResetTimer < uiDiff)
            {
                AssignRandomThreat();
                m_uiAggroResetTimer = urand(1000, 2000);
            }
            else
                m_uiAggroResetTimer -= uiDiff;

            // End Whirlwind Phase
            if (m_uiWhirlWindEndTimer <= uiDiff)
            {
                m_uiWhirlWindEndTimer = 0;
                m_uiWhirlWindTimer = urand(15000, 20000);
                m_uiAggroResetTimer = urand(13000, 17000);
                // Remove the negative haste modifier from Whirlwind to restore Sartura's auto attack
                m_creature->ApplyAttackTimePercentMod(BASE_ATTACK, 0, true);
                m_creature->setAttackTimer(BASE_ATTACK, 100);
                m_creature->RemoveAurasByCasterSpell(TAUNT_IMMUNE, m_creature->GetObjectGuid());
                m_creature->RemoveAurasByCasterSpell(DISARM_IMMUNE, m_creature->GetObjectGuid());
            }
            else
                m_uiWhirlWindEndTimer -= uiDiff;
        }
        else // if (!m_uiWhirlWindEndTimer)                 // Is not in whirlwind
        {

            // Enter Whirlwind Phase
            if (m_uiWhirlWindTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_WHIRLWIND) == CAST_OK)
                {
                    m_creature->CastSpell(m_creature, TAUNT_IMMUNE, true);
                    m_creature->CastSpell(m_creature, DISARM_IMMUNE, true);
                    AssignRandomThreat();
                    m_uiWhirlWindEndTimer = 15000;
                    m_uiAggroResetTimer = urand(1000, 2000);
                }
            }
            else
                m_uiWhirlWindTimer -= uiDiff;

            // Aquire a new target sometimes
            if (m_uiAggroResetTimer < uiDiff)
            {
                AssignRandomThreat();
                m_uiAggroResetTimer = urand(3000, 7000);
            }
            else
                m_uiAggroResetTimer -= uiDiff;


            // Mortal Strike
            if (m_uiMortalStrikeTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_MORTALSTRIKE) == CAST_OK)
                    m_uiMortalStrikeTimer = 12000;
            }
            else
                m_uiMortalStrikeTimer -= uiDiff;

            // Cleave
            if (m_uiCleaveTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_CLEAVE) == CAST_OK)
                    m_uiCleaveTimer = urand(4000, 8000);
            }
            else
                m_uiCleaveTimer -= uiDiff;

            // Fear
            if (m_uiFearTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_FEAR) == CAST_OK)
                    m_uiFearTimer = urand(35000, 45000);
            }
            else
                m_uiFearTimer -= uiDiff;

            // Disarm
            if (m_uiDisarmTimer < uiDiff)
            {
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                {
                    DoCastSpellIfCan(pTarget, SPELL_DISARM);
                    m_uiDisarmTimer = urand(8000, 14000);
                }
            }
            else
                m_uiDisarmTimer -= uiDiff;

            // Execute
            if (m_uiExeTimer < uiDiff)
            {
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, nullptr, SELECT_FLAG_IN_MELEE_RANGE))
                {
                    DoCastSpellIfCan(pTarget, SPELL_EXECUTE);
                    m_uiExeTimer = 2000;
                }
            }
            else
                m_uiExeTimer -= uiDiff;

        }

        // Frenzy
        if (m_creature->GetHealthPercent() <= 30.0f)
        {
            if (m_uiFrenzyTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_FRENZY) == CAST_OK)
                    m_uiFrenzyTimer = 110000;
            }
            else
                m_uiFrenzyTimer -= uiDiff;
        }
        
        if (m_uiEvadeRangeCheckTimer < uiDiff)
        {
            if (m_creature->GetMapId() == 229)
            {
                if (m_creature->GetDistance2d(REND_X, REND_Y) > 73.0f || m_creature->GetPositionZ() > REND_Z)
                {
                    m_creature->RespawnNearCreaturesByEntry(10339, 100.0f);
                    m_creature->DespawnOrUnsummon();
                    return;
                }
            }
            m_uiEvadeRangeCheckTimer = 3000;
        }
        else
            m_uiEvadeRangeCheckTimer -= uiDiff;

		DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_rend_blackhand(Creature* pCreature)
{
    return new boss_rend_blackhandAI(pCreature);
}

void AddSC_boss_rend_blackhand()
{
    Script* newscript;
    newscript = new Script;
    newscript->Name = "boss_rend_blackhand";
    newscript->GetAI = &GetAI_boss_rend_blackhand;
    newscript->RegisterSelf();
}
