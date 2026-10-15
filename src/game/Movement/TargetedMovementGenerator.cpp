/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
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

#include "ByteBuffer.h"
#include "TargetedMovementGenerator.h"
#include "Errors.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "Player.h"
#include "World.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"
#include "Anticheat.h"
#include "Transport.h"
#include "TemporarySummon.h"
#include "GameObjectAI.h"

static float GetDistance2DSq(float x1, float y1, float x2, float y2)
{
    float dx = x1 - x2;
    float dy = y1 - y2;
    return dx * dx + dy * dy;
}

static bool IsClimbablePath(PathFinder const& path, float maxStepHeight, float maxRisePerYard)
{
    PointsArray const& points = path.getPath();
    if (points.size() < 2)
        return false;

    for (uint32 i = 1; i < points.size(); ++i)
    {
        float dx = points[i].x - points[i - 1].x;
        float dy = points[i].y - points[i - 1].y;
        float horizontalDist = sqrtf(dx * dx + dy * dy);
        float zDiff = fabsf(points[i].z - points[i - 1].z);

        if (zDiff > maxStepHeight && (horizontalDist < 0.1f || zDiff / horizontalDist > maxRisePerYard))
            return false;
    }

    return true;
}

static bool GetSafeWalkPoint(Unit& owner, G3D::Vector3 const& from, G3D::Vector3 const& wanted, G3D::Vector3& result)
{
    result = wanted;
    owner.UpdateAllowedPositionZ(result.x, result.y, result.z);

    if (!owner.GetMap()->GetWalkHitPosition(owner.GetTransport(), from.x, from.y, from.z, result.x, result.y, result.z))
        return false;

    if (GetDistance2DSq(result.x, result.y, wanted.x, wanted.y) > 0.64f)
        return false;

    owner.UpdateAllowedPositionZ(result.x, result.y, result.z);
    return true;
}

static float GetWalkProbeDistance(Unit& owner, G3D::Vector3 const& point, float dirX, float dirY, float dist)
{
    float x = point.x + dirX * dist;
    float y = point.y + dirY * dist;
    float z = point.z;

    owner.UpdateAllowedPositionZ(x, y, z);

    if (!owner.GetMap()->GetWalkHitPosition(owner.GetTransport(), point.x, point.y, point.z, x, y, z))
        return 0.0f;

    return sqrtf(GetDistance2DSq(point.x, point.y, x, y));
}

static bool TryPushChasePathPointAwayFromWall(Unit& owner, G3D::Vector3 const& prev, G3D::Vector3& point, G3D::Vector3 const& next)
{
    float prevX = point.x - prev.x;
    float prevY = point.y - prev.y;
    float nextX = next.x - point.x;
    float nextY = next.y - point.y;
    float prevLen = sqrtf(prevX * prevX + prevY * prevY);
    float nextLen = sqrtf(nextX * nextX + nextY * nextY);

    if (prevLen < 2.0f || nextLen < 2.0f)
        return false;

    float dirX = next.x - prev.x;
    float dirY = next.y - prev.y;
    float len = sqrtf(dirX * dirX + dirY * dirY);

    if (len < 2.0f)
        return false;

    dirX /= len;
    dirY /= len;

    float sideX = -dirY;
    float sideY = dirX;


    float probeDist = 10.0f;
    float left = GetWalkProbeDistance(owner, point, sideX, sideY, probeDist);
    float right = GetWalkProbeDistance(owner, point, -sideX, -sideY, probeDist);

    if (left > probeDist * 0.95f && right > probeDist * 0.95f)
        return false;

    float diff = left - right;
    float absDiff = diff < 0.0f ? -diff : diff;

    if (absDiff < 0.75f)
        return false;

    float side = diff > 0.0f ? 1.0f : -1.0f;
    float shiftDist = absDiff * 0.5f;

    if (shiftDist < 0.4f)
        return false;

    if (shiftDist > 4.0f)
        shiftDist = 4.0f;


    G3D::Vector3 wanted(point.x + sideX * side * shiftDist, point.y + sideY * side * shiftDist, point.z);
    G3D::Vector3 shifted;

    if (!GetSafeWalkPoint(owner, prev, wanted, shifted))
        return false;

    G3D::Vector3 checkedNext;
    if (!GetSafeWalkPoint(owner, shifted, next, checkedNext))
        return false;

    point = shifted;
    return true;
}

static void PushChasePathAwayFromWalls(Unit& owner, Movement::PointsArray& path)
{
    if (path.size() < 3)
        return;

    uint32 checked = 0;
    uint32 maxPointsToCheck = 4;

    for (uint32 i = 1; i + 1 < path.size() && checked < maxPointsToCheck; ++i, ++checked)
        TryPushChasePathPointAwayFromWall(owner, path[i - 1], path[i], path[i + 1]);
}

//-----------------------------------------------//
template<class T, typename D>
void TargetedMovementGeneratorMedium<T, D>::_setTargetLocation(T &owner)
{
    // Note: Any method that accesses the target's movespline here must be
    // internally locked by the target's spline lock
    if (!i_target.isValid() || !i_target->IsInWorld())
        return;

    if (owner.hasUnitState(UNIT_STAT_CAN_NOT_MOVE | UNIT_STAT_POSSESSED))
        return;

    float x, y, z;
    bool losChecked = false;
    bool losResult = false;

    Transport* transport = NULL;
    bool isPet = (owner.GetTypeId() == TYPEID_UNIT && ((Creature*)&owner)->IsPet());
    if (isPet)
        transport = i_target.getTarget()->GetTransport();
    // prevent redundant micro-movement for pets, other followers.
    if (m_fOffset && i_target->IsWithinDistInMap(&owner, 1.4f * m_fOffset))
    {
        if (!owner.movespline->Finalized())
            return;

        owner.GetPosition(x, y, z);
    }
    else if (!m_fOffset)
    {
        if (owner.IsWithinMeleeRange(i_target.getTarget()))
        {
            losResult = owner.IsWithinLOSInMap(i_target.getTarget());
            losChecked = true;
            if (losResult)
                return;
        }

        // NOSTALRIUS: Eviter les collisions entre mobs.
        // Cette fonction prend un angle aleatoire.
        i_target->GetRandomAttackPoint(&owner, x, y, z);

        // Soft kite spread
        if (this->GetMovementGeneratorType() == CHASE_MOTION_TYPE && owner.GetTypeId() == TYPEID_UNIT && !isPet && !owner.CanFly() &&
            (i_target->GetTypeId() == TYPEID_PLAYER || i_target->GetCharmerOrOwnerGuid().IsPlayer()) && i_target->IsMoving() && !i_target->GetTransport() && !owner.GetTransport() && i_target->getAttackers().size() > 1)
        {
            uint32 index = 0;
            uint32 count = 0;
            bool found = false;

            for (auto const& attacker : i_target->getAttackers())
            {
                if (!attacker || !attacker->IsCreature())
                    continue;

                if (attacker == &owner)
                {
                    index = count;
                    found = true;
                }

                ++count;
            }

            if (found && count > 1)
            {
                uint32 rowSize = 8;
                uint32 maxRows = 4;

                uint32 rows = (count + rowSize - 1) / rowSize;
                if (rows > maxRows)
                    rows = maxRows;

                uint32 slots = (count + rows - 1) / rows;
                uint32 row = index / slots;
                uint32 slot = index % slots;

                if (row >= rows)
                    row = rows - 1;

                float center = (float(slots) - 1.0f) * 0.5f;
                float offset = float(slot) - center;

                float maxSide = 0.50f + sqrtf(float(count)) * 0.25f;
                if (maxSide > 2.60f)
                    maxSide = 2.60f;

                float sideOffset = center > 0.0f ? (offset / center) * maxSide : 0.0f;
                float backOffset = float(row) * 0.75f;

                float backAngle = i_target->GetAngle(x, y);
                float sideAngle = backAngle + M_PI_F * 0.5f;

                float spreadX = x + cos(sideAngle) * sideOffset + cos(backAngle) * backOffset;
                float spreadY = y + sin(sideAngle) * sideOffset + sin(backAngle) * backOffset;
                float spreadZ = z;

                float pullAngle = i_target->GetAngle(spreadX, spreadY);
                float pullDist = 0.35f;

                spreadX += cos(pullAngle) * pullDist;
                spreadY += sin(pullAngle) * pullDist;

                uint32 nav = NAV_GROUND | NAV_WATER | NAV_MAGMA | NAV_SLIME;

                if (owner.GetMap()->GetWalkHitPosition(nullptr, x, y, z, spreadX, spreadY, spreadZ, nav) &&
                    owner.CanReachWithMeleeAutoAttackAtPosition(i_target.getTarget(), spreadX, spreadY, spreadZ, 0.75f + backOffset))
                {
                    x = spreadX;
                    y = spreadY;
                    z = spreadZ;
                }
            }
        }

    }
    else
    {
        // to at m_fOffset distance from target and m_fAngle from target facing
        float srcX, srcY, srcZ;
        i_target->GetSafePosition(srcX, srcY, srcZ, transport);
        if (transport)
            transport->CalculatePassengerPosition(srcX, srcY, srcZ);

        // TRANSPORT_VMAPS
        if (transport)
        {
            i_target->GetNearPoint2D(x, y, m_fOffset, m_fAngle + i_target->GetOrientation());
            z = srcZ;
        }
        else
            i_target->GetClosePoint(x, y, z, owner.GetObjectBoundingRadius(), m_fOffset, m_fAngle, &owner);

        if (!i_target->m_movementInfo.HasMovementFlag(MOVEFLAG_SWIMMING) && !i_target->IsInWater())
            if (!owner.GetMap()->GetWalkHitPosition(transport, srcX, srcY, srcZ, x, y, z))
                i_target->GetSafePosition(x, y, z);
    }

    m_bTargetOnTransport = transport;
    i_target->GetPosition(m_fTargetLastX, m_fTargetLastY, m_fTargetLastZ, transport);

    PathFinder path(&owner);
    PathFinder normalPath(&owner);
    PathFinder* movePath = &path;

    // allow pets following their master to cheat while generating paths
    bool petFollowing = (isPet && owner.hasUnitState(UNIT_STAT_FOLLOW));
    bool avoidSteepSlopes = this->GetMovementGeneratorType() == CHASE_MOTION_TYPE && !owner.CanFly();

    if (avoidSteepSlopes)
        path.ExcludeSteepSlopes();

    Movement::MoveSplineInit init(owner, "TargetedMovementGenerator");
    path.SetTransport(transport);
    path.calculate(x, y, z, petFollowing);

    PathType pathType = path.getPathType();
    m_bReachable = pathType & PATHFIND_NORMAL;
    if (!m_bReachable && !!(pathType & PATHFIND_INCOMPLETE) && owner.hasUnitState(UNIT_STAT_ALLOW_INCOMPLETE_PATH))
        m_bReachable = true;

    if (avoidSteepSlopes && !m_bReachable)
    {
        normalPath.SetTransport(transport);
        normalPath.calculate(x, y, z, petFollowing);
        movePath = &normalPath;

        pathType = normalPath.getPathType();
        m_bReachable = pathType & PATHFIND_NORMAL;
        if (!m_bReachable && !!(pathType & PATHFIND_INCOMPLETE) && owner.hasUnitState(UNIT_STAT_ALLOW_INCOMPLETE_PATH))
            m_bReachable = true;
    }

    if (avoidSteepSlopes && movePath == &path && m_bReachable && (pathType & PATHFIND_NORMAL) && !(pathType & PATHFIND_NOT_USING_PATH))
    {
        float strictLength = path.Length();
        float directDistSq = GetDistance2DSq(owner.GetPositionX(), owner.GetPositionY(), x, y);

        if (strictLength > 8.0f && strictLength * strictLength > directDistSq * 2.25f)
        {
            normalPath.SetTransport(transport);
            normalPath.calculate(x, y, z, petFollowing);

            PathType normalPathType = normalPath.getPathType();
            bool normalReachable = normalPathType & PATHFIND_NORMAL;
            if (!normalReachable && !!(normalPathType & PATHFIND_INCOMPLETE) && owner.hasUnitState(UNIT_STAT_ALLOW_INCOMPLETE_PATH))
                normalReachable = true;

            // Maximum height difference between adjacent normal-path points.
            float const maxLowLedgeHeight = 2.0f;
            float const maxGroundRisePerYard = 1.5f;
            if (normalReachable && normalPath.Length() + 2.0f < strictLength && IsClimbablePath(normalPath, maxLowLedgeHeight, maxGroundRisePerYard))
            {
                movePath = &normalPath;
                pathType = normalPathType;
                m_bReachable = true;
            }
        }
    }

    // Enforce stricter checking inside dungeons
    if (m_bReachable && owner.GetMap() && owner.GetMap()->IsDungeon())
    {
        // Check dest coords to ensure reachability
        G3D::Vector3 dest = movePath->getActualEndPosition();
        if (!owner.CanReachWithMeleeAutoAttackAtPosition(i_target.getTarget(), dest[0], dest[1], dest[2]))
            m_bReachable = false;
    }

    m_bRecalculateTravel = false;
    if (this->GetMovementGeneratorType() == CHASE_MOTION_TYPE && !transport && owner.HasDistanceCasterMovement())
        if (movePath->UpdateForCaster(i_target.getTarget(), owner.GetMinChaseDistance(i_target.getTarget())))
        {
            if (!owner.movespline->Finalized())
                owner.StopMoving();
            return;
        }

    if (this->GetMovementGeneratorType() == CHASE_MOTION_TYPE && owner.GetTypeId() == TYPEID_UNIT && !isPet && !owner.CanFly() && !owner.GetTransport() && !transport && !owner.HasDistanceCasterMovement() && 
        (pathType & PATHFIND_NORMAL) && !(pathType & PATHFIND_NOT_USING_PATH))
    {
        PushChasePathAwayFromWalls(owner, movePath->getFullPath());
    }

    // Try to prevent redundant micro-moves
    float pathLength = movePath->Length();
    if (pathLength < 0.4f ||
            (pathLength < 4.0f && (i_target->GetPositionZ() - owner.GetPositionZ()) > 10.0f) || // He is flying too high for me. Moving a few meters wont change anything.
            (pathType & PATHFIND_NOPATH && !petFollowing) ||
            (pathType & PATHFIND_INCOMPLETE && !owner.hasUnitState(UNIT_STAT_ALLOW_INCOMPLETE_PATH) && !petFollowing) ||
            (!petFollowing && !m_bReachable))
    {
        if (!losChecked)
            losResult = owner.IsWithinLOSInMap(i_target.getTarget());
        if (losResult)
        {
            if (!owner.movespline->Finalized())
                owner.StopMoving();
            return;
        }
    }

    D::_addUnitStateMove(owner);
    m_bTargetReached = false;

    init.Move(movePath);
    if (petFollowing)
    {
        float dist = path.Length();
        init.SetWalk(false);
        if (dist > 15.0f)
        {
            Unit* pOwner = owner.GetCharmerOrOwner();
            if (pOwner && !pOwner->isInCombat() && !owner.isInCombat())
            {
                float distFactor = 1.0f;
                if (pOwner->IsMounted())
                    distFactor += 0.04 * (dist - 25.0f);
                else
                    distFactor += 0.04 * (dist - 15.0f);
                if (distFactor < 1.0f) distFactor = 1.0f;
                if (distFactor > 2.1f) distFactor = 2.1f;
                init.SetVelocity(distFactor * owner.GetSpeed(MOVE_RUN));
            }
        }
        else if (dist < 2.0f)
            init.SetWalk(true);
        float facing = i_target->GetOrientation();
        if (transport)
            facing -= transport->GetOrientation();
        init.SetFacing(facing);
    }
    else
        init.SetWalk(((D*)this)->EnableWalking());
    init.Launch();
    m_checkDistanceTimer.Reset(500);
    // Fly-hack
    if (Player* player = i_target->ToPlayer())
    {
        float allowed_dist = owner.GetCombatReach(false) + i_target->GetCombatReach(false) + 5.0f;
        G3D::Vector3 dest = owner.movespline->FinalDestination();
        if ((player->GetPositionZ() - allowed_dist - 5.0f) > dest.z)
            player->GetCheatData()->OnUnreachable(&owner);
    }
}

template<>
void TargetedMovementGeneratorMedium<Player, ChaseMovementGenerator<Player> >::UpdateFinalDistance(float /*fDistance*/)
{
    // nothing to do for Player
}

template<>
void TargetedMovementGeneratorMedium<Player, FollowMovementGenerator<Player> >::UpdateFinalDistance(float /*fDistance*/)
{
    // nothing to do for Player
}

template<>
void TargetedMovementGeneratorMedium<Creature, ChaseMovementGenerator<Creature> >::UpdateFinalDistance(float fDistance)
{
    m_fOffset = fDistance;
    m_bRecalculateTravel = true;
}

template<>
void TargetedMovementGeneratorMedium<Creature, FollowMovementGenerator<Creature> >::UpdateFinalDistance(float fDistance)
{
    m_fOffset = fDistance;
    m_bRecalculateTravel = true;
}

template<class T, typename D>
void TargetedMovementGeneratorMedium<T, D>::UpdateAsync(T &owner, uint32 /*diff*/)
{
    if (!m_bRecalculateTravel)
        return;
    // All these cases will be handled at next sync update
    if (!i_target.isValid() || !i_target->IsInWorld() || !owner.isAlive() || owner.hasUnitState(UNIT_STAT_CAN_NOT_MOVE | UNIT_STAT_POSSESSED)
            || (this->GetMovementGeneratorType() == CHASE_MOTION_TYPE && owner.hasUnitState(UNIT_STAT_NO_COMBAT_MOVEMENT))
            || static_cast<D*>(this)->_lostTarget(owner)
            || owner.IsNoMovementSpellCasted())
        return;

    // Lock async updates for safety, see Unit::asyncMovesplineLock doc
    ACE_Guard<ACE_Thread_Mutex> guard(owner.asyncMovesplineLock);
    _setTargetLocation(owner);
}

template<class T>
bool ChaseMovementGenerator<T>::Update(T &owner, const uint32 & time_diff)
{
    if (!i_target.isValid() || !i_target->IsInWorld())
        return false;

    if (!owner.isAlive())
        return true;

    if (owner.hasUnitState(UNIT_STAT_CAN_NOT_MOVE | UNIT_STAT_POSSESSED))
    {
        _clearUnitStateMove(owner);
        return true;
    }

    if (owner.hasUnitState(UNIT_STAT_NO_COMBAT_MOVEMENT))
    {
        _clearUnitStateMove(owner);
        return true;
    }

    // prevent movement while casting spells with cast time or channel time
    // don't stop creature movement for spells without interrupt movement flags
    if (owner.IsNoMovementSpellCasted())
    {
        if (!owner.IsStopped())
            owner.StopMoving();
        return true;
    }

    // prevent crash after creature killed pet
    if (_lostTarget(owner))
    {
        _clearUnitStateMove(owner);
        return true;
    }

    bool interrupted = false;
    m_checkDistanceTimer.Update(time_diff);
    if (m_checkDistanceTimer.Passed())
    {
        m_checkDistanceTimer.Reset(100);
        if (m_bIsSpreading)
        {
            if (!owner.movespline->Finalized() && !owner.CanReachWithMeleeAutoAttack(i_target.getTarget()))
            {
                owner.movespline->_Interrupt();
                interrupted = true;
            }
        }
        else
        {
            //More distance let have better performance, less distance let have more sensitive reaction at target move.
            if (!owner.movespline->Finalized() && i_target->IsWithinDist(&owner, 0.0f) && !m_fOffset)
            {
                owner.movespline->_Interrupt();
                interrupted = true;
            }
            else
            {
                float allowed_dist = owner.GetMaxChaseDistance(i_target.getTarget()) - 0.5f;
                bool targetMoved = false;
                G3D::Vector3 dest(m_fTargetLastX, m_fTargetLastY, m_fTargetLastZ);
                if (Transport* ownerTransport = owner.GetTransport())
                {
                    if (m_bTargetOnTransport)
                        ownerTransport->CalculatePassengerPosition(dest.x, dest.y, dest.z);
                    else
                        targetMoved = true;
                }
                else if (m_bTargetOnTransport)
                    targetMoved = true;

                if (!targetMoved)
                    targetMoved = !i_target->IsWithinDist3d(dest.x, dest.y, dest.z, 0.5f);

                // Chase movement may be interrupted
                if (!targetMoved)
                    if (owner.movespline->Finalized())
                        targetMoved = !owner.IsWithinDist3d(dest.x, dest.y, dest.z, allowed_dist - owner.GetObjectBoundingRadius());

                if (targetMoved)
                {
                    m_bRecalculateTravel = true;
                    owner.GetMotionMaster()->SetNeedAsyncUpdate();
                }
                else
                {
                    // Fly-hack
                    if (Player* player = i_target->ToPlayer())
                        if ((player->GetPositionZ() - allowed_dist - 5.0f) > dest.z)
                            player->GetCheatData()->OnUnreachable(&owner);
                }
            }
        }
    }

    if (owner.movespline->Finalized())
    {
        if (!owner.HasInArc(0.01f, i_target.getTarget()))
            owner.SetInFront(i_target.getTarget());

        if (m_bIsSpreading)
            m_bIsSpreading = false;
        else
        {
            MovementInform(owner);

            if (!m_bTargetReached)
            {
                m_uiSpreadAttempts = 0;
                m_bCanSpread = true;
                m_bTargetReached = true;
                _reachTarget(owner);
            }
        }

        if (interrupted)
            owner.StopMoving();

        m_spreadTimer.Update(time_diff);
        if (m_spreadTimer.Passed())
        {
            m_spreadTimer.Reset(urand(2500, 3500));
            if (Creature* creature = owner.ToCreature())
            {
                if (!(creature->GetCreatureInfo()->flags_extra & CREATURE_FLAG_EXTRA_CHASE_GEN_NO_BACKING) && !creature->IsPet() && !i_target.getTarget()->IsMoving())
                {
                    if (m_bRecalculateTravel && TargetDeepInBounds(owner, i_target.getTarget()))
                        DoBackMovement(owner, i_target.getTarget());
                    else if (m_bCanSpread)
                        DoSpreadIfNeeded(owner, i_target.getTarget());
                }
            }
        }
        
    }
    else if (m_bRecalculateTravel)
        owner.GetMotionMaster()->SetNeedAsyncUpdate();
    return true;
}

template<class T>
bool ChaseMovementGenerator<T>::TargetDeepInBounds(T &owner, Unit* target) const
{
    return TargetWithinBoundsPercentDistance(owner, target, 0.5f);
}

template<class T>
bool ChaseMovementGenerator<T>::TargetWithinBoundsPercentDistance(T &owner, Unit* target, float pct) const
{
    float radius = std::min(target->GetObjectBoundingRadius(), owner.GetObjectBoundingRadius());
        
    radius *= pct;

    return owner.GetDistanceSqr(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ()) < radius;
}

template<class T>
void ChaseMovementGenerator<T>::DoBackMovement(T &owner, Unit* target)
{
    float x, y, z;
    target->GetClosePoint(x, y, z, target->GetObjectBoundingRadius() + owner.GetObjectBoundingRadius(), 1.0f, m_fAngle, &owner);

    // Don't move beyond attack range.
    if (!owner.CanReachWithMeleeAutoAttackAtPosition(target, x, y, z, 0.0f))
        return;

    m_bIsSpreading = true;
    Movement::MoveSplineInit init(owner, "ChaseMovementGenerator");
    init.MoveTo(x, y, z, MOVE_WALK_MODE);
    init.SetWalk(true);
    init.Launch();
}

#define MAX_SPREAD_ATTEMPTS 3

template<class T>
void ChaseMovementGenerator<T>::DoSpreadIfNeeded(T &owner, Unit* target)
{
    // Move away from any NPC deep in our bounding box. There's no limit to the
    // angle moved; NPCs will eventually start spreading behind the target if
    // there's enough of them.
    Unit* pSpreadingTarget = nullptr;

    for (auto& attacker : target->getAttackers())
    {
        if (attacker->IsCreature() && (attacker != &owner) &&
            (owner.GetObjectBoundingRadius() - 2.0f < attacker->GetObjectBoundingRadius()) &&
            !attacker->IsMoving() &&
            (owner.GetDistanceSqr(attacker->GetPositionX(), attacker->GetPositionY(), attacker->GetPositionZ()) < std::min(std::max(owner.GetObjectBoundingRadius(), attacker->GetObjectBoundingRadius()), 0.25f)))
        {
            pSpreadingTarget = attacker;
            break;
        }
    }

    if (!pSpreadingTarget)
    {
        m_bCanSpread = false;
        return;
    }
    
    float const my_angle = target->GetAngle(&owner);
    float const his_angle = target->GetAngle(pSpreadingTarget);
    float const new_angle = (his_angle > my_angle) ? my_angle - frand(0.4f, 1.0f) : my_angle + frand(0.4f, 1.0f);
    
    float x, y, z;
    target->GetNearPoint(&owner, x, y, z, owner.GetObjectBoundingRadius(), frand(0.8f, (target->getAttackers().size() > 5 ? 4.0f : 2.0f)), new_angle);

    // Don't move beyond attack range.
    if (!owner.CanReachWithMeleeAutoAttackAtPosition(target, x, y, z, 0.0f))
        return;

    m_bIsSpreading = true;
    m_uiSpreadAttempts++;

    // Don't circle target infinitely if too many attackers.
    if (m_uiSpreadAttempts >= MAX_SPREAD_ATTEMPTS)
        m_bCanSpread = false;

    Movement::MoveSplineInit init(owner, "ChaseMovementGenerator");
    init.MoveTo(x, y, z, MOVE_WALK_MODE);
    init.SetWalk(true);
    init.Launch();
}

//-----------------------------------------------//
template<class T>
void ChaseMovementGenerator<T>::_reachTarget(T &owner)
{
    if (owner.CanReachWithMeleeAutoAttack(this->i_target.getTarget()))
        owner.Attack(this->i_target.getTarget(), true);
}

template<>
void ChaseMovementGenerator<Player>::Initialize(Player &owner)
{
    owner.addUnitState(UNIT_STAT_CHASE | UNIT_STAT_CHASE_MOVE);
    m_bRecalculateTravel = true;
    owner.GetMotionMaster()->SetNeedAsyncUpdate();
}

template<>
void ChaseMovementGenerator<Creature>::Initialize(Creature &owner)
{
    owner.SetWalk(false, false);
    owner.addUnitState(UNIT_STAT_CHASE | UNIT_STAT_CHASE_MOVE);
    m_bRecalculateTravel = true;
    owner.GetMotionMaster()->SetNeedAsyncUpdate();
}

template<class T>
void ChaseMovementGenerator<T>::Finalize(T &owner)
{
    owner.clearUnitState(UNIT_STAT_CHASE | UNIT_STAT_CHASE_MOVE);
    //MovementInform(owner);
}

template<class T>
void ChaseMovementGenerator<T>::Interrupt(T &owner)
{
    owner.clearUnitState(UNIT_STAT_CHASE | UNIT_STAT_CHASE_MOVE);
}

template<class T>
void ChaseMovementGenerator<T>::Reset(T &owner)
{
    Initialize(owner);
}

template<class T>
void ChaseMovementGenerator<T>::MovementInform(T& /*unit*/)
{
}

template<>
void ChaseMovementGenerator<Creature>::MovementInform(Creature& unit)
{
    if (!unit.isAlive())
        return;

    // Pass back the GUIDLow of the target. If it is pet's owner then PetAI will handle
    if (unit.AI())
        unit.AI()->MovementInform(CHASE_MOTION_TYPE, i_target.getTarget()->GetGUIDLow());

    if (unit.IsTemporarySummon())
    {
        TemporarySummon* pSummon = (TemporarySummon*)(&unit);
        if (pSummon->GetSummonerGuid().IsCreature())
        {
            if (Creature* pSummoner = unit.GetMap()->GetCreature(pSummon->GetSummonerGuid()))
                if (pSummoner->AI())
                    pSummoner->AI()->SummonedMovementInform(&unit, CHASE_MOTION_TYPE, i_target.getTarget()->GetGUIDLow());
        }
        else
        {
            if (GameObject* pSummoner = unit.GetMap()->GetGameObject(pSummon->GetSummonerGuid()))
                if (pSummoner->AI())
                    pSummoner->AI()->SummonedMovementInform(&unit, CHASE_MOTION_TYPE, i_target.getTarget()->GetGUIDLow());
        }
    }
}

//-----------------------------------------------//

template<class T>
bool FollowMovementGenerator<T>::Update(T &owner, const uint32 & time_diff)
{
    if (!i_target.isValid() || !i_target->IsInWorld())
    return false;

    if (!owner.isAlive())
        return true;

    if (owner.hasUnitState(UNIT_STAT_CAN_NOT_MOVE | UNIT_STAT_POSSESSED))
    {
        _clearUnitStateMove(owner);
        return true;
    }

    // prevent movement while casting spells with cast time or channel time
    // don't stop creature movement for spells without interrupt movement flags
    if (owner.IsNoMovementSpellCasted())
    {
        if (!owner.IsStopped())
            owner.StopMoving();
        return true;
    }

    // prevent crash after creature killed pet
    if (_lostTarget(owner))
    {
        _clearUnitStateMove(owner);
        return true;
    }

    bool interrupted = false;
    m_checkDistanceTimer.Update(time_diff);
    if (m_checkDistanceTimer.Passed())
    {
        m_checkDistanceTimer.Reset(100);
        //More distance let have better performance, less distance let have more sensitive reaction at target move.
        if (!owner.movespline->Finalized() && i_target->IsWithinDist(&owner, 0.0f) && !m_fOffset)
        {
            owner.movespline->_Interrupt();
            interrupted = true;
        }
        else
        {
            float allowed_dist = owner.GetMaxChaseDistance(i_target.getTarget()) - 0.5f;
            bool targetMoved = false;
            G3D::Vector3 dest(m_fTargetLastX, m_fTargetLastY, m_fTargetLastZ);
            if (Transport* ownerTransport = owner.GetTransport())
            {
                if (m_bTargetOnTransport)
                    ownerTransport->CalculatePassengerPosition(dest.x, dest.y, dest.z);
                else
                    targetMoved = true;
            }
            else if (m_bTargetOnTransport)
                targetMoved = true;

            if (!targetMoved)
                targetMoved = !i_target->IsWithinDist3d(dest.x, dest.y, dest.z, 0.5f);
            // Chase movement may be interrupted
            if (!targetMoved)
                if (owner.movespline->Finalized())
                    targetMoved = !owner.IsWithinDist3d(dest.x, dest.y, dest.z, allowed_dist - owner.GetObjectBoundingRadius());

            if (targetMoved)
            {
                m_bRecalculateTravel = true;
                owner.GetMotionMaster()->SetNeedAsyncUpdate();
            }
        }
    }

    if (owner.movespline->Finalized())
    {
        MovementInform(owner);

        if (m_fAngle == 0.f && !owner.HasInArc(0.01f, i_target.getTarget()))
            owner.SetInFront(i_target.getTarget());

        if (!m_bTargetReached)
        {
            m_bTargetReached = true;
            _reachTarget(owner);
        }
        if (interrupted)
            owner.StopMoving();
    }
    else if (m_bRecalculateTravel)
        owner.GetMotionMaster()->SetNeedAsyncUpdate();
    return true;
}

template<>
bool FollowMovementGenerator<Creature>::EnableWalking() const
{
    return i_target.isValid() && i_target->IsWalking();
}

template<>
bool FollowMovementGenerator<Player>::EnableWalking() const
{
    return false;
}

template<>
void FollowMovementGenerator<Player>::_updateSpeed(Player &/*u*/)
{
    // nothing to do for Player
}

template<>
void FollowMovementGenerator<Creature>::_updateSpeed(Creature &u)
{
    if (!i_target.isValid() || i_target->GetObjectGuid() != u.GetOwnerGuid())
        return;

    u.UpdateSpeed(MOVE_RUN, false);
    u.UpdateSpeed(MOVE_WALK, false);
    u.UpdateSpeed(MOVE_SWIM, false);
}

template<>
void FollowMovementGenerator<Player>::Initialize(Player &owner)
{
    owner.addUnitState(UNIT_STAT_FOLLOW | UNIT_STAT_FOLLOW_MOVE);
    _updateSpeed(owner);
    _setTargetLocation(owner);
}

template<>
void FollowMovementGenerator<Creature>::Initialize(Creature &owner)
{
    owner.addUnitState(UNIT_STAT_FOLLOW | UNIT_STAT_FOLLOW_MOVE);
    _updateSpeed(owner);
    _setTargetLocation(owner);
}

template<class T>
void FollowMovementGenerator<T>::Finalize(T &owner)
{
    owner.clearUnitState(UNIT_STAT_FOLLOW | UNIT_STAT_FOLLOW_MOVE);
    _updateSpeed(owner);
    //MovementInform(owner);
}

template<class T>
void FollowMovementGenerator<T>::Interrupt(T &owner)
{
    owner.clearUnitState(UNIT_STAT_FOLLOW | UNIT_STAT_FOLLOW_MOVE);
    _updateSpeed(owner);
}

template<class T>
void FollowMovementGenerator<T>::Reset(T &owner)
{
    Initialize(owner);
}

template<class T>
void FollowMovementGenerator<T>::MovementInform(T& /*unit*/)
{
}

template<>
void FollowMovementGenerator<Creature>::MovementInform(Creature& unit)
{
    if (!unit.isAlive())
        return;

    // Pass back the GUIDLow of the target. If it is pet's owner then PetAI will handle
    if (unit.AI())
        unit.AI()->MovementInform(FOLLOW_MOTION_TYPE, i_target.getTarget()->GetGUIDLow());

    if (unit.IsTemporarySummon())
    {
        TemporarySummon* pSummon = (TemporarySummon*)(&unit);
        if (pSummon->GetSummonerGuid().IsCreature())
        {
            if (Creature* pSummoner = unit.GetMap()->GetCreature(pSummon->GetSummonerGuid()))
                if (pSummoner->AI())
                    pSummoner->AI()->SummonedMovementInform(&unit, FOLLOW_MOTION_TYPE, i_target.getTarget()->GetGUIDLow());
        }
        else
        {
            if (GameObject* pSummoner = unit.GetMap()->GetGameObject(pSummon->GetSummonerGuid()))
                if (pSummoner->AI())
                    pSummoner->AI()->SummonedMovementInform(&unit, FOLLOW_MOTION_TYPE, i_target.getTarget()->GetGUIDLow());
        }
    }
}

//-----------------------------------------------//
template void TargetedMovementGeneratorMedium<Player, ChaseMovementGenerator<Player> >::_setTargetLocation(Player &);
template void TargetedMovementGeneratorMedium<Player, FollowMovementGenerator<Player> >::_setTargetLocation(Player &);
template void TargetedMovementGeneratorMedium<Creature, ChaseMovementGenerator<Creature> >::_setTargetLocation(Creature &);
template void TargetedMovementGeneratorMedium<Creature, FollowMovementGenerator<Creature> >::_setTargetLocation(Creature &);
template void TargetedMovementGeneratorMedium<Player, ChaseMovementGenerator<Player> >::UpdateAsync(Player &, uint32);
template void TargetedMovementGeneratorMedium<Player, FollowMovementGenerator<Player> >::UpdateAsync(Player &, uint32);
template void TargetedMovementGeneratorMedium<Creature, ChaseMovementGenerator<Creature> >::UpdateAsync(Creature &, uint32);
template void TargetedMovementGeneratorMedium<Creature, FollowMovementGenerator<Creature> >::UpdateAsync(Creature &, uint32);

template bool ChaseMovementGenerator<Player>::Update(Player &, const uint32 &);
template bool ChaseMovementGenerator<Creature>::Update(Creature &, const uint32 &);
template void ChaseMovementGenerator<Player>::_reachTarget(Player &);
template void ChaseMovementGenerator<Creature>::_reachTarget(Creature &);
template void ChaseMovementGenerator<Player>::Finalize(Player &);
template void ChaseMovementGenerator<Creature>::Finalize(Creature &);
template void ChaseMovementGenerator<Player>::Interrupt(Player &);
template void ChaseMovementGenerator<Creature>::Interrupt(Creature &);
template void ChaseMovementGenerator<Player>::Reset(Player &);
template void ChaseMovementGenerator<Creature>::Reset(Creature &);
template void ChaseMovementGenerator<Player>::MovementInform(Player&);

template bool FollowMovementGenerator<Player>::Update(Player &, const uint32 &);
template bool FollowMovementGenerator<Creature>::Update(Creature &, const uint32 &);
template void FollowMovementGenerator<Player>::Finalize(Player &);
template void FollowMovementGenerator<Creature>::Finalize(Creature &);
template void FollowMovementGenerator<Player>::Interrupt(Player &);
template void FollowMovementGenerator<Creature>::Interrupt(Creature &);
template void FollowMovementGenerator<Player>::Reset(Player &);
template void FollowMovementGenerator<Creature>::Reset(Creature &);
template void FollowMovementGenerator<Player>::MovementInform(Player&);
