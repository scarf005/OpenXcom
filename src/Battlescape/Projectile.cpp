/*
 * Copyright 2010-2016 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "Projectile.h"
#include "BattlescapeState.h"
#include "TileEngine.h"
#include "Map.h"
#include "Particle.h"
#include "Pathfinding.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleItem.h"
#include "../Mod/MapData.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "../Engine/RNG.h"
#include "../Engine/Options.h"
#include "../fmath.h"

namespace OpenXcom
{

/**
 * Sets up a UnitSprite with the specified size and position.
 * @param mod Pointer to mod.
 * @param save Pointer to battle savegame.
 * @param action An action.
 * @param origin Position the projectile originates from.
 * @param targetVoxel Position the projectile is targeting.
 * @param ammo the ammo that produced this projectile, where applicable.
 */
Projectile::Projectile(Mod *mod, SavedBattleGame *save, BattleAction action, Position origin, Position targetVoxel, BattleItem *ammo) : _mod(mod), _save(save), _action(action), _origin(origin), _targetVoxel(targetVoxel), _position(0), _distance(0.0f), _bulletSprite(-1), _reversed(false), _vaporColor(-1), _vaporDensity(-1), _vaporProbability(5)
{
	// this is the number of pixels the sprite will move between frames
	_speed = Options::battleFireSpeed;
	if (_action.weapon)
	{
		if (_action.type != BA_THROW)
		{
			// try to get all the required info from the ammo, if present
			if (ammo)
			{
				_bulletSprite = ammo->getRules()->getBulletSprite();
				_vaporColor = ammo->getRules()->getVaporColor(_save->getDepth());
				_vaporDensity = ammo->getRules()->getVaporDensity(_save->getDepth());
				_vaporProbability = ammo->getRules()->getVaporProbability(_save->getDepth());
				_speed = std::max(1, _speed + ammo->getRules()->getBulletSpeed());
			}

			// no ammo, or the ammo didn't contain the info we wanted, see what the weapon has on offer.
			if (_bulletSprite == Mod::NO_SURFACE)
			{
				_bulletSprite = _action.weapon->getRules()->getBulletSprite();
			}
			if (_vaporColor == -1)
			{
				_vaporColor = _action.weapon->getRules()->getVaporColor(_save->getDepth());
			}
			if (_vaporDensity == -1)
			{
				_vaporDensity = _action.weapon->getRules()->getVaporDensity(_save->getDepth());
			}
			if (_vaporProbability == 5)
			{
				_vaporProbability = _action.weapon->getRules()->getVaporProbability(_save->getDepth());
			}
			if (!ammo || (ammo != _action.weapon || ammo->getRules()->getBulletSpeed() == 0))
			{
				_speed = std::max(1, _speed + _action.weapon->getRules()->getBulletSpeed());
			}
		}
	}
	if ((targetVoxel.x - origin.x) + (targetVoxel.y - origin.y) >= 0)
	{
		_reversed = true;
	}
}

/**
 * Deletes the Projectile.
 */
Projectile::~Projectile()
{

}

/**
 * Calculates the trajectory for a straight path.
 * @param accuracy The unit's accuracy.
 * @return The objectnumber(0-3) or unit(4) or out of map (5) or -1 (no line of fire).
 */

int Projectile::calculateTrajectory(double accuracy)
{
	Position originVoxel = _save->getTileEngine()->getOriginVoxel(_action, _save->getTile(_origin));
	return calculateTrajectory(accuracy, originVoxel);
}

int Projectile::calculateTrajectory(double accuracy, const Position& originVoxel, bool excludeUnit)
{
	Tile *targetTile = _save->getTile(_action.target);
	BattleUnit *bu = _action.actor;

	_distance = 0.0f;
	int test;
	if (excludeUnit)
	{
		test = _save->getTileEngine()->calculateLineVoxel(originVoxel, _targetVoxel, false, &_trajectory, bu);
	}
	else
	{
		test = _save->getTileEngine()->calculateLineVoxel(originVoxel, _targetVoxel, false, &_trajectory, nullptr);
	}

	if (test != V_EMPTY &&
		!_trajectory.empty() &&
		_action.actor->getFaction() == FACTION_PLAYER &&
		_action.autoShotCounter == 1 &&
		(!_save->isCtrlPressed(true) || !Options::forceFire) &&
		_save->getBattleGame()->getPanicHandled() &&
		_action.type != BA_LAUNCH &&
		!_action.sprayTargeting)
	{
		Position hitPos = _trajectory.at(0).toTile();
		if (test == V_UNIT && _save->getTile(hitPos) && _save->getTile(hitPos)->getUnit() == 0) //no unit? must be lower
		{
			hitPos = Position(hitPos.x, hitPos.y, hitPos.z-1);
		}

		if (hitPos != _action.target && _action.result.empty())
		{
			if (test == V_NORTHWALL)
			{
				if (hitPos.y - 1 != _action.target.y)
				{
					_trajectory.clear();
					return V_EMPTY;
				}
			}
			else if (test == V_WESTWALL)
			{
				if (hitPos.x - 1 != _action.target.x)
				{
					_trajectory.clear();
					return V_EMPTY;
				}
			}
			else if (test == V_UNIT)
			{
				BattleUnit *hitUnit = _save->getTile(hitPos)->getUnit();
				BattleUnit *targetUnit = targetTile->getUnit();
				if (hitUnit != targetUnit)
				{
					_trajectory.clear();
					return V_EMPTY;
				}
			}
			else
			{
				_trajectory.clear();
				return V_EMPTY;
			}
		}
	}

	_trajectory.clear();

	bool extendLine = true;
	// even guided missiles drift, but how much is based on
	// the shooter's faction, rather than accuracy.
	if (_action.type == BA_LAUNCH)
	{
		if (_action.actor->getFaction() == FACTION_PLAYER)
		{
			accuracy = 0.60;
		}
		else
		{
			accuracy = 0.55;
		}
		extendLine = _action.waypoints.size() <= 1;
	}

	// apply some accuracy modifiers.
	// This will results in a new target voxel
	applyAccuracy(originVoxel, &_targetVoxel, accuracy, false, extendLine);

	// finally do a line calculation and store this trajectory.
	return _save->getTileEngine()->calculateLineVoxel(originVoxel, _targetVoxel, true, &_trajectory, bu);
}

/**
 * Calculates the trajectory for a curved path.
 * @param accuracy The unit's accuracy.
 * @return True when a trajectory is possible.
 */
int Projectile::calculateThrow(double accuracy)
{
	Tile *targetTile = _save->getTile(_action.target);

	Position originVoxel = _save->getTileEngine()->getOriginVoxel(_action, 0);
	Position targetVoxel;
	std::vector<Position> targets;
	double curvature;
	targetVoxel = _action.target.toVoxel() + Position(8,8, (1 + -targetTile->getTerrainLevel()));
	targets.clear();
	bool forced = false;

	if (_action.type == BA_THROW)
	{
		targets.push_back(targetVoxel);
	}
	else
	{
		BattleUnit *tu = targetTile->getOverlappingUnit(_save);
		if (Options::forceFire && _save->isCtrlPressed(true) && _save->getSide() == FACTION_PLAYER)
		{
			targets.push_back(_action.target.toVoxel() + Position(0, 0, 12));
			forced = true;
		}
		else if (tu && ((_action.actor->getFaction() != FACTION_PLAYER) ||
			tu->getVisible()))
		{ //unit
			targetVoxel.z += tu->getFloatHeight(); //ground level is the base
			targets.push_back(targetVoxel + Position(0, 0, tu->getHeight()/2 + 1));
			targets.push_back(targetVoxel + Position(0, 0, 2));
			targets.push_back(targetVoxel + Position(0, 0, tu->getHeight() - 1));
		}
		else if (targetTile->getMapData(O_OBJECT) != 0)
		{
			targetVoxel = _action.target.toVoxel() + Position(8,8,0);
			targets.push_back(targetVoxel + Position(0, 0, 13));
			targets.push_back(targetVoxel + Position(0, 0, 8));
			targets.push_back(targetVoxel + Position(0, 0, 23));
			targets.push_back(targetVoxel + Position(0, 0, 2));
		}
		else if (targetTile->getMapData(O_NORTHWALL) != 0)
		{
			targetVoxel = _action.target.toVoxel() + Position(8,0,0);
			targets.push_back(targetVoxel + Position(0, 0, 13));
			targets.push_back(targetVoxel + Position(0, 0, 8));
			targets.push_back(targetVoxel + Position(0, 0, 20));
			targets.push_back(targetVoxel + Position(0, 0, 3));
		}
		else if (targetTile->getMapData(O_WESTWALL) != 0)
 		{
			targetVoxel = _action.target.toVoxel() + Position(0,8,0);
			targets.push_back(targetVoxel + Position(0, 0, 13));
			targets.push_back(targetVoxel + Position(0, 0, 8));
			targets.push_back(targetVoxel + Position(0, 0, 20));
			targets.push_back(targetVoxel + Position(0, 0, 2));
		}
		else if (targetTile->getMapData(O_FLOOR) != 0)
		{
			targets.push_back(targetVoxel);
		}
	}

	_distance = 0.0f;
	int test = V_OUTOFBOUNDS;
	for (const auto& pos : targets)
	{
		targetVoxel = pos;
		if (_save->getTileEngine()->validateThrow(_action, originVoxel, targetVoxel, _save->getDepth(), &curvature, &test, forced))
		{
			break;
		}
	}
	if (!forced && test == V_OUTOFBOUNDS) return test; //no line of fire

	test = V_OUTOFBOUNDS;
	int tries = 0;
	// finally do a line calculation and store this trajectory, make sure it's valid.
	while (test == V_OUTOFBOUNDS && tries < 100)
	{
		++tries;
		Position deltas = targetVoxel;
		// apply some accuracy modifiers
		_trajectory.clear();
		if (_action.type == BA_THROW)
		{
			applyAccuracy(originVoxel, &deltas, accuracy, true, false); //calling for best flavor
			deltas -= targetVoxel;
		}
		else
		{
			applyAccuracy(originVoxel, &targetVoxel, accuracy, true, false); //arcing shot deviation
			deltas = Position(0,0,0);
		}


		test = _save->getTileEngine()->calculateParabolaVoxel(originVoxel, targetVoxel, true, &_trajectory, _action.actor, curvature, deltas);
		if (forced) return O_OBJECT; //fake hit
		Position endPoint = getPositionFromEnd(_trajectory, ItemDropVoxelOffset).toTile();
		Tile *endTile = _save->getTile(endPoint);
		// check if the item would land on a tile with a blocking object
		if (_action.type == BA_THROW
			&& endTile
			&& endTile->getMapData(O_OBJECT)
			&& endTile->getMapData(O_OBJECT)->getTUCost(MT_WALK) == Pathfinding::INVALID_MOVE_COST
			&& !(endTile->isBigWall() && (endTile->getMapData(O_OBJECT)->getBigWall()<1 || endTile->getMapData(O_OBJECT)->getBigWall()>3)))
		{
			test = V_OUTOFBOUNDS;
		}
	}
	return test;
}

/**
 * Calculates the new target in voxel space, based on the given accuracy modifier.
 * @param origin Start position of the trajectory in voxels.
 * @param target Endpoint of the trajectory in voxels.
 * @param accuracy Accuracy modifier.
 * @param keepRange Whether range affects accuracy.
 * @param extendLine should this line get extended to maximum distance?
 */
void Projectile::applyAccuracy(Position origin, Position *target, double accuracy, bool keepRange, bool extendLine)
{
	int xdiff = origin.x - target->x;
	int ydiff = origin.y - target->y;
	double realDistance = sqrt((double)(xdiff*xdiff)+(double)(ydiff*ydiff));
	double tilesDistance = realDistance / 16;
	// maxRange is the maximum range a projectile shall ever travel in voxel space
	double maxRange = keepRange?realDistance:16*1000; // 1000 tiles
	maxRange = _action.type == BA_HIT?46:maxRange; // up to 2 tiles diagonally (as in the case of reaper v reaper)
	const RuleItem *weapon = _action.weapon->getRules();

	// Apply short/long-range limits penalty
	if (_action.type != BA_THROW && _action.type != BA_HIT)
	{
		double modifier = 0.0;
		int upperLimit = weapon->getAimRange();
		int lowerLimit = weapon->getMinRange();
		if (Options::battleUFOExtenderAccuracy)
		{
			if (_action.type == BA_AUTOSHOT)
			{
				upperLimit = weapon->getAutoRange();
			}
			else if (_action.type == BA_SNAPSHOT)
			{
				upperLimit = weapon->getSnapRange();
			}
		}
		if (tilesDistance < lowerLimit)
		{
			modifier = (weapon->getDropoff() * (lowerLimit - tilesDistance)) / 100;
		}
		else if (upperLimit < tilesDistance)
		{
			modifier = (weapon->getDropoff() * (tilesDistance - upperLimit)) / 100;
		}
		accuracy = std::max(0.0, accuracy - modifier);
	}

	// Apply penalty for having no LOS to target
	int noLOSAccuracyPenalty = _action.weapon->getRules()->getNoLOSAccuracyPenalty(_mod);
	if (noLOSAccuracyPenalty != -1)
	{
		Tile *t = _save->getTile(target->toTile());
		if (t)
		{
			bool hasLOS = false;
			BattleUnit *bu = _action.actor;
			BattleUnit *targetUnit = t->getOverlappingUnit(_save);

			if (targetUnit)
			{
				hasLOS = _save->getTileEngine()->visible(bu, t);
			}
			else
			{
				hasLOS = _save->getTileEngine()->isTileInLOS(&_action, t);
			}

			if (!hasLOS)
			{
				accuracy = accuracy * noLOSAccuracyPenalty / 100;
			}
		}
	}

	if (Options::battleRealisticAccuracy)
	{
		int distance_in_tiles = 0;
		double exposure = 0.0;
		int real_accuracy = 0; // separate variable for realistic accuracy, just in case

		BattleUnit *shooterUnit = _action.actor;
		std::vector<Position> exposedVoxels;

		BattleUnit *targetUnit = nullptr;
		Tile *targetTile = _save->getTile(target->toTile());
		if (targetTile) targetUnit = targetTile->getOverlappingUnit(_save);

		if (targetUnit && targetUnit == shooterUnit) // We don't want to hurt ourselves, right?
		{
			targetUnit = nullptr;
		}

		if (targetUnit) // Do we still target a unit?
		{
			exposure = _save->getTileEngine()->checkVoxelExposure(&origin, targetTile, shooterUnit, &exposedVoxels);
			real_accuracy = (int)ceil((double)accuracy * exposure * 100);
		}
		else
			real_accuracy = (int)ceil(accuracy * 100); // ...or just an empty terrain tile?

		if (targetUnit && exposedVoxels.empty()) // We target a unit but can't get LOF
		{
			real_accuracy = 0;
		}
		else if (targetTile)
		{
			int deltaX = origin.x/16 - targetTile->getPosition().x;
			int deltaY = origin.y/16 - targetTile->getPosition().y;
			double deltaZ = (origin.z/24 - targetTile->getPosition().z)*1.5;
			distance_in_tiles = (int)floor(sqrt((double)(deltaX*deltaX + deltaY*deltaY + deltaZ*deltaZ))); // Distance in cube 16x16x16 tiles!

			if (distance_in_tiles==0)
				real_accuracy = 100;

			if (weapon->getMinRange() == 0 && _action.type == BA_AIMEDSHOT && distance_in_tiles <= 10) // For aimed shot...
			{
				if (real_accuracy*2 >= 100) // Use one of two algorithms, depending on which one gives better numbers
					real_accuracy = std::min(100, (int)ceil(real_accuracy*(2-((double)distance_in_tiles-1)/10))); // Multiplier x1.1..x2 for 10 tiles, nearest to target
				else
					real_accuracy += (100 - real_accuracy)/distance_in_tiles; // Or just evenly divide to get 100% accuracy on tile adjanced to a target
			}

			else if (weapon->getMinRange() == 0 && (_action.type == BA_AUTOSHOT || _action.type == BA_SNAPSHOT) && distance_in_tiles <= 5) // For snap/auto
			{
				if (real_accuracy*2 >= 100)
					real_accuracy = std::min(100, (int)ceil(real_accuracy*(2-((double)distance_in_tiles-1)/5))); // Multiplier x1.2..x2 for 5 nearest tiles
				else
					real_accuracy += (100 - real_accuracy)/distance_in_tiles;
			}

			if (real_accuracy < 5) // Rule for difficult/long-range shots
			{
				real_accuracy = 5; // If there's LOF - accuracy should be at least 5%
				if (exposedVoxels.size() > 0 && exposedVoxels.size() < 5) real_accuracy=exposedVoxels.size(); // Except cases when less than 5 voxels exposed
				if (shooterUnit->isKneeled()) real_accuracy += 2; // And let's make kneeling more meaningful for such shots
			}
		}

		int accuracy_check = RNG::generate(1, 100);
		bool hit_successful = accuracy_check <= real_accuracy;

		if (_save->getDebugMode())
		{
			std::ostringstream ss;
			ss << "Acc:" << accuracy*100 << " Exposure " << exposure*100 << "%";
			ss << " Dist:" << distance_in_tiles << " Total:" << real_accuracy << "%";
			ss << " Check:" << accuracy_check << " HIT? " << hit_successful;
			_save->getBattleState()->debug(ss.str());
		}

		// If there's no target unit - do nothing, leave target voxel "as is", as we've successfully hit the terrain, yay!
		if (hit_successful)
		{
			if (targetUnit)	*target = exposedVoxels.at(RNG::generate(0, exposedVoxels.size()-1)); // Aim to random exposed voxel of the target
		}
		else // We missed, time to find MISSING line of fire with realistic deviation
		{
			int heightRange = 0;
			int unitRadius = 4; // Targeting to empty terrain tile will use this size for fire deviation
			int targetSize = 0;

			int targetMinHeight = target->z - target->z%24 - targetTile->getTerrainLevel();
			int targetMaxHeight = targetMinHeight;

			if (targetUnit) // Finding boundaries of target unit
			{
				targetMinHeight += targetUnit->getFloatHeight();
				if (!targetUnit->isOut())
				{
					heightRange = targetUnit->getHeight();
				}
				else
				{
					heightRange = 12;
				}
				targetMaxHeight=targetMinHeight + heightRange;
				unitRadius = targetUnit->getLoftemps(); //width == loft in default loftemps set
				targetSize = targetUnit->getArmor()->getSize() - 1;

				if (targetSize > 0)
				{
					unitRadius = 3;
				}
			}

			Position visibleCenter{0,0,0};
			if (exposedVoxels.empty())
			{
				visibleCenter = *target; // No exposed voxels? Use initial target point as unit center
			}
			else // Find the center of exposed part
			{
				struct // Sint16 overflows, cannot use regular Position-type variable
				{
					Sint32 x = 0;
					Sint32 y = 0;
					Sint32 z = 0;
				} temp;
				for (const Position &vox : exposedVoxels) // Sum all exposed voxels to a single point with HUGE coordinates
				{
					temp.x += vox.x;
					temp.y += vox.y;
					temp.z += vox.z;
				}
				visibleCenter.x = (int)round((double)temp.x / exposedVoxels.size()); // Find arithmetic mean of all exposed voxels
				visibleCenter.y = (int)round((double)temp.y / exposedVoxels.size());
				visibleCenter.z = (int)round((double)temp.z / exposedVoxels.size());
			}

			Position deviate;
			std::vector<Position> trajectory;
			int accuracy_deviation = (accuracy_check - real_accuracy)/4; //  Highly accurate shots will land close to the target even if they miss
			int distance_deviation = tilesDistance/7; // 1 voxel of deviation per 7 tiles of distance
			int deviation = unitRadius + accuracy_deviation + distance_deviation;
\
			for ( int i=0; i<5; ++i) // Maximum possible deviation is deviation + 5, in case you're extremely unlucky
			{
				for ( int j=0; j<10; ++j ) // Randomly try to "shoot" to different points around the center of visible part
				{
					deviate = visibleCenter;
					deviate.x += RNG::generate(-deviation,deviation);
					deviate.y += RNG::generate(-deviation,deviation);
					deviate.z += RNG::generate(-deviation,deviation);

					// if the point is between shooter and target - we don't like it, look for the next one
					// we need a point close to normal to LOS, or behind that normal
					if (Position::distanceSq(origin, deviate) < Position::distanceSq(origin,visibleCenter)) continue;

					trajectory.clear();
					int test = _save->getTileEngine()->calculateLineVoxel(origin, deviate, false, &trajectory, shooterUnit);
					if (test != V_UNIT) // We successfully missed the target, use the point we found
					{
						*target = deviate;
						goto target_calculated;
					}
					else if (targetUnit && !trajectory.empty()) // Hit some unit eh?
					{
						int xOffset = targetUnit->getPosition().x - deviate.x/16;
						int yOffset = targetUnit->getPosition().y - deviate.y/16;
						for (int x = 0; x <= targetSize; ++x)
						{
							for (int y = 0; y <= targetSize; ++y)
							{
								//voxel of hit must be outside of scanned box
								if (trajectory.at(0).x/16 != (deviate.x/16) + x + xOffset || // If shot impact point is outside our target
									trajectory.at(0).y/16 != (deviate.y/16) + y + yOffset || // ...it means we missed it, success!
									trajectory.at(0).z < targetMinHeight ||
									trajectory.at(0).z > targetMaxHeight)
								{
									*target = deviate;
									goto target_calculated;
								}
							}
						}
					}
					else // Just in case of something unexpected
					{
						*target = deviate;
						goto target_calculated;
					}
				}
				++deviation; // Tried to miss many times but failed? Increase the deviation slightly and try again
			}
			target->z -= target->z%24; // Can't miss after even more tries? Just shoot to the ground under target and call it a day
		}
	}
	else // "Classic" XCOM
	{
		int xDist = abs(origin.x - target->x);
		int yDist = abs(origin.y - target->y);
		int zDist = abs(origin.z - target->z);
		int xyShift, zShift;

		if (xDist / 2 <= yDist)				//yes, we need to add some x/y non-uniformity
			xyShift = xDist / 4 + yDist;	//and don't ask why, please. it's The Commandment
		else
			xyShift = (xDist + yDist) / 2;	//that's uniform part of spreading

		if (xyShift <= zDist)				//slight z deviation
			zShift = xyShift / 2 + zDist;
		else
			zShift = xyShift + zDist / 2;

		int deviation = RNG::generate(0, 100) - (accuracy * 100);

		if (deviation >= 0)
			deviation += 50;				// add extra spread to "miss" cloud
		else
			deviation += 10;				//accuracy of 109 or greater will become 1 (tightest spread)

		deviation = std::max(1, zShift * deviation / 200);	//range ratio

		target->x += RNG::generate(0, deviation) - deviation / 2;
		target->y += RNG::generate(0, deviation) - deviation / 2;
		target->z += RNG::generate(0, deviation / 2) / 2 - deviation / 8;
	}

target_calculated:

	if (extendLine)
	{
		double rotation, tilt;
		rotation = atan2(double(target->y - origin.y), double(target->x - origin.x)) * 180 / M_PI;
		tilt = atan2(double(target->z - origin.z),
			sqrt(double(target->x - origin.x)*double(target->x - origin.x)+double(target->y - origin.y)*double(target->y - origin.y))) * 180 / M_PI;
		// calculate new target
		// this new target can be very far out of the map, but we don't care about that right now
		double cos_fi = cos(Deg2Rad(tilt));
		double sin_fi = sin(Deg2Rad(tilt));
		double cos_te = cos(Deg2Rad(rotation));
		double sin_te = sin(Deg2Rad(rotation));
		target->x = (int)(origin.x + maxRange * cos_te * cos_fi);
		target->y = (int)(origin.y + maxRange * sin_te * cos_fi);
		target->z = (int)(origin.z + maxRange * sin_fi);
	}
}

/**
 * Moves further in the trajectory.
 * @return false if the trajectory is finished - no new position exists in the trajectory.
 */
bool Projectile::move()
{
	for (int i = 0; i < _speed; ++i)
	{
		_position++;
		if (_position == _trajectory.size())
		{
			_position--;
			return false;
		}
		else if (_position > 1)
		{
			// calc avg of two voxel steps
			_distance += 0.5f * Position::distance(_trajectory[_position], _trajectory[_position - 2]);
		}
		else if (_position > 0)
		{
			_distance += Position::distance(_trajectory[_position], _trajectory[_position - 1]);
		}
		if (_vaporColor != -1 && _action.type != BA_THROW && RNG::percent(_vaporProbability))
		{
			addVaporCloud();
		}
	}
	return true;
}

/**
 * Get Position at offset from start from trajectory vector.
 * @param trajectory Vector that have trajectory.
 * @param pos Offset counted from begining of trajectory.
 * @return Position in voxel space.
 */
Position Projectile::getPositionFromStart(const std::vector<Position>& trajectory, int pos)
{
	if (pos >= 0 && pos < (int)trajectory.size())
		return trajectory.at(pos);
	else if (pos < 0)
		return trajectory.at(0);
	else
		return trajectory.at(trajectory.size() - 1);
}

/**
 * Get Position at offset from start from trajectory vector.
 * @param trajectory Vector that have trajectory.
 * @param pos Offset counted from ending of trajectory.
 * @return Position in voxel space.
 */
Position Projectile::getPositionFromEnd(const std::vector<Position>& trajectory, int pos)
{
	return getPositionFromStart(trajectory, trajectory.size() + pos - 1);
}

/**
 * Gets the current position in voxel space.
 * @param offset Offset.
 * @return Position in voxel space.
 */
Position Projectile::getPosition(int offset) const
{
	return getPositionFromStart(_trajectory, (int)_position + offset);
}

/**
 * Gets a particle reference from the projectile surfaces.
 * @param i Index.
 * @return Particle id.
 */
int Projectile::getParticle(int i) const
{
	if (_bulletSprite != Mod::NO_SURFACE)
		return _bulletSprite + i;
	else
		return Mod::NO_SURFACE;
}

/**
 * Gets the project tile item.
 * Returns 0 when there is no item thrown.
 * @return Pointer to BattleItem.
 */
BattleItem *Projectile::getItem() const
{
	if (_action.type == BA_THROW)
		return _action.weapon;
	else
		return 0;
}

/**
 * Skips to the end of the trajectory.
 */
void Projectile::skipTrajectory()
{
	while (move());
}

/**
 * Gets the Position of origin for the projectile
 * @return origin as a tile position.
 */
Position Projectile::getOrigin() const
{
	// instead of using the actor's position, we'll use the voxel origin translated to a tile position
	// this is a workaround for large units.
	return _trajectory.front().toTile();
}

/**
 * Gets the INTENDED target for this projectile
 * it is important to note that we do not use the final position of the projectile here,
 * but rather the targetted tile
 * @return target as a tile position.
 */
Position Projectile::getTarget() const
{
	return _action.target;
}

/**
 * Gets distances that projectile have traveled until now.
 * @return Returns traveled distance.
 */
float Projectile::getDistance() const
{
	return _distance;
}

/**
 * Is this projectile drawn back to front or front to back?
 * @return return if this is to be drawn in reverse order.
 */
bool Projectile::isReversed() const
{
	return _reversed;
}

/**
 * adds a cloud of vapor at the projectile's current position.
 */
void Projectile::addVaporCloud()
{
	Position voxelPos = _trajectory.at(_position);
	Position tilePos = voxelPos.toTile();
	for (int i = 0; i != _vaporDensity; ++i)
	{
		Particle particle = Particle(voxelPos, RNG::seedless(48, 224), _vaporColor, RNG::seedless(32, 44));
		Position tileOffset = particle.updateScreenPosition();
		_save->getBattleGame()->getMap()->addVaporParticle(tilePos + tileOffset, particle);
	}
}

}
