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
SDName: Teldrassil
SD%Complete: 100
SDComment: Quest support: 938
SDCategory: Teldrassil
EndScriptData */

/* ContentData
npc_mist
EndContentData */

#include "scriptPCH.h"

/*####
# npc_mist
####*/

enum
{
    SAY_AT_HOME             = -1000323,
    EMOTE_AT_HOME           = -1000324,
    QUEST_MIST              = 938,
    NPC_ARYNIA              = 3519,
    FACTION_DARNASSUS       = 79
};

struct npc_mistAI : public FollowerAI
{
    npc_mistAI(Creature* pCreature) : FollowerAI(pCreature)
    {
        Reset();
    }

    void Reset() { }

    void MoveInLineOfSight(Unit *pWho)
    {
        FollowerAI::MoveInLineOfSight(pWho);

        if (!m_creature->getVictim() && !HasFollowState(STATE_FOLLOW_COMPLETE) && pWho->GetEntry() == NPC_ARYNIA)
        {
            if (m_creature->IsWithinDistInMap(pWho, 10.0f))
            {
                DoScriptText(SAY_AT_HOME, pWho);
                DoComplete();
            }
        }
    }

    void DoComplete()
    {
        DoScriptText(EMOTE_AT_HOME, m_creature);

        if (Player* pPlayer = GetLeaderForFollower())
        {
            if (pPlayer->GetQuestStatus(QUEST_MIST) == QUEST_STATUS_INCOMPLETE)
                pPlayer->GroupEventHappens(QUEST_MIST, m_creature);
        }

        //The follow is over (and for later development, run off to the woods before really end)
        SetFollowComplete();
    }

    //call not needed here, no known abilities
    /*void UpdateFollowerAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        DoMeleeAttackIfReady();
    }*/
};

CreatureAI* GetAI_npc_mist(Creature* pCreature)
{
    return new npc_mistAI(pCreature);
}

struct npc_sethirAI : public ScriptedAI
{
    npc_sethirAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    void Reset()
    {
    }

    void Aggro(Unit* pUnit)
    {
        m_creature->MonsterSay("Filfh! Filfh everywhere! The forests must be cleansed!");
        for (uint32 counter = 0; counter < 6; counter++)
        {
            if (Creature* summoned = DoSpawnCreature(6911, 8.0f, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 1000))
                if (summoned->AI())
                    summoned->AI()->AttackStart(pUnit);
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        DoMeleeAttackIfReady();
    }
};


CreatureAI* GetAI_npc_sethir(Creature* pCreature)
{
    return new npc_sethirAI(pCreature);
}


bool QuestAccept_npc_mist(Player* pPlayer, Creature* pCreature, const Quest* pQuest)
{
    if (pQuest->GetQuestId() == QUEST_MIST)
    {
        if (npc_mistAI* pMistAI = dynamic_cast<npc_mistAI*>(pCreature->AI()))
        {
            pCreature->SetFactionTemporary(FACTION_DARNASSUS, TEMPFACTION_RESTORE_RESPAWN);
            pMistAI->StartFollow(pPlayer, FACTION_DARNASSUS, pQuest);
        } 
    }

    return true;
}

enum tyrSpells
{
    SPELL_STARFALL = 20687,
    SPELL_MOONFIRE = 20690,
    SPELL_SEARING_ARROW = 20688,
    SPELL_DODGE = 8273,
};

struct boss_tyrande_whisperwindAI : public ScriptedAI
{
public:
    boss_tyrande_whisperwindAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    void Reset() override
    {
        m_uiStarfallTimer = urand(10000, 15000);
        m_uiMoonfireTimer = 5000;
        m_uiSearingArrowTimer = 1500;
        m_uiEvadeCheck = 20000;
        DodgeTimer = 10000;
    }

    void JustRespawned() override
    {
        //V+ 5d buff for Racial Leaders
        if (m_creature->IsRacialLeader())
            m_creature->CastSpell(m_creature, 34746, true),
            m_creature->CastSpell(m_creature, 34741, true);
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiEvadeCheck <= uiDiff)
        {
            if (m_creature->GetPositionZ() < 1345.0f)
                EnterEvadeMode();
            m_uiEvadeCheck = 10000;
        }
        else
            m_uiEvadeCheck -= uiDiff;

        if (DodgeTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_DODGE) == CAST_OK)
                DodgeTimer = 10000;
        }
        else
            DodgeTimer -= uiDiff;

        if (m_uiStarfallTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_STARFALL) == CAST_OK)
                m_uiStarfallTimer = urand(15000, 20000);
        }
        else
            m_uiStarfallTimer -= uiDiff;

        if (m_uiMoonfireTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 1), SPELL_MOONFIRE) == CAST_OK)
                m_uiMoonfireTimer = urand(5000, 7000);
        }
        else
            m_uiMoonfireTimer -= uiDiff;

        if (m_uiSearingArrowTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->SelectAttackingTarget(ATTACKING_TARGET_BOTTOMAGGRO, 0), SPELL_SEARING_ARROW) == CAST_OK)
                m_uiSearingArrowTimer = urand(2000, 4000);
        }
        else
            m_uiSearingArrowTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }

private:
    uint32 m_uiStarfallTimer;
    uint32 m_uiMoonfireTimer;
    uint32 m_uiSearingArrowTimer;
    uint32 m_uiEvadeCheck;
    uint32 DodgeTimer;
};

CreatureAI* GetAI_boss_tyrande_whisperwind(Creature* pCreature)
{
    return new boss_tyrande_whisperwindAI(pCreature);
}


enum eSpells
{
    SPELL_WRATH         = 20698,
    SPELL_AE_ROOT       = 20699,
    SPELL_REJUVENATION  = 20701,
    SPELL_SUMM_TREANT   = 34752,
    SPELL_HURRICANE     = 27530,
};

struct boss_fandral_staghelmAI : public ScriptedAI
{
public:
    boss_fandral_staghelmAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    void Reset() override
    {
        m_uiWrathTimer = 0;
        m_uiRootTimer = 25000;
        m_uiRejuvenationTimer = 1500;
        m_uiTreantTimer = 15000;
        m_uiHurricaneTimer = 27000;
    }

    void JustRespawned() override
    {
        //V+ 5d buff for Racial Leaders
        if (m_creature->IsRacialLeader())
            m_creature->CastSpell(m_creature, 34746, true),
            m_creature->CastSpell(m_creature, 34741, true);
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiWrathTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_WRATH) == CAST_OK)
                m_uiWrathTimer = urand(1000, 1500);
        }
        else
            m_uiWrathTimer -= uiDiff;

        if (m_uiRootTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_AE_ROOT) == CAST_OK)
                m_uiRootTimer = 25000;
        }
        else
            m_uiRootTimer -= uiDiff;

        if (m_uiRejuvenationTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_REJUVENATION) == CAST_OK)
                m_uiRejuvenationTimer = 12000;
        }
        else
            m_uiRejuvenationTimer -= uiDiff;

        if (m_uiTreantTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_SUMM_TREANT) == CAST_OK)
                m_uiTreantTimer = 30000;
        }
        else
            m_uiTreantTimer -= uiDiff;

        if (m_uiHurricaneTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 1), SPELL_HURRICANE) == CAST_OK)
                m_uiHurricaneTimer = 30000;
        }
        else
            m_uiHurricaneTimer -= uiDiff;

        DoMeleeAttackIfReady();
        EnterEvadeIfOutOfCombatArea(uiDiff);
    }

private:
    uint32 m_uiWrathTimer;
    uint32 m_uiRootTimer;
    uint32 m_uiRejuvenationTimer;
    uint32 m_uiTreantTimer;
    uint32 m_uiHurricaneTimer;
};

CreatureAI* GetAI_boss_fandral_staghelm(Creature* pCreature)
{
    return new boss_fandral_staghelmAI(pCreature);
}
//Alita
enum
{
    QUEST_MORTALITY_WANES =  1142
};
bool QuestComplete_npc_treshala_fallowbrook(Player* pPlayer, Creature* pQuestGiver, Quest const* pQuest)
{
    if (!pQuestGiver)
            return false;
    if (pQuest->GetQuestId() == QUEST_MORTALITY_WANES)
    {
        pQuestGiver->HandleEmoteCommand(EMOTE_ONESHOT_CRY);
    }
    return false;//return false meaning let DB take over (probably)(nothing in db anyway)
}

void AddSC_teldrassil()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "npc_mist";
    newscript->GetAI = &GetAI_npc_mist;
    newscript->pQuestAcceptNPC = &QuestAccept_npc_mist;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_sethir";
    newscript->GetAI = &GetAI_npc_sethir;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "boss_tyrande_whisperwind";
    newscript->GetAI = &GetAI_boss_tyrande_whisperwind;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "boss_fandral_staghelm";
    newscript->GetAI = &GetAI_boss_fandral_staghelm;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_treshala_fallowbrook";
    newscript->pQuestRewardedNPC = &QuestComplete_npc_treshala_fallowbrook;
    newscript->RegisterSelf();
}
