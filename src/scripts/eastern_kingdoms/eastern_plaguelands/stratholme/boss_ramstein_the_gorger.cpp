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
SDName: Boss_Ramstein_the_Gorger
SD%Complete: 70
SDComment:
SDCategory: Stratholme
EndScriptData */

#include "scriptPCH.h"
#include "stratholme.h"

#define SPELL_TRAMPLE       5568
#define SPELL_KNOCKOUT      17307
#define SPELL_HATEFULSTRIKE 34434

#define C_MINDLESS_UNDEAD   11030

struct boss_ramstein_the_gorgerAI : public ScriptedAI
{
    boss_ramstein_the_gorgerAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Engaged = false;
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 Trample_Timer;
    uint32 Knockout_Timer;
	uint32 Hateful_Timer;
    bool Engaged;
	ObjectGuid previousTarget;

    void Reset()
    {
        Trample_Timer = 10000;
        Knockout_Timer = 12500;
		Hateful_Timer = 5000;
        if (Engaged)
            m_pInstance->SetData(TYPE_RAMSTEIN, FAIL);
        Engaged = false;
		previousTarget = 0;
    }

    void Aggro(Unit *who)
    {
        Engaged = true;
        if (m_pInstance)
            m_pInstance->SetData(TYPE_RAMSTEIN, IN_PROGRESS);
    }

    void JustDied(Unit* Killer)
    {
        for (uint8 i = 0; i < 25; ++i)
        {
            if (Creature* temp_skeleton =  m_creature->SummonCreature(C_MINDLESS_UNDEAD, 3969.35f + float(urand(1, 15)), -3391.87f + float(urand(1, 15)), 119.11f, 5.91f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 1800000))
            {
                temp_skeleton->GetMotionMaster()->MovePoint(0, 4033.34f, -3419.75f, 116.35f);
                temp_skeleton->SetHomePosition(4033.34f, -3419.75f, 116.35f, 4.80f);
            }
        }

        if (m_pInstance)
            m_pInstance->SetData(TYPE_RAMSTEIN, DONE);
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

		ThreatList const& tList = m_creature->getThreatManager().getThreatList();
		for (ThreatList::const_iterator iter = tList.begin(); iter != tList.end(); ++iter)
		{
			// Skipping maintank, only using him if there is no other viable target todo: not sure if this is correct. Should we target the MT over the offtanks, if the offtanks have less hp?
			if ((*iter)->getUnitGuid() == mainTankGuid)
				continue;
			/*if (!(*iter)->getUnitGuid().IsPlayer()) //  PLAYER ONLY!
				continue;*/

			if (Unit* pTempTarget = m_creature->GetMap()->GetUnit((*iter)->getUnitGuid()))
			{
				// target has higher hp than anyone checked so far
				if (pTempTarget->GetHealth() > uiHighestHP)
				{
					//v+ target is in melee range, 2d distance will do/added immunity check
					if (m_creature->IsInMap(pTempTarget) && m_creature->GetDistance2d(pTempTarget) < 5.0f && !pTempTarget->HasAura(4511) && !pTempTarget->IsImmuneToDamage(m_creature->GetMeleeDamageSchoolMask())) //v+
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
			// Nostalrius : Correction bug sheep/fear
			if (!m_creature->hasUnitState(UNIT_STAT_STUNNED | UNIT_STAT_PENDING_STUNNED | UNIT_STAT_DIED | UNIT_STAT_CONFUSED | UNIT_STAT_FLEEING)
				&& (!m_creature->HasAuraType(SPELL_AURA_MOD_FEAR) || m_creature->HasAuraType(SPELL_AURA_PREVENTS_FLEEING)) && !m_creature->HasAuraType(SPELL_AURA_MOD_CONFUSE))
			{

				if (!m_creature->isAttackReady(BASE_ATTACK) && m_creature->IsWithinMeleeRange(target)) // he does not have offhand attack
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

		// no target but something prevent go to evade mode // Nostalrius - fix evade quand CM.
		if (!m_creature->isInCombat() || m_creature->HasAuraType(SPELL_AURA_MOD_TAUNT) || m_creature->GetCharmerGuid())
			return false;

		// last case when creature don't must go to evade mode:
		// it in combat but attacker not make any damage and not enter to aggro radius to have record in threat list
		// for example at owner command to pet attack some far away creature
		// Note: creature not have targeted movement generator but have attacker in this case
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

    void UpdateAI(const uint32 diff)
    {
        /*if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())*/
		if (!CustomGetTarget() || !m_creature->getVictim())
            return;

		//Hateful
		if (Hateful_Timer < diff)
		{
			DoHatefulStr();
			Hateful_Timer = 10000;
			if (Knockout_Timer < 2000)
				Knockout_Timer += 2000;
		}
		else Hateful_Timer -= diff;

        //Trample
        if (Trample_Timer < diff)
        {
            DoCastSpellIfCan(m_creature, SPELL_TRAMPLE);
            Trample_Timer = 20000;
        }
        else Trample_Timer -= diff;

        //Knockout
        if (Knockout_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_KNOCKOUT) == CAST_OK)
            {
                m_creature->getThreatManager().modifyThreatPercent(m_creature->getVictim(), 20);
                Knockout_Timer = urand(9000, 15000);
				if (Hateful_Timer < 2000)
					Hateful_Timer += 2000;
            }
        }
        else Knockout_Timer -= diff;

        DoMeleeAttackIfReady();
    }
};
CreatureAI* GetAI_boss_ramstein_the_gorger(Creature* pCreature)
{
    return new boss_ramstein_the_gorgerAI(pCreature);
}

void AddSC_boss_ramstein_the_gorger()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "boss_ramstein_the_gorger";
    newscript->GetAI = &GetAI_boss_ramstein_the_gorger;
    newscript->RegisterSelf();
}
