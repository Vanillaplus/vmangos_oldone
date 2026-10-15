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

#ifndef __BATTLEGROUNDAC_H
#define __BATTLEGROUNDAC_H

#include "Common.h"
#include "BattleGround.h"
#include "World.h"

enum BG_AC_WorldStates
{
    BG_AC_OP_OCCUPIED_BASES_HORDE   = 2500,
    BG_AC_OP_OCCUPIED_BASES_ALLY    = 2501,
    BG_AC_OP_RESOURCES_ALLY         = 2502,
    BG_AC_OP_RESOURCES_HORDE        = 2503,
    BG_AC_OP_RESOURCES_MIN          = 2504,
    BG_AC_OP_RESOURCES_MAX          = 2505,
    BG_AC_OP_RESOURCES_MINWARNING   = 2506,
    BG_AC_OP_RESOURCES_MAXWARNING   = 2507,

    //TEMPLE
    BG_AC_TEMPLE_M_ICON_NONE        = 2508,
    BG_AC_TEMPLE_M_STATE_ALLY       = 2509,
    BG_AC_TEMPLE_M_STATE_HORDE      = 2510,
    BG_AC_TEMPLE_M_STATE_CON_ALI    = 2511,
    BG_AC_TEMPLE_M_STATE_CON_HOR    = 2512,

    BG_AC_TEMPLE_W_ICON_NONE        = 2513,
    BG_AC_TEMPLE_W_STATE_ALLY       = 2514,
    BG_AC_TEMPLE_W_STATE_HORDE      = 2515,
    BG_AC_TEMPLE_W_STATE_CON_ALI    = 2516,
    BG_AC_TEMPLE_W_STATE_CON_HOR    = 2517,

    BG_AC_TEMPLE_E_ICON_NONE        = 2518,
    BG_AC_TEMPLE_E_STATE_ALLY       = 2519,
    BG_AC_TEMPLE_E_STATE_HORDE      = 2520,
    BG_AC_TEMPLE_E_STATE_CON_ALI    = 2521,
    BG_AC_TEMPLE_E_STATE_CON_HOR    = 2522,
    //ALLY
    BG_AC_ZERO_NONE                 = 2523,
    BG_AC_ZERO_ALLY                 = 2524,
    BG_AC_ZERO_HORDE                = 2525,
    BG_AC_ZERO_CON_ALI              = 2526,
    BG_AC_ZERO_CON_HOR              = 2527,

    BG_AC_MONO_NONE                 = 2528,
    BG_AC_MONO_ALLY                 = 2529,
    BG_AC_MONO_HORDE                = 2530,
    BG_AC_MONO_CON_ALI              = 2531,
    BG_AC_MONO_CON_HOR              = 2532,

    BG_AC_OWL_NONE                  = 2533,
    BG_AC_OWL_ALLY                  = 2534,
    BG_AC_OWL_HORDE                 = 2535,
    BG_AC_OWL_CON_ALI               = 2536,
    BG_AC_OWL_CON_HOR               = 2537,

    BG_AC_LEFT_NONE                 = 2538,
    BG_AC_LEFT_ALLY                 = 2539,
    BG_AC_LEFT_HORDE                = 2540,
    BG_AC_LEFT_CON_ALI              = 2541,
    BG_AC_LEFT_CON_HOR              = 2542,

    BG_AC_RIGHT_NONE                = 2543,
    BG_AC_RIGHT_ALLY                = 2544,
    BG_AC_RIGHT_HORDE               = 2545,
    BG_AC_RIGHT_CON_ALI             = 2546,
    BG_AC_RIGHT_CON_HOR             = 2547,
    //HORDE
    BG_AC_GROMM_NONE                = 2548,
    BG_AC_GROMM_ALLY                = 2549,
    BG_AC_GROMM_HORDE               = 2550,
    BG_AC_GROMM_CON_ALI             = 2551,
    BG_AC_GROMM_CON_HOR             = 2552,

    BG_AC_BLIND_NONE                = 2553,
    BG_AC_BLIND_ALLY                = 2554,
    BG_AC_BLIND_HORDE               = 2555,
    BG_AC_BLIND_CON_ALI             = 2556,
    BG_AC_BLIND_CON_HOR             = 2557,

    BG_AC_LASTH_NONE                = 2558,
    BG_AC_LASTH_ALLY                = 2559,
    BG_AC_LASTH_HORDE               = 2560,
    BG_AC_LASTH_CON_ALI             = 2561,
    BG_AC_LASTH_CON_HOR             = 2562,

    BG_AC_N_IRON_NONE               = 2563,
    BG_AC_N_IRON_ALLY               = 2564,
    BG_AC_N_IRON_HORDE              = 2565,
    BG_AC_N_IRON_CON_ALI            = 2566,
    BG_AC_N_IRON_CON_HOR            = 2567,

    BG_AC_S_IRON_NONE               = 2568,
    BG_AC_S_IRON_ALLY               = 2569,
    BG_AC_S_IRON_HORDE              = 2570,
    BG_AC_S_IRON_CON_ALI            = 2571,
    BG_AC_S_IRON_CON_HOR            = 2572,
    //MID
    BG_AC_MID_NONE                  = 2573,
    BG_AC_MID_ALLY                  = 2574,
    BG_AC_MID_HORDE                 = 2575,
    BG_AC_MID_CON_ALI               = 2576,
    BG_AC_MID_CON_HORDE             = 2577,
    //TEMPLE GY
    BG_AC_TEMPLE_GY_ICON_NONE       = 2578,
    BG_AC_TEMPLE_GY_STATE_ALLY      = 2579,
    BG_AC_TEMPLE_GY_STATE_HORDE     = 2580,
    BG_AC_TEMPLE_GY_STATE_HCON_ALI  = 2581,
    BG_AC_TEMPLE_GY_STATE_CON_HOR   = 2582,

    BG_AC_NPC_ZERO                  = 23,
    BG_AC_NPC_MONO                  = 24,
    BG_AC_NPC_OWL                   = 25,
    BG_AC_NPC_LEFT                  = 26,
    BG_AC_NPC_RIGHT                 = 27,
    BG_AC_NPC_GROMM                 = 28,
    BG_AC_NPC_BLIND                 = 29,
    BG_AC_NPC_LASTH                 = 30,
    BG_AC_NPC_N_IRON                = 31,
    BG_AC_NPC_S_IRON                = 32


        /*EP_UI_TOWER_SLIDER_DISPLAY = 2426 2590,
        EP_UI_TOWER_SLIDER_POS = 2427 2591,
        EP_UI_TOWER_SLIDER_N = 2428 2592,*/
};

//Neutral
const uint32 BG_AC_OP_NODEICONS_NEUTRA[13] = { 2508, 2513, 2518, 2523, 2528, 2533, 2538, 2543, 2548, 2553, 2558, 2563, 2568 };
//A controlled
const uint32 BG_AC_OP_NODESTATES_A_OWN[13] = { 2509, 2514, 2519, 2524, 2529, 2534, 2539, 2544, 2549, 2554, 2559, 2564, 2569 };
//H controlled
const uint32 BG_AC_OP_NODESTATES_H_OWN[13] = { 2510, 2515, 2520, 2525, 2530, 2535, 2540, 2545, 2550, 2555, 2560, 2565, 2570 };
//A cont
const uint32 BG_AC_OP_NODESTATES_ACONT[13] = { 2511, 2516, 2521, 2526, 2531, 2536, 2541, 2546, 2551, 2556, 2561, 2566, 2571 };
//H cont
const uint32 BG_AC_OP_NODESTATES_HCONT[13] = { 2512, 2517, 2522, 2527, 2532, 2537, 2542, 2547, 2552, 2557, 2562, 2567, 2572 };
//Mid depths
//const uint32 BG_AC_OP_NODESTATES_MID[3] = { 2573, 2574, 2575 };
//const uint32 BG_AC_OP_NODESTATES_MGY[3] = { 2578, 2579, 2580 };
/* node events */
// node-events are just event1=BG_AC_Nodes, event2=BG_AC_NodeStatus
// so we don't need to define the constants here :)

enum BG_AC_Timers
{
    BG_AC_FLAG_CAPTURING_TIME   = 180000,
    BG_AC_TOWER_CAPTURING_TIME  = 180000,
};

enum BG_AC_Score
{
    BG_AC_WARNING_NEAR_VICTORY_SCORE = 900,
    BG_AC_WARNING_NEAR_LOSE_SCORE = 100,
    BG_AC_MAX_TEAM_SCORE = 1000,
    BG_AC_MIN_TEAM_SCORE = 0
};

/* do NOT change the order, else wrong behaviour */
enum BG_AC_Nodes
{
    BG_AC_NODE_TEMPLE_MID   = 0,
    BG_AC_NODE_TEMPLE_W     = 1,
    BG_AC_NODE_TEMPLE_E     = 2,
    BG_AC_NODE_ZERO         = 3,
    BG_AC_NODE_MONO         = 4,
    BG_AC_NODE_OWL          = 5,
    BG_AC_NODE_LEFT         = 6,
    BG_AC_NODE_RIGHT        = 7,
    BG_AC_NODE_GROMM        = 8,
    BG_AC_NODE_BLIND        = 9,
    BG_AC_NODE_LASTH        = 10,
    BG_AC_NODE_N_IRON       = 11,
    BG_AC_NODE_S_IRON       = 12,
    /*BG_AC_NODE_MID_DEPTHS   = 13,
    BG_AC_NODE_MID_GY       = 14,
    BG_AC_NPC_ZERO          = 23,
    BG_AC_NPC_MONO          = 24,
    BG_AC_NPC_OWL           = 25,
    BG_AC_NPC_LEFT          = 26,
    BG_AC_NPC_RIGHT         = 27,
    BG_AC_NPC_GROMM         = 28,
    BG_AC_NPC_BLIND         = 29,
    BG_AC_NPC_LASTH         = 30,
    BG_AC_NPC_N_IRON        = 31,
    BG_AC_NPC_S_IRON        = 32,*/
    BG_AC_NODES_ERROR       = 255
};

#define BG_AC_NODES_MAX   13

enum BG_AC_NodeStatus
{
    BG_AC_NODE_TYPE_NEUTRAL             = 0,
    BG_AC_NODE_TYPE_CONTESTED           = 1,
    BG_AC_NODE_STATUS_ALLY_CONTESTED    = 1,
    BG_AC_NODE_STATUS_HORDE_CONTESTED   = 2,
    BG_AC_NODE_TYPE_OCCUPIED            = 3,
    BG_AC_NODE_STATUS_ALLY_OCCUPIED     = 3,
    BG_AC_NODE_STATUS_HORDE_OCCUPIED    = 4
};

enum BG_AC_Sounds
{
    BG_AC_SOUND_NODE_CLAIMED                = 8192,
    BG_AC_SOUND_NODE_CAPTURED_ALLIANCE      = 8173,
    BG_AC_SOUND_NODE_CAPTURED_HORDE         = 8213,
    BG_AC_SOUND_NODE_ASSAULTED_ALLIANCE     = 8212,
    BG_AC_SOUND_NODE_ASSAULTED_HORDE        = 8174,
    BG_AC_SOUND_NEAR_VICTORY                = 8456
};

enum BG_AC_NodeOccupiedCredit
{
    BG_AC_NODE_STACLES_CREDIT               = 15001,
    BG_AC_NODE_BLACKSMITH_CREDIT            = 15002,
    BG_AC_NODE_FARM_CREDIT                  = 15003,
    BG_AC_NODE_LUMBER_MILL_CREDIT           = 15004,
    BG_AC_NODE_GOLD_MINE_CREDIT             = 15005,
};

enum BG_AC_NPC_ID
{
    BG_AC_BOWMAN_H                          = 22100,
    BG_AC_BOWMAN_A                          = 22101,
    BG_AC_WARRIOR_H                         = 22102,
    BG_AC_WARRIOR_A                         = 22107,
    BG_AC_SENTINEL_A                        = 22110,
    BG_AC_BOSS_ALLIANCE                     = 22116,
    BG_AC_BOSS_ALLIANCE_EVENT_ID            = 33,
    BG_AC_BOSS_HORDE                        = 22117,
    BG_AC_BOSS_HORDE_EVENT_ID               = 34,
    BG_AC_CRYSTALLUS                        = 22121,
    BG_AC_TRASH_DEER                        = 22119,
    BG_AC_TRASH_SPIDER                      = 22120,
    BG_AC_COMMANDER_A                       = 22138,
    BG_AC_COMMANDER_H                       = 22139,
    BG_AC_GOBLIN_A                          = 22108,
    BG_AC_GOBLIN_H                          = 22109,
    BG_AC_BS_A                              = 25069,
    BG_AC_BS_H                              = 25070,
    BG_AC_MEDIC_A                           = 25071,
    BG_AC_MEDIC_H                           = 25072,
    BG_AC_NATUR_A                           = 25073,
    BG_AC_NATUR_H                           = 25074,
    BG_AC_CAVEDRUID_A                       = 25075,
    BG_AC_CAVEDRUID_H                       = 25076,
    BG_AC_SPAWNER2_A                        = 22140,
    BG_AC_SPAWNER2_H                        = 22141,
    BG_AC_RUNNER_MELEE_A                    = 22122,
    BG_AC_RUNNER_MELEE_H                    = 22123,
    BG_AC_RUNNER_RANGED_A                   = 22124,
    BG_AC_RUNNER_RANGED_H                   = 22125,
    BG_AC_RUNNER_MELEE_A_ELITE              = 22143,
    BG_AC_RUNNER_MELEE_H_ELITE              = 22144,
    BG_AC_BUFFER_DUMMY_NPC                  = 22145,
};

enum BG_AC_REP
{
    AC_TOWER_REP                           = 50,
    AC_TICK_REP                            = 10,
    AC_MIDDLE_TICK_REP                     = 1,
    AC_BOSS_KILL_REP                       = 500,
    AC_CRYSTALUS_REP                       = 50,
};

const uint32 BG_AC_NPC_GUID[10] = { 97810, 97811, 97812, 97813, 97814, 97815, 97816, 97817, 97818, 97819 };

enum BG_EY_ProgressBarConsts
{
    BG_AC_POINT_MAX_CAPTURERS_COUNT = 5,
    BG_AC_POINT_RADIUS = 70,
    BG_AC_PROGRESS_BAR_DONT_SHOW = 0,
    BG_AC_PROGRESS_BAR_SHOW = 1,
    BG_AC_PROGRESS_BAR_PERCENT_GREY = 40,
    BG_AC_PROGRESS_BAR_STATE_MIDDLE = 50,
    BG_AC_PROGRESS_BAR_HORDE_CONTROLLED = 0,
    BG_AC_PROGRESS_BAR_NEUTRAL_LOW = 30,
    BG_AC_PROGRESS_BAR_NEUTRAL_HIGH = 70,
    BG_AC_PROGRESS_BAR_ALI_CONTROLLED = 100
};

// Tick intervals and given points: case 0,1,2,3,4,5 captured nodes
const uint32 BG_AC_TickIntervals[4] = { 0, 18000, 15000, 8000 };
const uint32 BG_AC_TickPoints[4] = { 0, 1, 2, 3 };

float const BuffSpawnLocs[4][3] =
{
    { 996.9f, 255.0f, 326.8f},
    { 651.3f, 135.8f, 277.9f},
    { 158.1f, 174.6f, 239.2f},
    { 45.6f, 805.6f, 284.5f}
};

class BattleGroundACScore : public BattleGroundScore
{
public:
    BattleGroundACScore() : PointsAssaulted(0), PointsDefended(0) {};
    virtual ~BattleGroundACScore() {};
    uint32 PointsAssaulted;
    uint32 PointsDefended;
};

class BattleGroundAC : public BattleGround
{
    friend class BattleGroundMgr;

public:
    BattleGroundAC();
    ~BattleGroundAC();

    void Update(uint32 diff);
    void AddPlayer(Player* plr);
    virtual void StartingEventCloseDoors();
    virtual void StartingEventOpenDoors();
    void RemovePlayer(Player* plr, ObjectGuid guid);
    void MiddlePoint();
    void MiddlePointEffect();
    void MiddleSetNeutral();
    void MiddleSetAlly();
    void MiddleSetHorde();
    void GYSetNeutral();
    void GYSetAlly();
    void GYSetHorde();
    void HandleAreaTrigger(Player* Source, uint32 Trigger);
    virtual void Reset();
    void EndBattleGround(Team winner);
    virtual WorldSafeLocsEntry const* GetClosestGraveYard(Player* player);

    /* Scorekeeping */
    virtual void UpdatePlayerScore(Player* Source, uint32 type, uint32 value);

    virtual void FillInitialWorldStates(WorldPacket& data, uint32& count);

    // AC Mid Aura
    virtual void BGAuraApplied(Player* plr);
    virtual void BGAuraRemoved(Player* plr);

    /* Nodes occupying */
    virtual void EventPlayerClickedOnFlag(Player* source, GameObject* target_obj);
    void HandleKillPlayer(Player* player, Player* killer);
    void HandleKillUnit(Creature* creature, Player* killer);
    bool SetupBattleGround();

private:
    /* Gameobject spawning/despawning */
    void _CreateBanner(uint32 node, uint32 type, uint8 teamIndex);
    void _SendNodeUpdate(uint32 node);

    /* Creature spawning/despawning */
    // TODO: working, scripted peons spawning
    void _NodeOccupied(uint32 node, Team team);

    int32 _GetNodeNameId(uint32 node);

    /* Nodes info:
        0: neutral
        1: ally contested
        2: horde contested
        3: ally occupied
        4: horde occupied     */
    uint32              m_Nodes[BG_AC_NODES_MAX];
    uint32              m_prevNodes[BG_AC_NODES_MAX];   // used for performant wordlstate-updating
    uint32              m_NodeTimers[BG_AC_NODES_MAX];
    uint32              m_lastTick[BG_TEAMS_COUNT];
    uint32              m_HonorScoreTics[BG_TEAMS_COUNT];
    uint32              m_ReputationScoreTics[BG_TEAMS_COUNT];
    bool                m_IsInformedNearVictory;
    bool                m_IsInformedNearDefeat;
    uint32              m_HonorTics;
    uint32              m_ReputationTics;
    bool                m_mid_neutral;
    bool                m_mid_ally;
    bool                m_mid_horde;
    bool                m_GY_neutral;
    bool                m_GY_ally;
    bool                m_GY_horde;
    uint32              m_mid_score;
    uint32              m_mid_update_timer;
    uint32              m_near_end_update_timer;
    uint32              m_holderStacks;
};
#endif
