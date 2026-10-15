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
SDName: Boss_Drakkisath
SD%Complete: 100
SDComment:
SDCategory: Blackrock Spire
EndScriptData */

#include "scriptPCH.h"
#include "blackrock_spire.h"

enum
{
//    SPELL_FIRENOVA       = 23462,
    SPELL_FLAMESTRIKE      = 16419, // Ustaag : en remplacement de SPELL_FIRENOVA
    /*SPELL_CLEAVE           = 15284,*/ // Ustaag : ancien cleave 20691
    SPELL_CONFLIGURATION   = 16805,
    SPELL_THUNDERCLAP      = 23931,                            //Not sure if right ID. 23931 would be a harder possibility. 15548
    SPELL_RAGE             = 16789, // Ustaag : ajout
    SPELL_PIERCEARMOR      = 12097,
	SPELL_BERSERKER_CHARGE = 16636,

    SPELL_DOMINANCE       = 34432,
    SPELL_TF_HASTE        = 2313,
    SPELL_TF_MOD_HEAL     = 26525,
    SPELL_TF_IMMUNITY     = 26526,
    SPELL_TF_CANCEL       = 26589,

    DRAKKI_X = 16,
    DRAKKI_Y = -305,
    DRAKKI_Z = 105
};

struct boss_drakkisathAI : public ScriptedAI
{
    boss_drakkisathAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Engaged = false;
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 m_uiFlameStrikeTimer;
    /*uint32 m_uiCleaveTimer;*/
    uint32 m_uiConfligurationTimer;
    uint32 m_uiThunderclapTimer;
    uint32 m_uiRageTimer;
    uint32 m_uiPierceArmorTimer;
	uint32 m_uiRangeThreatReset;
	uint32 Dominance_Timer;
    uint32 m_uiEvadeRangeCheckTimer;
    bool Engaged;


    void Reset()
    {
        m_uiFlameStrikeTimer    = 16000;
        /*m_uiCleaveTimer         = 12000;*/
        m_uiConfligurationTimer = 8000;
        m_uiThunderclapTimer    = 1000;
        m_uiRageTimer           = 20000;
        m_uiPierceArmorTimer    = 10000;
		m_uiRangeThreatReset    = 5000;
		Dominance_Timer         = urand(18000, 30000);
        m_uiEvadeRangeCheckTimer = 9000;

        if (Engaged)
            m_pInstance->SetData(TYPE_DRAKKI, FAIL);
        Engaged = false;
    }

    void Aggro(Unit* pWho)
    {
        Engaged = true;
        if (m_pInstance)
            m_pInstance->SetData(TYPE_DRAKKI, IN_PROGRESS);

        m_creature->SetInCombatWithZone();

        std::list<Creature*> m_lEnts;
        GetCreatureListWithEntryInGrid(m_lEnts, m_creature, 10371, 100.0f);
        if (!m_lEnts.empty())
        {
            for (std::list<Creature*>::iterator itr = m_lEnts.begin(); itr != m_lEnts.end(); ++itr)
            {
                if ((*itr)->isAlive())
                {
                    if (m_creature)
                    {
                        (*itr)->AI()->AttackStart(m_creature->getVictim());
                        (*itr)->SetInCombatWithZone();
                    }
                }
            }
        }
    }

    void JustDied(Unit* pVictim)
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_DRAKKI, DONE);
    }

	void AssignRandomThreat()
	{
		if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
		{
			if (m_creature->IsWithinDist(pTarget, VISIBLE_RANGE))
			{
				DoResetThreat();
				m_creature->getThreatManager().addThreatDirectly(pTarget, urand(800, 1200));
			}
		}
	}

    void UpdateAI(const uint32 uiDiff)
    {
        // Return since we have no target
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

		if (Dominance_Timer < uiDiff)
		{
			{
				if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
				{
                    if (m_creature->getVictim() != pTarget && !pTarget->HasAura(SPELL_CONFLIGURATION))
                    {
                        DoCastSpellIfCan(pTarget, SPELL_DOMINANCE);
                        DoCastSpellIfCan(pTarget, SPELL_TF_HASTE);
                        DoCastSpellIfCan(pTarget, SPELL_TF_MOD_HEAL);
                        Dominance_Timer = urand(22000, 28000);
                    }
				}
			}
		}
		else Dominance_Timer -= uiDiff;

		// Anti-kiting
		if (m_uiRangeThreatReset < uiDiff)
		{
			if ((m_creature->getVictim())->GetDistance(m_creature) > 30.0f)
			{
				if ((m_creature->getVictim())->GetDistance(m_creature) < 41.0f)
                {
                    m_creature->CastSpell(m_creature->getVictim(), SPELL_BERSERKER_CHARGE, true);
                    AssignRandomThreat();
                }
				m_uiRangeThreatReset = 6000;
			}
		}
		else
			m_uiRangeThreatReset -= uiDiff;

        // FlameStrike
        if (m_uiFlameStrikeTimer < uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 1))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_FLAMESTRIKE) == CAST_OK)
                    m_uiFlameStrikeTimer = urand(14000, 22000);
            }
        }
        else
            m_uiFlameStrikeTimer -= uiDiff;

        // Cleave
        /*if (m_uiCleaveTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_CLEAVE) == CAST_OK)
                m_uiCleaveTimer = urand(8000, 10000);
        }
        else
            m_uiCleaveTimer -= uiDiff;*/

        // Confliguration
        if (m_uiConfligurationTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_CONFLIGURATION, 0, m_creature->getVictim()->GetGUID()) == CAST_OK)
                m_uiConfligurationTimer = 18000;
        }
        else
            m_uiConfligurationTimer -= uiDiff;

        // Thunderclap
        if (m_uiThunderclapTimer < uiDiff)
        {
            if ((m_creature->getVictim())->GetDistance(m_creature) < 10.0f)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_THUNDERCLAP) == CAST_OK)
                    m_uiThunderclapTimer = 22000;
            }
        }
        else
            m_uiThunderclapTimer -= uiDiff;

        // Rage
        if (m_uiRageTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_RAGE) == CAST_OK)
                m_uiRageTimer = 30000;
        }
        else
            m_uiRageTimer -= uiDiff;

        if (m_uiPierceArmorTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_PIERCEARMOR) == CAST_OK)
                m_uiPierceArmorTimer = 40000;
        }
        else
            m_uiPierceArmorTimer -= uiDiff;

        if (m_uiEvadeRangeCheckTimer < uiDiff)
        {
            if (m_creature->GetMapId() == 229)
            {
                if (m_creature->GetDistance2d(DRAKKI_X, DRAKKI_Y) > 70.0f || m_creature->GetPositionZ() < DRAKKI_Z)
                {
                    EnterEvadeMode();

                    std::list<Creature*> DrakkiAddsList;
                    GetCreatureListWithEntryInGrid(DrakkiAddsList, m_creature, 10814, 100.0f);
                    if (!DrakkiAddsList.empty())
                    {
                        for (std::list<Creature*>::iterator itr = DrakkiAddsList.begin(); itr != DrakkiAddsList.end(); ++itr)
                        {
                            if ((*itr)->isAlive())
                            {
                                if (m_creature)
                                {
                                    (*itr)->AI()->EnterEvadeMode();
                                }
                            }
                        }
                    }

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

CreatureAI* GetAI_boss_drakkisath(Creature* pCreature)
{
    return new boss_drakkisathAI(pCreature);
}

void AddSC_boss_drakkisath()
{
    Script* newscript;
    newscript = new Script;
    newscript->Name = "boss_drakkisath";
    newscript->GetAI = &GetAI_boss_drakkisath;
    newscript->RegisterSelf();
}
