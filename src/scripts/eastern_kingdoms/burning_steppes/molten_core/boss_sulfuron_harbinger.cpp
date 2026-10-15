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
SDName: Boss_Sulfuron_Harbringer
SD%Complete: 80
SDComment: Adds NYI
SDCategory: Molten Core
EndScriptData */

/*
    Sulfuron should walk to a random point ~10 yds behind him before casting Flamespear
*/

#include "scriptPCH.h"
#include "molten_core.h"

//Sulfuron
#define SPELL_DARKSTRIKE            19777
#define SPELL_DEMORALIZINGSHOUT     19778
#define SPELL_INSPIRE               19779
#define SPELL_KNOCKDOWN             27758  /*19780*/
#define SPELL_FLAMESPEAR            19781
#define SPELL_CHARGE                34474
#define SPELL_DRAINLIFE             34479
#define SPELL_BATTLE_STANCE         34475
#define SPELL_BERSERKER_STANCE      34476
#define SPELL_DEFENSIVE_STANCE      34477
#define SPELL_SUNDER_ARMOR          27991
#define SPELL_UNBALANCING           26613
#define SPELL_REND                  16509
#define SPELL_RETALIATION           22857
#define SPELL_RECKLESSNESS          13847
#define SPELL_SHIELDWALL            29061

//Flamewaker Priest
#define ADD_SPELL_DARKMENDING       19775
#define ADD_SPELL_SHADOWWORDPAIN    19776
#define ADD_SPELL_IMMOLATE          20294
#define ADD_SPELL_SHADOWSHIELD      34788
#define ADD_SPELL_SHADOWNOVA        34789
#define ADD_SPELL_SHADOWNOVA_HEAL   34790

struct boss_sulfuronAI : public ScriptedAI
{
    boss_sulfuronAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    uint32 Darkstrike_Timer;
    uint32 DemoralizingShout_Timer;
    uint32 Inspire_Timer;
    uint32 Knockdown_Timer;
    uint32 Flamespear_Timer;
	uint32 Charge_Timer;
	uint32 DrainL_Timer;
    uint32 Stance_Timer;
    uint32 Sunder_Timer;
    uint32 Unbal_Timer;
    uint32 Rend_Timer;
    uint32 m_uiLinkCheckTimer;
    bool perc_50;
    bool perc_20;
    ScriptedInstance* m_pInstance;

    void Reset()
    {
        Darkstrike_Timer        = 10000;                     //These times are probably wrong
        DemoralizingShout_Timer = 15000;
        Inspire_Timer           = 13000;
        Knockdown_Timer         = 6000;
        Flamespear_Timer        = 5000;
		Charge_Timer            = urand(12000, 18000);
		DrainL_Timer            = urand(12000, 18000);
        Stance_Timer            = 1000;
        Sunder_Timer            = urand(8000, 12000);
        Unbal_Timer             = urand(8000, 12000);
        Rend_Timer              = urand(8000, 1200);
        m_uiLinkCheckTimer      = 2500;
        perc_50                 = false;
        perc_20                 = false;

        if (m_pInstance && m_creature->isAlive())
            m_pInstance->SetData(TYPE_SULFURON, NOT_STARTED);
    }

    void JustReachedHome()
    {
        m_creature->RespawnNearCreaturesByEntry(11662, 200.0f);
    }

    void Aggro(Unit* pWho)
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_SULFURON, IN_PROGRESS);
        m_creature->SetInCombatWithZone();
        m_creature->CallForHelp(VISIBLE_RANGE);
    }

    void JustDied(Unit* pKiller)
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_SULFURON, DONE);
    }

    void UpdateAI(const uint32 diff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (!perc_50)
        {
            if (m_creature->GetHealthPercent() < 50.0f)
            {
                if (m_creature->HasAura(SPELL_BATTLE_STANCE))
                    m_creature->CastSpell(m_creature, SPELL_RETALIATION, true);
                else if (m_creature->HasAura(SPELL_BERSERKER_STANCE))
                    m_creature->CastSpell(m_creature, SPELL_RECKLESSNESS, true);
                else if (m_creature->HasAura(SPELL_DEFENSIVE_STANCE))
                    m_creature->CastSpell(m_creature, SPELL_SHIELDWALL, true);

                perc_50 = true;
            }
        }

        if (!perc_20)
        {
            if (m_creature->GetHealthPercent() < 20.0f)
            {
                if (m_creature->HasAura(SPELL_BATTLE_STANCE))
                    m_creature->CastSpell(m_creature, SPELL_RETALIATION, true);
                else if (m_creature->HasAura(SPELL_BERSERKER_STANCE))
                    m_creature->CastSpell(m_creature, SPELL_RECKLESSNESS, true);
                else if (m_creature->HasAura(SPELL_DEFENSIVE_STANCE))
                    m_creature->CastSpell(m_creature, SPELL_SHIELDWALL, true);

                perc_20 = true;
            }
        }


        if (Stance_Timer < diff)
        {
            uint32 Stance_spell;
            uint32 roll = urand(0, 2);
            if (roll == 0)
                Stance_spell = SPELL_BATTLE_STANCE;
            else if (roll == 1)
                Stance_spell = SPELL_BERSERKER_STANCE;
            else
                Stance_spell = SPELL_DEFENSIVE_STANCE;

            if (m_creature->HasAura(Stance_spell))
                return;

            if (Stance_spell == SPELL_BATTLE_STANCE)
                Unbal_Timer = urand(5000, 8000),
                Rend_Timer = urand(1000, 3000);
            if (Stance_spell == SPELL_BERSERKER_STANCE)
                Charge_Timer = urand(5000, 8000);
            if (Stance_spell == SPELL_DEFENSIVE_STANCE)
                Knockdown_Timer = 5000,
                DemoralizingShout_Timer = 12000,
                Sunder_Timer = urand(1000, 3000);

            if (DoCastSpellIfCan(m_creature, Stance_spell) == CAST_OK)
                Stance_Timer = 59500;
        }
        else Stance_Timer -= diff;

        //
        //Battle SPELLS
        //
        //Unbalance Strike
        if (Unbal_Timer < diff)
        {
            if (m_creature->HasAura(SPELL_BATTLE_STANCE))
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_UNBALANCING) == CAST_OK)
                {
                    Unbal_Timer = urand(10000, 15000);
                    if (Darkstrike_Timer < 3000)
                        Darkstrike_Timer += 3000;
                    if (DrainL_Timer < 3000)
                        DrainL_Timer += 3000;
                }
        }
        else Unbal_Timer -= diff;

        if (Rend_Timer < diff)
        {
            if (m_creature->HasAura(SPELL_BATTLE_STANCE))
                if (Unit* target = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_REND, SELECT_FLAG_IN_MELEE_RANGE))
                {
                    if (!target->HasAura(SPELL_REND))
                    {
                        if (DoCastSpellIfCan(target, SPELL_REND) == CAST_OK)
                            Rend_Timer = urand(3000, 5000);
                    }
                }
        }
        else Rend_Timer -= diff;

        //
        //Berserker SPELLS
        //
        //Charge
		if (Charge_Timer < diff)
		{
            if (m_creature->HasAura(SPELL_BERSERKER_STANCE))
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_FARTHEST, 0, SPELL_CHARGE, SELECT_FLAG_PLAYER | SELECT_FLAG_IN_LOS))
                    if (DoCastSpellIfCan(pTarget, SPELL_CHARGE) == CAST_OK)
                    {
                        DoResetThreat();
                        m_creature->getThreatManager().addThreatDirectly(pTarget, urand(2000, 3000));
                        Charge_Timer = urand(25000, 30000);
                    }
		}
		else Charge_Timer -= diff;

        //
        //DEFENSIVE SPELLS
        //
        //DemoralizingShout_Timer
        if (DemoralizingShout_Timer < diff)
        {
            if (m_creature->HasAura(SPELL_DEFENSIVE_STANCE))
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_DEMORALIZINGSHOUT) == CAST_OK)
                    DemoralizingShout_Timer = urand(15000, 30000);
        }
        else DemoralizingShout_Timer -= diff;

        //Sunder_Timer
        if (Sunder_Timer < diff)
        {
            if (m_creature->HasAura(SPELL_DEFENSIVE_STANCE))
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_SUNDER_ARMOR) == CAST_OK)
                    Sunder_Timer = urand(5000, 8000);
        }
        else Sunder_Timer -= diff;

        //Knockdown_Timer
        if (Knockdown_Timer < diff)
        {
            if (m_creature->HasAura(SPELL_DEFENSIVE_STANCE))
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_KNOCKDOWN) == CAST_OK)
                    Knockdown_Timer = urand(20000, 25000);
        }
        else Knockdown_Timer -= diff;





        //DrainL_Timer
        if (DrainL_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_DRAINLIFE) == CAST_OK)
                DrainL_Timer = urand(8000, 12000);
        }
        else DrainL_Timer -= diff;

        //Inspire_Timer
        if (Inspire_Timer < diff)
        {
            Creature* target = NULL;
            std::list<Creature*> pList = DoFindFriendlyMissingBuff(45.0f, SPELL_INSPIRE);
            if (!pList.empty())
            {
                std::list<Creature*>::iterator i = pList.begin();
                advance(i, (rand() % pList.size()));
                target = (*i);
            }

            if (target)
            {
                if (DoCastSpellIfCan(target, SPELL_INSPIRE) != CAST_OK)
                    return;
            }

            if (DoCastSpellIfCan(m_creature, SPELL_INSPIRE) == CAST_OK)
                Inspire_Timer = urand(25000, 35000);
        }
        else Inspire_Timer -= diff;

        //Flamespear_Timer
        if (Flamespear_Timer < diff)
        {
            if (Unit* target = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
            {
                if (DoCastSpellIfCan(target, SPELL_FLAMESPEAR) == CAST_OK)
                    Flamespear_Timer = urand(8000, 12000);
            }
        }
        else Flamespear_Timer -= diff;

        //DarkStrike_Timer
        if (Darkstrike_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_DARKSTRIKE) == CAST_OK)
                Darkstrike_Timer = urand(8000, 12000);
        }
        else Darkstrike_Timer -= diff;

        // Prevent splitting Sulfuron and his adds
        if (m_uiLinkCheckTimer < diff)
        {
            std::list<Creature*> AddsList;
            GetCreatureListWithEntryInGrid(AddsList, m_creature, 11662, 200.0f);

            for (std::list<Creature*>::iterator it = AddsList.begin(); it != AddsList.end(); ++it)
            {
                if ((*it)->isAlive() && ((*it)->GetDistance2d(m_creature->GetPositionX(), m_creature->GetPositionY()) > 100.0f || !(*it)->IsWithinLOS(m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ())))
                {
                    ThreatList const& tList = (*it)->getThreatManager().getThreatList();
                    for (ThreatList::const_iterator itr = tList.begin(); itr != tList.end(); ++itr)
                    {
                        Unit* pUnit = (*it)->GetMap()->GetUnit((*itr)->getUnitGuid());

                        if (pUnit && (*it)->getThreatManager().getThreat(pUnit))
                            (*it)->getThreatManager().modifyThreatPercent(pUnit, -100);
                    }

                    if (!(*it)->HasAura(22863))
                        (*it)->CastSpell((*it), 22863, true);
                    (*it)->getThreatManager().addThreatDirectly(m_creature->getVictim(), urand(2500, 3500));
                }

            }
            m_uiLinkCheckTimer = 3000;
        }
        else
            m_uiLinkCheckTimer -= diff;

        DoMeleeAttackIfReady();
    }
};


struct npc_flamewaker_priestAI : public ScriptedAI
{
    npc_flamewaker_priestAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }
    uint32 DarkMending_Timer;
    uint32 Shadow_Word_Pain_Timer;
    uint32 Darkstrike_Timer;
    uint32 Immolate_Timer;
    uint32 ShadowNova_Timer;
    bool perc_50;
    bool perc_20;
    bool doNova;
    ScriptedInstance* m_pInstance;

    void Reset()
    {
        DarkMending_Timer = urand(10000, 15000);
        Shadow_Word_Pain_Timer = urand(2000, 5000);
        Darkstrike_Timer = urand(6000, 8000);
        Immolate_Timer = urand(5000, 10000);
        ShadowNova_Timer = urand(15000, 20000);

        perc_50 = false;
        perc_20 = false;
        doNova  = false;
    }

    void DoNova()
    {
        std::list<Creature*> FlamewakerPriestList;
        GetCreatureListWithEntryInGrid(FlamewakerPriestList, m_creature, 11662, 20.0f);

        for (std::list<Creature*>::iterator itr = FlamewakerPriestList.begin(); itr != FlamewakerPriestList.end();)
        {
            if ((*itr)->getDeathState() != ALIVE)
                itr = FlamewakerPriestList.erase(itr);
            else
                ++itr;
        }

        if (FlamewakerPriestList.size() > 1)
            doNova = true;
        else
            doNova = false;


        if (doNova)
        {
            if (DoCastSpellIfCan(m_creature, ADD_SPELL_SHADOWNOVA) == CAST_OK)
                ShadowNova_Timer = urand(2000, 5000);
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        //Immolate_Timer
        if (Immolate_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), ADD_SPELL_IMMOLATE) == CAST_OK)
                Immolate_Timer = urand(15000, 20000);
        }
        else Immolate_Timer -= diff;

        //DarkStrike_Timer
        if (Darkstrike_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_DARKSTRIKE) == CAST_OK)
                Darkstrike_Timer = urand(10000, 15000);
        }
        else Darkstrike_Timer -= diff;

        //Shadow_Word_Pain_Timer
        if (Shadow_Word_Pain_Timer < diff)
        {
            if (Unit* target = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
            {
                if (!target->HasAura(ADD_SPELL_SHADOWWORDPAIN))
                    if (DoCastSpellIfCan(target, ADD_SPELL_SHADOWWORDPAIN) == CAST_OK)
                        Shadow_Word_Pain_Timer = urand(8000, 12000);
            }
        }
        else Shadow_Word_Pain_Timer -= diff;

        //ShadowShield %
        if (!perc_50)
        {
            if (m_creature->GetHealthPercent() < 50.0f)
                if (DoCastSpellIfCan(m_creature, ADD_SPELL_SHADOWSHIELD) == CAST_OK)
                    perc_50 = true;
        }
        if (!perc_20)
        {
            if (m_creature->GetHealthPercent() < 20.0f)
                if (DoCastSpellIfCan(m_creature, ADD_SPELL_SHADOWSHIELD) == CAST_OK)
                    perc_20 = true;
        }

        //Dark Mending
        if (DarkMending_Timer < diff)
        {
            if (Unit* target = m_creature->FindLowestHpFriendlyUnit(160.0f, 75))
                if (DoCastSpellIfCan(target, ADD_SPELL_DARKMENDING) == CAST_OK)
                    DarkMending_Timer = urand(8000, 10000);
        }
        else DarkMending_Timer -= diff;

        //Shadow Nova
        if (ShadowNova_Timer < diff)
        {
            DoNova();
            ShadowNova_Timer = 2000;
        }
        else
            ShadowNova_Timer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_sulfuron(Creature* pCreature)
{
    return new boss_sulfuronAI(pCreature);
}

CreatureAI* GetAI_npc_flamewaker_priest(Creature* pCreature)
{
    return new npc_flamewaker_priestAI(pCreature);
}

void AddSC_boss_sulfuron()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "boss_sulfuron";
    newscript->GetAI = &GetAI_boss_sulfuron;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_flamewaker_priest";
    newscript->GetAI = &GetAI_npc_flamewaker_priest;
    newscript->RegisterSelf();
}
