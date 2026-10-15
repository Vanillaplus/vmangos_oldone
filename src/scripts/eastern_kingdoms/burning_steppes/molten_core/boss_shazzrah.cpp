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
SDName: Boss_Shazzrah
SD%Complete: 75
SDComment: Teleport NYI
SDCategory: Molten Core
EndScriptData */

#include "scriptPCH.h"
#include "molten_core.h"

enum
{
    SPELL_ARCANEEXPLOSION_MISSILES  = 34469,
    SPELL_ARCANEEXPLOSION           = 19712,
    SPELL_SHAZZRAHCURSE             = 19713,
    SPELL_DEADENMAGIC               = 19714,
    SPELL_COUNTERSPELL              = 19715,
    SPELL_GATE_DUMMY                = 23138,                 // effect spell: 23139
	SPELL_ARCANEBLAST               = 24857,                 //v+ channel arcane blast
	SPELL_POLY                      = 34482,
    SPELL_MANAGEM                   = 10058
};

struct boss_shazzrahAI : public ScriptedAI
{
    boss_shazzrahAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    uint32 ArcaneExplosion_Timer;
    uint32 ShazzrahCurse_Timer;
    uint32 DeadenMagic_Timer;
    uint32 Countspell_Timer;
    uint32 Blink_Timer;
	uint32 Poly_Timer;
	uint32 ArcaneBlast_Timer;
    uint32 Gem_Timer;
    uint32 Chronosphere_Timer;

    ScriptedInstance* m_pInstance;

    void Reset()
    {
		DeadenMagic_Timer = 1000;
        ShazzrahCurse_Timer = 2000;
        ArcaneBlast_Timer = 5000;
		ArcaneExplosion_Timer = 11000;
        Countspell_Timer = 13000;
        Blink_Timer = 30000;
		Poly_Timer = urand(18000, 25000);
        Gem_Timer = urand(18000, 25000);
        Chronosphere_Timer = urand(30000, 45000);

        if (m_pInstance && m_creature->isAlive())
            m_pInstance->SetData(TYPE_SHAZZRAH, NOT_STARTED);
    }

    void Aggro(Unit* pWho)
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_SHAZZRAH, IN_PROGRESS);
    }

    void JustDied(Unit* pKiller)
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_SHAZZRAH, DONE);
    }

    void UpdateAI(const uint32 diff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        //ArcaneExplosion_Timer
        if (ArcaneExplosion_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_ARCANEEXPLOSION) == CAST_OK)
            {
                ArcaneExplosion_Timer = urand(3000, 5000);
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_FARTHEST, 0, SPELL_ARCANEEXPLOSION_MISSILES, SELECT_FLAG_NOT_IN_MELEE_RANGE))
                    m_creature->CastSpell(pTarget, SPELL_ARCANEEXPLOSION_MISSILES, true);
            }
        }
        else ArcaneExplosion_Timer -= diff;

        //ShazzrahCurse_Timer
        if (ShazzrahCurse_Timer < diff)
        {
			if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_SHAZZRAHCURSE) == CAST_OK)
				ShazzrahCurse_Timer = urand(20000, 30000);
        }
        else ShazzrahCurse_Timer -= diff;

        //DeadenMagic_Timer
        if (DeadenMagic_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_DEADENMAGIC, CF_AURA_NOT_PRESENT) == CAST_OK)
                DeadenMagic_Timer = urand(10000, 35000);
        }
        else DeadenMagic_Timer -= diff;

        //Countspell_Timer
        if (Countspell_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_COUNTERSPELL) == CAST_OK)
                Countspell_Timer = urand(15000, 20000);
        }
        else Countspell_Timer -= diff;

		//Poly_Timer
		if (Poly_Timer < diff)
		{
			if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_POLY, CF_AURA_NOT_PRESENT) == CAST_OK)
				Poly_Timer = urand(18000, 25000);
		}
		else Poly_Timer -= diff;

		//ArcaneBlast_Timer
		if (ArcaneBlast_Timer < diff)
		{
			if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_ARCANEBLAST, CF_ONLY_IN_MELEE) == CAST_OK)
                ArcaneBlast_Timer = urand(20000, 28000),
				m_creature->getThreatManager().modifyThreatPercent(m_creature->getVictim(), -100);
		}
		else ArcaneBlast_Timer -= diff;

        //Blink_Timer
        if (Blink_Timer < diff)
        {
            // Teleporting him to a random gamer and casting Arcane Explosion after that.
            if (DoCastSpellIfCan(m_creature, SPELL_GATE_DUMMY, CF_TRIGGERED) == CAST_OK)
            {
                // manual, until added effect of dummy properly
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, nullptr, SELECT_FLAG_PLAYER))
                {
                    DoResetThreat();
                    //if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 1))
                    m_creature->NearTeleportTo(pTarget->GetPositionX(), pTarget->GetPositionY(), pTarget->GetPositionZ(), m_creature->GetOrientation());
                    m_creature->Attack(pTarget, true);
                    m_creature->getThreatManager().addThreatDirectly(pTarget, 300);
                }
                ArcaneExplosion_Timer += 5000;
                Blink_Timer = urand(25000, 35000);


                /*ThreatList const& tList = m_creature->getThreatManager().getThreatList();
                for (ThreatList::const_iterator itr = tList.begin(); itr != tList.end(); ++itr)
                {
                    if (Unit* pUnit = m_creature->GetMap()->GetUnit((*itr)->getUnitGuid()))
                        if (m_creature->GetDistance(pUnit) > 100.0f)
                            m_creature->CastSpell(pUnit, 21150, true);
                }*/
            }
        }
        else Blink_Timer -= diff;

        //Chronosphere
        if (Chronosphere_Timer < diff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, nullptr, SELECT_FLAG_PLAYER))
                m_creature->SummonCreature(25019, pTarget->GetPositionX(), pTarget->GetPositionY(), pTarget->GetPositionZ(), pTarget->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 5500),
                Chronosphere_Timer = urand(50000, 70000);
        }
        else Chronosphere_Timer -= diff;

        //Gem_Timer
        if (Gem_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_MANAGEM) == CAST_OK)
                Gem_Timer = urand(10000, 18000);
        }
        else Gem_Timer -= diff;

        DoMeleeAttackIfReady();
    }
};
CreatureAI* GetAI_boss_shazzrah(Creature* pCreature)
{
    return new boss_shazzrahAI(pCreature);
}

void AddSC_boss_shazzrah()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "boss_shazzrah";
    newscript->GetAI = &GetAI_boss_shazzrah;
    newscript->RegisterSelf();
}
