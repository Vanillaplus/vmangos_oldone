/* Copyright (C) 2006 - 2011 ScriptDev2 <http://www.scriptdev2.com/>
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
SDName: Boss_Nefarian
SD%Complete: 80
SDComment: Some issues with class calls effecting more than one class
SDCategory: Blackwing Lair
EndScriptData */

#include "scriptPCH.h"
#include "blackwing_lair.h"

enum
{
    SAY_AGGRO                   = -1469007,
    SAY_XHEALTH                 = -1469008,             // at 5% hp
    SAY_SHADOWFLAME             = -1469009,
    EMOTE_GENERIC_SHADOW_FLAME  = -1469033,
    SAY_RAISE_SKELETONS         = -1469010,
    SAY_SLAY                    = -1469011,
    SAY_DEATH                   = -1469012,
    EMOTE_ROAR                  = -1249005,

    SAY_MAGE                    = -1469013,
    SAY_WARRIOR                 = -1469014,
    SAY_DRUID                   = -1469015,
    SAY_PRIEST                  = -1469016,
    SAY_PALADIN                 = -1469017,
    SAY_SHAMAN                  = -1469018,
    SAY_WARLOCK                 = -1469019,
    SAY_HUNTER                  = -1469020,
    SAY_ROGUE                   = -1469021,
    //SAY_DEATH_KNIGHT            = -1469031,             // spell unk for the moment

    SPELL_SHADOWFLAME_INITIAL   = 22992,                // old spell id 22972 -> wrong
    SPELL_SHADOWFLAME           = 22539,
    SPELL_BELLOWING_ROAR        = 22686,
    SPELL_VEIL_OF_SHADOW        = 22687,                // old spell id 7068 -> wrong
    SPELL_DRAGON_SICKNESS       = 35324,
    SPELL_CLEAVE                = 20691,
    SPELL_TAIL_LASH             = 23364,
    SPELL_BONE_CONTRUST         = 23363,                //23362, 23361   Missing from DBC!

    SPELL_MAGE                  = 23410,                // wild magic
    SPELL_WARRIOR               = 23397,                // Berserk
    SPELL_WARRIOR_2             = 35320,                // Berserk WW
    SPELL_DRUID_CAT             = 23398,                // cat form
    SPELL_DRUID_BEAR            = 35303,                // bear form
    SPELL_PRIEST                = 23401,                // corrupted healing
    SPELL_PALADIN               = 23418,                // syphon blessing
    SPELL_SHAMAN                = 23425,                // totems
    SPELL_CORRUPTED_TOTEM       = 23424,
    SPELL_WARLOCK               = 35364,                // OLD 23427, infernals    -> should trigger 23426
    SPELL_HUNTER                = 35322,                // bow broke ole 23436
    SPELL_ROGUE                 = 23414,                // Paralise

    SPELL_WILD_POLYMORPH_CAST   = 23603,
    SPELL_WILD_PYROBLAST_CAST   = 35309,
    SPELL_WILD_WIND             = 35310,
    SPELL_WILD_POLYMORPH        = 35377,
    SPELL_WILD_PYROBLAST        = 35378,

    NPC_BONE_CONSTRUCT          = 14605,
    NPC_VAEL                    = 25123,                // Last found data : HP : 3,331,000 (+20% = 3997200). Damage: 5,930 - 6,807. Armor: 4,691.

    // Vael Emotes
    SAY_LINE_1                  = -1469026,             // Too late, friends! Nefarius' corruption has taken hold...I cannot...control myself.
    SAY_LINE_2                  = -1469027,             // I beg you, mortals - FLEE! Flee before I lose all sense of control! The black fire rages within my heart! I MUST- release it!
    SAY_LINE_3                  = -1469028,             // FLAME! DEATH! DESTRUCTION! Cower, mortals before the wrath of Lord...NO - I MUST fight this! Alexstrasza help me, I MUST fight it!
    SAY_HALFLIFE                = -1469029,             // Nefarius' hate has made me stronger than ever before! You should have fled while you could, mortals! The fury of Blackrock courses through my veins!
    SAY_KILLTARGET              = -1469030,             // Forgive me, $N! Your death only adds to my failure!
    // Vael Spells
    SPELL_ESSENCE_OF_THE_RED    = 23513,
    SPELL_CLEAR_ESSENCE_RED     = 23681,
    SPELL_FLAME_BREATH          = 23461,
    SPELL_FIRE_NOVA_AURA        = 35301,                // Inflicts 555-645 Fire damage to nearby enemies. OG 23462
    SPELL_TAIL_SWEEP            = 15847,                // Inflicts 925 to 1075 (600 à 700) damage on enemies in a cone behind the caster, knocking them back.
    SPELL_WING_BUFFET_SINGLE    = 35302,                // Knockbacks low threat taunters

    // TO FIX : explode at the end
    SPELL_BURNING_ADRENALINE    = 23620,                // 18173 ?
    SPELL_BURNING_ADRENALINE2   = 23478,                // AOE explosion
    SPELL_BURNING_ADRENALINE3   = 23644,                // instakill
    SPELL_BURNING_ADRENALINE4   = 24701,                // -75% threat
    SPELL_CHAIN_CLEAVE          = 22540,                // 22540 Chain Cleave hardcoded via the BD (Ustaag) // 19983 // 20684  ? // Chain cleave is most likely named something different and contains a dummy effect

    SPELL_NEFARIAN_SHACKLES     = 35293,                // Chains anim
    SPELL_BANISHEMENT_OF_SCALE  = 35292,                // Initiation spell. In OG: Green aura 16404

    FACTION_BLACK_DRAGON        = 103,
    FACTION_FRIENDLY            = 35,
};

struct ClassCallInfo
{
    ClassCallInfo(uint8 uiClass, int32 uiYell) : m_uiClass(uiClass), m_uiYell(uiYell) {}
    uint8 m_uiClass;
    int32 m_uiYell;
};

struct boss_nefarianAI : ScriptedAI
{
    explicit boss_nefarianAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = static_cast<ScriptedInstance*>(pCreature->GetInstanceData());

        boss_nefarianAI::Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 m_uiShadowFlameTimer;
    uint32 m_uiBellowingRoarTimer;
    uint32 m_uiVeilOfShadowTimer;
    uint32 m_uiCleaveTimer;
    uint32 m_uiTailLashTimer;
    uint32 m_uiClassCallTimer;
    uint32 m_uiDragonSicknessTimer;
    uint32 m_uiDragonSickness2Timer;
    uint32 m_uiBoneConstructTimer;
    bool m_bPhase3;
    bool m_bHasEndYell;
    uint8 m_uiTransitionStage;
    uint32 m_uiTransitionTimer;
    bool m_bTransitionDone;
    bool m_bWarriorStance;
    uint32 MageSpellId;
    uint32 chanceMage;
    WorldLocation Rloc;
    bool RposSearching;
    

    std::vector<ClassCallInfo> m_vPossibleCalls;
    std::list<ObjectGuid> MagePlayerGUID;
    uint32 m_uiMageTriggerTimer;

    void Reset() override
    {
        m_uiShadowFlameTimer    = urand(18000, 25000);
        m_uiBellowingRoarTimer  = urand(25000, 30000);
        m_uiVeilOfShadowTimer   = 15000;
        m_uiCleaveTimer         = urand(7000, 10000);
        m_uiTailLashTimer       = 10000;
        m_uiClassCallTimer      = urand(29000, 31000);                            // 25-35 seconds
        m_uiDragonSicknessTimer = urand(25000, 40000);
        m_uiDragonSickness2Timer = 600000;
        m_uiBoneConstructTimer  = 60 * MINUTE * IN_MILLISECONDS;
        m_bPhase3               = false;
        m_bHasEndYell           = false;
        m_bTransitionDone       = false;
        m_bWarriorStance        = false;
        m_uiTransitionStage     = 0;
        m_uiTransitionTimer     = 100;
        m_uiMageTriggerTimer    = 0;
        MageSpellId             = SPELL_WILD_POLYMORPH;
        chanceMage              = 0;
        m_creature->GetPosition(Rloc);
        RposSearching           = true;


        MagePlayerGUID.clear();
        m_vPossibleCalls.clear();
        m_vPossibleCalls.push_back(ClassCallInfo(CLASS_WARRIOR, SAY_WARRIOR));
        m_vPossibleCalls.push_back(ClassCallInfo(CLASS_PALADIN, SAY_PALADIN));
        m_vPossibleCalls.push_back(ClassCallInfo(CLASS_HUNTER, SAY_HUNTER));
        m_vPossibleCalls.push_back(ClassCallInfo(CLASS_ROGUE, SAY_ROGUE));
        m_vPossibleCalls.push_back(ClassCallInfo(CLASS_PRIEST, SAY_PRIEST));
        m_vPossibleCalls.push_back(ClassCallInfo(CLASS_SHAMAN, SAY_SHAMAN));
        m_vPossibleCalls.push_back(ClassCallInfo(CLASS_MAGE, SAY_MAGE));
        m_vPossibleCalls.push_back(ClassCallInfo(CLASS_WARLOCK, SAY_WARLOCK));
        m_vPossibleCalls.push_back(ClassCallInfo(CLASS_DRUID, SAY_DRUID));
    }

    void KilledUnit(Unit* pVictim) override
    {
        if (!pVictim || pVictim->GetTypeId() != TYPEID_PLAYER)
            return;

        if (m_creature->HasAura(34823))
            return;

        m_creature->CastSpell(m_creature, 34823, true);
        DoScriptText(SAY_SLAY, m_creature, pVictim);
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        DoScriptText(SAY_DEATH, m_creature);

        if (m_pInstance)
            m_pInstance->SetData(TYPE_NEFARIAN, DONE);

        std::list<Creature*> Vael;
        GetCreatureListWithEntryInGrid(Vael, m_creature, NPC_VAEL, 250.0f);
        for (std::list<Creature*>::iterator itr = Vael.begin(); itr != Vael.end(); ++itr)
        {
            if ((*itr)->isAlive())
            {
                (*itr)->AI()->EnterEvadeMode();
                (*itr)->setFaction(FACTION_FRIENDLY);
                (*itr)->RemoveAurasDueToSpell(35293); // Shackles visual
            }
        }
    }

    void EnterEvadeMode() override
    {
        if (m_pInstance)
        {
            m_pInstance->SetData(TYPE_NEFARIAN, FAIL);
            if (Creature* pNefarius = m_creature->GetMap()->GetCreature(m_pInstance->GetData64(DATA_NEFARIUS_GUID)))
                pNefarius->AI()->EnterEvadeMode();
        }
        m_creature->DeleteLater();
    }

    void JustSummoned(Creature* pSummoned) override
    {
        pSummoned->SetInCombatWithZone();
    }

    void MovementInform(uint32 uiType, uint32 uiPointId) override
    {
        if (uiType != POINT_MOTION_TYPE)
            return;

        switch (uiPointId)
        {
            case 1:
                m_creature->GetMotionMaster()->MovePoint(2, -7495.964f, -1252.402f, 476.795f, MOVE_FORCE_DESTINATION, 17.0f);
                m_uiTransitionTimer = 0;
                break;
            case 2:
                m_creature->GetMotionMaster()->MoveIdle();
                m_uiTransitionTimer = 100;
                break;
            default:
                break;
        }
    }

    /*
    void SpellHitTarget(Unit* pTarget, const SpellEntry* pSpell)
    {
        if (!pTarget)
            return;

        if (pSpell->Id == SPELL_BELLOWING_ROAR)
            if (SpellAuraHolder* holder = pTarget->GetSpellAuraHolder(SPELL_BELLOWING_ROAR))
                holder->SetTargetSecondaryThreatFocus(true);
    }
    */

    bool HandleClassCall(uint8 ClassCalled)
    {
        if (!ClassCalled)
            return false;

        Map::PlayerList const& players = m_creature->GetMap()->GetPlayers();
        if (players.isEmpty())
            return false;

        bool bClassFound = false;
        // Clear mage GUIDs or we'll be adding the same players again for subsequent
        // mage class calls, resulting in far more polymorphs than intended
        MagePlayerGUID.clear();
        for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
        {
            Player* pPlayer = itr->getSource();
            if (pPlayer && pPlayer->isAlive() && !pPlayer->HasAura(23174) && !pPlayer->IsGameMaster())
            {
                if (pPlayer->getClass() == ClassCalled)
                {
                    bClassFound = true;
                    switch (ClassCalled)
                    {
                    case CLASS_WARRIOR:
                        pPlayer->AddAura(SPELL_WARRIOR); //OK
                        pPlayer->AddAura(SPELL_WARRIOR_2);
                        break;
                    case CLASS_PALADIN:
                        pPlayer->CastSpell(pPlayer, SPELL_PALADIN, true); // OK
                        break;
                    case CLASS_HUNTER:
                        pPlayer->CastSpell(pPlayer, SPELL_HUNTER, true); // OK
                        break;
                    case CLASS_ROGUE:
                    {
                        Unit* pVictim = m_creature->getVictim();

                        if (pVictim && pPlayer != pVictim)
                        {
                            for (uint8 attempt = 0; attempt < 100; ++attempt)
                            {
                                pVictim->GetPosition(Rloc);

                                Rloc.coord_x += frand(0.0f, 10.0f) - 5.0f;
                                Rloc.coord_y += frand(0.0f, 10.0f) - 5.0f;
                                Rloc.orientation = m_creature->GetOrientation() + M_PI_F;

                                if (m_creature->GetDistanceToCenter(pVictim) > m_creature->GetDistance(Rloc.coord_x, Rloc.coord_y, Rloc.coord_z) && m_creature->IsWithinLOS(Rloc.coord_x, Rloc.coord_y, Rloc.coord_z))
                                {
                                    Rloc.coord_z += frand(5.0f, 8.5f);
                                    pPlayer->TeleportTo(Rloc);
                                    break;
                                }
                            }
                        }
                        pPlayer->AddAura(SPELL_ROGUE);
                        break;
                    }
                    case CLASS_PRIEST:
                        pPlayer->AddAura(SPELL_PRIEST); // OK
                        break;
                    case CLASS_SHAMAN:
                        pPlayer->AddAura(SPELL_SHAMAN); // OK
                        break;
                    case CLASS_MAGE:
                        pPlayer->AddAura(SPELL_MAGE); // OK
                        MagePlayerGUID.push_back(pPlayer->GetObjectGuid());
                        break;
                    case CLASS_WARLOCK:
                        pPlayer->CastSpell(pPlayer, SPELL_WARLOCK, true); // OK
                        m_creature->CastSpell(pPlayer, 23426, true);
                        break;
                    case CLASS_DRUID:
                        if (pPlayer->HasAura(768))  //if cat
                        {
                            pPlayer->RemoveAurasDueToSpell(768);
                            pPlayer->AddAura(SPELL_DRUID_BEAR);
                        }
                        else if (pPlayer->HasAura(5487))  //if bear
                        {
                            pPlayer->RemoveAurasDueToSpell(5487);
                            pPlayer->AddAura(SPELL_DRUID_CAT);
                        }
                        else
                        {
                            if (pPlayer->HasAura(24858)) //de-boom
                                pPlayer->RemoveAurasDueToSpell(24858);

                            if (urand(0,1) == 0)
                                pPlayer->AddAura(SPELL_DRUID_BEAR);
                            else
                                pPlayer->AddAura(SPELL_DRUID_CAT);

                        }
                        break;
                    }
                }
            }
        }
        m_bWarriorStance = bClassFound && ClassCalled == CLASS_WARRIOR;

        return bClassFound;
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (m_uiTransitionTimer && !m_bTransitionDone)
        {
            if (m_uiTransitionTimer <= uiDiff)
            {
                switch (m_uiTransitionStage)
                {
                    case 0:
                        SetCombatMovement(false);
                        m_creature->SetInCombatWithZone();
                        m_creature->SetFly(true);

                        m_creature->CastSpell(m_creature, SPELL_SHADOWFLAME_INITIAL, true); // Test speed 17 / fire damage on the initial ??? Oo

                        DoScriptText(SAY_AGGRO, m_creature);

                        m_creature->GetMotionMaster()->MovePoint(1, -7449.145f, -1320.647f, 476.795f);
                        //m_creature->MonsterMove(-7449.145f, -1320.647f, 476.795f);
                        m_creature->MonsterMoveWithSpeed(-7449.145f, -1320.647f, 476.795f, -10.0f, 17, uint32(MOVE_FORCE_DESTINATION));
                        m_uiTransitionTimer = 0;
                        break;
                    case 1:
                        m_creature->SetFly(false);
                        DoScriptText(SAY_SHADOWFLAME, m_creature);
                        m_creature->HandleEmote(EMOTE_ONESHOT_LAND);
                        m_uiTransitionTimer = 1000;
                        break;
                    case 2:
                        m_creature->SetWalk(true);
                        m_creature->RemoveAurasDueToSpell(17131);
                        if (Unit* pTarget = m_creature->getVictim())
                        {
                            //m_creature->AI()->AttackStart(pTarget);
                            //m_creature->GetMotionMaster()->Clear(false);
                            m_creature->GetMotionMaster()->MoveChase(pTarget);
                            SetCombatMovement(true);
                        }
                        m_bTransitionDone = true;
                        break;
                }
                ++m_uiTransitionStage;
            }
            else
                m_uiTransitionTimer -= uiDiff;
        }

        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim() || !m_bTransitionDone)
            return;


        // ShadowFlame_Timer
        if (m_uiShadowFlameTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_SHADOWFLAME) == CAST_OK)
            {
                DoScriptText(EMOTE_GENERIC_SHADOW_FLAME, m_creature);
                m_uiShadowFlameTimer = urand(18000, 25000);
            }
        }
        else
            m_uiShadowFlameTimer -= uiDiff;

        // BellowingRoar_Timer
        if (m_uiBellowingRoarTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_BELLOWING_ROAR) == CAST_OK)
            {
                DoScriptText(EMOTE_ROAR, m_creature);
                m_uiBellowingRoarTimer = urand(25000, 30000);
            }
        }
        else
            m_uiBellowingRoarTimer -= uiDiff;

        // VeilOfShadow_Timer
        if (m_uiVeilOfShadowTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_VEIL_OF_SHADOW) == CAST_OK)
                m_uiVeilOfShadowTimer = urand(11000, 15000);
        }
        else
            m_uiVeilOfShadowTimer -= uiDiff;

        // DragonSickness_Timer
        if (m_uiDragonSicknessTimer < uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, nullptr, SELECT_FLAG_PLAYER))
            {
                if (!pTarget->HasAura(23174))
                {
                    if (DoCastSpellIfCan(pTarget, SPELL_DRAGON_SICKNESS) == CAST_OK)
                    {
                        m_uiDragonSicknessTimer = urand(10000, 15000);
                        m_uiDragonSickness2Timer = 9500;
                    }
                }
            }
        }
        else
            m_uiDragonSicknessTimer -= uiDiff;

        if (m_uiDragonSickness2Timer < uiDiff)
        {
            ThreatList const& tList = m_creature->getThreatManager().getThreatList();
            for (ThreatList::const_iterator itr = tList.begin(); itr != tList.end(); ++itr)
            {
                if (Player* pPlayer = m_creature->GetMap()->GetPlayer((*itr)->getUnitGuid()))
                {
                    if (pPlayer->HasAura(SPELL_DRAGON_SICKNESS))
                    {
                        if (Creature* MutationNPC = m_creature->SummonCreature(25126, pPlayer->GetPositionX(), pPlayer->GetPositionY(), pPlayer->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 300500))
                        {
                            MutationNPC->SetInCombatWithZone();
                            m_creature->getThreatManager().modifyThreatPercent(pPlayer, -100);
                            MutationNPC->CastSpell(pPlayer, 23174, true);
                            m_uiDragonSickness2Timer = 600000;
                            break;
                        }
                    }
                }
            }
        }
        else
            m_uiDragonSickness2Timer -= uiDiff;

        // Cleave_Timer
        if (m_uiCleaveTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_CLEAVE) == CAST_OK)
                m_uiCleaveTimer = urand(7000, 10000);
        }
        else
            m_uiCleaveTimer -= uiDiff;

        // TailLash_Timer
        if (m_uiTailLashTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_TAIL_LASH) == CAST_OK)
                m_uiTailLashTimer = urand(4000, 8000);
        }
        else
            m_uiTailLashTimer -= uiDiff;

        // ClassCall_Timer
        if (m_uiClassCallTimer < uiDiff)
        {
            //Cast a random class call
            //On official it is based on what classes are currently on the hostil list
            //but we can't do that yet so just randomly call one
            // We do it now
            if (!m_vPossibleCalls.empty())
            {
                uint8 uiRandClass = urand(0, m_vPossibleCalls.size() - 1);
                if (HandleClassCall(m_vPossibleCalls[uiRandClass].m_uiClass))
                {
                    DoScriptText(m_vPossibleCalls[uiRandClass].m_uiYell, m_creature);
                    m_uiClassCallTimer = urand(29000, 31000);

                    if (m_uiShadowFlameTimer < 3000)
                        m_uiShadowFlameTimer = 3000;
                }
                else
                    m_vPossibleCalls.erase(m_vPossibleCalls.begin() + uiRandClass);
            }
        }
        else
            m_uiClassCallTimer -= uiDiff;

        if (!MagePlayerGUID.empty())
        {
            if (m_uiMageTriggerTimer < uiDiff)
            {
                for (std::list<ObjectGuid>::const_iterator itr = MagePlayerGUID.begin(); itr != MagePlayerGUID.end(); ++itr)
                {
                    if (Player* pMage = m_creature->GetMap()->GetPlayer(*itr))
                    {
                        if (pMage->HasAura(SPELL_MAGE))
                        {
                            chanceMage = urand(0, 2);
                            if (chanceMage == 0)
                                MageSpellId = SPELL_WILD_POLYMORPH_CAST;
                            else if (chanceMage == 1)
                                MageSpellId = SPELL_WILD_PYROBLAST_CAST;
                            else if (chanceMage == 2)
                                MageSpellId = SPELL_WILD_WIND;

                            if (MageSpellId == SPELL_WILD_WIND)
                            {
                                pMage->CastSpell(pMage, SPELL_WILD_WIND, true);
                            }
                            else
                            {
                                std::vector<ObjectGuid> m_vPossibleVictim;
                                ThreatList const& tList = m_creature->getThreatManager().getThreatList();
                                for (ThreatList::const_iterator itr2 = tList.begin(); itr2 != tList.end(); ++itr2)
                                {
                                    if (Player* pUnit = m_creature->GetMap()->GetPlayer((*itr2)->getUnitGuid()))
                                    {
                                        if (pUnit != pMage && pUnit->GetDistance(pMage) < 60.0f && !pUnit->HasAura(23174))
                                        {
                                            if (MageSpellId == SPELL_WILD_POLYMORPH_CAST)
                                            {
                                                if (!pUnit->HasAura(SPELL_WILD_POLYMORPH))
                                                    m_vPossibleVictim.push_back(pUnit->GetObjectGuid());
                                            }
                                            else if (MageSpellId == SPELL_WILD_PYROBLAST_CAST)
                                            {
                                                if (!pUnit->HasAura(SPELL_WILD_PYROBLAST))
                                                    m_vPossibleVictim.push_back(pUnit->GetObjectGuid());
                                            }
                                        }

                                    }

                                }

                                if (m_vPossibleVictim.empty())
                                    continue;

                                if (Unit* pTarget = m_creature->GetMap()->GetUnit(m_vPossibleVictim[urand(0, m_vPossibleVictim.size() - 1)]))
                                {
                                    pMage->InterruptNonMeleeSpells(false);
                                    pMage->CastSpell(pTarget, MageSpellId, true); //cast polymorph
                                    /*sLog.outError("test");*/
                                }
                            }
                        }
                    }
                }
                m_uiMageTriggerTimer = urand(3000, 5000);
            }
            else
                m_uiMageTriggerTimer -= uiDiff;
        }

        // Phase3 begins when we are below X health
        if (!m_bPhase3)
        {
            if (m_creature->GetHealthPercent() < 30.0f)
            {
                DoScriptText(SAY_RAISE_SKELETONS, m_creature);
                m_uiBoneConstructTimer = 2000;
                m_bPhase3 = true;
            }

        }

        if (m_uiBoneConstructTimer < uiDiff)
        {
            m_uiBoneConstructTimer = 5000;

            std::list<GameObject*> GOListe;
            GetGameObjectListWithEntryInGrid(GOListe, m_creature, 179804, 200.0f);
            int waves = 0;

            if (GOListe.empty())
            {
                m_uiBoneConstructTimer = 60 * MINUTE * IN_MILLISECONDS;
                return;
            }

            for (auto itr = GOListe.begin(); itr != GOListe.end(); ++itr)
            {
                if (waves < 15)
                {
                    m_creature->SummonCreature(NPC_BONE_CONSTRUCT,
                        (*itr)->GetPositionX(),
                        (*itr)->GetPositionY(),
                        (*itr)->GetPositionZ(),
                        (*itr)->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                    (*itr)->DeleteLater();

                    ++waves;
                }
            }
        }
        else
            m_uiBoneConstructTimer -= uiDiff;

        // 5% hp yell
        if (!m_bHasEndYell && m_creature->GetHealthPercent() < 5.0f)
        {
            m_bHasEndYell = true;
            DoScriptText(SAY_XHEALTH, m_creature);
        }

        if (DoMeleeAttackIfReady())
            if (m_creature->HasAura(10612) && !m_creature->HasAura(10610))
                if (!urand(0, 4))
                    m_creature->CastSpell(m_creature, 10610, true);
    }
};

CreatureAI* GetAI_boss_nefarian(Creature* pCreature)
{
    return new boss_nefarianAI(pCreature);
}

//struct npc_corrupted_totemAI : ScriptedAI
//{
//    explicit npc_corrupted_totemAI(Creature* pCreature) : ScriptedAI(pCreature)
//    {
//        m_uiCreatureEntry = pCreature->GetEntry();
//
//        uint32 HP = urand(200, 2000);
//        pCreature->SetMaxHealth(HP);
//        pCreature->SetHealth(HP);
//
//        m_bAuraAdded = false;
//
//        npc_corrupted_totemAI::Reset();
//    }
//
//    uint32 m_uiCreatureEntry;
//    uint32 m_uiCheckTimer;
//    bool m_bAuraAdded;
//
//    void Reset() override
//    {
//        m_creature->addUnitState(UNIT_STAT_ROOT);
//
//        if (!m_creature->HasAura(23973))
//            m_creature->AddAura(23973);
//
//        m_creature->AddAura(23198, ADD_AURA_PERMANENT); // Avoidance: not affected by AoE
//        m_uiCheckTimer = 1000;
//    }
//
//    void Aggro(Unit* /*pWho*/) override
//    {
//        m_creature->SetInCombatWithZone();
//    }
//
//    void JustDied(Unit* /*pKiller*/) override
//    {
//        switch (m_uiCreatureEntry)
//        {
//            case 14663:
//                SetAura(false, 10405);
//                break;    // Corrupted Stoneskin Totem VI
//            case 14664:
//                SetAura(false, 10461);
//                break;    // Corrupted Healing Stream Totem V
//            case 14666:
//                SetAura(false, 10612);
//                break;    // Corrupted Windfury Totem III
//            default:
//                break;
//        }
//    }
//
//    void SetAura(bool on, uint32 uiSpellId) const
//    {
//        int damage = 0;
//        switch (uiSpellId)
//        {
//            case 10405:
//                damage = -310;
//                break;     // Stoneskin : base -31
//            case 10461:
//                damage = 14000;
//                break;     // Healing Stream : base 14
//            default:
//                damage = 0;
//                break;
//        }
//
//        std::vector<uint32> mobsEntries;
//        std::vector<uint32>::iterator entriesIt;
//        mobsEntries.push_back(11583); // Nefarian
//        mobsEntries.push_back(14605); // Bone Construct
//        mobsEntries.push_back(14263); // NPC_BRONZE_DRAKANOID
//        mobsEntries.push_back(14261); // NPC_BLUE_DRAKANOID
//        mobsEntries.push_back(14264); // NPC_RED_DRAKANOID
//        mobsEntries.push_back(14262); // NPC_GREEN_DRAKANOID
//        mobsEntries.push_back(14265); // NPC_BLACK_DRAKANOID
//        mobsEntries.push_back(14302); // NPC_CHROMATIC_DRAKANOID
//
//        for (entriesIt = mobsEntries.begin(); entriesIt != mobsEntries.end(); ++entriesIt)
//        {
//            std::list<Creature*> tmpMobsList;
//            GetCreatureListWithEntryInGrid(tmpMobsList, m_creature, *entriesIt, 55.0f);
//            while (!tmpMobsList.empty())
//            {
//                Creature* curr = tmpMobsList.front();
//                tmpMobsList.pop_front();
//
//                if (!curr->isAlive())
//                    continue;
//
//                if (on && m_creature->isAlive())
//                {
//                    if (m_creature->IsWithinDistInMap(curr, 40.0f))
//                    {
//                        if (!curr->HasAura(uiSpellId))
//                        {
//                            if (damage)
//                                curr->CastCustomSpell(curr, uiSpellId, &damage, nullptr, nullptr, true);
//                            else
//                                curr->AddAura(uiSpellId);
//                        }
//                    }
//                    else
//                    {
//                        if (curr->HasAura(uiSpellId))
//                            curr->RemoveAurasDueToSpell(uiSpellId);
//                    }
//                }
//                else
//                    curr->RemoveAurasDueToSpell(uiSpellId);
//            }
//        }
//    }
//
//    void UpdateAI(const uint32 uiDiff) override
//    {
//        if (!m_creature->HasAura(23973))
//            m_creature->AddAura(23973);
//
//        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
//            return;
//
//        uint32 addAuraEntry = 0;
//        switch (m_uiCreatureEntry)
//        {
//            case 14662:         // Corrupted Fire Nova Totem V
//                if (!m_bAuraAdded)
//                {
//                    m_creature->AddAura(11311);  // Fire Nova
//                    m_bAuraAdded = true;
//                    m_creature->DeleteLater();
//                    return;
//                }
//                break;
//            case 14663:         // Corrupted Stoneskin Totem VI
//                addAuraEntry = 10405;    // Stoneskin -30 dmg really ???
//                break;
//            case 14664:         // Corrupted Healing Stream Totem V
//                addAuraEntry = 10461;    // Healing Stream +14 hp really ???
//                break;
//            case 14666:         // Corrupted Windfury Totem III
//                addAuraEntry = 10612;    // Windfury Totem Passive
//                break;
//        }
//        if (!addAuraEntry)
//            return;
//
//        if (m_uiCheckTimer < uiDiff)
//        {
//            m_uiCheckTimer = 1000; // Add immune to AoE + fear
//            SetAura(true, addAuraEntry);    // Stoneskin -30 dmg really ???
//        }
//        else
//            m_uiCheckTimer -= uiDiff;
//    }
//};
//
//CreatureAI* GetAI_npc_corrupted_totem(Creature* pCreature)
//{
//    return new npc_corrupted_totemAI(pCreature);
//}



struct npc_vaelAI : public ScriptedAI
{
    npc_vaelAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();

        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 m_uiSpeechTimer;
    uint32 m_uiSpeechNum;
    uint32 m_uiCleaveTimer;
    uint32 m_uiFlameBreathTimer;
    uint32 m_uiFireNovaTimer;
    uint32 m_uiBurningAdrenalineCasterTimer;
    uint32 m_uiBurningAdrenalineTankTimer;
    uint32 m_uiTailSweepTimer;
    bool m_bHasYelled;
    bool m_bIsDoingSpeech;
    bool Engaged;
    bool Chained;

    void Reset()
    {
        m_uiSpeechTimer                  = 0;
        m_uiSpeechNum                    = 0;
        m_uiCleaveTimer                  = 6000;
        m_uiFlameBreathTimer             = 8000;
        m_uiBurningAdrenalineCasterTimer = 15000;
        m_uiBurningAdrenalineTankTimer   = 45000;
        m_uiFireNovaTimer                = 4000;
        m_uiTailSweepTimer               = 8000;
        m_bHasYelled = false;
        m_bIsDoingSpeech = false;
        Engaged = false;

        m_creature->SetHealthPercent(30.0f);
    }

    void JustDied(Unit* /*pKiller*/)
    {
        m_creature->CastSpell(m_creature, SPELL_CLEAR_ESSENCE_RED, true);
    }

    void KilledUnit(Unit* pVictim)
    {
        if (!pVictim || pVictim->GetTypeId() != TYPEID_PLAYER)
            return;

        if (m_creature->HasAura(34823))
            return;

        m_creature->CastSpell(m_creature, 34823, true);
        DoScriptText(SAY_KILLTARGET, m_creature, pVictim);
    }

    void SpellHit(Unit* pWho, const SpellEntry* pSpell)
    {
        if (!Engaged)
        {
            if (pSpell->Id == SPELL_BANISHEMENT_OF_SCALE)
            {
                Engaged = true;

                m_uiSpeechTimer = 2000;
                m_uiSpeechNum = 0;
                m_bIsDoingSpeech = true;
            }
        }
        
        if (pSpell->HasAura(SPELL_AURA_MOD_TAUNT) || pSpell->HasEffect(SPELL_EFFECT_ATTACK_ME))
        {
            if (m_creature->getThreatManager().getThreat(pWho))
            {
                if (m_creature->getThreatManager().getThreatList().size() > 3)
                {
                    Unit* t0 = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 0, nullptr);
                    Unit* t1 = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 1, nullptr);
                    Unit* t2 = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 2, nullptr);
                    Unit* t3 = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 3, nullptr);

                    if (!(pWho == t0 || pWho == t1 || pWho == t2 || pWho == t3))
                    {
                        if (pSpell->HasAura(SPELL_AURA_MOD_TAUNT))
                            m_creature->RemoveAurasDueToSpell(pSpell->Id);

                        m_creature->CastSpell(pWho, 35302, true);
                        m_creature->getThreatManager().modifyThreatPercent(pWho, -50);
                        DoScriptText(-1900266, m_creature);				// Miserable and ignorant.
                    }
                }
            }

        }
    }

    void UpdateAI(const uint32 uiDiff)
    {
        // Speech
        if (m_bIsDoingSpeech)
        {
            if (m_uiSpeechTimer < uiDiff)
            {
                switch (m_uiSpeechNum)
                {
                    case 0:
                        m_creature->SetStandState(UNIT_STAND_STATE_STAND);
                        DoScriptText(SAY_LINE_3, m_creature);
                        m_uiSpeechTimer = 10300;

                        m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        m_creature->setFaction(FACTION_BLACK_DRAGON);
                        m_creature->SetInCombatWithZone();
                        m_creature->AddAura(35294);

                        ++m_uiSpeechNum;
                        break;
                    case 1:
                        m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        m_creature->CastSpell(m_creature, SPELL_ESSENCE_OF_THE_RED, true);

                        m_uiSpeechTimer = 0;
                        m_bIsDoingSpeech = false;
                        break;
                }
            }
            else
                m_uiSpeechTimer -= uiDiff;
        }

        // Return since we have no target
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        // Yell if hp lower than 15%
        if (m_creature->GetHealthPercent() < 15.0f && !m_bHasYelled)
        {
            DoScriptText(SAY_HALFLIFE, m_creature);
            m_bHasYelled = true;
        }

        // Burning Adrenaline non-tank Timer
        if (m_uiBurningAdrenalineCasterTimer < uiDiff)
        {
            Unit* ot1 = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 1, nullptr, SELECT_FLAG_PLAYER | SELECT_FLAG_IN_MELEE_RANGE);
            Unit* ot2 = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 2, nullptr, SELECT_FLAG_PLAYER | SELECT_FLAG_IN_MELEE_RANGE);
            Unit* ot3 = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 3, nullptr, SELECT_FLAG_PLAYER | SELECT_FLAG_IN_MELEE_RANGE);

            if (Unit* BATarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, nullptr, SELECT_FLAG_PLAYER))
            {
                if (BATarget != m_creature->getVictim() && BATarget != ot1 && BATarget != ot2 && BATarget != ot3 && !BATarget->HasAura(SPELL_BURNING_ADRENALINE))
                {
                    BATarget->CastSpell(BATarget, SPELL_BURNING_ADRENALINE, true);
                    BATarget->CastSpell(BATarget, SPELL_BURNING_ADRENALINE4, true);
                }
            }
            m_uiBurningAdrenalineCasterTimer = 15000;
        }
        else
            m_uiBurningAdrenalineCasterTimer -= uiDiff;

        // Burning Adrenaline Tank Timer
        if (m_uiBurningAdrenalineTankTimer < uiDiff)
        {
            if (m_creature->getVictim() && !m_creature->getVictim()->HasAura(SPELL_BURNING_ADRENALINE) && m_creature->getVictim()->isAlive())
            {
                m_creature->getVictim()->CastSpell(m_creature->getVictim(), SPELL_BURNING_ADRENALINE, true);
                m_creature->getVictim()->CastSpell(m_creature->getVictim(), SPELL_BURNING_ADRENALINE4, true);
                m_uiBurningAdrenalineTankTimer = 45000;
            }
        }
        else
            m_uiBurningAdrenalineTankTimer -= uiDiff;

        // Cleave Timer
        if (m_uiCleaveTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_CHAIN_CLEAVE) == CAST_OK)
                m_uiCleaveTimer = urand(5000, 10000);
        }
        else
            m_uiCleaveTimer -= uiDiff;

        // Flame Breath Timer
        if (m_uiFlameBreathTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_FLAME_BREATH) == CAST_OK)
                m_uiFlameBreathTimer = urand(6000, 9000);
        }
        else
            m_uiFlameBreathTimer -= uiDiff;

        // Fire Nova Timer
        if (m_uiFireNovaTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_FIRE_NOVA_AURA) == CAST_OK)
                m_uiFireNovaTimer = 4000;
        }
        else
            m_uiFireNovaTimer -= uiDiff;

        // Tail Sweep Timer
        if (m_uiTailSweepTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_TAIL_SWEEP) == CAST_OK)
                m_uiTailSweepTimer = urand(4000, 6000);
        }
        else
            m_uiTailSweepTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

bool GossipHello_npc_vael(Player* pPlayer, Creature* pCreature)
{
    ScriptedInstance* m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();

    if (nullptr == m_pInstance)
        return false;

    if (m_pInstance->GetData(TYPE_NEFARIAN) != DONE)
    {
        if (!pCreature->HasAura(35297))
        {
            pCreature->AddAura(35297);  //CD
            DoScriptText(SAY_LINE_1, pCreature);
            pCreature->AddAura(35298);  //SAY_LINE_2 -1469027
        }
    }
    else
    {
        pCreature->CastSpell(pPlayer, 35295, true);
    }

    return true;
}

CreatureAI* GetAI_npc_vael(Creature* pCreature)
{
    return new npc_vaelAI(pCreature);
}

void AddSC_boss_nefarian()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "boss_nefarian";
    pNewScript->GetAI = &GetAI_boss_nefarian;
    pNewScript->RegisterSelf();

    /*pNewScript = new Script;
    pNewScript->Name = "npc_corrupted_totem";
    pNewScript->GetAI = &GetAI_npc_corrupted_totem;
    pNewScript->RegisterSelf();*/

    pNewScript = new Script;
    pNewScript->Name = "npc_vael";
    pNewScript->pGossipHello = &GossipHello_npc_vael;
    pNewScript->GetAI = &GetAI_npc_vael;
    pNewScript->RegisterSelf();
}
