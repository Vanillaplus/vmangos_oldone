/*
 *
 */

#include "scriptPCH.h"
#include "scarlet_monastery.h"

//#define SAY_AGGRO                   -1189000
//#define SAY_WHIRLWIND               -1189001
//#define SAY_ENRAGE                  -1189002
//#define SAY_KILL                    -1189003
//#define EMOTE_ENRAGE                -1189004
//
//#define SAY_TRAINEE_SPAWN           -1189022
//
//#define SPELL_RUSHINGCHARGE         8260
//#define SPELL_CLEAVE                15496
//#define SPELL_WHIRLWIND             8989
//#define SPELL_FRENZY                8269

enum
{
    SAY_AGGRO                       = -1189000, // Ah, I have been waiting for a real challenge!
    SAY_BLADES_OF_LIGHT             = -1189001, // Blades of Light!
    SAY_BLADESTORM                  = -1189002, // Light, give me strength!
    SAY_KILL                        = -1189003, // Hah, is that all?
    EMOTE_ENRAGE                    = -1189004, // %s becomes enraged!

    SAY_TRAINEE_SPAWN               = -1189022, // Adds: The master has fallen! Avenge him my brethren!


    ENRAGE_TIMER                    = 5 * MINUTE * IN_MILLISECONDS,
    FLEX_TIMER_MIN                  = 2 * MINUTE * IN_MILLISECONDS,
    FLEX_TIMER_MAX                  = 5 * MINUTE * IN_MILLISECONDS,

    SPELL_PUMMEL                    = 35900,
    SPELL_RUSHINGCHARGE             = 35901,
    SPELL_BLADESTORM                = 35907,
    SPELL_BERSERKER_STANCE          = 35910,
    SPELL_BLADES_OF_LIGHT           = 35917,
    SPELL_THUNDER_CLAP              = 35918,
    SPELL_FRENZY                    = 35919,
    SPELL_BERSERK_HARD_ENRAGE       = 35926,
    SPELL_FRENZY_DEBUFF             = 35927,
    SPELL_ENRAGE_PROC               = 35928,

    SPELL_DEMORALIZE                = 35929,
    SPELL_DEMO_PLAYER_DPS_HOLDER    = 35930,
    SPELL_DEMO_BREAKPOINT_HOLDER    = 35931,
    SPELL_FIGHT_TIMER               = 35932,
    SPELL_SELF_FEAR                 = 35933,

    SPELL_HOLY_NOVA                 = 35921,
    SPELL_HOLY_NOVA_KNOCKBACK       = 35922,
    SPELL_HOLY_NOVA_KNOCKBACK_SLOW  = 35923,
    SPELL_HOLY_LINK                 = 35924,

    NPC_HOLY_NOVA                   = 25240,
    NPC_HOLY_LINK                   = 25241,
    NPC_HOLY_NOVA_KNOCKBACK         = 25242,



    NPC_SCARLET_TRAINEE             = 25257,
    GO_HEROD_DOOR                   = 101854
};

struct boss_herodAI : ScriptedAI
{
    explicit boss_herodAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        boss_herodAI::Reset();
    }

    ScriptedInstance* m_pInstance;

    bool Enrage;
    bool TraineeSay;
    bool m_bBladeStorm;
    uint32 PosCheckTimer;
    uint32 RushingCharge_Timer;
    uint32 Pummel_Timer;
    uint32 Demoral_Timer;
    uint32 Whirlwind_Timer;
    uint32 BladeStorm_Timer;
    uint32 m_uiRootTimer;
    uint32 ThunderClap_Timer;
    uint32 Enrage_Timer;
    uint32 CheckHP_Timer;
    uint32 Flex_Timer;
    float Last_Enrage_HP_Mark;

    void Reset() override
    {
        Enrage = false;
        TraineeSay = false;
        m_bBladeStorm = false;
        PosCheckTimer = 5000;
        RushingCharge_Timer = 1500;
        Pummel_Timer = 1000;
        Demoral_Timer = urand(10000, 20000);
        ThunderClap_Timer = urand(8000, 15000);
        Whirlwind_Timer = urand(10000, 20000);
        BladeStorm_Timer = urand(55000, 65000);
        m_uiRootTimer = 0;
        Enrage_Timer = ENRAGE_TIMER;
        Flex_Timer = urand(FLEX_TIMER_MIN, FLEX_TIMER_MAX);
        CheckHP_Timer = 1000;
        Last_Enrage_HP_Mark = 100.0f;
    }

    void Aggro(Unit* /*pWho*/) override
    {
        m_creature->SetInCombatWithZone();
        DoScriptText(SAY_AGGRO, m_creature);
        m_creature->CastSpell(m_creature, SPELL_BERSERKER_STANCE, true);
        m_creature->CastSpell(m_creature, SPELL_RUSHINGCHARGE, true);
        
        m_creature->CastSpell(m_creature, SPELL_FIGHT_TIMER, true);

        ThreatList const& tList = m_creature->getThreatManager().getThreatList();
        for (ThreatList::const_iterator iter = tList.begin(); iter != tList.end(); ++iter)
        {
            if (Unit* Targets = m_creature->GetMap()->GetUnit((*iter)->getUnitGuid()))
            {
                Targets->CastSpell(Targets, 35930, true);    // DPS
                Targets->CastSpell(Targets, 35931, true);    // Breakpoint
            }
        }

        if (m_pInstance)
            m_pInstance->SetData(TYPE_HEROD, IN_PROGRESS);
    }

    void KilledUnit(Unit* /*pVictim*/) override
    {
        DoScriptText(SAY_KILL, m_creature);
    }

    void EnterEvadeMode() override
    {
        RemoveDebuffs();
        m_creature->clearUnitState(UNIT_STAT_ROOT);
        ScriptedAI::EnterEvadeMode();
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        RemoveDebuffs();

        float x = 597.0f;
        float y = 1244.9f;
        float z = 49.7f;

        for (uint8 i = 0; i < 20; ++i)
        {
            if (i % 2 == 0)
            {
                y = 1247.4f;
                x += 1.0f;
            }
            else
                y = 1244.9f;

            if (Creature* Crea = m_creature->SummonCreature(NPC_SCARLET_TRAINEE, x, y, z, 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 60000))
            {
                if (!TraineeSay)
                {
                    DoScriptText(SAY_TRAINEE_SPAWN, Crea);
                    TraineeSay = true;
                }
                Crea->SetInCombatWithZone();
                Crea->SetNoXP();

                Map::PlayerList const& players = m_creature->GetMap()->GetPlayers();
                if (players.isEmpty())
                    break;

                for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                {
                    if (Player* pPlayer = itr->getSource())
                        if (pPlayer->isAlive() && Crea->GetDistance(pPlayer) < 70.0f)
                            Crea->getThreatManager().addThreat(pPlayer, urand(10, 3000));
                }
            }
        }

        if (m_pInstance)
            m_pInstance->SetData(TYPE_HEROD, DONE);
    }

    void JustReachedHome()
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_HEROD, FAIL);
    }

    void RemoveDebuffs()
    {
        m_creature->RemoveAllDynObjects();
        Map::PlayerList const& players = m_creature->GetMap()->GetPlayers();
        if (players.isEmpty())
            return;

        for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
        {
            if (Player* pPlayer = itr->getSource())
            {
                pPlayer->RemoveAurasDueToSpell(SPELL_DEMORALIZE);
                pPlayer->RemoveAurasDueToSpell(SPELL_DEMO_PLAYER_DPS_HOLDER);
                pPlayer->RemoveAurasDueToSpell(SPELL_DEMO_BREAKPOINT_HOLDER);
                pPlayer->RemoveAurasDueToSpell(SPELL_SELF_FEAR);
                pPlayer->RemoveAurasDueToSpell(SPELL_HOLY_NOVA_KNOCKBACK);
                pPlayer->RemoveAurasDueToSpell(SPELL_HOLY_NOVA_KNOCKBACK_SLOW);
            }
        }
    }

    void UpdateAI(const uint32 diff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
        {
            if (Flex_Timer < diff)
            {
                m_creature->HandleEmote(23);
                Flex_Timer = urand(FLEX_TIMER_MIN, FLEX_TIMER_MAX);
            }
            else Flex_Timer -= diff;

            return;
        }

        if (Enrage_Timer < diff)
        {
            m_creature->AddAura(SPELL_BERSERK_HARD_ENRAGE);
            DoScriptText(EMOTE_ENRAGE, m_creature);

            Enrage_Timer = 300000;
        }
        else Enrage_Timer -= diff;

        if (!Enrage && m_creature->GetHealthPercent() <= 30.0f)     //If we are <30% hp goes Frenzied
        {
            m_creature->CastSpell(m_creature, SPELL_FRENZY, true);
            m_creature->AddAura(SPELL_FRENZY_DEBUFF);
            Enrage = true;
        }

        if (CheckHP_Timer < diff)
        {
            if (m_creature->GetHealthPercent() < (Last_Enrage_HP_Mark - 5.0f))
            {
                m_creature->CastSpell(m_creature, SPELL_ENRAGE_PROC, true);
                Last_Enrage_HP_Mark = Last_Enrage_HP_Mark - 5.0f;
            }
            CheckHP_Timer = 1000;
        }
        else CheckHP_Timer -= diff;

        if (m_bBladeStorm)
        {
            if (m_uiRootTimer < diff)
            {
                m_creature->CastSpell(m_creature, SPELL_ENRAGE_PROC, true);
                m_creature->clearUnitState(UNIT_STAT_ROOT);
                m_bBladeStorm = false;
            }
            else
            {
                m_uiRootTimer -= diff;
                return;
            }
        }

        if (Demoral_Timer < diff)
        {
            m_creature->CastSpell(m_creature, SPELL_DEMORALIZE, true);

            Demoral_Timer = urand(10000, 18000);
            if (BladeStorm_Timer < 5000)
                Demoral_Timer += 5000;
        }
        else Demoral_Timer -= diff;

        if (PosCheckTimer < diff)
        {
            if (m_creature->GetPositionX() > 595.0f || (m_creature->GetPositionX() < 553.0f && m_creature->GetPositionZ() < 39.0f))
                EnterEvadeMode();

            PosCheckTimer = 1000;
        }
        else PosCheckTimer -= diff;


        if (!m_creature->IsNonMeleeSpellCasted(false))
        {
            if (Pummel_Timer < diff)    //Interrupt casting
            {
                if (m_creature->getVictim()->IsNonMeleeSpellCasted())
                {
                    if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_PUMMEL) == CAST_OK)
                        Pummel_Timer = 10000;
                }
                Pummel_Timer = 500;
            }
            else Pummel_Timer -= diff;

            if (RushingCharge_Timer < diff)     // Rushing Charge
            {
                if (!m_creature->HasAura(SPELL_RUSHINGCHARGE))
                {
                    if (m_creature->GetDistance(m_creature->getVictim()) > 15.0f || fabs(m_creature->GetPositionZ() - m_creature->getVictim()->GetPositionZ()) > 5.0f)
                    {
                        if (DoCastSpellIfCan(m_creature, SPELL_RUSHINGCHARGE) == CAST_OK)
                            RushingCharge_Timer = 2000;
                    }
                }
            }
            else
                RushingCharge_Timer -= diff;

            if (ThunderClap_Timer < diff)    // Clap cast interruption
            {
                m_creature->CastSpell(m_creature, SPELL_THUNDER_CLAP, true);
                ThunderClap_Timer = urand(8000, 10000);
            }
            else ThunderClap_Timer -= diff;

            if (Whirlwind_Timer < diff)    // Single Whirlwind
            {
                DoScriptText(SAY_BLADES_OF_LIGHT, m_creature);
                m_creature->CastSpell(m_creature, SPELL_BLADES_OF_LIGHT, false);
                Whirlwind_Timer = urand(10000, 14000);

                if (BladeStorm_Timer < 5000)
                    BladeStorm_Timer = 5000;
            }
            else Whirlwind_Timer -= diff;

            if (BladeStorm_Timer < diff)    // BladeStorm
            {
                m_creature->CastSpell(m_creature, SPELL_ENRAGE_PROC, true);
                m_creature->addUnitState(UNIT_STAT_ROOT);
                m_bBladeStorm = true;
                m_uiRootTimer = 12000;
                DoScriptText(SAY_BLADESTORM, m_creature);
                m_creature->CastSpell(m_creature, SPELL_BLADESTORM, false);
                BladeStorm_Timer = urand(55000, 65000);
                Whirlwind_Timer = urand(15000, 17000);
                ThunderClap_Timer = urand(12000, 15000);

                std::list<Creature*> HolyNova;
                GetCreatureListWithEntryInGrid(HolyNova, m_creature, NPC_HOLY_NOVA, 100.0f);
                for (Creature* NovaCrea : HolyNova)
                    if (NovaCrea)
                        NovaCrea->CastSpell(NovaCrea, SPELL_HOLY_NOVA, false);

                std::list<Creature*> HolyLink;
                GetCreatureListWithEntryInGrid(HolyLink, m_creature, NPC_HOLY_LINK, 100.0f);
                for (Creature* HolyLinkCrea : HolyLink)
                    if (HolyLinkCrea)
                        HolyLinkCrea->CastSpell(HolyLinkCrea, SPELL_HOLY_LINK, false);

                if (Creature* Crea = m_creature->FindNearestCreature(NPC_HOLY_NOVA_KNOCKBACK, 100.0f))
                    Crea->CastSpell(Crea, SPELL_HOLY_NOVA_KNOCKBACK, false);
            }
            else
                BladeStorm_Timer -= diff;
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_herod(Creature* pCreature)
{
    return new boss_herodAI(pCreature);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////


enum
{
    MICHAEL_SAY_AGGRO           = -1900320,  // Humility. You must learn humility.
    SAY_DISARM                  = -1900321,  // Your swordsmanship is amateur at best.
    SAY_CURSE                   = -1900322,  // Your anger amuses me.
    SAY_DEATH                   = -1900323,  // Do not mistake this... for mastery...
    SAY_KICK                    = -1900324,  // Now your body learns humility.
    SAY_SLAY1                   = -1900325,  // 
    SAY_SLAY2                   = -1900326,  // 
    SAY_SLAY3                   = -1900327,  // 


    SPELL_LOTUS_CLOUD_PASSIVE   = 35937,
    SPELL_FOUR_FINGERS          = 35940,
    SPELL_THORNED_ROOTS         = 35942,
    SPELL_CURSE_OF_THORNS       = 35944,
    SPELL_CD_1_SEC              = 35249,
    SPELL_FISTS_OF_FIRE         = 35946,
    SPELL_AOE_DISARM            = 35948,
    SPELL_KICK                  = 35949,
    SPELL_SOUL_CONTROL          = 35950,
    SPELL_SOUL_CONTROL_KNOCK    = 36335,
    SPELL_SOUL_LINK             = 35951,
    SPELL_INNER_FIRE            = 36155,

    NPC_BELTHERIS               = 25243,
    NPC_SOUL_EATER              = 25245,
    NPC_SOUL                    = 25246,
};

float const SilenceZonePos[4][2] =
{
    { 555.0f, 1352.0f},
    { 555.0f, 1324.5f},
    { 527.5f, 1352.0f},
    { 527.5f, 1324.5f}
};

struct boss_michaelAI : ScriptedAI
{
    explicit boss_michaelAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        boss_michaelAI::Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 PosCheckTimer;
    uint32 Kick_Timer;
    uint32 Finger_Timer;
    uint32 Root_Timer;
    uint32 Curse_Timer;
    uint32 Fists_Timer;
    uint32 Disarm_Timer;
    uint32 SoulLink_Timer;
    uint32 InnerFire_Timer;

    void Reset() override
    {
        PosCheckTimer = 5000;
        Kick_Timer = 25000;
        Finger_Timer = 15000;
        Root_Timer = urand(8000, 20000);
        Curse_Timer = urand(3000, 10000);
        Fists_Timer = 28000;
        Disarm_Timer = urand(12000, 18000);
        SoulLink_Timer = urand(25000, 35000);
        InnerFire_Timer = 1000;

        if (Creature* Crea = m_creature->FindNearestCreature(NPC_BELTHERIS, 100.0f))
            Crea->GetMotionMaster()->MoveFollow(m_creature, 0.0f, 0.0f);

        std::list<Creature*> PredatorNPC;
        GetCreatureListWithEntryInGrid(PredatorNPC, m_creature, NPC_SOUL_EATER, 100.0f);
        for (Creature* PredatorCrea : PredatorNPC)
            if (PredatorCrea)
            {
                if (!PredatorCrea)
                    continue;

                if (CreatureAI* ai = PredatorCrea->AI())
                    ai->EnterEvadeMode();

                PredatorCrea->GetMotionMaster()->MoveFollow(m_creature, 0.0f, 0.0f);
            }
    }

    void Aggro(Unit* /*pWho*/) override
    {
        m_creature->SetInCombatWithZone();
        DoScriptText(MICHAEL_SAY_AGGRO, m_creature);
        m_creature->RemoveAurasDueToSpell(SPELL_LOTUS_CLOUD_PASSIVE);

        std::list<Creature*> PredatorNPC;
        GetCreatureListWithEntryInGrid(PredatorNPC, m_creature, NPC_SOUL_EATER, 100.0f);
        for (Creature* PredatorCrea : PredatorNPC)
            if (PredatorCrea)
                PredatorCrea->AddAura(35953);

        if (m_pInstance)
            m_pInstance->SetData(TYPE_MICHAEL, IN_PROGRESS);
    }

    void KilledUnit(Unit* /*pVictim*/) override
    {
        if (m_creature->HasAura(34821))
            return;

        m_creature->CastSpell(m_creature, 34821, true);
        switch (urand(0, 2))
        {
            case 0: DoScriptText(SAY_SLAY1, m_creature); break;
            case 1: DoScriptText(SAY_SLAY2, m_creature); break;
            case 2: DoScriptText(SAY_SLAY3, m_creature); break;
        }
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        DoScriptText(SAY_DEATH, m_creature);

        std::list<Creature*> PredatorNPC;
        GetCreatureListWithEntryInGrid(PredatorNPC, m_creature, NPC_SOUL_EATER, 100.0f);
        GetCreatureListWithEntryInGrid(PredatorNPC, m_creature, NPC_BELTHERIS, 100.0f);
        for (Creature* PredatorCrea : PredatorNPC)
            if (PredatorCrea)
                PredatorCrea->DespawnOrUnsummon();

        if (m_pInstance)
            m_pInstance->SetData(TYPE_MICHAEL, DONE);
    }

    void JustReachedHome()
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_MICHAEL, FAIL);
    }

    void DoKick(Unit* pTarget)
    {
        if (pTarget->HasAura(SPELL_THORNED_ROOTS) || pTarget->HasAura(SPELL_SOUL_CONTROL))
            return;

        if (Creature* Crea = pTarget->SummonCreature(NPC_SOUL, pTarget->GetPositionX(), pTarget->GetPositionY(), pTarget->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 25200))
        {
            m_creature->CastSpell(pTarget, SPELL_KICK, false);
            m_creature->getThreatManager().modifyThreatPercent(pTarget, -100);

            pTarget->CastSpell(Crea, SPELL_SOUL_CONTROL, true);
            pTarget->CastSpell(pTarget, SPELL_SOUL_CONTROL_KNOCK, true);
            DoScriptText(SAY_KICK, m_creature);

            std::list<Creature*> PredatorNPC;
            GetCreatureListWithEntryInGrid(PredatorNPC, m_creature, NPC_SOUL_EATER, 100.0f);
            for (Creature* PredatorCrea : PredatorNPC)
                if (PredatorCrea)
                {
                    PredatorCrea->RemoveAurasDueToSpell(35953);
                    PredatorCrea->getThreatManager().addThreat(Crea, 100000);
                    PredatorCrea->AddAura(35192);   // 2sec stun delay
                }

            Kick_Timer = 30000;
            if (Curse_Timer < 4000)
                Curse_Timer = 4000;
            if (Disarm_Timer < 4500)
                Disarm_Timer = 4500;
        }

    }

    void DoRoot(Unit* pTarget)
    {
        if (pTarget->HasAura(SPELL_SOUL_CONTROL))
            return;

        float tarX = pTarget->GetPositionX();
        float tarY = pTarget->GetPositionY();
        float tarZ = pTarget->GetPositionZ();

        const float minX = 527.5f;
        const float maxX = 555.0f;
        const float minY = 1324.5f;
        const float maxY = 1352.0f;

        bool targetInside =
            tarX >= minX && tarX <= maxX &&
            tarY >= minY && tarY <= maxY;

        if (targetInside)
        {
            m_creature->CastSpell(pTarget, SPELL_THORNED_ROOTS, true);
            Root_Timer = urand(10000, 16000);
        }
    }

    void UpdateAI(const uint32 diff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (Creature* Crea = m_creature->FindNearestCreature(NPC_BELTHERIS, 100.0f))
        {
            if (Crea->GetDistance(m_creature) > 1.0f)
                Crea->GetMotionMaster()->MoveFollow(m_creature->getVictim(), 0.0f, 0.0f);
        }

        if (PosCheckTimer < diff)
        {
            if (m_creature->GetPositionY() < 1313.0f)
                EnterEvadeMode();

            PosCheckTimer = 1000;
        }
        else PosCheckTimer -= diff;

        if (Kick_Timer < diff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, nullptr, SELECT_FLAG_PLAYER | SELECT_FLAG_IN_MELEE_RANGE | SELECT_FLAG_NOT_TARGET))
                DoKick(pTarget);
            else if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, nullptr, SELECT_FLAG_PLAYER | SELECT_FLAG_IN_MELEE_RANGE))
                DoKick(pTarget);
        }
        else Kick_Timer -= diff;

        if (Finger_Timer < diff)
        {
            m_creature->CastSpell(m_creature->getVictim(), SPELL_FOUR_FINGERS, true);
            Finger_Timer = 15000;
        }
        else Finger_Timer -= diff;

        if (Root_Timer < diff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, nullptr, SELECT_FLAG_PLAYER | SELECT_FLAG_NOT_IN_MELEE_RANGE | SELECT_FLAG_NOT_TARGET))
                DoRoot(pTarget);
            else if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, nullptr, SELECT_FLAG_PLAYER | SELECT_FLAG_NOT_TARGET))
                DoRoot(pTarget);
        }
        else Root_Timer -= diff;

        if (Curse_Timer < diff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_CURSE_OF_THORNS, SELECT_FLAG_MISSING_AURA))
            {
                if (!pTarget->HasAura(SPELL_THORNED_ROOTS) && !pTarget->HasAura(SPELL_SOUL_CONTROL))
                {
                    pTarget->AddAura(SPELL_CD_1_SEC);
                    m_creature->CastSpell(pTarget, SPELL_CURSE_OF_THORNS, true);
                    Curse_Timer = urand(3000, 6000);

                    if (Disarm_Timer < 3000)
                        Disarm_Timer = 3000;


                    if (!m_creature->HasAura(34823))
                    {
                        DoScriptText(SAY_CURSE, m_creature);
                        m_creature->CastSpell(m_creature, 34823, true);
                    }
                }
            }
        }
        else Curse_Timer -= diff;

        if (Fists_Timer < diff)
        {
            m_creature->CastSpell(m_creature, SPELL_FISTS_OF_FIRE, true);
            Fists_Timer = 30000;
        }
        else Fists_Timer -= diff;

        if (Disarm_Timer < diff)
        {
            m_creature->CastSpell(m_creature, SPELL_AOE_DISARM, true);
            Disarm_Timer = urand(12000, 18000);

            if (urand(0, 2) == 0)
                DoScriptText(SAY_DISARM, m_creature);
        }
        else Disarm_Timer -= diff;

        if (InnerFire_Timer < diff)
        {
            m_creature->CastSpell(m_creature, SPELL_INNER_FIRE, true);
            InnerFire_Timer = 30000;
        }
        else InnerFire_Timer -= diff;

        if (m_creature->GetHealthPercent() < 30.0f)
        {
            if (SoulLink_Timer < diff)
            {
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, nullptr))
                {
                    if (!pTarget->HasAura(SPELL_SOUL_CONTROL))
                    {
                        m_creature->CastSpell(pTarget, SPELL_SOUL_LINK, true);
                        SoulLink_Timer = urand(25000, 35000);
                    }
                }
            }
            else SoulLink_Timer -= diff;
        }


        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_michael(Creature* pCreature)
{
    return new boss_michaelAI(pCreature);
}

void AddSC_boss_michael()
{
    Script* newscript;
    newscript = new Script;
    newscript->Name = "boss_michael";
    newscript->GetAI = &GetAI_boss_michael;
    newscript->RegisterSelf();
}











struct mob_scarlet_traineeAI : ScriptedAI
{
    explicit mob_scarlet_traineeAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Start_Timer = urand(1000, 6000);
        group1 = false;
        group2 = false;
        mob_scarlet_traineeAI::Reset();
    }

    uint32 Start_Timer;
    bool group1;
    bool group2;

    void Reset() override { }

    void UpdateAI(const uint32 diff) override
    {
        if (Start_Timer)
        {
            if (Start_Timer <= diff)
            {
                m_creature->SetSpeedRate(MOVE_WALK, 2.20f);

                if (group1)
                    m_creature->GetMotionMaster()->MovePoint(1, 1946.433594f, -435.955109f, 16.367277f, MOVE_PATHFINDING);
                else if (group2)
                    m_creature->GetMotionMaster()->MovePoint(101, 1940.257080f, -434.454315f, 17.094456f, MOVE_PATHFINDING);

                Start_Timer = 0;
            }
            else
                Start_Timer -= diff;
        }

        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        DoMeleeAttackIfReady();
    }

    void MovementInform(uint32 MovementType, uint32 id) override
    {
        if (MovementType == POINT_MOTION_TYPE)
        {
            switch (id)
            {
                case 0:
                    group1 = true;
                    break;
                case 100:
                    group2 = true;
                    break;
                case 1:
                    m_creature->GetMotionMaster()->MovePoint(2, 1952.834717f, -447.514130f, 13.804327f);
                    break;
                case 101:
                    m_creature->GetMotionMaster()->MovePoint(102, 1953.056763f, -416.109863f, 13.861217f);
                    break;
                case 2:
                    m_creature->GetMotionMaster()->MovePoint(3, 1965.592041f, -451.153778f, 11.272284f);
                    break;
                case 102:
                    m_creature->GetMotionMaster()->MovePoint(103, 1965.369629f, -412.147949f, 11.272387f);
                    break;
                case 3:
                    m_creature->GetMotionMaster()->MovePoint(4, 1982.692749f, -441.514343f, 11.272284f);
                    break;
                case 103:
                    m_creature->GetMotionMaster()->MovePoint(104, 1980.908081f, -421.008026f, 11.272387f);
                    break;
                case 4:
                    m_creature->GetMotionMaster()->MovePoint(5, 1978.061890f, -428.549500f, 11.272232f);
                    break;
                case 104:
                    m_creature->GetMotionMaster()->MovePoint(105, 1979.139038f, -434.856934f, 11.272370f);
                    break;
                case 5:
                    m_creature->GetMotionMaster()->MovePoint(6, 1971.447144f, -419.629272f, 8.087179f);
                    break;
                case 105:
                    m_creature->GetMotionMaster()->MovePoint(106, 1972.044800f, -442.568573f, 8.434578f);
                    break;
                case 6:
                    m_creature->GetMotionMaster()->MovePoint(7, 1964.354004f, -418.632904f, 6.177466f);
                    break;
                case 106:
                    m_creature->GetMotionMaster()->MovePoint(107, 1964.691162f, -444.223022f, 6.177622f);
                    break;
                case 7:
                case 107:
                    m_creature->GetMotionMaster()->MovePoint(116, 1965.039795f, -431.733856f, 6.177539f);
                    break;
            }
        }
    }
};

CreatureAI* GetAI_mob_scarlet_trainee(Creature* pCreature)
{
    return new mob_scarlet_traineeAI(pCreature);
}

/*
 *
 */

struct go_herod_leverAI : GameObjectAI
{
    explicit go_herod_leverAI(GameObject* pGo) : GameObjectAI(pGo)
    {

    }

    bool OnUse(Unit* /*pCaster*/) override
    {
        if (auto pDoor = me->FindNearestGameObject(GO_HEROD_DOOR, 40.0f))
        {
             if (pDoor->getLootState() == GO_READY || pDoor->getLootState() == GO_JUST_DEACTIVATED)
                 pDoor->UseDoorOrButton();
             else
                 pDoor->ResetDoorOrButton();
        }

        return true;
    }
};

GameObjectAI* GetAI_go_herod_lever(GameObject* pGo)
{
    return new go_herod_leverAI(pGo);
}

void AddSC_boss_herod()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "boss_herod";
    newscript->GetAI = &GetAI_boss_herod;
    newscript->RegisterSelf();





    newscript = new Script;
    newscript->Name = "mob_scarlet_trainee";
    newscript->GetAI = &GetAI_mob_scarlet_trainee;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "go_herod_lever";
    newscript->GOGetAI = &GetAI_go_herod_lever;
    newscript->RegisterSelf();
}

