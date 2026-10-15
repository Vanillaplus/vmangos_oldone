/* Copyright (C) 2006 - 2011 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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
SDName: boss_vael
SD%Complete: 75
SDComment: Burning Adrenaline not correctly implemented in core
SDCategory: Blackwing Lair
EndScriptData */

#include "scriptPCH.h"
#include "blackwing_lair.h"

enum
{
    SAY_RUN                 = -1900260,
    SAY_AGGRO               = -1900261,
    SAY_FIRE_DETECTED       = -1900262,
    SAY_EVADED              = -1900263,
    SAY_ENRAGE              = -1900264,
    SAY_KILL                = -1900265,

    NPC_BOSS                = 25100,
    NPC_SAWBLADE1           = 25103,
    NPC_SAWBLADE2           = 25104,
    NPC_SAWBLADE3           = 25105,
    NPC_PRE_SAWBLADE1       = 25106,
    NPC_PRE_SAWBLADE2       = 25107,
    NPC_PRE_SAWBLADE3       = 25108,
    NPC_SAWBLADE_MAIN       = 25109,
    NPC_BARREL              = 25102,
    NPC_OIL_FIRES           = 25110,
    NPC_BARREL_EXPLOSION    = 25111,
    NPC_BARREL_HITVISUAL    = 25112,
    NPC_BARREL_AURA         = 25113,
    NPC_FIREFIGHTER_VISUAL  = 25114,
    NPC_GOBLIN_LEFT         = 25115,
    NPC_GOBLIN_RIGHT        = 25118,
    NPC_POS_LEFT            = 25116,
    NPC_POS_RIGHT           = 25117,
    GO_BARREL               = 302100,
    GO_BARREL_LOS           = 302101,
    GO_BARREL_FIRE          = 178443,

    SPELL_OIL_TAR           = 35135,
    SPELL_GRENADES          = 35136,
    /*SPELL_MAGNETIC_GRIP   = 35158,*/
    SPELL_COKE_BREATH       = 35146,
    SPELL_FIREFIGHTER       = 35147,
    SPELL_ENRAGE            = 35150,

};

struct boss_shredderAI : public ScriptedAI
{
    boss_shredderAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 saw_timer;
    uint32 saw_back_timer;
    uint32 saw_back2_timer;
    uint32 saw_back3_timer;
    uint32 saw_back4_timer;
    uint32 oil_timer;
    /*uint32 aoe_timer;*/
    uint32 fire_aura_check_timer;
    uint32 frozentimer;
    uint32 FireFighterSystemIniTimer;
    uint32 FireFighterStartTimer;;
    uint32 EnrageTimer;
    uint32 FirethrowTimer;
    uint32 RangeCheckTimer;
    /*uint32 RestoreTargetTimer;*/

    bool SpottedFire;
    bool Frozen;
    bool enraged;


    Unit* pTarget1 = NULL;
    Unit* pTarget2 = NULL;
    Unit* pTarget3 = NULL;
    Unit* FireFigherNPCclone = NULL;

    void Reset()
    {
        saw_timer = 5000;
        saw_back_timer = 60000;
        saw_back2_timer = 60000;
        saw_back3_timer = 60000;
        saw_back4_timer = 60000;
        oil_timer = 15000;
        /*aoe_timer = 18000;*/
        fire_aura_check_timer = 2000;
        RangeCheckTimer = 10000;
        FirethrowTimer = urand(20000, 30000);
        /*RestoreTargetTimer = 0;*/
        FireFighterSystemIniTimer = 10 * MINUTE * IN_MILLISECONDS;
        FireFighterStartTimer = 10 * MINUTE * IN_MILLISECONDS;
        frozentimer = 10 * MINUTE * IN_MILLISECONDS;
        EnrageTimer = 5 * MINUTE * IN_MILLISECONDS;
        SpottedFire = false;
        Frozen = false;
        enraged = false;

        FireFigherNPCclone = NULL;
    }

    void Aggro(Unit* /*pWho*/)
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_VAELASTRASZ, IN_PROGRESS);

        if (m_pInstance->GetData(TYPE_RAZORGORE) != DONE)
        {
            EnterEvadeMode();
            return;
        }

        m_creature->SetInCombatWithZone();
        DoScriptText(SAY_AGGRO, m_creature);


        /*std::list<Creature*> barrels;
        GetCreatureListWithEntryInGrid(barrels, m_creature, NPC_BARREL, DEFAULT_VISIBILITY_INSTANCE);

        if (!barrels.empty())
        {
            for (auto iter = barrels.cbegin(); iter != barrels.cend(); ++iter)
            {
                auto barrel = *iter;
                if (barrel)
                {
                    if (!barrel->isAlive())
                    {
                        int displayID = 25078;
                        if (urand(0, 1) > 0)
                            displayID = 25079;

                        barrel->SetDisplayId(displayID);
                        barrel->Respawn();
                    }
                }
            }
        }*/
    }

    void KilledUnit(Unit* pVictim)
    {
        if (!pVictim || pVictim->GetTypeId() != TYPEID_PLAYER)
            return;

        if (m_creature->HasAura(34821))
            return;

        m_creature->CastSpell(m_creature, 34821, true);
        DoScriptText(SAY_KILL, m_creature);
    }

    void JustDied(Unit* /*pKiller*/)
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_VAELASTRASZ, DONE);

        //Despawn Saws and barrels
        std::list<Creature*> SawList;
        GetCreatureListWithEntryInGrid(SawList, m_creature, NPC_SAWBLADE_MAIN, 150.0f);
        GetCreatureListWithEntryInGrid(SawList, m_creature, NPC_BARREL, 150.0f);
        GetCreatureListWithEntryInGrid(SawList, m_creature, NPC_BARREL_AURA, 150.0f);
        for (auto it = SawList.cbegin(); it != SawList.cend(); ++it)
        {
            (*it)->SetDisplayId(11686);
            (*it)->ForcedDespawn();
        }
        
        float RangeL = 0.0f;
        float RangeR = 0.0f;
        uint32 GOBLIN_ID = NPC_GOBLIN_LEFT;

        std::list<Creature*> LeftList;
        GetCreatureListWithEntryInGrid(LeftList, m_creature, NPC_POS_LEFT, 150.0f);
        for (auto it = LeftList.cbegin(); it != LeftList.cend(); ++it)
        {
            RangeL = (*it)->GetDistance(m_creature);
        }
        std::list<Creature*> RightList;
        GetCreatureListWithEntryInGrid(RightList, m_creature, NPC_POS_RIGHT, 150.0f);
        for (auto it = RightList.cbegin(); it != RightList.cend(); ++it)
        {
            RangeR = (*it)->GetDistance(m_creature);
        }

        if (RangeL > RangeR)
            GOBLIN_ID = NPC_GOBLIN_RIGHT;

        if (Unit* GoblinNPCrunner = m_creature->SummonCreature(GOBLIN_ID, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), m_creature->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 40000))
        {
            GoblinNPCrunner->SetWalk(false);
            DoScriptText(SAY_RUN, GoblinNPCrunner);
            GoblinNPCrunner->GetMotionMaster()->MovePoint(0, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), MOVE_RUN_MODE);
            GoblinNPCrunner->CastSpell(GoblinNPCrunner, 35163, true);
        }



        std::list<GameObject*> listbarrel;
        GetGameObjectListWithEntryInGrid(listbarrel, m_creature, GO_BARREL, 150.0f);
        GetGameObjectListWithEntryInGrid(listbarrel, m_creature, GO_BARREL_FIRE, 150.0f);
        /*GetGameObjectListWithEntryInGrid(listbarrel, m_creature, GO_BARREL_LOS, 11.0f);*/
        for (std::list<GameObject*>::const_iterator itr = listbarrel.begin(); itr != listbarrel.end(); ++itr)
        {
            if (listbarrel.empty())
                break;
            if (!(*itr)->isSpawned())
                continue;

            (*itr)->Despawn();
        }
    }

    void JustReachedHome()
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_VAELASTRASZ, FAIL);

        DoScriptText(SAY_EVADED, m_creature);

        std::list<Creature*> barrelAuras;
        GetCreatureListWithEntryInGrid(barrelAuras, m_creature, NPC_BARREL_AURA, 150.0f);
        for (std::list<Creature*>::iterator itr = barrelAuras.begin(); itr != barrelAuras.end(); ++itr)
            (*itr)->DeleteLater();
        /*(*itr)->ForcedDespawn();*/

        DoSawBack();

        std::list<Creature*> barrels;
        GetCreatureListWithEntryInGrid(barrels, m_creature, NPC_BARREL, 150.0f);

        if (!barrels.empty())
        {
            for (auto iter = barrels.cbegin(); iter != barrels.cend(); ++iter)
            {
                auto barrel = *iter;
                if (barrel)
                {
                    if (!barrel->isAlive())
                    {
                        int displayID = 25078;
                        if (urand(0, 1) > 0)
                            displayID = 25079;

                        barrel->SetDisplayId(displayID);
                        barrel->Respawn();
                    }

                    if (barrel->HasAura(35143))
                        barrel->RemoveAurasDueToSpell(35143);
                    if (barrel->HasAura(35156))
                        barrel->RemoveAurasDueToSpell(35156);
                    if (barrel->HasAura(35157))
                        barrel->RemoveAurasDueToSpell(35157);
                }
            }
        }

        std::list<GameObject*> barrel_fire_list;
        GetGameObjectListWithEntryInGrid(barrel_fire_list, m_creature, GO_BARREL_FIRE, 150.0f);
        for (std::list<GameObject*>::const_iterator itr = barrel_fire_list.begin(); itr != barrel_fire_list.end(); ++itr)
        {
            if (barrel_fire_list.empty())
                break;
            if (!(*itr)->isSpawned())
                continue;

            (*itr)->Despawn();
        }
    }

    void SpellHit(Unit* pWho, const SpellEntry* pSpell)
    {
        if (!SpottedFire)
        {
            if (pSpell->Id == 35144)
            {
                SpottedFire = true;
                FireFighterSystemIniTimer = urand(1500, 3000);
            }
        }
    }

    void DoSawAttack()
    {
        pTarget1 = m_creature->getVictim();

        int32 NumberOfPlayers = 0;
        Map::PlayerList const& players = m_creature->GetMap()->GetPlayers();
        if (players.isEmpty())
            return;
        for (Map::PlayerList::const_iterator iter = players.begin(); iter != players.end(); ++iter)
        {
            if (Player* pTempTarget = iter->getSource())
                if (pTempTarget->isAlive() && !pTempTarget->IsGameMaster() && pTempTarget->isInCombat())
                    NumberOfPlayers += 1;
        }

        if (NumberOfPlayers > 3)
        {
            int attempt = 0;
            while (attempt < 300)
            {
                pTarget1 = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, nullptr, SELECT_FLAG_PLAYER);
                pTarget2 = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, nullptr, SELECT_FLAG_PLAYER);
                pTarget3 = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, nullptr, SELECT_FLAG_PLAYER);

                if (pTarget1 != pTarget2 && pTarget1 != pTarget3 && pTarget2 != pTarget3)
                {
                    attempt += 1;

                    if (pTarget1->GetDistance2d(-7482.5f, -1017.8f) < 70.0f && pTarget2->GetDistance2d(-7482.5f, -1017.8f) < 70.0f && pTarget3->GetDistance2d(-7482.5f, -1017.8f) < 70.0f)
                    {
                        if (m_creature->GetDistance(pTarget1) > 20.0f && m_creature->GetDistance(pTarget2) > 20.0f && m_creature->GetDistance(pTarget3) > 20.0f)
                        {
                            attempt += 20;
                            if (pTarget1->GetDistance(pTarget2) > 20.0f && pTarget1->GetDistance(pTarget3) > 20.0f && pTarget2->GetDistance(pTarget3) > 20.0f)
                                attempt += 300;
                        }
                    }
                    /*if (!pTarget1->HasAura(SPELL_MAGNETIC_GRIP) && !pTarget2->HasAura(SPELL_MAGNETIC_GRIP) && !pTarget3->HasAura(SPELL_MAGNETIC_GRIP))*/
                }

            }
            if (pTarget1)
                if (Creature* PreSawNPC1 = m_creature->SummonCreature(NPC_PRE_SAWBLADE1, pTarget1->GetPositionX(), pTarget1->GetPositionY(), pTarget1->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 8000))
                {
                    if (Creature* SawNPC1 = m_creature->SummonCreature(NPC_SAWBLADE1, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ() + 4.0f, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 8000))
                        SawNPC1->GetMotionMaster()->MovePoint(1, pTarget1->GetPositionX(), pTarget1->GetPositionY(), pTarget1->GetPositionZ() + 3.0f);
                }
            if (pTarget2)
                if (Creature* PreSawNPC2 = m_creature->SummonCreature(NPC_PRE_SAWBLADE2, pTarget2->GetPositionX(), pTarget2->GetPositionY(), pTarget2->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 8000))
                {
                    if (Creature* SawNPC2 = m_creature->SummonCreature(NPC_SAWBLADE2, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ() + 4.0f, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 8000))
                        SawNPC2->GetMotionMaster()->MovePoint(1, pTarget2->GetPositionX(), pTarget2->GetPositionY(), pTarget2->GetPositionZ() + 3.0f);
                }
            if (pTarget3)
                if (Creature* PreSawNPC3 = m_creature->SummonCreature(NPC_PRE_SAWBLADE3, pTarget3->GetPositionX(), pTarget3->GetPositionY(), pTarget3->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 8000))
                {
                    if (Creature* SawNPC3 = m_creature->SummonCreature(NPC_SAWBLADE3, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ() + 4.0f, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 8000))
                        SawNPC3->GetMotionMaster()->MovePoint(1, pTarget3->GetPositionX(), pTarget3->GetPositionY(), pTarget3->GetPositionZ() + 3.0f);
                }
        }
        else
            if (Creature* PreSawNPC1 = m_creature->SummonCreature(NPC_PRE_SAWBLADE1, pTarget1->GetPositionX(), pTarget1->GetPositionY(), pTarget1->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 8000))
            {
                if (Creature* SawNPC1 = m_creature->SummonCreature(NPC_SAWBLADE1, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ() + 4.0f, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 8000))
                    SawNPC1->GetMotionMaster()->MovePoint(1, pTarget1->GetPositionX(), pTarget1->GetPositionY(), pTarget1->GetPositionZ() + 3.0f);
            }
        
        m_creature->CastSpell(m_creature, 35141, true);
        m_creature->CastSpell(m_creature, 35154, true);
    }

    void DoSawBack()
    {
        std::list<Creature*> SawList;
        GetCreatureListWithEntryInGrid(SawList, m_creature, NPC_SAWBLADE_MAIN, 150.0f);
        for (auto it = SawList.cbegin(); it != SawList.cend(); ++it)
        {
            (*it)->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            (*it)->GetMotionMaster()->MovePoint(1, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ() + 4.0f);
        }
    }

    void DoSawBackF()
    {
        std::list<Creature*> SawList;
        GetCreatureListWithEntryInGrid(SawList, m_creature, NPC_SAWBLADE_MAIN, 150.0f);
        for (auto it = SawList.cbegin(); it != SawList.cend(); ++it)
        {
            (*it)->GetMotionMaster()->MoveFollow(m_creature, 0, 0);
        }
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        //Evade_Check
        if (RangeCheckTimer < uiDiff)
        {
            RangeCheckTimer = 3000;
            if (m_creature->GetDistance2d(-7482.5f, -1017.8f) > 70.0f)
            {
                EnterEvadeMode();
                return;
            }
        }
        else
            RangeCheckTimer -= uiDiff;

        //Fire_Check
        if (fire_aura_check_timer < uiDiff)
        {
            fire_aura_check_timer = 2000;
            if (!SpottedFire)
            {
                if (m_creature->HasAura(35161))
                {
                    SpottedFire = true;
                    FireFighterSystemIniTimer = urand(1500, 2500);
                }
            }
        }
        else
            fire_aura_check_timer -= uiDiff;

        //Enrage
        if (EnrageTimer < uiDiff)
        {
            if (!enraged)
                DoScriptText(SAY_ENRAGE, m_creature);

            m_creature->CastSpell(m_creature, SPELL_ENRAGE, true);
            EnrageTimer = 5000;
            enraged = true;
        }
        else
            EnrageTimer -= uiDiff;

        if (FirethrowTimer < uiDiff)
        {
            if (!enraged)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_COKE_BREATH) == CAST_OK)
                {
                    /*m_creature->SetInFront(fTarget);
                    m_creature->SetTargetGuid(fTarget->GetObjectGuid());
                    RestoreTargetTimer = 5000;
                    m_creature->CastSpell(fTarget, SPELL_COKE_BREATH, false);*/
                    FirethrowTimer = 45000;
                }
            }
            else
                FirethrowTimer = 45000;

        }
        else
            FirethrowTimer -= uiDiff;

        // Restore original target after casting Coke Ejectection
        /*if (RestoreTargetTimer)
        {
            if (RestoreTargetTimer <= uiDiff)
            {
                if (Unit* pTarget = m_creature->getVictim())
                {
                    m_creature->SetInFront(pTarget);
                    m_creature->SetTargetGuid(pTarget->GetObjectGuid());
                    RestoreTargetTimer = 0;
                }
            }
            else
                RestoreTargetTimer -= uiDiff;
        }*/

        //Frozen Timers
        //Few seconds after taken Oil damage, stuns for 4 sec and awaits for Firefighter
        if (FireFighterSystemIniTimer < uiDiff)
        {
            DoScriptText(SAY_FIRE_DETECTED, m_creature);
            m_creature->CastSpell(m_creature, 35162, true);
            FireFigherNPCclone = m_creature->SummonCreature(NPC_FIREFIGHTER_VISUAL, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), (m_creature->GetOrientation() + 3.14f), TEMPSUMMON_TIMED_DESPAWN, 5000);
            FireFighterSystemIniTimer = 10 * MINUTE * IN_MILLISECONDS;
            FireFighterStartTimer = 3000;

            if (FirethrowTimer < 5000)
                FirethrowTimer += 50000;
        }
        else
            FireFighterSystemIniTimer -= uiDiff;

        if (FireFighterStartTimer < uiDiff)
        {
            m_creature->CastSpell(m_creature, SPELL_FIREFIGHTER, true);
            FireFighterStartTimer = 10 * MINUTE * IN_MILLISECONDS;
            frozentimer = 60000;
            Frozen = true;
            if (FireFigherNPCclone)
                FireFigherNPCclone->CastSpell(FireFigherNPCclone, SPELL_FIREFIGHTER, true);
        }
        else
            FireFighterStartTimer -= uiDiff;

        if (frozentimer < uiDiff)
        {
            SpottedFire = false;
            Frozen = false;
            frozentimer = 10 * MINUTE * IN_MILLISECONDS;
        }
        else
            frozentimer -= uiDiff;

        //Oil+Grenades Timer
        if (oil_timer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_OIL_TAR) == CAST_OK)
            {
                m_creature->CastSpell(m_creature, SPELL_GRENADES, true);
                if (Frozen)
                    oil_timer = 7500;
                else
                    oil_timer = 5000;
            }
        }
        else
            oil_timer -= uiDiff;

        //AoE grenades Timer
        /*if (aoe_timer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_GRENADES) == CAST_OK)
            {
                if (Frozen)
                    aoe_timer = 15000;
                else
                    aoe_timer = 5000;
            }
        }
        else
            aoe_timer -= uiDiff;*/

        // Saw Blade Timers
        if (saw_timer < uiDiff)
        {
            DoSawAttack();

            saw_timer = urand(25000, 35000);
            saw_back_timer = 20000;
            saw_back2_timer = 20750;
            saw_back3_timer = 21500;
            saw_back4_timer = 22000;
        }
        else
            saw_timer -= uiDiff;
        if (saw_back_timer < uiDiff)
        {
            DoSawBack();
            saw_back_timer = 60000;
        }
        else
            saw_back_timer -= uiDiff;
        if (saw_back2_timer < uiDiff)
        {
            DoSawBack();
            saw_back2_timer = 60000;
        }
        else
            saw_back2_timer -= uiDiff;
        if (saw_back3_timer < uiDiff)
        {
            DoSawBack();
            saw_back3_timer = 60000;
        }
        else
            saw_back3_timer -= uiDiff;
        if (saw_back4_timer < uiDiff)
        {
            DoSawBackF();
            saw_back4_timer = 60000;
        }
        else
            saw_back4_timer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};
CreatureAI* GetAI_boss_shredder(Creature* pCreature)
{
    return new boss_shredderAI(pCreature);
}

struct npc_sawbladeAI : public ScriptedAI
{
    npc_sawbladeAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }
    bool spawned = false;
    bool hasaura = false;

    void Reset() { }

    void UpdateAI(const uint32 uiDiff)
    {
        if (m_creature->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP))
            return;

        int32 preNPCid = 0;
        int32 NPCid = 0;

        switch (m_creature->GetEntry())
        {
        case NPC_SAWBLADE1:
            preNPCid = NPC_PRE_SAWBLADE1;
            NPCid = NPC_SAWBLADE1;
            break;
        case NPC_SAWBLADE2:
            preNPCid = NPC_PRE_SAWBLADE2;
            NPCid = NPC_SAWBLADE2;
            break;
        case NPC_SAWBLADE3:
            preNPCid = NPC_PRE_SAWBLADE3;
            NPCid = NPC_SAWBLADE3;
            break;
        case NPC_SAWBLADE_MAIN:
            preNPCid = NPC_BOSS;
            NPCid = NPC_SAWBLADE_MAIN;
            break;
        }

        std::list<Creature*> PreSawList;
        GetCreatureListWithEntryInGrid(PreSawList, m_creature, preNPCid, 150.0f);

        for (std::list<Creature*>::iterator it = PreSawList.begin(); it != PreSawList.end(); ++it)
        {
            if (NPCid == NPC_SAWBLADE1 || NPCid == NPC_SAWBLADE2 || NPCid == NPC_SAWBLADE3)
            {
                //Apply aura 
                if (!hasaura)
                {
                    std::list<Creature*> BossList;
                    GetCreatureListWithEntryInGrid(BossList, m_creature, NPC_BOSS, 50.0f);

                    for (std::list<Creature*>::iterator i = BossList.begin(); i != BossList.end(); ++i)
                        if ((*i)->GetDistance(m_creature) > 5.0f)
                        {
                            hasaura = true;
                            m_creature->AddAura(35137);
                        }
                }
                //spawn main sawblade
                if (!spawned)
                {
                    if ((*it)->GetDistance(m_creature) < 5.0f)
                    {
                        if (Creature* NewSaw = m_creature->SummonCreature(NPC_SAWBLADE_MAIN, (*it)->GetPositionX(), (*it)->GetPositionY(), (*it)->GetPositionZ() + frand(2.2f, 2.8f), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 35000))
                        {
                            spawned = true;

                            m_creature->SetDisplayId(11686);
                            m_creature->ForcedDespawn();
                        }
                    }
                }
            }
            else
            {
                //Despawn main sawblade at boss
                if ((*it)->GetDistance(m_creature) < 5.0f)
                {
                    m_creature->SetDisplayId(11686);
                    m_creature->ForcedDespawn();
                    (*it)->CastSpell((*it), 35141, true);
                }
            }
        }
    }
};
CreatureAI* GetAI_npc_sawblade(Creature* pCreature)
{
    return new npc_sawbladeAI(pCreature);
}



struct npc_oildbarrelAI : public ScriptedAI
{
    npc_oildbarrelAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiCombatTimer;
    uint32 explosiontimer;
    bool preexplosion;
    bool explosion;
    uint32 CollisionTimer;
    uint32 StealthTimer;
    std::unordered_map<ObjectGuid, time_t> attackers;

    void Reset() override
    {
        m_uiCombatTimer = 15000;
        CollisionTimer = 500;
        StealthTimer = 500;
        explosiontimer = 14 * DAY * IN_MILLISECONDS;
        preexplosion = false;
        explosion = false;
        attackers.clear();
    }

    void AttackStart(Unit* /*pWho*/) override {}

    void Aggro(Unit* pWho) override
    {
        SetCombatMovement(false);
    }

    void AddAttackerToList(Unit* pWho)
    {
        auto itr = attackers.find(pWho->GetObjectGuid());
        if (itr != attackers.end())
        {
            itr->second = std::time(nullptr);
        }
        else
        {
            attackers.emplace(pWho->GetObjectGuid(), std::time(nullptr));
        }
    }

    void DamageTaken(Unit* pWho, uint32& /*uiDamage*/)
    {
        if (pWho)
            AddAttackerToList(pWho);
    }

    void SpellHit(Unit* pWho, const SpellEntry* pSpell)
    {
        if (pWho)
            AddAttackerToList(pWho);

        if (!preexplosion)
            if (pSpell->School == SPELL_SCHOOL_FIRE)
            {
                uint32 CastTime = pSpell->GetCastTime();
                uint32 Chance = 10;
                {
                    if (CastTime < 1501)
                        Chance = 8;
                    else if (CastTime < 2501)
                        Chance = 6;
                    else if (CastTime < 3501)
                        Chance = 4;
                    else if (CastTime > 3999)
                        Chance = 0;

                    //Heavy Thorium Grenade
                    if (pSpell->Id == 35155)
                        Chance = 5;
                }
                if (urand(0, 9) >= Chance)
                {
                    explosiontimer = 5000;
                    explosion = true;
                    preexplosion = true;
                    m_creature->CastSpell(m_creature, 35143, true);
                }
                //Barrel explosion
                if (pSpell->Id == 35144)
                {
                    explosiontimer = urand(200, 800);
                    explosion = true;
                    preexplosion = true;
                    m_creature->CastSpell(m_creature, 35156, true);
                }
                //Coke Ejection
                if (pSpell->Id == 35146 || pSpell->Id == 35149)
                {
                    explosiontimer = 2000;
                    explosion = true;
                    preexplosion = true;
                    m_creature->CastSpell(m_creature, 35157, true);
                }

                BarrelCast(35159);
                /*HitTargetNPC->CastSpell(HitTargetNPC, 35159, true);*/

                /*std::list<Creature*> HitAnimList;
                GetCreatureListWithEntryInGrid(HitAnimList, m_creature, NPC_BARREL_HITVISUAL, 1.0f);

                for (std::list<Creature*>::iterator it = HitAnimList.begin(); it != HitAnimList.end(); ++it)
                {
                    if ((*it)->GetDistance(m_creature) == 0.0f)
                        (*it)->CastSpell((*it), 35159, true);
                }*/
            }
    }

    void JustDied(Unit* /*pKiller*/)
    {
        if (!m_creature->HasAura(35143) || !m_creature->HasAura(35156) || !m_creature->HasAura(35157))
            BarrelCast(35160);
            /*HitTargetNPC->CastSpell(HitTargetNPC, 35160, true);*/


        std::list<GameObject*> listbarrel;
        GetGameObjectListWithEntryInGrid(listbarrel, m_creature, GO_BARREL, 1.0f);
        GetGameObjectListWithEntryInGrid(listbarrel, m_creature, GO_BARREL_FIRE, 1.0f);
        /*GetGameObjectListWithEntryInGrid(listbarrel, m_creature, GO_BARREL_LOS, 11.0f);*/
        for (std::list<GameObject*>::const_iterator itr = listbarrel.begin(); itr != listbarrel.end(); ++itr)
        {
            if (listbarrel.empty())
                break;
            /*if (!(*itr)->isSpawned())
                continue;*/

            if (m_creature->GetDistanceToCenter((*itr)) < 1.0f)
                (*itr)->Despawn();

            /*if ((*itr)->GetEntry() == GO_BARREL_FIRE)
            {
                (*itr)->Despawn();
            }
            else if (m_creature->GetDistance2d((*itr)) == 0.0f)
                (*itr)->Despawn();*/
        }
        listbarrel.clear();

        m_creature->SetDisplayId(11686);
        m_creature->ForcedDespawn();
        /*Creature* creatureToDespawn = (Creature*)HitTargetNPC;
        creatureToDespawn->ForcedDespawn();*/
        BarrelCast(0);
    }

    void BarrelCast(uint32 Spell)
    {
        std::list<Creature*> visual;
        GetCreatureListWithEntryInGrid(visual, m_creature, NPC_BARREL_HITVISUAL, 1.0f);
        for (std::list<Creature*>::iterator itr = visual.begin(); itr != visual.end(); ++itr)
        {
            if ((*itr)->GetDistanceToCenter(m_creature) < 1.0f)
            {
                if (Spell == 0)
                {
                    (*itr)->AddAura(35210);
                    return;
                }
                (*itr)->CastSpell((*itr), Spell, true);
                return;
            }
        }
    }

    void UpdateAI(const uint32 diff) override
    {
        if (StealthTimer < diff)
        {
            StealthTimer = 3000;
            //Stealth
            if (!m_creature->HasAura(35142))
                m_creature->AddAura(35142);
        }
        else
            StealthTimer -= diff;

        if (CollisionTimer < diff)
        {
            m_creature->SummonCreature(NPC_BARREL_HITVISUAL, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), m_creature->GetOrientation(), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 5 * MINUTE * IN_MILLISECONDS);
            m_creature->CastSpell(m_creature, 35145, true);
            CollisionTimer = 5 * MINUTE * IN_MILLISECONDS;
        }
        else
            CollisionTimer -= diff;
            

        if (explosion)
        {
            if (explosiontimer < diff)
            {
                explosiontimer = 14 * DAY * IN_MILLISECONDS;
                explosion = false;
                if (m_creature->HasAura(35143) || m_creature->HasAura(35156) || m_creature->HasAura(35157))
                {
                    std::list<GameObject*> listbarrel;
                    GetGameObjectListWithEntryInGrid(listbarrel, m_creature, GO_BARREL, 1.0f);
                    GetGameObjectListWithEntryInGrid(listbarrel, m_creature, GO_BARREL_FIRE, 1.0f);
                    for (std::list<GameObject*>::const_iterator itr = listbarrel.begin(); itr != listbarrel.end(); ++itr)
                    {
                        if (listbarrel.empty())
                            break;
                        if (!(*itr)->isSpawned())
                            continue;

                        if (m_creature->GetDistance2d((*itr)) == 0.0f)
                            (*itr)->Despawn();
                    }
                    listbarrel.clear();
                    BarrelCast(0);

                    m_creature->CastSpell(m_creature, 35144, true);
                    m_creature->SummonCreature(NPC_OIL_FIRES, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), m_creature->GetOrientation(), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 8000);
                    m_creature->SummonCreature(NPC_BARREL_EXPLOSION, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), m_creature->GetOrientation(), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 3000);
                    m_creature->SummonCreature(NPC_BARREL_AURA, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), m_creature->GetOrientation(), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 8000);
                    m_creature->SetDisplayId(11686);
                    m_creature->ForcedDespawn();
                }
            }
            else
                explosiontimer -= diff;
        }

        if (m_creature->isInCombat())
        {
            if (m_uiCombatTimer <= diff)
            {
                for (auto itr = attackers.begin(); itr != attackers.end();)
                {
                    Unit* pAttacker = m_creature->GetMap()->GetUnit(itr->first);

                    if (!pAttacker || !pAttacker->IsInWorld())
                    {
                        itr = attackers.erase(itr);
                        continue;
                    }
                    if (itr->second + 10 < std::time(nullptr))
                    {
                        m_creature->_removeAttacker(pAttacker);
                        m_creature->getThreatManager().modifyThreatPercent(pAttacker, -101.0f);
                        itr = attackers.erase(itr);
                        continue;
                    }
                    ++itr;
                }

                if (m_creature->getThreatManager().isThreatListEmpty())
                    EnterEvadeMode();

                m_uiCombatTimer = 15000;
            }
            else
                m_uiCombatTimer -= diff;
        }
    }
};
CreatureAI* GetAI_npc_oilbarrel(Creature* pCreature)
{
    return new npc_oildbarrelAI(pCreature);
}

enum
{
    // Last found data : HP : 3,331,000 (+20% = 3997200). Damage: 5,930 - 6,807. Armor: 4,691.
    NPC_VAELASTRAZ              = 13020,
    NPC_LORD_NEFARIAN_VAEL      = 10163,

    // Emotes
    // ------

    SAY_LINE_1                  = -1469026,
    SAY_LINE_2                  = -1469027,
    SAY_LINE_3                  = -1469028,
    SAY_HALFLIFE                = -1469029,
    SAY_KILLTARGET              = -1469030,
    SAY_NEFARIUS_CORRUPT_1      = -1469006,                 // When he corrupts Vaelastrasz
    SAY_NEFARIUS_CORRUPT_2      = -1469037,
    // Spells
    // ------

    // OK
    // This debuff is cast on the raid at the beginning of the encounter and lasts for 3 minutes. Restores 500 Mana per second to mana users. Restores 50 Energy per second to Rogues and Cat Form Druids. Generates 20 Rage per second for Warriors and Bear Form Druids. It essentially means players have infinite mana/rage/energy/runic power for the fight. It is not dispellable, but can be removed by Ice Block or Divine Shield (which is not advised because of it buffing a players damage at such an incredible rate).
    SPELL_ESSENCE_OF_THE_RED    = 23513,

    // OK
    // Inflicts 3063 to 3937 (3500 to 4500?) Fire damage to enemies in a cone in front of the caster. Every flame breath applies a stacking debuff (also called flame breath) that ticks for 1000ish fire damage every few seconds. Its maximum level, which is always reached by the time the MT is burning, ticks for 4600 damage. This debuff plays a huge role. Early in the rotation, the MT takes only 1200dps or so. By the time they are burning, they're taking more like 2000dps. The difference is all in this attack.
    SPELL_FLAME_BREATH          = 23461,

    // OK
    // Inflicts 555-645 Fire damage to nearby enemies.
    SPELL_FIRE_NOVA             = 23462,

    // OK
    // Inflicts 925 to 1075 (600 à 700) damage on enemies in a cone behind the caster, knocking them back.
    SPELL_TAIL_SWEEP            = 15847,

    // TO FIX : explode at the end
    // Damage done increased by 100%. Attack speed increased by 100%. Spells become instant cast. Reduces max health by 5% every second; eventually causes player to die. Vael only casts Burning Adrenaline in two scenarios: He will cast it on random mana users every 15 seconds and he will cast it on the current tank every 45 seconds. When mana-using raid members are afflicted, they must move away to a designated area or bomb the raid with a 4376 to 5624 AoE blast. Non mana users will not be afflicted unless they are currently tanking Vaelastrasz. Note that ice blocks or divine shield will not prevent this debuff from killing the player.
    SPELL_BURNING_ADRENALINE    = 23620, // 18173 ?
    SPELL_BURNING_ADRENALINE2   = 23478, // AOE
    SPELL_BURNING_ADRENALINE3   = 23644, // instakill
    SPELL_BURNING_ADRENALINE4   = 24701, // -75% threat : easy mode ???
    // See the heal of Kazzak

    // TO FIX : The chain effect
    // DB : spell_effect_mod.effectChainTarget = 25;
    // Cleave attack that hits for 2k. This is a chain cleave, so if positioning is poor it can chain to the entire raid, even to behind him. It is critical that nobody be within approximately 10 yards of the main tank for this reason. Offtanks should be generating threat far enough from the MT to avoid chaining the cleave, but close enough to slide into place when BA hits the MT.
    SPELL_CLEAVE                = 22540, // 22540 Chain Cleave hardcoded via the BD (Ustaag) // 19983 // 20684  ? // Chain cleave is most likely named something different and contains a dummy effect
    // change spell.cpp "//FIXME: This very like horrible hack and wrong for most spells"
    // Find out which creatures use both spells in the DB?

    SPELL_BANISHEMENT_OF_SCALE  = 16404,
    SPELL_NEFARIUS_CORRUPTION   = 23642,

    FACTION_BLACK_DRAGON        = 103,
    FACTION_FRIENDLY            = 35,

    MODEL_INVISIBLE             = 11686,

    GOSSIP_TEXT_VAEL_1          = 7156,
    GOSSIP_TEXT_VAEL_2          = 7256,

    QUEST_NEFARIUS_CORRUPTION   = 8730
};

// Coords used to spawn Nefarius at the throne
//static const float aNefariusSpawnLoc[4] = { -7466.16f, -1040.80f, 412.053f, 2.14675f};
//
//#define GOSSIP_ITEM_VAEL_1         "I cannot, Vaelastrasz! Surely something can be done to heal you!"
//#define GOSSIP_ITEM_VAEL_2         "Vaelastrasz, no!!!"

//struct boss_vaelAI : public ScriptedAI
//{
//    boss_vaelAI(Creature* pCreature) : ScriptedAI(pCreature)
//    {
//        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
//        if (m_pInstance)
//            m_bIntroEvent = m_pInstance->GetData(TYPE_VAEL_EVENT) == DONE;
//        else
//            m_bIntroEvent = false;
//        m_bEngaged = false;
//        Reset();
//    }
//
//    ScriptedInstance* m_pInstance;
//
//    uint32 m_uiSpeechTimer;
//    uint32 m_uiSpeechNum;
//    uint32 m_uiCleaveTimer;
//    uint32 m_uiFlameBreathTimer;
//    uint32 m_uiFireNovaTimer;
//    uint32 m_uiBurningAdrenalineCasterTimer;
//    uint32 m_uiBurningAdrenalineTankTimer;
//    uint32 m_uiTailSweepTimer;
//    uint32 m_uiSelectableTimer;
//    uint32 m_uiInitTimer;
//    uint32 m_uiIntroTimer;
//    uint32 m_uiIntroPhase;
//    bool m_bIntroEvent;
//    bool m_bHasYelled;
//    bool m_bIsDoingSpeech;
//    bool m_bCastedEssenceOfTheRed;
//    bool m_bCastedbanishment;
//    bool m_bFlagSet;
//    bool m_bEngaged;
//    ObjectGuid m_playerGuid;
//    ObjectGuid m_nefariusGuid;
//
//    void Reset()
//    {
//        m_uiSpeechTimer                  = 0;
//        m_uiSpeechNum                    = 0;
//        m_uiCleaveTimer                  = 6000;
//        m_uiFlameBreathTimer             = 8000;
//        m_uiBurningAdrenalineCasterTimer = 15000;
//        m_uiBurningAdrenalineTankTimer   = 45000;
//        m_uiFireNovaTimer                = 4000;
//        m_uiTailSweepTimer               = 8000;
//        m_uiSelectableTimer              = 0;
//        m_uiIntroTimer = 0;
//        m_uiIntroPhase = 0;
//        m_bHasYelled = false;
//        m_bIsDoingSpeech = false;
//        m_bCastedEssenceOfTheRed = false;
//        m_bCastedbanishment = false;
//        m_bFlagSet = false;
//        m_uiInitTimer = 2000;
//        m_playerGuid.Clear();
//        m_nefariusGuid.Clear();
//
//        m_bEngaged = false;
//
//        m_creature->SetHealthPercent(30.0f);
//    }
//
//    void BeginSpeech(Unit* target)
//    {
//        ASSERT(target);
//        // Stand up and begin speech
//        m_playerGuid = target->GetObjectGuid();
//        m_creature->SetStandState(UNIT_STAND_STATE_STAND);
//        m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER | UNIT_NPC_FLAG_GOSSIP);
//        // 10 seconds
//        DoScriptText(SAY_LINE_1, m_creature);
//
//        m_uiSpeechTimer = 10000;
//        m_uiSpeechNum = 0;
//        m_bIsDoingSpeech = true;
//
//        if (nullptr == m_pInstance)
//            return;
//
//        // If Nefarius's Corruption has not been accepted by this point, fail Scepter Run
//        if (m_pInstance->GetData(TYPE_SCEPTER_RUN) == NOT_STARTED)
//            m_pInstance->SetData(TYPE_SCEPTER_RUN, FAIL);
//
//    }
//
//    void KilledUnit(Unit* pVictim)
//    {
//        if (!pVictim || pVictim->GetTypeId() != TYPEID_PLAYER)
//            return;
//
//        if (urand(0, 4) == 0)
//            return;
//
//        DoScriptText(SAY_KILLTARGET, m_creature, pVictim);
//    }
//
//    void Aggro(Unit* /*pWho*/)
//    {
//        if (!m_bCastedEssenceOfTheRed)
//        {
//            DoCastSpellIfCan(m_creature, SPELL_ESSENCE_OF_THE_RED);
//            m_bCastedEssenceOfTheRed = true;
//        }
//        m_creature->SetInCombatWithZone();
//        if (m_pInstance)
//            m_pInstance->SetData(TYPE_VAELASTRASZ, IN_PROGRESS);
//
//        if (!m_bEngaged)
//        {
//            m_bEngaged = true;
//            m_creature->SetRespawnDelay(43200); // 12h 43200
//            m_creature->ForcedDespawn(3600000); // 1h 3600000
//        }
//    }
//
//    void JustDied(Unit* /*pKiller*/)
//    {
//        m_creature->SetRespawnDelay(604800); // 7 j. 604800
//        if (m_pInstance)
//            m_pInstance->SetData(TYPE_VAELASTRASZ, DONE);
//    }
//
//    void JustReachedHome()
//    {
//        if (m_pInstance)
//            m_pInstance->SetData(TYPE_VAELASTRASZ, FAIL);
//        m_creature->setFaction(FACTION_BLACK_DRAGON);
//        m_creature->SetStandState(UNIT_STAND_STATE_STAND);
//        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE);
//        m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER | UNIT_NPC_FLAG_GOSSIP);
//    }
//
//    void UpdateAI(const uint32 uiDiff)
//    {
//        if (!m_pInstance)
//            return;
//
//        if (!m_bIntroEvent && m_pInstance->GetData(TYPE_VAEL_EVENT) == DONE)
//        {
//            m_uiIntroTimer = 1000;
//            m_bIntroEvent = true;
//        }
//
//        if (m_uiIntroTimer)
//        {
//            if (m_uiIntroTimer <= uiDiff)
//            {
//                switch (m_uiIntroPhase)
//                {
//                case 0:
//                    if (Creature *pNefarius = m_creature->SummonCreature(NPC_LORD_NEFARIAN_VAEL, aNefariusSpawnLoc[0], aNefariusSpawnLoc[1], aNefariusSpawnLoc[2], aNefariusSpawnLoc[3], TEMPSUMMON_TIMED_DESPAWN, 25000))
//                    {
//                        pNefarius->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
//                        m_nefariusGuid = pNefarius->GetObjectGuid();
//                    }
//                    m_uiIntroTimer = 1000;
//                    break;
//                case 1:
//                    if (Creature* pNefarius = m_creature->GetMap()->GetCreature(m_nefariusGuid))
//                    {
//                        pNefarius->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, m_creature->GetObjectGuid());
//                        pNefarius->SetUInt32Value(UNIT_CHANNEL_SPELL, SPELL_BANISHEMENT_OF_SCALE);
//                        if (SpellAuraHolder* pHolder = m_creature->AddAura(SPELL_NEFARIUS_CORRUPTION))
//                        {
//                            pHolder->SetAuraMaxDuration(24000);
//                            pHolder->SetAuraDuration(24000);
//                        }
//                        DoScriptText(SAY_NEFARIUS_CORRUPT_1, pNefarius);
//                    }
//                    m_uiIntroTimer = 15750;
//                    break;
//                case 2:
//                    if (Creature* pNefarius = m_creature->GetMap()->GetCreature(m_nefariusGuid))
//                        DoScriptText(SAY_NEFARIUS_CORRUPT_2, pNefarius);
//                    m_uiSelectableTimer = 8250;
//                    m_bCastedbanishment = true;
//                    m_pInstance->SetData(TYPE_VAELASTRASZ, SPECIAL);
//                    m_uiIntroTimer = 0;
//                    break;
//                }
//                ++m_uiIntroPhase;
//            }
//            else
//                m_uiIntroTimer -= uiDiff;
//        }
//
//        if (!m_creature->isInCombat() && !m_bFlagSet)
//        {
//            if (m_uiInitTimer < uiDiff)
//            {
//                if (m_pInstance->GetData(TYPE_VAEL_EVENT) != DONE)
//                {
//                    m_creature->setFaction(FACTION_FRIENDLY);
//                    m_creature->SetStandState(UNIT_STAND_STATE_DEAD);
//                }
//                else
//                {
//                    m_creature->setFaction(FACTION_BLACK_DRAGON);
//                    m_creature->SetStandState(UNIT_STAND_STATE_STAND);
//                }
//                m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE);
//                m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER | UNIT_NPC_FLAG_GOSSIP);
//                m_bFlagSet = true;
//            }
//            else
//                m_uiInitTimer -= uiDiff;
//        }
//
//        if (m_bCastedbanishment)
//        {
//            if (m_uiSelectableTimer < uiDiff)
//            {
//                m_bCastedbanishment = false;
//                //m_uiSelectableTimer = 0;
//                m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER | UNIT_NPC_FLAG_GOSSIP);
//            }
//            else
//                m_uiSelectableTimer -= uiDiff;
//        }
//
//        // Speech
//        if (m_bIsDoingSpeech)
//        {
//            if (m_uiSpeechTimer < uiDiff)
//            {
//                switch (m_uiSpeechNum)
//                {
//                    case 0:
//                        // 16 seconds till next line
//                        DoScriptText(SAY_LINE_2, m_creature);
//                        m_uiSpeechTimer = 16000;
//                        ++m_uiSpeechNum;
//                        break;
//                    case 1:
//                        // This one is actually 16 seconds but we only go to 10 seconds because he starts attacking after he says "I must fight this!"
//                        // (French version should start attacking after 12 seconds)
//                        DoScriptText(SAY_LINE_3, m_creature);
//                        m_uiSpeechTimer = 10000;
//                        ++m_uiSpeechNum;
//                        break;
//                    case 2:
//                        m_creature->setFaction(FACTION_BLACK_DRAGON);
//                        if (m_playerGuid)
//                        {
//                            if (Player* pPlayer = m_creature->GetMap()->GetPlayer(m_playerGuid))
//                                AttackStart(pPlayer);
//
//                            DoCastSpellIfCan(m_creature, SPELL_ESSENCE_OF_THE_RED);
//                            m_bCastedEssenceOfTheRed = true;
//                        }
//                        m_uiSpeechTimer = 0;
//                        m_bIsDoingSpeech = false;
//                        break;
//                }
//            }
//            else
//                m_uiSpeechTimer -= uiDiff;
//        }
//
//        // Return since we have no target
//        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
//            return;
//
//        // Burning Adrenaline Caster Timer
//        // caster... caster... caster + tank... caster... caster... caster + tank...
//        if (m_uiBurningAdrenalineCasterTimer < uiDiff)
//        {
//            std::vector<ObjectGuid> vPossibleVictim;
//            ThreatList const& tList = m_creature->getThreatManager().getThreatList();
//            for (ThreatList::const_iterator itr = tList.begin(); itr != tList.end(); ++itr)
//            {
//                Player* pPlayer = m_creature->GetMap()->GetPlayer((*itr)->getUnitGuid());
//                if (pPlayer && pPlayer->isAlive() && pPlayer->getPowerType() == POWER_MANA && !pPlayer->HasAura(SPELL_BURNING_ADRENALINE, EFFECT_INDEX_0))
//                    vPossibleVictim.push_back(pPlayer->GetObjectGuid());
//            }
//            if (!vPossibleVictim.empty())
//            {
//                if (Player* pPlayer = m_creature->GetMap()->GetPlayer(vPossibleVictim[urand(0, vPossibleVictim.size() - 1)]))
//                    pPlayer->CastSpell(pPlayer, SPELL_BURNING_ADRENALINE, true);
//            }
//            m_uiBurningAdrenalineCasterTimer = 15000;
//        }
//        else
//            m_uiBurningAdrenalineCasterTimer -= uiDiff;
//
//        // Burning Adrenaline Tank Timer
//        if (m_uiBurningAdrenalineTankTimer < uiDiff)
//        {
//            // have the victim cast the spell on himself otherwise the third effect aura will be applied
//            // to Vael instead of the player
//            if (m_creature->getVictim() && !m_creature->getVictim()->HasAura(SPELL_BURNING_ADRENALINE) && m_creature->getVictim()->isAlive())
//            {
//                m_creature->getVictim()->CastSpell(m_creature->getVictim(), SPELL_BURNING_ADRENALINE, true);
//                m_uiBurningAdrenalineTankTimer = 45000;
//            }
//        }
//        else
//            m_uiBurningAdrenalineTankTimer -= uiDiff;
//
//        // Yell if hp lower than 15%
//        if (m_creature->GetHealthPercent() < 15.0f && !m_bHasYelled)
//        {
//            DoScriptText(SAY_HALFLIFE, m_creature);
//            m_bHasYelled = true;
//        }
//
//        // Cleave Timer
//        if (m_uiCleaveTimer < uiDiff)
//        {
//            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_CLEAVE) == CAST_OK)
//                m_uiCleaveTimer = urand(5000, 10000);
//        }
//        else
//            m_uiCleaveTimer -= uiDiff;
//
//        // Flame Breath Timer
//        if (m_uiFlameBreathTimer < uiDiff)
//        {
//            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_FLAME_BREATH) == CAST_OK)
//                m_uiFlameBreathTimer = urand(5000, 10000);
//        }
//        else
//            m_uiFlameBreathTimer -= uiDiff;
//
//        // Fire Nova Timer
//        if (m_uiFireNovaTimer < uiDiff)
//        {
//            if (DoCastSpellIfCan(m_creature, SPELL_FIRE_NOVA) == CAST_OK)
//                m_uiFireNovaTimer = 2000;
//        }
//        else
//            m_uiFireNovaTimer -= uiDiff;
//
//        // Tail Sweep Timer
//        if (m_uiTailSweepTimer < uiDiff)
//        {
//            if (DoCastSpellIfCan(m_creature, SPELL_TAIL_SWEEP) == CAST_OK)
//                m_uiTailSweepTimer = urand(4000, 6000);
//        }
//        else
//            m_uiTailSweepTimer -= uiDiff;
//
//        DoMeleeAttackIfReady();
//    }
//};
//
//bool GossipSelect_boss_vael(Player* pPlayer, Creature* pCreature, uint32 uiSender, uint32 uiAction)
//{
//    switch (uiAction)
//    {
//        case GOSSIP_ACTION_INFO_DEF+1:
//            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_VAEL_2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
//            pPlayer->SEND_GOSSIP_MENU(GOSSIP_TEXT_VAEL_2, pCreature->GetObjectGuid());
//            break;
//        case GOSSIP_ACTION_INFO_DEF+2: // Fight Time
//            pPlayer->CLOSE_GOSSIP_MENU();
//            if (boss_vaelAI* pVaelAI = dynamic_cast<boss_vaelAI*>(pCreature->AI()))
//                pVaelAI->BeginSpeech((Unit*)pPlayer);
//            break;
//    }
//
//    return true;
//}
//
//bool GossipHello_boss_vael(Player* pPlayer, Creature* pCreature)
//{
//    ScriptedInstance* m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
//
//    if (nullptr == m_pInstance)
//        return false;
//
//    if (m_pInstance->GetData(TYPE_RAZORGORE) != DONE && !pPlayer->IsGameMaster())
//        return false;
//
//    if (pCreature->isQuestGiver())
//        if (m_pInstance->GetData(TYPE_SCEPTER_RUN) == NOT_STARTED)
//            pPlayer->PrepareQuestMenu(pCreature->GetObjectGuid());
//
//    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_VAEL_1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
//    pPlayer->SEND_GOSSIP_MENU(GOSSIP_TEXT_VAEL_1, pCreature->GetObjectGuid());
//
//    return true;
//}
//
//bool QuestAccept_vaelastrasz(Player* pPlayer, Creature* pCreature, const Quest* pQuest)
//{
//    ScriptedInstance* m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
//
//    if (nullptr == m_pInstance)
//        return false;
//
//    if (pQuest->GetQuestId() == QUEST_NEFARIUS_CORRUPTION)
//    {
//            // Only one may accept
//            if (m_pInstance->GetData(TYPE_SCEPTER_RUN) != NOT_STARTED)
//            {
//                pPlayer->FailQuest(QUEST_NEFARIUS_CORRUPTION);
//                return false;
//            }
//
//            m_pInstance->SetData(TYPE_SCEPTER_RUN, SPECIAL);
//            m_pInstance->SetData(DATA_SCEPTER_CHAMPION, pPlayer->GetObjectGuid());
//
//            // Permanently bind player to instance
//            pCreature->GetMap()->BindToInstanceOrRaid(pPlayer, pCreature->GetRespawnTimeEx(), true);
//
//            return true;
//    }
//
//    return false;
//}
//
//CreatureAI* GetAI_boss_vael(Creature* pCreature)
//{
//    return new boss_vaelAI(pCreature);
//}

/**************************
*** Death Talon Captain ***
***************************/

enum
{
    MOB_RONGE_GRIFFEMORT        = 12464,
    MOB_WYRMIDE_GRIFFEMORT      = 12465,
    MOB_FLAMMECAILLE_GRIFFEMORT = 12463,

    SPELL_MARK_DETONATION       = 22438,
    SPELL_MARK_FLAMES           = 25050,
    SPELL_COMMANDING_SHOUT      = 22440,
    SPELL_CLEAVE2               = 15496,
    SPELL_AURA_FLAMES           = 22436
};

struct npc_death_talon_CaptainAI : public ScriptedAI
{
    npc_death_talon_CaptainAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 m_uiMarkDetonationTimer;
    uint32 m_uiMarkFlamesTimer;
    uint32 m_uiCommandingShoutTimer;
    uint32 m_uiCleaveTimer;

    void Reset()
    {
        m_uiMarkDetonationTimer     = 10000;
        m_uiMarkFlamesTimer         = 6000;
        m_uiCommandingShoutTimer    = urand(12000, 25000);
        m_uiCleaveTimer             = urand(4000, 8000);
        DoCastSpellIfCan(m_creature, SPELL_AURA_FLAMES, CF_AURA_NOT_PRESENT);
        SetAuraFlames(false);
    }

    void MoveInLineOfSight(Unit *pUnit)
    {
        if (!pUnit || m_creature->getVictim())
            return;

        if (pUnit->IsPlayer() && m_creature->GetDistance2d(pUnit) < 29.0f && m_creature->IsWithinLOSInMap(pUnit)
          && pUnit->isTargetableForAttack() && pUnit->isInAccessablePlaceFor(m_creature))
            AttackStart(pUnit);
    }

    void Aggro(Unit* /*pWho*/)
    {
        if (!m_creature->HasAura(SPELL_AURA_FLAMES))
            m_creature->AddAura(SPELL_AURA_FLAMES, ADD_AURA_PERMANENT);

        DoCastSpellIfCan(m_creature, SPELL_COMMANDING_SHOUT, CF_TRIGGERED);
    }

    void JustDied(Unit* /*pKiller*/)
    {
        SetAuraFlames(false);
    }

    void SetAuraFlames(bool on)
    {
        std::list<Creature *> lCreature;
        GetCreatureListWithEntryInGrid(lCreature, m_creature, MOB_FLAMMECAILLE_GRIFFEMORT, 50.0f);
        GetCreatureListWithEntryInGrid(lCreature, m_creature, MOB_WYRMIDE_GRIFFEMORT, 50.0f);
        GetCreatureListWithEntryInGrid(lCreature, m_creature, MOB_RONGE_GRIFFEMORT, 50.0f);

        for (std::list<Creature *>::iterator itr = lCreature.begin(); itr != lCreature.end(); ++itr)
        {
            if (!(*itr)->isAlive())
                continue;

            if (on && m_creature->isAlive())
            {
                if (m_creature->IsWithinDistInMap((*itr), 25.0f))
                {
                    if (!(*itr)->HasAura(SPELL_AURA_FLAMES))
                        (*itr)->AddAura(SPELL_AURA_FLAMES);
                }
                else
                    (*itr)->RemoveAurasDueToSpell(SPELL_AURA_FLAMES);
            }
            else
                (*itr)->RemoveAurasDueToSpell(SPELL_AURA_FLAMES);
        }
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        SetAuraFlames(true);

        if (m_uiCleaveTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_CLEAVE2) == CAST_OK)
                m_uiCleaveTimer = urand(4000, 8000);
        }
        else
            m_uiCleaveTimer -= uiDiff;

        if (m_uiCommandingShoutTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_COMMANDING_SHOUT) == CAST_OK)
                m_uiCommandingShoutTimer = urand(10000, 20000);
        }
        else
            m_uiCommandingShoutTimer -= uiDiff;

        if (m_uiMarkFlamesTimer < uiDiff)
        {
            if (Unit* pUnit = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
            {
                if (DoCastSpellIfCan(pUnit, SPELL_MARK_FLAMES) == CAST_OK)
                    m_uiMarkFlamesTimer = 15000;
            }
        }
        else
            m_uiMarkFlamesTimer -= uiDiff;

        if (m_uiMarkDetonationTimer < uiDiff)
        {
            if (Unit* pUnit = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
            {
                if (pUnit->isAlive())
                {
                    pUnit->CastSpell(pUnit, SPELL_MARK_DETONATION, true);
                    m_uiMarkDetonationTimer = urand(10000, 20000);
                }
            }
        }
        else
            m_uiMarkDetonationTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_death_talon_Captain(Creature* pCreature)
{
    return new npc_death_talon_CaptainAI(pCreature);
}

/**************************
*** Death Talon Seether ***
***************************/

enum
{
    SPELL_FRENZY         = 22428,
    SPELL_FLAME_BUFFET   = 22433,

    EMOTE_FRENZY         = -1000001
};

struct npc_death_talon_SeetherAI : public ScriptedAI
{
    npc_death_talon_SeetherAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiFlameBuffetTimer;
    uint32 m_uiFrenzyTimer;
    bool m_bEngaged;

    void Reset()
    {
        m_uiFlameBuffetTimer = urand(5000, 10000);
        m_uiFrenzyTimer     = 15000;
        m_bEngaged          = false;
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiFrenzyTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_FRENZY) == CAST_OK)
            {
                DoScriptText(EMOTE_FRENZY, m_creature);
                m_uiFrenzyTimer = 15000;
            }
        }
        else m_uiFrenzyTimer -= uiDiff;

        if (!m_bEngaged)
        {
            if (m_creature->IsWithinMeleeRange(m_creature->getVictim()))
                m_bEngaged = true;
        }
        else
        {
            if (m_uiFlameBuffetTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_FLAME_BUFFET) == CAST_OK)
                    m_uiFlameBuffetTimer = urand(8000, 12000);
            }
            else m_uiFlameBuffetTimer -= uiDiff;
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_death_talon_Seether(Creature* pCreature)
{
    return new npc_death_talon_SeetherAI(pCreature);
}

void AddSC_boss_shredder()
{
    Script* pNewScript;

    /*pNewScript = new Script;
    pNewScript->Name = "boss_vaelastrasz";
    pNewScript->GetAI = &GetAI_boss_vael;
    pNewScript->pGossipHello = &GossipHello_boss_vael;
    pNewScript->pGossipSelect = &GossipSelect_boss_vael;
    pNewScript->pQuestAcceptNPC = &QuestAccept_vaelastrasz;
    pNewScript->RegisterSelf();*/

    Script* newscript;
    newscript = new Script;
    newscript->Name = "boss_shredder";
    newscript->GetAI = &GetAI_boss_shredder;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_sawblade";
    newscript->GetAI = &GetAI_npc_sawblade;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_oilbarrel";
    newscript->GetAI = &GetAI_npc_oilbarrel;
    newscript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_death_talon_Captain";
    pNewScript->GetAI = &GetAI_npc_death_talon_Captain;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_death_talon_Seether";
    pNewScript->GetAI = &GetAI_npc_death_talon_Seether;
    pNewScript->RegisterSelf();
}
