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
SDName: Boss_Houndmaster_Loksey
SD%Complete: 100
SDComment: TODO: if this guy not involved in some special event, remove (and let ACID script)
SDCategory: Scarlet Monastery
EndScriptData */

#include "scriptPCH.h"
#include "scarlet_monastery.h"

enum
{
    SAY_AGGRO                       = -1189021, // Release the Hounds!
    SAY_TRANQ                       = -1900301, // Down, beasts! Down!
    SAY_DEATH                       = -1900302, // My hounds… protect me!

    NPC_SCARLET_HOUND               = 25202,
    NPC_TRAP_SUMMONER               = 25259,


    SPELL_BLOODLUST                 = 6742,
    SPELL_SUMMONSCARLETHOUND_VISUAL = 35836,
    SPELL_PARALYZING_POISON         = 35837,
    SPELL_PARALYZING_POISON_STUN    = 35842,
    SPELL_SCARE_BEAST               = 35838,
    SPELL_POWER_SHOT                = 35841,
    SPELL_TRANQUILIZING_POISON      = 35843,
    SPELL_TRAP_ARMING               = 35872,
    SPELL_FREEZING_TRAP             = 35873,
    SPELL_FREEZING_TRAP_EFFECT      = 35874,
};

struct boss_houndmaster_lokseyAI : public ScriptedAI
{
    boss_houndmaster_lokseyAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 PosCheckTimer;
    uint32 BloodLust_Timer;
    uint32 HoundCheck_Timer;
    uint32 SummonNewHound_Timer;
    uint32 Poison_Timer;
    uint32 CheckPoison_Timer;
    uint32 ScareBeast_Timer;
    uint32 PowerShot_Timer;
    uint32 TranqPoison_Timer;
    uint32 CheckHP_Timer;
    uint32 Trap_Timer;
    float Last_HP_Mark;

    void Reset()
    {
        PosCheckTimer = 1000;
        BloodLust_Timer = 20000;
        HoundCheck_Timer = 10000;
        SummonNewHound_Timer = 30000;
        Poison_Timer = urand(8000, 15000);
        CheckPoison_Timer = 18000;
        ScareBeast_Timer = 20000;
        PowerShot_Timer = urand(10000, 20000);
        TranqPoison_Timer = 5000;
        CheckHP_Timer = 10000;
        Trap_Timer = urand(20000, 22000);
        Last_HP_Mark = 100.0f;

        /*m_creature->RemoveAllDynObjects();*/
    }

    void Aggro(Unit *who)
    {
        m_creature->SetInCombatWithZone();
        DoScriptText(SAY_AGGRO, m_creature);

        if (m_pInstance)
            m_pInstance->SetData(TYPE_LOKSEY, IN_PROGRESS);
    }

    void JustDied(Unit* /*pKiller*/)
    {
        DoScriptText(SAY_DEATH, m_creature);

        if (m_pInstance)
            m_pInstance->SetData(TYPE_LOKSEY, DONE);
    }

    void JustReachedHome()
    {
        std::list<Creature*> dummy;
        GetCreatureListWithEntryInGrid(dummy, m_creature, NPC_TRAP_SUMMONER, 150.0f);
        for (auto it = dummy.cbegin(); it != dummy.cend(); ++it)
        {
            (*it)->RemoveAllDynObjects();
            (*it)->DespawnOrUnsummon();
        }

        if (m_pInstance)
            m_pInstance->SetData(TYPE_LOKSEY, FAIL);
    }

    void CheckHounds()
    {
        uint32 HoundsNumber = 0;
        std::list<Creature*> HoundsList;
        GetCreatureListWithEntryInGrid(HoundsList, m_creature, NPC_SCARLET_HOUND, 100.0f);

        for (std::list<Creature*>::iterator itr = HoundsList.begin(); itr != HoundsList.end(); ++itr)
        {
            if ((*itr)->isAlive() && (*itr)->isInCombat())
            {
                HoundsNumber += 1;
            }
        }
        if (HoundsNumber < 4)
        {
            SummonHound();
        }
    }

    void SummonHound()
    {
        m_creature->CastSpell(m_creature, SPELL_SUMMONSCARLETHOUND_VISUAL, true);

        if (Creature* Crea = m_creature->SummonCreature(NPC_SCARLET_HOUND, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 5000))
        {
            if (Unit* randomTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
            {
                Crea->SetInCombatWithZone();
                Crea->getThreatManager().addThreat(randomTarget, 1000);
            }
        }
    }

    void CheckPoison()
    {
        ThreatList const& tList = m_creature->getThreatManager().getThreatList();
        for (ThreatList::const_iterator itr = tList.begin(); itr != tList.end(); ++itr)
        {
            if (Unit* PoisonTargets = m_creature->GetMap()->GetUnit((*itr)->getUnitGuid()))
            {
                if (m_creature->GetDistance(PoisonTargets) < 100 && PoisonTargets->GetPositionX() > 320.0f && PoisonTargets->GetPositionX() < 380.0f)
                {
                    //No poison and no Constitution warrior's talent
                    if (!PoisonTargets->HasAura(SPELL_PARALYZING_POISON) && !PoisonTargets->HasAura(SPELL_PARALYZING_POISON_STUN) && !PoisonTargets->HasAura(34127))
                    {
                        if (DoCastSpellIfCan(m_creature, SPELL_PARALYZING_POISON) == CAST_OK)
                        {
                            Poison_Timer = urand(12000, 18000);
                            if (Trap_Timer < 2000)
                                Trap_Timer = 2000;

                            break;
                        }
                    }
                }
            }
        }
    }

    void TranqPoison()
    {
        bool AnnounceMessage = false;

        ThreatList const& tList = m_creature->getThreatManager().getThreatList();
        for (ThreatList::const_iterator itr = tList.begin(); itr != tList.end(); ++itr)
        {
            if (Unit* TranqTargets = m_creature->GetMap()->GetUnit((*itr)->getUnitGuid()))
            {
                if (m_creature->GetDistance(TranqTargets) < 100 && TranqTargets->GetPositionX() > 320.0f && TranqTargets->GetPositionX() < 380.0f)
                {
                    Unit::SpellAuraHolderMap const& auras = TranqTargets->GetSpellAuraHolderMap();
                    for (Unit::SpellAuraHolderMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                    {
                        SpellAuraHolder* holder = itr->second;

                        if (!holder)
                            continue;

                        if (holder->GetSpellProto()->Dispel == DISPEL_ENRAGE)
                        {
                            m_creature->CastSpell(TranqTargets, SPELL_TRANQUILIZING_POISON, true);
                            AnnounceMessage = true;
                            break;
                        }
                    }
                }
            }
        }

        if (AnnounceMessage)
        {
            if (m_creature->HasAura(34823))
                return;

            m_creature->CastSpell(m_creature, 34823, true);
            DoScriptText(SAY_TRANQ, m_creature);
        }
    }

    void DoPowerShot(Unit* pTarget)
    {
        if (DoCastSpellIfCan(pTarget, SPELL_POWER_SHOT) == CAST_OK)
        {
            PowerShot_Timer = urand(12000, 15000);

            // Prolong timers
            uint32* timers[] =
            {
                &HoundCheck_Timer,
                &SummonNewHound_Timer,
                &Poison_Timer,
                &CheckPoison_Timer,
                &TranqPoison_Timer,
                &Trap_Timer,
                &CheckHP_Timer,
                &BloodLust_Timer,
                &ScareBeast_Timer
            };

            for (uint32* t : timers)
                *t = std::max(*t, uint32(2700));
        }
    }

    void DoTrap(Unit* pTarget)
    {
        if (!pTarget->HasAura(SPELL_PARALYZING_POISON_STUN) && !pTarget->HasAura(SPELL_TRAP_ARMING))
        {
            if (Creature* TrapNPC = m_creature->SummonCreature(NPC_TRAP_SUMMONER, m_creature->GetPositionX(), m_creature->GetPositionY(), (m_creature->GetPositionZ() -45.0f), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 300000))
            {
                m_creature->CastSpell(pTarget, SPELL_TRAP_ARMING, true);
                TrapNPC->CastSpell(pTarget, SPELL_FREEZING_TRAP, true);
                Trap_Timer = urand(20000, 22000);
            }
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (PosCheckTimer < diff)
        {
            if (m_creature->GetPositionX() < 320.0f || m_creature->GetPositionX() > 380.0f)
                EnterEvadeMode();

            PosCheckTimer = 1000;
        }
        else PosCheckTimer -= diff;

        if (HoundCheck_Timer < diff)
        {
            CheckHounds();
            HoundCheck_Timer = 5000;
        }
        else HoundCheck_Timer -= diff;

        if (SummonNewHound_Timer < diff)
        {
            SummonHound();
            SummonNewHound_Timer = 30000;
        }
        else SummonNewHound_Timer -= diff;


        if (Poison_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_PARALYZING_POISON) == CAST_OK)
            {
                Poison_Timer = urand(12000, 18000);
                if (Trap_Timer < 2000)
                    Trap_Timer = 2000;
            }
        }
        else Poison_Timer -= diff;

        if (CheckPoison_Timer < diff)
        {
            CheckPoison();
            CheckPoison_Timer = 2000;
        }
        else CheckPoison_Timer -= diff;


        if (PowerShot_Timer < diff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_FARTHEST, urand(0,3), nullptr, SELECT_FLAG_IN_LOS))
            {
                DoPowerShot(pTarget);
            }
            else if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_FARTHEST, urand(0, 3)))
            {
                DoPowerShot(pTarget);
            }
        }
        else PowerShot_Timer -= diff;


        if (TranqPoison_Timer < diff)
        {
            TranqPoison();
            TranqPoison_Timer = 2000;
        }
        else TranqPoison_Timer -= diff;

        if (Trap_Timer < diff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_FREEZING_TRAP_EFFECT, SELECT_FLAG_PLAYER | SELECT_FLAG_MISSING_AURA | SELECT_FLAG_NOT_IN_MELEE_RANGE | SELECT_FLAG_IN_LOS))
            {
                DoTrap(pTarget);
            }
            else if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_FREEZING_TRAP_EFFECT, SELECT_FLAG_PLAYER | SELECT_FLAG_MISSING_AURA| SELECT_FLAG_NOT_IN_MELEE_RANGE))
            {
                DoTrap(pTarget);
            }
            else if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_FREEZING_TRAP_EFFECT, SELECT_FLAG_PLAYER | SELECT_FLAG_MISSING_AURA))
            {
                DoTrap(pTarget);
            }
        }
        else Trap_Timer -= diff;


        if (CheckHP_Timer < diff)
        {
            if (m_creature->GetHealthPercent() < (Last_HP_Mark - 10.0f))
            {
                Last_HP_Mark = Last_HP_Mark - 10.0f;
                if (SummonNewHound_Timer > 10000)
                    SummonNewHound_Timer = 500;
            }
            CheckHP_Timer = 1000;
        }
        else CheckHP_Timer -= diff;


        if (BloodLust_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_BLOODLUST) == CAST_OK)
                BloodLust_Timer = 20000;
        }
        else BloodLust_Timer -= diff;


        if (ScareBeast_Timer < diff)
        {
            ThreatList const& tList = m_creature->getThreatManager().getThreatList();
            for (ThreatList::const_iterator itr = tList.begin(); itr != tList.end(); ++itr)
            {
                if (Unit* BeastTargets = m_creature->GetMap()->GetUnit((*itr)->getUnitGuid()))
                {
                    if (BeastTargets->GetCreatureType() == CREATURE_TYPE_BEAST)
                    {
                        if (DoCastSpellIfCan(m_creature, SPELL_SCARE_BEAST) == CAST_OK)
                        {
                            ScareBeast_Timer = urand(18000, 25000);
                            return;
                        }
                    }
                }
            }
            ScareBeast_Timer = (3000, 5000);
        }
        else ScareBeast_Timer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_houndmaster_loksey(Creature* pCreature)
{
    return new boss_houndmaster_lokseyAI(pCreature);
}

void AddSC_boss_houndmaster_loksey()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "boss_houndmaster_loksey";
    newscript->GetAI = &GetAI_boss_houndmaster_loksey;
    newscript->RegisterSelf();
}







//////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////


enum
{
    SAY_ABBENDIS_AGGRO    = -1900303, // They shall not pass!
    SAY_ALL_DAY           = -1900304, // I can do this all day!
    SAY_ABBENDIS_DEATH    = -1900305, // Hold the...
    SAY_DEATH_DOAN        = -1900306, // Nooo… I'll sear the flesh from your bones!
    SAY_PHASE2            = -1900307, // Blades out!
    SAY_PHASE3            = -1900308, // Not today!
    SAY_ABBENDIS_TAUNT    = -1900309, // Face me!
    SAY_ABBENDIS_RESERVES = -1900310, // Reserves! NOW!

    NPC_SHARPSHOOTER      = 25230,
    NPC_RECRUIT           = 25234,
    NPC_CHARGER           = 25235,
    NPC_CHARGESPEEDDUMMY  = 25121,
    SPELL_ROOT_SELF       = 23973,
    SPELL_CHARGE_SPEED    = 35190,
    SPEED_LIMIT           = 9,     //P1 Speed check limit
    SPELL_CLOSE_QUARTERS  = 36154,
    //P1
    SPELL_LONGSHOT        = 35847,
    SPELL_COMMAND_GESTURE = 35848,
    SPELL_EXPLOSIVE_SHOT  = 35849,
    SPELL_VOLLEY          = 35852,
    SPELL_SPEED_TEST      = 35859,
    SPELL_EXPLOSIVE_CD    = 35860,
    //P2
    SPELL_SHIELD_BASH     = 35853,
    SPELL_CHARGE          = 35854,
    SPELL_CONSECRATION1   = 35855,
    SPELL_CONSECRATION2   = 35856,
    SPELL_AOE_TAUNT       = 35857,
    //P3
    SPELL_BLOCK_BONUS     = 36170,
    SPELL_P3_MOUNT        = 35858,
    SPELL_P3_INITIAL_STUN = 35861,
    SPELL_P3_INITIAL_HEAL = 35862,
    SPELL_P3_STORMBOLT    = 35863,
    SPELL_P3_HOOF_KICK    = 35865,
    SPELL_P3_TRAMPLE      = 35866,

    SPELL_P3_SPAWN_1      = 35867,
    SPELL_P3_SPAWN_2      = 35868,
    SPELL_P3_TRAMPLE_CD   = 35875,



};

float const SpawnLocs[4][3] =
{
    { 392.0f, 1329.5f, 47.6f},
    { 392.0f, 1321.5f, 47.6f},
    { 392.0f, 1313.5f, 47.6f},
    { 392.0f, 1305.5f, 47.6f}
};

float const ChargerMountSpawnLocs[2][3] =
{
    { 393.5f, 1318.0f, 47.46f},
    { 487.8f, 1313.8f, 48.1f}
};

float const ChargerP3Locs[3][2] =
{
    { 35869, 1327.0f},
    { 35870, 1317.6f},
    { 35871, 1309.0f}
};

struct boss_highgeneral_abbendisAI : public ScriptedAI
{
    boss_highgeneral_abbendisAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 PosCheckTimer;
    uint32 PosCheckTimer2;
    uint32 BossPhase;
    bool Phase_1_Pos;

    uint32 Phase_1_ExplosiveShot_Timer;
    uint32 Phase_1_Shot_Timer;
    uint32 Phase_1_Volley_Timer;
    uint32 Phase_1_Speed_Check_Timer;

    ObjectGuid BestTauntTargetGuid;
    bool Phase_2_IsTauntCharging;
    uint32 Phase_2_CheckHP_Timer;
    uint32 Phase_2_Threat_Timer;
    uint32 Phase_2_Shield_Bash_Timer;
    uint32 Phase_2_Charge_Timer;
    uint32 Phase_2_Consecration_Timer;
    uint32 Phase_2_AddSpawn_Timer;
    uint32 Phase_2_AddSpawn_Timer2;
    uint32 Phase_2_Taunt_Timer;

    bool Phase_3_ChargerSummoned;
    uint32 Phase_3_ChargerDelay;
    uint32 Phase_3_Charger_Ride_Delay;
    uint32 Phase_3_Stormbolt_Timer;
    uint32 Phase_3_HoofKick_Timer;
    uint32 Phase_3_Trample_Timer;
    uint32 Phase_3_Switch_Target_Timer;

    void Reset()
    {
        PosCheckTimer = 1000;
        PosCheckTimer2 = 10000;
        m_creature->RemoveAurasDueToSpell(SPELL_ROOT_SELF);
        BossPhase = 0;
        Phase_1_Pos = false;
        Phase_1_ExplosiveShot_Timer = urand(1000, 3000);
        Phase_1_Shot_Timer = 1000;
        Phase_1_Volley_Timer = 5000;
        Phase_1_Speed_Check_Timer = 5000;

        BestTauntTargetGuid.Clear();
        Phase_2_IsTauntCharging = false;
        Phase_2_CheckHP_Timer = 2000;
        Phase_2_Threat_Timer = 500;
        Phase_2_Shield_Bash_Timer = urand(6000, 10000);
        Phase_2_Charge_Timer = urand(1000, 2000);
        Phase_2_Consecration_Timer = urand(8000, 12000);
        Phase_2_AddSpawn_Timer = 40000;
        Phase_2_AddSpawn_Timer2 = 50000;
        Phase_2_Taunt_Timer = 60000;

        Phase_3_ChargerSummoned = false;
        Phase_3_ChargerDelay = 2000;
        Phase_3_Charger_Ride_Delay = 3000;
        Phase_3_Stormbolt_Timer = 500;
        Phase_3_HoofKick_Timer = urand(5000, 12000);
        Phase_3_Trample_Timer = 200;
        Phase_3_Switch_Target_Timer = 25000;
        
        if (m_creature->GetPositionX() < 470.0f)
            m_creature->SetWalk(false);

        std::list<Creature*> Archers;
        GetCreatureListWithEntryInGrid(Archers, m_creature, NPC_SHARPSHOOTER, 100.0f);
        for (Creature* ArcherCrea : Archers)
            if (ArcherCrea && ArcherCrea->isAlive())
                ArcherCrea->RemoveAurasDueToSpell(SPELL_ROOT_SELF);

        std::list<Creature*> ChargeNPC;
        GetCreatureListWithEntryInGrid(ChargeNPC, m_creature, NPC_CHARGESPEEDDUMMY, 200.0f);
        for (Creature* ChargeCrea : ChargeNPC)
            if (ChargeCrea && !ChargeCrea->HasAura(SPELL_CHARGE_SPEED))
                ChargeCrea->AddAura(SPELL_CHARGE_SPEED);
    }

    void Aggro(Unit* who)
    {
        m_creature->SetInCombatWithZone();
        DoScriptText(SAY_ABBENDIS_AGGRO, m_creature);
        BossPhase = 1;

        std::list<Creature*> Archers;
        GetCreatureListWithEntryInGrid(Archers, m_creature, NPC_SHARPSHOOTER, 100.0f);
        for (Creature* ArcherCrea : Archers)
        {
            if (ArcherCrea)
                ArcherCrea->AddAura(SPELL_ROOT_SELF);
        }

        ThreatList const& tList = m_creature->getThreatManager().getThreatList();
        for (ThreatList::const_iterator iter = tList.begin(); iter != tList.end(); ++iter)
        {
            if (Unit* PullTargets = m_creature->GetMap()->GetUnit((*iter)->getUnitGuid()))
            {
                if (PullTargets->GetPositionX() > 400.0f)
                    PullTargets->NearTeleportTo(393.4f, 1317.4f, 47.5f, 6.3f);

                int32 starterX = 400;
                PullTargets->CastCustomSpell(PullTargets, SPELL_SPEED_TEST, &starterX, nullptr, nullptr, true);
            }
        }

        if (m_pInstance)
            m_pInstance->SetData(TYPE_ABBENDIS, IN_PROGRESS);
    }

    void JustDied(Unit* /*pKiller*/)
    {
        DoScriptText(SAY_ABBENDIS_DEATH, m_creature);
        if (Unit* Doan = m_creature->FindNearestCreature(25267, 300))
            DoScriptText(SAY_DEATH_DOAN, Doan);

        if (m_pInstance)
            m_pInstance->SetData(TYPE_ABBENDIS, DONE);
    }

    void JustReachedHome()
    {
        m_creature->SetWalk(true);
        m_creature->GetMotionMaster()->MoveWaypoint();

        if (m_pInstance)
            m_pInstance->SetData(TYPE_ABBENDIS, FAIL);
    }

    void MovementInform(uint32 uiType, uint32 uiPointId)
    {
        if (uiType != POINT_MOTION_TYPE)
            return;

        switch (uiPointId)
        {
        case 5:
            if (BossPhase == 1)
            {
                m_creature->AddAura(SPELL_ROOT_SELF);
                m_creature->GetMotionMaster()->MoveIdle();
            }
            break;
        }
    }

    void DoExplosiveShot()
    {
        Unit* HighestXTar = nullptr;
        float HighestX = 0.0f;
        ThreatList const& tList = m_creature->getThreatManager().getThreatList();
        for (ThreatList::const_iterator iter = tList.begin(); iter != tList.end(); ++iter)
        {
            if (Unit* pTempTarget = m_creature->GetMap()->GetUnit((*iter)->getUnitGuid()))
            {
                if (pTempTarget->isAlive() && pTempTarget->GetPositionX() > HighestX && pTempTarget->GetPositionX() < 473.0f && pTempTarget->GetPositionX() >= 390.0f && !pTempTarget->HasAura(35850))
                {
                    HighestXTar = pTempTarget;
                    HighestX = HighestXTar->GetPositionX();
                }
            }
        }
        if (HighestXTar)
        {
            std::list<Creature*> Archers;
            GetCreatureListWithEntryInGrid(Archers, m_creature, NPC_SHARPSHOOTER, 100.0f);
            for (Creature* ArcherCrea : Archers)
            {
                if (ArcherCrea && ArcherCrea->isAlive() && !ArcherCrea->HasAura(SPELL_EXPLOSIVE_CD))
                {
                    ArcherCrea->AddAura(SPELL_EXPLOSIVE_CD);
                    ArcherCrea->CastSpell(HighestXTar, SPELL_EXPLOSIVE_SHOT, true);
                    Phase_1_ExplosiveShot_Timer = 5000;
                    return;
                }
            }
        }
        Phase_1_ExplosiveShot_Timer = 500;
    }

    void DoSpeedExplosiveShot()
    {
        Unit* HighestXTar = nullptr;
        float HighestX = 0.0f;
        bool DoubleCast = false;

        ThreatList const& tList = m_creature->getThreatManager().getThreatList();

        for (ThreatList::const_iterator iter = tList.begin(); iter != tList.end(); ++iter)
        {
            if (Unit* PullTarget = m_creature->GetMap()->GetUnit((*iter)->getUnitGuid()))
            {
                if (PullTarget->HasAura(SPELL_SPEED_TEST))
                {
                    Unit::AuraList const& mDummy = PullTarget->GetAurasByType(SPELL_AURA_DUMMY);

                    for (Unit::AuraList::const_iterator auraIter = mDummy.begin(); auraIter != mDummy.end(); ++auraIter)
                    {
                        if ((*auraIter)->GetSpellProto()->Id == SPELL_SPEED_TEST)
                        {
                            float diffX = PullTarget->GetPositionX() - (*auraIter)->GetModifier()->m_amount;

                            if (diffX > SPEED_LIMIT && PullTarget->GetPositionX() > HighestX)
                            {
                                HighestXTar = PullTarget;
                                HighestX = PullTarget->GetPositionX();
                                DoubleCast = diffX > SPEED_LIMIT * 2;
                            }

                            break;
                        }
                    }
                }

                int32 currentX = PullTarget->GetPositionX();
                PullTarget->CastCustomSpell(PullTarget, SPELL_SPEED_TEST, &currentX, nullptr, nullptr, true);
            }
        }

        if (!HighestXTar)
            return;

        for (int shot = 0; shot < (DoubleCast ? 2 : 1); ++shot)
        {
            std::list<Creature*> Archers;
            GetCreatureListWithEntryInGrid(Archers, m_creature, NPC_SHARPSHOOTER, 100.0f);

            for (Creature* ArcherCrea : Archers)
            {
                if (ArcherCrea && ArcherCrea->isAlive() && !ArcherCrea->HasAura(SPELL_EXPLOSIVE_CD))
                {
                    ArcherCrea->AddAura(SPELL_EXPLOSIVE_CD);
                    ArcherCrea->CastSpell(HighestXTar, SPELL_EXPLOSIVE_SHOT, false);
                    break;
                }
            }
        }

        Phase_1_Speed_Check_Timer = 3000;
    }

    bool CoveredBy(Unit* Candidate, Unit* Tanker)
    {
        if (Candidate == Tanker) return false;
        const float Xdist = 8.0f;
        const float Ydist = 2.5f;
        float dx = Tanker->GetPositionX() - Candidate->GetPositionX();
        if (dx <= 0.0f || dx > Xdist) return false;

        const float dy = std::fabs(Tanker->GetPositionY() - Candidate->GetPositionY());
        // (dx/a)^2 + (dy/b)^2 <= 1
        return (dx * dx) / (Xdist * Xdist) + (dy * dy) / (Ydist * Ydist) <= 1.0f;
    }

    void DoLongshot()
    {
        std::vector<Unit*> units;
        {
            ThreatList const& tl = m_creature->getThreatManager().getThreatList();
            units.reserve(tl.size());
            for (auto it = tl.begin(); it != tl.end(); ++it)
            {
                if (Unit* u = m_creature->GetMap()->GetUnit((*it)->getUnitGuid()))
                {
                    if (u->isAlive() && !u->HasAura(4511) && !u->IsImmuneToDamage(m_creature->GetMeleeDamageSchoolMask()))
                    {
                        if (u->GetPositionX() >= 473.0f) //P2 start check
                        {
                            DoScriptText(SAY_PHASE2, m_creature);

                            m_creature->RemoveAurasDueToSpell(SPELL_ROOT_SELF);
                            if (Unit* v = m_creature->getVictim())
                                m_creature->GetMotionMaster()->MoveChase(v);

                            std::list<Creature*> Archers;
                            GetCreatureListWithEntryInGrid(Archers, m_creature, NPC_SHARPSHOOTER, 100.0f);
                            for (Creature* ArcherCrea : Archers)
                                if (ArcherCrea && ArcherCrea->isAlive())
                                    ArcherCrea->RemoveAurasDueToSpell(SPELL_ROOT_SELF);

                            BossPhase = 2;

                            std::list<Creature*> ChargeNPC;
                            GetCreatureListWithEntryInGrid(ChargeNPC, m_creature, NPC_CHARGESPEEDDUMMY, 200.0f);
                            for (Creature* ChargeCrea : ChargeNPC)
                                if (ChargeCrea && ChargeCrea->HasAura(SPELL_CHARGE_SPEED))
                                    ChargeCrea->RemoveAurasDueToSpell(SPELL_CHARGE_SPEED);

                            return;
                        }
                        else if (u->GetPositionX() >= 390.0f)
                            units.push_back(u);
                    }
                }
            }
        }

        if (units.empty())  //Evade on players out of range during Phase 1
        {
            std::list<Creature*> Archers;
            GetCreatureListWithEntryInGrid(Archers, m_creature, NPC_SHARPSHOOTER, 100.0f);
            for (Creature* ArcherCrea : Archers)
                if (ArcherCrea && ArcherCrea->isAlive() && ArcherCrea->AI())
                {
                    ArcherCrea->RemoveAllDynObjects();
                    ArcherCrea->AI()->EnterEvadeMode();
                }

            EnterEvadeMode();
            return;
        }

        std::vector<Unit*> uncovered;
        for (Unit* candidate : units)
        {
            bool covered = false;
            for (Unit* other : units)
            {
                if (CoveredBy(candidate, other)) { covered = true; break; }
            }
            if (!covered)
                uncovered.push_back(candidate);
        }

        if (!uncovered.empty())
        {
            m_creature->CastSpell(m_creature->getVictim(), SPELL_COMMAND_GESTURE, true);

            std::list<Creature*> Archers;
            GetCreatureListWithEntryInGrid(Archers, m_creature, NPC_SHARPSHOOTER, 100.0f);
            for (Creature* ArcherCrea : Archers)
            {
                if (ArcherCrea && ArcherCrea->isAlive())
                {
                    Unit* target = uncovered[urand(0u, static_cast<uint32>(uncovered.size() - 1u))];
                    ArcherCrea->CastSpell(target, SPELL_LONGSHOT, false);
                }
            }
        }
    }

    void PreTaunt()
    {
        BestTauntTargetGuid.Clear();
        std::vector<Unit*> P;
        ThreatList const& tList = m_creature->getThreatManager().getThreatList();

        for (ThreatList::const_iterator it = tList.begin(); it != tList.end(); ++it)
        {
            Unit* u = m_creature->GetMap()->GetUnit((*it)->getUnitGuid());
            if (!u || !u->isAlive()) continue;
            if (!m_creature->IsWithinDistInMap(u, 80.0f)) continue;

            P.push_back(u);
        }

        if (P.empty())
            return;

        std::vector<uint16> cnt(P.size(), 0);

        for (size_t i = 0; i < P.size(); ++i)
        {
            for (size_t j = i + 1; j < P.size(); ++j)
            {
                if (P[i]->GetDistance2d(P[j]) <= 10.0f)
                {
                    ++cnt[i];
                    ++cnt[j];
                }
            }
        }
        size_t bestIdx = 0;
        uint16 bestCnt = cnt[0];
        float  bestDist = m_creature->GetDistance(P[0]);

        for (size_t i = 1; i < P.size(); ++i)
        {
            float d = m_creature->GetDistance(P[i]);
            if (cnt[i] > bestCnt || (cnt[i] == bestCnt && d < bestDist))
            {
                bestIdx = i;
                bestCnt = cnt[i];
                bestDist = d;
            }
        }
        Unit* bestTarget = P[bestIdx];
        if (m_creature->GetDistance(bestTarget) >= 5.0f)
        {
            BestTauntTargetGuid = bestTarget->GetObjectGuid();
            m_creature->CastSpell(bestTarget, SPELL_CHARGE, true);
            Phase_2_IsTauntCharging = true;
        }
        else
        {
            DoScriptText(SAY_ABBENDIS_TAUNT, m_creature);
            m_creature->CastSpell(m_creature, SPELL_AOE_TAUNT, false);
        }
    }

    void SummonRecruit()
    {
        for (int i = 0; i < 4; ++i)
        {
            if (Creature* Crea = m_creature->SummonCreature(NPC_RECRUIT, SpawnLocs[i][0], SpawnLocs[i][1], SpawnLocs[i][2], 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 5000))
            {
                Crea->SetInCombatWithZone();
                if (Unit* bottomTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_BOTTOMAGGRO, i, nullptr, SELECT_FLAG_PLAYER))
                    Crea->getThreatManager().addThreat(bottomTarget, 10000);
            }
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (PosCheckTimer < diff)
        {
            if (m_creature->GetPositionX() < 380.0f || m_creature->GetPositionX() > 500.0f)
            {
                std::list<Creature*> Archers;
                GetCreatureListWithEntryInGrid(Archers, m_creature, NPC_SHARPSHOOTER, 100.0f);
                for (Creature* ArcherCrea : Archers)
                    if (ArcherCrea && ArcherCrea->isAlive() && ArcherCrea->AI())
                    {
                        ArcherCrea->RemoveAllDynObjects();
                        ArcherCrea->AI()->EnterEvadeMode();
                    }

                EnterEvadeMode();
                return;
            }

            PosCheckTimer = 1000;
        }
        else PosCheckTimer -= diff;

        if (PosCheckTimer2 < diff)
        {
            if (m_creature->GetPositionX() < 392.0f || m_creature->GetPositionX() > 486.0f)
                m_creature->CastSpell(m_creature, SPELL_CLOSE_QUARTERS, true);

            PosCheckTimer2 = 10000;
        }
        else PosCheckTimer2 -= diff;

        switch (BossPhase)
        {
            case 1: // PHASE 1
            {
                if (!Phase_1_Pos) //Initial position
                {
                    if (m_creature->GetDistance2d(477.2f, 1319.75f) < m_creature->GetDistance2d(477.2f, 1315.75f))
                        m_creature->GetMotionMaster()->MovePoint(5, 477.2f, 1321.5f, 47.5f, MOVE_RUN_MODE, 4.0f);
                    else
                        m_creature->GetMotionMaster()->MovePoint(5, 477.2f, 1311.5f, 47.5f, MOVE_RUN_MODE, 4.0f);

                    Phase_1_Pos = true;
                }

                if (Phase_1_ExplosiveShot_Timer < diff) //Knockback
                {
                    DoExplosiveShot();
                }
                else
                    Phase_1_ExplosiveShot_Timer -= diff;

                if (Phase_1_Speed_Check_Timer < diff)  //Speed check
                {
                    DoSpeedExplosiveShot();
                }
                else
                    Phase_1_Speed_Check_Timer -= diff;

                if (Phase_1_Shot_Timer < diff)  //Shooting
                {
                    DoLongshot();
                    Phase_1_Shot_Timer = 1500;
                }
                else
                    Phase_1_Shot_Timer -= diff;

                if (Phase_1_Volley_Timer < diff)    //Volley
                {
                    std::list<Creature*> Archers;
                    GetCreatureListWithEntryInGrid(Archers, m_creature, NPC_SHARPSHOOTER, 100.0f);
                    for (Creature* ArcherCrea : Archers)
                    {
                        if (ArcherCrea && ArcherCrea->isAlive())
                        {
                            if (Unit* randomTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                            {
                                if (!randomTarget->HasAura(SPELL_VOLLEY) && randomTarget->GetPositionX() >= 400.0f)
                                {
                                    ArcherCrea->CastSpell(randomTarget, SPELL_VOLLEY, true);
                                    Phase_1_Volley_Timer = urand(10000, 12000);
                                    return;
                                }
                            }
                        }
                    }
                    Phase_1_Volley_Timer = 500;
                }
                else
                    Phase_1_Volley_Timer -= diff;

                break;
            }
            case 2: // PHASE 2
            {
                if (Phase_2_CheckHP_Timer < diff)   //Phase 3 Check
                {
                    if (m_creature->GetHealthPercent() < 20.0f && Phase_2_Taunt_Timer < 55000 && Phase_2_Charge_Timer < 4000)
                    {
                        m_creature->RemoveAllNegativeAuras();
                        DoScriptText(SAY_PHASE3, m_creature);
                        BossPhase = 3;
                        m_creature->CastSpell(m_creature, SPELL_P3_INITIAL_STUN, true);
                        m_creature->CastSpell(m_creature, SPELL_P3_INITIAL_HEAL, true);
                        Phase_2_Consecration_Timer = urand(15000, 20000);
                    }
                    Phase_2_CheckHP_Timer = 1000;
                }
                else Phase_2_CheckHP_Timer -= diff;

                if (Phase_2_IsTauntCharging)    //AoE taunting
                {
                    if (Unit* target = m_creature->GetMap()->GetUnit(BestTauntTargetGuid))
                    {
                        if (m_creature->GetDistance(target) < 5.0f)
                        {
                            m_creature->CastSpell(m_creature, SPELL_AOE_TAUNT, false);
                            BestTauntTargetGuid.Clear();
                            Phase_2_IsTauntCharging = false;
                        }
                    }
                    else
                    {
                        m_creature->CastSpell(m_creature, SPELL_AOE_TAUNT, false);
                        BestTauntTargetGuid.Clear();
                        Phase_2_IsTauntCharging = false;
                    }
                }

                if (Phase_2_Threat_Timer < diff)    //Aggro mechanics
                {
                    DoResetThreat();
                    ThreatList const& tList = m_creature->getThreatManager().getThreatList();
                    for (ThreatList::const_iterator iter = tList.begin(); iter != tList.end(); ++iter)
                    {
                        if (Unit* pUnit = m_creature->GetMap()->GetUnit((*iter)->getUnitGuid()))
                        {
                            m_creature->getThreatManager().addThreatDirectly(pUnit, pUnit->GetHealth() * (m_creature->GetDistance(pUnit) < 30.0f ? 20.0f : 1.0f));
                        }
                    }
                    Phase_2_Threat_Timer = 5000;
                }
                else
                    Phase_2_Threat_Timer -= diff;

                if (Phase_2_Shield_Bash_Timer < diff)   //Shield Bash
                {
                    if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_SHIELD_BASH) == CAST_OK)
                    {
                        Phase_2_Shield_Bash_Timer = 6000;
                        if (urand(0, 5) == 0)
                            DoScriptText(SAY_ALL_DAY, m_creature);
                    }
                }
                else
                    Phase_2_Shield_Bash_Timer -= diff;

                if (Phase_2_Charge_Timer < diff)    //Charge when too far
                {
                    if (m_creature->GetDistance(m_creature->getVictim()) > 10.0f)
                    {
                        m_creature->CastSpell(m_creature->getVictim(), SPELL_CHARGE, true);
                        Phase_2_Charge_Timer = 6000;
                    }
                    else
                        Phase_2_Charge_Timer = 500;

                }
                else
                    Phase_2_Charge_Timer -= diff;
                

                if (Phase_2_Consecration_Timer < diff)  //Consecration
                {
                    if (DoCastSpellIfCan(m_creature, SPELL_CONSECRATION1) == CAST_OK)
                    {
                        if (Unit* randomTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_FARTHEST, urand(0, 2), nullptr, SELECT_FLAG_PLAYER))
                        {
                            m_creature->CastSpell(randomTarget, SPELL_CONSECRATION2, true);
                        }
                        Phase_2_Consecration_Timer = 8000;
                    }
                }
                else
                    Phase_2_Consecration_Timer -= diff;
                
                    
                if (Phase_2_AddSpawn_Timer < diff)  //Spawn recruits
                {
                    DoScriptText(SAY_ABBENDIS_RESERVES, m_creature);
                    SummonRecruit();
                    Phase_2_AddSpawn_Timer = 40000;
                    Phase_2_AddSpawn_Timer2 = 3000;
                    Phase_2_Taunt_Timer = 10000;
                }
                else
                    Phase_2_AddSpawn_Timer -= diff;
                if (Phase_2_AddSpawn_Timer2 < diff) //Wave 2
                {
                    SummonRecruit();
                    Phase_2_AddSpawn_Timer2 = 50000;
                }
                else
                    Phase_2_AddSpawn_Timer2 -= diff;

                if (Phase_2_Taunt_Timer < diff) //Taunt
                {
                    PreTaunt();
                    Phase_2_Taunt_Timer = 60000;
                }
                else
                    Phase_2_Taunt_Timer -= diff;


                break;
            }
            case 3: // PHASE 3
            {
                if (!Phase_3_ChargerSummoned)   //Mounting
                {
                    if (Phase_3_ChargerDelay < diff)
                    {
                        int SpawnPos = 0;
                        if (m_creature->GetDistance2d(ChargerMountSpawnLocs[0][0], ChargerMountSpawnLocs[0][1]) < m_creature->GetDistance2d(ChargerMountSpawnLocs[1][0], ChargerMountSpawnLocs[1][1]))
                            SpawnPos = 1;

                        if (Creature* Crea = m_creature->SummonCreature(NPC_CHARGER, ChargerMountSpawnLocs[SpawnPos][0], ChargerMountSpawnLocs[SpawnPos][1], ChargerMountSpawnLocs[SpawnPos][2], 0.0f, TEMPSUMMON_TIMED_DESPAWN, 30000))
                        {
                            Crea->GetMotionMaster()->MoveFollow(m_creature, 0.0f, 0.0f);
                            m_creature->GetMotionMaster()->MoveFollow(Crea, 0.0f, 0.0f);
                            Phase_3_ChargerDelay = 30 * MINUTE * IN_MILLISECONDS;
                        }
                    }
                    else
                        Phase_3_ChargerDelay -= diff;

                    if (Phase_3_Charger_Ride_Delay < diff)
                    {
                        if (Creature* Crea = m_creature->FindNearestCreature(NPC_CHARGER, 5.0f))
                        {
                            Crea->DespawnOrUnsummon();
                            Phase_3_ChargerSummoned = true;
                            m_creature->CastSpell(m_creature, SPELL_P3_MOUNT, true);
                            m_creature->RemoveAurasDueToSpell(SPELL_BLOCK_BONUS);
                            m_creature->RemoveAurasDueToSpell(SPELL_ROOT_SELF);
                            DoResetThreat();
                            m_creature->GetMotionMaster()->MoveChase(m_creature->getVictim());

                            ThreatList const& tList = m_creature->getThreatManager().getThreatList();
                            for (ThreatList::const_iterator iter = tList.begin(); iter != tList.end(); ++iter)
                            {
                                if (Unit* pTempTarget = m_creature->GetMap()->GetUnit((*iter)->getUnitGuid()))
                                {
                                    m_creature->getThreatManager().addThreatDirectly(pTempTarget, pTempTarget->GetMaxHealth() * 1000);
                                    if (pTempTarget->HasAura(SPELL_P3_INITIAL_STUN))
                                        pTempTarget->RemoveAurasDueToSpell(SPELL_P3_INITIAL_STUN);  //Remove phase transition stun from enemies on P3 start
                                }
                            }
                        }
                        Phase_3_Charger_Ride_Delay = 200;
                    }
                    else
                        Phase_3_Charger_Ride_Delay -= diff;
                }
                else
                {
                    if (Phase_3_HoofKick_Timer < diff)
                    {
                        if (m_creature->GetDistance(m_creature->getVictim()) < 5.0f)
                        {
                            m_creature->CastSpell(m_creature, SPELL_P3_HOOF_KICK, true);
                            Phase_3_HoofKick_Timer = urand(5000, 8000);
                        }
                        else
                            Phase_3_HoofKick_Timer = 500;
                    }
                    else
                        Phase_3_HoofKick_Timer -= diff;

                    if (Phase_3_Trample_Timer < diff)
                    {
                        if (m_creature->GetDistance(m_creature->getVictim()) > 8.0f)
                        {
                            m_creature->CastSpell(m_creature, SPELL_P3_TRAMPLE, true);
                        }
                        Phase_3_Trample_Timer = 200;
                    }
                    else
                        Phase_3_Trample_Timer -= diff;

                    if (Phase_2_Consecration_Timer < diff)  //Consecration p3 too
                    {
                        if (DoCastSpellIfCan(m_creature, SPELL_CONSECRATION1) == CAST_OK)
                        {
                            if (Unit* randomTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_FARTHEST, urand(0, 2), nullptr, SELECT_FLAG_PLAYER))
                            {
                                m_creature->CastSpell(randomTarget, SPELL_CONSECRATION2, true);
                            }
                            Phase_2_Consecration_Timer = 8000;
                        }
                    }
                    else
                        Phase_2_Consecration_Timer -= diff;

                    if (Phase_3_Switch_Target_Timer < diff)
                    {
                        if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_FARTHEST, 0, nullptr, SELECT_FLAG_PLAYER))
                        {
                            DoResetThreat();
                            m_creature->getThreatManager().addThreatDirectly(pTarget, 10000000);
                            ThreatList const& tList = m_creature->getThreatManager().getThreatList();
                            for (ThreatList::const_iterator iter = tList.begin(); iter != tList.end(); ++iter)
                            {
                                if (Unit* pTempTarget = m_creature->GetMap()->GetUnit((*iter)->getUnitGuid()))
                                {
                                    m_creature->getThreatManager().addThreatDirectly(pTempTarget, pTempTarget->GetMaxHealth() * 1000);
                                }
                            }
                            Phase_3_Switch_Target_Timer = 25000;
                            Phase_3_HoofKick_Timer += 2500;
                            Phase_3_Trample_Timer = 300;

                            int skip = urand(0, 2);
                            for (int i = 0; i < 3; ++i)
                            {
                                if (i != skip)
                                {
                                    float SpawnX = 395.0f;
                                    int RootId = SPELL_P3_SPAWN_1;
                                    if (m_creature->GetDistance2d(ChargerMountSpawnLocs[0][0], ChargerMountSpawnLocs[0][1]) < m_creature->GetDistance2d(ChargerMountSpawnLocs[1][0], ChargerMountSpawnLocs[1][1]))
                                    {
                                        SpawnX = 484.0f;
                                        RootId = SPELL_P3_SPAWN_2;
                                    }

                                    if (Creature* Crea = m_creature->SummonCreature(NPC_CHARGER, SpawnX, ChargerP3Locs[i][1], 48.0f, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 7000))
                                    {
                                        Crea->AddAura(ChargerP3Locs[i][0]);
                                        Crea->AddAura(RootId);
                                    }
                                }
                            }

                        }

                    }
                    else
                        Phase_3_Switch_Target_Timer -= diff;


                    if (Phase_3_Stormbolt_Timer < diff)
                    {
                        if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_P3_STORMBOLT) == CAST_OK)
                            Phase_3_Stormbolt_Timer = 1500;
                    }
                    else
                        Phase_3_Stormbolt_Timer -= diff;
                }
                break;
            }
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_highgeneral_abbendis(Creature* pCreature)
{
    return new boss_highgeneral_abbendisAI(pCreature);
}

void AddSC_boss_highgeneral_abbendis()
{
    Script* newscript;
    newscript = new Script;
    newscript->Name = "boss_highgeneral_abbendis";
    newscript->GetAI = &GetAI_boss_highgeneral_abbendis;
    newscript->RegisterSelf();
}