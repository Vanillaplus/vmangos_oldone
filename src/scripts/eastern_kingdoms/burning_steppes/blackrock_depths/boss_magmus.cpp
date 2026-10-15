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
SDName: Boss_Magmus
SD%Complete: 80
SDComment: Missing pre-event to open doors
SDCategory: Blackrock Depths
EndScriptData */

#include "scriptPCH.h"
#include "blackrock_depths.h"

enum
{
    SPELL_FIERYBURST        = 13900,
    SPELL_WARSTOMP          = 24375,
	SPELL_LAVABOMB_DUMMY    = 19411,
	SPELL_LAVABOMB_ALT      = 34418,                 //This is the spell that the lava bomb casts
	SPELL_GOUT              = 15529,


	NPC_LAVABOMB            = 20007,
	NPC_IRONHAND            = 8982,
	NPC_IRONHAND_MID        = 8984,
    MAGMUS_X                = 1380,
    MAGMUS_Y                = -660

};

struct boss_magmusAI : public ScriptedAI
{
    boss_magmusAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Engaged = false;
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 m_uiFieryBurst_Timer;
    uint32 m_uiWarStomp_Timer;
	uint32 m_uiLavaBombTriggerTimer;
	uint32 m_uiRestoreTargetTimer;
	uint32 m_uiIronhandTimer;
	uint32 m_uiIronhand_MID_Timer;
    bool Engaged;

    void Reset()
    {
        m_uiFieryBurst_Timer = 5000;
        m_uiWarStomp_Timer = 0;
		m_uiLavaBombTriggerTimer = 0;
		m_uiRestoreTargetTimer = 0;
		m_uiIronhandTimer = 25000;
		m_uiIronhand_MID_Timer = 5000;

        if (Engaged)
            m_pInstance->SetData(TYPE_IRON_HALL, FAIL);
        Engaged = false;
    }

	void MoveInLineOfSight(Unit* pWho) override
	{
		// has an extremely large aggro radius
		if (pWho->GetTypeId() == TYPEID_PLAYER && !m_creature->isInCombat() && m_creature->IsWithinDistInMap(pWho, 99.0f) && !pWho->HasAuraType(SPELL_AURA_FEIGN_DEATH))
		{
			AttackStart(pWho);
		}
		ScriptedAI::MoveInLineOfSight(pWho);
	}

    void Aggro(Unit* pWho)
    {
        Engaged = true;
        if (m_pInstance)
            m_pInstance->SetData(TYPE_IRON_HALL, IN_PROGRESS);

    }

    void JustDied(Unit* pVictim)
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_IRON_HALL, DONE);
        {
            std::list<Creature*> IronListe;
            GetCreatureListWithEntryInGrid(IronListe, m_creature, NPC_IRONHAND, 120.0f);
            for (std::list<Creature*>::iterator itr = IronListe.begin(); itr != IronListe.end(); ++itr)
                if ((*itr)->HasAura(SPELL_GOUT))
                    (*itr)->RemoveAurasDueToSpell(SPELL_GOUT);
        }

        {
            std::list<Creature*> IronListe_MID;
            GetCreatureListWithEntryInGrid(IronListe_MID, m_creature, NPC_IRONHAND_MID, 120.0f);
            for (std::list<Creature*>::iterator itr = IronListe_MID.begin(); itr != IronListe_MID.end(); ++itr)
                if ((*itr)->HasAura(SPELL_GOUT))
                    (*itr)->RemoveAurasDueToSpell(SPELL_GOUT);
        }
    }

    void UpdateAI(const uint32 uiDiff)
    {
        //Return since we have no target
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_creature->GetMapId() == 230)
        {
            if (m_creature->GetDistance2d(MAGMUS_X, MAGMUS_Y) > 100.0f)
            {
                EnterEvadeMode();
                return;
            }
        }

        //FieryBurst_Timer
		if (m_uiFieryBurst_Timer < uiDiff)
		{
			if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, nullptr, SELECT_FLAG_PLAYER))
			{
				if (DoCastSpellIfCan(pTarget, SPELL_FIERYBURST) == CAST_OK)
				{
					m_creature->CastSpell((pTarget), SPELL_LAVABOMB_DUMMY, true);
					m_creature->SetInFront(pTarget);
					m_creature->SetTargetGuid(pTarget->GetObjectGuid());
					if (Creature* Cre = m_creature->SummonCreature(NPC_LAVABOMB,
						pTarget->GetPositionX(),
						pTarget->GetPositionY(),
						pTarget->GetPositionZ(),
						pTarget->GetOrientation(),
						TEMPSUMMON_TIMED_DESPAWN,
						90000))
					{
						//scale at 0 is necessary for spell animation
						Cre->SetObjectScale(0.0f);
						Cre->setFaction(m_creature->getFaction());
					}
					m_uiFieryBurst_Timer = urand(10000, 14000);
					m_uiLavaBombTriggerTimer = 1500;
					m_uiRestoreTargetTimer = 1500;
				}
			}
		}
        else
            m_uiFieryBurst_Timer -= uiDiff;


		// every second force Lava Bomb triggers to cast Conflagrate
		// don't refresh faster or Conflag does no damage to players sitting in the fire
		if (m_uiLavaBombTriggerTimer)
		{
			if (m_uiLavaBombTriggerTimer < uiDiff)
			{
				std::list<Creature*> CreListe;
				GetCreatureListWithEntryInGrid(CreListe, m_creature, NPC_LAVABOMB, 100.0f);
				for (std::list<Creature*>::iterator itr = CreListe.begin(); itr != CreListe.end(); ++itr)
					(*itr)->CastSpell((*itr), SPELL_LAVABOMB_ALT, false);
				m_uiLavaBombTriggerTimer = 1150;
			}
			else
				m_uiLavaBombTriggerTimer -= uiDiff;
		}

		// restore target after casting a Lava Bomb/FieryBurst
		if (m_uiRestoreTargetTimer)
		{
			if (m_uiRestoreTargetTimer <= uiDiff)
			{
				m_creature->SetInFront(m_creature->getVictim());
				m_creature->SetTargetGuid(m_creature->getVictim()->GetObjectGuid());
				m_uiRestoreTargetTimer = 0;
			}
			else
				m_uiRestoreTargetTimer -= uiDiff;
		}

		//Ironhand Guardian's AoE
		if (m_uiIronhandTimer < uiDiff)
		{
				std::list<Creature*> IronListe;
				GetCreatureListWithEntryInGrid(IronListe, m_creature, NPC_IRONHAND, 120.0f);
				for (std::list<Creature*>::iterator itr = IronListe.begin(); itr != IronListe.end(); ++itr)
					(*itr)->CastSpell((*itr), SPELL_GOUT, false);
				m_uiIronhandTimer = 40000;
		}
			else
				m_uiIronhandTimer -= uiDiff;

		if (m_uiIronhand_MID_Timer < uiDiff)
		{
				std::list<Creature*> IronListe_MID;
				GetCreatureListWithEntryInGrid(IronListe_MID, m_creature, NPC_IRONHAND_MID, 120.0f);
				for (std::list<Creature*>::iterator itr = IronListe_MID.begin(); itr != IronListe_MID.end(); ++itr)
					(*itr)->CastSpell((*itr), SPELL_GOUT, false);
				m_uiIronhand_MID_Timer = 40000;
		}
			else
				m_uiIronhand_MID_Timer -= uiDiff;

        //WarStomp_Timer
        if (m_creature->GetHealthPercent() < 51.0f)
        {
            if (m_uiWarStomp_Timer < uiDiff)
            {
                DoCastSpellIfCan(m_creature->getVictim(), SPELL_WARSTOMP);
                m_uiWarStomp_Timer = 25000;
            }
            else
                m_uiWarStomp_Timer -= uiDiff;
        }

        DoMeleeAttackIfReady();
    }

	void UpdateAI_corpse(const uint32 diff)
	{
		if (m_uiLavaBombTriggerTimer)
		{
			if (m_uiLavaBombTriggerTimer <= diff)
			{
				bool m_bAllDespawned = true;

				std::list<Creature*> CreListe;
				GetCreatureListWithEntryInGrid(CreListe, m_creature, NPC_LAVABOMB, 100.0f);
				for (std::list<Creature*>::iterator itr = CreListe.begin(); itr != CreListe.end(); ++itr)
				{
					(*itr)->CastSpell((*itr), SPELL_LAVABOMB_ALT, false);
					m_bAllDespawned = false;
				}
				m_uiLavaBombTriggerTimer = m_bAllDespawned ? 0 : 1125;
			}
			else
				m_uiLavaBombTriggerTimer -= diff;
		}
	}
};

CreatureAI* GetAI_boss_magmus(Creature* pCreature)
{
    return new boss_magmusAI(pCreature);
}

void AddSC_boss_magmus()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "boss_magmus";
    newscript->GetAI = &GetAI_boss_magmus;
    newscript->RegisterSelf();
}
