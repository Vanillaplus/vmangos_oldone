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
SDName: Boss_Golemagg
SD%Complete: 75
SDComment: Timers need to be confirmed, Golemagg's Trust need to be checked
SDCategory: Molten Core
EndScriptData */

/*
    Golemagg and his adds should evade if dogs are pulled too far away from Golemagg
*/

#include "scriptPCH.h"
#include "molten_core.h"

enum
{
    SPELL_MAGMASPLASH       = 13879,
    SPELL_PYROBLAST         = 20228,
    SPELL_EARTHQUAKE        = 19798,
    SPELL_ENRAGE            = 28131,
    SPELL_ENRAGE_10S        = 19953,
    SPELL_GOLEMAGG_TRUST    = 20553,
	SPELL_IMPLOSION         = 34480,
    SPELL_HATEFULSTRIKE     = 34434,

    // Core Rager
    EMOTE_LOWHP             = -1409002,
    SPELL_MANGLE            = 19820,
    SPELL_TRASH             = 3391,
	SPELL_SERRATED_BITE     = 19771,
    SPELL_MARK_OF_GOLEMAGG  = 34586,
};

struct boss_golemaggAI : public ScriptedAI
{
    boss_golemaggAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 m_uiPyroblastTimer;
    uint32 m_uiEarthquakeTimer;
    uint32 TickTimer;
	uint32 ImplosionTimer;
    bool m_bEnraged;
    ObjectGuid previousTarget;
    uint32 HatefulTimer;

    void Reset()
    {
        m_uiPyroblastTimer = 7 * IN_MILLISECONDS;            // These timers are probably wrong
        m_uiEarthquakeTimer = 3 * IN_MILLISECONDS;
        TickTimer = 10000;
		ImplosionTimer = urand(4000, 12000);
        m_bEnraged = false;
        HatefulTimer = 5000;

        if (m_pInstance && m_creature->isAlive())
            m_pInstance->SetData(TYPE_GOLEMAGG, NOT_STARTED);


        std::list<Creature*> ChiensListe;
        GetCreatureListWithEntryInGrid(ChiensListe, m_creature, 11672, 150.0f);
        if (ChiensListe.empty() == false)
        {
            for (std::list<Creature*>::iterator itr = ChiensListe.begin(); itr != ChiensListe.end(); ++itr)
            {
                if ((*itr)->getDeathState() == ALIVE)
                    (*itr)->DealDamage((*itr), (*itr)->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                (*itr)->Respawn();
            }
        }
        //    available in creature_addon
        //    m_creature->CastSpell(m_creature, SPELL_MAGMASPLASH, true);
    }

    void Aggro(Unit* pWho)
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_GOLEMAGG, IN_PROGRESS);
    }

    void JustDied(Unit* pKiller)
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_GOLEMAGG, DONE);
    }

    void DoHatefulStr()
    {
        // The ability is used on highest HP target in melee
        // current tank cannot be hit by hateful as long as there are other players in melee

        // todo: can it hit anything other than players?

        Unit* mainTank = m_creature->getVictim();

        // Shouldnt really be possible, but hey, weirder things have happened
        if (!mainTank)
            return;
        const ObjectGuid& mainTankGuid = mainTank->GetObjectGuid();

        Unit* pTarget = nullptr;
        uint32 uiHighestHP = 0;
        Unit* ot1 = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 1, nullptr, SELECT_FLAG_IN_MELEE_RANGE);
        Unit* ot2 = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 2, nullptr, SELECT_FLAG_IN_MELEE_RANGE);

        ThreatList const& tList = m_creature->getThreatManager().getThreatList();
        for (ThreatList::const_iterator iter = tList.begin(); iter != tList.end(); ++iter)
        {
            // Skipping maintank, only using him if there is no other viable target todo: not sure if this is correct. Should we target the MT over the offtanks, if the offtanks have less hp?
            if ((*iter)->getUnitGuid() == mainTankGuid)
                continue;

            if (Unit* pTempTarget = m_creature->GetMap()->GetUnit((*iter)->getUnitGuid()))
            {
                // target has higher hp than anyone checked so far
                if (pTempTarget->GetHealth() > uiHighestHP && (pTempTarget == ot1 || pTempTarget == ot2))
                {
                    if (m_creature->IsInMap(pTempTarget) && m_creature->GetDistance2d(pTempTarget) < 5.0f && !pTempTarget->HasAura(4511) && !pTempTarget->IsImmuneToDamage(m_creature->GetMeleeDamageSchoolMask()))
                    {
                        pTarget = pTempTarget;
                        uiHighestHP = pTarget->GetHealth();
                    }
                }
            }
        }

        // If we found no viable target, we choose the maintank
        if (!pTarget)
            pTarget = mainTank;
        if (pTarget->GetObjectGuid() != previousTarget)
        {
            m_creature->SetInFront(pTarget);
            m_creature->SetTargetGuid(pTarget->GetObjectGuid());
            previousTarget = pTarget->GetObjectGuid();
        }
        DoCastSpellIfCan(pTarget, SPELL_HATEFULSTRIKE, CF_TRIGGERED);
    }

    bool CustomGetTarget()
    {
        if (!m_creature->isAlive())
            return false;

        Unit* target = nullptr;

        // No taunt aura or taunt aura caster is dead, standard target selection
        if (!target && !m_creature->getThreatManager().isThreatListEmpty())
            target = m_creature->getThreatManager().getHostileTarget();

        if (target)
        {
            if (!m_creature->hasUnitState(UNIT_STAT_STUNNED | UNIT_STAT_PENDING_STUNNED | UNIT_STAT_DIED | UNIT_STAT_CONFUSED | UNIT_STAT_FLEEING)
                && (!m_creature->HasAuraType(SPELL_AURA_MOD_FEAR) || m_creature->HasAuraType(SPELL_AURA_PREVENTS_FLEEING)) && !m_creature->HasAuraType(SPELL_AURA_MOD_CONFUSE))
            {

                if (!m_creature->isAttackReady(BASE_ATTACK) && m_creature->IsWithinMeleeRange(target))
                    return true;

                if (target->GetObjectGuid() != previousTarget)
                {
                    m_creature->SetInFront(target);
                    m_creature->SetTargetGuid(target->GetObjectGuid());
                    previousTarget = target->GetObjectGuid();
                }
                AttackStart(target);
            }
            return true;
        }

        if (!m_creature->isInCombat() || m_creature->HasAuraType(SPELL_AURA_MOD_TAUNT) || m_creature->GetCharmerGuid())
            return false;

        if (m_creature->GetMotionMaster()->GetCurrentMovementGeneratorType() != CHASE_MOTION_TYPE)
        {
            for (std::set<Unit*>::const_iterator itr = m_creature->getAttackers().begin(); itr != m_creature->getAttackers().end(); ++itr)
            {
                if ((*itr)->IsInMap(m_creature) && (*itr)->isTargetableForAttack())
                    return false;
            }
        }

        // enter in evade mode in other case
        m_creature->OnLeaveCombat();
        return false;
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        //Hateful
        if (HatefulTimer < uiDiff)
        {
            DoHatefulStr();
            HatefulTimer = 1000;
        }
        else HatefulTimer -= uiDiff;

        // Pyroblast
        if (m_uiPyroblastTimer < uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
				if (!(pTarget->HasAura(SPELL_PYROBLAST) || pTarget->HasAura(SPELL_IMPLOSION) || pTarget->HasAura(SPELL_MARK_OF_GOLEMAGG)))
				{
					if (DoCastSpellIfCan(pTarget, SPELL_PYROBLAST) == CAST_OK)
						m_uiPyroblastTimer = (8 + rand() % 12) * IN_MILLISECONDS;
				}
        }
        else
            m_uiPyroblastTimer -= uiDiff;

		//IMPLOSION
		if (ImplosionTimer < uiDiff)
		{
			if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, nullptr, SELECT_FLAG_PLAYER))
			{
                Unit* ot1 = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 1, nullptr, SELECT_FLAG_IN_MELEE_RANGE);
                Unit* ot2 = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 2, nullptr, SELECT_FLAG_IN_MELEE_RANGE);
                Unit* ot3 = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 3, nullptr, SELECT_FLAG_IN_MELEE_RANGE);
                if (!(pTarget == m_creature->getVictim() || pTarget == ot1 || pTarget == ot2 || pTarget == ot3 || pTarget->HasAura(SPELL_MARK_OF_GOLEMAGG)))
                {
                    if (DoCastSpellIfCan(pTarget, SPELL_IMPLOSION) == CAST_OK)
                        if (m_bEnraged)
                            ImplosionTimer = 8000;
                        else
                            ImplosionTimer = urand(10000, 15000);
                }
			}
		}
		else
			ImplosionTimer -= uiDiff;

        // Enrage
        if (!m_bEnraged && m_creature->GetHealthPercent() < 21.0f)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_ENRAGE) == CAST_OK)
                m_creature->CastSpell(m_creature, 34804, true),
                m_creature->CastSpell(m_creature, SPELL_ENRAGE_10S, true),
                m_bEnraged = true;
        }

        if (TickTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_GOLEMAGG_TRUST) == CAST_OK)
                TickTimer = urand(5000, 8000);
        }
        else TickTimer -= uiDiff;

        // Earthquake
        if (m_bEnraged)
        {
            if (m_uiEarthquakeTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_EARTHQUAKE) == CAST_OK)
                    m_uiEarthquakeTimer = 4000;
            }
            else
                m_uiEarthquakeTimer -= uiDiff;
        }


        DoMeleeAttackIfReady();
    }
};

struct mob_core_ragerAI : public ScriptedAI
{
    mob_core_ragerAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;
    uint32 m_uiMangleTimer;
    uint32 TickTimer;
	uint32 BiteTimer;
    uint32 kiteTimer;

    void Reset()
    {
        TickTimer = 1000;
		BiteTimer = 1000;
        m_uiMangleTimer = 7 * IN_MILLISECONDS;               // These times are probably wrong
        kiteTimer = 20000;
    }

    void DamageTaken(Unit* pDoneBy, uint32& uiDamage)
    {
        if (m_pInstance)
        {
            if (Creature* pGolemagg = m_pInstance->instance->GetCreature(m_pInstance->GetData64(DATA_GOLEMAGG)))
            {
                if (pGolemagg->isAlive())
                {
                    if (m_creature->GetHealthPercent() < 50.0f)
                    {
                        DoScriptText(EMOTE_LOWHP, m_creature);
                        m_creature->SetHealth(m_creature->GetMaxHealth());
                    }
                }
                else
                    m_creature->DisappearAndDie();
            }
        }
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (kiteTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_MARK_OF_GOLEMAGG) == CAST_OK)
                m_creature->getThreatManager().addThreatDirectly(m_creature->getVictim(), urand(1500, 2000)),
                kiteTimer = 20 * IN_MILLISECONDS;
        }
        else
            kiteTimer -= uiDiff;

        // Mangle
        if (m_uiMangleTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_MANGLE) == CAST_OK)
                m_uiMangleTimer = urand(5000, 10000);
        }
        else
            m_uiMangleTimer -= uiDiff;

		//Bite
		if (BiteTimer < uiDiff)
		{
			if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_SERRATED_BITE) == CAST_OK)
				BiteTimer = 10 * IN_MILLISECONDS;
		}
		else
			BiteTimer -= uiDiff;

        if (TickTimer < uiDiff)
        {
            TickTimer = 1000;
            if (!m_creature->HasAura(SPELL_TRASH) && !(bool)(rand() % 10))
                m_creature->CastSpell(m_creature, SPELL_TRASH, true);
        }
        else TickTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_golemagg(Creature* pCreature)
{
    return new boss_golemaggAI(pCreature);
}

CreatureAI* GetAI_mob_core_rager(Creature* pCreature)
{
    return new mob_core_ragerAI(pCreature);
}

void AddSC_boss_golemagg()
{
    Script* newscript;

    newscript = new Script;
    newscript->Name = "boss_golemagg";
    newscript->GetAI = &GetAI_boss_golemagg;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "mob_core_rager";
    newscript->GetAI = &GetAI_mob_core_rager;
    newscript->RegisterSelf();
}
