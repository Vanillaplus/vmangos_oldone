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
SDName: Boss_Hakkar
SD%Complete: 95
SDComment: Blood siphon spell buggy cause of Core Issue.
SDCategory: Zul'Gurub
EndScriptData */

#include "scriptPCH.h"
#include "zulgurub.h"

enum
{
    // Datas :
    // NPC_HAKKAR                  = 14834,

    // Emotes
    // ------

    SAY_AGGRO                   = -1309020,
    SAY_FLEEING                 = -1309021,
    SAY_MINION_DESTROY          = -1309022,                //where does it belong?
    SAY_PROTECT_ALTAR           = -1309023,                //where does it belong?
    EMOTE_FRENZY_KILL           = -1000001,
    /*
    /run PlaySoundFile("SOUND\\CREATURE\\HAKKAR\\VO_ZG2_HAKKAR_DEFEAT_01.ogg");
    /run PlaySoundFile("SOUND\\CREATURE\\HAKKAR\\VO_ZG2_HAKKAR_DEFEAT_02.ogg");
    /run PlaySoundFile("SOUND\\CREATURE\\HAKKAR\\VO_ZG2_HAKKAR_EVENT_BREAK_CHAIN.ogg");
    /run PlaySoundFile("SOUND\\CREATURE\\HAKKAR\\VO_ZG2_HAKKAR_EVENT_START.ogg");
    */

    // Hakkar's spells
    // ---------------

    SPELL_BLOODSIPHON_STUN       = 24324, // Joueur assomme
    SPELL_BLOODSIPHON_DAMAGE     = 24323, // "Votre sang est empoisonne"
    SPELL_BLOODSIPHON_HEAL       = 24322, // "Inflige 200 points de dégâts par seconde. Donne à Hakkar 1000 points de vie par seconde." -> Rend de la vie aux joueurs ...
    SPELL_CORRUPTEDBLOOD         = 24328,
    SPELL_CAUSEINSANITY          = 24327,
    SPELL_WILLOFHAKKAR           = 24178,
    // SPELL_ENRAGE                 = 24318,
    SPELL_BERSERK                = 27680,
    SPELL_HYSTERIA               = 34956,
    SPELL_CURSE_OF_NEMESIS       = 34957,

    // Aspects of High Priests
    // -----------------------

    SPELL_ASPECT_OF_MARLI        = 24686, // étourdi 5 sec
    SPELL_ASPECT_OF_JEKLIK       = 24687, // silence 4 sec
    SPELL_ASPECT_OF_VENOXIS      = 24688, // poison
    SPELL_ASPECT_OF_THEKAL       = 24689, // enrage qqs sec
    SPELL_ASPECT_OF_ARLOKK       = 24690, // vanish qqs secs

    // Other spells
    // ------------
    SPELL_POISONOUS_BLOOD        = 24321,


    NPC_SON_OF_HAKKAR            = 11357
};

struct boss_hakkarAI : public ScriptedAI
{
    boss_hakkarAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 BloodSiphon_Timer;
    uint32 CorruptedBlood_Timer;
    uint32 CauseInsanity_Timer;
    uint32 CauseInsanity_Timer_3aggro;
    uint32 CauseInsanity_Timer_random;
    uint32 Berserk_Timer;
    uint32 Hysteria_Timer;
    uint32 Hysteria_Timer2;
    uint32 Curse_of_Nemesis_Timer;

    uint32 AspectOfJeklik_Timer;
    uint32 AspectOfVenoxis_Timer;
    uint32 AspectOfMarli_Timer;
    uint32 AspectOfThekal_Timer;
    uint32 AspectOfArlokk_Timer;
    uint32 m_uiExploit_Timer;
    uint32 pullCheckTimer;
    uint32 AzusInitialCheck;
    bool BoolAzusInitialCheck;

    bool Enraged;

    void Reset()
    {
        BloodSiphon_Timer = 90000;
        CorruptedBlood_Timer = 15000;
        CauseInsanity_Timer = 40000;
        CauseInsanity_Timer_3aggro = 35000;
        CauseInsanity_Timer_random = 30000;
        Berserk_Timer = 600000;
        Hysteria_Timer = urand(10000, 20000);
        Hysteria_Timer2 = 60000;
        Curse_of_Nemesis_Timer = urand(15000, 25000);

        AspectOfJeklik_Timer = 4000;
        AspectOfVenoxis_Timer = 7000;
        AspectOfMarli_Timer = 12000;
        AspectOfThekal_Timer = 8000;
        AspectOfArlokk_Timer = 18000;

        m_uiExploit_Timer = 10000;
        AzusInitialCheck = 5000;
        BoolAzusInitialCheck = false;

        Enraged = false;

        if (m_pInstance)
            m_pInstance->SetData(TYPE_HAKKAR, NOT_STARTED);
    }

    void Aggro(Unit *who)
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_HAKKAR, IN_PROGRESS);
        DoScriptText(SAY_AGGRO, m_creature);

        ScriptedAI::Aggro(who);
        m_creature->SetInCombatWithZone();

        // Respawn sons of Hakkar
        uint32 number_of_sons_progress = 0;
        uint32 number_of_sons_respawned = 0;
        std::list<Creature*> sons;
        GetCreatureListWithEntryInGrid(sons, m_creature, NPC_SON_OF_HAKKAR, DEFAULT_VISIBILITY_INSTANCE);
        if (!sons.empty())
        {
            for (auto iter = sons.cbegin(); iter != sons.cend(); ++iter)
            {
                auto sons = *iter;
                if (sons && !sons->isAlive())
                {
                    number_of_sons_progress += 1;
                    if (urand(0, 1) > 0)
                    {
                        number_of_sons_respawned += 1;
                        sons->Respawn();
                    }
                    else if (number_of_sons_progress > 12 && number_of_sons_respawned < 6)
                    {
                        number_of_sons_respawned += 1;
                        sons->Respawn();
                    }
                }
            }
        }
    }

    void JustDied(Unit* Killer)
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_HAKKAR, DONE);
    }

    void UpdateAI(const uint32 diff)
    {
        if (!m_pInstance || !m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        /** Prevent exploit */
        if (m_uiExploit_Timer < diff)
        {
            if (m_creature->GetPositionZ() > 57.28f || m_creature->GetPositionZ() < 45.0f)
            {
                EnterEvadeMode();
                return;
            }
            m_uiExploit_Timer = 5000;
        }
        else
            m_uiExploit_Timer -= diff;

        if (!BoolAzusInitialCheck)
        {
            if (AzusInitialCheck < diff)
            {
                //Azus alive check
                std::list<Creature*> Boss;
                GetCreatureListWithEntryInGrid(Boss, m_creature, 25031, 100.0f);
                for (std::list<Creature*>::iterator it_boss = Boss.begin(); it_boss != Boss.end(); ++it_boss)
                {
                    if ((*it_boss)->isAlive())
                    {
                        m_creature->CastSpell(m_creature, 34954, true);
                        AzusInitialCheck = 10000;
                        EnterEvadeMode();
                    }
                }
                BoolAzusInitialCheck = true;
            }
            else
                AzusInitialCheck -= diff;
        }

        if (m_creature->IsNonMeleeSpellCasted(false))
            return;

        // BLOODSIPHON
        if (BloodSiphon_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_BLOODSIPHON_STUN) == CAST_OK)
            {
                BloodSiphon_Timer = 90000;
            }
        }
        else
            BloodSiphon_Timer -= diff;

        // CORRUPTEDBLOOD
        if (CorruptedBlood_Timer < diff)
        {
            if (Unit* target = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
            {
                if (DoCastSpellIfCan(target, SPELL_CORRUPTEDBLOOD) == CAST_OK)
                {
                    CorruptedBlood_Timer = urand(14000, 16000);
                }
            }
        }
        else
            CorruptedBlood_Timer -= diff;

        //Hysteria
        if (Hysteria_Timer < diff)
        {
            if (Unit* target = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
            {
                if (target != m_creature->getVictim())
                    if (DoCastSpellIfCan(target, SPELL_HYSTERIA) == CAST_OK)
                    {
                        Hysteria_Timer = urand(10000, 15000);
                        Hysteria_Timer2 = 9500;

                        if (BloodSiphon_Timer < (Hysteria_Timer + 10000))
                            Hysteria_Timer += 20000;
                    }
            }
        }
        else
            Hysteria_Timer -= diff;

        if (Hysteria_Timer2 < diff)
        {
            ThreatList const& tList = m_creature->getThreatManager().getThreatList();
            for (ThreatList::const_iterator itr = tList.begin(); itr != tList.end(); ++itr)
            {
                if (Player* pPlayer = m_creature->GetMap()->GetPlayer((*itr)->getUnitGuid()))
                {
                    if (pPlayer->HasAura(SPELL_HYSTERIA))
                    {
                        if (Creature* HysteriaNPC = m_creature->SummonCreature(25124, pPlayer->GetPositionX(), pPlayer->GetPositionY(), pPlayer->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 20500))
                        {
                            HysteriaNPC->SetInCombatWithZone();
                            m_creature->getThreatManager().modifyThreatPercent(pPlayer, -100);
                            HysteriaNPC->CastSpell(pPlayer, SPELL_CAUSEINSANITY, true);
                            Hysteria_Timer2 = 600000;
                            break;
                        }
                    }
                }
            }
        }
        else
            Hysteria_Timer2 -= diff;

        //Curse of Nemesis
        if (Curse_of_Nemesis_Timer < diff)
        {
            if (Unit* target = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
            {
                if (!target->HasAura(SPELL_HYSTERIA))
                {
                    if (target == m_creature->getVictim())
                    {
                        if (CauseInsanity_Timer > 10000)
                            if (DoCastSpellIfCan(target, SPELL_CURSE_OF_NEMESIS) == CAST_OK)
                                Curse_of_Nemesis_Timer = urand(15000, 22000);
                    }
                    else if (target == m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 4, nullptr, SELECT_FLAG_PLAYER))
                    {
                        if (CauseInsanity_Timer > 10000)
                            if (DoCastSpellIfCan(target, SPELL_CURSE_OF_NEMESIS) == CAST_OK)
                                Curse_of_Nemesis_Timer = urand(15000, 22000);
                    }
                    else if (CauseInsanity_Timer > 5000)
                        if (DoCastSpellIfCan(target, SPELL_CURSE_OF_NEMESIS) == CAST_OK)
                            Curse_of_Nemesis_Timer = urand(15000, 22000);
                }
            }
        }
        else
            Curse_of_Nemesis_Timer -= diff;


        // CAUSEINSANITY
        if (CauseInsanity_Timer < diff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 0, nullptr, SELECT_FLAG_PLAYER))
            {
                if (!pTarget->HasAura(SPELL_HYSTERIA))
                {
                    m_creature->CastSpell(pTarget, SPELL_CAUSEINSANITY, true);
                    m_creature->getThreatManager().modifyThreatPercent(pTarget, -100);
                    CauseInsanity_Timer = 45000;
                }
                /*if (DoCastSpellIfCan(pTarget, SPELL_CAUSEINSANITY) == CAST_OK)*/
            }
        }
        else
            CauseInsanity_Timer -= diff;

        if (CauseInsanity_Timer_3aggro < diff)
        {
            if (Unit* secondTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 4, nullptr, SELECT_FLAG_PLAYER))
            {
                if (!secondTarget->HasAura(SPELL_HYSTERIA))
                {
                    m_creature->CastSpell(secondTarget, SPELL_CAUSEINSANITY, true);
                    m_creature->getThreatManager().modifyThreatPercent(secondTarget, -100);
                    CauseInsanity_Timer_3aggro = 45000;
                }
                /*if (DoCastSpellIfCan(secondTarget, SPELL_CAUSEINSANITY) == CAST_OK)*/
            }
        }
        else
            CauseInsanity_Timer_3aggro -= diff;

        if (CauseInsanity_Timer_random < diff)
        {
            if (Unit* MT = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 0, nullptr, SELECT_FLAG_PLAYER))
                if (Unit* secondTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 4, nullptr, SELECT_FLAG_PLAYER))
                    if (Unit* MCTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 1, nullptr, SELECT_FLAG_PLAYER))
                        if (MCTarget != MT && MCTarget != secondTarget)
                            if (!MCTarget->HasAura(SPELL_HYSTERIA))
                            {
                                m_creature->CastSpell(MCTarget, SPELL_CAUSEINSANITY, true);
                                m_creature->getThreatManager().modifyThreatPercent(MCTarget, -100);
                                CauseInsanity_Timer_random = 45000;
                            }
                                /*if (DoCastSpellIfCan(MCTarget, SPELL_CAUSEINSANITY) == CAST_OK)*/
        }
        else
            CauseInsanity_Timer_random -= diff;



        // BERSERK
        // if ((Berserk_Timer < diff) || (m_creature->GetHealthPercent() < 5.0f))
        if (Berserk_Timer < diff)
        {
            if (!Enraged)
                if (DoCastSpellIfCan(m_creature, SPELL_BERSERK, CF_AURA_NOT_PRESENT) == CAST_OK)
                {
                    Enraged = true;
                    Berserk_Timer = 600000;
                }
        }
        else
            Berserk_Timer -= diff;

        //Checking if Jeklik is dead. If not we cast her Aspect
        if (m_pInstance->GetData(TYPE_JEKLIK) != DONE)
        {
            if (AspectOfJeklik_Timer < diff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_ASPECT_OF_JEKLIK) == CAST_OK)
                    AspectOfJeklik_Timer = urand(10000, 14000);
            }
            else
                AspectOfJeklik_Timer -= diff;
        }

        //Checking if Venoxis is dead. If not we cast his Aspect
        if (m_pInstance->GetData(TYPE_VENOXIS) != DONE)
        {
            if (AspectOfVenoxis_Timer < diff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_ASPECT_OF_VENOXIS) == CAST_OK)
                    AspectOfVenoxis_Timer = 7000;
            }
            else
                AspectOfVenoxis_Timer -= diff;
        }

        //Checking if Marli is dead. If not we cast her Aspect
        if (m_pInstance->GetData(TYPE_MARLI) != DONE)
        {
            if (AspectOfMarli_Timer < diff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_ASPECT_OF_MARLI) == CAST_OK)
                    AspectOfMarli_Timer = 10000;
            }
            else
                AspectOfMarli_Timer -= diff;
        }

        //Checking if Thekal is dead. If not we cast his Aspect
        if (m_pInstance->GetData(TYPE_THEKAL) != DONE)
        {
            if (AspectOfThekal_Timer < diff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_ASPECT_OF_THEKAL) == CAST_OK)
                {
                    DoScriptText(EMOTE_FRENZY_KILL, m_creature);
                    AspectOfThekal_Timer = 15000;
                }
            }
            else
                AspectOfThekal_Timer -= diff;
        }

        //Checking if Arlokk is dead. If not we cast her Aspect
        if (m_pInstance->GetData(TYPE_ARLOKK) != DONE)
        {
            if (AspectOfArlokk_Timer < diff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_ASPECT_OF_ARLOKK) == CAST_OK)
                {
                    DoResetThreat();
                    AspectOfArlokk_Timer = urand(10000, 15000);
                }
            }
            else
                AspectOfArlokk_Timer -= diff;
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_hakkar(Creature* pCreature)
{
    return new boss_hakkarAI(pCreature);
}

void AddSC_boss_hakkar()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "boss_hakkar";
    newscript->GetAI = &GetAI_boss_hakkar;
    newscript->RegisterSelf();
}
