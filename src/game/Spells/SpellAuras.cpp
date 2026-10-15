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
#include "WorldSession.h"
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
#include "Group.h"
#include "UpdateData.h"
#include "ObjectAccessor.h"
#include "Policies/SingletonImp.h"
#include "Totem.h"
#include "Creature.h"
#include "Formulas.h"
#include "BattleGround.h"
#include "CreatureAI.h"
#include "ScriptMgr.h"
#include "Util.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "MapManager.h"
#include "MoveSpline.h"
#include "MovementPacketSender.h"
#include "ZoneScript.h"
#include "PlayerAI.h"
#include "Anticheat.h"
#include "scriptPCH.h"


using namespace Spells;

#define NULL_AURA_SLOT 0xFF

/**
 * An array with all the different handlers for taking care of
 * the various aura types that are defined in AuraType.
 */
pAuraHandler AuraHandler[TOTAL_AURAS] =
{
    &Aura::HandleNULL,                                      //  0 SPELL_AURA_NONE
    &Aura::HandleBindSight,                                 //  1 SPELL_AURA_BIND_SIGHT
    &Aura::HandleModPossess,                                //  2 SPELL_AURA_MOD_POSSESS
    &Aura::HandlePeriodicDamage,                            //  3 SPELL_AURA_PERIODIC_DAMAGE
    &Aura::HandleAuraDummy,                                 //  4 SPELL_AURA_DUMMY
    &Aura::HandleModConfuse,                                //  5 SPELL_AURA_MOD_CONFUSE
    &Aura::HandleModCharm,                                  //  6 SPELL_AURA_MOD_CHARM
    &Aura::HandleModFear,                                   //  7 SPELL_AURA_MOD_FEAR
    &Aura::HandlePeriodicHeal,                              //  8 SPELL_AURA_PERIODIC_HEAL
    &Aura::HandleModAttackSpeed,                            //  9 SPELL_AURA_MOD_ATTACKSPEED
    &Aura::HandleModThreat,                                 // 10 SPELL_AURA_MOD_THREAT
    &Aura::HandleModTaunt,                                  // 11 SPELL_AURA_MOD_TAUNT
    &Aura::HandleAuraModStun,                               // 12 SPELL_AURA_MOD_STUN
    &Aura::HandleModDamageDone,                             // 13 SPELL_AURA_MOD_DAMAGE_DONE
    &Aura::HandleNoImmediateEffect,                         // 14 SPELL_AURA_MOD_DAMAGE_TAKEN   implemented in Unit::MeleeDamageBonusTaken and Unit::SpellBaseDamageBonusTaken
    &Aura::HandleNoImmediateEffect,                         // 15 SPELL_AURA_DAMAGE_SHIELD      implemented in Unit::DealMeleeDamage
    &Aura::HandleModStealth,                                // 16 SPELL_AURA_MOD_STEALTH
    &Aura::HandleNoImmediateEffect,                         // 17 SPELL_AURA_MOD_STEALTH_DETECT
    &Aura::HandleInvisibility,                              // 18 SPELL_AURA_MOD_INVISIBILITY
    &Aura::HandleInvisibilityDetect,                        // 19 SPELL_AURA_MOD_INVISIBILITY_DETECTION
    &Aura::HandleAuraModTotalHealthPercentRegen,            // 20 SPELL_AURA_OBS_MOD_HEALTH
    &Aura::HandleAuraModTotalManaPercentRegen,              // 21 SPELL_AURA_OBS_MOD_MANA
    &Aura::HandleAuraModResistance,                         // 22 SPELL_AURA_MOD_RESISTANCE
    &Aura::HandlePeriodicTriggerSpell,                      // 23 SPELL_AURA_PERIODIC_TRIGGER_SPELL
    &Aura::HandlePeriodicEnergize,                          // 24 SPELL_AURA_PERIODIC_ENERGIZE
    &Aura::HandleAuraModPacify,                             // 25 SPELL_AURA_MOD_PACIFY
    &Aura::HandleAuraModRoot,                               // 26 SPELL_AURA_MOD_ROOT
    &Aura::HandleAuraModSilence,                            // 27 SPELL_AURA_MOD_SILENCE
    &Aura::HandleNoImmediateEffect,                         // 28 SPELL_AURA_REFLECT_SPELLS        implement in Unit::SpellHitResult
    &Aura::HandleAuraModStat,                               // 29 SPELL_AURA_MOD_STAT
    &Aura::HandleAuraModSkill,                              // 30 SPELL_AURA_MOD_SKILL
    &Aura::HandleAuraModIncreaseSpeed,                      // 31 SPELL_AURA_MOD_INCREASE_SPEED
    &Aura::HandleAuraModIncreaseMountedSpeed,               // 32 SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED
    &Aura::HandleAuraModDecreaseSpeed,                      // 33 SPELL_AURA_MOD_DECREASE_SPEED
    &Aura::HandleAuraModIncreaseHealth,                     // 34 SPELL_AURA_MOD_INCREASE_HEALTH
    &Aura::HandleAuraModIncreaseEnergy,                     // 35 SPELL_AURA_MOD_INCREASE_ENERGY
    &Aura::HandleAuraModShapeshift,                         // 36 SPELL_AURA_MOD_SHAPESHIFT
    &Aura::HandleAuraModEffectImmunity,                     // 37 SPELL_AURA_EFFECT_IMMUNITY
    &Aura::HandleAuraModStateImmunity,                      // 38 SPELL_AURA_STATE_IMMUNITY
    &Aura::HandleAuraModSchoolImmunity,                     // 39 SPELL_AURA_SCHOOL_IMMUNITY
    &Aura::HandleAuraModDmgImmunity,                        // 40 SPELL_AURA_DAMAGE_IMMUNITY
    &Aura::HandleAuraModDispelImmunity,                     // 41 SPELL_AURA_DISPEL_IMMUNITY
    &Aura::HandleAuraProcTriggerSpell,                      // 42 SPELL_AURA_PROC_TRIGGER_SPELL  implemented in Unit::ProcDamageAndSpellFor and Unit::HandleProcTriggerSpell
    &Aura::HandleNoImmediateEffect,                         // 43 SPELL_AURA_PROC_TRIGGER_DAMAGE implemented in Unit::ProcDamageAndSpellFor
    &Aura::HandleAuraTrackCreatures,                        // 44 SPELL_AURA_TRACK_CREATURES
    &Aura::HandleAuraTrackResources,                        // 45 SPELL_AURA_TRACK_RESOURCES
    &Aura::HandleUnused,                                    // 46 SPELL_AURA_MOD_PARRY_SKILL
    &Aura::HandleAuraModParryPercent,                       // 47 SPELL_AURA_MOD_PARRY_PERCENT
    &Aura::HandleUnused,                                    // 48 SPELL_AURA_MOD_DODGE_SKILL
    &Aura::HandleAuraModDodgePercent,                       // 49 SPELL_AURA_MOD_DODGE_PERCENT
    &Aura::HandleUnused,                                    // 50 SPELL_AURA_MOD_BLOCK_SKILL    obsolete?
    &Aura::HandleAuraModBlockPercent,                       // 51 SPELL_AURA_MOD_BLOCK_PERCENT
    &Aura::HandleAuraModCritPercent,                        // 52 SPELL_AURA_MOD_CRIT_PERCENT
    &Aura::HandlePeriodicLeech,                             // 53 SPELL_AURA_PERIODIC_LEECH
    &Aura::HandleModHitChance,                              // 54 SPELL_AURA_MOD_HIT_CHANCE
    &Aura::HandleModSpellHitChance,                         // 55 SPELL_AURA_MOD_SPELL_HIT_CHANCE
    &Aura::HandleAuraTransform,                             // 56 SPELL_AURA_TRANSFORM
    &Aura::HandleModSpellCritChance,                        // 57 SPELL_AURA_MOD_SPELL_CRIT_CHANCE
    &Aura::HandleAuraModIncreaseSwimSpeed,                  // 58 SPELL_AURA_MOD_INCREASE_SWIM_SPEED
    &Aura::HandleNoImmediateEffect,                         // 59 SPELL_AURA_MOD_DAMAGE_DONE_CREATURE implemented in Unit::MeleeDamageBonusDone and Unit::SpellDamageBonusDone
    &Aura::HandleAuraModPacifyAndSilence,                   // 60 SPELL_AURA_MOD_PACIFY_SILENCE
    &Aura::HandleAuraModScale,                              // 61 SPELL_AURA_MOD_SCALE
    &Aura::HandlePeriodicHealthFunnel,                      // 62 SPELL_AURA_PERIODIC_HEALTH_FUNNEL
    &Aura::HandleUnused,                                    // 63 SPELL_AURA_PERIODIC_MANA_FUNNEL obsolete?
    &Aura::HandlePeriodicManaLeech,                         // 64 SPELL_AURA_PERIODIC_MANA_LEECH
    &Aura::HandleModCastingSpeed,                           // 65 SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK
    &Aura::HandleFeignDeath,                                // 66 SPELL_AURA_FEIGN_DEATH
    &Aura::HandleAuraModDisarm,                             // 67 SPELL_AURA_MOD_DISARM
    &Aura::HandleAuraModStalked,                            // 68 SPELL_AURA_MOD_STALKED
    &Aura::HandleSchoolAbsorb,                              // 69 SPELL_AURA_SCHOOL_ABSORB implemented in Unit::CalculateAbsorbAndResist
    &Aura::HandleUnused,                                    // 70 SPELL_AURA_EXTRA_ATTACKS      Useless, used by only one spell that has only visual effect
    &Aura::HandleModSpellCritChanceSchool,                  // 71 SPELL_AURA_MOD_SPELL_CRIT_CHANCE_SCHOOL
    &Aura::HandleModPowerCostPCT,                           // 72 SPELL_AURA_MOD_POWER_COST_SCHOOL_PCT
    &Aura::HandleModPowerCost,                              // 73 SPELL_AURA_MOD_POWER_COST_SCHOOL
    &Aura::HandleReflectSpellsSchool,                       // 74 SPELL_AURA_REFLECT_SPELLS_SCHOOL  implemented in Unit::SpellHitResult
    &Aura::HandleNoImmediateEffect,                         // 75 SPELL_AURA_MOD_LANGUAGE
    &Aura::HandleFarSight,                                  // 76 SPELL_AURA_FAR_SIGHT
    &Aura::HandleModMechanicImmunity,                       // 77 SPELL_AURA_MECHANIC_IMMUNITY
    &Aura::HandleAuraMounted,                               // 78 SPELL_AURA_MOUNTED
    &Aura::HandleModDamagePercentDone,                      // 79 SPELL_AURA_MOD_DAMAGE_PERCENT_DONE
    &Aura::HandleModPercentStat,                            // 80 SPELL_AURA_MOD_PERCENT_STAT
    &Aura::HandleNoImmediateEffect,                         // 81 SPELL_AURA_SPLIT_DAMAGE_PCT       implemented in Unit::CalculateAbsorbAndResist
    &Aura::HandleWaterBreathing,                            // 82 SPELL_AURA_WATER_BREATHING
    &Aura::HandleModBaseResistance,                         // 83 SPELL_AURA_MOD_BASE_RESISTANCE
    &Aura::HandleModRegen,                                  // 84 SPELL_AURA_MOD_REGEN
    &Aura::HandleModPowerRegen,                             // 85 SPELL_AURA_MOD_POWER_REGEN
    &Aura::HandleChannelDeathItem,                          // 86 SPELL_AURA_CHANNEL_DEATH_ITEM
    &Aura::HandleNoImmediateEffect,                         // 87 SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN implemented in Unit::MeleeDamageBonusTaken and Unit::SpellDamageBonusTaken
    &Aura::HandleNoImmediateEffect,                         // 88 SPELL_AURA_MOD_HEALTH_REGEN_PERCENT implemented in Player::RegenerateHealth
    &Aura::HandlePeriodicDamagePCT,                         // 89 SPELL_AURA_PERIODIC_DAMAGE_PERCENT
    &Aura::HandleUnused,                                    // 90 SPELL_AURA_MOD_RESIST_CHANCE  Useless
    &Aura::HandleNoImmediateEffect,                         // 91 SPELL_AURA_MOD_DETECT_RANGE implemented in Creature::GetAttackDistance
    &Aura::HandlePreventFleeing,                            // 92 SPELL_AURA_PREVENTS_FLEEING
    &Aura::HandleModUnattackable,                           // 93 SPELL_AURA_MOD_UNATTACKABLE
    &Aura::HandleInterruptRegen,                            // 94 SPELL_AURA_INTERRUPT_REGEN implemented in Player::RegenerateAll
    &Aura::HandleAuraGhost,                                 // 95 SPELL_AURA_GHOST
    &Aura::HandleNoImmediateEffect,                         // 96 SPELL_AURA_SPELL_MAGNET implemented in Unit::SelectMagnetTarget
    &Aura::HandleManaShield,                                // 97 SPELL_AURA_MANA_SHIELD implemented in Unit::CalculateAbsorbAndResist
    &Aura::HandleAuraModSkill,                              // 98 SPELL_AURA_MOD_SKILL_TALENT
    &Aura::HandleAuraModAttackPower,                        // 99 SPELL_AURA_MOD_ATTACK_POWER
    &Aura::HandleAurasVisible,                              //100 SPELL_AURA_AURAS_VISIBLE
    &Aura::HandleModResistancePercent,                      //101 SPELL_AURA_MOD_RESISTANCE_PCT
    &Aura::HandleNoImmediateEffect,                         //102 SPELL_AURA_MOD_MELEE_ATTACK_POWER_VERSUS implemented in Unit::MeleeDamageBonusDone
    &Aura::HandleAuraModTotalThreat,                        //103 SPELL_AURA_MOD_TOTAL_THREAT
    &Aura::HandleAuraWaterWalk,                             //104 SPELL_AURA_WATER_WALK
    &Aura::HandleAuraFeatherFall,                           //105 SPELL_AURA_FEATHER_FALL
    &Aura::HandleAuraHover,                                 //106 SPELL_AURA_HOVER
    &Aura::HandleAddModifier,                               //107 SPELL_AURA_ADD_FLAT_MODIFIER
    &Aura::HandleAddModifier,                               //108 SPELL_AURA_ADD_PCT_MODIFIER
    &Aura::HandleNoImmediateEffect,                         //109 SPELL_AURA_ADD_TARGET_TRIGGER
    &Aura::HandleModPowerRegenPCT,                          //110 SPELL_AURA_MOD_POWER_REGEN_PERCENT
    &Aura::HandleUnused,                                    //111 SPELL_AURA_ADD_CASTER_HIT_TRIGGER
    &Aura::HandleNoImmediateEffect,                         //112 SPELL_AURA_OVERRIDE_CLASS_SCRIPTS implemented in diff functions.
    &Aura::HandleNoImmediateEffect,                         //113 SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN implemented in Unit::MeleeDamageBonusTaken
    &Aura::HandleNoImmediateEffect,                         //114 SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN_PCT implemented in Unit::MeleeDamageBonusTaken
    &Aura::HandleNoImmediateEffect,                         //115 SPELL_AURA_MOD_HEALING                 implemented in Unit::SpellBaseHealingBonusTaken
    &Aura::HandleNoImmediateEffect,                         //116 SPELL_AURA_MOD_REGEN_DURING_COMBAT     imppemented in Player::RegenerateAll and Player::RegenerateHealth
    &Aura::HandleNoImmediateEffect,                         //117 SPELL_AURA_MOD_MECHANIC_RESISTANCE     implemented in Unit::MagicSpellHitResult
    &Aura::HandleNoImmediateEffect,                         //118 SPELL_AURA_MOD_HEALING_PCT             implemented in Unit::SpellHealingBonusTaken
    &Aura::HandleUnused,                                    //119 SPELL_AURA_SHARE_PET_TRACKING useless
    &Aura::HandleAuraUntrackable,                           //120 SPELL_AURA_UNTRACKABLE
    &Aura::HandleAuraEmpathy,                               //121 SPELL_AURA_EMPATHY
    &Aura::HandleModOffhandDamagePercent,                   //122 SPELL_AURA_MOD_OFFHAND_DAMAGE_PCT
    &Aura::HandleNoImmediateEffect,                         //123 SPELL_AURA_MOD_TARGET_RESISTANCE  implemented in Unit::CalculateAbsorbAndResist and Unit::CalcArmorReducedDamage
    &Aura::HandleAuraModRangedAttackPower,                  //124 SPELL_AURA_MOD_RANGED_ATTACK_POWER
    &Aura::HandleNoImmediateEffect,                         //125 SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN implemented in Unit::MeleeDamageBonusTaken
    &Aura::HandleNoImmediateEffect,                         //126 SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN_PCT implemented in Unit::MeleeDamageBonusTaken
    &Aura::HandleNoImmediateEffect,                         //127 SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS implemented in Unit::MeleeDamageBonusDone
    &Aura::HandleModPossessPet,                             //128 SPELL_AURA_MOD_POSSESS_PET
    &Aura::HandleAuraModIncreaseSpeed,                      //129 SPELL_AURA_MOD_SPEED_ALWAYS
    &Aura::HandleAuraModIncreaseMountedSpeed,               //130 SPELL_AURA_MOD_MOUNTED_SPEED_ALWAYS
    &Aura::HandleNoImmediateEffect,                         //131 SPELL_AURA_MOD_RANGED_ATTACK_POWER_VERSUS implemented in Unit::MeleeDamageBonusDone
    &Aura::HandleAuraModIncreaseEnergyPercent,              //132 SPELL_AURA_MOD_INCREASE_ENERGY_PERCENT
    &Aura::HandleAuraModIncreaseHealthPercent,              //133 SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT
    &Aura::HandleAuraModRegenInterrupt,                     //134 SPELL_AURA_MOD_MANA_REGEN_INTERRUPT
    &Aura::HandleModHealingDone,                            //135 SPELL_AURA_MOD_HEALING_DONE
    &Aura::HandleNoImmediateEffect,                         //136 SPELL_AURA_MOD_HEALING_DONE_PERCENT   implemented in Unit::SpellHealingBonusDone
    &Aura::HandleModTotalPercentStat,                       //137 SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE
    &Aura::HandleModMeleeSpeedPct,                          //138 SPELL_AURA_MOD_MELEE_HASTE
    &Aura::HandleForceReaction,                             //139 SPELL_AURA_FORCE_REACTION
    &Aura::HandleAuraModRangedHaste,                        //140 SPELL_AURA_MOD_RANGED_HASTE
    &Aura::HandleRangedAmmoHaste,                           //141 SPELL_AURA_MOD_RANGED_AMMO_HASTE
    &Aura::HandleAuraModBaseResistancePercent,              //142 SPELL_AURA_MOD_BASE_RESISTANCE_PCT
    &Aura::HandleAuraModResistanceExclusive,                //143 SPELL_AURA_MOD_RESISTANCE_EXCLUSIVE
    &Aura::HandleAuraSafeFall,                              //144 SPELL_AURA_SAFE_FALL                         implemented in WorldSession::HandleMovementOpcodes
    &Aura::HandleUnused,                                    //145 SPELL_AURA_CHARISMA obsolete?
    &Aura::HandleUnused,                                    //146 SPELL_AURA_PERSUADED obsolete?
    &Aura::HandleModMechanicImmunityMask,                   //147 SPELL_AURA_MECHANIC_IMMUNITY_MASK            implemented in Unit::IsImmuneToSpell and Unit::IsImmuneToSpellEffect (check part)
    &Aura::HandleAuraRetainComboPoints,                     //148 SPELL_AURA_RETAIN_COMBO_POINTS
    &Aura::HandleNoImmediateEffect,                         //149 SPELL_AURA_RESIST_PUSHBACK
    &Aura::HandleShieldBlockValue,                          //150 SPELL_AURA_MOD_SHIELD_BLOCKVALUE_PCT
    &Aura::HandleAuraTrackStealthed,                        //151 SPELL_AURA_TRACK_STEALTHED
    &Aura::HandleNoImmediateEffect,                         //152 SPELL_AURA_MOD_DETECTED_RANGE         implemented in Creature::GetAttackDistance
    &Aura::HandleNoImmediateEffect,                         //153 SPELL_AURA_SPLIT_DAMAGE_FLAT          implemented in Unit::CalculateAbsorbAndResist
    &Aura::HandleNoImmediateEffect,                         //154 SPELL_AURA_MOD_STEALTH_LEVEL          implemented in Unit::isVisibleForOrDetect
    &Aura::HandleNoImmediateEffect,                         //155 SPELL_AURA_MOD_WATER_BREATHING        implemented in Player::GetMaxTimer
    &Aura::HandleNoImmediateEffect,                         //156 SPELL_AURA_MOD_REPUTATION_GAIN        implemented in Player::CalculateReputationGain
    &Aura::HandleUnused,                                    //157 SPELL_AURA_PET_DAMAGE_MULTI (single test like spell 20782, also single for 214 aura)
    &Aura::HandleShieldBlockValue,                          //158 SPELL_AURA_MOD_SHIELD_BLOCKVALUE
    &Aura::HandleNoImmediateEffect,                         //159 SPELL_AURA_NO_PVP_CREDIT      only for Honorless Target spell
    &Aura::HandleNoImmediateEffect,                         //160 SPELL_AURA_MOD_AOE_AVOIDANCE                 implemented in Unit::MagicSpellHitResult
    &Aura::HandleNoImmediateEffect,                         //161 SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT
    &Aura::HandleAuraPowerBurn,                             //162 SPELL_AURA_POWER_BURN_MANA
    &Aura::HandleUnused,                                    //163 SPELL_AURA_MOD_CRIT_DAMAGE_BONUS
    &Aura::HandleUnused,                                    //164 useless, only one test spell
    &Aura::HandleNoImmediateEffect,                         //165 SPELL_AURA_MELEE_ATTACK_POWER_ATTACKER_BONUS implemented in Unit::MeleeDamageBonusDone
    &Aura::HandleAuraModAttackPowerPercent,                 //166 SPELL_AURA_MOD_ATTACK_POWER_PCT
    &Aura::HandleAuraModRangedAttackPowerPercent,           //167 SPELL_AURA_MOD_RANGED_ATTACK_POWER_PCT
    &Aura::HandleNoImmediateEffect,                         //168 SPELL_AURA_MOD_DAMAGE_DONE_VERSUS            implemented in Unit::SpellDamageBonusDone, Unit::MeleeDamageBonusDone
    &Aura::HandleNoImmediateEffect,                         //169 SPELL_AURA_MOD_CRIT_PERCENT_VERSUS           implemented in Unit::DealDamageBySchool, Unit::DoAttackDamage, Unit::SpellCriticalBonus
    &Aura::HandleNULL,                                      //170 SPELL_AURA_DETECT_AMORE       only for Detect Amore spell
    &Aura::HandleAuraModIncreaseSpeed,                      //171 SPELL_AURA_MOD_SPEED_NOT_STACK
    &Aura::HandleAuraModIncreaseMountedSpeed,               //172 SPELL_AURA_MOD_MOUNTED_SPEED_NOT_STACK
    &Aura::HandleUnused,                                    //173 SPELL_AURA_ALLOW_CHAMPION_SPELLS  only for Proclaim Champion spell
    &Aura::HandleModSpellDamagePercentFromStat,             //174 SPELL_AURA_MOD_SPELL_DAMAGE_OF_STAT_PERCENT  implemented in Unit::SpellBaseDamageBonusDone (in 1.12.* only spirit)
    &Aura::HandleModSpellHealingPercentFromStat,            //175 SPELL_AURA_MOD_SPELL_HEALING_OF_STAT_PERCENT implemented in Unit::SpellBaseHealingBonusDone (in 1.12.* only spirit)
    &Aura::HandleSpiritOfRedemption,                        //176 SPELL_AURA_SPIRIT_OF_REDEMPTION   only for Spirit of Redemption spell, die at aura end
    &Aura::HandleAuraAoeCharm,                              //177 SPELL_AURA_AOE_CHARM
    &Aura::HandleNoImmediateEffect,                         //178 SPELL_AURA_MOD_DEBUFF_RESISTANCE          implemented in Unit::MagicSpellHitResult
    &Aura::HandleNoImmediateEffect,                         //179 SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_CHANCE implemented in Unit::SpellCriticalBonus
    &Aura::HandleNoImmediateEffect,                         //180 SPELL_AURA_MOD_FLAT_SPELL_DAMAGE_VERSUS   implemented in Unit::SpellDamageBonusDone
    &Aura::HandleUnused,                                    //181 SPELL_AURA_MOD_FLAT_SPELL_CRIT_DAMAGE_VERSUS unused
    &Aura::HandleAuraModResistenceOfStatPercent,            //182 SPELL_AURA_MOD_RESISTANCE_OF_STAT_PERCENT
    &Aura::HandleNoImmediateEffect,                         //183 SPELL_AURA_MOD_CRITICAL_THREAT only used in 28746, implemented in ThreatCalcHelper::CalcThreat
    &Aura::HandleNoImmediateEffect,                         //184 SPELL_AURA_MOD_ATTACKER_MELEE_HIT_CHANCE  implemented in Unit::RollMeleeOutcomeAgainst
    &Aura::HandleNoImmediateEffect,                         //185 SPELL_AURA_MOD_ATTACKER_RANGED_HIT_CHANCE implemented in Unit::RollMeleeOutcomeAgainst
    &Aura::HandleNoImmediateEffect,                         //186 SPELL_AURA_MOD_ATTACKER_SPELL_HIT_CHANCE  implemented in Unit::MagicSpellHitResult
    &Aura::HandleNoImmediateEffect,                         //187 SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_CHANCE  implemented in Unit::GetUnitCriticalChance
    &Aura::HandleNoImmediateEffect,                         //188 SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_CHANCE implemented in Unit::GetUnitCriticalChance
    &Aura::HandleUnused,                                    //189 SPELL_AURA_MOD_RATING (not used in 1.12.1)
    &Aura::HandleNoImmediateEffect,                         //190 SPELL_AURA_MOD_FACTION_REPUTATION_GAIN     implemented in Player::CalculateReputationGain
    &Aura::HandleAuraModUseNormalSpeed,                     //191 SPELL_AURA_USE_NORMAL_MOVEMENT_SPEED
    // Nostalrius - custom
    &Aura::HandleAuraAuraSpell,
    &Aura::HandleNoImmediateEffect,                         //193 SPELL_AURA_MOD_TARGET_RESISTANCE_PERC  implemented in Unit::CalculateAbsorbAndResist and Unit::CalcArmorReducedDamage
};

static AuraType const frozenAuraTypes[] = { SPELL_AURA_MOD_ROOT, SPELL_AURA_MOD_STUN, SPELL_AURA_NONE };

Aura::Aura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target, Unit *caster, Item* castItem) :
    m_spellmod(nullptr), m_periodicTimer(0), m_periodicTick(0), m_removeMode(AURA_REMOVE_BY_DEFAULT),
    m_effIndex(eff), m_positive(false), m_isPeriodic(false), m_isAreaAura(false),
    m_isPersistent(false), m_in_use(0), m_spellAuraHolder(holder),
// NOSTALRIUS: auras exclusifs
    m_applied(false)
{
    MANGOS_ASSERT(target);
    MANGOS_ASSERT(spellproto && spellproto == sSpellMgr.GetSpellEntry(spellproto->Id) && "`info` must be pointer to a sSpellMgr element");
    ASSERT(spellproto->EffectApplyAuraName[eff]);

    m_currentBasePoints = currentBasePoints ? *currentBasePoints : spellproto->CalculateSimpleValue(eff);

    m_positive = spellproto->IsPositiveEffect(m_effIndex);
    m_applyTime = time(nullptr);

    int32 damage;
    if (!caster)
        damage = m_currentBasePoints;
    else
        damage = caster->CalculateSpellDamage(target, spellproto, m_effIndex, &m_currentBasePoints);

    damage *= GetStackAmount();

    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Aura: construct Spellid : %u, Aura : %u Target : %d Damage : %d", spellproto->Id, spellproto->EffectApplyAuraName[eff], spellproto->EffectImplicitTargetA[eff], damage);

    SetModifier(AuraType(spellproto->EffectApplyAuraName[eff]), damage, spellproto->EffectAmplitude[eff], spellproto->EffectMiscValue[eff]);
    CalculatePeriodic(caster ? caster->GetSpellModOwner() : NULL, true);
    ComputeExclusive();
    if (IsLastAuraOnHolder() && !m_positive)
    {
        // Exclude passive spells.
        if (holder->IsNeedVisibleSlot(caster))
            holder->CalculateForDebuffLimit();

        holder->CalculateHeartBeat(caster, target);
    }
}

void Aura::Refresh(Unit* caster, Unit* target, SpellAuraHolder* pRefreshWithHolder)
{
    Aura* pHolderAura = pRefreshWithHolder->GetAuraByEffectIndex(m_effIndex);
    if (!pHolderAura)
        return;
    m_periodicTick = 0;
    Player* modOwner = caster ? caster->GetSpellModOwner() : NULL;
    m_applyTime = time(nullptr);
    // Refresh periodic period, but keep current timer.
    // If we chain refresh a DoT, it should not prevent first damage tick!
	//v+ Defiler ?
    /*int32 oldPeriodicTimer = m_periodicTimer;
    CalculatePeriodic(modOwner, true);
    if (oldPeriodicTimer < m_periodicTimer)
        m_periodicTimer = oldPeriodicTimer;*/

    // Re-calculation du montant de degats
    if (IsApplied() || !IsExclusive())
    {
        bool lockStats = false;
        switch (GetSpellProto()->EffectApplyAuraName[m_effIndex])
        {
            case SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE:
                if (GetSpellProto()->Attributes & 0x10)
                    break;
            // no break
            case SPELL_AURA_MOD_STAT:
            case SPELL_AURA_MOD_PERCENT_STAT:
            case SPELL_AURA_MOD_INCREASE_HEALTH:
            case SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT: // Exemple : 27038
                lockStats = true;
                break;
        }
        if (lockStats)
            target->SetCanModifyStats(false);
        //unapply with the old modifiers and reapply with the new.
        ApplyModifier(false, true, false);
        // Refresh de quelques variables du modifier
        m_modifier.m_auraname = pHolderAura->GetModifier()->m_auraname;
        m_modifier.m_amount = pHolderAura->GetModifier()->m_amount;
        m_modifier.m_miscvalue = pHolderAura->GetModifier()->m_miscvalue;
        ApplyModifier(true, true, false);
        if (lockStats)
        {
            target->SetCanModifyStats(true);
            target->UpdateAllStats();
        }
    }
}

void SpellAuraHolder::Refresh(Unit* caster, Unit* target, SpellAuraHolder* pRefreshWithHolder)
{
    m_casterGuid = caster ? caster->GetObjectGuid() : target->GetObjectGuid();
    m_applyTime = time(nullptr);
    m_duration = pRefreshWithHolder->GetAuraDuration();
    m_maxDuration = pRefreshWithHolder->GetAuraMaxDuration();
    for (int i = 0 ; i < MAX_EFFECT_INDEX; ++i)
    {
        if (Aura* pAura = GetAuraByEffectIndex(SpellEffectIndex(i)))
            pAura->Refresh(caster, target, pRefreshWithHolder);
    }
    UpdateAuraDuration();
    UpdateAuraApplication();
}

bool SpellAuraHolder::CanBeRefreshedBy(SpellAuraHolder* other) const
{
    if (!other)
        return false;
    if (other->GetCasterGuid() != GetCasterGuid())
        return false;
    // Meme ID/Effet de sort
    if (other->GetId() != GetId())
        return false;
    if (m_spellProto->StackAmount) // Se stack
        return false;
    if (m_spellProto->procCharges) // Ou a des charges (fix bug visuel)
        return false;
    return true;
}

bool SpellAuraHolder::IsMoreImportantDebuffThan(SpellAuraHolder* other) const
{
    // Same category : last aura applies
    if (m_debuffLimitScore == other->m_debuffLimitScore)
        return m_applyTime > other->m_applyTime;
    // Else, compare categories
    return m_debuffLimitScore > other->m_debuffLimitScore;
}

Aura::~Aura()
{
}

AreaAura::AreaAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target,
                   Unit *caster, Item* castItem) : Aura(spellproto, eff, currentBasePoints, holder, target, caster, castItem)
{
    m_isAreaAura = true;

    // caster==NULL in constructor args if target==caster in fact
    Unit* caster_ptr = caster ? caster : target;

    m_radius = GetSpellRadius(sSpellRadiusStore.LookupEntry(spellproto->EffectRadiusIndex[m_effIndex]));
    if (Player* modOwner = caster_ptr->GetSpellModOwner())
        modOwner->ApplySpellMod(spellproto->Id, SPELLMOD_RADIUS, m_radius);

    switch (spellproto->Effect[eff])
    {
        case SPELL_EFFECT_APPLY_AREA_AURA_PARTY:
            m_areaAuraType = AREA_AURA_PARTY;
            if (target->GetCharmerOrOwnerOrSelf()->GetTypeId() == TYPEID_UNIT)
                m_areaAuraType = AREA_AURA_FRIEND;
            if (target->GetTypeId() == TYPEID_UNIT && ((Creature*)target)->IsTotem())
                m_modifier.m_auraname = SPELL_AURA_NONE;
            //Tranquil Air totem not applied when target is in Defensive Stance
            else if (spellproto->Id == 25909)
                if (target->HasAura(7376) || target->HasAura(21178) || target->HasAura(25780) || target->HasAura(34599))
                    m_modifier.m_auraname = SPELL_AURA_NONE;
            break;
        case SPELL_EFFECT_APPLY_AREA_AURA_RAID:
            m_areaAuraType = AREA_AURA_RAID;
            if (target->GetCharmerOrOwnerOrSelf()->GetTypeId() == TYPEID_UNIT)
                m_areaAuraType = AREA_AURA_FRIEND;
            if (target->GetTypeId() == TYPEID_UNIT && ((Creature*)target)->IsTotem())
                m_modifier.m_auraname = SPELL_AURA_NONE;
            // Light's Beacon not applied to caster itself (TODO: more generic check for another similar spell if any?)
            else if (target == caster_ptr && spellproto->Id == 53651)
                m_modifier.m_auraname = SPELL_AURA_NONE;
            break;
        case SPELL_EFFECT_APPLY_AREA_AURA_FRIEND:
            m_areaAuraType = AREA_AURA_FRIEND;
            // Styleen's Darling Shades not applied to caster itself
            if (target == caster_ptr && spellproto->Id == 35443)
                m_modifier.m_auraname = SPELL_AURA_NONE;
            break;
        case SPELL_EFFECT_APPLY_AREA_AURA_ENEMY:
            m_areaAuraType = AREA_AURA_ENEMY;
            if (target == caster_ptr)
                m_modifier.m_auraname = SPELL_AURA_NONE;    // Do not do any effect on self
            break;
        case SPELL_EFFECT_APPLY_AREA_AURA_PET:
            m_areaAuraType = AREA_AURA_PET;
            break;
        case SPELL_EFFECT_APPLY_AREA_AURA_OWNER:
            m_areaAuraType = AREA_AURA_OWNER;
            if (target == caster_ptr)
                m_modifier.m_auraname = SPELL_AURA_NONE;
            break;
        default:
            sLog.outError("Wrong spell effect in AreaAura constructor");
            MANGOS_ASSERT(false);
            break;
    }
}

AreaAura::~AreaAura()
{
}

PersistentAreaAura::PersistentAreaAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target,
                                       Unit *caster, Item* castItem) : Aura(spellproto, eff, currentBasePoints, holder, target, caster, castItem)
{
    m_isPersistent = true;
}

PersistentAreaAura::~PersistentAreaAura()
{
}

SingleEnemyTargetAura::SingleEnemyTargetAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target,
        Unit *caster, Item* castItem) : Aura(spellproto, eff, currentBasePoints, holder, target, caster, castItem)
{
    if (caster)
        m_castersTargetGuid = caster->GetTypeId() == TYPEID_PLAYER ? ((Player*)caster)->GetSelectionGuid() : caster->GetTargetGuid();
}

SingleEnemyTargetAura::~SingleEnemyTargetAura()
{
}

Unit* SingleEnemyTargetAura::GetTriggerTarget() const
{
    return ObjectAccessor::GetUnit(*(m_spellAuraHolder->GetTarget()), m_castersTargetGuid);
}

Aura* CreateAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder* holder, Unit* target, Unit* caster, Item* castItem)
{
    if (IsAreaAuraEffect(spellproto->Effect[eff]))
        return new AreaAura(spellproto, eff, currentBasePoints, holder, target, caster, castItem);

    return new Aura(spellproto, eff, currentBasePoints, holder, target, caster, castItem);
}

SpellAuraHolder* CreateSpellAuraHolder(SpellEntry const* spellproto, Unit* target, Unit* caster, WorldObject* realCaster, Item* castItem)
{
    return new SpellAuraHolder(spellproto, target, caster, castItem, realCaster);
}

void Aura::SetModifier(AuraType t, int32 a, uint32 pt, int32 miscValue)
{
    m_modifier.m_auraname = t;
    m_modifier.m_amount = a;
    m_modifier.m_miscvalue = miscValue;
    m_modifier.periodictime = pt;
}

void Aura::UpdatePeriodicTimer(int32 duration)
{
    if (m_isPeriodic)
    {
        // If duration gets changed, also change the periodic timer so that we
        // aren't losing out on ticks. Required for channeled spells that utilize
        // dynamic objects - don't want periodic timer initialized to the full
        // tick timer on entry into the aura
        // Avoid using modulo if we don't need to (slight perf increase)
        int32 newtick = (duration > m_modifier.periodictime) ? (duration % m_modifier.periodictime) : duration;
        // can be a divisible amount, in which case set it to the full period
        if (!newtick)
            newtick = m_modifier.periodictime;

        m_periodicTimer = newtick;
    }
}

void Aura::UpdateForAffected(uint32 diff)
{
    SetInUse(true);
    Aura::Update(diff);
    SetInUse(false);
}

void Aura::Update(uint32 diff)
{
    if (m_isPeriodic)
    {
        m_periodicTimer -= diff;

        if (m_periodicTimer <= 0) // tick also at m_periodicTimer==0 to prevent lost last tick in case max m_duration == (max m_periodicTimer)*N
        {
            // update before applying (aura can be removed in TriggerSpell or PeriodicTick calls)
            m_periodicTimer += m_modifier.periodictime;

            // Que Marks random bonus timer
            if ((GetId() == 34761 || GetId() == 34763 || GetId() == 34765 || GetId() == 34767) && m_effIndex == EFFECT_INDEX_0)
                m_periodicTimer += urand(0, 600000);

            ++m_periodicTick;                               // for some infinity auras in some cases can overflow and reset
            PeriodicTick();
        }
    }
}

// Called by caster of this area aura to tick auras for affected targets
void AreaAura::UpdateForAffected(uint32 diff)
{
    Aura::UpdateForAffected(diff);
}

void AreaAura::Update(uint32 diff)
{
    uint32 spellId = GetId();
    // update for the caster of the aura
    if (GetCasterGuid() == GetTarget()->GetObjectGuid())
    {
        Unit* caster = GetTarget();

        if (!caster->hasUnitState(UNIT_STAT_ISOLATED))
        {
            Unit* owner = caster->GetCharmerOrOwner();
            if (!owner)
                owner = caster;
            Spell::UnitList targets;

            switch (m_areaAuraType)
            {
                case AREA_AURA_PARTY:
                {
                    Group *pGroup = nullptr;

                    if (owner->GetTypeId() == TYPEID_PLAYER)
                        pGroup = ((Player*)owner)->GetGroup();

                    if (pGroup)
                    {
                        uint8 subgroup = ((Player*)owner)->GetSubGroup();
                        for (GroupReference *itr = pGroup->GetFirstMember(); itr != nullptr; itr = itr->next())
                        {
                            Player* Target = itr->getSource();
                            if (Target && Target->isAlive() && Target->GetSubGroup() == subgroup && (!Target->duel || owner == Target) && caster->IsFriendlyTo(Target))
                            {
                                if (caster->IsWithinDistInMap(Target, m_radius))
                                    targets.push_back(Target);
                                Pet *pet = Target->GetPet();
                                if (pet && pet->isAlive() && caster->IsWithinDistInMap(pet, m_radius))
                                    targets.push_back(pet);
                            }
                        }
                    }
                    else
                    {
                        // add owner
                        if (owner != caster && caster->IsWithinDistInMap(owner, m_radius))
                            targets.push_back(owner);
                        // add caster's pet
                        Unit* pet = caster->GetPet();
                        if (pet && caster->IsWithinDistInMap(pet, m_radius))
                            targets.push_back(pet);
                    }
                    break;
                }
                case AREA_AURA_RAID:
                {
                    Group *pGroup = nullptr;

                    if (owner->GetTypeId() == TYPEID_PLAYER)
                        pGroup = ((Player*)owner)->GetGroup();

                    if (pGroup)
                    {
                        for (GroupReference *itr = pGroup->GetFirstMember(); itr != nullptr; itr = itr->next())
                        {
                            Player* Target = itr->getSource();
                            if (Target && Target->isAlive() && caster->IsFriendlyTo(Target))
                            {
                                if (caster->IsWithinDistInMap(Target, m_radius))
                                    targets.push_back(Target);
                                Pet *pet = Target->GetPet();
                                if (pet && pet->isAlive() && caster->IsWithinDistInMap(pet, m_radius))
                                    targets.push_back(pet);
                            }
                        }
                    }
                    else
                    {
                        // add owner
                        if (owner != caster && caster->IsWithinDistInMap(owner, m_radius))
                            targets.push_back(owner);
                        // add caster's pet
                        Unit* pet = caster->GetPet();
                        if (pet && caster->IsWithinDistInMap(pet, m_radius))
                            targets.push_back(pet);
                    }
                    break;
                }
                case AREA_AURA_FRIEND:
                {
                    MaNGOS::AnyFriendlyUnitInObjectRangeCheck u_check(caster, m_radius);
                    MaNGOS::UnitListSearcher<MaNGOS::AnyFriendlyUnitInObjectRangeCheck> searcher(targets, u_check);
                    Cell::VisitAllObjects(caster, searcher, m_radius);
                    break;
                }
                case AREA_AURA_ENEMY:
                {
                    MaNGOS::AnyAoETargetUnitInObjectRangeCheck u_check(caster, caster, m_radius); // No GetCharmer in searcher
                    MaNGOS::UnitListSearcher<MaNGOS::AnyAoETargetUnitInObjectRangeCheck> searcher(targets, u_check);
                    Cell::VisitAllObjects(caster, searcher, m_radius);
                    break;
                }
                case AREA_AURA_OWNER:
                {
                    if (owner != caster && caster->IsWithinDistInMap(owner, m_radius))
                        targets.push_back(owner);
                    break;
                }
                case AREA_AURA_PET:
                {
                    if (owner != caster && caster->IsWithinDistInMap(owner, m_radius))
                        targets.push_back(owner);
                    break;
                }
            }

            for (Spell::UnitList::iterator tIter = targets.begin(); tIter != targets.end(); tIter++)
            {
                // default flag is to apply aura to current iteration target, set to 
                // false if an area aura of the same spellid exists on the target
                bool apply = true;

                Unit* target = *tIter;

                SpellEntry const* actualSpellInfo = sSpellMgr.SelectAuraRankForLevel(GetSpellProto(), target->getLevel());
                if (!actualSpellInfo)
                    continue;
                uint32 actualSpellId = actualSpellInfo->Id;

                // we need to ignore present caster self applied area auras sometimes
                // in cases where this is the only aura applied for this spell effect
                Unit::SpellAuraHolderBounds spair = target->GetSpellAuraHolderBounds(actualSpellId);
                for (Unit::SpellAuraHolderMap::const_iterator i = spair.first; i != spair.second; ++i)
                {
                    if (i->second->IsDeleted())
                        continue;

                    Aura *aur = i->second->GetAuraByEffectIndex(m_effIndex);

                    if (aur)
                    {
                        // If this unit is the caster, update the tick. Units won't tick their
                        // own area auras. Make sure we don't double tick if this unit is the caster
                        // though
                        if (i->second->GetCasterGuid() == GetCasterGuid() && i->second->GetTarget()->GetObjectGuid() != GetCasterGuid())
                        {
                            aur->UpdateForAffected(diff);
                        }

                        // in generic case not allow stacking area auras
                        apply = false;
                        break;
                    }
                    else
                        continue;
                }

                if (!apply)
                    continue;

                // Skip some targets (TODO: Might require better checks, also unclear how the actual caster must/can be handled)
                if (GetSpellProto()->AttributesEx3 & SPELL_ATTR_EX3_TARGET_ONLY_PLAYER && target->GetTypeId() != TYPEID_PLAYER)
                    continue;

                // No auras for totems except of Stoneclaw Shield
                if (((Creature*)target)->IsTotem())
                {
                    if (spellId == 35056 || spellId == 35057 || spellId == 35058 || spellId == 35059 || spellId == 35060 || spellId == 35061)
                    {
                        if (target->GetEntry() == 3579 || target->GetEntry() == 3911 || target->GetEntry() == 3912 || target->GetEntry() == 3913 || target->GetEntry() == 7398 || target->GetEntry() == 7399 || 
                            target->GetEntry() == 25061 || target->GetEntry() == 25062 || target->GetEntry() == 25063 || target->GetEntry() == 25064 || target->GetEntry() == 25065 || target->GetEntry() == 25066 || target->GetEntry() == 25067)
                        {
                            continue;
                        }
                    }
                    else
                        continue;
                }
                if (spellId == 35056 || spellId == 35057 || spellId == 35058 || spellId == 35059 || spellId == 35060 || spellId == 35061)
                    if (!((Creature*)target)->IsTotem())
                        continue;

                if (!(GetSpellProto()->AttributesEx2 & SPELL_ATTR_EX2_IGNORE_LOS))
                    if (!caster->IsWithinLOSInMap(target))
                        continue;

                // Scarlet Chargers
                if (spellId == 35869 || spellId == 35870 || spellId == 35871)
                {
                    if (spellId == 35869)
                        if (target->GetPositionY() > 1335.0f || target->GetPositionY() <= 1322.3f)
                            continue;
                    if (spellId == 35870)
                        if (target->GetPositionY() > 1322.3f || target->GetPositionY() <= 1313.3f)
                            continue;
                    if (spellId == 35871)
                        if (target->GetPositionY() > 1313.3f || target->GetPositionY() <= 1300.0f)
                            continue;

                    if (caster->GetOrientation() > 1.57f && caster->GetOrientation() < 4.71f)
                    {
                        if (caster->GetPositionX() - target->GetPositionX() > 2.0f)
                            continue;
                    }
                    else
                    {
                        if (target->GetPositionX() - caster->GetPositionX() > 2.0f)
                            continue;
                    }
                }

                //Total Control Vishas
                if (spellId == 35885)
                    if (!target->HasAura(35887))
                        continue;
                if (spellId == 35886)
                    if (target->HasAura(35887))
                        continue;

                if (spellId == 35939)   // Halls of Silence
                {
                    float tarX = target->GetPositionX();
                    float tarY = target->GetPositionY();
                    float tarZ = target->GetPositionZ();

                    const float minX = 527.5f;
                    const float maxX = 555.0f;
                    const float minY = 1324.5f;
                    const float maxY = 1352.0f;

                    bool targetInside =
                        tarX >= minX && tarX <= maxX &&
                        tarY >= minY && tarY <= maxY;

                    if (targetInside || tarY < 1313.44f || tarZ < 60.0f)
                        continue;
                }

                int32 actualBasePoints = m_currentBasePoints;
                // recalculate basepoints for lower rank (all AreaAura spell not use custom basepoints?)
                if (actualSpellInfo != GetSpellProto())
                    actualBasePoints = actualSpellInfo->CalculateSimpleValue(m_effIndex);

                SpellAuraHolder* holder = target->GetSpellAuraHolder(actualSpellId, GetCasterGuid());

                bool addedToExisting = true;
                if (!holder)
                {
                    holder = CreateSpellAuraHolder(actualSpellInfo, target, caster, caster);
                    addedToExisting = false;
                }

                AreaAura* aur = new AreaAura(actualSpellInfo, m_effIndex, &actualBasePoints, holder, target, caster, nullptr);
                holder->AddAura(aur, m_effIndex);

                if (!holder->IsPassive() && !holder->IsPermanent())
                {
                    // Aura duration has already been decremented in caster holder update, so re-add
                    // for the target's holder or it will be one diff in the future
                    holder->SetAuraDuration(GetAuraDuration() + diff);
                }

                // Caster's aura will tick at the end of this method, so subtract now to remain synced
                holder->RefreshAuraPeriodicTimers(m_periodicTimer - diff);

                if (addedToExisting)
                {
                    target->AddAuraToModList(aur);
                    holder->SetInUse(true);
                    aur->ApplyModifier(true, true);
                    holder->SetInUse(false);
                }
                else if (!target->AddSpellAuraHolder(holder))
                    holder = nullptr;

                DETAIL_LOG("Added aura %u to holder for spell %u on %s", m_effIndex, actualSpellId, target->GetName());

                // Add holder to spell if it's channeled so the updates are synced
                if (holder && IsChanneled() && !addedToExisting)
                {
                    if (Spell* spell = caster->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
                    {
                        spell->AddChanneledAuraHolder(holder);
                    }
                }
            }
        }
        Aura::Update(diff);
    }
    else                                                    // aura at non-caster
    {
        Unit* caster = GetCaster();
        Unit* target = GetTarget();

        // Aura updates in caster update, see above
        //Aura::Update(diff);

        // only do range/removal check, tick is done on caster update to remain in sync
        // remove aura if out-of-range from caster (after teleport for example)
        // or caster is isolated or caster no longer has the aura
        // or caster is (no longer) friendly
        bool needFriendly = (m_areaAuraType == AREA_AURA_ENEMY ? false : true);
        bool casterHasAura = false;
        if (caster)
        {
            Unit::SpellAuraHolderMap const& casterAuras = caster->GetSpellAuraHolderMap();
            for (Unit::SpellAuraHolderMap::const_iterator itr = casterAuras.begin(); itr != casterAuras.end(); ++itr)
            {
                SpellAuraHolder* casterHolder = itr->second;
                if (!casterHolder || casterHolder->IsDeleted())
                    continue;

                if (casterHolder->GetCasterGuid() != GetCasterGuid())
                    continue;

                uint32 casterSpellId = casterHolder->GetId();
                if (casterSpellId != spellId && !sSpellMgr.IsHighRankOfSpell(casterSpellId, spellId))
                    continue;

                if (casterHolder->GetAuraByEffectIndex(GetEffIndex()))
                {
                    casterHasAura = true;
                    break;
                }
            }
        }

        if (!caster || caster->hasUnitState(UNIT_STAT_ISOLATED) ||
            !caster->IsWithinDistInMap(target, m_radius) ||
            !casterHasAura ||
            caster->IsFriendlyTo(target) != needFriendly ||
            (!(GetSpellProto()->AttributesEx2 & SPELL_ATTR_EX2_IGNORE_LOS) && !caster->IsWithinLOSInMap(target)))
        {
            target->RemoveSingleAuraFromSpellAuraHolder(spellId, GetEffIndex(), GetCasterGuid(), AURA_REMOVE_BY_RANGE);
        }
        else if (m_areaAuraType == AREA_AURA_PARTY)         // check if in same sub group
        {
            // not check group if target == owner or target == pet
            if (caster->GetCharmerOrOwnerGuid() != target->GetObjectGuid() && caster->GetObjectGuid() != target->GetCharmerOrOwnerGuid())
            {
                Player* check = caster->GetCharmerOrOwnerPlayerOrPlayerItself();

                Group *pGroup = check ? check->GetGroup() : NULL;
                if (pGroup)
                {
                    Player* checkTarget = target->GetCharmerOrOwnerPlayerOrPlayerItself();
                    if (!checkTarget || !pGroup->SameSubGroup(check, checkTarget))
                        target->RemoveSingleAuraFromSpellAuraHolder(spellId, GetEffIndex(), GetCasterGuid(), AURA_REMOVE_BY_GROUP);
                }
                else
                    target->RemoveSingleAuraFromSpellAuraHolder(spellId, GetEffIndex(), GetCasterGuid(), AURA_REMOVE_BY_GROUP);
            }
        }
        else if (m_areaAuraType == AREA_AURA_PET)
        {
            if (target->GetObjectGuid() != caster->GetCharmerOrOwnerGuid())
                target->RemoveSingleAuraFromSpellAuraHolder(spellId, GetEffIndex(), GetCasterGuid(), AURA_REMOVE_BY_GROUP);
        }
    }
}

void PersistentAreaAura::Update(uint32 diff)
{
    // remove the aura if its caster or the dynamic object causing it was removed
    // or if the target moves too far from the dynamic object

    bool remove = false;
    uint32 spellId = GetId();
    // Nostalrius: piege explosif. Ne doit pas etre retire lorsqu'on sort de la zone.
    if (spellId != 13812 && spellId != 14314 && spellId != 14315)
    {
        remove = true;
        if (WorldObject* caster = GetRealCaster())
        {
            std::vector<DynamicObject*> dynObjs;
            caster->GetDynObjects(spellId, GetEffIndex(), dynObjs);
            Unit* pUnitTarget = GetTarget();
            for (DynamicObject* obj : dynObjs)
            {
                if (pUnitTarget->IsWithinDistInMap(obj, obj->GetRadius()))
                    remove = false;
                else
                    obj->RemoveAffected(pUnitTarget);           // let later reapply if target return to range
            }
        }
    }

    // Note: Unit may have been removed from the world (mid-teleport) during this
    // update (since the caster updates area auras). We shouldn't be ticking it
    // when the target is out of range anyway.
    if (remove)
        GetTarget()->RemoveSingleAuraFromSpellAuraHolder(GetHolder(), m_effIndex, AURA_REMOVE_BY_RANGE);
    else
        Aura::Update(diff);
}

void Aura::ApplyModifier(bool apply, bool Real, bool skipCheckExclusive)
{
    // Dans Unit::RemoveAura, ApplyModifier est toujours appelle.
    if (IsApplied() == apply)
        return;
    AuraType aura = m_modifier.m_auraname;

    GetHolder()->SetInUse(true);
    SetInUse(true);
    // NOSTALRIUS: Auras exclusifs.
    if (apply && !skipCheckExclusive && IsExclusive() && !ExclusiveAuraCanApply())
    {
        GetHolder()->SetInUse(false);
        SetInUse(false);
        return;
    }
    m_applied = apply;
    if (aura < TOTAL_AURAS)
        (*this.*AuraHandler [aura])(apply, Real);

    if (!apply && !skipCheckExclusive && IsExclusive())
        ExclusiveAuraUnapply();

    SetInUse(false);
    GetHolder()->SetInUse(false);
}

bool Aura::isAffectedOnSpell(SpellEntry const *spell) const
{
    if (m_spellmod)
        return m_spellmod->isAffectedOnSpell(spell);

    // Check family name
    if (spell->SpellFamilyName != GetSpellProto()->SpellFamilyName)
        return false;

    uint64 mask = sSpellMgr.GetSpellAffectMask(GetId(), GetEffIndex());
    return spell->IsFitToFamilyMask(mask);
}

bool Aura::CanProcFrom(SpellEntry const *spell, uint32 EventProcEx, uint32 procEx, bool active, bool useClassMask) const
{
    // Check EffectClassMask (in pre-3.x stored in spell_affect in fact)
    uint64 mask = sSpellMgr.GetSpellAffectMask(GetId(), GetEffIndex());

    // Nostalrius: c'est la moindre des choses d'utiliser un peu 'spell_proc_event' non ?
    if (!mask)
        if (SpellProcEventEntry const* entry = sSpellMgr.GetSpellProcEvent(GetId()))
            mask = entry->spellFamilyMask[GetEffIndex()];

    // if no class mask defined, or spell_proc_event has SpellFamilyName=0 - allow proc
    if (!useClassMask || !mask)
    {
        if (!(EventProcEx & PROC_EX_EX_TRIGGER_ALWAYS))
        {
            // Check for extra req (if none) and hit/crit
            if (EventProcEx == PROC_EX_NONE)
            {
                // No extra req, so can trigger only for active (damage/healing present) and hit/crit
                if ((procEx & (PROC_EX_NORMAL_HIT | PROC_EX_CRITICAL_HIT)) && active)
                    return true;
                else
                    return false;
            }
            else // Passive spells hits here only if resist/reflect/immune/evade
            {
                // Passive spells can`t trigger if need hit (exclude cases when procExtra include non-active flags)
                if ((EventProcEx & (PROC_EX_NORMAL_HIT | PROC_EX_CRITICAL_HIT) & procEx) && !active)
                    return false;
            }
        }
        return true;
    }
    else
    {
        // SpellFamilyName check is performed in SpellMgr::IsSpellProcEventCanTriggeredBy and it is done once for whole holder
        // note: SpellFamilyName is not checked if no spell_proc_event is defined
        return !!(mask & spell->SpellFamilyFlags);
    }
}

void Aura::ReapplyAffectedPassiveAuras(Unit* target)
{
    // we need store cast item guids for self casted spells
    // expected that not exist permanent auras from stackable auras from different items
    std::map<uint32, ObjectGuid> affectedSelf;

    for (Unit::SpellAuraHolderMap::const_iterator itr = target->GetSpellAuraHolderMap().begin(); itr != target->GetSpellAuraHolderMap().end(); ++itr)
    {
        // permanent passive
        // passive spells can be affected only by own or owner spell mods)
        if (itr->second->IsPassive() && itr->second->IsPermanent() &&
                // non deleted and not same aura (any with same spell id)
                !itr->second->IsDeleted() && itr->second->GetId() != GetId() &&
                // and affected by aura
                itr->second->GetCasterGuid() == target->GetObjectGuid() &&
                // and affected by spellmod
                isAffectedOnSpell(itr->second->GetSpellProto()))
            affectedSelf[itr->second->GetId()] = itr->second->GetCastItemGuid();
    }

    if (!affectedSelf.empty())
    {
        Player* pTarget = target->GetTypeId() == TYPEID_PLAYER ? (Player*)target : NULL;

        for (std::map<uint32, ObjectGuid>::const_iterator map_itr = affectedSelf.begin(); map_itr != affectedSelf.end(); ++map_itr)
        {
            Item* item = pTarget && map_itr->second ? pTarget->GetItemByGuid(map_itr->second) : NULL;
            target->RemoveAurasDueToSpell(map_itr->first);
            target->CastSpell(target, map_itr->first, true, item);
        }
    }
}

struct ReapplyAffectedPassiveAurasHelper
{
    explicit ReapplyAffectedPassiveAurasHelper(Aura* _aura) : aura(_aura) {}
    void operator()(Unit* unit) const
    {
        aura->ReapplyAffectedPassiveAuras(unit);
    }
    Aura* aura;
};

void Aura::ReapplyAffectedPassiveAuras()
{
    // not reapply spell mods with charges (use original value because processed and at remove)
    if (GetSpellProto()->procCharges)
        return;

    // not reapply some spell mods ops (mostly speedup case)
    switch (m_modifier.m_miscvalue)
    {
        case SPELLMOD_DURATION:
        case SPELLMOD_CHARGES:
        case SPELLMOD_NOT_LOSE_CASTING_TIME:
        case SPELLMOD_CASTING_TIME:
        case SPELLMOD_COOLDOWN:
        case SPELLMOD_COST:
        case SPELLMOD_ACTIVATION_TIME:
        case SPELLMOD_GCD:
        case SPELLMOD_SPEED:
        case SPELLMOD_HASTE:
        case SPELLMOD_ATTACK_POWER:
            return;
    }

    // reapply talents to own passive persistent auras
    ReapplyAffectedPassiveAuras(GetTarget());

    // re-apply talents/passives/area auras applied to pet/totems (it affected by player spellmods)
    GetTarget()->CallForAllControlledUnits(ReapplyAffectedPassiveAurasHelper(this), CONTROLLED_PET | CONTROLLED_TOTEMS);
}

/*********************************************************/
/***               BASIC AURA FUNCTION                 ***/
/*********************************************************/
void Aura::HandleAddModifier(bool apply, bool Real)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER || !Real)
        return;

    if (m_modifier.m_miscvalue >= MAX_SPELLMOD)
        return;


    switch (GetSpellProto()->Id)
    {
        //Imp Slammer Shield Slam
        case 20497:
        {
            if (apply)
            {
                if (!GetTarget()->HasAura(35800))
                    GetTarget()->AddAura(35800);
            }
            else
            {
                GetTarget()->RemoveAurasDueToSpell(35800);
            }
        }
        //Defiler's extra target
        case 34305:
        {
            if (apply)
            {
                if (!GetTarget()->HasAura(35731))
                    GetTarget()->AddAura(35731);
            }
            else
            {
                GetTarget()->RemoveAurasDueToSpell(35731);
            }
        }
        break;
    }

    //Inevitable Doom 2nd component
    if (GetSpellProto()->SpellIconID == 91)
    {
        int32 DoomSpell = 0;
        switch (GetSpellProto()->Id)
        {
        case 33933:
            DoomSpell = 35389;
            break;
        case 33934:
            DoomSpell = 35390;
            break;
        case 33935:
            DoomSpell = 35391;
            break;
        case 33936:
            DoomSpell = 35392;
            break;
        case 33937:
            DoomSpell = 35393;
            break;
        }

        if (DoomSpell > 0)
        {
            if (apply)
            {
                if (!GetTarget()->HasAura(DoomSpell))
                    GetTarget()->AddAura(DoomSpell);
            }
            else
                GetTarget()->RemoveAurasDueToSpell(DoomSpell);
        }
    }


    if (apply)
    {
        // Add custom charges for some mod aura
        switch (GetSpellProto()->Id)
        {
            case 17941:                                     // Shadow Trance
            case 22008:                                     // Netherwind Focus
                GetHolder()->SetAuraCharges(1);
                break;
            case 23726:                                     // Venomous Totem Penetration
                GetTarget()->AddAura(35358);
                break;
        }

        m_spellmod = new SpellModifier(
            SpellModOp(m_modifier.m_miscvalue),
            SpellModType(m_modifier.m_auraname),            // SpellModType value == spell aura types
            m_modifier.m_amount,
            this,
            // prevent expire spell mods with (charges > 0 && m_stackAmount > 1)
            // all this spell expected expire not at use but at spell proc event check
            GetSpellProto()->StackAmount > 1 ? 0 : GetHolder()->GetAuraCharges());
    }
    else
    {
        //Dragonstalker remove AspeCt
        if (GetId() == 34551)
        {
            if (Unit* tTarget = GetTarget())
            {
                if (tTarget->HasAura(25296))
                {
                    tTarget->RemoveAurasDueToSpell(25296);
                }
                else if (tTarget->HasAura(14322))
                {
                    tTarget->RemoveAurasDueToSpell(14322);
                }
                else if (tTarget->HasAura(13163))
                {
                    tTarget->RemoveAurasDueToSpell(13163);
                }
                tTarget->RemoveAurasDueToSpellByCancel(34551);
            }
        }
        //Might remove Battle Shout
        if (GetId() == 23563 || GetId() == 34836)
        {
            if (Unit* tTarget = GetTarget())
            {
                if (tTarget->HasAura(25289))
                {
                    tTarget->RemoveAurasDueToSpell(25289);
                }
                else if (tTarget->HasAura(11551))
                {
                    tTarget->RemoveAurasDueToSpell(11551);
                }
            }
        }
        
        //Libram of Truth remove Devotion Aura
        if (GetId() == 27850)
        {
            if (Unit* tTarget = GetTarget())
            {
                if (tTarget->HasAura(10293)) //r7
                {
                    tTarget->RemoveAurasDueToSpell(10293);
                }
                else if (tTarget->HasAura(10292)) //r6
                {
                    tTarget->RemoveAurasDueToSpell(10292);
                }
            }
        }

    }

    ASSERT(m_spellmod);
    ((Player*)GetTarget())->AddSpellMod(m_spellmod, apply);
    if (!apply)
        m_spellmod = nullptr; // Deleted in Player::AddSpellMod.

    ReapplyAffectedPassiveAuras();
}

void Aura::TriggerSpell()
{
    ObjectGuid casterGUID = GetCasterGuid();
    Unit* triggerTarget = GetTriggerTarget();

    if (!casterGUID || !triggerTarget)
        return;

    // generic casting code with custom spells and target/caster customs
    uint32 trigger_spell_id = GetSpellProto()->EffectTriggerSpell[m_effIndex];

    SpellEntry const *triggeredSpellInfo = sSpellMgr.GetSpellEntry(trigger_spell_id);
    SpellEntry const *auraSpellInfo = GetSpellProto();
    uint32 auraId = auraSpellInfo->Id;
    Unit* target = GetTarget();

    uint32 spellRandom;

    // not in banished state
    if (triggerTarget->hasUnitState(UNIT_STAT_ISOLATED))
        return;

    // specific code for cases with no trigger spell provided in field
    if (triggeredSpellInfo == nullptr)
    {
        switch (auraSpellInfo->SpellFamilyName)
        {
            case SPELLFAMILY_GENERIC:
            {
                switch (auraId) //Chain rea
                {
                    case 36306: // Locket of Devotion
                    {
                        if (Aura* aura = target->GetAura(36306, EFFECT_INDEX_0))
                            if (!aura->GetCaster())
                                target->RemoveAurasDueToSpell(36306);

                        return;
                    }
                    case 36236: // Brigitte's Cross
                    {
                        int32 HP = target->GetStat(STAT_SPIRIT);
                        target->CastCustomSpell(target, 36237, &HP, nullptr, nullptr, true, nullptr, this);

                        return;
                    }
                    case 36211: //Abbendis Archers shooting
                    {
                        if (target->isInCombat() || !target->IsCreature())
                            return;

                        uint32 ArcherGuid = target->ToCreature()->GetDBTableGUIDLow();
                        uint32 targetEntry = 0;

                        switch (ArcherGuid)
                        {
                            case 98285: targetEntry = 25261; break;
                            case 98286: targetEntry = 25262; break;
                            case 98287: targetEntry = 25263; break;
                            case 98288: targetEntry = 25264; break;
                            default: return;
                        }

                        if (Creature* pTarget = target->FindNearestCreature(targetEntry, 150.0f, true))
                            target->CastSpell(pTarget, 36212, false);

                        return;
                    }
                    case 36161: //Char re
                    {
                        if (target->IsCreature() && target->getStandState() == UNIT_STAND_STATE_STAND)
                            target->ToCreature()->SeatCreatureOnNearestChair(1.5f);

                        return;
                    }
                    case 36207: // Do Prophecy
                    {
                        if (urand(0, 2) == 0)
                            return;

                        if (target->isInCombat() || target->HasAura(36208))
                            return;

                        uint32 SpellId = urand(0, 29);

                        int32 const Prophecy[30][2] =
                        {
                            {36175, -1900271}, // Immunity Holy - The Light will not sear you.
                            {36176, -1900272}, // Immunity Arcane - Arcane fails to reach you.
                            {36177, -1900273}, // Immunity Fire - You walk the fire unscathed
                            {36178, -1900274}, // Immunity Nature - Nature cannot harm you.
                            {36179, -1900275}, // Immunity Frost - Winter cannot freeze your soul.
                            {36180, -1900276}, // Immunity Shadow - Shadow will not engulf you
                            {36181, -1900277}, // Immunity Spells - All magic will fail against you.
                            {36182, -1900278}, // Immunity Physical - No steel will taste your blood.
                            {36183, -1900279}, // Immunity All - You walk between the rain.
                            {36184, -1900280}, // No Mitigation - Your skin is but a veil.
                            {36185, -1900281}, // 75% Mitigation - You stand while mountains fall.
                            {36186, -1900282}, // Unfettered - No chain can hold you.
                            {36187, -1900283}, // Weakness - Your strikes leave no mark.
                            {36188, -1900284}, // Death +300% - Your shroud is already woven!
                            {36189, -1900285}, // Death Immunity - Death averts its gaze!
                            {36190, -1900286}, // Vitality - Your life will not fade.
                            {36191, -1900287}, // Beyond Time - Time bends to your stride.
                            {36192, -1900288}, // Mitigation Penetration - Nothing will stand before you.
                            {36193, -1900289}, // Execution - Your scythe reaps them all.
                            {36194, -1900290}, // Slow - Your steps come too late.
                            {36195, -1900291}, // Beacon - You are a beacon for their malice.
                            {36196, -1900292}, // Agony - Agony is your constant guest.
                            {36197, -1900293}, // Helpless - No one will save you.
                            {36198, -1900294}, // Double-Edged - Power, paid in blood.
                            {36199, -1900295}, // Doppelganger - One shall become two.
                            {36200, -1900296}, // Decider - Your first blow will decide all.
                            {36201, -1900297}, // Instability - Fate is unstable.
                            {36202, -1900298}, // Shared Fate - Your fate is not yours alone.
                            {36203, -1900299}, // Terrifying - Terror paves your way.
                            {36204, -1900300}  // At the Edge - Only at the edge will you prevail.
                        };

                        std::list<Creature*> prophecyList;
                        GetCreatureListExceptInGrid(prophecyList, target, true, 62, 10.0f);

                        // Check for the last 3 casts
                        for (std::list<Creature*>::iterator itr = prophecyList.begin(); itr != prophecyList.end(); ++itr)
                        {
                            if (!(*itr) || (*itr) == target || (*itr)->IsTemporarySummon() || !(*itr)->isAlive() || (*itr)->IsMoving() || !(*itr)->IsWithinLOSInMap(target))
                                continue;
                            
                            if ((*itr)->HasAura(Prophecy[SpellId][0]))
                                return;
                        }

                        for (std::list<Creature*>::iterator itr = prophecyList.begin(); itr != prophecyList.end(); ++itr)
                        {
                            Creature* pTarget = *itr;

                            if (!(*itr) || (*itr) == target || (*itr)->IsTemporarySummon() || !(*itr)->isAlive() || (*itr)->IsMoving() || !(*itr)->IsWithinLOSInMap(target) || (*itr)->HasAura(36209))
                                continue;

                            for (size_t i = 0; i < 30; ++i)
                                (*itr)->RemoveAurasDueToSpell(36175 + i);


                            if (!target->HasInArc(M_PI / 18.0, (*itr)))
                            {
                                target->SetFacingToObject((*itr));
                                target->SetTargetGuid((*itr)->GetObjectGuid());
                            }


                            DoScriptText(Prophecy[SpellId][1], target);
                            target->CastSpell(target, 36208, true);  // self cd
                            target->CastSpell((*itr), 36209, true);  // target cd
                            target->CastSpell((*itr), Prophecy[SpellId][0], false);
                            return;

                        }

                        return;
                    }
                    //AC mounts
                    case 35328:
                    case 35329:
                    {
                        if (target->isInCombat() || !target->GetTerrain()->IsOutdoors(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ()))
                            return;

                        if (GetId() == 35328)
                            if (!target->HasAura(35330))
                                if (target->GetDistance(1184.4f, 419.0f, 351.3f) > 80.0f)
                                    target->CastSpell(target, 35330, true);
                        if (GetId() == 35329)
                            if (!target->HasAura(35331))
                                target->CastSpell(target, 35331, true);

                        return;
                    }
                    case 36203: // Prophecy: Terrifying
                    {
                        if (!target->isInCombat())
                            return;

                        if (urand(0, 1) == 0)
                            target->CastSpell(target, 36206, true);

                        return;
                    }
                    case 36174: //Chatting Base
                    {
                        if (!target->IsMoving())
                        {
                            static uint32 emotes[] =
                            {
                                EMOTE_ONESHOT_TALK,
                                EMOTE_ONESHOT_TALK,
                                EMOTE_ONESHOT_TALK,
                                EMOTE_ONESHOT_TALK,
                                EMOTE_ONESHOT_EXCLAMATION,
                                EMOTE_ONESHOT_QUESTION
                            };

                            if (urand(0, 2) == 0)
                            {
                                target->HandleEmoteCommand(emotes[urand(0, 5)]);
                            }
                        }

                        return;
                    }
                    case 36173: //Drinking Base
                    {
                        if (!target->IsMoving())
                        {
                            if (urand(0, 3) == 0)
                                target->CastSpell(target, 11009, true);
                            else
                            {
                                static uint32 emotes[] =
                                {
                                    EMOTE_ONESHOT_TALK,
                                    EMOTE_ONESHOT_TALK,
                                    EMOTE_ONESHOT_TALK,
                                    EMOTE_ONESHOT_TALK,
                                    EMOTE_ONESHOT_EXCLAMATION,
                                    EMOTE_ONESHOT_EXCLAMATION,
                                    EMOTE_ONESHOT_EXCLAMATION,
                                    EMOTE_ONESHOT_LAUGH,
                                    EMOTE_ONESHOT_LAUGH,
                                    EMOTE_ONESHOT_QUESTION
                                };

                                if (urand(0, 2) == 0)
                                {
                                    target->HandleEmoteCommand(emotes[urand(0, 9)]);
                                }
                            }
                        }

                        return;
                    }
                    case 36166: //Lovestruck Base
                    {
                        bool FoundLove = false;
                        std::list<Creature*> nearbyAlliesAny;

                        GetCreatureListExceptInGrid(nearbyAlliesAny, target, true, 63, 100.0f);

                        for (std::list<Creature*>::iterator itr = nearbyAlliesAny.begin(); itr != nearbyAlliesAny.end(); ++itr)
                        {
                            Creature* pAlly = *itr;

                            if (!pAlly || pAlly == target || !target->IsInMap(pAlly) || !pAlly->HasAura(36166) || target->isDead())
                                continue;

                            FoundLove = true;

                            if (pAlly->isDead())
                            {
                                pAlly->RemoveAurasDueToSpell(36166);
                                FoundLove = false;
                                break;
                            }
                            else if (!target->isInCombat() && !target->IsMoving() && !pAlly->IsMoving() && !target->HasInArc(M_PI / 18.0, pAlly))
                            {
                                target->SetFacingToObject(pAlly);

                                if (target->GetDisplayId() == pAlly->GetDisplayId())
                                {
                                    if (CreatureModelInfo const* modelInfo = sObjectMgr.GetCreatureModelInfo(target->GetDisplayId()))
                                    {
                                        if (modelInfo->modelid_other_gender)
                                            target->SetDisplayId(modelInfo->modelid_other_gender);
                                    }
                                }
                            }
                            break;
                        }

                        if (!FoundLove)
                        {
                            target->RemoveAurasDueToSpell(36166);
                            target->CastSpell(target, 36167, true);
                            target->CastSpell(target, 36169, false);
                        }


                        return;
                    }
                    case 36167:
                    {
                        if (!target->isInCombat() && !target->IsMoving())
                            target->HandleEmoteCommand(EMOTE_ONESHOT_CRY);

                        return;
                    }
                    case 36019:
                    {
                        if (Unit* pCaster = GetCaster())
                            pCaster->CastSpell(target, 36020, true);

                        return;
                    }
                    case 36156:     // Polymorph Event SM Mages
                    {
                        if (Unit* pCaster = GetCaster())
                        {
                            std::list<Creature*> list;
                            GetCreatureListWithEntryInGrid(list, pCaster, 25208, 10.0f);

                            for (Creature* PolyTarget : list)
                            {
                                if (!PolyTarget || PolyTarget == pCaster)
                                    continue;

                                if (!PolyTarget->HasAura(36156))
                                    continue;

                                if (!pCaster->isInCombat() && !PolyTarget->isInCombat())
                                {
                                    if (!PolyTarget->HasAura(36157) && !pCaster->HasAura(36159))    //Poly CD target && No poly @caster
                                    {
                                        pCaster->CastSpell(PolyTarget, 36159, false);    //Poly
                                    }
                                }
                                break;
                            }
                        }

                        return;
                    }
                    case 36031:     // Panic triggers Fear
                    {
                        if (SpellAuraHolder* SpellHolder = target->GetSpellAuraHolder(36031))
                        {
                            if (SpellHolder->GetStackAmount() == 10)
                            {
                                target->RemoveAurasDueToSpell(36030);
                                target->RemoveAurasDueToSpell(36031);
                                target->AddAura(36032);
                            }
                        }

                        return;
                    }
                    case 36030:     // Panic Aura
                    {
                        if (!target->HasAura(36026))
                        {
                            target->RemoveAurasDueToSpell(36030);
                            target->RemoveAurasDueToSpell(36031);
                            return;
                        }

                        target->CastSpell(target, 36031, true);

                        if (target->HasAura(36027))     // Claustrophobia 20% extra stack
                            if (urand(0, 4) == 0)
                                target->CastSpell(target, 36031, true);
                        return;
                    }
                    case 36214: // Wall of Flesh visual
                    {
                        if (!target->HasAura(36026))
                            target->RemoveAurasDueToSpell(36214);

                        return;
                    }
                    case 36026: // Wall of Flesh/Stomp
                    {
                        int32 FriendDamage = target->GetStat(STAT_STAMINA) + target->GetStat(STAT_STRENGTH) / 2;

                        if (target->IsMoving())
                            FriendDamage *= 1.5;

                        if (target->HasAura(36027))     // Claustrophobia
                            FriendDamage *= 1.5;

                        target->CastCustomSpell(target, 36028, &FriendDamage, nullptr, nullptr, true, nullptr, this);

                        if (!target->HasAura(36030))
                            target->AddAura(36030);
                        return;
                    }
                    case 36000: // Searing Heat Friendly
                    {
                        if (target->IsCreature() && target->GetEntry() == 25249)
                        {
                            if (Creature* Crea = target->FindNearestCreature(25250, 0.5f))
                            {
                            }
                            else
                            {
                                target->SummonCreature(25250,
                                    target->GetPositionX(),
                                    target->GetPositionY(),
                                    target->GetPositionZ(),
                                    target->GetOrientation(),
                                    TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,
                                    600000);
                            }
                        }
                        return;
                    }
                    case 35985: //Frozen Solid
                    {
                        target->RemoveAurasDueToSpell(35984);
                        target->AddAura(35986);
                        return;
                    }
                    case 35979: // Numbing Cold time
                    {
                        if (!target->HasAura(35978))
                        {
                            target->RemoveAurasDueToSpell(35979);
                            target->RemoveAurasDueToSpell(35980);
                            target->RemoveAurasDueToSpell(35981);
                            target->RemoveAurasDueToSpell(35982);
                            target->RemoveAurasDueToSpell(35984);
                            target->RemoveAurasDueToSpell(35986);
                        }

                        return;
                    }
                    case 35978: // Numbing Cold
                    {
                        if (target->HasAura(35984) || target->HasAura(35986))
                            return;

                        if (target->HasAura(35980))
                        {
                            target->AddAura(35981);
                            target->RemoveAurasDueToSpell(35980);
                        }
                        else if (target->HasAura(35981))
                        {
                            target->AddAura(35982);
                            target->RemoveAurasDueToSpell(35981);
                        }
                        else if (target->HasAura(35982))
                        {
                            target->AddAura(35984);
                            target->RemoveAurasDueToSpell(35982);
                        }
                        else 
                            target->AddAura(35980);

                        target->CastSpell(target, 35979, true);

                        return;
                    }
                    case 35939: //Remove Halls of Silence
                    {
                        float tarX = target->GetPositionX();
                        float tarY = target->GetPositionY();
                        float tarZ = target->GetPositionZ();

                        const float minX = 527.5f;
                        const float maxX = 555.0f;
                        const float minY = 1324.5f;
                        const float maxY = 1352.0f;

                        bool targetInside =
                            tarX >= minX && tarX <= maxX &&
                            tarY >= minY && tarY <= maxY;

                        if (targetInside || tarY < 1313.44f || tarZ < 60.0f)
                            target->RemoveAurasDueToSpell(35939);

                        return;
                    }
                    // Herod remove dummy auras
                    case 35930:
                    case 35931:
                    {
                        if (!GetCaster() || !GetCaster()->isInCombat())
                        {
                            target->RemoveAurasDueToSpell(35930);
                            target->RemoveAurasDueToSpell(35931);
                        }

                        return;
                    }
                    case 35903: //Sentry Totem ping Invisibility/Enemy check
                    {
                        if (Unit* NearestEnemy = target->FindNearestEnemy(8.0f))
                        {
                            if (Unit* owner = target->GetOwner())
                            {
                                if (owner->IsPlayer())
                                {
                                    if (owner->GetDistance(target) > 100.0f)
                                        owner->CastSpell(owner, 35906, true);   // Camera Shake

                                    WorldPacket data(MSG_MINIMAP_PING, (8 + 4 + 4));
                                    data << target->GetObjectGuid();
                                    data << float(target->GetPositionX());
                                    data << float(target->GetPositionY());

                                    if (owner->ToPlayer()->GetGroup())
                                        owner->ToPlayer()->GetGroup()->BroadcastPacket(&data, false);
                                    else
                                        owner->ToPlayer()->GetSession()->SendPacket(&data);
                                }
                            }
                        }
                        return;
                    }
                    case 35881: //Shared Sentence SM Vishas
                    {
                        if (Unit* NearestAlly = target->FindNearestFriendlyPlayerForSentence(2.0f))
                        {
                            if (SpellAuraHolder* holder = target->GetSpellAuraHolder(35881))
                            {
                                int32 BaseDamage = urand(6400, 8600);
                                int32 FriendDamagePerc = holder->GetAuraDuration() * 5 / 1000;
                                int32 SelfDamagePerc = 100 - FriendDamagePerc;
                                int32 FriendDamage = BaseDamage / 100 * FriendDamagePerc;
                                int32 SelfDamage = BaseDamage / 100 * SelfDamagePerc;

                                if (Unit* pCaster = GetCaster())
                                {
                                    if (FriendDamage > 0)
                                        pCaster->CastCustomSpell(NearestAlly, 35882, &FriendDamage, nullptr, nullptr, true, nullptr, this);

                                    if (SelfDamage > 0)
                                        pCaster->CastCustomSpell(target, 35882, &SelfDamage, nullptr, nullptr, true, nullptr, this);
                                }
                                target->RemoveAurasDueToSpell(35881);
                            }
                        }
                        return;
                    }
                    case 35837: //Paralyzing Poison
                    {
                        if (SpellAuraHolder* holder = target->GetSpellAuraHolder(35837))
                        {
                            if (holder->GetStackAmount() == 5)
                            {
                                if (Unit* pCaster = GetCaster())
                                    pCaster->CastSpell(target, 35842, true);
                                
                                target->RemoveAurasDueToSpell(35837);
                            }
                        }
                        return;
                    }
                    //Time Lapse
                    case 35234:
                    {
                        if (Player* pPlayer = ToPlayer(target))
                            pPlayer->TimeLapseTick();

                        return;
                    }
                    //AC Wand of Negation
                    case 35004:
                    {
                        if (Unit* pCaster = GetCaster())
                            pCaster->CastSpell(target, 35005, true);

                        return;
                    }
                    //Call of Jin'do
                    case 34899:
                    {
                        if (!target->HasAura(34900))
                        {
                            if (Unit* pCaster = GetCaster())
                            {
                                if (pCaster->GetDistance(target) < 15.0f)
                                    target->RemoveAurasDueToSpell(34899),
                                    pCaster->CastSpell(target, 34902, true);
                                else if (pCaster->GetDistance(target) < 25)
                                {
                                    if (target->HasInArc(M_PI_F, pCaster))
                                        target->RemoveAurasDueToSpell(34899),
                                        pCaster->CastSpell(target, 34902, true);
                                }
                                else if (pCaster->GetDistance(target) < 35)
                                {
                                    if (target->HasInArc(M_PI_F, pCaster))
                                        if (urand(0, 10) > 8)
                                            target->RemoveAurasDueToSpell(34899),
                                            pCaster->CastSpell(target, 34902, true);
                                }
                            }
                        }
                        return;
                    }
                    case 34896: //vanilla+ Touch of Death
                    {
                        if (SpellAuraHolder* holder = target->GetSpellAuraHolder(34896))
                        {
                            if (holder->GetStackAmount() == 66)
                                target->CastSpell(target, 5, true);
                            else if (holder->GetStackAmount() > 50)
                            {
                                uint32 rollTot = urand(0, 99);
                                uint32 roll = holder->GetStackAmount() / 10;
                                if (rollTot < roll)
                                    target->CastSpell(target, 5, true);
                            }
                            else if (holder->GetStackAmount() == 35 && !target->HasAura(34897))
                                target->CastSpell(target, 34897, true);
                            else if (holder->GetStackAmount() > 25)
                            {
                                uint32 rollTot = urand(0, 99);
                                uint32 roll = holder->GetStackAmount() / 5;
                                if (rollTot < roll)
                                    target->CastSpell(target, 34897, true);
                            }
                        }
                        return;
                    }
                    case 34932: //Hermit's Echo debufff
                    {
                        if (SpellAuraHolder* holder = target->GetSpellAuraHolder(34932))
                            (holder->ModStackAmount(-1));
                        return;
                    }
                    // Firestone Passive (1-5 ranks)
                    case 758:
                    case 17945:
                    case 17947:
                    case 17949:
                    {
                        if (triggerTarget->GetTypeId() != TYPEID_PLAYER)
                            return;
                        Item* item = ((Player*)triggerTarget)->GetWeaponForAttack(BASE_ATTACK);
                        if (!item)
                            return;
                        uint32 enchant_id = 0;
                        switch (GetId())
                        {
                            case   758:
                                enchant_id = 1803;
                                break;   // Rank 1
                            case 17945:
                                enchant_id = 1823;
                                break;   // Rank 2
                            case 17947:
                                enchant_id = 1824;
                                break;   // Rank 3
                            case 17949:
                                enchant_id = 1825;
                                break;   // Rank 4
                            default:
                                return;
                        }
                        // remove old enchanting before applying new
                        ((Player*)triggerTarget)->ApplyEnchantment(item, TEMP_ENCHANTMENT_SLOT, false);
                        item->SetEnchantment(TEMP_ENCHANTMENT_SLOT, enchant_id, m_modifier.periodictime + 1000, 0);
                        // add new enchanting
                        ((Player*)triggerTarget)->ApplyEnchantment(item, TEMP_ENCHANTMENT_SLOT, true);
                        return;
                    }
//                    // Periodic Mana Burn
//                    case 812: break;
//                    // Polymorphic Ray
//                    case 6965: break;
//                    // Fire Nova (1-7 ranks)
//                    case 8350:
//                    case 8508:
//                    case 8509:
//                    case 11312:
//                    case 11313:
//                    case 25540:
//                    case 25544:
//                        break;
                    // Thaumaturgy Channel
                    case 9712:
                        trigger_spell_id = 21029;
                        break;
//                    // Egan's Blaster
//                    case 17368: break;
//                    // Haunted
//                    case 18347: break;
//                    // Ranshalla Waiting
//                    case 18953: break;
//                    // Inferno
//                    case 19695: break;
//                    // Frostwolf Muzzle DND
//                    case 21794: break;
//                    // Alterac Ram Collar DND
//                    case 21866: break;
//                    // Celebras Waiting
//                    case 21916: break;
                    // Brood Affliction: Bronze
                    case 23170:
                    {
                        int rand = urand(0, 9);
                        if (rand < 4)   // Ustaag <Nostalrius> : 40% chance
                            target->CastSpell(target, 23171, true, nullptr, this);
                        return;
                    }
                    case 23493:                             // Restoration
                    {
                        int32 heal = triggerTarget->GetMaxHealth() / 10;
                        triggerTarget->DealHeal(triggerTarget, heal, auraSpellInfo);

                        if (int32 mana = triggerTarget->GetMaxPower(POWER_MANA))
                        {
                            mana /= 10;
                            triggerTarget->EnergizeBySpell(triggerTarget, 23493, mana, POWER_MANA);
                        }
                        return;
                    }
//                    // Stoneclaw Totem Passive TEST
//                    case 23792: break;
//                    // Axe Flurry
//                    case 24018: break;
//                    // Mark of Arlokk
//                    case 24210: break;
//                    // Restoration
//                    case 24379: break;
//                    // Happy Pet
//                    case 24716: break;
//                    // Dream Fog
//                    case 24780: break;
//                    // Cannon Prep
//                    case 24832: break;
                    case 24834:                             // Shadow Bolt Whirl
                    {
                        uint32 spellForTick[8] = { 24820, 24821, 24822, 24823, 24835, 24836, 24837, 24838 };
                        trigger_spell_id = spellForTick[GetAuraTicks() % 8];
                        break;
                    }
//                    // Stink Trap
//                    case 24918: break;
//                    // Agro Drones
//                    case 25152: break;
                    case 25371:                             // Consume
                    {
                        int32 bpDamage = triggerTarget->GetMaxHealth() * 10 / 100;
                        triggerTarget->CastCustomSpell(triggerTarget, 25373, &bpDamage, nullptr, nullptr, true, nullptr, this, casterGUID);
                        return;
                    }
//                    // Pain Spike
//                    case 25572: break;
                    case 26009:                             // Rotate 360
                    case 26136:                             // Rotate -360
                    {
                        float newAngle = target->GetOrientation();

                        if (auraId == 26009)
                            newAngle += M_PI_F / 40;
                        else
                            newAngle -= M_PI_F / 40;

                        newAngle = MapManager::NormalizeOrientation(newAngle);
                        target->SetFacingTo(newAngle);
                        target->CastSpell(target, 26029, true);
                        return;
                    }
//                    // Consume
//                    case 26196: break;
//                    // Berserk
//                    case 26615: break;
//                    // Defile
//                    case 27177: break;
//                    // Teleport: IF/UC
//                    case 27601: break;
//                     
                    // Fist of the Third Moon
                    case 36064:
                    {
                        if (SpellAuraHolder* holder = target->GetSpellAuraHolder(36064))
                        {
                            if (holder->GetStackAmount() == 3)
                            {
                                if (Unit* pCaster = GetCaster())
                                {
                                    if (pCaster->isAlive())
                                    {
                                        target->RemoveAurasDueToSpell(36064);
                                        pCaster->CastSpell(target, 36152, true);
                                    }

                                }
                            }
                        }

                        return;
                    }
                    // Four Finger Death Punch
                    case 35940:
                    {
                        if (SpellAuraHolder* holder = target->GetSpellAuraHolder(35940))
                        {
                            if (holder->GetStackAmount() == 4)
                            {
                                target->RemoveAurasDueToSpell(35940);
                                target->CastSpell(target, 35941, true);
                            }
                        }

                        return;
                    }
                    // Five Fat Finger Exploding Heart Technique
                    case 27673:
                    {
                        if (SpellAuraHolder* holder = target->GetSpellAuraHolder(27673))
                        {
                            if (holder->GetStackAmount() == 5)
                            {
                                if (target->IsMoving())
                                {
                                    target->RemoveAurasDueToSpell(27673);
                                    target->CastSpell(target, 27676, true);
                                }
                            }
                        }

                        return;
                    }
                    // Nitrous Boost
                    case 27746:
                    {
                        if (target->GetPower(POWER_MANA) >= 10)
                        {
                            target->ModifyPower(POWER_MANA, -10);
                            target->SendEnergizeSpellLog(target, 27746, -10, POWER_MANA);
                        }
                        else
                            target->RemoveAurasDueToSpell(27746);
                        return;
                    }
                    case 34370:
                    {
                        if (!target->HasAura(34487))
                            target->RemoveAurasDueToSpell(34370);
                        return;
                    }
                    case 34850:
                    {
                        if (target->GetHealthPercent() < 96.0f)
                            target->RemoveAurasDueToSpell(34850);
                        return;
                    }
                    // Steam Tank Passive
                    case 27747:
                    {
                        uint32 tonkmana = target->GetPower(POWER_MANA);
                        if (tonkmana < 100)
                        {
                            target->ModifyPower(POWER_MANA, tonkmana > 90 ? (100 - tonkmana) : 10);
                        }
                        return;
                    }
                    case 27808:                             // Frost Blast
                    {
                        int32 bpDamage = triggerTarget->GetMaxHealth() * 26 / 100;
                        triggerTarget->CastCustomSpell(triggerTarget, 29879, &bpDamage, nullptr, nullptr, true, nullptr, this, casterGUID);
                        return;
                    }
                    //Crystal of Zin-Malor
                    case 19307:
                    {
                        int32 randN = urand(10, 50);
                        int32 HpDamage = (int32)triggerTarget->GetMaxHealth() / 1000 * randN;
                        int32 MpDamage = (int32)triggerTarget->GetPower(POWER_MANA) / 1000 * randN;
                        triggerTarget->ModifyPower(POWER_MANA, -MpDamage);
                        triggerTarget->CastCustomSpell(triggerTarget, 35280, &HpDamage, nullptr, nullptr, true, nullptr, this);

                        return;
                    }
                    // Detonate Mana
                    case 27819:
                    {
                        // 50% Mana Burn
                        int32 bpDamage = (int32)triggerTarget->GetPower(POWER_MANA) / 2;
                        triggerTarget->ModifyPower(POWER_MANA, -bpDamage);
                        triggerTarget->CastCustomSpell(triggerTarget, 27820, &bpDamage, nullptr, nullptr, true, nullptr, this, triggerTarget->GetObjectGuid());
                        return;
                    }
                    // Stalagg Chain and Feugen Chain
                    case 28096:
                    case 28111:
                    {
                        // X-Chain is casted by Tesla to X, so: caster == Tesla, target = X
                        Unit* pCaster = GetCaster();
                        if (pCaster && pCaster->GetTypeId() == TYPEID_UNIT && !pCaster->IsWithinDistInMap(target, 60.0f))
                        {
                            pCaster->InterruptNonMeleeSpells(true);
                            ((Creature*)pCaster)->SetInCombatWithZone();
                            // Stalagg Tesla Passive or Feugen Tesla Passive
                            pCaster->CastSpell(pCaster, auraId == 28096 ? 28097 : 28109, true, nullptr, nullptr, target->GetObjectGuid());
                        }
                        return;
                    }
//                    // Controller Timer
//                    case 28095: break;
//                    // Stalagg Tesla Passive
//                    case 28097: break;
//                    // Feugen Tesla Passive
//                    case 28109: break;
//                    // Mark of Didier
//                    case 28114: break;
//                    // Communique Timer, camp
//                    case 28346: break;
//                    
                    case 28522: // Icebolt
                    {
                        if (target && target->isAlive() && !target->HasAura(31800))
                        {
                            target->CastSpell(target, 31800, true); // Icebolt which causes immunity to frost dmg
                            trigger_spell_id = 28535; //summon ice block
                            break;
                        }
                        else
                        {
                            return;
                        }
                    }
                    //Reported
                    case 34860:
                    {
                        if (Player* pPlayer = ToPlayer(target))
                        {
                            uint32 MinNumberKick = 2;
                            uint32 MinNumberAFK = 1;
                            //WSG
                            if (pPlayer->GetMapId() == 489)
                                MinNumberAFK = 2,
                                MinNumberKick = 5;
                            //AB
                            else if (pPlayer->GetMapId() == 529)
                                MinNumberAFK = 4,
                                MinNumberKick = 8;
                            //AC
                            else if (pPlayer->GetMapId() == 37)
                                MinNumberAFK = 5,
                                MinNumberKick = 10;
                            //AV
                            else if (pPlayer->GetMapId() == 30)
                                MinNumberAFK = 6,
                                MinNumberKick = 15;


                            //WSG
                            if (pPlayer->GetMapId() == 489)
                                MinNumberAFK = 1,
                                MinNumberKick = 2;
                            //AB
                            else if (pPlayer->GetMapId() == 529)
                                MinNumberAFK = 2,
                                MinNumberKick = 3;
                            //AC
                            else if (pPlayer->GetMapId() == 37)
                                MinNumberAFK = 3,
                                MinNumberKick = 4;
                            //AV
                            else if (pPlayer->GetMapId() == 30)
                                MinNumberAFK = 6,
                                MinNumberKick = 15;


                            if (SpellAuraHolder* holder = target->GetSpellAuraHolder(34860))
                                if (holder->GetStackAmount() >= MinNumberKick)
                                    pPlayer->LeaveBattleground();
                                else if (holder->GetStackAmount() >= MinNumberAFK)
                                    pPlayer->AddAura(34861, 0, pPlayer);
                        }
                        return;
                    }
                    // Silithyst
//                  case 29519: break;
                    default:
                        break;
                }
                break;
            }
            case SPELLFAMILY_MAGE:
            {
                switch (auraId)
                {
                    case 33900: //vanilla+ mage chain reaction
                    {
						int32 manaPercAmmount = target->GetMaxPower(POWER_MANA) / 100;
						int32 HPpercAmmount = target->GetMaxHealth() / 100;
                        if ((target->GetPower(POWER_MANA) >= manaPercAmmount) && (target->GetHealth() > HPpercAmmount))
                        {
                            target->ModifyPower(POWER_MANA, -manaPercAmmount);
                            target->SendEnergizeSpellLog(target, 33900, -manaPercAmmount, POWER_MANA);
                        }
                        else
                            target->RemoveAurasDueToSpell(33900);
                        return;
                    }
                    case 66:                                // Invisibility
                        // Here need periodic trigger reducing threat spell (or do it manually)
                        return;
                    default:
                        break;
                }
                break;
            }
            case SPELLFAMILY_WARLOCK:
            {
                switch (auraId)
                {
                    // 1% Mana Burn
                    case 23153:
                    {
                        int32 bpDamage = (int32)triggerTarget->GetPower(POWER_MANA) / 100;
                        triggerTarget->ModifyPower(POWER_MANA, -bpDamage);
                        triggerTarget->CastCustomSpell(triggerTarget, 23393, &bpDamage, nullptr, nullptr, true, nullptr, this, triggerTarget->GetObjectGuid());
                        return;
                    }
                        default:
                            break;
                }
                break;
            }
            case SPELLFAMILY_PALADIN:
            {
                switch (auraId)
                {   
                    //Morale
                    case 33462:
                    {
                        if (!target->HasAura(35261))
                            if (!target->HasAura(35262))
                                target->CastSpell(target, 35261, true);
                        return;
                    }
                    case 33463:
                    {
                        if (!target->HasAura(35261))
                            if (!target->HasAura(35263))
                                target->CastSpell(target, 35261, true);
                        return;
                    }
                    case 33464:
                    {
                        if (!target->HasAura(35261))
                            if (!target->HasAura(35264))
                                target->CastSpell(target, 35261, true);
                        return;
                    }
                    case 33465:
                    {
                        if (!target->HasAura(35261))
                            if (!target->HasAura(35265))
                                target->CastSpell(target, 35261, true);
                        return;
                    }
                    case 33466:
                    {
                        if (!target->HasAura(35261))
                            if (!target->HasAura(35266))
                                target->CastSpell(target, 35261, true);
                        return;
                    }
                    default:
                    break;
                }
                break;
            }
			case SPELLFAMILY_WARRIOR:
			{
				switch (auraId)
				{
				    //case 29131: //v+ warrior bloodrage
        //            {
        //                uint32 HPpercAmmount = (target->GetMaxHealth() * 0.01);
        //                if (target->GetHealth() > HPpercAmmount)
        //                {
        //                    /*target->ModifyHealth(-HPpercAmmount);*/
        //                    target->DealDamage(target, HPpercAmmount, NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
        //                }
        //                else
        //                    target->RemoveAurasDueToSpell(29131);
        //                return;
        //            }
                    case 12296:
                    {
                        if (target->isInCombat())
                        {
                            target->ModifyPower(POWER_RAGE, 10);
                            /*target->SendEnergizeSpellLog(target, 12296, 10, POWER_RAGE);*/
                        }

                        return;
                    }
				}
			}
			case SPELLFAMILY_ROGUE:
			{
				switch (auraId)
				{
                    //Survivor
                    case 35830:
                    case 35832:
                    {
                        if (target->HasAuraType(SPELL_AURA_MOD_STUN))
                        {
                            target->RemoveAurasDueToSpell(35831);
                            return;
                        }

                        int32 HPperc = 100 - uint32((float(target->GetHealth()) / target->GetMaxHealth()) * 100);
                        switch (auraId)
                        {
                            case 35830: HPperc *= 0.25f; break;
                            case 35832: HPperc *= 0.5f; break;
                            default: break;
                        }

                        if (HPperc > 0)
                        {
                            HPperc *= -1;
                            target->CastCustomSpell(target, 35831, &HPperc, nullptr, nullptr, true, nullptr);
                        }

                        return;
                    }
                    //Shadow Cut apply
                    case 14171:
                    case 14172:
                    case 14173:
                    {            
                        if (Unit* LinkCaster = GetCaster())
                        {
                            if (Unit* SelectedTarget = LinkCaster->ToPlayer()->GetSelectedUnit())
                            {
                                if (LinkCaster != SelectedTarget && !LinkCaster->IsFriendlyTo(SelectedTarget) && LinkCaster->IsWithinLOSInMap(SelectedTarget) && LinkCaster->isAlive() && !LinkCaster->IsMounted() && SelectedTarget->isAlive())
                                {
                                    float max_range = 5.0f + LinkCaster->GetCombatReach() + SelectedTarget->GetCombatReach();

                                    if (LinkCaster->IsWithinDist(SelectedTarget, max_range))
                                    {
                                        int32 TalProcSpellId = 0;
                                        switch (auraId)
                                        {
                                            case 14171: TalProcSpellId = 35764; break;
                                            case 14172: TalProcSpellId = 35765; break;
                                            case 14173: TalProcSpellId = 35766; break;
                                            default: break;
                                        }
                                        if (TalProcSpellId)
                                            LinkCaster->CastSpell(SelectedTarget, TalProcSpellId, true);
                                    }
                                }
                            }
                        }
                        return;
                    }
					return;
				}
			}
			case SPELLFAMILY_PRIEST:
			{
				switch (auraId)
				{
                //Holy Link
                case 35754:
                {
                    if (Unit* LinkCaster = GetCaster())
                    {
                        if (!LinkCaster->IsWithinLOSInMap(target) || LinkCaster->isDead() || LinkCaster->IsMounted())
                        {
                            target->RemoveAurasDueToSpell(35754);
                            LinkCaster->RemoveAurasDueToSpell(35760);
                            return;
                        }

                        SpellRangeEntry const* srange = sSpellRangeStore.LookupEntry(auraSpellInfo->rangeIndex);
                        float max_range = GetSpellMaxRange(srange) + LinkCaster->GetCombatReach() + target->GetCombatReach();

                        if (target->IsWithinDist(LinkCaster, max_range))
                        {
                            LinkCaster->CastSpell(target, 35755, true);
                            target->CastSpell(LinkCaster, 35760, true);
                        }
                        else
                        {
                            target->RemoveAurasDueToSpell(35754);
                            LinkCaster->RemoveAurasDueToSpell(35760);
                        }
                    }
                    else
                    {
                        target->RemoveAurasDueToSpell(35754);
                    }

                    return;
                }
				case 33859: //v+ priest burning of the soul
					int32 Burninghealth = int32((target->GetMaxHealth() * 0.03));
					if (!(target->GetHealth() > Burninghealth))
						target->RemoveAurasDueToSpell(33859);
					return;
				}
			}

//            case SPELLFAMILY_WARRIOR:
//            {
//                switch(auraId)
//                {
//                    // Wild Magic
//                    case 23410: break;
//                    // Corrupted Totems
//                    case 23425: break;
//                    default:
//                        break;
//                }
//                break;
//            }
//            case SPELLFAMILY_PRIEST:
//            {
//                switch(auraId)
//                {
//                    // Blue Beam
//                    case 32930: break;
//                    default:
//                        break;
//                }
//                break;
//            }
            case SPELLFAMILY_DRUID:
            {
                switch (auraId)
                {
                    case 768:                               // Cat Form
                        // trigger_spell_id not set and unknown effect triggered in this case, ignoring for while
                        return;
                    case 22842:                             // Frenzied Regeneration
                    case 22895:
                    case 22896:
                    {
                        int32 LifePerRage = GetModifier()->m_amount;

                        int32 lRage = target->GetPower(POWER_RAGE);
                        if (lRage > 100)                    // rage stored as rage*10
                            lRage = 100;
                        target->ModifyPower(POWER_RAGE, -lRage);
                        int32 FRTriggerBasePoints = int32(lRage * LifePerRage / 10);
                        target->CastCustomSpell(target, 22845, &FRTriggerBasePoints, nullptr, nullptr, true, nullptr, this);
                        return;
                    }
                    default:
                        break;
                }
                break;
            }
            case SPELLFAMILY_SHAMAN:
            {
                switch (auraId)
                {
                    case 3600:
                    {
                        if (!target->HasAura(6474))
                            target->RemoveAurasDueToSpell(3600);

                        return;
                    }
                    case 6474: //Earthbind Totem re-apply
                    {
                        if (!target->HasAura(3600))
                            if (const SpellEntry* pSpell = sSpellMgr.GetSpellEntry(3600))
                                if (!target->IsImmuneToSpell(pSpell, false))
                                    if (Unit* caster = GetCaster())
                                        caster->CastSpell(target, 3600, true);
                        return;
                    }
                    case 28820:                             // Lightning Shield (The Earthshatterer set trigger after cast Lighting Shield)
                    {
                        // Need remove self if Lightning Shield not active
                        Unit::SpellAuraHolderMap const& auras = triggerTarget->GetSpellAuraHolderMap();
                        for (Unit::SpellAuraHolderMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                        {
                            SpellEntry const* spell = itr->second->GetSpellProto();
                            if (spell->IsFitToFamily<SPELLFAMILY_SHAMAN, CF_SHAMAN_LIGHTNING_SHIELD>())
                                return;
                        }
                        triggerTarget->RemoveAurasDueToSpell(28820);
                        return;
                    }
                    default:
                        break;
                }
                break;
            }
            default:
                break;
        }

        // Reget trigger spell proto
        triggeredSpellInfo = sSpellMgr.GetSpellEntry(trigger_spell_id);
    }
    else
    {
        // Spell exist but require custom code
        switch (auraId)
        {
            //Que marks
            case 34761:
            {
                if (triggerTarget->GetTypeId() != TYPEID_PLAYER)
                    return;
                if (!(((Player*)target)->IsQueuedtoBGid(BATTLEGROUND_QUEUE_WS)))
                {
                    target->RemoveAurasDueToSpell(34761);
                    return;
                }
                break;
            }
            case 34763:
            {
                if (triggerTarget->GetTypeId() != TYPEID_PLAYER)
                    return;
                if (!(((Player*)target)->IsQueuedtoBGid(BATTLEGROUND_QUEUE_AB)))
                {
                    target->RemoveAurasDueToSpell(34763);
                    return;
                }
                break;
            }
            case 34765:
            {
                if (triggerTarget->GetTypeId() != TYPEID_PLAYER)
                    return;
                if (!(((Player*)target)->IsQueuedtoBGid(BATTLEGROUND_QUEUE_AV)))
                {
                    target->RemoveAurasDueToSpell(34765);
                    return;
                }
                break;
            }
            case 34767:
            {
                if (triggerTarget->GetTypeId() != TYPEID_PLAYER)
                    return;
                if (!(((Player*)target)->IsQueuedtoBGid(BATTLEGROUND_QUEUE_AC)))
                {
                    target->RemoveAurasDueToSpell(34767);
                    return;
                }
                break;
            }

            case 7054:
                spellRandom = urand(0, 14) + 7038;
                target->CastSpell(target, spellRandom, true, nullptr, this);
                return;
                break;
            case 8892:  // Goblin Rocket Boots
            case 13141: // Gnomish Rocket Boots
                // 20 ticks, et une chance sur 5 d'exploser.
                if (urand(0, 20 * 5 - 1) != 0)
                    return;
                break;
#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_8_4
            case 9347:                                      // Mortal Strike
            {
                // expected selection current fight target
                triggerTarget = GetTarget()->getVictim();
                if (!triggerTarget)
                    return;

                // avoid triggering for far target
                SpellRangeEntry const* srange = sSpellRangeStore.LookupEntry(triggeredSpellInfo->rangeIndex);
                float max_range = GetSpellMaxRange(srange);
                if (!triggerTarget->IsWithinDist(GetTarget(), max_range))
                    return;

                break;
            }
#endif
            case 1010:                                      // Curse of Idiocy
            {
                // TODO: spell casted by result in correct way mostly
                // BUT:
                // 1) target show casting at each triggered cast: target don't must show casting animation for any triggered spell
                //      but must show affect apply like item casting
                // 2) maybe aura must be replace by new with accumulative stat mods instead stacking

                // prevent cast by triggered auras
                if (casterGUID == triggerTarget->GetObjectGuid())
                    return;

                // stop triggering after each affected stats lost > 90
                int32 intelectLoss = 0;
                int32 spiritLoss = 0;

                Unit::AuraList const& mModStat = triggerTarget->GetAurasByType(SPELL_AURA_MOD_STAT);
                for (Unit::AuraList::const_iterator i = mModStat.begin(); i != mModStat.end(); ++i)
                {
                    if ((*i)->GetId() == 1010)
                    {
                        switch ((*i)->GetModifier()->m_miscvalue)
                        {
                            case STAT_INTELLECT:
                                intelectLoss += (*i)->GetModifier()->m_amount;
                                break;
                            case STAT_SPIRIT:
                                spiritLoss   += (*i)->GetModifier()->m_amount;
                                break;
                            default:
                                break;
                        }
                    }
                }

                if (intelectLoss <= -90 && spiritLoss <= -90)
                    return;

                break;
            }
            case 16191:                                     // Mana Tide
            {
                triggerTarget->CastCustomSpell(triggerTarget, trigger_spell_id, &m_modifier.m_amount, nullptr, nullptr, true, nullptr, this);
                return;
            }
            //Frost Trap Aura
            case 13810:
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;
                // Pour le talent hunt 'Piege' par exemple (chances de stun)
                caster->ProcDamageAndSpell(target, (PROC_FLAG_ON_TRAP_ACTIVATION | PROC_FLAG_SUCCESSFUL_AOE), PROC_FLAG_NONE, PROC_EX_NORMAL_HIT, 1, BASE_ATTACK, GetSpellProto());
                return;
            }
            // Thaddius negative charge
            case 28084:
            {
                // Lets only process the following when in naxx, otherwise it can become expensive as hell
                if (triggerTarget->GetMap()->GetId() != 533) 
                    break;
                Unit* caster = GetCaster();

                int numStacks = 0;
                // Finding the amount of other players within 13yd that has the same polarity
                Map::PlayerList const& pList = triggerTarget->GetMap()->GetPlayers();
                for (Map::PlayerList::const_iterator it = pList.begin(); it != pList.end(); ++it)
                {
                    Player* pPlayer = (*it).getSource();
                    if (pPlayer->GetGUID() == casterGUID) continue;
                    if (!pPlayer) continue;
                    if (pPlayer->isDead()) continue;
                    // 2d distance should be good enough
                    if (pPlayer->HasAura(28084) && caster->GetDistance2d(pPlayer) < 13.0f) // 13.0f taken from dbc
                    {
                        ++numStacks;
                    }
                }
                if (numStacks > 0)
                {
                    if (!triggerTarget->HasAura(29660))
                        triggerTarget->AddAura(29660);
                    triggerTarget->GetAura(29660, EFFECT_INDEX_0)->GetHolder()->SetStackAmount(numStacks);
                }
                else
                    triggerTarget->RemoveAurasDueToSpell(29659);
                break;
            }
            // Thaddius positive charge
            case 28059:
            {
                // Lets only process the following when in naxx, otherwise it can become expensive as hell
                if (triggerTarget->GetMap()->GetId() != 533)
                    break;
                Unit* caster = GetCaster();

                int numStacks = 0;
                // Finding the amount of other players within 13yd that has the same polarity
                Map::PlayerList const& pList = triggerTarget->GetMap()->GetPlayers();
                for (Map::PlayerList::const_iterator it = pList.begin(); it != pList.end(); ++it)
                {
                    Player* pPlayer = (*it).getSource();
                    if (!pPlayer) continue;
                    if (pPlayer->GetGUID() == triggerTarget->GetGUID()) continue;
                    if (pPlayer->isDead()) continue;
                    // 2d distance should be good enough
                    if (pPlayer->HasAura(28059) && caster->GetDistance2d(pPlayer) < 13.0f) // 13.0f taken from dbc
                    {
                        ++numStacks;
                    }
                }
                if (numStacks > 0)
                {
                    if (!triggerTarget->HasAura(29659))
                        triggerTarget->AddAura(29659);
                    triggerTarget->GetAura(29659, EFFECT_INDEX_0)->GetHolder()->SetStackAmount(numStacks);
                }
                else
                    triggerTarget->RemoveAurasDueToSpell(29659);
                break;
            }
            // Activate MG Turret
            case 25026:
            {
                if (target->GetPower(POWER_MANA) >= 10)
                {
                    target->ModifyPower(POWER_MANA, -10);
                    target->SendEnergizeSpellLog(target, 25026, -10, POWER_MANA);
                }
                else
                    target->RemoveAurasDueToSpell(25026);
                break;
            }
            // Flamethrower
            case 25027:
            {
                if (target->GetPower(POWER_MANA) >= 10)
                {
                    target->ModifyPower(POWER_MANA, -10);
                    target->SendEnergizeSpellLog(target, 25027, -10, POWER_MANA);
                }
                else
                    target->RemoveAurasDueToSpell(25027);
                break;
            }
        }
    }

    Unit* triggerCaster = triggerTarget;
    WorldObject* triggerTargetObject = nullptr;

    // for channeled spell cast applied from aura owner to channel target (persistent aura affects already applied to true target)
    // some periodic casts applied to targets, so need select proper caster (ex. 15790)
    if (IsChanneled() && GetSpellProto()->Effect[GetEffIndex()] != SPELL_EFFECT_PERSISTENT_AREA_AURA)
    {
        // interesting 2 cases: periodic aura at caster of channeled spell
        if (target->GetObjectGuid() == casterGUID)
        {
            triggerCaster = target;

            if (WorldObject* channelTarget = target->GetMap()->GetWorldObject(target->GetChannelObjectGuid()))
            {
                if (channelTarget->isType(TYPEMASK_UNIT))
                    triggerTarget = (Unit*)channelTarget;
                else
                    triggerTargetObject = channelTarget;
            }
        }
        // or periodic aura at caster channel target
        else if (Unit* caster = GetCaster())
        {
            if (target->GetObjectGuid() == caster->GetChannelObjectGuid())
            {
                triggerCaster = caster;
                triggerTarget = target;
            }
        }
    }

    // All ok cast by default case
    if (triggeredSpellInfo)
    {
        if (triggerTargetObject)
            triggerCaster->CastSpell(triggerTargetObject->GetPositionX(), triggerTargetObject->GetPositionY(), triggerTargetObject->GetPositionZ(),
                                     triggeredSpellInfo, true, nullptr, this, casterGUID);
        else
            triggerCaster->CastSpell(triggerTarget, triggeredSpellInfo, true, nullptr, this, casterGUID);
    }
    else
    {
        if (Unit* caster = GetCaster())
        {
            if (triggerTarget->GetTypeId() != TYPEID_UNIT || !sScriptMgr.OnEffectDummy(caster, GetId(), GetEffIndex(), (Creature*)triggerTarget))
                sLog.outError("Aura::TriggerSpell: Spell %u have 0 in EffectTriggered[%d], not handled custom case?", GetId(), GetEffIndex());
        }
    }
}

/*********************************************************/
/***                  AURA EFFECTS                     ***/
/*********************************************************/

void Aura::HandleAuraDummy(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
        return;

    Unit *target = GetTarget();

    // AT APPLY
    if (apply)
    {
        switch (GetSpellProto()->SpellFamilyName)
        {
            case SPELLFAMILY_GENERIC:
            {
                switch (GetId())
                {
                    case 36103: // Fate Revision CD
                    {
                        target->CastSpell(target, 36105, true);

                        return;
                    }
                    case 35905:
                    {
                        target->UpdateVisibilityAndView();

                        return;
                    }
                    // Scarlet Charger's Trample
                    case 35869:
                    case 35870:
                    case 35871:
                    {
                        if (!target->HasAura(35875))
                        {
                            if (Unit* Charger = GetCaster())
                            {
                                if (Creature* Crea = Charger->SummonCreature(25237, Charger->GetPositionX(), Charger->GetPositionY(), Charger->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 3000))
                                    Crea->CastSpell(target, 35876, true);
                            }
                        }
                        return;
                    }
                    //Banhammer
                    case 35203:
                    {
                        /*if (SpellAuraHolder* holder = target->GetSpellAuraHolder(35203))
                        {
                            int32 StunBanDuration = 0;
                            switch (holder->GetStackAmount())
                            {
                                case 1:
                                case 2:
                                case 3:
                                    StunBanDuration = 30 * IN_MILLISECONDS;
                                    break;
                                case 4:
                                    StunBanDuration = 60 * IN_MILLISECONDS;
                                    break;
                                case 5:
                                    StunBanDuration = 1 * MINUTE * IN_MILLISECONDS;
                                    break;
                                case 6:
                                    StunBanDuration = 3 * MINUTE * IN_MILLISECONDS;
                                    break;
                                case 7:
                                    StunBanDuration = 5 * MINUTE * IN_MILLISECONDS;
                                    break;
                                case 8:
                                    StunBanDuration = 10 * MINUTE * IN_MILLISECONDS;
                                    break;
                                case 9:
                                    StunBanDuration = 30 * MINUTE * IN_MILLISECONDS;
                                    break;
                                case 10:
                                    StunBanDuration = 60 * MINUTE * IN_MILLISECONDS;
                                    break;
                                    break;
                                default:
                                break;
                            }
                            if (StunBanDuration > 0)
                            {
                                target->AddAura(34800);
                                if (SpellAuraHolder* holder = target->GetSpellAuraHolder(34800))
                                    holder->SetAuraDuration(StunBanDuration);
                            }
                        }*/

                        return;
                    }
                    //Deserter
                    case 26013:
                    {
                        target->AddAura(34780, 0, target);

                        if (SpellAuraHolder* holder = target->GetSpellAuraHolder(34780))
                        {
                            if (holder->GetStackAmount() > 15)
                            {
                                target->RemoveAurasDueToSpell(34780);

                                if (target->HasAura(34771))
                                    target->RemoveAurasDueToSpell(26013),
                                    target->CastSpell(target, 34612, true); //3d
                                else
                                    target->RemoveAurasDueToSpell(26013),
                                    target->CastSpell(target, 34771, true),
                                    target->CastSpell(target, 34611, true); //1d
                            }
                            else if (holder->GetStackAmount() > 13)
                                target->RemoveAurasDueToSpell(26013),
                                target->CastSpell(target, 34610, true); //12h
                            else if (holder->GetStackAmount() > 10)
                                target->RemoveAurasDueToSpell(26013),
                                target->CastSpell(target, 34609, true); //4h
                            else if (holder->GetStackAmount() > 7)
                                target->RemoveAurasDueToSpell(26013),
                                target->CastSpell(target, 34608, true); //2h
                            else if (holder->GetStackAmount() > 4)
                                target->RemoveAurasDueToSpell(26013),
                                target->CastSpell(target, 34607, true); //1h
                        }

                        return;
                    }
                    //Earth Link
                    case 35043:
                    {
                        if (Unit* Ecaster = GetCaster())
                        {
                            if (SpellAuraHolder* auraHolder = Ecaster->GetSpellAuraHolder(35043))
                            {
                                const int Duration = auraHolder->GetAuraDuration();

                                if (Creature* EarthLink = Ecaster->SummonCreature(25060, Ecaster->GetPositionX(), Ecaster->GetPositionY(), Ecaster->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, Duration))
                                {
                                    EarthLink->CastSpell(target, 35069, true);
                                    EarthLink->ForcedDespawn(Duration);
                                }
                            }
                            /*if (!target->HasAura(25039))*/
                        }
                        return;
                    }
                    case 34931: //Hermit's Echo debufff
                    {
                        target->AddAura(34932);
                        return;
                    }
                    // AC Mid Aura
                    case 34803:
                    {
                        if (Player* pPlayer = ToPlayer(target))
                        {
                            if (BattleGround* bg = pPlayer->GetBattleGround())
                                bg->BGAuraApplied(pPlayer);
                        }
                        return;
                    }
                    case 2584:                              // Waiting to Resurrect
                    {
                        // for cases where aura would re-apply and player is no longer in BG
                        if (Unit* caster = GetCaster())
                        {
                            if (Player* player = caster->ToPlayer())
                                if (!player->InBattleGround())
                                    player->RemoveAurasDueToSpell(2584);
                        }
                        return;
                    }
					//case 12479:                                     // Hex of Jammal'an
					//{
					//	if (Unit* caster = GetCaster())
					//		caster->CastSpell(target, 12480, true, nullptr, this),
					//	    caster->CastSpell(target, 12483, true, nullptr, this);
					//		return;
					//}
                    case 6758:                              // Party Fever
                    {
                        if (Player* pPlayer = ToPlayer(target))
                        {
                            if (!pPlayer->isInCombat())
                                pPlayer->HandleEmoteCommand(EMOTE_STATE_DANCE);
                        }
                        return;
                    }
                    case 8067:                              // Party Time!
                    {
                        m_isPeriodic = true;
                        m_modifier.periodictime = 10 * IN_MILLISECONDS;;
                        return;
                    }
                    case 10255:                             // Stoned
                    {
                        if (Unit* caster = GetCaster())
                        {
                            if (caster->GetTypeId() != TYPEID_UNIT)
                                return;

                            caster->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            caster->addUnitState(UNIT_STAT_ROOT);
                        }
                        return;
                    }
                    case 13139:                             // net-o-matic
                    {
                        // root to self part of (root_target->charge->root_self sequence
                        if (Unit* caster = GetCaster())
                            caster->CastSpell(caster, 13138, true, nullptr, this);
                        GetHolder()->SetAuraDuration(0); // Remove aura (else stays for ever, and casts at login)
                        return;
                    }
                    case 16336: // Haunting Phantoms
                    {
                        m_isPeriodic = true;
                        m_modifier.periodictime = urand(30, 90) * IN_MILLISECONDS;;
                        return;
                    }
                    case 26234:                             // Ragnaros Submerge Visual
                    {
                        if (Unit* caster = GetCaster())
                        {
                            caster->CastSpell(caster, 21107, true); // Ragnaros Submerge Fade
                            caster->CastSpell(caster, 21859, true); // Ragnaros Submerge Effect
                            caster->RemoveAurasDueToSpell(26234);   // Need remove Submerge Visual after apply
                        }
                        return;
                    }
                    case 22646:                             // Goblin Rocket Helmet
                    {
                        if (Unit* caster = GetCaster())
                            caster->CastSpell(caster, 13360, true);
                        return;
                    }
                    case 24596: // Intoxicating Venom
                    {
                        if (target)
                        {
                            m_isPeriodic            = true;
                            m_modifier.periodictime = 1000;
                        }
                        return;
                    }
					case 34417: //v+ ravenian water
					{
						if (Player* pPlayer = ToPlayer(target))
						{
							pPlayer->RemoveAurasDueToSpell(34383);
						}
						return;
					}
                    case 29154:
                    {
                        Unit* caster = GetCaster();
                        if (!caster) break;
                        caster->HandleEmote(EMOTE_STATE_SUBMERGED);
                        break;
                    }
                }
                break;
            }
            case SPELLFAMILY_PALADIN:
            {
#if SUPPORTED_CLIENT_BUILD <= CLIENT_BUILD_1_8_4
                // Judgement of Command
                if (GetSpellProto()->SpellIconID == 561)
                {
                    if (GetCaster() != GetTarget())
                    {
                        m_isPeriodic = true;
                        m_modifier.periodictime = 2000; // guess
                    }
                    return;
                }
#endif
                break;
            }
            case SPELLFAMILY_SHAMAN:
            {
                uint32 spellID = GetId();

                //Stoneclaw Totem Totem Shield
                if (spellID == 35056 || spellID == 35057 || spellID == 35058 || spellID == 35059 || spellID == 35060 || spellID == 35061)
                {
                    uint32 shieldspellid = 0;
                    switch (GetId())
                    {
                        case 35056:
                            shieldspellid = 35062;
                            break;
                        case 35057:
                            shieldspellid = 35063;
                            break;
                        case 35058:
                            shieldspellid = 35064;
                            break;
                        case 35059:
                            shieldspellid = 35065;
                            break;
                        case 35060:
                            shieldspellid = 35066;
                            break;
                        case 35061:
                            shieldspellid = 35067;
                            break;
                        default:
                            break;
                    }
                    if (target != GetCaster())
                        target->AddAura(shieldspellid);
                }
            }
                break;
        }
    }
    // AT REMOVE
    else
    {
        if (IsQuestTameSpell(GetId()) && target->isAlive())
        {
            if (m_removeMode != AURA_REMOVE_BY_CHANNEL)
                return;

            Unit* caster = GetCaster();
            if (!caster || !caster->isAlive())
                return;

            uint32 finalSpellId = 0;
            switch (GetId())
            {
                case 19548:
                    finalSpellId = 19597;
                    break;
                case 19674:
                    finalSpellId = 19677;
                    break;
                case 19687:
                    finalSpellId = 19676;
                    break;
                case 19688:
                    finalSpellId = 19678;
                    break;
                case 19689:
                    finalSpellId = 19679;
                    break;
                case 19692:
                    finalSpellId = 19680;
                    break;
                case 19693:
                    finalSpellId = 19684;
                    break;
                case 19694:
                    finalSpellId = 19681;
                    break;
                case 19696:
                    finalSpellId = 19682;
                    break;
                case 19697:
                    finalSpellId = 19683;
                    break;
                case 19699:
                    finalSpellId = 19685;
                    break;
                case 19700:
                    finalSpellId = 19686;
                    break;
            }

            if (finalSpellId)
                caster->CastSpell(target, finalSpellId, true, nullptr, this);

            return;
        }

        switch (GetId())
        {
            case 36252: // Trophy of the Sin'dorei Shadowmeld
            {
                target->RemoveAurasDueToSpell(36253);

                return;
            }
            case 36158: // Poly caster emote
            {
                if (urand(0, 4) == 0)
                    target->HandleEmoteCommand(EMOTE_ONESHOT_LAUGH);

                return;
            }
            case 36103: // Fate Revision
            {
                int32 tarHp = target->GetMaxHealth() / 4;

                if (Unit* caster = GetCaster())
                    caster->CastCustomSpell(target, 36104, &tarHp, nullptr, nullptr, true, nullptr);
                else
                    target->CastCustomSpell(target, 36104, &tarHp, nullptr, nullptr, true, nullptr);

                return;
            }
            //Atonement SM Vishas
            case 35892:
            case 35893:
            case 35894:
            {
                if (Unit* caster = GetCaster())
                {
                    caster->CastSpell(target, 35883, true);
                }

                return;
            }
            case 22651: //DM Zevrim Sacrifice
            {
                if (Unit* caster = GetCaster())
                {
                    caster->CastSpell(target, 35895, true);
                }

                return;
            }
            //Vael Speech 2
            case 35298:
            {
                DoScriptText(-1469027, target);

                return;
            }
            //AC Portal Rune
            case 35090:
            {
                target->CastSpell(target, 35408, true);

                return;
            }
            //Creature Despawner on Unaura
            case 35209: //1
            case 35210: //2
            case 35211: //3
            case 35212: //4
            case 35213: //5
            case 35214: //8
            case 35215: //10
            case 35216: //15
            case 35217: //20
            case 35218: //30
            case 35219: //45
            case 35220: //60
            {
                if (Creature* creatureToDespawn = (Creature*)target)
                    creatureToDespawn->ForcedDespawn();

                return;
            }
            //Stoneclaw Totem Totem Shield
            case 35056:
            case 35057:
            case 35058:
            case 35059:
            case 35060:
            case 35061:
            {
                uint32 shieldspellid = 0;
                switch (GetId())
                {
                case 35056:
                    shieldspellid = 35062;
                    break;
                case 35057:
                    shieldspellid = 35063;
                    break;
                case 35058:
                    shieldspellid = 35064;
                    break;
                case 35059:
                    shieldspellid = 35065;
                    break;
                case 35060:
                    shieldspellid = 35066;
                    break;
                case 35061:
                    shieldspellid = 35067;
                    break;
                default:
                    break;
                }
                
                target->RemoveAurasDueToSpell(shieldspellid);
                return;
            }
            // Earth Link Stun proc on leaving the area (AC, Earth Elemental ability)
            case 35043:
            {
                if (!target->HasAura(35070))
                {
                    target->RemoveAurasDueToSpell(35069);
                    target->CastSpell(target, 35044, true);
                }
                return;
            }
            // Shielding Aura removes Caster visual on remove
            case 35000:
            {
                if (target->HasAura(35001))
                {
                    target->RemoveAurasDueToSpell(35001);
                }
                return;
            }
            //Interrupt Possess BWL
            case 35164:
            {
                if (target->IsNonMeleeSpellCasted())
                    if (target->HasAura(35200))
                        target->AddAura(35199);
                return;
            }
            // AC Mid Aura
            case 34803:
            {
                if (Player* pPlayer = ToPlayer(target))
                {
                    if (BattleGround* bg = pPlayer->GetBattleGround())
                        bg->BGAuraRemoved(pPlayer);
                }
                return;
            }
            case 34931:
            {
                if (Unit* caster = GetCaster())
                {
                    caster->RemoveAurasDueToSpell(34932);
                }
                return;
            }
            //Reckoning Reset
            case 20178:
            {
                if (Unit* caster = GetCaster())
                    caster->ResetExtraAttacks();
                return;
            }
            case 126:   // Kilrogg eye
            case 6272:  // Eye of Yesmur
            case 11403: // Dream Vision
                if (Player* pCaster = ToPlayer(GetCaster()))
                    pCaster->UnsummonPossessedMinion();
                return;
            case 10255:                                     // Stoned
            {
                if (Unit* caster = GetCaster())
                {
                    if (caster->GetTypeId() != TYPEID_UNIT)
                        return;
                    caster->clearUnitState(UNIT_STAT_ROOT | UNIT_STAT_PENDING_ROOT);
                    caster->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                }
                return;
            }
            case 11826:
                if (m_removeMode != AURA_REMOVE_BY_EXPIRE)
                    return;

                if (Unit* caster = GetCaster())
                    if (caster->GetTypeId() == TYPEID_PLAYER)
                        caster->CastSpell(target, 11828, true, caster->ToPlayer()->GetItemByGuid(GetCastItemGuid()), this);
                break;
            case 12479:                                     // Hex of Jammal'an
			{
				if (Unit* caster = GetCaster())
					caster->CastSpell(target, 12480, true, nullptr, this),
					caster->CastSpell(target, 12483, true, nullptr, this);
				return;
			}
            case 12774:                                     // (DND) Belnistrasz Idol Shutdown Visual
            {
                if (m_removeMode == AURA_REMOVE_BY_DEATH)
                    return;

                // Idom Rool Camera Shake <- wtf, don't drink while making spellnames?
                if (Unit* caster = GetCaster())
                    caster->CastSpell(caster, 12816, true);

                return;
            }
            case 20939: // Undying Soul - Dummy aura used for Unstuck command
            {
                if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
                { 
                    if (Unit* caster = GetCaster())
                        if (Player* casterPlayer = caster->ToPlayer())
                        {
                            if (casterPlayer->isAlive() && !casterPlayer->isInCombat() && !casterPlayer->IsTaxiFlying())
                            {
                                casterPlayer->AddAura(15007); // Add Resurrection Sickness
                                if (sObjectMgr.GetClosestGraveYard(casterPlayer->GetPositionX(), casterPlayer->GetPositionY(), casterPlayer->GetPositionZ(), casterPlayer->GetMapId(), casterPlayer->GetTeam()))
                                    casterPlayer->DealDamage(casterPlayer, casterPlayer->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                                else
                                {
                                    // If there is no nearby graveyard, player's ghost would spawn at the same spot.
                                    WorldSafeLocsEntry const *ClosestGrave = casterPlayer->GetTeamId() ? sWorldSafeLocsStore.LookupEntry(10) : sWorldSafeLocsStore.LookupEntry(4);
                                    if (ClosestGrave)
                                    {
                                        casterPlayer->SetHealth(1);
                                        casterPlayer->TeleportTo(ClosestGrave->map_id, ClosestGrave->x, ClosestGrave->y, ClosestGrave->z, 0, 0);
                                    }
                                }
                            }
                        }
                }
                return;
            }
            case 24906:                                     // Emeriss Aura
            {
                if (m_removeMode == AURA_REMOVE_BY_DEATH)
                    target->CastSpell(target, 24904, true, nullptr, this);

                return;
            }
            case 25042:                                     // Mark of Nature
            {
                if (m_removeMode == AURA_REMOVE_BY_DEATH)
                    target->CastSpell(target, 25040, true, nullptr, this);
                return;
            }
            case 34440:                                     // Mark of Desert
            {
                if (m_removeMode == AURA_REMOVE_BY_DEATH)
                    target->CastSpell(target, 34439, true, nullptr, this);
                return;
            }
            case 25185:                                     // Itch
            {
                if (m_removeMode != AURA_REMOVE_BY_EXPIRE)
                    return;

                if (Unit* caster = GetCaster())
                    caster->CastSpell(target, 25187, true);
                return;
            }
            case 26077:                                     // Itch
            {
                if (m_removeMode != AURA_REMOVE_BY_EXPIRE)
                    return;

                if (Unit* caster = GetCaster())
                    caster->CastSpell(target, 26078, true);
                return;
            }
            case 23183:                                     // Mark of Frost
            {
                if (m_removeMode == AURA_REMOVE_BY_DEATH)
                    target->CastSpell(target, 23182, true, nullptr, this);
                    return;
            }
            case 28169:                                     // Mutating Injection
            {
                Unit* caster = GetCaster();
                // Mutagen Explosion
                if (m_removeMode == AuraRemoveMode::AURA_REMOVE_BY_DISPEL)
                {
                    caster->CastSpell(target, 28206, true);
                }
                else
                {
                    caster->CastSpell(target, 28206, true, nullptr, this);
                }
                
                // Summons Poison Cloud creature
                target->CastSpell(target, 28240, true, nullptr, this);
                return;
            }
            //case 24324: // Ivina < Nostalrius > : Hakkar
            //{
            //    target->RemoveAurasDueToSpell(24321);
            //    return;
            //}
            case 28059: // Thaddius positive charge, removing aplify effect on remove
            {
                if (target->HasAura(29659))
                    target->RemoveAurasDueToSpell(29659);
                break;
            }
            case 28084: // Thaddius negative charge, removing aplify effect on remove
            {
                if (target->HasAura(29660))
                    target->RemoveAurasDueToSpell(29660);
                break;
            }
        }

        /*if (m_removeMode == AURA_REMOVE_BY_DEATH) // redundant, AM is cancelled in aura holder removal
        {
            // Stop caster Arcane Missle chanelling on death
            if (GetSpellProto()->IsFitToFamily<SPELLFAMILY_MAGE, CF_MAGE_ARCANE_MISSILES_CHANNEL>())
            {
                if (Unit* caster = GetCaster())
                    caster->InterruptSpell(CURRENT_CHANNELED_SPELL);

                return;
            }
        }*/
    }

    // AT APPLY & REMOVE

    switch (GetSpellProto()->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            switch (GetId())
            {
                case 36202: // Shared Fate
                {
                    if (apply)
                    {
                        std::list<Creature*> SharedList;
                        GetCreatureListExceptInGrid(SharedList, target, true, 62, 10.0f);

                        if (!SharedList.empty())
                        {
                            for (std::list<Creature*>::iterator itr = SharedList.begin(); itr != SharedList.end(); ++itr)
                            {
                                if ((*itr) && (*itr) != target && (*itr)->isAlive() && !(*itr)->IsMoving() && (*itr)->IsWithinLOSInMap(target))
                                {
                                    (*itr)->CastSpell(target, 36205, true);
                                }
                            }
                        }
                    }
                    else
                    {
                        target->RemoveAurasDueToSpell(36205);
                    }

                    return;
                }
                case 36199: // Doppelganger
                {
                    if (!target->IsCreature())
                        return;

                    if (apply)
                    {
                        float dist = 1.0f;
                        float angle = target->GetOrientation();

                        float x = target->GetPositionX() - dist * cos(angle);
                        float y = target->GetPositionY() - dist * sin(angle);
                        float z = target->GetPositionZ();

                        float summonAngle = angle + M_PI_F;

                        if (Creature* crea = target->SummonCreature(target->GetEntry(), x, y, z, summonAngle, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 600000))
                        {
                            crea->AddAura(36210);   //Visual
                            if (crea->GetDisplayId() != target->GetDisplayId())
                                crea->SetDisplayId(target->GetDisplayId());

                            crea->SetUInt32Value(UNIT_FIELD_BYTES_1, target->GetUInt32Value(UNIT_FIELD_BYTES_1));
                            crea->SetByteValue(UNIT_FIELD_BYTES_2, 0, target->GetByteValue(UNIT_FIELD_BYTES_2, 0));
                            crea->SetByteValue(UNIT_FIELD_BYTES_2, 1, target->GetByteValue(UNIT_FIELD_BYTES_2, 1));

                            uint32 emoteState = target->GetUInt32Value(UNIT_NPC_EMOTESTATE);
                            if (emoteState)
                                crea->SetUInt32Value(UNIT_NPC_EMOTESTATE, emoteState);
                        }
                    }
                    else
                    {
                        std::list<Creature*> DoppelList;
                        GetCreatureListWithEntryInGrid(DoppelList, target, target->GetEntry(), 100.0f);

                        for (std::list<Creature*>::iterator itr = DoppelList.begin(); itr != DoppelList.end(); ++itr)
                        {
                            if ((*itr) != target && (*itr)->IsTemporarySummon())
                            {
                                TemporarySummon* ts = static_cast<TemporarySummon*>((*itr));
                                if (ts->GetSummonerGuid() == target->GetObjectGuid())
                                {
                                    (*itr)->RemoveAllDynObjects();
                                    (*itr)->DespawnOrUnsummon();
                                }
                            }
                        }
                    }
                    return;
                }
                case 36161: //NPC Sit
                {
                    if (apply)
                        if (target->IsCreature())
                            target->ToCreature()->SeatCreatureOnNearestChair(1.5f);


                    return;
                }
                case 35951:
                {
                    if (Unit* DemonBoss = GetCaster())
                    {
                        if (apply)
                        {
                            target->CastSpell(DemonBoss, 35952, true);
                        }
                        else
                        {
                            DemonBoss->RemoveAurasByCasterSpell(35952, target->GetObjectGuid());
                        }
                    }

                    return;
                }
                case 35887:
                {
                    if (apply)
                    {
                        if (target->HasAura(35886))
                            target->RemoveAurasDueToSpell(35886);
                    }
                    else
                    {
                        if (target->HasAura(35885))
                            target->RemoveAurasDueToSpell(35885);
                    }
                    target->CastSpell(target, 35896, true); //Visual

                    return;
                }
                //Weapon Expertise Warrior/Rogue/Hunter
                case 35801:
                case 35802:
                case 35803:
                case 35804:
                case 35805:
                case 35806:
                case 35807:
                case 35808:
                case 35809:
                case 35810:
                case 35811:
                case 35812:
                case 35813:
                case 35814:
                case 35815:
                {
                    uint32 weapspell1 = 0, weapspell2 = 0, weapspell3 = 0;

                    switch (GetId())
                    {
                        //Warrior sword/poleaxe/mace
                        case 35801: weapspell1 = 12281; weapspell2 = 12700; weapspell3 = 12284;  break;
                        case 35802: weapspell1 = 12812; weapspell2 = 12781; weapspell3 = 12701;  break;
                        case 35803: weapspell1 = 12813; weapspell2 = 12783; weapspell3 = 12702;  break;
                        case 35804: weapspell1 = 12814; weapspell2 = 12784; weapspell3 = 12703;  break;
                        case 35805: weapspell1 = 12815; weapspell2 = 12785; weapspell3 = 12704;  break;
                        //Rogue sword/close-q/mace
                        case 35806: weapspell1 = 13960; weapspell2 = 13706; weapspell3 = 13709;  break;;
                        case 35807: weapspell1 = 13961; weapspell2 = 13804; weapspell3 = 13800;  break;
                        case 35808: weapspell1 = 13962; weapspell2 = 13805; weapspell3 = 13801;  break;
                        case 35809: weapspell1 = 13963; weapspell2 = 13806; weapspell3 = 13802;  break;
                        case 35810: weapspell1 = 13964; weapspell2 = 13807; weapspell3 = 13803;  break;
                        //Hunter xbow/bow/gun
                        case 35811: weapspell1 = 33536; weapspell2 = 33541; weapspell3 = 33546;  break;
                        case 35812: weapspell1 = 33537; weapspell2 = 33542; weapspell3 = 33547;  break;
                        case 35813: weapspell1 = 33538; weapspell2 = 33543; weapspell3 = 33548;  break;
                        case 35814: weapspell1 = 33539; weapspell2 = 33544; weapspell3 = 33549;  break;
                        case 35815: weapspell1 = 33540; weapspell2 = 33545; weapspell3 = 33550;  break;
                        default: break;
                    }

                    if (weapspell1 > 0)
                    {
                        if (apply)
                        {
                            target->AddAura(weapspell1);
                            target->AddAura(weapspell2);
                            target->AddAura(weapspell3);
                        }
                        else
                        {
                            target->RemoveAurasDueToSpell(weapspell1);
                            target->RemoveAurasDueToSpell(weapspell2);
                            target->RemoveAurasDueToSpell(weapspell3);
                        }
                    }
                    return;
                }
                //Stoicism
                case 35742:
                {
                    if (!apply)
                    {
                        int32 TotalDamage = 0;
                        int32 TotalHeal = 0;
                        Unit* TrueKiller = NULL;
                        Unit::AuraList const& mDummy = target->GetAurasByType(SPELL_AURA_DUMMY);
                        for (Unit::AuraList::const_iterator i = mDummy.begin(); i != mDummy.end(); ++i)
                        {
                            if ((*i)->GetSpellProto()->Id == 35743)
                            {
                                if (SpellAuraHolder* holder = (*i)->GetHolder())
                                {
                                    TotalDamage += (*i)->GetModifier()->m_amount;

                                    if ((*i)->GetCaster())
                                    {
                                        TrueKiller = (*i)->GetCaster();
                                    }
                                }
                            }

                            if ((*i)->GetSpellProto()->Id == 35744)
                            {
                                if (SpellAuraHolder* holder = (*i)->GetHolder())
                                {
                                    TotalHeal += (*i)->GetModifier()->m_amount;
                                }
                            }
                        }

                        // Clear counters
                        target->RemoveAurasDueToSpell(35743);
                        target->RemoveAurasDueToSpell(35744);

                        if (TotalDamage > TotalHeal)
                        {
                            TotalDamage -= TotalHeal;

                            target->RemoveSpellsCausingAura(SPELL_AURA_SCHOOL_IMMUNITY);

                            if (TrueKiller)
                            {
                                TrueKiller->CastCustomSpell(target, 35745, &TotalDamage, nullptr, nullptr, true, nullptr);
                            }
                            else
                            {
                                target->CastCustomSpell(target, 35745, &TotalDamage, nullptr, nullptr, true, nullptr);
                            }
                        }
                        else
                        {
                            TotalHeal -= TotalDamage;
                            target->CastCustomSpell(target, 35746, &TotalHeal, nullptr, nullptr, true, nullptr);
                        }
                    }

                    return;
                }
                // Arena Team visual Sword/Shield
                case 35437:
                {
                    if (target->HasAura(35430))
                        target->CastSpell(target, 35438, true);
                    if (target->HasAura(35431))
                        target->CastSpell(target, 35439, true);

                    return;
                }
                //Hysteria
                case 34956:
                {
                    if (apply)
                        target->AddAura(35380);
                    else
                        target->RemoveAurasDueToSpell(35380);

                    return;
                }
                // Amir Passive
                case 35359:
                {
                    uint32 SpellId = 0;
                    switch (target->getClass())
                    {
                    case CLASS_PRIEST:
                        SpellId = 35285;
                        break;
                    case CLASS_DRUID:
                        SpellId = 35286;
                        break;
                    case CLASS_SHAMAN:
                        SpellId = 35287;
                        break;
                    case CLASS_PALADIN:
                        SpellId = 35288;
                        break;
                    }

                    if (SpellId > 0)
                    {
                        if (apply)
                            target->AddAura(SpellId);
                        else
                            target->RemoveAurasDueToSpell(SpellId);
                    }

                    return;
                }
                //Divine Sacrifice Class Call Paladin
                case 35304:
                {
                    if (apply)
                    {
                        if (urand(0, 4) == 0)
                        {
                            target->AddAura(35305);
                        }
                    }
                    else
                    {
                        if (target->HasAura(35305))
                            target->CastSpell(target, 35306, false);
                    }

                    return;
                }
                case 34848:
                {
                    if (!apply)
                        if (GetRemoveMode() == AURA_REMOVE_BY_EXPIRE)
                            if (Unit* caster = GetCaster())
                                if (caster->HasAura(34847))
                                    caster->RemoveAurasDueToSpell(34847);
                    return;
                }
				case 2836: //Seek and Destroy
				{
					if (apply)
					{
						Unit* caster = GetCaster();
						if (!caster)
							return;

						if (caster->HasAura(33687) || caster->HasAura(33688))
							caster->CastSpell(caster, 34280, true);
					}
					else
						target->RemoveAurasDueToSpell(34280);
					return;
				}
                case 6606:                                  // Self Visual - Sleep Until Cancelled (DND)
                {
                    if (apply)
                    {
                        target->m_Events.AddLambdaEventAtOffset([target]
                        {
                            target->HandleEmoteState(EMOTE_STATE_SLEEP);
                            target->SetStandState(UNIT_STAND_STATE_SLEEP);
                        }, 500);
                    }

                    return;
                }
                case 24658:                                 // Unstable Power
                {
                    if (apply)
                    {
                        Unit* caster = GetCaster();
                        if (!caster)
                            return;

                        caster->CastSpell(target, 24659, true, nullptr, nullptr, GetCasterGuid());
                    }
                    else
                        target->RemoveAurasDueToSpell(24659);
                    return;
                }
                case 24661:                                 // Restless Strength
                {
                    if (apply)
                    {
                        Unit* caster = GetCaster();
                        if (!caster)
                            return;

                        caster->CastSpell(target, 24662, true, nullptr, nullptr, GetCasterGuid());
                    }
                    else
                        target->RemoveAurasDueToSpell(24662);
                    return;
                }
                case 29266:                                 // Permanent Feign Death
                {
                    // Unclear what the difference really is between them.
                    // Some has effect1 that makes the difference, however not all.
                    // Some appear to be used depending on creature location, in water, at solid ground, in air/suspended, etc
                    // For now, just handle all the same way
                    if (target->GetTypeId() == TYPEID_UNIT)
                        target->SetFeignDeath(apply);

                    return;
                }
                case 10848: // Shroud of Death
                case 27978: // Shroud of Death
                    if (apply)
                        target->m_AuraFlags |= UNIT_AURAFLAG_ALIVE_INVISIBLE;
                    else
                        target->m_AuraFlags |= ~UNIT_AURAFLAG_ALIVE_INVISIBLE;
                    return;
            }
            break;
        }
        case SPELLFAMILY_MAGE:
        {
#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_10_2
            switch (GetId())
            {
                // Frost Warding
                case 11189:
                case 28332:
                {
                    if (Player* pPlayer = target->ToPlayer())
                    {
                        if (apply)
                        {
                            SpellModifier* mod = new SpellModifier(SPELLMOD_RESIST_MISS_CHANCE, SPELLMOD_FLAT, m_modifier.m_amount, GetId(), UI64LIT(0x0000000000000100));
                            pPlayer->AddSpellMod(mod, true);
                        }
                        else
                        {
                            if (SpellModifier *mod = pPlayer->GetSpellMod(SPELLMOD_RESIST_MISS_CHANCE, GetId()))
                                pPlayer->AddSpellMod(mod, false);
                        }
                    }
                    return;
                }
                // Improved Fire Ward
                case 11094:
                case 13043:
                {
                    if (Player* pPlayer = target->ToPlayer())
                    {
                        if (apply)
                        {
                            SpellModifier *mod = new SpellModifier(SPELLMOD_RESIST_MISS_CHANCE, SPELLMOD_FLAT, m_modifier.m_amount, GetId(), UI64LIT(0x0000000000000008));
                            pPlayer->AddSpellMod(mod, true);
                        }
                        else
                        {
                            if (SpellModifier *mod = pPlayer->GetSpellMod(SPELLMOD_RESIST_MISS_CHANCE, GetId()))
                                pPlayer->AddSpellMod(mod, false);
                        }
                    }
                    return;
                }
            }
#endif
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            //Tranquility
            if (GetSpellProto()->SpellIconID == 100)
            {
                {
                    uint32 HealSpellId = 0;

                    switch (GetId())
                    {
                    case 740:   HealSpellId = 35768; break;
                    case 8918:  HealSpellId = 35769; break;
                    case 9862:  HealSpellId = 35770; break;
                    case 9863:  HealSpellId = 35771; break;
                    default:    break;
                    }

                    if (HealSpellId)
                    {
                        if (apply)
                            target->CastSpell(target, HealSpellId, true);
                        else
                            target->RemoveAurasDueToSpell(HealSpellId);
                    }
                }
            }
            // Predatory Strikes
            if (target->GetTypeId() == TYPEID_PLAYER && GetSpellProto()->SpellIconID == 1563)
            {
                ((Player*)target)->UpdateAttackPowerAndDamage();
                return;
            }
            // Enrage
            if ((target->GetTypeId() == TYPEID_PLAYER) && (GetId() == 5229))
            {
                ((Player*)target)->UpdateArmor(); // Spell managed in UpdateArmor()
                return;
            }
            break;
        }
        case SPELLFAMILY_PRIEST:
        {
            switch (GetId())
            {
                //Insanity
                case 35781:
                {
                    Unit* caster = GetCaster();
                    if (!caster)
                        return;
                    if (apply)
                    {
                        int32 Duration = GetSpellProto()->GetMaxDuration();
                        if (Player* modOwner = caster->GetSpellModOwner())
                            modOwner->ApplySpellMod(GetSpellProto()->Id, SPELLMOD_DURATION, Duration);


                        caster->SetInCombatWith(target);
                        target->setFaction(100);

                        if (Unit* NearestUnit = target->FindNearestTarget_Insanity(25.0f))
                        {
                            caster->SetInCombatWith(NearestUnit);
                            NearestUnit->CastSpell(target, 35772, true);
                        }
                    }
                    else
                    {
                        target->RemoveAurasDueToSpell(35772);
                        // restore faction
                        Creature* pCreatureTarget = target->ToCreature();
                        Player* pPlayerTarget = target->ToPlayer();
                        if (pCreatureTarget)
                        {
                            CreatureInfo const* cinfo = pCreatureTarget->GetCreatureInfo();

                            if (target->IsPet())
                            {
                                if (Unit* owner = target->GetOwner())
                                    target->setFaction(owner->getFaction());
                                else if (cinfo)
                                    target->setFaction(cinfo->faction);
                            }
                            else if (cinfo)                             // normal creature
                                target->setFaction(cinfo->faction);
                        }
                        if (pPlayerTarget)
                        {
                            pPlayerTarget->SetFactionForRace(target->getRace());
                            target->UpdateVisibilityAndView();
                        }
                    }
                    return;
                }
            }

            break;
        }
        case SPELLFAMILY_WARRIOR:
        {
            break;
        }
        case SPELLFAMILY_ROGUE:
        {
            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            break;
        }
        case SPELLFAMILY_SHAMAN:
        {
            switch (GetId())
            {
                case 6495:                                  // Sentry Totem
                {
                    if (target->GetTypeId() != TYPEID_PLAYER)
                        return;

                    if (!apply)
                        if (target->HasAura(35902))
                            target->RemoveAurasDueToSpell(35902);

                    /*Totem* totem = target->GetTotem(TOTEM_SLOT_AIR);

                    if (totem && apply)
                        ((Player*)target)->GetCamera().SetView(totem);
                    else
                        ((Player*)target)->GetCamera().ResetView();*/

                    

                    return;
                }
            }
            break;
        }
    }

    // pet auras
    if (PetAura const* petSpell = sSpellMgr.GetPetAura(GetId()))
    {
        if (apply)
            target->AddPetAura(petSpell);
        else
            target->RemovePetAura(petSpell);
        return;
    }

    if (GetEffIndex() == EFFECT_INDEX_0 && target->GetTypeId() == TYPEID_PLAYER)
    {
        SpellAreaForAreaMapBounds saBounds = sSpellMgr.GetSpellAreaForAuraMapBounds(GetId());
        if (saBounds.first != saBounds.second)
        {
            uint32 zone, area;
            target->GetZoneAndAreaId(zone, area);

            for (SpellAreaForAreaMap::const_iterator itr = saBounds.first; itr != saBounds.second; ++itr)
            {
                // some auras remove at aura remove
                if (!itr->second->IsFitToRequirements((Player*)target, zone, area))
                    target->RemoveAurasDueToSpell(itr->second->spellId);
                // some auras applied at aura apply
                else if (itr->second->autocast)
                {
                    if (!target->HasAura(itr->second->spellId, EFFECT_INDEX_0))
                        target->CastSpell(target, itr->second->spellId, true);
                }
            }
        }
    }

    // script has to "handle with care", only use where data are not ok to use in the above code.
    if (target->GetTypeId() == TYPEID_UNIT)
        sScriptMgr.OnAuraDummy(this, apply);
}

void Aura::HandleAuraMounted(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    Unit *target = GetTarget();

    if (apply)
    {
        CreatureInfo const* ci = ObjectMgr::GetCreatureTemplate(m_modifier.m_miscvalue);
        if (!ci)
        {
            sLog.outErrorDb("AuraMounted: `creature_template`='%u' not found in database (only need it modelid)", m_modifier.m_miscvalue);
            return;
        }

        uint32 display_id = Creature::ChooseDisplayId(ci);
        CreatureModelInfo const *minfo = sObjectMgr.GetCreatureModelRandomGender(display_id);
        if (minfo)
            display_id = minfo->modelid;

        //Azshara Boots of Travel
        if (target->HasAura(34984))
            target->RemoveAurasDueToSpell(34984);

        target->Mount(display_id, GetId());
    }
    else
    {
        target->Unmount(true);

        //Azshara Boots of Travel
        if (target->HasAura(35052))
            target->AddAura(34984);
    }
}

void Aura::HandleAuraWaterWalk(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    GetTarget()->SetWaterWalking(apply);
}

void Aura::HandleAuraFeatherFall(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    Unit *target = GetTarget();
    GetTarget()->SetFeatherFall(apply);

    if (!apply)
    {
        if (Player* player = target->ToPlayer())
        {
            Position const* currentPos = player->m_movementInfo.HasMovementFlag(MOVEFLAG_ONTRANSPORT)
                ? player->m_movementInfo.GetTransportPos()
                : player->m_movementInfo.GetPos();

            player->SetFallInformation(currentPos->z);
        }
    }
}

void Aura::HandleAuraHover(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    GetTarget()->SetHover(apply);
}

void Aura::HandleWaterBreathing(bool /*apply*/, bool /*Real*/)
{
    // update timers in client
    if (GetTarget()->GetTypeId() == TYPEID_PLAYER)
        ((Player*)GetTarget())->UpdateMirrorTimers();
}

std::pair<unsigned int, float> getShapeshiftModelInfo(ShapeshiftForm form, Unit *target){
    unsigned int modelid = 0;
    float mod = 1;
    switch (form)
    {
    case FORM_CAT:
        
        if (target->IsPlayer())
        {
            if (Player::TeamForRace(target->getRace()) == ALLIANCE)
                modelid = 892;
            else
                modelid = 8571;
        }
        else
            modelid = 892;
        mod = 0.80f;
        break;
    case FORM_TRAVEL:
        modelid = 632;
        mod = 0.80f;
        break;
    case FORM_AQUA:
        modelid = 2428;
        mod = 0.80f;
        break;
    case FORM_BEAR:
    case FORM_DIREBEAR:
        if (target->IsPlayer())
        {
            if (Player::TeamForRace(target->getRace()) == ALLIANCE)
                modelid = 2281;
            else
                modelid = 2289;
        }
        else
            modelid = 2281;
        break;
    /*case FORM_GHOUL:
        if (Player::TeamForRace(target->getRace()) == ALLIANCE)
            modelid = 10045;
        break;*/
    case FORM_CREATUREBEAR:
        modelid = 902;
        break;
    case FORM_GHOSTWOLF:
        modelid = 4613;
        mod = 0.80f;
        break;
    case FORM_GHOUL:
        modelid = 14555;
        mod = 1.7f;
        break;
    case FORM_MOONKIN:
        if (target->IsPlayer())
        {
            if (Player::TeamForRace(target->getRace()) == ALLIANCE)
                modelid = 15374;
            else
                modelid = 15375;
        }
        else
            modelid = 15374;
        break;
    case FORM_TREE:
        modelid = 864;
        break;
    case FORM_SPIRITOFREDEMPTION:
        modelid = 16031;
        break;
    /*case FORM_BATTLESTANCE:
    case FORM_BERSERKERSTANCE:
    case FORM_DEFENSIVESTANCE:
    case FORM_AMBIENT:
    case FORM_SHADOW:
    case FORM_STEALTH:*/
    default:
        break;
    }
    return {modelid,mod};
}

void Aura::HandleAuraModShapeshift(bool apply, bool Real)
{
    if (!Real)
        return;
    ShapeshiftForm form = ShapeshiftForm(m_modifier.m_miscvalue);

    Unit *target = GetTarget();

    SpellShapeshiftFormEntry const* ssEntry = sSpellShapeshiftFormStore.LookupEntry(form);
    if (!ssEntry)
    {
        sLog.outError("Unknown shapeshift form %u in spell %u", form, GetId());
        return;
    }

    switch (form)
    {
        case FORM_CAT:
        {
            if (target->HasAuraType(SPELL_AURA_TRANSFORM))
                target->RemoveSpellsCausingAura(SPELL_AURA_TRANSFORM);
            // Cast Shapeshift Form Effect to remove slows and roots.
            target->CastSpell(target, 9033, true);

            // cast regular cat form when removing Nefarian druid call
            if (!apply && GetSpellProto()->Id == 23398)
            {
                HandleShapeshiftBoosts(apply);
                target->CastSpell(target, 768, true);
                return;
            }
            break;
        }
        case FORM_TREE:
        case FORM_TRAVEL:
        case FORM_AQUA:
        case FORM_BEAR:
        case FORM_DIREBEAR:
        {
            if (target->HasAuraType(SPELL_AURA_TRANSFORM))
                target->RemoveSpellsCausingAura(SPELL_AURA_TRANSFORM);
            // Cast Shapeshift Form Effect to remove slows and roots.
            target->CastSpell(target, 9033, true);

            // cast regular bear form when removing Nefarian druid call
            if (!apply && GetSpellProto()->Id == 35303)
            {
                HandleShapeshiftBoosts(apply);
                target->CastSpell(target, 5487, true);
                return;
            }
            break;
        }
        case FORM_MOONKIN:
        {
            if (target->HasAuraType(SPELL_AURA_TRANSFORM))
                target->RemoveSpellsCausingAura(SPELL_AURA_TRANSFORM);
            // Cast Shapeshift Form Effect to remove slows and roots.
            target->CastSpell(target, 9033, true);
            break;
        }
        case FORM_BERSERKERSTANCE:
        {
            // cast regular berserker stance when removing Nefarian warrior call
            if (!apply && GetSpellProto()->Id == 23397)
            {
                HandleShapeshiftBoosts(apply);
                target->CastSpell(target, 2458, true);
                /*return;*/
            }
            break;
        }
        case FORM_GHOSTWOLF:
        {
            if (target->HasAuraType(SPELL_AURA_TRANSFORM))
                target->RemoveSpellsCausingAura(SPELL_AURA_TRANSFORM);
            break;
        }
        default:
            break;
    }

    std::pair<unsigned int, float> info = getShapeshiftModelInfo(form, target);
    unsigned int modelid = info.first;
    if (modelid > 0 && !target->getTransForm())
    {
        if (apply)
        {
            target->setTransformScale(info.second);
            target->SetDisplayId(modelid);
        }
        else
        {
            target->resetTransformScale();
            target->SetDisplayId(target->GetNativeDisplayId());
        }
    }

    if (apply)
    {


        Powers PowerType = POWER_MANA;
        switch (form)
        {
        case FORM_CAT:
            PowerType = POWER_ENERGY;
            break;
        case FORM_BEAR:
        case FORM_DIREBEAR:
            PowerType = POWER_RAGE;
            break;
        /*case FORM_TRAVEL:
        case FORM_AQUA:
        case FORM_GHOUL:
        case FORM_CREATUREBEAR:
        case FORM_GHOSTWOLF:
        case FORM_MOONKIN:
        case FORM_AMBIENT:
        case FORM_SHADOW:
        case FORM_STEALTH:
        case FORM_TREE:
        case FORM_SPIRITOFREDEMPTION:
            break;*/
        case FORM_BATTLESTANCE:
        case FORM_BERSERKERSTANCE:
        case FORM_DEFENSIVESTANCE:
            PowerType = POWER_RAGE;
            break;
        default:
            break;
        }
        // remove other shapeshift before applying a new one
        target->RemoveSpellsCausingAura(SPELL_AURA_MOD_SHAPESHIFT, GetHolder());

        if (PowerType != POWER_MANA)
        {
            // reset power to default values only at power change
            uint32 powaa = target->GetPower(PowerType);
            if (target->getPowerType() != PowerType)
                target->setPowerType(PowerType);

            switch (form)
            {
                case FORM_CAT:
                case FORM_BEAR:
                case FORM_DIREBEAR:
                {
                    if (m_modifier.m_miscvalue == FORM_CAT)
                    {
                        target->SetPower(POWER_ENERGY, 0);
                        if (target->HasAura(17056) && (!target->HasAura(34200) || target->HasAura(23724))) //r1
                            target->CastSpell(target, 17099, true, nullptr, this),
						    target->CastSpell(target, 34200, true, nullptr, this); //vanilla+ furor
						if (target->HasAura(17058) && (!target->HasAura(34202) || target->HasAura(23724)))  //r2
							target->CastSpell(target, 17099, true, nullptr, this),
							target->CastSpell(target, 34202, true, nullptr, this);
                    }
                    else
                    {
                        target->SetPower(POWER_RAGE, powaa);
                        if (target->HasAura(17056) && (!target->HasAura(34201) || target->HasAura(23724))) //r1
                            target->CastSpell(target, 17057, true, nullptr, this),
						    target->CastSpell(target, 34201, true, nullptr, this); //v+
						if (target->HasAura(17058) && (!target->HasAura(34203) || target->HasAura(23724))) //r2
							target->CastSpell(target, 17057, true, nullptr, this), 
							target->CastSpell(target, 34203, true, nullptr, this);

                    }
                    break;
                }
                case FORM_BATTLESTANCE:
                case FORM_DEFENSIVESTANCE:
                case FORM_BERSERKERSTANCE:
                {
                    uint32 Rage_val = 50;
                    Player* pPlayer = target->ToPlayer();
                    //Tactical mastery
                    if (target->GetTypeId() == TYPEID_PLAYER)
                    {
                        Unit::AuraList const& aurasOverrideClassScripts = target->GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
                        for (Unit::AuraList::const_iterator iter = aurasOverrideClassScripts.begin(); iter != aurasOverrideClassScripts.end(); ++iter)
                        {
                            // select by script id
                            switch ((*iter)->GetModifier()->m_miscvalue)
                            {
                                case 831:
                                    Rage_val += 100;
                                    break;
                                case 832:
                                    Rage_val += 200;
                                    break;
                                case 833:
                                    Rage_val += 300;
                                    break;
                                case 834:
                                    Rage_val = 50;
                                    break;
                                case 835:
                                    Rage_val = 50;
                                    break;
                            }
                            if (Rage_val != 0)
                                break;
                        }
                    }
                    if (form == FORM_DEFENSIVESTANCE && target->GetPower(POWER_RAGE) > Rage_val)
                    {
                        //Imp Defensive Stance
                        if (target->HasAura(12797) || target->HasAura(12799))
                        {
                            int32 ExtraRage = (target->GetPower(POWER_RAGE) - Rage_val) / 10;
                            if (ExtraRage > 0)
                            {
                                ExtraRage *= target->GetMaxHealth() / 100;

                                if (target->HasAura(12797))
                                    target->CastCustomSpell(target, 35818, &ExtraRage, nullptr, nullptr, true);
                                if (target->HasAura(12799))
                                    target->CastCustomSpell(target, 35819, &ExtraRage, nullptr, nullptr, true);
                            }
                        }
                    }
                    if (target->GetPower(POWER_RAGE) > Rage_val)
                        target->SetPower(POWER_RAGE, Rage_val);

                    break;
                }
                default:
                    break;
            }
        }

        target->SetShapeshiftForm(form);
    }
    else
    {
        if (target->getClass() == CLASS_DRUID)
        {
            target->setPowerType(POWER_MANA);
            target->SetPower(POWER_RAGE, 0);
        }

        target->SetShapeshiftForm(FORM_NONE);
    }

    // adding/removing linked auras
    // add/remove the shapeshift aura's boosts
    HandleShapeshiftBoosts(apply);
    target->UpdateModelData();

    if (target->GetTypeId() == TYPEID_PLAYER)
        ((Player*)target)->InitDataForForm();
}

void Aura::HandleAuraTransform(bool apply, bool Real)
{
    Unit *target = GetTarget();
    if (apply)
    {
        float mod_x = 1;
        uint32 model_id = 0;

        // Discombobulate removes mount auras.
        if (GetId() == 4060 && Real)
            target->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);


        // update active transform spell only not set or not overwriting negative by positive case
        if (!target->getTransForm() || !IsPositiveSpell(GetId()) || IsPositiveSpell(target->getTransForm()))
        {
            if (GetId() == 35377)   // Ustaag <Nostalrius> : Nefarian Class Call Mage
            {
                int rand = 0;
                rand = urand(0, 10);
                switch (rand)
                {
                    case 0:
                        model_id = 1060;
                        break;
                    case 1:
                        model_id = 4473;
                        break;
                    case 2:
                        model_id = 7898;
                        break;
                    case 3:
                        model_id = 304;
                        break;
                    case 4:
                        model_id = 856;
                        break;
                    case 5:
                        model_id = 857;
                        break;
                    case 6:
                        model_id = 16356;
                        break;
                    case 7:
                        model_id = 16357;
                        break;
                    case 8:
                        model_id = 16358;
                        break;
                    case 9:
                        model_id = 8013;
                        break;
                    case 10:
                        model_id = 2176;
                        break;
                }
            }
            else if (m_modifier.m_miscvalue == 0)         // special case (spell specific functionality)
            {
                switch (GetId())
                {
                    //case 24255:                                 // Primal Blessing
                    //{
                    //    uint8 gender = sObjectMgr.GetCreatureModelInfo(target->GetDisplayId())->gender;
                    //    if (gender == GENDER_MALE)
                    //        model_id = 25047;
                    //    else
                    //        model_id = 25048;

                    //    break;
                    //}
                    case 36159: // SM Polymorph
                    {
                        static const uint32 models[] =
                        {
                            304,    // Chicken
                            856,    // Sheep white
                            857,    // Sheep black
                            16356,  // Pig white
                            16357,  // Pig black
                            16358,  // Pig pink
                            2176,   // White rat
                            5554,   // Orange cat
                            5555,   // Striped cat
                            5556    // Black cat
                        };

                        model_id = models[urand(0, (sizeof(models) / sizeof(uint32)) - 1)];

                        break;
                    }
                    // Dark Blessing of the Old God
                    case 34994:
                    {
                        target->AddAura(35071);
                        uint8 gender = sObjectMgr.GetCreatureModelInfo(target->GetDisplayId())->gender;
                        mod_x = 1.5f;
                        int randMod = 0;
                        randMod = urand(0, 9);
                        switch (randMod)
                        {
                            //Murloc
                        case 0:
                            if (target->getGender() == GENDER_MALE)
                                model_id = 26031;
                            else
                                model_id = 26048;
                            break;
                        case 1:
                            if (target->getGender() == GENDER_MALE)
                                model_id = 26032;
                            else
                                model_id = 26049;
                            break;
                        case 2:
                            if (target->getGender() == GENDER_MALE)
                                model_id = 26033;
                            else
                                model_id = 26050;
                            break;
                        case 3:
                            if (target->getGender() == GENDER_MALE)
                                model_id = 26034;
                            else
                                model_id = 26051;
                            break;
                        case 4:
                            if (target->getGender() == GENDER_MALE)
                                model_id = 26035;
                            else
                                model_id = 26052;
                            break;
                            //Naga
                        case 5:
                            if (target->getGender() == GENDER_MALE)
                                model_id = 26036;
                            else
                                model_id = 26041;
                            break;
                        case 6:
                            if (target->getGender() == GENDER_MALE)
                                model_id = 26037;
                            else
                                model_id = 26042;
                            break;
                        case 7:
                            if (target->getGender() == GENDER_MALE)
                                model_id = 26038;
                            else
                                model_id = 26043;
                            break;
                        case 8:
                            if (target->getGender() == GENDER_MALE)
                                model_id = 26039;
                            else
                                model_id = 26044;
                            break;
                        case 9:
                            if (target->getGender() == GENDER_MALE)
                                model_id = 26040;
                            else
                                model_id = 26045;
                            break;
                        }

                        break;
                    }
                    case 36281: // Two-Faced Medallion/Borrowed Face
                    {
                        model_id = uint32(m_modifier.m_amount);

                        if (!model_id)
                            return;

                        break;
                    }
                    case 16739: // Orb of Deception
                    {
                        uint8 gender = sObjectMgr.GetCreatureModelInfo(target->GetDisplayId())->gender;
                        switch (target->getRace())
                        {
                        case RACE_TROLL:
                            model_id = gender == GENDER_MALE ? 10135 : 10134 ;
                            break;
                        case RACE_TAUREN:
                            model_id = gender == GENDER_MALE ? 10136 : 10147 ;
                            break;
                        case RACE_HUMAN:
                            model_id = gender == GENDER_MALE ? 10137 : 10138 ;
                            break;
                        case RACE_ORC:
                            model_id = gender == GENDER_MALE ? 10139 : 10140 ;
                            break;
                        case RACE_DWARF:
                            model_id = gender == GENDER_MALE ? 10141 : 10142 ;
                            break;
                        case RACE_NIGHTELF:
                            model_id = gender == GENDER_MALE ? 10143 : 10144 ;
                            /*model_id = gender == GENDER_MALE ? 57 : 58;*/
                            break;
                        case RACE_UNDEAD:
                            model_id = gender == GENDER_MALE ? 10146 : 10145 ;
                            break;
                        case RACE_GNOME:
                            if (gender == GENDER_MALE)
                            {
                                model_id = 10148;
                                mod_x = DEFAULT_TAUREN_MALE_SCALE;
                            }
                            else
                            {
                                model_id = 10149;
                                mod_x = DEFAULT_TAUREN_FEMALE_SCALE;
                            }
                            break;
                        default:
                            break;
                        }

                        if (model_id)
                        {
                            target->SetNativeDisplayId(model_id);
                        }

                        break;
                    }
                    default:
                        sLog.outError("Aura::HandleAuraTransform, spell %u does not have creature entry defined, need custom defined model.", GetId());
                        break;
                }
            }
            else
            {
                CreatureInfo const * ci = ObjectMgr::GetCreatureTemplate(m_modifier.m_miscvalue);
                if (!ci)
                {
                    model_id = 16358;                           // pig pink ^_^
                    sLog.outError("Auras: unknown creature id = %d (only need its modelid) Form Spell Aura Transform in Spell ID = %d", m_modifier.m_miscvalue, GetId());
                }
                else
                    model_id = Creature::ChooseDisplayId(ci);   // Will use the default model here

                // creature case, need to update equipment
                if (ci && target->GetTypeId() == TYPEID_UNIT)
                {
                    ((Creature*)target)->LoadEquipment(ci->equipment_id, true);
                    mod_x = ci->scale;
                }
                    
            }

            if (model_id)
            {
                target->SetDisplayId(model_id);
                target->setTransformScale(mod_x);
            }
            target->setTransForm(GetId());
        }
    }
    else
    {
        switch (GetId())
        {
            case 16739: // Orb of Deception reset
            /*case 36281:*/ // Two-faced medallion/Borrowed Face
            {
                target->InitPlayerDisplayIds();
                float OrbScale = 1.0f;
                Unit::AuraList const& ScaleSpell = target->GetAurasByType(SPELL_AURA_MOD_SCALE);
                for (Unit::AuraList::const_iterator i = ScaleSpell.begin(); i != ScaleSpell.end(); ++i)
                {
                    OrbScale *= 1.0f + float((*i)->GetModifier()->m_amount) / 100.0f;
                }
                target->SetObjectScale(OrbScale);

                break;
            }
            case 34994:
            {
                target->RemoveAurasDueToSpell(35071);
                break;
            }
        }

        //reset cosmetics only if it's the current transform
        if (target->getTransForm() == GetId())
        {
            target->setTransForm(0);
            target->SetDisplayId(target->GetNativeDisplayId());
            target->resetTransformScale();

            // apply default equipment for creature case
            if (target->GetTypeId() == TYPEID_UNIT)
                ((Creature*)target)->LoadEquipment(((Creature*)target)->GetCreatureInfo()->equipment_id, true);


            // re-apply some from still active with preference negative cases
            Unit::AuraList const& otherTransforms = target->GetAurasByType(SPELL_AURA_TRANSFORM);
            if (!otherTransforms.empty())
            {
                // look for other transform auras
                Aura* handledAura = *otherTransforms.rbegin();
                for (Unit::AuraList::const_reverse_iterator i = otherTransforms.rbegin(); i != otherTransforms.rend(); ++i)
                {
                    // negative auras are preferred
                    if (!(*i)->GetSpellProto()->IsPositiveSpell())
                    {
                        handledAura = *i;
                        break;
                    }
                }
                handledAura->HandleAuraTransform(true,false);
            }
            else //reapply shapeshifting, there should be only one.
            {
                std::pair<unsigned int, float> info = getShapeshiftModelInfo(target->GetShapeshiftForm(), target);
                if (info.first)
                {
                    target->SetDisplayId(info.first);
                    target->setTransformScale(info.second);
                }
            }
        }
    }


    //Chromatic Mutation
    if (GetId() == 23174)
    {
        if (apply)
        {
            target->AddAura(23175); //Mod DMG 500% + Mod Haste Melee 100 + Mod Haste Spell 300
            target->AddAura(23177); //Max Health 15000 + Mod healing 1000% + Mod Ranged  100
            target->AddAura(35363); //Dummy Aura + Cost 80 + Lang
        }
        else
        {
            target->RemoveAurasDueToSpell(23175); //Mod DMG 500% + Mod Haste Melee 100 + Mod Haste Spell 300
            target->RemoveAurasDueToSpell(23177); //Max Health 15000 + Mod healing 1000% + Mod Ranged  100
            target->RemoveAurasDueToSpell(35363); //Dummy Aura + Cost 80 + Lang
        }
    }
}

void Aura::HandleForceReaction(bool apply, bool Real)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    if (!Real)
        return;

    Player* player = (Player*)GetTarget();

    uint32 faction_id = m_modifier.m_miscvalue;
    ReputationRank faction_rank = ReputationRank(m_modifier.m_amount);

    player->GetReputationMgr().ApplyForceReaction(faction_id, faction_rank, apply);
    player->GetReputationMgr().SendForceReactions();

    // stop fighting if at apply forced rank friendly or at remove real rank friendly
    if ((apply && faction_rank >= REP_FRIENDLY) || (!apply && player->GetReputationRank(faction_id) >= REP_FRIENDLY))
        player->StopAttackFaction(faction_id);

    // sorry, need to hack ashbringer to make you friendly with scarlet crusade too
    if (m_spellAuraHolder && m_spellAuraHolder->GetSpellProto() &&  m_spellAuraHolder->GetId() == 28282)
    {
        uint32 scarlet_crusade_faction_id = 56;
        ReputationRank scarlet_crusade_rank = ReputationRank(7);
        player->GetReputationMgr().ApplyForceReaction(scarlet_crusade_faction_id, scarlet_crusade_rank, apply);
        player->GetReputationMgr().SendForceReactions();

        // stop fighting if at apply forced rank friendly or at remove real rank friendly
        if ((apply && scarlet_crusade_rank >= REP_FRIENDLY) || (!apply && player->GetReputationRank(scarlet_crusade_faction_id) >= REP_FRIENDLY))
            player->StopAttackFaction(scarlet_crusade_faction_id);
    }

    if (m_spellAuraHolder && m_spellAuraHolder->GetSpellProto() && m_spellAuraHolder->GetId() == 34655)
    {
        int32 const fac_id[8][1] =
        {
            { 530},
            { 81},
            { 68},
            { 72},
            { 47},
            { 54},
            { 69},
            { 76}
        };

        uint32 faction_id = 1;
        int i = 1;
        ReputationRank faction_rank = ReputationRank(0);

        while (i != 9)
        {
            faction_id = fac_id[i][0];
            i++;
            player->GetReputationMgr().ApplyForceReaction(faction_id, faction_rank, apply);
            player->GetReputationMgr().SendForceReactions();
        }
    }

    if (!apply && player->GetZoneId() == 1377 && GetId() == 29519 && m_removeMode == AURA_REMOVE_BY_DEATH)
    {
        // OutdoorPVP Silithus : Perte du buff silithyste
        if (ZoneScript* pScript = player->GetZoneScript())
            pScript->HandleDropFlag(player, GetId());
    }

}

void Aura::HandleAuraModSkill(bool apply, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    const uint16 skillId = uint16(GetSpellProto()->EffectMiscValue[m_effIndex]);

    // Can't modify an unknown skill
    if (!GetTarget()->ToPlayer()->HasSkill(skillId))
    {
        // Revert m_applied assigned in Aura::ApplyModidier
        m_applied = !apply;
        return;
    }

    Player* target = static_cast<Player*>(GetTarget());
    Modifier const* mod = GetModifier();
    const int16 amount = int16(mod->m_amount);
    const bool permanent = (mod->m_auraname == SPELL_AURA_MOD_SKILL_TALENT);

    if (target->ModifySkillBonus(skillId, (apply ? amount : -amount), permanent))
    {
        if (skillId == SKILL_DEFENSE)
            target->UpdateDefenseBonusesMod();
    }
}
void Aura::HandleChannelDeathItem(bool apply, bool Real)
{
    if (Real && !apply)
    {
        if (m_removeMode != AURA_REMOVE_BY_DEATH)
            return;
        // Item amount
        if (m_modifier.m_amount <= 0)
            return;

        SpellEntry const *spellInfo = GetSpellProto();
        if (spellInfo->EffectItemType[m_effIndex] == 0)
            return;

        Unit* victim = GetTarget();
        Unit* caster = GetCaster();
        if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
            return;
        // Demonistes : un seul fragment d'ame si on caste "Brulure de l'ombre" et "Siphon d'ame" sur une cible.
        if (spellInfo->SpellFamilyName == SPELLFAMILY_WARLOCK)
            if (victim->HasAuraTypeByCaster(SPELL_AURA_CHANNEL_DEATH_ITEM, caster->GetObjectGuid()))
                return;

        // Soul Shard (target req.)
        if (spellInfo->EffectItemType[m_effIndex] == 6265)
        {
            //CD reset
            caster->CastSpell(caster, 35729, true);
            // Only from non-grey units
            if (!((Player*)caster)->IsHonorOrXPTarget(victim) ||
                    (victim->GetTypeId() == TYPEID_UNIT && !victim->ToCreature()->IsTappedBy(caster->ToPlayer())))
                return;
        }

        if (spellInfo->Id == 20647)
        {
            if (!caster->IsPlayer())
                return;

            //CD reset
            caster->CastSpell(caster, 34600, true);
            /*((Player*)caster)->SendSpellCooldown(SPELL_ID_WEAPON_SWITCH_COOLDOWN_1_5s, 0, caster->GetObjectGuid());*/
            return;
        }

        //Adding items
        uint32 noSpaceForCount = 0;
        uint32 count = m_modifier.m_amount;

        ItemPosCountVec dest;
        InventoryResult msg = ((Player*)caster)->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, spellInfo->EffectItemType[m_effIndex], count, &noSpaceForCount);
        if (msg != EQUIP_ERR_OK)
        {
            count -= noSpaceForCount;
            ((Player*)caster)->SendEquipError(msg, nullptr, nullptr, spellInfo->EffectItemType[m_effIndex]);
            if (count == 0)
                return;
        }

        Item* newitem = ((Player*)caster)->StoreNewItem(dest, spellInfo->EffectItemType[m_effIndex], true);
        ((Player*)caster)->SendNewItem(newitem, count, true, true);
    }
}

void Aura::HandleBindSight(bool apply, bool /*Real*/)
{
    Unit* caster = GetCaster();
    if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Camera& camera = ((Player*)caster)->GetCamera();
    if (apply)
        camera.SetView(GetTarget());
    else
        camera.ResetView();
}

void Aura::HandleFarSight(bool apply, bool /*Real*/)
{
    Unit* caster = GetCaster();
    if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
        return;

    if (apply)
        caster->ToPlayer()->SetLongSight(this);
    else
        caster->ToPlayer()->SetLongSight();
}

void Aura::HandleAuraTrackCreatures(bool apply, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    if (apply)
        GetTarget()->RemoveNoStackAurasDueToAuraHolder(GetHolder());

    if (apply)
        GetTarget()->SetFlag(PLAYER_TRACK_CREATURES, uint32(1) << (m_modifier.m_miscvalue - 1));
    else
        GetTarget()->RemoveFlag(PLAYER_TRACK_CREATURES, uint32(1) << (m_modifier.m_miscvalue - 1));
}

void Aura::HandleAuraTrackResources(bool apply, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    if (apply)
        GetTarget()->RemoveNoStackAurasDueToAuraHolder(GetHolder());

    if (apply)
        GetTarget()->SetFlag(PLAYER_TRACK_RESOURCES, uint32(1) << (m_modifier.m_miscvalue - 1));
    else
        GetTarget()->RemoveFlag(PLAYER_TRACK_RESOURCES, uint32(1) << (m_modifier.m_miscvalue - 1));
}

void Aura::HandleAuraTrackStealthed(bool apply, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    if (apply)
        GetTarget()->RemoveNoStackAurasDueToAuraHolder(GetHolder());

    GetTarget()->ApplyModByteFlag(PLAYER_FIELD_BYTES, 0, PLAYER_FIELD_BYTE_TRACK_STEALTHED, apply);
}

void Aura::HandleAuraModScale(bool apply, bool /*Real*/)
{
    Unit* pTarget = GetTarget();
    Unit* pCaster = GetCaster();

    if (m_modifier.m_amount < 0)
        if (pTarget->IsCreature() && pTarget != pCaster)
            if (pTarget->ToCreature()->IsElite())
                return;

    pTarget->ApplyPercentModFloatValue(OBJECT_FIELD_SCALE_X, float(m_modifier.m_amount), apply);
    pTarget->UpdateModelData();
}

void Aura::HandleModPossess(bool apply, bool Real)
{
    if (!Real)
        return;

    Unit* pTarget = GetTarget();
    Unit* pCaster = GetCaster();
    if (!pCaster || !pTarget)
        return;

#if SUPPORTED_CLIENT_BUILD <= CLIENT_BUILD_1_9_4
    // Fix issue with control on old clients if player is afflicted by mind control,
    // while he himself is possessing another unit. Client needs to first be told
    // he no longer controls the other unit, have time to respond with set active mover,
    // and only then to be told he is mind controlled in order to have control disabled.
    if (pTarget->GetCharmGuid())
    {
        pTarget->CombatStop(true);
        AuraRemoveMode removeMode = m_removeMode;
        SpellSchoolMask schoolMask = GetSpellProto()->GetSpellSchoolMask();
        pTarget->m_Events.AddLambdaEventAtOffset([pTarget, pCaster, apply, removeMode, schoolMask]
        {
            pCaster->ModPossess(pTarget, apply, removeMode);
            pTarget->AddThreat(pCaster, pTarget->GetHealth(), false, schoolMask);
        }, 500);
    }
    else
#endif
    
    if (apply)
    {
        /*if (GetId() == 605 || GetId() == 10911 || GetId() == 10912)
        {
            float range_mod = ((pCaster->IsPlayer() ? 6.25f : 2.25f) + pCaster->GetLeewayBonusRange(pTarget, true));
            float max_range = 1.0f;

            if (Player* modOwner = pCaster->GetSpellModOwner())
                modOwner->ApplySpellMod(GetSpellProto()->Id, SPELLMOD_RANGE, max_range);

            max_range += range_mod;
            float dist = pCaster->GetCombatDistance(pTarget);
            if (dist > max_range)
                return;
        }*/
    }
    else
    {
        if (GetId() == 24937) // Controlling Steam Tonk
        {
            pTarget->CastSpell(pTarget, 27771, true); // Cast Damaged Tonk
            pCaster->CastSpell(pCaster, 9179, true); // Cast 3 sec Stun on self
            pCaster->RemoveAurasDueToSpell(24935); // Unroot player)
        }
    }

    {
        pCaster->ModPossess(pTarget, apply, m_removeMode);
        pTarget->AddThreat(pCaster, pTarget->GetHealth(), false, GetSpellProto()->GetSpellSchoolMask());
    }
}

void Unit::ModPossess(Unit* pTarget, bool apply, AuraRemoveMode m_removeMode)
{
    // Cannot possess yourself.
    if (pTarget == this)
        return;

    // Only a player can possess units, mobs can charm.
    Player* pCaster = ToPlayer();
    if (!pCaster)
        return;

    if (apply)
    {
        // Remove dummy auras from spells with EffectSummonPossessed.
        pTarget->RemoveAurasDueToSpell(126);
        pTarget->RemoveAurasDueToSpell(6272);
        pTarget->RemoveAurasDueToSpell(11403);

        FactionTemplateEntry const* origFactionTemplate = pTarget->getFactionTemplateEntry();

        pTarget->addUnitState(UNIT_STAT_POSSESSED);
        pTarget->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_POSSESSED);
        pTarget->SetCharmerGuid(pCaster->GetObjectGuid());
        pTarget->SetPossessorGuid(pCaster->GetObjectGuid());
        pTarget->setFaction(pCaster->getFaction());

        // Target should became visible at SetView call (if not visible before),
        // otherwise client will ignore packets from the target (SetClientControl for example).
        pCaster->GetCamera().SetView(pTarget);

        pCaster->SetCharm(pTarget);
        pCaster->SetMover(pTarget);

        pTarget->CombatStop(true);

        if (CharmInfo *charmInfo = pTarget->InitCharmInfo(pTarget))
        {
            charmInfo->InitPossessCreateSpells();
            charmInfo->SetReactState(REACT_PASSIVE);
            charmInfo->SetCommandState(COMMAND_STAY);
            charmInfo->SetOriginalFactionTemplate(origFactionTemplate);
        }

        pCaster->PossessSpellInitialize();

        if (Creature* pCreature = pTarget->ToCreature())
            if (!pCreature->hasUnitState(UNIT_STAT_CAN_NOT_REACT))
                if (pCreature->AI()->SwitchAiAtControl())
                    pCreature->AIM_Initialize();

        // Mobs should attack the target being mind controlled.
        // So we call 'MoveInLineOfSight' for nearby mobs.
        pTarget->ScheduleAINotify(0);

        pTarget->UpdateControl();
        if (pTarget->hasUnitState(UNIT_STAT_STUNNED | UNIT_STAT_PENDING_STUNNED | UNIT_STAT_ROOT | UNIT_STAT_PENDING_ROOT))
            pTarget->SetRooted(true);
        pTarget->StopMoving();
        pTarget->SetWalk(pCaster->IsWalking());
    }
    else
    {
        // Clear threat generated when MC ends.
        pTarget->RemoveAttackersThreat(pCaster);

        pCaster->SetMover(nullptr);
        pCaster->SetCharm(nullptr);
        pCaster->UpdateControl();
        pCaster->SetClientControl(pTarget, false);

        // There is a possibility that target became invisible for client at ResetView call.
        // It must be called after movement control unapplying, not before! The reason is same as at aura applying.
        pCaster->GetCamera().ResetView();
        pCaster->RemovePetActionBar();

        // On delete only do caster related effects.
        if (m_removeMode == AURA_REMOVE_BY_DELETE)
            return;

        pTarget->clearUnitState(UNIT_STAT_POSSESSED);
        pTarget->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_POSSESSED);
        pTarget->SetCharmerGuid(ObjectGuid());
        pTarget->SetPossessorGuid(ObjectGuid());

        if (Player* pPlayerTarget = pTarget->ToPlayer())
        {
            pPlayerTarget->RemoveAI();
            pPlayerTarget->RelocateToLastClientPosition(); // Movement interpolation - prevent undermap.
        }

        pTarget->RestoreFaction();
        pTarget->CombatStop(true);
        pTarget->UpdateControl();
        pTarget->SetWalk(false);

        if (Creature* pCreature = pTarget->ToCreature())
        {
            if (!pCreature->hasUnitState(UNIT_STAT_CAN_NOT_REACT))
            {
                pTarget->StopMoving(true);
                if (pCreature->AI() && pCreature->AI()->SwitchAiAtControl())
                    pCreature->AIM_Initialize();

                pCreature->AttackedBy(pCaster);
            }
        }
        else
            pTarget->StopMoving(true);

        // cast mind exhaustion on self when the posess possess ends if the creature
        // is death knight understudy (razuvious). 
        // todo: if there is a way to know a possess has ended through scriptAI, fix this.
        if (pTarget->GetEntry() == 16803)
        {
            pTarget->CastSpell(pTarget, 29051, true); 
        }
        if (pTarget->GetEntry() == 25020 || pTarget->GetEntry() == 25021)
        {
            pTarget->CastSpell(pTarget, 34807, true);
            pCaster->CastSpell(pCaster, 34808, true);
        }

        if (pTarget->IsPlayer() && pTarget->IsRooted())
            MovementPacketSender::AddMovementFlagChangeToController(pTarget, MOVEFLAG_ROOT, true);
    }

#if SUPPORTED_CLIENT_BUILD <= CLIENT_BUILD_1_9_4
    pTarget->SendCreateUpdateToPlayer(pCaster);
#endif
}

void Aura::HandleModPossessPet(bool apply, bool Real)
{
    if (!Real)
        return;

    Player* pCaster = ToPlayer(GetCaster());
    if (!pCaster)
        return;

    Unit* pTarget = GetTarget();
    if (!pTarget->IsPet())
        return;

    Pet* pPet = (Pet*)pTarget;
    pCaster->ModPossessPet(pPet, apply, m_removeMode);
}

void Player::ModPossessPet(Pet* pPet, bool apply, AuraRemoveMode m_removeMode)
{
    Player* pCaster = this;
    Camera& camera = pCaster->GetCamera();

    if (apply)
    {
        pPet->addUnitState(UNIT_STAT_POSSESSED);

        // Target should became visible at SetView call(if not visible before),
        // otherwise client will ignore packets from the target(SetClientControl for example).
        camera.SetView(pPet);

        pCaster->SetCharm(pPet);
        pCaster->SetMover(pPet);

        pPet->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_POSSESSED);
        pPet->SetCharmerGuid(pCaster->GetObjectGuid());
        pPet->SetPossessorGuid(pCaster->GetObjectGuid());

        pPet->StopMoving();
        pPet->GetMotionMaster()->Clear(false);
        pPet->GetMotionMaster()->MoveIdle();
        pPet->UpdateControl();
        pPet->SetWalk(pCaster->IsWalking());

        if (CharmInfo* charmInfo = pPet->GetCharmInfo())
        {
            charmInfo->SetIsAtStay(false);
            charmInfo->SetIsReturning(false);
            charmInfo->SetIsFollowing(false);
        }

        if (pPet->IsRooted())
            MovementPacketSender::AddMovementFlagChangeToController(pPet, MOVEFLAG_ROOT, true);
    }
    else
    {
        pCaster->SetCharm(nullptr);
        pCaster->SetMover(nullptr);

        // There is a possibility that target became invisible for client at ResetView call.
        // It must be called after movement control unapplying, not before! the reason is same as at aura applying.
        camera.ResetView();
        pPet->UpdateControl();
        pPet->SetCharmerGuid(ObjectGuid());
        pPet->SetPossessorGuid(ObjectGuid());

        if (!pPet->hasUnitState(UNIT_STAT_CAN_NOT_REACT))
            pPet->StopMoving(true);

        // To avoid moving the wrong unit on server side between cancellation and mover swap
        // the pet has the controlled state removed in WorldSession::HandleSetActiveMoverOpcode
    }

#if SUPPORTED_CLIENT_BUILD <= CLIENT_BUILD_1_9_4
    pPet->SendCreateUpdateToPlayer(pCaster);
#endif
}

void Aura::HandleModCharm(bool apply, bool Real)
{
    if (!Real)
        return;

    Unit *target = GetTarget();

    if (!target)
        return;

    // not charm yourself
    if (GetCasterGuid() == target->GetObjectGuid())
        return;

    Unit* caster = GetCaster();

    if (apply)
    {
        if (!caster)
            return;

        FactionTemplateEntry const* origFactionTemplate = target->getFactionTemplateEntry();

        // Remove dummy auras from spells with EffectSummonPossessed
        target->RemoveAurasDueToSpell(126);
        target->RemoveAurasDueToSpell(6272);
        target->RemoveAurasDueToSpell(11403);

        // is it really need after spell check checks?
        target->RemoveSpellsCausingAura(SPELL_AURA_MOD_CHARM, GetHolder());
        target->RemoveSpellsCausingAura(SPELL_AURA_MOD_POSSESS, GetHolder());
        target->RemoveSpellsCausingAura(SPELL_AURA_AOE_CHARM, GetHolder()); // this is a bit dodgy, like most removals of SPELL_AURA_AOE_CHARM 

        target->SetCharmerGuid(GetCasterGuid());
        target->setFaction(caster->getFaction());
        target->CastStop(target == caster ? GetId() : 0);
        caster->SetCharm(target);

        target->CombatStop(true);
        target->DeleteThreatList();
        target->getHostileRefManager().deleteReferences();

        CharmInfo *charmInfo = target->InitCharmInfo(target);
        charmInfo->InitCharmCreateSpells();
        charmInfo->SetReactState(REACT_DEFENSIVE);
        // Default movement is follow
        charmInfo->SetCommandState(COMMAND_FOLLOW);

        charmInfo->SetIsCommandAttack(false);
        charmInfo->SetIsAtStay(false);
        charmInfo->SetIsReturning(true);
        charmInfo->SetIsCommandFollow(true);
        charmInfo->SetIsFollowing(false);
        charmInfo->SetOriginalFactionTemplate(origFactionTemplate);

        target->AttackStop();
        target->InterruptNonMeleeSpells(false);

        if (caster->IsPlayer()) // Units will make the controlled player attack (MoveChase)
            target->GetMotionMaster()->MoveFollow(caster, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);

        if (Creature* pCreatureTarget = target->ToCreature())
        {
            if (pCreatureTarget->AI() && pCreatureTarget->AI()->SwitchAiAtControl())
                pCreatureTarget->AIM_Initialize();

            if (caster->IsPlayer() && caster->getClass() == CLASS_WARLOCK)
            {
                CreatureInfo const *cinfo = pCreatureTarget->GetCreatureInfo();
                if (cinfo && cinfo->type == CREATURE_TYPE_DEMON)
                {
                    // creature with pet number expected have class set
                    if (target->GetByteValue(UNIT_FIELD_BYTES_0, 1) == 0)
                    {
                        if (cinfo->unit_class == 0)
                            sLog.outErrorDb("Creature (Entry: %u) have unit_class = 0 but used in charmed spell, that will be result client crash.", cinfo->entry);
                        else
                            sLog.outError("Creature (Entry: %u) have unit_class = %u but at charming have class 0!!! that will be result client crash.", cinfo->entry, cinfo->unit_class);

                        target->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_MAGE);
                    }

                    //just to enable stat window
                    charmInfo->SetPetNumber(sObjectMgr.GeneratePetNumber(), true);
                    //if charmed two demons the same session, the 2nd gets the 1st one's name
                    target->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(time(nullptr)));
                }
            }
        }
        else if (Player* pPlayer = target->ToPlayer())
        {
            if (caster->IsCreature())
            {
                pPlayer->SetControlledBy(caster);
                if (pPlayer->i_AI && m_spellAuraHolder->GetId() == 28410)
                    pPlayer->i_AI->enablePositiveSpells = true;
            }
            else
            {
                PlayerAI *oldAi = pPlayer->i_AI;
                delete oldAi;
                pPlayer->i_AI = new PlayerControlledAI(pPlayer, caster);
            }
        }
        target->UpdateControl();

        if (caster->IsPlayer())
            static_cast<Player*>(caster)->CharmSpellInitialize();
    }
    else
    {
        Creature* pCreatureTarget = target->ToCreature();
        Player* pPlayerTarget = target->ToPlayer();

        target->SetCharmerGuid(ObjectGuid());

        if (pCreatureTarget)
        {
            CreatureInfo const *cinfo = pCreatureTarget->GetCreatureInfo();

            // restore faction
            if (target->IsPet())
            {
                if (Unit* owner = target->GetOwner())
                    target->setFaction(owner->getFaction());
                else if (cinfo)
                    target->setFaction(cinfo->faction);
            }
            else if (cinfo)                             // normal creature
                target->setFaction(cinfo->faction);

            // restore UNIT_FIELD_BYTES_0
            if (cinfo && caster && caster->IsPlayer() && caster->getClass() == CLASS_WARLOCK && cinfo->type == CREATURE_TYPE_DEMON)
            {
                // DB must have proper class set in field at loading, not req. restore, including workaround case at apply
                // target->SetByteValue(UNIT_FIELD_BYTES_0, 1, cinfo->unit_class);

                if (target->GetCharmInfo())
                    target->GetCharmInfo()->SetPetNumber(0, true);
                else
                    sLog.outError("Aura::HandleModCharm: target (GUID: %u TypeId: %u) has a charm aura but no charm info!", target->GetGUIDLow(), target->GetTypeId());
            }
        }

        if (caster)
        {
            caster->SetCharm(nullptr);
            if (caster->IsPlayer())
                static_cast<Player*>(caster)->RemovePetActionBar();
        }
        
        target->UpdateControl();

        if (pPlayerTarget)
            pPlayerTarget->SetFactionForRace(target->getRace());

        if (pPlayerTarget && pPlayerTarget->isAlive() && caster && caster->isAlive() && caster->isInCombat())
        {
            pPlayerTarget->SendAttackSwingCancelAttack();

            if (target->IsNonMeleeSpellCasted(false))
                target->InterruptNonMeleeSpells(false);

            target->AttackStop();
            target->RemoveAllAttackers();
            target->DeleteThreatList();
            target->getHostileRefManager().deleteReferences();

            caster->SetInCombatWith(target);
            target->SetInCombatWith(caster);

            target->SetInCombatState(false, caster);
        }
        else
        {
            target->CombatStop(true);
            target->DeleteThreatList();
            target->getHostileRefManager().deleteReferences();
        }

        //target->SetUnitMovementFlags(MOVEFLAG_NONE);
        target->StopMoving();
        target->GetMotionMaster()->Clear(false);
        target->GetMotionMaster()->MoveIdle();

        if (pCreatureTarget)
        {
            if (pCreatureTarget->AI() && pCreatureTarget->AI()->SwitchAiAtControl())
                pCreatureTarget->AIM_Initialize();

            if (caster)
                pCreatureTarget->AttackedBy(caster);
        }
        else if (pPlayerTarget)
        {
            pPlayerTarget->RemoveAI();

            // Charmed players are seen as hostile and not in the group for other clients, restore
            // group upon charm end
            if(pPlayerTarget->GetGroup())
                pPlayerTarget->GetGroup()->BroadcastGroupUpdate();
        }
    }

    switch (GetId())
    {
    case 34902: //Jindo Call
    {
        if (apply)
        {
            target->AddAura(34903);
            target->AddAura(34904);
        }
        else
        {
            target->RemoveAurasDueToSpell(34903);
            target->RemoveAurasDueToSpell(34904);
        }
        break;
    }
    case 35782: //Insanity on kill. Insanity removed by combatstop
    {
        if (!apply)
        {
            if (target->HasAura(35781))
                target->RemoveAurasDueToSpell(35781);
        }

        break;
    }
    case 36013: // Dominate Mind (Whitemane)
    {
        if (apply)
        {
            target->AddAura(36014);
            target->AddAura(36015);
            target->AddAura(36016);
        }
        else
        {
            target->RemoveAurasDueToSpell(36014);
            target->RemoveAurasDueToSpell(36015);
            target->RemoveAurasDueToSpell(36016);
        }

        break;
    }
    default:
    break;
    }
}

void Aura::HandleModConfuse(bool apply, bool Real)
{
    if (!Real)
        return;

    if (Unit* pTarget = GetTarget())
    {
        switch (GetId())
        {
            case 2094:  //V+ Blinding
            {
                if (pTarget->IsPlayer())
                    pTarget->UpdateVisibilityAndView();
                break;
            }
            case 35168: // Greatest polymoprh
            {
                if (apply)
                {
                    pTarget->AddAura(35181);
                }
                else
                {
                    pTarget->RemoveAurasDueToSpell(35181);
                }

                break;
            }
            case 36159: // SM poly-duel
            {
                if (apply)
                {
                    if (Unit* pCaster = GetCaster())
                    {
                        pCaster->CastSpell(pTarget, 36157, true);    //Poly CD
                        pCaster->CastSpell(pCaster, 36158, true);   //Poly Caster Emote
                    }
                }
                else
                {
                    if (pTarget->IsCreature())
                        pTarget->ToCreature()->GetMotionMaster()->MoveTargetedHome();
                }

                break;
            }
        }
    }


    GetTarget()->SetConfused(apply, GetCasterGuid(), GetId());
}

void Aura::HandleModFear(bool apply, bool Real)
{
    if (!Real)
        return;

    GetTarget()->SetFeared(apply, GetCasterGuid(), GetId());
}

void Aura::HandleFeignDeath(bool apply, bool Real)
{
    if (!Real)
        return;

    bool success = true;
    Unit* pTarget = GetTarget();
    
    if (apply)
    {
        HostileReference* pReference = pTarget->getHostileRefManager().getFirst();
        while (pReference)
        {
            if (Creature* refTarget = ToCreature(pReference->getSourceUnit()))
            {
                if (!refTarget->GetCharmerOrOwnerOrSelf()->IsPlayer() && refTarget->IsWithinDistInMap(pTarget, refTarget->GetAttackDistance(pTarget))
                    && pTarget->MagicSpellHitResult(refTarget, GetHolder()->GetSpellProto(), nullptr) != SPELL_MISS_NONE)
                {
                    success = false;
                    break;
                }
            }
            pReference = pReference->next();
        }
    }

	if (GetId() == 34363) //v+ Fake death
		success = false;

    pTarget->SetFeignDeath(apply, GetCasterGuid(), success);
}

void Aura::HandleAuraModDisarm(bool apply, bool Real)
{
    if (!Real)
        return;

    Unit *target = GetTarget();

    if (!apply && target->HasAuraType(SPELL_AURA_MOD_DISARM))
        return;

    // Main-hand speed and attack damage is already set for feral form (and they cannot be disarmed anyway)
    // If the form is switched the disarm speed/damage will be applied automatically
    // Only update swing timers for players on disarm
    // https://www.youtube.com/watch?v=8TDUpudEL-M&t=6m5s
    // Furthermore, we need to apply/unapply weapon mods for players on disarm
    // so they don't have weapon stats (or talent boosts) whilst disarmed
    if (target->GetTypeId() == TYPEID_PLAYER && !target->IsInFeralForm())
    {
        Player* pTarget = target->ToPlayer();

        // (un)applying weapon dependent mods is more practical on a non disarmed player
        // if we unapply the aura remove the flag before _ApplyWeaponDependentAuraMods
        if (!apply)
        {
            target->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISARMED, false);
            pTarget->SetRegularAttackTime();
        }

        if (Item* weapon = pTarget->GetWeaponForAttack(BASE_ATTACK, true, true))
            pTarget->_ApplyWeaponDependentAuraMods(weapon, BASE_ATTACK, !apply);

        // if we apply the aura add the flag after _ApplyWeaponDependentAuraMods
        if (apply)
        {
            target->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISARMED, true);
            target->SetAttackTime(BASE_ATTACK, BASE_ATTACK_TIME);
        }
    }
    else
        target->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISARMED, apply);

    // Warrior 'Disarm' skill generates 104 base threat
    // http://wowwiki.wikia.com/wiki/Disarm?direction=prev&oldid=200198 (2006) implies it 
    // generates a large amount
    // http://wowwiki.wikia.com/wiki/Threat has threat listed at 104, which is in line
    // with the values of other warrior abilities
    // We can suppose that the same is true for all spells which apply disarm
    if (apply)
    {
        float threat = 204.0f * sSpellMgr.GetSpellThreatMultiplier(GetHolder()->GetSpellProto());
        target->AddThreat(GetCaster(), threat, false, SPELL_SCHOOL_MASK_NONE, GetHolder()->GetSpellProto());
    }

    // Don't update damage if in feral
    if (!target->IsInFeralForm())
        target->UpdateDamagePhysical(BASE_ATTACK);
}

void Aura::HandleAuraModStun(bool apply, bool Real)
{
    if (!Real)
        return;

    Unit *target = GetTarget();


    switch (GetId())
    {
        // Imp Sap
        case 6770:
        case 2070:
        case 11297:
        {
            uint32 BrainDamageSpellId2 = 0;
            if (Unit* caster = GetCaster())
            {
                if (caster->HasAura(14076))
                    BrainDamageSpellId2 = 35726;
                else if (caster->HasAura(14094))
                    BrainDamageSpellId2 = 35727;
                else if (caster->HasAura(14095))
                    BrainDamageSpellId2 = 35728;
            }

            if (apply)
            {
                if (BrainDamageSpellId2 > 0)
                    target->AddAura(35725);
            }
            else
            {
                target->RemoveAurasDueToSpell(35725);

                if (BrainDamageSpellId2 > 0)
                    target->AddAura(BrainDamageSpellId2);
            }

            break;
        }
        // Devour visual
        case 34318:
        case 34319:
        {
            if (Unit* caster = GetCaster())
            {
                if (apply)
                    caster->CastSpell(target, 36216, true);
                else
                    target->RemoveAurasDueToSpell(36216);
            }

            break;
        }
        
    }

    if (apply)
    {
        if (target->IsTaxiFlying())
            return;
        // Stun/roots effects apply at charge end
        bool inCharge = target->GetMotionMaster()->GetCurrentMovementGeneratorType() == CHARGE_MOTION_TYPE;
        // Frost stun aura -> freeze/unfreeze target
        if (GetSpellProto()->GetSpellSchoolMask() & SPELL_SCHOOL_MASK_FROST)
            target->ModifyAuraState(AURA_STATE_FROZEN, apply);

        target->addUnitState(inCharge ? UNIT_STAT_PENDING_STUNNED : UNIT_STAT_STUNNED);
        target->SetTargetGuid(ObjectGuid());

        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
        target->CastStop(target->GetObjectGuid() == GetCasterGuid() ? GetId() : 0);
        if (target->GetObjectGuid() != GetCasterGuid())
            target->InterruptNonMeleeSpells(false);

        // Player specific
        if (Player* targetPlayer = target->ToPlayer())
        {
            if (!targetPlayer->IsMounted())
                targetPlayer->SetStandState(UNIT_STAND_STATE_STAND);// in 1.5 client

            // Release loot if any
            if (ObjectGuid lootGuid = targetPlayer->GetLootGuid())
                targetPlayer->GetSession()->DoLootRelease(lootGuid);


            if (GetId() == 17624)
            {
                targetPlayer->RemoveAurasDueToSpell(23333);
                targetPlayer->RemoveAurasDueToSpell(23335);
            }
            
            if (GetId() == 35276)
            {
                targetPlayer->RemoveAurasDueToSpell(23153);
                targetPlayer->RemoveAurasDueToSpell(23154);
                targetPlayer->RemoveAurasDueToSpell(23155);
                targetPlayer->RemoveAurasDueToSpell(23169);
                targetPlayer->RemoveAurasDueToSpell(23170);
            }
        }

        if (!target->movespline->Finalized() || target->GetTypeId() == TYPEID_UNIT)
            if (!inCharge)
                target->StopMoving();

        target->SetRooted(true);
    }
    else
    {
        // Frost stun aura -> freeze/unfreeze target
        if (GetSpellProto()->GetSpellSchoolMask() & SPELL_SCHOOL_MASK_FROST)
        {
            bool found_another = false;
            for (AuraType const* itr = &frozenAuraTypes[0]; *itr != SPELL_AURA_NONE; ++itr)
            {
                Unit::AuraList const& auras = target->GetAurasByType(*itr);
                for (Unit::AuraList::const_iterator i = auras.begin(); i != auras.end(); ++i)
                {
                    if ((*i)->GetSpellProto()->GetSpellSchoolMask() & SPELL_SCHOOL_MASK_FROST)
                    {
                        found_another = true;
                        break;
                    }
                }
                if (found_another)
                    break;
            }

            if (!found_another)
                target->ModifyAuraState(AURA_STATE_FROZEN, apply);
        }

        // Real remove called after current aura remove from lists, check if other similar auras active
        if (target->HasAuraType(SPELL_AURA_MOD_STUN))
            return;

        target->clearUnitState(UNIT_STAT_STUNNED | UNIT_STAT_PENDING_STUNNED);
        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);


        if (target->getVictim() && target->isAlive())
            target->SetTargetGuid(target->getVictim()->GetObjectGuid());

        if (!target->hasUnitState(UNIT_STAT_ROOT | UNIT_STAT_PENDING_ROOT))         // prevent allow move if have also root effect
            target->SetRooted(false);

        // Wyvern Sting
        if (GetSpellProto()->IsFitToFamily<SPELLFAMILY_HUNTER, CF_HUNTER_SERPENT_STING>())
        {
            Unit* caster = GetCaster();
            if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
                return;

            uint32 spell_id = 0;

            switch (GetId())
            {
                case 19386:
                    spell_id = 24131;
                    break;
                case 24132:
                    spell_id = 24134;
                    break;
                case 24133:
                    spell_id = 24135;
                    break;
                default:
                    sLog.outError("Spell selection called for unexpected original spell %u, new spell for this spell family?", GetId());
                    return;
            }

            SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(spell_id);

            if (!spellInfo)
                return;

            caster->CastSpell(target, spellInfo, true, nullptr, this);
            return;
        }
    }

    if (target->IsPlayer() && target->getClass() == CLASS_ROGUE)
    {
        if (target->HasAura(35830) || target->HasAura(35832)) // Survivor
        {
            if (apply)
            {
                target->RemoveAurasDueToSpell(35831);
            }
            else
            {
                int32 HPperc = 100 - uint32((float(target->GetHealth()) / target->GetMaxHealth()) * 100);

                if (target->HasAura(35832))
                    HPperc *= 0.5f;
                else if (target->HasAura(35830))
                    HPperc *= 0.25f;

                if (HPperc > 0 && !target->HasAuraType(SPELL_AURA_MOD_STUN))
                {
                    HPperc *= -1;
                    target->CastCustomSpell(target, 35831, &HPperc, nullptr, nullptr, true, nullptr);
                }
            }
        }
    }
}

void Aura::HandleModStealth(bool apply, bool Real)
{
    Unit *target = GetTarget();

    //Arena Stealth penalty
    if (target->GetZoneId() == 3625)
    {
        if (apply)
        {
            target->AddAura(35435);
        }
        else
        {
            target->RemoveAurasDueToSpell(35435);
        }
    }

    if (apply)
    {
        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            if (SpellAuraHolder *holder = ((Player*)target)->GetSpellAuraHolder(29519))
            {
                if (target->GetZoneId() == 1377)
                {
                    // OutdoorPVP Silithus : Perte du buff silithyste
                    if (ZoneScript* pScript = ((Player*)target)->GetZoneScript())
                        pScript->HandleDropFlag((Player*)target, 29519);
                }
            }
        }

        // drop flag at stealth in bg
        target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

        // only at real aura add
        if (Real)
        {
            target->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAGS_CREEP);
            if (target->GetTypeId() == TYPEID_PLAYER)
                target->SetByteFlag(PLAYER_FIELD_BYTES2, 1, PLAYER_FIELD_BYTE2_STEALTH);
            // apply only if not in GM invisibility (and overwrite invisibility state)
            if (target->GetVisibility() != VISIBILITY_OFF)
            {
                target->SetVisibility(VISIBILITY_GROUP_NO_DETECT);
                target->SetVisibility(VISIBILITY_GROUP_STEALTH);
            }

            // for RACE_NIGHTELF stealth
            if (target->GetTypeId() == TYPEID_PLAYER && GetId() == 20580)
                target->CastSpell(target, 21009, true, nullptr, this);

            target->InterruptSpellsCastedOnMe();

            //Enveloping Shadows while stealthed
            if (GetSpellProto()->IsFitToFamilyMask<CF_ROGUE_STEALTH>())
            {
                int32 rank = 0;
                if (target->HasAura(33993))
                    rank = 34579;
                if (target->HasAura(33994))
                    rank = 34580;
                if (target->HasAura(33995))
                    rank = 34581;

                if (rank > 0)
                    target->CastSpell(target, rank, true, nullptr, this);
            }
        }
    }
    else
    {
        // for RACE_NIGHTELF stealth
        if (Real && target->GetTypeId() == TYPEID_PLAYER && GetId() == 20580)
            target->RemoveAurasDueToSpell(21009);

        if (Real && (m_removeMode == AURA_REMOVE_BY_CANCEL) &&
            GetSpellProto()->IsFitToFamilyMask<CF_ROGUE_STEALTH>())
        {
            // https://us.forums.blizzard.com/en/wow/t/wow-classic-not-a-bug-list/175887/45?blzcmp=app
            // "Manually cancelling Stealth after using Vanish will
            //  remove the Vanish buff as well as the Stealth buff."
            target->RemoveAurasDueToSpellByCancel(11327);
            target->RemoveAurasDueToSpellByCancel(11329);
        }   

        // only at real aura remove of _last_ SPELL_AURA_MOD_STEALTH
        if (Real && !target->HasAuraType(SPELL_AURA_MOD_STEALTH))
        {
            // if no GM invisibility
            if (target->GetVisibility() != VISIBILITY_OFF)
            {
                target->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAGS_CREEP);
                if (target->GetTypeId() == TYPEID_PLAYER)
                    target->RemoveByteFlag(PLAYER_FIELD_BYTES2, 1, PLAYER_FIELD_BYTE2_STEALTH);
                // restore invisibility if any
                if (target->HasAuraType(SPELL_AURA_MOD_INVISIBILITY))
                {
                    target->SetVisibility(VISIBILITY_GROUP_NO_DETECT);
                    target->SetVisibility(VISIBILITY_GROUP_INVISIBILITY);
                }
                else
                    target->SetVisibility(VISIBILITY_ON);
            }
        }
    }
}

void Aura::HandleInvisibility(bool apply, bool Real)
{
    Unit *target = GetTarget();

    if (apply)
    {
        target->m_invisibilityMask |= (1 << m_modifier.m_miscvalue);
        target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

        if (Real && target->GetTypeId() == TYPEID_PLAYER)
        {
            // apply glow vision
            target->SetByteFlag(PLAYER_FIELD_BYTES2, 1, PLAYER_FIELD_BYTE2_INVISIBILITY_GLOW);
        }

        // apply only if not in GM invisibility and not stealth
        if (target->GetVisibility() == VISIBILITY_ON)
        {
            // Aura not added yet but visibility code expect temporary add aura
            target->SetVisibility(VISIBILITY_GROUP_NO_DETECT);
            target->SetVisibility(VISIBILITY_GROUP_INVISIBILITY);
        }
    }
    else
    {
        // recalculate value at modifier remove (current aura already removed)
        target->m_invisibilityMask = 0;
        Unit::AuraList const& auras = target->GetAurasByType(SPELL_AURA_MOD_INVISIBILITY);
        for (Unit::AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
            target->m_invisibilityMask |= (1 << (*itr)->GetModifier()->m_miscvalue);

        // only at real aura remove and if not have different invisibility auras.
        if (Real && target->m_invisibilityMask == 0)
        {
            // remove glow vision
            if (target->GetTypeId() == TYPEID_PLAYER)
                target->RemoveByteFlag(PLAYER_FIELD_BYTES2, 1, PLAYER_FIELD_BYTE2_INVISIBILITY_GLOW);
            // apply only if not in GM invisibility & not stealthed while invisible
            if (target->GetVisibility() != VISIBILITY_OFF)
            {
                // if have stealth aura then already have stealth visibility
                if (!target->HasAuraType(SPELL_AURA_MOD_STEALTH))
                    target->SetVisibility(VISIBILITY_ON);
            }
        }
    }
}

void Aura::HandleInvisibilityDetect(bool apply, bool Real)
{
    Unit *target = GetTarget();

    if (apply)
        target->m_detectInvisibilityMask |= (1 << m_modifier.m_miscvalue);
    else
    {
        // recalculate value at modifier remove (current aura already removed)
        target->m_detectInvisibilityMask = 0;
        Unit::AuraList const& auras = target->GetAurasByType(SPELL_AURA_MOD_INVISIBILITY_DETECTION);
        for (Unit::AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
            target->m_detectInvisibilityMask |= (1 << (*itr)->GetModifier()->m_miscvalue);
    }
    if (Real && target->GetTypeId() == TYPEID_PLAYER)
        ((Player*)target)->GetCamera().UpdateVisibilityForOwner();
}

void Aura::HandleAuraModRoot(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    Unit *target = GetTarget();



    switch (GetId())
    {
        case 35867:
        case 35868:
        {
            if (!target->IsCreature())
            return;

            if (!apply)
            {
                float MoveX = 484.0f;

                if (GetId() == 35868)
                {
                    MoveX = 395.0f;
                }

                target->GetMotionMaster()->MovePoint(1, MoveX, target->GetPositionY(), 48.0f, MOVE_RUN_MODE);
                target->AddAura(35879); //Poof visual
            }
            break;
        }
        case 35942: //Thorn volley
        {
            if (!apply)
            {
                target->CastSpell(target, 35943, true);
            }
            break;
        }
    }

    if (apply)
    {
        // Stun/roots effects apply at charge end
        bool inCharge = target->GetMotionMaster()->GetCurrentMovementGeneratorType() == CHARGE_MOTION_TYPE;
        // Frost root aura -> freeze/unfreeze target
        if (GetSpellProto()->GetSpellSchoolMask() & SPELL_SCHOOL_MASK_FROST)
            target->ModifyAuraState(AURA_STATE_FROZEN, apply);

        target->addUnitState(inCharge ? UNIT_STAT_PENDING_ROOT : UNIT_STAT_ROOT);

        //Save last orientation
        if (target->getVictim())
            target->SetOrientation(target->GetAngle(target->getVictim()));

        if (!target->movespline->Finalized() && !inCharge)
            target->StopMoving();

        target->SetRooted(true);
    }
    else
    {
        // Frost root aura -> freeze/unfreeze target
        if (GetSpellProto()->GetSpellSchoolMask() & SPELL_SCHOOL_MASK_FROST)
        {
            bool found_another = false;
            for (AuraType const* itr = &frozenAuraTypes[0]; *itr != SPELL_AURA_NONE; ++itr)
            {
                Unit::AuraList const& auras = target->GetAurasByType(*itr);
                for (Unit::AuraList::const_iterator i = auras.begin(); i != auras.end(); ++i)
                {
                    if ((*i)->GetSpellProto()->GetSpellSchoolMask() & SPELL_SCHOOL_MASK_FROST)
                    {
                        found_another = true;
                        break;
                    }
                }
                if (found_another)
                    break;
            }

            if (!found_another)
                target->ModifyAuraState(AURA_STATE_FROZEN, apply);
        }

        // Real remove called after current aura remove from lists, check if other similar auras active
        if (target->HasAuraType(SPELL_AURA_MOD_ROOT))
            return;

        target->clearUnitState(UNIT_STAT_ROOT | UNIT_STAT_PENDING_ROOT);

        if (!target->hasUnitState(UNIT_STAT_STUNNED | UNIT_STAT_PENDING_STUNNED))      // prevent allow move if have also stun effect
        {
            target->SetRooted(false);
            //target->SetUnitMovementFlags(MOVEFLAG_NONE);
        }
    }
}

void Aura::HandleAuraModSilence(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    Unit *target = GetTarget();

    if (apply)
    {
        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED);
        // Stop cast only spells vs PreventionType == SPELL_PREVENTION_TYPE_SILENCE
        for (uint32 i = CURRENT_MELEE_SPELL; i < CURRENT_MAX_SPELL; ++i)
            if (Spell* spell = target->GetCurrentSpell(CurrentSpellTypes(i)))
                if (spell->m_spellInfo->PreventionType == SPELL_PREVENTION_TYPE_SILENCE)
                    // Stop spells on prepare or casting state
                    target->InterruptSpell(CurrentSpellTypes(i), false);

        //Prophecy 3/4 on Silence
        if (target->HasAura(34573) && !target->HasAura(34575))
            target->CastSpell(target, 34574, true),
            target->CastSpell(target, 34575, true);
    }
    else
    {
        // Real remove called after current aura remove from lists, check if other similar auras active
        if (target->HasAuraType(SPELL_AURA_MOD_SILENCE))
            return;

        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED);
    }
}

void Aura::HandleModThreat(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    Unit *target = GetTarget();

    if (GetId() == 7376 || GetId() == 21178 || GetId() == 25780 || GetId() == 34599)
        if (target->HasAura(25909))
            target->RemoveAurasDueToSpell(25909);

    int level_diff = 0;
    int multiplier = 0;
    switch (GetId())
    {
        // Arcane Shroud
        case 26400:
            level_diff = target->getLevel() - 60;
            multiplier = 2;
            break;
        // The Eye of Diminution
        case 28862:
            level_diff = target->getLevel() - 60;
            multiplier = 1;
            break;
    }

    if (level_diff > 0)
        m_modifier.m_amount += multiplier * level_diff;

    if (target->GetTypeId() == TYPEID_PLAYER)
        for (int8 x = 0; x < MAX_SPELL_SCHOOL; x++)
            if (m_modifier.m_miscvalue & int32(1 << x))
                ApplyPercentModFloatVar(target->m_threatModifier[x], float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModTotalThreat(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    Unit *target = GetTarget();

    if (!target->isAlive() || target->GetTypeId() != TYPEID_PLAYER)
        return;

    Unit* caster = GetCaster();

    if (!caster || !caster->isAlive())
        return;

    target->getHostileRefManager().addTempThreat(m_modifier.m_amount, apply);
}

void Aura::HandleModTaunt(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    Unit *target = GetTarget();

    if (!target->isAlive())
        return;

    Unit* caster = GetCaster();

    //PvE Taunt
    if (!target->IsPlayer())
    {
        if (caster && caster->isAlive())
        {
            if (apply)
                target->TauntApply(caster);
            else
            {
                // When taunt aura fades out, mob will switch to previous target if current has less than 1.1 * secondthreat
                target->TauntFadeOut(caster);
            }
        }
    }
    //PvP Taunt
    else
    {
        if (apply)
        {
            if (target->GetCharmerGuid() || target->HasAuraType(SPELL_AURA_MOD_CHARM) || target->HasAuraType(SPELL_AURA_MOD_POSSESS) || target->HasAuraType(SPELL_AURA_AOE_CHARM))
                return;

            if (caster && caster->isAlive())
            {
                int32 Duration = GetSpellProto()->GetMaxDuration();
                if (Player* modOwner = caster->GetSpellModOwner())
                    modOwner->ApplySpellMod(GetSpellProto()->Id, SPELLMOD_DURATION, Duration);

                if (GetId() == 35772) //Insanity
                {
                    if (target->HasAura(35781))
                    {
                        Unit::AuraList const& mDummyAuras = target->GetAurasByType(SPELL_AURA_DUMMY);
                        for (Unit::AuraList::const_iterator i = mDummyAuras.begin(); i != mDummyAuras.end(); ++i)
                        {
                            if ((*i)->GetSpellProto()->Id == 35781)
                            {
                                Duration = (*i)->GetAuraDuration();
                            }
                        }
                    }
                }
                if (GetId() == 36124)   // Duel SM
                {
                    target->CastSpell(caster, 36125, true); // Duel Buff casted by Player at Champion
                }

                if (Spell* spell = target->GetCurrentSpell(CURRENT_GENERIC_SPELL))
                    if (spell->m_targets.getUnitTargetGuid() == caster->GetObjectGuid())
                        spell->ApplyRemainingCastTimePercentMod(150.0f);

                //Start Attack
                target->ClearTarget();
                /*target->UpdateVisibilityAndView();*/
                target->SetFacingToObject(caster);
                target->SetTargetGuid(caster->GetObjectGuid());
                /*target->AttackerStateUpdate(caster);*/

                if (Creature* TauntNPC = target->SummonCreature(25132, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ() + 2.0f, 0.0f, TEMPSUMMON_TIMED_DESPAWN, Duration))
                {
                    TauntNPC->SetInCombatWith(caster);
                    TauntNPC->getThreatManager().addThreatDirectly(caster, 100000.0f);

                    if (Player* tauntedPlayer = target->ToPlayer())
                    {
                        if (tauntedPlayer->getClass() == CLASS_WARRIOR)
                        {
                            if (tauntedPlayer->GetShapeshiftForm() != FORM_BERSERKERSTANCE && tauntedPlayer->HasSpell(2458))
                                tauntedPlayer->CastSpell(tauntedPlayer, 2458, true);
                            else if (tauntedPlayer->GetShapeshiftForm() == FORM_DEFENSIVESTANCE)
                                tauntedPlayer->CastSpell(tauntedPlayer, 2457, true);
                        }
                    }

                    target->StopMoving(true);
                    target->RemoveUnitMovementFlag(MOVEFLAG_MASK_MOVING_OR_TURN);
                    target->SendHeartBeat(true);

                    if (GetId() == 35772) //Insanity MC
                    {
                        TauntNPC->CastSpell(target, 35773, true); //Haste
                        TauntNPC->CastSpell(target, 35774, true); //Speed

                        TauntNPC->CastSpell(target, 35782, true); //MC
                    }
                    else //Normal PvP Taunt
                    {
                        TauntNPC->CastSpell(target, 35773, true); //Haste
                        TauntNPC->CastSpell(target, 35774, true); //Speed

                        target->ToPlayer()->SetControlledBy(TauntNPC);

                        //Grey buttons
                        WorldPacket data(SMSG_CLIENT_CONTROL_UPDATE, 10);
                        data << target->ToPlayer()->GetPackGUID();
                        data << uint8(false);
                        target->ToPlayer()->SendDirectMessage(&data);
                    }
                }
            }
        }
        else
        {
            if (!target->HasAuraType(SPELL_AURA_MOD_TAUNT))
            {
                target->RemoveAurasDueToSpell(35773);
                target->RemoveAurasDueToSpell(35774);
                target->AttackStop();
                target->CastStop();
                target->ToPlayer()->UpdatePvPContested(false);

                if (!target->GetCharmerGuid() && !target->HasAuraType(SPELL_AURA_MOD_CHARM) && !target->HasAuraType(SPELL_AURA_MOD_POSSESS) && !target->HasAuraType(SPELL_AURA_AOE_CHARM))
                {
                    // Remove CC
                    target->StopMoving();
                    target->GetMotionMaster()->Clear(false);
                    target->GetMotionMaster()->MoveIdle();

                    target->ToPlayer()->RemoveAI();

                    target->UpdateControl();
                    /*target->ToPlayer()->SetClientControl(target, true);*/

                    /*target->UpdateVisibilityAndView();*/
                }
                else
                {
                    target->UpdateControl();
                }
            }
        }
    }
}

/*********************************************************/
/***                  MODIFY SPEED                     ***/
/*********************************************************/
void Aura::HandleAuraModIncreaseSpeed(bool apply, bool Real)
{
    // all applied/removed only at real aura add/remove
    if (!Real)
        return;

    if (GetSpellProto()->Id == 34984)
        if (Unit* target = GetTarget())
            if (target->IsMounted())
                return;

    if (Unit* caster = GetCaster())
        if (Player* modOwner = caster->GetSpellModOwner())
            modOwner->ApplySpellMod(GetSpellProto()->Id, SPELLMOD_SPEED, m_modifier.m_amount);

    switch (GetId())
    {
        // Warchief
        case 16609:
        {
            if (Unit* target = GetTarget())
            {
                if (apply)
                    target->AddAura(35387);
                else
                    target->RemoveAurasDueToSpell(35387);
            }
            break;
        }
        // Windsor
        case 35382:
        {
            if (Unit* target = GetTarget())
            {
                if (apply)
                    target->AddAura(35388);
                else
                    target->RemoveAurasDueToSpell(35388);
            }
            break;
        }
        default:
            break;
    }

    GetTarget()->UpdateSpeed(MOVE_RUN, false);
}

void Aura::HandleAuraModIncreaseMountedSpeed(bool /*apply*/, bool Real)
{
    // all applied/removed only at real aura add/remove
    if (!Real)
        return;
    if (Player* player = GetTarget()->ToPlayer())
    {
        bool mountAura = GetSpellProto()->EffectApplyAuraName[0] == SPELL_AURA_MOUNTED;
        uint32 skillValue = player->GetSkillValue(762);

        if (mountAura)
        {
            uint32 itemEntry = GetCastItemGuid() && player->GetItemByGuid(GetCastItemGuid()) ? player->GetItemByGuid(GetCastItemGuid())->GetEntry() : 0;
            // Only shop mounts should have dynamic speed
            switch (itemEntry) 
            {
            case 13317:
            case 23720:
            case 18768:
            case 8630:
            case 12326:
            case 70024:
            case 70025:
            case 70026:
                switch (skillValue)
                {
                case 0: m_modifier.m_amount = 10; break;
                case 75: m_modifier.m_amount = 60; break;
                case 150: m_modifier.m_amount = 100; break;
                default:
                    player->Unmount();
                    player->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);
                    return;
                    break;
                }

            default: break;
            }

        }
    }

    GetTarget()->UpdateSpeed(MOVE_RUN, false);
}

void Aura::HandleAuraModIncreaseSwimSpeed(bool /*apply*/, bool Real)
{
    // all applied/removed only at real aura add/remove
    if (!Real)
        return;

    GetTarget()->UpdateSpeed(MOVE_SWIM, false);
}

void Aura::HandleAuraModDecreaseSpeed(bool apply, bool Real)
{
    // all applied/removed only at real aura add/remove
    if (!Real)
        return;


    Unit* target = GetTarget();

    if (Unit* caster = GetCaster())
    {
        if (Player* modOwner = caster->GetSpellModOwner())
            modOwner->ApplySpellMod(GetSpellProto()->Id, SPELLMOD_SPEED, m_modifier.m_amount);

        if (apply)
        {
            switch (GetId())
            {
                case 35137: //Saw Blade dmg
                {
                    caster->CastSpell(target, 35138, true);
                    /*std::list<Creature*> BossList;
                    GetCreatureListWithEntryInGrid(BossList, caster, 25100, 150.0f);

                    for (std::list<Creature*>::iterator itr = BossList.begin(); itr != BossList.end();)
                    {
                        if ((*itr))
                            (*itr)->CastSpell(target, 35138, true);
                    }*/
                    break;
                }
            }
        }
        else
        {
            switch (GetId())
            {
                // Numbing Cold reapply
                case 35981:
                case 35982:
                {
                    if (target->HasAura(35978))
                    {
                        if (!(target->HasAura(35982) || target->HasAura(35984)))
                            target->AddAura(35980);
                    }

                    break;
                }
            }

        }
    }

    target->UpdateSpeed(MOVE_RUN, false);
    target->UpdateSpeed(MOVE_SWIM, false);
}

void Aura::HandleAuraModUseNormalSpeed(bool /*apply*/, bool Real)
{
    // all applied/removed only at real aura add/remove
    if (!Real)
        return;

    Unit *target = GetTarget();

    target->UpdateSpeed(MOVE_RUN, false);
    target->UpdateSpeed(MOVE_SWIM, false);
}

/*********************************************************/
/***                     IMMUNITY                      ***/
/*********************************************************/

void Aura::HandleModMechanicImmunity(bool apply, bool /*Real*/)
{
    uint32 misc  = m_modifier.m_miscvalue;
    Unit *target = GetTarget();

    if (apply && GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY)
        target->RemoveAurasAtMechanicImmunity(1 << (misc - 1), GetId());

    //Adrenaline Injector regen part
    if (GetId() == 34987)
    {
        if (apply)
            target->AddAura(34988);
        else
            target->RemoveAurasDueToSpell(34988);
    }
    //Morale
    if (GetId() == 35261)
    {
        if (!apply)
        {
            if (target->HasAura(33462)) //r1
                target->CastSpell(target, 35262, true);
            if (target->HasAura(33463)) //r2
                target->CastSpell(target, 35263, true);
            if (target->HasAura(33464)) //r3
                target->CastSpell(target, 35264, true);
            if (target->HasAura(33465)) //r4
                target->CastSpell(target, 35265, true);
            if (target->HasAura(33466)) //r5
                target->CastSpell(target, 35266, true);
        }
    }
    // Gladiator's Trinket Rare
    if (GetId() == 35440)
    {
        if (apply)
        {
            if (!target->HasAura(35441))
            {
                target->AddAura(35441);
            }
        }
    }

    target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, misc, apply);
}

void Aura::HandleModMechanicImmunityMask(bool apply, bool /*Real*/)
{
    uint32 mechanic  = m_modifier.m_miscvalue;

    if (apply && GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY)
        GetTarget()->RemoveAurasAtMechanicImmunity(mechanic, GetId());

    // check implemented in Unit::IsImmuneToSpell and Unit::IsImmuneToSpellEffect
}

//this method is called whenever we add / remove aura which gives m_target some imunity to some spell effect
void Aura::HandleAuraModEffectImmunity(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();

    // when removing flag aura, handle flag drop
    if (target->IsPlayer() && !target->HasAuraType(SPELL_AURA_MOD_POSSESS) && (GetSpellProto()->AuraInterruptFlags & AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION))
    {
        Player* player = static_cast<Player*>(target);

        if (apply)
            player->pvpInfo.isPvPFlagCarrier = true;
        else
        {
            player->pvpInfo.isPvPFlagCarrier = false;

            if (BattleGround* bg = player->GetBattleGround())
                bg->EventPlayerDroppedFlag(player);
        }
    }

    target->ApplySpellImmune(GetId(), IMMUNITY_EFFECT, m_modifier.m_miscvalue, apply);
}

void Aura::HandleAuraModStateImmunity(bool apply, bool Real)
{
    if (apply && Real && GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY)
    {
        Unit::AuraList const& auraList = GetTarget()->GetAurasByType(AuraType(m_modifier.m_miscvalue));
        for (Unit::AuraList::const_iterator itr = auraList.begin(); itr != auraList.end();)
        {
            if (auraList.front() != this)                   // skip itself aura (it already added)
            {
                GetTarget()->RemoveAurasDueToSpell(auraList.front()->GetId());
                itr = auraList.begin();
            }
            else
                ++itr;
        }
    }

    GetTarget()->ApplySpellImmune(GetId(), IMMUNITY_STATE, m_modifier.m_miscvalue, apply);
}

void Aura::HandleAuraModSchoolImmunity(bool apply, bool Real)
{
    Unit* target = GetTarget();

    switch (GetId())
    {
        case 35442:  //Unfettering Effect
        {
            if (apply)
            {
                target->CastSpell(target, 35724, true);
            }
            return; 
        }
        case 35950:
        {
            if (!apply) //Michael ghost freezing
            {
                std::list<Creature*> PredatorNPC;
                GetCreatureListWithEntryInGrid(PredatorNPC, target, 25245, 100.0f);
                for (Creature* PredatorCrea : PredatorNPC)
                    if (PredatorCrea)
                    {
                        if (CreatureAI* ai = PredatorCrea->AI())
                        {
                            if (ScriptedAI* scripted = dynamic_cast<ScriptedAI*>(ai))
                                scripted->DoResetThreat();
                        }
                        PredatorCrea->AddAura(35953);
                    }
            }
            break;
        }
        default:
            break;
    }

    target->ApplySpellImmune(GetId(), IMMUNITY_SCHOOL, m_modifier.m_miscvalue, apply);

    // remove all flag auras (they are positive, but they must be removed when you are immune)
    if (apply && GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY
              && GetSpellProto()->AttributesEx2 & SPELL_ATTR_EX2_DAMAGE_REDUCED_SHIELD
              && target->IsPlayer() && !target->isCharmed())
        target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

    // TODO: optimalize this cycle - use RemoveAurasWithInterruptFlags call or something else
    if (Real && apply
            && GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY
            && IsPositiveSpell(GetId()))                        //Only positive immunity removes auras
    {
        uint32 school_mask = m_modifier.m_miscvalue;
        Unit::SpellAuraHolderMap& Auras = target->GetSpellAuraHolderMap();
        for (Unit::SpellAuraHolderMap::iterator iter = Auras.begin(), next; iter != Auras.end(); iter = next)
        {
            next = iter;
            ++next;
            SpellEntry const *spell = iter->second->GetSpellProto();
            if ((spell->GetSpellSchoolMask() & school_mask) //Check for school mask
                    && !(spell->Attributes & SPELL_ATTR_UNAFFECTED_BY_INVULNERABILITY)    //Spells unaffected by invulnerability
                    && !iter->second->IsPositive()          //Don't remove positive spells
                    && spell->Id != GetId())                //Don't remove self
            {
                target->RemoveAurasDueToSpell(spell->Id);
                if (Auras.empty())
                    break;
                else
                    next = Auras.begin();
            }
        }
    }
    if (Real && GetSpellProto()->Mechanic == MECHANIC_BANISH)
    {
        if (apply)
            target->addUnitState(UNIT_STAT_ISOLATED);
        else
            target->clearUnitState(UNIT_STAT_ISOLATED);
    }
    // Water Shield
    if (GetId() == 35035)
    {
        if (apply)
            target->AddAura(35068);
        else
            target->RemoveAurasDueToSpell(35068);
    }
}

void Aura::HandleAuraModDmgImmunity(bool apply, bool /*Real*/)
{
    GetTarget()->ApplySpellImmune(GetId(), IMMUNITY_DAMAGE, m_modifier.m_miscvalue, apply);
}

void Aura::HandleAuraModDispelImmunity(bool apply, bool Real)
{
    Unit* target = GetTarget();

    if (!target)
        return;

    // all applied/removed only at real aura add/remove
    if (!Real)
        return;

    if (GetId() == 20594) // Forme de pierre
    {
        target->ApplySpellDispelImmunity(GetSpellProto(), DISPEL_DISEASE, apply);
        target->ApplySpellDispelImmunity(GetSpellProto(), DISPEL_POISON, apply);
        return;
    }
    target->ApplySpellDispelImmunity(GetSpellProto(), DispelType(m_modifier.m_miscvalue), apply);
}

void Aura::HandleAuraProcTriggerSpell(bool apply, bool Real)
{
    if (!Real)
        return;

    switch (GetId())
    {
        // some spell have charges by functionality not have its in spell data
        case 28200:                                         // Ascendance (Talisman of Ascendance trinket)
            if (apply)
                GetHolder()->SetAuraCharges(6);
            break;
        default:
            break;
    }
}

void Aura::HandleAuraModStalked(bool apply, bool /*Real*/)
{
    // used by spells: Hunter's Mark, Mind Vision, Syndicate Tracker (MURP) DND
    if (apply)
    {
        GetTarget()->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TRACK_UNIT);
        GetTarget()->ForceValuesUpdateAtIndex(UNIT_DYNAMIC_FLAGS);
    }
    else
        GetTarget()->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TRACK_UNIT);
}

/*********************************************************/
/***                   PERIODIC                        ***/
/*********************************************************/

void Aura::HandlePeriodicTriggerSpell(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;

    Unit *target = GetTarget();

    if (apply)
    {
        switch (GetId())
        {
            case 36166:
            {
                if (target->HasAura(36167))
                    target->RemoveAurasDueToSpell(36166);

                break;
            }
            case 24743:                             // Cannon Prep
            case 24832:                             // Cannon Prep
            {
                if (GameObject* pGo = ToGameObject(GetRealCaster()))
                {
                    pGo->m_Events.AddLambdaEventAtOffset([pGo]()
                        {
                            pGo->SendGameObjectCustomAnim();
                        }, 3600);
                }
                break;
            }
        }
    }
    else
    {
        switch (GetId())
        {
            case 35881:
                if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
                {
                    if (Unit* pCaster = GetCaster())
                    {
                        int32 BaseDamage = urand(6400, 8600);
                        pCaster->CastCustomSpell(target, 35882, &BaseDamage, nullptr, nullptr, true, nullptr, this);
                    }
                }
                break;
            case 23620:                                     // Burning Adrenaline
                if (m_removeMode == AURA_REMOVE_BY_DEATH)
                    target->CastSpell(target, 23478, true);
                return;
                /*
                this is not needed. Might have been in the past, but if functions correct without this hack now.
            case 29213:                                     // Curse of the Plaguebringer
                if (m_removeMode != AURA_REMOVE_BY_DISPEL)
                    // Cast Wrath of the Plaguebringer if not dispelled
                    target->CastSpell(target, 29214, true, nullptr, this);
                return;
                */
            default:
                break;
        }
    }
    
    switch (GetId())
    {
        case 36026: // Wall of Flesh visual
        {
            if (apply)
                target->AddAura(36214);
            else
                target->RemoveAurasDueToSpell(36214);

            break;
        }
        case 35978: // Numbing Cold apply
        {
            if (apply)
            {
                target->AddAura(35979);
                target->AddAura(35980);
            }
            else
            {
                target->RemoveAurasDueToSpell(35979);
                target->RemoveAurasDueToSpell(35980);
                target->RemoveAurasDueToSpell(35981);
                target->RemoveAurasDueToSpell(35982);
                target->RemoveAurasDueToSpell(35984);
                target->RemoveAurasDueToSpell(35986);
            }

            break;
        }
        case 35754: //Holy Link on end
        {
            if (!apply)
            {
                if (Unit* caster = GetCaster())
                    caster->RemoveAurasDueToSpell(35760);

            }

            break;
        }
        case 6474:  //Earthbind Totem
        {
            if (apply)
            {
                if (Unit* caster = GetCaster())
                    caster->CastSpell(target, 3600, true);
            }
            else
            {
                target->RemoveAurasDueToSpell(3600);
            }
            break;
        }
        default:
        break;
    }
}

void Aura::HandlePeriodicTriggerSpellWithValue(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

void Aura::HandlePeriodicEnergize(bool apply, bool /*Real*/)
{
	m_isPeriodic = apply;

	Unit *target = GetTarget();

	if (GetId() == 5229) //vanilla+ druid enrage fear immunity
	{
		if (apply)
		{
			Unit* caster = GetCaster();
			if (!caster)
				return;

			caster->CastSpell(target, 34145, true);
		}
		else
			target->RemoveAurasDueToSpell(34145);
		return;
	}
}



void Aura::HandleAuraPowerBurn(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

void Aura::HandlePeriodicHeal(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;

    Unit *target = GetTarget();

    // For prevent double apply bonuses
    bool loading = (target->GetTypeId() == TYPEID_PLAYER && ((Player*)target)->GetSession()->PlayerLoading());

    // Custom damage calculation after
    if (apply)
    {
        if (loading)
            return;

        Unit *caster = GetCaster();
        if (!caster)
            return;
     
        // World of Warcraft Client Patch 1.11.0 (2006-06-20)
        // - Periodic Healing: Spells which do periodic healing will now have 
        //   their strength determined at the moment they are cast.Changing the
        //   amount of bonus healing you have during the duration of the periodic
        //   spell will have no impact on how much it heals for.
#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_10_2
        m_modifier.m_amount = caster->SpellHealingBonusDone(target, GetSpellProto(), m_modifier.m_amount, DOT, GetStackAmount());
#endif
    }
}

uint32 Aura::CalculateDotDamage() const
{
    Unit* target = GetTarget();
    Unit* caster = GetCaster();
    SpellEntry const* spellProto = GetSpellProto();

    uint32 damage = m_modifier.m_amount;

    switch (spellProto->SpellFamilyName)
    {
        case SPELLFAMILY_WARLOCK:
        {
            //Nemesis 4/4 DoT
            if (spellProto->IsFitToFamilyMask<CF_WARLOCK_RAIN_OF_FIRE, CF_WARLOCK_IMMOLATE>())
            {
                if (caster->HasAura(34570))
                {
                    uint32 wlHP = caster->GetHealth() / caster->GetMaxHealth() * 100;
                    uint32 tarHP = target->GetHealth() / target->GetMaxHealth() * 100;
                    if (wlHP > tarHP)
                    {
                        damage += 0.05f * caster->SpellDamageBonusDone(target, spellProto, damage, DOT, GetStackAmount());
                        damage += 0.05f * target->SpellDamageBonusTaken(caster, spellProto, damage, DOT, GetStackAmount());
                        damage *= 1.05;
                    }
                }
            }
        }
        case SPELLFAMILY_DRUID:
        {
            // Rip
            if (spellProto->IsFitToFamilyMask<CF_DRUID_RIP_BITE>())
            {
                // $AP * min(0.06*$cp, 0.24)/6 [Yes, there is no difference, whether 4 or 5 CPs are being used]
                if (caster->GetTypeId() == TYPEID_PLAYER)
                {
                    uint8 cp = ((Player*)caster)->GetComboPoints();

                    if (cp > 4) cp = 4;
                    damage += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * cp / 100);
                }
            }
            break;
        }
        case SPELLFAMILY_WARRIOR:
        {
            break;
        }
        case SPELLFAMILY_ROGUE:
        {
#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_11_2
            // World of Warcraft Client Patch 1.12.0 (2006-08-22)
            // - Rupture: Rupture now increases in potency with greater attack power.
            if (spellProto->IsFitToFamilyMask<CF_ROGUE_RUPTURE>())
            {
                // Dmg/tick = $AP*min(0.01*$cp, 0.03) [Like Rip: only the first three CP increase the contribution from AP]
                if (caster->GetTypeId() == TYPEID_PLAYER)
                {
                    uint8 cp = ((Player*)caster)->GetComboPoints();
                    if (cp > 3) cp = 3;
                    damage += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * cp / 100);
                }
            }
#elif SUPPORTED_CLIENT_BUILD <= CLIENT_BUILD_1_11_2
            // World of Warcraft Client Patch 1.12.0 (2006-08-22)
            // - Garrote: The damage from this ability has been increased. In
            //   addition, Garrote now increases in potency with greater attack power.
            if (spellProto->IsFitToFamilyMask<CF_ROGUE_GARROTE>())
                return damage;
#endif
            break;
        }
        default:
            break;
    }

    if (m_modifier.m_auraname == SPELL_AURA_PERIODIC_DAMAGE)
    {
        // SpellDamageBonusDone for magic spells
        if (spellProto->DmgClass == SPELL_DAMAGE_CLASS_NONE || spellProto->DmgClass == SPELL_DAMAGE_CLASS_MAGIC)
            damage = caster->SpellDamageBonusDone(target, GetSpellProto(), damage, DOT, GetStackAmount());
        // MeleeDamagebonusDone for weapon based spells
        else
        {
            WeaponAttackType attackType = GetSpellProto()->GetWeaponAttackType();
            damage = caster->MeleeDamageBonusDone(target, damage, attackType, GetSpellProto(), DOT, GetStackAmount());
        }
    }

    //Orc Racial Command
    if (caster->HasAura(20575) || caster->HasAura(20576) || caster->HasAura(21563))
    {
        if (caster->FindAllyUnit(10.0f))
        {
            damage *= 1.01;
        }
    }
    //Prestor's Talisman of Connivery
    if (caster->HasAura(35282))
    {
        if (!target->HasInArc(M_PI_F, caster))
            damage *= 1.03;
    }
    //Rogue's Connivery talent
    if (caster->HasAura(35398) || caster->HasAura(35399) || caster->HasAura(35400) || caster->HasAura(35401) || caster->HasAura(35402))
    {
        if (!target->HasInArc(M_PI_F, caster))
        {
            if (caster->HasAura(35398))
                damage *= 1.02;
            if (caster->HasAura(35399))
                damage *= 1.04;
            if (caster->HasAura(35400))
                damage *= 1.06;
            if (caster->HasAura(35401))
                damage *= 1.08;
            if (caster->HasAura(35402))
                damage *= 1.1;
        }
    }
    //Rogue's Coup De Grace talent
    if (caster->HasAura(14113) || caster->HasAura(14114) || caster->HasAura(14115) || caster->HasAura(14116) || caster->HasAura(14117))
    {
        if (target->GetHealthPercent() < 20.0f)
        {
            if (caster->HasAura(14113))
                damage *= 1.05;
            if (caster->HasAura(14114))
                damage *= 1.1;
            if (caster->HasAura(14115))
                damage *= 1.15;
            if (caster->HasAura(14116))
                damage *= 1.2;
            if (caster->HasAura(14117))
                damage *= 1.25;
        }
    }
    // Prophecy: Execution
    if (caster->HasAura(36193))
    {
        if (target->GetHealthPercent() < 50.0f)
            damage *= 5;
    }
    // Prophecy: At the Edge
    if (caster->HasAura(36204))
    {
        if (caster->GetHealthPercent() < 50.0f)
            damage *= 2;
    }
    // Executioner's Hood
    if (caster->HasAura(36249))
    {
        if (target->GetHealthPercent() < 20.0f)
            damage *= 1.05;
    }
    // Herod's Shoulder
    if (target->HasAura(36261))
    {
        if (target->HasInArc(M_PI_F / 2.0f, caster, -M_PI_F / 2.0f))
            damage *= 0.95;
    }
    if (target->HasAura(36262))
    {
        if (target->HasInArc(M_PI_F / 2.0f, caster, -M_PI_F / 2.0f))
            damage *= 0.25;
    }

    //Resilience DOT
    if (target->GetTypeId() == TYPEID_PLAYER)
        if (caster->GetTypeId() == TYPEID_PLAYER)
            damage *= 0.9;

    return damage;
}

void Aura::HandlePeriodicDamage(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
        return;

    m_isPeriodic = apply;

    // World of Warcraft Client Patch 1.10.0 (2006-03-28)
    // - Damage over time spells are no longer affected by changes in
    //   equipment after the cast.
#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_9_4
    Unit *target = GetTarget();
    SpellEntry const* spellProto = GetSpellProto();

    // For prevent double apply bonuses
    bool loading = (target->GetTypeId() == TYPEID_PLAYER && ((Player*)target)->GetSession()->PlayerLoading());

    // Custom damage calculation after
    if (apply)
    {
        if (loading)
            return;

        if (!GetCaster())
            return;


        //Garrote Silence and -cast time
        if (GetSpellProto()->IsFitToFamily<SPELLFAMILY_ROGUE, CF_ROGUE_GARROTE>())
        {
            GetCaster()->CastSpell(GetTarget(), 34132, true);
            GetCaster()->CastSpell(GetTarget(), 35763, true);
        }

        m_modifier.m_amount = CalculateDotDamage();
    }
#endif
}

void Aura::HandlePeriodicDamagePCT(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
    
    //Arena Ignition debuff
    if (GetId() == 35432)
    {
        Unit* target = GetTarget();
        if (apply)
            if (target->HasAuraType(SPELL_AURA_MOD_POSSESS) || target->HasAuraType(SPELL_AURA_MOD_CHARM))
                target->RemoveCharmAuras();
    }
}

void Aura::HandlePeriodicLeech(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;

    // For prevent double apply bonuses
    bool loading = (GetTarget()->GetTypeId() == TYPEID_PLAYER && ((Player*)GetTarget())->GetSession()->PlayerLoading());

    // Custom damage calculation after
    if (apply)
    {
        if (loading)
            return;

        Unit *caster = GetCaster();
        if (!caster)
            return;

        m_modifier.m_amount = caster->SpellDamageBonusDone(GetTarget(), GetSpellProto(), m_modifier.m_amount, DOT, GetStackAmount());
    }
}

void Aura::HandlePeriodicManaLeech(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

void Aura::HandlePeriodicHealthFunnel(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;

    // For prevent double apply bonuses
    bool loading = (GetTarget()->GetTypeId() == TYPEID_PLAYER && ((Player*)GetTarget())->GetSession()->PlayerLoading());

    // Custom damage calculation after
    if (apply)
    {
        if (loading)
            return;

        Unit *caster = GetCaster();
        if (!caster)
            return;

        m_modifier.m_amount = caster->SpellDamageBonusDone(GetTarget(), GetSpellProto(), m_modifier.m_amount, DOT, GetStackAmount());
    }
}

/*********************************************************/
/***                  MODIFY STATS                     ***/
/*********************************************************/

/********************************/
/***        RESISTANCE        ***/
/********************************/

void Aura::HandleAuraModResistanceExclusive(bool apply, bool /*Real*/)
{
    if (!m_modifier.m_amount)
        return;

    Unit* target = GetTarget();

    for (uint32 i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; ++i)
    {
        if (m_modifier.m_miscvalue & (1 << i))
        {
            // Apply the value exclusively for each school
            const int32 amount = m_modifier.m_amount;
            int32 bonusMax = 0;
            int32 malusMax = 0;

            for (Aura* aura : target->GetAurasByType(m_modifier.m_auraname))
            {
                if (aura == this || !(aura->GetMiscValue() & (1 << i)))
                    continue;

                const int32 mod = aura->GetModifier()->m_amount;

                if (mod > bonusMax)
                    bonusMax = mod;
                else if (mod < malusMax)
                    malusMax = mod;
            }

            if (amount > bonusMax)
            {
                target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + i), TOTAL_VALUE, float(apply ? bonusMax : amount), false);
                target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + i), TOTAL_VALUE, float(apply ? amount : bonusMax), true);
                // UI bonus info:
                if (target->GetTypeId() == TYPEID_PLAYER)
                {
                    static_cast<Player*>(target)->ApplyResistanceBuffModsMod(SpellSchools(i), true, float(apply ? bonusMax : amount), false);
                    static_cast<Player*>(target)->ApplyResistanceBuffModsMod(SpellSchools(i), true, float(apply ? amount : bonusMax), true);
                }
            }
            else if (amount < malusMax)
            {
                target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + i), TOTAL_VALUE, float(apply ? malusMax : amount), false);
                target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + i), TOTAL_VALUE, float(apply ? amount : malusMax), true);
                // UI malus info:
                if (target->GetTypeId() == TYPEID_PLAYER)
                {
                    static_cast<Player*>(target)->ApplyResistanceBuffModsMod(SpellSchools(i), false, float(apply ? malusMax : amount), false);
                    static_cast<Player*>(target)->ApplyResistanceBuffModsMod(SpellSchools(i), false, float(apply ? amount : malusMax), true);
                }
            }
        }
    }
}

void Aura::HandleAuraModResistance(bool apply, bool /*Real*/)
{
    if (!m_modifier.m_amount)
        return;

    Unit* target = GetTarget();

    for (uint32 i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; ++i)
    {
        if (m_modifier.m_miscvalue & (1 << i))
        {
            target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + i), TOTAL_VALUE, float(m_modifier.m_amount), apply);
            // UI bonus/malus info:
            if (target->GetTypeId() == TYPEID_PLAYER)
                static_cast<Player*>(target)->ApplyResistanceBuffModsMod(SpellSchools(i), (m_modifier.m_amount > 0), float(m_modifier.m_amount), apply);
        }
}

    // Faerie Fire (druid versions)
    /*SpellEntry const* spellProto = GetSpellProto();
    if (spellProto->SpellIconID == 109 && spellProto->IsFitToFamily<SPELLFAMILY_DRUID, CF_DRUID_FAERIE_FIRE>())
    {
        target->ApplySpellDispelImmunity(spellProto, DISPEL_STEALTH, apply);
        target->ApplySpellDispelImmunity(spellProto, DISPEL_INVISIBILITY, apply);
    }*/
}


void Aura::HandleAurasVisible(bool apply, bool /*Real*/)
{
    GetTarget()->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_AURAS_VISIBLE, apply);
}

void Aura::HandleModResistancePercent(bool apply, bool /*Real*/)
{
    if (!m_modifier.m_amount)
        return;

    Unit* target = GetTarget();

    for (uint32 i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; ++i)
    {
        if (m_modifier.m_miscvalue & (1 << i))
        {
            target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + i), TOTAL_PCT, float(m_modifier.m_amount), apply);
            // UI bonus/malus info:
            if (target->GetTypeId() == TYPEID_PLAYER)
            {
                static_cast<Player*>(target)->ApplyResistanceBuffModsPercentMod(SpellSchools(i), true, float(m_modifier.m_amount), apply);
                static_cast<Player*>(target)->ApplyResistanceBuffModsPercentMod(SpellSchools(i), false, float(m_modifier.m_amount), apply);
            }
        }
    }
}

void Aura::HandleModBaseResistance(bool apply, bool /*Real*/)
{
    if (!m_modifier.m_amount)
        return;

    Unit* target = GetTarget();

    for (uint32 i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; ++i)
    {
        if (m_modifier.m_miscvalue & (1 << i))
            target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + i), BASE_VALUE, float(m_modifier.m_amount), apply);
    }
}

void Aura::HandleAuraModBaseResistancePercent(bool apply, bool /*Real*/)
{
    if (!m_modifier.m_amount)
        return;

    Unit* target = GetTarget();

    for (uint32 i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; ++i)
    {
        if (m_modifier.m_miscvalue & (1 << i))
            target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + i), BASE_PCT, float(m_modifier.m_amount), apply);
    }
}

/********************************/
/***           STAT           ***/
/********************************/

void Aura::HandleAuraModStat(bool apply, bool /*Real*/)
{
    if (m_modifier.m_miscvalue < -2 || m_modifier.m_miscvalue > 4)
    {
        sLog.outError("WARNING: Spell %u effect %u have unsupported misc value (%i) for SPELL_AURA_MOD_STAT ", GetId(), GetEffIndex(), m_modifier.m_miscvalue);
        return;
    }

	Unit* target = GetTarget();

	// Improved Scorpid Sting
	if (GetSpellProto()->IsFitToFamilyMask<CF_HUNTER_SCORPID_STING>() && (m_modifier.m_miscvalue == STAT_STRENGTH))
	{
		if (apply)
		{
			int32 staminaToRemove = 0;
			Unit::AuraList const& auraClassScripts = GetCaster()->GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
            //if (GetCaster())  Spell Attributes Ex4 REAL_TIME_DURATION (4)
			for (const auto& aura : auraClassScripts)
			{
				bool exitLoop = false;
				switch (aura->GetModifier()->m_miscvalue)
				{
				case 2388: staminaToRemove = m_modifier.m_amount * 10 / 100; exitLoop = true; break; // Rank 1
				case 2389: staminaToRemove = m_modifier.m_amount * 30 / 100; exitLoop = true; break; // Rank 2
				case 2390: staminaToRemove = m_modifier.m_amount * 50 / 100; exitLoop = true; break; // Rank 3
				}
				if (exitLoop)
					break;
			}
			if (staminaToRemove)
				GetCaster()->CastCustomSpell(target, 19486, &staminaToRemove, &staminaToRemove, nullptr, true);
		}
		else
			target->RemoveAurasDueToSpell(19486, this->GetHolder());
	}

    for (int32 i = STAT_STRENGTH; i < MAX_STATS; i++)
    {
        // -1 or -2 is all stats ( misc < -2 checked in function beginning )
        if (m_modifier.m_miscvalue < 0 || m_modifier.m_miscvalue == i)
        {
            //m_target->ApplyStatMod(Stats(i), m_modifier.m_amount,apply);
            target->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_VALUE, float(m_modifier.m_amount), apply);
            if (target->GetTypeId() == TYPEID_PLAYER)
                ((Player*)target)->ApplyStatBuffMod(Stats(i), float(m_modifier.m_amount), apply);
        }
    }
}

void Aura::HandleModPercentStat(bool apply, bool /*Real*/)
{
    if (m_modifier.m_miscvalue < -1 || m_modifier.m_miscvalue > 4)
    {
        sLog.outError("WARNING: Misc Value for SPELL_AURA_MOD_PERCENT_STAT not valid");
        return;
    }

    // only players have base stats
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    for (int32 i = STAT_STRENGTH; i < MAX_STATS; ++i)
    {
        if (m_modifier.m_miscvalue == i || m_modifier.m_miscvalue == -1)
            GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), BASE_PCT, float(m_modifier.m_amount), apply);
    }
}

void Aura::HandleModSpellDamagePercentFromStat(bool /*apply*/, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    // Magic damage modifiers implemented in Unit::SpellDamageBonusDone
    // This information for client side use only
    // Recalculate bonus
    ((Player*)GetTarget())->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleModSpellHealingPercentFromStat(bool /*apply*/, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    // Recalculate bonus
    ((Player*)GetTarget())->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleModHealingDone(bool /*apply*/, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;
    // implemented in Unit::SpellHealingBonusDone
    // this information is for client side only
    ((Player*)GetTarget())->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleModTotalPercentStat(bool apply, bool /*Real*/)
{
    if (m_modifier.m_miscvalue < -1 || m_modifier.m_miscvalue > 4)
    {
        sLog.outError("WARNING: Misc Value for SPELL_AURA_MOD_PERCENT_STAT not valid");
        return;
    }

    Unit *target = GetTarget();

    //save current and max HP before applying aura
    uint32 curHPValue = target->GetHealth();
    uint32 maxHPValue = target->GetMaxHealth();

    for (int32 i = STAT_STRENGTH; i < MAX_STATS; i++)
    {
        if (m_modifier.m_miscvalue == i || m_modifier.m_miscvalue == -1)
        {
            target->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_PCT, float(m_modifier.m_amount), apply);
            if (target->GetTypeId() == TYPEID_PLAYER)
                ((Player*)target)->ApplyStatPercentBuffMod(Stats(i), float(m_modifier.m_amount), apply);
        }
    }

    //recalculate current HP/MP after applying aura modifications (only for spells with 0x10 flag)
    if ((m_modifier.m_miscvalue == STAT_STAMINA) && (maxHPValue > 0) && (GetSpellProto()->Attributes & 0x10) && target->isAlive())
    {
        // newHP = (curHP / maxHP) * newMaxHP = (newMaxHP * curHP) / maxHP -> which is better because no int -> double -> int conversion is needed
        uint32 newHPValue = (target->GetMaxHealth() * curHPValue) / maxHPValue;
        target->SetHealth(newHPValue);
    }
}

void Aura::HandleAuraModResistenceOfStatPercent(bool /*apply*/, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    if (m_modifier.m_miscvalue != SPELL_SCHOOL_MASK_NORMAL)
    {
        // support required adding replace UpdateArmor by loop by UpdateResistence at intellect update
        // and include in UpdateResistence same code as in UpdateArmor for aura mod apply.
        sLog.outError("Aura SPELL_AURA_MOD_RESISTANCE_OF_STAT_PERCENT(182) need adding support for non-armor resistances!");
        return;
    }

    // Recalculate Armor
    GetTarget()->UpdateArmor();
}

/********************************/
/***      HEAL & ENERGIZE     ***/
/********************************/
void Aura::HandleAuraModTotalHealthPercentRegen(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

void Aura::HandleAuraModTotalManaPercentRegen(bool apply, bool /*Real*/)
{
    if (m_modifier.periodictime == 0)
        m_modifier.periodictime = 1000;

    m_periodicTimer = m_modifier.periodictime;
    m_isPeriodic = apply;
}

void Aura::HandleModRegen(bool apply, bool /*Real*/)        // eating
{
    if (m_modifier.periodictime == 0)
        m_modifier.periodictime = 5000;

    m_periodicTimer = 5000;
    m_isPeriodic = apply;
}

void Aura::HandleModPowerRegen(bool apply, bool Real)       // drinking
{
    if (!Real)
        return;

    Powers pt = GetTarget()->getPowerType();
    if (m_modifier.periodictime == 0)
    {
        // Anger Management (only spell use this aura for rage)
        if (pt == POWER_RAGE)
            m_modifier.periodictime = 3000;
        else
            m_modifier.periodictime = 2000;
    }

    m_periodicTimer = 5000;

    if (m_modifier.m_miscvalue == POWER_MANA)
        (GetTarget())->UpdateManaRegen();

    m_isPeriodic = apply;
}

void Aura::HandleModPowerRegenPCT(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
        return;

    // Update manaregen value
    if (m_modifier.m_miscvalue == POWER_MANA)
        (GetTarget())->UpdateManaRegen();
}

void Aura::HandleAuraModIncreaseHealth(bool apply, bool Real)
{
    Unit *target = GetTarget();

    // Special case with temporary increase max/current health
    switch (GetId())
    {
        case 12976:                                         //vanilla+ Warrior Last Stand triggered spell
        {
            if (Real)
			{
				target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_PCT, 20.0f, apply);
				if (apply)
				{
					int32 healAmount = target->GetMaxHealth() * 0.20f;
					target->CastCustomSpell(target, 33765, &healAmount, nullptr, nullptr, true, nullptr, this);
                }
            }
            return;
        }
		case 34114:                                         //vanilla+ Druid Untamed Heart triggered spell
		{
			if (Real)
			{
				target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_PCT, 20.0f, apply);
				if (apply)
				{
					int32 healAmount = target->GetMaxHealth() * 0.20f;
					target->CastCustomSpell(target, 33765, &healAmount, nullptr, nullptr, true, nullptr, this);
				}
			}
			return;
		}
        // Druid bear form
        case 1178:
        case 9635:
        {
            if (Real)
            {
                float fHealthPercent = float(target->GetHealth()) / target->GetMaxHealth();
                int32 newMaxHealth = target->GetMaxHealth();
                if (apply)
                    newMaxHealth += m_modifier.m_amount;
                else
                    newMaxHealth -= m_modifier.m_amount;

                uint32 newHealth = ceil(newMaxHealth * fHealthPercent);
                target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);
                target->SetHealth(newHealth);
            }
            return;
        }
        // Trinket BWL (Don de vie)
        case 23782:
        {
            if (Real)
            {
                target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_PCT, 15.0f, apply);
                if (apply)
                {
                    int32 healAmount = target->GetMaxHealth() * 0.15f;
                    target->CastCustomSpell(target, 23783, &healAmount, nullptr, nullptr, true, nullptr, this);
                }
            }
            return;
        }
    }

    // generic case
    target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModIncreaseEnergy(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();
    Powers powerType = Powers(m_modifier.m_miscvalue);

    UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + powerType);

    target->HandleStatModifier(unitMod, TOTAL_VALUE, float(m_modifier.m_amount), apply);
    target->ModifyPower(powerType, apply ? m_modifier.m_amount : -m_modifier.m_amount);
}

void Aura::HandleAuraModIncreaseEnergyPercent(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();
    Powers powerType = Powers(m_modifier.m_miscvalue);
    float powerPercent = target->GetPowerPercent(powerType);

    UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + powerType);
    target->HandleStatModifier(unitMod, TOTAL_PCT, float(m_modifier.m_amount), apply);

    if (target->isAlive())
        target->SetPower(powerType, target->GetMaxPower(powerType) * powerPercent / 100.0f);
}

void Aura::HandleAuraModIncreaseHealthPercent(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();
    float healthPercent = target->GetHealthPercent();
    target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_PCT, float(m_modifier.m_amount), apply);

    if (target->isAlive())
        target->SetHealthPercent(healthPercent);

    if (target->GetMaxHealth() == 1)
        target->DoKillUnit(target);
}

/********************************/
/***          FIGHT           ***/
/********************************/

void Aura::HandleAuraModParryPercent(bool /*apply*/, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)GetTarget())->UpdateParryPercentage();
}

void Aura::HandleAuraModDodgePercent(bool /*apply*/, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)GetTarget())->UpdateDodgePercentage();
    //sLog.outError("BONUS DODGE CHANCE: + %f", float(m_modifier.m_amount));
}

void Aura::HandleAuraModBlockPercent(bool /*apply*/, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)GetTarget())->UpdateBlockPercentage();
    //sLog.outError("BONUS BLOCK CHANCE: + %f", float(m_modifier.m_amount));
}

void Aura::HandleAuraModRegenInterrupt(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
        return;

    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)GetTarget())->UpdateManaRegen();
}

void Aura::HandleAuraModCritPercent(bool apply, bool Real)
{
    Unit *target = GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    // apply item specific bonuses for already equipped weapon
    if (Real)
    {
        for (int i = 0; i < MAX_ATTACK; ++i)
            if (Item* pItem = ((Player*)target)->GetWeaponForAttack(WeaponAttackType(i), true, false))
                ((Player*)target)->_ApplyWeaponDependentAuraCritMod(pItem, WeaponAttackType(i), this, apply);
    }

    // mods must be applied base at equipped weapon class and subclass comparison
    // with spell->EquippedItemClass and  EquippedItemSubClassMask and EquippedItemInventoryTypeMask
    // m_modifier.m_miscvalue comparison with item generated damage types
    if (GetId() == 13163)
    {
        ((Player*)target)->HandleBaseModValue(CRIT_PERCENTAGE, FLAT_MOD, float(m_modifier.m_amount), apply);
        ((Player*)target)->HandleBaseModValue(OFFHAND_CRIT_PERCENTAGE, FLAT_MOD, float(m_modifier.m_amount), apply);
        return;
    }

    if (GetSpellProto()->EquippedItemClass == -1)
    {
        ((Player*)target)->HandleBaseModValue(CRIT_PERCENTAGE,         FLAT_MOD, float(m_modifier.m_amount), apply);
        ((Player*)target)->HandleBaseModValue(OFFHAND_CRIT_PERCENTAGE, FLAT_MOD, float(m_modifier.m_amount), apply);
        ((Player*)target)->HandleBaseModValue(RANGED_CRIT_PERCENTAGE,  FLAT_MOD, float(m_modifier.m_amount), apply);
    }
    else
    {
        // done in Player::_ApplyWeaponDependentAuraMods
    }
}

void Aura::HandleModHitChance(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();


    switch (GetId())
    {
        case 36100: // Script of Destiny
        {
            if (apply)
            {
                target->CastSpell(target, 36101, true);
            }
            else
            {
                target->RemoveAurasDueToSpell(36101);
            }
            return;
        }
    }



    if (GetId() != 22780 && GetId() != 36246) // [Ranged Hit Bonus +3] as stated in name ...
        target->m_modMeleeHitChance += apply ? m_modifier.m_amount : (-m_modifier.m_amount);
    target->m_modRangedHitChance += apply ? m_modifier.m_amount : (-m_modifier.m_amount);
}

void Aura::HandleModSpellHitChance(bool apply, bool /*Real*/)
{
    GetTarget()->m_modSpellHitChance += apply ? m_modifier.m_amount : (-m_modifier.m_amount);
}

void Aura::HandleModSpellCritChance(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
        return;

    if (GetTarget()->GetTypeId() == TYPEID_PLAYER)
        ((Player*)GetTarget())->UpdateAllSpellCritChances();
    else
        GetTarget()->m_baseSpellCritChance += apply ? m_modifier.m_amount : (-m_modifier.m_amount);
}

void Aura::HandleModSpellCritChanceSchool(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
        return;

    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    for (int school = SPELL_SCHOOL_NORMAL; school < MAX_SPELL_SCHOOL; ++school)
        if (m_modifier.m_miscvalue & (1 << school))
            ((Player*)GetTarget())->UpdateSpellCritChance(school);
}

/********************************/
/***         ATTACK SPEED     ***/
/********************************/

void Aura::HandleModCastingSpeed(bool apply, bool /*Real*/)
{
    float amount = m_modifier.m_amount;

    if (Unit* caster = GetCaster())
        if (Player* modOwner = caster->GetSpellModOwner())
            modOwner->ApplySpellMod(GetSpellProto()->Id, SPELLMOD_HASTE, amount);

    GetTarget()->ApplyCastTimePercentMod(amount, apply);
}

void Aura::HandleModAttackSpeed(bool apply, bool /*Real*/)
{
    float amount = m_modifier.m_amount;

    if (Unit* caster = GetCaster())
        if (Player* modOwner = caster->GetSpellModOwner())
            modOwner->ApplySpellMod(GetSpellProto()->Id, SPELLMOD_HASTE, amount);

    Unit *target = GetTarget();
    target->ApplyAttackTimePercentMod(BASE_ATTACK, amount, apply);
    target->ApplyAttackTimePercentMod(OFF_ATTACK, amount, apply);
    target->ApplyAttackTimePercentMod(RANGED_ATTACK, amount, apply);

    // Seal of the Crusader damage reduction
    // SoC increases attack speed but reduces damage to maintain the same DPS
    if (GetSpellProto()->IsFitToFamily<SPELLFAMILY_PALADIN, CF_PALADIN_SEAL_OF_THE_CRUSADER>())
    {
        float reduction = (-100.0f * m_modifier.m_amount) / (m_modifier.m_amount + 100.0f);
        target->HandleStatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_PCT, reduction, apply);
    }
}

void Aura::HandleModMeleeSpeedPct(bool apply, bool /*Real*/)
{
    float amount = m_modifier.m_amount;

    if (Unit* caster = GetCaster())
        if (Player* modOwner = caster->GetSpellModOwner())
            modOwner->ApplySpellMod(GetSpellProto()->Id, SPELLMOD_HASTE, amount);

    Unit* target = GetTarget();
    target->ApplyAttackTimePercentMod(BASE_ATTACK, amount, apply);
    target->ApplyAttackTimePercentMod(OFF_ATTACK, amount, apply);

    switch (GetSpellProto()->Id)
    {
    case 36244: // Scarlet battle orders
    {
        if (apply)
            target->CastSpell(target, 36245, true);
        else
            target->RemoveAurasDueToSpell(36245);

        break;
    }
    case 36169: // Vengeful Heart
    {
        if (apply)
        {
            target->CastSpell(target, 36168, true); // Cursing Shout
            DoScriptText(-1900270, target); //CURSE YOU!
        }

        break;
    }
    case 34487: // v+ Dominance
    {
        if (apply)
            GetCaster()->CastSpell(target, 34370, true, nullptr, this);
        else
            target->RemoveAurasDueToSpell(34370, this->GetHolder());
        break;
    }
    case 13747: // Slow (Grethok)
    {
        if (apply)
            target->AddAura(35167);
        else
            target->RemoveAurasDueToSpell(35167);
        break;
    }
    // Lightning Reflexes
    case 19168:
    case 19180:
    case 19181:
    case 24296:
    case 24297:
    {
        int32 BuffSpellId = 0;
        switch (GetSpellProto()->Id)
        {
        case 19168: BuffSpellId = 35403; break;
        case 19180: BuffSpellId = 35404; break;
        case 19181: BuffSpellId = 35405; break;
        case 24296: BuffSpellId = 35406; break;
        case 24297: BuffSpellId = 35407; break;
        }

        if (BuffSpellId > 0)
        {
            if (apply)
                target->AddAura(BuffSpellId);
            else
                target->RemoveAurasDueToSpell(BuffSpellId);
        }
        break;
    }
    // Savage Flurry
    case 33497:
    case 33498:
    case 33499:
    case 33500:
    case 33501:
    {
        int32 BuffSpellId = 0;
        switch (GetSpellProto()->Id)
        {
        case 33497: BuffSpellId = 35409; break;
        case 33498: BuffSpellId = 35410; break;
        case 33499: BuffSpellId = 35411; break;
        case 33500: BuffSpellId = 35412; break;
        case 33501: BuffSpellId = 35413; break;
        }

        if (BuffSpellId > 0)
        {
            if (apply)
                target->AddAura(BuffSpellId);
            else
                target->RemoveAurasDueToSpell(BuffSpellId);
        }
        break;
    }
    case 35982: // Numbing Cold
    {
        if (apply)
            target->AddAura(35983);
        else
            target->RemoveAurasDueToSpell(35983);

        break;
    }
    case 35984: // Chilled to the Bone
    {
        if (apply)
            target->AddAura(35985);
        else
            target->RemoveAurasDueToSpell(35985);

        break;
    }

    default:
        break;
    }
}

void Aura::HandleAuraModRangedHaste(bool apply, bool /*Real*/)
{
    float amount = m_modifier.m_amount;

    if (Unit* caster = GetCaster())
        if (Player* modOwner = caster->GetSpellModOwner())
            modOwner->ApplySpellMod(GetSpellProto()->Id, SPELLMOD_HASTE, amount);

    GetTarget()->ApplyAttackTimePercentMod(RANGED_ATTACK, amount, apply);
}

void Aura::HandleRangedAmmoHaste(bool apply, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    // Quivers should not increase attack speed for ranged weapons which do not require any ammo.
    Item* ranged_weapon = GetTarget()->ToPlayer()->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_RANGED);
    if ((!ranged_weapon || ranged_weapon->GetProto()->AmmoType == 0) && apply)
    {
        // Revert m_applied assigned in Aura::ApplyModidier
        m_applied = !apply;
        return;
    }

    if (apply)
    {
        if (Unit* caster = GetCaster())
            if (Player* modOwner = caster->GetSpellModOwner())
                modOwner->ApplySpellMod(GetSpellProto()->Id, SPELLMOD_HASTE, m_modifier.m_amount);
    }

    GetTarget()->ApplyAttackTimePercentMod(RANGED_ATTACK, m_modifier.m_amount, apply);
}

/********************************/
/***        ATTACK POWER      ***/
/********************************/

void Aura::HandleAuraModAttackPower(bool apply, bool /*Real*/)
{
    float amount = m_modifier.m_amount;

    if (Unit* caster = GetCaster())
        if (Player* modOwner = caster->GetSpellModOwner())
            modOwner->ApplySpellMod(GetSpellProto()->Id, SPELLMOD_ATTACK_POWER, amount);

    GetTarget()->HandleStatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_VALUE, amount, apply);

#if (SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_3_1) && (SUPPORTED_CLIENT_BUILD <= CLIENT_BUILD_1_8_4)
    // Blood Fury- Add aura to decrease attack power on remove
    if (!apply && GetId() == 23234 && (m_removeMode == AURA_REMOVE_BY_CANCEL || m_removeMode == AURA_REMOVE_BY_EXPIRE))
    {
        Unit* target = GetTarget();
        // using delayed event because of an error in ExclusiveAuraUnapply
        target->m_Events.AddLambdaEventAtOffset([target]
        {
            int32 attackPower = -25 * (target->GetInt32Value(UNIT_FIELD_ATTACK_POWER)) / 100;
            if (attackPower < 0)
                target->CastCustomSpell(target, 23230, &attackPower, nullptr, nullptr, true, nullptr);
        }, 1);
    }
#endif
}

void Aura::HandleAuraModRangedAttackPower(bool apply, bool /*Real*/)
{
    if ((GetTarget()->getClassMask() & CLASSMASK_WAND_USERS) != 0)
        return;

    float amount = m_modifier.m_amount;

    if (Unit* caster = GetCaster())
        if (Player* modOwner = caster->GetSpellModOwner())
            modOwner->ApplySpellMod(GetSpellProto()->Id, SPELLMOD_ATTACK_POWER, amount);

    GetTarget()->HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE, amount, apply);
}

void Aura::HandleAuraModAttackPowerPercent(bool apply, bool /*Real*/)
{
    float amount = m_modifier.m_amount;

    if (Unit* caster = GetCaster())
        if (Player* modOwner = caster->GetSpellModOwner())
            modOwner->ApplySpellMod(GetSpellProto()->Id, SPELLMOD_ATTACK_POWER, amount);

    // UNIT_FIELD_ATTACK_POWER_MULTIPLIER = multiplier - 1
    GetTarget()->HandleStatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_PCT, amount, apply);
}

void Aura::HandleAuraModRangedAttackPowerPercent(bool apply, bool /*Real*/)
{
#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_8_4
    if ((GetTarget()->getClassMask() & CLASSMASK_WAND_USERS) != 0)
        return;

    float amount = m_modifier.m_amount;

    if (Unit* caster = GetCaster())
        if (Player* modOwner = caster->GetSpellModOwner())
            modOwner->ApplySpellMod(GetSpellProto()->Id, SPELLMOD_ATTACK_POWER, amount);

    // UNIT_FIELD_RANGED_ATTACK_POWER_MULTIPLIER = multiplier - 1
    GetTarget()->HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_PCT, amount, apply);
#endif
}

/********************************/
/***        DAMAGE BONUS      ***/
/********************************/
void Aura::HandleModDamageDone(bool apply, bool Real)
{
    Unit *target = GetTarget();

    // apply item specific bonuses for already equipped weapon
    if (Real && target->GetTypeId() == TYPEID_PLAYER)
    {
        for (int i = 0; i < MAX_ATTACK; ++i)
            if (Item* pItem = ((Player*)target)->GetWeaponForAttack(WeaponAttackType(i), true, false))
                ((Player*)target)->_ApplyWeaponDependentAuraDamageMod(pItem, WeaponAttackType(i), this, apply);
    }

    // m_modifier.m_miscvalue is bitmask of spell schools
    // 1 ( 0-bit ) - normal school damage (SPELL_SCHOOL_MASK_NORMAL)
    // 126 - full bitmask all magic damages (SPELL_SCHOOL_MASK_MAGIC) including wands
    // 127 - full bitmask any damages
    //
    // mods must be applied base at equipped weapon class and subclass comparison
    // with spell->EquippedItemClass and  EquippedItemSubClassMask and EquippedItemInventoryTypeMask
    // m_modifier.m_miscvalue comparison with item generated damage types

    if ((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL) != 0)
    {
        // apply generic physical damage bonuses including wand case
        if (GetSpellProto()->EquippedItemClass == -1 || target->GetTypeId() != TYPEID_PLAYER)
        {
            target->HandleStatModifier(UNIT_MOD_DAMAGE_PHYSICAL, TOTAL_VALUE, float(m_modifier.m_amount), apply);
        }
        else
        {
            // done in Player::_ApplyWeaponDependentAuraMods
        }

        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            if (m_positive)
                target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS, m_modifier.m_amount, apply);
            else
                target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG, m_modifier.m_amount, apply);
        }
    }

    // Skip non magic case for speedup
    if ((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_MAGIC) == 0)
        return;

    if (GetSpellProto()->EquippedItemClass != -1 || GetSpellProto()->EquippedItemInventoryTypeMask != 0)
    {
        // wand magic case (skip generic to all item spell bonuses)
        // done in Player::_ApplyWeaponDependentAuraMods

        // Skip item specific requirements for not wand magic damage
        return;
    }

    // Magic damage modifiers implemented in Unit::SpellDamageBonusDone
    // This information for client side use only
    if (target->GetTypeId() == TYPEID_PLAYER)
    {
        if (m_positive)
        {
            for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
            {
                if ((m_modifier.m_miscvalue & (1 << i)) != 0)
                    target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + i, m_modifier.m_amount, apply);
            }
        }
        else
        {
            for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
            {
                if ((m_modifier.m_miscvalue & (1 << i)) != 0)
                    target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG + i, m_modifier.m_amount, apply);
            }
        }
        Pet* pet = target->GetPet();
        if (pet)
            pet->UpdateAttackPowerAndDamage();
    }
}

void Aura::HandleModDamagePercentDone(bool apply, bool Real)
{
    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "AURA MOD DAMAGE type:%u negative:%u", m_modifier.m_miscvalue, m_positive ? 0 : 1);
    Unit *target = GetTarget();

    // apply item specific bonuses for already equipped weapon
    if (Real && target->GetTypeId() == TYPEID_PLAYER)
    {
        for (int i = 0; i < MAX_ATTACK; ++i)
            if (Item* pItem = ((Player*)target)->GetWeaponForAttack(WeaponAttackType(i), true, false))
                ((Player*)target)->_ApplyWeaponDependentAuraDamageMod(pItem, WeaponAttackType(i), this, apply);
    }

    // m_modifier.m_miscvalue is bitmask of spell schools
    // 1 ( 0-bit ) - normal school damage (SPELL_SCHOOL_MASK_NORMAL)
    // 126 - full bitmask all magic damages (SPELL_SCHOOL_MASK_MAGIC) including wand
    // 127 - full bitmask any damages
    //
    // mods must be applied base at equipped weapon class and subclass comparison
    // with spell->EquippedItemClass and  EquippedItemSubClassMask and EquippedItemInventoryTypeMask
    // m_modifier.m_miscvalue comparison with item generated damage types

    if ((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL) != 0)
    {
        // apply generic physical damage bonuses including wand case
        if (GetSpellProto()->EquippedItemClass == -1 || target->GetTypeId() != TYPEID_PLAYER)
        {
            target->HandleStatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_PCT, float(m_modifier.m_amount), apply);
            target->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_PCT, float(m_modifier.m_amount), apply);
            target->HandleStatModifier(UNIT_MOD_DAMAGE_RANGED, TOTAL_PCT, float(m_modifier.m_amount), apply);


			// For show in client
			if (target->GetTypeId() == TYPEID_PLAYER)
				target->ApplyModSignedFloatValue(PLAYER_FIELD_MOD_DAMAGE_DONE_PCT, m_modifier.m_amount / 100.0f, apply);
        }
        else
        {
            // done in Player::_ApplyWeaponDependentAuraMods
        }
    }

    // Skip non magic case for speedup
    if ((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_MAGIC) == 0)
        return;

    if (GetSpellProto()->EquippedItemClass != -1 || GetSpellProto()->EquippedItemInventoryTypeMask != 0)
    {
        // wand magic case (skip generic to all item spell bonuses)
        // done in Player::_ApplyWeaponDependentAuraMods

        // Skip item specific requirements for not wand magic damage
        return;
    }

    // Magic damage percent modifiers implemented in Unit::SpellDamageBonusDone
    // Send info to client
    if (target->GetTypeId() == TYPEID_PLAYER)
        for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
            target->ApplyModSignedFloatValue(PLAYER_FIELD_MOD_DAMAGE_DONE_PCT + i, m_modifier.m_amount / 100.0f, apply);

    if (GetSpellProto()->Id == 34063 || GetSpellProto()->Id == 34064 || GetSpellProto()->Id == 34065 || GetSpellProto()->Id == 34066 || GetSpellProto()->Id == 34067) //v+ Successful Reconnaissance
    {
        if ((apply) && !GetCaster()->HasAura(34499))
        {
            GetCaster()->CastSpell(target, 34499, true, nullptr, this);
        }
    }
}

void Aura::HandleModOffhandDamagePercent(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
        return;

    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "AURA MOD OFFHAND DAMAGE");

    GetTarget()->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

/********************************/
/***        POWER COST        ***/
/********************************/

void Aura::HandleModPowerCostPCT(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
        return;

    float amount = m_modifier.m_amount / 100.0f;
    for (int i = 0; i < MAX_SPELL_SCHOOL; ++i)
        if (m_modifier.m_miscvalue & (1 << i))
            GetTarget()->ApplyModSignedFloatValue(UNIT_FIELD_POWER_COST_MULTIPLIER + i, amount, apply);
}

void Aura::HandleModPowerCost(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
        return;

    for (int i = 0; i < MAX_SPELL_SCHOOL; ++i)
        if (m_modifier.m_miscvalue & (1 << i))
            GetTarget()->ApplyModInt32Value(UNIT_FIELD_POWER_COST_MODIFIER + i, m_modifier.m_amount, apply);
}

/*********************************************************/
/***                    OTHERS                         ***/
/*********************************************************/

void Aura::HandleReflectSpellsSchool(bool apply, bool Real)
{
    // all applied/removed only at real aura add/remove
    if (!Real)
        return;

    if (Unit* pCaster = GetCaster())
        if (Player *modOwner = pCaster->GetSpellModOwner())
            modOwner->ApplySpellMod(GetId(), SPELLMOD_RESIST_MISS_CHANCE, m_modifier.m_amount);
}

void Aura::HandleShapeshiftBoosts(bool apply)
{
    uint32 spellId1 = 0;
    uint32 spellId2 = 0;
    uint32 spellId3 = 0;
    uint32 HotWSpellId = 0;
    bool demonization = false;

    ShapeshiftForm form = ShapeshiftForm(GetModifier()->m_miscvalue);

    Unit *target = GetTarget();

    switch (form)
    {
        case FORM_CAT:
            spellId1 = 3025;
            HotWSpellId = 24900;
            break;
        case FORM_TREE:
            spellId1 = 5420;
            break;
        case FORM_TRAVEL:
            spellId1 = 5419;
            break;
        case FORM_AQUA:
            spellId1 = 5421;
            break;
        case FORM_BEAR:
            spellId1 = 1178;
            spellId2 = 21178;
            HotWSpellId = 24899;
            break;
        case FORM_DIREBEAR:
            spellId1 = 9635;
            spellId2 = 21178;
            HotWSpellId = 24899;
            break;
        case FORM_BATTLESTANCE:
            spellId1 = 21156;
            //Defiance
            if (target->HasAura(12303))
                spellId3 = 35790;
            else if (target->HasAura(12788))
                spellId3 = 35791;
            else if (target->HasAura(12789))
                spellId3 = 35792;
            else if (target->HasAura(12791))
                spellId3 = 35793;
            else if (target->HasAura(12792))
                spellId3 = 35794;
            break;
        case FORM_DEFENSIVESTANCE:
            spellId1 = 7376;
            //Defiance
            if (target->HasAura(12303))
                spellId3 = 35785;
            else if (target->HasAura(12788))
                spellId3 = 35786;
            else if (target->HasAura(12789))
                spellId3 = 35787;
            else if (target->HasAura(12791))
                spellId3 = 35788;
            else if (target->HasAura(12792))
                spellId3 = 35789;
            break;
        case FORM_BERSERKERSTANCE:
            spellId1 = 7381;
            spellId2 = 35490;
            //Defiance
            if (target->HasAura(12303))
                spellId3 = 35795;
            else if (target->HasAura(12788))
                spellId3 = 35796;
            else if (target->HasAura(12789))
                spellId3 = 35797;
            else if (target->HasAura(12791))
                spellId3 = 35798;
            else if (target->HasAura(12792))
                spellId3 = 35799;
            break;
        case FORM_MOONKIN:
            spellId1 = 24905;
            break;
        case FORM_SPIRITOFREDEMPTION:
            spellId1 = 27792;
            spellId2 = 35720;
            spellId3 = 27795;                               // must be third, this important at aura remove to prevent to early iterator invalidation.
            break;
        case FORM_GHOUL:
            spellId1 = 34653;
            spellId2 = 34654;
            spellId3 = 34655;
            demonization = true;
            break;
        case FORM_GHOSTWOLF:
        case FORM_AMBIENT:
        case FORM_SHADOW:
        case FORM_STEALTH:
        case FORM_CREATURECAT:
        case FORM_CREATUREBEAR:
            break;
    }

    if (apply)
    {
        if (spellId1)
            target->AddAura(spellId1, 0, target);
        if (spellId2)
            target->AddAura(spellId2, 0, target);
        if (spellId3)
            target->AddAura(spellId3, 0, target);


        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            const PlayerSpellMap& sp_list = ((Player *)target)->GetSpellMap();
            for (PlayerSpellMap::const_iterator itr = sp_list.begin(); itr != sp_list.end(); ++itr)
            {
                if (itr->second.state == PLAYERSPELL_REMOVED) continue;
                if (itr->first == spellId1 || itr->first == spellId2 || itr->first == spellId3) continue;
                SpellEntry const *spellInfo = sSpellMgr.GetSpellEntry(itr->first);
                if (!spellInfo || !spellInfo->IsNeedCastSpellAtFormApply(form))
                    continue;
                target->CastSpell(target, itr->first, true, nullptr, this);
            }

            //Metamorph
            if (demonization)
            {
                if (Player* player = (Player*)target)
                {
                    player->LearnSpell(34659, false);
                    player->LearnSpell(34661, false);
                    player->LearnSpell(34662, false);
                    player->LearnSpell(34663, false);

                    if (target->getClass() != CLASS_ROGUE && target->getClass() != CLASS_WARRIOR)
                        player->LearnSpell(34660, false);

                    player->RemoveFromGroup();
                    target->setFaction(1681);
                    /*target->setFaction(1620);*/
                    /*target->setFaction(168);*/
                    target->CastSpell(target, 34656, true);
                }

            }

            auto allowedInForm = [&](uint32 auraId, int32 form) -> bool
            {
                if (form <= 0 || form > 32) return false;
                SpellEntry const* info = sSpellMgr.GetSpellEntry(auraId);
                if (!info) return false;
                uint32 mask = (1u << (form - 1));
                return (info->Stances & mask) != 0;
            };

            auto applyIf = [&](uint32 learnedSpellId, uint32 auraId)
            {
                if (target->HasSpell(learnedSpellId) && allowedInForm(auraId, form))
                    target->CastSpell(target, auraId, true, nullptr, this);
            };

            // Leader of the Pack
            applyIf(17007, 24932);

            // Moonkin Aura
            applyIf(34196, 24907);

            // Vanilla+ Druid Naturalist ranks
            applyIf(17069, 34146);
            applyIf(17070, 34147);
            applyIf(17071, 34148);
            applyIf(17072, 34149);
            applyIf(17073, 34150);


            //if (((Player*)target)->HasSpell(17002)) //vanilla+ druid feline swiftnes r1
            //{
            //    SpellEntry const *spellInfo = sSpellMgr.GetSpellEntry(25333);
            //    if (spellInfo && spellInfo->Stances & (1 << (form - 1)))
            //        target->CastSpell(target, 25333, true, nullptr, this);
            //}

            //if (((Player*)target)->HasSpell(24866)) //vanilla+ druid feline swiftnes r2
            //{
            //    SpellEntry const *spellInfo = sSpellMgr.GetSpellEntry(25205);
            //    if (spellInfo && spellInfo->Stances & (1 << (form - 1)))
            //        target->CastSpell(target, 25205, true, nullptr, this);
            //}

            // Heart of the Wild
            if (HotWSpellId)
            {
                Unit::AuraList const& mModTotalStatPct = target->GetAurasByType(SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE);
                for (Unit::AuraList::const_iterator i = mModTotalStatPct.begin(); i != mModTotalStatPct.end(); ++i)
                {
                    if ((*i)->GetSpellProto()->SpellIconID == 240 && (*i)->GetModifier()->m_miscvalue == 3)
                    {
                        int32 HotWMod = ((*i)->GetModifier()->m_amount / 2);
                        target->CastCustomSpell(target, HotWSpellId, &HotWMod, nullptr, nullptr, true, nullptr, this);
                        break;
                    }
                }
            }
        }

        if (form == FORM_DIREBEAR)
            if (target->HasAura(34863))
                target->RemoveAurasDueToSpell(34863);
        if (form == FORM_BEAR)
            if (target->HasAura(34864))
                target->RemoveAurasDueToSpell(34864);
    }
    else
    {
        if (spellId1)
            target->RemoveAurasDueToSpell(spellId1);
        if (spellId2)
            target->RemoveAurasDueToSpell(spellId2);
        if (spellId3)
            target->RemoveAurasDueToSpell(spellId3);

        if (form == FORM_DIREBEAR)
            target->CastSpell(target, 34863, true);
        if (form == FORM_BEAR)
            target->CastSpell(target, 34864, true);
        //Imp Defensive Stance dispell on stance dance
        if (form == FORM_DEFENSIVESTANCE)
        {
            target->RemoveAurasDueToSpell(35818);
            target->RemoveAurasDueToSpell(35819);
        }

        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            if (demonization)
            {
                Player *player = (Player*)target;

                player->RemoveSpell(34659, false);
                player->RemoveSpell(34661, false);
                player->RemoveSpell(34662, false);
                player->RemoveSpell(34663, false);

                if (target->getClass() != CLASS_ROGUE && target->getClass() != CLASS_WARRIOR)
                    player->RemoveSpell(34660, false);

                player->RemoveAurasDueToSpell(34657);
                player->RemoveAurasDueToSpell(34658);
                player->RemoveAurasDueToSpell(34666);
                player->SetFactionForRace(player->getRace());
            }
        }

        Unit::SpellAuraHolderMap& tAuras = target->GetSpellAuraHolderMap();
        for (Unit::SpellAuraHolderMap::iterator itr = tAuras.begin(); itr != tAuras.end();)
        {
            if (itr->second->IsRemovedOnShapeLost())
            {
                target->RemoveAurasDueToSpell(itr->second->GetId());
                itr = tAuras.begin();
            }
            else
                ++itr;
        }

        // Interrupt current shape specific spells
        for (uint32 i = CURRENT_MELEE_SPELL; i < CURRENT_MAX_SPELL; ++i)
            if (Spell* spell = target->GetCurrentSpell(CurrentSpellTypes(i)))
                if (spell->m_spellInfo->IsRemovedOnShapeLostSpell())
                    target->InterruptSpell(CurrentSpellTypes(i), false);
    }
}

void Aura::HandleAuraEmpathy(bool apply, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_UNIT)
        return;

    CreatureInfo const * ci = ObjectMgr::GetCreatureTemplate(GetTarget()->GetEntry());
    if (ci && ci->type == CREATURE_TYPE_BEAST)
        GetTarget()->ApplyModUInt32Value(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_SPECIALINFO, apply);
}

void Aura::HandleAuraUntrackable(bool apply, bool /*Real*/)
{
    if (apply)
        GetTarget()->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_UNTRACKABLE);
    else
        GetTarget()->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_UNTRACKABLE);

    if (GetId() == 13161)
    {
        Unit* target = GetTarget();
        if (apply)
        {
            target->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAGS_CREEP);
            if (target->GetTypeId() == TYPEID_PLAYER)
                target->SetByteFlag(PLAYER_FIELD_BYTES2, 1, PLAYER_FIELD_BYTE2_STEALTH);
        }
        else
        {
            if (!target->HasAuraType(SPELL_AURA_MOD_STEALTH))
            {
                // if no GM invisibility
                if (target->GetVisibility() != VISIBILITY_OFF)
                {
                    target->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAGS_CREEP);
                    if (target->GetTypeId() == TYPEID_PLAYER)
                        target->RemoveByteFlag(PLAYER_FIELD_BYTES2, 1, PLAYER_FIELD_BYTE2_STEALTH);
                }
            }

        }
    }
}

void Aura::HandleAuraModPacify(bool apply, bool /*Real*/)
{
    if (apply)
        GetTarget()->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
    else
        GetTarget()->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
}

void Aura::HandleAuraModPacifyAndSilence(bool apply, bool Real)
{
    HandleAuraModPacify(apply, Real);
    HandleAuraModSilence(apply, Real);
}

void Aura::HandleAuraGhost(bool apply, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    if (apply)
        GetTarget()->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST);
    else
        GetTarget()->RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST);

    if (GetId() == 35950)   //Ghost visuals
        return;

    if (((Player*)GetTarget())->GetGroup())
        ((Player*)GetTarget())->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_STATUS);
}

void Aura::HandleShieldBlockValue(bool apply, bool /*Real*/)
{
    BaseModType modType = FLAT_MOD;
    if (m_modifier.m_auraname == SPELL_AURA_MOD_SHIELD_BLOCKVALUE_PCT)
        modType = PCT_MOD;

    if (GetTarget()->GetTypeId() == TYPEID_PLAYER)
        ((Player*)GetTarget())->HandleBaseModValue(SHIELD_BLOCK_VALUE, modType, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraRetainComboPoints(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
        return;

    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    Player *target = (Player*)GetTarget();

    // combo points was added in SPELL_EFFECT_ADD_COMBO_POINTS handler
    // remove only if aura expire by time (in case combo points amount change aura removed without combo points lost)
    if (!apply && m_removeMode == AURA_REMOVE_BY_EXPIRE && target->GetComboTargetGuid())
        if (Unit* unit = ObjectAccessor::GetUnit(*GetTarget(), target->GetComboTargetGuid()))
            target->AddComboPoints(unit, -m_modifier.m_amount);
}

void Aura::HandleModUnattackable(bool Apply, bool Real)
{
    if (Real && Apply)
    {
        GetTarget()->CombatStop();
        GetTarget()->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);
    }
    GetTarget()->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE_2, Apply);
}

void Aura::HandleSpiritOfRedemption(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
        return;

    Unit *target = GetTarget();

    // prepare spirit state
    if (apply)
    {
        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            // disable breath/etc timers
            ((Player*)target)->StopMirrorTimers();

            // set stand state (expected in this form)
            if (!target->IsStandingUp())
                target->SetStandState(UNIT_STAND_STATE_STAND);
        }

        target->SetHealth(1);
    }
    // die at aura end
    else
        target->DealDamage(target, target->GetHealth(), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, GetSpellProto(), false);
}

void Aura::HandleAuraAoeCharm(bool apply, bool real)
{
    if (!real)
        return;
    if (GetSpellProto()->Id == 28410) // Chains of Kel'Thuzad 
    {
        HandleModCharm(apply, real);
    }
}

void Aura::HandleSchoolAbsorb(bool apply, bool Real)
{
    if (!Real)
        return;

    Unit* caster = GetCaster();
    if (!caster)
        return;

    Unit *target = GetTarget();
    SpellEntry const* spellProto = GetSpellProto();
    if (apply)
    {
        // prevent double apply bonuses
        if (target->GetTypeId() != TYPEID_PLAYER || !((Player*)target)->GetSession()->PlayerLoading())
        {
            float DoneActualBenefit = 0.0f;
            switch (spellProto->SpellFamilyName)
            {
                case SPELLFAMILY_PRIEST:
                    // Power Word: Shield
                    if (spellProto->IsFitToFamilyMask<CF_PRIEST_POWER_WORD_SHIELD>())
                    {
                        //+10% from +healing bonus
                        DoneActualBenefit = caster->SpellBaseHealingBonusDone(spellProto->GetSpellSchoolMask()) * 0.1f;
                        break;
                    }
                    break;
                case SPELLFAMILY_MAGE:
                    // Frost ward, Fire ward
                    if (spellProto->IsFitToFamilyMask<CF_MAGE_FIRE_WARD, CF_MAGE_FROST_WARD>())
                    {
                        //+10% from +spd bonus
                        DoneActualBenefit = caster->SpellBaseDamageBonusDone(spellProto->GetSpellSchoolMask()) * 0.1f;
                        break;
                    }
                    break;
                case SPELLFAMILY_WARLOCK:
                    // Shadow Ward
                    if (spellProto->SpellIconID == 207 && spellProto->Category == 56)
                    {
                        //+10% from +spd bonus
                        DoneActualBenefit = caster->SpellBaseDamageBonusDone(spellProto->GetSpellSchoolMask()) * 0.1f;
                        break;
                    }
                    break;
                default:
                    break;
            }

            DoneActualBenefit *= caster->CalculateLevelPenalty(GetSpellProto());

            m_modifier.m_amount += (int32)DoneActualBenefit;

            // Power Word: Shield generates half the threat as healing for the same amount
            if (spellProto->IsFitToFamily<SPELLFAMILY_PRIEST, CF_PRIEST_POWER_WORD_SHIELD>() && spellProto->Id != 27779)
                caster->getHostileRefManager().threatAssist(caster, float(m_modifier.m_amount) * 0.25, spellProto);
        }
    }
}

void Aura::PeriodicTick(SpellEntry const* sProto, AuraType auraType, uint32 data)
{
    Unit *target = GetTarget();
    SpellEntry const* spellProto = sProto ? sProto : GetSpellProto();
    AuraType const aura_type = sProto ? auraType : m_modifier.m_auraname;

    switch (aura_type)
    {
        case SPELL_AURA_PERIODIC_DAMAGE:
        case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
        {
            // don't damage target if not alive, possible death persistent effects
            if (!target->isAlive())
                return;

            Unit *pCaster = GetCaster();
            if (!pCaster)
                return;

            if (spellProto->Effect[GetEffIndex()] == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
                    pCaster->SpellHitResult(target, spellProto, GetEffIndex(), false) != SPELL_MISS_NONE)
                return;

            // Check for immune (not use charges)
            if (target->IsImmuneToDamage(spellProto->GetSpellSchoolMask(), spellProto))
                return;

            uint32 absorb = 0;
            int32 resist = 0;
            uint32 pdamage;
            CleanDamage cleanDamage = CleanDamage(0, BASE_ATTACK, MELEE_HIT_NORMAL, 0, 0);

            if (sProto)
            {
                if (auraType == SPELL_AURA_PERIODIC_DAMAGE)
                    pdamage = data;
                else
                    pdamage = uint32(target->GetMaxHealth() * data / 100);
            }
            else
            {
                if (m_modifier.m_auraname == SPELL_AURA_PERIODIC_DAMAGE)
#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_9_4
                    pdamage = (m_modifier.m_amount > 0 ? uint32(m_modifier.m_amount) : 0);
#else
                    pdamage = CalculateDotDamage();
#endif
                else
                    pdamage = uint32(target->GetMaxHealth() * (m_modifier.m_amount > 0 ? m_modifier.m_amount : 0) / 100);
            }

            //Arena Ignition debuff
            if (spellProto->Id == 35432)
            {
                if (target->HasAuraType(SPELL_AURA_MOD_POSSESS) || target->HasAuraType(SPELL_AURA_MOD_CHARM))
                    target->RemoveCharmAuras();
            }

            //Deep Wounds
            if (spellProto->Id == 12721)
            {
                float dmg = 4.0f;
                if (pCaster->haveOffhandWeapon())
                {
                    dmg = (pCaster->GetFloatValue(UNIT_FIELD_MINDAMAGE) + pCaster->GetFloatValue(UNIT_FIELD_MAXDAMAGE)) / 2;
                    dmg += (pCaster->GetFloatValue(UNIT_FIELD_MINOFFHANDDAMAGE) + pCaster->GetFloatValue(UNIT_FIELD_MAXOFFHANDDAMAGE)) / 2;
                }
                else
                    dmg = (pCaster->GetFloatValue(UNIT_FIELD_MINDAMAGE) + pCaster->GetFloatValue(UNIT_FIELD_MAXDAMAGE)) / 2;

                if (pCaster->HasAura(12714)) //Imp 2H R5 
                    dmg *= 1.25f;
                else if (pCaster->HasAura(12713))  //R4
                    dmg *= 1.2f;
                else if (pCaster->HasAura(12712))  //R3
                    dmg *= 1.15f;
                else if (pCaster->HasAura(12711))  //R2
                    dmg *= 1.1f;
                else if (pCaster->HasAura(12163))  //R1
                    dmg *= 1.05f;

                if (pCaster->HasAura(12867)) //R3
                    dmg *= 0.30f;
                else if (pCaster->HasAura(12849)) //R2
                    dmg *= 0.20f;
                else if (pCaster->HasAura(12834)) //R1
                    dmg *= 0.10f;

                int32 deepWoundsDotBasePoints0 = int32(dmg / 4);

                pdamage = deepWoundsDotBasePoints0 * GetStackAmount();
            }

            // Consecration: recalculate the damage on each tick
            if (spellProto->IsFitToFamily<SPELLFAMILY_PALADIN, CF_PALADIN_CONSECRATION>())
                pdamage = pCaster->SpellDamageBonusDone(target, GetSpellProto(), m_currentBasePoints, DOT, GetStackAmount());

            // SpellDamageBonus for magic spells
            if (spellProto->DmgClass == SPELL_DAMAGE_CLASS_NONE || spellProto->DmgClass == SPELL_DAMAGE_CLASS_MAGIC)
                pdamage = target->SpellDamageBonusTaken(pCaster, spellProto, pdamage, DOT, GetStackAmount());
            // MeleeDamagebonus for weapon based spells
            else
            {
                WeaponAttackType attackType = spellProto->GetWeaponAttackType();
                pdamage = target->MeleeDamageBonusTaken(pCaster, pdamage, attackType, spellProto, DOT, GetStackAmount());
            }

            // Calculate armor mitigation if it is a physical spell
            // But not for bleed mechanic spells
            if (spellProto->GetSpellSchoolMask() & SPELL_SCHOOL_MASK_NORMAL && spellProto->GetEffectMechanic(m_effIndex) != MECHANIC_BLEED && !(spellProto->Custom & SPELL_CUSTOM_IGNORE_ARMOR))
            {
                uint32 pdamageReductedArmor = pCaster->CalcArmorReducedDamage(target, pdamage);
                cleanDamage.damage += pdamage - pdamageReductedArmor;
                pdamage = pdamageReductedArmor;
            }

            // TODO (Curse of Agony/Starshards):
            // only base damage should be affected, not whole damage

            // Curse of Agony damage-per-tick calculation
            if (spellProto->IsFitToFamily<SPELLFAMILY_WARLOCK, CF_WARLOCK_CURSE_OF_AGONY>())
            {
                // 1..4 ticks, 1/2 from normal tick damage
                if (GetAuraTicks() <= 4)
                    pdamage = pdamage / 2;
                // 9..12 ticks, 3/2 from normal tick damage
                else if (GetAuraTicks() >= 9)
                    pdamage += (pdamage + 1) / 2;       // +1 prevent 0.5 damage possible lost at 1..4 ticks
                // 5..8 ticks have normal tick damage
            }
            if (spellProto->IsFitToFamily<SPELLFAMILY_PRIEST, CF_PRIEST_STARSHARDS>())
            {
                //ticks: 2/3, 2/3, 1, 1, 4/3, 4/3
                float ticks[] = {0,.111f,.222f,.389f,.556f,.778f,1};
                float dmg = ticks[GetAuraTicks() -1];
                float ddone = ticks[GetAuraTicks()];
                pdamage *= 6;
                pdamage = std::round(std::round(pdamage * ddone) - pdamage * dmg);
            }


            target->CalculateDamageAbsorbAndResist(pCaster, spellProto->GetSpellSchoolMask(), DOT, pdamage, &absorb, &resist, spellProto);

            DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %s attacked %s for %u dmg inflicted by %u",
                              GetCasterGuid().GetString().c_str(), target->GetGuidStr().c_str(), pdamage, GetId());

            pCaster->DealDamageMods(target, pdamage, &absorb);

            // Set trigger flag
            uint32 procAttacker = PROC_FLAG_ON_DO_PERIODIC;
            uint32 procVictim   = PROC_FLAG_ON_TAKE_PERIODIC;

            const uint32 bonus = (resist < 0 ? uint32(std::abs(resist)) : 0);
            pdamage += bonus;
            const uint32 malus = (resist > 0 ? (absorb + uint32(resist)) : absorb);
            pdamage = (pdamage <= malus ? 0 : (pdamage - malus));

            SpellPeriodicAuraLogInfo pInfo(this, pdamage, absorb, resist, 0.0f);
            target->SendPeriodicAuraLog(&pInfo, sProto ? auraType : SPELL_AURA_NONE);

            if (pdamage)
                procVictim |= PROC_FLAG_TAKEN_ANY_DAMAGE;

            pCaster->ProcDamageAndSpell(target, procAttacker, procVictim, PROC_EX_NORMAL_HIT, pdamage, BASE_ATTACK, spellProto);

            cleanDamage.absorb = absorb;
            cleanDamage.resist = resist;
            pCaster->DealDamage(target, pdamage, &cleanDamage, DOT, spellProto->GetSpellSchoolMask(), spellProto, true);
            // Curse of Doom: If the target dies from this damage, there is a chance that a Doomguard will be summoned.
            if (spellProto->Id == 603 && !target->isAlive() && !urand(0, 9))
                pCaster->CastSpell(pCaster, 18662, true);
            break;
        }
        case SPELL_AURA_PERIODIC_LEECH:
        case SPELL_AURA_PERIODIC_HEALTH_FUNNEL:
        {
            // don't damage target if not alive, possible death persistent effects
            if (!target->IsInWorld() || !target->isAlive())
                return;

            Unit *pCaster = GetCaster();
            if (!pCaster)
                return;

            if (!pCaster->IsInWorld() || !pCaster->isAlive())
                return;

            if (spellProto->Effect[GetEffIndex()] == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
                    pCaster->SpellHitResult(target, spellProto, GetEffIndex(), false) != SPELL_MISS_NONE)
                return;

            // Check for immune
            if (target->IsImmuneToDamage(spellProto->GetSpellSchoolMask(), spellProto))
                return;

            uint32 absorb = 0;
            int32 resist = 0;
            CleanDamage cleanDamage =  CleanDamage(0, BASE_ATTACK, MELEE_HIT_NORMAL, 0, 0);

            uint32 pdamage = (m_modifier.m_amount > 0 ? uint32(m_modifier.m_amount) : 0);

            //Calculate armor mitigation if it is a physical spell
            if (spellProto->GetSpellSchoolMask() & SPELL_SCHOOL_MASK_NORMAL)
            {
                uint32 pdamageReductedArmor = pCaster->CalcArmorReducedDamage(target, pdamage);
                cleanDamage.damage += pdamage - pdamageReductedArmor;
                pdamage = pdamageReductedArmor;
            }

            pdamage = target->SpellDamageBonusTaken(pCaster, spellProto, pdamage, DOT, GetStackAmount());

            target->CalculateDamageAbsorbAndResist(pCaster, spellProto->GetSpellSchoolMask(), DOT, pdamage, &absorb, &resist, spellProto);

            DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %s health leech of %s for %u dmg inflicted by %u abs is %u",
                              GetCasterGuid().GetString().c_str(), target->GetGuidStr().c_str(), pdamage, GetId(), absorb);

            pCaster->DealDamageMods(target, pdamage, &absorb);

            pCaster->SendSpellNonMeleeDamageLog(target, GetId(), pdamage, spellProto->GetSpellSchoolMask(), absorb, resist, true, 0);

            float multiplier = spellProto->EffectMultipleValue[GetEffIndex()] > 0 ? spellProto->EffectMultipleValue[GetEffIndex()] : 1;

            // Set trigger flag
            uint32 procAttacker = PROC_FLAG_ON_DO_PERIODIC;
            uint32 procVictim   = PROC_FLAG_ON_TAKE_PERIODIC;

            const uint32 bonus = (resist < 0 ? uint32(std::abs(resist)) : 0);
            pdamage += bonus;
            const uint32 malus = (resist > 0 ? (absorb + uint32(resist)) : absorb);
            pdamage = (pdamage <= malus ? 0 : (pdamage - malus));

            pdamage = std::min(pdamage, target->GetHealth());

            if (pdamage)
                procVictim |= PROC_FLAG_TAKEN_ANY_DAMAGE;

            cleanDamage.absorb = absorb;
            cleanDamage.resist = resist;

            pCaster->ProcDamageAndSpell(target, procAttacker, procVictim, PROC_EX_NORMAL_HIT, pdamage, BASE_ATTACK, spellProto);
            int32 new_damage = pCaster->DealDamage(target, pdamage, &cleanDamage, DOT, spellProto->GetSpellSchoolMask(), spellProto, false);

            if (!target->isAlive() && pCaster->IsNonMeleeSpellCasted(false))
                for (uint32 i = CURRENT_FIRST_NON_MELEE_SPELL; i < CURRENT_MAX_SPELL; ++i)
                    if (Spell* spell = pCaster->GetCurrentSpell(CurrentSpellTypes(i)))
                        if (spell->m_spellInfo->Id == GetId())
                            spell->cancel();

            if (Player *modOwner = pCaster->GetSpellModOwner())
                modOwner->ApplySpellMod(GetId(), SPELLMOD_MULTIPLE_VALUE, multiplier);

            uint32 heal = int32(new_damage * multiplier);

            int32 gain = pCaster->DealHeal(pCaster, heal, spellProto);
            pCaster->getHostileRefManager().threatAssist(pCaster, gain * 0.25f * sSpellMgr.GetSpellThreatMultiplier(spellProto), spellProto);
            break;
        }
        case SPELL_AURA_PERIODIC_HEAL:
        case SPELL_AURA_OBS_MOD_HEALTH:
        {
            // don't heal target if not alive, mostly death persistent effects from items
            if (!target->isAlive())
                return;

            Unit *pCaster = GetCaster();
            if (!pCaster)
                return;

            // Don't heal target if it is already at max health. We still need
            // to do procs on the tick, however
            if (target->GetHealth() == target->GetMaxHealth())
            {
                uint32 procAttacker = PROC_FLAG_ON_DO_PERIODIC;
                uint32 procVictim = PROC_FLAG_ON_TAKE_PERIODIC;
                uint32 procEx = PROC_EX_NORMAL_HIT | PROC_EX_PERIODIC_POSITIVE;
                pCaster->ProcDamageAndSpell(target, procAttacker, procVictim, procEx, 1, BASE_ATTACK, spellProto);

                return;
            }

            // heal for caster damage (must be alive)
            if (target != pCaster && spellProto->SpellVisual == 163 && !pCaster->isAlive())
                return;

#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_10_2
            // Ignore non positive values (can be result apply spellmods to aura damage).
            uint32 amount = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;
#else
            // Before 1.11 the heal bonuses were calculated each tick, not upon initial cast.
            uint32 amount;
            if (aura_type == SPELL_AURA_PERIODIC_HEAL)
            {
                int32 const intAmount = pCaster->SpellHealingBonusDone(target, GetSpellProto(), m_modifier.m_amount, DOT, GetStackAmount());
                amount = intAmount > 0 ? intAmount : 0;
            }
            else
                amount = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;
#endif

            uint32 pdamage;

            if (m_modifier.m_auraname == SPELL_AURA_OBS_MOD_HEALTH)
            {
                pdamage = uint32(target->GetMaxHealth() * amount / 100);
                //Blood Craze/Earth Shield/Enveloping Shadows Stealth/Mar'li Transform/Metamorph/Demonic Embrace hack
                if (spellProto->SpellIconID == 215 || spellProto->SpellIconID == 2111 || spellProto->SpellIconID == 90 || spellProto->Id == 34599)
                    pdamage *= 0.1f;
            }
            else
                pdamage = amount;

            pdamage = target->SpellHealingBonusTaken(pCaster, spellProto, pdamage, DOT, GetStackAmount());

            
            if (spellProto->Mechanic == MECHANIC_BANDAGE)
            {
                if (pCaster->HasAura(36266)) // Scarlet Triage Kit
                    pdamage *= 1.5f;
            }
            if (pCaster->HasAura(36279) && target->HasAuraWithMechanic(MECHANIC_BLEED))    //Bloodied Gloves
            {
                    pdamage = pdamage * 105 / 100; // +5%
            }
            // Locket of Devotion
            if (pCaster->HasAura(36307))
            {
                if (Aura* aura = target->GetAura(36306, EFFECT_INDEX_0))
                {
                    if (aura->GetCasterGuid() == pCaster->GetObjectGuid())
                        pdamage = pdamage * 103 / 100; // +3%
                }
            }
            // Fairbanks Healing 5%
            if (pCaster->HasAura(36316))
            {
                if (pCaster->GetHealthPercent() < target->GetHealthPercent())
                    pdamage = pdamage * 105 / 100; // +5%
            }

            DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %s heal of %s for %u health inflicted by %u",
                              GetCasterGuid().GetString().c_str(), target->GetGuidStr().c_str(), pdamage, GetId());

            int32 gain = target->ModifyHealth(pdamage);
            SpellPeriodicAuraLogInfo pInfo(this, pdamage, 0, 0, 0.0f);
            target->SendPeriodicAuraLog(&pInfo);

            // Set trigger flag
            uint32 procAttacker = PROC_FLAG_ON_DO_PERIODIC;
            uint32 procVictim   = PROC_FLAG_ON_TAKE_PERIODIC;
            uint32 procEx = PROC_EX_NORMAL_HIT | PROC_EX_PERIODIC_POSITIVE;
            pCaster->ProcDamageAndSpell(target, procAttacker, procVictim, procEx, gain, BASE_ATTACK, spellProto);

            float Onythreat = 1.0f;
            if (pCaster->HasAura(34444) || pCaster->HasAura(24314))
                Onythreat = 0.0f;

            target->getHostileRefManager().threatAssist(pCaster, float(gain) * Onythreat * 0.5f * sSpellMgr.GetSpellThreatMultiplier(spellProto), spellProto);

            // heal for caster damage, warlock's health funnel already cost hps
            if (target != pCaster && spellProto->SpellVisual == 163 &&
                !(spellProto->SpellFamilyName == SPELLFAMILY_WARLOCK &&
                spellProto->IsFitToFamilyMask<CF_WARLOCK_HEALTH_FUNNEL>()))
            {
                uint32 dmg = spellProto->manaPerSecond;
                if (pCaster->GetHealth() <= dmg && pCaster->GetTypeId() == TYPEID_PLAYER)
                {
                    pCaster->RemoveAurasDueToSpell(GetId());

                    // finish current generic/channeling spells, don't affect autorepeat
                    pCaster->FinishSpell(CURRENT_GENERIC_SPELL);
                    pCaster->FinishSpell(CURRENT_CHANNELED_SPELL);
                }
                else
                {
                    uint32 damage = gain;
                    uint32 absorb = 0;
                    pCaster->DealDamageMods(pCaster, damage, &absorb);
                    pCaster->SendSpellNonMeleeDamageLog(pCaster, GetId(), damage, spellProto->GetSpellSchoolMask(), absorb, 0, true, 0, false);

                    CleanDamage cleanDamage =  CleanDamage(0, BASE_ATTACK, MELEE_HIT_NORMAL, absorb, 0);
                    pCaster->DealDamage(pCaster, damage, &cleanDamage, NODAMAGE, spellProto->GetSpellSchoolMask(), spellProto, true);
                }
            }
            break;
        }
        case SPELL_AURA_PERIODIC_MANA_LEECH:
        {
            // don't damage target if not alive, possible death persistent effects
            if (!target->isAlive())
                return;

            if (m_modifier.m_miscvalue < 0 || m_modifier.m_miscvalue >= MAX_POWERS)
                return;

            Powers power = Powers(m_modifier.m_miscvalue);

            // power type might have changed between aura applying and tick (druid's shapeshift)
            if (target->getPowerType() != power)
                return;

            Unit *pCaster = GetCaster();
            if (!pCaster)
                return;

            if (!pCaster->isAlive())
                return;

            if (GetSpellProto()->Effect[GetEffIndex()] == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
                    pCaster->SpellHitResult(target, spellProto, GetEffIndex(), false) != SPELL_MISS_NONE)
                return;

            // Check for immune (not use charges)
            if (target->IsImmuneToDamage(spellProto->GetSpellSchoolMask(), spellProto))
                return;

            // ignore non positive values (can be result apply spellmods to aura damage
            uint32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %s power leech of %s for %u dmg inflicted by %u",
                              GetCasterGuid().GetString().c_str(), target->GetGuidStr().c_str(), pdamage, GetId());

            int32 drain_amount = target->GetPower(power) > pdamage ? pdamage : target->GetPower(power);

            target->ModifyPower(power, -drain_amount);

            float gain_multiplier = 0;

            if (pCaster->GetMaxPower(power) > 0)
            {
                gain_multiplier = spellProto->EffectMultipleValue[GetEffIndex()];

                if (Player *modOwner = pCaster->GetSpellModOwner())
                    modOwner->ApplySpellMod(GetId(), SPELLMOD_MULTIPLE_VALUE, gain_multiplier);
            }

            SpellPeriodicAuraLogInfo pInfo(this, drain_amount, 0, 0, gain_multiplier);
            target->SendPeriodicAuraLog(&pInfo);

            int32 gain_amount = int32(drain_amount * gain_multiplier);

            if (gain_amount)
            {
                float threat = pCaster->ModifyPower(power, gain_amount) * sSpellMgr.GetSpellThreatMultiplier(spellProto);
                threat *= 0.25; //v+ Mana Drain was 0.5 
                target->AddThreat(pCaster, threat, false, spellProto->GetSpellSchoolMask(), spellProto);
            }
            if (target->GetPower(power) == 0)
            {
                if (spellProto->Id == 21056) //Marque de Kazzak
                {
                    // Explose quand y'a plus de mana a drainer
                    target->CastSpell(target, 21058, true);
                    GetHolder()->SetAuraDuration(0);
                }
            }

            // Improved Drain Mana
            auto improvedManaDrain1 = pCaster->GetAura(17864, EFFECT_INDEX_0);
            auto improvedManaDrain2 = pCaster->GetAura(18393, EFFECT_INDEX_0);

            if (improvedManaDrain2)
                PeriodicTick(improvedManaDrain2->GetHolder()->GetSpellProto(), SPELL_AURA_PERIODIC_DAMAGE, drain_amount * 0.5f);
            else if (improvedManaDrain1)
                PeriodicTick(improvedManaDrain1->GetHolder()->GetSpellProto(), SPELL_AURA_PERIODIC_DAMAGE, drain_amount * 0.25f);

            // Nostalrius: break des controles type 'AURA_INTERRUPT_FLAG_DAMAGE'
            if (!(target->HasAuraType(SPELL_AURA_MOD_STEALTH) && !target->CanBeDetected()))
                target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_DAMAGE);
            break;
        }
        case SPELL_AURA_PERIODIC_ENERGIZE:
        {
            // don't energize target if not alive, possible death persistent effects
            if (!target->isAlive())
                return;

            // ignore non positive values (can be result apply spellmods to aura damage
            uint32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %s energize %s for %u dmg inflicted by %u",
                              GetCasterGuid().GetString().c_str(), target->GetGuidStr().c_str(), pdamage, GetId());

            if (m_modifier.m_miscvalue < 0 || m_modifier.m_miscvalue >= MAX_POWERS)
                break;

            Powers power = Powers(m_modifier.m_miscvalue);

            if (target->GetMaxPower(power) == 0)
                break;

            SpellPeriodicAuraLogInfo pInfo(this, pdamage, 0, 0, 0.0f);
            target->SendPeriodicAuraLog(&pInfo);

            int32 gain = target->ModifyPower(power, pdamage);

            if (power != POWER_MANA)     // 1.9 - Mana regeneration over time will no longer generate threat. v+ 0.5>1.0f
                if (Unit* pCaster = GetCaster())
                    target->getHostileRefManager().threatAssist(pCaster, float(gain) * 1.1f * sSpellMgr.GetSpellThreatMultiplier(spellProto), spellProto);
            break;
        }
        case SPELL_AURA_OBS_MOD_MANA:
        {
            // don't energize target if not alive, possible death persistent effects
            if (!target->isAlive())
                return;

            // ignore non positive values (can be result apply spellmods to aura damage
            uint32 amount = (m_modifier.m_amount > 0 ? uint32(m_modifier.m_amount) : 0);

            uint32 pdamage = uint32(target->GetMaxPower(POWER_MANA) * amount / 100);

            DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %s energize %s for %u mana inflicted by %u",
                              GetCasterGuid().GetString().c_str(), target->GetGuidStr().c_str(), pdamage, GetId());

            if (target->GetMaxPower(POWER_MANA) == 0)
                break;

            SpellPeriodicAuraLogInfo pInfo(this, pdamage, 0, 0, 0.0f);
            target->SendPeriodicAuraLog(&pInfo);

            int32 gain = target->ModifyPower(POWER_MANA, pdamage);

            if (Unit* pCaster = GetCaster())
                target->getHostileRefManager().threatAssist(pCaster, float(gain) * 0.5f * sSpellMgr.GetSpellThreatMultiplier(spellProto), spellProto);
            break;
        }
        case SPELL_AURA_POWER_BURN_MANA:
        {
            // don't mana burn target if not alive, possible death persistent effects
            if (!target->isAlive())
                return;

            Unit *pCaster = GetCaster();
            if (!pCaster)
                return;

            // Check for immune (not use charges)
            if (target->IsImmuneToDamage(spellProto->GetSpellSchoolMask(), spellProto))
                return;

            int32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            Powers powerType = Powers(m_modifier.m_miscvalue);

            if (!target->isAlive() || target->getPowerType() != powerType)
                return;

            uint32 gain = uint32(-target->ModifyPower(powerType, -pdamage));

            gain = uint32(gain * spellProto->EffectMultipleValue[GetEffIndex()]);

            // maybe has to be sent different to client, but not by SMSG_PERIODICAURALOG
            SpellNonMeleeDamage damageInfo(pCaster, target, spellProto->Id, SpellSchools(spellProto->School));
            damageInfo.periodicLog = true;

            pCaster->CalculateSpellDamage(&damageInfo, gain, spellProto);

            damageInfo.target->CalculateAbsorbResistBlock(pCaster, &damageInfo, spellProto);

            pCaster->DealDamageMods(damageInfo.target, damageInfo.damage, &damageInfo.absorb);

            pCaster->SendSpellNonMeleeDamageLog(&damageInfo);

            // Set trigger flag
            uint32 procAttacker = PROC_FLAG_ON_DO_PERIODIC;
            uint32 procVictim   = PROC_FLAG_ON_TAKE_PERIODIC;
            uint32 procEx       = createProcExtendMask(&damageInfo, SPELL_MISS_NONE);
            if (damageInfo.damage)
                procVictim |= PROC_FLAG_TAKEN_ANY_DAMAGE;

            pCaster->ProcDamageAndSpell(damageInfo.target, procAttacker, procVictim, procEx, damageInfo.damage, BASE_ATTACK, spellProto);

            pCaster->DealSpellDamage(&damageInfo, true);
            break;
        }
        case SPELL_AURA_MOD_REGEN:
        {
            // don't heal target if not alive, possible death persistent effects
            if (!target->isAlive())
                return;

            int32 gain = target->ModifyHealth(m_modifier.m_amount);
            if (Unit *caster = GetCaster())
                target->getHostileRefManager().threatAssist(caster, float(gain) * 0.5f  * sSpellMgr.GetSpellThreatMultiplier(spellProto), spellProto);
            // Eating anim
            if (spellProto->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED)
                target->HandleEmoteCommand(EMOTE_ONESHOT_EAT);
            break;
        }
        case SPELL_AURA_MOD_POWER_REGEN:
        {
            // don't energize target if not alive, possible death persistent effects
            if (!target->isAlive())
                return;

            Powers pt = target->getPowerType();
            if (int32(pt) != m_modifier.m_miscvalue)
                return;

            if (spellProto->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED)
            {
                // eating anim
                target->HandleEmoteCommand(EMOTE_ONESHOT_EAT);
            }
            else if (GetId() == 20577)
            {
                // cannibalize anim
                target->HandleEmoteCommand(EMOTE_STATE_CANNIBALIZE);
            }

            // Anger Management
            // amount = 1+ 16 = 17 = 3,4*5 = 10,2*5/3
            // so 17 is rounded amount for 5 sec tick grow ~ 1 range grow in 3 sec
            if (pt == POWER_RAGE)
                target->ModifyPower(pt, m_modifier.m_amount * 3 / 5);
            break;
        }
        // Here tick dummy auras
        case SPELL_AURA_DUMMY:                              // some spells have dummy aura
        {
            PeriodicDummyTick();
            break;
        }
        case SPELL_AURA_PERIODIC_TRIGGER_SPELL:
        {
            TriggerSpell();
            break;
        }
        default:
            break;
    }
}

void Aura::PeriodicDummyTick()
{
    SpellEntry const* spell = GetSpellProto();
    Unit *target = GetTarget();
    switch (spell->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            switch (spell->Id)
            {
                // Forsaken Skills
                case 7054:
                {
                    uint32 spellRandom = urand(0, 14) + 7038;
                    sLog.outInfo("7054 %u", spellRandom);

                    target->CastSpell(target, spellRandom, true, nullptr, this);
                    // Possibly need cast one of them (but
                    // 7038 Forsaken Skill: Swords
                    // 7039 Forsaken Skill: Axes
                    // 7040 Forsaken Skill: Daggers
                    // 7041 Forsaken Skill: Maces
                    // 7042 Forsaken Skill: Staves
                    // 7043 Forsaken Skill: Bows
                    // 7044 Forsaken Skill: Guns
                    // 7045 Forsaken Skill: 2H Axes
                    // 7046 Forsaken Skill: 2H Maces
                    // 7047 Forsaken Skill: 2H Swords
                    // 7048 Forsaken Skill: Defense
                    // 7049 Forsaken Skill: Fire
                    // 7050 Forsaken Skill: Frost
                    // 7051 Forsaken Skill: Holy
                    // 7053 Forsaken Skill: Shadow
                    return;
                }
                case 8067:                                  // Party Time!
                {
                    switch (urand(0, 4))
                    {
                        case 0:
                            target->HandleEmoteCommand(EMOTE_ONESHOT_APPLAUD);
                            return;
                        case 1:
                            target->HandleEmoteCommand(EMOTE_ONESHOT_CHEER);
                            return;
                        case 2:
                            target->HandleEmoteCommand(EMOTE_ONESHOT_CHICKEN);
                            return;
                        case 3:
                            target->HandleEmoteCommand(EMOTE_ONESHOT_LAUGH);
                            return;
                        case 4:
                            target->HandleEmoteCommand(EMOTE_ONESHOT_DANCE);
                            return;
                    }
                    return;
                }
                case 7057:                                  // Haunting Spirits
                    if (roll_chance_i(33))
                        target->CastSpell(target, m_modifier.m_amount, true, nullptr, this);
                    return;
                case 16336:                                 // Haunting Phantoms
                {
                    if (urand(0,1))
                        target->CastSpell(target, 16334, true); // Summon Spiteful Phantom
                    else
                        target->CastSpell(target, 16335, true); // Summon Wrath Phantom
                    return;
                }
                case 24596:                                 // Intoxicating Venom
                    if (target->isInCombat())
                    {
                        uint32 rand = urand(0, 99);
                        if (rand < 7)
                            target->CastSpell(target, 8379, true, nullptr, this);     // Disarm
                        else if (rand < 14)
                            target->CastSpell(target, 6869, true, nullptr, this);     // Fall Down
                    }
                    return;
            }
            break;
        }

        case SPELLFAMILY_PALADIN:
        {
#if SUPPORTED_CLIENT_BUILD <= CLIENT_BUILD_1_8_4
            // Judgement of Command
            if (spell->SpellIconID == 561)
            {
                if (target && target->hasUnitState(UNIT_STAT_STUNNED))
                {
                    if (Unit* pCaster = GetCaster())
                    {
                        uint32 spellId = 0;
                        switch (spell->Id)
                        {
                            case 20425: // Rank 1
                                spellId = 20467;
                                break;
                            case 20962: // Rank 2
                                spellId = 20963;
                                break;
                            case 20961: // Rank 3
                                spellId = 20964;
                                break;
                            case 20967: // Rank 4
                                spellId = 20965;
                                break;
                            case 20968: // Rank 5
                                spellId = 20966;
                                break;
                        }
                        if (spellId)
                            pCaster->CastSpell(target, spellId, true);
                    }
                }
                return;
            }
#endif
            break;
        }
        default:
            break;
    }
}

void Aura::HandlePreventFleeing(bool apply, bool Real)
{
    if (!Real)
        return;

    Unit::AuraList const& fearAuras = GetTarget()->GetAurasByType(SPELL_AURA_MOD_FEAR);
    if (fearAuras.empty())
    {
        // Units may be feared without spell (Creature::DoFleeToGetAssistance)
        if (GetTarget()->GetTypeId() == TYPEID_UNIT)
            if (GetTarget()->GetMotionMaster()->GetCurrentMovementGeneratorType() == FLEEING_MOTION_TYPE ||
                    GetTarget()->GetMotionMaster()->GetCurrentMovementGeneratorType() == ASSISTANCE_MOTION_TYPE ||
                    GetTarget()->GetMotionMaster()->GetCurrentMovementGeneratorType() == ASSISTANCE_DISTRACT_MOTION_TYPE ||
                    GetTarget()->GetMotionMaster()->GetCurrentMovementGeneratorType() == TIMED_FLEEING_MOTION_TYPE)
                GetTarget()->GetMotionMaster()->MovementExpired(false);
    }
    else
    {
        if (apply)
            GetTarget()->SetFeared(false, fearAuras.front()->GetCasterGuid());
        else
            GetTarget()->SetFeared(true, fearAuras.front()->GetCasterGuid(), fearAuras.front()->GetId());
    }
}

void Aura::HandleManaShield(bool apply, bool Real)
{
    if (!Real)
        return;

    // prevent double apply bonuses
    if (apply && (GetTarget()->GetTypeId() != TYPEID_PLAYER || !((Player*)GetTarget())->GetSession()->PlayerLoading()))
    {
        if (Unit* caster = GetCaster())
        {
            float DoneActualBenefit = 0.0f;

            // Mana Shield
            // 0% coeff in vanilla (changed patch 2.4.0)
             if (GetSpellProto()->IsFitToFamily<SPELLFAMILY_MAGE, CF_MAGE_MANA_SHIELD>())
                DoneActualBenefit = caster->SpellBaseDamageBonusDone(GetSpellProto()->GetSpellSchoolMask()) * 0.5f;

             DoneActualBenefit *= caster->CalculateLevelPenalty(GetSpellProto());

            m_modifier.m_amount += (int32)DoneActualBenefit;
        }
    }
}

bool Aura::IsLastAuraOnHolder()
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (i != GetEffIndex() && GetHolder()->m_auras[i])
            return false;
    return true;
}

SpellAuraHolder::SpellAuraHolder(SpellEntry const* spellproto, Unit *target, Unit *caster, Item *castItem, WorldObject* pRealCaster) :
    m_spellProto(spellproto), m_target(target), m_castItemGuid(castItem ? castItem->GetObjectGuid() : ObjectGuid()),
    m_auraSlot(MAX_AURAS), m_auraLevel(1), m_procCharges(0),
    m_stackAmount(1), m_removeMode(AURA_REMOVE_BY_DEFAULT), m_AuraDRGroup(DIMINISHING_NONE), m_timeCla(1000),
    m_permanent(false), m_isRemovedOnShapeLost(true), m_deleted(false), m_in_use(0),
    m_debuffLimitAffected(false), m_debuffLimitScore(0), _heartBeatRandValue(0), _pveHeartBeatData(nullptr),
    m_spellTriggered(false), m_AuraDRLevel(DIMINISHING_LEVEL_1)
{
    MANGOS_ASSERT(target);
    MANGOS_ASSERT(spellproto && spellproto == sSpellMgr.GetSpellEntry(spellproto->Id) && "`info` must be pointer to a sSpellMgr element");

    if (!caster)
        m_casterGuid = target->GetObjectGuid();
    else
    {
        // remove this assert when not unit casters will be supported
        MANGOS_ASSERT(caster->isType(TYPEMASK_UNIT))
        m_casterGuid = caster->GetObjectGuid();
    }

    if (pRealCaster)
        m_realCasterGuid = pRealCaster->GetObjectGuid();
    else
        m_realCasterGuid = m_casterGuid;

    m_applyTime      = time(nullptr);
    m_isPassive      = IsPassiveSpell(GetId()) || spellproto->Attributes == 0x40; //vanilla+
    m_isDeathPersist = spellproto->IsDeathPersistentSpell();
    m_isSingleTarget = spellproto->HasSingleTargetAura();
    m_procCharges    = spellproto->procCharges;
    m_isChanneled    = spellproto->IsChanneledSpell();

    m_isRemovedOnShapeLost = m_casterGuid == m_target->GetObjectGuid() && m_spellProto->IsRemovedOnShapeLostSpell();

    // Exceptions
    // Attaques circulaires
    if (m_spellProto->Id == 12292)
        m_isRemovedOnShapeLost = false;

    Unit* unitCaster = caster && caster->isType(TYPEMASK_UNIT) ? (Unit*)caster : NULL;

    m_duration = m_maxDuration = spellproto->CalculateDuration(unitCaster);

    if (m_maxDuration == -1 || (m_isPassive && spellproto->DurationIndex == 0))
        m_permanent = true;
    // Fix de l'affichage dans le journal de combat des buffs tres cours.
    // Exemple: immunite des trinket PvP.
    else if (m_maxDuration < 200)
    {
        m_duration = 300;
        m_maxDuration = 300;
    }

    if (unitCaster)
    {
        if (Player* modOwner = unitCaster->GetSpellModOwner())
            modOwner->ApplySpellMod(GetId(), SPELLMOD_CHARGES, m_procCharges);
    }

    // some custom stack values at aura holder create
    switch (m_spellProto->Id)
    {
        // some auras applied with max stack
        case 24575:                                         // Brittle Armor
        case 24659:                                         // Unstable Power
        case 24662:                                         // Restless Strength
        case 26464:                                         // Mercurial Shield
            m_stackAmount = m_spellProto->StackAmount;
            break;
    }

    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        m_auras[i] = nullptr;

    m_makesTargetSecondaryFocus = !GetSpellProto()->IsPositiveSpell() && ((GetSpellProto()->AuraInterruptFlags & AURA_INTERRUPT_FLAG_DAMAGE) || ((m_spellProto->HasAura(SPELL_AURA_MOD_CONFUSE) || m_spellProto->HasAura(SPELL_AURA_MOD_FEAR)) &&
                !(m_spellProto->AttributesEx4 & SPELL_ATTR_EX4_NO_SECONDARY_TARGET_AGGRO)));
}

void SpellAuraHolder::AddAura(Aura *aura, SpellEffectIndex index)
{
    ASSERT(index == aura->GetEffIndex());
    m_auras[index] = aura;
}

void SpellAuraHolder::RemoveAura(SpellEffectIndex index)
{
    m_auras[index] = nullptr;
}

void SpellAuraHolder::ApplyAuraModifiers(bool apply, bool real)
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX && !IsDeleted(); ++i)
        if (Aura *aur = GetAuraByEffectIndex(SpellEffectIndex(i)))
            aur->ApplyModifier(apply, real);
}

void SpellAuraHolder::_AddSpellAuraHolder()
{
    if (!GetId())
        return;
    if (!m_target)
        return;

    // Try find slot for aura
    uint8 slot = NULL_AURA_SLOT;
    Unit* caster = GetCaster();

    // Lookup free slot
    // will be < MAX_AURAS slot (if find free) with !secondaura
    if (IsNeedVisibleSlot(caster))
    {
        if (IsPositive())                                   // empty positive slot
        {
            for (uint8 i = 0; i < MAX_POSITIVE_AURAS; i++)
            {
                if (m_target->GetUInt32Value((uint16)(UNIT_FIELD_AURA + i)) == 0)
                {
                    slot = i;
                    break;
                }
            }
        }
        else                                                // empty negative slot
        {
            for (uint8 i = MAX_POSITIVE_AURAS; i < MAX_AURAS; i++)
            {
                if (m_target->GetUInt32Value((uint16)(UNIT_FIELD_AURA + i)) == 0)
                {
                    slot = i;
                    break;
                }
            }
        }
    }

    // set infinity cooldown state for spells
    if (caster)
    {
        if (m_spellProto->Attributes & SPELL_ATTR_DISABLED_WHILE_ACTIVE)
        {
            Item* castItem = nullptr;
            if (m_castItemGuid && caster->GetTypeId() == TYPEID_PLAYER)
                castItem = ((Player*)caster)->GetItemByGuid(m_castItemGuid);
            caster->AddSpellAndCategoryCooldowns(m_spellProto, castItem ? castItem->GetEntry() : 0, nullptr, true);
        }
    }

    SetAuraSlot(slot);

    // Not update fields for not first spell's aura, all data already in fields
    if (slot < MAX_AURAS)                                   // slot found
    {
        SetAura(slot, false);
        SetAuraFlag(slot, true);
        SetAuraLevel(slot, caster ? caster->getLevel() : sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL));
        UpdateAuraApplication();

        // update for out of range group members
        m_target->UpdateAuraForGroup(slot);

        UpdateAuraDuration();
    }

    //*****************************************************
    // Update target aura state flag (at 1 aura apply)
    // TODO: Make it easer
    //*****************************************************
    // Sitdown on apply aura req seated
    if (m_spellProto->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED && !m_target->IsSittingDown())
        m_target->SetStandState(UNIT_STAND_STATE_SIT);

    // register aura diminishing on apply
    if (getDiminishGroup() != DIMINISHING_NONE)
        m_target->ApplyDiminishingAura(getDiminishGroup(), true);

    // Update Seals information
    if (GetSpellProto()->IsSealSpell())
        m_target->ModifyAuraState(AURA_STATE_JUDGEMENT, true);

    // Break stealth on target
    if (GetSpellProto()->Custom & SPELL_CUSTOM_AURA_APPLY_BREAKS_STEALTH)
        m_target->RemoveSpellsCausingAura(SPELL_AURA_MOD_STEALTH, this);
}

void SpellAuraHolder::_RemoveSpellAuraHolder()
{
    // Remove all triggered by aura spells vs unlimited duration
    // except same aura replace case
    if (m_removeMode != AURA_REMOVE_BY_STACK)
        CleanupTriggeredSpells();

    Unit* caster = GetCaster();

    if (IsPersistent())
    {
        if (WorldObject* realCaster = GetRealCaster())
        {
            DynamicObject *dynObj = realCaster->GetDynObject(GetId());
            if (dynObj)
                dynObj->RemoveAffected(m_target);
        }
    }

    //passive auras do not get put in slots
    // Note: but totem can be not accessible for aura target in time remove (to far for find in grid)
    //if(m_isPassive && !(caster && caster->GetTypeId() == TYPEID_UNIT && ((Creature*)caster)->isTotem()))
    //    return false;

    uint8 slot = GetAuraSlot();

    if (slot < MAX_AURAS)
    {
        if (m_target->GetUInt32Value((uint16)(UNIT_FIELD_AURA + slot)) == 0)
            return;
        SetAura(slot, true);
        SetAuraFlag(slot, false);
        SetAuraLevel(slot, caster ? caster->getLevel() : sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL));
    }

    // unregister aura diminishing (and store last time)
    if (getDiminishGroup() != DIMINISHING_NONE)
        m_target->ApplyDiminishingAura(getDiminishGroup(), false);

    m_procCharges = 0;
    m_stackAmount = 1;
    UpdateAuraApplication();

    if (m_removeMode != AURA_REMOVE_BY_DELETE)
    {
        // update for out of range group members
        if (slot < MAX_AURAS)
            m_target->UpdateAuraForGroup(slot);

        //*****************************************************
        // Update target aura state flag (at last aura remove)
        //*****************************************************
        uint32 removeState = 0;
        uint64 removeFamilyFlag = m_spellProto->SpellFamilyFlags;
        switch (m_spellProto->SpellFamilyName)
        {
            case SPELLFAMILY_PALADIN:
                if (m_spellProto->IsSealSpell())
                    removeState = AURA_STATE_JUDGEMENT;     // Update Seals information
                break;
        }

        // Remove state (but need check other auras for it)
        if (removeState)
        {
            bool found = false;
            Unit::SpellAuraHolderMap const& holders = m_target->GetSpellAuraHolderMap();
            for (Unit::SpellAuraHolderMap::const_iterator i = holders.begin(); i != holders.end(); ++i)
            {
                SpellEntry const *auraSpellInfo = (*i).second->GetSpellProto();
                if (auraSpellInfo->IsFitToFamily(SpellFamily(m_spellProto->SpellFamilyName), removeFamilyFlag))
                {
                    found = true;
                    break;
                }
            }

            // this has been last aura
            if (!found)
                m_target->ModifyAuraState(AuraState(removeState), false);
        }

        // reset cooldown state for spells
        if (caster)
            if (GetSpellProto()->Attributes & SPELL_ATTR_DISABLED_WHILE_ACTIVE)
                // note: item based cooldowns and cooldown spell mods with charges ignored (unknown existing cases)
                caster->CooldownEvent(GetSpellProto());
    }
}

void SpellAuraHolder::CleanupTriggeredSpells()
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        if (!m_spellProto->EffectApplyAuraName[i])
            continue;

        uint32 tSpellId = m_spellProto->EffectTriggerSpell[i];
        if (!tSpellId)
            continue;

        SpellEntry const* tProto = sSpellMgr.GetSpellEntry(tSpellId);
        if (!tProto)
            continue;

        if (tProto->GetDuration() != -1)
            continue;

        // needed for spell 43680, maybe others
        // TODO: is there a spell flag, which can solve this in a more sophisticated way?
        if (m_spellProto->EffectApplyAuraName[i] == SPELL_AURA_PERIODIC_TRIGGER_SPELL &&
                m_spellProto->GetDuration() == int32(m_spellProto->EffectAmplitude[i]))
            continue;

        m_target->RemoveAurasDueToSpell(tSpellId);
    }
}

bool SpellAuraHolder::ModStackAmount(int32 num)
{
    uint32 protoStackAmount = m_spellProto->StackAmount;

    // Can`t mod
    if (!protoStackAmount)
        return true;

    // Modify spell stacks
    if (GetCaster())
    {
        if (Player* modOwner = GetCaster()->GetSpellModOwner())
            modOwner->ApplySpellMod(GetSpellProto()->Id, SPELLMOD_STACKS, protoStackAmount);
    }

    // Modify stack but limit it
    int32 stackAmount = m_stackAmount + num;
    if (stackAmount > (int32)protoStackAmount)
        stackAmount = protoStackAmount;
    else if (stackAmount <= 0) // Last aura from stack removed
    {
        m_stackAmount = 0;
        return true; // need remove aura
    }

    // Update stack amount
    SetStackAmount(stackAmount);
    return false;
}

void SpellAuraHolder::SetStackAmount(uint32 stackAmount)
{
    Unit *target = GetTarget();
    Unit *caster = GetCaster();
    if (!target || !caster)
        return;

    bool refresh = stackAmount >= m_stackAmount;
    if (stackAmount != m_stackAmount)
    {
        m_stackAmount = stackAmount;
        UpdateAuraApplication();

        for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        {
            if (Aura *aur = m_auras[i])
            {
                int32 bp = aur->GetBasePoints();
                int32 amount = m_stackAmount * caster->CalculateSpellDamage(target, m_spellProto, SpellEffectIndex(i), &bp);
                // Reapply if amount change
                if (aur->GetModifier()->m_auraname == SPELL_AURA_PERIODIC_DAMAGE) {
                    aur->ApplyModifier(false, true);
                    aur->GetModifier()->m_amount = amount;
                    aur->ApplyModifier(true, true);
                }
                else if (amount != aur->GetModifier()->m_amount) {
                    aur->ApplyModifier(false, true);
                    aur->GetModifier()->m_amount = amount;
                    aur->ApplyModifier(true, true);
                }
            }
        }
    }

    if (refresh)
        // Stack increased refresh duration
        RefreshHolder();
}

Unit* SpellAuraHolder::GetCaster() const
{
    if (GetCasterGuid() == m_target->GetObjectGuid())
        return m_target;

    return ObjectAccessor::GetUnit(*m_target, GetCasterGuid());// player will search at any maps
}

WorldObject* SpellAuraHolder::GetRealCaster() const
{
    if (GetRealCasterGuid() == GetCasterGuid())
        return GetCaster();

    if (GetRealCasterGuid().IsUnit())
        return ObjectAccessor::GetUnit(*m_target, GetRealCasterGuid());

    if (m_target->FindMap())
        return m_target->FindMap()->GetWorldObject(GetRealCasterGuid());

    return nullptr;
}

bool SpellAuraHolder::IsWeaponBuffCoexistableWith(SpellAuraHolder const* ref) const
{
    // only item casted spells
    if (!GetCastItemGuid())
        return false;

    // Exclude Debuffs
    if (!IsPositive())
        return false;

    // Exclude Non-generic Buffs and Executioner-Enchant
    if (GetSpellProto()->SpellFamilyName != SPELLFAMILY_GENERIC)
        return false;

    // Exclude Stackable Buffs [ie: Blood Reserve]
    if (GetSpellProto()->StackAmount)
        return false;

    // only self applied player buffs
    if (m_target->GetTypeId() != TYPEID_PLAYER || m_target->GetObjectGuid() != GetCasterGuid())
        return false;

    Item* castItem = ((Player*)m_target)->GetItemByGuid(GetCastItemGuid());
    if (!castItem)
        return false;

    // Limit to Weapon-Slots
    if (!castItem->IsEquipped() ||
            (castItem->GetSlot() != EQUIPMENT_SLOT_MAINHAND && castItem->GetSlot() != EQUIPMENT_SLOT_OFFHAND))
        return false;

    // form different weapons
    return ref->GetCastItemGuid() && ref->GetCastItemGuid() != GetCastItemGuid();
}

bool SpellAuraHolder::IsNeedVisibleSlot(Unit const* caster) const
{
    bool totemAura = caster && caster->GetTypeId() == TYPEID_UNIT && ((Creature*)caster)->IsTotem();

    // Check for persistent area auras that only do damage. If it has a secondary effect, it takes
    // up a slot
    bool persistent = m_spellProto->Effect[EFFECT_INDEX_0] == SPELL_EFFECT_PERSISTENT_AREA_AURA;
    bool persistentWithSecondaryEffect = false;

    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        // Check for persistent aura here since the effect aura is applied to the holder
        // by a dynamic object as the target passes through the object field, meaning
        // m_auras will be unset when this method is called (initialization)
        if (!m_auras[i] && !persistent)
            continue;

        // special area auras cases
        switch (m_spellProto->Effect[i])
        {
            case SPELL_EFFECT_APPLY_AREA_AURA_PET:
            case SPELL_EFFECT_APPLY_AREA_AURA_PARTY:
                // passive auras (except totem auras) do not get placed in caster slot
                return (m_target != caster || totemAura || !m_isPassive) && m_auras[i]->GetModifier()->m_auraname != SPELL_AURA_NONE;

                break;
            case SPELL_EFFECT_PERSISTENT_AREA_AURA:
                // If spell aura applies something other than plain damage, it takes
                // up a debuff slot.
                if (m_spellProto->EffectApplyAuraName[i] != SPELL_AURA_PERIODIC_DAMAGE)
                    persistentWithSecondaryEffect = true;

                break;
            default:
                break;
        }
    }

    /*  Persistent area auras such as Blizzard/RoF/Volley do not get require debuff slots
        since they just do area damage with no additional effects. However, spells like
        Hurricane do since they have a secondary effect attached to them. There are enough
        persistent area spells in-game that making a switch for all of them is a bit
        unreasonable. Any spell with a secondary affect should take up a slot. Note
        that most (usable) persistent spells only deal damage.

        It was considered whether spells with secondary effects should still deal damage,
        even if there is no room for the other effect, however the debuff tooltip states
        that the spell causes damage AND slows, therefore it must take a debuff slot.
     */
    if (persistent && !persistentWithSecondaryEffect)
    {
        return false;
    }

    // necessary for some spells, e.g. Immolate visual passive 28330
    if (m_spellProto->SpellVisual)
        return true;

    // passive auras (except totem auras) do not get placed in the slots
    return !m_isPassive || totemAura;
}

void SpellAuraHolder::HandleSpellSpecificBoosts(bool apply)
{
    uint32 spellId1 = 0;
    uint32 spellId2 = 0;
    uint32 spellId3 = 0;
    uint32 spellId4 = 0;

    switch (GetSpellProto()->SpellFamilyName)
    {
        case SPELLFAMILY_SHAMAN:
        {
            // Nostalrius : Pas de marche sur l'eau + loup fantome.
            if (apply && GetSpellProto()->Id == 2645)
                if (Unit* pCaster = GetCaster())
                    if (pCaster->HasAura(546))
                        pCaster->RemoveAurasDueToSpell(546);
                    /*if (Aura* aura = pCaster->GetAura(546, EFFECT_INDEX_0))
                        pCaster->RemoveAura(aura);*/
            break;
        }
        case SPELLFAMILY_MAGE:
        {
            return;
        }
        case SPELLFAMILY_HUNTER:
        {
            switch (GetId())
            {
                // The Beast Within and Bestial Wrath - immunity
                case 19574:
                {
                    spellId1 = 24395;
                    spellId2 = 24396;
                    spellId3 = 24397;
                    spellId4 = 26592;
                    break;
                }
				//vanilla+ Tame Beast - immunity
				case 1515:
				{
					spellId1 = 34038;
					spellId2 = 34039;
					spellId3 = 34040;
					spellId4 = 34041;
					break;
				}
                default:
                    return;
            }
            break;
        }
        default:
            return;
    }

    // prevent aura deletion, specially in multi-boost case
    SetInUse(true);

    if (apply)
    {
        if (spellId1)
            m_target->CastSpell(m_target, spellId1, true, nullptr, nullptr, GetCasterGuid());
        if (spellId2 && !IsDeleted())
            m_target->CastSpell(m_target, spellId2, true, nullptr, nullptr, GetCasterGuid());
        if (spellId3 && !IsDeleted())
            m_target->CastSpell(m_target, spellId3, true, nullptr, nullptr, GetCasterGuid());
        if (spellId4 && !IsDeleted())
            m_target->CastSpell(m_target, spellId4, true, nullptr, nullptr, GetCasterGuid());
    }
    else
    {
        if (spellId1)
            m_target->RemoveAurasByCasterSpell(spellId1, GetCasterGuid());
        if (spellId2)
            m_target->RemoveAurasByCasterSpell(spellId2, GetCasterGuid());
        if (spellId3)
            m_target->RemoveAurasByCasterSpell(spellId3, GetCasterGuid());
        if (spellId4)
            m_target->RemoveAurasByCasterSpell(spellId4, GetCasterGuid());
    }

    SetInUse(false);
}

void SpellAuraHolder::HandleCastOnAuraRemoval() const
{
    uint32 uiTriggeredSpell = 0;
    AuraRemoveMode mode = GetRemoveMode();

    switch (GetId())
    {
        case 26180:
        {
            if (mode == AURA_REMOVE_BY_DISPEL)
                uiTriggeredSpell = 26233;        // Wyvern Sting (AQ40, Princess Huhuran)
            break;
        }
		case 19365:
		{
			if (mode == AURA_REMOVE_BY_DISPEL)
				uiTriggeredSpell = 34449;        // MC Ancient Dread(Core hound)
			break;
		}
		case 19366:
		{
			if (mode == AURA_REMOVE_BY_DISPEL)
				uiTriggeredSpell = 34450;       //aoe on dispell  
			break;
		}
		case 19367:
		{
			if (mode == AURA_REMOVE_BY_DISPEL)
				uiTriggeredSpell = 34451;  //Stun+dmg
			break;
		}
		case 19372:
		{
			if (mode == AURA_REMOVE_BY_DISPEL)
				uiTriggeredSpell = 34452; //root+silence
			break;
		}
		case 19659:
		{
			if (mode == AURA_REMOVE_BY_DISPEL)
				uiTriggeredSpell = 34473; //Baron Mana Burn Silence proc
			break;
		}
        case 34957: //Curse of Nemesis ZG Hakkar
        {
            if (mode == AURA_REMOVE_BY_DISPEL)
                uiTriggeredSpell = 34958; //damage
            break;
        }
        case 24002:
        case 24003:
        case 35843:
        {
            if (mode == AURA_REMOVE_BY_EXPIRE)
                uiTriggeredSpell = 24004;        // Tranquilizing Poison (ZG, Razzashi Serpent, SM Loksey)
            break;
        }
        case 35260:
        {
            if (mode == AURA_REMOVE_BY_CANCEL)
                uiTriggeredSpell = 35199;        //Cancel possess
            break;
        }
		//case 16914:
		//case 17401:
		//case 17402:
		//{
		//	if (mode == AURA_REMOVE_BY_RANGE)
		//		uiTriggeredSpell = 34255; //V+ druid hurricane
		//	break;
		//}
        default:
            return;
    }

    if (uiTriggeredSpell)
    /*{
        if (Unit* caster = GetCaster())
            caster->CastSpell(GetTarget(), uiTriggeredSpell, true);
        else
            GetTarget()->CastSpell(GetTarget(), uiTriggeredSpell, true); 
    }*/
    {
        /*if (Unit* caster = GetCaster())
            if (caster->isAlive())
                caster->CastSpell(GetTarget(), uiTriggeredSpell, true);
            else
                m_target->CastSpell(m_target, uiTriggeredSpell, true);
        else
            GetTarget()->CastSpell(GetTarget(), uiTriggeredSpell, true);*/


        if (uiTriggeredSpell == 26180)
        {
            if (Unit* caster = GetCaster())
                if (caster->isAlive())
                    caster->CastSpell(GetTarget(), uiTriggeredSpell, true);
                else
                    m_target->CastSpell(m_target, uiTriggeredSpell, true);
            else
                GetTarget()->CastSpell(GetTarget(), uiTriggeredSpell, true);
        }
        else
        {
            m_target->CastSpell(GetTarget(), uiTriggeredSpell, true);
        }

    }
}

void Aura::HandleAuraSafeFall(bool apply, bool Real)
{
    // implemented in WorldSession::HandleMovementOpcodes
}

SpellAuraHolder::~SpellAuraHolder()
{
    // note: auras in delete list won't be affected since they clear themselves from holder when adding to deletedAuraslist
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        delete m_auras[i];

    delete _pveHeartBeatData;
}

void SpellAuraHolder::Update(uint32 diff)
{
    // Battements de coeur : 2 fonctionnements.
    // PvP
    if (_heartBeatRandValue)
    {
        float dynamicAVGduration = 12.0f;
        float dynamicMAXduration = 15.0f;
        switch (m_spellProto->Id)
        {
            case 2094: //Blind
            dynamicAVGduration = 10.0f;
            dynamicMAXduration = 10.5f;
            if (Unit* caster = GetCaster())
                if (caster->HasAura(33699)) //Total Control rank2
                    dynamicAVGduration = dynamicAVGduration * 1.2f,
                    dynamicMAXduration = dynamicMAXduration * 1.2f;
                else if (caster->HasAura(33698)) //Total Control rank1
                    dynamicAVGduration = dynamicAVGduration * 1.1f,
                    dynamicMAXduration = dynamicMAXduration * 1.1f;
            break;
            case 6770:  //Sap 1
            case 2070:  //Sap 2
            case 11297: //Sap 3
                if (Unit* caster = GetCaster())
                    if (caster->HasAura(33699)) //Total Control rank2
                        dynamicAVGduration = dynamicAVGduration * 1.2f,
                        dynamicMAXduration = dynamicMAXduration * 1.2f;
                    else if (caster->HasAura(33698)) //Total Control rank1
                        dynamicAVGduration = dynamicAVGduration * 1.1f,
                        dynamicMAXduration = dynamicMAXduration * 1.1f;
            break;
            case 20066: //Repentance
                dynamicAVGduration = 6.0f;
                dynamicMAXduration = 7.0f;
            break;
            case 3355:
            case 14308:
            case 14309: //Freezing Trap + Clever Traps
                if (Unit* caster = GetCaster())
                    if (caster->HasAura(19245)) //rank2
                        dynamicAVGduration = dynamicAVGduration * 1.3f,
                        dynamicMAXduration = dynamicMAXduration * 1.3f;
                    else if (caster->HasAura(19239)) //rank1
                        dynamicAVGduration = dynamicAVGduration * 1.15f,
                        dynamicMAXduration = dynamicMAXduration * 1.15f;
            break;
        }

        float diminishRate = GetDiminishingRate(m_AuraDRLevel);
        float elapsedTime = (m_maxDuration - m_duration) / 1000.0f;
        float averageBreakTime = dynamicAVGduration * diminishRate; // 50% chance to break after 12 secs
        float maxBreakTime = dynamicMAXduration * diminishRate;
        static const float chanceBreakAtMax = 1.0f; // 1% break > 15 secs. Patch 1.2: "Players now have an increasing chance to break free of the effect, such that it is unlikely the effect will last more than 15 seconds."
        static const float chanceBreakAtMaxLog = log((100 - chanceBreakAtMax) / chanceBreakAtMax);
        float coeff = (1.0f / (maxBreakTime - averageBreakTime)) * chanceBreakAtMaxLog;
        float currHeartBeatValue = 100.0f / (1.0f + exp(coeff * (averageBreakTime - elapsedTime)));
        DEBUG_UNIT(GetTarget(), DEBUG_DR, "|HB Duration [Curr%.2f|Max%u]. Value[Curr%.2f|Limit%.2f]",
                           elapsedTime, m_maxDuration / 1000, currHeartBeatValue, _heartBeatRandValue);
        if (_heartBeatRandValue <=  currHeartBeatValue)
        {
            if (Unit* pTarget = GetTarget())
                pTarget->RemoveSpellAuraHolder(this);
            return;
        }
    }
    // PvE
    if (_pveHeartBeatData)
    {
        if (_pveHeartBeatData->timer <= diff)
        {
            _pveHeartBeatData->timer += 5000 - diff;
            if (Unit* pTarget = GetTarget())
            {
                uint32 missChance = 10000 - _pveHeartBeatData->hitChance;
                uint32 rand = urand(0, 10000);

                if (rand < missChance)
                {
                    delete _pveHeartBeatData;
                    _pveHeartBeatData = nullptr;
                    pTarget->RemoveSpellAuraHolder(this);
                    return;
                }
            }
        }
        else
            _pveHeartBeatData->timer -= diff;
    }

    if (m_duration > 0)
    {
        m_duration -= diff;
        if (m_duration < 0)
            m_duration = 0;

        m_timeCla -= diff;

        if (m_timeCla <= 0)
        {
            if (Unit* caster = GetCaster())
            {
                Powers powertype = Powers(m_spellProto->powerType);
                int32 manaPerSecond = m_spellProto->manaPerSecond + m_spellProto->manaPerSecondPerLevel * caster->getLevel();
                m_timeCla = 1 * IN_MILLISECONDS;

                Unit* target = GetTarget();
                if (manaPerSecond && // avoid double cost for health funnel :
                    !(m_spellProto->SpellFamilyName == SPELLFAMILY_WARLOCK &&
                    m_spellProto->IsFitToFamilyMask<CF_WARLOCK_HEALTH_FUNNEL>() &&
                    target && (target->GetCharmerOrOwnerGuid() == GetCasterGuid())))
                {
                    if (powertype == POWER_HEALTH && int32(caster->GetHealth()) > manaPerSecond)
                        caster->ModifyHealth(-manaPerSecond);
                    else if (int32(caster->GetPower(powertype)) >= manaPerSecond)
                        caster->ModifyPower(powertype, -manaPerSecond);
                    else
                    {
                        if (target)
                            target->RemoveAurasDueToSpell(m_spellProto->Id, this);
                        if (m_isChanneled)
                            caster->InterruptSpell(CURRENT_CHANNELED_SPELL);
                        if (Player* plCaster = caster->ToPlayer())
                            Spell::SendCastResult(plCaster, m_spellProto, SPELL_FAILED_FIZZLE);
                    }
                }
            }
        }
    }

    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (Aura *aura = m_auras[i])
            aura->UpdateAura(diff);

    // Channeled aura required check distance from caster
    if (m_isChanneled && GetCasterGuid() != m_target->GetObjectGuid())
    {
        Unit* caster = GetCaster();
        if (!caster)
        {
            m_target->RemoveAurasByCasterSpell(GetId(), GetCasterGuid());
            return;
        }

        // Nostalrius : mise en combat
        if (!(GetSpellProto()->AttributesEx3 & SPELL_ATTR_EX3_NO_INITIAL_AGGRO) && !IsPositive() &&
                caster->isVisibleForOrDetect(m_target, m_target, false))
        {
            m_target->SetInCombatWith(caster);
            caster->SetInCombatWith(m_target);
        }
        // unlimited range
        if (m_spellProto->Custom & SPELL_CUSTOM_CHAN_NO_DIST_LIMIT)
            return;

        // need check distance for channeled target only
        if (caster->GetChannelObjectGuid() == m_target->GetObjectGuid())
        {
            // spell range + guessed interrupt leeway range
            float max_range = GetSpellMaxRange(sSpellRangeStore.LookupEntry(m_spellProto->rangeIndex));

            if (m_target->IsHostileTo(caster))
                max_range *= 1.33f;
            else // add radius of caster (see Spell::CheckRange)
                max_range += 1.25f;

            if (Player* modOwner = caster->GetSpellModOwner())
                modOwner->ApplySpellMod(GetId(), SPELLMOD_RANGE, max_range, nullptr);

            if (caster->GetCombatDistance(m_target) > max_range)
                caster->InterruptSpell(CURRENT_CHANNELED_SPELL);
        }
    }
}

void SpellAuraHolder::RefreshHolder()
{
    SetAuraDuration(GetAuraMaxDuration());
    UpdateAuraDuration();
}

/**
 * Updates periodic timers based on the current duration. Used for channeled spells
 * which are applied using dynamic objects, when we want the tick to be based on
 * the current spell timer rather than a fresh holder.
 * @param duration Custom duration to base tick on (typically in the case of passive auras)
 *
 */
void SpellAuraHolder::RefreshAuraPeriodicTimers(uint32 duration)
{
    for (int i = 0 ; i < MAX_EFFECT_INDEX; ++i)
    {
        if (Aura* pAura = GetAuraByEffectIndex(SpellEffectIndex(i)))
        {
            // If the aura is periodic, update the periodic timer to correspond with the new
            // aura timer
            if (pAura->IsPeriodic())
                pAura->UpdatePeriodicTimer(duration ? duration : m_duration);
        }
    }
}

void SpellAuraHolder::SetAuraMaxDuration(int32 duration)
{
    m_maxDuration = duration;

    // possible overwrite persistent state
    if (duration > 0)
    {
        if (!(IsPassive() && GetSpellProto()->DurationIndex == 0))
            SetPermanent(false);
    }
}

uint32 SpellAuraHolder::GetAuraPeriodicTickTimer(SpellEffectIndex index) const
{
    Aura *aur = m_auras[index];
    if (!aur)
        return -1;

    return aur->GetAuraPeriodicTimer();
}

bool SpellAuraHolder::HasMechanic(uint32 mechanic) const
{
    if (mechanic == m_spellProto->Mechanic)
        return true;

    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (m_auras[i] && m_spellProto->EffectMechanic[i] == mechanic)
            return true;
    return false;
}

bool SpellAuraHolder::HasMechanicMask(uint32 mechanicMask) const
{
    if (m_spellProto->Mechanic && mechanicMask & (1 << (m_spellProto->Mechanic - 1)))
        return true;

    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (m_auras[i] && m_spellProto->EffectMechanic[i] && ((1 << (m_spellProto->EffectMechanic[i] - 1)) & mechanicMask))
            return true;
    return false;
}

bool SpellAuraHolder::IsDeathSavedSpell(Unit* pUnit) const
{
    if (!pUnit)
        return false;

    uint8 saveChance = 0;
    if (pUnit->HasAura(16184) || pUnit->HasAura(16209))
    {
        if (!pUnit->HasSpellCategoryCooldown(1161))
        {
            pUnit->RemoveAllNegativeAuras();
            saveChance = 100;
        }
    }

    return urand(0, 99) < saveChance;
}

bool SpellAuraHolder::IsPersistent() const
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (Aura *aur = m_auras[i])
            if (aur->IsPersistent())
                return true;
    return false;
}

bool SpellAuraHolder::IsAreaAura() const
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (Aura *aur = m_auras[i])
            if (aur->IsAreaAura())
                return true;
    return false;
}

bool SpellAuraHolder::IsPositive() const
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (Aura *aur = m_auras[i])
            if (!aur->IsPositive())
                return false;
    return true;
}

bool SpellAuraHolder::IsEmptyHolder() const
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (m_auras[i])
            return false;
    return true;
}

void SpellAuraHolder::UnregisterSingleCastHolder()
{
    if (IsSingleTarget())
    {
        if (Unit* caster = GetCaster())
            caster->GetSingleCastSpellTargets().erase(GetSpellProto());

        m_isSingleTarget = false;
    }
}

void SpellAuraHolder::SetAuraFlag(uint32 slot, bool add)
{
    uint32 index    = slot >> 3;
    uint32 byte     = (slot & 7) << 2;
    uint32 val      = m_target->GetUInt32Value(UNIT_FIELD_AURAFLAGS + index);
    if (add)
        val |= ((uint32)AFLAG_MASK << byte);
    else
        val &= ~((uint32)AFLAG_MASK << byte);

    m_target->SetUInt32Value(UNIT_FIELD_AURAFLAGS + index, val);
}

void SpellAuraHolder::SetAuraLevel(uint32 slot, uint32 level)
{
    uint32 index    = slot / 4;
    uint32 byte     = (slot % 4) * 8;
    uint32 val      = m_target->GetUInt32Value(UNIT_FIELD_AURALEVELS + index);
    val &= ~(0xFF << byte);
    val |= (level << byte);
    m_target->SetUInt32Value(UNIT_FIELD_AURALEVELS + index, val);
}

void SpellAuraHolder::UpdateAuraApplication()
{
    if (m_auraSlot >= MAX_AURAS)
        return;

    uint32 stackCount = m_procCharges > 0 ? m_procCharges * m_stackAmount : m_stackAmount;

    uint32 index    = m_auraSlot / 4;
    uint32 byte     = (m_auraSlot % 4) * 8;
    uint32 val      = m_target->GetUInt32Value(UNIT_FIELD_AURAAPPLICATIONS + index);
    val &= ~(0xFF << byte);
    // field expect count-1 for proper amount show, also prevent overflow at client side
    val |= ((uint8(stackCount <= 255 ? stackCount - 1 : 255 - 1)) << byte);
    m_target->SetUInt32Value(UNIT_FIELD_AURAAPPLICATIONS + index, val);
}

void SpellAuraHolder::UpdateAuraDuration() const
{
    if (GetAuraSlot() >= MAX_AURAS || m_isPassive)
        return;

    if (m_target->GetTypeId() == TYPEID_PLAYER)
    {
        WorldPacket data(SMSG_UPDATE_AURA_DURATION, 5);
        data << uint8(GetAuraSlot());
        data << uint32(GetAuraDuration());
        ((Player*)m_target)->SendDirectMessage(&data);
    }
}

void SpellAuraHolder::SetAffectedByDebuffLimit(bool isAffectedByDebuffLimit)
{
    m_debuffLimitAffected = isAffectedByDebuffLimit;
}

/** NOSTALRIUS
 Debuff limitation
    Categories:
     * 4 (highest)  Highest Priority Debuffs (Generally, a debuff that alters the ability of a mob to act normally)
     * 3            Generally, a standalone debuff ability intentionally casted that gives benefits group/raid wide
     * 2            Generally, a debuff that is a secondary result of some other ability but still gives raid-wide benefits
     * 1            Generally, debuffs that are intentionally cast, stand-alone damage-over-time abilities
     * 0            Generally, a debuff that is a secondarily applied damage-over-time effect
  */
void SpellAuraHolder::CalculateForDebuffLimit()
{
    m_debuffLimitAffected = true;
    m_debuffLimitScore = 0;

    // First, some exceptions
    switch (m_spellProto->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            switch (m_spellProto->Id)
            {
                // Hakkar's Blood Siphon
                case 24323:
                case 24322:
                case 28732: // Faerlina Widow's Embrace
                case 25181: // Ossirian Arcane Weakness
                case 25177: // Ossirian Fire Weakness
                case 25178: // Ossirian Frost Weakness
                case 25180: // Ossirian Nature Weakness
                case 25183: // Ossirian Shadow Weakness
                    m_debuffLimitScore = 4;
                    return;
            }
        }
        case SPELLFAMILY_PALADIN:
        {
            switch (m_spellProto->Id)
            {
                // Judgement of Wisdom (3 ranks)
                case 20186:
                case 20354:
                case 20355:
                // Judgement of Light
                case 20185: // r1
                case 20344: // r2
                case 20345: // r3
                case 20346: // r4
                    /*m_debuffLimitScore = 3;
                    return;*/
                // Judgement of the Crusader (6 ranks)
                case 21183:
                case 20188:
                case 20300:
                case 20301:
                case 20302:
                case 20303:
                    m_debuffLimitScore = 3;
                    return;
            }
        }
        case SPELLFAMILY_MAGE:
        {
            switch (m_spellProto->Id)
            {
                // Ignite talent trigger
                case 12654:
                    m_debuffLimitScore = 3;
                    return;
            }
        }
		case SPELLFAMILY_ROGUE:
		{
			switch (m_spellProto->Id)
			{
				//v+ Losing an Advantage
			case 33708:
				m_debuffLimitScore = 0;
				return;
			}
		}
    }

    // Fireball
    if (m_spellProto->IsFitToFamily<SPELLFAMILY_MAGE, CF_MAGE_FIREBALL, CF_MAGE_CONJURE>())
    {
        m_debuffLimitScore = 0;
        return;
    }

    // Gestion des priorites par type d'aura
    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        if (!m_spellProto->EffectApplyAuraName[i])
            continue;

        uint32 currEffectScore = 0;
        switch (m_spellProto->EffectApplyAuraName[i])
        {
            case SPELL_AURA_MOD_TAUNT:
            case SPELL_AURA_MOD_THREAT:
            case SPELL_AURA_MOD_CHARM:
            case SPELL_AURA_MOD_FEAR:
            case SPELL_AURA_MOD_CONFUSE:
            case SPELL_AURA_MOD_POSSESS:
            case SPELL_AURA_MOD_STUN:
                currEffectScore = 4;
                break;
            case SPELL_AURA_MOD_RESISTANCE:
            case SPELL_AURA_MOD_LANGUAGE:                   // Language curse
            case SPELL_AURA_MOD_STALKED:                    // Hunter mark
            case SPELL_AURA_MOD_DISARM:
            case SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN:
            case SPELL_AURA_PREVENTS_FLEEING:               // Curse of recklessness
            case SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_CHANCE: // Winter's Chill
            case SPELL_AURA_MOD_MELEE_HASTE:
            case SPELL_AURA_MOD_ATTACK_POWER:
            case SPELL_AURA_MOD_DAMAGE_DONE:
            case SPELL_AURA_MOD_DAMAGE_TAKEN:
            case SPELL_AURA_MOD_HEALING_PCT:                // Mortal Strike
                currEffectScore = 3;
                break;
            case SPELL_AURA_PERIODIC_DAMAGE:
            case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
            case SPELL_AURA_PERIODIC_LEECH:
                currEffectScore = 1;
                break;
			case SPELL_AURA_MELEE_ATTACK_POWER_ATTACKER_BONUS: //v+
            case SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS: // Expose Weakness
                m_debuffLimitScore = 3;
                return;
            case SPELL_AURA_DUMMY:
                if (m_spellProto->SpellVisual == 3582 && m_spellProto->SpellIconID == 150) // Vampiric Embrace
                {
                    m_debuffLimitScore = 3;
                    return;
                }
                break;
        }
        if (currEffectScore > m_debuffLimitScore)
            m_debuffLimitScore = currEffectScore;
    }

    if (IsTriggered())
    {
        if (m_debuffLimitScore > 2)
            m_debuffLimitScore = 2;
        else
            m_debuffLimitScore = 0;
    }
    else if (m_isChanneled && m_debuffLimitScore < 1)
        m_debuffLimitScore = 1;
}

// Fix premiers tics
void Aura::CalculatePeriodic(Player * modOwner, bool create)
{
    //m_modifier.periodictime = GetSpellProto()->EffectAmplitude[m_effIndex];

    // prepare periodics
    switch (GetSpellProto()->EffectApplyAuraName[m_effIndex])
    {
        case SPELL_AURA_PERIODIC_DAMAGE:
        case SPELL_AURA_PERIODIC_HEAL:
        case SPELL_AURA_PERIODIC_ENERGIZE:
        case SPELL_AURA_OBS_MOD_HEALTH:
        case SPELL_AURA_PERIODIC_LEECH:
        case SPELL_AURA_PERIODIC_HEALTH_FUNNEL:
        case SPELL_AURA_PERIODIC_MANA_LEECH:
        case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
        case SPELL_AURA_POWER_BURN_MANA:
        case SPELL_AURA_PERIODIC_TRIGGER_SPELL:
            m_isPeriodic = true;
            break;
        default:
            break;
    }

    if (!m_isPeriodic)
        return;

    // Apply casting time mods
    if (m_modifier.periodictime && modOwner)
    {
        // Apply periodic time mod
        modOwner->ApplySpellMod(GetId(), SPELLMOD_ACTIVATION_TIME, m_modifier.periodictime);
    }

    // Totem griffes de pierre
    if (GetSpellProto()->SpellVisual == 0 && GetSpellProto()->SpellIconID == 689)
        return;

    switch (GetId())
    {
        case 36239:     // Scarlet Banner visual
            m_periodicTimer = 500;
            break;
        case 36211:     // Shooting Abbendis
            m_periodicTimer = urand(0, 2000);
            break;
        case 36173:     // Drinking
        case 36174:     // Chatting
            m_periodicTimer = urand(0, 4000);
            break;
        case 36166:     // Lovestruck
            m_periodicTimer = urand(3000, 8000);
            break;
        case 36030:     // Panic Aura
            if (modOwner)
            {
                m_periodicTimer = int32(std::max(500.0f, 20.0f * modOwner->GetStat(STAT_SPIRIT)));
                m_modifier.periodictime = m_periodicTimer;
            }
            break;
        case 35435:     // Arena Stealth penalty in 60sec
            m_periodicTimer = 60000;
            break;
        case 34761:
        case 34763:
        case 34765:
        case 34767:
            m_periodicTimer = m_modifier.periodictime + urand(0, 600000);
            break;
        case 35328:
        case 35329:
            m_periodicTimer = m_modifier.periodictime + urand(1, 4000);
            break;

        case 5728:  //Stoneclaw Totem Passive (Rank 1)
        case 6397:  //Stoneclaw Totem Passive (Rank 2)
        case 6398:  //Stoneclaw Totem Passive (Rank 3)
        case 6399:  //Stoneclaw Totem Passive (Rank 4)
        case 10425: //Stoneclaw Totem Passive (Rank 5)
        case 10426: //Stoneclaw Totem Passive (Rank 6)
        case 8145:  // Tremor Tortem Passive
        //case 6474:  // Earthbind Totem Passive
        case 8179:  // Grounding Totem Passive
        //case 8172:  // Disease Cleansing Totem Passive
        //case 8167:  // Poison Cleansing Totem Passive
        case 8515:  // Windfury Totem Passive (Rank 1)
        case 10609: // Windfury Totem Passive (Rank 2)
        case 10612: // Windfury Totem Passive (Rank 3)
        case 8229:  // Flametongue Totem Passive (Rank 1)
        case 8251:  // Flametongue Totem Passive (Rank 2)
        case 10524: // Flametongue Totem Passive (Rank 3)
        case 16388: // Flametongue Totem Passive (Rank 4)
        case 13797: // Immolation Trap Effect (Rank 1)
        case 14298: // Immolation Trap Effect (Rank 2)
        case 14299: // Immolation Trap Effect (Rank 3)
        case 14300: // Immolation Trap Effect (Rank 4)
        case 14301: // Immolation Trap Effect (Rank 5)
        case 23184: // Mark of Frost
        case 25041: // Mark of Nature
        case 34441: // Mark of Desert
        case 35754: // Holy Link
        case 35879: // Poof visual
        case 35881: // The Price of Silence
        case 35924: // Herod's Holy Link Visual
        case 35977: // Doan's Ice Blast
            break;
        default:
            m_periodicTimer = m_modifier.periodictime;
    }

    if (create && IsChanneled())
    {
        if (Unit* caster = GetCaster())
        {
            if (Spell* spell = caster->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
            {
                if (spell->m_spellInfo->Id == GetId())
                {
                    uint32 reduction = spell->GetSpellQueueLatencyReduction();

                    if (reduction && m_periodicTimer > 0)
                    {
                        if (reduction >= uint32(m_periodicTimer))
                            m_periodicTimer = 1;
                        else
                            m_periodicTimer -= int32(reduction);
                    }
                }
            }
        }
    }
}

// Battements de coeur (chance de briser les CC)
void SpellAuraHolder::CalculateHeartBeat(Unit* caster, Unit* target)
{
    if (_pveHeartBeatData)
    {
        delete _pveHeartBeatData;
        _pveHeartBeatData = nullptr;
    }

    _heartBeatRandValue = 0;

    // Ni les sorts permanents, ni les sorts positifs ne sont affectes.
    // Check si positif fait dans Aura::Aura car ici le dernier Aura ajoute n'est pas encore dans 'm_auras'
    if (!m_permanent && m_maxDuration > 10000)
    {
        if (m_spellProto->Attributes & SPELL_ATTR_DIMINISHING_RETURNS
                // Exception pour la coiffe de controle gnome/Heaume-fusee gobelin
                || m_spellProto->Id == 13181 || m_spellProto->Id == 13327)
        {
            // Pour les joueurs
            if (target->GetCharmerOrOwnerPlayerOrPlayerItself())
                _heartBeatRandValue = rand_norm_f() * 100.0f;
        }
        // En PvE. Ne concerne pas certains sorts avec DR (fear geres avec dmg par exemple).
        if (caster && target->GetTypeId() == TYPEID_UNIT && m_spellProto->IsPvEHeartBeat())
        {
            _pveHeartBeatData = new HeartBeatData;
            _pveHeartBeatData->timer = 5000;
            _pveHeartBeatData->hitChance = caster->MagicSpellHitChance(target, m_spellProto);
        }
    }
}

void Aura::HandleInterruptRegen(bool apply, bool real)
{
    if (!real)
        return;
    if (!apply)
        return;
    GetTarget()->SetInCombatState(false);
}

// Un nouvel aura ...
void Aura::HandleAuraAuraSpell(bool apply, bool real)
{
    Unit* target = GetTarget();
    if (!real || !target)
        return;
    uint32 spell = GetSpellProto()->EffectTriggerSpell[m_effIndex];
    if (apply)
        target->AddAura(spell, ADD_AURA_PASSIVE, target);
    else
        target->RemoveAurasDueToSpell(spell);
}


// Auras exclusifs
/*
Sorts d'exemple:
11364 (+50), 21849 (+15)
*/

bool _IsExclusiveSpellAura(SpellEntry const* spellproto, SpellEffectIndex eff, AuraType auraname)
{
    // Flametongue Totem / Totem of Wrath / Strength of Earth Totem / Fel Intelligence / Leader of the Pack
    // Moonkin Aura / Mana Spring Totem / Tree of Life Aura / Improved Devotion Aura / Improved Icy Talons / Trueshot Aura
    // Improved Moonkin Form / Sanctified Retribution Aura / Blood Pact
    if (spellproto->Effect[eff] == SPELL_EFFECT_APPLY_AREA_AURA_RAID)
        return false;

    // Exceptions - se stack avec tout.
    switch (spellproto->Id)
    {
        // Terres foudroyees et Zanza
        case 10693:
        case 10691: // +25 esprit
        case 10668:
        case 10671: // +25 endu
        case 10667:
        case 10670: // +25 force
        case 10669:
        case 10672: // +25 agi
        case 10692:
        case 10690: // +25 intel
        case 24382:             // Buff zanza
        // Alcools
        case 25804:
        case 20875:
        case 25722:
        case 25037:
        case 22789:
        case 22790:
        case 6114:
        case 5020:
        case 5021:
        case 23179: //le proc de l'�p�e de Razorgore (+300 force) devrait se cumuler avec TOUT : ID 23179
        case 8348:
        case 20007: //le proc Crois� devrait se cumuler avec TOUT : ID 20007
        case 20572: //Le racial Orc (ID 20572)
        case 17038: //l'Eau des Tombe-Hiver (ID 17038)
        case 16329: //le Juju's Might (ID 16329)
        case 25891: //le buff du Bijou Choc de Terre (ID : 25891)
        case 18264: // Charge du maitre (baton epique scholo)
        case 12022: // Chapeau pirate
        case 22817: // PA HT Nord
        // Aura de precision (hunt)
        case 19506:
        case 20905:
        case 20906:
        case 18262: // Pierre � Aiguiser El�mentaire (+2% crit)
        case 24932: // chef de la Meute
        case 24907: // aura S�l�nien
        case 22888: // Buff Onyxia
        case 15366: // Buff Felwood
        case 22820: // HT +3% crit sorts
        case 17628: // Supreme Power
        case 22730: // Bouffe +10 Intell
        case 18141: // Bouffe +10 Esprit
        case 18125: // Bouffe +10 Force
        case 18192: // Bouffe +10 Agility
        case 18191: // Bouffe +10 Endu
        case 25661: // Bouffe +25 Endu
        case 24427: // Diamond Flask
        case 17528: // Mighty Rage Potion
        case 23697: // Alterac Spring Water
        // Love is in the Air buffs
        case 27664:
        case 27665:
        case 27666:
        case 27669:
        case 27670:
        case 27671:
        case 34129: //Prowess
        case 34186: //Spirit Bond
        case 17626: //Flask of the Titans
        case 17627: //Flask of Distilled Wisdom
        case 17629: //Flask of Chromatic Resistance
        case 34323: //Flask of Indomitable Might
        case 17535: //Elyxir of Sages
        case 34360: //Orc Racial new
        case 17537: //Brute Force
        case 11406: //Demonslaying
        case 16327: //Juju Guile
        case 15231: //Crystal Force
        case 20587: //Ragged John's Neverending Cup
        case 34374: //King Mosh Heart
        case 6150:  //Quick Shots proc
        case 3045:  //Rapid Fire
        case 34090: //Rapid Strikes
        case 21927: //Gianstalker's pet +40 stamina
        case 21921: //Felheart's pet +stamina/res
        case 8118:  //Scrolls
        case 8119:
        case 8120:
        case 12179:
        case 8099:
        case 8100:
        case 8101:
        case 12178:
        case 8112:
        case 8113:
        case 8114:
        case 12177:
        case 8091:
        case 8094:
        case 8095:
        case 12175:
        case 8096:
        case 8097:
        case 8098:
        case 12176:
        case 8115:
        case 8116:
        case 8117:
        case 12174: //Scrolls
            return false;

        case 17538: // Le +crit du buff de l'Elixir de la Mangouste 17538, devrait se stack avec TOUT.
            return (eff == EFFECT_INDEX_0);
    }
    switch (spellproto->SpellFamilyName)
    {
        case SPELLFAMILY_WARLOCK:
            // Blood Pact
            if (spellproto->IsFitToFamilyMask<CF_WARLOCK_IMP_BUFFS>())
                return false;
            break;
        case SPELLFAMILY_SHAMAN:
            // Strength of Earth (ID 8076, 8162, 8163, 10441, 25362)
            if (spellproto->IsFitToFamilyMask<CF_SHAMAN_STRENGTH_OF_EARTH, CF_SHAMAN_GRACE_OF_AIR, CF_SHAMAN_STONESKIN_WINDWALL>())
                return false;
            break;
        case SPELLFAMILY_WARRIOR:
            // Battle Shout (ID 6673, 5242, 6192, 11549, 11550, 11551, 25289)
            if (spellproto->IsFitToFamilyMask<CF_WARRIOR_BATTLE_SHOUT>())
                return false;
            break;
        case SPELLFAMILY_PALADIN:
            // Blessing of Might (ID 19740, 19834, 19835, 19836, 19837, 19838, 25291, 25782, 25916)
            if (spellproto->IsFitToFamilyMask<CF_PALADIN_BLESSING_OF_MIGHT, CF_PALADIN_BLESSINGS>())
                return false;
            break;
        case SPELLFAMILY_HUNTER:
            // Aspect of the Hawk
            if (spellproto->IsFitToFamilyMask<CF_HUNTER_ASPECT_OF_THE_HAWK>())
                return false;
            break;
    }

    // La bouffe
    if (spellproto->AttributesEx2 & SPELL_ATTR_EX2_FOOD_BUFF)
        return false;

    switch (auraname)
    {
        //case SPELL_AURA_PERIODIC_DAMAGE:
        //case SPELL_AURA_DUMMY:
        //    return false;
        case SPELL_AURA_MOD_HEALING_DONE:                               // Demonic Pact
        case SPELL_AURA_MOD_DAMAGE_DONE:                                // Demonic Pact
        case SPELL_AURA_MOD_ATTACK_POWER_PCT:                           // Abomination's Might / Unleashed Rage
        case SPELL_AURA_MOD_RANGED_ATTACK_POWER_PCT:
        case SPELL_AURA_MOD_ATTACK_POWER:                               // (Greater) Blessing of Might / Battle Shout
        case SPELL_AURA_MOD_RANGED_ATTACK_POWER:
        case SPELL_AURA_MOD_POWER_REGEN:                                // (Greater) Blessing of Wisdom
        case SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN:                       // Glyph of Salvation / Pain Suppression / Safeguard ?
        case SPELL_AURA_MOD_STAT:
            return true;
        case SPELL_AURA_MOD_SPELL_CRIT_CHANCE:
            return true;
        case SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_CHANCE:                 // Winter's Chill / Improved Scorch
            if (spellproto->SpellFamilyName == SPELLFAMILY_MAGE)
                return false;
            return true;
        case SPELL_AURA_MOD_RESISTANCE_EXCLUSIVE: // A gere autrement (exlusif par rapport a la resistance)
            return false;
        case SPELL_AURA_MOD_HEALING_PCT:                                // Mortal Strike / Wound Poison / Aimed Shot / Furious Attacks
            // Healing taken debuffs
            if (spellproto->EffectBasePoints[eff] < 0)
                return false;
            return true;
        case SPELL_AURA_MOD_RESISTANCE_PCT:
            // Ancestral Healing / Inspiration
            if (spellproto->SpellFamilyName == SPELLFAMILY_SHAMAN ||
                    spellproto->SpellFamilyName == SPELLFAMILY_PRIEST)
                return false;
            return true;
        case SPELL_AURA_MOD_MELEE_HASTE:
        case SPELL_AURA_MOD_DECREASE_SPEED:
            // Attack and movement speed reduction
            if (spellproto->EffectBasePoints[eff] >= 0)
                return false;
            return true;
        default:
            return false;
    }
}

void Aura::ComputeExclusive()
{
    m_exclusive = false;
    //return;
    if (GetHolder()->IsPassive())
        return;
    m_exclusive = _IsExclusiveSpellAura(GetSpellProto(), GetEffIndex(), GetModifier()->m_auraname);
}

// Resultat :
// - 0 : pas dans la meme categorie.
// - 1 : je suis plus important. Je m'applique.
// - 2 : il est plus important. Il s'applique.
int Aura::CheckExclusiveWith(Aura const* other) const
{
    ASSERT(IsExclusive());
    ASSERT(other);
    ASSERT(other->IsExclusive());
    if (other->GetModifier()->m_auraname != GetModifier()->m_auraname)
        return 0;
    if (other->GetModifier()->m_miscvalue != GetModifier()->m_miscvalue)
        return 0;
    if (other->GetSpellProto()->EffectItemType[other->GetEffIndex()] != GetSpellProto()->EffectItemType[GetEffIndex()])
        return 0;

    // Lui est mieux
    if (other->GetModifier()->m_amount > GetModifier()->m_amount && GetModifier()->m_amount >= 0)
        return 2;
    else if (other->GetModifier()->m_amount < GetModifier()->m_amount && GetModifier()->m_amount < 0)
        return 2;

    return 1;
}

bool Aura::ExclusiveAuraCanApply()
{
    Unit* target = GetTarget();
    ASSERT(target);
    if (Aura* mostImportant = target->GetMostImportantAuraAfter(this, this))
    {
        // Il y a un souci dans le sort.
        if (mostImportant->IsInUse())
        {
            sLog.outInfo("[%s:Map%u:Aura%u:AuraImportant%u] Aura::ExclusiveAuraCanApply IsInUse", target->GetName(), target->GetMapId(), GetId(), mostImportant->GetId());
            return false;
        }
        ASSERT(!mostImportant->IsInUse());
        int checkResult = CheckExclusiveWith(mostImportant);
        switch (checkResult)
        {
            case 1: // Je suis plus important.
                // Normalement, 'mostImportant' en etant le plus important de sa categorie
                // doit etre applique.
                if (!mostImportant->IsApplied())
                {
                    sLog.outInfo("[%s:Map%u:Aura%u:AuraImportant%u] Aura::ExclusiveAuraCanApply IsApplied", target->GetName(), target->GetMapId(), GetId(), mostImportant->GetId());
                    return false;
                }
                ASSERT(mostImportant->IsApplied());
                // On le desactive, et je m'active.
                mostImportant->ApplyModifier(false, true, true);
                break;
            case 2: // Il est plus important, je le laisse.
                return false;
            case 0: // Impossible.
                ASSERT(false);
            default: // Impossible aussi
                ASSERT(false);
        }
    }
    // Pas d'autre aura trouve, je m'applique.
    return true;
}

void Aura::ExclusiveAuraUnapply()
{
    Unit* target = GetTarget();
    ASSERT(target);
    // On restaure le precedent plus grand aura.
    if (Aura* mostImportant = target->GetMostImportantAuraAfter(this, this))
    {
        if (mostImportant->IsInUse())
        {
            sLog.outInfo("[%s:Map%u:Aura%u:AuraImportant%u] Aura::ExclusiveAuraUnapply IsInUse", target->GetName(), target->GetMapId(), GetId(), mostImportant->GetId());
            return;
        }
        if (mostImportant->IsApplied())
        {
            sLog.outInfo("[%s:Map%u:Aura%u:AuraImportant%u] Aura::ExclusiveAuraUnapply IsApplied", target->GetName(), target->GetMapId(), GetId(), mostImportant->GetId());
            return;
        }
        ASSERT(!mostImportant->IsInUse());
        ASSERT(!mostImportant->IsApplied());
        mostImportant->ApplyModifier(true, true, true);
    }
}
