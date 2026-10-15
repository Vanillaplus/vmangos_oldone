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
SDName: Boss_The_Best
SD%Complete: 100
SDComment:
SDCategory: Blackrock Spire
EndScriptData */

#include "scriptPCH.h"

enum
{
    SPELL_FLAMEBREAK       = 16785,
    //SPELL_IMMOLATE         = 20294,
    AURA_IMMOLATE          = 15506,
    SPELL_TERRIFYINGROAR   = 14100,
    SPELL_BERSERKER_CHARGE = 16636,
    SPELL_FIREBALL         = 16788,
    SPELL_LAVABREATH       = 34430,
    SPELL_SUMMON_FINKLE    = 16710,
	SPELL_FRENZY           = 28371,
    SPELL_DOUBLE           = 34429,

	EMOTE_GENERIC_FRENZY_KILL = -1000001,
};

struct boss_thebeastAI : public ScriptedAI
{
    boss_thebeastAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiFlamebreakTimer;
    //uint32 m_uiImmolateTimer;
    uint32 m_uiTerrifyingRoarTimer;
    uint32 m_uiBeserkerChargeTimer;
    uint32 m_uiFireballTimer;
    uint32 m_uiLavaBreathTimer;
	uint32 m_uiFrenzyTimer;
    uint32 m_uiDoubleTimer;
    uint32 m_uiWeakFBTimer;

    void Reset()
    {
        m_uiFlamebreakTimer     = urand(8000, 12000);
        //m_uiImmolateTimer       = 3000;
        m_uiTerrifyingRoarTimer = 13000;
        m_uiBeserkerChargeTimer = 0;
        m_uiFireballTimer       = 10000;
        m_uiLavaBreathTimer     = urand(8000, 11000);
		m_uiFrenzyTimer         = urand(10000, 20000);
        m_uiDoubleTimer         = 5000;
        m_uiWeakFBTimer         = 15000;
    }

    void SpellHit(Unit* pCaster, const SpellEntry* pSpell)
    {
        if (pSpell->Effect[0] == SPELL_EFFECT_SKINNING)
            pCaster->CastSpell(pCaster, SPELL_SUMMON_FINKLE, true);
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->HasAura(AURA_IMMOLATE))
            m_creature->CastSpell(m_creature, AURA_IMMOLATE, true);
        // Return since we have no target
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        // Flamebreak
        if (m_uiFlamebreakTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_FLAMEBREAK) == CAST_OK)
                m_uiFlamebreakTimer = urand(28000, 42000);
        }
        else
            m_uiFlamebreakTimer -= uiDiff;

        // Immolate
        // if (m_uiImmolateTimer < uiDiff)
        // {
        // if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
        // {
        // DoCastSpellIfCan(pTarget, SPELL_IMMOLATE);
        // m_uiImmolateTimer = urand(8000, 12000);
        // }
        // }
        // else
        // m_uiImmolateTimer -= uiDiff;

        // Terrifying Roar
        if (m_uiTerrifyingRoarTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_TERRIFYINGROAR) == CAST_OK)
                m_uiTerrifyingRoarTimer = urand(30000, 40000);
        }
        else
            m_uiTerrifyingRoarTimer -= uiDiff;

        // Berserker Charge
        if (m_uiBeserkerChargeTimer <= uiDiff)
        {
            Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 1);
            if (m_uiBeserkerChargeTimer == 0) pTarget = m_creature->getVictim();

            if (DoCastSpellIfCan(pTarget, SPELL_BERSERKER_CHARGE) == CAST_OK)
                m_uiBeserkerChargeTimer = urand(20000, 30000);
        }
        else
            m_uiBeserkerChargeTimer -= uiDiff;

        // Fireball
        if (m_uiFireballTimer < uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 1))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_FIREBALL) == CAST_OK)
                    m_uiFireballTimer = urand(5000, 9000);
            }
        }
        else
            m_uiFireballTimer -= uiDiff;

        if (m_uiLavaBreathTimer < uiDiff)
        {
            if (Unit* pTarget = m_creature->getVictim())
            {
                if (DoCastSpellIfCan(pTarget, SPELL_LAVABREATH) == CAST_OK)
					m_uiLavaBreathTimer = urand(12000, 18000);
            }
        }
        else
			m_uiLavaBreathTimer -= uiDiff;

		//Frenzy_Timer
		if (m_uiFrenzyTimer < uiDiff)
		{
			DoCastSpellIfCan(m_creature, SPELL_FRENZY);
			DoScriptText(EMOTE_GENERIC_FRENZY_KILL, m_creature);
			m_uiFrenzyTimer = urand(40000, 50000);
		}
		else m_uiFrenzyTimer -= uiDiff;

        //Double
        if (m_uiDoubleTimer <= uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 1, nullptr, SELECT_FLAG_IN_MELEE_RANGE))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_DOUBLE) == CAST_OK)
                    m_uiDoubleTimer = 4000,
                    m_creature->getThreatManager().addThreatDirectly(pTarget, 750);
                else if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_DOUBLE) == CAST_OK)
                    m_uiDoubleTimer = 1000;
            }
            else if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_DOUBLE) == CAST_OK)
                m_uiDoubleTimer = 1000;
        }
        else
            m_uiDoubleTimer -= uiDiff;

        //Weak FB
        if (m_uiWeakFBTimer <= uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_BOTTOMAGGRO, 0))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_FIREBALL) == CAST_OK)
                    m_uiWeakFBTimer = urand(12000, 15000);
            }
        }
        else
            m_uiWeakFBTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_thebeast(Creature* pCreature)
{
    return new boss_thebeastAI(pCreature);
}

void AddSC_boss_thebeast()
{
    Script* newscript;
    newscript = new Script;
    newscript->Name = "boss_the_beast";
    newscript->GetAI = &GetAI_boss_thebeast;
    newscript->RegisterSelf();
}
