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
SDName: Boss_Venoxis
SD%Complete: 100
SDComment:
SDCategory: Zul'Gurub
EndScriptData */

#include "scriptPCH.h"
#include "zulgurub.h"

enum
{
    NPC_RAZZASHI_COBRA  = 11373,

    SAY_TRANSFORM       = -1309000,
    SAY_DEATH           = -1309001,

    // P1 spells
    SPELL_HOLY_NOVA     = 23858,
    SPELL_DISPELL       = 23859,
    SPELL_HOLY_FIRE     = 23860,
    SPELL_HOLY_SMITE    = 34879,
    SPELL_RENEW         = 23895,
    SPELL_HOLY_WRATH    = 23979,

    SPELL_SNAKE_FORM    = 23849,

    // P2 spells
    SPELL_TRASH         = 3391,
    SPELL_POISON_CLOUD  = 23861,
    SPELL_VENOMSPIT     = 23862,

    SPELL_FRENZY        = 8269,
    // Cobra spell
    SPELL_PARASITIC     = 23865,
    GO_VENOXIS_DOOR     = 400005,


};

/*
SELECT * FROM creature_template WHERE entry=14877;
INSERT INTO creature_template (entry, modelid_1, modelid_2, name, faction)
VALUES (14877, 15102, 15102, "High Priest Venoxis Transform Visual", 35);
*/

struct boss_venoxisAI : public ScriptedAI
{
    boss_venoxisAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        m_fDefaultSize = m_creature->GetFloatValue(OBJECT_FIELD_SCALE_X);
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 m_uiHolyFire_Timer;
    uint32 m_uiHolyWrath_Timer;
    uint32 m_uiHolySmite_Timer;
    uint32 m_uiVenomSpit_Timer;
    uint32 m_uiRenew_Timer;
    uint32 m_uiPoisonCloud_Timer;
    uint32 m_uiHolyNova_Timer;
    uint32 m_uiDispell_Timer;
    uint32 m_uiParasitic_Timer;
    uint32 m_uiTrash_Timer;
    uint32 m_uiExploit_Timer;

    ObjectGuidSet lAddsGUIDs;

    uint8 m_uiTargetsInRangeCount;

    bool m_bPhaseTwo;
    bool bFrenzy;

    float m_fDefaultSize;

    void Reset()
    {
        m_uiHolyFire_Timer = 10000;
        m_uiHolyWrath_Timer = 12000;
        m_uiHolySmite_Timer = 15000;
        m_uiVenomSpit_Timer = 5500;
        m_uiRenew_Timer = 30500;
        m_uiPoisonCloud_Timer = 2000;
        m_uiDispell_Timer = 35000;
        m_uiParasitic_Timer = 10000;
        m_uiTrash_Timer = 5000;
        m_uiHolyNova_Timer = 7500;
        m_uiExploit_Timer = 10000;
        bFrenzy = false;

        m_uiTargetsInRangeCount = 0;

        m_creature->SetFloatValue(OBJECT_FIELD_SCALE_X, m_fDefaultSize);
        m_creature->ResetStats();

        m_bPhaseTwo = false;
        if (!lAddsGUIDs.empty())
        {
            for (ObjectGuidSet::iterator it = lAddsGUIDs.begin(); it != lAddsGUIDs.end(); ++it)
            {
                if (Creature *pSerpent = m_creature->GetMap()->GetCreature(*it))
                    pSerpent->AddObjectToRemoveList();
            }
        }
    }

    void Aggro(Unit* pWho)
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_VENOXIS, IN_PROGRESS);
        if (GameObject* door = m_creature->FindNearestGameObject(GO_VENOXIS_DOOR, 100.0f))
            door->UseDoorOrButton();
    }

    void EnterEvadeMode()
    {
        // Despawn snakes immediately when we're running home.
        std::list<Creature*> cobras;
        GetCreatureListWithEntryInGrid(cobras, m_creature, NPC_RAZZASHI_COBRA, DEFAULT_VISIBILITY_INSTANCE);
        for (auto it = cobras.cbegin(); it != cobras.cend(); ++it)
        {
            (*it)->RemoveAllDynObjects();
            (*it)->ForcedDespawn();
        }
        std::list<Creature*> clouds;
        GetCreatureListWithEntryInGrid(clouds, m_creature, 25025, DEFAULT_VISIBILITY_INSTANCE);
        for (auto it = clouds.cbegin(); it != clouds.cend(); ++it)
        {
            (*it)->ForcedDespawn();
        }
        
        ScriptedAI::EnterEvadeMode();
    }

    void JustReachedHome()
    {
        // Respawn snakes
        std::list<Creature*> cobras;
        GetCreatureListWithEntryInGrid(cobras, m_creature, NPC_RAZZASHI_COBRA, DEFAULT_VISIBILITY_INSTANCE);

        if (!cobras.empty())
        {
            for (auto iter = cobras.cbegin(); iter != cobras.cend(); ++iter)
            {
                auto cobra = *iter;
                if (cobra && !cobra->isAlive())
                    cobra->Respawn();
            }
        }

        if (m_pInstance)
            m_pInstance->SetData(TYPE_VENOXIS, NOT_STARTED);
        if (GameObject* door = m_creature->FindNearestGameObject(GO_VENOXIS_DOOR, 150.0f))
            door->ResetDoorOrButton();
    }

    void JustDied(Unit* pKiller)
    {
        DoScriptText(SAY_DEATH, m_creature);
        m_creature->CastSpell(m_creature, SPELL_POISON_CLOUD, true);

        m_creature->SetFloatValue(OBJECT_FIELD_SCALE_X, m_fDefaultSize);

        // despawn snakes
        std::list<Creature*> cobras;
        GetCreatureListWithEntryInGrid(cobras, m_creature, NPC_RAZZASHI_COBRA, DEFAULT_VISIBILITY_INSTANCE);

        if (!cobras.empty())
        {
            for (auto iter = cobras.cbegin(); iter != cobras.cend(); ++iter)
            {
                auto cobra = *iter;
                if (cobra && cobra->isAlive())
                    cobra->AddObjectToRemoveList();
            }
        }

        if (m_pInstance)
            m_pInstance->SetData(TYPE_VENOXIS, DONE);
        if (GameObject* door = m_creature->FindNearestGameObject(GO_VENOXIS_DOOR, 150.0f))
            door->ResetDoorOrButton();
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;
        if (m_creature->IsNonMeleeSpellCasted(false))
            return;

        /** Prevent exploit */
        if (m_uiExploit_Timer < uiDiff)
        {
            if (m_creature->GetDistance2d(-12005.5f, -1690.5f) > 50.0f || m_creature->GetPositionZ() > 43.0f || m_creature->GetPositionZ() < 27.0f)
            {
                EnterEvadeMode();
                return;
            }
            m_uiExploit_Timer = 5000;
        }
        else
            m_uiExploit_Timer -= uiDiff;


        // Handle phase change
        if (!m_bPhaseTwo && m_creature->GetHealthPercent()  < 50)
        {
            DoScriptText(SAY_TRANSFORM, m_creature);

            m_creature->InterruptNonMeleeSpells(false);
            m_creature->CastSpell(m_creature, SPELL_SNAKE_FORM, true);

            m_creature->SetFloatValue(OBJECT_FIELD_SCALE_X, m_fDefaultSize * 2);
            DoResetThreat();
            m_bPhaseTwo = true;
        }

        if (!m_bPhaseTwo)
        {
            /// Phase 1
            if (m_uiHolyNova_Timer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_HOLY_NOVA) == CAST_OK)
                    m_uiHolyNova_Timer = urand(14000, 16000);
            }
            else
                m_uiHolyNova_Timer -= uiDiff;

            if (m_uiDispell_Timer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_DISPELL) == CAST_OK)
                    m_uiDispell_Timer = urand(8000, 15000);
            }
            else
                m_uiDispell_Timer -= uiDiff;

            if (m_uiRenew_Timer < uiDiff)
            {
                if (!m_creature->HasAura(SPELL_RENEW))
                {
                    if (DoCastSpellIfCan(m_creature, SPELL_RENEW) == CAST_OK)
                        m_uiRenew_Timer = urand(6000, 18000);
                }
                else
                    m_uiRenew_Timer = 1000;

            }
            else
                m_uiRenew_Timer -= uiDiff;

            if (m_uiHolyWrath_Timer < uiDiff)
            {
                if (Unit* Wtarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                {
                    if (m_creature->GetDistance(Wtarget) > 15.0f)
                        if (DoCastSpellIfCan(Wtarget, SPELL_HOLY_WRATH) == CAST_OK)
                            m_uiHolyWrath_Timer = urand(5000, 12000);
                }
            }
            else
                m_uiHolyWrath_Timer -= uiDiff;

            if (m_uiHolyFire_Timer < uiDiff)
            {
                if (Unit* target = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                {
                    if (DoCastSpellIfCan(target, SPELL_HOLY_FIRE) == CAST_OK)
                        m_uiHolyFire_Timer = urand(5000, 8000);
                }
            }
            else
                m_uiHolyFire_Timer -= uiDiff;

            if (m_uiHolySmite_Timer < uiDiff)
            {
                if (Unit* Starget = m_creature->getVictim())
                {
                    if (m_creature->GetDistance(Starget) < 15.0f)
                        if (DoCastSpellIfCan(Starget, SPELL_HOLY_SMITE) == CAST_OK)
                            m_uiHolySmite_Timer = 1500;
                }
            }
            else
                m_uiHolySmite_Timer -= uiDiff;

        }
        else
        {
            // Phase 2

            if (m_uiPoisonCloud_Timer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_POISON_CLOUD) == CAST_OK)
                    m_uiPoisonCloud_Timer = urand(6000, 10000);
            }
            else
                m_uiPoisonCloud_Timer -= uiDiff;

            if (m_uiTrash_Timer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_TRASH) == CAST_OK)
                    m_uiTrash_Timer = urand(10000, 18000);
            }
            else
                m_uiTrash_Timer -= uiDiff;

            if (m_uiVenomSpit_Timer < uiDiff)
            {
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                    if (DoCastSpellIfCan(pTarget, SPELL_VENOMSPIT) == CAST_OK)
                        m_uiVenomSpit_Timer = urand(8000, 15000);
            }
            else
                m_uiVenomSpit_Timer -= uiDiff;

            if (m_uiParasitic_Timer < uiDiff)
            {
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                {
                    if (DoCastSpellIfCan(pTarget, SPELL_PARASITIC) == CAST_OK)
                    {
                        if (Unit* pSerpent = m_creature->SummonCreatureAndAttack(14884, pTarget))
                            lAddsGUIDs.insert(pSerpent->GetObjectGuid());
                        m_uiParasitic_Timer = urand(5000,10000);
                    }
                }
            }
            else
                m_uiParasitic_Timer -= uiDiff;
        }

        // FRENZY
        if (!bFrenzy && m_creature->GetHealthPercent() < 20.0f)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_FRENZY, CF_AURA_NOT_PRESENT) == CAST_OK)
                m_creature->AddAura(26602),
                bFrenzy = true;
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_venoxis(Creature* pCreature)
{
    return new boss_venoxisAI(pCreature);
}

void AddSC_boss_venoxis()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "boss_venoxis";
    newscript->GetAI = &GetAI_boss_venoxis;
    newscript->RegisterSelf();
}
