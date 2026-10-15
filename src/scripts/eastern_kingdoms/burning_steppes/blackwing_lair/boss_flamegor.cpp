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
SDName: Boss_Flamegor
SD%Complete: 90
SDComment: Thrash is missing
SDCategory: Blackwing Lair
EndScriptData */

#include "scriptPCH.h"
#include "blackwing_lair.h"

enum DrakeSpells
{
    EMOTE_GENERIC_WING_BUFFET   = -1469032,
    EMOTE_GENERIC_SHADOW_FLAME  = -1469033,
    EMOTE_GENERIC_FRENZY        = -1000002,

    SPELL_SHADOW_FLAME          = 22539,
    SPELL_WING_BUFFET           = 23339,
    SPELL_FRENZY_FIRE           = 23342,                    // This spell periodically triggers fire nova
    SPELL_FRENZY_SHADOW         = 35238,                    // This spell periodically triggers shadow nova
    SPELL_THRASH                = 3391,                     // TODO missing
    /*SPELL_SHADOW_OF_EBONROC     = 23340,*/
    SPELL_SHADOWS_OF_EBONROC    = 35361,
    SPELL_FLAMES_OF_FLAMEGOR    = 35360,
    SPELL_CD_1_SEC              = 35249,

    MARK_EMBRACE_OF_SHADOWS     = 35241,
    MARK_EMBRACE_OF_FLAMES      = 35242,
    SPELL_EMBRACE_OF_SHADOWS    = 35243,
    SPELL_EMBRACE_OF_FLAMES     = 35244,
};



struct boss_ebonrocAI : public ScriptedAI
{
    boss_ebonrocAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 m_uiShadowFlameTimer;
    uint32 m_uiWingBuffetTimer;
    uint32 m_uiShadowOfEbonrocTimer;

    void Reset()
    {
        m_uiShadowFlameTimer = urand(12000, 25000);
        m_uiWingBuffetTimer = m_uiShadowFlameTimer + 18000;
        m_uiShadowOfEbonrocTimer = 8000;
    }

    void Aggro(Unit* pWho)
    {
        if (!m_pInstance)
            return;

        if (m_pInstance->GetData(TYPE_LASHLAYER) != DONE)
        {
            EnterEvadeMode();
            return;
        }

        if (m_pInstance->GetData(TYPE_EBONROC_FLAMEGOR) == IN_PROGRESS)
            return;

        if (m_pInstance)
            m_pInstance->SetData(TYPE_EBONROC_FLAMEGOR, IN_PROGRESS);

        m_creature->SetInCombatWithZone();

        if (Creature* pTwin = m_creature->GetMap()->GetCreature(m_pInstance->GetData64(DATA_FLAMEGOR_GUID)))
        {
            // The "other boss" will start running to the same initial puller.
            pTwin->AI()->AttackStart(pWho);
            pTwin->SetInCombatWithZone();
        }
    }

    void JustDied(Unit* pKiller)
    {
        if (!m_pInstance)
            return;

        if (m_pInstance->GetData(TYPE_EBONROC_FLAMEGOR) == DONE)
            return;

        m_pInstance->SetData(TYPE_EBONROC_FLAMEGOR, DONE);


        if (Creature* pTwin = m_creature->GetMap()->GetCreature(m_pInstance->GetData64(DATA_FLAMEGOR_GUID)))
        {
            pKiller->Kill(pTwin, nullptr, false);
        }
    }

    void JustReachedHome()
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_EBONROC_FLAMEGOR, FAIL);
    }

    void DamageTaken(Unit* done_by, uint32& damage) override
    {
        if (!m_pInstance)
            return;

        if (Creature* pTwin = m_creature->GetMap()->GetCreature(m_pInstance->GetData64(DATA_FLAMEGOR_GUID)))
        {
            float fDamPercent = ((float)damage) / ((float)m_creature->GetMaxHealth());
            uint32 uiTwinDamage = (uint32)(fDamPercent * ((float)pTwin->GetMaxHealth()));
            uint32 uiTwinHealth = pTwin->GetHealth() - std::min(uiTwinDamage, pTwin->GetHealth());
            pTwin->SetHealth(std::max((uint32)0, uiTwinHealth));

            // Possibly needed to make sure the damage dealth through setHealth is counted 
            pTwin->CountDamageTaken(uiTwinDamage, true);
        }
    }

    void HealedBy(Unit* done_by, uint32& healed) override
    {
        if (!m_pInstance)
            return;

        if (Creature* pTwin = m_creature->GetMap()->GetCreature(m_pInstance->GetData64(DATA_FLAMEGOR_GUID)))
        {
            float fHealPercent = ((float)healed) / ((float)m_creature->GetMaxHealth());
            uint32 uiTwinHeal = (uint32)(fHealPercent * ((float)pTwin->GetMaxHealth()));
            uint32 uiTwinHealth = pTwin->GetHealth() + std::min(uiTwinHeal, pTwin->GetHealth());
            pTwin->SetHealth(std::max((uint32)0, uiTwinHealth));
        }
    }

    void SpellHitTarget(Unit* pTarget, const SpellEntry* pSpell)
    {
        if (pSpell->Id == SPELL_WING_BUFFET)
        {
            if (!pTarget || pTarget->GetTypeId() != TYPEID_PLAYER)
                return;
            if (m_creature->getThreatManager().getThreat(pTarget))
                m_creature->getThreatManager().modifyThreatPercent(pTarget, -50);
        }

        if (pSpell->Id == MARK_EMBRACE_OF_SHADOWS && pTarget)
        {
            SpellAuraHolder* holder = pTarget->GetSpellAuraHolder(MARK_EMBRACE_OF_SHADOWS);
            if (!holder || holder->GetStackAmount() <= 1)
                return;

            int32 damage;
            damage = 500 * (holder->GetStackAmount() - 1);

            m_creature->CastCustomSpell(pTarget, SPELL_EMBRACE_OF_SHADOWS, &damage, nullptr, nullptr, true, nullptr, holder->GetAuraByEffectIndex(EFFECT_INDEX_0), m_creature->GetObjectGuid(), pSpell);

            if (m_creature->getThreatManager().getThreat(pTarget))
                m_creature->getThreatManager().modifyThreatPercent(pTarget, -10);
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
                m_uiShadowFlameTimer = 20000;
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
                m_uiWingBuffetTimer = 20000;
            }
        }
        else
            m_uiWingBuffetTimer -= uiDiff;

        // Shadow of Ebonroc Timer
        if (m_uiShadowOfEbonrocTimer < uiDiff)
        {
            if (urand(0, 1) == 0)
            {
                if (Creature* FlamegorUnit = m_creature->GetMap()->GetCreature(m_pInstance->GetData64(DATA_FLAMEGOR_GUID)))
                {
                    if (Unit* sTarget = FlamegorUnit->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, urand(2, 9), nullptr, SELECT_FLAG_PLAYER))
                        if (!sTarget->HasAura(SPELL_FLAMES_OF_FLAMEGOR))
                            if (DoCastSpellIfCan(sTarget, SPELL_SHADOWS_OF_EBONROC, CF_AURA_NOT_PRESENT) == CAST_OK)
                            {
                                sTarget->AddAura(SPELL_CD_1_SEC);
                                m_uiShadowOfEbonrocTimer = urand(8000, 10000);
                            }
                }
            }
            else
            {
                if (Unit* sTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, urand(2, 9), nullptr, SELECT_FLAG_PLAYER))
                    if (!sTarget->HasAura(SPELL_FLAMES_OF_FLAMEGOR))
                        if (DoCastSpellIfCan(sTarget, SPELL_SHADOWS_OF_EBONROC, CF_AURA_NOT_PRESENT) == CAST_OK)
                        {
                            sTarget->AddAura(SPELL_CD_1_SEC);
                            m_uiShadowOfEbonrocTimer = urand(8000, 10000);
                        }
            }
        }
        else
            m_uiShadowOfEbonrocTimer -= uiDiff;

        if (m_creature->isAttackReady() && !urand(0, 2))
            DoCastSpellIfCan(m_creature, SPELL_THRASH);

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_ebonroc(Creature* pCreature)
{
    return new boss_ebonrocAI(pCreature);
}





struct boss_flamegorAI : public ScriptedAI
{
    boss_flamegorAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 m_uiShadowFlameTimer;
    uint32 m_uiWingBuffetTimer;
    uint32 m_uiFrenzyTimer;
    uint32 m_uiEmbracesTimer;
    uint32 m_uiFlamesOfFlamegorTimer;

    void Reset()
    {
        m_uiShadowFlameTimer = urand(12000, 25000);
        m_uiWingBuffetTimer = m_uiShadowFlameTimer + 18000;
        m_uiFrenzyTimer = urand(8000, 12000);
        m_uiEmbracesTimer = 20000;
        m_uiFlamesOfFlamegorTimer = 8000;
    }

    void Aggro(Unit* pWho)
    {
        if (m_pInstance->GetData(TYPE_LASHLAYER) != DONE)
        {
            EnterEvadeMode();
            return;
        }

        if (m_pInstance->GetData(TYPE_EBONROC_FLAMEGOR) == IN_PROGRESS)
            return;

        if (m_pInstance)
            m_pInstance->SetData(TYPE_EBONROC_FLAMEGOR, IN_PROGRESS);

        m_creature->SetInCombatWithZone();


        if (m_pInstance)
        {
            if (Creature* pTwin = m_creature->GetMap()->GetCreature(m_pInstance->GetData64(DATA_EBONROC_GUID)))
            {
                // The "other boss" will start running to the same initial puller.
                pTwin->AI()->AttackStart(pWho);
                pTwin->SetInCombatWithZone();
            }
        }
    }

    void JustDied(Unit* pKiller)
    {
        if (!m_pInstance)
            return;

        if (m_pInstance->GetData(TYPE_EBONROC_FLAMEGOR) == DONE)
            return;

        m_pInstance->SetData(TYPE_EBONROC_FLAMEGOR, DONE);


        if (Creature* pTwin = m_creature->GetMap()->GetCreature(m_pInstance->GetData64(DATA_EBONROC_GUID)))
        {
            pKiller->Kill(pTwin, nullptr, false);
        }
    }

    void JustReachedHome()
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_EBONROC_FLAMEGOR, FAIL);
    }

    void DamageTaken(Unit* done_by, uint32& damage) override
    {
        if (!m_pInstance)
            return;

        if (Creature* pTwin = m_creature->GetMap()->GetCreature(m_pInstance->GetData64(DATA_EBONROC_GUID)))
        {
            float fDamPercent = ((float)damage) / ((float)m_creature->GetMaxHealth());
            uint32 uiTwinDamage = (uint32)(fDamPercent * ((float)pTwin->GetMaxHealth()));
            uint32 uiTwinHealth = pTwin->GetHealth() - std::min(uiTwinDamage, pTwin->GetHealth());
            pTwin->SetHealth(std::max((uint32)0, uiTwinHealth));

            // Possibly needed to make sure the damage dealth through setHealth is counted 
            pTwin->CountDamageTaken(uiTwinDamage, true);
        }
    }

    void HealedBy(Unit* done_by, uint32& healed) override
    {
        if (!m_pInstance)
            return;

        if (Creature* pTwin = m_creature->GetMap()->GetCreature(m_pInstance->GetData64(DATA_EBONROC_GUID)))
        {
            float fHealPercent = ((float)healed) / ((float)m_creature->GetMaxHealth());
            uint32 uiTwinHeal = (uint32)(fHealPercent * ((float)pTwin->GetMaxHealth()));
            uint32 uiTwinHealth = pTwin->GetHealth() + std::min(uiTwinHeal, pTwin->GetHealth());
            pTwin->SetHealth(std::max((uint32)0, uiTwinHealth));
        }
    }

    void SpellHitTarget(Unit* pTarget, const SpellEntry* pSpell)
    {
        if (pSpell->Id == SPELL_WING_BUFFET)
        {
            if (!pTarget || pTarget->GetTypeId() != TYPEID_PLAYER)
                return;
            if (m_creature->getThreatManager().getThreat(pTarget))
                m_creature->getThreatManager().modifyThreatPercent(pTarget, -50);
        }

        if (pSpell->Id == MARK_EMBRACE_OF_FLAMES && pTarget)
        {
            SpellAuraHolder* holder = pTarget->GetSpellAuraHolder(MARK_EMBRACE_OF_FLAMES);
            if (!holder || holder->GetStackAmount() <= 1)
                return;

            int32 damage;
            damage = 500 * (holder->GetStackAmount() - 1);

            m_creature->CastCustomSpell(pTarget, SPELL_EMBRACE_OF_FLAMES, &damage, nullptr, nullptr, true, nullptr, holder->GetAuraByEffectIndex(EFFECT_INDEX_0), m_creature->GetObjectGuid(), pSpell);

            if (m_creature->getThreatManager().getThreat(pTarget))
                m_creature->getThreatManager().modifyThreatPercent(pTarget, -10);
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
                m_uiShadowFlameTimer = 20000;
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
                m_uiWingBuffetTimer = 20000;
            }
        }
        else
            m_uiWingBuffetTimer -= uiDiff;

        // Frenzy Timer
        if (m_uiFrenzyTimer < uiDiff)
        {
            if (urand(0, 1) == 0)
            {
                if (Unit* EbonrocUnit = m_creature->GetMap()->GetCreature(m_pInstance->GetData64(DATA_EBONROC_GUID)))
                {
                    EbonrocUnit->CastSpell(EbonrocUnit, SPELL_FRENZY_SHADOW, true);
                    DoScriptText(EMOTE_GENERIC_FRENZY, EbonrocUnit);
                    m_uiFrenzyTimer = urand(13000, 17000);
                }
            }
            else
            {
                if (DoCastSpellIfCan(m_creature, SPELL_FRENZY_FIRE) == CAST_OK)
                {
                    DoScriptText(EMOTE_GENERIC_FRENZY, m_creature);
                    m_uiFrenzyTimer = urand(13000, 17000);
                }
            }
        }
        else
            m_uiFrenzyTimer -= uiDiff;

        //Embraces
        if (m_uiEmbracesTimer < uiDiff)
        {
            if (Unit* EbonrocUnit = m_creature->GetMap()->GetCreature(m_pInstance->GetData64(DATA_EBONROC_GUID)))
                EbonrocUnit->CastSpell(EbonrocUnit, MARK_EMBRACE_OF_SHADOWS, true);

            m_creature->CastSpell(m_creature, MARK_EMBRACE_OF_FLAMES, true);

            m_uiEmbracesTimer = 15000;
        }
        else
            m_uiEmbracesTimer -= uiDiff;

        // Flames of Flamegor Timer
        if (m_uiFlamesOfFlamegorTimer < uiDiff)
        {
            if (urand(0, 1) == 0)
            {
                if (Creature* EbonrocUnit = m_creature->GetMap()->GetCreature(m_pInstance->GetData64(DATA_EBONROC_GUID)))
                {
                    if (Unit* sTarget = EbonrocUnit->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, urand(2, 9), nullptr, SELECT_FLAG_PLAYER))
                        if (!sTarget->HasAura(SPELL_SHADOWS_OF_EBONROC))
                            if (DoCastSpellIfCan(sTarget, SPELL_FLAMES_OF_FLAMEGOR, CF_AURA_NOT_PRESENT) == CAST_OK)
                            {
                                sTarget->AddAura(SPELL_CD_1_SEC);
                                m_uiFlamesOfFlamegorTimer = urand(8000, 10000);
                            }
                }
            }
            else
            {
                if (Unit* sTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, urand(2, 9), nullptr, SELECT_FLAG_PLAYER))
                    if (!sTarget->HasAura(SPELL_SHADOWS_OF_EBONROC))
                        if (DoCastSpellIfCan(sTarget, SPELL_FLAMES_OF_FLAMEGOR, CF_AURA_NOT_PRESENT) == CAST_OK)
                        {
                            sTarget->AddAura(SPELL_CD_1_SEC);
                            m_uiFlamesOfFlamegorTimer = urand(8000, 10000);
                        }
            }
        }
        else
            m_uiFlamesOfFlamegorTimer -= uiDiff;

        if (m_creature->isAttackReady() && !urand(0, 2))
            DoCastSpellIfCan(m_creature, SPELL_THRASH);

        DoMeleeAttackIfReady();
    }
};
CreatureAI* GetAI_boss_flamegor(Creature* pCreature)
{
    return new boss_flamegorAI(pCreature);
}




void AddSC_boss_flamegor()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "boss_ebonroc";
    newscript->GetAI = &GetAI_boss_ebonroc;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "boss_flamegor";
    newscript->GetAI = &GetAI_boss_flamegor;
    newscript->RegisterSelf();
}
