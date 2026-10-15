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
SDName: Thunder_Bluff
SD%Complete: 100
SDComment: Quest support: 925
SDCategory: Thunder Bluff
EndScriptData */

#include "scriptPCH.h"

/*#####
# npc_cairne_bloodhoof
######*/


enum eSpells
{
    SPELL_CLEAVE        = 15284,
    SPELL_MORTAL_STRIKE = 16856,
    SPELL_THUNDERCLAP   = 23931,
    SPELL_UPPERCUT      = 22916,
    SPELL_CHARGE        = 22911,
    SPELL_WARSTOMP      = 16727,
    SPELL_FEAR          = 19134,
};

struct boss_cairne_bloodhoofAI : public ScriptedAI
{
public:
    boss_cairne_bloodhoofAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    void Reset() override
    {
        m_uiCleaveTimer = 10000;
        m_uiMortalStrikeTimer = 15000;
        m_uiUppercutTimer = 5000;
        m_uiChargeTimer = 20000;
        m_uiWarstompTimer = 0;
        m_ui_EvadeTimer = urand(10000, 15000);
        m_uiFear = urand(10000, 15000);
    }
    
    void JustRespawned() override
    {
        //V+ 5d buff for Racial Leaders
        if (m_creature->IsRacialLeader())
            m_creature->CastSpell(m_creature, 34746, true),
            m_creature->CastSpell(m_creature, 34741, true);
    }

    void SpellHitTarget(Unit* /*pTarget*/, const SpellEntry* pSpell) override
    {
        if (pSpell->Id == SPELL_CHARGE)
            m_uiWarstompTimer = 500;
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_ui_EvadeTimer <= uiDiff)
        {
            if (m_creature->GetPositionY() < -135.0f)
                EnterEvadeMode();
            m_ui_EvadeTimer = 10000;
        }
        else
            m_ui_EvadeTimer -= uiDiff;

        //Immune Disarm
        if (!m_creature->HasAura(7219))
            DoCastSpellIfCan(m_creature, 7219);

        if (m_uiFear <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_FEAR) == CAST_OK)
                DoResetThreat(),
                m_uiFear = urand(30000, 40000);
        }
        else
            m_uiFear -= uiDiff;

        if (m_uiCleaveTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_CLEAVE) == CAST_OK)
                m_uiCleaveTimer = 10000;
        }
        else
            m_uiCleaveTimer -= uiDiff;

        if (m_uiMortalStrikeTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_MORTAL_STRIKE) == CAST_OK)
                m_uiMortalStrikeTimer = urand(10000, 15000);
        }
        else
            m_uiMortalStrikeTimer -= uiDiff;

        if (m_uiUppercutTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_UPPERCUT) == CAST_OK)
                m_uiUppercutTimer = urand(10000, 15000);
        }
        else
            m_uiUppercutTimer -= uiDiff;

        if (m_uiChargeTimer <= uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 1))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_CHARGE) == CAST_OK)
                    DoResetThreat(),
                    m_uiChargeTimer = urand(15000, 25000);
            }
        }
        else
            m_uiChargeTimer -= uiDiff;

        if (m_uiWarstompTimer)
        {
            if (m_uiWarstompTimer <= uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_WARSTOMP) == CAST_OK)
                    m_uiWarstompTimer = 0;
            }
            else
                m_uiWarstompTimer -= uiDiff;
        }

        DoMeleeAttackIfReady();
        EnterEvadeIfOutOfCombatArea(uiDiff);
    }

private:
    uint32 m_uiCleaveTimer;
    uint32 m_uiMortalStrikeTimer;
    uint32 m_uiUppercutTimer;
    uint32 m_uiChargeTimer;
    uint32 m_uiWarstompTimer;
    uint32 m_ui_EvadeTimer;
    uint32 m_uiFear;
};

CreatureAI* GetAI_boss_cairne_bloodhoof(Creature* pCreature)
{
    return new boss_cairne_bloodhoofAI(pCreature);
}

bool GossipHello_npc_cairne_bloodhoof(Player* pPlayer, Creature* pCreature)
{
    if (pCreature->isQuestGiver())
        pPlayer->PrepareQuestMenu(pCreature->GetGUID());
    if (pPlayer->GetQuestStatus(925) == QUEST_STATUS_INCOMPLETE)
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "I know this is rather silly but a young ward who is a bit shy would like your hoofprint.", GOSSIP_SENDER_MAIN, GOSSIP_SENDER_INFO);
    pPlayer->SEND_GOSSIP_MENU(7013, pCreature->GetGUID());

    return true;
}

bool GossipSelect_npc_cairne_bloodhoof(Player* pPlayer, Creature* pCreature, uint32 uiSender, uint32 uiAction)
{
    switch (uiAction)
    {
        case GOSSIP_SENDER_INFO:
            pPlayer->CastSpell(pPlayer, 23123, false);
            pPlayer->SEND_GOSSIP_MENU(7014, pCreature->GetGUID());
            break;
    }
    return true;
}

void AddSC_thunder_bluff()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "npc_cairne_bloodhoof";
    newscript->GetAI = &GetAI_boss_cairne_bloodhoof;
    newscript->pGossipHello = &GossipHello_npc_cairne_bloodhoof;
    newscript->pGossipSelect = &GossipSelect_npc_cairne_bloodhoof;
    newscript->RegisterSelf();
}
