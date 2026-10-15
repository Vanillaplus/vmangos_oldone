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
SDName: Boss_Baron_Geddon
SD%Complete: 100
SDComment:
SDCategory: Molten Core
EndScriptData */

#include "scriptPCH.h"
#include "molten_core.h"

#define EMOTE_SERVICE               -1409000

#define SPELL_INFERNO               19695
#define SPELL_IGNITEMANA            19659
#define SPELL_LIVINGBOMB            20475
#define SPELL_ARMAGEDDOM            20478
#define SPELL_SHAREDFLAMES          34471 //v+

struct boss_baron_geddonAI : public ScriptedAI
{
    boss_baron_geddonAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    uint32 m_uiIgniteManaTimer;
    uint32 m_uiLivingBombTimer;
    uint32 m_uiInfernoTimer;
    uint32 m_uiInfernoAltTimer;
    /*uint32 m_uiRestoreTargetTimer;*/
    uint32 InfCount;
    uint32 Tick;
    uint32 SpreadingFlamesInRow;
    bool m_bInferno;
    bool m_bArmageddon;

    ScriptedInstance* m_pInstance;

    void Reset()
    {
        m_uiIgniteManaTimer    = urand(10000, 15000);
        m_uiLivingBombTimer    = urand(12000, 15000);
        m_uiInfernoTimer       = urand(18000, 24000);
        SpreadingFlamesInRow   = 0;
        /*m_uiRestoreTargetTimer = 0;*/
        m_bInferno             = false;
        m_bArmageddon          = false;

        if (m_pInstance && m_creature->isAlive())
            m_pInstance->SetData(TYPE_GEDDON, NOT_STARTED);

        m_creature->clearUnitState(UNIT_STAT_ROOT);
    }

    void Aggro(Unit* pWho)
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_GEDDON, IN_PROGRESS);
        m_creature->SetInCombatWithZone();
    }

    void EnterEvadeMode()
    {
        std::list<Creature*> clouds;
        GetCreatureListWithEntryInGrid(clouds, m_creature, 25017, 400.0f);
        for (auto it = clouds.cbegin(); it != clouds.cend(); ++it)
        {
            (*it)->RemoveAllDynObjects();
            (*it)->ForcedDespawn();
        }

        m_creature->RemoveAllDynObjects();

        ScriptedAI::EnterEvadeMode();
    }

    void JustDied(Unit* pKiller)
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_GEDDON, DONE);

        std::list<Creature*> clouds;
        GetCreatureListWithEntryInGrid(clouds, m_creature, 25017, 400.0f);
        for (auto it = clouds.cbegin(); it != clouds.cend(); ++it)
        {
            (*it)->RemoveAllDynObjects();
            (*it)->ForcedDespawn();
        }

        m_creature->RemoveAllDynObjects();
    }

    void UpdateAI(const uint32 diff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_bArmageddon)
            return;

        //If we are <5% hp cast Armageddom
        if (!m_bArmageddon)
        {
            if (m_creature->GetHealthPercent() < 5.0f)
            {
                m_creature->InterruptNonMeleeSpells(true);
                SetCombatMovement(false);
                //m_creature->SetTargetGuid(ObjectGuid());
                m_creature->CastSpell(m_creature, SPELL_ARMAGEDDOM, true);
                DoScriptText(EMOTE_SERVICE, m_creature);
                m_bArmageddon = true;
                return;
            }
        }

        if (m_uiLivingBombTimer < diff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, nullptr, SELECT_FLAG_PLAYER))
            {
				if (!(pTarget->HasAura(SPELL_LIVINGBOMB) || pTarget->HasAura(SPELL_SHAREDFLAMES) || m_creature->getVictim() == pTarget))
				{
					uint32 bombspell;
                    if (SpreadingFlamesInRow > 1)
                        bombspell = SPELL_LIVINGBOMB;
                    else
                    {
                        if (urand(0, 2) > 0)
                            bombspell = SPELL_LIVINGBOMB;
                        else
                            bombspell = SPELL_SHAREDFLAMES;
                    }


					if (DoCastSpellIfCan(pTarget, bombspell) == CAST_OK)
					{
                        if (bombspell == SPELL_SHAREDFLAMES)
                            SpreadingFlamesInRow += 1;
                        else if (SpreadingFlamesInRow > 0)
                            SpreadingFlamesInRow -= 1;

						//m_creature->SetInFront(pTarget);
						//m_creature->SetTargetGuid(pTarget->GetObjectGuid());
						m_uiLivingBombTimer = urand(6000, 8000);
						//m_uiRestoreTargetTimer = 800;
					}

				}
            }
        }
        else
            m_uiLivingBombTimer -= diff;

        //// Restore original target after casting Living Bomb on someone
        //if (m_uiRestoreTargetTimer)
        //{
        //    if (m_uiRestoreTargetTimer <= diff)
        //    {
        //        if (Unit* pTarget = m_creature->getVictim())
        //        {
        //            m_creature->SetInFront(pTarget);
        //            m_creature->SetTargetGuid(pTarget->GetObjectGuid());
        //            m_uiRestoreTargetTimer = 0;
        //        }
        //    }
        //    else
        //        m_uiRestoreTargetTimer -= diff;
        //}

        if (m_uiIgniteManaTimer < diff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_IGNITEMANA) == CAST_OK)
                m_uiIgniteManaTimer = 30000;
        }
        else
            m_uiIgniteManaTimer -= diff;

        if (m_uiInfernoTimer < diff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_INFERNO) == CAST_OK)
            {
                m_uiLivingBombTimer += 10000;
                m_uiIgniteManaTimer += 10000;

                m_creature->CastSpell(m_creature->getVictim(), 34583, true);
                m_uiInfernoTimer = urand(20000, 25000);
                InfCount = 0;
                Tick = 1000;
                m_bInferno = true;
                m_creature->addUnitState(UNIT_STAT_ROOT);
            }
        }
        else
            m_uiInfernoTimer -= diff;

        // Inferno damage increases with each tick
        if (m_bInferno)
        {
            if (Tick >= 1000)
            {
                int Damage = 0;
                switch (InfCount)
                {
                    case 0:
                    case 1:
                        Damage = urand(500, 750);
                        break;
                    case 2:
                    case 3:
                        Damage = urand(1000, 1500);
                        break;
                    case 4:
                    case 5:
                        Damage = urand(1500, 2000);
                        break;
                    case 6:
                    case 7:
                        Damage = urand(2000, 2750);
                        break;
                    case 8:
                        Damage = urand(2750, 3500);
                        m_bInferno = false;
                        m_creature->clearUnitState(UNIT_STAT_ROOT);
                        m_creature->CastSpell(m_creature->getVictim(), 34584, true);
                        m_uiIgniteManaTimer += 8000;
                        DoResetThreat();
                        break;
                }
                m_creature->CastCustomSpell(m_creature, 19698, &Damage, NULL, NULL, true);
                InfCount++;
                Tick = 0;
            }
            Tick += diff;
            return;
        }


        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_baron_geddon(Creature* pCreature)
{
    return new boss_baron_geddonAI(pCreature);
}

void AddSC_boss_baron_geddon()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "boss_baron_geddon";
    newscript->GetAI = &GetAI_boss_baron_geddon;
    newscript->RegisterSelf();
}
