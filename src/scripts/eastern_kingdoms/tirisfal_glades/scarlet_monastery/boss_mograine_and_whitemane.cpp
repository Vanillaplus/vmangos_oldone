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
SDName: Boss_Mograine_And_Whitemane
SD%Complete: 90
SDComment:
SDCategory: Scarlet Monastery
EndScriptData */

#include "scriptPCH.h"
#include "scarlet_monastery.h"
#include "PathFinder.h"

enum
{
    //Mograine says
    SAY_MO_AGGRO                 = -1189005,    // Infidels! They must be purified!
    SAY_MO_KILL                  = -1189006,    // Unworthy!
    SAY_MO_RESSURECTED           = -1189007,    // At your side, milady!

    //Whitemane says
    SAY_WH_INTRO                 = -1189008,    // What, Mograine has fallen? You shall pay for this treachery!
    SAY_WH_KILL                  = -1189009,    // The Light has spoken!
    SAY_WH_RESSURECT             = -1189010,    // Arise, my champion!


    //Mograine Spells
    SPELL_PILAR_OF_LIGHT        = 36002,
    /*SPELL_BURNING_LIGHT         = 36022,*/
    SPELL_BLINDING_LIGHT        = 36003,
    SPELL_EYE_FOR_AN_EYE        = 36004,

    SPELL_RETRIBUTION_AURA      = 36005,
    SPELL_DEVOTION_AURA         = 36006,
    SPELL_FIRE_RES_AURA         = 36007,
    SPELL_FROST_RES_AURA        = 36008,
    SPELL_SHADOW_RES_AURA       = 36009,

    SPELL_PURIFY                = 36010,
    SPELL_CRUSADERSTRIKE        = 36011,
    SPELL_LAYONHANDS            = 9257,
    SPELL_DIVINESHIELD          = 1020,

    //Whitemanes Spells
    SPELL_HEAL                   = 36012,
    SPELL_DOMINATEMIND           = 36013,
    SPELL_HOLYSMITE              = 36017,
    SPELL_POWERWORDSHIELD        = 36018,
    SPELL_HOLYFIRE               = 36019,
    SPELL_ABSOLUTION             = 36021,
    SPELL_DISPEL_MAGIC           = 36023,
    SPELL_DEEPSLEEP              = 9256,
    SPELL_SCARLETRESURRECTION    = 9232,
    SPELL_SCARLETRESURRECTION_2  = 36160,
    SPELL_ERADICATION            = 36215,


    NPC_MOGRAINE                 = 25227,
    NPC_WHITEMANE                = 25228,
    NPC_PILAR_OF_LIGHT           = 25253,

    GO_HOLY_DOOR                 = 400021,
    GO_PILAR_OF_LIGHT            = 302200,
};

struct boss_scarlet_commander_mograineAI : public ScriptedAI
{
    boss_scarlet_commander_mograineAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    int32 Eye_for_an_Eye_Damage;
    uint32 LastAura;
    uint32 AuraSpell;

    uint32 PosCheckTimer;
    uint32 Eye_for_an_Eye_Timer;
    uint32 Pilar_Timer;
    uint32 Aura_Timer;
    uint32 DivineShield_Timer;
    uint32 Cleanse_Timer;
    uint32 Crusader_Timer;





    bool m_bDivineShield;
    bool m_bHasDied;
    bool m_bHeal;
    bool m_bFakeDeath;

    void Reset()
    {
        Eye_for_an_Eye_Damage = 0;
        LastAura = 0;
        AuraSpell = 0;

        PosCheckTimer = 1000;
        Eye_for_an_Eye_Timer = 2000;
        Pilar_Timer = urand(25000, 35000);
        Aura_Timer = 1000;
        Cleanse_Timer = 3000;
        Crusader_Timer = urand(8000, 12000);
        DivineShield_Timer = 1000;

        //Incase wipe during phase that mograine fake death
        m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
        m_creature->SetStandState(UNIT_STAND_STATE_STAND);

        if (m_creature->GetHealth() == 0)
            m_creature->SetHealth(m_creature->GetMaxHealth());

        m_bDivineShield = false;
        m_bHasDied = false;
        m_bHeal = false;
        m_bFakeDeath = false;


        // Commented to prevent an abuse of infinite respawn of Whitemane for hunter
        if (m_pInstance)
        {
            if (!(m_pInstance->GetData(TYPE_MOGRAINE_AND_WHITE_EVENT) == SPECIAL))
            {
                if (Creature* pWhitemane = m_creature->FindNearestCreature(NPC_WHITEMANE, 150.0f))
                {
                    if (m_creature->isAlive() && !pWhitemane->isAlive())
                        pWhitemane->Respawn();
                }
            }
        }
    }

    void Aggro(Unit* pWho)
    {
        m_creature->SetInCombatWithZone();
        DoScriptText(SAY_MO_AGGRO, m_creature);

        std::list<Creature*> mograinesAssist;
        GetCreatureListExceptInGrid(mograinesAssist, m_creature, true, 62, 130.0f);

        if (!mograinesAssist.empty())
        {
            for (std::list<Creature*>::iterator itr = mograinesAssist.begin(); itr != mograinesAssist.end(); ++itr)
            {
                if ((*itr)->isAlive())
                {
                    (*itr)->SetInCombatWithZone();
                    (*itr)->getThreatManager().addThreat(pWho, 1000);
                    (*itr)->AI()->AttackStart(pWho);
                }
            }
        }

        if (GameObject* door = m_creature->FindNearestGameObject(GO_HOLY_DOOR, 200.0f))
            if (door->GetGoState() == GO_STATE_ACTIVE) // Open
                door->UseDoorOrButton();

        /*if (m_pInstance)
            m_pInstance->SetData(TYPE_MOGRAINE_AND_WHITE_EVENT, IN_PROGRESS);*/
    }

    void JustReachedHome()
    {
        if (Creature* pWhitemane = m_creature->FindNearestCreature(NPC_WHITEMANE, 150.0f))
            if (pWhitemane->isInCombat())
                pWhitemane->AI()->EnterEvadeMode();

        DespawnPilars();
        if (m_pInstance)
            m_pInstance->SetData(TYPE_MOGRAINE_AND_WHITE_EVENT, FAIL);

    }

    void JustDied(Unit* pKiller)
    {
        DespawnPilars();

        if (Creature* pWhitemane = m_creature->FindNearestCreature(NPC_WHITEMANE, 150.0f))
        {
            if (pWhitemane->isAlive())
            {
                pWhitemane->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                pWhitemane->SetStandState(UNIT_STAND_STATE_STAND);
                pWhitemane->SetHealth(1);

                if (pKiller)
                    pKiller->DealDamage(pWhitemane, 1, NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
            }
        }

        if (m_pInstance)
            m_pInstance->SetData(TYPE_MOGRAINE_AND_WHITE_EVENT, DONE);
    }

    void KilledUnit(Unit* pVictim)
    {
        DoScriptText(SAY_MO_KILL, m_creature);
    }

    void DespawnPilars()
    {
        std::list<Creature*> NPCList;
        GetCreatureListWithEntryInGrid(NPCList, m_creature, NPC_PILAR_OF_LIGHT, 150.0f);
        for (auto it = NPCList.cbegin(); it != NPCList.cend(); ++it)
        {
            (*it)->RemoveAllDynObjects();
            (*it)->DespawnOrUnsummon();
        }

        std::list<GameObject*> GOList;
        GetGameObjectListWithEntryInGrid(GOList, m_creature, GO_PILAR_OF_LIGHT, 150.0f);
        for (std::list<GameObject*>::const_iterator itr = GOList.begin(); itr != GOList.end(); ++itr)
        {
            if (GOList.empty())
                break;

            (*itr)->SetLootState(GO_JUST_DEACTIVATED);
        }
    }

    void FakeDeath()
    {
        DespawnPilars();

        if (m_creature->IsNonMeleeSpellCasted(false))
            m_creature->InterruptNonMeleeSpells(false);

        m_creature->SetHealth(0);

        m_creature->GetMotionMaster()->MovementExpired();
        m_creature->GetMotionMaster()->MoveIdle();

        m_creature->ClearComboPointHolders();
        m_creature->RemoveAllAuras();
        m_creature->ClearAllReactives();

        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

        m_creature->SetStandState(UNIT_STAND_STATE_DEAD);

        m_bDivineShield = false;
        m_bHasDied = true;
        m_bFakeDeath = true;
    }

    void DamageTaken(Unit* pDoneBy, uint32 &uiDamage)
    {
        Eye_for_an_Eye_Damage += uiDamage;

        if (uiDamage < m_creature->GetHealth())
            return;

        //On second death, fake death if whitemane is alive for prevent loot abuse.
        if (m_bHasDied)
        {
            if (Creature* pWhitemane = m_creature->FindNearestCreature(NPC_WHITEMANE, 150.0f))
            {
                if (pWhitemane->isAlive() && pWhitemane->GetHealth() > 0 && !pWhitemane->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
                {
                    if (!m_bFakeDeath)
                        FakeDeath();

                    uiDamage = 0;
                }
            }

            return;
        }

        //On first death, fake death and open door, as well as initiate whitemane if exist
        if (Creature* pWhitemane = m_creature->FindNearestCreature(NPC_WHITEMANE, 150.0f))
        {
            if (pWhitemane->isAlive())
            {
                m_pInstance->SetData(TYPE_MOGRAINE_AND_WHITE_EVENT, IN_PROGRESS);

                pWhitemane->GetMotionMaster()->MovePoint(1, 832.9f, 1336.0f, 49.0f);

                FakeDeath();

                uiDamage = 0;
            }
        }
    }

    void SpellHit(Unit* pWho, const SpellEntry* pSpell)
    {
        //When hit with ressurection stop fake death and say text
        if (pSpell->Id == SPELL_SCARLETRESURRECTION_2)
        {
            m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            m_creature->SetStandState(UNIT_STAND_STATE_STAND);
            m_creature->SetHealth(m_creature->GetMaxHealth());
            m_creature->SetPower(POWER_MANA, m_creature->GetMaxPower(POWER_MANA));

            DoScriptText(SAY_MO_RESSURECTED, m_creature);
            m_bFakeDeath = false;


            Map::PlayerList const& players = m_creature->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
            {
                if (Player* pPlayer = itr->getSource())
                    pPlayer->RemoveAurasDueToSpell(SPELL_DEEPSLEEP);
            }

            if (m_pInstance)
                m_pInstance->SetData(TYPE_MOGRAINE_AND_WHITE_EVENT, SPECIAL);
        }
    }

    void DoCleanse()
    {
        Unit* CleanseTarget = m_creature;

        if (Creature* Crea = m_creature->FindNearestCreature(NPC_WHITEMANE, 30.0f))
        {
            if (Crea->isInCombat() && m_creature->IsWithinLOSInMap(Crea) && urand(0, 1) == 0)
                CleanseTarget = Crea;
        }

        Unit::SpellAuraHolderMap const& auras = CleanseTarget->GetSpellAuraHolderMap();
        for (Unit::SpellAuraHolderMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
        {
            SpellAuraHolder* holder = itr->second;

            if (!holder)
                continue;

            if (holder->GetSpellProto()->Dispel == DISPEL_DISEASE || holder->GetSpellProto()->Dispel == DISPEL_POISON)
            {
                if (DoCastSpellIfCan(CleanseTarget, SPELL_PURIFY) == CAST_OK)
                {
                    Cleanse_Timer = 3000;
                    return;
                }
                else
                {
                    Cleanse_Timer = 200;
                    return;
                }
            }
        }
        Cleanse_Timer = 200;
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        //This if-check to make sure mograine does not attack while fake death
        if (m_bFakeDeath)
            return;

        if (m_bHasDied && !m_bHeal)
        {
            //On ressurection, heal whitemane and resume fight
            if (Creature* pWhitemane = m_creature->FindNearestCreature(NPC_WHITEMANE, 150.0f))
            {
                if (DoCastSpellIfCan(pWhitemane, SPELL_LAYONHANDS) == CAST_OK)
                    m_bHeal = true;

                if (m_creature->getVictim())
                    m_creature->GetMotionMaster()->MoveChase(m_creature->getVictim());

            }
        }

        if (!m_bDivineShield)
        {
            if (DivineShield_Timer < uiDiff)
            {
                if (m_creature->GetHealthPercent() < 25.0f)
                {
                    m_bDivineShield = true;
                    m_creature->CastSpell(m_creature, SPELL_DIVINESHIELD, true);
                }
                DivineShield_Timer = 200;
            }
            else DivineShield_Timer -= uiDiff;
        }

        if (PosCheckTimer < uiDiff)
        {
            if (m_creature->GetPositionX() < 709.5f)
            {
                if (Creature* pWhitemane = m_creature->FindNearestCreature(NPC_WHITEMANE, 150.0f))
                    if (pWhitemane->isInCombat())
                        pWhitemane->AI()->EnterEvadeMode();

                DespawnPilars();
                EnterEvadeMode();
            }

            PosCheckTimer = 1000;
        }
        else PosCheckTimer -= uiDiff;

        if (Eye_for_an_Eye_Timer < uiDiff)
        {
            if (Eye_for_an_Eye_Damage > 1000)
            {
                Eye_for_an_Eye_Damage = Eye_for_an_Eye_Damage / (m_creature->HasAura(SPELL_RETRIBUTION_AURA) ? 5 : 10);
                m_creature->CastCustomSpell(m_creature, SPELL_EYE_FOR_AN_EYE, &Eye_for_an_Eye_Damage, nullptr, nullptr, true);
                Eye_for_an_Eye_Damage = 0;
                Eye_for_an_Eye_Timer = 4000;
            }
        }
        else Eye_for_an_Eye_Timer -= uiDiff;

        if (Aura_Timer < uiDiff)
        {
            AuraSpell = urand(SPELL_RETRIBUTION_AURA, SPELL_SHADOW_RES_AURA);

            if (AuraSpell != LastAura)
            {
                for (int i = SPELL_RETRIBUTION_AURA; i <= SPELL_SHADOW_RES_AURA; ++i)
                {
                    m_creature->RemoveAurasDueToSpell(i);
                }
                m_creature->CastSpell(m_creature, AuraSpell, true);
                LastAura = AuraSpell;

                Aura_Timer = 20000;
            }

        }
        else Aura_Timer -= uiDiff;

        if (Cleanse_Timer < uiDiff)
        {
            DoCleanse();
        }
        else Cleanse_Timer -= uiDiff;

        if (Crusader_Timer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_CRUSADERSTRIKE) == CAST_OK)
                Crusader_Timer = urand(6000, 8000);
        }
        else Crusader_Timer -= uiDiff;

        if (Pilar_Timer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_PILAR_OF_LIGHT) == CAST_OK)
                Pilar_Timer = urand(25000, 35000);
        }
        else Pilar_Timer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

struct boss_high_inquisitor_whitemaneAI : public ScriptedAI
{
    boss_high_inquisitor_whitemaneAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 PosCheckTimer;
    uint32 MovementCheck_Timer;
    uint32 m_uiHeal_Timer;
    uint32 m_uiPowerWordShield_Timer;
    uint32 m_uiHolySmite_Timer;
    uint32 m_uiStopAttack_Timer;
    uint32 DominateMind_Timer;
    uint32 HolyFire_Timer;
    uint32 Absolution_Timer;
    uint32 LoSCheck_Timer;
    uint32 DispelMagic_Timer;
    uint32 Ress_Bug_Check;

    uint32 LoSAbuseThreshold;

    bool m_bCanResurrectCheck;
    bool m_bCanResurrect;
    bool m_bRessurecting;
    bool m_bStopAttack;
    bool CastingNotMoving;
    bool m_LastCastingState;
    bool m_bWaitingForMove;
    bool m_bFakeDeath;

    void Reset()
    {
        PosCheckTimer = 1000;
        MovementCheck_Timer = 0;
        m_uiHeal_Timer = 10000;
        m_uiPowerWordShield_Timer = 15000;
        m_uiHolySmite_Timer = 6000;
        m_uiStopAttack_Timer = 2000;
        DominateMind_Timer = urand(20000, 30000);
        HolyFire_Timer = urand(10000, 20000);
        Absolution_Timer = 1000;
        LoSCheck_Timer = 30000;
        DispelMagic_Timer = 3000;

        Ress_Bug_Check = 3600000;
        LoSAbuseThreshold = 0;

        m_bCanResurrectCheck = false;
        m_bCanResurrect = false;
        m_bRessurecting = false;
        m_bStopAttack = false;

        CastingNotMoving = false;
        m_bWaitingForMove = false;
        m_LastCastingState = false;


        m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
        m_creature->SetStandState(UNIT_STAND_STATE_STAND);

        if (m_creature->GetHealth() == 0)
            m_creature->SetHealth(m_creature->GetMaxHealth());

        m_bFakeDeath = false;

        if (!m_pInstance)
            return;

        if (Creature* pMograine = m_creature->FindNearestCreature(NPC_MOGRAINE, 150.0f))
        {
            if (m_creature->isAlive() && !pMograine->isAlive())
                pMograine->Respawn();
        }
    }


    void JustReachedHome()
    {
        if (Creature* pMograine = m_creature->FindNearestCreature(NPC_MOGRAINE, 150.0f))
            if (pMograine->isInCombat())
                pMograine->AI()->EnterEvadeMode();

        if (m_pInstance)
        {
            if (!(m_pInstance->GetData(TYPE_MOGRAINE_AND_WHITE_EVENT) == NOT_STARTED) && !(m_pInstance->GetData(TYPE_MOGRAINE_AND_WHITE_EVENT) == FAIL))
                m_pInstance->SetData(TYPE_MOGRAINE_AND_WHITE_EVENT, FAIL);
        }
        ScriptedAI::JustReachedHome();
    }

    void MoveInLineOfSight(Unit*) override
    {
        //This needs to be empty because Whitemane should NOT aggro while fighting Mograine. Mograine will give us a target.
    }

    void DamageTaken(Unit* pDoneBy, uint32 &uiDamage)
    {
        if (uiDamage < m_creature->GetHealth())
            return;

        if (!m_bCanResurrectCheck || m_bCanResurrect)
        {
            // prevent killing blow before rezzing commander
            m_creature->SetHealth(1);
            uiDamage = 0;
            return;
        }

        if (Creature* pMograine = m_creature->FindNearestCreature(NPC_MOGRAINE, 150.0f))
        {
            if (pMograine->isAlive() && !pMograine->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE) && m_pInstance && m_pInstance->GetData(TYPE_MOGRAINE_AND_WHITE_EVENT) == SPECIAL)
            {
                FakeDeath();
                uiDamage = 0;
                return;
            }
        }
    }

    void AttackStart(Unit* pWho)
    {
        if (m_pInstance && (m_pInstance->GetData(TYPE_MOGRAINE_AND_WHITE_EVENT) == NOT_STARTED || m_pInstance->GetData(TYPE_MOGRAINE_AND_WHITE_EVENT) == FAIL))
            return;

        if (m_bStopAttack)
            return;

        ScriptedAI::AttackStart(pWho);
    }

    void Aggro(Unit* pWho)
    {
        if (Creature* pMograine = m_creature->FindNearestCreature(NPC_MOGRAINE, 150.0f))
        {
            if (!pMograine->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE) || pMograine->isDead())
            {
                EnterEvadeMode();
                return;
            }
        }
        else
        {
            EnterEvadeMode();
            return;
        }

        DoScriptText(SAY_WH_INTRO, m_creature);

        if (!IsCombatMovementEnabled())
            SetCombatMovement(true);
    }

    void SpellHitTarget(Unit* pCaster, const SpellEntry* pSpell)
    {
        if (pSpell->Id == SPELL_HOLYSMITE)
            LoSAbuseThreshold = 0;
    }

    void KilledUnit(Unit* pVictim)
    {
        if (!m_creature->HasAura(34821))    //5sec CD
        {
            DoScriptText(SAY_WH_KILL, m_creature);
            m_creature->CastSpell(m_creature, 34821, true);
        }
    }

    void JustDied(Unit* pKiller)
    {
        if (Creature* pMograine = m_creature->FindNearestCreature(NPC_MOGRAINE, 150.0f))
            if (pMograine->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
            {
                pKiller->DealDamage(pMograine, 1, NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);

                if (m_pInstance)
                    m_pInstance->SetData(TYPE_MOGRAINE_AND_WHITE_EVENT, DONE);
            }
    }

    void FakeDeath()
    {
        if (m_creature->IsNonMeleeSpellCasted(false))
            m_creature->InterruptNonMeleeSpells(false);

        m_creature->SetHealth(0);

        m_creature->GetMotionMaster()->MovementExpired();
        m_creature->GetMotionMaster()->MoveIdle();

        m_creature->ClearComboPointHolders();
        m_creature->RemoveAllAuras();
        m_creature->ClearAllReactives();

        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

        m_creature->SetStandState(UNIT_STAND_STATE_DEAD);

        m_bFakeDeath = true;
    }

    void DoShield()
    {
        Unit* ShieldTarget = m_creature;

        if (Creature* Crea = m_creature->FindNearestCreature(NPC_MOGRAINE, 150.0f))
        {
            if (Crea->isAlive() && !Crea->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE) && urand(0, 1) == 0)
                ShieldTarget = Crea;
        }

        if (DoCastSpellIfCan(ShieldTarget, SPELL_POWERWORDSHIELD) == CAST_OK)
        {
            m_uiPowerWordShield_Timer = urand(15000, 20000);
        }
        else
            m_uiPowerWordShield_Timer = 500;

    }

    void DoCleanse()
    {
        Unit* CleanseTarget = m_creature;

        if (Creature* Crea = m_creature->FindNearestCreature(NPC_MOGRAINE, 30.0f))
        {
            if (Crea->isAlive() && !Crea->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE) && m_creature->IsWithinLOSInMap(Crea) && urand(0, 1) == 0)
                CleanseTarget = Crea;
        }


        Unit::SpellAuraHolderMap const& auras = CleanseTarget->GetSpellAuraHolderMap();
        for (Unit::SpellAuraHolderMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
        {
            SpellAuraHolder* holder = itr->second;

            if (!holder)
                continue;

            if (holder->GetSpellProto()->Dispel == DISPEL_MAGIC)
            {
                if (holder->GetCaster() && !holder->GetCaster()->IsFriendlyTo(m_creature))
                {
                    if (DoCastSpellIfCan(CleanseTarget, SPELL_DISPEL_MAGIC) == CAST_OK)
                    {
                        DispelMagic_Timer = 3000;
                        return;
                    }
                    else
                    {
                        DispelMagic_Timer = 200;
                        return;
                    }
                }
            }
        }
        DispelMagic_Timer = 500;
    }

    void RushToMograine(bool Forced)
    {
        if (Creature* pMograine = m_creature->FindNearestCreature(NPC_MOGRAINE, 150.0f))
        {
            m_creature->AttackStop(true);
            m_creature->GetMotionMaster()->Clear();

            float x = pMograine->GetPositionX();
            float y = pMograine->GetPositionY();
            float z = pMograine->GetPositionZ();

            if (Forced)
            {
                m_creature->GetMotionMaster()->MovePoint(2, x, y, z);
                return;
            }

            if (pMograine->IsWithinLOS(x - 2.0f, y, z))
                x -= 2.0f;
            else if (pMograine->IsWithinLOS(x + 2.0f, y, z))
                x += 2.0f;
            else if (pMograine->IsWithinLOS(x, y - 2.0f, z))
                y -= 2.0f;
            else if (pMograine->IsWithinLOS(x, y + 2.0f, z))
                y += 2.0f;

            PathInfo path(m_creature);
            path.calculate(x, y, z, false);

            PathType pathType = path.getPathType();

            if (pathType & PATHFIND_NORMAL)
                m_creature->GetMotionMaster()->MovePoint(2, x, y, z, MOVE_PATHFINDING);
            else
                m_creature->GetMotionMaster()->MovePoint(2, x, y, z);
        }
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
        if ((!m_creature->SelectHostileTarget() || !m_creature->getVictim()) && !m_bStopAttack)
            return;

        if (m_bFakeDeath)
            return;

        //while in "resurrect-mode", don't do anything
        if (m_bRessurecting)
        {
            if (Creature* pMograine = m_creature->FindNearestCreature(NPC_MOGRAINE, 150.0f))
            {
                if (!pMograine->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
                {
                    m_bRessurecting = false;
                }
            }

            return;
        }

        if (m_bCanResurrect)
        {
            //When casting resuruction make sure to delay so on rez when reinstate battle deepsleep runs out
            if (Creature* pMograine = m_creature->FindNearestCreature(NPC_MOGRAINE, 150.0f))
            {
                if (!m_creature->IsMoving() && DoCastSpellIfCan(pMograine, SPELL_SCARLETRESURRECTION) == CAST_OK)
                {
                    pMograine->CastSpell(pMograine, SPELL_SCARLETRESURRECTION_2, false);
                    DoScriptText(SAY_WH_RESSURECT, m_creature);
                    m_bCanResurrect = false;
                    m_bRessurecting = true;
                }
            }

            if (Ress_Bug_Check < uiDiff)
            {
                if (!m_bRessurecting && !m_creature->IsMoving())
                {
                    RushToMograine(true);
                    Ress_Bug_Check = 3600000;
                }
            }
            else Ress_Bug_Check -= uiDiff;

            return;
        }

        //Cast Deep sleep when health is less than 50%
        if (!m_bCanResurrectCheck && m_creature->GetHealthPercent() <= 50.0f)
        {
            if (m_creature->IsNonMeleeSpellCasted(false))
                m_creature->InterruptNonMeleeSpells(false);


            if (m_pInstance)
            {
                Map::PlayerList const& lPlayers = m_pInstance->GetMap()->GetPlayers();
                for (auto& p : lPlayers)
                {
                    if (Player* pPlayer = p.getSource())
                        if (pPlayer->HasAura(SPELL_DOMINATEMIND))
                            pPlayer->RemoveAurasDueToSpell(SPELL_DOMINATEMIND);

                }
            }

            m_creature->CastSpell(m_creature, SPELL_DEEPSLEEP, true);
            m_bCanResurrectCheck = true;
            m_bCanResurrect = true;
            m_bStopAttack = true;

            Ress_Bug_Check = 5000;
            RushToMograine(false);

            return;
        }

        //m_uiStopAttack_Timer
        if (m_bStopAttack)
        {
            if (m_uiStopAttack_Timer < uiDiff)
                m_bStopAttack = false;
            else
                m_uiStopAttack_Timer -= uiDiff;
        }

        if (PosCheckTimer < uiDiff)
        {
            if (m_creature->GetPositionX() < 709.5f)
            {
                if (Creature* pMograine = m_creature->FindNearestCreature(NPC_MOGRAINE, 150.0f))
                    if (pMograine->isInCombat())
                        pMograine->AI()->EnterEvadeMode();

                EnterEvadeMode();
            }

            PosCheckTimer = 1000;
        }
        else PosCheckTimer -= uiDiff;

        if (m_uiPowerWordShield_Timer < uiDiff)
        {
            if (!m_creature->IsNonMeleeSpellCasted(false))
                DoShield();
        }
        else
            m_uiPowerWordShield_Timer -= uiDiff;

        if (DispelMagic_Timer < uiDiff)
        {
            if (!m_creature->IsNonMeleeSpellCasted(false))
                DoCleanse();
        }
        else
            DispelMagic_Timer -= uiDiff;

        if (DominateMind_Timer < uiDiff)
        {
            if (!m_creature->IsNonMeleeSpellCasted(false))
                if (Unit* randomTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_DOMINATEMIND, SELECT_FLAG_PLAYER | SELECT_FLAG_IN_LOS | SELECT_FLAG_NOT_TARGET))
                {
                    if (DoCastSpellIfCan(randomTarget, SPELL_DOMINATEMIND) == CAST_OK)
                        DominateMind_Timer = urand(15000, 20000);
                }
        }
        else
            DominateMind_Timer -= uiDiff;

        if (HolyFire_Timer < uiDiff)
        {
            if (!m_creature->IsNonMeleeSpellCasted(false))
            {
                if (Unit* randomTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_HOLYFIRE, SELECT_FLAG_PLAYER | SELECT_FLAG_IN_LOS | SELECT_FLAG_NOT_TARGET))
                {
                    if (DoCastSpellIfCan(randomTarget, SPELL_HOLYFIRE) == CAST_OK)
                        HolyFire_Timer = 10000;
                }
                else if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_HOLYFIRE) == CAST_OK)
                    HolyFire_Timer = 10000;
            }
        }
        else
            HolyFire_Timer -= uiDiff;

        if (Absolution_Timer < uiDiff)
        {
            if (!m_creature->IsNonMeleeSpellCasted(false))
            {
                ThreatList const& tList = m_creature->getThreatManager().getThreatList();
                for (ThreatList::const_iterator itr = tList.begin(); itr != tList.end(); ++itr)
                {
                    if (Unit* AbsolutionTargets = m_creature->GetMap()->GetUnit((*itr)->getUnitGuid()))
                    {
                        if (AbsolutionTargets->GetHealthPercent() < 20.0f && m_creature->IsWithinLOSInMap(AbsolutionTargets))
                        {
                            if (DoCastSpellIfCan(AbsolutionTargets, SPELL_ABSOLUTION) == CAST_OK)
                            {
                                Absolution_Timer = 5000;
                                return;
                            }
                        }
                    }
                }
            }

            Absolution_Timer = 200;
        }
        else
            Absolution_Timer -= uiDiff;

        if (m_uiHeal_Timer < uiDiff)    //If we are <75% hp cast healing spells at self or Mograine
        {
            if (!m_creature->IsNonMeleeSpellCasted(false))
            {
                Creature* pTarget = NULL;

                if (Creature* pMograine = m_creature->FindNearestCreature(NPC_MOGRAINE, 150.0f))
                {
                    if (pMograine->isAlive() && !pMograine->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE) && pMograine->GetHealthPercent() <= 90.0f)
                        pTarget = pMograine;
                }

                if (m_creature->GetHealthPercent() <= 75.0f)
                    pTarget = m_creature;

                if (pTarget)
                {
                    if (DoCastSpellIfCan(pTarget, SPELL_HEAL) == CAST_OK)
                        m_uiHeal_Timer = urand(10000, 15000);
                }
            }

        }
        else
            m_uiHeal_Timer -= uiDiff;

        if (m_uiHolySmite_Timer <= uiDiff)
        {
            if (!m_creature->IsNonMeleeSpellCasted(false))
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_HOLYSMITE) == CAST_OK)
                {
                    m_uiHolySmite_Timer = 2500;
                }
            }
        }
        else
            m_uiHolySmite_Timer -= uiDiff;

        if (Unit* Victim = m_creature->getVictim())
        {
            if (!m_creature->IsWithinLOSInMap(Victim) && m_creature->GetDistance(Victim) < 10.0f)
            {
                ++LoSAbuseThreshold;
                if (LoSAbuseThreshold >= 50)
                {
                    if (DoCastSpellIfCan(Victim, SPELL_ERADICATION) == CAST_OK)
                        LoSAbuseThreshold = 0;
                }
            }
        }

        CastingNotMoving = m_creature->IsNonMeleeSpellCasted(false);
        if (CastingNotMoving)
        {
            if (m_creature->AI()->IsCombatMovementEnabled())
            {
                m_creature->AI()->SetCombatMovement(false);
                m_creature->StopMoving();
            }

            m_bWaitingForMove = false;
            MovementCheck_Timer = 500;
        }
        else
        {
            if (m_LastCastingState)
            {
                m_bWaitingForMove = true;
                MovementCheck_Timer = 500;
            }
            else if (m_bWaitingForMove)
            {
                if (MovementCheck_Timer <= uiDiff)
                {
                    MovementCheck_Timer = 0;

                    if (!m_creature->IsNonMeleeSpellCasted(false))
                    {
                        m_bWaitingForMove = false;

                        if (!m_creature->AI()->IsCombatMovementEnabled())
                            m_creature->AI()->SetCombatMovement(true);
                    }
                    else
                        m_bWaitingForMove = false;
                }
                else
                    MovementCheck_Timer -= uiDiff;
            }
        }
        m_LastCastingState = CastingNotMoving;


        if (LoSCheck_Timer < uiDiff)
        {
            if (!m_creature->IsNonMeleeSpellCasted(false))
            {
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

                if (Creature* pMograine = m_creature->FindNearestCreature(NPC_MOGRAINE, 150.0f))
                {
                    if (pMograine->isAlive() && !pMograine->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
                    {
                        if (m_creature->IsWithinLOSInMap(pMograine))
                        {
                            LoSCheck_Timer = 1000;
                            return;
                        }
                        else if (m_creature->GetPower(POWER_MANA) > 1000)
                        {
                            DoResetThreat();
                            if (Unit* pMograineTarget = pMograine->getVictim())
                                m_creature->getThreatManager().addThreat(pMograineTarget, 20000);

                            m_creature->GetMotionMaster()->MoveFollow(pMograine, 0.0f, 0.0f);

                            float pathLen = GetPathLen(m_creature, pMograine);
                            uint32 travelTime = uint32((pathLen / 5.0f) * 1000.0f);
                            if (travelTime > 8000)
                                travelTime = 8000;

                            LoSCheck_Timer = travelTime;
                            m_uiHeal_Timer = travelTime;
                            m_uiHolySmite_Timer = travelTime;
                            return;
                        }
                    }
                }

                LoSCheck_Timer = 1000;
            }
        }
        else
            LoSCheck_Timer -= uiDiff;

        DoMeleeAttackIfReady();
    }

    void MovementInform(uint32 MovementType, uint32 id)
    {
        if (MovementType == POINT_MOTION_TYPE && id == 1)
            m_creature->SetInCombatWithZone();
        else
        {
            if (Creature* pMograine = m_creature->FindNearestCreature(NPC_MOGRAINE, 150.0f))
                m_creature->SetFacingToObject(pMograine);
        }
    }
};

CreatureAI* GetAI_boss_scarlet_commander_mograine(Creature* pCreature)
{
    return new boss_scarlet_commander_mograineAI(pCreature);
}

CreatureAI* GetAI_boss_high_inquisitor_whitemane(Creature* pCreature)
{
    return new boss_high_inquisitor_whitemaneAI(pCreature);
}

void AddSC_boss_mograine_and_whitemane()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "boss_scarlet_commander_mograine";
    newscript->GetAI = &GetAI_boss_scarlet_commander_mograine;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "boss_high_inquisitor_whitemane";
    newscript->GetAI = &GetAI_boss_high_inquisitor_whitemane;
    newscript->RegisterSelf();
}
