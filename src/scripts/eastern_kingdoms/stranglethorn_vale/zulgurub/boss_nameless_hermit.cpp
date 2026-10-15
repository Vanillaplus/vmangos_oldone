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
SDName: Boss_Nameless_Hermit
SDCategory: Zul'Gurub
EndScriptData */

#include "scriptPCH.h"
#include "zulgurub.h"

enum
{
    SAY_AGGRO1              = -1900250,
    SAY_AGGRO2              = -1900251,
    SAY_DEATH1              = -1900252,
    SAY_DEATH2              = -1900253,
    SAY_TRANSFORM           = -1900254,

    SPELL_TRANSFORM         = 34929,
    SPELL_SILENCE           = 34930,
    SPELL_ECHO_DUMMY        = 34931,
    SPELL_ECHO              = 34932,
    SPELL_ECHO_DISPEL       = 34933,
    SPELL_CHARGE            = 34934,
    SPELL_SUMMON_PLAYER     = 26229,
    SPELL_INFECTED_BITE     = 34935, //34936 for Trash mobs
    SPELL_DEVOUR            = 34937,
    SPELL_WHIRLWIND         = 1680,
    SPELL_MT_MARK           = 34939,


    /*m_creature->MonsterTextEmote("ECHO TEST TRIGGERED!", NULL);*/
    GO_HERMIT_DOOR = 400007,
    NPC_CROCOLISK  = 25040,
};

struct boss_hermitAI : public ScriptedAI
{
    boss_hermitAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        m_fDefaultSize = m_creature->GetFloatValue(OBJECT_FIELD_SCALE_X);
        Reset();
    }
    ScriptedInstance* m_pInstance;

    bool Transformed;
    float m_fDefaultSize;
    uint32 m_uiExploit_Timer;
    uint32 Silence_Timer;
    uint32 Echo_Timer;
    uint32 Transformation_Timer;
    uint32 DeTransformation_Timer;
    uint32 Devour_Timer;
    uint32 InfectedBite_Timer;
    uint32 Echo_ReCheck_Timer;
    uint32 Summon_Crocs_Timer;
    uint32 Whirlwind_Timer;
    Unit* pTarget;
    uint32 uiHighestStacks;
    uint32 holderStacks;

    void Reset()
    {
        m_uiExploit_Timer = 10000;
        Silence_Timer = urand(10000, 15000);
        Transformed = false;
        Echo_Timer = 2000;
        Transformation_Timer = 60000;
        DeTransformation_Timer = 120000;
        Devour_Timer = 300000;
        InfectedBite_Timer = 60000;
        Echo_ReCheck_Timer = 60000;
        Summon_Crocs_Timer = urand(15000, 25000);
        Whirlwind_Timer = 10000;
        pTarget = nullptr;
        uiHighestStacks = 0;
        uint32 holderStacks = 0;
    }

    void Aggro(Unit* pWho)
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_AZUS, IN_PROGRESS);
        if (GameObject* door = m_creature->FindNearestGameObject(GO_HERMIT_DOOR, 200.0f))
            door->UseDoorOrButton();

        DoScriptText(urand(0, 1) ? SAY_AGGRO1 : SAY_AGGRO2, m_creature);
    }

    void JustReachedHome()
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_AZUS, NOT_STARTED);

        if (GameObject* door = m_creature->FindNearestGameObject(GO_HERMIT_DOOR, 200.0f))
            door->ResetDoorOrButton();
    }

    void JustDied(Unit* pKiller)
    {
        DoScriptText(urand(0, 1) ? SAY_DEATH1 : SAY_DEATH2, m_creature);

        m_creature->SetFloatValue(OBJECT_FIELD_SCALE_X, m_fDefaultSize);

        if (m_pInstance)
            m_pInstance->SetData(TYPE_AZUS, DONE);
        if (GameObject* door = m_creature->FindNearestGameObject(GO_HERMIT_DOOR, 200.0f))
            door->ResetDoorOrButton();
    }

    void KilledUnit(Unit* pVictim)
    {
        if (pVictim->GetTypeId() == TYPEID_PLAYER)
        {
            if (Transformed)
                if (pVictim->HasAura(SPELL_MT_MARK))
                    GetEchoTarget();
        }
    }

    void GetEchoTarget()
    {
        pTarget = nullptr;
        uiHighestStacks = 0;

        Map::PlayerList const& players = m_creature->GetMap()->GetPlayers();
        if (players.isEmpty())
            return;

        for (Map::PlayerList::const_iterator iter = players.begin(); iter != players.end(); ++iter)
        {
            if (Player* pTempTarget = iter->getSource())
            {
                if (pTempTarget->HasAura(SPELL_ECHO))
                {
                    if (SpellAuraHolder* holder = pTempTarget->GetSpellAuraHolder(SPELL_ECHO))
                    {
                        if (holderStacks = holder->GetStackAmount())
                        {
                            if (holderStacks > uiHighestStacks)
                            {
                                pTarget = pTempTarget;
                                uiHighestStacks = holderStacks;
                            }
                        }
                    }
                }
            }
        }

        if (!pTarget)
            return;
            /*pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, nullptr, SELECT_FLAG_PLAYER);*/

        if (pTarget->HasAura(SPELL_MT_MARK))
        {
            return;
        }
        DoResetThreat();
        pTarget->AddAura(SPELL_MT_MARK);
        m_creature->getThreatManager().addThreatDirectly(pTarget, 1000000);
        /*DoCastSpellIfCan(pTarget, SPELL_MT_MARK, CF_TRIGGERED);
        m_creature->CastSpell(pTarget, SPELL_MT_MARK, true);*/


        if (DoCastSpellIfCan(pTarget, SPELL_CHARGE) == CAST_OK)
        {
        }
        else
        {
            m_creature->CastSpell(pTarget, SPELL_SUMMON_PLAYER, true);
        }

        Devour_Timer = 1000;
    }

    void SummonCrocs()
    {
        static constexpr float CrocPos[5][3] =
        {
            {-11268.3f, -1907.8f, 71.7},
            {-11264.9f, -1931.4f, 71.64f},
            {-11248.3f, -1942.6f, 71.63f},
            {-11259.7f, -1957.8f, 71.6f},
            {-11263.96f, -1979.2f, 71.1f}
        };

        for (int i = 0; i < 5; i++)
        {
            if (Unit* target = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, nullptr, SELECT_FLAG_PLAYER))
            {
                if (Creature* Croc = m_creature->SummonCreature(NPC_CROCOLISK, CrocPos[i][0], CrocPos[i][1], CrocPos[i][2], 3.3f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 8000))
                    if (Croc)
                    {
                        /*Croc->AddThreatsOf(m_creature);*/
                        Croc->AI()->AttackStart(target);
                        Croc->getThreatManager().addThreatDirectly(target, urand(100, 500));
                    }
            }
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        /** Prevent exploit */
        if (m_uiExploit_Timer < diff)
        {
            if (m_creature->GetDistance2d(-11332.0f, -1938.0f) > 140.0f)
            {
                EnterEvadeMode();
                return;
            }
            m_uiExploit_Timer = 5000;
        }
        else
            m_uiExploit_Timer -= diff;

        //Echo dummy
        if (Echo_Timer < diff)
        {
            m_creature->CastSpell(m_creature, SPELL_ECHO_DUMMY, true);
            Echo_Timer = 20000;

        }
        else Echo_Timer -= diff;

        //Transformation apply
        if (Transformation_Timer < diff)
        {
            DoScriptText(SAY_TRANSFORM, m_creature);
            Transformation_Timer = 120000;
            Transformed = true;

            m_creature->CastSpell(m_creature, SPELL_SILENCE, true);
            m_creature->AddAura(SPELL_TRANSFORM);
            GetEchoTarget();
        }
        else
            Transformation_Timer -= diff;
        //Transformation remove
        if (DeTransformation_Timer < diff)
        {
            DeTransformation_Timer = 120000;
            Silence_Timer = urand(3000, 10000);
            Transformed = false;

            Map::PlayerList const& players = m_creature->GetMap()->GetPlayers();
            if (players.isEmpty())
                return;

            for (Map::PlayerList::const_iterator iter = players.begin(); iter != players.end(); ++iter)
            {
                if (Player* pPlayer = iter->getSource())
                {
                    pPlayer->RemoveAurasDueToSpell(SPELL_ECHO);
                    pPlayer->RemoveAurasDueToSpell(SPELL_MT_MARK);
                }
            }
        }
        else
            DeTransformation_Timer -= diff;

        //TROLL
        if (!Transformed)
        {
            //Silence_Timer
            if (Silence_Timer < diff)
            {
                if (Transformation_Timer > 15000)
                {
                    m_creature->CastSpell(m_creature, SPELL_SILENCE, true);
                    Silence_Timer = urand(20000, 30000);
                }
            }
            else 
                Silence_Timer -= diff;

            //Whirlwind
            if (Whirlwind_Timer < diff)
            {
                m_creature->CastSpell(m_creature->getVictim(), SPELL_WHIRLWIND, true);
                Whirlwind_Timer = 10000;
            }
            else
                Whirlwind_Timer -= diff;

            //summon crocs
            if (Summon_Crocs_Timer < diff)
            {
                SummonCrocs();
                if (m_creature->GetDistance2d(-11223.7f, -1943.3f) > 100.0f)
                    Summon_Crocs_Timer = 12000;
                else
                    Summon_Crocs_Timer = 25000;
            }
            else
                Summon_Crocs_Timer -= diff;
        }
        //CROCOLISK
        else
        {
            //summon crocs
            if (Summon_Crocs_Timer < diff)
            {
                SummonCrocs();
                if (m_creature->GetDistance2d(-11223.7f, -1943.3f) > 100.0f)
                    Summon_Crocs_Timer = 8000;
                else
                    Summon_Crocs_Timer = 35000;
            }
            else
                Summon_Crocs_Timer -= diff;

            if (Echo_ReCheck_Timer < diff)
            {
                GetEchoTarget();
                Echo_ReCheck_Timer = urand(5000,8000);
            }
            else Echo_ReCheck_Timer -= diff;

            if (Devour_Timer < diff)
            {
                Devour_Timer = 300000;
                m_creature->CastSpell(m_creature->getVictim(), SPELL_DEVOUR, true);
            }
            else 
                Devour_Timer -= diff;

            if (InfectedBite_Timer < diff)
            {
                if ((DoCastSpellIfCan(m_creature->getVictim(), SPELL_INFECTED_BITE)) == CAST_OK)
                    InfectedBite_Timer = urand(4000, 8000);
            }
            else 
                InfectedBite_Timer -= diff;
        }

        DoMeleeAttackIfReady();
    }
};
CreatureAI* GetAI_boss_hermit(Creature* pCreature)
{
    return new boss_hermitAI(pCreature);
}

void AddSC_boss_hermit()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "boss_hermit";
    newscript->GetAI = &GetAI_boss_hermit;
    newscript->RegisterSelf();
}
