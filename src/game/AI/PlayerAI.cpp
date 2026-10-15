/*
 * Copyright (C) 2005-2010 MaNGOS <http://getmangos.com/>
 * Copyright (C) 2009-2010 MaNGOSZero <http://github.com/mangoszero/mangoszero/>
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

#include "PlayerAI.h"
#include "Player.h"
#include "DBCStores.h"
#include "Log.h"
#include "SpellMgr.h"
#include "MotionMaster.h"
#include "MoveSpline.h"
#include "Spell.h"
#include "SpellAuras.h"

// Misc spells we dont want players to cast
static std::vector<uint32> priestSkipSpells =
{
    453, 8192, 10953,  // mind soothe
    2096, 10909,                      // mind vision
    586, 9578, 9579, 9592, 10941, 10942,  // fade
    605, 10911, 10912,               // Mind Control
};
static std::vector<uint32> hunterSkipSpells =
{
    //75, // auto shot
    982, //revive pet
    2641, //dismiss pet
};

static std::vector<uint32> warriorSkipSpells =
{
    355, // Taunt
    1161, //Challenging Shout
    694, 7400, 7402, 20559, 20560, // Mocking Blow
};

static std::vector<uint32> druidSkipSpells =
{
    6795, //Growl
    5209, //Challenging Roar
};

static const uint32 PVP_TAUNT_MAX_SPELL_COOLDOWN = 20 * IN_MILLISECONDS;

bool IsPvPTauntBlockedMechanic(Mechanics mechanic)
{
    switch (mechanic)
    {
    case MECHANIC_CHARM:
    case MECHANIC_DISORIENTED:
    case MECHANIC_DISTRACT:
    case MECHANIC_FEAR:
    case MECHANIC_FUMBLE:
    case MECHANIC_ROOT:
    case MECHANIC_PACIFY:
    case MECHANIC_SILENCE:
    case MECHANIC_SLEEP:
    case MECHANIC_SNARE:
    case MECHANIC_FREEZE:
    case MECHANIC_POLYMORPH:
    case MECHANIC_BANISH:
    case MECHANIC_SHACKLE:
    case MECHANIC_TURN:
    case MECHANIC_HORROR:
    case MECHANIC_DAZE:
    case MECHANIC_SAPPED:
    case MECHANIC_SLOW_CAST_SPEED:
    return true;
    default:
    return false;
    }
}

bool HasPvPTauntBlockedMechanic(SpellEntry const* spellInfo)
{
    if (!spellInfo)
        return true;

    if (IsPvPTauntBlockedMechanic(Mechanics(spellInfo->Mechanic)))
        return true;

    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (IsPvPTauntBlockedMechanic(Mechanics(spellInfo->EffectMechanic[i])))
            return true;

    return false;
}

bool IsPvPTauntBlockedAura(AuraType aura)
{
    switch (aura)
    {
    case SPELL_AURA_MOD_POSSESS:
    case SPELL_AURA_MOD_CHARM:
    case SPELL_AURA_MOD_CONFUSE:
    case SPELL_AURA_MOD_FEAR:
    case SPELL_AURA_MOD_TAUNT:
    case SPELL_AURA_MOD_PACIFY:
    case SPELL_AURA_MOD_ROOT:
    case SPELL_AURA_MOD_SILENCE:
    case SPELL_AURA_MOD_PACIFY_SILENCE:
    case SPELL_AURA_MOD_POSSESS_PET:
    case SPELL_AURA_AOE_CHARM:
    case SPELL_AURA_MOD_SHAPESHIFT:
    return true;
    default:
    return false;
    }
}

bool IsPvPTauntAggressiveEffect(SpellEntry const* spellInfo, SpellEffectIndex effectIndex)
{
    switch (SpellEffects(spellInfo->Effect[effectIndex]))
    {
    case SPELL_EFFECT_SCHOOL_DAMAGE:
    case SPELL_EFFECT_POWER_DRAIN:
    case SPELL_EFFECT_HEALTH_LEECH:
    case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
    case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
    case SPELL_EFFECT_WEAPON_DAMAGE:
    case SPELL_EFFECT_POWER_BURN:
    case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
    return true;
    case SPELL_EFFECT_APPLY_AURA:
    case SPELL_EFFECT_PERSISTENT_AREA_AURA:
    case SPELL_EFFECT_APPLY_AREA_AURA_ENEMY:
    {
        AuraType aura = AuraType(spellInfo->EffectApplyAuraName[effectIndex]);
        return aura && !IsPvPTauntBlockedAura(aura) && !spellInfo->IsPositiveEffect(effectIndex, nullptr, nullptr);
    }
    default:
    return false;
    }
}

bool IsPvPTauntAllowedSpell(Player const* player, SpellEntry const* spellInfo)
{
    if (!player || !spellInfo)
        return false;

    if (spellInfo->GetRecoveryTime() > PVP_TAUNT_MAX_SPELL_COOLDOWN)
        return false;

    if (spellInfo->IsPositiveSpell() || spellInfo->IsHealSpell())
        return false;

    if (spellInfo->HasEffect(SPELL_EFFECT_HEAL))
        return false;

    if (HasPvPTauntBlockedMechanic(spellInfo))
        return false;

    bool hasAggressiveEffect = false;
    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        if (!spellInfo->Effect[i])
            continue;

        AuraType aura = AuraType(spellInfo->EffectApplyAuraName[i]);
        if (aura && IsPvPTauntBlockedAura(aura))
            return false;

        if (IsPvPTauntAggressiveEffect(spellInfo, SpellEffectIndex(i)))
            hasAggressiveEffect = true;
    }

    if (!hasAggressiveEffect)
        return false;

    if ((player->getClass() == CLASS_DRUID || player->getClass() == CLASS_PRIEST) &&
        spellInfo->GetErrorAtShapeshiftedCast(player->GetShapeshiftForm()) != SPELL_CAST_OK)
        return false;

    return true;
}

void PlayerAI::Remove()
{
    me->setAI(NULL);
    delete this;
}

PlayerAI::~PlayerAI()
{
}

CanCastResult PlayerAI::CanCastSpell(Unit* pTarget, const SpellEntry *pSpell, bool isTriggered, bool checkControlled)
{
    if (!pTarget)
        return CAST_FAIL_OTHER;
    // If not triggered, we check
    if (!isTriggered)
    {
        // State does not allow
        if (me->hasUnitState(checkControlled ? UNIT_STAT_CAN_NOT_REACT_OR_LOST_CONTROL : UNIT_STAT_CAN_NOT_REACT))
            return CAST_FAIL_STATE;

        if (pSpell->PreventionType == SPELL_PREVENTION_TYPE_SILENCE && me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED))
            return CAST_FAIL_STATE;

        if (pSpell->PreventionType == SPELL_PREVENTION_TYPE_PACIFY && me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED))
            return CAST_FAIL_STATE;

        // Check for power (also done by Spell::CheckCast())
        if (me->GetPower((Powers)pSpell->powerType) < pSpell->manaCost)
            return CAST_FAIL_POWER;
    }

    if (const SpellRangeEntry *pSpellRange = sSpellRangeStore.LookupEntry(pSpell->rangeIndex))
    {
        if (pTarget != me)
        {
            // pTarget is out of range of this spell (also done by Spell::CheckCast())
            float fDistance = me->GetCombatDistance(pTarget);

            if (fDistance > pSpellRange->maxRange)
                return CAST_FAIL_TOO_FAR;

            float fMinRange = pSpellRange->minRange;

            if (fMinRange && fDistance < fMinRange)
                return CAST_FAIL_TOO_CLOSE;

            if (!pTarget->IsWithinLOSInMap(me))
                return CAST_FAIL_NOT_IN_LOS;
        }
        return CAST_OK;
    }
    else
        return CAST_FAIL_OTHER;
}

void PlayerAI::UpdateAI(const uint32 /*diff*/)
{
}

PlayerControlledAI::PlayerControlledAI(Player* pPlayer, Unit* caster) : uiGlobalCD(0), PlayerAI(pPlayer), controllerGuid(caster ? caster->GetObjectGuid() : ObjectGuid())
{
    ASSERT(pPlayer);
    
    bIsMelee = false;
    isHealer = false;
    isPvPTaunt = pPlayer->HasAuraType(SPELL_AURA_MOD_TAUNT);

    switch (pPlayer->getClass())
    {
        case CLASS_WARRIOR:
        case CLASS_ROGUE:
            bIsMelee = true;
        break;

        case CLASS_PALADIN:
        case CLASS_DRUID:
            bIsMelee = true;
            /*isHealer = true;*/
        break;

        case CLASS_SHAMAN:
            /*isHealer = true;*/
            if (pPlayer->haveOffhandWeapon() || pPlayer->HasAura(35752) || pPlayer->GetTotalAuraModifier(SPELL_AURA_MOD_CRIT_PERCENT) > pPlayer->GetTotalAuraModifier(SPELL_AURA_MOD_SPELL_CRIT_CHANCE_SCHOOL))
            {
                bIsMelee = true;
                /*isHealer = false;*/
            }
            else if (pPlayer->HasSpell(16166) || pPlayer->GetTotalAuraModifier(SPELL_AURA_MOD_DAMAGE_DONE) > pPlayer->GetTotalAuraModifier(SPELL_AURA_MOD_HEALING_DONE))
            {
                /*isHealer = false;*/
                bIsMelee = pPlayer->GetPowerPercent(POWER_MANA) < 20.0f;
            }
            else
            {
                bIsMelee = pPlayer->GetPowerPercent(POWER_MANA) < 20.0f;
            }
        break;

        case CLASS_PRIEST:
            /*isHealer = true;*/
            bIsMelee = pPlayer->GetPowerPercent(POWER_MANA) < 20.0f;
        break;

        case CLASS_MAGE:
        case CLASS_WARLOCK:
        case CLASS_HUNTER:
            bIsMelee = pPlayer->GetPowerPercent(POWER_MANA) < 20.0f;
        break;
    }
    PlayerSpellMap spells = me->GetSpellMap();
    usableSpells.clear();
    for (PlayerSpellMap::iterator itr = spells.begin(); itr != spells.end(); ++itr)
    {
        if (itr->second.state == PLAYERSPELL_REMOVED || itr->second.disabled)
            continue;
        SpellEntry const *spellInfo = sSpellMgr.GetSpellEntry(itr->first);
        if (!spellInfo) 
            continue;
        if (spellInfo->SpellFamilyName == SPELLFAMILY_GENERIC)
            continue;
        if (spellInfo->Attributes & (SPELL_ATTR_PASSIVE | 0x80))
            continue;
        if (spellInfo->AttributesEx & SPELL_ATTR_EX_NO_AUTOCAST_AI)
            continue;
        if (spellInfo->AuraInterruptFlags & AURA_INTERRUPT_FLAG_DAMAGE)
            continue;
        if (isPvPTaunt && !IsPvPTauntAllowedSpell(pPlayer, spellInfo))
            continue;
        if (Spells::IsPositiveSpell(itr->first) && !enablePositiveSpells)
            continue;
        switch (pPlayer->getClass())
        {
        case CLASS_ROGUE:
        case CLASS_PALADIN:
        case CLASS_SHAMAN:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
            break;
        case CLASS_WARRIOR:
            if (std::find(warriorSkipSpells.begin(), warriorSkipSpells.end(), itr->first) != warriorSkipSpells.end())
                continue;
            break;
        case CLASS_DRUID:
            if (std::find(druidSkipSpells.begin(), druidSkipSpells.end(), itr->first) != druidSkipSpells.end())
                continue;
            break;
        case CLASS_HUNTER:
            if (std::find(hunterSkipSpells.begin(), hunterSkipSpells.end(), itr->first) != hunterSkipSpells.end())
                continue;
            break;
        case CLASS_PRIEST:
            if (std::find(priestSkipSpells.begin(), priestSkipSpells.end(), itr->first) != priestSkipSpells.end())
                continue;
            break;
        }
        usableSpells.push_back(itr->first);
    }
    // Suppression des sorts dont on a deja des rangs superieurs
    for (std::vector<uint32>::iterator it = usableSpells.begin(); it != usableSpells.end();)
    {
        bool foundSupRank = false;
        SpellEntry const *pCurrSpell_1 = sSpellMgr.GetSpellEntry(*(it));
        for (std::vector<uint32>::iterator it2 = usableSpells.begin(); it2 != usableSpells.end(); ++it2)
        {
            SpellEntry const *pCurrSpell_2 = sSpellMgr.GetSpellEntry(*(it2));
            if (pCurrSpell_2->SpellFamilyName == pCurrSpell_1->SpellFamilyName && pCurrSpell_2->SpellIconID == pCurrSpell_1->SpellIconID && pCurrSpell_2->SpellVisual == pCurrSpell_1->SpellVisual) // Meme sort, rangs differents
            {
                if (Spells::CompareAuraRanks(pCurrSpell_1->Id, pCurrSpell_2->Id) < 0) // pCurrSpell_1 < pCurrSpell_2
                {
                    // Donc on supprime pCurrSpell_1
                    foundSupRank = true;
                    break;
                }
            }
        }
        if (foundSupRank)
        {
            usableSpells.erase(it);
            it = usableSpells.begin();
        }
        else
            ++it;
    }
    
    me->GetMotionMaster()->Clear();
    
    if (caster && caster->ToCreature())
        if (Unit* victim = caster->ToCreature()->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
            UpdateTarget(victim);

}

Unit* PlayerControlledAI::FindController()
{
    return me->GetMap()->GetUnit(controllerGuid);
}

void PlayerControlledAI::UpdateTarget(Unit* victim)
{
    if ((victim->isCharmed() && victim->GetCharmerGuid() == me->GetCharmerGuid()) || me->isFeared() || me->IsPolymorphed())
    {
        me->AttackStop();
        me->CastStop();
        return;
    }

    Unit* controller = FindController();

    bool isNewVictim = me->getVictim() != victim;

    if (isNewVictim)
        me->Attack(victim, true);

    // Mode combat
    if (controller && controller->GetTypeId() == TYPEID_PLAYER)
    {
        me->SetInCombatWith(victim);
        controller->SetInCombatWith(victim);

        if (!me->GetMotionMaster()->empty() && me->GetMotionMaster()->GetCurrentMovementGeneratorType() != IDLE_MOTION_TYPE)
            return;
        me->GetMotionMaster()->Clear();
        me->GetMotionMaster()->MoveChase(victim);
    }
    else
    {
        me->SetInCombatWith(victim);
        victim->SetInCombatWith(me);
        if (controller)
        {
            controller->SetInCombatWith(victim);
            victim->SetInCombatWith(controller);
        }

        if (!bIsMelee)
            me->SetCasterChaseDistance(25.0f);

        if (isNewVictim)
        {
            me->GetMotionMaster()->Clear();
            me->GetMotionMaster()->MoveChase(victim);
            return;
        }
        else
        {
            bool inMeleeRange = me->IsWithinMeleeRange(victim);
            if ( (bIsMelee && inMeleeRange) || (!bIsMelee && !me->IsMoving() && me->IsWithinDist(victim, 30.0f)))
            {
                me->GetMotionMaster()->Clear();
                if (bIsMelee && !me->HasInArc(0.2f, victim))
                {
                    me->SetFacingToObject(victim);
                }
                else if (!me->HasInArc(0.5f, victim))
                {
                    me->SetFacingToObject(victim);
                }
            }
            else
            {
                // melee not in melee range or nonmoving caster not in caster range
                if (!me->isInRoots() && !me->IsMoving())
                {
                    me->GetMotionMaster()->Clear();
                    me->GetMotionMaster()->MoveChase(victim);
                }
            }
        }
    }
}

PlayerControlledAI::~PlayerControlledAI()
{
}

void PlayerControlledAI::UpdateAI(const uint32 uiDiff)
{
    if (me->IsDeleted() || !me->IsInWorld() || !me->isAlive())
    {
        me->RemoveAI();
        return;
    }

    Unit* victim = NULL;
    CharmInfo* charmInfo = me->GetCharmInfo();
    Unit* controller = FindController();

    // If controller is a player
    if (controller && controller->GetTypeId() == TYPEID_PLAYER)
    {
        Player* Pcontroller = ((Player*)controller);

        if (!Pcontroller->isAlive())
        {
            me->RemoveCharmAuras();
            // Note that we CANNOT continue after this point since this object has been deleted
            // Pcontroller is a local var on the stack, not a member object, so it is safe to use
            Pcontroller->RemoveCharmAuras();
            return;
        }

        if (charmInfo)
        {
            if (charmInfo->HasReactState(REACT_PASSIVE))
                victim = me->getVictim();
            else if (charmInfo->HasReactState(REACT_DEFENSIVE) || charmInfo->HasReactState(REACT_AGGRESSIVE))
            {
                victim = me->getVictim();
                if (!victim || (victim == Pcontroller))
                    victim = Pcontroller->getVictim();
            }
        }
        else
        {
            victim = me->getVictim();
            if (!victim || (victim == Pcontroller))
                victim = Pcontroller->getVictim();
        }

        if (!victim || (victim == me))
            return;

        UpdateTarget(victim);
    }
    else // If controller is a creature
    {
        Creature* Ccontroller = controller ? controller->ToCreature() : nullptr;

        // Unit * victim = controller-> getVictim ();
        // Ivina <Nostalrius>: chooses the target randomly and not always the target of the controller.
        victim = Ccontroller ? Ccontroller->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0) : me->SelectNearestTarget(50.0f);
        if (Unit* v2 = me->getVictim())
            if (me->canAttack(v2, false))
                victim = v2;

        if (Ccontroller && (!Ccontroller->isAlive() || !Ccontroller->isInCombat()))
        {
            if (!me->HasAuraType(SPELL_AURA_MOD_TAUNT))
            {
                me->RemoveCharmAuras();
                // Note that we CANNOT continue after this point since this object has been deleted
                // Ccontroller is a local var on the stack, not a member object, so it is safe to use
                Ccontroller->RemoveCharmAuras();
                return;
            }
            else
            {
                std::vector<uint32> tauntAuras;
                Unit::AuraList const& lst = me->GetAurasByType(SPELL_AURA_MOD_TAUNT);

                for (Unit::AuraList::const_iterator it = lst.begin(); it != lst.end(); ++it)
                {
                    Aura* aura = *it;
                    if (!aura)
                        continue;

                    uint32 spellId = aura->GetId();
                    if (spellId)
                        tauntAuras.push_back(spellId);
                }

                for (std::vector<uint32>::const_iterator it = tauntAuras.begin(); it != tauntAuras.end(); ++it)
                    me->RemoveAurasDueToSpell(*it);

                return;
            }
        }

        if (!victim || (victim == me)) // Ivina <Nostalrius>: avoid being targeted
            return;

        // Need a valid target
        if (!me->IsHostileTo(victim))
            return;

        UpdateTarget(victim);


        if (uiGlobalCD < uiDiff)
        {
            if (me->getClass() == CLASS_HUNTER)
            {
                float dist = me->GetDistance(victim);
                if (dist > 5.0f)
                {
                    if (Spell* pSpell = me->GetCurrentSpell(CURRENT_AUTOREPEAT_SPELL))
                    {
                        if (pSpell->m_spellInfo && pSpell->m_spellInfo->Id != 75) // auto shoot
                            me->CastSpell(victim, 75, true);
                    }
                }
            }

            if (me->IsNonMeleeSpellCasted(true))
                uiGlobalCD = 200;
            else
            {
                // Since we are going to do something usableSpells [0, urand (0, usableSpells.size () - 1)], we must have at least one element.
                if (usableSpells.empty())
                {
                    uiGlobalCD = 200;
                    return;
                }


                uint32 spellId = usableSpells[urand(0, usableSpells.size() - 1)];
                SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(spellId);

                if (spellInfo)
                {
                    if (isPvPTaunt && !IsPvPTauntAllowedSpell(me, spellInfo))
                    {
                        uiGlobalCD = 200;
                        return;
                    }

                    // If its a positive spell we prioritize controller, if he's out of range,
                    // ourself, otherwise it will probably not be cast.
                    Unit* spellTarget = victim;
                    if (!isPvPTaunt && controller && spellInfo->IsPositiveSpell(me, controller))
                        spellTarget = controller;
                    else if (!isPvPTaunt && spellInfo->IsPositiveSpell(me, me))
                        spellTarget = me;

                    if (CanCastSpell(spellTarget, spellInfo, false, false) == CAST_OK)
                    {
                        me->CastSpell(spellTarget, spellId, false);
                        uiGlobalCD = 1000;
                        if(!bIsMelee)
                            me->SetCasterChaseDistance(25.0f);
                    }
                    else if (!me->IsWithinLOSInMap(spellTarget))
                    {
                        me->GetMotionMaster()->MoveChase(spellTarget);
                    }
                    else if (!bIsMelee)
                        me->SetCasterChaseDistance(0.0f);
                }
            }
        }
        else
            uiGlobalCD -= uiDiff;
    }
}
