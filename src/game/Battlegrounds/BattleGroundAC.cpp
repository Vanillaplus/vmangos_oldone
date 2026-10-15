/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 * Copyright (C) 2009-2011 MaNGOSZero <https://github.com/mangos/zero>
 * Copyright (C) 2011-2016 Nostalrius <https://nostalrius.org>
 * Copyright (C) 2016-2017 Elysium Project <https://github.com/elysium-project>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTACILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Object.h"
#include "Player.h"
#include "BattleGround.h"
#include "BattleGroundAC.h"
#include "Creature.h"
#include "GameObject.h"
#include "BattleGroundMgr.h"
#include "Language.h"
#include "Util.h"
#include "WorldPacket.h"
#include "MapManager.h"
#include "ObjectMgr.h"
#include "scriptPCH.h"

BattleGroundAC::BattleGroundAC()
{
    m_StartMessageIds[BG_STARTING_EVENT_FIRST] = 0;
    m_StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_BG_AC_START_ONE_MINUTE;
    m_StartMessageIds[BG_STARTING_EVENT_THIRD] = LANG_BG_AC_START_HALF_MINUTE;
    m_StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_BG_AC_HAS_BEGUN;
}

BattleGroundAC::~BattleGroundAC()
{
}

void BattleGroundAC::Update(uint32 diff)
{
    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        int team_points[BG_TEAMS_COUNT] = { 0, 0 };

        for (int node = 0; node < BG_AC_NODES_MAX; ++node)
        {

            // 1-minute to occupy a node from contested state
            if (m_NodeTimers[node])
            {
                if (m_NodeTimers[node] > diff)
                    m_NodeTimers[node] -= diff;
                else
                {
                    m_NodeTimers[node] = 0;
                    // Change from contested to occupied !
                    uint8 teamIndex = m_Nodes[node] - 1;
                    m_prevNodes[node] = m_Nodes[node];
                    m_Nodes[node] += 2;
                    // create new occupied banner
                    _CreateBanner(node, BG_AC_NODE_TYPE_OCCUPIED, teamIndex);
                    _SendNodeUpdate(node);
                    _NodeOccupied(node, (teamIndex == 0) ? ALLIANCE : HORDE);
                    // Message to chatlog
                    int messageType = LANG_BG_AC_NODE_TAKEN;
                    if (node > BG_AC_NODE_TEMPLE_E)
                    {
                        SpawnEvent(node + 20, 0, true, true, 1);
                        messageType = LANG_BG_AC_TOWER_DESTOYED;

                        if (teamIndex == 0)
                        {
                            CastSpellOnTeam(34828, ALLIANCE);  //Victory Rush
                            //AC repweek
                            RewardReputationToTeam(956, AC_TOWER_REP, ALLIANCE);
                            RewardHonorToTeam(500, ALLIANCE);
                            m_TeamScores[BG_TEAM_HORDE] -= 100;
                            UpdateWorldState(BG_AC_OP_RESOURCES_HORDE, m_TeamScores[BG_TEAM_HORDE]);

                            if (Creature* BossCrea = GetBgMap()->GetCreature(GetSingleCreatureGuid(BG_AC_BOSS_ALLIANCE_EVENT_ID, 1)))
                            {
                                BossCrea->AddAura(34828);
                                std::list<Creature*> SpawnerList;
                                GetCreatureListWithEntryInGrid(SpawnerList, BossCrea, BG_AC_SPAWNER2_A, 150.0f);
                                for (auto it = SpawnerList.cbegin(); it != SpawnerList.cend(); ++it)
                                {
                                    if (Unit* RunnerUnit = (*it)->SummonCreature(BG_AC_RUNNER_MELEE_A_ELITE, (*it)->GetPositionX(), (*it)->GetPositionY(), (*it)->GetPositionZ(), 0, TEMPSUMMON_DEAD_DESPAWN, 5000, true))
                                        RunnerUnit->AddAura(35328);
                                }

                                for (int i = 0; i < 4; ++i)
                                {
                                    if (Creature* Crea = BossCrea->SummonCreature(BG_AC_BUFFER_DUMMY_NPC, BuffSpawnLocs[i][0], BuffSpawnLocs[i][1], BuffSpawnLocs[i][2], 0.0f, TEMPSUMMON_TIMED_DESPAWN, 5000))
                                    {
                                        std::list<Creature*> ArmyList;
                                        GetCreatureListWithEntryInGrid(ArmyList, Crea, BG_AC_COMMANDER_A, 500.0f);
                                        GetCreatureListWithEntryInGrid(ArmyList, Crea, BG_AC_BOWMAN_A, 500.0f);
                                        GetCreatureListWithEntryInGrid(ArmyList, Crea, BG_AC_WARRIOR_A, 500.0f);
                                        GetCreatureListWithEntryInGrid(ArmyList, Crea, BG_AC_SENTINEL_A, 500.0f);
                                        GetCreatureListWithEntryInGrid(ArmyList, Crea, BG_AC_RUNNER_MELEE_A, 500.0f);
                                        GetCreatureListWithEntryInGrid(ArmyList, Crea, BG_AC_RUNNER_RANGED_A, 500.0f);
                                        GetCreatureListWithEntryInGrid(ArmyList, Crea, BG_AC_RUNNER_MELEE_A_ELITE, 500.0f);
                                        for (auto it = ArmyList.cbegin(); it != ArmyList.cend(); ++it)
                                        {
                                            (*it)->AddAura(34828);
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            CastSpellOnTeam(34828, HORDE);  //Victory Rush
                            //AC repweek
                            RewardReputationToTeam(955, AC_TOWER_REP, HORDE);
                            RewardHonorToTeam(500, HORDE);
                            m_TeamScores[BG_TEAM_ALLIANCE] -= 100;
                            UpdateWorldState(BG_AC_OP_RESOURCES_ALLY, m_TeamScores[BG_TEAM_ALLIANCE]);

                            if (Creature* BossCrea = GetBgMap()->GetCreature(GetSingleCreatureGuid(BG_AC_BOSS_HORDE_EVENT_ID, 1)))
                            {
                                BossCrea->AddAura(34828);
                                std::list<Creature*> SpawnerList;
                                GetCreatureListWithEntryInGrid(SpawnerList, BossCrea, BG_AC_SPAWNER2_H, 150.0f);
                                for (auto it = SpawnerList.cbegin(); it != SpawnerList.cend(); ++it)
                                {
                                    if (Unit* RunnerUnit = (*it)->SummonCreature(BG_AC_RUNNER_MELEE_H_ELITE, (*it)->GetPositionX(), (*it)->GetPositionY(), (*it)->GetPositionZ(), 0, TEMPSUMMON_DEAD_DESPAWN, 5000, true))
                                        RunnerUnit->AddAura(35329);
                                }

                                for (int i = 0; i < 4; ++i)
                                {
                                    if (Creature* Crea = BossCrea->SummonCreature(BG_AC_BUFFER_DUMMY_NPC, BuffSpawnLocs[i][0], BuffSpawnLocs[i][1], BuffSpawnLocs[i][2], 0.0f, TEMPSUMMON_TIMED_DESPAWN, 5000))
                                    {
                                        std::list<Creature*> ArmyList;
                                        GetCreatureListWithEntryInGrid(ArmyList, Crea, BG_AC_COMMANDER_H, 500.0f);
                                        GetCreatureListWithEntryInGrid(ArmyList, Crea, BG_AC_BOWMAN_H, 500.0f);
                                        GetCreatureListWithEntryInGrid(ArmyList, Crea, BG_AC_WARRIOR_H, 500.0f);
                                        GetCreatureListWithEntryInGrid(ArmyList, Crea, BG_AC_RUNNER_MELEE_H, 500.0f);
                                        GetCreatureListWithEntryInGrid(ArmyList, Crea, BG_AC_RUNNER_RANGED_H, 500.0f);
                                        GetCreatureListWithEntryInGrid(ArmyList, Crea, BG_AC_RUNNER_MELEE_H_ELITE, 500.0f);
                                        for (auto it = ArmyList.cbegin(); it != ArmyList.cend(); ++it)
                                        {
                                            (*it)->AddAura(34828);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if (teamIndex == 0)
                    {
                        SendMessage2ToAll(messageType, CHAT_MSG_BG_SYSTEM_ALLIANCE, nullptr, _GetNodeNameId(node), LANG_BG_ALLY);
                        PlaySoundToAll(BG_AC_SOUND_NODE_CAPTURED_ALLIANCE);
                    }
                    else
                    {
                        SendMessage2ToAll(messageType, CHAT_MSG_BG_SYSTEM_HORDE, nullptr, _GetNodeNameId(node), LANG_BG_HORDE);
                        PlaySoundToAll(BG_AC_SOUND_NODE_CAPTURED_HORDE);
                    }
                }
            }

            for (int team = 0; team < BG_TEAMS_COUNT; ++team)
                if (m_Nodes[node] == team + BG_AC_NODE_TYPE_OCCUPIED && node < BG_AC_NODE_ZERO)
                    ++team_points[team];
        }

        // Accumulate points
        for (int team = 0; team < BG_TEAMS_COUNT; ++team)
        {
            int points = team_points[team];
            int otherteam = team == BG_TEAM_ALLIANCE ? BG_TEAM_HORDE : BG_TEAM_ALLIANCE;
            if (!points)
                continue;
            m_lastTick[team] += diff;
            if (m_lastTick[team] > BG_AC_TickIntervals[points])
            {
                m_lastTick[team] -= BG_AC_TickIntervals[points];
                m_TeamScores[otherteam] -= BG_AC_TickPoints[points];
                m_HonorScoreTics[team] += BG_AC_TickPoints[points];
                m_ReputationScoreTics[team] += BG_AC_TickPoints[points];
                if (m_ReputationScoreTics[team] >= m_ReputationTics)
                {
                    //AC repweek
                    (team == BG_TEAM_ALLIANCE) ? RewardReputationToTeam(956, AC_TICK_REP, ALLIANCE) : RewardReputationToTeam(955, AC_TICK_REP, HORDE);
                    m_ReputationScoreTics[team] -= m_ReputationTics;
                }
                if (m_HonorScoreTics[team] >= m_HonorTics)
                {
                    RewardHonorToTeam(50, (team == BG_TEAM_ALLIANCE) ? ALLIANCE : HORDE);
                    m_HonorScoreTics[team] -= m_HonorTics;
                }

                if (m_TeamScores[team] > BG_AC_MAX_TEAM_SCORE)
                    m_TeamScores[team] = BG_AC_MAX_TEAM_SCORE;
                if (otherteam == BG_TEAM_ALLIANCE)
                    UpdateWorldState(BG_AC_OP_RESOURCES_ALLY, m_TeamScores[otherteam]);
                if (otherteam == BG_TEAM_HORDE)
                    UpdateWorldState(BG_AC_OP_RESOURCES_HORDE, m_TeamScores[otherteam]);
            }
        }

        // Middle Depths
        if (m_mid_update_timer < diff)
        {
            MiddlePoint();
            m_mid_update_timer = 5000;
        }
        else
            m_mid_update_timer -= diff;


        if (m_near_end_update_timer < diff)
        {
            if (!m_IsInformedNearVictory)
            {
                if (m_TeamScores[BG_TEAM_ALLIANCE] >= BG_AC_WARNING_NEAR_VICTORY_SCORE)
                    PlaySoundToAll(BG_AC_SOUND_NEAR_VICTORY),
                    m_IsInformedNearVictory = true,
                    SendMessageToAll(LANG_BG_AC_NEAR_WIN_ALLY, CHAT_MSG_BG_SYSTEM_NEUTRAL);
                if (m_TeamScores[BG_TEAM_HORDE] >= BG_AC_WARNING_NEAR_VICTORY_SCORE)
                    PlaySoundToAll(BG_AC_SOUND_NEAR_VICTORY),
                    m_IsInformedNearVictory = true,
                    SendMessageToAll(LANG_BG_AC_NEAR_WIN_HORDE, CHAT_MSG_BG_SYSTEM_NEUTRAL);
            }
            if (!m_IsInformedNearDefeat)
            {
                if (m_TeamScores[BG_TEAM_ALLIANCE] <= BG_AC_WARNING_NEAR_LOSE_SCORE)
                    PlaySoundToAll(BG_AC_SOUND_NEAR_VICTORY),
                    m_IsInformedNearDefeat = true,
                    SendMessageToAll(LANG_BG_AC_NEAR_DEFEAT_ALLY, CHAT_MSG_BG_SYSTEM_NEUTRAL);
                if (m_TeamScores[BG_TEAM_HORDE] <= BG_AC_WARNING_NEAR_LOSE_SCORE)
                    PlaySoundToAll(BG_AC_SOUND_NEAR_VICTORY),
                    m_IsInformedNearDefeat = true,
                    SendMessageToAll(LANG_BG_AC_NEAR_DEFEAT_HORDE, CHAT_MSG_BG_SYSTEM_NEUTRAL);
            }
            m_near_end_update_timer = 2000;
        }
        else
            m_near_end_update_timer -= diff;

        // Test win condition
        if (m_TeamScores[BG_TEAM_ALLIANCE] >= BG_AC_MAX_TEAM_SCORE)
            EndBattleGround(ALLIANCE);
        else if (m_TeamScores[BG_TEAM_HORDE] >= BG_AC_MAX_TEAM_SCORE)
            EndBattleGround(HORDE);
        else if (m_TeamScores[BG_TEAM_ALLIANCE] <= BG_AC_MIN_TEAM_SCORE)
            EndBattleGround(HORDE);
        else if (m_TeamScores[BG_TEAM_HORDE] <= BG_AC_MIN_TEAM_SCORE)
            EndBattleGround(ALLIANCE);
    }
    // Execute this at the end, since it can delete the BattleGround object!
    BattleGround::Update(diff);
}

void BattleGroundAC::StartingEventCloseDoors()
{
}

void BattleGroundAC::StartingEventOpenDoors()
{
    OpenDoorEvent(BG_EVENT_DOOR);
}

void BattleGroundAC::AddPlayer(Player* plr)
{
    BattleGround::AddPlayer(plr);
    //create score and add it to map, default values are set in the constructor
    BattleGroundACScore* sc = new BattleGroundACScore;

    m_PlayerScores[plr->GetObjectGuid()] = sc;
}

void BattleGroundAC::RemovePlayer(Player* plr, ObjectGuid guid)
{
}

void BattleGroundAC::BGAuraApplied(Player* plr)
{
    plr->SendUpdateWorldState(2426, 1);
}

void BattleGroundAC::BGAuraRemoved(Player* plr)
{
    plr->SendUpdateWorldState(2426, 0);
}

void BattleGroundAC::MiddlePoint()
{
    uint32 MinValue = 0;
    uint32 MaxValue = 100;
    uint32 AllyControlReqPoints = 60;
    uint32 HordeControlRqPoints = 40;
    uint32 AllyChar = 0;
    uint32 HordeChar = 0;
    uint32 Delta = 0;

    for (BattleGroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
    {
        Player* plr = sObjectMgr.GetPlayer(itr->first);

        if (!plr)
        {
            sLog.outError("BattleGround:MiddlePoints: %s not found!", itr->first.GetString().c_str());
            continue;
        }

        if (plr->HasAura(34803))
        {
            //Except Invisible/Stealthed/Petrification/Divine Intervention
            if (!plr->HasInvisibilityAura() && !plr->HasStealthAura() && !plr->HasAura(17624) && !plr->HasAura(19753))
            {
                if (plr->GetTeam() == ALLIANCE)
                    AllyChar += 1;
                else
                    HordeChar += 1;
            }
        }
    }

    if (AllyChar > 0 || HordeChar > 0)
    {
        Delta = AllyChar - HordeChar;

        if (100 + Delta > 103)
            Delta = 3;
        else if (100 + Delta < 97)
            Delta = -3;
    }

    m_mid_score += Delta;
    if (m_mid_score + 100 < MinValue + 100)
        m_mid_score = MinValue;
    if (m_mid_score + 100 > MaxValue + 100)
        m_mid_score = MaxValue;

    if (m_mid_neutral)
    {
        if (m_mid_score > AllyControlReqPoints)
            MiddleSetAlly();
        else if (m_mid_score < HordeControlRqPoints)
            MiddleSetHorde();
    }
    else if (m_mid_ally)
    {
        MiddlePointEffect();
        if (m_mid_score < AllyControlReqPoints)
            PlaySoundToAll(BG_AC_SOUND_NODE_ASSAULTED_HORDE),
            SendMessageToAll(LANG_BG_AC_MID_DEEP_ALLY_LOST, CHAT_MSG_BG_SYSTEM_ALLIANCE, nullptr),
            MiddleSetNeutral();
    }
    else if (m_mid_horde)
    {
        MiddlePointEffect();
        if (m_mid_score > HordeControlRqPoints)
            PlaySoundToAll(BG_AC_SOUND_NODE_ASSAULTED_ALLIANCE),
            SendMessageToAll(LANG_BG_AC_MID_DEEP_HORDE_LOST, CHAT_MSG_BG_SYSTEM_HORDE, nullptr),
            MiddleSetNeutral();
    }

    UpdateWorldState(2427, m_mid_score);
    UpdateWorldState(2428, 1);
}

void BattleGroundAC::MiddlePointEffect()
{
    if (m_mid_ally)
    {
        RewardHonorToTeam(5, ALLIANCE);
        //AC repweek
        RewardReputationToTeam(956, AC_MIDDLE_TICK_REP, ALLIANCE);
        m_TeamScores[BG_TEAM_ALLIANCE] += 1;
        UpdateWorldState(BG_AC_OP_RESOURCES_ALLY, m_TeamScores[BG_TEAM_ALLIANCE]);
    }
    else
    {
        RewardHonorToTeam(5, HORDE);
        //AC repweek
        RewardReputationToTeam(955, AC_MIDDLE_TICK_REP, HORDE);
        m_TeamScores[BG_TEAM_HORDE] += 1;
        UpdateWorldState(BG_AC_OP_RESOURCES_HORDE, m_TeamScores[BG_TEAM_HORDE]);
    }
}

void BattleGroundAC::MiddleSetNeutral()
{
    SpawnEvent(13, 0, true, true, 1);
    UpdateWorldState(2574, 0);
    UpdateWorldState(2575, 0);
    UpdateWorldState(2573, 1);
    m_mid_neutral = true;
    m_mid_ally = false;
    m_mid_horde = false;
}

void BattleGroundAC::MiddleSetAlly()
{
    SpawnEvent(13, 3, true, true, 1);
    PlaySoundToAll(BG_AC_SOUND_NODE_CAPTURED_ALLIANCE);
    SendMessageToAll(LANG_BG_AC_MID_DEEP_ALLY_TAKEN, CHAT_MSG_BG_SYSTEM_ALLIANCE, nullptr);
    UpdateWorldState(2573, 0);
    UpdateWorldState(2575, 0);
    UpdateWorldState(2574, 1);
    m_mid_neutral = false;
    m_mid_ally = true; 
    m_mid_horde = false;
}

void BattleGroundAC::MiddleSetHorde()
{
    SpawnEvent(13, 4, true, true, 1);
    PlaySoundToAll(BG_AC_SOUND_NODE_CAPTURED_HORDE);
    SendMessageToAll(LANG_BG_AC_MID_DEEP_HORDE_TAKEN, CHAT_MSG_BG_SYSTEM_HORDE, nullptr);
    UpdateWorldState(2573, 0);
    UpdateWorldState(2574, 0);
    UpdateWorldState(2575, 1);
    m_mid_neutral = false;
    m_mid_ally = false;
    m_mid_horde = true;
}

void BattleGroundAC::GYSetNeutral()
{
    SpawnEvent(14, 0, true, true, 1);
    UpdateWorldState(2578, 1);
    UpdateWorldState(2579, 0);
    UpdateWorldState(2580, 0);
    m_GY_neutral = true;
    m_GY_ally = false;
    m_GY_horde = false;

    //Repop player spirits
    for (BattleGroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
    {
        if (Player* plr = sObjectMgr.GetPlayer(itr->first))
            if (!plr->isAlive())
                if (plr->HasAura(2584))
                    if (plr->GetDistance2d(-230.0f, 4.0f) < 25.0f)
                        if (plr->GetTeam() == ALLIANCE)
                            plr->TeleportTo(37, 829.0f, 118.0f, 269.32f, 2.0f, 0);
                        else
                            plr->TeleportTo(37, 48.0f, 685.0f, 259.6f, 5.0f, 0);
    }
}

void BattleGroundAC::GYSetAlly()
{
    SpawnEvent(14, 3, true, true, 1);
    UpdateWorldState(2578, 0);
    UpdateWorldState(2579, 1);
    UpdateWorldState(2580, 0);
    m_GY_neutral = false;
    m_GY_ally = true;
    m_GY_horde = false;
}

void BattleGroundAC::GYSetHorde()
{
    SpawnEvent(14, 4, true, true, 1);
    UpdateWorldState(2578, 0);
    UpdateWorldState(2579, 0);
    UpdateWorldState(2580, 1);
    m_GY_neutral = false;
    m_GY_ally = false;
    m_GY_horde = true;
}

void BattleGroundAC::HandleKillPlayer(Player* player, Player* killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    if (killer && player->getFaction() != killer->getFaction())
    {
        UpdatePlayerScore(killer, SCORE_HONORABLE_KILLS, 1);
        UpdatePlayerScore(killer, SCORE_KILLING_BLOWS, 1);

        for (BattleGroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
        {
            Player* plr = sObjectMgr.GetPlayer(itr->first);

            if (!plr || plr == killer)
                continue;

            if (plr->GetTeam() == killer->GetTeam() && plr->IsAtGroupRewardDistance(player))
                UpdatePlayerScore(plr, SCORE_HONORABLE_KILLS, 1);
        }
    }

    if (!player->HasAura(27827))
    {
        // add +1 deaths
        UpdatePlayerScore(player, SCORE_DEATHS, 1);
        player->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);
    }

    if (killer)
    {
        if (player->HasAura(34975))
        {
            if (SpellAuraHolder* holder = player->GetSpellAuraHolder(34975))
            {
                uint32 victimStacks = holder->GetStackAmount();
                uint32 StacksDelta = ceil(float(victimStacks / 10.0f));

                //Killer mod
                if (killer->HasAura(34975))
                {
                    if (SpellAuraHolder* Kholder = killer->GetSpellAuraHolder(34975))
                        Kholder->ModStackAmount(StacksDelta);
                }
                else
                {
                    killer->AddAura(34975);
                    if (SpellAuraHolder* Kholder = killer->GetSpellAuraHolder(34975))
                        Kholder->ModStackAmount(StacksDelta - 1);
                }

                //Victim mod
                if (victimStacks == 1)
                    player->RemoveAurasDueToSpell(34975);
                else
                    holder->ModStackAmount(-StacksDelta);
            }
        }
    }



    //BattleGround::HandleKillPlayer(player, killer);
    //if (!player->HasAura(27827)) // Esprit de redemption
    //    m_TeamScores[(GetTeamIndexByTeamId(player->GetTeam()))] -= 1;
}

void BattleGroundAC::HandleKillUnit(Creature* creature, Player* killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;
    switch (creature->GetEntry())
    {
        case BG_AC_BOSS_ALLIANCE:
        {
            SendMessageToAll(LANG_BG_AC_H_KILLED_SHADOWSONG, CHAT_MSG_BG_SYSTEM_ALLIANCE, nullptr);
            //CastSpellOnTeam(BG_AC_BOSS_KILL_QUEST_SPELL, ALLIANCE);   // this is a spell which finishes a quest where a player has to kill the boss
                            //AC repweek
            RewardReputationToTeam(955, AC_BOSS_KILL_REP, HORDE);
            RewardHonorToTeam(2200, HORDE);
            EndBattleGround(HORDE);
            break;
        }
        case BG_AC_BOSS_HORDE:
        {
            SendMessageToAll(LANG_BG_AC_A_KILLED_SAURFANG, CHAT_MSG_BG_SYSTEM_HORDE, nullptr);
            //CastSpellOnTeam(BG_AC_BOSS_KILL_QUEST_SPELL, HORDE);   // this is a spell which finishes a quest where a player has to kill the boss
                            //AC repweek
            RewardReputationToTeam(956, AC_BOSS_KILL_REP, ALLIANCE);
            RewardHonorToTeam(2200, ALLIANCE);
            EndBattleGround(ALLIANCE);
            break;
        }
        case BG_AC_CRYSTALLUS:
        {
            {
                if (killer->GetTeam() == ALLIANCE)
                {
                    //AC repweek
                    RewardReputationToTeam(956, AC_CRYSTALUS_REP, ALLIANCE);
                    RewardHonorToTeam(540, ALLIANCE);
                    m_TeamScores[BG_TEAM_ALLIANCE] += 50;
                    SendMessageToAll(LANG_BG_AC_CRYSTALLUS_KILL_A, CHAT_MSG_BG_SYSTEM_ALLIANCE, nullptr);
                    UpdateWorldState(BG_AC_OP_RESOURCES_ALLY, m_TeamScores[BG_TEAM_ALLIANCE]);
                }
                else if (killer->GetTeam() == HORDE)
                {
                    //AC repweek
                    RewardReputationToTeam(955, AC_CRYSTALUS_REP, HORDE);
                    RewardHonorToTeam(540, HORDE);
                    m_TeamScores[BG_TEAM_HORDE] += 50;
                    SendMessageToAll(LANG_BG_AC_CRYSTALLUS_KILL_H, CHAT_MSG_BG_SYSTEM_HORDE, nullptr);
                    UpdateWorldState(BG_AC_OP_RESOURCES_HORDE, m_TeamScores[BG_TEAM_HORDE]);
                }
            }

            if (killer->HasAura(34975))
            {
                if (SpellAuraHolder* Kholder = killer->GetSpellAuraHolder(34975))
                    Kholder->ModStackAmount(200);
            }
            else
            {
                killer->AddAura(34975);
                if (SpellAuraHolder* Kholder = killer->GetSpellAuraHolder(34975))
                    Kholder->ModStackAmount(199);
            }
            break;
        }
        case BG_AC_TRASH_DEER:
        case BG_AC_TRASH_SPIDER:
        {
            if (!killer->HasAura(35318))
            {
                if (killer->HasAura(35319))
                {
                    if (urand(0, 9) > 7)
                    {
                        killer->AddAura(34975);
                        killer->AddAura(35318);
                        killer->AddAura(35319);
                    }
                }
                else
                {
                    if (urand(0, 1) > 0)
                    {
                        killer->AddAura(34975);
                        killer->AddAura(35318);
                        killer->AddAura(35319);
                    }
                }
            }
                

            break;
        }
        case BG_AC_COMMANDER_A:
        case BG_AC_COMMANDER_H:
        {
            if (killer->HasAura(34975))
            {
                if (SpellAuraHolder* Kholder = killer->GetSpellAuraHolder(34975))
                    Kholder->ModStackAmount(30);
            }
            else
            {
                killer->AddAura(34975);
                if (SpellAuraHolder* Kholder = killer->GetSpellAuraHolder(34975))
                    Kholder->ModStackAmount(29);
            }

            break;
        }
        case BG_AC_GOBLIN_A:
        case BG_AC_GOBLIN_H:
        case BG_AC_BS_A:
        case BG_AC_BS_H:
        case BG_AC_MEDIC_A:
        case BG_AC_MEDIC_H:
        case BG_AC_NATUR_A:
        case BG_AC_NATUR_H:
        case BG_AC_CAVEDRUID_A:
        case BG_AC_CAVEDRUID_H:
        {
            if (killer->HasAura(34975))
            {
                if (SpellAuraHolder* Kholder = killer->GetSpellAuraHolder(34975))
                    Kholder->ModStackAmount(50);
            }
            else
            {
                killer->AddAura(34975);
                if (SpellAuraHolder* Kholder = killer->GetSpellAuraHolder(34975))
                    Kholder->ModStackAmount(49);
            }

            break;
        }
        default:
        {
            if (!killer->HasAura(35318))
            {
                if (killer->HasAura(35319))
                {
                    if (urand(0, 2) > 1)
                    {
                        killer->AddAura(34975);
                        killer->AddAura(35318);
                        killer->AddAura(35319);
                    }
                }
                else
                {
                    killer->AddAura(34975);
                    killer->AddAura(35318);
                    killer->AddAura(35319);
                }
            }
        }
            break;
    }
}

void BattleGroundAC::HandleAreaTrigger(Player* Source, uint32 Trigger)
{
    switch (Trigger)
    {
        case 4306:                                          // Azshara Crater Alliance Exit.
            if (Source->GetTeam() != ALLIANCE)
                Source->GetSession()->SendNotification(LANG_BATTLEGROUND_ONLY_ALLIANCE_USE);
            else
                Source->LeaveBattleground();
            break;
        case 4307:                                          // Azshara Crater Horde Exit.
            if (Source->GetTeam() != HORDE)
                Source->GetSession()->SendNotification(LANG_BATTLEGROUND_ONLY_HORDE_USE);
            else
                Source->LeaveBattleground();
            break;
        case 4308: // Crystal depths
        case 3866:                                          // Stables
        case 3869:                                          // Gold Mine
        case 3867:                                          // Farm
        case 3868:                                          // Lumber Mill
        case 3870:                                          // Black Smith
        case 4020:                                          // Unk1
        case 4021:                                          // Unk2
        //break;
        default:
            //sLog.outError("WARNING: Unhandled AreaTrigger in Battleground: %u", Trigger);
            //Source->GetSession()->SendAreaTriggerMessage("Warning: Unhandled AreaTrigger in Battleground: %u", Trigger);
            break;
    }
}

void BattleGroundAC::_CreateBanner(uint32 node, uint32 type, uint8 teamIndex)
{
    uint32 delay = 0;
    switch (type) 
    {
        case BG_AC_NODE_TYPE_CONTESTED:
            delay = 1;
            break;
        case BG_AC_NODE_TYPE_OCCUPIED:
            delay = 5;
            break;
    }
    // cause the node-type is in the generic form
    // please see in the headerfile for the ids
    if (type != BG_AC_NODE_TYPE_NEUTRAL)
        type += teamIndex;

    SpawnEvent(node, type, true, true, delay);                           // will automaticly despawn other events
}

int32 BattleGroundAC::_GetNodeNameId(uint32 node)
{
    switch (node)
    {
        case BG_AC_NODE_TEMPLE_MID:
            return LANG_BG_AC_NODE_TEMPLE_MID;
        case BG_AC_NODE_TEMPLE_W:
            return LANG_BG_AC_NODE_TEMPLE_W;
        case BG_AC_NODE_TEMPLE_E:
            return LANG_BG_AC_NODE_TEMPLE_E;

        case BG_AC_NODE_ZERO:
            return LANG_BG_AC_NODE_ZERO;
        case BG_AC_NODE_MONO:
            return LANG_BG_AC_NODE_MONO;
        case BG_AC_NODE_OWL:
            return LANG_BG_AC_NODE_OWL;
        case BG_AC_NODE_LEFT:
            return LANG_BG_AC_NODE_LEFT;
        case BG_AC_NODE_RIGHT:
            return LANG_BG_AC_NODE_RIGHT;

        case BG_AC_NODE_GROMM:
            return LANG_BG_AC_NODE_GROMM;
        case BG_AC_NODE_BLIND:
            return LANG_BG_AC_NODE_BLIND;
        case BG_AC_NODE_LASTH:
            return LANG_BG_AC_NODE_LASTH;
        case BG_AC_NODE_N_IRON:
            return LANG_BG_AC_NODE_N_IRON;
        case BG_AC_NODE_S_IRON:
            return LANG_BG_AC_NODE_S_IRON;
    default:
        MANGOS_ASSERT(0);
    }
    return 0;
}

void BattleGroundAC::FillInitialWorldStates(WorldPacket& data, uint32& count)
{
    // Node icons
    for (uint32 node = 0; node < BG_AC_NODES_MAX; ++node)
        FillInitialWorldState(data, count, BG_AC_OP_NODEICONS_NEUTRA[node], m_Nodes[node] == 0);
    // Node occupied states
    // How many bases each team owns
    uint8 ally = 0, horde = 0;
    for (uint32 node = 0; node < BG_AC_NODES_MAX; ++node)
        if (m_Nodes[node] == BG_AC_NODE_STATUS_ALLY_OCCUPIED)
            FillInitialWorldState(data, count, BG_AC_OP_NODESTATES_A_OWN[node], 2),
            ++ally;
        else if (m_Nodes[node] == BG_AC_NODE_STATUS_HORDE_OCCUPIED)
            FillInitialWorldState(data, count, BG_AC_OP_NODESTATES_H_OWN[node], 3),
            ++horde;
        else if (m_Nodes[node] == BG_AC_NODE_STATUS_ALLY_CONTESTED)
            FillInitialWorldState(data, count, BG_AC_OP_NODESTATES_ACONT[node], 4);
        else if (m_Nodes[node] == BG_AC_NODE_STATUS_HORDE_CONTESTED)
            FillInitialWorldState(data, count, BG_AC_OP_NODESTATES_HCONT[node], 1);

    if (m_mid_neutral)
        FillInitialWorldState(data, count, 2573, 1);
    else if (m_mid_ally)
        FillInitialWorldState(data, count, 2574, 1);
    else if (m_mid_horde)
        FillInitialWorldState(data, count, 2575, 1);

    if (m_GY_neutral)
        FillInitialWorldState(data, count, 2578, 1);
    else if (m_GY_ally)
        FillInitialWorldState(data, count, 2579, 1);
    else if (m_GY_horde)
        FillInitialWorldState(data, count, 2580, 1);

    //Mid Slider
    FillInitialWorldState(data, count, 2426, 0);
    FillInitialWorldState(data, count, 2427, 50);

    FillInitialWorldState(data, count, BG_AC_OP_OCCUPIED_BASES_ALLY, ally);
    FillInitialWorldState(data, count, BG_AC_OP_OCCUPIED_BASES_HORDE, horde);

    // Team scores
    FillInitialWorldState(data, count, BG_AC_OP_RESOURCES_MAX, BG_AC_MAX_TEAM_SCORE);
    FillInitialWorldState(data, count, BG_AC_OP_RESOURCES_MAXWARNING, BG_AC_WARNING_NEAR_VICTORY_SCORE);
    FillInitialWorldState(data, count, BG_AC_OP_RESOURCES_ALLY, m_TeamScores[BG_TEAM_ALLIANCE]);
    FillInitialWorldState(data, count, BG_AC_OP_RESOURCES_HORDE, m_TeamScores[BG_TEAM_HORDE]);
}

void BattleGroundAC::_SendNodeUpdate(uint32 node)
{
    // Send node owner state update to refresh map icons on client  a con 2416, a 2417, h con 2419, h 2418
    const uint32 plusArray[] = { 0, 2, 3, 0, 1 };

    if (m_prevNodes[node])
        UpdateWorldState(BG_AC_OP_NODESTATES_A_OWN[node] + plusArray[m_prevNodes[node]], 0);
    else
        UpdateWorldState(BG_AC_OP_NODEICONS_NEUTRA[node], 0);

    UpdateWorldState(BG_AC_OP_NODESTATES_A_OWN[node] + plusArray[m_Nodes[node]], 1);

    // How many bases each team owns
    uint8 ally = 0, horde = 0;
    for (uint32 i = 0; i < BG_AC_NODES_MAX; ++i)
        if (m_Nodes[i] == BG_AC_NODE_STATUS_ALLY_OCCUPIED)
            ++ally;
        else if (m_Nodes[i] == BG_AC_NODE_STATUS_HORDE_OCCUPIED)
            ++horde;

    // Temple nodes + GY
    if (node < BG_AC_NODE_ZERO)
    {
        uint8 allyTemple = 0, hordeTemple = 0;
        for (uint32 i = 0; i < BG_AC_NODE_ZERO; ++i)
            if (m_Nodes[i] == BG_AC_NODE_STATUS_ALLY_OCCUPIED)
                ++allyTemple;
            else if (m_Nodes[i] == BG_AC_NODE_STATUS_HORDE_OCCUPIED)
                ++hordeTemple;
        if (allyTemple == 3)
            GYSetAlly();
        else if (hordeTemple == 3)
            GYSetHorde();
        else
            GYSetNeutral();
    }

    UpdateWorldState(BG_AC_OP_OCCUPIED_BASES_ALLY, ally);
    UpdateWorldState(BG_AC_OP_OCCUPIED_BASES_HORDE, horde);
}

void BattleGroundAC::_NodeOccupied(uint32 node, Team team)
{
    uint32 capturedNodes = 0;
    for (uint32 i = 0; i < BG_AC_NODE_ZERO; ++i)
    {
        if (m_Nodes[node] == GetTeamIndexByTeamId(team) + BG_AC_NODE_TYPE_OCCUPIED && !m_NodeTimers[i])
            ++capturedNodes;
    }
    /*if (capturedNodes >= 3)
        CastSpellOnTeam(15062, team);*/
}

/* Invoked if a player used a banner as a gameobject */
void BattleGroundAC::EventPlayerClickedOnFlag(Player* source, GameObject* target_obj)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    uint32 event = (sBattleGroundMgr.GetGameObjectEventIndex(target_obj->GetGUIDLow())).event1;
    if (event >= BG_AC_NODES_MAX)                           // not a node
        return;

    BG_AC_Nodes node = BG_AC_Nodes(event);

    BattleGroundTeamIndex teamIndex = GetTeamIndexByTeamId(source->GetTeam());

    // Check if player really could use this banner, not cheated
    if (!(m_Nodes[node] == 0 || teamIndex == m_Nodes[node] % 2))
        return;

    /*uint32 killCredits[BG_AC_NODES_MAX] = {
        BG_AC_NODE_STACLES_CREDIT,
        BG_AC_NODE_BLACKSMITH_CREDIT,
        BG_AC_NODE_FARM_CREDIT,
        BG_AC_NODE_LUMBER_MILL_CREDIT,
        BG_AC_NODE_GOLD_MINE_CREDIT };
    source->KilledMonsterCredit(killCredits[node]);*/

    source->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);

    uint32 sound = 0;

    // 1-st CAP
    // If node is neutral, change to contested
    if (m_Nodes[node] == BG_AC_NODE_TYPE_NEUTRAL)
    {
        UpdatePlayerScore(source, SCORE_POINTS_ASSAULTED, 1);
        m_prevNodes[node] = m_Nodes[node];
        m_Nodes[node] = teamIndex + 1;
        // create new contested banner
        _CreateBanner(node, BG_AC_NODE_TYPE_CONTESTED, teamIndex);
        _SendNodeUpdate(node);
        if (event < BG_AC_NODE_ZERO)
            m_NodeTimers[node] = BG_AC_FLAG_CAPTURING_TIME;
        else
            m_NodeTimers[node] = BG_AC_TOWER_CAPTURING_TIME;

        if (teamIndex == 0)
            SendMessage2ToAll(LANG_BG_AC_NODE_CLAIMED, CHAT_MSG_BG_SYSTEM_ALLIANCE, source, _GetNodeNameId(node), LANG_BG_ALLY);
        else
            SendMessage2ToAll(LANG_BG_AC_NODE_CLAIMED, CHAT_MSG_BG_SYSTEM_HORDE, source, _GetNodeNameId(node), LANG_BG_HORDE);

        sound = BG_AC_SOUND_NODE_CLAIMED;
    }
    // If node is contested
    else if ((m_Nodes[node] == BG_AC_NODE_STATUS_ALLY_CONTESTED) || (m_Nodes[node] == BG_AC_NODE_STATUS_HORDE_CONTESTED))
    {
        // If last state is NOT occupied, change node to enemy-contested
        if (m_prevNodes[node] < BG_AC_NODE_TYPE_OCCUPIED)
        {
            UpdatePlayerScore(source, SCORE_POINTS_ASSAULTED, 1);
            m_prevNodes[node] = m_Nodes[node];
            m_Nodes[node] = teamIndex + BG_AC_NODE_TYPE_CONTESTED;
            // create new contested banner
            _CreateBanner(node, BG_AC_NODE_TYPE_CONTESTED, teamIndex);
            _SendNodeUpdate(node);
            if (event < BG_AC_NODE_ZERO)
                m_NodeTimers[node] = BG_AC_FLAG_CAPTURING_TIME;
            else
                m_NodeTimers[node] = BG_AC_TOWER_CAPTURING_TIME;

            if (teamIndex == BG_TEAM_ALLIANCE)
                SendMessage2ToAll(LANG_BG_AC_NODE_ASSAULTED, CHAT_MSG_BG_SYSTEM_ALLIANCE, source, _GetNodeNameId(node));
            else
                SendMessage2ToAll(LANG_BG_AC_NODE_ASSAULTED, CHAT_MSG_BG_SYSTEM_HORDE, source, _GetNodeNameId(node));
        }
        // If contested, change back to occupied
        else
        {
            UpdatePlayerScore(source, SCORE_POINTS_DEFENDED, 1);
            m_prevNodes[node] = m_Nodes[node];
            m_Nodes[node] = teamIndex + BG_AC_NODE_TYPE_OCCUPIED;
            // create new occupied banner
            _CreateBanner(node, BG_AC_NODE_TYPE_OCCUPIED, teamIndex);
            _SendNodeUpdate(node);
            m_NodeTimers[node] = 0;
            _NodeOccupied(node, (teamIndex == BG_TEAM_ALLIANCE) ? ALLIANCE : HORDE);

            if (teamIndex == BG_TEAM_ALLIANCE)
                SendMessage2ToAll(LANG_BG_AC_NODE_DEFENDED, CHAT_MSG_BG_SYSTEM_ALLIANCE, source, _GetNodeNameId(node));
            else
                SendMessage2ToAll(LANG_BG_AC_NODE_DEFENDED, CHAT_MSG_BG_SYSTEM_HORDE, source, _GetNodeNameId(node));
        }
        sound = (teamIndex == BG_TEAM_ALLIANCE) ? BG_AC_SOUND_NODE_ASSAULTED_ALLIANCE : BG_AC_SOUND_NODE_ASSAULTED_HORDE;
    }
    // If node is occupied, change to enemy-contested
    else
    {
        UpdatePlayerScore(source, SCORE_POINTS_ASSAULTED, 1);
        m_prevNodes[node] = m_Nodes[node];
        m_Nodes[node] = teamIndex + BG_AC_NODE_TYPE_CONTESTED;
        // create new contested banner
        _CreateBanner(node, BG_AC_NODE_TYPE_CONTESTED, teamIndex);
        _SendNodeUpdate(node);
        if (event < BG_AC_NODE_ZERO)
            m_NodeTimers[node] = BG_AC_FLAG_CAPTURING_TIME;
        else
            m_NodeTimers[node] = BG_AC_TOWER_CAPTURING_TIME;

        if (teamIndex == BG_TEAM_ALLIANCE)
            SendMessage2ToAll(LANG_BG_AC_NODE_ASSAULTED, CHAT_MSG_BG_SYSTEM_ALLIANCE, source, _GetNodeNameId(node));
        else
            SendMessage2ToAll(LANG_BG_AC_NODE_ASSAULTED, CHAT_MSG_BG_SYSTEM_HORDE, source, _GetNodeNameId(node));

        sound = (teamIndex == BG_TEAM_ALLIANCE) ? BG_AC_SOUND_NODE_ASSAULTED_ALLIANCE : BG_AC_SOUND_NODE_ASSAULTED_HORDE;
    }
    // If node is occupied again, send "X has taken the Y" msg.
    /*if (m_Nodes[node] >= BG_AC_NODE_TYPE_OCCUPIED)
    {
        if (teamIndex == BG_TEAM_ALLIANCE)
            SendMessage2ToAll(LANG_BG_AC_NODE_TAKEN, CHAT_MSG_BG_SYSTEM_ALLIANCE, nullptr, _GetNodeNameId(node), LANG_BG_ALLY);
        else
            SendMessage2ToAll(LANG_BG_AC_NODE_TAKEN, CHAT_MSG_BG_SYSTEM_HORDE, nullptr, _GetNodeNameId(node), LANG_BG_HORDE);
    }*/
    PlaySoundToAll(sound);
}

bool BattleGroundAC::SetupBattleGround()
{
    return true;
}

void BattleGroundAC::Reset()
{
    BattleGround::Reset();

    for (uint8 i = 0; i < BG_TEAMS_COUNT; ++i)
    {
        m_TeamScores[i] = 500;
        m_lastTick[i] = 0;
        m_HonorScoreTics[i] = 0;
        m_ReputationScoreTics[i] = 0;
    }
    m_mid_score = 50;
    m_mid_update_timer = 10000;
    m_near_end_update_timer = 10000;
    m_mid_neutral = true;
    m_mid_ally = false;
    m_mid_horde = false;
    m_GY_neutral = true;
    m_GY_ally = false;
    m_GY_horde = false;
    m_IsInformedNearVictory = false;
    m_IsInformedNearDefeat = false;
    bool isBGWeekend = BattleGroundMgr::IsBGWeekend(GetTypeID());
    m_HonorTics = (isBGWeekend) ? 10 : 15;
    //AC repweek
    m_ReputationTics = (isBGWeekend) ? 10 : 15;

    for (uint32 i = 0; i < BG_AC_NODE_ZERO; ++i)
    {
        m_Nodes[i] = 0;
        m_prevNodes[i] = 0;
        m_NodeTimers[i] = 0;

        // Temple nodes owned by neutral team at beginning
        m_ActiveEvents[i] = BG_AC_NODE_TYPE_NEUTRAL;
    }
    for (uint32 i = BG_AC_NODE_ZERO; i < BG_AC_NODE_GROMM; ++i)
    {
        m_Nodes[i] = 3;
        m_prevNodes[i] = 0;
        m_NodeTimers[i] = 0;
        //Spawn initial mobs
        SpawnEvent(i + 20, 1, true, true, 1);

        // Alliance tower nodes owned by Alliance team at beginning
        m_ActiveEvents[i] = BG_AC_NODE_STATUS_ALLY_OCCUPIED;
    }
    for (uint32 i = BG_AC_NODE_GROMM; i < BG_AC_NODES_MAX; ++i)
    {
        m_Nodes[i] = 4;
        m_prevNodes[i] = 0;
        m_NodeTimers[i] = 0;
        //Spawn initial mobs
        SpawnEvent(i + 20, 1, true, true, 1);

        // Horde tower nodes owned by Horde team at beginning
        m_ActiveEvents[i] = BG_AC_NODE_STATUS_HORDE_OCCUPIED;
    }

    //Mid
    SpawnEvent(13, 0, true, true, 1);
    SpawnEvent(14, 0, true, true, 1);

    //Bosses
    SpawnEvent(33, 1, true, true, 1);
    SpawnEvent(34, 1, true, true, 1);
}

void BattleGroundAC::EndBattleGround(Team winner)
{
    bool Relief = false;
    bool isBGWeekend = BattleGroundMgr::IsBGWeekend(GetTypeID());
    //win reward
    if (winner == ALLIANCE)
    {
        if (isBGWeekend)
            RewardHonorToTeam(1720, ALLIANCE);
        RewardHonorToTeam(920, ALLIANCE);
        RewardHonorToTeam(410, HORDE); //temp

        if (m_TeamScores[BG_TEAM_HORDE] > 700 || m_TeamScores[BG_TEAM_ALLIANCE] < 200)
            Relief = true;
        else if (m_TeamScores[BG_TEAM_HORDE] > 600 || m_TeamScores[BG_TEAM_ALLIANCE] < 300)
        {
            if (urand(0, 2) > 0)
                Relief = true;
        }
        else if (m_TeamScores[BG_TEAM_HORDE] > 500 || m_TeamScores[BG_TEAM_ALLIANCE] < 500)
        {
            if (urand(0, 3) > 0)
                Relief = true;
        }
        else if (GetStartTime() > BG_STOP_DELAY_40M)
        {
            Relief = true;
        }
        else if (GetStartTime() > BG_STOP_DELAY_30M)
        {
            if (urand(0, 2) > 0)
                Relief = true;
        }
    }
    if (winner == HORDE)
    {
        if (isBGWeekend)
            RewardHonorToTeam(1720, HORDE);
        RewardHonorToTeam(920, HORDE);
        RewardHonorToTeam(410, ALLIANCE); //temp

        if (m_TeamScores[BG_TEAM_ALLIANCE] > 700 || m_TeamScores[BG_TEAM_HORDE] < 200)
            Relief = true;
        else if (m_TeamScores[BG_TEAM_ALLIANCE] > 600 || m_TeamScores[BG_TEAM_HORDE] < 300)
        {
            if (urand(0, 3) > 0)
                Relief = true;
        }
        else if (m_TeamScores[BG_TEAM_ALLIANCE] > 500 || m_TeamScores[BG_TEAM_HORDE] < 500)
        {
            if (urand(0, 3) > 0)
                Relief = true;
        }
        else if (GetStartTime() > BG_STOP_DELAY_40M)
        {
            Relief = true;
        }
        else if (GetStartTime() > BG_STOP_DELAY_30M)
        {
            if (urand(0, 2) > 0)
                Relief = true;
        }
    }

    // Que Marks reward
    for (BattleGroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
    {
        Player* plr = sObjectMgr.GetPlayer(itr->first);

        if (!plr)
        {
            sLog.outError("BattleGround:RewardHonorToTeam: %s not found!", itr->first.GetString().c_str());
            continue;
        }

        Team Pteam = itr->second.PlayerTeam;
        if (!Pteam) Pteam = plr->GetTeam();

        if (Pteam == winner)
        {
            if (SpellAuraHolder* SpellHolder = plr->GetSpellAuraHolder(36333))
            {
                int32 SpellStacks = SpellHolder->GetStackAmount();

                if (SpellStacks > 0)
                {
                    if (SpellStacks > 50)
                        SpellStacks = 50;

                    int32 honor = plr->getLevel() * frand(6.0f, 8.0f) * SpellStacks;

                    plr->CastCustomSpell(plr, 34769, nullptr, &SpellStacks, nullptr, true, nullptr);    //Marks
                    plr->CastCustomSpell(plr, 31418, &honor, nullptr, nullptr, true, nullptr);          // Honor

                    if (plr->GetTeam() == HORDE)
                        plr->GetReputationMgr().ModifyReputation(sObjectMgr.GetFactionEntry(955), 20 * SpellStacks);
                    else
                        plr->GetReputationMgr().ModifyReputation(sObjectMgr.GetFactionEntry(956), 20 * SpellStacks);

                    plr->RemoveAurasDueToSpell(36333);
                }
            }
        }
    }

    BattleGround::EndBattleGround(winner, Relief);
}

WorldSafeLocsEntry const* BattleGroundAC::GetClosestGraveYard(Player* player)
{
    WorldSafeLocsEntry const* good_entry = nullptr;
    WorldSafeLocsEntry const* entryBaseGY = sWorldSafeLocsStore.LookupEntry(player->GetTeam() == ALLIANCE ? 944 : 945);
    WorldSafeLocsEntry const* entryMidGY = sWorldSafeLocsStore.LookupEntry(946);
    // repop players at the entrance GY if BG is not started yet
    if (GetStatus() != STATUS_IN_PROGRESS)
    {
        if (good_entry = sWorldSafeLocsStore.LookupEntry(player->GetTeam() == ALLIANCE ? 269 : 49))
        {
            return good_entry;
        }
    }
    else
    {
        if (!m_GY_neutral)
        {
            if (player->GetTeam() == ALLIANCE)
            {
                float dBx = player->GetPositionX() - entryBaseGY->x;
                float dBy = player->GetPositionY() - entryBaseGY->y;
                float dBz = player->GetPositionZ() - entryBaseGY->z;
                float dMx = player->GetPositionX() - entryMidGY->x;
                float dMy = player->GetPositionY() - entryMidGY->y;
                float dMz = player->GetPositionZ() - entryMidGY->z;
                float distanceToBase = sqrt((dBx * dBx) + (dBy * dBy) + (dBz * dBz));
                float distanceToMid = sqrt((dMx * dMx) + (dMy * dMy) + (dMz * dMz));

                if (distanceToMid < distanceToBase && m_GY_ally)
                    good_entry = entryMidGY;
                else
                    good_entry = entryBaseGY;
            }
            else if (player->GetTeam() == HORDE)
            {
                float dBx = player->GetPositionX() - entryBaseGY->x;
                float dBy = player->GetPositionY() - entryBaseGY->y;
                float dBz = player->GetPositionZ() - entryBaseGY->z;
                float dMx = player->GetPositionX() - entryMidGY->x;
                float dMy = player->GetPositionY() - entryMidGY->y;
                float dMz = player->GetPositionZ() - entryMidGY->z;
                float distanceToBase = sqrt((dBx * dBx) + (dBy * dBy) + (dBz * dBz));
                float distanceToMid = sqrt((dMx * dMx) + (dMy * dMy) + (dMz * dMz));

                if (distanceToMid < distanceToBase && m_GY_horde)
                    good_entry = entryMidGY;
                else
                    good_entry = entryBaseGY;
            }
        }
        else
        {
            if (player->GetTeam() == ALLIANCE)
                good_entry = sWorldSafeLocsStore.LookupEntry(944);
            else if (player->GetTeam() == HORDE)
                good_entry = sWorldSafeLocsStore.LookupEntry(945);
        }

        if (good_entry)
            return good_entry;
    }

    if (!good_entry)
        good_entry = sWorldSafeLocsStore.LookupEntry(player->GetTeam() == ALLIANCE ? 269 : 49);

    return good_entry;
}

void BattleGroundAC::UpdatePlayerScore(Player* Source, uint32 type, uint32 value)
{
    BattleGroundScoreMap::iterator itr = m_PlayerScores.find(Source->GetObjectGuid());
    if (itr == m_PlayerScores.end())                          // player not found...
        return;

    switch (type)
    {
    case SCORE_POINTS_ASSAULTED:
        ((BattleGroundACScore*)itr->second)->PointsAssaulted += value;
        break;
    case SCORE_POINTS_DEFENDED:
        ((BattleGroundACScore*)itr->second)->PointsDefended += value;
        break;
    default:
        BattleGround::UpdatePlayerScore(Source, type, value);
        break;
    }
}
