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
SDName: Boss_Thekal
SD%Complete: 95
SDComment: Almost finished.
SDCategory: Zul'Gurub
EndScriptData */

#include "scriptPCH.h"
#include "zulgurub.h"


#define SAY_AGGRO               -1309009
#define SAY_DEATH               -1309010
#define EMOTE_FRENZY_KILL       -1000001

#define EMOTE_DIES              -1309027
#define EMOTE_RESURRECT         -1309028

#define SPELL_MORTALCLEAVE        22859
#define SPELL_SILENCE             22666
#define SPELL_FRENZY              34888
#define SPELL_FORCEPUNCH          24189
#define SPELL_CHARGE              24408
#define SPELL_ENRAGE              8269
#define SPELL_SUMMONTIGERS        24183
#define NPC_TIGER                 15068
#define SPELL_TIGER_FORM          24169
#define SPELL_RESURRECT           24173                     //We will not use this spell.
#define SPELL_LAST_STAND          34889
#define SPELL_FEAR                19408

//Zealot Lor'Khan Spells
#define SPELL_SHIELD              34881
#define SPELL_BLOODLUST           24185
#define SPELL_GREATERHEAL         24208
#define SPELL_DISARM              34887
#define SPELL_FIRENOVA_TOTEM      34758
#define SPELL_ASTRAL_PROTECTION   34885

//Zealot Lor'Khan Spells
#define SPELL_SWEEPINGSTRIKES     13877
#define SPELL_SINISTERSTRIKE      15667
#define SPELL_GOUGE               24698
#define SPELL_KICK                15614
#define SPELL_BLIND               21060
#define SPELL_CLOAK_OF_SHADOWS    34882
#define SPELL_EVASION             15087
#define SPELL_SPRINT              34886
#define SPELL_FIXATE              12021

#define ZELOTH_LOR_KHAN           11347
#define ZELOTH_ZATH               11348

#define RESSURECT_TIMER           7000

/*
INSERT INTO creature_template SET entry=14966, modelid_1=14792, modelid_2=14792, name="High Priest Thekal Transform Visual";
INSERT INTO creature_template SET entry=15109, modelid_1=15214, modelid_2=15214, name="Primal Blessing Visual";
*/

class ChargeUnitSelector : public ThreatListProcesser
{
public:
    ChargeUnitSelector(Unit* thekal) : _victim(NULL), _charger(thekal)
    {
        ASSERT(_charger);
    }
    bool Process(Unit* unit)
    {
        ASSERT(unit);

        // Imperatif de verifier ces conditions
        if (!unit->IsMoving())
            return false;
        float currDist = unit->GetDistance(_charger);
        if (currDist > 35.0f || currDist < 5.0f)
            return false;

        // Ensuite on classe les autres potentielles victimes
        if (_victim)
        {
            float bestDist = _victim->GetDistance(_charger);
            if (currDist > bestDist)
                _victim = unit;
        }
        else
            _victim = unit;
        return false;
    }
    Unit* GetUnit()
    {
        return _victim;
    }
    Unit* _victim;
    Unit* _charger;
};

void DoRessurectUnit(Unit* pUnit, Unit* pVictim);

struct zg_rez_add : public ScriptedAI
{
    zg_rez_add(Creature* pCreature, uint32 instMobType) : ScriptedAI(pCreature),
        m_uiInstMobType(instMobType), m_uiRezzeurGUID(0), m_uiRessurectTimer(0)
    {
        m_pInstance = (instance_zulgurub*)pCreature->GetInstanceData();
        _realyDead   = false;
        _justRevived = false;
    }
    void SetRealyDead(bool value)
    {
        _realyDead = value;
    }
    bool CanBeLooted() const
    {
        return _realyDead;
    }
    void Aggro(Unit *who)
    {
        if (m_pInstance)
            m_pInstance->SetData(m_uiInstMobType, IN_PROGRESS);
        ScriptedAI::Aggro(who);
    }

    void JustDied(Unit* Killer)
    {
        DoScriptText(EMOTE_DIES, m_creature);
        Unit* pRezzer = NULL;
        // Mort et non selectionnable (= pas de loots)
        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        if (m_pInstance)
        {
            pRezzer = m_pInstance->Thekal_GetUnitThatCanRez();
        }
        if (pRezzer)
        {
            m_uiRezzeurGUID = pRezzer->GetGUID();
            m_uiRessurectTimer = RESSURECT_TIMER;
        }
        else // Personne pour le rez : Adds morts : passage en phase 2 (tigre).
        {
            m_uiRessurectTimer = 10000;
            m_uiRezzeurGUID = 0;
        }
    }
    bool HandleWaitRez(const uint32 uiDiff)
    {
        if (!m_pInstance)
            return false;

        if (m_uiRessurectTimer > 2000 && (m_uiRessurectTimer - uiDiff) < 2000)
        {
            if (Unit* pRezzeur = m_pInstance->instance->GetUnit(m_uiRezzeurGUID))
            {
                if (pRezzeur->isAlive())
                {
                    pRezzeur->InterruptNonMeleeSpells(true);
                    pRezzeur->CastSpell(m_creature, SPELL_RESURRECT, false);
                }
            }
        }
        if (m_uiRessurectTimer < uiDiff)
        {
            if (m_uiRezzeurGUID)
            {
                if (Unit* pRezzeur = m_pInstance->instance->GetUnit(m_uiRezzeurGUID))
                {
                    if (!pRezzeur->isAlive())
                    {
                        m_uiRessurectTimer = 8000;
                        m_uiRezzeurGUID = 0;
                        return true;
                    }
                }
            }

            if (Unit* pFriend = m_pInstance->Thekal_GetUnitThatCanRez())
                DoRessurectUnit(m_creature, pFriend->getVictim());
            else
            {
                // Apres les 10 sec sans rez :
                return false;
            }
        }
        else
            m_uiRessurectTimer -= uiDiff;
        return true;
    }
    void EnterCombat(Unit* who)
    {
        if (!m_pInstance)
            return;

        Creature *pThekal = m_creature->GetMap()->GetCreature(m_pInstance->GetData64(DATA_THEKAL));
        Creature *pZath = m_creature->GetMap()->GetCreature(m_pInstance->GetData64(DATA_ZATH));
        Creature *pLorkhan = m_creature->GetMap()->GetCreature(m_pInstance->GetData64(DATA_LORKHAN));
        if (pThekal && pThekal->AI())
            pThekal->AI()->AttackStart(who);
        if (pZath && pZath->AI())
            pZath->AI()->AttackStart(who);
        if (pLorkhan && pLorkhan->AI())
            pLorkhan->AI()->AttackStart(who);
        ScriptedAI::EnterCombat(who);
    }
    void UpdateAI(const uint32 diff)
    {
        if (_justRevived)
        {
            m_creature->SendSpellGo(m_creature, 20770);
            _justRevived = false;
        }
    }
    void Reset()
    {
        if (m_pInstance)
            if (m_pInstance->GetData(TYPE_THEKAL) != DONE)
                m_pInstance->SetData(m_uiInstMobType, NOT_STARTED);
    }

    uint32 m_uiInstMobType;
    instance_zulgurub* m_pInstance;
    uint32 m_uiRessurectTimer;
    uint64 m_uiRezzeurGUID;
    bool _realyDead;
    bool _justRevived;
};

void DoRessurectUnit(Unit* unit, Unit* victim)
{
    DEBUG_UNIT(unit, DEBUG_AI, "Rez %s", unit->GetName());
    float x, y, z, o;
    unit->GetPosition(x, y, z);
    o = unit->GetOrientation();

    unit->SetStandState(UNIT_STAND_STATE_STAND);
    unit->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
    unit->SetHealth(unit->GetMaxHealth());

    unit->AttackStop();

    if (Creature* creature = unit->ToCreature())
    {
        DoScriptText(EMOTE_RESURRECT, creature);
        creature->Respawn();
        creature->NearTeleportTo(x, y, z, o);
        // TRES DANGEREUX.
        // Peut provoquer un stack overflow, donc faire gaffe en modifiant le script.
        // Permet d'effectuer *vraiment* le respawn du mob, pour pouvoir attaquer la victime, et avoir le visuel du sort.
        creature->Update(0, 0);
        creature->SendSpellGo(creature, 20770);
        if (victim)
            creature->AI()->AttackStart(victim);
    }
}


#define TIGER_COUNT 2

struct boss_thekalAI : public zg_rez_add
{
    boss_thekalAI(Creature* pCreature) : zg_rez_add(pCreature, TYPE_THEKAL)
    {
        for (int i = 0; i < TIGER_COUNT; ++i)
            TigerGUIDs[i] = 0;
        m_fDefaultSize = m_creature->GetFloatValue(OBJECT_FIELD_SCALE_X);
        Reset();
    }

    uint32 MortalCleave_Timer;
    uint32 Silence_Timer;
    uint32 Frenzy_Timer;
    uint32 ForcePunch_Timer;
    uint32 Charge_Timer;
    uint32 Enrage_Timer;
    uint32 CheckTigers_Timer;
    uint32 m_uiExploit_Timer;
    uint32 NoTargetReset_Timer;
    uint64 TigerGUIDs[TIGER_COUNT];
    float m_fDefaultSize;

    bool Enraged;
    bool PhaseTwo;
    bool per50;
    bool per3;

    void GetAIInformation(ChatHandler& handler)
    {
        handler.SendSysMessage("DEBUG -- THEKAL");
        handler.PSendSysMessage("Can be looted : [%s]", CanBeLooted() ? "YES" : "NO");
        handler.PSendSysMessage("PhaseTwo = %s, Enraged = %s", PhaseTwo ? "true" : "false", Enraged ? "true" : "false");
        handler.SendSysMessage("Timers :");
        handler.PSendSysMessage("[%u:%u:%u:%u:%u:%u:%u:%u]",
                                MortalCleave_Timer, Silence_Timer, Frenzy_Timer, ForcePunch_Timer, Charge_Timer, Enrage_Timer, CheckTigers_Timer, NoTargetReset_Timer);
    }
    void Reset()
    {
        m_creature->ResetStats();

        MortalCleave_Timer = 4000;
        Silence_Timer = 9000;
        Frenzy_Timer = 30000;
        ForcePunch_Timer = 4000;
        Charge_Timer = 12000;
        Enrage_Timer = 32000;
        CheckTigers_Timer = 2000;
        m_uiExploit_Timer = 10000;
        m_creature->SetFloatValue(OBJECT_FIELD_SCALE_X, m_fDefaultSize);

        Enraged = false;
        per50 = false;
        per3 = false;

        // Si en phase 2 avant de reset, faut repop les 2 adds.
        m_creature->RespawnNearCreaturesByEntry(ZELOTH_LOR_KHAN, 100.0f);
        m_creature->RespawnNearCreaturesByEntry(ZELOTH_ZATH, 100.0f);
        PhaseTwo = false;
        zg_rez_add::Reset();
    }

    void JustDied(Unit* Killer)
    {
        // Si reelement mort ...
        if (PhaseTwo)
        {
            DoScriptText(SAY_DEATH, m_creature);
            DEBUG_UNIT(m_creature, DEBUG_AI, "Thekal is dead for real");

            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_LORKHAN, DONE);
                m_pInstance->SetData(TYPE_ZATH, DONE);
                m_pInstance->SetData(TYPE_THEKAL, DONE);
            }

            ScriptedAI::JustDied(Killer);
        }
        else
        {
            // Mettre tout le monde en combat.
            DEBUG_UNIT(m_creature, DEBUG_AI, "Thekal fake death. Put map in combat.");
            Map::PlayerList const& players = m_creature->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                if (itr->getSource()->GetDistance(m_creature) < 200.0f)
                {
                    itr->getSource()->SetCombatTimer(20000);
                    itr->getSource()->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);
                }
            zg_rez_add::JustDied(Killer);
        }
    }
    void UpdateAI_corpse(const uint32 uiDiff)
    {
        if (PhaseTwo || !m_pInstance)
            return; // La c'est vraiment fini.

        if (!zg_rez_add::HandleWaitRez(uiDiff))
        {
            // Apres les 10 sec sans rez :
            // Rez
            DoRessurectUnit(m_creature, NULL);
            m_creature->SetEntry(14509);
            // Phase 2
            DoSetInPhaseTwo();
            zg_rez_add::SetRealyDead(true);
        }
    }


    void JustRespawned()
    {
        // Pour ne pas appeller ScriptedAI::JustRespawned qui appelle "Reset()"
    }

    void EnterEvadeMode()
    {
        SetRealyDead(false);
        Reset();
        zg_rez_add::EnterEvadeMode();
    }

    void Aggro(Unit *who)
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_THEKAL, IN_PROGRESS);
        if (PhaseTwo)
            DoScriptText(SAY_AGGRO, m_creature);
        m_creature->SetInCombatWithZone();
    }

    void DoSetInPhaseTwo()
    {
        DEBUG_UNIT(m_creature, DEBUG_AI, "Thekal : set P2.");
        PhaseTwo = true;
        DoCastSpellIfCan(m_creature, SPELL_TIGER_FORM);
        m_creature->SetFloatValue(OBJECT_FIELD_SCALE_X, 2.00f);
        m_creature->SetStandState(UNIT_STAND_STATE_STAND);
        m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

        NoTargetReset_Timer = 5000;
        m_creature->DespawnNearCreaturesByEntry(ZELOTH_LOR_KHAN, 100.0f);
        m_creature->DespawnNearCreaturesByEntry(ZELOTH_ZATH, 100.0f);
    }

    void CheckTiger(uint64& guid)
    {
        Creature* pTiger = m_creature->GetMap()->GetCreature(guid);
        if (!pTiger || !pTiger->isAlive())
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
            {
                if (Unit* pCrea = m_creature->SummonCreatureAndAttack(NPC_TIGER, pTarget))
                    guid = pCrea->GetGUID();
            }
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if (!m_creature->isAlive())
            return;
        zg_rez_add::UpdateAI(diff);
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
        {
            if (PhaseTwo)
            {
                if (NoTargetReset_Timer < diff)
                {
                    EnterEvadeMode();
                    return;
                }
                else
                {
                    // On tente tout de même de trouver une cible.
                    Map::PlayerList const& players = m_creature->GetMap()->GetPlayers();
                    float nearestDist = 200.0f;
                    Player* target = NULL;
                    for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                        if (m_creature->canAttack(itr->getSource()))
                            if (itr->getSource()->GetDistance(m_creature) < nearestDist)
                            {
                                nearestDist = itr->getSource()->GetDistance(m_creature);
                                target = itr->getSource();
                            }
                    if (target)
                        AttackStart(target);
                    NoTargetReset_Timer -= diff;
                }
            }
            return;
        }

        /** Prevent exploit */
        if (m_uiExploit_Timer < diff)
        {
            if (m_creature->GetDistance(m_creature->getVictim()) > 30)
                m_creature->CastSpell(m_creature->getVictim(), 26229, true);

            if (m_creature->GetPositionZ() < 51.0f)
            {
                EnterEvadeMode();
                return;
            }
            m_uiExploit_Timer = 5000;
        }
        else
            m_uiExploit_Timer -= diff;

        if (CheckTigers_Timer < diff)
        {
            for (int i = 0; i < TIGER_COUNT; ++i)
                CheckTiger(TigerGUIDs[i]);
            CheckTigers_Timer = 5000;
        }
        else
            CheckTigers_Timer -= diff;

        if (!PhaseTwo)
        {
            if (MortalCleave_Timer < diff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_MORTALCLEAVE) == CAST_OK)
                    MortalCleave_Timer = urand(12000, 18000);
            }
            else
                MortalCleave_Timer -= diff;

            if (Silence_Timer < diff)
            {
                if (m_creature->CastSpellOnHostileCasterInRange(SPELL_SILENCE, 0.0f, 40.0f, false))
                    Silence_Timer = urand(20000, 30000);
                else
                    Silence_Timer = 500;
            }
            else
                Silence_Timer -= diff;
        }
        else
        {
            if (Charge_Timer < diff)
            {
                ChargeUnitSelector* selector = new ChargeUnitSelector(m_creature);
                m_creature->ProcessThreatList(selector);
                if (Unit* target = selector->GetUnit())
                {
                    DoResetThreat();
                    m_creature->getThreatManager().modifyThreatPercent(target, 100);
                    DoCastSpellIfCan(target, SPELL_CHARGE);
                    Charge_Timer = urand(15000, 22000);
                }
                else
                    Charge_Timer = 500; // On reverifiera dans 500ms
                delete selector;
            }
            else
                Charge_Timer -= diff;

            if (Frenzy_Timer < diff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_FRENZY) == CAST_OK)
                    DoScriptText(EMOTE_FRENZY_KILL, m_creature),
                    Frenzy_Timer = urand(20000, 30000);
            }
            else
                Frenzy_Timer -= diff;

            if (ForcePunch_Timer < diff)
            {
                m_creature->CastSpell(m_creature->getVictim(), SPELL_FORCEPUNCH, true);
                ForcePunch_Timer = urand(16000, 21000);
            }
            else
                ForcePunch_Timer -= diff;

            if (!Enraged)
            {
                if (m_creature->GetHealthPercent() < 20.0f)
                {
                    m_creature->AddAura(SPELL_ENRAGE);
                    Enraged = true;
                }
            }

            if (!per50)
            {
                if (m_creature->GetHealthPercent() < 50.0f)
                {
                    m_creature->CastSpell(m_creature->getVictim(),SPELL_FEAR, true);
                    per50 = true;
                }
            }

            if (!per3)
            {
                if (m_creature->GetHealthPercent() < 3.0f)
                {
                    m_creature->MonsterTextEmote("High Priest Thekal emits a piercing howl.", NULL);
                    m_creature->CastSpell(m_creature->getVictim(), SPELL_FEAR, true);
                    m_creature->CastSpell(m_creature, SPELL_LAST_STAND, true);
                    per3 = true;
                }
            }
        }

        DoMeleeAttackIfReady();
    }
};

//Zealot Lor'Khan
struct mob_zealot_lorkhanAI : public zg_rez_add
{
    mob_zealot_lorkhanAI(Creature* pCreature) : zg_rez_add(pCreature, TYPE_LORKHAN)
    {
        bRealyDead = false;
        Reset();
    }

    uint32 Shield_Timer;
    uint32 BloodLust_Timer;
    uint32 GreaterHeal_Timer;
    uint32 Disarm_Timer;
    uint32 FireNovaTotem_Timer;

    uint32 Resurrect_Timer;
    uint64 uiRezzeurGUID;
    bool bRealyDead;

    bool perc75;
    bool perc50;
    bool perc25;
    bool BossBloodlust;

    void Reset()
    {
        Shield_Timer = 1000;
        BloodLust_Timer = 16000;
        GreaterHeal_Timer = 32000;
        Disarm_Timer = 6000;
        FireNovaTotem_Timer = urand(10000, 15000);

        Resurrect_Timer = 10000;
        uiRezzeurGUID = 0;

        perc75 = false;
        perc50 = false;
        perc25 = false;
        BossBloodlust = false;

        m_creature->SetStandState(UNIT_STAND_STATE_STAND);
        m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        zg_rez_add::Reset();
    }

    void JustDied(Unit* Killer)
    {
        zg_rez_add::JustDied(Killer);
    }

    void UpdateAI_corpse(const uint32 uiDiff)
    {
        if (!zg_rez_add::HandleWaitRez(uiDiff))
            zg_rez_add::SetRealyDead(true);
    }

    void DoBloodlust()
    {
        BloodLust_Timer = 5000;
        bool Rogue_Ready = false;
        //Boss
        std::list<Creature*> Boss;
        GetCreatureListWithEntryInGrid(Boss, m_creature, 14599, 100.0f);
        for (std::list<Creature*>::iterator it_boss = Boss.begin(); it_boss != Boss.end(); ++it_boss)
        {
            if ((*it_boss)->isAlive())
            {
                if (!(*it_boss)->HasAura(SPELL_BLOODLUST))
                {
                    m_creature->CastSpell((*it_boss), SPELL_BLOODLUST, false);
                    BloodLust_Timer = urand(15000, 24000);
                }
                else
                {
                    Rogue_Ready = true;
                }
            }
            else
                Rogue_Ready - true;
        }

        //Add Rogue
        if (Rogue_Ready)
        {
            std::list<Creature*> AddRogue;
            GetCreatureListWithEntryInGrid(AddRogue, m_creature, 11348, 100.0f);
            for (std::list<Creature*>::iterator it = AddRogue.begin(); it != AddRogue.end(); ++it)
            {
                if ((*it)->isAlive())
                {
                    if (!(*it)->HasAura(SPELL_BLOODLUST))
                    {
                        m_creature->CastSpell((*it), SPELL_BLOODLUST, false);
                        BloodLust_Timer = urand(15000, 24000);
                    }
                }
            }
        }
    }

    void UpdateAI(const uint32 diff)
    {
        zg_rez_add::UpdateAI(diff);
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        //Shield_Timer
        if (Shield_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_SHIELD) == CAST_OK)
                Shield_Timer = urand(8000,20000);
        }
        else Shield_Timer -= diff;

        //Firenova_Timer
        if (FireNovaTotem_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_FIRENOVA_TOTEM) == CAST_OK)
                FireNovaTotem_Timer = urand(8000, 12000);
        }
        else FireNovaTotem_Timer -= diff;

        //BloodLust_Timer
        if (BloodLust_Timer < diff)
        {
            DoBloodlust();
        }
        else BloodLust_Timer -= diff;

        //Disarm_Timer
        if (Disarm_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_DISARM) == CAST_OK)
                Disarm_Timer = urand(18000, 22000);
        }
        else Disarm_Timer -= diff;

        if (!perc25)
        {
            if (m_creature->GetHealthPercent() < 25.0f)
            {
                m_creature->AddAura(SPELL_ASTRAL_PROTECTION);
                perc25 = true;
            }
        }
        else if (!perc50)
        {
            if (m_creature->GetHealthPercent() < 50.0f)
            {
                m_creature->AddAura(SPELL_ASTRAL_PROTECTION);
                perc50 = true;
            }
        }
        else if (!perc75)
        {
            if (m_creature->GetHealthPercent() < 75.0f)
            {
                m_creature->AddAura(SPELL_ASTRAL_PROTECTION);
                perc75 = true;
            }
        }

        //GreaterHeal
        if (GreaterHeal_Timer < diff)
        {
            // Lorkhan prioritizes himself with heal when under 50%
            if (m_creature->GetHealthPercent() < 50.0f)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_GREATERHEAL) == CAST_OK)
                    GreaterHeal_Timer = 12000;
            }
            else if (Unit* pTarget = m_creature->FindLowestHpFriendlyUnit(100.0f, 20000))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_GREATERHEAL) == CAST_OK)
                    GreaterHeal_Timer = urand(12000, 15000);
            }
        }
        else
            GreaterHeal_Timer -= diff;


        DoMeleeAttackIfReady();
    }
};

//Zealot Zath
struct mob_zealot_zathAI : public zg_rez_add
{
    mob_zealot_zathAI(Creature* pCreature) : zg_rez_add(pCreature, TYPE_ZATH)
    {
        bRealyDead = false;
        Reset();
    }

    uint32 SweepingStrikes_Timer;
    uint32 SinisterStrike_Timer;
    uint32 Gouge_Timer;
    uint32 Kick_Timer;
    uint32 Blind_Timer;
    uint32 Cloak_Timer;
    uint32 Sprint_Timer;
    uint32 Fixate_Timer;

    uint32 Resurrect_Timer;
    uint64 uiRezzeurGUID;
    bool bRealyDead;

    bool perc75;
    bool perc50;
    bool perc25;

    void Reset()
    {
        SweepingStrikes_Timer = 18000;
        SinisterStrike_Timer = 8000;
        Gouge_Timer = 30000;
        Kick_Timer = 8000;
        Blind_Timer = 15000;
        Cloak_Timer = urand(10000, 15000);
        Sprint_Timer = 500;
        Fixate_Timer = 900000;

        perc75 = false;
        perc50 = false;
        perc25 = false;

        Resurrect_Timer = 10000;
        uiRezzeurGUID = 0;

        m_creature->SetStandState(UNIT_STAND_STATE_STAND);
        m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        zg_rez_add::Reset();
    }

    void JustDied(Unit* Killer)
    {
        zg_rez_add::JustDied(Killer);
    }

    void UpdateAI_corpse(const uint32 uiDiff)
    {
        if (!zg_rez_add::HandleWaitRez(uiDiff))
            zg_rez_add::SetRealyDead(true);
    }

    void UpdateAI(const uint32 diff)
    {
        zg_rez_add::UpdateAI(diff);
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        //SweepingStrikes_Timer
        if (SweepingStrikes_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_SWEEPINGSTRIKES) == CAST_OK)
                SweepingStrikes_Timer = urand(30000, 40000);
        }
        else SweepingStrikes_Timer -= diff;

        //SinisterStrike_Timer
        if (SinisterStrike_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_SINISTERSTRIKE) == CAST_OK)
                SinisterStrike_Timer = urand(8000, 16000);
        }
        else SinisterStrike_Timer -= diff;

        //Gouge_Timer
        if (Gouge_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_GOUGE) == CAST_OK)
                Gouge_Timer = urand(25000, 30000);

            /*if (m_creature->getThreatManager().getThreat(m_creature->getVictim()))
                m_creature->getThreatManager().modifyThreatPercent(m_creature->getVictim(), -100);*/
        }
        else Gouge_Timer -= diff;

        //Blind_Timer
        if (Blind_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_BLIND) == CAST_OK)
                Gouge_Timer = 2000,
                Blind_Timer = 20000;
        }
        else Blind_Timer -= diff;

        //CoS_Timer
        if (Cloak_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_CLOAK_OF_SHADOWS) == CAST_OK)
                Cloak_Timer = urand(12000, 20000);
        }
        else Cloak_Timer -= diff;

        //Kick_Timer
        if (Kick_Timer < diff)
        {
            if (m_creature->getVictim()->IsNonMeleeSpellCasted(false))
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_KICK) == CAST_OK)
                    Kick_Timer = urand(10000, 15000);
            }
            else
                Kick_Timer = 1000;
        }
        else Kick_Timer -= diff;

        //Sprint
        if (Sprint_Timer < diff)
        {
            if (!m_creature->HasAura(SPELL_SPRINT))
                if (m_creature->GetDistance(m_creature->getVictim()) > 8.0f)
                    if (DoCastSpellIfCan(m_creature, SPELL_SPRINT) == CAST_OK)
                        Fixate_Timer = 3000,
                        Sprint_Timer = 8000;
        }
        else Sprint_Timer -= diff;

        if (Fixate_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_FIXATE) == CAST_OK)
                Fixate_Timer = 900000;
        }
        else Kick_Timer -= diff;

        if (!perc25)
        {
            if (m_creature->GetHealthPercent() < 25.0f)
            {
                m_creature->AddAura(SPELL_EVASION);
                perc25 = true;
            }
        }
        else if (!perc50)
        {
            if (m_creature->GetHealthPercent() < 50.0f)
            {
                m_creature->AddAura(SPELL_EVASION);
                perc50 = true;
            }
        }
        else if (!perc75)
        {
            if (m_creature->GetHealthPercent() < 75.0f)
            {
                m_creature->AddAura(SPELL_EVASION);
                perc75 = true;
            }
        }


        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_thekal(Creature* pCreature)
{
    return new boss_thekalAI(pCreature);
}

CreatureAI* GetAI_mob_zealot_lorkhan(Creature* pCreature)
{
    return new mob_zealot_lorkhanAI(pCreature);
}

CreatureAI* GetAI_mob_zealot_zath(Creature* pCreature)
{
    return new mob_zealot_zathAI(pCreature);
}

void AddSC_boss_thekal()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "boss_thekal";
    newscript->GetAI = &GetAI_boss_thekal;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "mob_zealot_lorkhan";
    newscript->GetAI = &GetAI_mob_zealot_lorkhan;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "mob_zealot_zath";
    newscript->GetAI = &GetAI_mob_zealot_zath;
    newscript->RegisterSelf();
}
