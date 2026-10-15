

#include "Object.h"
#include "Player.h"
#include "BattleGround.h"
#include "BattleGroundBRA.h"
#include "Creature.h"
#include "GameObject.h"
#include "BattleGroundMgr.h"
#include "Language.h"
#include "Util.h"
#include "WorldPacket.h"
#include "MapManager.h"
#include "ObjectMgr.h"
#include "scriptPCH.h"

BattleGroundBRA::BattleGroundBRA()
{
    m_StartMessageIds[BG_STARTING_EVENT_FIRST] = 0;
    m_StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_ARENA_START_ONE_MINUTE;
    m_StartMessageIds[BG_STARTING_EVENT_THIRD] = LANG_ARENA_START_HALF_MINUTE;
    m_StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_ARENA_START_HAS_BEGUN;
}

BattleGroundBRA::~BattleGroundBRA()
{
}

void BattleGroundBRA::Update(uint32 diff)
{
    if (GetStatus() == STATUS_WAIT_JOIN)
    {
        UpdateWorldState(ARENA_TEAM_A_SCORE, Team_A_score);
        UpdateWorldState(ARENA_TEAM_B_SCORE, Team_B_score);
        UpdateWorldState(ARENA_WINS_TILL_WIN, BG_BRA_Wins_till_Win[GetBracketId()]);
        UpdateWorldState(ARENA_TEAM_A_PLAYERS_LEFT, team_A_alive);
        UpdateWorldState(ARENA_TEAM_B_PLAYERS_LEFT, team_B_alive);

        UpdateWorldState(ARENA_ROUND_TIME_MIN, Round_Time_Minutes);
        UpdateWorldState(ARENA_ROUND_NUMBER, Round_Number);
    }
    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        //Initial check
        if (!ArenaStarted)
        {
            ArenaStarted = true;
            if (!sBattleGroundMgr.isTesting())
            {
                if (team_A_total == 0 || team_B_total == 0 || team_A_total != team_B_total)
                {
                    EndBattleGround(TEAM_NONE);
                    return;
                }
            }
        }

        // Check End Conditions
        if (periodic_check_alive_status < diff)
        {
            CheckEndConditions(false);
            periodic_check_alive_status = 10000;
        }
        else
            periodic_check_alive_status -= diff;

        if (recheck_alive_status < diff)
        {
            CheckEndConditions(true);
            recheck_alive_status = 30 * MINUTE * IN_MILLISECONDS;
        }
        else
            recheck_alive_status -= diff;

        //Round timer
        Round_Timer -= diff;

        //Overtime penalty
        if (Round_Timer < 300000)
        {
            if (Overtime_Timer < diff)
            {
                CastSpellOnTeam(SPELL_OVERTIME, HORDE);
                CastSpellOnTeam(SPELL_OVERTIME, ALLIANCE);

                Overtime_Timer = 6000;
            }
            else
                Overtime_Timer -= diff;
        }

        //New rounds
        if (New_Round_Timer < diff)
        {
            StartingEventOpenDoors();
            PlaySoundToAll(SOUND_BG_START);
            PSendMessageToAll(LANG_ARENA_ROUND_NUMBER_X_BEGINS, CHAT_MSG_BG_SYSTEM_NEUTRAL, nullptr, Round_Number);

            New_Round_Timer = 30 * MINUTE * IN_MILLISECONDS;
        }
        else
            New_Round_Timer -= diff;

        if (Announce_Timer_10s < diff)
        {
            SendMessageToAll(LANG_ARENA_10_SEC_TILL_NEXT_ROUND, CHAT_MSG_BG_SYSTEM_NEUTRAL);
            Announce_Timer_10s = 30 * MINUTE * IN_MILLISECONDS;
        }
        else
            Announce_Timer_10s -= diff;

        //Round time limit
        if (Round_Limit_Timer < diff)
        {
            uint32 roundtimer_insec = Round_Timer / 1000;

            Round_Limit_Total_Seconds -= 1;
            Round_Time_Minutes = roundtimer_insec / 60;
            Round_Time_SecondsDigit1 = (roundtimer_insec - Round_Time_Minutes * 60) / 10;
            Round_Time_SecondsDigit2 = roundtimer_insec % 10;

            UpdateWorldState(ARENA_ROUND_TIME_MIN, Round_Time_Minutes);
            UpdateWorldState(ARENA_ROUND_TIME_SEC_DIG1, Round_Time_SecondsDigit1);
            UpdateWorldState(ARENA_ROUND_TIME_SEC_DIG2, Round_Time_SecondsDigit2);

            Round_Limit_Timer = 1000;

            if (roundtimer_insec <= 0)
            {
                Round_Limit_Timer = 30 * MINUTE * IN_MILLISECONDS;
                ArenaTimeOut();
            }
        }
        else
            Round_Limit_Timer -= diff;

        if (Arena_ResetPlayers_Delay_Timer < diff)
        {
            Round_Number += 1;
            Round_Time_Minutes = ROUND_DURATION_LIMIT / 60;
            Round_Time_SecondsDigit1 = (ROUND_DURATION_LIMIT - Round_Time_Minutes * 60) / 10;
            Round_Time_SecondsDigit2 = ROUND_DURATION_LIMIT % 10;

            UpdateWorldState(ARENA_ROUND_NUMBER, Round_Number);
            UpdateWorldState(ARENA_ROUND_TIME_MIN, Round_Time_Minutes);
            UpdateWorldState(ARENA_ROUND_TIME_SEC_DIG1, Round_Time_SecondsDigit1);
            UpdateWorldState(ARENA_ROUND_TIME_SEC_DIG2, Round_Time_SecondsDigit2);

            ResetArenaPlayers();
            Arena_ResetPlayers_Delay_Timer = 30 * MINUTE * IN_MILLISECONDS;
        }
        else
            Arena_ResetPlayers_Delay_Timer -= diff;
    }
    // Execute this at the end, since it can delete the BattleGround object!
    BattleGround::Update(diff);
}

void BattleGroundBRA::StartingEventCloseDoors()
{
    CloseDoorEvent(BG_EVENT_DOOR);
}

void BattleGroundBRA::StartingEventOpenDoors()
{
    OpenDoorEvent(BG_EVENT_DOOR);
}

void BattleGroundBRA::AddPlayer(Player* plr)
{
    BattleGround::AddPlayer(plr);
    //create score and add it to map, default values are set in the constructor
    BattleGroundBRAScore* sc = new BattleGroundBRAScore;

    m_PlayerScores[plr->GetObjectGuid()] = sc;
    UpdatePlayerScore(plr, SCORE_REPUTATION, plr->GetReputationMgr().GetReputation(REPUTATION_CRIMSON_RING));


    //-Buffs
    plr->RemoveAllAurasOnDeath();
    //-Stealth
    plr->AddAura(35437);

    //Apply Flag Team-A or Team-B
    float x, y, z, o;
    GetTeamStartLoc(ALLIANCE, x, y, z, o);

    if (plr->IsWithinDist3d(x, y, z, 20.0f))
    {
        plr->CastSpell(plr, SPELL_TEAM_A, true);
        team_A_total += 1;
        team_A_alive += 1;
        team_A_Total_rep += plr->GetReputationMgr().GetReputation(REPUTATION_CRIMSON_RING);
        UpdateWorldState(ARENA_TEAM_A_PLAYERS_LEFT, team_A_alive);
    }
    else
    {
        plr->CastSpell(plr, SPELL_TEAM_B, true);
        team_B_total += 1;
        team_B_alive += 1;
        team_B_Total_rep += plr->GetReputationMgr().GetReputation(REPUTATION_CRIMSON_RING);
        UpdateWorldState(ARENA_TEAM_B_PLAYERS_LEFT, team_B_alive);
    }

    UpdateWorldStateForPlayer(ARENA_TEAM_A_SCORE, Team_A_score, plr);
    UpdateWorldStateForPlayer(ARENA_TEAM_B_SCORE, Team_B_score, plr);
    UpdateWorldStateForPlayer(ARENA_WINS_TILL_WIN, BG_BRA_Wins_till_Win[GetBracketId()], plr);
    UpdateWorldStateForPlayer(ARENA_ROUND_TIME_MIN, Round_Limit_Total_Seconds / 60, plr);
    UpdateWorldStateForPlayer(ARENA_ROUND_TIME_SEC_DIG1, Round_Limit_Total_Seconds % 10, plr);
    UpdateWorldStateForPlayer(ARENA_ROUND_TIME_SEC_DIG2, Round_Limit_Total_Seconds / 10, plr);
    UpdateWorldStateForPlayer(ARENA_ROUND_NUMBER, Round_Number, plr);
    UpdateWorldStateForPlayer(ARENA_MAX_ROUNDS, BG_BRA_Max_Rounds[GetBracketId()], plr);
}

void BattleGroundBRA::RemovePlayer(Player* plr, ObjectGuid guid)
{
    if (plr)
    {
        if (plr->HasAura(SPELL_TEAM_A) || plr->HasAura(SPELL_TEAM_B))
        {
            if (GetStatus() != STATUS_WAIT_LEAVE)
            {
                uint32 plrRep = plr->GetReputationMgr().GetReputation(REPUTATION_CRIMSON_RING);
                uint32 enemyTeamTotalRep = plrRep / 2;

                CalcRepRepChange(plr, plrRep, enemyTeamTotalRep, false);
            }

            plr->RemoveAurasDueToSpell(SPELL_OVERTIME);

            if (GetStatus() == STATUS_IN_PROGRESS)
            {
                CheckEndConditions(false);
                periodic_check_alive_status = 10000;
            }
        }
    }
}

void BattleGroundBRA::ArenaTimeOut()
{
    if (team_A_alive != team_B_alive)
    {
        if (team_A_alive > team_B_alive)
            FinishRound(ALLIANCE);
        else if (team_B_alive > team_A_alive)
            FinishRound(HORDE);
    }
    else
    {
        float team_A_HP_perc = 0.0f;
        float team_B_HP_perc = 0.0f;
        for (BattleGroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
        {
            Player* plr = sObjectMgr.GetPlayer(itr->first);

            if (!plr)
                continue;

            if (plr->isAlive())
            {
                if (plr->HasAura(SPELL_TEAM_A))
                {
                    team_A_HP_perc += (float(plr->GetHealth()) / plr->GetMaxHealth()) * 100;
                }
                else if (plr->HasAura(SPELL_TEAM_B))
                {
                    team_B_HP_perc += (float(plr->GetHealth()) / plr->GetMaxHealth()) * 100;
                }
            }
        }
        
        if (team_A_HP_perc > team_B_HP_perc)
            FinishRound(ALLIANCE);
        else if (team_A_HP_perc < team_B_HP_perc)
            FinishRound(HORDE);
        else
            FinishRound(TEAM_NONE);
    }
}

void BattleGroundBRA::CheckEndConditions(bool isrecheck)
{
    team_A_alive_temp = 0;
    team_B_alive_temp = 0;

    for (BattleGroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
    {
        Player* plr = sObjectMgr.GetPlayer(itr->first);

        if (!plr)
            continue;

        if (plr->isAlive())
        {
            if (plr->HasAura(SPELL_TEAM_A))
                team_A_alive_temp += 1;
            else if (plr->HasAura(SPELL_TEAM_B))
                team_B_alive_temp += 1;
        }
        else if (!isrecheck)
        {
            if (plr->GetResurrectionSpellId() != 0 || plr->HasAura(33848))
            {
                if (plr->HasAura(SPELL_TEAM_A))
                    team_A_alive_temp += 1;
                else if (plr->HasAura(SPELL_TEAM_B))
                    team_B_alive_temp += 1;

                recheck_alive_status = 30000;
                periodic_check_alive_status += 30000;
            }
        }
    }

    team_A_alive = team_A_alive_temp;
    team_B_alive = team_B_alive_temp;

    if (sBattleGroundMgr.isTesting())
    {
        if (team_A_alive == 0)
            team_A_alive = team_B_alive;
        if (team_B_alive == 0)
            team_B_alive = team_A_alive;
    }
    UpdateWorldState(ARENA_TEAM_A_PLAYERS_LEFT, team_A_alive);
    UpdateWorldState(ARENA_TEAM_B_PLAYERS_LEFT, team_B_alive);



    if (team_A_alive == 0)
        FinishRound(HORDE);
    else if (team_B_alive == 0)
        FinishRound(ALLIANCE);
}

void BattleGroundBRA::FinishRound(Team winner)
{
    bool NextArenaInc = true;
    //Add score
    if (winner == ALLIANCE)
    {
        Team_A_score += 1;
        UpdateWorldState(ARENA_TEAM_A_SCORE, Team_A_score);
    }
    else if (winner == HORDE)
    {
        Team_B_score += 1;
        UpdateWorldState(ARENA_TEAM_B_SCORE, Team_B_score);
    }

    if (winner != TEAM_NONE)
    {
        //Best of 1/3/5
        if (Team_A_score >= BG_BRA_Wins_till_Win[GetBracketId()]) 
        {
            EndBattleGround(ALLIANCE);
            NextArenaInc = false;
        }
        else if (Team_B_score >= BG_BRA_Wins_till_Win[GetBracketId()])
        {
            EndBattleGround(HORDE);
            NextArenaInc = false;
        }
    }
    else
    {
        if (Team_A_score >= BG_BRA_Wins_till_Win[GetBracketId()] || Team_B_score >= BG_BRA_Wins_till_Win[GetBracketId()])
        {
            EndBattleGround(TEAM_NONE);
            NextArenaInc = false;
        }

    }

    if (NextArenaInc)
    {
        // Round winner announcement
        if (winner == ALLIANCE)
        {
            SendMessageToAll(LANG_ARENA_TEAM_A_ROUND_WIN, CHAT_MSG_BG_SYSTEM_NEUTRAL);
        }
        else if (winner == HORDE)
        {
            SendMessageToAll(LANG_ARENA_TEAM_B_ROUND_WIN, CHAT_MSG_BG_SYSTEM_NEUTRAL);
        }
        else
        {
            SendMessageToAll(LANG_ARENA_DRAW_ROUND, CHAT_MSG_BG_SYSTEM_NEUTRAL);
        }

        PlaySoundToAll(ARENA_BRA_ROUND_END);
        //Reset Variables
        periodic_check_alive_status = 70 * IN_MILLISECONDS;
        recheck_alive_status = 30 * MINUTE * IN_MILLISECONDS;
        New_Round_Timer = 70 * IN_MILLISECONDS;
        Overtime_Timer = 0;
        Round_Limit_Timer = 70 * IN_MILLISECONDS;
        Announce_Timer_10s = 60 * IN_MILLISECONDS;

        Round_Timer = ROUND_DURATION_LIMIT * IN_MILLISECONDS + New_Round_Timer;

        Arena_ResetPlayers_Delay_Timer = 10000;
    }
}

void BattleGroundBRA::ResetArenaPlayers()
{
    StartingEventCloseDoors();
    team_A_alive = 0;
    team_B_alive = 0;
    Team team1 = urand(0, 1) == 0 ? ALLIANCE : HORDE;
    Team team2 = team1 == ALLIANCE ? HORDE : ALLIANCE;
    for (BattleGroundPlayerMap::iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
    {
        Player* plr = sObjectMgr.GetPlayer(itr->first);
        if (!plr)
            continue;

        // should remove spirit of redemption
        if (plr->HasAuraType(SPELL_AURA_SPIRIT_OF_REDEMPTION))
            plr->RemoveSpellsCausingAura(SPELL_AURA_MOD_SHAPESHIFT);

        if (!plr->isAlive())
        {
            plr->ResurrectPlayer(1.0f);
            plr->SpawnCorpseBones();
        }
        else
        {
            plr->CombatStop();
            plr->getHostileRefManager().deleteReferences();
        }


        //Teleport to spawn
        float x, y, z, O;
        if (plr->HasAura(SPELL_TEAM_A))
        {
            GetTeamStartLoc(team1, x, y, z, O);
            x += frand(-3.0, 3.0f);
            y += frand(-3.0, 3.0f);

            team_A_alive += 1;
            UpdateWorldState(ARENA_TEAM_A_PLAYERS_LEFT, team_A_alive);
        }
        else if (plr->HasAura(SPELL_TEAM_B))
        {
            GetTeamStartLoc(team2, x, y, z, O);
            x += frand(-3.0, 3.0f);
            y += frand(-3.0, 3.0f);

            team_B_alive += 1;
            UpdateWorldState(ARENA_TEAM_B_PLAYERS_LEFT, team_B_alive);
        }

        plr->RemoveAurasDueToSpell(SPELL_OVERTIME);
        plr->TeleportTo(plr->GetMapId(), x, y, z, O);
        plr->CombatStopWithPets(true);

        // Preparations buff
        CastSpellOnTeam(SPELL_PREPARATIONS, HORDE);
        CastSpellOnTeam(SPELL_PREPARATIONS, ALLIANCE);
        // Dispell Stealth
        CastSpellOnTeam(SPELL_STEALTH_IMMUNITY, HORDE);
        CastSpellOnTeam(SPELL_STEALTH_IMMUNITY, ALLIANCE);
        PlaySoundToAll(ARENA_BRA_PLAYER_RESET);
    }
}

void BattleGroundBRA::HandleKillPlayer(Player* player, Player* killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    if (killer && player->GetGroup() != killer->GetGroup())
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
        /*player->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);*/
    }

    CheckEndConditions(false);
}

void BattleGroundBRA::CalcRepRepChange(Player* plr, uint32 plrTotalRep, uint32 enemyTeamTotalRep, bool IsWinner)
{
    float BaseGain = 100.0f;
    float BaseLost = 5.0f;  //10
    float RepChange = 0.0f;
    float RepDelta = 0.0f;

    ReputationRank rank = plr->GetReputationRank(REPUTATION_CRIMSON_RING);
    if (rank == REP_EXALTED)
    {
        BaseGain = 200.0f;
        BaseLost = 75.0f;
    }
    else if (rank == REP_REVERED)
    {
        BaseGain = 500.0f;
        BaseLost = 400.0f;
    }
    else if (rank >= REP_HONORED)
    {
        BaseGain = 300.0f;
        BaseLost = 100.0f;  //150
    }
    else if (rank >= REP_FRIENDLY)
    {
        BaseGain = 150.0f;
        BaseLost = 15.0f;  //30
    }

    if (IsWinner)
    {
        RepDelta = (float(plrTotalRep) - float(enemyTeamTotalRep));
        RepChange = BaseGain;
    }
    else
    {
        RepDelta = (float(enemyTeamTotalRep) - float(plrTotalRep));
        RepChange = -BaseLost;
    }

    // ELO system
    RepChange *= 1.0f / (1.0f + std::log(10.0f) * RepDelta / 65000.0f);

    if (plr->GetMapId() == 38)
        RepChange /= 2;


    FactionEntry const* factionEntry = sObjectMgr.GetFactionEntry(REPUTATION_CRIMSON_RING);
    if (!factionEntry)
        return;

    if (plrTotalRep + RepChange < 0)
        RepChange = -plrTotalRep;

    plr->GetReputationMgr().ModifyReputation(factionEntry, RepChange);
    UpdatePlayerScore(plr, SCORE_REPUTATION_CHANGE, RepChange);
    UpdatePlayerScore(plr, SCORE_REPUTATION, plr->GetReputationMgr().GetReputation(REPUTATION_CRIMSON_RING));

}

bool BattleGroundBRA::SetupBattleGround()
{
    return true;
}

void BattleGroundBRA::Reset()
{
    BattleGround::Reset();

    ArenaStarted = false;
    team_A_total = 0;
    team_B_total = 0;
    team_A_alive = 0;
    team_B_alive = 0;
    team_A_alive_temp = 0;
    team_B_alive_temp = 0;
    periodic_check_alive_status = 0;
    recheck_alive_status = 30 * MINUTE * IN_MILLISECONDS;
    team_A_Total_rep = 0;
    team_B_Total_rep = 0;

    Team_A_score = 0;
    Team_B_score = 0;
    Round_Number = 1;
    Round_Timer = ROUND_DURATION_LIMIT * IN_MILLISECONDS;
    New_Round_Timer = 30 * MINUTE * IN_MILLISECONDS;
    Announce_Timer_10s = 30 * MINUTE * IN_MILLISECONDS;
    Arena_ResetPlayers_Delay_Timer = 30 * MINUTE * IN_MILLISECONDS;
    Overtime_Timer = 0;
    Round_Limit_Timer = 0;
    Round_Limit_Total_Seconds = ROUND_DURATION_LIMIT;
    Round_Time_Minutes = ROUND_DURATION_LIMIT / 60;
}

void BattleGroundBRA::ArenaForcedEnd()
{
    if (Team_A_score > Team_B_score)
    {
        Team_A_score = BG_BRA_Wins_till_Win[GetBracketId()] - 1;
        FinishRound(ALLIANCE);
    }
    else if (Team_A_score < Team_B_score)
    {
        Team_B_score = BG_BRA_Wins_till_Win[GetBracketId()] - 1;
        FinishRound(HORDE);
    }
    else
    {
        if (urand(0,1) == 0)
        {
            Team_A_score = BG_BRA_Wins_till_Win[GetBracketId()] - 1;
            FinishRound(ALLIANCE);
        }
        else
        {
            Team_B_score = BG_BRA_Wins_till_Win[GetBracketId()] - 1;
            FinishRound(HORDE);
        }
    }
}

void BattleGroundBRA::EndBattleGround(Team winner)
{
    bool Relief = false;
    uint32 HonorMod = 1;

    //rep rewards
    for (BattleGroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
    {
        Player* plr = sObjectMgr.GetPlayer(itr->first);

        if (!plr)
        {
            itr = m_Players.erase(itr);
            continue;
        }

        plr->RemoveAurasDueToSpell(SPELL_OVERTIME);

        bool plrIsWinner = false;
        uint32 enemyTeamTotalRep = 0;
        uint32 plrRep = plr->GetReputationMgr().GetReputation(REPUTATION_CRIMSON_RING);

        if (plr->HasAura(SPELL_TEAM_A))
        {
            enemyTeamTotalRep = team_B_Total_rep / (team_B_total == 0 ? 1 : team_B_total);

            if (winner == ALLIANCE)
                plrIsWinner = true;
        }
        else if (plr->HasAura(SPELL_TEAM_B))
        {
            enemyTeamTotalRep = team_A_Total_rep / (team_A_total == 0 ? 1 : team_A_total);

            if (winner == HORDE)
                plrIsWinner = true;
        }

        if (winner != TEAM_NONE)
        {
            if (plrIsWinner)
            {
                CalcRepRepChange(plr, plrRep, enemyTeamTotalRep, true);
            }
            else
            {
                CalcRepRepChange(plr, plrRep, enemyTeamTotalRep, false);
            }
        }

        if (plr->GetMapId() == 38)
            HonorMod = 2;
    }

    if (winner == ALLIANCE)
    {
        RewardHonorToArenaTeam(BG_BRA_Winner_Bonus_Honor[GetBracketId()] / HonorMod, true);
        RewardHonorToArenaTeam(BG_BRA_Looser_Bonus_Honor[GetBracketId()] / HonorMod, false);

    }
    if (winner == HORDE)
    {
        RewardHonorToArenaTeam(BG_BRA_Winner_Bonus_Honor[GetBracketId()] / HonorMod, false);
        RewardHonorToArenaTeam(BG_BRA_Looser_Bonus_Honor[GetBracketId()] / HonorMod, true);
    }

    BattleGround::EndBattleGround(winner, Relief);
}

void BattleGroundBRA::UpdatePlayerScore(Player* Source, uint32 type, uint32 value)
{
    BattleGroundScoreMap::iterator itr = m_PlayerScores.find(Source->GetObjectGuid());
    if (itr == m_PlayerScores.end())                          // player not found...
        return;

    switch (type)
    {
    case SCORE_REPUTATION:
        ((BattleGroundBRAScore*)itr->second)->PReputation = value;
        break;
    case SCORE_REPUTATION_CHANGE:
        ((BattleGroundBRAScore*)itr->second)->PRepChange = value;
        break;
    default:
        BattleGround::UpdatePlayerScore(Source, type, value);
        break;
    }
}

void BattleGroundBRA::FillInitialWorldStates(WorldPacket& data, uint32& count)
{
    FillInitialWorldState(data, count, ARENA_TEAM_A_PLAYERS_LEFT, 1110);
    FillInitialWorldState(data, count, ARENA_TEAM_B_PLAYERS_LEFT, 1110);
    FillInitialWorldState(data, count, ARENA_ROUND_NUMBER, 1);

    FillInitialWorldState(data, count, ARENA_ROUND_TIME_MIN, Round_Time_Minutes);
    FillInitialWorldState(data, count, ARENA_ROUND_TIME_SEC_DIG1, Round_Time_SecondsDigit1);
    FillInitialWorldState(data, count, ARENA_ROUND_TIME_SEC_DIG2, Round_Time_SecondsDigit2);
    FillInitialWorldState(data, count, ARENA_TEAM_A_SCORE, 1110);
    FillInitialWorldState(data, count, ARENA_TEAM_B_SCORE, 1110);

    FillInitialWorldState(data, count, ARENA_MAX_ROUNDS, BG_BRA_Max_Rounds[GetBracketId()]);
    FillInitialWorldState(data, count, ARENA_WINS_TILL_WIN, BG_BRA_Wins_till_Win[GetBracketId()]);
}
