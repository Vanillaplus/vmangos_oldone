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
SDName: Boss_High_Inquisitor_Fairbanks
SD%Complete: 100
SDComment: TODO: if this guy not involved in some special event, remove (and let ACID script)
SDCategory: Scarlet Monastery
EndScriptData */

#include "scriptPCH.h"
#include "scarlet_monastery.h"
#include "PathFinder.h"

enum
{
    SAY_AGGRO                       = -1900330, // The air... it hurts! It hurts!
    SAY_DEATH1                      = -1900331, // At last… the wall is silent.
    SAY_DEATH2                      = -1900332, // Is that… sunlight?
    SAY_DEATH3                      = -1900333, // I feel... warm...
    SAY_SLAY1                       = -1900334, // Sleep there. It is warmer than it looks.
    SAY_SLAY2                       = -1900335, // Now you can hear it too.
    SAY_SLAY3                       = -1900336, // Don't scream... they hate it.
    SAY_CLAUSTRO                    = -1900337, // Not again! Not again!

    SPELL_VISIBILITY_STEALTH        = 35142,

    SPELL_BILE                      = 36024,
    SPELL_STOMP_AURA                = 36026,
    SPELL_CLAUSTROPHOBIA            = 36027,
    SPELL_CLAUSTROPHOBIA_SELF       = 36213,
    SPELL_POWERWORDSHIELD           = 36033,
    SPELL_HATRED                    = 36035,
    SPELL_CRUSH                     = 36036,


    NPC_COCKROACH                   = 25255,
    NPC_BILE_GREEN_FOG              = 25256,
};

struct boss_high_inquisitor_fairbanksAI : public ScriptedAI
{
    boss_high_inquisitor_fairbanksAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 PosCheckTimer;
    uint32 Visibility_Check_Timer;
    uint32 Bile_Timer;
    uint32 ClPhobia_Timer;
    uint32 PowerWordShield;

    bool bAshbringer;

    void Reset()
    {
        PosCheckTimer = 500;
        Visibility_Check_Timer = 1000;
        Bile_Timer = urand(8000, 15000);
        ClPhobia_Timer = urand(25000, 35000);
        PowerWordShield = urand(15000, 22000);
        bAshbringer = false;
    }

    void Aggro(Unit* pWho)
    {
        m_creature->SetInCombatWithZone();
        m_creature->AddAura(SPELL_STOMP_AURA);

        DoScriptText(SAY_AGGRO, m_creature);

        Map::PlayerList const& players = m_creature->GetMap()->GetPlayers();
        if (players.isEmpty())
            return;

        for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
        {
            if (Player* pPlayer = itr->getSource())
            {
                if (pPlayer->GetPositionY() > 1283.47f && pPlayer->GetPositionY() < 1287.3f && pPlayer->GetPositionX() > 804.0f)
                    m_creature->CastSpell(pPlayer, SPELL_CRUSH, true);
            }
        }

        if (m_pInstance)
            m_pInstance->SetData(TYPE_FAIRBANKS, IN_PROGRESS);
    }

    void KilledUnit(Unit* /*pVictim*/) override
    {
        if (m_creature->HasAura(34821))
            return;

        m_creature->CastSpell(m_creature, 34821, true);
        switch (urand(0, 2))
        {
            case 0: DoScriptText(SAY_SLAY1, m_creature); break;
            case 1: DoScriptText(SAY_SLAY2, m_creature); break;
            case 2: DoScriptText(SAY_SLAY3, m_creature); break;
        }
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        switch (urand(0, 2))
        {
            case 0: DoScriptText(SAY_DEATH1, m_creature); break;
            case 1: DoScriptText(SAY_DEATH2, m_creature); break;
            case 2: DoScriptText(SAY_DEATH3, m_creature); break;
        }

        if (m_pInstance)
            m_pInstance->SetData(TYPE_FAIRBANKS, DONE);
    }

    void JustReachedHome()
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_FAIRBANKS, FAIL);

        std::list<Creature*> dummy;
        GetCreatureListWithEntryInGrid(dummy, m_creature, NPC_BILE_GREEN_FOG, 50.0f);
        GetCreatureListWithEntryInGrid(dummy, m_creature, NPC_COCKROACH, 50.0f);
        for (auto it = dummy.cbegin(); it != dummy.cend(); ++it)
        {
            (*it)->RemoveAllDynObjects();
            (*it)->DespawnOrUnsummon();
        }
        m_creature->RemoveAllDynObjects();
    }

    void SpellHit(Unit* pCaster, const SpellEntry* pSpell) override
    {
        if (pSpell->Id == 28441 && !bAshbringer && pCaster && me->IsWithinLOSInMap(pCaster))
        {
            me->SetSheath(SHEATH_STATE_UNARMED);
            me->CastSpell(me, 28443, true); // transform
            me->SetFacingToObject(pCaster);
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            bAshbringer = true;
        }

        for (uint8 i = 0; i < MAX_EFFECT_INDEX; ++i)
        {
            if (pSpell->Effect[i] == SPELL_EFFECT_ATTACK_ME)
            {
                DoResetThreat();
                m_creature->getThreatManager().addThreat(pCaster, 1000000);
                m_creature->AddAura(SPELL_HATRED);
            }
        }
    }

    void DoBile()
    {
        float dist = 2.0f;
        float o = m_creature->GetOrientation();
        float x = m_creature->GetPositionX() + cos(o) * dist;
        float y = m_creature->GetPositionY() + sin(o) * dist;
        float z = m_creature->GetPositionZ();

        m_creature->UpdateAllowedPositionZ(x, y, z);
        m_creature->SummonCreature(NPC_BILE_GREEN_FOG, x, y, z + 0.05f, o, TEMPSUMMON_TIMED_DESPAWN, 120000);

        std::list<Player*> players;
        GetPlayersWithinRange(players, 20.0f);
        int Number = 1;
        if (players.size() > 3)
            Number = players.size() / 3;

        for (int i = 0; i < Number * 4; ++i)
        {
            if (Creature* Crea = m_creature->SummonCreature(NPC_COCKROACH, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 60000))
                if (Unit* target = m_creature->SelectAttackingTarget(ATTACKING_TARGET_BOTTOMAGGRO, (i / 4), nullptr, SELECT_FLAG_PLAYER))
                    Crea->getThreatManager().addThreat(target, 1000000);
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
        {
            if (Visibility_Check_Timer < diff)
            {
                m_creature->HandleEmoteState(EMOTE_STATE_SLEEP);
                m_creature->SetStandState(UNIT_STAND_STATE_SLEEP);

                Visibility_Check_Timer = 5000;
            }
            else Visibility_Check_Timer -= diff;

            return;
        }

        if (PosCheckTimer < diff)
        {
            if (m_creature->GetPositionY() < 1285.0f)
                EnterEvadeMode();

            PosCheckTimer = 1000;
        }
        else PosCheckTimer -= diff;

        if (Bile_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_BILE) == CAST_OK)
            {
                DoBile();
                Bile_Timer = urand(24000, 30000);
            }
        }
        else Bile_Timer -= diff;

        if (ClPhobia_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_CLAUSTROPHOBIA) == CAST_OK)
            {
                DoScriptText(SAY_CLAUSTRO, m_creature);
                m_creature->CastSpell(m_creature, SPELL_CLAUSTROPHOBIA_SELF, true);
                ClPhobia_Timer = urand(50000, 60000);
            }
        }
        else ClPhobia_Timer -= diff;

        if (PowerWordShield < diff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_POWERWORDSHIELD) == CAST_OK)
                PowerWordShield = urand(25000, 32000);
        }
        else PowerWordShield -= diff;

        DoMeleeAttackIfReady();
    }
};


CreatureAI* GetAI_boss_high_inquisitor_fairbanks(Creature* pCreature)
{
    return new boss_high_inquisitor_fairbanksAI(pCreature);
}

/********************
*** Scarlet Adept ***
*********************/

enum
{
    SPELL_SMITE = 36106,
    SPELL_HEAL  = 36107,
};

struct npc_scarlet_adeptAI : public ScriptedAI
{
    npc_scarlet_adeptAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiTimer;

    void Reset()
    {
        m_uiTimer = 1000;
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiTimer < uiDiff)
        {
            if (Unit* pUnit = m_creature->FindLowestHpFriendlyUnit(30.0f, 49, true, m_creature))
            {
                if (DoCastSpellIfCan(pUnit, SPELL_HEAL) == CAST_OK)
                    m_uiTimer = 2000;
                else if (m_creature->GetHealthPercent() < 30.0f && DoCastSpellIfCan(m_creature, SPELL_HEAL) == CAST_OK)
                    m_uiTimer = 2000;
                else if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_SMITE) == CAST_OK)
                    m_uiTimer = 2000;
            }
            else if (m_creature->GetHealthPercent() < 30.0f && DoCastSpellIfCan(m_creature, SPELL_HEAL) == CAST_OK)
                m_uiTimer = 2000;
            else if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_SMITE) == CAST_OK)
                m_uiTimer = 2000;
            else m_uiTimer = 200;
        }
        else m_uiTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_scarlet_adept(Creature* pCreature)
{
    return new npc_scarlet_adeptAI(pCreature);
}

/********************
*** Scarlet Evoker ***
*********************/

enum
{
    SPELL_PURYFING_FLAMES = 36086,
    SPELL_FIERY_PURGE     = 36088,
    SPELL_FIREBALL        = 36090,
    SPELL_FLAMESTRIKE     = 36091,
    SPELL_FIRE_SHIELD     = 36092,
};

struct npc_scarlet_evokerAI : public ScriptedAI
{
    npc_scarlet_evokerAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiFriendlyDispel_Timer;
    uint32 m_uiOffensiveDispel_Timer;
    uint32 m_uiFlamestrike_Timer;
    uint32 m_uiFireShield_Timer;
    uint32 m_uiFireball_Timer;

    void Reset()
    {
        m_uiFriendlyDispel_Timer = urand(5000, 10000);
        m_uiOffensiveDispel_Timer = urand(5000, 10000);
        m_uiFlamestrike_Timer = urand(8000, 10000);
        m_uiFireShield_Timer = urand(2000, 16000);
        m_uiFireball_Timer = 500;
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiFlamestrike_Timer < uiDiff)
        {
            if (Unit* target = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_FLAMESTRIKE, SELECT_FLAG_PLAYER))
            {
                if (DoCastSpellIfCan(target, SPELL_FLAMESTRIKE) == CAST_OK)
                    m_uiFlamestrike_Timer = urand(8000, 10000);
            }
        }
        else m_uiFlamestrike_Timer -= uiDiff;

        if (m_uiFriendlyDispel_Timer < uiDiff)
        {
            if (Unit* pUnit = m_creature->FindFriendlyUnitDispel(40.0f, true, true, true, false))
            {
                if (DoCastSpellIfCan(pUnit, SPELL_PURYFING_FLAMES) == CAST_OK)
                    m_uiFriendlyDispel_Timer = 3000;
                else
                {
                    Unit::SpellAuraHolderMap const& auras = m_creature->getVictim()->GetSpellAuraHolderMap();
                    for (Unit::SpellAuraHolderMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                    {
                        SpellAuraHolder* holder = itr->second;

                        if (!holder)
                            continue;

                        SpellEntry const* proto = holder->GetSpellProto();
                        if (!proto)
                            continue;

                        if (proto->Dispel == DISPEL_MAGIC)
                        {
                            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_FIERY_PURGE) == CAST_OK)
                            {
                                m_uiFriendlyDispel_Timer = 3000;
                            }
                            return;
                        }
                    }
                    m_uiFriendlyDispel_Timer = 200;
                }
            }
        }
        else m_uiFriendlyDispel_Timer -= uiDiff;

        if (m_uiOffensiveDispel_Timer < uiDiff)
        {
            Unit::SpellAuraHolderMap const& auras = m_creature->getVictim()->GetSpellAuraHolderMap();
            for (Unit::SpellAuraHolderMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
            {
                SpellAuraHolder* holder = itr->second;

                if (!holder)
                    continue;

                SpellEntry const* proto = holder->GetSpellProto();
                if (!proto)
                    continue;

                if (proto->Dispel == DISPEL_MAGIC)
                {
                    if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_FIERY_PURGE) == CAST_OK)
                    {
                        m_uiOffensiveDispel_Timer = urand(5000, 10000);
                    }
                    return;
                }
            }
            m_uiOffensiveDispel_Timer = 200;
        }
        else m_uiOffensiveDispel_Timer -= uiDiff;

        if (m_uiFireShield_Timer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_FIRE_SHIELD) == CAST_OK)
                m_uiFireShield_Timer = urand(18000, 25000);
        }
        else m_uiFireShield_Timer -= uiDiff;

        if (m_uiFireball_Timer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_FIREBALL) == CAST_OK)
                m_uiFireball_Timer = 1800;
        }
        else m_uiFireball_Timer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_scarlet_evoker(Creature* pCreature)
{
    return new npc_scarlet_evokerAI(pCreature);
}


/********************
*** Scarlet Conjuror ***
*********************/

enum
{
    SPELL_FIREBALL_CONJ = 36090,
    SPELL_SUMMON_VISUAL = 36108,
    NPC_FIRE_ELEMENTAL  = 25200,
    NPC_WATER_ELEMENTAL = 25258,
};

struct npc_scarlet_conjurorAI : public ScriptedAI
{
    npc_scarlet_conjurorAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiSummonTimer;
    uint32 m_uiFireball_Timer;
    uint32 m_uiConsume_Timer;
    uint32 m_uiConsume_Timer2;

    void Reset()
    {
        m_uiSummonTimer = 1000;
        m_uiFireball_Timer = 1000;
        m_uiConsume_Timer = 1000;
        m_uiConsume_Timer2 = 1000;
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiSummonTimer < uiDiff)
        {
            if (!m_creature->hasUnitState(UNIT_STAT_STUNNED | UNIT_STAT_CONFUSED | UNIT_STAT_FLEEING) && !m_creature->HasAuraType(SPELL_AURA_MOD_SILENCE))
            {
                if (Creature* Crea = m_creature->SummonCreature(NPC_FIRE_ELEMENTAL, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 90000))
                {
                    Crea->SetInCombatWithZone();
                    m_creature->CastSpell(Crea, SPELL_SUMMON_VISUAL, true);
                    m_uiSummonTimer = 12000;
                }
            }
        }
        else m_uiSummonTimer -= uiDiff;

        if (m_uiConsume_Timer < uiDiff)
        {
            std::list<Creature*> dummy;
            GetCreatureListWithEntryInGrid(dummy, m_creature, NPC_FIRE_ELEMENTAL, 30.0f);
            GetCreatureListWithEntryInGrid(dummy, m_creature, NPC_WATER_ELEMENTAL, 30.0f);
            for (auto it = dummy.cbegin(); it != dummy.cend(); ++it)
            {
                if ((*it)->isAlive() && (*it)->IsWithinLOSInMap(m_creature))
                {
                    if ((*it)->GetHealthPercent() < 20.0f)
                    {
                        int32 HP = (*it)->GetHealth();

                        m_creature->CastCustomSpell(m_creature, 36110, &HP, &HP, nullptr, true, nullptr);
                        m_creature->CastSpell((*it), 36111, true);
                        m_uiConsume_Timer = 1500;
                        break;
                    }
                }
            }
            if (m_uiConsume_Timer < 1000)
                m_uiConsume_Timer = 500;
        }
        else m_uiConsume_Timer -= uiDiff;


        if (m_uiConsume_Timer2 < uiDiff)
        {
            if (m_creature->GetHealthPercent() < 30.0f)
            {
                std::list<Creature*> dummy;
                GetCreatureListWithEntryInGrid(dummy, m_creature, NPC_FIRE_ELEMENTAL, 30.0f);
                GetCreatureListWithEntryInGrid(dummy, m_creature, NPC_WATER_ELEMENTAL, 30.0f);

                Creature* pBestTarget = nullptr;
                float bestHp = 100.1f;

                for (auto it = dummy.cbegin(); it != dummy.cend(); ++it)
                {
                    Creature* pElem = *it;
                    if (!pElem || !pElem->isAlive() || !pElem->IsWithinLOSInMap(m_creature))
                        continue;

                    float hp = pElem->GetHealthPercent();
                    if (hp < bestHp)
                    {
                        bestHp = hp;
                        pBestTarget = pElem;
                    }
                }

                if (pBestTarget)
                {
                    int32 HP = pBestTarget->GetHealth();

                    m_creature->CastCustomSpell(m_creature, 36110, &HP, &HP, nullptr, true, nullptr);
                    m_creature->CastSpell(pBestTarget, 36111, true);
                    m_uiConsume_Timer2 = 10000;
                }
            }

            if (m_uiConsume_Timer2 < 1000)
                m_uiConsume_Timer2 = 500;
        }
        else m_uiConsume_Timer2 -= uiDiff;



        if (m_uiFireball_Timer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_FIREBALL_CONJ) == CAST_OK)
                m_uiFireball_Timer = 1800;
        }
        else m_uiFireball_Timer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_scarlet_conjuror(Creature* pCreature)
{
    return new npc_scarlet_conjurorAI(pCreature);
}

/********************
*** Scarlet Abbot ***
*********************/

enum
{
    SPELL_GREATER_HEAL    = 36113,
    SPELL_FLASH_HEAL      = 36114,
    SPELL_RENEW           = 36115,
    SPELL_RENEW_CD        = 36116,
    SPELL_PRAYER_HEAL     = 36117,
    SPELL_LOS_CHECK_AURA  = 36118,
    SPELL_NEAR_CHECK_AURA = 36119,
};

struct npc_scarlet_abbotAI : public ScriptedAI
{
    npc_scarlet_abbotAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiRenew_Timer;
    uint32 m_uiHealing_Timer;

    void Reset()
    {
        m_uiRenew_Timer = 1000;
        m_uiHealing_Timer = 1000;
    }
    
    float GetPathLen(Unit* mover, Unit* target)
    {
        PathInfo path(mover);

        float x = target->GetPositionX();
        float y = target->GetPositionY();
        float z = target->GetPositionZ();

        if (!path.calculate(x, y, z, false, false))
            return mover->GetDistance(target);

        PathType t = path.getPathType();

        if (t & (PATHFIND_NOPATH | PATHFIND_NOT_USING_PATH))
            return mover->GetDistance(target);

        float len = path.Length();
        if (len <= 0.0f)
            return mover->GetDistance(target);

        return len;
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiRenew_Timer < uiDiff)
        {
            if (!m_creature->IsNonMeleeSpellCasted(false))
            {
                std::list<Creature*> nearbyAllies;
                GetCreatureListFriendlyWithMissingBuff(nearbyAllies, m_creature, 45.0f, SPELL_RENEW);

                if (!nearbyAllies.empty())
                {
                    for (std::list<Creature*>::iterator itr = nearbyAllies.begin(); itr != nearbyAllies.end(); ++itr)
                    {
                        if (!(*itr)->HasAura(SPELL_RENEW_CD))
                        {
                            if (DoCastSpellIfCan((*itr), SPELL_RENEW) == CAST_OK)
                            {
                                m_creature->CastSpell((*itr), SPELL_RENEW_CD, true);
                                m_uiRenew_Timer = 1500;
                                m_uiHealing_Timer = 1000;
                                return;
                            }
                        }
                    }
                }
                m_uiRenew_Timer = 200;
            }
        }
        else m_uiRenew_Timer -= uiDiff;

        if (m_uiHealing_Timer < uiDiff)
        {
            if (!m_creature->IsNonMeleeSpellCasted(false))
            {
                std::list<Creature*> nearbyAlliesAny;

                GetCreatureListExceptInGrid(nearbyAlliesAny, m_creature, true, 63, 45.0f);

                Unit* pBestLoSTarget = nullptr;
                float bestLoSHp = 100.1f;

                Unit* pBestAnyTarget = nullptr;
                float bestAnyHp = 100.1f;

                for (std::list<Creature*>::iterator itr = nearbyAlliesAny.begin(); itr != nearbyAlliesAny.end(); ++itr)
                {
                    Creature* pAlly = *itr;

                    if (!pAlly || !pAlly->isAlive() || !pAlly->isInCombat() || !m_creature->IsInMap(pAlly))
                        continue;

                    float hp = pAlly->GetHealthPercent();

                    if (hp < bestAnyHp)
                    {
                        pBestAnyTarget = pAlly;
                        bestAnyHp = hp;
                    }

                    if (m_creature->IsWithinLOSInMap(pAlly) && hp < bestLoSHp)
                    {
                        pBestLoSTarget = pAlly;
                        bestLoSHp = hp;
                    }
                }

                if (m_creature->GetMotionMaster()->GetCurrentMovementGeneratorType() == FOLLOW_MOTION_TYPE)
                {
                    if (Unit* pVictim = m_creature->getVictim())
                    {
                        m_creature->GetMotionMaster()->Clear(false);
                        m_creature->GetMotionMaster()->MoveChase(pVictim);
                    }
                    else
                    {
                        m_creature->GetMotionMaster()->Clear(false);
                        m_creature->GetMotionMaster()->MoveIdle();
                    }
                }

                if (pBestLoSTarget && bestLoSHp < 90.0f)
                {
                    if (bestLoSHp < 20.0f || (pBestLoSTarget->GetMaxHealth() - pBestLoSTarget->GetHealth()) < 15000)
                        m_creature->CastSpell(pBestLoSTarget, SPELL_FLASH_HEAL, false);
                    else
                        m_creature->CastSpell(pBestLoSTarget, SPELL_GREATER_HEAL, false);

                    m_uiHealing_Timer = 1200;
                    return;
                }

                if (pBestAnyTarget && bestAnyHp < 90.0f && m_creature->GetPower(POWER_MANA) > 1000)
                {
                    DoResetThreat();
                    m_creature->GetMotionMaster()->MoveFollow(pBestAnyTarget, 0.0f, 0.0f);

                    float pathLen = GetPathLen(m_creature, pBestAnyTarget);
                    uint32 travelTime = uint32((pathLen / 5.0f) * 1000.0f);
                    if (travelTime > 8000)
                        travelTime = 8000;

                    m_uiHealing_Timer = travelTime;
                    return;
                }

                m_uiHealing_Timer = 1000;
            }
        }
        else
            m_uiHealing_Timer -= uiDiff;


        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_scarlet_abbot(Creature* pCreature)
{
    return new npc_scarlet_abbotAI(pCreature);
}


/********************
*** Scarlet Champion ***
*********************/

enum
{
    SPELL_SHATTERED_BONES       = 36121,
    SPELL_DUEL                  = 36124,
    SPELL_DUEL_BUFF             = 36125,
    SPELL_DUEL_CD               = 36126,
    SPELL_DUEL_WINNER           = 36130,
    SPELL_DUEL_TAUNT_IMMUNITY   = 36334,
    SPELL_VENGEANCE_CAST        = 36127,
    SPELL_VENGEANCE_CAST_DUMMY  = 36128,
    SPELL_VENGEANCE_DAM         = 36129,
};

struct npc_scarlet_championAI : public ScriptedAI
{
    npc_scarlet_championAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiDuel_Timer;
    uint32 m_uiVengeance_Timer;
    uint32 m_uiVengeance_Casting_Timer;
    uint32 Vengeance_Damage;
    bool Vengeance_Cast;
    bool Duel;

    void Reset()
    {
        m_uiDuel_Timer = urand(10000, 20000);
        m_uiVengeance_Timer = urand(8000, 15000);
        m_uiVengeance_Casting_Timer = 600000;
        Vengeance_Damage = 0;
        Vengeance_Cast = false;
        Duel = false;
    }

    void DamageTaken(Unit* pDoneBy, uint32& uiDamage)
    {
        if (Vengeance_Cast)
            Vengeance_Damage += uiDamage;
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiDuel_Timer < uiDiff)
        {
            if (!Duel)
            {
                Duel = true;
                m_uiDuel_Timer = urand(15000, 22000);

                if (Unit* target = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_DUEL, SELECT_FLAG_PLAYER))
                {
                    if (!target->HasAura(SPELL_SHATTERED_BONES))
                    {
                        DoResetThreat();
                        m_creature->getThreatManager().addThreat(target, 1000000);
                        m_creature->CastSpell(target, SPELL_DUEL, true);
                        m_creature->CastSpell(target, SPELL_DUEL_BUFF, true);
                        m_creature->CastSpell(m_creature, SPELL_DUEL_TAUNT_IMMUNITY, true);
                    }
                }

            }
            else if (!m_creature->HasAura(SPELL_DUEL))
                Duel = false;

        }
        else m_uiDuel_Timer -= uiDiff;

        if (m_uiVengeance_Timer < uiDiff)
        {
            Vengeance_Cast = true;
            m_creature->CastSpell(m_creature, SPELL_VENGEANCE_CAST, false);
            m_creature->CastSpell(m_creature, SPELL_VENGEANCE_CAST_DUMMY, true);
            m_uiVengeance_Casting_Timer = 3000;

            if (m_uiDuel_Timer < 3500)
                m_uiDuel_Timer = 3500;

            m_uiVengeance_Timer = urand(12000, 15000);
        }
        else m_uiVengeance_Timer -= uiDiff;

        if (m_uiVengeance_Casting_Timer < uiDiff)
        {
            int32 Dam = Vengeance_Damage / 3;
            m_creature->CastCustomSpell(m_creature, SPELL_VENGEANCE_DAM, &Dam, nullptr, nullptr, true, nullptr);

            Vengeance_Cast = false;
            Vengeance_Damage = 0;

            m_uiVengeance_Casting_Timer = 600000;
        }
        else m_uiVengeance_Casting_Timer -= uiDiff;


        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_scarlet_champion(Creature* pCreature)
{
    return new npc_scarlet_championAI(pCreature);
}



/********************
*** Scarlet Chaplain ***
*********************/

enum
{
    SPELL_AOE_DISPEL        = 36138,
    SPELL_AOE_BUFF          = 36140,
    SPELL_AOE_SHIELD        = 36141,
    SPELL_REQUITAL          = 36143,
};

struct npc_scarlet_chaplainAI : public ScriptedAI
{
    npc_scarlet_chaplainAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiDispel_Timer;
    uint32 m_uiBuff_Timer;
    uint32 m_uiShield_Timer;
    uint32 m_uiRequital_Timer;
    uint32 m_uiRequital_Stun_Timer;

    void Reset()
    {
        m_uiDispel_Timer = urand(8000, 10000);
        m_uiBuff_Timer = 1000;
        m_uiShield_Timer = 5000;
        m_uiRequital_Timer = urand(8000, 15000);
        m_uiRequital_Stun_Timer = 5000;
    }

    void DoStunRequital()
    {
        ThreatList const& tList = m_creature->getThreatManager().getThreatList();
        for (ThreatList::const_iterator itr = tList.begin(); itr != tList.end(); ++itr)
        {
            if (Unit* RequitalTargets = m_creature->GetMap()->GetUnit((*itr)->getUnitGuid()))
            {
                if (m_creature->GetDistance(RequitalTargets) < 20.0f && m_creature->IsWithinLOSInMap(RequitalTargets))
                {
                    if (RequitalTargets->hasUnitState(UNIT_STAT_STUNNED | UNIT_STAT_PENDING_STUNNED | UNIT_STAT_FLEEING))
                    {
                        if (DoCastSpellIfCan(RequitalTargets, SPELL_REQUITAL) == CAST_OK)
                        {
                            m_uiRequital_Timer = urand(12000, 15000);
                            m_uiRequital_Stun_Timer = 8000;
                            return;
                        }
                    }
                }
            }
        }

        m_uiRequital_Stun_Timer = 500;
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiDispel_Timer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_AOE_DISPEL) == CAST_OK)
                m_uiDispel_Timer = urand(30000, 35000);
        }
        else m_uiDispel_Timer -= uiDiff;

        if (m_uiBuff_Timer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_AOE_BUFF) == CAST_OK)
                m_uiBuff_Timer = urand(30000, 35000);
        }
        else m_uiBuff_Timer -= uiDiff;

        if (m_uiShield_Timer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_AOE_SHIELD) == CAST_OK)
                m_uiShield_Timer = urand(30000, 35000);
        }
        else m_uiShield_Timer -= uiDiff;


        if (m_uiRequital_Timer < uiDiff)
        {
            if (Unit* target = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_REQUITAL, SELECT_FLAG_PLAYER))
            {
                if (DoCastSpellIfCan(target, SPELL_REQUITAL) == CAST_OK)
                {
                    m_uiRequital_Timer = urand(12000, 15000);
                    m_uiRequital_Stun_Timer = 8000;
                }
            }
        }
        else m_uiRequital_Timer -= uiDiff;

        if (m_uiRequital_Stun_Timer < uiDiff)
        {
            DoStunRequital();
        }
        else m_uiRequital_Stun_Timer -= uiDiff;


        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_scarlet_chaplain(Creature* pCreature)
{
    return new npc_scarlet_chaplainAI(pCreature);
}


/********************
*** Scarlet Wizard ***
*********************/

enum
{
    SPELL_ICE_ARMOR  = 36144,
    SPELL_FROSTBOLT  = 36146,
    SPELL_WATER_ELE  = 36147,
    SPELL_CONE_COLD  = 36148,

    SPELL_FROST_AURA = 36149,
    SPELL_FROST_NOVA = 36150,
    SPELL_ICE_BLOCK  = 36151,
};

struct npc_scarlet_wizardAI : public ScriptedAI
{
    npc_scarlet_wizardAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiFrostbolt_Timer;
    uint32 m_uiElemental_Timer;
    uint32 m_uiConeOfCold_Timer;
    uint32 HP_Check_Timer;
    uint32 Iceblock_Timer;

    int32 TotalTargets;
    int32 FaceTargets;

    void Reset()
    {
        m_uiFrostbolt_Timer = 500;
        m_uiElemental_Timer = urand(12000, 20000);
        m_uiConeOfCold_Timer = 5000;
        HP_Check_Timer = 200;
        Iceblock_Timer = 6000000;
        TotalTargets = 0;
        FaceTargets = 0;
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (HP_Check_Timer < uiDiff)
        {
            HP_Check_Timer = 200;
            if (m_creature->GetHealthPercent() < 30.0f)
            {
                Iceblock_Timer = 6000;
                HP_Check_Timer = 6000;
            }
                
        }
        else HP_Check_Timer -= uiDiff;

        if (Iceblock_Timer < uiDiff)
        {
            m_creature->CastSpell(m_creature, SPELL_FROST_AURA, true);
            m_creature->CastSpell(m_creature, SPELL_FROST_NOVA, true);
            m_creature->CastSpell(m_creature, SPELL_ICE_BLOCK, true);

            Iceblock_Timer = 6000000;
        }
        else Iceblock_Timer -= uiDiff;


        if (m_uiConeOfCold_Timer < uiDiff)
        {
            TotalTargets = 0;
            FaceTargets = 0;

            ThreatList const& tList = m_creature->getThreatManager().getThreatList();
            for (ThreatList::const_iterator itr = tList.begin(); itr != tList.end(); ++itr)
            {
                if (Unit* AoETarget = m_creature->GetMap()->GetUnit((*itr)->getUnitGuid()))
                {
                    if (m_creature->GetDistance(AoETarget) < 14.0f && m_creature->IsWithinLOSInMap(AoETarget))
                    {
                        ++TotalTargets;

                        if (m_creature->HasInArc(M_PI_F, AoETarget))
                            ++FaceTargets;
                    }
                }
            }

            if (FaceTargets * 2 >= TotalTargets)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_CONE_COLD) == CAST_OK)
                    m_uiConeOfCold_Timer = urand(15000, 20000);
                else
                    m_uiConeOfCold_Timer = 200;
            }
        }
        else m_uiConeOfCold_Timer -= uiDiff;

        if (m_uiElemental_Timer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_WATER_ELE) == CAST_OK)
                m_uiElemental_Timer = urand(15000, 20000);
        }
        else m_uiElemental_Timer -= uiDiff;

        if (m_uiFrostbolt_Timer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_FROSTBOLT) == CAST_OK)
                m_uiFrostbolt_Timer = 1700;
        }
        else m_uiFrostbolt_Timer -= uiDiff;


        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_scarlet_wizard(Creature* pCreature)
{
    return new npc_scarlet_wizardAI(pCreature);
}

void AddSC_boss_high_inquisitor_fairbanks()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "boss_high_inquisitor_fairbanks";
    newscript->GetAI = &GetAI_boss_high_inquisitor_fairbanks;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_scarlet_adept";
    newscript->GetAI = &GetAI_npc_scarlet_adept;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_scarlet_evoker";
    newscript->GetAI = &GetAI_npc_scarlet_evoker;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_scarlet_conjuror";
    newscript->GetAI = &GetAI_npc_scarlet_conjuror;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_scarlet_abbot";
    newscript->GetAI = &GetAI_npc_scarlet_abbot;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_scarlet_champion";
    newscript->GetAI = &GetAI_npc_scarlet_champion;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_scarlet_chaplain";
    newscript->GetAI = &GetAI_npc_scarlet_chaplain;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_scarlet_wizard";
    newscript->GetAI = &GetAI_npc_scarlet_wizard;
    newscript->RegisterSelf();
}
