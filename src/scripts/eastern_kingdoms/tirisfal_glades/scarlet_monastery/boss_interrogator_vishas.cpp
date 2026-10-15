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
SDName: Boss_Interrogator_Vishas
SD%Complete: 100
SDComment:
SDCategory: Scarlet Monastery
EndScriptData */

#include "scriptPCH.h"
#include "scarlet_monastery.h"

enum
{
    SAY_AGGRO                   = -1189011, // Tell me, tell me everything!
    SAY_NAUGHTY_SECRETS         = -1189012, // Naughty secrets!
    SAY_ATONEMENT               = -1189013, // I'll rip the secrets from your flesh!
    SAY_KILL                    = -1189014, // Purged by pain!
    SAY_TRIGGER_VORREL          = -1189015, // The monster got what he deserved. NO SOUND VORREL SENGUTZ

    SPELL_IMPENDING_SENTENCE    = 35880,
    SPELL_COST_OF_SILENCE       = 35881,
    SPELL_RECIDIVISM            = 35897,
    SPELL_ATONEMENT             = 35883,
    SPELL_SHADOWWORDPAIN        = 35884,
    SPELL_ATONEMENT_1           = 35892,
    SPELL_ATONEMENT_2           = 35893,
    SPELL_ATONEMENT_3           = 35894,
    SPELL_ORDEAL_GRIP           = 35898,
    SPELL_ORDEAL_GRIP_CD        = 35899,
    SPELL_PUMMEL                = 35900,

    SPELL_TOTAL_CONTROL_MELEE   = 35885,
    SPELL_TOTAL_CONTROL_RANGED  = 35886,
    SPELL_TOTAL_CONTROL_BOSS    = 35887,
    SPELL_KILL_VICTIMS          = 35888,

    NPC_TOTAL_CONTROL_DUMMY     = 25238,
    NPC_VORREL                  = 25231,
    NPC_VICTIM                  = 25232,
};

//float const RackPos[3][4] =
//{
//    { 417.16f, 1392.3f, 17.52f, 2.38f},
//    { 417.92f, 1429.42f, 17.6f, 0.72f},
//    { 443.19f, 1421.44f, 17.6f, 6.18f}
//};

struct boss_interrogator_vishasAI : public ScriptedAI
{
    boss_interrogator_vishasAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 ImpendingSentence_Timer;
    uint32 Atonement_Timer;
    uint32 ShadowWordPain_Timer;
    uint32 Grip_Timer;
    uint32 Pummel_Timer;
    int32 LastRack;
    uint32 PosCheckTimer;

    void Reset()
    {
        ImpendingSentence_Timer = urand(6000, 10000);
        Atonement_Timer = 20000;
        ShadowWordPain_Timer = 5000;
        Grip_Timer = urand(8000, 15000);
        Pummel_Timer = 1000;
        LastRack = 0;
        PosCheckTimer = 10000;
    }

    void Aggro(Unit *who)
    {
        m_creature->SetInCombatWithZone();
        DoScriptText(SAY_AGGRO, m_creature);
        if (Creature* Dummy = m_creature->FindNearestCreature(NPC_TOTAL_CONTROL_DUMMY, 100.0f))
        {
            Dummy->AddAura(SPELL_TOTAL_CONTROL_MELEE);
            Dummy->AddAura(SPELL_TOTAL_CONTROL_RANGED);
        }

        std::list<Creature*> Victims;
        GetCreatureListWithEntryInGrid(Victims, m_creature, NPC_VICTIM, 100.0f);
        for (Creature* VictimCrea : Victims)
        {
            if (VictimCrea && VictimCrea->isAlive())
            {
                VictimCrea->CastSpell(VictimCrea, SPELL_KILL_VICTIMS, true);
            }
        }

        if (m_pInstance)
            m_pInstance->SetData(TYPE_VISHAS, IN_PROGRESS);
    }

    void KilledUnit(Unit* Victim)
    {
        if (!m_creature->HasAura(34821))    //5sec CD
        {
            DoScriptText(SAY_KILL, m_creature);
            m_creature->CastSpell(m_creature, 34821, true);
        }
    }

    void JustDied(Unit* Killer)
    {
        /*if (!m_pInstance)
            return;*/

        //Any other actions to do with vorrel? setStandState?
        if (Unit* vorrel = m_creature->FindNearestCreature(NPC_VORREL, 100.0f))
            DoScriptText(SAY_TRIGGER_VORREL, vorrel);

        if (Creature* Dummy = m_creature->FindNearestCreature(NPC_TOTAL_CONTROL_DUMMY, 100.0f))
        {
            Dummy->RemoveAurasDueToSpell(SPELL_TOTAL_CONTROL_MELEE);
            Dummy->RemoveAurasDueToSpell(SPELL_TOTAL_CONTROL_RANGED);
        }

        if (m_pInstance)
            m_pInstance->SetData(TYPE_VISHAS, DONE);
    }

    void JustReachedHome()
    {
        std::list<Creature*> Victims;
        GetCreatureListWithEntryInGrid(Victims, m_creature, NPC_VICTIM, 100.0f);
        for (Creature* VictimCrea : Victims)
        {
            if (VictimCrea && !VictimCrea->isAlive())
            {
                VictimCrea->SetDeadByDefault(false);
                VictimCrea->Respawn();

                VictimCrea->SetHealth(VictimCrea->GetMaxHealth());
                VictimCrea->clearUnitState(UNIT_STAT_DIED);
                VictimCrea->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD);

                VictimCrea->UpdateObjectVisibility();

            }
        }

        if (Creature* Dummy = m_creature->FindNearestCreature(NPC_TOTAL_CONTROL_DUMMY, 100.0f))
        {
            Dummy->RemoveAurasDueToSpell(SPELL_TOTAL_CONTROL_MELEE);
            Dummy->RemoveAurasDueToSpell(SPELL_TOTAL_CONTROL_RANGED);
        }

        if (m_pInstance)
            m_pInstance->SetData(TYPE_VISHAS, FAIL);
    }

    void UpdateAI(const uint32 diff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (PosCheckTimer < diff)
        {
            if (m_creature->GetPositionX() > 457.0f)
                EnterEvadeMode();

            PosCheckTimer = 1000;
        }
        else PosCheckTimer -= diff;

        if (Pummel_Timer < diff)    //Interrupt casting
        {
            if (m_creature->getVictim()->IsNonMeleeSpellCasted())
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_PUMMEL) == CAST_OK)
                    Pummel_Timer = 10000;
                else
                    Pummel_Timer = 100;
            }
            Pummel_Timer = 500;
        }
        else Pummel_Timer -= diff;

        if (Grip_Timer < diff)  //Stun
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_ORDEAL_GRIP) == CAST_OK)
                Grip_Timer = urand(12000, 16000);
        }
        else Grip_Timer -= diff;

        if (ImpendingSentence_Timer < diff) //Judgement
        {
            if (Unit* randomTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, nullptr, SELECT_FLAG_PLAYER | SELECT_FLAG_NOT_TARGET))
            {
                if (!randomTarget->HasAura(SPELL_RECIDIVISM) && !randomTarget->HasAura(SPELL_IMPENDING_SENTENCE) && !randomTarget->HasAura(SPELL_COST_OF_SILENCE) && !randomTarget->HasAura(SPELL_ATONEMENT))
                {
                    m_creature->CastSpell(randomTarget, SPELL_IMPENDING_SENTENCE, true);
                    DoScriptText(SAY_NAUGHTY_SECRETS, m_creature);
                    ImpendingSentence_Timer = urand(15000, 20000);
                }
            }
        }
        else
            ImpendingSentence_Timer -= diff;

        if (Atonement_Timer < diff) //Stun + hpDoT
        {
            if (Unit* randomTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, nullptr, SELECT_FLAG_PLAYER | SELECT_FLAG_NOT_TARGET))
            {
                if (!randomTarget->HasAura(SPELL_ATONEMENT) && !randomTarget->HasAura(SPELL_IMPENDING_SENTENCE) && !randomTarget->HasAura(SPELL_COST_OF_SILENCE))
                {
                    int32 RandPos = urand(SPELL_ATONEMENT_1, SPELL_ATONEMENT_3);

                    if (LastRack != RandPos)
                    {
                        LastRack = RandPos;
                        m_creature->CastSpell(randomTarget, RandPos, true);
                        DoScriptText(SAY_ATONEMENT, m_creature);
                        Atonement_Timer = 20000;
                    }
                }
            }
        }
        else
            Atonement_Timer -= diff;

        if (ShadowWordPain_Timer < diff)    //ShadowWordPain_Timer
        {
            if (DoCastSpellIfCan(m_creature, SPELL_SHADOWWORDPAIN) == CAST_OK)
                ShadowWordPain_Timer = urand(6000, 10000);
        }
        else ShadowWordPain_Timer -= diff;


        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_interrogator_vishas(Creature* pCreature)
{
    return new boss_interrogator_vishasAI(pCreature);
}

void AddSC_boss_interrogator_vishas()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "boss_interrogator_vishas";
    newscript->GetAI = &GetAI_boss_interrogator_vishas;
    newscript->RegisterSelf();
}
