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
SDName: Boss_Marli
SD%Complete: 100
SDComment: Vilebranch Speaker respawned when wipe?
SDCategory: Zul'Gurub
EndScriptData */

#include "scriptPCH.h"
#include "zulgurub.h"

enum
{
    GO_EGG                      = 179985,

    // the spider
    NPC_SPAWN_OF_MARLI          = 15041,

    SAY_AGGRO                   = -1309005,
    SAY_TRANSFORM               = -1309006,
    SAY_SPIDER_SPAWN            = -1309007,
    SAY_DEATH                   = -1309008,

    SPELL_CHARGE                = 22911,
    SPELL_ENVELOPINGWEBS        = 24110,
    SPELL_POISONVOLLEY          = 24099,
    SPELL_SPIDER_FORM           = 24084,
    SPELL_DRAIN_LIFE            = 24300,
    SPELL_CORROSIVE_POISON      = 24111,
    SPELL_TRANSFORM_BACK        = 24085,
    SPELL_TRASH                 = 3391,
    SPELL_HATCH                 = 24083,                    //visual
    SPELL_AGGRANDIR             = 24109,
    SPELL_SUMMON_VENOM_TOTEM    = 34951,

    //The Spider Spells
    SPELL_LEVELUP               = 24312,                     //visual

    GO_MARLI_DOOR               = 400010,
    NPC_VENOMOUS_TOTEM          = 25027
};

//venomous totem with 24099 spell

struct boss_marliAI : public ScriptedAI
{
    boss_marliAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        m_uiDefaultModel = m_creature->GetDisplayId();
        m_fDefaultSize = m_creature->GetFloatValue(OBJECT_FIELD_SCALE_X);
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 m_uiPoisonVolley_Timer;
    uint32 m_uiSpawnSpider_Timer;
    uint32 m_uiCharge_Timer;
    uint32 m_uiAspect_Timer;
    uint32 m_uiTransformBack_Timer;
    uint32 m_uiDrainLife_Timer;
    uint32 m_uiCorrosivePoison_Timer;
    uint32 m_uiWebs_Timer;
    uint32 m_uiTrash_Timer;
    uint32 m_uiAggrandir_Timer;
    uint32 m_uiVenomous_Totem_Timer;
    uint32 m_uiExploit_Timer;

    bool m_bFirstSpidersAreSpawned;
    bool m_bIsInPhaseTwo;
    bool m_bHasWebbed;

    uint32 m_uiDefaultModel;
    float m_fDefaultSize;
    std::list<ObjectGuid> summonedCreatures;

    void Reset()
    {
        m_uiPoisonVolley_Timer = 15000;
        m_uiSpawnSpider_Timer = 20000;
        m_uiAspect_Timer = 12000;
        m_uiTransformBack_Timer = 35000;
        m_uiDrainLife_Timer = 30000;
        m_uiCorrosivePoison_Timer = 1000;
        m_uiWebs_Timer = 5000;
        m_uiTrash_Timer = 5000;
        m_uiAggrandir_Timer = 0;
        m_uiExploit_Timer = 15000;
        m_uiVenomous_Totem_Timer = urand(5000, 25000);

        m_bFirstSpidersAreSpawned = false;
        m_bIsInPhaseTwo = false;
        m_bHasWebbed = false;
        m_creature->SetFloatValue(OBJECT_FIELD_SCALE_X, m_fDefaultSize);

        DespawnAllSummons();

        if (m_pInstance)
            if (m_pInstance->GetData(TYPE_MARLI) != DONE)
                m_pInstance->SetData(TYPE_MARLI, NOT_STARTED);

        if (GameObject* door = m_creature->FindNearestGameObject(GO_MARLI_DOOR, 200.0f))
            door->ResetDoorOrButton();

        std::list<GameObject*> lSpiderEggs;
        GetGameObjectListWithEntryInGrid(lSpiderEggs, m_creature, GO_EGG, DEFAULT_VISIBILITY_INSTANCE);
        if (lSpiderEggs.empty())
            sLog.outDebug("boss_marli, no Eggs with the entry %u were found", GO_EGG);
        else
        {
            for (std::list<GameObject*>::iterator iter = lSpiderEggs.begin(); iter != lSpiderEggs.end(); ++iter)
            {
                if ((*iter)->GetGoState() == GO_STATE_ACTIVE)
                    (*iter)->SetGoState(GO_STATE_READY);
            }
        }
        // Annule les changements de buffs de transition de phases.
        m_creature->ResetStats();
    }

    void DespawnAllSummons()
    {
        while (!summonedCreatures.empty())
        {
            ObjectGuid const& g = *(summonedCreatures.begin());
            summonedCreatures.pop_front();
            switch (g.GetEntry())
            {
            case NPC_SPAWN_OF_MARLI:
            case NPC_VENOMOUS_TOTEM:
                if (Creature* c = m_creature->GetMap()->GetCreature(g))
                    c->AddObjectToRemoveList();
                break;
            }
        }
        while (Creature* Crea = m_creature->FindNearestCreature(NPC_VENOMOUS_TOTEM, 150.0f))
            Crea->DisappearAndDie();
    }

    void Aggro(Unit* pWho)
    {
        DoScriptText(SAY_AGGRO, m_creature);

        if (m_pInstance)
            if (m_pInstance->GetData(TYPE_MARLI) != IN_PROGRESS)
                m_pInstance->SetData(TYPE_MARLI, IN_PROGRESS);

        if (GameObject* door = m_creature->FindNearestGameObject(GO_MARLI_DOOR, 200.0f))
            door->UseDoorOrButton();

        if (!m_bFirstSpidersAreSpawned)
        {
            DoScriptText(SAY_SPIDER_SPAWN, m_creature);
            DoCastSpellIfCan(m_creature, SPELL_HATCH);

            for (uint8 i = 0; i < 4 ; ++i)
            {
                if (GameObject *pEgg = SelectNextEgg())
                {
                    pEgg->SetGoState(GO_STATE_ACTIVE);
                    m_creature->SummonCreature(NPC_SPAWN_OF_MARLI, pEgg->GetPositionX(), pEgg->GetPositionY(), pEgg->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 15000);
                }
            }

            m_bFirstSpidersAreSpawned = true;
        }
    }

    GameObject* SelectNextEgg()
    {
        std::list<GameObject*> lEggs;
        GetGameObjectListWithEntryInGrid(lEggs, m_creature, GO_EGG, DEFAULT_VISIBILITY_INSTANCE);
        if (lEggs.empty())
            sLog.outDebug("boss_marli, no Eggs with the entry %i were found", GO_EGG);
        else
        {
            lEggs.sort(ObjectDistanceOrder(m_creature));
            for (std::list<GameObject*>::iterator iter = lEggs.begin(); iter != lEggs.end(); ++iter)
            {
                if ((*iter)->GetGoState() == (GO_STATE_READY))
                    return (*iter);
            }
        }
        return NULL;
    }

    void JustSummoned(Creature* pSummoned)
    {
        if (pSummoned->GetEntry() == NPC_SPAWN_OF_MARLI)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                pSummoned->AI()->AttackStart(pTarget);
        }
        summonedCreatures.push_back(pSummoned->GetObjectGuid());
        ScriptedAI::JustSummoned(pSummoned);
    }

    void JustReachedHome()
    {
        if (GameObject* door = m_creature->FindNearestGameObject(GO_MARLI_DOOR, 200.0f))
            door->ResetDoorOrButton();
    }

    void JustDied(Unit* pKiller)
    {
        DoScriptText(SAY_DEATH, m_creature);

        if (m_pInstance)
            m_pInstance->SetData(TYPE_MARLI, DONE);

        if (GameObject* door = m_creature->FindNearestGameObject(GO_MARLI_DOOR, 200.0f))
            door->ResetDoorOrButton();

        // despawn totems
        std::list<Creature*> totems;
        GetCreatureListWithEntryInGrid(totems, m_creature, NPC_VENOMOUS_TOTEM, 180.0f);

        if (!totems.empty())
        {
            for (auto iter = totems.cbegin(); iter != totems.cend(); ++iter)
            {
                auto totems = *iter;
                if (totems && totems->isAlive())
                    totems->AddObjectToRemoveList();
            }
        }
    }

    void SpellHitTarget(Unit* pCaster, const SpellEntry* pSpell)
    {
        if (!pCaster)
            return;

        if (pSpell->Id == SPELL_ENVELOPINGWEBS)
        {
            if (pCaster->GetTypeId() != TYPEID_PLAYER)
                return;

            m_creature->getThreatManager().modifyThreatPercent(pCaster, -100);
        }
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        /** Prevent exploit */
        if (m_uiExploit_Timer < uiDiff)
        {
            if (m_creature->GetPositionZ() < 128.0f)
            {
                EnterEvadeMode();
                return;
            }
            m_uiExploit_Timer = 5000;
        }
        else
            m_uiExploit_Timer -= uiDiff;

        // Troll
        if (!m_bIsInPhaseTwo)
        {
            if (m_uiVenomous_Totem_Timer < uiDiff)
            {
                if (Unit* vTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                {
                    m_creature->CastSpell(vTarget, SPELL_SUMMON_VENOM_TOTEM, true);
                    m_uiVenomous_Totem_Timer = urand(25000, 35000);
                }
            }
            else
                m_uiVenomous_Totem_Timer -= uiDiff;

            if (m_uiPoisonVolley_Timer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_POISONVOLLEY) == CAST_OK)
                    m_uiPoisonVolley_Timer = urand(5000, 10000);
            }
            else
                m_uiPoisonVolley_Timer -= uiDiff;

            if (m_uiSpawnSpider_Timer < uiDiff)
            {
                // Mar'li doit invoquer entre 1 et 4 "rejetons"
                uint32 uiInvocCount = urand(1, 4);
                for (uint8 i = 0; i < uiInvocCount; ++i)
                {
                    if (GameObject *pEgg = SelectNextEgg())
                    {
                        pEgg->SetGoState(GO_STATE_ACTIVE);
                        m_creature->SummonCreature(NPC_SPAWN_OF_MARLI, pEgg->GetPositionX(), pEgg->GetPositionY(), pEgg->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 15000);
                    }
                }
                m_uiSpawnSpider_Timer = urand(15000, 25000);
            }
            else
                m_uiSpawnSpider_Timer -= uiDiff;

            if (m_uiAggrandir_Timer < uiDiff)
            {
                if (!m_creature->HasAura(SPELL_AGGRANDIR))
                {
                    if (DoCastSpellIfCan(m_creature, SPELL_AGGRANDIR) == CAST_OK)
                        m_uiAggrandir_Timer = urand(10000, 20000);
                }
                else
                    m_uiAggrandir_Timer = 2000;
            }
            else
                m_uiAggrandir_Timer -= uiDiff;

            if (m_uiDrainLife_Timer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_DRAIN_LIFE) == CAST_OK)
                    m_uiDrainLife_Timer = urand(8000, 12000);
            }
            else
                m_uiDrainLife_Timer -= uiDiff;

        }
        // Spider
        else
        {
            if (!m_bHasWebbed && m_uiWebs_Timer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_ENVELOPINGWEBS) == CAST_OK)
                {
                    m_uiWebs_Timer = urand(10000, 15000);
                    m_uiCharge_Timer = 1000;
                    m_bHasWebbed = true;
                }
            }
            else
                m_uiWebs_Timer -= uiDiff;

            if (m_bHasWebbed && m_uiCharge_Timer < uiDiff)
            {
                //Shouldn't be random target but highestaggro not Webbed player
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 0))
                {
                    if (DoCastSpellIfCan(pTarget, SPELL_CHARGE) == CAST_OK)
                    {
                        DoResetThreat();
                        AttackStart(pTarget);
                        m_bHasWebbed = false;
                        /*
                            DoResetThreat();
                            Unit* pTarget = NULL;
                            uint8 i = 0 ;
                            while (i < 5)                               // max 3 tries to get a random target with power_mana
                            {
                                ++i;                                    //not aggro leader
                                pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM,0);
                                if (pTarget && pTarget->getPowerType() == POWER_MANA)
                                    i=5;
                            }
                        */
                    }
                }
                m_uiWebs_Timer = urand(10000, 20000);
            }
            else
                m_uiCharge_Timer -= uiDiff;

            if (m_uiCorrosivePoison_Timer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_CORROSIVE_POISON) == CAST_OK)
                    m_uiCorrosivePoison_Timer = urand(5000, 15000);
            }
            else
                m_uiCorrosivePoison_Timer -= uiDiff;
        }

        if (m_uiTransformBack_Timer < uiDiff)
        {
            if (!m_bIsInPhaseTwo)
            {
                if (m_creature->IsNonMeleeSpellCasted(false))
                    m_creature->InterruptNonMeleeSpells(false);

                DoScriptText(SAY_TRANSFORM, m_creature);
                DoCastSpellIfCan(m_creature, SPELL_SPIDER_FORM);

                DoResetThreat();

                m_bIsInPhaseTwo = true;
            }
            else
            {
                if (m_creature->IsNonMeleeSpellCasted(false))
                    m_creature->InterruptNonMeleeSpells(false);

                m_creature->RemoveAurasDueToSpell(SPELL_SPIDER_FORM);

                m_creature->SetDisplayId(m_uiDefaultModel);


                m_bIsInPhaseTwo = false;
                m_uiVenomous_Totem_Timer += urand(1000, 3000);
            }

            m_uiTransformBack_Timer = 35000;
        }
        else
            m_uiTransformBack_Timer -= uiDiff;

        if (m_uiTrash_Timer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_TRASH) == CAST_OK)
                m_uiTrash_Timer = urand(10000, 20000);
        }
        else
            m_uiTrash_Timer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

//Spawn of Marli
struct mob_spawn_of_marliAI : public ScriptedAI
{
    mob_spawn_of_marliAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
        m_pInstance = (ScriptedInstance*)m_creature->GetInstanceData();
    }

    ScriptedInstance* m_pInstance;
    uint32 m_uiLevelUp_Timer;

    void Reset()
    {
        m_uiLevelUp_Timer = 3000;
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiLevelUp_Timer < uiDiff)
        {
            DoCastSpellIfCan(m_creature, SPELL_LEVELUP);
            m_creature->SetLevel(m_creature->getLevel() + 1);
            m_uiLevelUp_Timer = 5000;
        }
        else
            m_uiLevelUp_Timer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_marli(Creature* pCreature)
{
    return new boss_marliAI(pCreature);
}

CreatureAI* GetAI_mob_spawn_of_marli(Creature* pCreature)
{
    return new mob_spawn_of_marliAI(pCreature);
}

void AddSC_boss_marli()
{
    Script* newscript;

    newscript = new Script;
    newscript->Name = "boss_marli";
    newscript->GetAI = &GetAI_boss_marli;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "mob_spawn_of_marli";
    newscript->GetAI = &GetAI_mob_spawn_of_marli;
    newscript->RegisterSelf();
}
