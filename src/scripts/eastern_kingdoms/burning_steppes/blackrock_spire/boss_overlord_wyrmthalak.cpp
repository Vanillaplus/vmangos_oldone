/* Copyright (C) 2006 - 2010 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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
SDName: Boss_Overlord_Wyrmthalak
SD%Complete: 100
SDComment:
SDCategory: Blackrock Spire
EndScriptData */

#include "scriptPCH.h"

enum
{
    SPELL_BLASTWAVE            = 20229,
    SPELL_SHOUT                = 23511,
    SPELL_CLEAVE               = 20691,
    SPELL_KNOCKAWAY            = 20686,
	SPELL_DISARM               = 13534,
	SPELL_POW_STUN             = 34424,

    NPC_SPIRESTONE_WARLORD       = 9216,
	NPC_SPIRESTONE_MYSTIC        = 9198,
    NPC_SMOLDERTHORN_BERSERKER   = 9268,
	NPC_SMOLDERTHORN_WITCHDOCTOR = 9266,
    NPC_BLOODAXE_RAIDER          = 9692,
    NPC_BLOODAXE_EVOKER          = 9693,

    WYRMTALAK_X        = -45,
    WYRMTALAK_Y        = -519

};

struct boss_overlordwyrmthalakAI : public ScriptedAI
{
    boss_overlordwyrmthalakAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiBlastWaveTimer;
    uint32 m_uiShoutTimer;
    /*uint32 m_uiCleaveTimer;*/
    uint32 m_uiKnockawayTimer;
    uint32 SummonCounter;
    bool berzerkspawned;
    bool m_bPulledByPet;
    uint32 m_uiLeashCheckTimer;
    uint32 m_uiEvadeRangeCheckTimer;
	uint32 m_uiDisarmTimer;
	uint32 m_uiPowStunTimer;

    void Reset()
    {
        m_uiBlastWaveTimer = 20000;
        m_uiShoutTimer     = 8000;
        /*m_uiCleaveTimer    = 6000;*/
        m_uiKnockawayTimer = 12000;
		m_uiDisarmTimer    = urand(10000, 15000);
		m_uiPowStunTimer   = urand(15000, 25000);
        SummonCounter = 1;
        berzerkspawned = false;
        m_bPulledByPet = false;

        m_uiLeashCheckTimer = 5000;
        m_uiEvadeRangeCheckTimer = 5000;
    }

    void JustSummoned(Creature* pSummoned)
    {
        if (pSummoned->GetEntry() != NPC_SPIRESTONE_WARLORD && pSummoned->GetEntry() != NPC_SPIRESTONE_MYSTIC 
            && pSummoned->GetEntry() != NPC_SMOLDERTHORN_WITCHDOCTOR && pSummoned->GetEntry() != NPC_SMOLDERTHORN_BERSERKER
            && pSummoned->GetEntry() != NPC_BLOODAXE_RAIDER && pSummoned->GetEntry() != NPC_BLOODAXE_EVOKER)
            return;

        if (m_creature->getVictim())
        {
            Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0);
            pSummoned->AI()->AttackStart(pTarget ? pTarget : m_creature->getVictim());
        }
    }

    void EnterCombat(Unit* pUnit) override
    {
        // Prevent exploit where pet can run through the wall and pull the boss.
        if (Unit* pOwner = pUnit->GetOwner())
            if (!pOwner->IsWithinLOSInMap(m_creature))
                m_bPulledByPet = true;

        ScriptedAI::EnterCombat(pUnit);
    }

    void LeashIfOutOfCombatArea(uint32 uiDiff)
    {
        if (m_uiLeashCheckTimer < uiDiff)
            m_uiLeashCheckTimer = 3500;
        else
        {
            m_uiLeashCheckTimer -= uiDiff;
            return;
        }

        if (m_bPulledByPet || (m_creature->GetPositionZ() > 100.0f))
            EnterEvadeMode();
    } 

    void SummonAdds()
    {
        uint32 spawnId;
        uint32 spawnrnd = urand(0, 99);
        float spawnlocX = -45.864f;
        float spawnlocY = -511.316f;
        float spawnlocZ = 88.465f;

        if (m_creature->GetMapId() == 229)
        {
            if (m_creature->GetPositionZ() < 82.0f)
            {
                spawnlocX = -54.004f;
                spawnlocY = -471.858f;
                spawnlocZ = 77.916f;
            }
        }

        if (spawnrnd < 20)
            spawnId = NPC_BLOODAXE_EVOKER;
        else if (spawnrnd < 40)
            spawnId = NPC_BLOODAXE_RAIDER;
        else if (spawnrnd < 60)
            spawnId = NPC_SMOLDERTHORN_WITCHDOCTOR;
        else if (spawnrnd < 80)
            spawnId = NPC_SPIRESTONE_MYSTIC;
        else if (spawnrnd < 90)
            spawnId = NPC_SPIRESTONE_WARLORD;
        else if (spawnrnd < 100 && !berzerkspawned)
            berzerkspawned = true,
            spawnId = NPC_SMOLDERTHORN_BERSERKER;

        m_creature->SummonCreature(spawnId, spawnlocX, spawnlocY, spawnlocZ, 4.6f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 5000);
    }

    void UpdateAI(const uint32 uiDiff)
    {
        // Return since we have no target
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        // Prevent players from pulling Wyrmthalak into UBRS
        LeashIfOutOfCombatArea(uiDiff);

		if (m_uiEvadeRangeCheckTimer < uiDiff)
		{
            if (m_creature->GetMapId() == 229)
            {
                if (m_creature->GetDistance2d(WYRMTALAK_X, WYRMTALAK_Y) > 60.0f)
                {
                    EnterEvadeMode();
                    return;
                }
            }
            m_uiEvadeRangeCheckTimer = 3000;
		}
		else
            m_uiEvadeRangeCheckTimer -= uiDiff;


		// STUN
		if (m_uiPowStunTimer < uiDiff)
		{
			DoCastSpellIfCan(m_creature->getVictim(), SPELL_POW_STUN);
			m_uiPowStunTimer = urand(25000, 35000);
		}
		else
			m_uiPowStunTimer -= uiDiff;

        // BlastWave
        if (m_uiBlastWaveTimer < uiDiff)
        {
            DoCastSpellIfCan(m_creature, SPELL_BLASTWAVE);
            m_uiBlastWaveTimer = 20000;
        }
        else
            m_uiBlastWaveTimer -= uiDiff;

		// DISARM
		if (m_uiDisarmTimer < uiDiff)
		{
			DoCastSpellIfCan(m_creature->getVictim(), SPELL_DISARM);
			m_uiDisarmTimer = urand(25000, 30000);
		}
		else
			m_uiDisarmTimer -= uiDiff;

        // Shout
        if (m_uiShoutTimer < uiDiff)
        {
            DoCastSpellIfCan(m_creature, SPELL_SHOUT);
            m_uiShoutTimer = urand(30000, 40000);
        }
        else
            m_uiShoutTimer -= uiDiff;

        // Cleave
        /*if (m_uiCleaveTimer < uiDiff)
        {
            DoCastSpellIfCan(m_creature->getVictim(), SPELL_CLEAVE);
            m_uiCleaveTimer = urand(5000, 8000);
        }
        else
            m_uiCleaveTimer -= uiDiff;*/

        // Knockaway
        /*if (m_uiKnockawayTimer < uiDiff)
        {
            DoCastSpellIfCan(m_creature, SPELL_KNOCKAWAY);
            m_uiKnockawayTimer = urand(25000, 35000);
        }
        else
            m_uiKnockawayTimer -= uiDiff;*/

        // Summon mobs
        if (m_creature->GetHealthPercent() < (100.0f - SummonCounter * 15.0f))
        {
            SummonAdds();
            ++SummonCounter;
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_overlordwyrmthalak(Creature* pCreature)
{
    return new boss_overlordwyrmthalakAI(pCreature);
}

void AddSC_boss_overlordwyrmthalak()
{
    Script* newscript;
    newscript = new Script;
    newscript->Name = "boss_overlord_wyrmthalak";
    newscript->GetAI = &GetAI_boss_overlordwyrmthalak;
    newscript->RegisterSelf();
}
