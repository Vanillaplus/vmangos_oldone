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
SDName: Molten_Core
SD%Complete: 100
SDComment:
SDCategory: Molten Core
EndScriptData */

/* ContentData
mob_ancient_core_hound
EndContentData */

#include "scriptPCH.h"
#include "molten_core.h"

/*######
## mob_firewalker
######*/

enum
{
    SPELL_FIREBLOSSOM         =   19637,
    SPELL_FIREBLOSSOM_CASTING =   19636,
    SPELL_INCITE_FLAMES       =   19635,
};

struct FirewalkerAI : public ScriptedAI
{
    FirewalkerAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;
    uint32 m_uiFireBlossomCasting_Timer;
    uint32 m_uiFireBlossom_Timer;
    uint32 m_uiFireBlossomPreparing_Timer;
    uint32 m_uiInciteFlames_Timer;
    uint32 m_uiNbBlossom;

    void Reset()
    {
        m_uiFireBlossomCasting_Timer   =  6000;
        m_uiFireBlossom_Timer          =     0;
        m_uiFireBlossomPreparing_Timer =     0;
        m_uiInciteFlames_Timer         = 20000;
        m_uiNbBlossom                  =     0;
    }

    void Aggro(Unit* pWho)
    {
        m_creature->SetInCombatWithZone();
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiFireBlossomCasting_Timer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_FIREBLOSSOM_CASTING) == CAST_OK)
                m_uiFireBlossomCasting_Timer   = 12000;

            m_uiNbBlossom = 6;
            m_uiFireBlossomPreparing_Timer = 1000;
        }
        else
            m_uiFireBlossomCasting_Timer -= uiDiff;

        if (m_uiInciteFlames_Timer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_INCITE_FLAMES) == CAST_OK)
                m_uiInciteFlames_Timer = 20000;
        }
        else
            m_uiInciteFlames_Timer -= uiDiff;

        if (m_uiFireBlossomPreparing_Timer < uiDiff)
        {
            if (m_uiNbBlossom > 0)
            {
                if (Unit* pUnit = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                    DoCastSpellIfCan(pUnit, SPELL_FIREBLOSSOM);

                m_uiFireBlossomPreparing_Timer = 1000;
                m_uiNbBlossom = m_uiNbBlossom - 1;
            }
        }
        else
            m_uiFireBlossomPreparing_Timer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_Firewalker(Creature* pCreature)
{
    return new FirewalkerAI(pCreature);
}


/*######
## mob_ancient_core_hound
######*/

enum 
{
    SPELL_CONE_OF_FIRE          = 19630,
    SPELL_VICIOUS_BITE          = 19319,
    SPELL_BITE                  = 19771,

    //Random Debuff (each hound has only one of these)
    SPELL_GROUND_STOMP          = 19364,
    SPELL_ANCIENT_DREAD         = 19365,
    SPELL_CAUTERIZING_FLAMES    = 19366,
    SPELL_WITHERING_HEAT        = 19367,
    SPELL_ANCIENT_DESPAIR       = 19369,
    SPELL_ANCIENT_HYSTERIA      = 19372
};

struct mob_ancient_core_houndAI : public ScriptedAI
{
    mob_ancient_core_houndAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    uint32 m_uiConeOfFireTimer;
    uint32 m_uiRandomDebuffTimer;
    uint32 m_uiBiteTimer;
    uint32 RandDebuff;

    ScriptedInstance* m_pInstance;

    void Reset()
    {
        switch (urand(0, 5))
        {
            case 0 :
                RandDebuff = SPELL_GROUND_STOMP;
                break;
            case 1 :
                RandDebuff = SPELL_ANCIENT_DREAD;
                break;
            case 2 :
                RandDebuff = SPELL_CAUTERIZING_FLAMES;
                break;
            case 3 :
                RandDebuff = SPELL_WITHERING_HEAT;
                break;
            case 4 :
                RandDebuff = SPELL_ANCIENT_DESPAIR;
                break;
            case 5 :
                RandDebuff = SPELL_ANCIENT_HYSTERIA;
                break;
        }
        m_uiConeOfFireTimer   = urand(5000, 7000);
        m_uiRandomDebuffTimer = urand(12000, 15000);
        m_uiBiteTimer         = 4000;
        
        m_creature->SetNoCallAssistance(true);
    }

    void JustDied(Unit* pKiller)
    {
        if (m_pInstance && m_pInstance->GetData(TYPE_MAGMADAR) == DONE)
        {
            m_creature->SetRespawnTime(7 * DAY);
            m_creature->SaveRespawnTime();
        }
    }

    /*
    void MoveInLineOfSight(Unit *pWho) override
    {        
        if (!m_creature->isInCombat())
        {
            if (pWho->IsPlayer() && m_creature->IsWithinDistInMap(pWho, 20.0f) && pWho->isTargetableForAttack())
            {
                // allow Soothe Animal to lower aggro range
                if (m_creature->HasAuraType(SPELL_AURA_MOD_DETECT_RANGE) && !m_creature->IsWithinDistInMap(pWho, 10.0f))
                    return;

                pWho->RemoveSpellsCausingAura(SPELL_AURA_MOD_STEALTH);
                m_creature->AI()->AttackStart(pWho);
            }
        }
    }
    */

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiConeOfFireTimer < uiDiff)
        {
            m_creature->CastSpell(m_creature, SPELL_CONE_OF_FIRE, false);
            m_uiConeOfFireTimer = urand(6000, 8000);
        }
        else m_uiConeOfFireTimer -= uiDiff;

        if (m_uiRandomDebuffTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, RandDebuff) == CAST_OK)
                m_uiRandomDebuffTimer = 22000;
        }
        else m_uiRandomDebuffTimer -= uiDiff;

        if (m_uiBiteTimer < uiDiff)
        {
            if (m_creature->IsWithinMeleeRange(m_creature->getVictim()))
            {
                m_creature->CastSpell(m_creature->getVictim(), SPELL_BITE, false);
                m_uiBiteTimer = 3200;
            }
        }
        else m_uiBiteTimer -= uiDiff;

        if (m_creature->isAttackReady())
        {
            //If we are within range melee the target
            if (m_creature->IsWithinMeleeRange(m_creature->getVictim()))
            {
                m_creature->CastSpell(m_creature->getVictim(), SPELL_VICIOUS_BITE, true);
                m_creature->resetAttackTimer();
            }
        }
    }
};

CreatureAI* GetAI_mob_ancient_core_hound(Creature* pCreature)
{
    return new mob_ancient_core_houndAI(pCreature);
}


/*######
## mob_core_hound
######*/

enum
{
    SPELL_SERRATED_BITE     = 19771,
    SPELL_FIRE_NOVA_VISUAL  = 19823,
	SPELL_PIERCING_HOWL     = 34457,
    SPELL_BREATH            = 34430,
    SPELL_LOWAGGRO          = 34456,
};

struct mob_core_houndAI : public ScriptedAI
{
    mob_core_houndAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 m_uiSerratedBiteTimer;
	uint32 m_uiHowlTimer;
    uint32 m_uiCastTimer;
    uint32 m_uiResurrectTimer;
    uint32 m_uiBreathTimer;
    uint32 m_uiLowAggroTimer;

    bool m_bDead;
    bool m_bResurrectionOkay;

    void ResurrectSelf()
    {
        m_creature->SetHealth(m_creature->GetMaxHealth());
        m_creature->SetStandState(UNIT_STAND_STATE_STAND);
        m_creature->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD);
        m_creature->AttackStop();
        
        m_bDead = false;
    }

    void FeignDeath()
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        m_creature->MonsterTextEmote("Core Hound collapses and begins to smolder.");
        m_creature->SetHealth(0);
        m_creature->RemoveAllAuras();
        m_creature->GetMotionMaster()->Clear();
        m_creature->GetMotionMaster()->MoveIdle();
        m_creature->SetStandState(UNIT_STAND_STATE_DEAD);
        m_creature->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD);
        m_uiResurrectTimer = 10000;
        m_bDead = true;
    }
    
    void Kill_Self()
    {
        m_creature->DealDamage(m_creature, 1, NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
        m_creature->ForcedDespawn();
    }

    void Reset() override
    {
        m_uiSerratedBiteTimer = urand(4000, 7000);
		m_uiHowlTimer = urand(6000, 10000);
        m_uiResurrectTimer = 10000;
        m_uiBreathTimer = urand(10000, 15000);
        m_uiLowAggroTimer = 2000;

        ResurrectSelf();
    }

    void Aggro(Unit* pWho) override
    {
        m_creature->SetInCombatWithZone();
    }

    void DamageTaken(Unit* pDoneBy, uint32 &uiDamage) override
    {
        if (m_creature->GetHealth() < uiDamage)
        {
            // time to fake death
            uiDamage = 0;

            if (!m_bDead)
                FeignDeath();
            return;
       }
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        // Return since we have no target
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim() || !m_pInstance)
            return;

        // Resurrect pack if not died during 10 seconds
        if (m_bDead)
        {
            if (m_uiResurrectTimer < uiDiff)
            {
                m_bResurrectionOkay = false;

                std::list<Creature*> m_CoreHoundList;
                GetCreatureListWithEntryInGrid(m_CoreHoundList, m_creature, NPC_CORE_HOUND, 100.0f);
                for (std::list<Creature*>::const_iterator itr = m_CoreHoundList.begin(); itr != m_CoreHoundList.end(); ++itr)
                {
                    if ((*itr) && (*itr)->isInCombat())
                    {
                        if ((*itr)->GetHealth() > 0)
                            m_bResurrectionOkay = true;
                    }
                }
            
                if (m_bResurrectionOkay)
                {
                    ResurrectSelf();
                    DoCastSpellIfCan(m_creature, SPELL_FIRE_NOVA_VISUAL, CF_TRIGGERED);
                    m_creature->MonsterTextEmote("Core Hound reignites from the heat of another Core Hound!");
                }
                else
                    Kill_Self();
            }
            else
                m_uiResurrectTimer -= uiDiff;
                
            return;
        }

        //Serrated Bite
        if (m_uiSerratedBiteTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(),SPELL_SERRATED_BITE) == CAST_OK)
				m_creature->getThreatManager().modifyThreatPercent(m_creature->getVictim(), -30),
                m_uiSerratedBiteTimer = urand(4000, 7000);
        }
        else
            m_uiSerratedBiteTimer -= uiDiff;

        if (m_uiLowAggroTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_LOWAGGRO) == CAST_OK)
                m_uiLowAggroTimer = urand(8000, 14000);
        }
        else
            m_uiLowAggroTimer -= uiDiff;

		//Piercing Howl
		if (m_uiHowlTimer < uiDiff)
		{
			if (DoCastSpellIfCan(m_creature, SPELL_PIERCING_HOWL) == CAST_OK)
				m_uiHowlTimer = urand(8000, 14000);
		}
		else
			m_uiHowlTimer -= uiDiff;

        if (m_uiBreathTimer < uiDiff)
        {
            if (Unit* pTarget = m_creature->getVictim())
            {
                if (DoCastSpellIfCan(pTarget, SPELL_BREATH) == CAST_OK)
                    m_uiBreathTimer = 12000;
            }
        }
        else
            m_uiBreathTimer -= uiDiff;

        if (!m_bDead)
            DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_mob_core_hound(Creature* pCreature)
{
    return new mob_core_houndAI(pCreature);
}

/*######
## mob_firelord
######*/

enum
{
    SPELL_INCINERATE_AURA   = 19396,
    SPELL_INCINERATE        = 19397,
    SPELL_LAVASPAWN         = 19569,
    SPELL_SOULBURN          = 19393,
	SPELL_CONFLIGURATION    = 16805
};

struct mob_firelordAI : public ScriptedAI
{
    mob_firelordAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 m_uiSummonLavaSpawnTimer;
    uint32 m_uiSoulBurnTimer;
	uint32 m_uiConfligurationTimer;
    uint32 firelords_check_timer;
    bool double_firelords;

    void Reset() override
    {
        m_uiSummonLavaSpawnTimer  = urand(5000, 12000);
        m_uiSoulBurnTimer         = urand(2000, 5000);
		m_uiConfligurationTimer   = urand(13000, 18000);
        firelords_check_timer     = 5000;
        double_firelords          = false;
    }

    void Aggro(Unit* pWho) override
    {
        DoCastSpellIfCan(m_creature, SPELL_INCINERATE_AURA, CF_TRIGGERED | CF_AURA_NOT_PRESENT);
    }
    
    void JustSummoned(Creature* pSummoned) override
    {
        pSummoned->AI()->AttackStart(m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0));
    }

    void Double_Firelord_Check()
    {
        std::list<Creature*> FirelordList;
        GetCreatureListWithEntryInGrid(FirelordList, m_creature, 11668, 50.0f);

        for (std::list<Creature*>::iterator itr = FirelordList.begin(); itr != FirelordList.end();)
        {
            if ((*itr)->getDeathState() != ALIVE)
                itr = FirelordList.erase(itr);
            else
                ++itr;
        }

        if (FirelordList.size() > 1)
            double_firelords = true;
        else
            double_firelords = false;
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        // Return since we have no target
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (firelords_check_timer < uiDiff)
        {
            Double_Firelord_Check();
            firelords_check_timer = 5000;
        }
        else
            firelords_check_timer -= uiDiff;

		// Confliguration
		if (m_uiConfligurationTimer < uiDiff)
		{
			if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_CONFLIGURATION, 0, m_creature->getVictim()->GetGUID()) == CAST_OK)
            {
                if (m_creature->getThreatManager().getThreat(m_creature->getVictim()))
                    m_creature->getThreatManager().modifyThreatPercent(m_creature->getVictim(), -90);
                m_uiConfligurationTimer = urand(12000, 15000);
            }
		}
		else
			m_uiConfligurationTimer -= uiDiff;

        //Summon Lava Spawn
        if (m_uiSummonLavaSpawnTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_LAVASPAWN) == CAST_OK)
                if (double_firelords)
                    m_uiSummonLavaSpawnTimer = urand(20000, 30000);
                else
                    m_uiSummonLavaSpawnTimer = 15000;

        }
        else m_uiSummonLavaSpawnTimer -= uiDiff;

        // Soul Burn
        if (m_uiSoulBurnTimer < uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, nullptr, SELECT_FLAG_PLAYER))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_SOULBURN, CF_TRIGGERED) == CAST_OK)
                    if (double_firelords)
                        m_uiSoulBurnTimer = urand(10000, 12000);
                    else
                        m_uiSoulBurnTimer = 5000;
            }
        }
        else m_uiSoulBurnTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_mob_firelord(Creature* pCreature)
{
    return new mob_firelordAI(pCreature);
}

/*######
## mob_lava_surger
######*/

enum
{
    SPELL_SURGE            = 19196,
	SPELL_SHACKLES         = 34453,
	SPELL_MAGMA_STRIKE     = 34454
};

struct mob_lava_surgerAI : public ScriptedAI
{
    mob_lava_surgerAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    uint32 m_uiSurgeTimer;
	uint32 m_uiMagmaTimer;
	uint32 m_uiMagmaSTimer;
    ScriptedInstance* m_pInstance;

    void Reset()
    {
        m_uiSurgeTimer  = 500;
		m_uiMagmaTimer  = urand(12000, 15000);
		m_uiMagmaSTimer = urand(5000, 8000);

    }

    void JustDied(Unit* pKiller)
    {
        if (m_pInstance && m_pInstance->GetData(TYPE_GARR) == DONE)
        {
            m_creature->SetRespawnTime(7 * DAY);
            m_creature->SaveRespawnTime();
        }
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_creature->HasAuraType(SPELL_AURA_MOD_STUN))     // don't update CDs while Banished
            return;

        // Surge Timer
        if (m_uiSurgeTimer < uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_FARTHEST, 0, SPELL_SURGE, SELECT_FLAG_PLAYER | SELECT_FLAG_IN_LOS))
            {
                if (m_creature->GetDistance2d(pTarget) > 8.0f)
                {
                    if (DoCastSpellIfCan(pTarget, SPELL_SURGE) == CAST_OK)
						DoResetThreat(),
                        m_uiSurgeTimer = urand(5000, 6000);
                }
            }
        }
        else
            m_uiSurgeTimer -= uiDiff;

		// Magmastrike
		if (m_uiMagmaTimer < uiDiff)
		{
			DoCastSpellIfCan(m_creature, SPELL_MAGMA_STRIKE);
			m_uiMagmaTimer = urand(12000, 15000);
		}
		else
			m_uiMagmaTimer -= uiDiff;

		// Magmasshackles
		if (m_uiMagmaSTimer < uiDiff)
		{
			DoCastSpellIfCan(m_creature, SPELL_SHACKLES);
			m_uiMagmaSTimer = urand(12000, 15000);
		}
		else
			m_uiMagmaSTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_mob_lava_surger(Creature* pCreature)
{
    return new mob_lava_surgerAI(pCreature);
}

/*######
## mob_molten_destroyer
######*/

enum
{
    SPELL_TREMOR = 19129,
    SPELL_HATEFULSTRIKE = 34434,
    SPELL_PYROBLAST = 20228
};

struct mob_molten_destroyerAI : public ScriptedAI
{
    mob_molten_destroyerAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    uint32 m_uiTremorTimer;
    uint32 m_uiHatefulTimer;
    uint32 m_uiPyroblastTimer;
    ObjectGuid previousTarget;
    ScriptedInstance* m_pInstance;

    void Reset()
    {
        m_uiTremorTimer = 500;
        m_uiHatefulTimer = 5000;
        m_uiPyroblastTimer = 6000;
        previousTarget = 0;
    }

    void DoHatefulStr()
    {
        Unit* mainTank = m_creature->getVictim();

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
                if (pTempTarget->GetHealth() > uiHighestHP)
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
        m_creature->CastSpell(pTarget, SPELL_HATEFULSTRIKE, false);
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
        /*if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())*/
        if (!CustomGetTarget() || !m_creature->getVictim())
            return;

        //Hateful
        if (m_uiHatefulTimer < uiDiff)
        {
            DoHatefulStr();
            m_uiHatefulTimer = 2000;
        }
        else m_uiHatefulTimer -= uiDiff;

        //Tremor
        if (m_uiTremorTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_TREMOR) == CAST_OK)
                m_uiTremorTimer = urand(10000, 16000);
        }
        else m_uiTremorTimer -= uiDiff;

        //Pyroblast
        if (m_uiPyroblastTimer < uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_BOTTOMAGGRO, 0))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_PYROBLAST) == CAST_OK)
                {
                    m_creature->getThreatManager().modifyThreatPercent(m_creature->getVictim(), 20);
                    m_uiPyroblastTimer = urand(8000, 12000);
                }
                else
                    m_creature->getThreatManager().addThreatDirectly(pTarget, urand(100, 500));
            }
        }
        else m_uiPyroblastTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_mob_molten_destroyer(Creature* pCreature)
{
    return new mob_molten_destroyerAI(pCreature);
}

void AddSC_molten_core()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "mob_firewalker";
    newscript->GetAI = &GetAI_Firewalker;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "mob_ancient_core_hound";
    newscript->GetAI = &GetAI_mob_ancient_core_hound;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "mob_core_hound";
    newscript->GetAI = &GetAI_mob_core_hound;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "mob_lava_surger";
    newscript->GetAI = &GetAI_mob_lava_surger;
	newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "mob_firelord";
	newscript->GetAI = &GetAI_mob_firelord;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "mob_molten_destroyer";
    newscript->GetAI = &GetAI_mob_molten_destroyer;
    newscript->RegisterSelf();
}
