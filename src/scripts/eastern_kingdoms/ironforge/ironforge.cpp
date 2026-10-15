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
SDName: Ironforge
SD%Complete: 100
SDComment: Quest support: 3702
SDCategory: Ironforge
EndScriptData */

/* ContentData
npc_royal_historian_archesonus
EndContentData */

#include "scriptPCH.h"

/*######
## npc_royal_historian_archesonus
######*/

#define GOSSIP_ITEM_ROYAL   "I am ready to listen"
#define GOSSIP_ITEM_ROYAL_1 "That is tragic. How did this happen?"
#define GOSSIP_ITEM_ROYAL_2 "Interesting, continue please."
#define GOSSIP_ITEM_ROYAL_3 "Unbelievable! How dare they??"
#define GOSSIP_ITEM_ROYAL_4 "Of course I will help!"

bool GossipHello_npc_royal_historian_archesonus(Player* pPlayer, Creature* pCreature)
{
    if (pCreature->isQuestGiver())
        pPlayer->PrepareQuestMenu(pCreature->GetGUID());

    if (pPlayer->GetQuestStatus(3702) == QUEST_STATUS_INCOMPLETE)
    {
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_ROYAL, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
        pPlayer->SEND_GOSSIP_MENU(2235, pCreature->GetGUID());
    }
    else
        pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetGUID());

    return true;
}

bool GossipSelect_npc_royal_historian_archesonus(Player* pPlayer, Creature* pCreature, uint32 uiSender, uint32 uiAction)
{
    switch (uiAction)
    {
        case GOSSIP_ACTION_INFO_DEF:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_ROYAL_1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->SEND_GOSSIP_MENU(2236, pCreature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+1:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_ROYAL_2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            pPlayer->SEND_GOSSIP_MENU(2237, pCreature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+2:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_ROYAL_3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
            pPlayer->SEND_GOSSIP_MENU(2238, pCreature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+3:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_ROYAL_4, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
            pPlayer->SEND_GOSSIP_MENU(2239, pCreature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+4:
            pPlayer->CLOSE_GOSSIP_MENU();
            pPlayer->AreaExploredOrEventHappens(3702);
            break;
    }
    return true;
}


/*######
## npc_magni_bronzebeard
######*/

bool GossipHello_npc_magni_bronzebeard(Player* pPlayer, Creature* pCreature)
{
    if (pCreature->isQuestGiver())
        pPlayer->PrepareQuestMenu(pCreature->GetGUID());
    pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetGUID());

    return true;
}

/*######
## npc_tinker_mekkatorque
######*/

bool GossipHello_npc_tinker_mekkatorque(Player* pPlayer, Creature* pCreature)
{
    if (pCreature->isQuestGiver())
        pPlayer->PrepareQuestMenu(pCreature->GetGUID());
    pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetGUID());

    return true;
}

enum eSpells
{
    //Magni
    SPELL_STORM_BOLT    = 20685,
    SPELL_KNOCK_AWAY    = 20686,
    SPELL_AVATAR        = 19135,
    SPELL_THUNDER_CLAP  = 23931,
    SPELL_LAY_ON_HAND   = 17233,
    SPELL_CHARGE        = 22911,

    //Mechgineer
    SPELL_SHRINK_RAY    = 22742,
    SPELL_SHIELD        = 34749,
    SPELL_REFLECT       = 34750,
    SPELL_DEATH_RAY     = 34751,

};

struct boss_magni_bronzebeardAI : public ScriptedAI
{
public:
    boss_magni_bronzebeardAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    void Reset() override
    {
        m_uiStormBoltTimer = 10000;
        m_uiKnockAwayTimer = 20000;
        m_uiAvatarTimer= 20000;
        m_uiThunderClapTimer = 15000;
        m_uiChargeTimer = 10000;
        m_bHasUsedLOH = false;
        m_bHasStormBolted = false;
    }

    void JustRespawned() override
    {
        //V+ 5d buff for Racial Leaders
        if (m_creature->IsRacialLeader())
            m_creature->CastSpell(m_creature, 34746, true),
            m_creature->CastSpell(m_creature, 34741, true);
    }

    void SpellHitTarget(Unit* pTarget, const SpellEntry* pSpell) override
    {
        if (pSpell->Id == SPELL_KNOCK_AWAY)
            if (m_creature->getThreatManager().getThreat(pTarget))
                m_creature->getThreatManager().modifyThreatPercent(pTarget, -25);
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_creature->GetHealthPercent() < 50.0f && !m_bHasUsedLOH)
        {
            DoCastSpellIfCan(m_creature, SPELL_LAY_ON_HAND);
            m_bHasUsedLOH = true;
        }

        if (m_uiStormBoltTimer <= uiDiff)
        {
            if (m_bHasStormBolted)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_STORM_BOLT) == CAST_OK)
                {
                    m_bHasStormBolted = false;
                    m_uiStormBoltTimer = 10000;
                }
            }
            else if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 1))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_STORM_BOLT) == CAST_OK)
                {
                    m_uiStormBoltTimer = 0;
                    m_bHasStormBolted = true;
                }
            }
        }
        else
            m_uiStormBoltTimer -= uiDiff;

        if (m_uiKnockAwayTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_KNOCK_AWAY) == CAST_OK)
                m_uiKnockAwayTimer = 25000;
        }
        else
            m_uiKnockAwayTimer -= uiDiff;

        if (m_uiAvatarTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_AVATAR) == CAST_OK)
                m_uiAvatarTimer = 25000;
        }
        else
            m_uiAvatarTimer -= uiDiff;

        if (m_uiThunderClapTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_THUNDER_CLAP) == CAST_OK)
                m_uiThunderClapTimer = 20000;
        }
        else
            m_uiThunderClapTimer -= uiDiff;

        if (m_uiChargeTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_CHARGE) == CAST_OK)
                m_uiChargeTimer = 5000;
        }
        else
            m_uiChargeTimer -= uiDiff;

        DoMeleeAttackIfReady();
        EnterEvadeIfOutOfCombatArea(uiDiff);
    }

private:
    uint32 m_uiStormBoltTimer;
    uint32 m_uiKnockAwayTimer;
    uint32 m_uiAvatarTimer;
    uint32 m_uiThunderClapTimer;
    uint32 m_uiChargeTimer;
    bool m_bHasUsedLOH;
    bool m_bHasStormBolted;
};

CreatureAI* GetAI_boss_magni_bronzebeard(Creature* pCreature)
{
    return new boss_magni_bronzebeardAI(pCreature);
}




struct npc_tinker_mekkatorqueAI : public ScriptedAI
{
public:
    npc_tinker_mekkatorqueAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    void Reset() override
    {
        m_uiShrinkRayTimer = 10000;
        m_uiShieldTimer = urand(12000, 15000);
        m_uiShieldReflection = urand(15000, 20000);
        m_uiDeathRay = urand(1000, 5000);
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        //Immune Disarm
        if (!m_creature->HasAura(7219))
            DoCastSpellIfCan(m_creature, 7219);


        if (m_uiShrinkRayTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0), SPELL_SHRINK_RAY) == CAST_OK)
                m_uiShrinkRayTimer = urand(5000, 8000);
        }
        else
            m_uiShrinkRayTimer -= uiDiff;

        if (m_uiShieldTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_SHIELD) == CAST_OK)
                m_uiShieldTimer = urand(12000, 18000);
        }
        else
            m_uiShieldTimer -= uiDiff;

        if (m_uiShieldReflection <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_REFLECT) == CAST_OK)
                m_uiShieldReflection = urand(20000, 28000);
        }
        else
            m_uiShieldReflection -= uiDiff;

        if (m_uiDeathRay <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_DEATH_RAY) == CAST_OK)
                m_uiDeathRay = urand(10000, 18000);
        }
        else
            m_uiDeathRay -= uiDiff;

        DoMeleeAttackIfReady();
        EnterEvadeIfOutOfCombatArea(uiDiff);
    }

private:
    uint32 m_uiShrinkRayTimer;
    uint32 m_uiShieldTimer;
    uint32 m_uiShieldReflection;
    uint32 m_uiDeathRay;
};

CreatureAI* GetAI_npc_tinker_mekkatorque(Creature* pCreature)
{
    return new npc_tinker_mekkatorqueAI(pCreature);
}

void AddSC_ironforge()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "npc_royal_historian_archesonus";
    newscript->pGossipHello =  &GossipHello_npc_royal_historian_archesonus;
    newscript->pGossipSelect = &GossipSelect_npc_royal_historian_archesonus;
    newscript->RegisterSelf();
    /*
    newscript = new Script;
    newscript->Name = "npc_magni_bronzebeard";
    newscript->pGossipHello   = &GossipHello_npc_magni_bronzebeard;
    newscript->RegisterSelf();
    */
    newscript = new Script;
    newscript->Name = "npc_tinker_mekkatorque";
    /*newscript->pGossipHello   = &GossipHello_npc_tinker_mekkatorque;*/
    newscript->GetAI = &GetAI_npc_tinker_mekkatorque;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "boss_magni_bronzebeard";
    newscript->GetAI = &GetAI_boss_magni_bronzebeard;
    newscript->RegisterSelf();
}
