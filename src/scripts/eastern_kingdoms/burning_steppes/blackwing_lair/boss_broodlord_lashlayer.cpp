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
SDName: Boss_Broodlord_Lashlayer
SD%Complete: 100
SDComment:
SDCategory: Blackwing Lair
EndScriptData */

#include "scriptPCH.h"
#include "blackwing_lair.h"

enum
{
    SAY_AGGRO                   = -1469000,
    SAY_LEASH                   = -1469001,

    SPELL_CLEAVE                = 15284, //26350
    SPELL_BLAST_WAVE            = 23331,
    SPELL_MORTAL_STRIKE         = 24573,
    SPELL_KNOCK_AWAY            = 18670 //25778
};

struct boss_broodlordAI : public ScriptedAI
{
    boss_broodlordAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        m_bMobsDesactives = false;
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 m_uiCleaveTimer;
    uint32 m_uiBlastWaveTimer;
    uint32 m_uiMortalStrikeTimer;
    uint32 m_uiKnockAwayTimer;
    uint32 m_uiInCombatTimer;
    uint32 enrage_cast_timer;
    uint32 enrage_spell_id;
    bool m_bMobsDesactives;
    bool Enrage;

    void Reset()
    {
        m_uiCleaveTimer         = 8000;                     // These times are probably wrong
        m_uiBlastWaveTimer      = 20000;
        m_uiMortalStrikeTimer   = 25000;
        m_uiKnockAwayTimer      = urand(20000, 25000);
        m_uiInCombatTimer       = 2000;
        enrage_cast_timer       = 20000;
        Enrage = false;
    }

    void Aggro(Unit* /*pWho*/)
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_LASHLAYER, IN_PROGRESS);

        if (m_pInstance->GetData(TYPE_VAELASTRASZ) != DONE)
        {
            EnterEvadeMode();
            return;
        }

        if (m_creature->isAlive())
        {
            DoScriptText(SAY_AGGRO, m_creature);
            m_creature->SetInCombatWithZone();
        }
    }

    void JustDied(Unit* /*pKiller*/)
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_LASHLAYER, DONE);
    }

    void JustReachedHome()
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_LASHLAYER, FAIL);
    }

    void MoveInLineOfSight(Unit* who)
    {
        if (who->GetTypeId() == TYPEID_PLAYER && m_creature->IsWithinDistInMap(who, 40.0f) && m_creature->IsWithinLOSInMap(who) && !m_creature->isInCombat()
                && who->isInAccessablePlaceFor(m_creature) && !who->HasStealthAura())
            m_creature->SetInCombatWithZone();
    }

    void SpellHitTarget(Unit* pCaster, const SpellEntry* pSpell)
    {
        if (pSpell->Id == SPELL_KNOCK_AWAY)
            m_creature->getThreatManager().modifyThreatPercent(pCaster, -50);
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiInCombatTimer < uiDiff)
        {
            m_creature->SetInCombatWithZone();
            m_uiInCombatTimer = 2000;
        }
        else
            m_uiInCombatTimer -= uiDiff;

        if (!Enrage)
        {
            if (m_creature->GetHealthPercent() < 30.0f)
                Enrage = true;
        }
        else
        {
            if (enrage_cast_timer < uiDiff)
            {
                {
                    switch (urand(1, 6))
                    {
                        case 1:
                            enrage_spell_id = 35184;
                            break;
                        case 2:
                            enrage_spell_id = 35185;
                            break;
                        case 3:
                            enrage_spell_id = 35186;
                            break;
                        case 4:
                            enrage_spell_id = 35187;
                            break;
                        case 5:
                            enrage_spell_id = 35188;
                            break;
                        case 6:
                            enrage_spell_id = 35189;
                            break;
                    }
                    if (enrage_spell_id)
                        m_creature->CastSpell(m_creature, enrage_spell_id, true);
                }
                enrage_cast_timer = 20000;
            }
            else
                enrage_cast_timer -= uiDiff;
        }

        // Cleave Timer
        if (m_uiCleaveTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_CLEAVE) == CAST_OK)
                m_uiCleaveTimer = urand(9000, 15000);
        }
        else
            m_uiCleaveTimer -= uiDiff;

        // Blast Wave
        if (m_uiBlastWaveTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_BLAST_WAVE) == CAST_OK)
                m_uiBlastWaveTimer = urand(20000, 25000);
        }
        else
            m_uiBlastWaveTimer -= uiDiff;

        // Mortal Strike Timer
        if (m_uiMortalStrikeTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_MORTAL_STRIKE) == CAST_OK)
                m_uiMortalStrikeTimer = 10000;
        }
        else
            m_uiMortalStrikeTimer -= uiDiff;

        // Drop aggro Timer
        if (m_uiKnockAwayTimer < uiDiff)
        {
            m_creature->getThreatManager().modifyThreatPercent(m_creature->getVictim(), -50);
            m_uiKnockAwayTimer = urand(8000, 15000);
            /*if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_KNOCK_AWAY) == CAST_OK)*/
        }
        else
            m_uiKnockAwayTimer -= uiDiff;

        DoMeleeAttackIfReady();

        if (EnterEvadeIfOutOfCombatArea(uiDiff))
            DoScriptText(SAY_LEASH, m_creature);
    }
};

CreatureAI* GetAI_boss_broodlord(Creature* pCreature)
{
    return new boss_broodlordAI(pCreature);
}

void AddSC_boss_broodlord()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "boss_broodlord";
    pNewScript->GetAI = &GetAI_boss_broodlord;
    pNewScript->RegisterSelf();
}
