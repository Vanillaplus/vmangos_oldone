

#ifndef __BATTLEGROUNDBRA_H
#define __BATTLEGROUNDBRA_H

#include "Common.h"
#include "BattleGround.h"
#include "World.h"

enum BG_BRA_WorldStates
{
    ARENA_TEAM_A_PLAYERS_LEFT = 2601,
    ARENA_TEAM_B_PLAYERS_LEFT = 2602,
    ARENA_ROUND_NUMBER        = 2603,
    ARENA_MAX_ROUNDS          = 2604,
    ARENA_ROUND_TIME_MIN      = 2605,
    ARENA_ROUND_TIME_SEC_DIG1 = 2606,
    ARENA_TEAM_A_SCORE        = 2607,
    ARENA_TEAM_B_SCORE        = 2608,
    ARENA_WINS_TILL_WIN       = 2609,
    ARENA_ROUND_TIME_SEC_DIG2 = 2610,
};

enum ARENA_BRA_VALUES
{
    ROUND_DURATION_LIMIT    = 600,
    REPUTATION_CRIMSON_RING = 958,
    SPELL_PREPARATIONS      = 34495,
    SPELL_TEAM_A            = 35430,
    SPELL_TEAM_B            = 35431,
    SPELL_OVERTIME          = 35434,
    SPELL_STEALTH_IMMUNITY  = 35437,
};

enum BG_BRA_Sounds
{
    ARENA_BRA_ROUND_END     = 8213,
    ARENA_BRA_PLAYER_RESET  = 8232,
};

const uint32 BG_BRA_Max_Rounds[MAX_BATTLEGROUND_BRACKETS] = { 1, 3, 5};
const uint32 BG_BRA_Wins_till_Win[MAX_BATTLEGROUND_BRACKETS] = { 1, 2, 3};
const uint32 BG_BRA_Winner_Bonus_Honor[MAX_BATTLEGROUND_BRACKETS] = { 132, 264, 396 };
const uint32 BG_BRA_Looser_Bonus_Honor[MAX_BATTLEGROUND_BRACKETS] = { 66, 132, 198 };

class BattleGroundBRAScore : public BattleGroundScore
{
public:
    BattleGroundBRAScore() : PReputation(0), PRepChange(0) {};
    virtual ~BattleGroundBRAScore() {};
    uint32 PReputation;
    uint32 PRepChange;
};

class BattleGroundBRA : public BattleGround
{
    friend class BattleGroundMgr;

public:
    BattleGroundBRA();
    ~BattleGroundBRA();

    void Update(uint32 diff);
    void AddPlayer(Player* plr);
    virtual void StartingEventCloseDoors();
    virtual void StartingEventOpenDoors();
    void RemovePlayer(Player* plr, ObjectGuid guid);
    virtual void Reset();
    virtual void ArenaForcedEnd();
    void EndBattleGround(Team winner);
    virtual void FillInitialWorldStates(WorldPacket& data, uint32& count);
    /*virtual WorldSafeLocsEntry const* GetClosestGraveYard(Player* player);*/

    /* Scorekeeping */
    virtual void UpdatePlayerScore(Player* Source, uint32 type, uint32 value);
    void HandleKillPlayer(Player* player, Player* killer);
    void CheckEndConditions(bool isrecheck);
    void ArenaTimeOut();
    void FinishRound(Team winner);
    void ResetArenaPlayers();
    void CalcRepRepChange(Player* plr, uint32 plrTotalRep, uint32 enemyTeamTotalRep, bool IsWinner);
    bool SetupBattleGround();

private:
    uint32              team_A_total;
    uint32              team_B_total;
    uint32              team_A_alive;
    uint32              team_B_alive;
    uint32              team_A_alive_temp;
    uint32              team_B_alive_temp;
    uint32              periodic_check_alive_status;
    uint32              recheck_alive_status;
    uint32              team_A_Total_rep;
    uint32              team_B_Total_rep;
    uint32              Team_A_score;
    uint32              Team_B_score;
    uint32              Max_Rounds;
    uint32              Wins_till_Win;
    uint32              Round_Number;
    uint32              Round_Timer;
    uint32              New_Round_Timer;
    uint32              Announce_Timer_10s;
    uint32              Overtime_Timer;
    uint32              Round_Limit_Timer;
    uint32              Round_Limit_Total_Seconds;
    uint32              Round_Time_Minutes;
    uint32              Round_Time_SecondsDigit1;
    uint32              Round_Time_SecondsDigit2;
    uint32              Arena_ResetPlayers_Delay_Timer;
    bool                ArenaStarted;
    /* Gameobject spawning/despawning */

};
#endif
