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


#include "scriptPCH.h"
#include "blackwing_lair.h"

enum
{
    EMOTE_GENERIC_WING_BUFFET   = -1469032,
    EMOTE_GENERIC_SHADOW_FLAME  = -1469033,

    SPELL_ELEMENTAL_BLAST_NA    = 35268,
    SPELL_ELEMENTAL_BLAST_FI    = 35269,
    SPELL_ELEMENTAL_BLAST_FR    = 35270,
    SPELL_MIRRORS_SYSTEM        = 35271,
    SPELL_DISRUPTING_RAY        = 35272, //AoE Chain -armors
    SPELL_SHRINK_RAY            = 35273, //AoE long cast
    SPELL_DEATH_RAY             = 35274, //AoE furthest 3 random
    SPELL_GRAVITY_DEFEAT_PULL   = 35487,
    SPELL_GRAVITY_DEFEAT_KNOCK  = 35488,
    SPELL_ENLARGE               = 35489,
};

struct boss_tinkerAI : public ScriptedAI
{
    boss_tinkerAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 Elemental_BLast_Timer;
    uint32 Mirrors_System_Timer;
    uint32 Ele_Blast_Id;
    uint32 dis_ray_timer;
    uint32 shr_ray_timer;
    uint32 dea_ray_timer;
    uint32 RestoreTargetTimer;
    uint32 GravityDefeatPullTimer;
    uint32 GravityDefeatKnockTimer;
    uint32 DoResetThreatTimer;

    void Reset()
    {
        Elemental_BLast_Timer = 8000;
        Mirrors_System_Timer = 10000;
        Ele_Blast_Id = SPELL_ELEMENTAL_BLAST_NA;
        dis_ray_timer = 20000;
        shr_ray_timer = 35000;
        dea_ray_timer = 50000;
        RestoreTargetTimer = 0;
        GravityDefeatPullTimer = 37000;
        GravityDefeatKnockTimer = 39000;
        DoResetThreatTimer = 30 * MINUTE * IN_MILLISECONDS;
    }

    void Aggro(Unit* /*pWho*/)
    {
        if (m_pInstance->GetData(TYPE_LASHLAYER) != DONE)
        {
            EnterEvadeMode();
            return;
        }

        if (m_pInstance)
            m_pInstance->SetData(TYPE_GOBLIN, IN_PROGRESS);

        m_creature->SetInCombatWithZone();
    }

    void JustDied(Unit* /*pKiller*/)
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_GOBLIN, DONE);
    }

    void JustReachedHome()
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_GOBLIN, FAIL);
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        // Restore original target after casting Shrink Ray on someone
        if (RestoreTargetTimer)
        {
            if (RestoreTargetTimer <= uiDiff)
            {
                if (Unit* pTarget = m_creature->getVictim())
                {
                    m_creature->SetInFront(pTarget);
                    m_creature->SetTargetGuid(pTarget->GetObjectGuid());
                    RestoreTargetTimer = 0;
                }
            }
            else
                RestoreTargetTimer -= uiDiff;
        }

        if (Mirrors_System_Timer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_MIRRORS_SYSTEM) == CAST_OK)
            {
                Mirrors_System_Timer = 20000;
                m_creature->AddAura(SPELL_ENLARGE);
            }
        }
        else
            Mirrors_System_Timer -= uiDiff;

        if (GravityDefeatPullTimer < uiDiff)
        {
            m_creature->CastSpell(m_creature, SPELL_GRAVITY_DEFEAT_PULL, true);
            GravityDefeatPullTimer = 45000;
        }
        else
            GravityDefeatPullTimer -= uiDiff;

        if (GravityDefeatKnockTimer < uiDiff)
        {
            m_creature->CastSpell(m_creature, SPELL_GRAVITY_DEFEAT_KNOCK, true);
            GravityDefeatKnockTimer = 45000;
            DoResetThreatTimer = 1000;
        }
        else
            GravityDefeatKnockTimer -= uiDiff;

        if (DoResetThreatTimer < uiDiff)
        {
            DoResetThreat();

            if (Unit* closestPlayer = m_creature->GetNearestVictimInRange(0, 100.0f))
                m_creature->getThreatManager().addThreat(closestPlayer, 500);

            DoResetThreatTimer = 30 * MINUTE * IN_MILLISECONDS;
        }
        else
            DoResetThreatTimer -= uiDiff;


        if (dis_ray_timer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_DISRUPTING_RAY) == CAST_OK)
            {
                dis_ray_timer = 45000;
                shr_ray_timer = 15000;
                dea_ray_timer = 30000;
            }
        }
        else
            dis_ray_timer -= uiDiff;


        if (shr_ray_timer < uiDiff)
        {
            Unit* ot1 = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 1, nullptr);
            Unit* ot2 = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 2, nullptr);

            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_SHRINK_RAY, SELECT_FLAG_PLAYER | SELECT_FLAG_IN_LOS))
            {
                if (pTarget != ot1 && pTarget != ot2 && pTarget != m_creature->getVictim())
                {
                    if (DoCastSpellIfCan(pTarget, SPELL_SHRINK_RAY) == CAST_OK)
                    {
                        shr_ray_timer = 45000;
                        dea_ray_timer = 15000;
                        dis_ray_timer = 30000;

                        RestoreTargetTimer = 3200;
                        m_creature->SetInFront(pTarget);
                        m_creature->SetTargetGuid(pTarget->GetObjectGuid());
                    }
                }
            }
        }
        else
            shr_ray_timer -= uiDiff;


        if (dea_ray_timer < uiDiff)
        {
            if (Unit* rayTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_FARTHEST, (urand(0, 4)), SPELL_DEATH_RAY, SELECT_FLAG_PLAYER | SELECT_FLAG_IN_LOS))
            {
                if (DoCastSpellIfCan(rayTarget, SPELL_DEATH_RAY) == CAST_OK)
                {
                    m_creature->SetInFront(rayTarget);
                    m_creature->SetTargetGuid(rayTarget->GetObjectGuid());
                    RestoreTargetTimer = 1200;
                    dea_ray_timer = 45000;
                    dis_ray_timer = 15000;
                    shr_ray_timer = 30000;
                }
            }
        }
        else
            dea_ray_timer -= uiDiff;



        if (Elemental_BLast_Timer < uiDiff)
        {
            uint32 chanceN = urand(0, 2);
            if (chanceN == 0)
                Ele_Blast_Id = SPELL_ELEMENTAL_BLAST_NA;
            else if (chanceN == 1)
                Ele_Blast_Id = SPELL_ELEMENTAL_BLAST_FI;
            else if (chanceN == 2)
                Ele_Blast_Id = SPELL_ELEMENTAL_BLAST_FR;
                
            if (DoCastSpellIfCan(m_creature->getVictim(), Ele_Blast_Id) == CAST_OK)
            {
                Elemental_BLast_Timer = 2000;

                if (urand(0, 99) < 10)
                    m_creature->CastSpell(m_creature->getVictim(), Ele_Blast_Id, true);
            }
        }
        else
            Elemental_BLast_Timer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};
CreatureAI* GetAI_boss_tinker(Creature* pCreature)
{
    return new boss_tinkerAI(pCreature);
}

void AddSC_boss_ebonroc()
{
    Script* pNewScript;

    /*pNewScript = new Script;
    pNewScript->Name = "boss_ebonroc";
    pNewScript->GetAI = &GetAI_boss_ebonroc;
    pNewScript->RegisterSelf();*/

    pNewScript = new Script;
    pNewScript->Name = "boss_tinker";
    pNewScript->GetAI = &GetAI_boss_tinker;
    pNewScript->RegisterSelf();
}
