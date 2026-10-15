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
SDName: Boss_Arcanist_Doan
SD%Complete: 100
SDComment:
SDCategory: Scarlet Monastery
EndScriptData */

#include "scriptPCH.h"
#include "scarlet_monastery.h"

enum
{
    SAY_AGGRO                   = -1189019,     // You will not defile these mysteries!
    SAY_SPECIALAE               = -1189020,     // Burn in righteous fire!

    ENRAGE_TIMER                = 10 * MINUTE * IN_MILLISECONDS,

    SPELL_ARCANE_PULSE_INT      = 36001,
    SPELL_ARCANE_PULSE          = 35954,
    SPELL_MANA_SHIELD           = 35955,
    SPELL_BLINK_VISUAL          = 35956,

    SPELL_FIRE_WARD             = 35957,
    SPELL_FROST_WARD            = 35958,
    SPELL_ARCANE_WARD           = 35959,
    SPELL_FIRE_VISUAL           = 35971,
    SPELL_FROST_VISUAL          = 35972,
    SPELL_ARCANE_VISUAL         = 35973,
    SPELL_FIREBOLT              = 35965,
    SPELL_FROSTBOLT             = 35966,
    SPELL_ARCANEBOLT            = 35967,
    
    SPELL_EVOCATION             = 35968,
    SPELL_POLYMORPH             = 35969,
    SPELL_SLOW                  = 35970,
    SPELL_SIPHON_MAGIC          = 35974,
    SPELL_DRAGON_BREATH         = 35963,
    SPELL_FLAMESTRIKE           = 35976,
    SPELL_ICE_BLAST             = 35977,

    SPELL_ARCANE_MEDITATION     = 35987,
    SPELL_SEARING_HEAT          = 35999,


    NPC_FROST_ZONE              = 25247,
    NPC_SNOW_VISUALS            = 25248,
    NPC_FIRES                   = 25250,

    SPELL_FIREAOE               = 9435,
    SPELL_ARCANEBUBBLE          = 9438,
};

float const FrostZonePos[2][3] =
{
    { 579.14f, 1454.2f, 50.1f},
    { 615.56f, 1453.32f, 50.1f}
};

struct boss_arcanist_doanAI : public ScriptedAI
{
    boss_arcanist_doanAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    float BlinkCenterX = 596.4f;
    float BlinkCenterY = 1453.3f;
    float BlinkCenterR = 20.0f;

    uint32 MovementCheck_Timer;
    uint32 PosCheckTimer;
    uint32 Enrage_Timer;
    uint32 Pulse_Timer;
    uint32 Pulse_Timer_OOC;
    uint32 Blink_Timer;
    uint32 Evocation_Timer;

    uint32 WardSpell;
    uint32 Firebolt_Timer;
    uint32 Frostbolt_Timer;
    uint32 Arcanebolt_Timer;

    uint32 Polymorph_Timer;
    bool Polymorph_is_Up;
    uint32 Slow_Timer;
    uint32 Slowed_Targets;
    uint32 Siphon_Timer;
    uint32 Flamestrike_Timer;
    uint32 Dragonbreath_Timer;
    uint32 Searingheat_Timer;
    uint32 Iceblast_Timer;

    uint32 Previous_Stance;


    bool CastingNotMoving;
    bool m_LastCastingState;
    bool m_bWaitingForMove;


    void Reset()
    {
        MovementCheck_Timer = 200;
        PosCheckTimer = 1000;
        Enrage_Timer = ENRAGE_TIMER;
        Pulse_Timer = 3000;
        Pulse_Timer_OOC = 11000;
        Blink_Timer = 20000;
        Evocation_Timer = urand(40000, 60000);

        WardSpell = SPELL_ARCANE_WARD;
        Firebolt_Timer = 2000;
        Frostbolt_Timer = 2000;
        Arcanebolt_Timer = 2000;

        Polymorph_Timer = urand(5000, 8000);
        Polymorph_is_Up = false;
        Slow_Timer = urand(5000, 8000);
        Slowed_Targets = 0;
        Siphon_Timer = urand(5000, 8000);
        Flamestrike_Timer = urand(8000, 12000);
        Dragonbreath_Timer = urand(5000, 8000);
        Searingheat_Timer = 1000;
        Iceblast_Timer = urand(8000, 12000);

        Previous_Stance = SPELL_FIRE_WARD;


        CastingNotMoving = false;
        m_bWaitingForMove = false;
        m_LastCastingState = false;
    }

    void Aggro(Unit* who)
    {
        m_creature->RemoveAurasDueToSpell(SPELL_ARCANE_MEDITATION);
        m_creature->SetInCombatWithZone();
        DoScriptText(SAY_AGGRO, m_creature);
        m_creature->CastSpell(m_creature, SPELL_MANA_SHIELD, true);

        WardSpell = urand(SPELL_FIRE_WARD, SPELL_ARCANE_WARD);
        m_creature->CastSpell(m_creature, WardSpell, false);

        if (m_pInstance)
            m_pInstance->SetData(TYPE_DOAN, IN_PROGRESS);
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        UnsummonFires();
        UnsummonFrost();

        if (m_pInstance)
            m_pInstance->SetData(TYPE_DOAN, DONE);
    }

    void JustReachedHome()
    {
        UnsummonFires();
        UnsummonFrost();

        if (m_pInstance)
            m_pInstance->SetData(TYPE_DOAN, FAIL);
    }

    void SpellHit(Unit* /*pCaster*/, const SpellEntry* pSpell)
    {
        if (pSpell)
        {
            if (pSpell->Id == SPELL_FIRE_WARD)
            {
                m_creature->getThreatManager().addThreat(m_creature->GetNearestVictimInRange(0.0f, 100.0f), 100000);
                m_creature->CastSpell(m_creature, SPELL_FIRE_VISUAL, true);
                UnsummonFrost();
                m_creature->RemoveAurasDueToSpell(SPELL_FROST_WARD);
                m_creature->RemoveAurasDueToSpell(SPELL_ARCANE_WARD);

                if (Previous_Stance != SPELL_FIRE_WARD)
                {
                    Flamestrike_Timer = urand(5000, 10000);
                    Dragonbreath_Timer = urand(2000, 5000);
                }
            }
            if (pSpell->Id == SPELL_FROST_WARD)
            {
                m_creature->getThreatManager().addThreat(m_creature->GetNearestVictimInRange(0.0f, 100.0f), 100000);
                m_creature->CastSpell(m_creature, SPELL_FROST_VISUAL, true);
                UnsummonFires();
                /*UnsummonFrost();*/
                m_creature->RemoveAurasDueToSpell(SPELL_FIRE_WARD);
                m_creature->RemoveAurasDueToSpell(SPELL_ARCANE_WARD);
                m_creature->SummonCreature(NPC_FROST_ZONE, 579.14f, 1454.2f, 50.1f, 3.0f, TEMPSUMMON_TIMED_DESPAWN, 30000);
                m_creature->SummonCreature(NPC_FROST_ZONE, 615.56f, 1453.32f, 50.1f, 3.0f, TEMPSUMMON_TIMED_DESPAWN, 30000);
                m_creature->SummonCreature(NPC_SNOW_VISUALS, 596.3f, 1453.1f, 50.13f, 3.0f, TEMPSUMMON_TIMED_DESPAWN, 28000);

                if (Previous_Stance != SPELL_FROST_WARD)
                    Iceblast_Timer = urand(5000, 10000);
            }
            if (pSpell->Id == SPELL_ARCANE_WARD)
            {
                m_creature->getThreatManager().addThreat(m_creature->GetNearestVictimInRange(0.0f, 100.0f), 100000);
                m_creature->CastSpell(m_creature, SPELL_ARCANE_VISUAL, true);
                UnsummonFrost();
                m_creature->RemoveAurasDueToSpell(SPELL_FIRE_WARD);
                m_creature->RemoveAurasDueToSpell(SPELL_FROST_WARD);

                if (Previous_Stance != SPELL_ARCANE_WARD)
                {
                    Polymorph_Timer = urand(5000, 8000);
                    Siphon_Timer = urand(5000, 8000);
                    Slow_Timer = urand(5000, 8000);
                }
            }
            Previous_Stance = pSpell->Id;
        }
    }

    void UnsummonFires()
    {
        std::list<Creature*> dummy;
        GetCreatureListWithEntryInGrid(dummy, m_creature, NPC_FIRES, 100.0f);
        for (auto it = dummy.cbegin(); it != dummy.cend(); ++it)
        {
            (*it)->SetDisplayId(11686);
            (*it)->RemoveAllDynObjects();
            (*it)->DespawnOrUnsummon();
        }
    }

    void UnsummonFrost()
    {
        std::list<Creature*> dummy;
        GetCreatureListWithEntryInGrid(dummy, m_creature, NPC_FROST_ZONE, 100.0f);
        GetCreatureListWithEntryInGrid(dummy, m_creature, NPC_SNOW_VISUALS, 100.0f);
        for (auto it = dummy.cbegin(); it != dummy.cend(); ++it)
        {
            (*it)->RemoveAllDynObjects();
            (*it)->DespawnOrUnsummon();
        }

        m_creature->RemoveAllDynObjects();
    }

    void DoBlink()
    {
        float angle = frand(0.0f, float(M_PI * 2.0f));
        float radius = sqrtf(frand(0.0f, 1.0f)) * BlinkCenterR;
        float TeleX = BlinkCenterX + cosf(angle) * radius;
        float TeleY = BlinkCenterY + sinf(angle) * radius;

        if (m_creature->GetDistance2d(TeleX, TeleY) > 15.0f && m_creature->GetMap()->isInLineOfSight(596.4f, 1453.3f, 72.3f, TeleX, TeleY, 49.3))
        {
            m_creature->CastSpell(m_creature, SPELL_BLINK_VISUAL, true);
            m_creature->NearTeleportTo(TeleX, TeleY, 49.3, m_creature->GetOrientation());
            WardSpell = urand(SPELL_FIRE_WARD, SPELL_ARCANE_WARD);

            Firebolt_Timer = 2000;
            Frostbolt_Timer = 2000;
            Arcanebolt_Timer = 2000;

            DoResetThreat();
            m_creature->CastSpell(m_creature, WardSpell, false);
            Blink_Timer = urand(25000, 28000);

            /*sLog.outError("Attempt %u | X: %.2f Y: %.2f",AttemptNumber,TeleX,TeleY);*/
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
        {
            if (Pulse_Timer_OOC < diff)
            {
                m_creature->CastSpell(m_creature, SPELL_ARCANE_PULSE, true);
                m_creature->CastSpell(m_creature, SPELL_ARCANE_PULSE_INT, true);
                Pulse_Timer_OOC = 10500;
                /*m_creature->SetTargetGuid(ObjectGuid());*/
            }
            else Pulse_Timer_OOC -= diff;
            return;
        }

        if (PosCheckTimer < diff)
        {
            if (m_creature->GetPositionY() < 1428.5f)
                EnterEvadeMode();

            PosCheckTimer = 1000;
        }
        else PosCheckTimer -= diff;

        if (Enrage_Timer < diff)
        {
            if (!m_creature->IsNonMeleeSpellCasted(false))
            {
                m_creature->CastSpell(m_creature, SPELL_ARCANEBUBBLE, true);
                m_creature->CastSpell(m_creature, SPELL_FIREAOE, false);
                DoScriptText(SAY_SPECIALAE, m_creature);

                Enrage_Timer = 10 * MINUTE * IN_MILLISECONDS;
            }
        }
        else Enrage_Timer -= diff;

        if (Evocation_Timer < diff)
        {
            if (m_creature->GetPowerPercent(POWER_MANA) < 75.0f)
            {
                if (!m_creature->IsNonMeleeSpellCasted(false))
                    if (DoCastSpellIfCan(m_creature, SPELL_EVOCATION) == CAST_OK)
                        Evocation_Timer = urand(30000, 45000);
            }
            else Evocation_Timer = urand(3000, 5000);
        }
        else Evocation_Timer -= diff;

        if (Pulse_Timer < diff)
        {
            m_creature->CastSpell(m_creature, SPELL_ARCANE_PULSE, true);
            m_creature->CastSpell(m_creature, SPELL_ARCANE_PULSE_INT, true);
            Pulse_Timer = 3000;
        }
        else Pulse_Timer -= diff;

        if (Blink_Timer < diff)
        {
            if (!m_creature->IsNonMeleeSpellCasted(false))
            {
                DoBlink();
                return;
            }
        }
        else Blink_Timer -= diff;

        switch (WardSpell)
        {
            case SPELL_FIRE_WARD:
            {
                if (Searingheat_Timer < diff)
                {
                    m_creature->CastSpell(m_creature, SPELL_SEARING_HEAT, true);
                    Searingheat_Timer = 1000;
                }
                else Searingheat_Timer -= diff;

                if (Dragonbreath_Timer < diff)
                {
                    if (!m_creature->IsNonMeleeSpellCasted(false))
                    {
                        if (DoCastSpellIfCan(m_creature, SPELL_DRAGON_BREATH) == CAST_OK)
                        {
                            Dragonbreath_Timer = urand(8000, 15000);
                            if (Flamestrike_Timer < 2000)
                                Flamestrike_Timer = 2000;
                        }
                    }
                }
                else Dragonbreath_Timer -= diff;

                if (Flamestrike_Timer < diff)
                {
                    if (!m_creature->IsNonMeleeSpellCasted(false))
                    {
                        if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_FLAMESTRIKE))
                        {
                            if (DoCastSpellIfCan(pTarget, SPELL_FLAMESTRIKE) == CAST_OK)
                            {
                                pTarget->CastSpell(pTarget, SPELL_FIRE_VISUAL, true);
                                Flamestrike_Timer = urand(8000, 12000);

                                if (Dragonbreath_Timer < 2000)
                                    Dragonbreath_Timer = 2000;
                            }
                        }
                    }
                }
                else Flamestrike_Timer -= diff;

                if (Firebolt_Timer < diff)
                {
                    if (!m_creature->IsNonMeleeSpellCasted(false))
                    {
                        if (m_creature->GetDistance(m_creature->getVictim()) < 10.0f)
                        {
                            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_FIREBOLT) == CAST_OK)
                                Firebolt_Timer = 3000;
                        }
                    }
                }
                else Firebolt_Timer -= diff;

                break;
            }
            case SPELL_FROST_WARD:
            {
                if (Iceblast_Timer < diff)
                {
                    if (!m_creature->IsNonMeleeSpellCasted(false))
                    {
                        if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_ICE_BLAST))
                            if (DoCastSpellIfCan(pTarget, SPELL_ICE_BLAST) == CAST_OK)
                            {
                                pTarget->CastSpell(pTarget, SPELL_FROST_VISUAL, true);
                                Iceblast_Timer = urand(8000, 12000);
                            }
                    }
                }
                else Iceblast_Timer -= diff;

                if (Frostbolt_Timer < diff)
                {
                    if (!m_creature->IsNonMeleeSpellCasted(false))
                    {
                        if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_FROSTBOLT) == CAST_OK)
                            Frostbolt_Timer = 2000;
                    }
                }
                else Frostbolt_Timer -= diff;

                break;
            }
            case SPELL_ARCANE_WARD:
            {
                if (Siphon_Timer < diff)
                {
                    if (!m_creature->IsNonMeleeSpellCasted(false))
                    {
                        Unit::SpellAuraHolderMap const& auras = m_creature->GetSpellAuraHolderMap();
                        for (Unit::SpellAuraHolderMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                        {
                            SpellAuraHolder* holder = itr->second;

                            if (!holder)
                                continue;

                            if (holder->GetSpellProto()->Dispel == DISPEL_MAGIC)
                            {
                                if (DoCastSpellIfCan(m_creature, SPELL_SIPHON_MAGIC) == CAST_OK)
                                {
                                    Siphon_Timer = 1000;
                                    Polymorph_Timer = 1000;
                                    Slow_Timer = 1000;
                                    return;
                                }
                            }
                        }
                        Siphon_Timer = 500;
                    }
                } 
                else Siphon_Timer -= diff;

                if (Polymorph_Timer < diff)
                {
                    if (!m_creature->IsNonMeleeSpellCasted(false))
                    {
                        std::list<Player*> players;
                        GetPlayersWithinRange(players, 45.0f);
                        if (players.size() > 1)
                        {
                            for (Player* pTarget : players)
                            {
                                if (pTarget && pTarget->HasAura(SPELL_POLYMORPH))
                                {
                                    Polymorph_is_Up = true;
                                    break;
                                }
                            }
                        }

                        if (!Polymorph_is_Up)
                        {
                            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_POLYMORPH, SELECT_FLAG_PLAYER | SELECT_FLAG_MISSING_AURA))
                            {
                                if (players.size() > 3)
                                {
                                    if (!pTarget->HasAura(SPELL_SLOW))
                                        if (DoCastSpellIfCan(pTarget, SPELL_POLYMORPH) == CAST_OK)
                                        {
                                            Polymorph_Timer = 1000;
                                            Siphon_Timer = 1000;
                                            Slow_Timer = 1000;
                                        }
                                }
                                else if (DoCastSpellIfCan(pTarget, SPELL_POLYMORPH) == CAST_OK)
                                {
                                    Polymorph_Timer = 1000;
                                    Siphon_Timer = 1000;
                                    Slow_Timer = 1000;
                                }
                            }
                        }
                        else Polymorph_Timer = 500;

                        Polymorph_is_Up = false;
                    }
                }
                else Polymorph_Timer -= diff;

                if (Slow_Timer < diff)
                {
                    if (!m_creature->IsNonMeleeSpellCasted(false))
                    {
                        std::list<Player*> players;
                        GetPlayersWithinRange(players, 45.0f);

                        for (Player* pTarget : players)
                        {
                            if (pTarget && !pTarget->HasAura(SPELL_POLYMORPH) && pTarget->HasAura(SPELL_SLOW))
                                ++Slowed_Targets;
                        }

                        if (Slowed_Targets < (players.size() > 3 ? 3 : 1))
                        {
                            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_SLOW, SELECT_FLAG_PLAYER | SELECT_FLAG_MISSING_AURA))
                                if (DoCastSpellIfCan(pTarget, SPELL_SLOW) == CAST_OK)
                                {
                                    Polymorph_Timer = 1000;
                                    Siphon_Timer = 1000;
                                    Slow_Timer = 1000;
                                }
                        }
                        Slowed_Targets = 0;
                    }
                }
                else Slow_Timer -= diff;

                if (Arcanebolt_Timer < diff)
                {
                    if (!m_creature->IsNonMeleeSpellCasted(false))
                    {
                        if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_ARCANEBOLT) == CAST_OK)
                            Arcanebolt_Timer = 500;
                    }
                }
                else Arcanebolt_Timer -= diff;


                break;
            }
            default:
                break;
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
                if (MovementCheck_Timer <= diff)
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
                    MovementCheck_Timer -= diff;
            }
        }
        m_LastCastingState = CastingNotMoving;

        DoMeleeAttackIfReady();
    }
};
CreatureAI* GetAI_boss_arcanist_doan(Creature* pCreature)
{
    return new boss_arcanist_doanAI(pCreature);
}

void AddSC_boss_arcanist_doan()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "boss_arcanist_doan";
    newscript->GetAI = &GetAI_boss_arcanist_doan;
    newscript->RegisterSelf();
}
