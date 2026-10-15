/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 * Copyright (C) 2009-2011 MaNGOSZero <https://github.com/mangos/zero>
 * Copyright (C) 2011-2016 Nostalrius <https://nostalrius.org>
 * Copyright (C) 2016-2017 Elysium Project <https://github.com/elysium-project>
 *
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

#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Log.h"
#include "UpdateMask.h"
#include "World.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "Unit.h"
#include "Spell.h"
#include "DynamicObject.h"
#include "SpellAuras.h"
#include "Group.h"
#include "UpdateData.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "SharedDefines.h"
#include "Pet.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "GossipDef.h"
#include "Creature.h"
#include "Totem.h"
#include "CreatureAI.h"
#include "BattleGroundMgr.h"
#include "BattleGround.h"
#include "BattleGroundWS.h"
#include "Language.h"
#include "SocialMgr.h"
#include "VMapFactory.h"
#include "Util.h"
#include "TemporarySummon.h"
#include "MoveMapSharedDefines.h"
#include "GameEventMgr.h"
#include "InstanceData.h"
#include "ScriptMgr.h"
#include "scriptPCH.h"
#include "Chat.h"

using namespace Spells;

pEffect SpellEffects[TOTAL_SPELL_EFFECTS] =
{
	&Spell::EffectNULL,                                     //  0
	&Spell::EffectInstaKill,                                //  1 SPELL_EFFECT_INSTAKILL
	&Spell::EffectSchoolDMG,                                //  2 SPELL_EFFECT_SCHOOL_DAMAGE
	&Spell::EffectDummy,                                    //  3 SPELL_EFFECT_DUMMY
	&Spell::EffectUnused,                                   //  4 SPELL_EFFECT_PORTAL_TELEPORT          unused from pre-1.2.1
	&Spell::EffectTeleportUnits,                            //  5 SPELL_EFFECT_TELEPORT_UNITS
	&Spell::EffectApplyAura,                                //  6 SPELL_EFFECT_APPLY_AURA
	&Spell::EffectEnvironmentalDMG,                         //  7 SPELL_EFFECT_ENVIRONMENTAL_DAMAGE
	&Spell::EffectPowerDrain,                               //  8 SPELL_EFFECT_POWER_DRAIN
	&Spell::EffectHealthLeech,                              //  9 SPELL_EFFECT_HEALTH_LEECH
	&Spell::EffectHeal,                                     // 10 SPELL_EFFECT_HEAL
	&Spell::EffectBind,                                     // 11 SPELL_EFFECT_BIND
	&Spell::EffectUnused,                                   // 12 SPELL_EFFECT_PORTAL                   unused from pre-1.2.1, exist 2 spell, but not exist any data about its real usage
	&Spell::EffectUnused,                                   // 13 SPELL_EFFECT_RITUAL_BASE              unused from pre-1.2.1
	&Spell::EffectUnused,                                   // 14 SPELL_EFFECT_RITUAL_SPECIALIZE        unused from pre-1.2.1
	&Spell::EffectUnused,                                   // 15 SPELL_EFFECT_RITUAL_ACTIVATE_PORTAL   unused from pre-1.2.1
	&Spell::EffectQuestComplete,                            // 16 SPELL_EFFECT_QUEST_COMPLETE
	&Spell::EffectWeaponDmg,                                // 17 SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL
	&Spell::EffectResurrect,                                // 18 SPELL_EFFECT_RESURRECT
	&Spell::EffectAddExtraAttacks,                          // 19 SPELL_EFFECT_ADD_EXTRA_ATTACKS
	&Spell::EffectEmpty,                                    // 20 SPELL_EFFECT_DODGE                    one spell: Dodge
	&Spell::EffectEmpty,                                    // 21 SPELL_EFFECT_EVADE                    one spell: Evade (DND)
	&Spell::EffectParry,                                    // 22 SPELL_EFFECT_PARRY
	&Spell::EffectBlock,                                    // 23 SPELL_EFFECT_BLOCK                    one spell: Block
	&Spell::EffectCreateItem,                               // 24 SPELL_EFFECT_CREATE_ITEM
	&Spell::EffectEmpty,                                    // 25 SPELL_EFFECT_WEAPON                   spell per weapon type, in ItemSubclassmask store mask that can be used for usability check at equip, but current way using skill also work.
	&Spell::EffectEmpty,                                    // 26 SPELL_EFFECT_DEFENSE                  one spell: Defense
	&Spell::EffectPersistentAA,                             // 27 SPELL_EFFECT_PERSISTENT_AREA_AURA
	&Spell::EffectSummon,                                   // 28 SPELL_EFFECT_SUMMON
	&Spell::EffectLeapForward,                              // 29 SPELL_EFFECT_LEAP
	&Spell::EffectEnergize,                                 // 30 SPELL_EFFECT_ENERGIZE
	&Spell::EffectWeaponDmg,                                // 31 SPELL_EFFECT_WEAPON_PERCENT_DAMAGE
	&Spell::EffectTriggerMissileSpell,                      // 32 SPELL_EFFECT_TRIGGER_MISSILE
	&Spell::EffectOpenLock,                                 // 33 SPELL_EFFECT_OPEN_LOCK
	&Spell::EffectSummonChangeItem,                         // 34 SPELL_EFFECT_SUMMON_CHANGE_ITEM
	&Spell::EffectApplyAreaAura,                            // 35 SPELL_EFFECT_APPLY_AREA_AURA_PARTY
	&Spell::EffectLearnSpell,                               // 36 SPELL_EFFECT_LEARN_SPELL
	&Spell::EffectEmpty,                                    // 37 SPELL_EFFECT_SPELL_DEFENSE            one spell: SPELLDEFENSE (DND)
	&Spell::EffectDispel,                                   // 38 SPELL_EFFECT_DISPEL
	&Spell::EffectEmpty,                                    // 39 SPELL_EFFECT_LANGUAGE                 misc store lang id
	&Spell::EffectDualWield,                                // 40 SPELL_EFFECT_DUAL_WIELD
	&Spell::EffectSummonWild,                               // 41 SPELL_EFFECT_SUMMON_WILD
	&Spell::EffectSummonGuardian,                           // 42 SPELL_EFFECT_SUMMON_GUARDIAN
	&Spell::EffectTeleUnitsFaceCaster,                      // 43 SPELL_EFFECT_TELEPORT_UNITS_FACE_CASTER
	&Spell::EffectLearnSkill,                               // 44 SPELL_EFFECT_SKILL_STEP
	&Spell::EffectAddHonor,                                 // 45 SPELL_EFFECT_ADD_HONOR                honor/pvp related
	&Spell::EffectSpawn,                                    // 46 SPELL_EFFECT_SPAWN                    spawn/login animation, expected by spawn unit cast, also base points store some dynflags
	&Spell::EffectTradeSkill,                               // 47 SPELL_EFFECT_TRADE_SKILL
	&Spell::EffectUnused,                                   // 48 SPELL_EFFECT_STEALTH                  one spell: Base Stealth
	&Spell::EffectUnused,                                   // 49 SPELL_EFFECT_DETECT                   one spell: Detect
	&Spell::EffectTransmitted,                              // 50 SPELL_EFFECT_TRANS_DOOR
	&Spell::EffectUnused,                                   // 51 SPELL_EFFECT_FORCE_CRITICAL_HIT       unused from pre-1.2.1
	&Spell::EffectUnused,                                   // 52 SPELL_EFFECT_GUARANTEE_HIT            unused from pre-1.2.1
	&Spell::EffectEnchantItemPerm,                          // 53 SPELL_EFFECT_ENCHANT_ITEM
	&Spell::EffectEnchantItemTmp,                           // 54 SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY
	&Spell::EffectTameCreature,                             // 55 SPELL_EFFECT_TAMECREATURE
	&Spell::EffectSummonPet,                                // 56 SPELL_EFFECT_SUMMON_PET
	&Spell::EffectLearnPetSpell,                            // 57 SPELL_EFFECT_LEARN_PET_SPELL
	&Spell::EffectWeaponDmg,                                // 58 SPELL_EFFECT_WEAPON_DAMAGE
	&Spell::EffectOpenLock,                                 // 59 SPELL_EFFECT_OPEN_LOCK_ITEM
	&Spell::EffectProficiency,                              // 60 SPELL_EFFECT_PROFICIENCY
	&Spell::EffectSendEvent,                                // 61 SPELL_EFFECT_SEND_EVENT
	&Spell::EffectPowerBurn,                                // 62 SPELL_EFFECT_POWER_BURN
	&Spell::EffectThreat,                                   // 63 SPELL_EFFECT_THREAT
	&Spell::EffectTriggerSpell,                             // 64 SPELL_EFFECT_TRIGGER_SPELL
	&Spell::EffectUnused,                                   // 65 SPELL_EFFECT_HEALTH_FUNNEL            unused
	&Spell::EffectUnused,                                   // 66 SPELL_EFFECT_POWER_FUNNEL             unused from pre-1.2.1
	&Spell::EffectHealMaxHealth,                            // 67 SPELL_EFFECT_HEAL_MAX_HEALTH
	&Spell::EffectInterruptCast,                            // 68 SPELL_EFFECT_INTERRUPT_CAST
	&Spell::EffectDistract,                                 // 69 SPELL_EFFECT_DISTRACT
	&Spell::EffectPull,                                     // 70 SPELL_EFFECT_PULL                     one spell: Distract Move
	&Spell::EffectPickPocket,                               // 71 SPELL_EFFECT_PICKPOCKET
	&Spell::EffectAddFarsight,                              // 72 SPELL_EFFECT_ADD_FARSIGHT
	&Spell::EffectSummonPossessed,                          // 73 SPELL_EFFECT_SUMMON_POSSESSED
	&Spell::EffectSummonTotem,                              // 74 SPELL_EFFECT_SUMMON_TOTEM
	&Spell::EffectHealMechanical,                           // 75 SPELL_EFFECT_HEAL_MECHANICAL          one spell: Mechanical Patch Kit
	&Spell::EffectSummonObjectWild,                         // 76 SPELL_EFFECT_SUMMON_OBJECT_WILD
	&Spell::EffectScriptEffect,                             // 77 SPELL_EFFECT_SCRIPT_EFFECT
	&Spell::EffectUnused,                                   // 78 SPELL_EFFECT_ATTACK
	&Spell::EffectSanctuary,                                // 79 SPELL_EFFECT_SANCTUARY
	&Spell::EffectAddComboPoints,                           // 80 SPELL_EFFECT_ADD_COMBO_POINTS
	&Spell::EffectUnused,                                   // 81 SPELL_EFFECT_CREATE_HOUSE             one spell: Create House (TEST)
	&Spell::EffectNULL,                                     // 82 SPELL_EFFECT_BIND_SIGHT
	&Spell::EffectDuel,                                     // 83 SPELL_EFFECT_DUEL
	&Spell::EffectStuck,                                    // 84 SPELL_EFFECT_STUCK
	&Spell::EffectSummonPlayer,                             // 85 SPELL_EFFECT_SUMMON_PLAYER
	&Spell::EffectActivateObject,                           // 86 SPELL_EFFECT_ACTIVATE_OBJECT
	&Spell::EffectSummonTotem,                              // 87 SPELL_EFFECT_SUMMON_TOTEM_SLOT1
	&Spell::EffectSummonTotem,                              // 88 SPELL_EFFECT_SUMMON_TOTEM_SLOT2
	&Spell::EffectSummonTotem,                              // 89 SPELL_EFFECT_SUMMON_TOTEM_SLOT3
	&Spell::EffectSummonTotem,                              // 90 SPELL_EFFECT_SUMMON_TOTEM_SLOT4
	&Spell::EffectUnused,                                   // 91 SPELL_EFFECT_THREAT_ALL               one spell: zzOLDBrainwash
	&Spell::EffectEnchantHeldItem,                          // 92 SPELL_EFFECT_ENCHANT_HELD_ITEM
	&Spell::EffectUnused,                                   // 93 SPELL_EFFECT_SUMMON_PHANTASM
	&Spell::EffectSelfResurrect,                            // 94 SPELL_EFFECT_SELF_RESURRECT
	&Spell::EffectSkinning,                                 // 95 SPELL_EFFECT_SKINNING
	&Spell::EffectCharge,                                   // 96 SPELL_EFFECT_CHARGE
	&Spell::EffectSummonCritter,                            // 97 SPELL_EFFECT_SUMMON_CRITTER
	&Spell::EffectKnockBack,                                // 98 SPELL_EFFECT_KNOCK_BACK
	&Spell::EffectDisEnchant,                               // 99 SPELL_EFFECT_DISENCHANT
	&Spell::EffectInebriate,                                //100 SPELL_EFFECT_INEBRIATE
	&Spell::EffectFeedPet,                                  //101 SPELL_EFFECT_FEED_PET
	&Spell::EffectDismissPet,                               //102 SPELL_EFFECT_DISMISS_PET
	&Spell::EffectReputation,                               //103 SPELL_EFFECT_REPUTATION
	&Spell::EffectSummonObject,                             //104 SPELL_EFFECT_SUMMON_OBJECT_SLOT1
	&Spell::EffectSummonObject,                             //105 SPELL_EFFECT_SUMMON_OBJECT_SLOT2
	&Spell::EffectSummonObject,                             //106 SPELL_EFFECT_SUMMON_OBJECT_SLOT3
	&Spell::EffectSummonObject,                             //107 SPELL_EFFECT_SUMMON_OBJECT_SLOT4
	&Spell::EffectDispelMechanic,                           //108 SPELL_EFFECT_DISPEL_MECHANIC
	&Spell::EffectSummonDeadPet,                            //109 SPELL_EFFECT_SUMMON_DEAD_PET
	&Spell::EffectDestroyAllTotems,                         //110 SPELL_EFFECT_DESTROY_ALL_TOTEMS
	&Spell::EffectDurabilityDamage,                         //111 SPELL_EFFECT_DURABILITY_DAMAGE
	&Spell::EffectSummonDemon,                              //112 SPELL_EFFECT_SUMMON_DEMON
	&Spell::EffectResurrectNew,                             //113 SPELL_EFFECT_RESURRECT_NEW
	&Spell::EffectTaunt,                                    //114 SPELL_EFFECT_ATTACK_ME
	&Spell::EffectDurabilityDamagePCT,                      //115 SPELL_EFFECT_DURABILITY_DAMAGE_PCT
	&Spell::EffectSkinPlayerCorpse,                         //116 SPELL_EFFECT_SKIN_PLAYER_CORPSE       one spell: Remove Insignia, bg usage, required special corpse flags...
	&Spell::EffectSpiritHeal,                               //117 SPELL_EFFECT_SPIRIT_HEAL              one spell: Spirit Heal
	&Spell::EffectSkill,                                    //118 SPELL_EFFECT_SKILL                    professions and more
	&Spell::EffectApplyAreaAura,                            //119 SPELL_EFFECT_APPLY_AREA_AURA_PET
	&Spell::EffectUnused,                                   //120 SPELL_EFFECT_TELEPORT_GRAVEYARD       one spell: Graveyard Teleport Test
	&Spell::EffectWeaponDmg,                                //121 SPELL_EFFECT_NORMALIZED_WEAPON_DMG
	&Spell::EffectUnused,                                   //122 SPELL_EFFECT_122                      unused
	&Spell::EffectSendTaxi,                                 //123 SPELL_EFFECT_SEND_TAXI                taxi/flight related (misc value is taxi path id)
	&Spell::EffectPlayerPull,                               //124 SPELL_EFFECT_PLAYER_PULL              opposite of knockback effect (pulls player twoard caster)
	&Spell::EffectModifyThreatPercent,                      //125 SPELL_EFFECT_MODIFY_THREAT_PERCENT
	&Spell::EffectUnused,                                   //126 SPELL_EFFECT_126                      future spell steal effect? now only used one test spell
	&Spell::EffectUnused,                                   //127 SPELL_EFFECT_127                      future Prospecting spell, not have spells
	&Spell::EffectApplyAreaAura,                            //128 SPELL_EFFECT_APPLY_AREA_AURA_FRIEND
	&Spell::EffectApplyAreaAura,                            //129 SPELL_EFFECT_APPLY_AREA_AURA_ENEMY
	&Spell::EffectDespawnObject,                            //130 SPELL_EFFECT_DESPAWN_OBJECT
	&Spell::EffectNostalrius,                               //131 SPELL_EFFECT_NOSTALRIUS
	&Spell::EffectApplyAreaAura,                            //132 SPELL_EFFECT_APPLY_AREA_AURA_RAID
	&Spell::EffectUnused
};

void Spell::EffectEmpty(SpellEffectIndex /*eff_idx*/)
{
	// NOT NEED ANY IMPLEMENTATION CODE, EFFECT POSISBLE USED AS MARKER OR CLIENT INFORM
}

void Spell::EffectNULL(SpellEffectIndex /*eff_idx*/)
{
	DEBUG_LOG("WORLD: Spell Effect DUMMY");
}

void Spell::EffectUnused(SpellEffectIndex /*eff_idx*/)
{
	// NOT USED BY ANY SPELL OR USELESS OR IMPLEMENTED IN DIFFERENT WAY IN MANGOS
}

void Spell::EffectResurrectNew(SpellEffectIndex eff_idx)
{
	if (!unitTarget || unitTarget->isAlive())
		return;

	if (!unitTarget->IsInWorld())
		return;

	if (unitTarget->GetTypeId() != TYPEID_PLAYER)
	{
		// Pet case
		Pet* pet = unitTarget->ToPet();
		if (!pet)
			return;
		Unit* owner = pet->GetOwner();
		if (!owner)
			return;
		uint32 health = damage;

		pet->NearTeleportTo(m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ(), m_caster->GetOrientation(), false);
		pet->SetUInt32Value(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_NONE);
		pet->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);
		pet->SetDeathState(ALIVE);
		pet->clearUnitState(UNIT_STAT_ALL_DYN_STATES);
		pet->SetHealth(pet->GetMaxHealth() > health ? health : pet->GetMaxHealth());

		pet->AIM_Initialize();

		// m_caster->PetSpellInitialize(); -- action bar not removed at death and not required send at revive
		pet->SavePetToDB(PET_SAVE_AS_CURRENT);

		// Remove Demonic Sacrifice auras (Blizzlike - cf patchnote 1.12)
		//Unit::AuraList const& auraClassScripts = owner->GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
		//for (Unit::AuraList::const_iterator itr = auraClassScripts.begin(); itr != auraClassScripts.end();)
		//{
		//	if ((*itr)->GetModifier()->m_miscvalue == 2228)
		//	{
		//		owner->RemoveAurasDueToSpell((*itr)->GetId());
		//		itr = auraClassScripts.begin();
		//	}
		//	else
		//		++itr;
		//}
		return;
	}

	Player* pTarget = ((Player*)unitTarget);

	if (pTarget->IsRessurectRequested())      // already have one active request
		return;

	uint32 health = damage;
	uint32 mana = m_spellInfo->EffectMiscValue[eff_idx];
	pTarget->SetResurrectRequestData(m_caster->GetObjectGuid(), m_caster->GetMapId(), m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ(), health, mana);
	SendResurrectRequest(pTarget);

	AddExecuteLogInfo(eff_idx, ExecuteLogInfo(unitTarget->GetObjectGuid()));
}

void Spell::EffectInstaKill(SpellEffectIndex /*eff_idx*/)
{
	if (!unitTarget || !unitTarget->isAlive())
		return;

	// Demonic Sacrifice
	if (m_spellInfo->Id == 18788 && unitTarget->IsCreature() && m_casterUnit)
	{
		uint32 entry = unitTarget->GetEntry();
		uint32 spellID;
		switch (entry)
		{
		case   416:
			spellID = 18789;
			break;               //imp
		case   417:
			spellID = 18792;
			break;               //fellhunter
		case  1860:
			spellID = 18790;
			break;               //void
		case  1863:
			spellID = 18791;
			break;               //succubus
		default:
			sLog.outError("EffectInstaKill: Unhandled creature entry (%u) case.", entry);
			return;
		}

		m_casterUnit->CastSpell(m_casterUnit, spellID, true);

		if (entry != 416)
			m_casterUnit->CastSpell(m_casterUnit, 23464, true);
	}

	if (m_caster == unitTarget)                             // prevent interrupt message
		finish();

	WorldObject* caster = GetCastingObject();               // we need the original casting object

#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_8_4
	WorldPacket data(SMSG_SPELLINSTAKILLLOG, (8 + 4));
	data << unitTarget->GetObjectGuid();                    // Victim GUID
	data << uint32(m_spellInfo->Id);
	m_caster->SendMessageToSet(&data, true);
#endif

	m_caster->DealDamage(unitTarget, unitTarget->GetHealth(), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
}

void Spell::EffectEnvironmentalDMG(SpellEffectIndex eff_idx)
{
	if (!unitTarget || !unitTarget->isAlive())
		return;

	//% Dmg based on base HP
	damage *= unitTarget->GetMaxHealth() / 100;

	if (unitTarget->GetTypeId() == TYPEID_PLAYER)
		((Player*)unitTarget)->EnvironmentalDamage(DAMAGE_FIRE, damage);
	else
	{
		uint32 absorb = 0;
		int32 resist = 0;
		unitTarget->CalculateDamageAbsorbAndResist(m_caster, m_spellInfo->GetSpellSchoolMask(), SPELL_DIRECT_DAMAGE, damage, &absorb, &resist, m_spellInfo);
		m_caster->SendSpellNonMeleeDamageLog(unitTarget, m_spellInfo->Id, damage, m_spellInfo->GetSpellSchoolMask(), absorb, resist, false, 0, false);

	}
}

void Spell::EffectSchoolDMG(SpellEffectIndex effect_idx)
{
	if (unitTarget && unitTarget->isAlive())
	{
		switch (m_spellInfo->SpellFamilyName)
		{
		case SPELLFAMILY_GENERIC:
		{
			switch (m_spellInfo->Id)                    // better way to check unknown
			{
			case 36231:	//Holy Water Flask
			{
				if (unitTarget->GetCreatureType() == CREATURE_TYPE_UNDEAD || unitTarget->GetCreatureType() == CREATURE_TYPE_DEMON)
					damage = damage * 3 + urand(0, 2);

				break;
			}
			case 29324:
			{
				if (unitTarget != m_casterUnit)
				{
					if (unitTarget->GetMaxHealth() < damage || m_casterUnit->getLevel() - unitTarget->getLevel() >= 10)
						damage = damage / 10;
				}

				break;
			}
			case 36062:	// Kick/Knockout
			{
				m_casterUnit->CastSpell(unitTarget, 36063, true);

				if (m_casterUnit->IsCreature())
					m_casterUnit->getThreatManager().modifyThreatPercent(unitTarget, -100);

				break;
			}
			case 36041:	// Rebuke
			{
				m_casterUnit->CastSpell(unitTarget, 36042, true);

				if (m_casterUnit->IsCreature())
					m_casterUnit->getThreatManager().modifyThreatPercent(unitTarget, -20);

				break;
			}
			case 35999:	// Searing Heat
			{
				if (Creature* Crea = unitTarget->FindNearestCreature(25250, 50.0f))
				{
					float dist = Crea->GetDistance(unitTarget);

					float scale = 1.0f - (dist / 10.0f) * 0.8f;

					if (scale < 0.2f)
						scale = 0.2f;

					damage *= scale;
				}

				break;
			}
			case 35976:	//Flamestrike
			{
				float dist = m_casterUnit->GetDistance(unitTarget);

				float scale = 1.0f;

				if (dist >= 8.0f)
				{
					float t = (dist - 8.0f) / (15.0f - 8.0f);
					scale = 1.0f - t * 0.8f;

					if (scale < 0.2f)
						scale = 0.2f;
				}

				damage *= scale;

				break;
			}
			case 35943:	//Thorn Volley
			{
				float dist = m_casterUnit->GetDistance(unitTarget);
				float scale = 1.0f - (dist / 30.0f) * 0.8f;

				if (scale < 0.2f)
					scale = 0.2f;

				damage *= scale;

				break;
			}
			case 35882:
			{
				unitTarget->CastSpell(unitTarget, 35897, true);
				break;
			}
			//Trample Scarlet Charger
			case 35866:
			case 35876:
			{
				if (m_spellInfo->Id == 35876)
					if (unitTarget->HasAura(35875))
						return;

				unitTarget->AddAura(35875);

				break;
			}
			case 35850:	//Explosive Shot SM pet knockback
			{
				if (unitTarget)
				{
					if (!unitTarget->IsPlayer())
					{
						float xpos = unitTarget->GetPositionX() - 20.0f;

						if (xpos < 400.0f)
							xpos = 400.0f;

						unitTarget->NearTeleportTo(xpos, unitTarget->GetPositionY(), unitTarget->GetPositionZ() + 1.0f, 6.3f);
					}
				}

				break;
			}
			//BWL wild winds/pyroblast
			case 35310:
			case 35378:
			{
				float tarHP = unitTarget->GetMaxHealth();

				damage = uint32(damage * tarHP / 100);
				break;
			}
			// AC Ring of Chaos
			case 35100:
			{
				if (!unitTarget)
					return;

				if (urand(0, 99) < 30)
				{
					((Player*)m_caster)->DestroyItem(m_CastItem->GetBagSlot(), m_CastItem->GetSlot(), true);
					unitTarget->CastSpell(m_casterUnit, 35201, true);
					/*unitTarget->CastSpell(m_casterUnit, 28748, true);*/
				}
				else if (urand(0, 99) < 30)
					((Player*)m_caster)->DestroyItem(m_CastItem->GetBagSlot(), m_CastItem->GetSlot(), true);


				break;
			}
			//Root AC elemental casters
			case 35026:
			case 35029:
			case 35034:
				//Krixix cast root
			case 35268:
			case 35269:
			case 35270:
			{
				m_casterUnit->CastSpell(m_casterUnit, 35086, true);
				break;
			}
#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_8_4
			// Meteor-like spells (divided damage to targets)
			case 24340:
			case 26558:
			case 28884:		// Meteor
			case 26789:		// Shard of the Fallen Star
			case 34472:		// Baron Geddon Spreading Flames
			case 36301:		// Arcane Pulse trinket	
			{
				uint32 count = 0;
				for (TargetList::const_iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
					if (ihit->effectMask & (1 << effect_idx))
						++count;

				damage /= count;                    // divide to all targets
				break;
			}
#endif
			// percent from health with min
			case 25599:                             // Thundercrash
			{
				damage = unitTarget->GetHealth() / 2;
				if (damage < 200)
					damage = 200;
				break;
			}
			//MC
			case 34789:
			{
				m_caster->CastSpell(unitTarget, 34790, true);
				break;
			}
			//ZG
			case 34873:
			{
				m_caster->CastSpell(unitTarget, 34874, true);
				break;
			}
			case 34998:
			{
				damage = unitTarget->GetHealth() * 1.3 + unitTarget->GetHealth() / urand(1, 10);
				break;
			}
			//Seed of Corruption(Dire Maul)/Living Bomb
			case 34621:
			case 20476:
			{
				if (unitTarget != m_casterUnit)
					damage *= 2;
			}
			case 23206:                             // Chain Lightning (Simone the Seductress)
			{
				if (unitTarget->HasAura(20190))     // reduce damage by 75% if target has Aspect of the Wild (Rank 2)
					damage *= 0.25;
				break;
			}
			// Thaddius positive charge tick
			case 28062:
			{
				// Target also has positive charge, so no damage
				if (unitTarget->HasAura(28059))
					damage = 0;
				break;
			}
			// Thaddius negative charge tick
			case 28085:
			{
				// Target also has negative charge, so no damage
				if (unitTarget->HasAura(28084))
					damage = 0;
				break;
			}
			case 28375: // Gluth decimate
			{
				// damage should put target at maximum 5% hp, but not reduce it below that
				damage = std::max(0, int32(unitTarget->GetHealth() - uint32(unitTarget->GetMaxHealth() * 0.05f)));
				break;
			}
			case 28206: // Grobbulus Mutagen Explosion
			{
				// All sources say the explosion should do around 4.5k physical dmg if it runs out,
				// but "less" if dispelled. I have been able to find different variations of this spell,
				// so the hack has become to set m_triggeredBySpellInfo when casting this spell from Aura::HandleAuraDummy
				// when 28169 expires, and NOT set m_triggeredBySpellInfo 28169 is dispelled.
				if (m_triggeredBySpellInfo)
					damage = uint32(damage * 1.5f);
				else
					damage = uint32(damage / 1.5f);
				break;
			}
			case 27812: // Kel'Thuzad Void Blast
			{
				// If target has the chains of kel'thuzad aura the spell should not do any damage.
				// This check should not be necessary as you should be friendly to the caster of
				// the spell, but some bug caused players to take damage anyway, and even if that is fixed,
				// this is a safetycheck.
				if (unitTarget->HasAura(28410))
					damage = 0;
				break;
			}
			case 24933:                             // Cannon (Darkmoon Steam Tonk)
			{
				m_caster->CastSpell(unitTarget, 27766, true);
				break;
			}
			}
			break;
		}

		case SPELLFAMILY_MAGE:
			break;
		case SPELLFAMILY_WARRIOR:
		{
			if (!m_casterUnit)
				break;

			// Bloodthirst
			if (m_spellInfo->SpellIconID == 38 && m_spellInfo->IsFitToFamilyMask<CF_WARRIOR_BLOODTHIRST>())
			{
				float attackPower = m_casterUnit->GetTotalAttackPowerValue(BASE_ATTACK);
				if (unitTarget)
					attackPower += m_casterUnit->GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_MELEE_ATTACK_POWER_VERSUS, unitTarget->GetCreatureTypeMask());
				damage = uint32(damage * attackPower / 100);

				if (m_casterUnit->HasAura(20497))
				{
					//Slam cooldown
					uint32 CDSpell = 0;

					if (m_casterUnit->HasSpell(34800))
						CDSpell = 34800;
					else if (m_casterUnit->HasSpell(11605))
						CDSpell = 11605;
					else if (m_casterUnit->HasSpell(11604))
						CDSpell = 11604;
					else if (m_casterUnit->HasSpell(8820))
						CDSpell = 8820;
					else if (m_casterUnit->HasSpell(1464))
						CDSpell = 1464;

					if (CDSpell > 0)
					{
						m_casterUnit->AddSpellCooldown(CDSpell, 0, time(nullptr) + 6);
						if (Player* pPlayer = m_casterUnit->ToPlayer())
							pPlayer->SendSpellCooldown(CDSpell, 6000, pPlayer->GetObjectGuid());
					}
				}
			}
			//v+ Scholo Unholy Strike/Gorulak strike
			else if ((m_spellInfo->Id == 34384) || (m_spellInfo->Id == 34427))
			{
				float tarHP = unitTarget->GetHealth();

				damage = uint32(damage * tarHP / 100);
			}
			//BWL mental strike
			else if (m_spellInfo->Id == 35165)
			{
				float tarHP = unitTarget->GetMaxHealth();

				damage = uint32(damage * tarHP / 100);
			}
			//Untamed Strike
			else if (m_spellInfo->Id == 35178)
			{
				float tarCurHP = unitTarget->GetHealth();

				uint32 PotentialCurDmg = uint32(damage * tarCurHP / 100);

				if (PotentialCurDmg < (urand(5500, 6000)))
					damage = urand(6000, 7000);
				else
					damage = PotentialCurDmg;
			}
			// Shield Slam
			else if (m_spellInfo->IsFitToFamilyMask<CF_WARRIOR_SHIELD_SLAM>())
			{
				if (m_casterUnit->IsPlayer())
				{
					damage += m_casterUnit->ToPlayer()->GetPlayerBlockValue();
				}

				if (m_casterUnit->HasAura(20497))
				{
					//Slam cooldown
					uint32 CDSpell = 0;

					if (m_casterUnit->HasSpell(34800))
						CDSpell = 34800;
					else if (m_casterUnit->HasSpell(11605))
						CDSpell = 11605;
					else if (m_casterUnit->HasSpell(11604))
						CDSpell = 11604;
					else if (m_casterUnit->HasSpell(8820))
						CDSpell = 8820;
					else if (m_casterUnit->HasSpell(1464))
						CDSpell = 1464;

					if (CDSpell > 0)
					{
						m_casterUnit->AddSpellCooldown(CDSpell, 0, time(nullptr) + 6);
						if (Player* pPlayer = m_casterUnit->ToPlayer())
							pPlayer->SendSpellCooldown(CDSpell, 6000, pPlayer->GetObjectGuid());
					}
				}
			}
			// Execute trigger
			else if (m_spellInfo->Id == 20647)
				m_casterUnit->SetPower(POWER_RAGE, 0);
			break;
		}
		case SPELLFAMILY_SHAMAN: //vanilla+
		{
			switch (m_spellInfo->Id)
			{
				case 33641:	// Armaments of Storm
				{
					damage = m_caster->SpellDamageBonusDone(unitTarget, m_spellInfo, damage, SPELL_DIRECT_DAMAGE);
					damage = unitTarget->SpellDamageBonusTaken(m_caster, m_spellInfo, damage, SPELL_DIRECT_DAMAGE);

					break;
				}
				//Magma Totem threat
				case 8187:
				case 10579:
				case 10580:
				case 10581:
				{
					if (m_caster->IsCreature() && m_caster->ToCreature()->IsTotem())
					{
						if (unitTarget->CanHaveThreatList())
						{
							Unit* ThreatTarget = m_caster->ToCreature()->GetOwner();
							float threat = damage * 2.0f;

							if (ThreatTarget->HasAura(16544))
								threat *= 0.0f;
							else if (ThreatTarget->HasAura(16086))
								threat *= 0.5f;

							if (threat > 0.0f)
								unitTarget->AddThreat(ThreatTarget, threat, false, m_spellInfo->GetSpellSchoolMask(), m_spellInfo);
						}
					}
					break;
				}
				case 33652: // Flame Aftershock
				{
					// for caster applied auras only
					Unit::AuraList const& mPeriodic = unitTarget->GetAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
					Aura* targetAura = nullptr;
					for (Unit::AuraList::const_iterator i = mPeriodic.begin(); i != mPeriodic.end(); ++i)
					{
						// Flame Shock
						if ((*i)->GetSpellProto()->IsFitToFamily<SPELLFAMILY_SHAMAN, CF_SHAMAN_FLAME_SHOCK>() &&
							(*i)->GetCasterGuid() == m_caster->GetObjectGuid())
							targetAura = *i;
					}

					if (!targetAura)
					{
						sLog.outError("Target (GUID: %u TypeId: %u) has aurastate AURA_STATE_SWIFTMEND but no matching aura.", unitTarget->GetGUIDLow(), unitTarget->GetTypeId());
						return;
					}
					int idx = 0;
					while (idx < 3)
					{
						if (targetAura->GetSpellProto()->EffectApplyAuraName[idx] == SPELL_AURA_PERIODIC_DAMAGE)
							break;
						idx++;
					}

					int32 flametickdmg = targetAura->GetModifier()->m_amount;

					unitTarget->RemoveAurasDueToSpell(targetAura->GetId());

					damage += flametickdmg * 5;

					break;
				}
			}

			break;
		}
		case SPELLFAMILY_WARLOCK:
		{
			// Conflagrate - consumes Immolate
			if (m_spellInfo->IsFitToFamilyMask<CF_WARLOCK_CONFLAGRATE>())
			{
				// for caster applied auras only
				Unit::AuraList const& mPeriodic = unitTarget->GetAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
				for (Unit::AuraList::const_iterator i = mPeriodic.begin(); i != mPeriodic.end(); ++i)
				{
					// Immolate
					if ((*i)->GetSpellProto()->IsFitToFamily<SPELLFAMILY_WARLOCK, CF_WARLOCK_IMMOLATE>() && (*i)->GetCasterGuid() == m_caster->GetObjectGuid())
					{
						if (SpellAuraHolder* holder = (*i)->GetHolder())
						{
							Modifier const* mod = (*i)->GetModifier();
							if (mod)
							{
								uint32 stackAmount = holder->GetStackAmount();
								if (!stackAmount)
									stackAmount = 1;

								uint32 amplitude = mod->periodictime ? mod->periodictime : 3000;
								uint32 totalTicks = (*i)->GetAuraTicks();

								uint32 tickDamage = mod->m_amount > 0 ? uint32(mod->m_amount) : 0;
								uint32 baseTickDamage = tickDamage / stackAmount;

								uint32 ticksAfterRefresh = 0;
								if (stackAmount > 1)
								{
									uint32 elapsedSinceRefresh = 0;
									if (holder->GetAuraMaxDuration() > 0 && holder->GetAuraDuration() < holder->GetAuraMaxDuration())
										elapsedSinceRefresh = uint32(holder->GetAuraMaxDuration() - holder->GetAuraDuration());

									uint32 periodicTimer = (*i)->GetAuraPeriodicTimer();
									if (!periodicTimer || periodicTimer > amplitude)
										periodicTimer = amplitude;

									ticksAfterRefresh = (elapsedSinceRefresh + periodicTimer - 1) / amplitude;
									if (ticksAfterRefresh > totalTicks)
										ticksAfterRefresh = totalTicks;
								}

								uint32 ticksBeforeRefresh = totalTicks - ticksAfterRefresh;

								uint64 conflagrateDamage = uint64(baseTickDamage) * ticksBeforeRefresh;
								conflagrateDamage += uint64(baseTickDamage) * stackAmount * ticksAfterRefresh;

								uint32 maxTicks = (*i)->GetAuraMaxTicks();
								if (!maxTicks && holder->GetAuraMaxDuration() > 0)
									maxTicks = uint32(holder->GetAuraMaxDuration()) / amplitude;

								if (maxTicks)
								{
									uint64 maxDamage = uint64(baseTickDamage) * maxTicks * stackAmount;
									if (conflagrateDamage > maxDamage)
										conflagrateDamage = maxDamage;
								}

								damage += uint32(conflagrateDamage);
							}
						}
						unitTarget->RemoveAurasByCasterSpell((*i)->GetId(), m_caster->GetObjectGuid());
						break;
					}
				}
			}

			//Nemesis 4/4 Direct DMG
			if (m_spellInfo->IsFitToFamily<SPELLFAMILY_WARLOCK, CF_WARLOCK_SHADOW_BOLT, CF_WARLOCK_SEARING_PAIN, CF_WARLOCK_HELLFIRE, CF_WARLOCK_IMMOLATE, CF_WARLOCK_CONFLAGRATE, CF_WARLOCK_SHADOWBURN>())
			{
				if (m_casterUnit->HasAura(34570))
				{
					uint32 warHP = m_casterUnit->GetHealth() / m_casterUnit->GetMaxHealth() * 100;
					uint32 tarHP = unitTarget->GetHealth() / unitTarget->GetMaxHealth() * 100;
					if (warHP > tarHP)
					{
						damage += 0.05f * m_casterUnit->SpellDamageBonusDone(unitTarget, m_spellInfo, damage, SPELL_DIRECT_DAMAGE);
						damage += 0.05f * unitTarget->SpellDamageBonusTaken(m_casterUnit, m_spellInfo, damage, SPELL_DIRECT_DAMAGE);
						damage *= 1.05;
					}
				}
			}
			break;
		}
		//vanilla+
		case SPELLFAMILY_PRIEST:
		{
			// Power Word Requital
			if ((m_spellInfo->SpellIconID == 1930) && (m_spellInfo->IsFitToFamilyMask<CF_PRIEST_HOLY_NOVA1>()))
			{
				// base damage halved if target not stunned/feared
				{
					if (!unitTarget->hasUnitState(UNIT_STAT_STUNNED | UNIT_STAT_PENDING_STUNNED | UNIT_STAT_FLEEING))
						damage = int32(damage * 0.5f);
				}
			}
			break;
		}
		case SPELLFAMILY_DRUID:
		{
			// Talonclaw 4/4
			if (m_spellInfo->IsFitToFamilyMask<CF_DRUID_WRATH>() && m_casterUnit->HasAura(34555))
			{
				Unit::AuraList const& mPeriodic = unitTarget->GetAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
				for (Unit::AuraList::const_iterator i = mPeriodic.begin(); i != mPeriodic.end(); ++i)
				{
					// Insect Swarm
					if ((*i)->GetSpellProto()->IsFitToFamily<SPELLFAMILY_DRUID, CF_DRUID_INSECT_SWARM>())
					{
						damage *= 1.05f;
						/*damage += 0.05f * m_caster->SpellDamageBonusDone(unitTarget, m_spellInfo, damage, SPELL_DIRECT_DAMAGE);*/
						/*damage += 0.05f * unitTarget->SpellDamageBonusTaken(m_caster, m_spellInfo, damage, SPELL_DIRECT_DAMAGE);*/
						break;
					}
				}
			}
			if (m_spellInfo->IsFitToFamilyMask<CF_DRUID_STARFIRE>() && m_casterUnit->HasAura(34555))
			{
				Unit::AuraList const& mPeriodic = unitTarget->GetAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
				for (Unit::AuraList::const_iterator i = mPeriodic.begin(); i != mPeriodic.end(); ++i)
				{
					// Moonfire
					if ((*i)->GetSpellProto()->IsFitToFamily<SPELLFAMILY_DRUID, CF_DRUID_MOONFIRE>())
					{
						damage *= 1.05f;
						/*damage += 0.05f * m_caster->SpellDamageBonusDone(unitTarget, m_spellInfo, damage, SPELL_DIRECT_DAMAGE);*/
						/*damage += 0.05f * unitTarget->SpellDamageBonusTaken(m_caster, m_spellInfo, damage, SPELL_DIRECT_DAMAGE);*/
						break;
					}
				}
			}
			// Ferocious Bite
			if (m_spellInfo->IsFitToFamilyMask<CF_DRUID_RIP_BITE>() && m_spellInfo->SpellVisual == 6587)
			{
				Player* pPlayer = m_caster->ToPlayer();
				if (!pPlayer)
					break;

#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_11_2
				// World of Warcraft Client Patch 1.12.0 (2006-08-22)
				// - Ferocious Bite: Book of Ferocious Bite (Rank 5) now drops off The
				//   Beast in Black Rock Spire. In addition, Ferocious Bite now increases
				//   in potency with greater attack power.
				// ( AP * 3% * combo + energy * 2,7 + damage )
				if (uint32 combo = ((Player*)pPlayer)->GetComboPoints())
					damage += int32(pPlayer->GetTotalAttackPowerValue(BASE_ATTACK) * combo * 0.03f);
#endif
				damage += int32(pPlayer->GetPower(POWER_ENERGY) * m_spellInfo->DmgMultiplier[effect_idx]);
				pPlayer->SetPower(POWER_ENERGY, 0);
			}
			break;
		}
		case SPELLFAMILY_ROGUE:
		{
#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_11_2
			// World of Warcraft Client Patch 1.12.0 (2006-08-22)
			// - Eviscerate: Manual of Eviscerate (Rank 9) now drops off Blackhand
			//   Assassins in Black Rock Spire.In addition, Eviscerate now increases
			//   in potency with greater attack power.
			if (m_spellInfo->IsFitToFamilyMask<CF_ROGUE_EVISCERATE>())
			{
				if (Player* pPlayer = m_caster->ToPlayer())
					if (uint32 combo = pPlayer->GetComboPoints())
						damage += int32(pPlayer->GetTotalAttackPowerValue(BASE_ATTACK) * combo * 0.03f);
			}
#endif
			break;
		}
		case SPELLFAMILY_HUNTER:
			if (!m_casterUnit)
				break;

			//v+ Whirling Axe
			if (m_spellInfo->Id == 34216)
			{
				float attackPower = m_casterUnit->GetTotalAttackPowerValue(BASE_ATTACK);
				if (unitTarget)
					attackPower += m_casterUnit->GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_MELEE_ATTACK_POWER_VERSUS, unitTarget->GetCreatureTypeMask());
				damage = uint32(damage * attackPower / 100);
			}
			break;
		case SPELLFAMILY_PALADIN:
		{
			// Hammer of Wrath - receive bonus from spell damage
			if (m_spellInfo->SpellIconID == 42)
			{
				m_attackType = BASE_ATTACK;    // Set as base attack to benefit from melee crit
				/*damage = m_caster->SpellDamageBonusDone(unitTarget, m_spellInfo, damage, SPELL_DIRECT_DAMAGE);
				damage = unitTarget->SpellDamageBonusTaken(m_caster, m_spellInfo, damage, SPELL_DIRECT_DAMAGE);*/
				//v+ long arm of justice
				/*if (!m_casterUnit->HasAura(20361))
					m_casterUnit->resetAttackTimer(BASE_ATTACK);*/
			}
			// Judgement of Command - receive bonus from spell damage
			else if (m_spellInfo->IsFitToFamilyMask<CF_PALADIN_EXORCISM>() && m_spellInfo->SpellIconID == 292)
			{
				if (unitTarget->GetCreatureType() == CREATURE_TYPE_UNDEAD || unitTarget->GetCreatureType() == CREATURE_TYPE_DEMON)
					damage = int32(damage * 1.5f);
			}
			else if (m_spellInfo->SpellIconID == 561)
			{
				//vanilla+ base damage halved if target not stunned/CONFUSED
				if (!unitTarget->hasUnitState(UNIT_STAT_STUNNED | UNIT_STAT_PENDING_STUNNED | UNIT_STAT_CONFUSED))
					damage = int32(damage * 0.5f);

				damage = m_caster->SpellDamageBonusDone(unitTarget, m_spellInfo, damage, SPELL_DIRECT_DAMAGE);
				damage = unitTarget->SpellDamageBonusTaken(m_caster, m_spellInfo, damage, SPELL_DIRECT_DAMAGE);
			}
			//Holy Wrath
			/*else if (m_spellInfo->Id == 2812 || m_spellInfo->Id == 10318)
			{
				if (!m_casterUnit->HasAura(20361))
					m_casterUnit->resetAttackTimer(BASE_ATTACK);
			}*/
			break;
		}
		}

		if (damage >= 0)
			m_damage += damage;
	}
}

void Spell::EffectDummy(SpellEffectIndex eff_idx)
{
	if (!unitTarget && !gameObjTarget && !itemTarget && !corpseTarget)
		return;

	// selection by spell family
	switch (m_spellInfo->SpellFamilyName)
	{
	case SPELLFAMILY_GENERIC:
	{
		switch (m_spellInfo->Id)
		{

		case 36280:	// /Two-Faced Medallion/Borrowed Face
		{
			




			if (unitTarget->IsCreature() && unitTarget->isDead())
			{
				CreatureModelInfo const* modelInfo = sObjectMgr.GetCreatureModelInfo(unitTarget->GetDisplayId());

				if (modelInfo)
				{
					int32 model_id = unitTarget->GetDisplayId();
					m_casterUnit->CastCustomSpell(m_casterUnit, 36281, &model_id, nullptr, nullptr, true);
				}
			}

			break;
		}
		case 36219: // Dog Whip
		{
			if (m_casterUnit->IsFriendlyTo(unitTarget))
				m_casterUnit->CastSpell(unitTarget, 36220, true);
			else
				m_casterUnit->CastSpell(unitTarget, 36221, true);

			return;
		}
		case 36112:	// Consume Elemental
		{
			std::list<Creature*> dummy;
			m_casterUnit->GetCreatureListWithEntryInGrid(dummy, 25200, 30.0f);
			m_casterUnit->GetCreatureListWithEntryInGrid(dummy, 25258, 30.0f);
			for (auto it = dummy.cbegin(); it != dummy.cend(); ++it)
			{
				if ((*it)->isAlive() && (*it)->IsWithinLOSInMap(m_casterUnit))
				{
					if ((*it)->GetHealthPercent() < 20.0f)
					{
						int32 HP = (*it)->GetHealth();

						m_casterUnit->CastCustomSpell(m_casterUnit, 36110, &HP, &HP, nullptr, true, nullptr);
						m_casterUnit->CastSpell((*it), 36111, true);
						break;
					}
				}
			}

			return;
		}
		case 36099:	// Script of Destiny
		{
			m_casterUnit->CastSpell(m_casterUnit, 36100, true);

			return;
		}
		case 36094: // Flaming Purge
		{
			if (m_caster->IsFriendlyTo(unitTarget))
				m_caster->CastSpell(unitTarget, 36086, true);
			else
				m_caster->CastSpell(unitTarget, 36088, true);

			return;
		}
		case 36083:	// Berserker's Blood SM Myrmidon
		{
			uint32 hp = m_casterUnit->GetHealth();
			uint32 maxHp = m_casterUnit->GetMaxHealth();

			int32 zerk_mod = 100 - (hp * 100 / maxHp);
			int32 zerk_mod2 = zerk_mod / 5;

			if (zerk_mod >= 1)
				m_casterUnit->CastCustomSpell(m_casterUnit, 36084, &zerk_mod, &zerk_mod2, &zerk_mod2, true, nullptr);

			return;
		}
		case 36073:	// Smart Purify
		{
			if (Unit* pUnit = m_casterUnit->FindFriendlyUnitDispel(30.0f, false, false, true, true))
				m_casterUnit->CastSpell(pUnit, 36074, true);

			return;
		}
		case 36075:	// Smart Cleanse
		{
			if (Unit* pUnit = m_casterUnit->FindFriendlyUnitDispel(30.0f, true, false, true, true))
				m_casterUnit->CastSpell(pUnit, 36076, true);

			return;
		}
		case 36085:	// Smart Puryfying Flames
		{
			if (Unit* pUnit = m_casterUnit->FindFriendlyUnitDispel(40.0f, true, true, true, false))
				m_casterUnit->CastSpell(pUnit, 36086, true);

			return;
		}
		case 36102: // Smart Smart Fate Revision
		{
			if (Unit* pUnit = m_casterUnit->FindLowestHpFriendlyUnit(30.0f, 79, true, m_casterUnit))
			{
				if (!m_casterUnit->HasAura(36105))
				{
					m_casterUnit->CastSpell(pUnit, 36103, true);
				}
			}

			return;
		}
		case 36077: // Smart Intercept
		{
			if (m_casterUnit->IsCreature())
			{
				Unit* pTarget = nullptr;
				float lowestEHP = std::numeric_limits<float>::max();

				ThreatList const& tList = m_casterUnit->getThreatManager().getThreatList();
				for (ThreatList::const_iterator iter = tList.begin(); iter != tList.end(); ++iter)
				{
					Unit* pTempTarget = m_casterUnit->GetMap()->GetUnit((*iter)->getUnitGuid());
					if (!pTempTarget)
						continue;

					if (!pTempTarget->isAlive())
						continue;

					if (pTempTarget == m_casterUnit->getVictim())
						continue;

					if (m_casterUnit->GetDistance2d(pTempTarget) >= 40.0f)
						continue;

					if (pTempTarget->HasAura(4511))
						continue;

					if (pTempTarget->IsImmuneToDamage(m_casterUnit->GetMeleeDamageSchoolMask()))
						continue;

					float armor = float(pTempTarget->GetArmor());

					float lvl = float(m_casterUnit->getLevel());
					float tmp = 0.1f * armor / (8.5f * lvl + 40.0f);
					float reduction = tmp / (1.0f + tmp);

					if (reduction < 0.0f) reduction = 0.0f;
					if (reduction > 0.75f) reduction = 0.75f;

					float takenMul = 1.0f - reduction;
					if (takenMul < 0.01f) takenMul = 0.01f;

					float ehp = float(pTempTarget->GetMaxHealth()) / takenMul;

					if (ehp < lowestEHP)
					{
						lowestEHP = ehp;
						pTarget = pTempTarget;
					}
				}

				if (pTarget)
				{
					if (CreatureAI* ai = m_casterUnit->AI())
					{
						if (ScriptedAI* scripted = dynamic_cast<ScriptedAI*>(ai))
							scripted->DoResetThreat();
					}

					if (m_casterUnit->GetDistance(pTarget) > 10.0f)
						m_casterUnit->CastSpell(pTarget, 36078, true);

					m_casterUnit->CastSpell(m_casterUnit, 36079, false);
					m_casterUnit->getThreatManager().addThreat(pTarget, 100000);
				}
			}

			return;
		}
		case 36059:	//Kill Command Scarlet Tracking Hound
		{
			if (m_casterUnit->IsCreature())
			{
				Unit* pTarget = nullptr;
				float lowestEHP = std::numeric_limits<float>::max();

				ThreatList const& tList = m_casterUnit->getThreatManager().getThreatList();
				for (ThreatList::const_iterator iter = tList.begin(); iter != tList.end(); ++iter)
				{
					Unit* pTempTarget = m_casterUnit->GetMap()->GetUnit((*iter)->getUnitGuid());
					if (!pTempTarget)
						continue;

					if (!pTempTarget->isAlive())
						continue;

					if (m_casterUnit->GetDistance2d(pTempTarget) >= 40.0f)
						continue;

					if (pTempTarget->HasAura(4511))
						continue;

					if (pTempTarget->IsImmuneToDamage(m_casterUnit->GetMeleeDamageSchoolMask()))
						continue;

					float armor = float(pTempTarget->GetArmor());

					float lvl = float(m_casterUnit->getLevel());
					float tmp = 0.1f * armor / (8.5f * lvl + 40.0f);
					float reduction = tmp / (1.0f + tmp);

					if (reduction < 0.0f) reduction = 0.0f;
					if (reduction > 0.75f) reduction = 0.75f;

					float takenMul = 1.0f - reduction;
					if (takenMul < 0.01f) takenMul = 0.01f;

					float ehp = float(pTempTarget->GetHealth()) / takenMul;

					if (ehp < lowestEHP)
					{
						lowestEHP = ehp;
						pTarget = pTempTarget;
					}
				}

				if (pTarget)
				{
					CreatureGroup* bmGroup = m_casterUnit->ToCreature()->GetCreatureGroup();
					ObjectGuid bmGuid = m_casterUnit->GetObjectGuid();

					std::list<Creature*> dogs;
					GetCreatureListWithEntryInGrid(dogs, m_casterUnit, 25202, 100.0f);

					bool Casted = false;

					for (Creature* dog : dogs)
					{
						if (!dog || !dog->isAlive())
							continue;

						bool ok = (bmGroup && dog->GetCreatureGroup() == bmGroup);

						if (!ok && dog->IsTemporarySummon())
						{
							TemporarySummon* ts = static_cast<TemporarySummon*>(dog);
							if (ts->GetSummonerGuid() == bmGuid)
								ok = true;
						}

						if (!ok)
							continue;

						Casted = true;

						if (CreatureAI* ai = dog->AI())
						{
							if (ScriptedAI* scripted = dynamic_cast<ScriptedAI*>(ai))
								scripted->DoResetThreat();
						}
						dog->CastSpell(dog, 36061, true);
						dog->getThreatManager().addThreat(pTarget, 100000);
					}

					if (Casted)
						m_casterUnit->CastSpell(pTarget, 36060, true);
				}
			}


			return;
		}
		case 36058:	//Scatter Shot SM
		{
			if (m_casterUnit->IsCreature())
				m_casterUnit->getThreatManager().modifyThreatPercent(unitTarget, -100);

			return;
		}
		//Dragon Breath
		case 35963:
		case 35964:
		{
			if (m_casterUnit && unitTarget->IsCreature() && unitTarget->GetEntry() == 25249)
			{
				if (Creature* Crea = unitTarget->FindNearestCreature(25250, 1.0f))
				{
				}
				else
				{
					unitTarget->SummonCreature(25250,
						unitTarget->GetPositionX(),
						unitTarget->GetPositionY(),
						unitTarget->GetPositionZ(),
						unitTarget->GetOrientation(),
						TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,
						600000);
				}
			}


			return;
		}
		case 35929:	// SM Herod Demo
		{
			if (m_casterUnit)
			{
				if (!unitTarget->HasAura(35930))
					unitTarget->CastSpell(unitTarget, 35930, true);

				if (!unitTarget->HasAura(35931))
					unitTarget->CastSpell(unitTarget, 35931, true);

				if (SpellAuraHolder* SpellHolder = unitTarget->GetSpellAuraHolder(35929))
				{
					if (SpellHolder->GetStackAmount() > 3)
					{
						unitTarget->RemoveAurasDueToSpell(35929);
						unitTarget->CastSpell(unitTarget, 35933, true);	//Self-fear
						if (m_casterUnit->IsCreature())
						{
							ThreatManager& threatMgr = m_casterUnit->ToCreature()->getThreatManager();

							if (HostileReference* ref = threatMgr.getCurrentVictim())
								threatMgr.addThreatDirectly(unitTarget, ref->getThreat() * 1.3f);
						}
					}
				}
			}

			return;
		}
		// Explosive Shot SM
		case 35849:
		{
			if (m_casterUnit)
			{
				if (Creature* Crea = m_casterUnit->SummonCreature(25233, unitTarget->GetPositionX() + 5.0f, unitTarget->GetPositionY(), unitTarget->GetPositionZ() + 1.0f, 3.14f, TEMPSUMMON_TIMED_DESPAWN, 3000))
				{
					Crea->CastSpell(Crea, 35851, true);
					Crea->CastSpell(Crea, 35850, true);
				}
			}


			return;
		}
		case 8348:
		{
			if (m_CastItem)
			{
				if (m_CastItem->GetSlot() == EQUIPMENT_SLOT_MAINHAND)
					m_casterUnit->CastSpell(m_casterUnit, 35839, true);
				else if (m_CastItem->GetSlot() == EQUIPMENT_SLOT_OFFHAND)
					m_casterUnit->CastSpell(m_casterUnit, 35840, true);
			}

			return;
		}
		// Wild Polymorph Dummy
		case 23603:
		{
			if (unitTarget)
				unitTarget->CastSpell(unitTarget, 35377, true);
			return;
		}
		// Wild Pyroblast Dummy
		case 35309:
		{
			if (unitTarget)
				unitTarget->CastSpell(unitTarget, 35378, true);
			return;
		}
		// Shadows of Ebonroc Dummy
		case 35361:
		{
			if (unitTarget)
				unitTarget->CastSpell(unitTarget, 35245, true);

			return;
		}
		// Flames of Flamegor Dummy
		case 35360:
		{
			if (unitTarget)
				unitTarget->CastSpell(unitTarget, 35247, true);

			return;
		}
		case 35326:
		{
			if (unitTarget)
			{
				int32 damtotale = urand(1, damage);
				int32 damheal = damage - damtotale;
				int32 spellIdRan = urand(0, 2);
				int32 spellId = 35355;

				if (spellIdRan == 0)
					spellId = 35356;
				else if (spellIdRan == 1)
					spellId = 35357;

				m_casterUnit->CastCustomSpell(unitTarget, spellId, &damtotale, nullptr, nullptr, true, nullptr);
				m_casterUnit->CastCustomSpell(m_casterUnit, 35333, &damheal, nullptr, nullptr, true, nullptr);

				if (urand(0, 99) > 0) //Tentacle
				{
					if (urand(0, 1) == 0)
						m_casterUnit->CastSpell(unitTarget, 35334, true);
					else
						unitTarget->CastSpell(m_casterUnit, 35334, true);
				}
			}

			return;
		}
		//Clear Essence of the Red
		case 23681:
		{
			if (unitTarget->HasAura(23513))
				unitTarget->RemoveAurasDueToSpell(23513);

			break;
		}
		case 35278:
		{
			if (unitTarget->HasAura(35278))
			{
				if (SpellAuraHolder* holder = unitTarget->GetSpellAuraHolder(35278))
				{
					/*uint32 stacks = holder->GetStackAmount();*/
					if (holder->GetStackAmount() > 1)
						holder->ModStackAmount(10);
				}
			}
			break;
		}
		// Hakkar's Sacrifice
		case 34954:
		{
			if (Player* pTarget = unitTarget->ToPlayer())
			{
				pTarget->CastSpell(pTarget, 35424, true);
			}

			break;
		}
		//TIme Lapse cast
		case 35235:
		{
			if (Player* pPlayer = ToPlayer(unitTarget))
				pPlayer->TimeLapseCast();

			return;
		}
		//Spell Stealing cast
		case 35308:
		{
			if (Player* pPlayer = ToPlayer(unitTarget))
			{
				uint32 StolenSpellId = pPlayer->GetLastCastedSpell();
				bool StolenSpellIsPositive = pPlayer->GetLastCastedSpellIsPositive();

				if (StolenSpellIsPositive)
					m_casterUnit->CastSpell(m_casterUnit, StolenSpellId, true);
				else
					m_casterUnit->CastSpell(unitTarget, StolenSpellId, true);
			}

			return;
		}
		case 35198:
		{
			if (unitTarget)
				unitTarget->CastSpell(unitTarget, 35140, true);

			break;
		}
		//Barrel's Max stack oil tar
		case 35160:
		{
			if (!unitTarget->HasAura(35135))
				unitTarget->AddAura(35135);
			{
				if (unitTarget->HasAura(35135))
				{
					if (SpellAuraHolder* holder = unitTarget->GetSpellAuraHolder(35135))
					{
						if (holder->GetStackAmount() < 11)
							holder->SetStackAmount(10);
						else
							holder->UpdateAuraDuration();
					}
				}
			}
			break;
		}
		//Apply Grenades AoE
		case 35136:
		{
			m_casterUnit->CastSpell(unitTarget, 35155, true);

			break;
		}
		case 34783: //Soul Link max HP/MP
		{
			if (Pet* pet = m_casterUnit->GetPet())
			{
				uint32 HealthPool = pet->GetHealth();
				uint32 ManaPool = pet->GetPower(POWER_MANA);
				pet->UpdateAllStats();

				uint32 NewHP = HealthPool + (m_casterUnit->GetStat(STAT_STAMINA) * 10 / 2);
				uint32 NewMP = ManaPool + (m_casterUnit->GetStat(STAT_INTELLECT) * 15 / 2);

				if (m_casterUnit->HasAura(18752))
					NewHP *= 1.25f;
				else if (m_casterUnit->HasAura(18751))
					NewHP *= 1.2f;
				else if (m_casterUnit->HasAura(18750))
					NewHP *= 1.15f;
				else if (m_casterUnit->HasAura(18749))
					NewHP *= 1.1f;
				else if (m_casterUnit->HasAura(18748))
					NewHP *= 1.05f;

				if (m_casterUnit->HasAura(18746))
					NewMP *= 1.25f;
				else if (m_casterUnit->HasAura(18745))
					NewMP *= 1.2f;
				else if (m_casterUnit->HasAura(18744))
					NewMP *= 1.15f;
				else if (m_casterUnit->HasAura(18743))
					NewMP *= 1.1f;
				else if (m_casterUnit->HasAura(18741))
					NewMP *= 1.05f;

				pet->SetHealth(NewHP);
				pet->SetPower(POWER_MANA, NewMP);
			}
			return;
		}
		case 34785: //Lock update pet stats
		{
			if (Pet* pet = m_casterUnit->GetPet())
				pet->UpdateAllStats();
			return;
		}
		case 6700: // Dimensional Portal (Used by Arugal)
		{
			if (unitTarget->GetTypeId() == TYPEID_UNIT)
			{
				if (InstanceData* m_pInstance = unitTarget->GetInstanceData())
					m_pInstance->SetData(5, 3);

				unitTarget->SetVisibility(VISIBILITY_OFF);
			}
			return;
		}
		case 34647: //Tome of Experience
		{
			Player* pPlayer = m_caster->ToPlayer();
			pPlayer->GiveXP(7777777, NULL);


			uint32 chestid = 0;
			uint32 plate = 26096;
			uint32 mail = 26097;
			uint32 leather = 26098;
			uint32 cloth = 26099;

			switch (unitTarget->getClass())
			{
			case CLASS_WARRIOR:
				chestid = plate;
				break;
			case CLASS_PALADIN:
				chestid = plate;
				break;
			case CLASS_SHAMAN:
				chestid = mail;
				break;
			case CLASS_HUNTER:
				chestid = mail;
				break;
			case CLASS_ROGUE:
				chestid = leather;
				break;
			case CLASS_DRUID:
				chestid = leather;
				break;
			case CLASS_MAGE:
				chestid = cloth;
				break;
			case CLASS_WARLOCK:
				chestid = cloth;
				break;
			case CLASS_PRIEST:
				chestid = cloth;
				break;
			default:
				chestid = cloth;
				break;
			}

			pPlayer->AddItem(chestid);
			pPlayer->ModifyMoney(urand(1500000, 2500000));
			pPlayer->AddItem(22719);
			return;
		}
		case 11885: // Capture Treant
		case 11886: // Capture Wildkin
		case 11887: // Capture Hippogryph
		case 11888: // Capture Faerie Dragon
		case 11889: // Capture Mountain Giant
		case 12699: // Summon Screecher Spirit
		{
			if (unitTarget->isDead() && unitTarget->GetTypeId() == TYPEID_UNIT)
				((Creature*)unitTarget)->ForcedDespawn(1000);
			return;
		}
		case 19395: // Gordunni Trap
		{
			if (unitTarget && unitTarget->GetTypeId() == TYPEID_PLAYER)
			{
				// If GameObject casting was implemented, or activating a trap actually despawned it, this wouldn't be needed.
				if (GameObject* pObject = unitTarget->FindNearestGameObject(144050, INTERACTION_DISTANCE))
				{
					if (pObject->HasStaticDBSpawnData())
						unitTarget->CastSpell(unitTarget, urand(0, 1) ? 19394 : 11756, true);
					else
						pObject->AddObjectToRemoveList();
				}
			}

			return;
		}
		case 19614: // Despawn Caster
		{
			if (Creature* pCreature = ToCreature(unitTarget))
				pCreature->DespawnOrUnsummon();

			return;
		}
		case 25716: // Force Self - Bow
		{
			if (m_casterUnit)
				m_casterUnit->HandleEmote(EMOTE_ONESHOT_BOW);
			return;
		}
		case 27798: // Nature's Bounty
		{
			switch (unitTarget->getPowerType())
			{
			case POWER_RAGE:
				unitTarget->CastSpell(unitTarget, 27783, true);
				return;
			case POWER_ENERGY:
				unitTarget->CastSpell(unitTarget, 27784, true);
				return;
			case POWER_MANA:
				unitTarget->CastSpell(unitTarget, 27782, true);
				return;
			}
			return;
		}
		case 24531: // Refocus : "Instantly clears the cooldowns of Aimed Shot, Multishot, Volley, and Arcane Shot."
		{
			if (!m_caster->IsPlayer())
				return;
			SpellCooldowns cm = ((Player*)m_caster)->GetSpellCooldownMap();
			for (SpellCooldowns::const_iterator itr = cm.begin(); itr != cm.end();)
			{
				SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(itr->first);

				if (spellInfo->IsFitToFamily<SPELLFAMILY_HUNTER, CF_HUNTER_ARCANE_SHOT, CF_HUNTER_MULTI_SHOT, CF_HUNTER_VOLLEY, CF_HUNTER_AIMED_SHOT>() &&
					spellInfo->Id != m_spellInfo->Id && spellInfo->GetRecoveryTime() > 0)
					((Player*)m_caster)->RemoveSpellCooldown((itr++)->first, true);
				else
					++itr;
			}
			return;
		}
		case 34601: // Execute cd
		{
			if (!m_caster->IsPlayer())
				return;
			SpellCooldowns cm = ((Player*)m_caster)->GetSpellCooldownMap();
			for (SpellCooldowns::const_iterator itr = cm.begin(); itr != cm.end();)
			{
				SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(itr->first);

				if (spellInfo->IsFitToFamily<SPELLFAMILY_WARRIOR, CF_WARRIOR_EXECUTE>() &&
					spellInfo->Id != m_spellInfo->Id && spellInfo->GetRecoveryTime() > 0)
					((Player*)m_caster)->RemoveSpellCooldown((itr++)->first, true),
					((Player*)m_caster)->SendSpellCooldown(SPELL_ID_WEAPON_SWITCH_COOLDOWN_1_5s, 0, m_caster->GetObjectGuid());
				else
					++itr;
			}
			return;
		}
		case 35730: // Drain Soul cd reset
		{
			if (!m_caster->IsPlayer())
				return;
			SpellCooldowns cm = ((Player*)m_caster)->GetSpellCooldownMap();
			for (SpellCooldowns::const_iterator itr = cm.begin(); itr != cm.end();)
			{
				SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(itr->first);

				if (spellInfo->IsFitToFamily<SPELLFAMILY_WARLOCK, CF_WARLOCK_DRAIN_SOUL>() &&
					spellInfo->Id != m_spellInfo->Id && spellInfo->GetRecoveryTime() > 0)
					((Player*)m_caster)->RemoveSpellCooldown((itr++)->first, true);
					/*((Player*)m_caster)->SendSpellCooldown(SPELL_ID_WEAPON_SWITCH_COOLDOWN_1_5s, 0, m_caster->GetObjectGuid());*/
				else
					++itr;
			}
			return;
		}
		case 8344: // Universal Remote
		{
			if (!m_originalCaster)
				return;

			if (unitTarget->getLevel() > m_originalCaster->getLevel() && unitTarget->getLevel() - m_originalCaster->getLevel() > 2)
			{
				// Random spell
				if (urand(0, 4) == 0)
				{
					m_originalCaster->CastSpell(unitTarget, 8346, true);
				}
				else
				{
					m_originalCaster->CastSpell(unitTarget, 8347, true);
					unitTarget->CastSpell(unitTarget, 8599, true);
				}
			}
			else
			{
				// Random spell
				uint32 randomSpell = urand(0, 2);
				switch (randomSpell)
				{
				case 0:
					m_originalCaster->CastSpell(unitTarget, 8345, true);
					break;
				case 1:
					m_originalCaster->CastSpell(unitTarget, 8346, true);
					break;
				case 2:
					m_originalCaster->CastSpell(unitTarget, 8347, true);
					unitTarget->CastSpell(unitTarget, 8599, true);
					break;
				default:
					m_originalCaster->CastSpell(unitTarget, 8346, true);
					break;
				}
			}

			return;
		}
		case 13006: // Shrink ray
		{
			if (!m_originalCaster)
				return;
			/* Source: http://www.wowwiki.com/Gnomish_Shrink_Ray
				Not implemented yet:
				Make every attacking enemy bigger
				Make every attacking enemy smaller
				Give shield-wielding enemies "Improved Blocking" for 30 seconds (this shows no visible effect other than target is hit by the ray)
				13003 - Shrink Ray (single target)
				13010 - Shrink (TARGET_ALL_PARTY_AROUND_CASTER)
				13004 - Grow  (TARGET_ALL_PARTY_AROUND_CASTER)
			 */
			uint32 r = urand(0, 99);
			// Normal behavior
			if (r > 15)
				m_originalCaster->CastSpell(unitTarget, 13003, true);
			else if (r > 13) // Make the user bigger
			{
				m_originalCaster->AddAura(13004);
				m_originalCaster->SendSpellGo(m_originalCaster, 13004);
			}
			else if (r > 11) // Make the user smaller
			{
				m_originalCaster->AddAura(13010);
				m_originalCaster->SendSpellGo(m_originalCaster, 13010);
			}
			else if (r > 9) // Make the target bigger
			{
				unitTarget->AddAura(13004);
				m_originalCaster->SendSpellGo(unitTarget, 13004);
			}
			else if (r > 4) // Make the user's entire party smaller
				m_originalCaster->CastSpell(unitTarget, 13010, true);
			else // Make the user's entire party bigger
				m_originalCaster->CastSpell(unitTarget, 13004, true);
			break;
		}
		//AC Elemental Orbs
		//Air
		case 35007:
		{
			if (!m_originalCaster)
				return;

			float px = m_targets.m_destX;
			float py = m_targets.m_destY;
			float pz = m_targets.m_destZ;

			Creature* Charmed = m_caster->SummonCreature(25053, px, py, pz, m_caster->GetOrientation(), TEMPSUMMON_TIMED_COMBAT_OR_DEAD_DESPAWN, 3600000);
			if (!Charmed)
				return;

			m_caster->CastSpell(Charmed, 260, true);

			AddExecuteLogInfo(eff_idx, ExecuteLogInfo(Charmed->GetObjectGuid()));

			return;
		}
		//Fire
		case 35008:
		{
			if (!m_originalCaster)
				return;

			float px = m_targets.m_destX;
			float py = m_targets.m_destY;
			float pz = m_targets.m_destZ;

			Creature* Charmed = m_caster->SummonCreature(25054, px, py, pz, m_caster->GetOrientation(), TEMPSUMMON_TIMED_COMBAT_OR_DEAD_DESPAWN, 3600000);
			if (!Charmed)
				return;

			m_caster->CastSpell(Charmed, 260, true);

			AddExecuteLogInfo(eff_idx, ExecuteLogInfo(Charmed->GetObjectGuid()));

			return;
		}
		//Water
		case 35009:
		{
			if (!m_originalCaster)
				return;

			float px = m_targets.m_destX;
			float py = m_targets.m_destY;
			float pz = m_targets.m_destZ;

			Creature* Charmed = m_caster->SummonCreature(25055, px, py, pz, m_caster->GetOrientation(), TEMPSUMMON_TIMED_COMBAT_OR_DEAD_DESPAWN, 3600000);
			if (!Charmed)
				return;

			m_caster->CastSpell(Charmed, 260, true);

			AddExecuteLogInfo(eff_idx, ExecuteLogInfo(Charmed->GetObjectGuid()));

			return;
		}
		//Earth
		case 35010:
		{
			if (!m_originalCaster)
				return;

			float px = m_targets.m_destX;
			float py = m_targets.m_destY;
			float pz = m_targets.m_destZ;

			Creature* Charmed = m_caster->SummonCreature(25056, px, py, pz, m_caster->GetOrientation(), TEMPSUMMON_TIMED_COMBAT_OR_DEAD_DESPAWN, 3600000);
			if (!Charmed)
				return;

			m_caster->CastSpell(Charmed, 260, true);

			AddExecuteLogInfo(eff_idx, ExecuteLogInfo(Charmed->GetObjectGuid()));

			return;
		}
		// Gnomish Mind Control Cap
		case 13180:
		{
			if (!m_originalCaster)
				return;

			// [WowWiki] Possible malfunctions
			// - Fails with no effect, but cooldown is trigged.
			// - User becomes mind controlled by target. However, if the Gnomish Mind Control Cap is used while in a shapeshift form (such as cat form), this effect does not trigger.
			switch (urand(0, 5))
			{
			case 0:
				break;
			case 1:
				if (m_originalCaster->GetShapeshiftForm() == FORM_NONE)
					unitTarget->CastSpell(m_originalCaster, 13181, true);
				break;
			default:
				m_originalCaster->CastSpell(unitTarget, 13181, true);
				break;
			}
			return;
		}
		// BRD : Invocation de Theldren
		case 27517:
		{
			if (!m_originalCaster)
				return;

			if (InstanceData* pInst = m_originalCaster->GetInstanceData())
				pInst->CustomSpellCasted(27517, m_originalCaster, unitTarget);
			return;
		}
		//case 34460: //clear marks
		//{
		//	CreatureAI::ClearTargetIcon();
		//	return;
		//}
		//V+ AFK check
		case 34734:
		{
			Player* pPlayer = m_caster->ToPlayer();
			if (!pPlayer)
				return;

			// WSG flagcarriers
			if (pPlayer->HasAura(23333) || pPlayer->HasAura(23335))
				return;

			pPlayer->RemoveAurasDueToSpell(34733);
			pPlayer->ToggleAFK();
			return;
		}
		//AoE AFK
		case 34833:
		{
			if (Player* pPlayer = unitTarget->ToPlayer())
				pPlayer->ToggleAFK(),
				pPlayer->LeaveBattleground();

			return;
		}
		//AC AoE Gold Steal/Give
		case 35096:
		{
			if (Player* pPlayer = unitTarget->ToPlayer())
				if (pPlayer->IsFriendlyTo(m_caster))
				{
					if (pPlayer->HasAura(34975))
					{
						if (SpellAuraHolder* holder = pPlayer->GetSpellAuraHolder(34975))
						{
							if (holder->GetStackAmount() < 250)
								holder->ModStackAmount(1);
						}
					}
					else
						pPlayer->AddAura(34975);
				}
				else
				{
					if (pPlayer->HasAura(34975))
					{
						if (SpellAuraHolder* holder = pPlayer->GetSpellAuraHolder(34975))
						{
							if (holder->GetStackAmount() > 1)
								holder->ModStackAmount(-1);
							else
								pPlayer->RemoveAurasDueToSpell(34975);
						}
					}
				}

			return;
		}
		//Toad Explode
		case 24062:
		{
			unitTarget->CastSpell(unitTarget, 24063, true);
			return;
		}
		//V+ Queue Marks Accumulating
		case 34762:
		case 34764:
		case 34766:
		case 34768:
		{
			if (m_casterUnit->HasAura(34856))
				return;

			if (m_casterUnit->IsTaxiFlying())
				return;

			int32 MarkSpellId = 0;
			switch (m_spellInfo->Id)
			{
				case 34762: MarkSpellId = 36330; break;	//WSG
				case 34764: MarkSpellId = 36331; break;	//AB
				case 34766: MarkSpellId = 36332; break;	//AV
				case 34768: MarkSpellId = 36333; break;	//AC
			}

			if (urand(0, 99) < 70)
				m_casterUnit->CastSpell(m_casterUnit, MarkSpellId, true);

			return;
		}
		case 34985:
		{
			if (Unit* SumTarget = (((Player*)m_casterUnit)->GetSelectedUnit()))
				/*if (Unit* SumTarget = sObjectMgr.GetPlayer(((Player*)m_caster)->GetSelectionGuid()))*/
			{
				if (SumTarget != m_casterUnit)
					m_casterUnit->NearTeleportTo(SumTarget->GetPositionX(), SumTarget->GetPositionY(), SumTarget->GetPositionZ() + 3.0f, SumTarget->GetOrientation(), TELE_TO_NOT_LEAVE_TRANSPORT | TELE_TO_NOT_UNSUMMON_PET | TELE_TO_SPELL);
			}

			return;
		}
		//Azus Check TP
		case 35424:
		{
			float desX = -11793.0f;
			float desY = -1631.0f;
			float desZ = 21.2f;
			float desO = 1.5f;

			float random_X = frand(0.0f, 5.0f);
			float random_Y = frand(0.0f, 5.0f);

			desX += random_X;
			desY += random_Y;

			if (Player* pl = (Player*)unitTarget)
				pl->TeleportTo(309, desX, desY, desZ, desO, TELE_TO_NOT_UNSUMMON_PET);

			return;
		}
		// Shala'thalas Runestone
		case 35408:
		{
			uint32 random_pos = urand(0, 8);
			float random_X = frand(0.0f, 5.0f);
			float random_Y = frand(0.0f, 5.0f);

			float const PortalLoc[9][4] =
			{
				{ -608.6f, -108.2f, 356.4f, 4.5f},
				{ -611.2f, -238.7f, 382.3f, 0.7f},
				{ -695.9f, -201.1f, 359.6f, 0.7f},
				{ -589.6f, -331.5f, 359.6f, 0.7f},
				{ -523.4f, -401.6f, 314.6f, 0.9f},
				{ -384.5f, -293.9f, 300.1f, 1.4f},
				{ -271.1f, -211.1f, 298.1f, 4.2f},
				{ -415.8f, -10.76f, 311.5f, 2.8f},
				{ -500.3f, -68.35f, 320.6f, 0.1f}
			};

			float desX = PortalLoc[random_pos][0];
			float desY = PortalLoc[random_pos][1];
			float desZ = PortalLoc[random_pos][2];
			float desO = PortalLoc[random_pos][3];

			desX += random_X;
			desY += random_Y;

			if (Player* pl = (Player*)unitTarget)
				pl->TeleportTo(37, desX, desY, desZ, desO, TELE_TO_NOT_UNSUMMON_PET);

			return;
		}
		//AC Evacuation teleport
		case 34981:
		{
			uint32 random_pos = urand(0, 5);

			float const ALoc[6][4] =
			{
				{ 1147.7f, 365.4f, 351.3f, 1.0f},
				{ 1187.7f, 194.1f, 358.8f, 2.3f},
				{ 1241.1f, 236.5f, 351.9f, 3.0f},
				{ 1050.9f, 366.5f, 328.9f, 4.8f},
				{ 1060.9f, 236.3f, 329.6f, 1.2f},
				{ 1093.3f, 309.8f, 338.2f, 0.6f}
			};

			float const HLoc[6][4] =
			{
				{ 3.6f, 992.5f, 356.0f, 4.0f},
				{ 20.7f, 920.6f, 356.9f, 6.3f},
				{ 80.9f, 1180.3f, 370.6f, 4.0f},
				{ 33.0f, 1073.7f, 352.3f, 4.3f},
				{ 48.7f, 996.3f, 358.6f, 3.2f},
				{ -28.8f, 1028.9f, 365.8f, 5.3f}
			};

			float desX = 0.0f;
			float desY = 0.0f;
			float desZ = 0.0f;
			float desO = 0.0f;

			if (Player* pl = (Player*)unitTarget)
			{
				if (pl->GetTeam() == HORDE)
				{
					desX = HLoc[random_pos][0];
					desY = HLoc[random_pos][1];
					desZ = HLoc[random_pos][2];
					desO = HLoc[random_pos][3];
				}
				else
				{
					desX = ALoc[random_pos][0];
					desY = ALoc[random_pos][1];
					desZ = ALoc[random_pos][2];
					desO = ALoc[random_pos][3];
				}

				pl->TeleportTo(37, desX, desY, desZ, desO, TELE_TO_NOT_UNSUMMON_PET);
			}

			return;
		}
		case 14537:                                 // Six Demon Bag
		{
			if (!unitTarget)
				return;

			Unit* newTarget = unitTarget;
			uint32 spell_id = 0;
			uint32 roll = urand(0, 99);
			if (roll < 25)                          // Fireball (25% chance)
				spell_id = 11921;
			else if (roll < 50)                     // Frostbolt (25% chance)
				spell_id = 13322;
			else if (roll < 70)                     // Chain Lighting (20% chance)
				spell_id = 21179;
			else if (roll < 77)                     // Polymorph (10% chance, 7% to target)
				spell_id = 13323;
			else if (roll < 80)                     // Polymorph (10% chance, 3% to self, backfire)
			{
				spell_id = 13323;
				newTarget = m_casterUnit;
			}
			else if (roll < 95)                     // Enveloping Winds (15% chance)
				spell_id = 25189;
			else                                    // Summon Felhund minion (5% chance)
			{
				spell_id = 14642;
				newTarget = m_casterUnit;
			}

			if (newTarget)
				m_caster->CastSpell(newTarget, spell_id, true, m_CastItem);
			return;
		}
		case 34144: //vanilla+ Sacrifice res dummy trigger pPlayer->CastSpell(pPlayer, 33849, true, nullptr);
		{
			Player* pPlayer = m_caster->ToPlayer();
			if (!pPlayer)
				return;

			uint32 roll = urand(0, 99);

			if (roll < 10)   //10% proc chance
				pPlayer->CastSpell(pPlayer, 33849, true, nullptr);
			return;
		}
		case 17009:                                 // Voodoo
		{
			if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
				return;

			uint32 spell_id = 0;
			switch (urand(0, 6))
			{
			case 0:
				spell_id = 16707;
				break;    // Hex
			case 1:
				spell_id = 16708;
				break;    // Hex
			case 2:
				spell_id = 16709;
				break;    // Hex
			case 3:
				spell_id = 16711;
				break;    // Grow
			case 4:
				spell_id = 16712;
				break;    // Special Brew
			case 5:
				spell_id = 16713;
				break;    // Ghostly
			case 6:
				spell_id = 16716;
				break;    // Launch
			}

			m_caster->CastSpell(unitTarget, spell_id, true, nullptr, nullptr, m_originalCasterGUID, m_spellInfo);
			return;
		}

		case 25720:     //spell Place Loot / Quest 8606 Decoy!
		{
			if (!m_originalCaster)
				return;
			if (m_originalCaster->GetTypeId() != TYPEID_PLAYER)
				return;

			sLog.outString("Quest 8606 item Dummy Effect.");
			Player* pPlayer = (Player*)m_originalCaster;
			// Check for buff Narain!
			if (!pPlayer->HasAura(25688))
				return;
			// Start event 9527 using custom spell to avoid db error
			pPlayer->CastSpell(pPlayer, 33031, true);
			// Delete item Bag of Gold (21041)
			pPlayer->DestroyItemCount(21041, -1, true, false);
			return;
		}
		case 3360:                                  // Curse of the Eye
		{
			if (!unitTarget)
				return;

			uint32 spell_id = (unitTarget->getGender() == GENDER_MALE) ? 10651 : 10653;

			m_caster->CastSpell(unitTarget, spell_id, true);
			return;
		}
		case 7671:                                  // Transformation (human<->worgen)
		{
			if (!unitTarget)
				return;

			// Transform Visual
			unitTarget->CastSpell(unitTarget, 24085, true);
			return;
		}
		case 8063:                                  // Deviate Fish
		{
			Player* pPlayer = m_caster->ToPlayer();
			if (!pPlayer)
				return;

			uint32 spell_id = 0;
			switch (urand(1, 5))
			{
			case 1:
				spell_id = 8064;
				break;     // Sleepy
			case 2:
				spell_id = 8065;
				break;     // Invigorate
			case 3:
				spell_id = 8066;
				break;     // Shrink
			case 4:
				spell_id = 8067;
				break;     // Party Time!
			case 5:
				spell_id = 8068;
				break;     // Healthy Spirit
			}
			pPlayer->CastSpell(pPlayer, spell_id, true, nullptr);
			return;
		}
		case 8213:                                  // Savory Deviate Delight
		{
			Player* pPlayer = m_caster->ToPlayer();
			if (!pPlayer)
				return;

			uint32 spell_id = 0;
			uint32 spells[6] = {
				(pPlayer->getGender() == GENDER_MALE ? 8219u : 8220u), // Flip Out - ninja
				(pPlayer->getGender() == GENDER_MALE ? 8221u : 8222u), // Yaaarrrr - pirate
				8223u, // Oops - goo
				8224u, // Cowardice
				8226u, // Fake Death
				8215u  // Rapid Cast
			};

			// Had additional effects before BWL patch.
			if (sWorld.GetWowPatch() < WOW_PATCH_106 && sWorld.getConfig(CONFIG_BOOL_ACCURATE_SPELL_EFFECTS))
				spell_id = spells[urand(0, 5)];
			else
				spell_id = spells[urand(0, 4)];

			pPlayer->CastSpell(pPlayer, spell_id, true, nullptr);
			return;
		}
		case 8593:                                  // Symbol of life (restore creature to life)
		{
			if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
				return;

			((Creature*)unitTarget)->SetDeathState(JUST_ALIVED);
			return;
		}
		case 34518:                                  // Hate to Zero(Reset)
		{
			if (!m_casterUnit)
				return;


			ThreatList const& tList = m_casterUnit->getThreatManager().getThreatList();
			for (ThreatList::const_iterator itr = tList.begin(); itr != tList.end(); ++itr)
			{
				Unit* pUnit = m_casterUnit->GetMap()->GetUnit((*itr)->getUnitGuid());

				if (pUnit && m_casterUnit->getThreatManager().getThreat(pUnit))
					m_casterUnit->getThreatManager().modifyThreatPercent(pUnit, -100);
			}
			return;
		}
		case 9204:                                  // Hate to Zero
		{
			if (!unitTarget || !m_casterUnit)
				return;

			m_casterUnit->getThreatManager().modifyThreatPercent(unitTarget, -100);
			return;
		}
		case 19707:                                  // Hate to 50%
		{
			if (!unitTarget || !m_casterUnit)
				return;

			m_casterUnit->getThreatManager().modifyThreatPercent(unitTarget, -50);
			return;
		}
		case 9976:                                  // Polly Eats the E.C.A.C.
		{
			if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
				return;

			// Summon Polly Jr.
			unitTarget->CastSpell(unitTarget, 9998, true);

			((Creature*)unitTarget)->ForcedDespawn(100);
			return;
		}
		case 34310:
		{
			if (!unitTarget || !m_casterUnit)
				return;

			unitTarget->CastSpell(m_casterUnit, 34308, true);
			return;

		}
		case 12021: //Fixate
		{
			if (!unitTarget || !m_casterUnit)
				return;

			unitTarget->CastSpell(m_casterUnit, 34375, true);
			m_casterUnit->AddAura(34867);
			return;

		}
		case 12162:                                 // Deep wounds
		case 12850:                                 // (now good common check for this spells)
		case 12868:
		{
			if (!unitTarget || !m_casterUnit)
				return;

			float damage;
			// DW should benefit of attack power, damage percent mods etc.
			// TODO: check if using offhand damage is correct and if it should be divided by 2
			//if (m_casterUnit->haveOffhandWeapon() && m_casterUnit->getAttackTimer(BASE_ATTACK) > m_casterUnit->getAttackTimer(OFF_ATTACK))
			//	damage = (m_casterUnit->GetFloatValue(UNIT_FIELD_MINOFFHANDDAMAGE) + m_casterUnit->GetFloatValue(UNIT_FIELD_MAXOFFHANDDAMAGE)) / 2;
			//else
			//	damage = (m_casterUnit->GetFloatValue(UNIT_FIELD_MINDAMAGE) + m_casterUnit->GetFloatValue(UNIT_FIELD_MAXDAMAGE)) / 2;

			//switch (m_spellInfo->Id)
			//{
			//case 12162:
			//	damage *= 0.10f; //vanilla+
			//	break;
			//case 12850:
			//	damage *= 0.2f; //v+
			//	break;
			//case 12868:
			//	damage *= 0.3f; //v+
			//	break;
			//default:
			//	sLog.outError("Spell::EffectDummy: Spell %u not handled in DW", m_spellInfo->Id);
			//	return;
			//};


			//int32 deepWoundsDotBasePoints0 = int32(damage / 4);
			/*m_casterUnit->CastCustomSpell(unitTarget, 12721, &deepWoundsDotBasePoints0, nullptr, nullptr, true, nullptr);*/
			m_casterUnit->CastSpell(unitTarget, 12721, true);
			return;
		}
		//case 33648:                                 //vanilla+ Aftershock
		//{
		//	if (!m_targets.getUnitTarget())
		//		return;

		//	// for caster applied auras only FLAMESHOCK
		//	bool flamefound = false;
		//	auto const & mPeriodic = m_targets.getUnitTarget()->GetAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
		//	for (auto i = mPeriodic.begin(); i != mPeriodic.end(); ++i)
		//	{
		//		// Frost Shock
		//		if ((*i)->GetSpellProto()->IsFitToFamily<SPELLFAMILY_SHAMAN, CF_SHAMAN_FLAME_SHOCK>() &&
		//			(*i)->GetCasterGuid() == m_caster->GetObjectGuid())
		//		{
		//			flamefound = true;
		//			m_caster->CastSpell(unitTarget, 33652, true); //Proc burst effect
		//			break;
		//		}
		//	}

		//	// for caster applied auras only FROSTSHOCK
		//	bool frostfound = false;
		//	auto const & slowingAuras = m_targets.getUnitTarget()->GetAurasByType(SPELL_AURA_MOD_DECREASE_SPEED);
		//	for (auto i = slowingAuras.begin(); i != slowingAuras.end(); ++i)
		//	{
		//		// Frost Shock
		//		if ((*i)->GetSpellProto()->IsFitToFamily<SPELLFAMILY_SHAMAN, CF_SHAMAN_FROST_SHOCK>() &&
		//			(*i)->GetCasterGuid() == m_caster->GetObjectGuid())
		//		{
		//			frostfound = true;
		//			m_caster->CastSpell(unitTarget, 33653, true); //Proc stun effect
		//			unitTarget->RemoveAurasByCasterSpell((*i)->GetId(), m_caster->GetObjectGuid());
		//			break;
		//		}
		//	}

		//	// for caster applied auras only EARTHSHOCK
		//	bool earthfound = false;
		//	auto const & nModDetectRange = m_targets.getUnitTarget()->GetAurasByType(SPELL_AURA_MOD_DETECT_RANGE);
		//	for (auto i = nModDetectRange.begin(); i != nModDetectRange.end(); ++i)
		//	{
		//		// Earth Shock
		//		if ((*i)->GetSpellProto()->IsFitToFamily<SPELLFAMILY_SHAMAN, CF_SHAMAN_EARTH_SHOCK>() &&
		//			(*i)->GetCasterGuid() == m_caster->GetObjectGuid())
		//		{
		//			earthfound = true;
		//			m_caster->CastSpell(unitTarget, 33654, true); //Proc taunt effect
		//			unitTarget->RemoveAurasByCasterSpell((*i)->GetId(), m_caster->GetObjectGuid());
		//			break;
		//		}
		//	}
		//	return;
		//}
		case 34419: //v+ Fake Death UD
		{
			if (!m_casterUnit)
				return;

			if ((m_casterUnit->GetMapId() == 0) || (m_casterUnit->GetMapId() == 1) || m_casterUnit->ToPlayer()->InBattleGround())
				m_casterUnit->CastSpell(m_casterUnit, 34362, true, nullptr);
			else
			{
				m_casterUnit->CastSpell(m_casterUnit, 34363, true, nullptr);
			}
			return;
		}
		case 34805:
		{
			if (!m_casterUnit)
				return;

			if (m_casterUnit->isInCombat())
				return;

			int32 healthModSpellBasePoints0 = m_casterUnit->GetMaxHealth() * 0.05;
			m_casterUnit->CastCustomSpell(m_casterUnit, 34806, &healthModSpellBasePoints0, nullptr, nullptr, true, nullptr);
			return;
		}
		case 33764:                                 //Vanilla+ Untamed Heart
		{
			if (!m_casterUnit)
				return;

			int32 healthModSpellBasePoints0 = m_casterUnit->GetMaxHealth() * 0.3;
			m_casterUnit->CastCustomSpell(m_casterUnit, 34114, &healthModSpellBasePoints0, nullptr, nullptr, true, nullptr);
			return;
		}
		case 23602:                                 //Shield Spec heal
		{
			if (!m_casterUnit)
				return;

			int32 healthModShieldSpellBasePoints0 = m_casterUnit->GetMaxHealth() * 0.01;
			m_casterUnit->CastCustomSpell(m_casterUnit, 34115, &healthModShieldSpellBasePoints0, nullptr, nullptr, true, nullptr);
			return;
		}
		case 36038:                                 //Improved Blocking SM heal
		{
			if (!m_casterUnit)
				return;

			int32 healthModShieldSpellBasePoints0 = m_casterUnit->GetMaxHealth() * 0.01;
			m_casterUnit->CastCustomSpell(m_casterUnit, 36039, &healthModShieldSpellBasePoints0, nullptr, nullptr, true, nullptr);
			return;
		}
		case 36171:                                 //Improved Blocking SM Boss heal
		{
			if (!m_casterUnit)
				return;

			int32 healthModShieldSpellBasePoints0 = m_casterUnit->GetMaxHealth() * 0.001;
			m_casterUnit->CastCustomSpell(m_casterUnit, 36172, &healthModShieldSpellBasePoints0, nullptr, nullptr, true, nullptr);
			return;
		}
		//case 34037:                                 //Second Wind mana regen
		//{
		//	if (!m_casterUnit)
		//		return;

		//	int32 manaModperc = m_casterUnit->GetMaxPower(POWER_MANA) * 0.01;
		//	m_casterUnit->CastCustomSpell(m_casterUnit, 34116, &manaModperc, nullptr, nullptr, true, nullptr);
		//	return;
		//}
		//Second Wind(righteo)
		case 34529:
		{
			if (!m_casterUnit)
				return;

			if (m_casterUnit->GetPower(POWER_MANA) > m_casterUnit->GetMaxPower(POWER_MANA) / 5)
				return;

			m_casterUnit->CastSpell(m_casterUnit, 34530, true);

			return;
		}
		case 34178:                                 //Shaman shield spec
		{
			if (!m_casterUnit)
				return;

			int32 SmanaModperc = m_casterUnit->GetMaxPower(POWER_MANA) * 0.01;
			m_casterUnit->CastCustomSpell(m_casterUnit, 34179, &SmanaModperc, nullptr, nullptr, true, nullptr);
			return;
		}
		case 33777:                                 //Survival Instincts R1
		case 33778:
		case 33779:
		case 33780:
		case 33781:
		{
			if (!m_casterUnit)
				return;


			int32 ModHPBasePoints0 = int32(m_casterUnit->GetMaxHealth());

			switch (m_spellInfo->Id)
			{
			case 33777:
				ModHPBasePoints0 *= 0.01f; //r1
				break;
			case 33778:
				ModHPBasePoints0 *= 0.02f; //r2
				break;
			case 33779:
				ModHPBasePoints0 *= 0.03f; //r3
				break;
			case 33780:
				ModHPBasePoints0 *= 0.04f; //r4
				break;
			case 33781:
				ModHPBasePoints0 *= 0.05f; //r5
				break;
			default:
				sLog.outError("Spell::EffectDummy: Spell %u not handled in DW", m_spellInfo->Id);
				return;
			};

			m_casterUnit->CastCustomSpell(m_casterUnit, 34117, &ModHPBasePoints0, nullptr, nullptr, true, nullptr);
			return;
		}
		case 33941:                                 //Warlock's Distortion
		{
			if (!m_casterUnit)
				return;

			int32 ModHPBasePoints0 = int32(m_casterUnit->GetMaxHealth() * 0.1);
			m_casterUnit->CastCustomSpell(m_casterUnit, 34131, &ModHPBasePoints0, nullptr, nullptr, true, nullptr);
			return;
		}
		case 34245:                                 //Druid Starfall
		case 34246:
		case 34247:
		case 34248:
		case 34249:
		{
			if (!unitTarget || !m_casterUnit)
				return;

			if (unitTarget == m_casterUnit)
				return;

			uint32 CDaura = 0;

			switch (m_spellInfo->Id)
			{
			case 34245:
				CDaura = 34250;
				break;
			case 34246:
				CDaura = 34251;
				break;
			case 34247:
				CDaura = 34252;
				break;
			case 34248:
				CDaura = 34253;
				break;
			case 34249:
				CDaura = 34254;
				break;
			default:
				sLog.outError("Spell::EffectDummy: Spell %u not handled in DW", m_spellInfo->Id);
				return;
			};

			if (m_casterUnit->IsWithinDistInMap(unitTarget, 8.0f) && !m_casterUnit->HasAura(CDaura))
				m_casterUnit->CastSpell(unitTarget, 16922, true, nullptr),
				m_casterUnit->CastSpell(m_casterUnit, CDaura, true, nullptr);
			return;
		}
		case 34183:                                 //Hunter's Two heart one soul (spirit bond)
		case 34184:
		case 34185:
		case 34210:
		case 34211:
		{
			if (!m_casterUnit)
				return;


			float SB_stat_mod = 0;

			switch (m_spellInfo->Id)
			{
			case 34183:
				SB_stat_mod = 0.06f;
				break;
			case 34184:
				SB_stat_mod = 0.12f;
				break;
			case 34185:
				SB_stat_mod = 0.18f;
				break;
			case 34210:
				SB_stat_mod = 0.24f;
				break;
			case 34211:
				SB_stat_mod = 0.3f;
				break;
			default:
				sLog.outError("Spell::EffectDummy: Spell %u not handled in DW", m_spellInfo->Id);
				return;
			};

			int32 AP_value = m_casterUnit->GetTotalAttackPowerValue(BASE_ATTACK) * SB_stat_mod;
			int32 crit_value = round(m_casterUnit->GetFloatValue(PLAYER_CRIT_PERCENTAGE) * SB_stat_mod);
			int32 sta_value = m_casterUnit->GetTotalStatValue(STAT_STAMINA) * SB_stat_mod;

			m_casterUnit->CastCustomSpell(m_casterUnit, 34186, &AP_value, &crit_value, &sta_value, true, nullptr);
			return;
		}
		case 34224:                                 //Spirit Bond heal
		case 34225:
		case 34226:
		case 34227:
		case 34228:
		{
			if (!m_casterUnit)
				return;

			int32 SB_heal_ammount = int32(unitTarget->GetMaxHealth()) * 0.01;

			/*switch (m_spellInfo->Id)
			{
			case 34224:
				SB_heal_ammount *= 0.01f;
				break;
			case 34225:
				SB_heal_ammount *= 0.02f;
				break;
			case 34226:
				SB_heal_ammount *= 0.03f;
				break;
			case 34227:
				SB_heal_ammount *= 0.04f;
				break;
			case 34228:
				SB_heal_ammount *= 0.05f;
				break;
			default:
				sLog.outError("Spell::EffectDummy: Spell %u not handled in DW", m_spellInfo->Id);
				return;
			};*/

			m_casterUnit->CastCustomSpell(unitTarget, 34229, &SB_heal_ammount, nullptr, nullptr, true, nullptr);
			return;
		}
		case 34846:
		{
			if (m_casterUnit->HasAura(34850))
			{
				m_casterUnit->RemoveAurasDueToSpell(34847);
				m_casterUnit->RemoveAurasDueToSpell(34848);

				return;
			}

			std::list<Player*> players;
			MaNGOS::AnyPlayerInObjectRangeCheck checker(m_casterUnit, 120.0f, false);
			MaNGOS::PlayerListSearcher<MaNGOS::AnyPlayerInObjectRangeCheck> searcher(players, checker);
			Cell::VisitWorldObjects(m_casterUnit, searcher, 120.0f);


			for (std::list<Player*>::iterator itr = players.begin(); itr != players.end();)
			{
				if (!(*itr)->IsAttackingFactionLeader() || !((*itr)->GetReactionTo(m_casterUnit) == REP_HOSTILE))
					itr = players.erase(itr);
				else
					++itr;
			}

			if (players.size() > 9)
			{
				m_casterUnit->CastSpell(m_casterUnit, 34847, true);
				m_casterUnit->CastSpell(m_casterUnit, 34848, true);
				if (!m_casterUnit->HasAura(34849))
					m_casterUnit->CastSpell(m_casterUnit, 34849, true);
			}

			return;
		}
		//Seeker's Bloodthirsty
		case 34944:
		{
			if (Unit* target = m_casterUnit->ToPlayer()->GetSelectedUnit())
			{
				if (target == m_casterUnit)
					return;
				if (!target->IsFriendlyTo(m_casterUnit))
					if (target->GetHealthPercent() < 20.0f)
					{
						if (!target->isDead())
							if (m_casterUnit->GetDistance(target) < 50.0f)
							{
								m_casterUnit->AddAura(34941);
								m_casterUnit->AddAura(34942);
							}
					}
			}
			return;
		}
		//Zerker Blood(berserker blood)
		case 33414:
		case 33415:
		case 33416:
		case 33417:
		case 33418:
		{
			if (!unitTarget)
				return;

			uint32 zerkHPperc = uint32((float(m_casterUnit->GetHealth()) / m_casterUnit->GetMaxHealth()) * 100);
			int32 zerk_mod = 1;
			uint32 ZerkCap = 40;
			float rankPerc = 1.5f;
			uint32 rankHP = 40;

			switch (m_spellInfo->Id)
			{
			case 33414:
				rankHP = 1;
				rankPerc = 2.5f;
				break;
			case 33415:
				rankHP = 10;
				rankPerc = 2.25f;
				break;
			case 33416:
				rankHP = 20;
				rankPerc = 2.0f;
				break;
			case 33417:
				rankHP = 30;
				rankPerc = 1.75f;
				break;
			case 33418:
				rankHP = 40;
				rankPerc = 1.5f;
				break;
			default:
				break;
			}

			//if (m_spellInfo->Id == 33418)  //R5
			//	rankHP = 40,
			//	rankPerc = 1.5f;
			//else if (m_spellInfo->Id == 33414) //R1
			//	rankHP = 1,
			//	rankPerc = 2.5f;
			//else if (m_spellInfo->Id == 33415)  //R2
			//	rankHP = 10,
			//	rankPerc = 2.25f;
			//else if (m_spellInfo->Id == 33416)  //R3
			//	rankHP = 20,
			//	rankPerc = 2.0f;
			//else if (m_spellInfo->Id == 33417)  //R4
			//	rankHP = 30,
			//	rankPerc = 1.75f;

			if (zerkHPperc <= rankHP)
				zerk_mod = ZerkCap;
			else if (zerkHPperc < 100 && zerkHPperc > 0)
				zerk_mod = roundf((100 - zerkHPperc) / rankPerc + 0.5f);

			int32 zerk_mod2 = zerk_mod / 3;


			m_casterUnit->CastCustomSpell(m_casterUnit, 34113, &zerk_mod, &zerk_mod, &zerk_mod2, true, nullptr);
			return;
		}
		//v+ Warrior's Prowess Constitution (str from stam)
		case 34128:
		{
			if (!unitTarget)
				return;

			uint32 stamHP = uint32(float(m_casterUnit->GetHealth()));
			uint32 prowHPperc = uint32((float(m_casterUnit->GetHealth()) / m_casterUnit->GetMaxHealth()) * 100);
			int32 str_mod = 0;
			int32 agi_mod = 0;
			int32 spi_mod = 0;

			if (stamHP > 0)
			{
				str_mod = stamHP / 100;
				agi_mod = str_mod;
				spi_mod = str_mod;
			}

			m_casterUnit->CastCustomSpell(m_casterUnit, 34129, &str_mod, &agi_mod, &spi_mod, true, nullptr);

			if (prowHPperc >= 50) //immune to disease
				m_casterUnit->CastSpell(m_casterUnit, 34137, true, nullptr);
			if (prowHPperc >= 75) //immune to poison
				m_casterUnit->CastSpell(m_casterUnit, 34138, true, nullptr);
			return;
		}
		// Hederine Bodyguard Aura
		case 34816:
		{
			if (!unitTarget)
				return;

			if (m_casterUnit->HasAura(34817))
				m_casterUnit->CastSpell(m_casterUnit, 34818, true);

			return;
		}
		case 13120:                                 // net-o-matic
		{
			if (!unitTarget)
				return;

			uint32 spell_id = 0;

			uint32 roll = urand(0, 99);

			if (roll < 10)                           // 10% for 30 sec self root (off-like chance unknown)
				spell_id = 16566;
			else if (roll < 20)                      // 10% for 20 sec root, charge to target (off-like chance unknown)
				spell_id = 13119;                   // triggers 13139
			else                                    // normal root
				spell_id = 13099;

			m_caster->CastSpell(unitTarget, spell_id, true, nullptr);
			return;
		}
		case 13535:                                 // Tame Beast
		{
			if (!m_originalCaster || m_originalCaster->GetTypeId() != TYPEID_PLAYER)
				return;

			Creature* channelTarget = m_originalCaster->GetMap()->GetCreature(m_originalCaster->GetChannelObjectGuid());

			if (!channelTarget)
				return;

			m_originalCaster->CastSpell(channelTarget, 13481, true, nullptr, nullptr, m_originalCasterGUID, m_spellInfo);
			return;
		}
		case 13567:                                 // Dummy Trigger
		{
			// can be used for different aura triggering, so select by aura
			if (!m_triggeredByAuraSpell || !unitTarget)
				return;

			switch (m_triggeredByAuraSpell->Id)
			{
			case 26467:                         // Persistent Shield
				m_caster->CastCustomSpell(unitTarget, 26470, &damage, nullptr, nullptr, true);
				break;
			default:
				sLog.outError("EffectDummy: Non-handled case for spell 13567 for triggered aura %u", m_triggeredByAuraSpell->Id);
				break;
			}
			return;
		}
		case 14185:                                 // Preparation Rogue
		{
			if (m_caster->GetTypeId() != TYPEID_PLAYER)
				return;

			//immediately finishes the cooldown on certain Rogue abilities
			SpellCooldowns cm = ((Player*)m_caster)->GetSpellCooldownMap();
			for (SpellCooldowns::const_iterator itr = cm.begin(); itr != cm.end(); ++itr)
			{
				SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(itr->first);

				if (spellInfo->SpellFamilyName == SPELLFAMILY_ROGUE &&
					spellInfo->Id != m_spellInfo->Id && spellInfo->GetRecoveryTime() > 0)
					((Player*)m_caster)->RemoveSpellCooldown(itr->first, true);
			}
			return;
		}
		case 15998:                                 // Capture Worg Pup
		{
			if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
				return;

			Creature* creatureTarget = (Creature*)unitTarget;

			creatureTarget->ForcedDespawn();
			return;
		}
		case 16589:                                 // Noggenfogger Elixir
		{
			Player* pPlayer = m_caster->ToPlayer();
			if (!pPlayer)
				return;

			uint32 spell_id = 0;
			switch (urand(1, 3))
			{
			case 1:
				spell_id = 16595;
				break;
			case 2:
				spell_id = 16593;
				break;
			default:
				spell_id = 16591;
				break;
			}

			pPlayer->CastSpell(pPlayer, spell_id, true, nullptr);
			return;
		}
		case 17251:                                 // Spirit Healer Res
		{
			if (!unitTarget)
				return;

			Unit* caster = GetAffectiveCaster();

			if (caster && caster->GetTypeId() == TYPEID_PLAYER)
			{
				WorldPacket data(SMSG_SPIRIT_HEALER_CONFIRM, 8);
				data << unitTarget->GetObjectGuid();
				((Player*)caster)->GetSession()->SendPacket(&data);
			}
			return;
		}
		case 17271:                                 // Test Fetid Skull
		{
			if (!m_casterUnit)
				return;

			if (!itemTarget && m_caster->GetTypeId() != TYPEID_PLAYER)
				return;

			uint32 spell_id = roll_chance_i(50)
				? 17269                             // Create Resonating Skull
				: 17270;                            // Create Bone Dust


			m_casterUnit->CastSpell(m_casterUnit, spell_id, true, nullptr);
			return;
		}
		case 17770:                                 // Wolfshead Helm Energy
		{
			if (m_casterUnit)
				m_casterUnit->CastSpell(m_casterUnit, 29940, true, nullptr);
			return;
		}
		case 17950:
		{
			if (!unitTarget)
				return;

			// Shadow Portal
			const uint32 spell_list[6] = { 17863, 17939, 17943, 17944, 17946, 17948 };

			m_caster->CastSpell(unitTarget, spell_list[urand(0, 5)], true);
			return;
		}
		case 18350:                                 // Dummy Trigger
		{
			if (unitTarget->GetTypeId() != TYPEID_PLAYER)
				return;

			// Need remove self if Lightning Shield not active
			Unit::SpellAuraHolderMap const& auras = unitTarget->GetSpellAuraHolderMap();
			for (Unit::SpellAuraHolderMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
			{
				SpellEntry const* spell = itr->second->GetSpellProto();
				if (spell->IsFitToFamily<SPELLFAMILY_SHAMAN, CF_SHAMAN_LIGHTNING_SHIELD>())
					return;
			}
			unitTarget->RemoveAurasDueToSpell(28820);
			return;
		}
		case 19411:                                 // Lava Bomb
		case 20474:                                 // Lava Bomb
		{
			if (!unitTarget)
				return;

			unitTarget->CastSpell(unitTarget, 20494, true);
			return;
		}
#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_8_4
		case 19593:                                // Egg Explosion (Buru)
		{
			if (!unitTarget)
				return;

			if (!m_originalCaster)
				return;

			float tempDamage = 0.0f;
			float distance = m_originalCaster->GetDistance(unitTarget);
			distance = 1.0 - (distance / 25.0f);

			if (unitTarget->isAlive())
			{
				if (unitTarget->IsPlayer())
				{
					tempDamage = 500 * distance;
					if (tempDamage < 100)
						tempDamage = 100;
				}
				else
				{
					if (unitTarget->GetEntry() == 15370 && unitTarget->GetHealthPercent() > 20.0f)
						tempDamage = 45000 * distance;
					else
						return;
				}

				m_originalCaster->DealDamage(unitTarget, tempDamage, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
			}
			return;
		}
#endif
		case 20572:                                 // Blood Fury
		{
			Player* pPlayer = m_caster->ToPlayer();
			if (!pPlayer)
				return;

#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_8_4
			pPlayer->CastSpell(pPlayer, 23230, true);
#endif

			damage = damage * (pPlayer->GetInt32Value(UNIT_FIELD_ATTACK_POWER)) / 100;
			if (damage > 0)
				pPlayer->CastCustomSpell(pPlayer, 23234, &damage, nullptr, nullptr, true, nullptr);
			return;
		}
		case 20577:                                 // Cannibalize
		{
			if (m_casterUnit && (unitTarget || corpseTarget))
				m_casterUnit->CastSpell(m_casterUnit, 20578, true, nullptr);
			return;
		}
		case 36258:	// Corpseburner
		{
			if (m_casterUnit)
			{
				if (unitTarget)
					m_casterUnit->CastSpell(unitTarget->GetPositionX(), unitTarget->GetPositionY(), unitTarget->GetPositionZ(), 36260, true);
				else if (corpseTarget)
					m_casterUnit->CastSpell(corpseTarget->GetPositionX(), corpseTarget->GetPositionY(), corpseTarget->GetPositionZ(), 36260, true);
			}

			return;
		}
		case 34693: //Lady Hederine Sonic Lash
		{
			if (unitTarget && m_casterUnit)
			{
				if (m_casterUnit->getThreatManager().getThreat(unitTarget))
					m_casterUnit->getThreatManager().modifyThreatPercent(unitTarget, -30);
			}
			return;
		}
		case 20683: //Highlord's Justice
		case 10987: //geyser
		case 6253: //Backhand
		case 11891: //Antu'Sul knockback
		case 11902: //Ghazz'rill knkbk
		case 13586:                                 //v+ Aqua Jet
		case 30121:
		case 20686: //knockback LBRS
		case 21790:
		case 34393: //Azuregos Deep Freeze
		{
			if (unitTarget && m_casterUnit)
			{
				//Resets the target threat
				if (m_casterUnit->getThreatManager().getThreat(unitTarget))
					m_casterUnit->getThreatManager().modifyThreatPercent(unitTarget, -100);

				if (m_spellInfo->Id == 34393)
					unitTarget->CombatStop();
			}
			return;
		}
		//case 34954:									// Hakkar Vacuum
		//{
		//	if (unitTarget && m_casterUnit)
		//	{
		//		// cast summon player
		//		std::list<Creature*> add;
		//		GetCreatureListWithEntryInGrid(add, m_casterUnit, 25031, DEFAULT_VISIBILITY_INSTANCE);
		//		if (!add.empty())
		//		{
		//			for (auto iter = add.cbegin(); iter != add.cend(); ++iter)
		//			{
		//				auto add = *iter;
		//				if (add && add->isAlive())
		//					add->CastSpell(unitTarget, 34964, true);
		//			}
		//		}
		//	}
		//	return;
		//}
		case 21147:                                 // Arcane Vacuum
		{
			if (unitTarget && m_casterUnit)
			{
				// Spell used by Azuregos to teleport all the players to him
				// This also resets the target threat
				if (!m_casterUnit->CanHaveThreatList() || m_casterUnit->getThreatManager().isThreatListEmpty())
				{
					sLog.outError("DoResetThreat called for creature that either cannot have threat list or has empty threat list (m_creature entry = %d)", m_casterUnit->GetEntry());
					return;
				}

				ThreatList const& tList = m_casterUnit->getThreatManager().getThreatList();
				for (ThreatList::const_iterator itr = tList.begin(); itr != tList.end(); ++itr)
				{
					Unit* pUnit = m_casterUnit->GetMap()->GetUnit((*itr)->getUnitGuid());

					if (pUnit && m_casterUnit->getThreatManager().getThreat(pUnit))
						m_casterUnit->getThreatManager().modifyThreatPercent(pUnit, -100);
					if (m_casterUnit->GetDistance2d(pUnit) > 100.f)
						pUnit->CombatStop();
				}

				// cast summon player
				m_casterUnit->CastSpell(unitTarget, 21150, true);
			}

			return;
		}
		case 23019:                                 // Crystal Prison Dummy DND
		{
			if (!unitTarget || !unitTarget->isAlive() || unitTarget->GetTypeId() != TYPEID_UNIT || ((Creature*)unitTarget)->IsPet())
				return;

			Creature* creatureTarget = (Creature*)unitTarget;
			if (creatureTarget->IsPet())
				return;

			GameObject* pGameObj = new GameObject;

			Map* map = creatureTarget->GetMap();

			// create before death for get proper coordinates
			if (!pGameObj->Create(map->GenerateLocalLowGuid(HIGHGUID_GAMEOBJECT), 179644, map,
				creatureTarget->GetPositionX(), creatureTarget->GetPositionY(), creatureTarget->GetPositionZ(),
				creatureTarget->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, GO_ANIMPROGRESS_DEFAULT, GO_STATE_READY))
			{
				delete pGameObj;
				return;
			}

			pGameObj->SetRespawnTime(creatureTarget->GetRespawnTime() - time(nullptr));
			pGameObj->SetOwnerGuid(m_caster->GetObjectGuid());
			//Pose un soucis(Maxinus)
			// pGameObj->SetUInt32Value(GAMEOBJECT_LEVEL, m_caster->getLevel());
			pGameObj->SetSpellId(m_spellInfo->Id);
			pGameObj->SetWorldMask(m_caster->GetWorldMask());

			creatureTarget->ForcedDespawn();

			DEBUG_LOG("AddObject at SpellEfects.cpp EffectDummy");
			map->Add(pGameObj);

			WorldPacket data(SMSG_GAMEOBJECT_SPAWN_ANIM, 8);
			data << ObjectGuid(pGameObj->GetObjectGuid());
			m_caster->SendMessageToSet(&data, true);

			return;
		}
		case 23074:                                 // Arcanite Dragonling
		{
			if (!m_CastItem || !m_casterUnit)
				return;

			m_casterUnit->CastSpell(m_casterUnit, 19804, true, m_CastItem);
			return;
		}
		case 23075:                                 // Mithril Mechanical Dragonling
		{
			if (!m_CastItem || !m_casterUnit)
				return;

			m_casterUnit->CastSpell(m_casterUnit, 12749, true, m_CastItem);
			return;
		}
		case 23076:                                 // Mechanical Dragonling
		{
			if (!m_CastItem || !m_casterUnit)
				return;

			m_casterUnit->CastSpell(m_casterUnit, 4073, true, m_CastItem);
			return;
		}
		case 23133:                                 // Gnomish Battle Chicken
		{
			if (!m_CastItem || !m_casterUnit)
				return;

			m_casterUnit->CastSpell(m_casterUnit, 13166, true, m_CastItem);
			return;
		}
		case 23134:                                 // Goblin Bomb Dispenser
		{
			if (!m_CastItem || !m_casterUnit)
				return;

			uint32 roll = urand(0, 99);

			if (roll < 10)
				unitTarget->CastSpell(m_casterUnit, 13261, true);

			else
				m_casterUnit->CastSpell(m_casterUnit, 13258, true, m_CastItem);

			return;
		}
		case 4050:                                    // Explosive Sheep
		{
			Unit* pOwner = m_casterUnit ? m_casterUnit->GetCharmerOrOwner() : nullptr;
			if (pOwner && pOwner->GetTypeId() == TYPEID_PLAYER)
			{
				uint32 spellid = m_casterUnit->GetUInt32Value(UNIT_CREATED_BY_SPELL);
				SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(spellid);
				if (spellInfo)
					pOwner->CooldownEvent(spellInfo);
			}
			return;
		}
		case 23138:                                 // Gate of Shazzrah
		{
			if (!unitTarget)
				return;

			// Effect probably include a threat change, but it is unclear if fully
			// reset or just forced upon target for teleport (SMSG_HIGHEST_THREAT_UPDATE)

			// Gate of Shazzrah
			m_caster->CastSpell(unitTarget, 23139, true);
			return;
		}
		case 23448:                                 // Transporter Arrival - Ultrasafe Transporter: Gadgetzan - backfires
		{
			if (!m_casterUnit)
				return;

			int32 r = irand(0, 119);
			if (r < 20)                             // Transporter Malfunction - 1/6 polymorph
				m_casterUnit->CastSpell(m_casterUnit, 23444, true);
			else if (r < 100)                       // Evil Twin               - 4/6 evil twin
				m_casterUnit->CastSpell(m_casterUnit, 23445, true);

			return;
		}
		case 23453:                                 // Gnomish Transporter - Ultrasafe Transporter: Gadgetzan
		{
			if (!m_casterUnit)
				return;

			uint64 m_playerGUID = m_casterUnit->GetObjectGuid();

			if (roll_chance_i(50))                  // Gadgetzan Transporter         - success
				m_casterUnit->CastSpell(m_casterUnit, 23441, true);
			else // Gadgetzan Transporter Failure - failure
			{
				if (roll_chance_i(50))
					m_casterUnit->CastSpell(m_casterUnit, 23446, true);
				else
				{
					if (Player* pPlayer = m_casterUnit->GetMap()->GetPlayer(m_playerGUID))
						pPlayer->TeleportTo(1, -7341.38f, -3908.11f, 150.7f, 0.51f);
				}
			}
			return;
		}
		case 23645:                                 // Hourglass Sand
			if (m_casterUnit)
				m_casterUnit->RemoveAurasDueToSpell(23170); // Brood Affliction: Bronze
			return;
		case 23725:                                 // Gift of Life (warrior bwl trinket)
			if (m_casterUnit)
				m_casterUnit->CastSpell(m_casterUnit, 23782, true);
			return;
		case 24781:                                 // Dream Fog
		{
			if (m_caster->GetTypeId() != TYPEID_UNIT || !unitTarget)
				return;
			// TODO Note: Should actually not only AttackStart, but fixate on the target
			((Creature*)m_caster)->AI()->AttackStart(unitTarget);
			return;
		}
		case 24930:                                 // Hallow's End Treat
		{
			if (!m_casterUnit)
				return;

			uint32 spell_id = 0;

			switch (urand(1, 4))
			{
			case 1:
				spell_id = 24924;
				break;    // Larger and Orange
			case 2:
				spell_id = 24925;
				break;    // Skeleton
			case 3:
				spell_id = 24926;
				break;    // Pirate
			case 4:
				spell_id = 24927;
				break;    // Ghost
			}

			m_casterUnit->CastSpell(m_casterUnit, spell_id, true);
			return;
		}
		case 25860:                                 // Reindeer Transformation
		{
			if (!m_casterUnit)
				return;

			if (!m_casterUnit->HasAuraType(SPELL_AURA_MOUNTED))
				return;

			float speed = m_casterUnit->GetSpeedRate(MOVE_RUN);

			m_casterUnit->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);

			//5 different spells used depending on mounted speed
			if (speed >= 2.0f)
				m_casterUnit->CastSpell(m_casterUnit, 25859, true); //100% ground Reindeer
			else
				// Reindeer
				m_casterUnit->CastSpell(m_casterUnit, 25858, true); //60% ground Reindeer

			return;
		}
		case 25938:                                 // Viscidus explode
		{
			if (!m_casterUnit)
				return;

			if (m_casterUnit->GetHealthPercent() < 5.0f) // Only way to kill this boss
			{
				m_casterUnit->CastSpell(m_casterUnit, 26003, true);
				return;
			}

			// Stop combat
			m_casterUnit->CombatStop();

			// Fake death
			//m_casterUnit->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_FEING_DEATH);
			m_casterUnit->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
			m_casterUnit->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD);
			m_casterUnit->addUnitState(UNIT_STAT_DIED);

			// Summon globs
			m_casterUnit->CastSpell(m_casterUnit, 25885, true);

			return;
		}
		case 26003:                                 // Viscidus suicide
		{
			if (!m_casterUnit)
				return;

			if (!m_casterUnit->isAlive())
				return;

			m_casterUnit->CastSpell(m_casterUnit, 26002, true);
			return;
		}
		case 26074:                                 // Holiday Cheer
			// implemented at client side
			return;
		case 26399:                                 // Despawn Tentacles (C'thun)
		{
			if (!m_caster)
				return;

			if (unitTarget)
				m_caster->DealDamage(unitTarget, unitTarget->GetHealth(), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NONE, nullptr, false);

			return;
		}
		case 26626:                                 // Mana Burn Area
		{
			if (unitTarget->getPowerType() != POWER_MANA)
				return;

			m_caster->CastSpell(unitTarget, 25779, true);
			return;
		}
		case 28006:                                 // Arcane Cloaking
		{
			if (unitTarget && unitTarget->GetTypeId() == TYPEID_PLAYER)
				// Naxxramas Entry Flag Effect DND
				m_caster->CastSpell(unitTarget, 29294, true);

			return;
		}
		case 28098:                                 // Stalagg Tesla Effect
		case 28110:                                 // Feugen Tesla Effect
		{
			if (!m_casterUnit)
				return;

			if (unitTarget->GetTypeId() != TYPEID_UNIT)
				return;

			if (m_casterUnit->getVictim() && !m_casterUnit->IsWithinDistInMap(unitTarget, 60.0f))
			{
				// Cast Shock on nearby targets
				if (Unit* pTarget = ((Creature*)m_casterUnit)->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
					unitTarget->CastSpell(pTarget, 28099, false);
			}
			else
			{
				// "Evade"
				unitTarget->RemoveAurasDueToSpell(m_spellInfo->Id == 28098 ? 28097 : 28109);
				unitTarget->DeleteThreatList();
				unitTarget->CombatStop(true);
				// Recast chain (Stalagg Chain or Feugen Chain
				unitTarget->CastSpell(m_casterUnit, m_spellInfo->Id == 28098 ? 28096 : 28111, false);
			}
			return;
		}
		case 8897:                                 // Destroy Rocket Boots
		{
			m_caster->CastSpell(unitTarget, 8893, true);
			m_caster->CastSpell(unitTarget, 13158, true);

			return;
		}
		case 23185:                                 // Aura of Frost
		case 25044:                                 // Aura of Nature
		case 34442:                                 // Aura of Desert
		{
			if (!unitTarget)
				return;

			if (unitTarget->HasAura(25040))         // Mark of Nature
				unitTarget->CastSpell(unitTarget, 25043, true);

			if (unitTarget->HasAura(23182))         // Mark of Frost
				unitTarget->CastSpell(unitTarget, 23186, true);

			if (unitTarget->HasAura(34439))         // Mark of Desert
				unitTarget->CastSpell(unitTarget, 34443, true);

			/*unitTarget->RemoveAurasAtMechanicImmunity(1 << (MECHANIC_BANDAGE - 1), 0);*/
			if (m_casterUnit->isInCombat())
			{
				unitTarget->RemoveSpellsCausingAura(SPELL_AURA_MOD_STEALTH);

				if (!unitTarget->IsStandingUp())
					unitTarget->SetStandState(UNIT_STAND_STATE_STAND);
			}

			return;
		}
		case 28414:                                 // Call of the Ashbringer
		{
			if (!m_caster || !m_caster->IsPlayer())
				return;
			static constexpr uint32 AshbringerSounds[12] = { 8906,8907,8908,8920,8921,8922,8923,8924,8925,8926,8927,8928 };
			m_caster->PlayDirectSound(AshbringerSounds[urand(0, 11)], m_caster->ToPlayer());
			return;
		}
		case 28441:                                 // AB Effect 000
		{

			return;
		}
		case 28697:                                 // Forgiveness (SM Ashbringer event)
		{
			if (unitTarget && m_casterUnit)
			{
				m_casterUnit->Kill(unitTarget, nullptr);
			}
			return;
		}
		case 21343:                                 // Snowball Knockdown
		{
			if (unitTarget && m_caster && unitTarget->IsPlayer() && m_caster->IsPlayer())
			{
				if (!unitTarget->HasAura(21354) &&                                      // Has no Snowball Resistant aura
					unitTarget->ToPlayer()->IsInSameRaidWith(m_caster->ToPlayer()))     // Is grouped with target
				{
					unitTarget->CastSpell(unitTarget, 21167, true);
				}
			}
			return;
		}
		case 26899: // Friendship Bracelet
		{
			if (unitTarget && m_casterUnit && unitTarget->HasAura(26898))
			{
				unitTarget->RemoveAurasDueToSpell(26898);        // Remove Heartbroken
				unitTarget->CastSpell(unitTarget, 26921, true);  // Create Bracelet
				m_casterUnit->CastSpell(m_casterUnit, 26664, true);      // Cast The Power of Friendship
			}
			return;
		}
		case 27662: // Silver Shafted Arrow
		{
			if (unitTarget && m_caster && unitTarget->IsPlayer())
			{
				if (!unitTarget->GetMiniPet())
					unitTarget->CastSpell(unitTarget, 27570, true);
			}
			return;
		}
		case 14813: // Dark Iron Drunk Mug
		{
			if (unitTarget->HasAura(14823) || unitTarget->GetEntry() == 14871)
				return;

			if (m_originalCasterGUID && m_originalCasterGUID.IsGameObject())
			{
				if (GameObject* pMug = unitTarget->GetMap()->GetGameObject(m_originalCasterGUID))
				{
					float fX, fY, fZ;
					pMug->GetContactPoint(unitTarget, fX, fY, fZ);
					unitTarget->GetMotionMaster()->MovePoint(1, fX, fY, fZ, MOVE_WALK_MODE);
					m_caster->SummonGameObject(165738, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 30000);
				}
			}
			return;
		}
		case 23845: // Attract Jubjub
		{
			if (GameObject* pMug = m_caster->FindNearestGameObject(165578, 3.0f))
			{
				float fX, fY, fZ;
				pMug->GetContactPoint(unitTarget, fX, fY, fZ);
				unitTarget->GetMotionMaster()->MovePoint(1, fX, fY, fZ, MOVE_WALK_MODE);
			}
			return;
		}
		case 23852: // Jubling Cooldown
		{
			// Trigger 7 day cooldown
			SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(23851);
			unitTarget->AddSpellAndCategoryCooldowns(spellInfo, 19462);
			return;
		}
		case 17190: // Ras Frostwhisper Visual Dummy
		{
			if (unitTarget)
			{
				unitTarget->CastSpell(unitTarget, 17186, true);    // Human form
				unitTarget->SetHealth(unitTarget->GetMaxHealth()); // Back to full health
			}
			return;
		}
		case 16032: // Merging Oozes
		{
			if (unitTarget && m_caster && unitTarget->IsCreature() && m_caster->IsCreature())
			{
				// Summon Gargantuan Ooze
				m_caster->SummonCreature(9621, 0.0f, 0.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 420000);
				((Creature*)m_caster)->DespawnOrUnsummon();
				((Creature*)unitTarget)->DespawnOrUnsummon();
			}
			return;
		}
		case 34143: //vanilla+ Sacrifice heal
		{
			if (!m_casterUnit)
				return;

			uint32 hp_perc_sacr = uint32(float(m_casterUnit->GetMaxHealth()));
			int32 sacr_heal = 0;
			sacr_heal = hp_perc_sacr;

			m_casterUnit->CastCustomSpell(m_casterUnit, 33847, &sacr_heal, nullptr, nullptr, true, nullptr);
			return;
		}
		}

		//All IconID Check in there
		switch (m_spellInfo->SpellIconID)
		{
			// Berserking (troll racial traits)
		case 1661:
		{
			if (!m_casterUnit)
				return;

			uint32 healthPerc = uint32((float(m_casterUnit->GetHealth()) / m_casterUnit->GetMaxHealth()) * 100);
			int32 melee_mod = 10;
			if (healthPerc <= 50)
				melee_mod = 30;
			if (healthPerc < 100 && healthPerc > 50)
				melee_mod = 10 + (100 - healthPerc) / 2.5;

			// FIXME: custom spell required this aura state by some unknown reason, we not need remove it anyway
			m_casterUnit->ModifyAuraState(AURA_STATE_BERSERKING, true);
			m_casterUnit->CastCustomSpell(m_casterUnit, 26635, &melee_mod, &melee_mod, &melee_mod, true, nullptr);
			return;
		}
		}
		break;
	}
	case SPELLFAMILY_MAGE:
	{
		switch (m_spellInfo->Id)
		{
		case 35714:  //Ice Shards
		{
			if (!unitTarget || !m_casterUnit)
				return;

			m_caster->CastSpell(unitTarget, 35716, true);  //1
			m_caster->CastSpell(unitTarget, 35715, true);  //visual
			m_caster->CastSpell(unitTarget, 35717, true);  //2
			m_caster->CastSpell(unitTarget, 35718, true);  //3
			m_caster->CastSpell(unitTarget, 35719, true);  //4

			if (m_casterUnit->HasAura(12536))
				m_casterUnit->RemoveAurasDueToSpell(12536);

			return;
		}
		case 12472:                                 // Cold Snap
		{
			if (m_caster->GetTypeId() != TYPEID_PLAYER)
				return;

			// immediately finishes the cooldown on Frost spells
			SpellCooldowns cm = ((Player*)m_caster)->GetSpellCooldownMap();
			for (SpellCooldowns::const_iterator itr = cm.begin(); itr != cm.end();)
			{
				SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(itr->first);

				if (spellInfo->SpellFamilyName == SPELLFAMILY_MAGE &&
					(spellInfo->GetSpellSchoolMask() & SPELL_SCHOOL_MASK_FROST) &&
					spellInfo->Id != m_spellInfo->Id && spellInfo->GetRecoveryTime() > 0)
					((Player*)m_caster)->RemoveSpellCooldown((itr++)->first, true);
				else
					++itr;
			}
			return;
		}
		}
		break;
	}
	case SPELLFAMILY_WARRIOR:
	{
		if (m_spellInfo->Id == 12975)                              //Last Stand
		{
			if (!m_casterUnit)
				return;

			int32 healthModSpellBasePoints0 = m_casterUnit->GetMaxHealth() * 0.3;
			m_casterUnit->CastCustomSpell(m_casterUnit, 12976, &healthModSpellBasePoints0, nullptr, nullptr, true, nullptr);
			return;
		}
		// Execute
		if (m_spellInfo->IsFitToFamilyMask<CF_WARRIOR_EXECUTE>())
		{
			if (!unitTarget || !m_casterUnit)
				return;

			int32 basePoints0 = damage + int32(m_casterUnit->GetPower(POWER_RAGE) * m_spellInfo->DmgMultiplier[eff_idx]);
			// m_casterUnit->SetPower(POWER_RAGE, 0); // Done in EffectSchoolDMG - spell 20647
			m_casterUnit->CastCustomSpell(unitTarget, 20647, &basePoints0, nullptr, nullptr, true, nullptr);
			return;
		}
		if (m_spellInfo->Id == 21977)                   //Warrior's Wrath
		{
			if (!unitTarget)
				return;

			m_caster->CastSpell(unitTarget, 21887, true); // spell mod
			return;
		}
		if (m_spellInfo->Id == 23424)                   // Ustaag <Nostalrius> : Nefarian Class Call Chaman Corrupted Totems
		{
			if (!m_casterUnit)
				return;

			switch (urand(0, 3))
			{
			case 0:        // Corrupted Fire Nova Totem
				m_casterUnit->CastSpell(m_casterUnit, 23419, true);
				break;
			case 1:        // Corrupted Stoneskin Totem
				m_casterUnit->CastSpell(m_casterUnit, 23420, true);
				break;
			case 2:        // Corrupted Windfury Totem
				m_casterUnit->CastSpell(m_casterUnit, 23423, true);
				break;
			case 3:        // Corrupted Healing Stream Totem
				m_casterUnit->CastSpell(m_casterUnit, 23422, true);
				break;
			}
			return;
		}
		break;
	}
	case SPELLFAMILY_WARLOCK:
	{
		// Life Tap
		if (m_spellInfo->IsFitToFamilyMask<CF_WARLOCK_LIFE_TAP>())
		{
			if (!m_casterUnit)
				return;

			float cost = m_currentBasePoints[EFFECT_INDEX_0];

			if (Player* modOwner = m_casterUnit->GetSpellModOwner())
				modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_COST, cost, this);

			int32 dmg = m_casterUnit->SpellDamageBonusDone(m_casterUnit, m_spellInfo, uint32(cost > 0 ? cost : 0), SPELL_DIRECT_DAMAGE);
			/*dmg = m_casterUnit->SpellDamageBonusTaken(m_casterUnit, m_spellInfo, dmg, SPELL_DIRECT_DAMAGE);*/

			if (int32(m_casterUnit->GetHealth()) > dmg)
			{
				// Shouldn't Appear in Combat Log
				m_casterUnit->ModifyHealth(-dmg);

				int32 mana = dmg;

				Unit::AuraList const& auraDummy = m_casterUnit->GetAurasByType(SPELL_AURA_DUMMY);
				for (Unit::AuraList::const_iterator itr = auraDummy.begin(); itr != auraDummy.end(); ++itr)
				{
					// only Imp. Life Tap have this in combination with dummy aura
					if ((*itr)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_WARLOCK && (*itr)->GetSpellProto()->SpellIconID == 208)
						mana = ((*itr)->GetModifier()->m_amount + 100) * mana / 100;
				}
				if (m_casterUnit->HasAura(34569))
					mana *= 2;

				m_casterUnit->CastCustomSpell(m_casterUnit, 31818, &mana, nullptr, nullptr, true, nullptr);
				// Mana Feed
				/*int32 manaFeedVal = m_casterUnit->CalculateSpellDamage(m_casterUnit, m_spellInfo, EFFECT_INDEX_1);
				manaFeedVal = manaFeedVal * mana / 100;
				if (manaFeedVal > 0)
					m_casterUnit->CastCustomSpell(m_casterUnit, 32553, &manaFeedVal, nullptr, nullptr, true, nullptr);*/
			}
			else
				SendCastResult(SPELL_FAILED_FIZZLE);

			return;
		}
		break;
	}
	case SPELLFAMILY_PRIEST:
	{
		switch (m_spellInfo->Id)
		{
		case 28598:                                 // Touch of Weakness triggered spell
		{
			if (!unitTarget || !m_triggeredByAuraSpell)
				return;

			uint32 spellid = 0;
			switch (m_triggeredByAuraSpell->Id)
			{
			case 2652:
				spellid = 2943;
				break; // Rank 1
			case 19261:
				spellid = 19249;
				break; // Rank 2
			case 19262:
				spellid = 19251;
				break; // Rank 3
			case 19264:
				spellid = 19252;
				break; // Rank 4
			case 19265:
				spellid = 19253;
				break; // Rank 5
			case 19266:
				spellid = 19254;
				break; // Rank 6
			case 25461:
				spellid = 25460;
				break; // Rank 7
			default:
				sLog.outError("Spell::EffectDummy: Spell 28598 triggered by unhandeled spell %u", m_triggeredByAuraSpell->Id);
				return;
			}
			m_caster->CastSpell(unitTarget, spellid, true, nullptr);
			return;
		}
		}
		break;
	}
	case SPELLFAMILY_DRUID:
		switch (m_spellInfo->Id)
		{
		case 5229:                                  // Enrage
		{
			// Reduce base armor by 27% in Bear Form and 16% in Dire Bear Form
			break;
		}
		case 29201: // Loatheb Corrupted Mind triggered sub spells
		{
			uint32 spellid = 0;
			switch (unitTarget->getClass())
			{
				// priests should be getting 29185, but it triggers on dmg effects as well, don't know why.
				// stealing druid version for priests until anyone has a reason priests cant smite.s
			case CLASS_PRIEST:  spellid = 29194; break;//29185; break;
			case CLASS_DRUID:   spellid = 29194; break;
			case CLASS_PALADIN: spellid = 29196; break;
			case CLASS_SHAMAN:  spellid = 29198; break;
			default: break;
			}
			if (spellid != 0)
				m_caster->CastSpell(unitTarget, spellid, true);

			break;
		}
		}
		break;
	case SPELLFAMILY_HUNTER:
	{
		switch (m_spellInfo->Id)
		{
		case 35841:  //Power Shot Loksey
		{
			m_casterUnit->CastSpell(unitTarget, 35844, true);

			return;
		}
		case 35845:  //Power Shot avg
		{
			m_casterUnit->CastSpell(unitTarget, 35846, true);

			return;
		}
		// Power Shot visual
		case 35844:
		case 35846:
		{
			unitTarget->CastSpell(unitTarget, 36153, true);
			return;
		}
		case 23989:                                 // Readiness talent
		{
			if (m_caster->GetTypeId() != TYPEID_PLAYER)
				return;

			//immediately finishes the cooldown for hunter abilities
			SpellCooldowns cm = ((Player*)m_caster)->GetSpellCooldownMap();
			for (SpellCooldowns::const_iterator itr = cm.begin(); itr != cm.end();)
			{
				SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(itr->first);

				if (spellInfo->SpellFamilyName == SPELLFAMILY_HUNTER && spellInfo->Id != 23989 && spellInfo->GetRecoveryTime() > 0)
					((Player*)m_caster)->RemoveSpellCooldown((itr++)->first, true);
				else
					++itr;
			}
			return;
		}
		}
		break;
	}
	case SPELLFAMILY_PALADIN:
	{
#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_8_4
		switch (m_spellInfo->SpellIconID)
		{
		case 156:                                   // Holy Shock
		{
			if (!unitTarget)
				return;

			int hurt = 0;
			int heal = 0;

			switch (m_spellInfo->Id)
			{
			case 20473:
				hurt = 25912;
				heal = 25914;
				break;
			case 20929:
				hurt = 25911;
				heal = 25913;
				break;
			case 20930:
				hurt = 25902;
				heal = 25903;
				break;
			default:
				sLog.outError("Spell::EffectDummy: Spell %u not handled in HS", m_spellInfo->Id);
				return;
			}

			if (m_caster->IsFriendlyTo(unitTarget))
				m_caster->CastSpell(unitTarget, heal, true);
			else
				m_caster->CastSpell(unitTarget, hurt, true);

			return;
		}
		case 561:                                   // Judgement of command
		{
			if (!unitTarget)
				return;

			uint32 spell_id = m_currentBasePoints[eff_idx];
			SpellEntry const* spell_proto = sSpellMgr.GetSpellEntry(spell_id);
			if (!spell_proto)
				return;

			m_caster->CastSpell(unitTarget, spell_proto, true, nullptr);
			return;
		}
		}
#endif
		break;
	}
	case SPELLFAMILY_SHAMAN:
	{
		if (m_spellInfo->Id == 33648)
		{
			if (!m_targets.getUnitTarget())
				return;

			// for caster applied auras only FLAMESHOCK
			bool flamefound = false;
			auto const& mPeriodic = m_targets.getUnitTarget()->GetAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
			for (auto i = mPeriodic.begin(); i != mPeriodic.end(); ++i)
			{
				// Frost Shock
				if ((*i)->GetSpellProto()->IsFitToFamily<SPELLFAMILY_SHAMAN, CF_SHAMAN_FLAME_SHOCK>() &&
					(*i)->GetCasterGuid() == m_caster->GetObjectGuid())
				{
					flamefound = true;
					m_caster->CastSpell(unitTarget, 33652, true); //Proc burst effect
					break;
				}
			}

			// for caster applied auras only FROSTSHOCK
			bool frostfound = false;
			auto const& slowingAuras = m_targets.getUnitTarget()->GetAurasByType(SPELL_AURA_MOD_DECREASE_SPEED);
			for (auto i = slowingAuras.begin(); i != slowingAuras.end(); ++i)
			{
				// Frost Shock
				if ((*i)->GetSpellProto()->IsFitToFamily<SPELLFAMILY_SHAMAN, CF_SHAMAN_FROST_SHOCK>() &&
					(*i)->GetCasterGuid() == m_caster->GetObjectGuid())
				{
					frostfound = true;
					m_caster->CastSpell(unitTarget, 33653, true); //Proc stun effect
					unitTarget->RemoveAurasByCasterSpell((*i)->GetId(), m_caster->GetObjectGuid());
					break;
				}
			}

			// for caster applied auras only EARTHSHOCK
			bool earthfound = false;
			auto const& nModDetectRange = m_targets.getUnitTarget()->GetAurasByType(SPELL_AURA_MOD_DETECT_RANGE);
			for (auto i = nModDetectRange.begin(); i != nModDetectRange.end(); ++i)
			{
				// Earth Shock
				if ((*i)->GetSpellProto()->IsFitToFamily<SPELLFAMILY_SHAMAN, CF_SHAMAN_EARTH_SHOCK>() &&
					(*i)->GetCasterGuid() == m_caster->GetObjectGuid())
				{
					earthfound = true;
					m_caster->CastSpell(unitTarget, 33654, true); //Proc taunt effect
					unitTarget->RemoveAurasByCasterSpell((*i)->GetId(), m_caster->GetObjectGuid());
					break;
				}
			}
			return;
		}

		// Rockbiter Weapon
		if (m_spellInfo->IsFitToFamilyMask<CF_SHAMAN_ROCKBITER_WEAPON>())
		{
			uint32 spell_id = 0;
			switch (m_spellInfo->Id)
			{
			case  8017:
				spell_id = 36494;
				break;    // Rank 1
			case  8018:
				spell_id = 36750;
				break;    // Rank 2
			case  8019:
				spell_id = 36755;
				break;    // Rank 3
			case 10399:
				spell_id = 36759;
				break;    // Rank 4
			case 16314:
				spell_id = 36763;
				break;    // Rank 5
			case 16315:
				spell_id = 36766;
				break;    // Rank 6
			case 16316:
				spell_id = 36771;
				break;    // Rank 7
			default:
				sLog.outError("Spell::EffectDummy: Spell %u not handled in RW", m_spellInfo->Id);
				return;
			}

			SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(spell_id);

			if (!spellInfo)
			{
				sLog.outError("WORLD: unknown spell id %i", spell_id);
				return;
			}

			Player* pPlayer = m_caster->ToPlayer();
			if (!pPlayer)
				return;

			for (int j = BASE_ATTACK; j <= OFF_ATTACK; ++j)
			{
				if (Item* item = pPlayer->GetWeaponForAttack(WeaponAttackType(j)))
				{
					if (item->IsFitToSpellRequirements(m_spellInfo))
					{
						Spell* spell = new Spell(pPlayer, spellInfo, true);

						// enchanting spell selected by calculated damage-per-sec in enchanting effect
						// at calculation applied affect from Elemental Weapons talent
						// real enchantment damage
						spell->m_currentBasePoints[1] = damage;

						SpellCastTargets targets;
						targets.setItemTarget(item);
						spell->prepare(std::move(targets));
					}
				}
			}
			return;
		}

		// Flametongue Weapon Proc, Ranks
		// Daemon : et totem langue de feu
		if (m_spellInfo->IsFitToFamilyMask<CF_SHAMAN_FLAMETONGUE_WEAPON, CF_SHAMAN_FLAMETONGUE_TOTEM>())
		{
			if (!m_CastItem)
			{
				sLog.outError("Spell::EffectDummy: spell %i requires cast Item", m_spellInfo->Id);
				return;
			}
			// found spelldamage coefficients of 0.381% per 0.1 speed and 15.244 per 4.0 speed
			// but own calculation say 0.385 gives at most one point difference to published values
			int32 spellDamage = m_caster->SpellBaseDamageBonusDone(m_spellInfo->GetSpellSchoolMask());
			float weaponSpeed = (1.0f / IN_MILLISECONDS) * m_CastItem->GetProto()->Delay;
			int32 totalDamage = int32((damage + 3.85f * spellDamage) * 0.01 * weaponSpeed);

			m_caster->CastCustomSpell(unitTarget, 10444, &totalDamage, nullptr, nullptr, true, m_CastItem);
			return;
		}
		break;
	}
	}

	// pet auras
	if (PetAura const* petSpell = sSpellMgr.GetPetAura(m_spellInfo->Id))
	{
		if (m_casterUnit)
			m_casterUnit->AddPetAura(petSpell);
		return;
	}

	// Script based implementation. Must be used only for not good for implementation in core spell effects
	// So called only for not processed cases
	if (gameObjTarget)
		sScriptMgr.OnEffectDummy(m_caster, m_spellInfo->Id, eff_idx, gameObjTarget);
	else if (unitTarget && unitTarget->GetTypeId() == TYPEID_UNIT)
		sScriptMgr.OnEffectDummy(m_caster, m_spellInfo->Id, eff_idx, (Creature*)unitTarget);
}

void Spell::EffectTriggerSpell(SpellEffectIndex eff_idx)
{
	// only unit case known
	if (!unitTarget)
	{
		if (gameObjTarget || itemTarget)
			sLog.outError("Spell::EffectTriggerSpell (Spell: %u): Unsupported non-unit case!", m_spellInfo->Id);
		return;
	}

	uint32 triggered_spell_id = m_spellInfo->EffectTriggerSpell[eff_idx];

	// special cases
	switch (m_spellInfo->Id)
	{
	case 33686:	// Cleansing Wave Curse cleanse effect initial cast
		m_caster->CastSpell(unitTarget, 36338, true);
		break;
		// The Only Cure is More Green Glow quest 2962
	case 12709:
		if (urand(0, 2) == 0)
			m_caster->CastSpell(unitTarget, 11638, true, m_CastItem, nullptr, m_originalCasterGUID);
		else
			m_caster->CastSpell(unitTarget, 11637, true, m_CastItem, nullptr, m_originalCasterGUID);
		return;
		// Linken's Boomerang: 10% chance to proc stun, 3% chance to proc disarm (dubious numbers)
	case 15712:
		if (triggered_spell_id == 15753 && urand(0, 10))
		{
			return;
		}

		if (triggered_spell_id == 15752 && urand(0, 30))
		{
			return;
		}

		break;
	}
	switch (triggered_spell_id)
	{
		// Item [Scorpid Surprise] - Heals 294 damage over 21 sec, assuming you don't bite down on a poison sac.
		// 10% proc rate (no source !)
	case 6411:
		if (urand(0, 10) == 0)
			return;
		break;
		// Vanish (not exist)
	case 18461:
	{
		unitTarget->RemoveSpellsCausingAura(SPELL_AURA_MOD_ROOT);
		unitTarget->RemoveSpellsCausingAura(SPELL_AURA_MOD_DECREASE_SPEED);
		unitTarget->RemoveSpellsCausingAura(SPELL_AURA_MOD_STALKED);

		// if this spell is given to NPC it must handle rest by it's own AI
		if (unitTarget->GetTypeId() != TYPEID_PLAYER)
			return;


		// Clean Escape New
		if (unitTarget->HasAura(23582))
		{
			int32 healthGain = int32(unitTarget->GetMaxHealth()) / 2;
			unitTarget->CastCustomSpell(unitTarget, 23583, &healthGain, NULL, NULL, true, NULL);
		}

		// get highest rank of the Stealth spell
		uint32 spellId = 0;
		const PlayerSpellMap& sp_list = ((Player*)unitTarget)->GetSpellMap();
		for (PlayerSpellMap::const_iterator itr = sp_list.begin(); itr != sp_list.end(); ++itr)
		{
			// only highest rank is shown in spell book, so simply check if shown in spell book
			if (!itr->second.active || itr->second.disabled || itr->second.state == PLAYERSPELL_REMOVED)
				continue;

			SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(itr->first);
			if (!spellInfo)
				continue;

			if (spellInfo->IsFitToFamily<SPELLFAMILY_ROGUE, CF_ROGUE_STEALTH>())
			{
				spellId = spellInfo->Id;
				break;
			}
		}

		// no Stealth spell found
		if (!spellId)
			return;

		// reset cooldown on it if needed
		if (((Player*)unitTarget)->HasSpellCooldown(spellId))
			((Player*)unitTarget)->RemoveSpellCooldown(spellId);

		m_caster->CastSpell(unitTarget, spellId, true);
		return;
	}
	// just skip
	case 23770:                                         // Sayge's Dark Fortune of *
		// not exist, common cooldown can be implemented in scripts if need.
		return;
		// Brittle Armor - (need add max stack of 24575 Brittle Armor)
	case 29284:
		m_caster->CastSpell(unitTarget, 24575, true, m_CastItem, nullptr, m_originalCasterGUID);
		return;
		// Mercurial Shield - (need add max stack of 26464 Mercurial Shield)
	case 29286:
		m_caster->CastSpell(unitTarget, 26464, true, m_CastItem, nullptr, m_originalCasterGUID);
		return;
	}

	// normal case
	SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(triggered_spell_id);
	if (!spellInfo)
	{
		sLog.outError("EffectTriggerSpell of spell %u: triggering unknown spell id %i", m_spellInfo->Id, triggered_spell_id);
		return;
	}

	// select formal caster for triggered spell
	WorldObject* caster = m_caster;

	// some triggered spells require specific equipment
	if (spellInfo->EquippedItemClass >= 0 && m_caster->GetTypeId() == TYPEID_PLAYER)
	{
		// main hand weapon required
		if (spellInfo->AttributesEx3 & SPELL_ATTR_EX3_MAIN_HAND)
		{
			Item* item = ((Player*)m_caster)->GetWeaponForAttack(BASE_ATTACK, true, false);

			// skip spell if no weapon in slot or broken
			if (!item)
				return;

			// skip spell if weapon not fit to triggered spell
			if (!item->IsFitToSpellRequirements(spellInfo))
				return;
		}

		// offhand hand weapon required
		if (spellInfo->AttributesEx3 & SPELL_ATTR_EX3_REQ_OFFHAND)
		{
			Item* item = ((Player*)m_caster)->GetWeaponForAttack(OFF_ATTACK, true, false);

			// skip spell if no weapon in slot or broken
			if (!item)
				return;

			// skip spell if weapon not fit to triggered spell
			if (!item->IsFitToSpellRequirements(spellInfo))
				return;
		}
	}
	else
	{
		// Note: not exist spells with weapon req. and IsSpellHaveCasterSourceTargets == true
		// so this just for speedup places in else
		caster = spellInfo->IsSpellWithCasterSourceTargetsOnly() ? unitTarget : m_caster;
	}

	caster->CastSpell(unitTarget, spellInfo, true, nullptr, nullptr, m_originalCasterGUID);
}

void Spell::EffectTriggerMissileSpell(SpellEffectIndex effect_idx)
{
	uint32 triggered_spell_id = m_spellInfo->EffectTriggerSpell[effect_idx];

	// normal case
	SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(triggered_spell_id);

	if (!spellInfo)
	{
		sLog.outError("EffectTriggerMissileSpell of spell %u (eff: %u): triggering unknown spell id %u",
			m_spellInfo->Id, effect_idx, triggered_spell_id);
		return;
	}

	if (m_CastItem)
		DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "WORLD: cast Item spellId - %i", spellInfo->Id);

	m_caster->CastSpell(m_targets.m_destX, m_targets.m_destY, m_targets.m_destZ, spellInfo, true, m_CastItem, nullptr, m_originalCasterGUID);
}

void Spell::EffectTeleportUnits(SpellEffectIndex eff_idx)
{
	if (!unitTarget || unitTarget->IsTaxiFlying())
		return;

	switch (m_spellInfo->EffectImplicitTargetB[eff_idx])
	{
	case TARGET_INNKEEPER_COORDINATES:
	{
		// Only players can teleport to innkeeper
		if (unitTarget->GetTypeId() != TYPEID_PLAYER)
			return;

		bool hearthCooldown = m_spellInfo->Id != 556;   // Astral Recall doesn't set HS on cooldown
		((Player*)unitTarget)->TeleportToHomebind(unitTarget == m_caster ? TELE_TO_SPELL : 0, hearthCooldown);
		return;
	}
	case TARGET_AREAEFFECT_INSTANT:                     // in all cases first TARGET_TABLE_X_Y_Z_COORDINATES
	case TARGET_TABLE_X_Y_Z_COORDINATES:
	{
		switch (m_spellInfo->Id)
		{
			break;
		case 17334:
		case 17607:
		case 17608:
			if (Player* pl = (Player*)unitTarget)
				if (pl->GetTeam() == HORDE)
					return;
			break;
		case 17609:
		case 17610:
		case 17611:
			if (Player* pl = (Player*)unitTarget)
				if (pl->GetTeam() == ALLIANCE)
					return;
			break;
		default:
			break;
		}

		SpellTargetPosition const* st = sSpellMgr.GetSpellTargetPosition(m_spellInfo->Id);
		if (!st)
		{
			sLog.outError("Spell::EffectTeleportUnits - unknown Teleport coordinates for spell ID %u", m_spellInfo->Id);
			return;
		}

		// Death Flight random bonus
		float xrandom = 0.0f;
		float yrandom = 0.0f;
		float zrandom = 0.0f;
		if (m_spellInfo->Id == 34814)
		{
			xrandom = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 20.0f));
			yrandom = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 20.0f));
			zrandom = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 20.0f));
		}

		if (st->target_mapId == unitTarget->GetMapId())
			unitTarget->NearTeleportTo(st->target_X + xrandom, st->target_Y + yrandom, st->target_Z + zrandom, st->target_Orientation, TELE_TO_NOT_LEAVE_COMBAT | TELE_TO_NOT_UNSUMMON_PET | (unitTarget == m_caster ? TELE_TO_SPELL : 0));
		else if (unitTarget->GetTypeId() == TYPEID_PLAYER)
			((Player*)unitTarget)->TeleportTo(st->target_mapId, st->target_X, st->target_Y, st->target_Z, st->target_Orientation, unitTarget == m_caster ? TELE_TO_SPELL : 0);
		break;
	}
	case TARGET_EFFECT_SELECT:
	{
		if (!m_casterUnit)
			return;

		// m_destN filled, but sometimes for wrong dest and does not have TARGET_FLAG_DEST_LOCATION
		float x = unitTarget->GetPositionX();
		float y = unitTarget->GetPositionY();
		float z = unitTarget->GetPositionZ();
		float orientation = m_caster->GetOrientation();

		m_casterUnit->NearTeleportTo(x, y, z, orientation, TELE_TO_NOT_LEAVE_COMBAT | TELE_TO_NOT_UNSUMMON_PET | (unitTarget == m_caster ? TELE_TO_SPELL : 0));
		return;
	}
	default:
	{
		// If not exist data for dest location - return
		if (!(m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION))
		{
			sLog.outError("Spell::EffectTeleportUnits - unknown EffectImplicitTargetB[%u] = %u for spell ID %u", eff_idx, m_spellInfo->EffectImplicitTargetB[eff_idx], m_spellInfo->Id);
			return;
		}
		// Init dest coordinates
		float x = m_targets.m_destX;
		float y = m_targets.m_destY;
		float z = m_targets.m_destZ;
		float orientation = unitTarget->GetOrientation();
		// Teleport
		unitTarget->NearTeleportTo(x, y, z, orientation, TELE_TO_NOT_LEAVE_COMBAT | TELE_TO_NOT_UNSUMMON_PET | (unitTarget == m_caster ? TELE_TO_SPELL : 0));
		return;
	}
	}

	// post effects for TARGET_TABLE_X_Y_Z_COORDINATES
	if (m_spellInfo->Id == 23442 && m_casterUnit)
	{
		int32 r = irand(0, 119);
		if (r >= 70)                                    // 7/12 success
		{
			if (r < 100)                                // 4/12 evil twin
				m_casterUnit->CastSpell(m_casterUnit, 23445, true);
			else                                        // 1/12 fire
				m_casterUnit->CastSpell(m_casterUnit, 23449, true);
		}
	}
}

void Spell::EffectApplyAura(SpellEffectIndex eff_idx)
{
	if (!unitTarget || !m_spellAuraHolder)
		return;

	if (!m_spellInfo->EffectApplyAuraName[eff_idx])
		return;

	// ghost spell check, allow apply any auras at player loading in ghost mode (will be cleanup after load)
	if ((!unitTarget->isAlive() && !(m_spellInfo->IsDeathOnlySpell() || m_spellInfo->IsDeathPersistentSpell())) &&
		(unitTarget->GetTypeId() != TYPEID_PLAYER || !((Player*)unitTarget)->GetSession()->PlayerLoading()))
		return;

	if (unitTarget->HasMorePowerfullSpellActive(m_spellInfo))
		return;

	Unit* caster = GetAffectiveCaster();
	if (!caster)
	{
		// FIXME: currently we can't have auras applied explicitly by gameobjects
		// so for auras from wild gameobjects (no owner) target used
		if (m_originalCasterGUID.IsGameObject())
			caster = unitTarget;
		else
			return;
	}

	//Salvation + Righteous Fury
	if (m_spellInfo->Id == 1038 || m_spellInfo->Id == 25895)
		if (unitTarget->HasAura(25780))
			return;

	DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Spell: Aura is: %u [Spell%u:DiminishingGroup%u]", m_spellInfo->EffectApplyAuraName[eff_idx], m_spellInfo->Id, m_diminishGroup);

	//// Sayge's Dark Fortune of Damage: +1-10% (random)
	//if (m_spellInfo->Id == 23768)
	//	m_currentBasePoints[EFFECT_INDEX_0] = urand(1, 10);
	// Gnomish Death Ray
	// rarely has a chance of dealing double damage, 14.29% chance (guess)
	// for now we use linear level scaling, but this is likely incorrect (hp pools don't scale exactly linearly)
	// there is some speculation that this should be tied to Engineering skill level, but since you don't need Engineering to use the item at all this seems doubtful
	/*else */
	if (m_spellInfo->Id == 13278)
		m_currentBasePoints[eff_idx] = eff_idx == EFFECT_INDEX_0 ? int32(urand(600, 1200) * (caster->getLevel() / 60.0f)) * (!urand(0, 6) ? 2 : 1)
		: m_currentBasePoints[EFFECT_INDEX_0] * 0.1249f;
	// Paladin T3 JoL
	else if (m_spellInfo->IsFitToFamilyMask<CF_PALADIN_JUDGEMENT_OF_WISDOM_LIGHT>() && m_spellInfo->SpellIconID == 299 && m_casterUnit && m_casterUnit->HasAura(28775))
		m_currentBasePoints[eff_idx] = 20;

	Aura* aur = CreateAura(m_spellInfo, eff_idx, &m_currentBasePoints[eff_idx], m_spellAuraHolder, unitTarget, caster, m_CastItem);
	m_spellAuraHolder->AddAura(aur, eff_idx);
}

void Spell::EffectPowerDrain(SpellEffectIndex eff_idx)
{
	if (m_spellInfo->EffectMiscValue[eff_idx] < 0 || m_spellInfo->EffectMiscValue[eff_idx] >= MAX_POWERS)
		return;

	Powers drain_power = Powers(m_spellInfo->EffectMiscValue[eff_idx]);

	if (!unitTarget)
		return;
	if (!unitTarget->isAlive())
		return;
	if (unitTarget->getPowerType() != drain_power)
		return;
	if (damage < 0)
		return;

	int32 curPower = unitTarget->GetPower(drain_power);

	//add spell damage bonus
	damage = m_caster->SpellDamageBonusDone(unitTarget, m_spellInfo, uint32(damage), SPELL_DIRECT_DAMAGE, 1, this);
	damage = unitTarget->SpellDamageBonusTaken(m_caster, m_spellInfo, uint32(damage), SPELL_DIRECT_DAMAGE, 1, this);

	int32 new_damage;
	if (curPower < damage)
		new_damage = curPower;
	else
		new_damage = damage;

	unitTarget->ModifyPower(drain_power, -new_damage);

	ExecuteLogInfo info(unitTarget->GetObjectGuid());
	info.powerDrain.power = drain_power;
	info.powerDrain.amount = new_damage;
	info.powerDrain.multiplier = 0.0f;

	// Don`t restore from self drain
	if (drain_power == POWER_MANA && m_caster != unitTarget)
	{
		float manaMultiplier = m_spellInfo->EffectMultipleValue[eff_idx];
		if (manaMultiplier == 0)
			manaMultiplier = 1;

		if (m_casterUnit)
		{
			if (Player* modOwner = m_casterUnit->GetSpellModOwner())
				modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_MULTIPLE_VALUE, manaMultiplier);
		}

		int32 gain = int32(new_damage * manaMultiplier);

		if (m_casterUnit)
			m_casterUnit->ModifyPower(POWER_MANA, gain);

		info.powerDrain.multiplier = manaMultiplier;
	}
}

void Spell::EffectSendEvent(SpellEffectIndex eff_idx)
{
	/*
	we do not handle a flag dropping or clicking on flag in battleground by sendevent system
	*/
	DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Spell ScriptStart %u for spellid %u in EffectSendEvent ", m_spellInfo->EffectMiscValue[eff_idx], m_spellInfo->Id);

	// In some cases, the spell does not require a focus but still uses a game object
	// eg. using an Altar or similar GO.
	// Therefore, pass the GO as the target if this is the case.
	GameObject* gObject = focusObject ? focusObject : m_targets.getGOTarget();

	if (!sScriptMgr.OnProcessEvent(m_spellInfo->EffectMiscValue[eff_idx], m_caster, gObject, true))
		m_caster->GetMap()->ScriptsStart(sEventScripts, m_spellInfo->EffectMiscValue[eff_idx], m_caster, gObject);
}

void Spell::EffectPowerBurn(SpellEffectIndex eff_idx)
{
	if (m_spellInfo->EffectMiscValue[eff_idx] < 0 || m_spellInfo->EffectMiscValue[eff_idx] >= MAX_POWERS)
		return;

	Powers powertype = Powers(m_spellInfo->EffectMiscValue[eff_idx]);

	if (!unitTarget)
		return;
	if (!unitTarget->isAlive())
		return;
	if (unitTarget->getPowerType() != powertype)
		return;
	if (damage < 0)
		return;

	int32 curPower = int32(unitTarget->GetPower(powertype));

	int32 new_damage = (curPower < damage) ? curPower : damage;

	unitTarget->ModifyPower(powertype, -new_damage);
	float multiplier = m_spellInfo->EffectMultipleValue[eff_idx];

	if (m_casterUnit)
	{
		if (Player* modOwner = m_casterUnit->GetSpellModOwner())
			modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_MULTIPLE_VALUE, multiplier);
	}

	new_damage = int32(new_damage * multiplier);
	m_damage += new_damage;
}

void Spell::EffectHeal(SpellEffectIndex eff_idx)
{
	if (unitTarget && unitTarget->isAlive() && damage >= 0)
	{
		// Try to get original caster
		WorldObject* caster = GetAffectiveCasterObject();
		if (!caster)
			return;

		int32 addhealth = damage;

		//Earth Shield proc heal hack
		if (m_spellInfo->Id == 35757)
		{
			addhealth = unitTarget->GetMaxHealth() / 100 * addhealth * 0.1f;
		}

		//Holy Shock x2 heal when <=20%
		if (m_spellInfo->IsFitToFamilyMask<CF_PALADIN_HOLY_SHOCK>())
		{
			if (!unitTarget)
				return;

			uint32 tarHPperc = uint32((float(unitTarget->GetHealth()) / unitTarget->GetMaxHealth()) * 100);
			if (tarHPperc < 21)
				addhealth *= 2.0f;
		}

#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_10_2
		// Swiftmend - consumes Regrowth or Rejuvenation
		if (m_spellInfo->Id == 18562)
		{
			Unit::AuraList const& RejorRegr = unitTarget->GetAurasByType(SPELL_AURA_PERIODIC_HEAL);
			// find most short by duration
			Aura* targetAura = nullptr;
			for (Unit::AuraList::const_iterator i = RejorRegr.begin(); i != RejorRegr.end(); ++i)
			{
				// Regrowth or Rejuvenation
				if ((*i)->GetSpellProto()->IsFitToFamily<SPELLFAMILY_DRUID, CF_DRUID_REJUVENATION, CF_DRUID_REGROWTH>())
					if (!targetAura || (*i)->GetAuraDuration() < targetAura->GetAuraDuration())
						targetAura = *i;
			}

			if (!targetAura)
			{
				sLog.outError("Target (GUID: %u TypeId: %u) has aurastate AURA_STATE_SWIFTMEND but no matching aura.", unitTarget->GetGUIDLow(), unitTarget->GetTypeId());
				return;
			}
			int idx = 0;
			while (idx < 3)
			{
				if (targetAura->GetSpellProto()->EffectApplyAuraName[idx] == SPELL_AURA_PERIODIC_HEAL)
					break;
				idx++;
			}

			int32 tickheal = targetAura->GetModifier()->m_amount;
			int32 tickcount = 0;
			// Regrowth : 0x40
			// "18 sec of Regrowth" -> 6 ticks
			if (targetAura->GetSpellProto()->IsFitToFamilyMask<CF_DRUID_REGROWTH>())
				tickcount = 6;
			// Rejuvenation : 0x10
			// "12 sec of Rejuvenation" -> 4 ticks
			if (targetAura->GetSpellProto()->IsFitToFamilyMask<CF_DRUID_REJUVENATION>())
				tickcount = 4;

			unitTarget->RemoveAurasDueToSpell(targetAura->GetId());

			addhealth += tickheal * tickcount;
		}
#endif
		//Stormrage 3/4
		if (m_spellInfo->IsFitToFamily<SPELLFAMILY_DRUID, CF_DRUID_HEALING_TOUCH>() && m_casterUnit->HasAura(34562))
		{
			if (unitTarget->GetHealth() <= unitTarget->GetMaxHealth() / 5)
				addhealth *= 1.3f;
		}

		if (m_spellInfo->IsFitToFamily<SPELLFAMILY_PALADIN, CF_PALADIN_HOLY_LIGHT2>())
		{
			if (m_casterUnit->HasAura(34651))
				m_casterUnit->RemoveAurasDueToSpell(34651);
		}
		// JoL - Extra heal stored in m_triggeredByAuraBasePoints
		if (m_spellInfo->SpellIconID == 299 && m_spellInfo->SpellVisual == 5560 && m_spellInfo->SpellFamilyFlags == 0 && m_triggeredByAuraBasePoints > 0)
			addhealth += m_triggeredByAuraBasePoints;

		addhealth = caster->SpellHealingBonusDone(unitTarget, m_spellInfo, addhealth, HEAL, 1, this);

		
		if (Unit* casterUnit = caster->ToUnit())
		{
			//Bloodied Gloves
			if (casterUnit->HasAura(36279) && unitTarget->HasAuraWithMechanic(MECHANIC_BLEED))
				addhealth = addhealth * 105 / 100;	// +5%

			// Locket of Devotion
			if (casterUnit->HasAura(36307))
			{
				if (Aura* aura = unitTarget->GetAura(36306, EFFECT_INDEX_0))
				{
					if (aura->GetCasterGuid() == casterUnit->GetObjectGuid())
						addhealth = addhealth * 103 / 100; // +3%
				}
			}
			// Fairbanks Healing 5%
			if (casterUnit->HasAura(36316))
			{
				if (casterUnit->GetHealthPercent() < unitTarget->GetHealthPercent())
					addhealth = addhealth * 105 / 100; // +5%
			}
		}


		addhealth = unitTarget->SpellHealingBonusTaken(caster, m_spellInfo, addhealth, HEAL, 1, this);

#if SUPPORTED_CLIENT_BUILD <= CLIENT_BUILD_1_9_4
		ExecuteLogInfo info(unitTarget->GetObjectGuid());
		info.heal.amount = addhealth;
		info.heal.critical = 0;
		AddExecuteLogInfo(eff_idx, info);
#endif

		m_healing += addhealth;
	}
}

void Spell::EffectHealMechanical(SpellEffectIndex /*eff_idx*/)
{
	// Mechanic creature type should be correctly checked by targetCreatureType field
	if (unitTarget && unitTarget->isAlive() && damage >= 0)
	{
		// Try to get original caster
		WorldObject* caster = GetAffectiveCasterObject();
		if (!caster)
			return;

		uint32 addhealth = caster->SpellHealingBonusDone(unitTarget, m_spellInfo, damage, HEAL);
		addhealth = unitTarget->SpellHealingBonusTaken(caster, m_spellInfo, addhealth, HEAL);

		caster->DealHeal(unitTarget, addhealth, m_spellInfo);
	}
}

void Spell::EffectHealthLeech(SpellEffectIndex effIndex)
{
	if (!unitTarget || !unitTarget->isAlive() || damage < 0)
		return;

	int32 initialDamage = damage;

	/*if (!IsCastByItem()) weapon procs*/

	damage = m_caster->SpellDamageBonusDone(unitTarget, m_spellInfo, uint32(damage), SPELL_DIRECT_DAMAGE);
	damage = unitTarget->SpellDamageBonusTaken(m_caster, m_spellInfo, uint32(damage), SPELL_DIRECT_DAMAGE);

	DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "HealthLeech :%i", damage);

	float healMultiplier = m_spellInfo->EffectMultipleValue[effIndex];//vanilla+
	float minval = float(m_casterUnit->GetMaxNegativeAuraModifier(SPELL_AURA_MOD_HEALING_PCT));
	if (minval)
		healMultiplier *= healMultiplier + minval / 100.0f;

	float maxval = float(m_casterUnit->GetMaxPositiveAuraModifier(SPELL_AURA_MOD_HEALING_PCT));
	if (maxval)
		healMultiplier *= healMultiplier + maxval / 100.0f;


	if (m_casterUnit)
	{
		if (Player* modOwner = m_casterUnit->GetSpellModOwner())
			modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_MULTIPLE_VALUE, healMultiplier);
	}

	// get max possible damage, don't count overkill for heal
	if (damage > static_cast<int32>(unitTarget->GetHealth()))
		damage = unitTarget->GetHealth();

	if (m_casterUnit && m_casterUnit->isAlive())
		m_casterUnit->DealHeal(m_casterUnit, uint32(damage * healMultiplier), m_spellInfo);

	// Non delayed spells bonus damage is added later
	if (!m_delayed)
		damage = initialDamage;

	m_damage += damage;
}

void Spell::DoCreateItem(SpellEffectIndex eff_idx, uint32 itemtype)
{
	if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
		return;

	Player* player = (Player*)unitTarget;

	uint32 newitemid = itemtype;
	ItemPrototype const* pProto = ObjectMgr::GetItemPrototype(newitemid);
	if (!pProto)
	{
		player->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, nullptr, nullptr);
		return;
	}

	// bg reward have some special in code work
	uint32 bgType = 0;
	switch (m_spellInfo->Id)
	{
	case SPELL_AV_MARK_WINNER:
	case SPELL_AV_MARK_LOSER:
	case SPELL_AV_MARK_RELIEF:
	case 34727:
		bgType = BATTLEGROUND_AV;
		break;
	case SPELL_WS_ALLY_WINNER:
	case SPELL_WS_HORDE_WINNER:
	case SPELL_WS_OLD_LOSER:
	case SPELL_WS_MARK_WINNER:
	case SPELL_WS_MARK_LOSER:
	case SPELL_WS_MARK_RELIEF:
		bgType = BATTLEGROUND_WS;
		break;
	case SPELL_AB_OLD_WINNER:
	case SPELL_AB_MARK_WINNER:
	case SPELL_AB_MARK_LOSER:
	case SPELL_AB_MARK_RELIEF:
		bgType = BATTLEGROUND_AB;
		break;
	case SPELL_AC_MARK_WINNER:
	case SPELL_AC_MARK_LOSER:
	case SPELL_AC_MARK_RELIEF:
		bgType = BATTLEGROUND_AC;
		break;
	default:
		break;
	}

	uint32 num_to_add = damage;

	if (num_to_add < 1)
		num_to_add = 1;
	if (num_to_add > pProto->Stackable)
		num_to_add = pProto->Stackable;

	// init items_count to 1, since 1 item will be created regardless of specialization
	int items_count = 1;

	// really will be created more items
	num_to_add *= items_count;

	// can the player store the new item?
	ItemPosCountVec dest;
	uint32 no_space = 0;
	InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, newitemid, num_to_add, &no_space);
	if (msg != EQUIP_ERR_OK)
	{
		// convert to possible store amount
		if (msg == EQUIP_ERR_INVENTORY_FULL || msg == EQUIP_ERR_CANT_CARRY_MORE_OF_THIS)
			num_to_add -= no_space;
		else
		{
			// if not created by another reason from full inventory or unique items amount limitation
			player->SendEquipError(msg, nullptr, nullptr, newitemid);
			return;
		}
	}

	if (num_to_add)
	{
		// create the new item and store it
		Item* pItem = player->StoreNewItem(dest, newitemid, true, Item::GenerateItemRandomPropertyId(newitemid));

		// was it successful? return error if not
		if (!pItem)
		{
			player->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, nullptr, nullptr);
			return;
		}

		// set the "Crafted by ..." property of the item
		if (pItem->GetProto()->HasSignature())
			pItem->SetGuidValue(ITEM_FIELD_CREATOR, player->GetObjectGuid());

		// send info to the client
		if (pItem)
			player->SendNewItem(pItem, num_to_add, true, bgType == 0);

		// we succeeded in creating at least one item, so a levelup is possible
		if (bgType == 0)
			player->UpdateCraftSkill(m_spellInfo->Id);
	}

	// for battleground marks send by mail if not add all expected
	if (no_space > 0 && bgType)
	{
		if (BattleGround* bg = sBattleGroundMgr.GetBattleGroundTemplate(BattleGroundTypeId(bgType)))
			bg->SendRewardMarkByMail(player, newitemid, no_space);
	}
}

void Spell::EffectCreateItem(SpellEffectIndex eff_idx)
{
	DoCreateItem(eff_idx, m_spellInfo->EffectItemType[eff_idx]);

	ExecuteLogInfo info;
	info.createItem.itemEntry = m_spellInfo->EffectItemType[eff_idx];
	AddExecuteLogInfo(eff_idx, info);
}

void Spell::EffectPersistentAA(SpellEffectIndex eff_idx)
{
	WorldObject* pCaster = GetAffectiveCasterObject();

	if (GameObject* pGo = ToGameObject(pCaster))
		if (Unit* pOwner = pGo->GetOwner())
			pCaster = pOwner;

	if (!pCaster)
		pCaster = m_caster;


	if (m_spellInfo->Id == 35003)
	{
		m_casterUnit->NearTeleportTo(m_targets.m_destX, m_targets.m_destY, m_targets.m_destZ, m_casterUnit->GetOrientation(), TELE_TO_NOT_LEAVE_TRANSPORT | TELE_TO_NOT_LEAVE_COMBAT | TELE_TO_NOT_UNSUMMON_PET | (unitTarget == m_caster ? TELE_TO_SPELL : 0));
		return;
	}

	float radius = GetSpellRadius(sSpellRadiusStore.LookupEntry(m_spellInfo->EffectRadiusIndex[eff_idx]));

	if (Unit* pUnit = pCaster->ToUnit())
	{
		if (Player* modOwner = pUnit->GetSpellModOwner())
			modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_RADIUS, radius);
	}

	//Modified duration
	switch (m_spellInfo->Id)
	{
	case 35977: // Ice Blast (Doan)
	{
		/*int32 duration = m_spellInfo->GetMaxDuration();*/
		int32 duration = 1000;
		uint32 auraTargetCount = 0;
		uint32 totalTargetCount = 0;

		Map::PlayerList const& plList = pCaster->GetMap()->GetPlayers();
		for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
		{
			Player* plr = itr->getSource();
			if (!plr || !plr->isAlive())
				continue;

			if (plr->GetDistance(m_targets.m_destX, m_targets.m_destY, m_targets.m_destZ) <= radius)
				++auraTargetCount;
		}

		if (auraTargetCount == 0)
			auraTargetCount = 1;

		if (m_casterUnit && m_casterUnit->IsCreature())
		{
			std::list<Player*> players;

			MaNGOS::AnyPlayerInObjectRangeCheck check(m_casterUnit, 50.0f);
			MaNGOS::PlayerListSearcher<MaNGOS::AnyPlayerInObjectRangeCheck> searcher(players, check);
			Cell::VisitWorldObjects(m_casterUnit, searcher, 50.0f);

			totalTargetCount = players.size();
		}
		if (totalTargetCount == 0)
			totalTargetCount = 1;

		float scale = float(totalTargetCount) / float(auraTargetCount);
		if (scale < 1.0f)
			scale = 1.0f;

		m_duration = int32(float(duration) * scale);

		break;
	}
	default:
		break;
	}

	DynamicObject* dynObj = new DynamicObject;
	if (!dynObj->Create(pCaster->GetMap()->GenerateLocalLowGuid(HIGHGUID_DYNAMICOBJECT), pCaster, m_spellInfo->Id,
		eff_idx, m_targets.m_destX, m_targets.m_destY, m_targets.m_destZ, m_duration, radius, DYNAMIC_OBJECT_AREA_SPELL))
	{
		delete dynObj;
		return;
	}

	pCaster->AddDynObject(dynObj);
	pCaster->GetMap()->Add(dynObj);
}

void Spell::EffectEnergize(SpellEffectIndex eff_idx)
{
	if (!unitTarget)
		return;
	if (!unitTarget->isAlive())
		return;

	if (m_spellInfo->EffectMiscValue[eff_idx] < 0 || m_spellInfo->EffectMiscValue[eff_idx] >= MAX_POWERS)
		return;

	Powers power = Powers(m_spellInfo->EffectMiscValue[eff_idx]);

	if (damage < 0)
		return;

	if (unitTarget->GetMaxPower(power) == 0)
		return;

	if (m_spellInfo->Id == 2687)
		unitTarget->SetInCombatState(false, nullptr);

#if SUPPORTED_CLIENT_BUILD <= CLIENT_BUILD_1_9_4
	ExecuteLogInfo info(unitTarget->GetObjectGuid());
	info.energize.amount = damage;
	info.energize.powerType = power;
	AddExecuteLogInfo(eff_idx, info);
#endif

	m_caster->EnergizeBySpell(unitTarget, m_spellInfo->Id, damage, power);
}

void Spell::SendLoot(ObjectGuid guid, LootType loottype, LockType lockType)
{
	if (!m_casterUnit)
		return;

	if (gameObjTarget)
	{
		switch (gameObjTarget->GetGoType())
		{
		case GAMEOBJECT_TYPE_DOOR:
		case GAMEOBJECT_TYPE_BUTTON:
		case GAMEOBJECT_TYPE_QUESTGIVER:
		case GAMEOBJECT_TYPE_SPELL_FOCUS:
		case GAMEOBJECT_TYPE_GOOBER:
			gameObjTarget->Use(m_casterUnit);
			return;

		case GAMEOBJECT_TYPE_CHEST:
			gameObjTarget->Use(m_casterUnit);
			// Don't return, let loots been taken
			break;

		case GAMEOBJECT_TYPE_TRAP:
			if (lockType == LOCKTYPE_DISARM_TRAP)
			{
				gameObjTarget->SetLootState(GO_JUST_DEACTIVATED);
				return;
			}
			else if ((m_spellInfo->Id == 15748) || (m_spellInfo->Id == 16028)) // Freeze Rookery Egg
			{
				if (gameObjTarget->getLootState() == GO_READY)
					gameObjTarget->UseDoorOrButton(0, true),
					gameObjTarget->SetActiveObjectState(true);
				return;
			}
			sLog.outError("Spell::SendLoot unhandled locktype %u for GameObject trap (entry %u) for spell %u.", lockType, gameObjTarget->GetEntry(), m_spellInfo->Id);
			return;
		default:
			sLog.outError("Spell::SendLoot unhandled GameObject type %u (entry %u) for spell %u.", gameObjTarget->GetGoType(), gameObjTarget->GetEntry(), m_spellInfo->Id);
			return;
		}
	}

	if (m_caster->GetTypeId() != TYPEID_PLAYER)
		return;

	// Send loot
	((Player*)m_caster)->SendLoot(guid, loottype);
}

void Spell::EffectOpenLock(SpellEffectIndex eff_idx)
{
	if (!m_caster || m_caster->GetTypeId() != TYPEID_PLAYER)
	{
		DEBUG_LOG("WORLD: Open Lock - No Player Caster!");
		return;
	}

	Player* player = (Player*)m_caster;

	uint32 lockId = 0;
	ObjectGuid guid;

	// Get lockId
	if (gameObjTarget)
	{
		GameObjectInfo const* goInfo = gameObjTarget->GetGOInfo();
		// Arathi Basin banner opening !
		if ((goInfo->type == GAMEOBJECT_TYPE_BUTTON && goInfo->button.noDamageImmune) ||
			(goInfo->type == GAMEOBJECT_TYPE_GOOBER && goInfo->goober.losOK))
		{
			//CanUseBattleGroundObject() already called in CheckCast()
			// in battleground check
			if (BattleGround* bg = player->GetBattleGround())
			{
				// check if it's correct bg
				if (bg->GetTypeID() == BATTLEGROUND_AB || bg->GetTypeID() == BATTLEGROUND_AV || bg->GetTypeID() == BATTLEGROUND_AC)
					bg->EventPlayerClickedOnFlag(player, gameObjTarget);
				return;
			}
		}
		else if (goInfo->type == GAMEOBJECT_TYPE_FLAGSTAND)
		{
			//CanUseBattleGroundObject() already called in CheckCast()
			// in battleground check
			if (BattleGround* bg = player->GetBattleGround())
				return;
		}
		lockId = goInfo->GetLockId();
		guid = gameObjTarget->GetObjectGuid();
	}
	else if (itemTarget)
	{
		lockId = itemTarget->GetProto()->LockID;
		guid = itemTarget->GetObjectGuid();
	}
	else
	{
		DEBUG_LOG("WORLD: Open Lock - No GameObject/Item Target!");
		return;
	}

	SkillType skillId = SKILL_NONE;
	int32 reqSkillValue = 0;
	int32 skillValue;

	SpellCastResult res = CanOpenLock(eff_idx, lockId, skillId, reqSkillValue, skillValue);
	if (res != SPELL_CAST_OK)
	{
		SendCastResult(res);
		return;
	}

	// mark item as unlocked
	if (itemTarget)
		itemTarget->SetFlag(ITEM_FIELD_FLAGS, ITEM_DYNFLAG_UNLOCKED);

	SendLoot(guid, LOOT_SKINNING, LockType(m_spellInfo->EffectMiscValue[eff_idx]));

	if (gameObjTarget && m_casterUnit)
	{
		if (Unit* pOwner = gameObjTarget->GetOwner())
			m_casterUnit->TogglePlayerPvPFlagOnAttackVictim(pOwner, true);
		if (player)
			sScriptMgr.OnGameObjectOpen(player, gameObjTarget);
		if (gameObjTarget->AI())
			gameObjTarget->AI()->OnUse(m_casterUnit);
	}

	// not allow use skill grow at item base open
	if (!m_CastItem && skillId != SKILL_NONE)
	{
		// update skill if really known
		if (uint32 pureSkillValue = player->GetSkillValuePure(skillId))
		{
			if (gameObjTarget)
			{
				// Allow one skill-up until respawned
				if (!gameObjTarget->IsInSkillupList(player) &&
					player->UpdateGatherSkill(skillId, pureSkillValue, reqSkillValue))
					gameObjTarget->AddToSkillupList(player);
			}
			else if (itemTarget)
			{
				// Do one skill-up
				player->UpdateGatherSkill(skillId, pureSkillValue, reqSkillValue);
			}
		}
	}

	AddExecuteLogInfo(eff_idx, ExecuteLogInfo(guid));
}

void Spell::EffectSummonChangeItem(SpellEffectIndex eff_idx)
{
	if (m_caster->GetTypeId() != TYPEID_PLAYER)
		return;

	Player* player = (Player*)m_caster;

	// applied only to using item
	if (!m_CastItem)
		return;

	// ... only to item in own inventory/bank/equip_slot
	if (m_CastItem->GetOwnerGuid() != player->GetObjectGuid())
		return;

	uint32 newitemid = m_spellInfo->EffectItemType[eff_idx];
	if (!newitemid)
		return;

	uint16 pos = m_CastItem->GetPos();

	Item* pNewItem = Item::CreateItem(newitemid, 1, player);
	if (!pNewItem)
		return;

	for (uint8 j = PERM_ENCHANTMENT_SLOT; j <= TEMP_ENCHANTMENT_SLOT; ++j)
	{
		if (m_CastItem->GetEnchantmentId(EnchantmentSlot(j)))
			pNewItem->SetEnchantment(EnchantmentSlot(j), m_CastItem->GetEnchantmentId(EnchantmentSlot(j)), m_CastItem->GetEnchantmentDuration(EnchantmentSlot(j)), m_CastItem->GetEnchantmentCharges(EnchantmentSlot(j)));
	}

	if (m_CastItem->GetUInt32Value(ITEM_FIELD_DURABILITY) < m_CastItem->GetUInt32Value(ITEM_FIELD_MAXDURABILITY))
	{
		double loosePercent = 1 - m_CastItem->GetUInt32Value(ITEM_FIELD_DURABILITY) / double(m_CastItem->GetUInt32Value(ITEM_FIELD_MAXDURABILITY));
		player->DurabilityLoss(pNewItem, loosePercent);
	}

	if (player->IsInventoryPos(pos))
	{
		ItemPosCountVec dest;
		uint8 msg = player->CanStoreItem(m_CastItem->GetBagSlot(), m_CastItem->GetSlot(), dest, pNewItem, true);
		if (msg == EQUIP_ERR_OK)
		{
			player->DestroyItem(m_CastItem->GetBagSlot(), m_CastItem->GetSlot(), true);

			// prevent crash at access and unexpected charges counting with item update queue corrupt
			ClearCastItem();

			player->StoreItem(dest, pNewItem, true);
			return;
		}
	}
	else if (player->IsBankPos(pos))
	{
		ItemPosCountVec dest;
		uint8 msg = player->CanBankItem(m_CastItem->GetBagSlot(), m_CastItem->GetSlot(), dest, pNewItem, true);
		if (msg == EQUIP_ERR_OK)
		{
			player->DestroyItem(m_CastItem->GetBagSlot(), m_CastItem->GetSlot(), true);

			// prevent crash at access and unexpected charges counting with item update queue corrupt
			ClearCastItem();

			player->BankItem(dest, pNewItem, true);
			return;
		}
	}
	else if (player->IsEquipmentPos(pos))
	{
		uint16 dest;
		uint8 msg = player->CanEquipItem(m_CastItem->GetSlot(), dest, pNewItem, true, false);
		if (msg == EQUIP_ERR_OK)
		{
			player->DestroyItem(m_CastItem->GetBagSlot(), m_CastItem->GetSlot(), true);

			// prevent crash at access and unexpected charges counting with item update queue corrupt
			ClearCastItem();

			player->EquipItem(dest, pNewItem, true);
			player->AutoUnequipOffhandIfNeed();
			return;
		}
	}

	// fail
	delete pNewItem;
}

void Spell::EffectProficiency(SpellEffectIndex /*eff_idx*/)
{
	if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
		return;
	Player* p_target = (Player*)unitTarget;

	uint32 subClassMask = m_spellInfo->EquippedItemSubClassMask;
	if (m_spellInfo->EquippedItemClass == ITEM_CLASS_WEAPON && !(p_target->GetWeaponProficiency() & subClassMask))
	{
		p_target->AddWeaponProficiency(subClassMask);
		p_target->SendProficiency(ITEM_CLASS_WEAPON, p_target->GetWeaponProficiency());
	}
	if (m_spellInfo->EquippedItemClass == ITEM_CLASS_ARMOR && !(p_target->GetArmorProficiency() & subClassMask))
	{
		p_target->AddArmorProficiency(subClassMask);
		p_target->SendProficiency(ITEM_CLASS_ARMOR, p_target->GetArmorProficiency());
	}
}

void Spell::EffectApplyAreaAura(SpellEffectIndex eff_idx)
{
	if (!unitTarget)
		return;
	if (!unitTarget->isAlive())
		return;

	AreaAura* Aur = new AreaAura(m_spellInfo, eff_idx, &m_currentBasePoints[eff_idx], m_spellAuraHolder, unitTarget, m_casterUnit ? m_casterUnit : unitTarget, m_CastItem);
	m_spellAuraHolder->AddAura(Aur, eff_idx);
}

void Spell::EffectSummon(SpellEffectIndex eff_idx)
{
	if (!m_casterUnit)
		return;

	if (!m_casterUnit->GetPetGuid().IsEmpty())
		return;

	if (!unitTarget)
		return;

	uint32 pet_entry = m_spellInfo->EffectMiscValue[eff_idx];
	if (!pet_entry)
		return;

	CreatureInfo const* cInfo = sCreatureStorage.LookupEntry<CreatureInfo>(pet_entry);
	if (!cInfo)
	{
		sLog.outErrorDb("Spell::DoSummon: creature entry %u not found for spell %u.", pet_entry, m_spellInfo->Id);
		return;
	}

	uint32 level = m_casterUnit->getLevel();
	Pet* spawnCreature = new Pet(SUMMON_PET);

	if (m_casterUnit->GetTypeId() == TYPEID_PLAYER && spawnCreature->LoadPetFromDB((Player*)m_casterUnit, pet_entry))
	{
		// Summon in dest location
		if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
			spawnCreature->Relocate(m_targets.m_destX, m_targets.m_destY, m_targets.m_destZ, -m_casterUnit->GetOrientation());

		// set timer for unsummon
		if (m_duration > 0)
			spawnCreature->SetDuration(m_duration);

		return;
	}

	// Summon in dest location
	CreatureCreatePos pos(m_casterUnit->GetMap(), m_targets.m_destX, m_targets.m_destY, m_targets.m_destZ, -m_casterUnit->GetOrientation());

	if (!(m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION))
		pos = CreatureCreatePos(m_casterUnit, -m_casterUnit->GetOrientation());

	Map* map = m_casterUnit->GetMap();
	uint32 pet_number = sObjectMgr.GeneratePetNumber();
	if (!spawnCreature->Create(map->GenerateLocalLowGuid(HIGHGUID_PET), pos, cInfo, pet_number))
	{
		sLog.outErrorDb("Spell::EffectSummon: can't create creature with entry %u for spell %u", cInfo->entry, m_spellInfo->Id);
		delete spawnCreature;
		return;
	}

	spawnCreature->SetSummonPoint(pos);

	// set timer for unsummon
	if (m_duration > 0)
		spawnCreature->SetDuration(m_duration);

	spawnCreature->SetOwnerGuid(m_casterUnit->GetObjectGuid());
	spawnCreature->SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);
	spawnCreature->SetInitCreaturePowerType();
	spawnCreature->setFaction(m_casterUnit->getFaction());
	spawnCreature->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, 0);
	spawnCreature->SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, 0);
	spawnCreature->SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, 1000);
	spawnCreature->SetCreatorGuid(m_casterUnit->GetObjectGuid());
	spawnCreature->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->Id);

	spawnCreature->InitStatsForLevel(level, m_casterUnit);

	spawnCreature->GetCharmInfo()->SetPetNumber(pet_number, false);

	spawnCreature->AIM_Initialize();
	spawnCreature->InitPetCreateSpells();
	spawnCreature->SetHealth(spawnCreature->GetMaxHealth());
	spawnCreature->SetPower(POWER_MANA, spawnCreature->GetMaxPower(POWER_MANA));

	//spawnCreature->SetName("");                           // generated by client

	map->Add((Creature*)spawnCreature);

	m_casterUnit->SetPet(spawnCreature);

	if (m_casterUnit->GetTypeId() == TYPEID_PLAYER)
	{
		spawnCreature->SetReactState(REACT_DEFENSIVE);
		spawnCreature->SavePetToDB(PET_SAVE_AS_CURRENT);
		((Player*)m_casterUnit)->PetSpellInitialize();
	}
	else
		spawnCreature->SetReactState(REACT_AGGRESSIVE);

	if (m_casterUnit->GetTypeId() == TYPEID_UNIT && ((Creature*)m_casterUnit)->AI())
		((Creature*)m_casterUnit)->AI()->JustSummoned((Creature*)spawnCreature);

	AddExecuteLogInfo(eff_idx, ExecuteLogInfo(spawnCreature->GetObjectGuid()));
}

void Spell::EffectLearnSpell(SpellEffectIndex eff_idx)
{
	if (!unitTarget)
		return;

	if (unitTarget->GetTypeId() != TYPEID_PLAYER)
	{
		if (m_caster->GetTypeId() == TYPEID_PLAYER)
			EffectLearnPetSpell(eff_idx);

		return;
	}

	Player* player = (Player*)unitTarget;

	uint32 spellToLearn = m_spellInfo->EffectTriggerSpell[eff_idx];
	player->LearnSpell(spellToLearn, false);

	DEBUG_LOG("Spell: Player %u has learned spell %u from NpcGUID=%u", player->GetGUIDLow(), spellToLearn, m_caster->GetGUIDLow());
}

void Spell::EffectDispel(SpellEffectIndex eff_idx)
{
	if (!unitTarget)
		return;

	// Shield Slam 50% chance dispel
	/*if (m_spellInfo->IsFitToFamily<SPELLFAMILY_WARRIOR, CF_WARRIOR_SHIELD_SLAM>() && !roll_chance_i(50))
		return;*/

		// Fill possible dispel list
	int32 priority_dispel = -1;
	std::list <std::pair<SpellAuraHolder*, uint32> > dispel_list;

	bool checkFaction = true;
	// Pierre de sort dissipe sorts negatifs et positifs.
	if (m_spellInfo->IsFitToFamily<SPELLFAMILY_WARLOCK, CF_WARLOCK_SPELLSTONE>())
		checkFaction = true; //vanilla+ "false"= dispell ALL magic effects
	bool friendly = checkFaction && !isReflected ? unitTarget->IsFriendlyTo(m_caster) : false;

	if (m_spellInfo->Id == 35005)
	{
		if (SpellAuraHolder* holder = unitTarget->GetSpellAuraHolder(34975))
			if (Unit* OGcaster = holder->GetCaster())
				if (OGcaster->IsFriendlyTo(m_caster))
					friendly = true;
	}
	// Create dispel mask by dispel type
	int32 dispel_type = m_spellInfo->EffectMiscValue[eff_idx];
	uint32 dispelMask = GetDispellMask(dispel_type < 0 ? DISPEL_ALL : DispelType(dispel_type));
	Unit::SpellAuraHolderMap const& auras = unitTarget->GetSpellAuraHolderMap();
	for (Unit::SpellAuraHolderMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
	{
		SpellAuraHolder* holder = itr->second;
		if ((1 << holder->GetSpellProto()->Dispel) & dispelMask)
		{
			if (holder->GetSpellProto()->Dispel == DISPEL_MAGIC ||
				holder->GetSpellProto()->Dispel == DISPEL_POISON)
			{
				if (checkFaction)
				{
					bool positive = holder->IsPositive();
					// do not remove positive auras if friendly target
					// do not remove negative auras if non-friendly target
					// when removing charm auras ignore hostile reaction from the charm
					if (!friendly && holder->GetSpellProto()->IsCharmSpell())
					{
						if (CharmInfo* charm = unitTarget->GetCharmInfo())
							if (FactionTemplateEntry const* ft = charm->GetOriginalFactionTemplate())
								if (ft->IsFriendlyTo(*m_caster->getFactionTemplateEntry()))
									priority_dispel = dispel_list.size();
					}
					else if (positive == friendly)
						continue;
				}
			}
			dispel_list.push_back(std::pair<SpellAuraHolder*, uint32>(holder, holder->GetStackAmount()));
		}
	}
	// Ok if exist some buffs for dispel try dispel it
	if (!dispel_list.empty())
	{
		std::list<std::pair<SpellAuraHolder*, uint32> > success_list; // (spell_id,casterGuid)
		std::list < uint32 > fail_list;                     // spell_id

		// some spells have effect value = 0 and all from its by meaning expect 1
		if (!damage)
			damage = 1;

		// Dispel N = damage buffs (or while exist buffs for dispel)
		for (int32 count = 0; count < damage && !dispel_list.empty(); ++count)
		{
			// Random select buff for dispel
			std::list<std::pair<SpellAuraHolder*, uint32> >::iterator dispel_itr = dispel_list.begin();
			if (priority_dispel >= 0)
			{
				std::advance(dispel_itr, priority_dispel);
				priority_dispel = -1;
			}
			else
			{
				std::advance(dispel_itr, urand(0, dispel_list.size() - 1));
			}

			SpellAuraHolder* holder = dispel_itr->first;

			dispel_itr->second -= 1;

			// remove entry from dispel_list if nothing left in stack
			if (dispel_itr->second == 0)
				dispel_list.erase(dispel_itr);

			SpellEntry const* spellInfo = holder->GetSpellProto();
			// Base dispel chance
			// TODO: possible chance depend from spell level??
			int32 miss_chance = 1;
			int32 dispell_chance = 0;
			//Brood Afflictions
			if (holder->GetId() == 23153 || holder->GetId() == 23154 || holder->GetId() == 23155 || holder->GetId() == 23169)
			{
				miss_chance += 15;
			}
			// Apply dispel mod from aura caster
			if (Unit* caster = holder->GetCaster())
			{
				uint32 DisLevel = m_caster->getLevel();
				uint32 OgCasLevel = caster->getLevel();

				if ((OgCasLevel + 3 - DisLevel) > 0)
					miss_chance += (OgCasLevel + 3 - DisLevel) * 2;

				if (Player* modOwner = caster->GetSpellModOwner())
					modOwner->ApplySpellMod(spellInfo->Id, SPELLMOD_RESIST_DISPEL_CHANCE, miss_chance, this);

				if (!friendly)
				{
					if (m_spellInfo->IsAreaOfEffectSpell())
					{
						miss_chance += unitTarget->GetTotalAuraModifier(SPELL_AURA_MOD_AOE_AVOIDANCE);
					}
				}

				//Dispeller's hit chance
				dispell_chance += m_casterUnit->GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_SPELL_HIT_CHANCE, m_spellInfo->GetSpellSchoolMask());

				if (Player* modOwner = m_casterUnit->GetSpellModOwner())
					modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_RESIST_MISS_CHANCE, dispell_chance, this);

				miss_chance -= dispell_chance;
				if (miss_chance < 1)
					miss_chance = 1;
			}
			// Try dispel
			if (roll_chance_i(miss_chance))
				fail_list.push_back(spellInfo->Id);
			else
			{
				bool foundDispelled = false;
				for (std::list<std::pair<SpellAuraHolder*, uint32> >::iterator success_iter = success_list.begin(); success_iter != success_list.end(); ++success_iter)
				{
					if (success_iter->first->GetId() == holder->GetId() && success_iter->first->GetCasterGuid() == holder->GetCasterGuid())
					{
						success_iter->second += 1;
						foundDispelled = true;
						break;
					}
				}
				if (!foundDispelled)
					success_list.push_back(std::pair<SpellAuraHolder*, uint32>(holder, 1));
			}
		}
		// Send success log and really remove auras
		if (!success_list.empty())
		{
			int32 count = success_list.size();
			WorldPacket data(SMSG_SPELLDISPELLOG, 8 + 8 + 4 + count * 4);
#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_8_4
			data << unitTarget->GetPackGUID();              // Victim GUID
			data << m_caster->GetPackGUID();                // Caster GUID
#else
			data << unitTarget->GetGUID();              // Victim GUID
			data << m_caster->GetGUID();                // Caster GUID
#endif
			data << uint32(count);
			for (std::list<std::pair<SpellAuraHolder*, uint32> >::iterator j = success_list.begin(); j != success_list.end(); ++j)
			{
				SpellAuraHolder* dispelledHolder = j->first;
				data << uint32(dispelledHolder->GetId());   // Spell Id
				unitTarget->RemoveAuraHolderDueToSpellByDispel(dispelledHolder->GetId(), j->second, dispelledHolder->GetCasterGuid());


				if (m_casterUnit)
				{
					if (m_casterUnit->HasAura(35748)) //Holy Purge
					{
						if (m_spellInfo->Id != 35751)
						{
							int32 ManaDamHeal = m_casterUnit->GetPower(POWER_MANA) / 100 * 5;
							if (unitTarget->IsFriendlyTo(m_casterUnit))
							{
								m_casterUnit->CastCustomSpell(unitTarget, 35750, &ManaDamHeal, NULL, NULL, true);
							}
							else
							{
								m_casterUnit->CastCustomSpell(unitTarget, 35751, &ManaDamHeal, NULL, NULL, true);
							}
						}
					}

					switch (m_spellInfo->Id)
					{
						case 35974:		// Doan's Siphon Magic
						{
							int32 Mana = dispelledHolder->GetSpellProto()->manaCost * 10;
							if (Mana == 0)
								Mana = 1000;
							m_casterUnit->CastCustomSpell(m_casterUnit, 35975, &Mana, NULL, NULL, true);

							break;
						}
						case 36086:		// Puryfying Flames
						{
							m_casterUnit->CastSpell(unitTarget, 36087, true);

							break;
						}
						case 36088:		// Fiery Purge
						{
							m_casterUnit->CastSpell(unitTarget, 36089, true);

							break;
						}
						case 36138:		// Prayer of Purity
						{
							int32 Heal = unitTarget->GetMaxHealth() / 20;

							m_casterUnit->CastCustomSpell(unitTarget, 36139, &Heal, NULL, NULL, true);

							break;
						}
					}
				}
			}
			m_caster->SendMessageToSet(&data, true);

			// On success dispel
			// Devour Magic
			if (m_spellInfo->SpellFamilyName == SPELLFAMILY_WARLOCK && m_spellInfo->Category == SPELLCATEGORY_HEALING_SPELL)
			{
				if (!m_casterUnit)
					return;

				uint32 heal_spell = 0;
				switch (m_spellInfo->Id)
				{
				case 19505:
					heal_spell = 19658;
					break;
				case 19731:
					heal_spell = 19732;
					break;
				case 19734:
					heal_spell = 19733;
					break;
				case 19736:
					heal_spell = 19735;
					break;
				default:
					DEBUG_LOG("Spell for Devour Magic %d not handled in Spell::EffectDispel", m_spellInfo->Id);
					break;
				}
				if (heal_spell)
					m_casterUnit->CastSpell(m_casterUnit, heal_spell, true);
			}
		}
		// Send fail log to client
		if (!fail_list.empty())
		{
			// Failed to dispel
			WorldPacket data(SMSG_DISPEL_FAILED, 8 + 8 + 4 * fail_list.size());
			data << m_caster->GetObjectGuid();              // Caster GUID
			data << unitTarget->GetObjectGuid();            // Victim GUID
			for (std::list< uint32 >::iterator j = fail_list.begin(); j != fail_list.end(); ++j)
				data << uint32(*j);                         // Spell Id
			m_caster->SendMessageToSet(&data, true);
		}
	}

	AddExecuteLogInfo(eff_idx, ExecuteLogInfo(unitTarget->GetObjectGuid()));
}

void Spell::EffectDualWield(SpellEffectIndex /*eff_idx*/)
{
	if (unitTarget && unitTarget->GetTypeId() == TYPEID_PLAYER)
		((Player*)unitTarget)->SetCanDualWield(true);
}

void Spell::EffectPull(SpellEffectIndex /*eff_idx*/)
{
	// TODO: create a proper pull towards distract spell center for distract
	DEBUG_LOG("WORLD: Spell Effect DUMMY");
}

void Spell::EffectDistract(SpellEffectIndex eff_idx)
{
	// Check for possible target
	if (!unitTarget || unitTarget->isInCombat())
		return;

	// target must be OK to do this
	if (unitTarget->hasUnitState(UNIT_STAT_CAN_NOT_REACT))
		return;

	unitTarget->SetFacingTo(unitTarget->GetAngle(m_targets.m_destX, m_targets.m_destY));
	unitTarget->clearUnitState(UNIT_STAT_MOVING);

	if (unitTarget->GetTypeId() == TYPEID_UNIT)
		unitTarget->GetMotionMaster()->MoveDistract(damage * IN_MILLISECONDS);

	AddExecuteLogInfo(eff_idx, ExecuteLogInfo(unitTarget->GetObjectGuid()));
}

void Spell::EffectPickPocket(SpellEffectIndex /*eff_idx*/)
{
	if (m_caster->GetTypeId() != TYPEID_PLAYER)
		return;

	//AC Gold Pickpocket
	if (unitTarget->HasAura(34975))
	{
		if (SpellAuraHolder* holder = unitTarget->GetSpellAuraHolder(34975))
		{
			uint32 victimStacks = holder->GetStackAmount();
			uint32 StacksDelta = ceil(float(victimStacks / 20.0f));

			//Rogue mod
			if (m_casterUnit->HasAura(34975))
			{
				if (SpellAuraHolder* Rholder = m_casterUnit->GetSpellAuraHolder(34975))
					Rholder->ModStackAmount(StacksDelta);
			}
			else
			{
				m_casterUnit->AddAura(34975);
				if (SpellAuraHolder* Kholder = m_casterUnit->GetSpellAuraHolder(34975))
					Kholder->ModStackAmount(StacksDelta - 1);
			}

			//Victim mod
			if (victimStacks == 1)
				unitTarget->RemoveAurasDueToSpell(34975);
			else
				holder->ModStackAmount(-StacksDelta);
		}
	}

	// victim must be creature and attackable
	if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT || m_caster->IsFriendlyTo(unitTarget))
		return;

	// victim have to be alive and humanoid or undead
	if (unitTarget->isAlive() && (unitTarget->GetCreatureTypeMask() & CREATURE_TYPEMASK_HUMANOID_OR_UNDEAD) != 0)
		((Player*)m_caster)->SendLoot(unitTarget->GetObjectGuid(), LOOT_PICKPOCKETING);
}

void Spell::EffectAddFarsight(SpellEffectIndex eff_idx)
{
	if (m_caster->GetTypeId() != TYPEID_PLAYER)
		return;

	int32 duration = m_spellInfo->GetDuration();
	DynamicObject* dynObj = new DynamicObject;

	// set radius to 0: spell not expected to work as persistent aura
	if (!dynObj->Create(m_caster->GetMap()->GenerateLocalLowGuid(HIGHGUID_DYNAMICOBJECT), m_caster,
		m_spellInfo->Id, eff_idx, m_targets.m_destX, m_targets.m_destY, m_targets.m_destZ, duration, 0, DYNAMIC_OBJECT_FARSIGHT_FOCUS))
	{
		delete dynObj;
		return;
	}

	m_caster->AddDynObject(dynObj);
	m_caster->GetMap()->Add(dynObj);

	((Player*)m_caster)->GetCamera().SetView(dynObj);
}

void Spell::EffectSummonWild(SpellEffectIndex eff_idx)
{
	uint32 creature_entry = m_spellInfo->EffectMiscValue[eff_idx];
	if (!creature_entry)
		return;

	uint32 level = m_caster->getLevel();

	// level of creature summoned using engineering item based at engineering skill level
	if (m_caster->GetTypeId() == TYPEID_PLAYER && m_CastItem)
	{
		ItemPrototype const* proto = m_CastItem->GetProto();
		if (proto && proto->RequiredSkill == SKILL_ENGINEERING)
		{
			uint16 skill202 = ((Player*)m_caster)->GetSkillValue(SKILL_ENGINEERING);
			if (skill202)
				level = skill202 / 5;
		}
	}

	// select center of summon position
	float center_x = m_targets.m_destX;
	float center_y = m_targets.m_destY;
	float center_z = m_targets.m_destZ;

	float radius = GetSpellRadius(sSpellRadiusStore.LookupEntry(m_spellInfo->EffectRadiusIndex[eff_idx]));
	int32 duration = m_spellInfo->GetDuration();
	TempSummonType summonType = (duration == 0) ? TEMPSUMMON_DEAD_DESPAWN : TEMPSUMMON_TIMED_COMBAT_OR_DEAD_DESPAWN;

	int32 amount = damage > 0 ? damage : 1;

	for (int32 count = 0; count < amount; ++count)
	{
		float px, py, pz;
		// If dest location if present
		if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
		{
			// Summon 1 unit in dest location
			if (count == 0)
			{
				px = m_targets.m_destX;
				py = m_targets.m_destY;
				pz = m_targets.m_destZ;
			}
			// Summon in random point all other units if location present
			else
				m_caster->GetRandomPoint(center_x, center_y, center_z, radius, px, py, pz);
		}
		// Summon if dest location not present near caster
		else
		{
			if (radius > 0.0f)
			{
				// not using bounding radius of caster here
				m_caster->GetClosePoint(px, py, pz, 0.0f, radius);
			}
			else
			{
				// EffectRadiusIndex 0 or 36
				px = m_caster->GetPositionX();
				py = m_caster->GetPositionY();
				pz = m_caster->GetPositionZ();
			}
		}

		if ((m_spellInfo->Id == 24670))
		{
			px = m_targets.m_destX;
			py = m_targets.m_destY;
			pz = m_targets.m_destZ;
		}

		if (Creature* summon = m_caster->SummonCreature(creature_entry, px, py, pz, m_caster->GetOrientation(), summonType, duration))
		{
			summon->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->Id);
			// Exception for Alterac Shredder. The second effect of the spell (possess) can't target the shredder
			// because it is not summoned at target selection phase.
			switch (m_spellInfo->Id)
			{
				// Inferno
			case 23426: //BWL
			{
				// Inferno effect
				summon->CastSpell(summon, 22703, true, nullptr);

				break;
			}
			case 24670: //ZG trash
			{
				// Inferno effect
				summon->CastSpell(summon, 22703, true, nullptr);

				if (Unit* pTarget = m_casterUnit->getAttackerForHelper())
					summon->AI()->AttackStart(pTarget);

				break;
			}
			// Both sides
			case 21544:
			case 21565:
				summon->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->EffectTriggerSpell[1]);
				summon->SetCreatorGuid(m_caster->GetObjectGuid());
				*m_selfContainer = nullptr;
				m_caster->CastSpell(summon, m_spellInfo->EffectTriggerSpell[1], true);
				break;
				// Target Dummy
			case 4071:
			case 4072:
			case 19805:
				summon->SetFactionTemporary(m_caster->getFaction(), TEMPFACTION_NONE);
				summon->lootForCreator = true;
				summon->SetCreatorGuid(m_caster->GetObjectGuid());
				if (m_casterUnit)
					summon->SetLootRecipient(m_casterUnit);
				break;
				// Rockwing Gargoyle
			case 16381:
				if (m_casterUnit)
				{
					if (Unit* pTarget = m_casterUnit->getAttackerForHelper())
						summon->AI()->AttackStart(pTarget);
				}
				break;
				// Chained Essence of Eranikus
			case 12766:
				int EranikusText = -1001012;
				switch (urand(0, 7))
				{
				case 1:
					EranikusText = -1001013;
					break;
				case 2:
					EranikusText = -1001014;
					break;
				case 3:
					EranikusText = -1001015;
					break;
				case 4:
					EranikusText = -1001016;
					break;
				case 5:
					EranikusText = -1001017;
					break;
				case 6:
					EranikusText = -1001018;
					break;
				case 7:
					EranikusText = -1001019;
					break;
				}
				if (m_casterUnit)
					summon->MonsterWhisper(EranikusText, m_casterUnit);
				break;
			}

			// UNIT_FIELD_CREATEDBY are not set for these kind of spells.
			// Does exceptions exist? If so, what are they?
			// summon->SetCreatorGuid(m_caster->GetObjectGuid());

			if (count == 0)
				AddExecuteLogInfo(eff_idx, ExecuteLogInfo(summon->GetObjectGuid()));
		}
	}
}

// Used for SPELL_EFFECT_SUMMON_GUARDIAN and SPELL_EFFECT_SUMMON_POSSESSED
void Spell::EffectSummonGuardian(SpellEffectIndex eff_idx)
{
	if (!m_casterUnit)
		return;

	uint32 pet_entry = m_spellInfo->EffectMiscValue[eff_idx];
	if (!pet_entry)
		return;

	CreatureInfo const* cInfo = sCreatureStorage.LookupEntry<CreatureInfo>(pet_entry);
	if (!cInfo)
	{
		sLog.outErrorDb("Spell::DoSummonGuardian: creature entry %u not found for spell %u.", pet_entry, m_spellInfo->Id);
		return;
	}

	// set timer for unsummon
	int32 duration = m_spellInfo->CalculateDuration(m_casterUnit);

	// second direct cast unsummon guardian(s) (guardians without like functionality have cooldown > spawn time)
	if (!m_IsTriggeredSpell && m_casterUnit->GetTypeId() == TYPEID_PLAYER)
	{
		if (!m_spellInfo->SpellIconID == 1753)
		{
			bool found = false;
			// including protector
			while (Pet* old_summon = m_casterUnit->FindGuardianWithEntry(pet_entry))
			{
				old_summon->Unsummon(PET_SAVE_AS_DELETED, m_casterUnit);
				found = true;
			}

			if (found && !(m_spellInfo->DurationIndex && m_spellInfo->Category))
				return;
		}
	}

	// Hard cap for NPC summoned guardians
	if (m_casterUnit->GetTypeId() != TYPEID_PLAYER && m_casterUnit->GetGuardianCountWithEntry(pet_entry) > 15)
		return;

	// in another case summon new
	uint32 level = m_casterUnit->getLevel();

	// Summoned by unit: use caster level ///use creature level but cap at owner level
	if (m_casterUnit->GetTypeId() == TYPEID_UNIT)
	{
		/*uint32 creaturelevel = cInfo->level_min == cInfo->level_max ? cInfo->level_min : urand(cInfo->level_min, cInfo->level_max);
		if (creaturelevel < level) level = creaturelevel;*/
	}
	// Summoned by player
	else
	{
		// Use spell level if possible
		if (m_spellInfo->spellLevel > 1)
		{
			level = m_spellInfo->spellLevel;
		}
		// Else match with owner level but cap at creature max
		else if (cInfo->level_max < level)
		{
			level = cInfo->level_max;
		}
		// Engineering summons always scale with skill points
		if (m_CastItem)
		{
			ItemPrototype const* proto = m_CastItem->GetProto();
			if (proto && proto->RequiredSkill == SKILL_ENGINEERING)
			{
				uint16 engiLevel = ((Player*)m_casterUnit)->GetSkillValue(SKILL_ENGINEERING);
				if (engiLevel)
				{
					level = engiLevel / 5;
				}
			}
		}
		// Eye of Kilrog
		if (m_spellInfo->Effect[eff_idx] == SPELL_EFFECT_SUMMON_POSSESSED)
			level = m_casterUnit->getLevel();
	}

	// select center of summon position
	float center_x = m_targets.m_destX;
	float center_y = m_targets.m_destY;
	float center_z = m_targets.m_destZ;

	if (m_spellInfo->Id == 35334 || m_spellInfo->Id == 36317) //tentacle call/death by bug
	{
		center_x = unitTarget->GetPositionX();
		center_y = unitTarget->GetPositionY();
		center_z = unitTarget->GetPositionZ();
	}

	float radius = GetSpellRadius(sSpellRadiusStore.LookupEntry(m_spellInfo->EffectRadiusIndex[eff_idx]));

	int32 amount = damage > 0 ? damage : 1;

	for (int32 count = 0; count < amount; ++count)
	{
		Pet* spawnCreature = new Pet(GUARDIAN_PET);

		// If dest location if present
		// Summon 1 unit in dest location
		CreatureCreatePos pos(m_casterUnit->GetMap(), m_targets.m_destX, m_targets.m_destY, m_targets.m_destZ, -m_casterUnit->GetOrientation());

		if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
		{
			// Summon in random point all other units if location present
			if (count > 0)
			{
				float x, y, z;
				m_casterUnit->GetRandomPoint(center_x, center_y, center_z, radius, x, y, z);
				pos = CreatureCreatePos(m_casterUnit->GetMap(), x, y, z, m_casterUnit->GetOrientation());
			}
		}
		// Summon if dest location not present near caster
		else
			pos = CreatureCreatePos(m_casterUnit, m_casterUnit->GetOrientation());

		Map* map = m_casterUnit->GetMap();
		uint32 pet_number = sObjectMgr.GeneratePetNumber();
		if (!spawnCreature->Create(map->GenerateLocalLowGuid(HIGHGUID_PET), pos, cInfo, pet_number))
		{
			sLog.outError("Spell::DoSummonGuardian: can't create creature entry %u for spell %u.", pet_entry, m_spellInfo->Id);
			delete spawnCreature;
			return;
		}

		spawnCreature->SetSummonPoint(pos);

		if (m_duration > 0)
			spawnCreature->SetDuration(m_duration);

		//spawnCreature->SetName("");                       // generated by client
		spawnCreature->SetOwnerGuid(m_casterUnit->GetObjectGuid());
		spawnCreature->SetInitCreaturePowerType();
		spawnCreature->SetUInt32Value(UNIT_NPC_FLAGS, spawnCreature->GetCreatureInfo()->npc_flags);
		spawnCreature->setFaction(m_casterUnit->getFaction());
		spawnCreature->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, 0);
		spawnCreature->SetCreatorGuid(m_casterUnit->GetObjectGuid());
		spawnCreature->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->Id);

		spawnCreature->InitStatsForLevel(level, m_casterUnit);
		spawnCreature->GetCharmInfo()->SetPetNumber(pet_number, false);

		spawnCreature->AIM_Initialize();

		m_casterUnit->AddGuardian(spawnCreature);

		map->Add((Creature*)spawnCreature);

		// Notify Summoner
		if (m_casterUnit->GetTypeId() == TYPEID_UNIT && ((Creature*)m_casterUnit)->AI())
			((Creature*)m_casterUnit)->AI()->JustSummoned(spawnCreature);

		//Eg set summoned units to get 
		if (Unit* OwnerOfOwner = m_casterUnit->GetCharmerOrOwner())
			if (OwnerOfOwner->isInCombat())
				if (Unit* sTarget = OwnerOfOwner->getAttackerForHelper())
					spawnCreature->SetInCombatWith(sTarget);

		switch (m_spellInfo->Id)
		{
		case 17166: // Release Umi's Yeti - Quest Are We There, Yeti? Part 3
		{
			spawnCreature->MonsterTextEmote(-1900169);
			spawnCreature->MonsterSay(-1900170);

			switch (spawnCreature->GetAreaId())
			{
			case 541: // Un'Goro Crater
				if (Creature* pCreature = spawnCreature->FindNearestCreature(10977, 30.0f, true)) // NPC_QUIXXIL
				{
					spawnCreature->GetMotionMaster()->MoveFollow(pCreature, 0.6f, M_PI_F);
					pCreature->MonsterSay(-1900171);
					pCreature->SetWalk(false);
					pCreature->GetMotionMaster()->MoveWaypoint(0, 0, 0, 0, 0, false);
				}
				break;
			case 976: // Tanaris
				if (Creature* pCreature = spawnCreature->FindNearestCreature(7583, 30.0f, true)) // NPC_SPRINKLE
				{
					spawnCreature->GetMotionMaster()->MoveFollow(pCreature, 0.6f, M_PI_F);
					pCreature->MonsterTextEmote(-1900172);
					pCreature->SetWalk(false);
					pCreature->GetMotionMaster()->MoveWaypoint(0, 0, 0, 0, 0, false);
				}
				break;
			case 2255: // Winterspring
				if (Creature* pCreature = spawnCreature->FindNearestCreature(10978, 30.0f, true)) // NPC_LEGACKI
				{
					spawnCreature->GetMotionMaster()->MoveFollow(pCreature, 0.6f, M_PI_F);
					pCreature->MonsterTextEmote(-1900173);
					pCreature->SetWalk(false);
					pCreature->GetMotionMaster()->MoveWaypoint(0, 0, 0, 0, 0, false);
				}
				break;
			}
			break;
		}
		case 26391: // Vanquished Tentacle
		case 35334:
		{
			CharmInfo* charmInfo = spawnCreature->GetCharmInfo();
			charmInfo->SetIsAtStay(true);
			charmInfo->SetCommandState(COMMAND_STAY);
			charmInfo->SetIsCommandFollow(false);
			charmInfo->SaveStayPosition();
			break;
		}
		case 36222: // Scarlet Tracking Hound
		{
			spawnCreature->AddAura(18950);
			spawnCreature->AddAura(36053);

			break;
		}
		}

		if (count == 0)
			AddExecuteLogInfo(eff_idx, ExecuteLogInfo(spawnCreature->GetObjectGuid()));
	}
}

void Spell::EffectSummonPossessed(SpellEffectIndex eff_idx)
{
	Player* pCaster = m_caster->ToPlayer();
	if (!pCaster)
		return;

	uint32 creatureEntry = m_spellInfo->EffectMiscValue[eff_idx];

	Creature* pMinion = pCaster->SummonPossessedMinion(creatureEntry, m_spellInfo->Id, m_targets.m_destX, m_targets.m_destY, m_targets.m_destZ, m_caster->GetOrientation());
	if (!pMinion)
	{
		sLog.outError("Spell::EffectSummonPossessed: creature entry %u for spell %u could not be summoned.", creatureEntry, m_spellInfo->Id);
		return;
	}

	// Notify Summoner
	if (m_originalCaster && m_originalCaster != m_caster && m_originalCaster->AI())
		m_originalCaster->AI()->JustSummoned(pMinion);
}

void Spell::EffectTeleUnitsFaceCaster(SpellEffectIndex eff_idx)
{
	if (!unitTarget)
		return;

	if (unitTarget->IsTaxiFlying())
		return;

	float fx, fy, fz;

	if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
		m_targets.getDestination(fx, fy, fz);
	else
	{
		float dis = GetSpellRadius(sSpellRadiusStore.LookupEntry(m_spellInfo->EffectRadiusIndex[eff_idx]));
		m_caster->GetClosePoint(fx, fy, fz, unitTarget->GetObjectBoundingRadius(), dis);
	}

	unitTarget->NearTeleportTo(fx, fy, fz, -m_caster->GetOrientation(), TELE_TO_NOT_LEAVE_COMBAT | TELE_TO_NOT_UNSUMMON_PET | (unitTarget == m_caster ? TELE_TO_SPELL : 0));
}

void Spell::EffectLearnSkill(SpellEffectIndex eff_idx)
{
	if (unitTarget->GetTypeId() != TYPEID_PLAYER)
		return;

	if (damage < 0)
		return;

	Player* target = static_cast<Player*>(unitTarget);

	uint16 skillid = uint16(m_spellInfo->EffectMiscValue[eff_idx]);
	uint16 step = uint16(damage);
	uint16 current = std::max(uint16(1), target->GetSkillValuePure(skillid));
	uint16 max = (step * 75);
	target->SetSkill(skillid, current, max, step);

	if (WorldObject const* caster = GetCastingObject())
		DEBUG_LOG("Spell: %s has learned skill %u (to maxlevel %u) from %s", target->GetGuidStr().c_str(), skillid, max, caster->GetGuidStr().c_str());
}

void Spell::EffectAddHonor(SpellEffectIndex /*eff_idx*/)
{
	if (unitTarget->GetTypeId() != TYPEID_PLAYER)
		return;

	// honor-spells don't scale with level and won't be casted by an item
	// also we must use damage (spelldescription says +25 honor but damage is only 24)
	((Player*)unitTarget)->GetHonorMgr().Add(float(damage), QUEST);
	DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "SpellEffect::AddHonor (spell_id %u) rewards %u honor points (non scale) for player: %u", m_spellInfo->Id, damage, ((Player*)unitTarget)->GetGUIDLow());
}

void Spell::EffectSpawn(SpellEffectIndex /*eff_idx*/)
{
	if (!unitTarget || (unitTarget->GetTypeId() != TYPEID_UNIT))
		return;

	unitTarget->SetVisibility(VISIBILITY_ON);
}

void Spell::EffectTradeSkill(SpellEffectIndex /*eff_idx*/)
{
	if (unitTarget->GetTypeId() != TYPEID_PLAYER)
		return;
	// uint32 skillid =  m_spellInfo->EffectMiscValue[i];
	// uint16 skillmax = ((Player*)unitTarget)->(skillid);
	// ((Player*)unitTarget)->SetSkill(skillid,skillval?skillval:1,skillmax+75);
}

void Spell::EffectEnchantItemPerm(SpellEffectIndex eff_idx)
{
	if (m_caster->GetTypeId() != TYPEID_PLAYER)
		return;
	if (!itemTarget)
		return;

	Player* p_caster = (Player*)m_caster;

	// not grow at item use at item case
	p_caster->UpdateCraftSkill(m_spellInfo->Id);

	uint32 enchant_id = m_spellInfo->EffectMiscValue[eff_idx];
	if (!enchant_id)
		return;

	SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
	if (!pEnchant)
		return;

	// item can be in trade slot and have owner diff. from caster
	Player* item_owner = itemTarget->GetOwner();
	if (!item_owner)
		return;

	if (!sWorld.getConfig(CONFIG_BOOL_GM_ALLOW_TRADES) && p_caster->GetSession()->GetSecurity() > SEC_PLAYER)
		return;

	if (item_owner != p_caster && p_caster->GetSession()->GetSecurity() > SEC_PLAYER && sWorld.getConfig(CONFIG_BOOL_GM_LOG_TRADE))
	{
		sLog.outCommand(p_caster->GetSession()->GetAccountId(), "GM %s (Account: %u) enchanting(perm): %s (Entry: %d) for player: %s (Account: %u)",
			p_caster->GetName(), p_caster->GetSession()->GetAccountId(),
			itemTarget->GetProto()->Name1, itemTarget->GetEntry(),
			item_owner->GetName(), item_owner->GetSession()->GetAccountId());
	}

	// remove old enchanting before applying new if equipped
	item_owner->ApplyEnchantment(itemTarget, PERM_ENCHANTMENT_SLOT, false);

	itemTarget->SetEnchantment(PERM_ENCHANTMENT_SLOT, enchant_id, 0, 0);

	// add new enchanting if equipped
	item_owner->ApplyEnchantment(itemTarget, PERM_ENCHANTMENT_SLOT, true);
}

void Spell::EffectEnchantItemTmp(SpellEffectIndex eff_idx)
{
	if (m_caster->GetTypeId() != TYPEID_PLAYER)
		return;

	Player* p_caster = (Player*)m_caster;

	if (!itemTarget)
		return;

	uint32 enchant_id = m_spellInfo->EffectMiscValue[eff_idx];
	uint32 charges = sSpellMgr.GetSpellEnchantCharges(m_spellInfo->Id);

	if (!enchant_id)
	{
		sLog.outError("Spell %u Effect %u (SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY) have 0 as enchanting id", m_spellInfo->Id, eff_idx);
		return;
	}

	SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
	if (!pEnchant)
	{
		sLog.outError("Spell %u Effect %u (SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY) have nonexistent enchanting id %u ", m_spellInfo->Id, eff_idx, enchant_id);
		return;
	}

	// item can be in trade slot and have owner diff. from caster
	Player* item_owner = itemTarget->GetOwner();
	if (!item_owner)
		return;

	if (!sWorld.getConfig(CONFIG_BOOL_GM_ALLOW_TRADES) && p_caster->GetSession()->GetSecurity() > SEC_PLAYER)
		return;

	if (item_owner != p_caster && p_caster->GetSession()->GetSecurity() > SEC_PLAYER && sWorld.getConfig(CONFIG_BOOL_GM_LOG_TRADE))
	{
		sLog.outCommand(p_caster->GetSession()->GetAccountId(), "GM %s (Account: %u) enchanting(temp): %s (Entry: %d) for player: %s (Account: %u)",
			p_caster->GetName(), p_caster->GetSession()->GetAccountId(),
			itemTarget->GetProto()->Name1, itemTarget->GetEntry(),
			item_owner->GetName(), item_owner->GetSession()->GetAccountId());
	}

	// remove old enchant before applying new
	item_owner->ApplyEnchantment(itemTarget, TEMP_ENCHANTMENT_SLOT, false);

	itemTarget->SetEnchantment(TEMP_ENCHANTMENT_SLOT, enchant_id, damage * 1000, charges);

	// add new enchanting if equipped
	item_owner->ApplyEnchantment(itemTarget, TEMP_ENCHANTMENT_SLOT, true);
}

void Spell::EffectTameCreature(SpellEffectIndex /*eff_idx*/)
{
	// Caster must be player, checked in Spell::CheckCast
	// Spell can be triggered, we need to check original caster prior to caster
	Player* plr = (Player*)GetAffectiveCaster();

	Creature* creatureTarget = (Creature*)unitTarget;

	// cast finish successfully
	//SendChannelUpdate(0);
	finish();

	Pet* pet = new Pet(HUNTER_PET);

	// Nostalrius: defensive as default behaviour
	pet->SetReactState(REACT_DEFENSIVE);

	if (!pet->CreateBaseAtCreature(creatureTarget))
	{
		delete pet;
		return;
	}

	pet->SetOwnerGuid(plr->GetObjectGuid());
	pet->SetCreatorGuid(plr->GetObjectGuid());
	pet->setFaction(plr->getFaction());
	pet->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->Id);

	if (plr->IsPvP())
		pet->SetPvP(true);

	if (!pet->InitStatsForLevel(creatureTarget->getLevel()))
	{
		sLog.outError("Pet::InitStatsForLevel() failed for creature (Entry: %u)!", creatureTarget->GetEntry());
		delete pet;
		return;
	}

	pet->GetCharmInfo()->SetPetNumber(sObjectMgr.GeneratePetNumber(), true);
	// this enables pet details window (Shift+P)
	pet->AIM_Initialize();
	pet->InitPetCreateSpells();
	pet->SetHealth(pet->GetMaxHealth());

	// "kill" original creature
	creatureTarget->ForcedDespawn();

	// prepare visual effect for levelup
	pet->SetUInt32Value(UNIT_FIELD_LEVEL, creatureTarget->getLevel() - 1);

	// Apply default loyalty at summon
	LoyaltyLevel defaultLoyalty = LoyaltyLevel(sWorld.getConfig(CONFIG_UINT32_PET_DEFAULT_LOYALTY));
	while (pet->GetLoyaltyLevel() != defaultLoyalty)
		pet->ModifyLoyalty(pet->GetStartLoyaltyPoints(defaultLoyalty));

	// add to world
	pet->GetMap()->Add((Creature*)pet);

	// visual effect for levelup
	pet->SetUInt32Value(UNIT_FIELD_LEVEL, creatureTarget->getLevel());

	// caster have pet now
	plr->SetPet(pet);

	pet->SavePetToDB(PET_SAVE_AS_CURRENT);
	plr->PetSpellInitialize();
}

void Spell::EffectSummonPet(SpellEffectIndex eff_idx)
{
	if (!m_casterUnit)
		return;

	uint32 petLevel = m_casterUnit->IsPlayer() ? m_casterUnit->getLevel() : std::max(int32(m_casterUnit->getLevel()) + int32(m_spellInfo->EffectMultipleValue[eff_idx]), 1);

	ObjectGuid petGuid = m_casterUnit->EffectSummonPet(m_spellInfo->Id, m_spellInfo->EffectMiscValue[eff_idx], petLevel);
	if (petGuid)
		AddExecuteLogInfo(eff_idx, ExecuteLogInfo(petGuid));
}

ObjectGuid Unit::EffectSummonPet(uint32 spellId, uint32 petEntry, uint32 petLevel)
{
	if (!UnsummonOldPetBeforeNewSummon(petEntry))
		return ObjectGuid();

	CreatureInfo const* cInfo = petEntry ? sCreatureStorage.LookupEntry<CreatureInfo>(petEntry) : NULL;

	// == 0 in case call current pet, check only real summon case
	if (petEntry && !cInfo)
	{
		sLog.outErrorDb("EffectSummonPet: creature entry %u not found for spell %u.", petEntry, spellId);
		return ObjectGuid();
	}

	Pet* NewSummon = new Pet;

	// petEntry==0 for hunter "call pet" (current pet summoned if any)
	if (GetTypeId() == TYPEID_PLAYER && NewSummon->LoadPetFromDB((Player*)this, petEntry))
	{
		/*if (NewSummon->getPetType() == SUMMON_PET)
		{*/
		// Remove Demonic Sacrifice auras (known pet)
		//Unit::AuraList const& auraClassScripts = GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
		//for (Unit::AuraList::const_iterator itr = auraClassScripts.begin(); itr != auraClassScripts.end();)
		//{
		//	if ((*itr)->GetModifier()->m_miscvalue == 2228)
		//	{
		//		RemoveAurasDueToSpell((*itr)->GetId());
		//		itr = auraClassScripts.begin();
		//	}
		//	else
		//		++itr;
		//}
	/*}*/
		return NewSummon->GetObjectGuid();
	}

	// not error in case fail hunter call pet
	if (!petEntry)
	{
		delete NewSummon;
		return ObjectGuid();
	}

	CreatureCreatePos pos(this, GetOrientation(), PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);

	Map* map = GetMap();
	uint32 pet_number = sObjectMgr.GeneratePetNumber();
	if (!NewSummon->Create(map->GenerateLocalLowGuid(HIGHGUID_PET), pos, cInfo, pet_number))
	{
		delete NewSummon;
		return ObjectGuid();
	}

	NewSummon->SetSummonPoint(pos);

	NewSummon->setPetType(SUMMON_PET);

	uint32 faction = getFaction();
	if (GetTypeId() == TYPEID_UNIT)
		NewSummon->SetReactState(REACT_AGGRESSIVE);
	else
		NewSummon->SetReactState(REACT_DEFENSIVE);

	NewSummon->SetOwnerGuid(GetObjectGuid());
	NewSummon->SetCreatorGuid(GetObjectGuid());
	NewSummon->SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);
	NewSummon->setFaction(faction);
	NewSummon->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(time(nullptr)));
	NewSummon->SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, 0);
	NewSummon->SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, 1000);
	NewSummon->SetUInt32Value(UNIT_CREATED_BY_SPELL, spellId);

	// Only players have permanent pets. Fix elite pets for NPCs.
	NewSummon->GetCharmInfo()->SetPetNumber(pet_number, GetTypeId() == TYPEID_PLAYER);
	NewSummon->GetCharmInfo()->SetCommandState(COMMAND_FOLLOW);

	// this enables pet details window (Shift+P)
	if (IsPvP())
		NewSummon->SetPvP(true);

	NewSummon->InitStatsForLevel(petLevel, this);
	NewSummon->InitPetCreateSpells();

	if (NewSummon->getPetType() == SUMMON_PET)
	{
		// Remove Demonic Sacrifice auras (new pet)
		//Unit::AuraList const& auraClassScripts = GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
		//for (Unit::AuraList::const_iterator itr = auraClassScripts.begin(); itr != auraClassScripts.end();)
		//{
		//	if ((*itr)->GetModifier()->m_miscvalue == 2228)
		//	{
		//		RemoveAurasDueToSpell((*itr)->GetId());
		//		itr = auraClassScripts.begin();
		//	}
		//	else
		//		++itr;
		//}

		// generate new name for summon pet
		std::string new_name = sObjectMgr.GeneratePetName(petEntry);
		if (!new_name.empty())
			NewSummon->SetName(new_name);
	}
	else if (NewSummon->getPetType() == HUNTER_PET)
		NewSummon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PET_RENAME);

	NewSummon->AIM_Initialize();
	NewSummon->SetHealth(NewSummon->GetMaxHealth());
	NewSummon->SetPower(POWER_MANA, NewSummon->GetMaxPower(POWER_MANA));

	map->Add((Creature*)NewSummon);

	SetPet(NewSummon);
	DEBUG_LOG("New Pet has guid %u", NewSummon->GetGUIDLow());

	if (Player* caster = ToPlayer())
	{
		NewSummon->SavePetToDB(PET_SAVE_AS_CURRENT);
		caster->PetSpellInitialize();
		caster->m_petEntry = NewSummon->GetEntry();
		caster->m_petSpell = NewSummon->GetUInt32Value(UNIT_CREATED_BY_SPELL);
	}

	return NewSummon->GetObjectGuid();
}

void Spell::EffectLearnPetSpell(SpellEffectIndex eff_idx)
{
	if (m_caster->GetTypeId() != TYPEID_PLAYER)
		return;

	Player* _player = (Player*)m_caster;

	Pet* pet = _player->GetPet();
	if (!pet)
		return;
	if (!pet->isAlive())
		return;

	SpellEntry const* learn_spellproto = sSpellMgr.GetSpellEntry(m_spellInfo->EffectTriggerSpell[eff_idx]);
	if (!learn_spellproto)
		return;

	if (!pet->CanLearnPetSpell(learn_spellproto->Id))
		return;

	pet->SetTP(pet->m_TrainingPoints - pet->GetTPForSpell(learn_spellproto->Id));
	pet->LearnSpell(learn_spellproto->Id);

	pet->SavePetToDB(PET_SAVE_AS_CURRENT);
	_player->PetSpellInitialize();
}

void Spell::EffectTaunt(SpellEffectIndex eff_idx)
{
	if (!unitTarget || !m_casterUnit)
		return;

	// this effect use before aura Taunt apply for prevent taunt already attacking target
	// for spell as marked "non effective at already attacking target"
	if (unitTarget->GetTypeId() != TYPEID_PLAYER)
	{
		if (unitTarget->getVictim() == m_casterUnit)
		{
			SendCastResult(SPELL_FAILED_DONT_REPORT);
			return;
		}
	}

	// Also use this effect to set the taunter's threat to the taunted creature's highest value
	if (unitTarget->CanHaveThreatList() && unitTarget->getThreatManager().getCurrentVictim())
	{
		unitTarget->getThreatManager().addThreat(m_casterUnit, unitTarget->getThreatManager().getCurrentVictim()->getThreat());

		// Patch 1.11 notes
		// https://web.archive.org/web/20061109034626/http://evilempireguild.org:80/guides/kenco2.php
		// Recently(1.11.x), the behaviour of Taunt has been buffed slightly. It now does three things :

		// Taunt debuff.The mob is forced to attack you for 3 seconds.Later taunts by other players override this.
		// You are given threat equal to the mob's previous aggro target, permanently. Importantly, you won't necessarily get as much threat
		// as the highest person on the mob's list, only as much as whoever is currently tanking it.
		// You gain complete aggro on the mob at the instant you taunt.Usually you would need 10 % more threat to gain aggro(see section 3),
		// but a taunt now gives you instant aggro on the mob.
		// Of course if other people are generating significant threat on the mob, they could exceed your threat by more than 10 % before the taunt debuff wears off,
		// and will gain aggro as soon as it does.There is no limit to the amount of threat you can gain from Taunt.
#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_10_2
		unitTarget->getThreatManager().setCurrentVictimIfCan(m_casterUnit);
#endif
	}

	AddExecuteLogInfo(eff_idx, ExecuteLogInfo(unitTarget->GetObjectGuid()));
}

void Spell::EffectWeaponDmg(SpellEffectIndex eff_idx)
{
	if (!m_casterUnit)
		return;
	if (!unitTarget)
		return;
	if (!unitTarget->isAlive())
		return;

	//if (m_spellInfo->Id == 17364) // Courroux naturel
	//{
	//	if (!m_casterUnit->isAlive()) // CalculateMeleeDamage does not work in that case.
	//		return;
	//	m_casterUnit->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_MELEE_ATTACK);
	//	if (Spell* spell = m_casterUnit->GetCurrentSpell(CURRENT_MELEE_SPELL))
	//		spell->cast();
	//	CalcDamageInfo damageInfo;
	//	m_casterUnit->CalculateMeleeDamage(unitTarget, 0, &damageInfo, BASE_ATTACK);

	//	// Send log damage message to client
	//	for (uint8 i = 0; i < m_casterUnit->GetWeaponDamageCount(BASE_ATTACK); i++)
	//	{
	//		damageInfo.totalDamage -= damageInfo.subDamage[i].damage;
	//		m_casterUnit->DealDamageMods(unitTarget, damageInfo.subDamage[i].damage, &damageInfo.subDamage[i].absorb);
	//		damageInfo.totalDamage += damageInfo.subDamage[i].damage;
	//	}

	//	m_casterUnit->SendAttackStateUpdate(&damageInfo);
	//	m_casterUnit->ProcDamageAndSpell(damageInfo.target, damageInfo.procAttacker, damageInfo.procVictim, damageInfo.procEx, damageInfo.totalDamage, damageInfo.attackType);
	//	m_casterUnit->DealMeleeDamage(&damageInfo, true);

	//	// if damage unitTarget call AI reaction
	//	unitTarget->AttackedBy(m_casterUnit);
	//	m_damage = 0;
	//	return;
	//}

	// multiple weapon dmg effect workaround
	// execute only the last weapon damage
	// and handle all effects at once
	for (int j = 0; j < MAX_EFFECT_INDEX; ++j)
	{
		switch (m_spellInfo->Effect[j])
		{
		case SPELL_EFFECT_WEAPON_DAMAGE:
		case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
		case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
		case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
			if (j < int(eff_idx))                             // we must calculate only at last weapon effect
				return;
			break;
		}
	}

	// some spell specific modifiers
	bool customBonusDamagePercentMod = false;
	float bonusDamagePercentMod = 1.0f;                    // applied to fixed effect damage bonus if set customBonusDamagePercentMod
	float weaponDamagePercentMod = 1.0f;                    // applied to weapon damage (and to fixed effect damage bonus if customBonusDamagePercentMod not set
	bool normalized = false;

	switch (m_spellInfo->SpellFamilyName)
	{
	case SPELLFAMILY_GENERIC:
	{
		switch (m_spellInfo->Id)
		{
			case 36049: // Impale
			{
				m_casterUnit->CastSpell(unitTarget, 36051, true);
				break;
			}
		}
		break;
	}
	case SPELLFAMILY_ROGUE:
	{
		// Ambush
		if (m_spellInfo->IsFitToFamilyMask<CF_ROGUE_AMBUSH>())
		{
			customBonusDamagePercentMod = true;
			bonusDamagePercentMod = 2.5f;               // 250%
		}
		break;
	}
	case SPELLFAMILY_HUNTER:
	{
		//vanilla+ savage blow hunter
		//OH proc
		if (m_spellInfo->Id == 34591)
		{
			if (!unitTarget || !m_casterUnit->haveOffhandWeapon())
				return;
		}

		if (m_spellInfo->Id == 33590)
		{
			if (!unitTarget)
				return;

			if (m_casterUnit->HasAura(13161)) //Beast 34414
				/*m_casterUnit->CastSpell(unitTarget, 34235, true);*/
				m_casterUnit->CastSpell(unitTarget, 34414, true), //cleave effect MH
				m_casterUnit->CastSpell(unitTarget, 35134, true); //cleave effect OH

			if (m_casterUnit->HasAura(5118)) //Cheetah
			{
				m_casterUnit->CastSpell(m_casterUnit, 34236, true);
				if (m_casterUnit->HasAura(15571))
					m_casterUnit->RemoveAurasDueToSpell(15571);
			}

			if (m_casterUnit->HasAura(13163)) //Monkey
			{
				m_casterUnit->ModifyAuraState(AURA_STATE_DEFENSE, true);
				m_casterUnit->StartReactiveTimer(REACTIVE_DEFENSE, unitTarget->GetObjectGuid());
			}

			if (m_casterUnit->HasAura(13159)) //Pack
				m_casterUnit->CastSpell(m_casterUnit, 34237, true);

			if (m_casterUnit->HasAura(20043) || m_casterUnit->HasAura(20190)) //Wild
				m_casterUnit->CastSpell(m_casterUnit, 34238, true);

			if (m_casterUnit->HasAura(13165) || m_casterUnit->HasAura(14318) || m_casterUnit->HasAura(14319) || m_casterUnit->HasAura(14320) || m_casterUnit->HasAura(14321) || m_casterUnit->HasAura(14322) || m_casterUnit->HasAura(25296)) //Hawk
				m_casterUnit->CastSpell(unitTarget, 35891, false);
		}
		break;
	}
	case SPELLFAMILY_SHAMAN:
	{
		//Stormstrike + Stormforged
		if (m_spellInfo->Id == 17364)
		{
			if (m_casterUnit->HasAura(35752))
			{
				int32 ManaGain = m_casterUnit->GetMaxPower(POWER_MANA) / 100 * 5;
				m_casterUnit->CastCustomSpell(m_casterUnit, 35753, &ManaGain, NULL, NULL, true);

				//Lightning Shield + Earth Shield
				Unit::AuraList const& mEleShield = m_casterUnit->GetAurasByType(SPELL_AURA_PROC_TRIGGER_SPELL);
				for (Unit::AuraList::const_iterator i = mEleShield.begin(); i != mEleShield.end(); ++i)
				{
					if ((*i)->GetSpellProto()->IsFitToFamily<SPELLFAMILY_SHAMAN, CF_SHAMAN_LIGHTNING_SHIELD, CF_SHAMAN_EARTH_SHIELD>())
					{
						if (SpellAuraHolder* holder = (*i)->GetHolder())
						{
							holder->SetAuraCharges(holder->GetAuraCharges() + 3);
						}
						break;
					}
				}
			}
		}

		//Stormstrike OH proc
		if (m_spellInfo->Id == 34592)
		{
			if (!unitTarget || !m_casterUnit->haveOffhandWeapon())
				return;
		}
		break;
	}
	case SPELLFAMILY_WARRIOR:
	{
		//V+ Slam imp.
		if (m_spellInfo->IsFitToFamily<SPELLFAMILY_WARRIOR, CF_WARRIOR_SLAM>())
		{
			if (!m_casterUnit->HasAura(20497))
			{
				m_casterUnit->resetAttackTimer(BASE_ATTACK);
				if (m_casterUnit->haveOffhandWeapon())
					m_casterUnit->resetAttackTimer(OFF_ATTACK);
			}
			else
			{
				int32 const WarSpell[3][4] =
				{
					{ 21553, 21552, 21551, 12294}, //Mortal Str
					{ 23894, 23893, 23892, 23881}, //BT
					{ 23925, 23924, 23923, 23922}  //Shield Slam
				};

				for (uint8 i = 0; i < 3; ++i)
				{
					uint32 CDSpell = 0;

					if (m_casterUnit->HasSpell(WarSpell[i][0]))
						CDSpell = (WarSpell[i][0]);
					else if (m_casterUnit->HasSpell(WarSpell[i][1]))
						CDSpell = (WarSpell[i][1]);
					else if (m_casterUnit->HasSpell(WarSpell[i][2]))
						CDSpell = (WarSpell[i][2]);
					else if (m_casterUnit->HasSpell(WarSpell[i][3]))
						CDSpell = (WarSpell[i][3]);

					if (CDSpell > 0)
					{
						m_casterUnit->AddSpellCooldown(CDSpell, 0, time(nullptr) + 6);
						if (Player* pPlayer = m_casterUnit->ToPlayer())
							pPlayer->SendSpellCooldown(CDSpell, 6000, pPlayer->GetObjectGuid());
					}
				}
			}
		}
		//vanilla+ Mortal Strike dmg buff when vs wounded targets
		if (((m_spellInfo->SpellIconID == 564)) && (m_spellInfo->IsFitToFamilyMask<CF_WARRIOR_MORTAL_STRIKE>()))
		{
			if (!unitTarget)
				return;


			if (m_casterUnit->HasAura(20497))
			{
				//Slam cooldown
				uint32 CDSpell = 0;

				if (m_casterUnit->HasSpell(34800))
					CDSpell = 34800;
				else if (m_casterUnit->HasSpell(11605))
					CDSpell = 11605;
				else if (m_casterUnit->HasSpell(11604))
					CDSpell = 11604;
				else if (m_casterUnit->HasSpell(8820))
					CDSpell = 8820;
				else if (m_casterUnit->HasSpell(1464))
					CDSpell = 1464;

				if (CDSpell > 0)
				{
					m_casterUnit->AddSpellCooldown(CDSpell, 0, time(nullptr) + 6);
					if (Player* pPlayer = m_casterUnit->ToPlayer())
						pPlayer->SendSpellCooldown(CDSpell, 6000, pPlayer->GetObjectGuid());
				}
			}

			// 1.2x dmg at <30%
			uint32 tarHPperc = uint32((float(unitTarget->GetHealth()) / unitTarget->GetMaxHealth()) * 100);
			float dmg_mod_final = 1.0f;
			/*float dmg_mod = 1.0f;
			dmg_mod = (100 - tarHPperc) / 4;
			dmg_mod_final = dmg_mod / 100 + 1;*/
			if (tarHPperc < 20)
				dmg_mod_final = 1.2f;

			weaponDamagePercentMod = dmg_mod_final;
		}
		break;
	}
	}

	int32 bonus = 0;
	for (int j = 0; j < MAX_EFFECT_INDEX; ++j)
	{
		switch (m_spellInfo->Effect[j])
		{
		case SPELL_EFFECT_WEAPON_DAMAGE:
		case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
			bonus += CalculateDamage(SpellEffectIndex(j), unitTarget);
			break;
		case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
			bonus += CalculateDamage(SpellEffectIndex(j), unitTarget);
			normalized = true;
			break;
		case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
			weaponDamagePercentMod *= float(CalculateDamage(SpellEffectIndex(j), unitTarget)) / 100.0f;

			// applied only to prev.effects fixed damage
			if (customBonusDamagePercentMod)
				bonus = int32(bonus * bonusDamagePercentMod);
			else
				bonus = int32(bonus * weaponDamagePercentMod);
			break;
		default:
			break;                                      // not weapon damage effect, just skip
		}
	}

	// apply to non-weapon bonus weapon total pct effect, weapon total flat effect included in weapon damage
	if (bonus)
	{
		UnitMods unitMod;
		switch (m_attackType)
		{
		default:
		case BASE_ATTACK:
			unitMod = UNIT_MOD_DAMAGE_MAINHAND;
			break;
		case OFF_ATTACK:
			unitMod = UNIT_MOD_DAMAGE_OFFHAND;
			break;
		case RANGED_ATTACK:
			unitMod = UNIT_MOD_DAMAGE_RANGED;
			break;
		}

		float weapon_total_pct = m_casterUnit->GetModifierValue(unitMod, TOTAL_PCT);
		bonus = int32(bonus * weapon_total_pct);
	}

	// + weapon damage with applied weapon% dmg to base weapon damage in call
	for (uint8 i = 0; i < m_casterUnit->GetWeaponDamageCount(m_attackType); i++)
	{
		if (unitTarget->IsImmuneToDamage(GetSchoolMask(m_casterUnit->GetWeaponDamageSchool(m_attackType, i))))
			continue;

		bonus += int32(m_casterUnit->CalculateDamage(m_attackType, normalized, i) * weaponDamagePercentMod);
	}

	// Seal of Command
	if (m_spellInfo->School == SPELL_SCHOOL_HOLY)
	{
		// Add spell gear bonus and spell modifiers
		bonus = m_casterUnit->SpellDamageBonusDone(unitTarget, m_spellInfo, bonus, SPELL_DIRECT_DAMAGE);
		bonus = unitTarget->SpellDamageBonusTaken(m_casterUnit, m_spellInfo, bonus, SPELL_DIRECT_DAMAGE);
	}

	// prevent negative damage
	m_damage += uint32(bonus > 0 ? bonus : 0);
}

void Spell::EffectThreat(SpellEffectIndex eff_idx)
{
	if (!unitTarget || !unitTarget->isAlive() || !m_casterUnit || !m_casterUnit->isAlive())
		return;

	if (!unitTarget->CanHaveThreatList())
		return;

	//Stoneclaw Totem casts on spawn
	if (m_spellInfo->SpellIconID == 689 && m_spellInfo->SpellVisual == 370)
	{
		if (m_spellInfo->Id == 5729 || m_spellInfo->Id == 6393 || m_spellInfo->Id == 6394 || m_spellInfo->Id == 6395 || m_spellInfo->Id == 10423 || m_spellInfo->Id == 10424)
		{
			if (!m_casterUnit->HasAura(35053))
			{
				if (!(m_casterUnit->HasAura(5728) || m_casterUnit->HasAura(6397) || m_casterUnit->HasAura(6398) || m_casterUnit->HasAura(6399) || m_casterUnit->HasAura(10425) || m_casterUnit->HasAura(10426)))
				{
					m_casterUnit->AddAura(35053);
					m_casterUnit->AddAura(35054);
					//m_casterUnit->CastSpell(m_casterUnit, 35055, true);	//Taunt
				}
			}
		}
	}

	//imp Distracting Shot
	if (m_spellInfo->SpellIconID == 1499 && m_spellInfo->SpellVisual == 5794)
	{
		if (m_casterUnit->HasAura(35820))
			m_casterUnit->CastSpell(unitTarget, 35822, true);
		if (m_casterUnit->HasAura(35821))
			m_casterUnit->CastSpell(unitTarget, 35823, true);
	}

	unitTarget->AddThreat(m_casterUnit, float(damage), false, m_spellInfo->GetSpellSchoolMask(), m_spellInfo);

	AddExecuteLogInfo(eff_idx, ExecuteLogInfo(unitTarget->GetObjectGuid()));
}

void Spell::EffectHealMaxHealth(SpellEffectIndex eff_idx)
{
	if (!unitTarget)
		return;
	if (!unitTarget->isAlive())
		return;

	uint32 heal = m_casterUnit ? m_casterUnit->GetMaxHealth() : unitTarget->GetMaxHealth();

	// Healing percent modifiers
	float  DoneTotalMod = 1.0f;
	float  TakenTotalMod = 1.0f;

	// Healing done percent
	if (m_casterUnit)
	{
		std::list <Aura*> const& mHealingDonePct = m_casterUnit->GetAurasByType(SPELL_AURA_MOD_HEALING_DONE_PERCENT);
		for (std::list <Aura*>::const_iterator i = mHealingDonePct.begin(); i != mHealingDonePct.end(); ++i)
			DoneTotalMod *= (100.0f + (*i)->GetModifier()->m_amount) / 100.0f;
	}

	heal *= DoneTotalMod;

	// Healing taken percent
	float minval = float(unitTarget->GetMaxNegativeAuraModifier(SPELL_AURA_MOD_HEALING_PCT));
	if (minval)
		TakenTotalMod *= (100.0f + minval) / 100.0f;

	float maxval = float(unitTarget->GetMaxPositiveAuraModifier(SPELL_AURA_MOD_HEALING_PCT));
	if (maxval)
		TakenTotalMod *= (100.0f + maxval) / 100.0f;

	heal *= TakenTotalMod;

#if SUPPORTED_CLIENT_BUILD <= CLIENT_BUILD_1_9_4
	ExecuteLogInfo info(unitTarget->GetObjectGuid());
	info.heal.amount = heal;
	info.heal.critical = 0;
	AddExecuteLogInfo(eff_idx, info);
#endif

	m_healing += heal;
}

void Spell::EffectInterruptCast(SpellEffectIndex eff_idx)
{
	if (!unitTarget)
		return;
	if (!unitTarget->isAlive())
		return;

	// TODO: not all spells that used this effect apply cooldown at school spells
	// also exist case: apply cooldown to interrupted cast only and to all spells
	for (uint32 i = CURRENT_FIRST_NON_MELEE_SPELL; i < CURRENT_MAX_SPELL; ++i)
	{
		if (Spell* spell = unitTarget->GetCurrentSpell(CurrentSpellTypes(i)))
		{
			// Nostalrius: fix CS of instant spells (with CC Delay)
			if (i != CURRENT_CHANNELED_SPELL && !spell->GetCastTime())
				continue;

			SpellEntry const* curSpellInfo = spell->m_spellInfo;
			// check if we can interrupt spell
			if ((spell->getState() == SPELL_STATE_CASTING
				|| (spell->getState() == SPELL_STATE_PREPARING && spell->GetCastTime() > 0.0f))
				&& curSpellInfo->PreventionType == SPELL_PREVENTION_TYPE_SILENCE
				&& ((i == CURRENT_GENERIC_SPELL && curSpellInfo->InterruptFlags & SPELL_INTERRUPT_FLAG_INTERRUPT)
					|| (i == CURRENT_CHANNELED_SPELL && curSpellInfo->ChannelInterruptFlags & CHANNEL_FLAG_INTERRUPT)))
			{
				unitTarget->ProhibitSpellSchool(curSpellInfo->GetSpellSchoolMask(), m_spellInfo->GetDuration());
				unitTarget->InterruptSpell(CurrentSpellTypes(i), false);

				//Prophecy 3/4 on interrupt
				if (unitTarget->HasAura(34573) && !unitTarget->HasAura(34575))
					unitTarget->CastSpell(unitTarget, 34574, true),
					unitTarget->CastSpell(unitTarget, 34575, true);

				// Duelist's Spellmantle
				if (m_casterUnit->HasAura(36304))
				{
					int32 ManaGain = m_casterUnit->GetMaxPower(POWER_MANA) / 100;
					m_casterUnit->CastCustomSpell(m_casterUnit, 36305, &ManaGain, nullptr, nullptr, true);
				}


				switch (m_spellInfo->Id)
				{
					case 35918:
					{
						if (m_caster)
						{
							if (SpellAuraHolder* holder = unitTarget->GetSpellAuraHolder(35929))
								holder->ModStackAmount(1);
							else
								unitTarget->AddAura(35929);
						}

						break;
					}
				}


				ExecuteLogInfo info(unitTarget->GetObjectGuid());
				info.interruptCast.spellId = curSpellInfo->Id;
				AddExecuteLogInfo(eff_idx, info);
			}
		}
	}
}

void Spell::EffectSummonObjectWild(SpellEffectIndex eff_idx)
{
	// TODO: Objects summoned here should probably be _removed from the map_ once their
	// duration has expired, rather than simply made invisible
	uint32 gameobject_id = m_spellInfo->EffectMiscValue[eff_idx];

	GameObject* pGameObj = new GameObject;

	WorldObject* target = focusObject;
	if (!target)
		target = m_caster;

	int32 duration = m_spellInfo->GetDuration();


	float x, y, z, o;
	if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
	{
		x = m_targets.m_destX;
		y = m_targets.m_destY;
		z = m_targets.m_destZ;
		o = target->GetOrientation();
	}
	else
	{
		m_caster->GetPosition(x, y, z);
		o = m_caster->GetOrientation();
	}


	switch (m_spellInfo->Id)
	{
	case 23363:
	{
		z -= 0.5f;
		break;
	}
	// Sapphirons summoned iceblocks have a duration *just* long enough to dissapear before the ice bomb.
	// Since they are despawned by another spell anyway, I make the gobj add slightly longer to avoid any lag
	// causing the gobj to despawn before the bomb goes off
	case 28535:
	{
		duration = 30000;
		break;
	}
	case 34978:	// Azsharite Land mine
	{
		if (Player* plcaster = (Player*)m_caster)
		{
			if (plcaster->GetTeam() == HORDE)
				gameobject_id = 302041;
			else
				gameobject_id = 302040;
		}

		break;
	}
	case 35143:	//5s fire spot Oil Barrels
	{
		z += frand(1.5, 2.5);
		break;
		/*if (m_spellInfo->Id == 35145)
			if (gameobject_id == 302101)
				z -= 1.0f;*/
	}
	case 36002:	//Pilar of Light
	{
		z -= 3.45f;
		m_casterUnit->CastSpell(m_casterUnit, 36003, true);
		m_casterUnit->SummonCreature(25253, m_casterUnit->GetPositionX(), m_casterUnit->GetPositionY(), m_casterUnit->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 120000);

		break;
	}
	case 36124:	// Duel flag SM Champion
	case 36269:	// Duel flag SM Trinket
	{	 x = (m_casterUnit->GetPositionX() + unitTarget->GetPositionX()) * 0.5f;
		 y = (m_casterUnit->GetPositionY() + unitTarget->GetPositionY()) * 0.5f;
		 z = m_casterUnit->GetPositionZ();

		 z = m_casterUnit->GetMap()->GetHeight(x, y, z, true);

		 if (m_spellInfo->Id == 36269)
			 m_casterUnit->CastSpell(unitTarget, 36270, true);

		break;
	}
	default:
		break;
	}

	Map* map = target->GetMap();

	if (!pGameObj->Create(map->GenerateLocalLowGuid(HIGHGUID_GAMEOBJECT), gameobject_id, map,
		x, y, z, o, 0.0f, 0.0f, 0.0f, 0.0f, GO_ANIMPROGRESS_DEFAULT, GO_STATE_READY))
	{
		delete pGameObj;
		return;
	}

	pGameObj->SetRespawnTime(duration > 0 ? duration / IN_MILLISECONDS : 0);
	pGameObj->SetSpellId(m_spellInfo->Id);

	// Wild object not have owner and check clickable by players
	map->Add(pGameObj);

	if (pGameObj->GetGoType() == GAMEOBJECT_TYPE_FLAGDROP && m_caster->GetTypeId() == TYPEID_PLAYER)
	{
		Player* pl = (Player*)m_caster;
		BattleGround* bg = ((Player*)m_caster)->GetBattleGround();

		switch (pGameObj->GetMapId())
		{
		case 489:                                       //WS
		{
			if (bg && bg->GetTypeID() == BATTLEGROUND_WS && bg->GetStatus() == STATUS_IN_PROGRESS)
			{
				Team team = ALLIANCE;
				if (pl->GetTeam() == team)
					team = HORDE;

				((BattleGroundWS*)bg)->SetDroppedFlagGuid(pGameObj->GetObjectGuid(), team);
			}
			break;
		}
		}
	}

	pGameObj->SetWorldMask(m_caster->GetWorldMask());
	pGameObj->SummonLinkedTrapIfAny();

	if (m_caster->GetTypeId() == TYPEID_UNIT && ((Creature*)m_caster)->AI())
		((Creature*)m_caster)->AI()->JustSummoned(pGameObj);

	AddExecuteLogInfo(eff_idx, ExecuteLogInfo(pGameObj->GetObjectGuid()));
}

void Spell::EffectScriptEffect(SpellEffectIndex eff_idx)
{
	switch (m_spellInfo->SpellFamilyName)
	{
	case SPELLFAMILY_GENERIC:
	{
		switch (m_spellInfo->Id)
		{
#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_10_2
		case 456: // SHOWLABEL Only OFF
		{
			if (Player* pPlayer = ToPlayer(m_caster))
				pPlayer->SetGMChat(false, true);
			return;
		}
		//MC Molten Ground
		case 34731:
		{
			m_caster->SummonCreature(25017, m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ(), m_caster->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 120000);
			return;
		}
		//MC Chronosphere
		case 34798:
		{
			m_caster->SummonCreature(25018, m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ(), m_caster->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 60000);
			return;
		}
		case 2765: // SHOWLABEL Only ON
		{
			if (Player* pPlayer = ToPlayer(m_caster))
				pPlayer->SetGMChat(true, true);;
			return;
		}
		case 1509: // GM Only OFF
		{
			if (Player* pPlayer = ToPlayer(m_caster))
				pPlayer->SetGameMaster(false, true);
			return;
		}
		case 18139: // GM Only ON
		{
			if (Player* pPlayer = ToPlayer(m_caster))
				pPlayer->SetGameMaster(true, true);
			return;
		}
		case 6147: // INVIS Only OFF
		{
			if (Player* pPlayer = ToPlayer(m_caster))
				pPlayer->SetGMVisible(true, true);
			return;
		}
		case 2763: // INVIS Only ON
		{
			if (Player* pPlayer = ToPlayer(m_caster))
				pPlayer->SetGMVisible(false, true);
			return;
		}
		case 20114: // BM Only OFF
		{
			if (Player* pPlayer = ToPlayer(m_caster))
				pPlayer->SetGodMode(false, true);
			return;
		}
		case 20115: // BM Only ON
		{
			if (Player* pPlayer = ToPlayer(m_caster))
				pPlayer->SetGodMode(true, true);
			return;
		}
		case 29313: // CooldownAll
		{
			if (m_casterUnit)
				m_casterUnit->RemoveAllSpellCooldown();
			return;
		}
#endif
		case 8856:                                  // Bending Shinbone
		{
			if (!itemTarget && m_caster->GetTypeId() != TYPEID_PLAYER)
				return;

			uint32 spell_id = 0;
			switch (urand(1, 5))
			{
			case 1:
				spell_id = 8854;
				break;
			default:
				spell_id = 8855;
				break;
			}

			m_casterUnit->CastSpell(m_casterUnit, spell_id, true, nullptr);
			return;
		}
		case 10101:                                 // Knock Away
		{
			if (!unitTarget || !m_casterUnit)
				return;

			m_casterUnit->getThreatManager().modifyThreatPercent(unitTarget, -100);

			return;
		}
		case 17512:                                 // Piccolo of the Flaming Fire
		{
			if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
				return;

			unitTarget->HandleEmoteCommand(EMOTE_STATE_DANCE);

			return;
		}
		// Styleen's Darling Shades
		case 35444:
		{
			if (!unitTarget || unitTarget->isInCombat() || unitTarget == m_casterUnit)
				return;

			// target must be OK to do this
			if (unitTarget->hasUnitState(UNIT_STAT_CAN_NOT_REACT))
				return;

			if (!unitTarget->HasAura(35445))
			{
				unitTarget->SetFacingTo(unitTarget->GetAngle(m_targets.m_destX, m_targets.m_destY));
				unitTarget->clearUnitState(UNIT_STAT_MOVING);

				if (unitTarget->GetTypeId() == TYPEID_UNIT)
					unitTarget->GetMotionMaster()->MoveDistract(15 * IN_MILLISECONDS);

				if (unitTarget->IsFriendlyTo(m_casterUnit))
				{
					int32 EmoteRand = urand(0, 6);
					uint32 EmoteId = EMOTE_ONESHOT_SALUTE;
					switch (EmoteRand)
					{
					case 0:
						EmoteId = EMOTE_ONESHOT_BOW;
						break;
					case 1:
						EmoteId = EMOTE_ONESHOT_WAVE;
						break;
					case 2:
						EmoteId = EMOTE_ONESHOT_CHEER;
						break;
					case 3:
						EmoteId = EMOTE_ONESHOT_EXCLAMATION;
						break;
					case 4:
						EmoteId = EMOTE_ONESHOT_APPLAUD;
						break;
					case 5:
						EmoteId = EMOTE_ONESHOT_POINT;
						break;
					case 6:
						EmoteId = EMOTE_ONESHOT_SALUTE;
						break;
					}

					unitTarget->HandleEmoteCommand(EmoteId);
				}
				else
				{
					unitTarget->HandleEmoteCommand(EMOTE_ONESHOT_EXCLAMATION);
				}

				unitTarget->AddAura(35445);
			}
			return;
		}
		case 22539:                                 // Shadow Flame (All script effects, not just end ones to
		case 22972:                                 // prevent player from dodging the last triggered spell)
		case 22975:
		case 22976:
		case 22977:
		case 22978:
		case 22979:
		case 22980:
		case 22981:
		case 22982:
		case 22983:
		case 22984:
		case 22985:
		{
			if (!unitTarget || !unitTarget->isAlive())
				return;

			// Onyxia Scale Cloak
			if (unitTarget->GetDummyAura(22683))
				return;

			// Shadow Flame
			m_caster->CastSpell(unitTarget, 22682, true);
			return;
		}
		case 24194:                                 // Uther's Tribute
		case 24195:                                 // Grom's Tribute
		{
			if (m_caster->GetTypeId() != TYPEID_PLAYER)
				return;

			uint8 race = m_casterUnit->getRace();
			uint32 spellId = 0;

			switch (m_spellInfo->Id)
			{
			case 24194:
				switch (race)
				{
				case RACE_HUMAN:
					spellId = 24105;
					break;
				case RACE_DWARF:
					spellId = 24107;
					break;
				case RACE_NIGHTELF:
					spellId = 24108;
					break;
				case RACE_GNOME:
					spellId = 24106;
					break;
				}
				break;
			case 24195:
				switch (race)
				{
				case RACE_ORC:
					spellId = 24104;
					break;
				case RACE_UNDEAD:
					spellId = 24103;
					break;
				case RACE_TAUREN:
					spellId = 24102;
					break;
				case RACE_TROLL:
					spellId = 24101;
					break;
				}
				break;
			}

			if (spellId)
				m_casterUnit->CastSpell(m_casterUnit, spellId, true);

			return;
		}
		case 24320:                                 // Poisonous Blood
		{
			unitTarget->CastSpell(unitTarget, 24321, true, nullptr, nullptr, m_caster->GetObjectGuid());
			return;
		}
		case 24324: // Nostalrius : Ivina (Hakkar)
		{
			enum
			{
				SPELL_BLOODSIPHON_STUN = 24324,
				SPELL_BLOODSIPHON_DAMAGE = 24323,
				SPELL_BLOODSIPHON_HEAL = 24322,
				SPELL_POISONOUS_BLOOD = 24321
			};
			if (!unitTarget || !unitTarget->isAlive() || !m_casterUnit)
				return;

			if (unitTarget->HasAura(SPELL_POISONOUS_BLOOD))
				unitTarget->CastSpell(m_casterUnit, SPELL_BLOODSIPHON_DAMAGE, true);
			else
				unitTarget->CastSpell(m_casterUnit, SPELL_BLOODSIPHON_HEAL, true);
			return;
		}
		case 24590:                                 // Brittle Armor - need remove one 24575 Brittle Armor aura
			unitTarget->RemoveAuraHolderFromStack(24575);
			return;
		case 24714:                                 // Trick
		{
			if (m_caster->GetTypeId() != TYPEID_PLAYER)
				return;

			bool gender = unitTarget->getGender();
			uint32 spellId = 0;
			uint32 spells[8] = {
				gender == GENDER_MALE ? 24708u : 24709u,   // Pirate
				gender == GENDER_MALE ? 24711u : 24710u,   // Ninja
				gender == GENDER_MALE ? 24712u : 24713u,   // Leper
				gender == GENDER_MALE ? 24735u : 24736u,   // Ghost
				24723u,                                    // Skeleton
				24732u,                                    // Bat
				24740u,                                    // Wisp
				24753u                                     // Critter
			};

			spellId = spells[urand(0, 7)];
			m_casterUnit->CastSpell(m_casterUnit, spellId, true);

			return;
		}
		case 24717:                                 // Pirate Costume
		{
			if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
				return;

			// Pirate Costume (male or female)
			m_caster->CastSpell(unitTarget, unitTarget->getGender() == GENDER_MALE ? 24708 : 24709, true);
			return;
		}
		case 24718:                                 // Ninja Costume
		{
			if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
				return;

			// Ninja Costume (male or female)
			m_caster->CastSpell(unitTarget, unitTarget->getGender() == GENDER_MALE ? 24711 : 24710, true);
			return;
		}
		case 24719:                                 // Leper Gnome Costume
		{
			if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
				return;

			// Leper Gnome Costume (male or female)
			m_caster->CastSpell(unitTarget, unitTarget->getGender() == GENDER_MALE ? 24712 : 24713, true);
			return;
		}
		case 24720:                                 // Random Costume
		{
			if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
				return;

			uint32 spellId = 0;

			switch (urand(0, 6))
			{
			case 0:
				spellId = unitTarget->getGender() == GENDER_MALE ? 24708 : 24709;
				break;
			case 1:
				spellId = unitTarget->getGender() == GENDER_MALE ? 24711 : 24710;
				break;
			case 2:
				spellId = unitTarget->getGender() == GENDER_MALE ? 24712 : 24713;
				break;
			case 3:
				spellId = 24723;
				break;
			case 4:
				spellId = 24732;
				break;
			case 5:
				spellId = unitTarget->getGender() == GENDER_MALE ? 24735 : 24736;
				break;
			case 6:
				spellId = 24740;
				break;
			}

			m_caster->CastSpell(unitTarget, spellId, true);
			return;
		}
		case 24737:                                 // Ghost Costume
		{
			if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
				return;

			// Ghost Costume (male or female)
			m_caster->CastSpell(unitTarget, unitTarget->getGender() == GENDER_MALE ? 24735 : 24736, true);
			return;
		}
		case 24751:                                 // Trick or Treat
		{
			if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
				return;

			// Tricked or Treated
			unitTarget->CastSpell(unitTarget, 24755, true);

			// Treat / Trick
			unitTarget->CastSpell(unitTarget, roll_chance_i(50) ? 24714 : 24715, true);
			return;
		}
		case 24936:                                 // RC Tank Control
		{
			if (Creature* pTonk = m_caster->FindNearestCreature(15328, 5, true))
			{
				pTonk->SetWalk(false);
				pTonk->m_spells[0] = 24933;
				pTonk->m_spells[1] = 25003;
				pTonk->m_spells[2] = 27746;
				switch (urand(0, 3))
				{
				case 0:
					pTonk->m_spells[3] = 25026;
					break;
				case 1:
					pTonk->m_spells[3] = 25027;
					break;
				case 2:
					pTonk->m_spells[3] = 27759;
					break;
				case 3:
					pTonk->m_spells[3] = 25024;
					break;
				}
				m_caster->CastSpell(pTonk, 24937, true);
			}
			return;
		}
		case 26004:                                 // Mistletoe
		{
			if (!unitTarget)
				return;

			unitTarget->HandleEmote(EMOTE_ONESHOT_CHEER);
			return;
		}
		case 26137:                                 // Rotate Trigger
		{
			if (!unitTarget)
				return;

			unitTarget->CastSpell(unitTarget, urand(0, 1) ? 26009 : 26136, true);
			return;
		}
		case 26218:                                 // Mistletoe
		{
			if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
				return;

			uint32 spells[2] = { 26206, 26207 };

			m_caster->CastSpell(unitTarget, spells[urand(0, 1)], true);
			return;
		}
		case 26275:                                 // PX-238 Winter Wondervolt TRAP
		{
			uint32 spells[4] = { 26272, 26157, 26273, 26274 };

			// check presence
			for (int j = 0; j < 4; ++j)
				if (unitTarget->HasAura(spells[j], EFFECT_INDEX_0))
					return;

			unitTarget->CastSpell(unitTarget, spells[urand(0, 3)], true);
			return;
		}
		//Elune's Blessing quest trib
		case 26393:
		{
			if (!unitTarget && unitTarget->GetTypeId() != TYPEID_PLAYER && ((Player*)unitTarget)->IsActiveQuest(8868))
				return;

			/*((Player*)unitTarget)->CompleteQuest(8868);*/
			unitTarget->CastSpell(unitTarget, 26394, true);
		}
		case 26465:                                 // Mercurial Shield - need remove one 26464 Mercurial Shield aura
			unitTarget->RemoveAuraHolderFromStack(26464);
			return;
		case 34782:                                 // BG Punishing dummy
		{
			if (Player* pPlayer = m_caster->ToPlayer())
			{
				if (pPlayer->InBattleGround())
					unitTarget->RemoveAuraHolderFromStack(34780);
			}
			return;
		}
		case 34858:
		{
			if (Player* pPlayer = m_caster->ToPlayer())
			{
				if (!pPlayer->InBattleGround())
					pPlayer->PunishTheAbuser();
			}
			return;
		}
		case 26656:                                 // Summon Black Qiraji Battle Tank
		{
			if (!unitTarget)
				return;

			// Prevent stacking of mounts
			unitTarget->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);

			const MapEntry* mapEntry = sMapStorage.LookupEntry<MapEntry>(unitTarget->GetMapId());
			if (!mapEntry)
				return;

			// Two separate mount spells depending on if mounting is allowed or not
			if (!mapEntry->IsMountAllowed())
				unitTarget->CastSpell(unitTarget, 25863, true, m_CastItem);
			else
				unitTarget->CastSpell(unitTarget, 26655, true, m_CastItem);

			return;
		}
		case 27687:                                 // Summon Bone Minions
		{
			if (!unitTarget)
				return;

			unitTarget->CastSpell(unitTarget, 27690, true);
			unitTarget->CastSpell(unitTarget, 27691, true);
			unitTarget->CastSpell(unitTarget, 27692, true);
			unitTarget->CastSpell(unitTarget, 27693, true);
			return;
		}
		case 27695:                                 // Summon Bone Mages
		{
			if (!unitTarget)
				return;

			unitTarget->CastSpell(unitTarget, 27696, true);
			unitTarget->CastSpell(unitTarget, 27697, true);
			unitTarget->CastSpell(unitTarget, 27698, true);
			unitTarget->CastSpell(unitTarget, 27699, true);
			return;
		}
		case 28374:                                 // Decimate (Naxxramas: Gluth)
		{
			//implemented in SpellEffects::EffectSchoolDMG  instead, under case 28375
			/*
				if (!unitTarget)
					return;
				// Should only affect players and zombies
				if (unitTarget->IsPlayer() || unitTarget->GetEntry() == 16360)
				{
					int32 damage = unitTarget->GetHealth() - unitTarget->GetMaxHealth() * 0.05f;
					if (damage > 0)
						m_caster->CastCustomSpell(unitTarget, 28375, &damage, nullptr, nullptr, true);
				}
			*/
			return;
		}
		case 28408:                                 // Chains of Kel'Thuzad (Naxxramas: Kel'thuzad)
		{
			if (!m_casterUnit)
				return;

			// Select maintank + 4 random targets
			std::vector<Unit*> viableTargets;
			const ThreatList& tl = m_casterUnit->getThreatManager().getThreatList();
			for (auto it = tl.begin(); it != tl.end(); it++)
			{
				if ((*it)->getUnitGuid().IsPlayer())
				{
					if (Unit* pUnit = m_casterUnit->GetMap()->GetUnit((*it)->getUnitGuid()))
					{
						if (pUnit->isAlive())
							viableTargets.push_back(pUnit);
					}
				}
			}

			int num_targets = std::min(int(viableTargets.size()), 5) - 1; // leaving 1 target not MCed to avoid reset due to all MCed

			// always MC maintank
			if (Unit* maintank = m_casterUnit->getVictim())
			{
				auto it = std::find(viableTargets.begin(), viableTargets.end(), maintank);
				if (it != viableTargets.end())
					viableTargets.erase(it);
				num_targets -= 1;
				maintank->CastSpell(maintank, 28409, true); // modifies scale
				m_casterUnit->CastSpell(maintank, 28410, true); // applies dmg and healing mod, as well as the charm itself
			}

			for (int i = 0; i < num_targets; i++)
			{
				int rand = irand(0, viableTargets.size() - 1);
				Unit* target = viableTargets[rand];
				viableTargets.erase(viableTargets.begin() + rand);

				target->CastSpell(target, 28409, true); // modifies scale
				m_casterUnit->CastSpell(target, 28410, true); // applies dmg and healing mod, as well as the charm itself
			}
			return;
		}
		case 28560:                                 // Summon Blizzard
		{
			if (!unitTarget)
				return;

			unitTarget->CastSpell(unitTarget, 28561, true, nullptr, nullptr, m_caster->GetObjectGuid());
			return;
		}
		case 30918:                                 // Improved Sprint
		{
			if (!unitTarget)
				return;

			// Removes snares and roots.
			unitTarget->RemoveAurasAtMechanicImmunity(IMMUNE_TO_ROOT_AND_SNARE_MASK, 30918, true);
			break;
		}
		case 29182:                                 // Teleport to Twin Emperors
		{
			if (unitTarget && unitTarget->IsPlayer())
				unitTarget->CastSpell(unitTarget, 29181, true);
			return;
		}
		case 29188:                                 // Teleport to Final Chamber
		{
			if (unitTarget && unitTarget->IsPlayer())
				unitTarget->CastSpell(unitTarget, 29190, true);
			return;
		}
		case 30132:                                 // Despawn Ice Block
		{
			if (unitTarget && unitTarget->IsPlayer())
			{
				unitTarget->RemoveAurasDueToSpell(31800); // Icebolt immunity spell
				unitTarget->RemoveAurasDueToSpell(28522); // Icebolt stun/damage spell
			}
			return;
		}
		case 28352:                                    // Atiesh - Breath of Sargeras
		{
			if (unitTarget && m_caster)
				m_caster->CastSpell(unitTarget, 28342, true);
			return;
		}
		case 26532:                                 // Winter Veil summons
		case 26541:
		case 26469:
		case 26528:
		{
			if (Player* player = m_caster->ToPlayer())
			{
				// Remove minipet without consuming a snowball
				if (player->GetMiniPet())
				{
					player->RemoveMiniPet();
					return;
				}
			}
			break;
		}
		case 26678:                                 // Bag of Candies
		{
			uint32 candySpells[8] = { 26668, 26670, 26671, 26672, 26673, 26674, 26675, 26676 };
			if (m_casterUnit)
				m_casterUnit->CastSpell(m_casterUnit, candySpells[urand(0, 7)], true);
			return;
		}
		case 27657:                                 // Valentine End Check
		{
			if (unitTarget && !sGameEventMgr.IsActiveEvent(8))
			{
				unitTarget->RemoveAurasDueToSpell(26869);
				unitTarget->RemoveAurasDueToSpell(27741);
			}
			return;
		}
		case 27686:                                 // Teleport to Razelikh (GROUP)
		{
			if (Player* pPlayerTarget = ToPlayer(unitTarget))
			{
				if (Group* pGroup = pPlayerTarget->GetGroup())
				{
					uint32 zone_id = pPlayerTarget->GetCachedZoneId();
					for (GroupReference* itr = pGroup->GetFirstMember(); itr != nullptr; itr = itr->next())
					{
						Player* pGroupMember = itr->getSource();
						if (pGroupMember && pGroupMember->IsWithinDistInMap(m_caster, 20.0f))
							pGroupMember->CastSpell(pGroupMember, 13142, true); // Teleport to Razelikh
					}
				}
			}
			return;
		}
		}
		break;
	}
	case SPELLFAMILY_WARLOCK:
	{
		switch (m_spellInfo->Id)
		{
		case  6201:                                 // Healthstone creating spells
		case  6202:
		case  5699:
		case 11729:
		case 11730:
		case 27230:
		{
			if (!unitTarget)
				return;

			uint32 itemtype;
			uint32 rank = 0;
			Unit::AuraList const& mDummyAuras = unitTarget->GetAurasByType(SPELL_AURA_DUMMY);
			for (Unit::AuraList::const_iterator i = mDummyAuras.begin(); i != mDummyAuras.end(); ++i)
			{
				if ((*i)->GetId() == 18692)
				{
					rank = 1;
					break;
				}
				else if ((*i)->GetId() == 18693)
				{
					rank = 2;
					break;
				}
			}

			static uint32 const itypes[6][3] =
			{
				{ 5512, 19004, 19005},              // Minor Healthstone
				{ 5511, 19006, 19007},              // Lesser Healthstone
				{ 5509, 19008, 19009},              // Healthstone
				{ 5510, 19010, 19011},              // Greater Healthstone
				{ 9421, 19012, 19013},              // Major Healthstone
				{22103, 22104, 22105}               // Master Healthstone
			};

			switch (m_spellInfo->Id)
			{
			case  6201:
				itemtype = itypes[0][rank];
				break; // Minor Healthstone
			case  6202:
				itemtype = itypes[1][rank];
				break; // Lesser Healthstone
			case  5699:
				itemtype = itypes[2][rank];
				break; // Healthstone
			case 11729:
				itemtype = itypes[3][rank];
				break; // Greater Healthstone
			case 11730:
				itemtype = itypes[4][rank];
				break; // Major Healthstone
			case 27230:
				itemtype = itypes[5][rank];
				break; // Master Healthstone
			default:
				return;
			}
			DoCreateItem(eff_idx, itemtype);
			return;
		}
		}
		break;
	}
	case SPELLFAMILY_DRUID:
	{
		// Shapeshift Form Effect
		if (m_spellInfo->Id == 9033)
		{
			// remove movement affects
			Unit* target = unitTarget ? unitTarget : m_casterUnit;
			if (!target)
				return;
			if (target->HasAura(20184))
				target->RemoveAurasDueToSpell(20184);
			target->RemoveSpellsCausingAura(SPELL_AURA_MOD_ROOT);
			Unit::AuraList const& slowingAuras = target->GetAurasByType(SPELL_AURA_MOD_DECREASE_SPEED);
			for (Unit::AuraList::const_iterator iter = slowingAuras.begin(); iter != slowingAuras.end();)
			{
				SpellEntry const* aurSpellInfo = (*iter)->GetSpellProto();

				uint32 aurMechMask = aurSpellInfo->GetAllSpellMechanicMask();

				// If spell that caused this aura has Croud Control or Daze effect
				if ((aurMechMask & MECHANIC_NOT_REMOVED_BY_SHAPESHIFT) ||
					// some Daze spells have these parameters instead of MECHANIC_DAZE (skip snare spells)
					(aurSpellInfo->SpellIconID == 15 && aurSpellInfo->Dispel == 0 &&
						(aurMechMask & (1 << (MECHANIC_SNARE - 1))) == 0))
				{
					++iter;
					continue;
				}

				// All OK, remove aura now
				target->RemoveAurasDueToSpellByCancel(aurSpellInfo->Id);
				iter = slowingAuras.begin();
			}
			return;
		}
		break;
	}
	case SPELLFAMILY_PALADIN:
	{
		// Flash of Light
		if (m_spellInfo->SpellIconID == 242)
		{
			if (!unitTarget || !unitTarget->isAlive())
				return;
			int32 heal = damage;

			int32 spellid = m_spellInfo->Id;            // send main spell id as basepoints for not used effect
			m_caster->CastCustomSpell(unitTarget, 19993, &heal, &spellid, nullptr, true);
		}
		else if (m_spellInfo->IsFitToFamilyMask<CF_PALADIN_JUDGEMENT>())
		{
			if (!unitTarget || !unitTarget->isAlive() || !m_casterUnit)
				return;
			uint32 spellId2 = 0;

			// all seals have aura dummy
			Unit::AuraList const& m_dummyAuras = m_casterUnit->GetAurasByType(SPELL_AURA_DUMMY);
			for (Unit::AuraList::const_iterator itr = m_dummyAuras.begin(); itr != m_dummyAuras.end(); ++itr)
			{
				SpellEntry const* spellInfo = (*itr)->GetSpellProto();

				// search seal (all seals have judgement's aura dummy spell id in 2 effect
				if (!spellInfo || !(*itr)->GetSpellProto()->IsSealSpell() || (*itr)->GetEffIndex() != 2)
					continue;

				// must be calculated base at raw base points in spell proto, GetModifier()->m_value for S.Righteousness modified by SPELLMOD_DAMAGE
				spellId2 = (*itr)->GetSpellProto()->CalculateSimpleValue(EFFECT_INDEX_2);

				if (spellId2 <= 1)
					continue;

				// found, remove seal
				m_casterUnit->RemoveAurasDueToSpell((*itr)->GetId());
				break;
			}

			m_caster->CastSpell(unitTarget, spellId2, true);
			// Judgement Armor setbonus
			if (m_casterUnit->HasAura(23591))
				m_caster->CastSpell(unitTarget, 23590, true);

			// Libram of Vengeance
			if (m_casterUnit->HasAura(35349))
				m_caster->CastSpell(m_casterUnit, 35350, true);

			//Judgement of Justice interrupt
			if (spellId2 == 20184)
				m_caster->CastSpell(unitTarget, 34799, true);
			return;
		}
		// Seal of Fury proc
		else if (m_spellInfo->SpellFamilyFlags == 4096)
		{
			if (!unitTarget || !unitTarget->CanHaveThreatList() || !m_casterUnit)
				return;

			if (unitTarget->getThreatManager().getThreat(m_casterUnit))
				unitTarget->getThreatManager().addThreat(m_casterUnit, damage * m_casterUnit->GetAttackTime(BASE_ATTACK) / 1000);
		}
		break;
	}
	case SPELLFAMILY_SHAMAN:
	{
		switch (m_spellInfo->Id)
		{
		case  20860:    // Rockbiter Weapon Proc
		case  20865:
		case  20866:
		case  20867:
		case  20868:
		case  20870:
		case  20871:
		{
			if (!unitTarget || !unitTarget->CanHaveThreatList() || !m_casterUnit)
				return;

			if (m_casterUnit->haveShield())
				unitTarget->getThreatManager().addThreat(m_casterUnit, damage * m_casterUnit->GetAttackTime(BASE_ATTACK) / 1000);
		}
		}
		break;
	}
	}

	// normal DB scripted effect
	if (!unitTarget)
		return;

	DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Spell ScriptStart spellid %u in EffectScriptEffect ", m_spellInfo->Id);
	m_caster->GetMap()->ScriptsStart(sSpellScripts, m_spellInfo->Id, m_caster, unitTarget);
}

void Spell::EffectSanctuary(SpellEffectIndex eff_idx)
{
	if (!unitTarget)
		return;

	// World of Warcraft Client Patch 1.12.0 (2006-08-22)
	// - Neutral guards are now able to see through the rogue Vanish ability.
	bool guard_check = m_spellInfo->IsFitToFamily<SPELLFAMILY_ROGUE, CF_ROGUE_VANISH>() && (sWorld.GetWowPatch() >= WOW_PATCH_112);
	bool no_guards = true;

	unitTarget->InterruptSpellsCastedOnMe(true);
	unitTarget->CombatStop();

	HostileReference* pReference = unitTarget->getHostileRefManager().getFirst();

	while (pReference)
	{
		HostileReference* pNextRef = pReference->next();
		if (!guard_check || !pReference->getSource()->getOwner()->IsContestedGuard())
		{
			pReference->removeReference();
			delete pReference;
		}
		else
			no_guards = false;

		pReference = pNextRef;
	}

	unitTarget->m_lastSanctuaryTime = WorldTimer::getMSTime();

	// Vanish allows to remove all threat and cast regular stealth so other spells can be used
	if (m_spellInfo->IsFitToFamily<SPELLFAMILY_ROGUE, CF_ROGUE_VANISH>())
	{
		unitTarget->RemoveSpellsCausingAura(SPELL_AURA_MOD_ROOT);
		unitTarget->InterruptAttacksOnMe(0.0f, guard_check);

		if (Player* pPlayer = m_caster->ToPlayer())
		{
			if (no_guards)
				pPlayer->SetCannotBeDetectedTimer(1000);
		}
	}

	AddExecuteLogInfo(eff_idx, ExecuteLogInfo(unitTarget->GetObjectGuid()));
}

void Spell::EffectAddComboPoints(SpellEffectIndex /*eff_idx*/)
{
	if (!unitTarget)
		return;

	if (m_caster->GetTypeId() != TYPEID_PLAYER)
		return;

	if (damage <= 0)
		return;

	((Player*)m_caster)->AddComboPoints(unitTarget, damage);
	((Player*)m_caster)->SetUInt64Value(PLAYER_FIELD_COMBO_TARGET, unitTarget->GetGUID());
}

void Spell::EffectDuel(SpellEffectIndex eff_idx)
{
	if (!m_casterUnit || !unitTarget || !m_casterUnit->IsPlayer() || !unitTarget->IsPlayer())
		return;

	Player* caster = (Player*)m_casterUnit;
	Player* target = (Player*)unitTarget;

	// if the caster is already in a duel or has issued a challenge
	if (caster->duel && caster->duel->opponent != target)
	{
		if (caster->duel->startTime)
			caster->DuelComplete(DUEL_WON);
		else
			caster->DuelComplete(DUEL_INTERUPTED);

		delete caster->duel;
		delete target->duel;
		caster->duel = target->duel = nullptr;
	}

	// if the caster attempts to duel somebody they're already in a duel with
	if (caster->duel && caster->duel->opponent == target && caster->duel->startTime)
	{
		SendCastResult(SPELL_FAILED_TARGET_ENEMY);
		return;
	}

	// if the target already has a pending duel/is dueling, reject the request
	if (target->duel)
	{
		SendCastResult(SPELL_FAILED_TARGET_DUELING);
		return;
	}

	// caster or target already have requested duel
	if (caster->duel || !target->GetSocial() || target->GetSocial()->HasIgnore(caster->GetObjectGuid()) || target->FindMap() != caster->FindMap())
		return;

	// Players can only fight a duel with each other outside (=not inside dungeons and not in capital cities)
	const auto* casterAreaEntry = AreaEntry::GetById(caster->GetAreaId());
	if (casterAreaEntry && !(casterAreaEntry->Flags & AREA_FLAG_DUEL))
	{
		SendCastResult(SPELL_FAILED_NO_DUELING);            // Dueling isn't allowed here
		return;
	}

	const auto* targetAreaEntry = AreaEntry::GetById(target->GetAreaId());
	if (targetAreaEntry && !(targetAreaEntry->Flags & AREA_FLAG_DUEL))
	{
		SendCastResult(SPELL_FAILED_NO_DUELING);            // Dueling isn't allowed here
		return;
	}

	//CREATE DUEL FLAG OBJECT
	GameObject* pGameObj = new GameObject;

	uint32 gameobject_id = m_spellInfo->EffectMiscValue[eff_idx];

	Map* map = m_casterUnit->GetMap();
	float x = (m_casterUnit->GetPositionX() + unitTarget->GetPositionX()) * 0.5f;
	float y = (m_casterUnit->GetPositionY() + unitTarget->GetPositionY()) * 0.5f;
	float z = m_casterUnit->GetPositionZ();

	if (!pGameObj->Create(map->GenerateLocalLowGuid(HIGHGUID_GAMEOBJECT), gameobject_id, map, x, y, z,
		m_casterUnit->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, GO_ANIMPROGRESS_DEFAULT, GO_STATE_READY))
	{
		delete pGameObj;
		return;
	}

	pGameObj->SetUInt32Value(GAMEOBJECT_FACTION, m_casterUnit->getFaction());
	pGameObj->SetUInt32Value(GAMEOBJECT_LEVEL, m_casterUnit->getLevel() + 1);
	int32 duration = m_spellInfo->GetDuration();
	pGameObj->SetRespawnTime(duration > 0 ? duration / IN_MILLISECONDS : 0);
	pGameObj->SetSpellId(m_spellInfo->Id);

	m_casterUnit->AddGameObject(pGameObj);
	map->Add(pGameObj);
	//END

	// Send request
	WorldPacket data(SMSG_DUEL_REQUESTED, 8 + 8);
	data << pGameObj->GetObjectGuid();
	data << caster->GetObjectGuid();
	caster->GetSession()->SendPacket(&data);
	target->GetSession()->SendPacket(&data);

	// create duel-info
	DuelInfo* duel = new DuelInfo;
	duel->initiator = caster;
	duel->opponent = target;
	duel->startTime = 0;
	duel->startTimer = 0;

	DuelInfo* duel2 = new DuelInfo;
	duel2->initiator = caster;
	duel2->opponent = caster;
	duel2->startTime = 0;
	duel2->startTimer = 0;

	if (Transport* t = caster->GetTransport())
	{
		duel->transportGuid = t->GetGUIDLow();
		duel2->transportGuid = t->GetGUIDLow();
	}
	caster->duel = duel;
	target->duel = duel2;

	caster->SetGuidValue(PLAYER_DUEL_ARBITER, pGameObj->GetObjectGuid());
	target->SetGuidValue(PLAYER_DUEL_ARBITER, pGameObj->GetObjectGuid());
}

void Spell::EffectStuck(SpellEffectIndex /*eff_idx*/)
{
	if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
		return;

	if (!sWorld.getConfig(CONFIG_BOOL_CAST_UNSTUCK))
		return;

	Player* pTarget = (Player*)unitTarget;

	DEBUG_LOG("Spell Effect: Stuck");
	sLog.outInfo("Player %s (guid %u) used auto-unstuck feature at map %u (%f, %f, %f).", pTarget->GetName(), pTarget->GetGUIDLow(), m_caster->GetMapId(), m_caster->GetPositionX(), pTarget->GetPositionY(), pTarget->GetPositionZ());

	if (pTarget->IsTaxiFlying())
		return;

	// TP to last overmap position
	if (fabs(pTarget->_lastSafeX) > 0.1f && fabs(pTarget->_lastSafeY) > 0.1f)
		pTarget->TeleportTo(pTarget->GetMapId(), pTarget->_lastSafeX, pTarget->_lastSafeY, pTarget->_lastSafeZ - 2.0f + 0.7f, 0.0f);
}

void Spell::EffectSummonPlayer(SpellEffectIndex /*eff_idx*/)
{
	if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
		return;

	uint32 warlocklevel = m_caster->getLevel();
	uint32 targetlevel = unitTarget->getLevel();


	// Evil Twin (ignore player summon, but hide this for summoner)
	if (unitTarget->GetDummyAura(23445))
		return;

	//Ritul of Summon(and other summons) can't summon high lvl target as low lvl character
	if ((targetlevel - 10) > warlocklevel)
	{
		SendCastResult(SPELL_FAILED_HIGHLEVEL);
		return;
	}

	float x, y, z;
	WorldObject* landingObject = m_caster;
	// summon to the ritual go location if any
	if (GameObject* pGo = m_targets.getGOTarget())
		if (pGo->GetGoType() == GAMEOBJECT_TYPE_SUMMONING_RITUAL)
			landingObject = pGo;

	landingObject->GetClosePoint(x, y, z, unitTarget->GetObjectBoundingRadius());

	((Player*)unitTarget)->SetSummonPoint(m_caster->GetMapId(), x, y, z);

	WorldPacket data(SMSG_SUMMON_REQUEST, 8 + 4 + 4);
	data << m_caster->GetObjectGuid();                      // summoner guid
	data << uint32(m_caster->GetZoneId());                  // summoner zone
	data << uint32(MAX_PLAYER_SUMMON_DELAY * IN_MILLISECONDS); // auto decline after msecs
	((Player*)unitTarget)->GetSession()->SendPacket(&data);
}

void Spell::EffectActivateObject(SpellEffectIndex eff_idx)
{
	if (!gameObjTarget)
		return;

	GameObjectActions action = (GameObjectActions)m_spellInfo->EffectMiscValue[eff_idx];

	switch (action)
	{
	case GameObjectActions::None:
		sLog.outError("Spell::EffectActivateObject: Incorrect GameObjectActions::None action in spell %u", m_spellInfo->Id);
		break;
	case GameObjectActions::AnimateCustom0:
		gameObjTarget->SendGameObjectCustomAnim(0);
		break;
	case GameObjectActions::AnimateCustom1:
		gameObjTarget->SendGameObjectCustomAnim(1);
		break;
	case GameObjectActions::AnimateCustom2:
		gameObjTarget->SendGameObjectCustomAnim(2);
		break;
	case GameObjectActions::AnimateCustom3:
		gameObjTarget->SendGameObjectCustomAnim(3);
		break;
	case GameObjectActions::Disturb:
		if (m_casterUnit)
			gameObjTarget->Use(m_casterUnit);
		break;
	case GameObjectActions::Unlock:
		gameObjTarget->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_LOCKED);
		break;
	case GameObjectActions::Lock:
		gameObjTarget->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_LOCKED);
		break;
	case GameObjectActions::Open:
		if (m_casterUnit)
			gameObjTarget->Use(m_casterUnit);
		/* Seems like Open can also trigger traps, cause linked trap to trigger, etc... go figure...
		gameObjTarget->UseDoorOrButton(0, false, m_caster);
		*/
		break;
	case GameObjectActions::OpenAndUnlock:
		gameObjTarget->UseDoorOrButton(0, false);
		gameObjTarget->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_LOCKED);
		break;
	case GameObjectActions::Close:
		gameObjTarget->ResetDoorOrButton();
		break;
	case GameObjectActions::ToggleOpen:
		// No use cases, implementation unknown
		break;
	case GameObjectActions::Destroy:
		gameObjTarget->UseDoorOrButton(0, true);
		break;
	case GameObjectActions::Rebuild:
		gameObjTarget->ResetDoorOrButton();
		break;
	case GameObjectActions::Creation:
		// No use cases, implementation unknown
		break;
	case GameObjectActions::Despawn:
		gameObjTarget->Despawn();
		gameObjTarget->SetLootState(GO_JUST_DEACTIVATED);
		break;
	case GameObjectActions::MakeInert:
		gameObjTarget->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NO_INTERACT);
		break;
	case GameObjectActions::MakeActive:
		gameObjTarget->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NO_INTERACT);
		break;
	case GameObjectActions::CloseAndLock:
		gameObjTarget->ResetDoorOrButton();
		gameObjTarget->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_LOCKED);
		break;
	case GameObjectActions::UseArtKit0:
		// Source for values unknown
		/*
		switch (m_spellInfo->Id)
		{
			case 46904: // Light Bonfire (Art Kit)
				gameObjTarget->SetGoArtKit(121);
				break;
		}
		*/
		break;
	case GameObjectActions::UseArtKit1:
		// Source for values unknown
		/*
		switch (m_spellInfo->Id)
		{
			case 36639: // Test Cauldron Bubble
				gameObjTarget->SetGoArtKit(81);
				break;
			case 46903: // Stamp Out Bonfire (Art Kit)
				gameObjTarget->SetGoArtKit(122);
				break;
		}
		*/
		break;
	case GameObjectActions::UseArtKit2:
		// No use cases, implementation unknown
		break;
	case GameObjectActions::UseArtKit3:
		// No use cases, implementation unknown
		break;
	case GameObjectActions::SetTapList:
		// No use cases, implementation unknown
		break;
	default:
		sLog.outError("Spell::EffectActivateObject: Unhandled GameObjectActions action %u in spell %u", (uint32)action, m_spellInfo->Id);
		break;
	}
}

void Spell::EffectSummonTotem(SpellEffectIndex eff_idx)
{
	if (!m_casterUnit)
		return;

	int slot = 0;
	switch (m_spellInfo->Effect[eff_idx])
	{
	case SPELL_EFFECT_SUMMON_TOTEM:
		slot = TOTEM_SLOT_NONE;
		break;
	case SPELL_EFFECT_SUMMON_TOTEM_SLOT1:
		slot = TOTEM_SLOT_FIRE;
		break;
	case SPELL_EFFECT_SUMMON_TOTEM_SLOT2:
		slot = TOTEM_SLOT_EARTH;
		break;
	case SPELL_EFFECT_SUMMON_TOTEM_SLOT3:
		slot = TOTEM_SLOT_WATER;
		break;
	case SPELL_EFFECT_SUMMON_TOTEM_SLOT4:
		slot = TOTEM_SLOT_AIR;
		break;
	default:
		return;
	}

	// unsummon old totem
	if (slot < MAX_TOTEM_SLOT)
		if (Totem* OldTotem = m_casterUnit->GetTotem(TotemSlot(slot)))
			OldTotem->UnSummon();

	// FIXME: Setup near to finish point because GetObjectBoundingRadius set in Create but some Create calls can be dependent from proper position
	// if totem have creature_template_addon.auras with persistent point for example or script call
	float angle = slot < MAX_TOTEM_SLOT ? M_PI_F / MAX_TOTEM_SLOT - (slot * 2 * M_PI_F / MAX_TOTEM_SLOT) : 0;
	float distance = 2.0f;

	if (m_spellInfo->Id == 35036 || m_spellInfo->Id == 35042)
		distance = 0.0f;

	CreatureCreatePos pos(m_casterUnit, m_casterUnit->GetOrientation(), distance, angle);

	CreatureInfo const* cinfo = ObjectMgr::GetCreatureTemplate(m_spellInfo->EffectMiscValue[eff_idx]);
	if (!cinfo)
	{
		sLog.outErrorDb("Creature entry %u does not exist but used in spell %u totem summon.", m_spellInfo->EffectMiscValue[eff_idx], m_spellInfo->Id);
		return;
	}

	Totem* pTotem = new Totem;

	if (!pTotem->Create(m_casterUnit->GetMap()->GenerateLocalLowGuid(HIGHGUID_UNIT), pos, cinfo, m_casterUnit))
	{
		delete pTotem;
		return;
	}

	pTotem->SetSummonPoint(pos);

	if (slot < MAX_TOTEM_SLOT)
		m_casterUnit->_AddTotem(TotemSlot(slot), pTotem);

	//pTotem->SetName("");                                  // generated by client
	pTotem->SetOwner(m_casterUnit);
	pTotem->SetTypeBySummonSpell(m_spellInfo);              // must be after Create call where m_spells initialized

	pTotem->SetDuration(m_duration);

	if (damage)                                             // if not spell info, DB values used
	{
		pTotem->SetMaxHealth(damage);
		pTotem->SetHealth(damage);
	}

	pTotem->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->Id);

	if (m_casterUnit->GetTypeId() == TYPEID_PLAYER)
		pTotem->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);

	if (m_casterUnit->IsPvP())
		pTotem->SetPvP(true);

	pTotem->Summon(m_casterUnit);

	AddExecuteLogInfo(eff_idx, ExecuteLogInfo(pTotem->GetObjectGuid()));
}

void Spell::EffectEnchantHeldItem(SpellEffectIndex eff_idx)
{
	// this is only item spell effect applied to main-hand weapon of target player (players in area)
	if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
		return;

	Player* item_owner = (Player*)unitTarget;
	Item* item = item_owner->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);

	if (!item)
		return;

	// must be equipped
	if (!item->IsEquipped())
		return;

	// Nostalrius (INTERFACTION) : Totem furie-des-vents ecrase les benes de puissance et des rois Paladin.
	//if (m_spellInfo->SpellFamilyName == SPELLFAMILY_SHAMAN)
	//{
	//	Unit::AuraList const& auras = unitTarget->GetAurasByType(SPELL_AURA_MOD_ATTACK_POWER);
	//	for (Unit::AuraList::const_iterator i = auras.begin(); i != auras.end(); ++i)
	//	{
	//		if ((*i)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_PALADIN &&
	//			// Bene de puissance inf et sup
	//			((*i)->GetSpellProto()->SpellIconID == 298 || (*i)->GetSpellProto()->SpellIconID == 1802))
	//		{
	//			unitTarget->RemoveAurasDueToSpell((*i)->GetSpellProto()->Id);
	//			break; // Normalement on ne peut avoir qu'une seule bene de ce type.
	//		}
	//	}
	//	Unit::AuraList const& auras2 = unitTarget->GetAurasByType(SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE);
	//	for (Unit::AuraList::const_iterator i = auras2.begin(); i != auras2.end(); ++i)
	//	{
	//		if ((*i)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_PALADIN &&
	//			// Bene des rois inf et sup
	//			((*i)->GetSpellProto()->Id == 20217 || (*i)->GetSpellProto()->Id == 25898))
	//		{
	//			unitTarget->RemoveAurasDueToSpell((*i)->GetSpellProto()->Id);
	//			break; // Normalement on ne peut avoir qu'une seule bene de ce type.
	//		}
	//	}
	//}

	if (m_spellInfo->EffectMiscValue[eff_idx])
	{
		uint32 enchant_id = m_spellInfo->EffectMiscValue[eff_idx];

		// Base points first
		int32 duration = m_currentBasePoints[eff_idx] * IN_MILLISECONDS;

		// Try duration index next
		if (!duration)
			duration = m_spellInfo->GetDuration();

		// 10 seconds for enchants which don't have listed duration
		if (!duration)
			duration = 10 * IN_MILLISECONDS;

		SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
		if (!pEnchant)
			return;

		uint32 charges = sSpellMgr.GetSpellEnchantCharges(m_spellInfo->Id);

		// Always go to temp enchantment slot
		EnchantmentSlot slot = TEMP_ENCHANTMENT_SLOT;

		// Enchantment will not be applied if a different one already exists
		if (item->GetEnchantmentId(slot) && item->GetEnchantmentId(slot) != enchant_id)
			return;

		// Apply the temporary enchantment
		item->SetEnchantment(slot, enchant_id, duration, charges);
		item_owner->ApplyEnchantment(item, slot, true);
	}
}

void Spell::EffectDisEnchant(SpellEffectIndex /*eff_idx*/)
{
	if (m_caster->GetTypeId() != TYPEID_PLAYER)
		return;

	if (!itemTarget || !itemTarget->GetProto()->DisenchantID)
		return;

	Player* pCaster = static_cast<Player*>(m_caster);

	pCaster->UpdateCraftSkill(m_spellInfo->Id);
	pCaster->SendLoot(itemTarget->GetObjectGuid(), LOOT_DISENCHANTING);

	// quick solution to prevent exploiting; this will cause items loss on full bags though
	// in order to fix this correctly have to fix first plMover->SendLootRelease(lootGuid); @ MovementHandler
	// and in order to fix it the loot manager has to be rewritten; not me pls
	if (ObjectGuid lootGuid = pCaster->GetLootGuid())
		pCaster->GetSession()->DoLootRelease(lootGuid);

	// item will be removed at disenchanting end
}

void Spell::EffectInebriate(SpellEffectIndex /*eff_idx*/)
{
	if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
		return;

	Player* player = (Player*)unitTarget;
	uint16 currentDrunk = player->GetDrunkValue();
	uint16 drunkMod = damage * 256;
	if (currentDrunk + drunkMod > 0xFFFF)
		currentDrunk = 0xFFFF;
	else
		currentDrunk += drunkMod;
	player->SetDrunkValue(currentDrunk, m_CastItem ? m_CastItem->GetEntry() : 0);
}

void Spell::EffectFeedPet(SpellEffectIndex eff_idx)
{
	Player* pPlayer = m_caster->ToPlayer();
	if (!pPlayer)
		return;

	Item* foodItem = itemTarget;
	if (!foodItem)
		return;

	Pet* pet = pPlayer->GetPet();
	if (!pet)
		return;

	if (!pet->isAlive())
		return;

	if (!pet->IsWithinLOSInMap(pPlayer))
	{
		SendCastResult(SPELL_FAILED_LINE_OF_SIGHT);
		return;
	}

	int32 benefit = pet->GetCurrentFoodBenefitLevel(foodItem->GetProto()->ItemLevel);
	if (benefit <= 0)
		return;

	ExecuteLogInfo info;
	info.feedPet.itemEntry = foodItem->GetProto()->ItemId;

	uint32 count = 1;
	pPlayer->DestroyItemCount(foodItem, count, true);
	pPlayer->CastCustomSpell(pPlayer, m_spellInfo->EffectTriggerSpell[eff_idx], &benefit, nullptr, nullptr, true);

	AddExecuteLogInfo(eff_idx, info);
}

void Spell::EffectDismissPet(SpellEffectIndex eff_idx)
{
	Player* pPlayer = m_caster->ToPlayer();
	if (!pPlayer)
		return;

	Pet* pet = pPlayer->GetPet();

	// not let dismiss dead pet
	if (!pet || !pet->isAlive())
		return;

	//Bestial Wrath/Intimidation dispell on dismiss
	if (pet->HasAura(19574))
		pet->RemoveAurasDueToSpell(19574);
	if (pet->HasAura(19577))
		pet->RemoveAurasDueToSpell(19577);

	pet->Unsummon(PET_SAVE_NOT_IN_SLOT, pPlayer);

	AddExecuteLogInfo(eff_idx, ExecuteLogInfo(pet->GetObjectGuid()));
}

void Spell::EffectSummonObject(SpellEffectIndex eff_idx)
{
	if (!m_casterUnit)
		return;

	uint32 go_id = m_spellInfo->EffectMiscValue[eff_idx];

	uint8 slot = 0;
	switch (m_spellInfo->Effect[eff_idx])
	{
	case SPELL_EFFECT_SUMMON_OBJECT_SLOT1:
		slot = 0;
		break;
	case SPELL_EFFECT_SUMMON_OBJECT_SLOT2:
		slot = 1;
		break;
	case SPELL_EFFECT_SUMMON_OBJECT_SLOT3:
		slot = 2;
		break;
	case SPELL_EFFECT_SUMMON_OBJECT_SLOT4:
		slot = 3;
		break;
	default:
		return;
	}

	if (ObjectGuid guid = m_casterUnit->m_ObjectSlotGuid[slot])
	{
		if (GameObject* obj = m_casterUnit ? m_casterUnit->GetMap()->GetGameObject(guid) : NULL)
			obj->SetLootState(GO_JUST_DEACTIVATED);
		m_casterUnit->m_ObjectSlotGuid[slot].Clear();
	}

	GameObject* pGameObj = new GameObject;

	float x, y, z;
	// If dest location if present
	if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
	{
		x = m_targets.m_destX;
		y = m_targets.m_destY;
		z = m_targets.m_destZ;
	}
	// Summon in random point all other units if location present
	else
		m_casterUnit->GetClosePoint(x, y, z, DEFAULT_WORLD_OBJECT_SIZE);

	Map* map = m_casterUnit->GetMap();
	if (!pGameObj->Create(map->GenerateLocalLowGuid(HIGHGUID_GAMEOBJECT), go_id, map,
		x, y, z, m_casterUnit->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, GO_ANIMPROGRESS_DEFAULT, GO_STATE_READY))
	{
		delete pGameObj;
		return;
	}

	uint32 GameObjLevel = m_casterUnit->getLevel();

	switch (m_spellInfo->Id)
	{
		case 35873:
		{
			GameObjLevel = 60;
			break;
		}
	}

	pGameObj->SetUInt32Value(GAMEOBJECT_LEVEL, GameObjLevel);
	int32 duration = m_spellInfo->GetDuration();
	pGameObj->SetRespawnTime(duration > 0 ? duration / IN_MILLISECONDS : 0);
	pGameObj->SetSpellId(m_spellInfo->Id);
	m_casterUnit->AddGameObject(pGameObj);

	map->Add(pGameObj);
	WorldPacket data(SMSG_GAMEOBJECT_SPAWN_ANIM, 8);
	data << ObjectGuid(pGameObj->GetObjectGuid());
	m_casterUnit->SendMessageToSet(&data, true);

	m_casterUnit->m_ObjectSlotGuid[slot] = pGameObj->GetObjectGuid();

	pGameObj->SummonLinkedTrapIfAny();

	if (m_casterUnit->GetTypeId() == TYPEID_UNIT && ((Creature*)m_casterUnit)->AI())
		((Creature*)m_casterUnit)->AI()->JustSummoned(pGameObj);

	AddExecuteLogInfo(eff_idx, ExecuteLogInfo(pGameObj->GetObjectGuid()));
}

void Spell::EffectResurrect(SpellEffectIndex eff_idx)
{
	if (!unitTarget)
		return;
	if (unitTarget->GetTypeId() != TYPEID_PLAYER)
		return;

	if (unitTarget->isAlive())
		return;
	if (!unitTarget->IsInWorld())
		return;

	switch (m_spellInfo->Id)
	{
	case 35011:
	{
		if (unitTarget == m_casterUnit)
			m_casterUnit->CastSpell(m_casterUnit, 35050, true);
		return;
	}
	// Defibrillate (Goblin Jumper Cables) have 25% chance on success
	case 8342:
		if (roll_chance_i(75))
		{
			if (m_casterUnit)
				m_casterUnit->CastSpell(m_casterUnit, 8338, true, m_CastItem);
			return;
		}
		break;
		// Defibrillate (Goblin Jumper Cables XL) have 75% chance on success
	case 22999:
		if (roll_chance_i(25))
		{
			if (m_casterUnit)
				m_casterUnit->CastSpell(m_casterUnit, 23055, true, m_CastItem);
			return;
		}
		break;
	default:
		break;
	}

	Player* pTarget = ((Player*)unitTarget);

	if (pTarget->IsRessurectRequested())      // already have one active request
		return;

	uint32 health = pTarget->GetMaxHealth() * damage / 100;
	uint32 mana = pTarget->GetMaxPower(POWER_MANA) * damage / 100;

	pTarget->SetResurrectRequestData(m_caster->GetObjectGuid(), m_caster->GetMapId(), m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ(), health, mana);
	SendResurrectRequest(pTarget);

	AddExecuteLogInfo(eff_idx, ExecuteLogInfo(unitTarget->GetObjectGuid()));
}

void Spell::EffectAddExtraAttacks(SpellEffectIndex eff_idx)
{
	if (!unitTarget)
		return;

	if (!unitTarget->isAlive() || unitTarget->IsExtraAttacksLocked())
		return;

	if (m_spellInfo->Id == 20178) // Reckoning
	{
		if (unitTarget->GetExtraAttacks() < 3)
			unitTarget->AddExtraAttack();
		return;
	}

	if (unitTarget->GetExtraAttacks())
		return;

	unitTarget->AddExtraAttackOnUpdate();
	unitTarget->SetExtraAttaks(damage);

	ExecuteLogInfo info(unitTarget->GetObjectGuid());
	info.extraAttacks.count = damage;
	AddExecuteLogInfo(eff_idx, info);
}

void Spell::EffectParry(SpellEffectIndex /*eff_idx*/)
{
	if (unitTarget && unitTarget->GetTypeId() == TYPEID_PLAYER)
		((Player*)unitTarget)->SetCanParry(true);
}

void Spell::EffectBlock(SpellEffectIndex /*eff_idx*/)
{
	if (unitTarget && unitTarget->GetTypeId() == TYPEID_PLAYER)
		((Player*)unitTarget)->SetCanBlock(true);
}

void Spell::EffectLeapForward(SpellEffectIndex eff_idx)
{
	if (!unitTarget)
		return;

	if (unitTarget->IsTaxiFlying())
		return;

	float x, y, z;
	m_targets.getDestination(x, y, z);

	unitTarget->NearTeleportTo(x, y, z, unitTarget->GetOrientation(), TELE_TO_NOT_LEAVE_TRANSPORT | TELE_TO_NOT_LEAVE_COMBAT | TELE_TO_NOT_UNSUMMON_PET | (unitTarget == m_caster ? TELE_TO_SPELL : 0));

}

void Spell::EffectReputation(SpellEffectIndex eff_idx)
{
	if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
		return;

	Player* _player = (Player*)unitTarget;

	int32  rep_change = m_currentBasePoints[eff_idx];
	uint32 faction_id = m_spellInfo->EffectMiscValue[eff_idx];

	FactionEntry const* factionEntry = sObjectMgr.GetFactionEntry(faction_id);

	if (!factionEntry)
		return;

	rep_change = _player->CalculateReputationGain(REPUTATION_SOURCE_SPELL, rep_change, faction_id);

	_player->GetReputationMgr().ModifyReputation(factionEntry, rep_change);
}

void Spell::EffectQuestComplete(SpellEffectIndex eff_idx)
{
	if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
		return;

	uint32 quest_id = m_spellInfo->EffectMiscValue[eff_idx];
	((Player*)unitTarget)->AreaExploredOrEventHappens(quest_id);
}

void Spell::EffectSelfResurrect(SpellEffectIndex eff_idx)
{
	if (!unitTarget || unitTarget->isAlive())
		return;
	if (unitTarget->GetTypeId() != TYPEID_PLAYER)
		return;
	if (!unitTarget->IsInWorld())
		return;

	uint32 health = 0;
	uint32 mana = 0;

	// flat case
	if (damage < 0)
	{
		health = uint32(-damage);
		mana = m_spellInfo->EffectMiscValue[eff_idx];
	}
	// percent case
	else
	{
		health = uint32(damage / 100.0f * unitTarget->GetMaxHealth());
		if (unitTarget->GetMaxPower(POWER_MANA) > 0)
			mana = uint32(damage / 100.0f * unitTarget->GetMaxPower(POWER_MANA));
	}

	Player* plr = ((Player*)unitTarget);
	plr->ResurrectPlayer(0.0f);

	plr->SetHealth(health);
	plr->SetPower(POWER_MANA, mana);
	plr->SetPower(POWER_RAGE, 0);
	plr->SetPower(POWER_ENERGY, plr->GetMaxPower(POWER_ENERGY));


	bool DispellBuffs = true;
	// Reincarnation
	if (m_spellInfo->Id == 21169)
	{
		if (plr->HasAura(16184) || plr->HasAura(16209))
			DispellBuffs = false;
	}

	plr->SpawnCorpseBones(DispellBuffs);
}

void Spell::EffectSkinning(SpellEffectIndex /*eff_idx*/)
{
	if (!unitTarget->IsInWorld() || unitTarget->GetTypeId() != TYPEID_UNIT)
		return;
	if (!m_caster || m_caster->GetTypeId() != TYPEID_PLAYER || !m_caster->IsInWorld())
		return;

	Creature* creature = (Creature*)unitTarget;
	int32 targetLevel = creature->getLevel();

	((Player*)m_caster)->SendLoot(creature->GetObjectGuid(), LOOT_SKINNING);
	creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);

	int32 reqValue = targetLevel < 10 ? 0 : targetLevel < 20 ? (targetLevel - 10) * 10 : targetLevel * 5;

	int32 skillValue = ((Player*)m_caster)->GetSkillValuePure(SKILL_SKINNING);

	// Double chances for elites
	((Player*)m_caster)->UpdateGatherSkill(SKILL_SKINNING, skillValue, reqValue, creature->IsElite() ? 2 : 1);
}

void Spell::EffectCharge(SpellEffectIndex /*eff_idx*/)
{
	if (!unitTarget)
		return;

	// cf Spell::OnSpellLaunch
}

void Spell::EffectSummonCritter(SpellEffectIndex eff_idx)
{
	if (m_caster->GetTypeId() != TYPEID_PLAYER)
		return;
	Player* player = (Player*)m_caster;

	uint32 pet_entry = m_spellInfo->EffectMiscValue[eff_idx];
	if (!pet_entry)
		return;

	CreatureInfo const* cInfo;
	// Custom pet handling (see table 'custom_pet_entry_relation')
	if (m_spellInfo->Id == 28505 && m_CastItem->GetEntry() != 22781)
	{ // 22781 = Original item for spell 28505
		uint32 creature_entry = sObjectMgr.GetCustomPetCreatureEntryFromItem(m_CastItem->GetEntry());
		if (creature_entry)
		{
			cInfo = sCreatureStorage.LookupEntry<CreatureInfo>(creature_entry);
		}
		else
		{
			sLog.outErrorDb("Spell::DoSummonCritter: (custom pet) creature entry not found for item %u.",
				m_CastItem->GetEntry());
			return;
		}
		// End of Custom pet handling
	}
	else
	{
		cInfo = sCreatureStorage.LookupEntry<CreatureInfo>(pet_entry);
	}

	if (!cInfo)
	{
		sLog.outErrorDb("Spell::DoSummonCritter: creature entry %u not found for spell %u.", pet_entry, m_spellInfo->Id);
		return;
	}

	Pet* old_critter = player->GetMiniPet();

	// for same pet just despawn
	if (old_critter && old_critter->GetEntry() == cInfo->entry)
	{
		player->RemoveMiniPet();
		return;
	}

	// despawn old pet before summon new
	if (old_critter)
		player->RemoveMiniPet();

	CreatureCreatePos pos(m_caster, m_caster->GetOrientation(), PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
	if (!(m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION))
		pos = CreatureCreatePos(m_caster, m_caster->GetOrientation());

	// summon new pet
	Pet* critter = new Pet(MINI_PET);

	Map* map = m_caster->GetMap();
	uint32 pet_number = sObjectMgr.GeneratePetNumber();
	if (!critter->Create(map->GenerateLocalLowGuid(HIGHGUID_PET), pos, cInfo, pet_number))
	{
		sLog.outError("Spell::EffectSummonCritter, spellid %u: no such creature entry %u", m_spellInfo->Id, pet_entry);
		delete critter;
		return;
	}

	critter->SetSummonPoint(pos);

	//critter->SetName("");                                 // generated by client
	critter->SetOwnerGuid(m_caster->GetObjectGuid());
	critter->SetCreatorGuid(m_caster->GetObjectGuid());
	critter->setFaction(m_caster->getFaction());
	critter->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->Id);

	critter->AIM_Initialize();
	critter->InitPetCreateSpells();                         // e.g. disgusting oozeling has a create spell as critter...
	critter->SelectLevel(critter->GetCreatureInfo());       // some summoned creaters have different from 1 DB data for level/hp
	critter->SetUInt32Value(UNIT_NPC_FLAGS, critter->GetCreatureInfo()->npc_flags);
	// some mini-pets have quests

	// set timer for unsummon
	int32 duration = m_spellInfo->GetDuration();
	if (duration > 0)
		critter->SetDuration(duration);

	player->_SetMiniPet(critter);

	map->Add((Creature*)critter);

	// Notify Summoner
	if (m_caster->GetTypeId() == TYPEID_UNIT && ((Creature*)m_caster)->AI())
		((Creature*)m_caster)->AI()->JustSummoned(critter);

	AddExecuteLogInfo(eff_idx, ExecuteLogInfo(critter->GetObjectGuid()));
}

void Spell::EffectKnockBack(SpellEffectIndex eff_idx)
{
	if (!unitTarget || unitTarget->IsTaxiFlying())
		return;

	// remove Dream Fog Sleep aura to let target be launched
	// ugly and barely working solution untill proper pending states handling implemented
	unitTarget->RemoveAurasDueToSpell(24778);

	uint32 KnockBackDistance = m_spellInfo->EffectMiscValue[eff_idx];

	if (m_spellInfo->Id == 35488)
	{
		KnockBackDistance = urand(30, 700);
	}

	unitTarget->KnockBackFrom(m_caster, float(KnockBackDistance) / 10, float(damage) / 10);


	switch (m_spellInfo->Id)//
	{

	case 35095: //AC BS Thunder Clap threat
	{
		if (unitTarget && m_casterUnit)
		{
			if (m_casterUnit->getThreatManager().getThreat(unitTarget))
				m_casterUnit->getThreatManager().modifyThreatPercent(unitTarget, -30);
		}
		return;
	}
	case 13009:                                //Amnnennar Wrath (Knock Away)
	{
		if (!unitTarget || !m_casterUnit)
			return;

		m_casterUnit->getThreatManager().modifyThreatPercent(unitTarget, -90);

		return;
	}
	break;
	}//
}

void Spell::EffectSendTaxi(SpellEffectIndex eff_idx)
{
	if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
		return;

	((Player*)unitTarget)->ActivateTaxiPathTo(m_spellInfo->EffectMiscValue[eff_idx], m_spellInfo->Id, true);
}


void Spell::EffectPlayerPull(SpellEffectIndex eff_idx)
{
	if (!unitTarget)
		return;

	if (m_casterUnit->HasAuraType(SPELL_AURA_FEATHER_FALL))
		m_casterUnit->RemoveSpellsCausingAura(SPELL_AURA_FEATHER_FALL);

	switch (m_spellInfo->Id)
	{
	case 28337: // thaddius Magnetic Pull
	case 35487: // Gravity Defied
	{
		float speedXY = float(m_spellInfo->EffectMiscValue[eff_idx]) * 0.1f;
		float speedZ = unitTarget->GetDistance(m_caster) / speedXY * 0.5f * 20.0f;
		unitTarget->KnockBackFrom(m_caster, -speedXY, speedZ);
		break;
	}
	case 34590: // Hook Pull
	case 35002: // Bloody Hook
	case 35922:	// Holy Nova reversed knockback
	{
		float speedXY = float(m_spellInfo->EffectMiscValue[eff_idx]) * 0.1f;
		float speedZ = unitTarget->GetDistance(m_caster) / speedXY * 0.5f * 20.0f;
		float Zmod = 1.0f;
		float XYmod = 1.0f;
		if (m_casterUnit->GetPositionZ() > unitTarget->GetPositionZ())
			Zmod = 2.0f;
		else XYmod = 0.75f;
		unitTarget->KnockBackFrom(m_caster, -speedXY * XYmod, speedZ * Zmod);

		if (m_spellInfo->Id == 35922)	//Debuff
			unitTarget->AddAura(35923);
		break;
	}
	case 28434: // Spider Web
	{
		// see boss_maexxnaAI::DoCastWebWrap() for some info on this rather weird implementation
		float dx = unitTarget->GetPositionX() - m_caster->GetPositionX();
		float dy = unitTarget->GetPositionY() - m_caster->GetPositionY();
		float dist = sqrt((dx * dx) + (dy * dy));
		const float  distXY = (dist > 0 ? dist : 0);
		float yDist = m_caster->GetPositionZ() - unitTarget->GetPositionZ();
		float horizontalSpeed = dist / 1.5f;
		float verticalSpeed = 12.0f + (yDist * 0.5f);
		float angle = unitTarget->GetAngle(m_caster->GetPositionX(), m_caster->GetPositionY());

		// set immune anticheat and calculate speed
		if (Player* plr = unitTarget->ToPlayer())
		{
			plr->SetLaunched(true);
			plr->SetXYSpeed(horizontalSpeed);
		}

		unitTarget->KnockBack(angle, horizontalSpeed, verticalSpeed);
		break;
	}
	//case 33760: //vanilla+ Leap druid cat
	//{
	//	float speedXY = float(m_spellInfo->EffectMiscValue[eff_idx]) * 0.15f;
	//	float speedZ = unitTarget->GetDistance(m_caster) / speedXY * 0.5f * 100.0f;
	//	unitTarget->KnockBackFrom(m_caster, -speedXY, speedZ);
	//	break;
	//}
	case 33758: //vanilla+ Leap druid cat M_PI_F EffectBasePoints
	{
		float speedXY = float(m_spellInfo->EffectMiscValue[eff_idx]);
		float speedZ = float(m_spellInfo->EffectBasePoints[eff_idx]);
		m_casterUnit->KnockBackFrom(m_caster, -speedXY, speedZ);
		break;
	}
	case 781: //vanilla+ Disengage
	case 14272:
	case 14273:
	{
		float speedXY = float(m_spellInfo->EffectMiscValue[eff_idx]);
		float speedZ = float(m_spellInfo->EffectBasePoints[eff_idx]);
		m_casterUnit->KnockBackFrom(m_caster, speedXY, speedZ);
		break;
	}
	case 34462: //vanilla+ Magnetize
	{
		float dist = 5.5f - unitTarget->GetDistance2d(m_caster);
		if (damage && dist > damage)
			dist = float(damage);
		unitTarget->KnockBackFrom(m_caster, dist * 5, dist * 5);
		break;
	}
	default:
	{
		// Todo: this implementation seems very wrong. Gives terrible results for maexxna web-wrap and
		// thaddius magnetic pull
		float dist = unitTarget->GetDistance2d(m_caster);
		if (damage && dist > damage)
			dist = float(damage);
		unitTarget->KnockBackFrom(m_caster, -dist, float(m_spellInfo->EffectMiscValue[eff_idx]) / 10);
	}
	}
}

void Spell::EffectDispelMechanic(SpellEffectIndex eff_idx)
{
	if (!unitTarget)
		return;

	uint32 mechanic = m_spellInfo->EffectMiscValue[eff_idx];

	Unit::SpellAuraHolderMap& Auras = unitTarget->GetSpellAuraHolderMap();
	for (Unit::SpellAuraHolderMap::iterator iter = Auras.begin(), next; iter != Auras.end(); iter = next)
	{
		next = iter;
		++next;
		SpellEntry const* spell = iter->second->GetSpellProto();
		if (iter->second->HasMechanic(mechanic))
		{
			unitTarget->RemoveAurasDueToSpell(spell->Id);
			if (Auras.empty())
				break;
			else
				next = Auras.begin();
		}
	}

	AddExecuteLogInfo(eff_idx, ExecuteLogInfo(unitTarget->GetObjectGuid()));
}

void Spell::EffectSummonDeadPet(SpellEffectIndex /*eff_idx*/)
{
	if (m_caster->GetTypeId() != TYPEID_PLAYER)
		return;
	Player* _player = (Player*)m_caster;
	Pet* pet = _player->GetPet();
	if (!pet)
		return;
	if (pet->isAlive())
		return;
	if (damage < 0)
		return;

	// Chakor : Teleport the pet to the player's location
	pet->NearTeleportTo(_player->GetPositionX(), _player->GetPositionY(), _player->GetPositionZ(), _player->GetOrientation(), false);
	pet->SetUInt32Value(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_NONE);
	pet->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);
	pet->SetDeathState(ALIVE);
	pet->clearUnitState(UNIT_STAT_ALL_DYN_STATES);
	pet->SetHealth(uint32(pet->GetMaxHealth() * (float(damage) / 100)));

	pet->AIM_Initialize();

	// _player->PetSpellInitialize(); -- action bar not removed at death and not required send at revive
	pet->SavePetToDB(PET_SAVE_AS_CURRENT);
}

void Spell::EffectDestroyAllTotems(SpellEffectIndex /*eff_idx*/)
{
	if (!m_casterUnit)
		return;

	/*for (int slot = 0; slot < MAX_TOTEM_SLOT; ++slot)
		//if (Totem* totem = m_casterUnit->GetTotem(TotemSlot(slot)))
			totem->UnSummon();*/


	int32 manaback = 0;

	for (int slot = 0; slot < MAX_TOTEM_SLOT; ++slot)
		if (Totem* totem = m_casterUnit->GetTotem(TotemSlot(slot)))
		{
			int32 totemcost = 0;
			uint32 spell_id = totem->GetUInt32Value(UNIT_CREATED_BY_SPELL);

			SpellEntry const* originalSpell = sSpellMgr.GetSpellEntry(spell_id);
			if (originalSpell->ManaCostPercentage)
				totemcost += originalSpell->ManaCostPercentage * m_casterUnit->GetCreateMana() / 200;
			else
				totemcost += originalSpell->manaCost;

			if (Player* modOwner = m_casterUnit->GetSpellModOwner())
				modOwner->ApplySpellMod(spell_id, SPELLMOD_COST, totemcost, this);

			SpellSchools school = GetFirstSchoolInMask(originalSpell->GetSpellSchoolMask());
			totemcost = int32(totemcost * (1.0f + m_casterUnit->GetFloatValue(UNIT_FIELD_POWER_COST_MULTIPLIER + school)));

			if (totemcost < 0)
				totemcost = 0;

			switch (spell_id)
			{
				case 1535:
				case 8498:
				case 8499:
				case 11314:
				case 11315:
				{
					if (totem->GetTotemDuration() < 1000)
						totemcost = 0;
				}
			}

			manaback += totemcost;

			totem->UnSummon();
		}

	//Totemic Call mana return
	if (manaback > 0)
		m_casterUnit->CastCustomSpell(m_casterUnit, 34649, &manaback, nullptr, nullptr, true, nullptr);
}

void Spell::EffectDurabilityDamage(SpellEffectIndex eff_idx)
{
	if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
		return;

	int32 slot = m_spellInfo->EffectMiscValue[eff_idx];

	// FIXME: some spells effects have value -1/-2
	// Possibly its mean -1 all player equipped items and -2 all items
	if (slot < 0)
	{
		((Player*)unitTarget)->DurabilityPointsLossAll(damage, (slot < -1));

		ExecuteLogInfo info(unitTarget->GetObjectGuid());
		info.durabilityDamage.itemEntry = -1;
		info.durabilityDamage.unk = -1;
		AddExecuteLogInfo(eff_idx, info);

		return;
	}

	// invalid slot value
	if (slot >= INVENTORY_SLOT_BAG_END)
		return;

	if (Item* item = ((Player*)unitTarget)->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
	{
		((Player*)unitTarget)->DurabilityPointsLoss(item, damage);

		ExecuteLogInfo info(unitTarget->GetObjectGuid());
		info.durabilityDamage.itemEntry = item->GetProto()->ItemId;
		info.durabilityDamage.unk = -1;
		AddExecuteLogInfo(eff_idx, info);
	}
}

void Spell::EffectDurabilityDamagePCT(SpellEffectIndex eff_idx)
{
	if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
		return;

	int32 slot = m_spellInfo->EffectMiscValue[eff_idx];

	// FIXME: some spells effects have value -1/-2
	// Possibly its mean -1 all player equipped items and -2 all items
	if (slot < 0)
	{
		((Player*)unitTarget)->DurabilityLossAll(double(damage) / 100.0f, (slot < -1));
		return;
	}

	// invalid slot value
	if (slot >= INVENTORY_SLOT_BAG_END)
		return;

	if (damage <= 0)
		return;

	if (Item* item = ((Player*)unitTarget)->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
		((Player*)unitTarget)->DurabilityLoss(item, double(damage) / 100.0f);
}

void Spell::EffectModifyThreatPercent(SpellEffectIndex eff_idx)
{
	if (!unitTarget || !m_casterUnit)
		return;

	unitTarget->getThreatManager().modifyThreatPercent(m_casterUnit, damage);

	AddExecuteLogInfo(eff_idx, ExecuteLogInfo(unitTarget->GetObjectGuid()));
}

void Spell::EffectTransmitted(SpellEffectIndex eff_idx)
{
	if (!m_casterUnit)
		return;

	uint32 name_id = m_spellInfo->EffectMiscValue[eff_idx];

	GameObjectInfo const* goinfo = ObjectMgr::GetGameObjectInfo(name_id);

	if (!goinfo)
	{
		sLog.outErrorDb("Gameobject (Entry: %u) not exist and not created at spell (ID: %u) cast", name_id, m_spellInfo->Id);
		return;
	}

	float fx, fy, fz;
	if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
	{
		fx = m_targets.m_destX;
		fy = m_targets.m_destY;
		fz = m_targets.m_destZ;
	}
	//FIXME: this can be better check for most objects but still hack
	else if (m_spellInfo->EffectRadiusIndex[eff_idx] && m_spellInfo->speed == 0)
	{
		float dis = GetSpellRadius(sSpellRadiusStore.LookupEntry(m_spellInfo->EffectRadiusIndex[eff_idx]));
		float x, y, z;
		m_casterUnit->GetPosition(x, y, z);
		fx = x + dis * cos(m_casterUnit->GetOrientation());
		fy = y + dis * sin(m_casterUnit->GetOrientation());
		fz = z;
		m_casterUnit->GetMap()->GetLosHitPosition(x, y, z + 0.5f, fx, fy, fz, -1.5f);
	}
	else
	{
		float min_dis = GetSpellMinRange(sSpellRangeStore.LookupEntry(m_spellInfo->rangeIndex));
		float max_dis = GetSpellMaxRange(sSpellRangeStore.LookupEntry(m_spellInfo->rangeIndex));
		float dis = rand_norm_f() * (max_dis - min_dis) + min_dis;

		float x, y, z;
		m_casterUnit->GetPosition(x, y, z);
		fx = x + dis * cos(m_casterUnit->GetOrientation());
		fy = y + dis * sin(m_casterUnit->GetOrientation());
		fz = z;
		m_casterUnit->GetMap()->GetLosHitPosition(x, y, z + 0.5f, fx, fy, fz, -1.5f);
	}

	Map *cMap = m_casterUnit->GetMap();

	if (goinfo->type == GAMEOBJECT_TYPE_FISHINGNODE)
	{
		float waterLevel = m_casterUnit->GetTerrain()->GetWaterLevel(fx, fy, fz);
		if (waterLevel == VMAP_INVALID_HEIGHT_VALUE)             // Hack to prevent fishing bobber from failing to land on fishing hole
		{
			// but this is not proper, we really need to ignore not materialized objects
			SendCastResult(SPELL_FAILED_NOT_FISHABLE);
			SendChannelUpdate(0);
			finish();
			return;
		}

		// replace by water level in this case
		//fz = cMap->GetWaterLevel(fx, fy);
		fz = waterLevel;
	}

	GameObject* pGameObj = new GameObject;

	if (!pGameObj->Create(cMap->GenerateLocalLowGuid(HIGHGUID_GAMEOBJECT), name_id, cMap,
		fx, fy, fz, m_casterUnit->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, GO_ANIMPROGRESS_DEFAULT, GO_STATE_READY))
	{
		delete pGameObj;
		return;
	}

	int32 duration = m_spellInfo->GetDuration();

	switch (goinfo->type)
	{
	case GAMEOBJECT_TYPE_FISHINGNODE:
	{
		m_casterUnit->SetChannelObjectGuid(pGameObj->GetObjectGuid());
		m_casterUnit->AddGameObject(pGameObj);              // will removed at spell cancel

		// end time of range when possible catch fish (FISHING_BOBBER_READY_TIME..GetDuration(m_spellInfo))
		// start time == fish-FISHING_BOBBER_READY_TIME (0..GetDuration(m_spellInfo)-FISHING_BOBBER_READY_TIME)
		int32 lastSec = 0;
		switch (urand(0, 3))
		{
		case 0:
			lastSec = 3;
			break;
		case 1:
			lastSec = 7;
			break;
		case 2:
			lastSec = 13;
			break;
		case 3:
			lastSec = 17;
			break;
		}

		duration = duration - lastSec * IN_MILLISECONDS + FISHING_BOBBER_READY_TIME * IN_MILLISECONDS;
		break;
	}
	case GAMEOBJECT_TYPE_SUMMONING_RITUAL:
	{
		if (m_casterUnit->GetTypeId() == TYPEID_PLAYER)
		{
			// Set the summoning target
			if (m_casterUnit->GetTypeId() == TYPEID_PLAYER && m_casterUnit->ToPlayer()->GetSelectionGuid())
				pGameObj->SetSummonTarget(m_casterUnit->ToPlayer()->GetSelectionGuid());

			// will be removed at Spell::Cancel or GameObject::RemoveUniqueUse if activated
			m_casterUnit->AddGameObject(pGameObj);

			// the caster becomes a go user too
			pGameObj->AddUniqueUse(m_casterUnit->ToPlayer());
			m_targets.setGOTarget(pGameObj);
			SetChannelingVisual(true);
		}
		break;
	}
	case GAMEOBJECT_TYPE_FISHINGHOLE:
	case GAMEOBJECT_TYPE_CHEST:
	default:
		break;
	}

	pGameObj->SetRespawnTime(duration > 0 ? duration / IN_MILLISECONDS : 0);

	pGameObj->SetOwnerGuid(m_casterUnit->GetObjectGuid());
	if (m_casterUnit->GetTypeId() == TYPEID_PLAYER)
	{
		if (Group * group = ((Player*)m_casterUnit)->GetGroup())
			pGameObj->SetOwnerGroupId(group->GetId());
	}
	pGameObj->SetUInt32Value(GAMEOBJECT_LEVEL, m_casterUnit->getLevel());
	pGameObj->SetSpellId(m_spellInfo->Id);

	DEBUG_LOG("AddObject at SpellEfects.cpp EffectTransmitted");
	//m_casterUnit->AddGameObject(pGameObj);
	//m_ObjToDel.push_back(pGameObj);

	cMap->Add(pGameObj);

	pGameObj->SummonLinkedTrapIfAny();

	if (m_casterUnit->GetTypeId() == TYPEID_UNIT && ((Creature*)m_casterUnit)->AI())
		((Creature*)m_casterUnit)->AI()->JustSummoned(pGameObj);

	AddExecuteLogInfo(eff_idx, ExecuteLogInfo(pGameObj->GetObjectGuid()));
}

void Spell::EffectSkill(SpellEffectIndex /*eff_idx*/)
{
	DEBUG_LOG("WORLD: SkillEFFECT");
}

void Spell::EffectSummonDemon(SpellEffectIndex eff_idx)
{
	float px = m_targets.m_destX;
	float py = m_targets.m_destY;
	float pz = m_targets.m_destZ;

	// summon to the ritual object location if any
	if (GameObject* pGo = m_targets.getGOTarget())
		if (pGo->GetGoType() == GAMEOBJECT_TYPE_SUMMONING_RITUAL)
			pGo->GetPosition(px, py, pz);

	Creature* Charmed = m_caster->SummonCreature(m_spellInfo->EffectMiscValue[eff_idx], px, py, pz, m_caster->GetOrientation(), TEMPSUMMON_TIMED_COMBAT_OR_DEAD_DESPAWN, 3600000);
	if (!Charmed)
		return;

	// might not always work correctly, maybe the creature that dies from CoD casts the effect on itself and is therefore the caster?
	Charmed->SetLevel(m_caster->getLevel());

	// TODO: Add damage/mana/hp according to level

	if (m_spellInfo->EffectMiscValue[eff_idx] == 89)        // Inferno summon
	{ 
		// Enslave demon effect, without mana cost and cooldown
		m_caster->CastSpell(Charmed, 20882, true);          // FIXME: enslave does not scale with level, level 62+ minions cannot be enslaved

		// Inferno effect
		Charmed->CastSpell(Charmed, 22703, true, nullptr);
	}

	AddExecuteLogInfo(eff_idx, ExecuteLogInfo(Charmed->GetObjectGuid()));
}

void Spell::EffectSpiritHeal(SpellEffectIndex /*eff_idx*/)
{
	// TODO player can't see the heal-animation - he should respawn some ticks later
	if (!unitTarget || unitTarget->isAlive())
		return;
	if (unitTarget->GetTypeId() != TYPEID_PLAYER)
		return;
	if (!unitTarget->IsInWorld())
		return;
	if (!unitTarget->HasAura(2584))
		return;

	auto player = unitTarget->ToPlayer();

	if (!player)
		return;

	// no resurrection on a GY other than homie if BG is not in progress
	if (player->GetBattleGround()->GetStatus() != STATUS_IN_PROGRESS && !player->IsGameMaster())
		player->RepopAtGraveyard();

	player->RemoveAurasDueToSpell(2584);
	player->ResurrectPlayer(1.0f);
	player->SpawnCorpseBones();
	player->AutoReSummonPet();
}

// remove insignia spell effect
void Spell::EffectSkinPlayerCorpse(SpellEffectIndex eff_idx)
{
	DEBUG_LOG("Effect: SkinPlayerCorpse");
	Player* playerCaster = m_caster->ToPlayer();
	if (!playerCaster)
		return;

	Unit *target = unitTarget;
	if (!target && corpseTarget)
		target = ObjectAccessor::FindPlayer(corpseTarget->GetOwnerGuid());
	if (!target)
	{
		ASSERT(corpseTarget);
		sObjectAccessor.ConvertCorpseForPlayer(corpseTarget->GetOwnerGuid(), playerCaster);
		playerCaster->SendLoot(corpseTarget->GetObjectGuid(), LOOT_INSIGNIA);
		DEBUG_LOG("Effect SkinPlayerCorpse: corpse owner was not found");
		return;
	}

	if (target->GetTypeId() != TYPEID_PLAYER || target->isAlive())
		return;

	((Player*)target)->RemovedInsignia(playerCaster, corpseTarget);

	AddExecuteLogInfo(eff_idx, ExecuteLogInfo(target->GetObjectGuid()));
}

void Spell::EffectBind(SpellEffectIndex eff_idx)
{
	if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
		return;

	Player* player = (Player*)unitTarget;

	uint32 area_id;
	WorldLocation loc;
	player->GetPosition(loc);
	area_id = player->GetAreaId();

	player->SetHomebindToLocation(loc, area_id);

	// binding
	WorldPacket data(SMSG_BINDPOINTUPDATE, (4 + 4 + 4 + 4 + 4));
	data << float(loc.coord_x);
	data << float(loc.coord_y);
	data << float(loc.coord_z);
	data << uint32(loc.mapid);
	data << uint32(area_id);
	player->SendDirectMessage(&data);

	DEBUG_LOG("New Home Position X is %f", loc.coord_x);
	DEBUG_LOG("New Home Position Y is %f", loc.coord_y);
	DEBUG_LOG("New Home Position Z is %f", loc.coord_z);
	DEBUG_LOG("New Home MapId is %u", loc.mapid);
	DEBUG_LOG("New Home AreaId is %u", area_id);

	// zone update
	data.Initialize(SMSG_PLAYERBOUND, 8 + 4);
	data << m_caster->GetObjectGuid();
	data << uint32(area_id);
	player->SendDirectMessage(&data);
}

void Spell::EffectDespawnObject(SpellEffectIndex eff_idx)
{
	DEBUG_LOG("SPELL_EFFECT_DESPAWN_OBJECT");
	if (!gameObjTarget)
		return;
	gameObjTarget->AddObjectToRemoveList();
}

void Spell::EffectNostalrius(SpellEffectIndex eff_idx)
{
	DEBUG_LOG("SPELL_EFFECT_NOSTALRIUS");
}

