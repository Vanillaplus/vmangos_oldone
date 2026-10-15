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
SDName: Boss_Azus
SDCategory: Zul'Gurub
EndScriptData */

#include "scriptPCH.h"
#include "zulgurub.h"

enum
{
    SAY_AGGRO1                  = -1900240,
    SAY_AGGRO2                  = -1900241,
    SAY_AGGRO3                  = -1900242,
    SAY_DEATH1                  = -1900243,
    SAY_DEATH2                  = -1900244,
    SAY_KILL1                   = -1900245,
    SAY_KILL2                   = -1900246,

    SPELL_TAUNT                 = 34912, //On charge
    SPELL_CHARGE                = 24408,
    SPELL_EXECUTE               = 34913,
    SPELL_THIRST                = 34914, //Heal on Kill
    SPELL_BERSERKER_BLOOD       = 34915, //Haste based on raid HP %
    SPELL_LACERATE_DUMMY        = 34916, //Movement=Damage debuff
    SPELL_LACERATE_DAMAGE       = 34917, //Movement=Damage damage spell
    SPELL_TRANSFORM             = 34920, //Zerker
    SPELL_TRANSFORM_BONUS       = 34921, //regen+taunt immune
    SPELL_CHARGE_CD             = 34922,
    SPELL_RUPTURE               = 34923, //Long DoT bleed
    SPELL_RUPTURE_CD            = 34924,
    SPELL_BLOOD_CLOUD           = 34925, //AoE
    SPELL_BLOOD_CLOUD_SUMMON    = 34926, //AoE npc+anim
    SPELL_BLOOD_TIDE_VISUAL     = 34927, //Start Blood Tide
    //SPELL_BLOOD_TIDE            = 34945, //HUGE AoE
    SPELL_BLOOD_LEECH           = 34928, //AoE HP drain
    SPELL_6yd_AURA              = 34948,


    NPC_BLOOD_HP                = 25035,
    NPC_BLOOD_MANA              = 25036,
    NPC_BLOOD_CLOUD_PASSIVE     = 25033,

    GO_AZUS_DOOR1               = 400008,
    GO_AZUS_DOOR2               = 400009,
};

struct boss_azusAI : public ScriptedAI
{
    boss_azusAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        m_fDefaultSize = m_creature->GetFloatValue(OBJECT_FIELD_SCALE_X);
        Reset();
    }
    ScriptedInstance* m_pInstance;

    float m_fDefaultSize;
    uint32 m_uiExploit_Timer;
    uint32 Rupture_Timer;
    uint32 HP_Check_Timer;
    uint32 LacerateDummy_Timer;
    uint32 LacerateProc_Timer;
    uint32 Execute_Timer;
    uint32 Transform_Timer;
    bool Transformed;
    uint32 BloodOrb_Timer;
    uint32 Blood_Tide_Timer;
    uint32 Blood_Leech_Timer;
    bool IsCastingBloodBoom;

    float HPperc;
    int32 HP50;
    int32 HP20;
    int32 HPbuff;
    int32 Size;
    bool Charged_Normal;
    bool Charged_Zerk;

    void Reset()
    {
        m_uiExploit_Timer = 10000;
        Rupture_Timer = urand(5000, 10000);
        HP_Check_Timer = 5000; 
        Execute_Timer = 2000;
        LacerateDummy_Timer = urand(5000, 15000);
        LacerateProc_Timer = urand(10000, 18000);
        Transform_Timer = 45000;
        Transformed = false;
        BloodOrb_Timer = urand(14000, 25000);
        Blood_Tide_Timer = urand(25000, 30000);
        Blood_Leech_Timer = urand(3000, 8000);
        IsCastingBloodBoom = false;

        HPperc = 0.0f;
        HP50 = 0;
        HP20 = 0;
        HPbuff = 0;
        Size = 0;
        Charged_Normal = false;
        Charged_Zerk = false;
    }

    void Aggro(Unit* pWho)
    {
        if (urand(0,2) > 1)
            DoScriptText(SAY_AGGRO1, m_creature);
        else if (urand(0,1) > 0)
            DoScriptText(SAY_AGGRO2, m_creature);
        else
            DoScriptText(SAY_AGGRO3, m_creature);

        if (m_pInstance)
            m_pInstance->SetData(TYPE_AZUS, IN_PROGRESS);
        if (GameObject* door = m_creature->FindNearestGameObject(GO_AZUS_DOOR1, 150.0f))
            door->UseDoorOrButton();
        if (GameObject* door = m_creature->FindNearestGameObject(GO_AZUS_DOOR2, 150.0f))
            door->UseDoorOrButton();
    }

    void JustReachedHome()
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_AZUS, NOT_STARTED);

        if (GameObject* door = m_creature->FindNearestGameObject(GO_AZUS_DOOR1, 150.0f))
            door->ResetDoorOrButton();
        if (GameObject* door = m_creature->FindNearestGameObject(GO_AZUS_DOOR2, 150.0f))
            door->ResetDoorOrButton();


        std::list<Creature*> bloodclouds;
        GetCreatureListWithEntryInGrid(bloodclouds, m_creature, NPC_BLOOD_CLOUD_PASSIVE, DEFAULT_VISIBILITY_INSTANCE);
        for (auto it = bloodclouds.cbegin(); it != bloodclouds.cend(); ++it)
        {
            (*it)->RemoveAllDynObjects();
            (*it)->ForcedDespawn();
        }
        m_creature->RemoveAllDynObjects();
    }

    void JustDied(Unit* pKiller)
    {
        DoScriptText(urand(0, 1) ? SAY_DEATH1 : SAY_DEATH2, m_creature);

        m_creature->SetFloatValue(OBJECT_FIELD_SCALE_X, m_fDefaultSize);

        if (m_pInstance)
            m_pInstance->SetData(TYPE_AZUS, DONE);
        if (GameObject* door = m_creature->FindNearestGameObject(GO_AZUS_DOOR1, 150.0f))
            door->ResetDoorOrButton();
        if (GameObject* door = m_creature->FindNearestGameObject(GO_AZUS_DOOR2, 150.0f))
            door->ResetDoorOrButton();

        std::list<Creature*> bloodclouds;
        GetCreatureListWithEntryInGrid(bloodclouds, m_creature, NPC_BLOOD_CLOUD_PASSIVE, DEFAULT_VISIBILITY_INSTANCE);
        for (auto it = bloodclouds.cbegin(); it != bloodclouds.cend(); ++it)
        {
            (*it)->RemoveAllDynObjects();
            (*it)->ForcedDespawn();
        }
        m_creature->RemoveAllDynObjects();
    }

    void KilledUnit(Unit* pVictim)
    {
        if (pVictim->GetTypeId() == TYPEID_PLAYER)
        {
            m_creature->CastSpell(m_creature, SPELL_THIRST, true);

            if (m_creature->HasAura(SPELL_TAUNT))
                m_creature->RemoveAurasDueToSpell(SPELL_TAUNT);

            if (!m_creature->HasAura(34821))
            {
                m_creature->CastSpell(m_creature, 34821, true);
                DoScriptText(urand(0, 1) ? SAY_KILL1 : SAY_KILL2, m_creature);
            }
        }
    }

    void Do_HP_Check()
    {
        HP50 = 0;
        HP20 = 0;
        Charged_Normal = false;
        Charged_Zerk = false;;

        Map::PlayerList const& players = m_creature->GetMap()->GetPlayers();
        if (players.isEmpty())
            return;

        for (Map::PlayerList::const_iterator iter = players.begin(); iter != players.end(); ++iter)
        {

            if (Player* pTempTarget = iter->getSource())
            {
                if (pTempTarget->isAlive())
                {
                    if (!pTempTarget->IsGameMaster())
                    {
                        if (pTempTarget->isAlive())
                        {
                            if (HPperc = pTempTarget->GetHealthPercent())
                            {
                                if (HPperc < 20.0f)
                                {
                                    HP20 += 1;

                                    if (pTempTarget != m_creature->getVictim() && pTempTarget->isInCombat())
                                    {
                                        if (Transformed)
                                        {
                                            if (!Charged_Zerk)
                                            {
                                                if (!m_creature->HasAura(SPELL_CHARGE_CD))
                                                {
                                                    Charged_Zerk = true;
                                                    DoResetThreat();
                                                    m_creature->AddAura(SPELL_CHARGE_CD);
                                                    m_creature->CastSpell(pTempTarget, SPELL_CHARGE, true);
                                                    m_creature->getThreatManager().addThreatDirectly(pTempTarget, urand(7500, 10000));
                                                }
                                            }
                                        }
                                        else
                                        {
                                            if (!Charged_Normal)
                                            {
                                                if (!m_creature->HasAura(SPELL_TAUNT))
                                                {
                                                    Charged_Normal = true;
                                                    pTempTarget->CastSpell(m_creature, SPELL_TAUNT, true);
                                                    m_creature->CastSpell(pTempTarget, SPELL_CHARGE, true);
                                                }
                                            }
                                        }
                                    }
                                }
                                else if (HPperc < 50.0f)
                                    HP50 += 1;
                            }
                        }
                    }
                }
            }
        }
        HPbuff = (HP50 + HP20 * 3) * 5;
        Size = HPbuff / 2;

        m_creature->CastCustomSpell(m_creature, SPELL_BERSERKER_BLOOD, &HPbuff, &HPbuff, &Size, true);
    }

    /*void Do_HP_Lacerate()
    {
        Map::PlayerList const& players = m_creature->GetMap()->GetPlayers();
        if (players.isEmpty())
            return;

        for (Map::PlayerList::const_iterator iter = players.begin(); iter != players.end(); ++iter)
        {
            if (Player* pPlayer = iter->getSource())
            {
                if (pPlayer->HasAura(SPELL_LACERATE_DUMMY))
                    if (pPlayer->isAlive())
                        if (pPlayer->IsMoving())
                            DoCastSpellIfCan(pPlayer, SPELL_LACERATE_DAMAGE, CF_TRIGGERED);
            }
        }
    }*/

    void Summon_Blood_Orbs()
    {
        Map::PlayerList const& players = m_creature->GetMap()->GetPlayers();
        if (players.isEmpty())
            return;

        for (Map::PlayerList::const_iterator iter = players.begin(); iter != players.end(); ++iter)
        {
            if (Player* pPlayer = iter->getSource())
            {
                uint32 NPC_ID = NPC_BLOOD_HP;

                switch (pPlayer->getClass())
                {
                case CLASS_WARRIOR:
                case CLASS_ROGUE:
                {
                    NPC_ID = NPC_BLOOD_HP;
                }
                break;
                case CLASS_PRIEST:
                case CLASS_MAGE:
                case CLASS_WARLOCK:
                {
                    if (urand(0, 99) > 19)
                        NPC_ID = NPC_BLOOD_MANA;
                }
                break;
                case CLASS_PALADIN:
                {
                    //healer/tank
                    if (pPlayer->HasAura(25780) || pPlayer->HasSpell(20473))
                        if (urand(0, 99) > 19)
                            NPC_ID = NPC_BLOOD_MANA;

                    //Ret
                    if (urand(0, 99) > 90)
                        NPC_ID = NPC_BLOOD_MANA;
                }
                break;
                case CLASS_SHAMAN:
                {
                    //healer/caster
                    if (pPlayer->HasAura(16117) || pPlayer->HasSpell(16188))
                        if (urand(0, 99) > 19)
                            NPC_ID = NPC_BLOOD_MANA;

                    //Enh
                    if (urand(0, 99) > 90)
                        NPC_ID = NPC_BLOOD_MANA;
                }
                break;
                case CLASS_HUNTER:
                {
                    if (urand(0, 99) > 90)
                        NPC_ID = NPC_BLOOD_MANA;
                }
                break;
                case CLASS_DRUID:
                {
                    //Feral
                    if (!(pPlayer->HasAura(9634) || pPlayer->HasAura(768)))
                        if (urand(0, 99) > 19)
                            NPC_ID = NPC_BLOOD_MANA;
                }
                break;
                default:
                    break;
                }

                if (m_creature->GetDistance(pPlayer) < 70 && pPlayer->GetPositionZ() < 24.0f)
                {
                    if (Creature* Shade = m_creature->SummonCreature(NPC_ID, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 5000))
                        if (Shade)
                        {
                            // Ajoute les ennemis du boss dans sa liste de menace.
                            Shade->AI()->AttackStart(pPlayer);
                            Shade->getThreatManager().addThreatDirectly(pPlayer, 100000);
                        }
                }
            }
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_creature->IsNonMeleeSpellCasted(false))
            IsCastingBloodBoom = true;

        //Transform
        if (Transform_Timer < diff)
        {
            if (!Transformed)
            {
                m_creature->MonsterTextEmote("Azus the Bloodseeker goes into a Berserking rage", NULL);
                Transformed = true;
                m_creature->AddAura(SPELL_TRANSFORM);
                m_creature->AddAura(SPELL_TRANSFORM_BONUS);
                Transform_Timer = 45000;
            }
            else
            {
                /*m_creature->MonsterTextEmote("Transformed = false", NULL);*/
                Transformed = false;
                /*m_creature->RemoveAurasDueToSpell(SPELL_TRANSFORM);
                m_creature->RemoveAurasDueToSpell(SPELL_TRANSFORM_BONUS);*/
                Blood_Tide_Timer = urand(15000, 40000);
                Transform_Timer = 45000;
            }
        }
        else
            Transform_Timer -= diff;

        /** Prevent exploit */
        if (m_uiExploit_Timer < diff)
        {
            if (m_creature->GetDistance2d(-11788.0f, -1645.5f) > 36.0f)
            {
                EnterEvadeMode();
                return;
            }
            m_uiExploit_Timer = 5000;
        }
        else
            m_uiExploit_Timer -= diff;

        //Rupture (DoT)
        if (Rupture_Timer < diff)
        {
            if (!IsCastingBloodBoom)
            {
                if (Unit* target = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, nullptr, SELECT_FLAG_PLAYER))
                {
                    if (!target->HasAura(SPELL_RUPTURE_CD))
                    {
                        if ((DoCastSpellIfCan(target, SPELL_RUPTURE)) == CAST_OK)
                        {
                            target->AddAura(SPELL_RUPTURE_CD);
                            Rupture_Timer = urand(9000, 15000);
                        }
                    }
                }
            }
        }
        else Rupture_Timer -= diff;

        //Lacerate DONT MOVE!
        if (LacerateDummy_Timer < diff)
        {
            if (!IsCastingBloodBoom)
            {
                if (Unit* target = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, nullptr, SELECT_FLAG_PLAYER))
                {
                    if (!target->HasAura(SPELL_LACERATE_DUMMY) && target != m_creature->getVictim())
                    {
                        if ((DoCastSpellIfCan(target, SPELL_LACERATE_DUMMY)) == CAST_OK)
                        {
                            LacerateDummy_Timer = urand(8000, 10000);
                        }
                    }
                }
            }
        }
        else LacerateDummy_Timer -= diff;

        //Blood Leech
        if (Blood_Leech_Timer < diff)
        {
            if (!IsCastingBloodBoom)
            {
                DoCastSpellIfCan(m_creature->getVictim(), SPELL_BLOOD_LEECH);
                Blood_Leech_Timer = urand(5000, 8000);
            }
        }
        else Blood_Leech_Timer -= diff;

        //HP Check buff
        if (HP_Check_Timer < diff)
        {
            if (!IsCastingBloodBoom)
            {
                Do_HP_Check();
                HP_Check_Timer = 3000;
            }
        }
        else HP_Check_Timer -= diff;

        if (LacerateProc_Timer < diff)
        {
            /*Do_HP_Lacerate();*/
            DoCastSpellIfCan(m_creature, SPELL_LACERATE_DAMAGE, CF_TRIGGERED);
            LacerateProc_Timer = 1000;
        }
        else LacerateProc_Timer -= diff;

        if (BloodOrb_Timer < diff)
        {
            if (!IsCastingBloodBoom)
            {
                m_creature->CastSpell(m_creature, SPELL_BLOOD_CLOUD, true);
                m_creature->SummonCreature(NPC_BLOOD_CLOUD_PASSIVE, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 120000);
                /*m_creature->CastSpell(m_creature, SPELL_BLOOD_CLOUD_SUMMON, true);*/

                Summon_Blood_Orbs();
                BloodOrb_Timer = urand(25000, 38000);
            }
        }
        else BloodOrb_Timer -= diff;

        if (!Transformed)
        {
            //Execute
            if (Execute_Timer < diff)
            {
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, nullptr, SELECT_FLAG_IN_MELEE_RANGE))
                {
                    DoCastSpellIfCan(pTarget, SPELL_EXECUTE);
                    Execute_Timer = 2000;
                }
            }
            else
                Execute_Timer -= diff;

            //Waves of Blood
            if (Blood_Tide_Timer < diff)
            {
                if (!IsCastingBloodBoom)
                {
                    m_creature->MonsterTextEmote("Azus the Bloodseeker starts a bloody ritual.", NULL);
                    DoCastSpellIfCan(m_creature->getVictim(), SPELL_BLOOD_TIDE_VISUAL);
                    /*m_creature->CastSpell(m_creature, SPELL_BLOOD_TIDE_VISUAL, true);*/
                    /*m_creature->CastSpell(m_creature, SPELL_BLOOD_TIDE, true);*/
                    Blood_Tide_Timer = 60000;
                }
            }
            else
                Blood_Tide_Timer -= diff;
        }

        IsCastingBloodBoom = false;
        DoMeleeAttackIfReady();
    }
};

struct boss_azus_dummy_npcAI : public ScriptedAI
{
    boss_azus_dummy_npcAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    void Reset()
    {
    }

    void SpellHit(Unit* pCaster, const SpellEntry* pSpell)
    {
        if (pSpell->Id == 34927)
        {
            if (!m_creature->HasAura(SPELL_6yd_AURA))
                m_creature->CastSpell(m_creature, 34946, true);

        }
    }

    void JustDied(Unit* pKiller)
    {
    }
};

CreatureAI* GetAI_boss_azus(Creature* pCreature)
{
    return new boss_azusAI(pCreature);
}

CreatureAI* GetAI_npc_azus_dummy(Creature* pCreature)
{
    return new boss_azus_dummy_npcAI(pCreature);
}

void AddSC_boss_azus()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "boss_azus";
    newscript->GetAI = &GetAI_boss_azus;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_azus_dummy";
    newscript->GetAI = &GetAI_npc_azus_dummy;
    newscript->RegisterSelf();
}