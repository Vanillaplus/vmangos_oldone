/* Copyright (C) 2006 - 2011 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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
SDName: Boss_Firemaw
SD%Complete: 80
SDComment: Thrash missing
SDCategory: Blackwing Lair
EndScriptData */

#include "scriptPCH.h"
#include "blackwing_lair.h"

enum
{
    EMOTE_GENERIC_WING_BUFFET   = -1469032,
    EMOTE_GENERIC_SHADOW_FLAME  = -1469033,

    SPELL_SHADOW_FLAME          = 22539,
    SPELL_WING_BUFFET           = 23339,
    SPELL_FLAME_BUFFET          = 23341,
    SPELL_THRASH                = 3391,
    SPELL_RIFT                  = 35205,

    NPC_RIFT                    = 25122,
    GO_RIFT                     = 302103,
};

struct boss_firemawAI : public ScriptedAI
{
    boss_firemawAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 m_uiShadowFlameTimer;
    uint32 m_uiWingBuffetTimer;
    uint32 m_uiFlameBuffetTimer;
    uint32 m_uiRiftTimer;
    uint32 m_uiEvadeRangeCheckTimer;

    void Reset()
    {
        m_uiShadowFlameTimer = 25000;
        m_uiWingBuffetTimer = 30000;
        m_uiFlameBuffetTimer = 2000;
        m_uiRiftTimer = 20000;
        m_uiEvadeRangeCheckTimer = 5000;
    }

    void Aggro(Unit* /*pWho*/)
    {
        if (m_pInstance)
        {
            if (m_pInstance->GetData(TYPE_LASHLAYER) != DONE)
            {
                EnterEvadeMode();
                return;
            }
            m_pInstance->SetData(TYPE_FIREMAW, IN_PROGRESS);
        }

        m_creature->SetInCombatWithZone();
    }

    void JustDied(Unit* /*pKiller*/)
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_FIREMAW, DONE);

        DespawnRifts();
    }

    void JustReachedHome()
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_FIREMAW, FAIL);

        DespawnRifts();
    }

    // Despawn rifts on evade/death
    void DespawnRifts()
    {
        std::list<GameObject*> rifts;
        GetGameObjectListWithEntryInGrid(rifts, m_creature, GO_RIFT, 300.0f);
        for (std::list<GameObject*>::const_iterator itr = rifts.begin(); itr != rifts.end(); ++itr)
        {
            if (rifts.empty())
                break;
            if (!(*itr)->isSpawned())
                continue;

            (*itr)->Despawn();
        }

        std::list<Creature*> riftsNPC;
        GetCreatureListWithEntryInGrid(riftsNPC, m_creature, NPC_RIFT, 300.0f);
        for (auto it = riftsNPC.cbegin(); it != riftsNPC.cend(); ++it)
        {
            (*it)->DeleteLater();
        }
    }

    void SpellHitTarget(Unit* pCaster, const SpellEntry* pSpell)
    {
        if (pSpell->Id == SPELL_WING_BUFFET)
        {
            if (!pCaster || pCaster->GetTypeId() != TYPEID_PLAYER)
                return;
            if (m_creature->getThreatManager().getThreat(pCaster))
                m_creature->getThreatManager().modifyThreatPercent(pCaster, -80);
        }
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        // Shadow Flame Timer
        if (m_uiShadowFlameTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_SHADOW_FLAME) == CAST_OK)
            {
                DoScriptText(EMOTE_GENERIC_SHADOW_FLAME, m_creature);
                m_uiShadowFlameTimer = 15000;
                if (m_uiWingBuffetTimer < 5000)
                    m_uiWingBuffetTimer = 5000;
            }
        }
        else
            m_uiShadowFlameTimer -= uiDiff;

        // Wing Buffet Timer
        if (m_uiWingBuffetTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_WING_BUFFET) == CAST_OK)
            {
                DoScriptText(EMOTE_GENERIC_WING_BUFFET, m_creature);
                m_uiWingBuffetTimer = 30000;
                if (m_uiShadowFlameTimer < 5000)
                    m_uiShadowFlameTimer = 5000;
            }
        }
        else
            m_uiWingBuffetTimer -= uiDiff;

        // Flame Buffet Timer
        if (m_uiFlameBuffetTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_FLAME_BUFFET) == CAST_OK)
                m_uiFlameBuffetTimer = 4500;
        }
        else
            m_uiFlameBuffetTimer -= uiDiff;

        if (m_uiRiftTimer < uiDiff)
        {
            if (Unit* rtarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, nullptr, SELECT_FLAG_PLAYER))
            {
                if (!rtarget->HasAura(35207) && rtarget->GetDistance(m_creature) < 100.0f)
                {
                    rtarget->CastSpell(rtarget, SPELL_RIFT, true);
                    m_creature->SummonCreature(NPC_RIFT, rtarget->GetPositionX(), rtarget->GetPositionY(), rtarget->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 120000);
                    m_uiRiftTimer = urand(43000, 47000);

                    if (m_uiShadowFlameTimer < 5000)
                        m_uiShadowFlameTimer = 5000;
                    if (m_uiWingBuffetTimer < 5000)
                        m_uiWingBuffetTimer = 5000;
                }
            }
        }
        else
            m_uiRiftTimer -= uiDiff;

        if (m_creature->isAttackReady() && !urand(0, 2))
            DoCastSpellIfCan(m_creature, SPELL_THRASH);

        DoMeleeAttackIfReady();
    }
};
CreatureAI* GetAI_boss_firemaw(Creature* pCreature)
{
    return new boss_firemawAI(pCreature);
}

void AddSC_boss_firemaw()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "boss_firemaw";
    newscript->GetAI = &GetAI_boss_firemaw;
    newscript->RegisterSelf();
}
