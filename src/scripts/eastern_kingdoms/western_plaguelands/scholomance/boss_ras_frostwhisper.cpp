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
SDName: Boss_Ras_Frostwhisper
SD%Complete: 100
SDComment:
SDCategory: Scholomance
EndScriptData */

#include "scriptPCH.h"

#define SPELL_FROSTBOLT         21369
#define SPELL_ICEARMOR          18100                       //This is actually a buff he gives himself
#define SPELL_FREEZE            18763
#define SPELL_FEAR              26070
#define SPELL_CHILLNOVA         18099
#define SPELL_FROSTVOLLEY       8398
#define SPELL_ICING             34378
#define SPELL_ICING_SOLID       34379

struct boss_rasfrostAI : public ScriptedAI
{
    boss_rasfrostAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 IceArmor_Timer;
    uint32 Frostbolt_Timer;
    uint32 Freeze_Timer;
    uint32 Fear_Timer;
    uint32 ChillNova_Timer;
    uint32 FrostVolley_Timer;
	uint32 Icing_Timer;

    void Reset()
    {
        IceArmor_Timer = 1000;
        Frostbolt_Timer = 8000;
        ChillNova_Timer = 18000;
        Freeze_Timer = 5000;
        FrostVolley_Timer = 24000;
        Fear_Timer = 15000;
		Icing_Timer = 10000;

        m_creature->CastSpell(m_creature, SPELL_ICEARMOR, true);
    }

    void UpdateAI(const uint32 diff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        //IceArmor_Timer
        if (IceArmor_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_ICEARMOR) == CAST_OK)
                IceArmor_Timer = 180000;
        }
        else IceArmor_Timer -= diff;

        //Icing_Timer
        if (Icing_Timer < diff)
        {
            Unit* itarget = NULL;
            itarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0);
            if (itarget && itarget != m_creature->getVictim() && !(itarget->HasAura(SPELL_ICING_SOLID) || itarget->HasAura(SPELL_FREEZE) || itarget->HasAura(SPELL_FEAR)))
                if (DoCastSpellIfCan(itarget, SPELL_ICING) == CAST_OK)
                    Icing_Timer = urand(8000, 10000);
        }
        else Icing_Timer -= diff;

        //ChillNova_Timer
        if (ChillNova_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_CHILLNOVA) == CAST_OK)
            {
                ChillNova_Timer = urand(20000, 28000);
                DoResetThreat(); //v+
            }
        }
        else ChillNova_Timer -= diff;

        //Fear_Timer
        if (Fear_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_FEAR) == CAST_OK)
            {
                Fear_Timer = 40000;
                if (Icing_Timer < 3000)
                    Icing_Timer = 3000;
            }
        }
        else Fear_Timer -= diff;

        //FrostVolley_Timer
        if (FrostVolley_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_FROSTVOLLEY) == CAST_OK)
                FrostVolley_Timer = 15000;
        }
        else FrostVolley_Timer -= diff;

        //Freeze_Timer
        if (Freeze_Timer < diff)
        {
            if (!m_creature->getVictim()->HasAura(SPELL_ICING_SOLID))
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_FREEZE) == CAST_OK)
                    Freeze_Timer = urand(22000, 24000);
            }
        }
        else Freeze_Timer -= diff;

        //Frostbolt_Timer
        if (Frostbolt_Timer < diff)
        {
            Unit* target = NULL;
            target = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0);
            if (target) DoCastSpellIfCan(target, SPELL_FROSTBOLT);

            Frostbolt_Timer = urand(3000, 8000);
        }
        else Frostbolt_Timer -= diff;

        DoMeleeAttackIfReady();
    }
};
CreatureAI* GetAI_boss_rasfrost(Creature* pCreature)
{
    return new boss_rasfrostAI(pCreature);
}

void AddSC_boss_rasfrost()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "boss_boss_ras_frostwhisper";
    newscript->GetAI = &GetAI_boss_rasfrost;
    newscript->RegisterSelf();
}
