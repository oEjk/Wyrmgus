//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//         Stratagus - A free fantasy real time strategy game engine
//
//      (c) Copyright 1998-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.

#include "stratagus.h"

#include "unit/unit.h"

#include "action/action_attack.h"
//Wyrmgus start
#include "action/action_resource.h"
#include "action/action_upgradeto.h"
//Wyrmgus end
#include "actions.h"
#include "ai.h"
//Wyrmgus start
#include "ai/ai_local.h" //for using AiHelpers
//Wyrmgus end
#include "animation/animation_set.h"
#include "character.h"
#include "commands.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "economy/resource_storage_type.h"
#include "epithet.h"
#include "game/game.h"
#include "editor.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "item/item_slot.h"
#include "item/persistent_item.h"
#include "item/unique_item.h"
#include "language/name_generator.h"
#include "luacallback.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/map_settings.h"
#include "map/nearby_sight_unmarker.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "map/world.h"
#include "missile.h"
#include "network/network.h"
#include "pathfinder/pathfinder.h"
#include "player/civilization.h"
#include "player/civilization_group.h"
#include "player/faction.h"
#include "player/player.h"
#include "player/player_color.h"
#include "player/player_type.h"
#include "quest/achievement.h"
#include "quest/quest.h"
#include "religion/deity.h"
#include "script.h"
#include "script/condition/and_condition.h"
#include "sound/game_sound_set.h"
#include "sound/sound.h"
#include "sound/sound_server.h"
#include "sound/unitsound.h"
#include "sound/unit_sound_type.h"
#include "species/species.h"
#include "spell/spell.h"
#include "spell/status_effect.h"
#include "time/time_of_day.h"
#include "translator.h"
#include "ui/button.h"
#include "ui/button_cmd.h"
#include "ui/interface.h"
#include "ui/ui.h"
#include "unit/build_restriction/on_top_build_restriction.h"
#include "unit/unit_domain.h"
#include "unit/unit_find.h"
#include "unit/unit_manager.h"
#include "unit/unit_ref.h"
#include "unit/unit_type.h"
#include "unit/unit_type_variation.h"
#include "upgrade/upgrade.h"
#include "upgrade/upgrade_modifier.h"
#include "util/assert_util.h"
#include "util/gender.h"
#include "util/log_util.h"
#include "util/point_util.h"
#include "util/rect_util.h"
#include "util/size_util.h"
#include "util/string_util.h"
//Wyrmgus start
#include "util/util.h"
//Wyrmgus end
#include "util/vector_random_util.h"
#include "util/vector_util.h"
#include "video/video.h"

/**
**  @class CUnit unit.h
**
**  \#include "unit/unit.h"
**
**  Everything belonging to a unit. FIXME: rearrange for less memory.
**
**  This class contains all information about a unit in game.
**  A unit could be anything: a man, a vehicle, a ship, or a building.
**  Currently only a tile, a unit, or a missile could be placed on the map.
**
**  The unit structure members:
**
**  CUnit::Refs
**
**  The reference counter of the unit. If the pointer to the unit
**  is stored the counter must be incremented and if this reference
**  is destroyed the counter must be decremented. Alternative it
**  would be possible to implement a garbage collector for this.
**
**  CUnit::Slot
**
**  This is the unique slot number. It is not possible that two
**  units have the same slot number at the same time. The slot
**  numbers are reused.
**  This field could be accessed by the macro UnitNumber(Unit *).
**
**  CUnit::UnitSlot
**
**  This is the index into #Units[], where the unit pointer is
**  stored.  #Units[] is a table of all units currently active in
**  game. This pointer is only needed to speed up, the remove of
**  the unit pointer from #Units[], it didn't must be searched in
**  the table.
**
**  CUnit::PlayerSlot
**
**  The index into Player::Units[], where the unit pointer is
**  stored. Player::Units[] is a table of all units currently
**  belonging to a player. This pointer is only needed to speed
**  up, the remove of the unit pointer from Player::Units[].
**
**  CUnit::Container
**
**  Pointer to the unit containing it, or null if the unit is
**  free. This points to the transporter for units on board, or to
**  the building for peasants inside(when they are mining).
**
**  CUnit::BoardCount
**
**  The number of units transported inside the container. This
**  does not include for instance stuff like harvesters returning
**  cargo.
**
**  CUnit::tilePos
**
**  The tile map coordinates of the unit.
**  0,0 is the upper left on the map.
**
**  CUnit::Type
**
**  Pointer to the unit-type (::UnitType). The unit-type contains
**  all information that all units of the same type shares.
**  (Animations, Name, Stats, ...)
**
**  CUnit::SeenType
**  Pointer to the unit-type that this unit was, when last seen.
**  Currently only used by buildings.
**
**  CUnit::Player
**
**  Pointer to the owner of this unit (::Player). An unit could
**  only be owned by one player.
**
**  CUnit::Stats
**
**  Pointer to the current status (::UnitStats) of a unit. The
**  units of the same player and the same type could share the same
**  stats. The status contains all values which could be different
**  for each player. This f.e. the upgradeable abilities of an
**  unit.  (CUnit::Stats::SightRange, CUnit::Stats::Armor,
**  CUnit::Stats::HitPoints, ...)
**
**  CUnit::CurrentSightRange
**
**  Current sight range of a unit, this changes when a unit enters
**  a transporter or building or exits one of these.
**
**  CUnit::Colors
**
**  Player colors of the unit. Contains the hardware dependent
**  pixel values for the player colors (palette index #208-#211).
**  Setup from the global palette. This is a pointer.
**  @note Index #208-#211 are various SHADES of the team color
**  (#208 is brightest shade, #211 is darkest shade) .... these
**  numbers are NOT red=#208, blue=#209, etc
**
**  CUnit::IX CUnit::IY
**
**  Coordinate displacement in pixels or coordinates inside a tile.
**  Currently only !=0, if the unit is moving from one tile to
**  another (0-32 and for ships/flyers 0-64).
**
**  CUnit::Frame
**
**  Current graphic image of the animation sequence. The high bit
**  (128) is used to flip this image horizontal (x direction).
**  This also limits the number of different frames/image to 126.
**
**  CUnit::SeenFrame
**
**  Graphic image (see CUnit::Frame) what the player on this
**  computer has last seen. If UnitNotSeen the player haven't seen
**  this unit yet.
**
**  CUnit::Direction
**
**  Contains the binary angle (0-255) in which the direction the
**  unit looks. 0, 32, 64, 128, 160, 192, 224, 256 corresponds to
**  0?, 45?, 90?, 135?, 180?, 225?, 270?, 315?, 360? or north,
**  north-east, east, south-east, south, south-west, west,
**  north-west, north. Currently only 8 directions are used, this
**  is more for the future.
**
**  CUnit::Attacked
**
**  Last cycle the unit was attacked. 0 means never.
**
**  CUnit::Burning
**
**  If Burning is non-zero, the unit is burning.
**
**  CUnit::VisCount[PlayerMax]
**
**              Used to keep track of visible units on the map, it counts the
**              Number of seen tiles for each player. This is only modified
**              in UnitsMarkSeen and UnitsUnmarkSeen, from fow.
**              We keep track of visilibty for each player, and combine with
**              Shared vision ONLY when querying and such.
**
**  CUnit::SeenByPlayer
**
**              This is a bitmask of 1 and 0 values. SeenByPlayer & (1<<p) is 0
**              If p never saw the unit and 1 if it did. This is important for
**              keeping track of dead units under fog. We only keep track of units
**              that are visible under fog with this.
**
**  CUnit::Removed
**
**  This flag means the unit is not active on map. This flag
**  have workers set if they are inside a building, units that are
**  on board of a transporter.
**
**  CUnit::Selected
**
**  Unit is selected. (So you can give it orders)
**
**  CUnit::UnderConstruction
**  Set when a building is under construction, and still using the
**  generic building animation.
**
**  CUnit::SeenUnderConstruction
**  Last seen state of construction.  Used to draw correct building
**  frame. See CUnit::UnderConstruction for more information.
**
**  CUnit::SeenState
**  The Seen State of the building.
**  01 The building in being built when last seen.
**  10 The building was been upgraded when last seen.
**
**  CUnit::Boarded
**
**  This is 1 if the unit is on board a transporter.
**
**
**  CUnit::XP
**
**  Number of XP of the unit.
**
**  CUnit::Kills
**
**  How many units have been killed by the unit.
**
**  CUnit::GroupId
**
**  Number of the group to that the unit belongs. This is the main
**  group showed on map, a unit can belong to many groups.
**
**  CUnit::LastGroup
**
**  Automatic group number, to reselect the same units. When the
**  user selects more than one unit all units is given the next
**  same number. (Used for ALT-CLICK)
**
**  CUnit::Value
**
**  This values hold the amount of resources in a resource or in
**  in a harvester.
**  @todo continue documentation
**
**  CUnit::Wait
**
**  The unit is forced too wait for that many cycles. Be careful,
**  setting this to 0 will lock the unit.
**
**  CUnit::State
**
**  Animation state, currently position in the animation script.
**  0 if an animation has just started, it should only be changed
**  inside of actions.
**
**  CUnit::Orders
**
**  Contains all orders of the unit. Slot 0 is always used.
**
**  CUnit::SavedOrder
**
**  This order is executed, if the current order is finished.
**  This is used for attacking units, to return to the old
**  place or for patrolling units to return to patrol after
**  killing some enemies. Any new order given to the unit,
**  clears this saved order.
**
**  CUnit::NewOrder
**
**  This field is only used by buildings and this order is
**  assigned to any by this building new trained unit.
**  This is can be used to set the exit or gathering point of a
**  building.
**
**  CUnit::Goal
**
**  Generic goal pointer. Used by teleporters to point to circle of power.
**
**
** @todo continue documentation
**
*/

bool EnableTrainingQueue;                 /// Config: training queues enabled
bool RevealAttacker;                      /// Config: reveal attacker enabled
int ResourcesMultiBuildersMultiplier = 0; /// Config: spend resources for building with multiple workers

static unsigned long HelpMeLastCycle;     /// Last cycle HelpMe sound played
static int HelpMeLastX;                   /// Last X coordinate HelpMe sound played
static int HelpMeLastY;                   /// Last Y coordinate HelpMe sound played

static void RemoveUnitFromContainer(CUnit &unit);

extern int ExtraDeathIndex(const char *death);

CUnit::CUnit()
{
	this->Init();
}

CUnit::~CUnit()
{
}

void CUnit::Init()
{
	if (this->base_ref != nullptr) {
		throw std::runtime_error("Unit being initialized has a base reference.");
	}

	this->ref.reset();
	this->ReleaseCycle = 0;
	this->PlayerSlot = static_cast<size_t>(-1);
	this->units_inside.clear();
	this->BoardCount = 0;
	this->Container = nullptr;

	this->Resource.Workers.clear();
	this->Resource.Active = 0;

	this->garrisoned_gathering_income = 0;
	
	for (int i = 0; i < static_cast<int>(wyrmgus::item_slot::count); ++i) {
		this->EquippedItems[i].clear();
	}
	this->SoldUnits.clear();

	tilePos.x = 0;
	tilePos.y = 0;
	//Wyrmgus start
	this->rally_point_pos = QPoint(-1, -1);
	this->MapLayer = nullptr;
	this->rally_point_map_layer = nullptr;
	//Wyrmgus end
	Offset = 0;
	this->Type = nullptr;
	Player = nullptr;
	Stats = nullptr;
	//Wyrmgus start
	this->character = nullptr;
	this->settlement = nullptr;
	this->site = nullptr;
	this->traits.clear();
	this->Prefix = nullptr;
	this->Suffix = nullptr;
	this->Spell = nullptr;
	this->Work = nullptr;
	this->Elixir = nullptr;
	this->unique = nullptr;
	this->Bound = false;
	this->Identified = true;
	this->ConnectingDestination = nullptr;
	//Wyrmgus end
	CurrentSightRange = 0;
	this->best_contained_unit_attack_range = 0;

	pathFinderData = std::make_unique<PathFinderData>();
	pathFinderData->input.SetUnit(*this);

	//Wyrmgus start
	this->Name.clear();
	this->epithet = nullptr;
	this->surname.clear();
	Variation = 0;
	memset(LayerVariation, -1, sizeof(LayerVariation));
	//Wyrmgus end
	this->pixel_offset = QPoint(0, 0);
	Frame = 0;
	Direction = 0;
	DamagedType = ANIMATIONS_DEATHTYPES;
	Attacked = 0;
	Burning = 0;
	Destroyed = 0;
	Removed = 0;
	Selected = 0;
	TeamSelected = 0;
	UnderConstruction = 0;
	Active = 0;
	Boarded = 0;
	this->player_from = nullptr;
	this->VisCount.fill(0);
	this->Seen = _seen_stuff_();
	this->Variable.clear();
	TTL = 0;
	Threshold = 0;
	GroupId = 0;
	LastGroup = 0;
	ResourcesHeld = 0;
	Wait = 0;
	Blink = 0;
	Moving = 0;
	ReCast = 0;
	CacheLock = 0;
	Summoned = 0;
	Waiting = 0;
	MineLow = 0;
	Anim = _unit_anim_();
	WaitBackup = _unit_anim_();
	this->GivesResource = 0;
	this->CurrentResource = 0;
	this->reset_step_count();
	this->Orders.clear();
	this->clear_special_orders();
	this->autocast_spells.clear();
	this->spell_cooldown_timers.clear();
	this->AutoRepair = 0;
	this->Goal = nullptr;
	this->IndividualUpgrades.clear();
}

/**
**  Release an unit.
**
**  The unit is only released, if all references are dropped.
*/
void CUnit::Release(const bool final)
{
	if (this->ReleaseCycle != 0) {
		throw std::runtime_error("Unit already free.");
	}

	if (this->Type == nullptr) {
		throw std::runtime_error("Unit being released has no type.");
	}

	if (this->Orders.size() > 1) {
		log::log_error("Unit to be released has more than 1 order; Unit Type: \"" + this->Type->get_identifier() + "\", Orders: " + std::to_string(this->Orders.size()) + ", First Order Type: " + std::to_string(static_cast<int>(this->CurrentAction())) + ".");
	}

	assert_throw(Orders.size() == 1);
	// Must be removed before here
	assert_throw(Removed);

	// First release, remove from lists/tables.
	if (!Destroyed) {
		DebugPrint("%d: First release %d\n" _C_ this->Player->get_index() _C_ UnitNumber(*this));

		// Are more references remaining?
		this->Destroyed = 1; // mark as destroyed

		if (Container && !final) {
			if (Boarded) {
				Container->BoardCount--;
			}
			MapUnmarkUnitSight(*this);
			RemoveUnitFromContainer(*this);
		}

		this->Resource.Workers.clear();
		this->clear_all_orders();

		if (this->get_ref_count() == 0) {
			throw std::runtime_error("Unit is having its base reference cleared for release, despite its reference count already being 0.");
		}

		this->clear_base_reference();
		return;
	}

	if (this->get_ref_count() != 0) {
		throw std::runtime_error("Unit of type \"" + this->Type->get_identifier() + "\" being released despite there still being " + std::to_string(this->get_ref_count()) + " references to it.");
	}

	// No more references remaining, but the network could have an order
	// on the way. We must wait a little time before we could free the
	// memory.

	//Wyrmgus start
	this->character = nullptr;
	this->set_settlement(nullptr);
	this->set_site(nullptr);
	this->unique = nullptr;
	this->ConnectingDestination = nullptr;
	
	for (int i = 0; i < static_cast<int>(wyrmgus::item_slot::count); ++i) {
		this->EquippedItems[i].clear();
	}
	this->SoldUnits.clear();
	//Wyrmgus end

	this->pathFinderData.reset();
	this->autocast_spells.clear();
	this->spell_cooldown_timers.clear();
	this->Variable.clear();
	this->Orders.clear();
	this->clear_special_orders();

	// Remove the unit from the global units table.
	unit_manager::get()->ReleaseUnit(this);
}

std::shared_ptr<wyrmgus::unit_ref> CUnit::acquire_ref() const
{
	if (this->base_ref == nullptr) {
		throw std::runtime_error("Tried to acquire a reference to a unit which already had its base reference cleared.");
	}

	if (!SaveGameLoading) {
		if (this->Destroyed) {
			throw std::runtime_error("Tried to acquire a reference to a unit which has already been destroyed.");
		}
	}

	return this->base_ref;
}

//Wyrmgus start
void CUnit::SetResourcesHeld(int quantity)
{
	this->ResourcesHeld = quantity;
	
	const wyrmgus::unit_type_variation *variation = this->GetVariation();
	if (
		variation != nullptr
		&& (
			(variation->ResourceMin && this->ResourcesHeld < variation->ResourceMin)
			|| (variation->ResourceMax && this->ResourcesHeld > variation->ResourceMax)
		)
	) {
		this->ChooseVariation();
	}
}

void CUnit::ChangeResourcesHeld(int quantity)
{
	this->SetResourcesHeld(this->ResourcesHeld + quantity);
}

void CUnit::ReplaceOnTop(CUnit &replaced_unit)
{
	if (replaced_unit.get_unique() != nullptr) {
		this->set_unique(replaced_unit.get_unique());
	} else {
		if (replaced_unit.Prefix != nullptr) {
			this->SetPrefix(replaced_unit.Prefix);
		}
		if (replaced_unit.Suffix != nullptr) {
			this->SetSuffix(replaced_unit.Suffix);
		}
		if (replaced_unit.Spell != nullptr) {
			this->SetSpell(replaced_unit.Spell);
		}
	}

	if (replaced_unit.get_settlement() != nullptr) {
		this->set_settlement(replaced_unit.get_settlement());
	}

	if (replaced_unit.get_site() != nullptr) {
		this->set_site(replaced_unit.get_site());

		if (replaced_unit.site->is_settlement()) {
			CMap::get()->remove_settlement_unit(&replaced_unit);
			CMap::get()->add_settlement_unit(this);
		}

		replaced_unit.set_site(nullptr);
	}

	this->SetResourcesHeld(replaced_unit.ResourcesHeld); // We capture the value of what is beneath.
	this->Variable[GIVERESOURCE_INDEX].Value = replaced_unit.Variable[GIVERESOURCE_INDEX].Value;
	this->Variable[GIVERESOURCE_INDEX].Max = replaced_unit.Variable[GIVERESOURCE_INDEX].Max;
	this->Variable[GIVERESOURCE_INDEX].Enable = replaced_unit.Variable[GIVERESOURCE_INDEX].Enable;
	
	replaced_unit.Remove(nullptr); // Destroy building beneath
	UnitLost(replaced_unit);
	replaced_unit.clear_orders();
	replaced_unit.Release();
}

void CUnit::restore_ontop()
{
	// Destroy resource-platform, must re-make resource patch.
	//Wyrmgus start
//	const on_top_build_restriction *b = OnTopDetails(*this, nullptr);
	const on_top_build_restriction *b = OnTopDetails(*this->Type, nullptr);
	//Wyrmgus end
	if (b != nullptr) {
		if (b->ReplaceOnDie && (this->Type->get_given_resource() == nullptr || this->ResourcesHeld != 0) && this->MapLayer != nullptr) {
			CUnit *temp = MakeUnitAndPlace(this->tilePos, *b->Parent, CPlayer::get_neutral_player(), this->MapLayer->ID);
			if (temp == nullptr) {
				DebugPrint("Unable to allocate Unit");
			} else {
				//Wyrmgus start
//				temp->ResourcesHeld = this->ResourcesHeld;
//				temp->Variable[GIVERESOURCE_INDEX].Value = this->Variable[GIVERESOURCE_INDEX].Value;
//				temp->Variable[GIVERESOURCE_INDEX].Max = this->Variable[GIVERESOURCE_INDEX].Max;
//				temp->Variable[GIVERESOURCE_INDEX].Enable = this->Variable[GIVERESOURCE_INDEX].Enable;
				//Wyrmgus end
				//Wyrmgus start
				if (this->get_unique() != nullptr) {
					temp->set_unique(this->get_unique());
				} else {
					if (this->Prefix != nullptr) {
						temp->SetPrefix(this->Prefix);
					}
					if (this->Suffix != nullptr) {
						temp->SetSuffix(this->Suffix);
					}
					if (this->Spell != nullptr) {
						temp->SetSpell(this->Spell);
					}
				}
				if (this->get_site() != nullptr) {
					temp->set_site(this->get_site());

					if (this->get_site()->is_settlement()) {
						temp->set_settlement(this->settlement);
						CMap::get()->remove_settlement_unit(this);
						CMap::get()->add_settlement_unit(temp);
					}

					this->set_site(nullptr);
				}
				if (this->Type->get_given_resource() != nullptr && this->ResourcesHeld != 0) {
					temp->SetResourcesHeld(this->ResourcesHeld);
					temp->Variable[GIVERESOURCE_INDEX].Value = this->Variable[GIVERESOURCE_INDEX].Value;
					temp->Variable[GIVERESOURCE_INDEX].Max = this->Variable[GIVERESOURCE_INDEX].Max;
					temp->Variable[GIVERESOURCE_INDEX].Enable = this->Variable[GIVERESOURCE_INDEX].Enable;
				}
				//Wyrmgus end
			}
			//Wyrmgus start
		} else if (this->get_site() != nullptr && this->get_site()->get_game_data()->get_site_unit() == this) {
			this->get_site()->get_game_data()->set_site_unit(nullptr);
			//Wyrmgus end
		}
	}
}

void CUnit::set_experience(const int amount)
{
	if (!this->Type->can_gain_experience()) {
		return;
	}

	this->Variable[XP_INDEX].Max = amount;

	//prevent experience from becoming negative
	this->Variable[XP_INDEX].Max = std::max(0, this->Variable[XP_INDEX].Max);

	this->Variable[XP_INDEX].Value = this->Variable[XP_INDEX].Max;
	this->XPChanged();
}

void CUnit::change_experience(const int base_amount, const int around_range)
{
	std::vector<CUnit *> table;
	if (around_range > 0) {
		SelectAroundUnit(*this, around_range, table, MakeAndPredicate(HasSamePlayerAs(*this->Player), IsNotBuildingType()));
	}
	
	const int amount = base_amount / (1 + table.size());

	if (this->Type->can_gain_experience()) {
		this->set_experience(this->Variable[XP_INDEX].Max + amount);
	}

	if (around_range > 0) {
		for (CUnit *nearby_unit : table) {
			if (nearby_unit->Type->can_gain_experience()) {
				nearby_unit->set_experience(nearby_unit->Variable[XP_INDEX].Max + amount);
			}
		}
	}
}

void CUnit::IncreaseLevel(int level_quantity, bool automatic_learning)
{
	while (level_quantity > 0) {
		this->Variable[LEVEL_INDEX].Value += 1;
		if (this->Type->Stats[this->Player->get_index()].Variables[LEVEL_INDEX].Value < this->Variable[LEVEL_INDEX].Value) {
			if (GetAvailableLevelUpUpgrades(true) == 0 || (this->Variable[LEVEL_INDEX].Value - this->Type->Stats[this->Player->get_index()].Variables[LEVEL_INDEX].Value) > 1) {
				this->Variable[POINTS_INDEX].Max += 5 * (this->Variable[LEVEL_INDEX].Value + 1);
				this->Variable[POINTS_INDEX].Value += 5 * (this->Variable[LEVEL_INDEX].Value + 1);
			}
			
			this->Variable[LEVELUP_INDEX].Value += 1;
			this->Variable[LEVELUP_INDEX].Max = this->Variable[LEVELUP_INDEX].Value;
			// if there are no level-up upgrades available for the unit, increase its HP instead
			if (this->GetAvailableLevelUpUpgrades() < this->Variable[LEVELUP_INDEX].Value) {
				this->Variable[HP_INDEX].Max += 10;
				this->Variable[LEVELUP_INDEX].Value -= 1;
				this->Variable[LEVELUP_INDEX].Max = this->Variable[LEVELUP_INDEX].Value;
			}
		}
		this->Variable[HP_INDEX].Value = this->GetModifiedVariable(HP_INDEX, VariableAttribute::Max);
		level_quantity -= 1;
	}
	
	UpdateXPRequired();
	
	bool upgrade_found = true;
	while (this->Variable[LEVELUP_INDEX].Value > 0 && upgrade_found && automatic_learning) {
		upgrade_found = false;

		if (((int) AiHelpers.ExperienceUpgrades.size()) > Type->Slot) {
			std::vector<const wyrmgus::unit_type *> potential_upgrades;
			
			if ((this->Player->AiEnabled || this->get_character() == nullptr) && this->Type->BoolFlag[HARVESTER_INDEX].value && this->CurrentResource && AiHelpers.ExperienceUpgrades[Type->Slot].size() > 1) {
				//if is a harvester who is currently gathering, try to upgrade to a unit type which is best for harvesting the current resource
				unsigned int best_gathering_rate = 0;
				for (size_t i = 0; i != AiHelpers.ExperienceUpgrades[Type->Slot].size(); ++i) {
					const wyrmgus::unit_type *experience_upgrade_type = AiHelpers.ExperienceUpgrades[Type->Slot][i];
					if (check_conditions(experience_upgrade_type, this, true)) {
						if (this->get_character() == nullptr || !wyrmgus::vector::contains(this->get_character()->ForbiddenUpgrades, experience_upgrade_type)) {
							if (experience_upgrade_type->get_resource_info(this->get_current_resource()) == nullptr) {
								continue;
							}

							unsigned int gathering_rate = experience_upgrade_type->get_resource_step(this->get_current_resource(), this->Player->get_index());
							if (gathering_rate >= best_gathering_rate) {
								if (gathering_rate > best_gathering_rate) {
									best_gathering_rate = gathering_rate;
									potential_upgrades.clear();
								}
								potential_upgrades.push_back(experience_upgrade_type);
							}
						}
					}
				}
			} else if (this->Player->AiEnabled || (this->get_character() == nullptr && AiHelpers.ExperienceUpgrades[Type->Slot].size() == 1)) {
				for (size_t i = 0; i != AiHelpers.ExperienceUpgrades[Type->Slot].size(); ++i) {
					if (check_conditions(AiHelpers.ExperienceUpgrades[Type->Slot][i], this, true)) {
						if (this->get_character() == nullptr || !wyrmgus::vector::contains(this->get_character()->ForbiddenUpgrades, AiHelpers.ExperienceUpgrades[Type->Slot][i])) {
							potential_upgrades.push_back(AiHelpers.ExperienceUpgrades[Type->Slot][i]);
						}
					}
				}
			}
			
			if (potential_upgrades.size() > 0) {
				this->Variable[LEVELUP_INDEX].Value -= 1;
				this->Variable[LEVELUP_INDEX].Max = this->Variable[LEVELUP_INDEX].Value;
				const wyrmgus::unit_type *chosen_unit_type = potential_upgrades[SyncRand(potential_upgrades.size())];
				if (this->Player == CPlayer::GetThisPlayer()) {
					this->Player->Notify(notification_type::green, this->tilePos, this->MapLayer->ID, _("%s has upgraded to %s!"), this->GetMessageName().c_str(), chosen_unit_type->get_name().c_str());
				}
				TransformUnitIntoType(*this, *chosen_unit_type);
				upgrade_found = true;
			}
		}
			
		if ((this->Player->AiEnabled || this->get_character() == nullptr) && this->Variable[LEVELUP_INDEX].Value) {
			if (((int) AiHelpers.LearnableAbilities.size()) > Type->Slot) {
				std::vector<const CUpgrade *> potential_abilities;
				for (size_t i = 0; i != AiHelpers.LearnableAbilities[Type->Slot].size(); ++i) {
					if (this->can_learn_ability(AiHelpers.LearnableAbilities[Type->Slot][i])) {
						potential_abilities.push_back(AiHelpers.LearnableAbilities[Type->Slot][i]);
					}
				}
				if (potential_abilities.size() > 0) {
					if (potential_abilities.size() == 1 || this->Player->AiEnabled) { //if can only acquire one particular ability, get it automatically
						const CUpgrade *chosen_ability = potential_abilities[SyncRand(potential_abilities.size())];
						AbilityAcquire(*this, chosen_ability);
						upgrade_found = true;
						if (this->Player == CPlayer::GetThisPlayer()) {
							this->Player->Notify(notification_type::green, this->tilePos, this->MapLayer->ID, _("%s has acquired the %s ability!"), this->GetMessageName().c_str(), chosen_ability->get_name().c_str());
						}
					}
				}
			}
		}
	}
	
	this->Variable[LEVELUP_INDEX].Enable = 1;
	
	this->Player->UpdateLevelUpUnits();
}

void CUnit::restore_hp(const int amount)
{
	this->Variable[HP_INDEX].Value += amount;
	this->Variable[HP_INDEX].Value = std::clamp(this->Variable[HP_INDEX].Value, 0, this->Variable[HP_INDEX].Max);
}

void CUnit::restore_hp_percent(const int percent)
{
	this->restore_hp(this->Variable[HP_INDEX].Max * percent / 100);
}

void CUnit::restore_mana(const int amount)
{
	this->Variable[MANA_INDEX].Value += amount;
	this->Variable[MANA_INDEX].Value = std::clamp(this->Variable[MANA_INDEX].Value, 0, this->Variable[MANA_INDEX].Max);
}

void CUnit::restore_mana_percent(const int percent)
{
	this->restore_mana(this->Variable[MANA_INDEX].Max * percent / 100);
}


void CUnit::Retrain()
{
	//lose all abilities (the AbilityLost function also returns the level-ups to the unit)
	for (CUpgrade *upgrade : CUpgrade::get_all()) {
		const int upgrade_count = this->GetIndividualUpgrade(upgrade);
		if (upgrade_count == 0) {
			continue;
		}

		if (upgrade->is_ability()) {
			int remove_count = upgrade_count;

			for (const CUpgrade *bonus_ability : this->bonus_abilities) {
				if (bonus_ability == upgrade) {
					--remove_count;
				}
			}

			if (this->get_character() != nullptr) {
				for (const CUpgrade *base_ability : this->get_character()->get_base_abilities()) {
					if (base_ability == upgrade) {
						--remove_count;
					}
				}
			}

			for (int i = 0; i < remove_count; ++i) {
				AbilityLost(*this, upgrade);
			}
		} else if (upgrade->get_deity() != nullptr && this->get_character() != nullptr && this->get_character()->is_custom()) {
			//allow changing the deity for custom heroes
			IndividualUpgradeLost(*this, upgrade, true);
		}
	}
	
	const std::string unit_name = GetMessageName();

	int base_level = 1;
	if (this->get_character() != nullptr) {
		base_level = this->get_character()->get_base_level();
	}
	
	//now, revert the unit's type to the base level one
	while (this->Type->Stats[this->Player->get_index()].Variables[LEVEL_INDEX].Value > base_level) {
		bool found_previous_unit_type = false;
		for (unit_type *unit_type : unit_type::get_all()) {
			if (unit_type->is_template()) {
				continue;
			}

			if (this->get_character() != nullptr && vector::contains(this->get_character()->ForbiddenUpgrades, unit_type)) {
				continue;
			}

			if (static_cast<int>(AiHelpers.ExperienceUpgrades.size()) > unit_type->Slot) {
				for (size_t j = 0; j != AiHelpers.ExperienceUpgrades[unit_type->Slot].size(); ++j) {
					if (AiHelpers.ExperienceUpgrades[unit_type->Slot][j] == this->Type) {
						this->Variable[LEVELUP_INDEX].Value += 1;
						this->Variable[LEVELUP_INDEX].Max = this->Variable[LEVELUP_INDEX].Value;
						this->Variable[LEVELUP_INDEX].Enable = 1;
						TransformUnitIntoType(*this, *unit_type);
						if (game::get()->is_persistency_enabled() && this->get_character() != nullptr) {	//save the unit-type experience upgrade for persistent characters
							if (this->get_character()->get_unit_type() != unit_type) {
								if (this->Player == CPlayer::GetThisPlayer()) {
									this->get_character()->set_unit_type(unit_type);
									achievement::check_achievements();
								}
							}
						}
						found_previous_unit_type = true;
						break;
					}
				}
			}
			if (found_previous_unit_type) {
				break;
			}
		}
		if (!found_previous_unit_type) {
			break;
		}
	}

	//save the retraining for persistent characters
	if (game::get()->is_persistency_enabled() && this->get_character() != nullptr && this->Player == CPlayer::GetThisPlayer()) {
		this->get_character()->save();
	}

	if (this->Player == CPlayer::GetThisPlayer()) {
		this->Player->Notify(notification_type::green, this->tilePos, this->MapLayer->ID, _("%s's level-up choices have been reset."), unit_name.c_str());
	}
}

void CUnit::auto_use_item()
{
	if (!HasInventory()) {
		return;
	}
	
	for (CUnit *uins : this->get_units_inside()) {
		if (!uins->Type->BoolFlag[ITEM_INDEX].value || uins->Elixir) {
			continue;
		}
		
		if (!is_consumable_item_class(uins->Type->get_item_class())) {
			continue;
		}

		bool usable_item = false;
		
		if (uins->Variable[HITPOINTHEALING_INDEX].Value > 0) {
			//use a healing item if has less than 20% health
			if (
				uins->Variable[HITPOINTHEALING_INDEX].Value <= (this->GetModifiedVariable(HP_INDEX, VariableAttribute::Max) - this->Variable[HP_INDEX].Value)
				|| (this->Variable[HP_INDEX].Value * 100 / this->GetModifiedVariable(HP_INDEX, VariableAttribute::Max)) <= 20
			) {
				usable_item = true;
			}
		}

		if (!usable_item && uins->Variable[MANA_RESTORATION_INDEX].Value > 0) {
			if (uins->Variable[MANA_RESTORATION_INDEX].Value <= (this->GetModifiedVariable(MANA_INDEX, VariableAttribute::Max) - this->Variable[MANA_INDEX].Value)) {
				usable_item = true;
			}
		}

		if (usable_item) {
			if (this->CriticalOrder == nullptr) {
				this->CriticalOrder = COrder::NewActionUse(*uins);
			}
			return;
		}
	}
}

void CUnit::set_character(wyrmgus::character *character)
{
	if (this->CurrentAction() == UnitAction::Die) {
		return;
	}

	if (this->get_character() != nullptr) {
		vector::remove(this->Player->Heroes, this);

		this->Variable[HERO_INDEX].Max = this->Variable[HERO_INDEX].Value = this->Variable[HERO_INDEX].Enable = 0;
	}

	this->character = character;

	if (this->get_character() != nullptr) {
		this->apply_character_properties();
	}

	this->ChooseVariation(); //choose a new variation now
	for (int i = 0; i < MaxImageLayers; ++i) {
		ChooseVariation(nullptr, false, i);
	}
	this->UpdateButtonIcons();
	this->UpdateXPRequired();
}

void CUnit::SetCharacter(const std::string &character_ident, bool custom_hero)
{
	wyrmgus::character *character = nullptr;
	if (!custom_hero) {
		character = character::get(character_ident);
	} else {
		character = character::get_custom_hero(character_ident);
	}

	this->set_character(character);
}

//apply the properties of the unit's character on it
void CUnit::apply_character_properties()
{
	int old_mana_percent = 0;
	if (this->Variable[MANA_INDEX].Max > 0) {
		old_mana_percent = this->Variable[MANA_INDEX].Value * 100 / this->Variable[MANA_INDEX].Max;
	}

	this->Name = this->get_character()->get_name();
	this->epithet = this->get_character()->get_epithet();
	this->surname = this->get_character()->get_surname();

	if (this->get_character()->get_unit_type() != nullptr) {
		if (this->get_character()->get_unit_type() != this->Type) { //set type to that of the character
			TransformUnitIntoType(*this, *this->get_character()->get_unit_type());
		}

		this->Variable = this->get_character()->get_unit_type()->Stats[this->Player->get_index()].Variables;
	} else {
		fprintf(stderr, "Character \"%s\" has no unit type.\n", character->get_identifier().c_str());
		return;
	}

	this->IndividualUpgrades.clear(); //reset the individual upgrades and then apply the character's
	this->traits.clear();

	if (this->Type->get_civilization() != nullptr) {
		CUpgrade *civilization_upgrade = this->Type->get_civilization()->get_upgrade();
		if (civilization_upgrade != nullptr) {
			this->SetIndividualUpgrade(civilization_upgrade, 1);
		}
	}
	if (this->Type->get_faction() != nullptr && this->Type->get_faction()->get_upgrade() != nullptr) {
		const CUpgrade *faction_upgrade = this->Type->get_faction()->get_upgrade();
		this->SetIndividualUpgrade(faction_upgrade, 1);
	}

	this->remove_traits();

	for (const CUpgrade *trait : this->get_character()->get_traits()) {
		this->add_trait(trait);
	}

	if (static_cast<int>(this->get_traits().size()) < CUnit::min_traits && !CEditor::get()->is_running() && !this->Type->get_traits().empty()) {
		this->generate_traits();
	}

	//load worshipped deities
	for (size_t i = 0; i < this->get_character()->Deities.size(); ++i) {
		const CUpgrade *deity_upgrade = this->get_character()->Deities[i]->get_upgrade();
		if (deity_upgrade != nullptr) {
			IndividualUpgradeAcquire(*this, deity_upgrade);
		}
	}

	for (const CUpgrade *ability_upgrade : this->Type->StartingAbilities) {
		if (check_conditions(ability_upgrade, this)) {
			this->add_bonus_ability(ability_upgrade);
		}
	}

	for (const CUpgrade *ability_upgrade : this->get_character()->get_bonus_abilities()) {
		if (check_conditions(ability_upgrade, this)) {
			this->add_bonus_ability(ability_upgrade);
		}
	}

	this->Variable[LEVEL_INDEX].Max = 100000; // because the code above sets the max level to the unit type stats' Level variable (which is the same as its value)
	if (this->Variable[LEVEL_INDEX].Value < this->get_character()->get_level()) {
		this->IncreaseLevel(this->get_character()->get_level() - this->Variable[LEVEL_INDEX].Value, false);
	}

	//ensure that the XP required is properly updated
	this->UpdateXPRequired();

	this->Variable[XP_INDEX].Enable = 1;
	this->Variable[XP_INDEX].Value = this->Variable[XPREQUIRED_INDEX].Value * this->get_character()->ExperiencePercent / 100;
	this->Variable[XP_INDEX].Max = this->Variable[XP_INDEX].Value;

	if (this->Variable[MANA_INDEX].Max > 0) {
		this->Variable[MANA_INDEX].Value = this->Variable[MANA_INDEX].Max * old_mana_percent / 100;
	}

	//load learned abilities
	std::vector<const CUpgrade *> abilities_to_remove;
	for (const CUpgrade *ability : this->get_character()->get_abilities()) {
		if (this->can_learn_ability(ability)) {
			AbilityAcquire(*this, ability, false);
		} else {
			//can't learn the ability? something changed in the game's code, remove it from persistent data and allow the hero to repick the ability
			abilities_to_remove.push_back(ability);
		}
	}

	if (!abilities_to_remove.empty()) {
		for (size_t i = 0; i < abilities_to_remove.size(); ++i) {
			this->get_character()->remove_ability(abilities_to_remove[i]);
		}

		if (game::get()->is_persistency_enabled() && this->Player == CPlayer::GetThisPlayer()) {
			this->get_character()->save();
		}
	}

	//load read works
	for (size_t i = 0; i < this->get_character()->ReadWorks.size(); ++i) {
		ReadWork(this->get_character()->ReadWorks[i], false);
	}

	//load consumed elixirs
	for (size_t i = 0; i < this->get_character()->ConsumedElixirs.size(); ++i) {
		ConsumeElixir(this->get_character()->ConsumedElixirs[i], false);
	}

	//load items
	for (const auto &persistent_item : this->get_character()->get_items()) {
		CUnit *item = MakeUnitAndPlace(this->tilePos, *persistent_item->get_unit_type(), CPlayer::get_neutral_player(), this->MapLayer->ID);
		if (persistent_item->Prefix != nullptr) {
			item->SetPrefix(persistent_item->Prefix);
		}
		if (persistent_item->Suffix != nullptr) {
			item->SetSuffix(persistent_item->Suffix);
		}
		if (persistent_item->Spell != nullptr) {
			item->SetSpell(persistent_item->Spell);
		}
		if (persistent_item->Work != nullptr) {
			item->SetWork(persistent_item->Work);
		}
		if (persistent_item->Elixir != nullptr) {
			item->SetElixir(persistent_item->Elixir);
		}
		item->unique = persistent_item->get_unique();
		if (!persistent_item->get_name().empty()) {
			item->Name = persistent_item->get_name();
		}
		item->Bound = persistent_item->is_bound();
		item->Identified = persistent_item->is_identified();
		item->Remove(this);
		if (this->get_character()->is_item_equipped(persistent_item.get()) && this->can_equip_item(item)) {
			this->EquipItem(*item, false);
		}
	}

	this->Player->Heroes.push_back(this);

	this->Variable[HERO_INDEX].Max = this->Variable[HERO_INDEX].Value = this->Variable[HERO_INDEX].Enable = 1;
}

bool CUnit::CheckTerrainForVariation(const wyrmgus::unit_type_variation *variation) const
{
	//if the variation has one or more terrain types set as a precondition, then all tiles underneath the unit must match at least one of those terrains
	if (variation->Terrains.size() > 0) {
		if (!CMap::get()->Info->IsPointOnMap(this->tilePos, this->MapLayer)) {
			return false;
		}

		for (int x = 0; x < this->Type->get_tile_width(); ++x) {
			for (int y = 0; y < this->Type->get_tile_height(); ++y) {
				if (CMap::get()->Info->IsPointOnMap(this->tilePos + Vec2i(x, y), this->MapLayer)) {
					if (!wyrmgus::vector::contains(variation->Terrains, CMap::get()->GetTileTopTerrain(this->tilePos + Vec2i(x, y), false, this->MapLayer->ID, true))) {
						return false;
					}
				}
			}
		}
	}
	
	//if the variation has one or more terrains set as a forbidden precondition, then no tiles underneath the unit may match one of those terrains
	if (variation->TerrainsForbidden.size() > 0) {
		if (CMap::get()->Info->IsPointOnMap(this->tilePos, this->MapLayer)) {
			for (int x = 0; x < this->Type->get_tile_width(); ++x) {
				for (int y = 0; y < this->Type->get_tile_height(); ++y) {
					if (CMap::get()->Info->IsPointOnMap(this->tilePos + Vec2i(x, y), this->MapLayer)) {
						if (wyrmgus::vector::contains(variation->TerrainsForbidden, CMap::get()->GetTileTopTerrain(this->tilePos + Vec2i(x, y), false, this->MapLayer->ID, true))) {
							return false;
						}
					}
				}
			}
		}
	}
	
	return true;
}

bool CUnit::CheckSeasonForVariation(const wyrmgus::unit_type_variation *variation) const
{
	if (
		!variation->Seasons.empty()
		&& (!this->MapLayer || !vector::contains(variation->Seasons, this->MapLayer->get_tile_season(this->get_center_tile_pos())))
	) {
		return false;
	}
	
	if (
		!variation->ForbiddenSeasons.empty()
		&& this->MapLayer
		&& vector::contains(variation->ForbiddenSeasons, this->MapLayer->get_tile_season(this->get_center_tile_pos()))
	) {
		return false;
	}
	
	return true;
}

bool CUnit::can_have_variation(const wyrmgus::unit_type_variation *variation) const
{
	if (variation->ResourceMin && this->ResourcesHeld < variation->ResourceMin) {
		return false;
	}
	if (variation->ResourceMax && this->ResourcesHeld > variation->ResourceMax) {
		return false;
	}

	if (!this->CheckSeasonForVariation(variation)) {
		return false;
	}

	if (!this->CheckTerrainForVariation(variation)) {
		return false;
	}

	bool upgrades_check = true;
	bool requires_weapon = false;
	bool found_weapon = false;
	bool requires_shield = false;
	bool found_shield = false;
	for (const CUpgrade *required_upgrade : variation->UpgradesRequired) {
		if (required_upgrade->is_weapon()) {
			requires_weapon = true;
			if (UpgradeIdentAllowed(*this->Player, required_upgrade->get_identifier().c_str()) == 'R' || this->GetIndividualUpgrade(required_upgrade)) {
				found_weapon = true;
			}
		} else if (required_upgrade->is_shield()) {
			requires_shield = true;
			if (UpgradeIdentAllowed(*this->Player, required_upgrade->get_identifier().c_str()) == 'R' || this->GetIndividualUpgrade(required_upgrade)) {
				found_shield = true;
			}
		} else if (UpgradeIdentAllowed(*this->Player, required_upgrade->get_identifier().c_str()) != 'R' && this->GetIndividualUpgrade(required_upgrade) == false) {
			upgrades_check = false;
			break;
		}
	}

	if (upgrades_check) {
		for (const CUpgrade *forbidden_upgrade : variation->UpgradesForbidden) {
			if (UpgradeIdentAllowed(*this->Player, forbidden_upgrade->get_identifier().c_str()) == 'R' || this->GetIndividualUpgrade(forbidden_upgrade)) {
				upgrades_check = false;
				break;
			}
		}
	}

	for (const wyrmgus::item_class item_class_not_equipped : variation->item_classes_not_equipped) {
		if (this->is_item_class_equipped(item_class_not_equipped)) {
			upgrades_check = false;
			break;
		}
	}
	for (size_t j = 0; j < variation->ItemsNotEquipped.size(); ++j) {
		if (this->is_item_type_equipped(variation->ItemsNotEquipped[j])) {
			upgrades_check = false;
			break;
		}
	}
	if (upgrades_check == false) {
		return false;
	}
	for (const wyrmgus::item_class item_class_equipped : variation->item_classes_equipped) {
		if (wyrmgus::get_item_class_slot(item_class_equipped) == wyrmgus::item_slot::weapon) {
			requires_weapon = true;
			if (is_item_class_equipped(item_class_equipped)) {
				found_weapon = true;
			}
		} else if (wyrmgus::get_item_class_slot(item_class_equipped) == wyrmgus::item_slot::shield) {
			requires_shield = true;
			if (is_item_class_equipped(item_class_equipped)) {
				found_shield = true;
			}
		}
	}
	for (const wyrmgus::unit_type *item_type_equipped : variation->ItemsEquipped) {
		if (wyrmgus::get_item_class_slot(item_type_equipped->get_item_class()) == wyrmgus::item_slot::weapon) {
			requires_weapon = true;
			if (this->is_item_type_equipped(item_type_equipped)) {
				found_weapon = true;
			}
		} else if (wyrmgus::get_item_class_slot(item_type_equipped->get_item_class()) == wyrmgus::item_slot::shield) {
			requires_shield = true;
			if (this->is_item_type_equipped(item_type_equipped)) {
				found_shield = true;
			}
		}
	}
	if ((requires_weapon && !found_weapon) || (requires_shield && !found_shield)) {
		return false;
	}

	const read_only_context ctx = read_only_context::from_scope(this);

	if (variation->get_player_conditions() != nullptr && !variation->get_player_conditions()->check(this->Player, ctx)) {
		return false;
	}

	if (variation->get_unit_conditions() != nullptr && !variation->get_unit_conditions()->check(this, ctx)) {
		return false;
	}

	return true;
}

void CUnit::ChooseVariation(const wyrmgus::unit_type *new_type, const bool ignore_old_variation, const int image_layer, const bool notify)
{
	const std::set<const variation_tag *> *current_tags = nullptr;
	if (image_layer == -1) {
		if (this->get_character() != nullptr && !this->get_character()->get_variation_tags().empty()) {
			current_tags = &this->get_character()->get_variation_tags();
		} else if (this->GetVariation() != nullptr) {
			current_tags = &this->GetVariation()->get_tags();
		}
	} else {
		if (image_layer == HairImageLayer && this->get_character() != nullptr && !this->get_character()->get_variation_tags().empty()) {
			current_tags = &this->get_character()->get_variation_tags();
		} else if (this->GetLayerVariation(image_layer)) {
			current_tags = &this->GetLayerVariation(image_layer)->get_tags();
		}
	}
	
	std::vector<const unit_type_variation *> potential_variations;
	const std::vector<qunique_ptr<unit_type_variation>> &variation_list = image_layer == -1 ? (new_type != nullptr ? new_type->get_variations() : this->Type->get_variations()) : (new_type != nullptr ? new_type->LayerVariations[image_layer] : this->Type->LayerVariations[image_layer]);
	
	size_t best_shared_tag_count = 0;

	for (const auto &variation : variation_list) {
		if (!this->can_have_variation(variation.get())) {
			continue;
		}

		if (!ignore_old_variation && current_tags != nullptr) {
			const size_t shared_tag_count = variation->get_shared_tag_count(*current_tags);

			if (shared_tag_count < best_shared_tag_count) {
				continue;
			}

			if (shared_tag_count > best_shared_tag_count) {
				//clear the variations previously added to the list if this one shares more tags with the current tag list
				potential_variations.clear();
				best_shared_tag_count = shared_tag_count;
			}
		}

		for (int i = 0; i < variation->Weight; ++i) {
			potential_variations.push_back(variation.get());
		}
	}

	if (!potential_variations.empty()) {
		this->SetVariation(vector::get_random(potential_variations), image_layer, notify);
	}
}

void CUnit::SetVariation(const wyrmgus::unit_type_variation *new_variation, const int image_layer, const bool notify)
{
	if (image_layer == -1) {
		if (
			(this->GetVariation() != nullptr && this->GetVariation()->get_animation_set() != nullptr)
			|| (new_variation != nullptr && new_variation->get_animation_set() != nullptr)
		) { //if the old (if any) or the new variation has specific animations, set the unit's frame to its type's still frame
			this->Frame = this->Type->StillFrame;
		}
		this->Variation = new_variation ? new_variation->get_index() : 0;

		if (notify && this->MapLayer != nullptr && !this->Removed && game::get()->is_running()) {
			emit this->MapLayer->unit_image_changed(UnitNumber(*this), this->Type, this->GetVariation(), this->get_player_color());
			emit this->MapLayer->unit_frame_changed(UnitNumber(*this), this->Frame);
		}
	} else {
		this->LayerVariation[image_layer] = new_variation ? new_variation->get_index() : -1;
	}
}

const wyrmgus::unit_type_variation *CUnit::GetVariation() const
{
	if (this->Variation < static_cast<int>(this->Type->get_variations().size())) {
		return this->Type->get_variations()[this->Variation].get();
	}
	
	return nullptr;
}

const wyrmgus::unit_type_variation *CUnit::GetLayerVariation(const unsigned int image_layer) const
{
	if (this->LayerVariation[image_layer] >= 0 && this->LayerVariation[image_layer] < (int) this->Type->LayerVariations[image_layer].size()) {
		return this->Type->LayerVariations[image_layer][this->LayerVariation[image_layer]].get();
	}
	
	return nullptr;
}

void CUnit::UpdateButtonIcons()
{
	this->ChooseButtonIcon(ButtonCmd::Attack);
	this->ChooseButtonIcon(ButtonCmd::Stop);
	this->ChooseButtonIcon(ButtonCmd::Move);
	this->ChooseButtonIcon(ButtonCmd::StandGround);
	this->ChooseButtonIcon(ButtonCmd::Patrol);
	if (this->Type->BoolFlag[HARVESTER_INDEX].value) {
		this->ChooseButtonIcon(ButtonCmd::Return);
	}
}

void CUnit::ChooseButtonIcon(const ButtonCmd button_action)
{
	if (button_action == ButtonCmd::Attack) {
		if (this->EquippedItems[static_cast<int>(wyrmgus::item_slot::arrows)].size() > 0 && this->EquippedItems[static_cast<int>(wyrmgus::item_slot::arrows)][0]->get_icon() != nullptr) {
			this->ButtonIcons[button_action] = this->EquippedItems[static_cast<int>(wyrmgus::item_slot::arrows)][0]->get_icon();
			return;
		}
		
		if (this->EquippedItems[static_cast<int>(wyrmgus::item_slot::weapon)].size() > 0 && this->EquippedItems[static_cast<int>(wyrmgus::item_slot::weapon)][0]->Type->get_item_class() != wyrmgus::item_class::bow && this->EquippedItems[static_cast<int>(wyrmgus::item_slot::weapon)][0]->get_icon() != nullptr) {
			this->ButtonIcons[button_action] = this->EquippedItems[static_cast<int>(wyrmgus::item_slot::weapon)][0]->get_icon();
			return;
		}
	} else if (button_action == ButtonCmd::Stop) {
		if (this->EquippedItems[static_cast<int>(wyrmgus::item_slot::shield)].size() > 0 && this->EquippedItems[static_cast<int>(wyrmgus::item_slot::shield)][0]->Type->get_item_class() == wyrmgus::item_class::shield && this->EquippedItems[static_cast<int>(wyrmgus::item_slot::shield)][0]->get_icon() != nullptr) {
			this->ButtonIcons[button_action] = this->EquippedItems[static_cast<int>(wyrmgus::item_slot::shield)][0]->get_icon();
			return;
		}
	} else if (button_action == ButtonCmd::Move) {
		if (this->EquippedItems[static_cast<int>(wyrmgus::item_slot::boots)].size() > 0 && this->EquippedItems[static_cast<int>(wyrmgus::item_slot::boots)][0]->get_icon() != nullptr) {
			this->ButtonIcons[button_action] = this->EquippedItems[static_cast<int>(wyrmgus::item_slot::boots)][0]->get_icon();
			return;
		}
	} else if (button_action == ButtonCmd::StandGround) {
		if (this->EquippedItems[static_cast<int>(wyrmgus::item_slot::arrows)].size() > 0 && this->EquippedItems[static_cast<int>(wyrmgus::item_slot::arrows)][0]->Type->ButtonIcons.find(button_action) != this->EquippedItems[static_cast<int>(wyrmgus::item_slot::arrows)][0]->Type->ButtonIcons.end()) {
			this->ButtonIcons[button_action] = this->EquippedItems[static_cast<int>(wyrmgus::item_slot::arrows)][0]->Type->ButtonIcons.find(button_action)->second.Icon;
			return;
		}

		if (this->EquippedItems[static_cast<int>(wyrmgus::item_slot::weapon)].size() > 0 && this->EquippedItems[static_cast<int>(wyrmgus::item_slot::weapon)][0]->Type->ButtonIcons.find(button_action) != this->EquippedItems[static_cast<int>(wyrmgus::item_slot::weapon)][0]->Type->ButtonIcons.end()) {
			this->ButtonIcons[button_action] = this->EquippedItems[static_cast<int>(wyrmgus::item_slot::weapon)][0]->Type->ButtonIcons.find(button_action)->second.Icon;
			return;
		}
	}
	
	const wyrmgus::unit_type_variation *variation = this->GetVariation();
	if (variation && variation->ButtonIcons.find(button_action) != variation->ButtonIcons.end()) {
		this->ButtonIcons[button_action] = variation->ButtonIcons.find(button_action)->second.Icon;
		return;
	}
	for (int i = 0; i < MaxImageLayers; ++i) {
		const wyrmgus::unit_type_variation *layer_variation = this->GetLayerVariation(i);
		if (layer_variation && layer_variation->ButtonIcons.find(button_action) != layer_variation->ButtonIcons.end()) {
			this->ButtonIcons[button_action] = layer_variation->ButtonIcons.find(button_action)->second.Icon;
			return;
		}
	}

	for (int i = (wyrmgus::upgrade_modifier::UpgradeModifiers.size() - 1); i >= 0; --i) {
		const wyrmgus::upgrade_modifier *modifier = wyrmgus::upgrade_modifier::UpgradeModifiers[i];
		const CUpgrade *upgrade = modifier->get_upgrade();
		if (this->Player->Allow.Upgrades[upgrade->ID] == 'R' && modifier->applies_to(this->Type)) {
			if (
				(
					(button_action == ButtonCmd::Attack && ((upgrade->is_weapon() && upgrade->get_item()->get_item_class() != wyrmgus::item_class::bow) || upgrade->is_arrows()))
					|| (button_action == ButtonCmd::Stop && upgrade->is_shield())
					|| (button_action == ButtonCmd::Move && upgrade->is_boots())
				)
				&& upgrade->get_item()->get_icon() != nullptr
			) {
				this->ButtonIcons[button_action] = upgrade->get_item()->get_icon();
				return;
			} else if (button_action == ButtonCmd::StandGround && (upgrade->is_weapon() || upgrade->is_arrows()) && upgrade->get_item()->ButtonIcons.contains(button_action)) {
				this->ButtonIcons[button_action] = upgrade->get_item()->ButtonIcons.find(button_action)->second.Icon;
				return;
			}
		}
	}
	
	if (button_action == ButtonCmd::Attack) {
		if (this->Type->DefaultEquipment.find(item_slot::arrows) != this->Type->DefaultEquipment.end() && this->Type->DefaultEquipment.find(item_slot::arrows)->second->get_icon() != nullptr) {
			this->ButtonIcons[button_action] = this->Type->DefaultEquipment.find(item_slot::arrows)->second->get_icon();
			return;
		}
		
		if (this->Type->DefaultEquipment.find(wyrmgus::item_slot::weapon) != this->Type->DefaultEquipment.end() && this->Type->DefaultEquipment.find(wyrmgus::item_slot::weapon)->second->get_icon() != nullptr) {
			this->ButtonIcons[button_action] = this->Type->DefaultEquipment.find(wyrmgus::item_slot::weapon)->second->get_icon();
			return;
		}
	} else if (button_action == ButtonCmd::Stop) {
		if (this->Type->DefaultEquipment.find(item_slot::shield) != this->Type->DefaultEquipment.end() && this->Type->DefaultEquipment.find(item_slot::shield)->second->get_item_class() == item_class::shield && this->Type->DefaultEquipment.find(item_slot::shield)->second->get_icon() != nullptr) {
			this->ButtonIcons[button_action] = this->Type->DefaultEquipment.find(wyrmgus::item_slot::shield)->second->get_icon();
			return;
		}
	} else if (button_action == ButtonCmd::Move) {
		if (this->Type->DefaultEquipment.find(item_slot::boots) != this->Type->DefaultEquipment.end() && this->Type->DefaultEquipment.find(item_slot::boots)->second->get_icon() != nullptr) {
			this->ButtonIcons[button_action] = this->Type->DefaultEquipment.find(item_slot::boots)->second->get_icon();
			return;
		}
	} else if (button_action == ButtonCmd::StandGround) {
		if (this->Type->DefaultEquipment.find(wyrmgus::item_slot::arrows) != this->Type->DefaultEquipment.end() && this->Type->DefaultEquipment.find(wyrmgus::item_slot::arrows)->second->ButtonIcons.find(button_action) != this->Type->DefaultEquipment.find(wyrmgus::item_slot::arrows)->second->ButtonIcons.end()) {
			this->ButtonIcons[button_action] = this->Type->DefaultEquipment.find(wyrmgus::item_slot::arrows)->second->ButtonIcons.find(button_action)->second.Icon;
			return;
		}
		
		if (this->Type->DefaultEquipment.find(wyrmgus::item_slot::weapon) != this->Type->DefaultEquipment.end() && this->Type->DefaultEquipment.find(wyrmgus::item_slot::weapon)->second->ButtonIcons.find(button_action) != this->Type->DefaultEquipment.find(wyrmgus::item_slot::weapon)->second->ButtonIcons.end()) {
			this->ButtonIcons[button_action] = this->Type->DefaultEquipment.find(wyrmgus::item_slot::weapon)->second->ButtonIcons.find(button_action)->second.Icon;
			return;
		}
	}
	
	if (this->Type->ButtonIcons.find(button_action) != this->Type->ButtonIcons.end()) {
		this->ButtonIcons[button_action] = this->Type->ButtonIcons.find(button_action)->second.Icon;
		return;
	}
	
	if (this->Type->get_civilization() != nullptr) {
		const wyrmgus::civilization *civilization = this->Type->get_civilization();
		const wyrmgus::faction *faction = this->Type->get_faction();
		
		if (faction == nullptr && this->Player->get_civilization() == civilization) {
			faction = this->Player->get_faction();
		}
		
		if (faction != nullptr && faction->ButtonIcons.find(button_action) != faction->ButtonIcons.end()) {
			this->ButtonIcons[button_action] = faction->ButtonIcons.find(button_action)->second;
			return;
		} else if (PlayerRaces.ButtonIcons[civilization->ID].find(button_action) != PlayerRaces.ButtonIcons[civilization->ID].end()) {
			this->ButtonIcons[button_action] = PlayerRaces.ButtonIcons[civilization->ID][button_action].Icon;
			return;
		}
	}
	
	if (this->ButtonIcons.find(button_action) != this->ButtonIcons.end()) { //if no proper button icon found, make sure any old button icon set for this button action isn't used either
		this->ButtonIcons.erase(button_action);
	}
}

void CUnit::EquipItem(CUnit &item, bool affect_character)
{
	const wyrmgus::item_class item_class = item.Type->get_item_class();
	const wyrmgus::item_slot item_slot = wyrmgus::get_item_class_slot(item_class);
	
	if (item_slot == wyrmgus::item_slot::none) {
		fprintf(stderr, "Trying to equip item of type \"%s\", which has no item slot.\n", item.get_type_name().c_str());
		return;
	}
	
	if (this->get_item_slot_quantity(item_slot) > 0 && EquippedItems[static_cast<int>(item_slot)].size() == this->get_item_slot_quantity(item_slot)) {
		DeequipItem(*EquippedItems[static_cast<int>(item_slot)][EquippedItems[static_cast<int>(item_slot)].size() - 1]);
	}
	
	if (item_slot == wyrmgus::item_slot::weapon && EquippedItems[static_cast<int>(item_slot)].size() == 0) {
		// remove the upgrade modifiers from weapon technologies or from abilities which require the base weapon class but aren't compatible with this weapon's class; and apply upgrade modifiers from abilities which require this weapon's class
		for (const wyrmgus::upgrade_modifier *modifier : wyrmgus::upgrade_modifier::UpgradeModifiers) {
			const CUpgrade *modifier_upgrade = modifier->get_upgrade();
			if (
				(modifier_upgrade->is_weapon() && Player->Allow.Upgrades[modifier_upgrade->ID] == 'R' && modifier->applies_to(this->Type))
				|| (modifier_upgrade->is_ability() && this->GetIndividualUpgrade(modifier_upgrade) && modifier_upgrade->WeaponClasses.size() > 0 && std::find(modifier_upgrade->WeaponClasses.begin(), modifier_upgrade->WeaponClasses.end(), this->Type->WeaponClasses[0]) != modifier_upgrade->WeaponClasses.end() && std::find(modifier_upgrade->WeaponClasses.begin(), modifier_upgrade->WeaponClasses.end(), item_class) == modifier_upgrade->WeaponClasses.end())
			) {
				if (this->GetIndividualUpgrade(modifier_upgrade)) {
					for (int i = 0; i < this->GetIndividualUpgrade(modifier_upgrade); ++i) {
						RemoveIndividualUpgradeModifier(*this, modifier);
					}
				} else {
					RemoveIndividualUpgradeModifier(*this, modifier);
				}
			} else if (
				modifier_upgrade->is_ability() && this->GetIndividualUpgrade(modifier_upgrade) && modifier_upgrade->WeaponClasses.size() > 0 && std::find(modifier_upgrade->WeaponClasses.begin(), modifier_upgrade->WeaponClasses.end(), this->Type->WeaponClasses[0]) == modifier_upgrade->WeaponClasses.end() && std::find(modifier_upgrade->WeaponClasses.begin(), modifier_upgrade->WeaponClasses.end(), item_class) != modifier_upgrade->WeaponClasses.end()
			) {
				if (this->GetIndividualUpgrade(modifier_upgrade)) {
					for (int i = 0; i < this->GetIndividualUpgrade(modifier_upgrade); ++i) {
						ApplyIndividualUpgradeModifier(*this, modifier);
					}
				} else {
					ApplyIndividualUpgradeModifier(*this, modifier);
				}
			}
		}
	} else if (item_slot == wyrmgus::item_slot::shield && EquippedItems[static_cast<int>(item_slot)].size() == 0) {
		// remove the upgrade modifiers from shield technologies
		for (const wyrmgus::upgrade_modifier *modifier : wyrmgus::upgrade_modifier::UpgradeModifiers) {
			const CUpgrade *modifier_upgrade = modifier->get_upgrade();
			if (modifier_upgrade->is_shield() && Player->Allow.Upgrades[modifier_upgrade->ID] == 'R' && modifier->applies_to(this->Type)) {
				RemoveIndividualUpgradeModifier(*this, modifier);
			}
		}
	} else if (item_slot == wyrmgus::item_slot::boots && EquippedItems[static_cast<int>(item_slot)].size() == 0) {
		// remove the upgrade modifiers from boots technologies
		for (const wyrmgus::upgrade_modifier *modifier : wyrmgus::upgrade_modifier::UpgradeModifiers) {
			const CUpgrade *modifier_upgrade = modifier->get_upgrade();
			if (modifier_upgrade->is_boots() && Player->Allow.Upgrades[modifier_upgrade->ID] == 'R' && modifier->applies_to(this->Type)) {
				RemoveIndividualUpgradeModifier(*this, modifier);
			}
		}
	} else if (item_slot == wyrmgus::item_slot::arrows && EquippedItems[static_cast<int>(item_slot)].size() == 0) {
		// remove the upgrade modifiers from arrows technologies
		for (const wyrmgus::upgrade_modifier *modifier : wyrmgus::upgrade_modifier::UpgradeModifiers) {
			const CUpgrade *modifier_upgrade = modifier->get_upgrade();
			if (modifier_upgrade->is_arrows() && Player->Allow.Upgrades[modifier_upgrade->ID] == 'R' && modifier->applies_to(this->Type)) {
				RemoveIndividualUpgradeModifier(*this, modifier);
			}
		}
	}
	
	if (item.get_unique() != nullptr && item.get_unique()->get_set() != nullptr && this->equipping_item_completes_set(&item)) {
		for (const auto &modifier : item.get_unique()->get_set()->get_modifiers()) {
			ApplyIndividualUpgradeModifier(*this, modifier.get());
		}
	}

	if (!IsNetworkGame() && this->get_character() != nullptr && this->Player == CPlayer::GetThisPlayer() && affect_character) {
		const wyrmgus::persistent_item *persistent_item = this->get_character()->get_item(&item);
		if (persistent_item != nullptr) {
			if (!this->get_character()->is_item_equipped(persistent_item)) {
				this->get_character()->equip_item(persistent_item);
				this->get_character()->save();
			} else {
				log::log_error("Item is not equipped by character \"" + this->get_character()->get_identifier() + "\"'s unit, but is equipped by the character itself.");
			}
		} else {
			fprintf(stderr, "Item is present in the inventory of the character \"%s\"'s unit, but not in the character's inventory itself.\n", this->get_character()->get_identifier().c_str());
		}
	}

	EquippedItems[static_cast<int>(item_slot)].push_back(&item);
	
	//change variation, if the current one has become forbidden
	const wyrmgus::unit_type_variation *variation = this->GetVariation();
	if (variation != nullptr && !this->can_have_variation(variation)) {
		this->ChooseVariation(); //choose a new variation now
	}
	for (int i = 0; i < MaxImageLayers; ++i) {
		const wyrmgus::unit_type_variation *layer_variation = this->GetLayerVariation(i);
		if (
			layer_variation
			&& (
				layer_variation->item_classes_not_equipped.contains(item.Type->get_item_class())
				|| std::find(layer_variation->ItemsNotEquipped.begin(), layer_variation->ItemsNotEquipped.end(), item.Type) != layer_variation->ItemsNotEquipped.end()
			)
		) {
			ChooseVariation(nullptr, false, i);
		}
	}

	if (item.GetVariation() != nullptr && !item.can_have_variation(item.GetVariation())) {
		item.ChooseVariation();
	}
	
	if (item_slot == wyrmgus::item_slot::weapon || item_slot == wyrmgus::item_slot::arrows) {
		this->ChooseButtonIcon(ButtonCmd::Attack);
		this->ChooseButtonIcon(ButtonCmd::StandGround);
	} else if (item_slot == wyrmgus::item_slot::shield) {
		this->ChooseButtonIcon(ButtonCmd::Stop);
	} else if (item_slot == wyrmgus::item_slot::boots) {
		this->ChooseButtonIcon(ButtonCmd::Move);
	}
	this->ChooseButtonIcon(ButtonCmd::Patrol);
	
	//add item bonuses
	for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); i++) {
		if (
			i == BASICDAMAGE_INDEX || i == PIERCINGDAMAGE_INDEX || i == THORNSDAMAGE_INDEX
			|| i == FIREDAMAGE_INDEX || i == COLDDAMAGE_INDEX || i == ARCANEDAMAGE_INDEX || i == LIGHTNINGDAMAGE_INDEX
			|| i == AIRDAMAGE_INDEX || i == EARTHDAMAGE_INDEX || i == WATERDAMAGE_INDEX || i == ACIDDAMAGE_INDEX || i == SHADOW_DAMAGE_INDEX
			|| i == ARMOR_INDEX || i == FIRERESISTANCE_INDEX || i == COLDRESISTANCE_INDEX || i == ARCANERESISTANCE_INDEX || i == LIGHTNINGRESISTANCE_INDEX
			|| i == AIRRESISTANCE_INDEX || i == EARTHRESISTANCE_INDEX || i == WATERRESISTANCE_INDEX || i == ACIDRESISTANCE_INDEX || i == SHADOW_RESISTANCE_INDEX
			|| i == HACKRESISTANCE_INDEX || i == PIERCERESISTANCE_INDEX || i == BLUNTRESISTANCE_INDEX
			|| i == ACCURACY_INDEX || i == EVASION_INDEX || i == SPEED_INDEX || i == CHARGEBONUS_INDEX || i == BACKSTAB_INDEX
			|| i == ATTACKRANGE_INDEX
		) {
			Variable[i].Value += item.Variable[i].Value;
			Variable[i].Max += item.Variable[i].Max;
		} else if (i == HITPOINTBONUS_INDEX) {
			Variable[HP_INDEX].Value += item.Variable[i].Value;
			Variable[HP_INDEX].Max += item.Variable[i].Max;
			Variable[HP_INDEX].Increase += item.Variable[i].Increase;
		} else if (i == SIGHTRANGE_INDEX || i == DAYSIGHTRANGEBONUS_INDEX || i == NIGHTSIGHTRANGEBONUS_INDEX) {
			if (!SaveGameLoading) {
				MapUnmarkUnitSight(*this);
			}
			Variable[i].Value += item.Variable[i].Value;
			Variable[i].Max += item.Variable[i].Max;
			if (!SaveGameLoading) {
				if (i == SIGHTRANGE_INDEX) {
					CurrentSightRange = Variable[i].Value;
				}
				UpdateUnitSightRange(*this);
				MapMarkUnitSight(*this);
			}
		}
	}
}

void CUnit::DeequipItem(CUnit &item, bool affect_character)
{
	//remove item bonuses
	for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); i++) {
		if (
			i == BASICDAMAGE_INDEX || i == PIERCINGDAMAGE_INDEX || i == THORNSDAMAGE_INDEX
			|| i == FIREDAMAGE_INDEX || i == COLDDAMAGE_INDEX || i == ARCANEDAMAGE_INDEX || i == LIGHTNINGDAMAGE_INDEX
			|| i == AIRDAMAGE_INDEX || i == EARTHDAMAGE_INDEX || i == WATERDAMAGE_INDEX || i == ACIDDAMAGE_INDEX || i == SHADOW_DAMAGE_INDEX
			|| i == ARMOR_INDEX || i == FIRERESISTANCE_INDEX || i == COLDRESISTANCE_INDEX || i == ARCANERESISTANCE_INDEX || i == LIGHTNINGRESISTANCE_INDEX
			|| i == AIRRESISTANCE_INDEX || i == EARTHRESISTANCE_INDEX || i == WATERRESISTANCE_INDEX || i == ACIDRESISTANCE_INDEX || i == SHADOW_RESISTANCE_INDEX
			|| i == HACKRESISTANCE_INDEX || i == PIERCERESISTANCE_INDEX || i == BLUNTRESISTANCE_INDEX
			|| i == ACCURACY_INDEX || i == EVASION_INDEX || i == SPEED_INDEX || i == CHARGEBONUS_INDEX || i == BACKSTAB_INDEX
			|| i == ATTACKRANGE_INDEX
		) {
			Variable[i].Value -= item.Variable[i].Value;
			Variable[i].Max -= item.Variable[i].Max;
		} else if (i == HITPOINTBONUS_INDEX) {
			Variable[HP_INDEX].Value -= item.Variable[i].Value;
			Variable[HP_INDEX].Max -= item.Variable[i].Max;
			Variable[HP_INDEX].Increase -= item.Variable[i].Increase;
		} else if (i == SIGHTRANGE_INDEX || i == DAYSIGHTRANGEBONUS_INDEX || i == NIGHTSIGHTRANGEBONUS_INDEX) {
			MapUnmarkUnitSight(*this);
			Variable[i].Value -= item.Variable[i].Value;
			Variable[i].Max -= item.Variable[i].Max;
			if (i == SIGHTRANGE_INDEX) {
				CurrentSightRange = Variable[i].Value;
			}
			UpdateUnitSightRange(*this);
			MapMarkUnitSight(*this);
		}
	}
	
	if (item.get_unique() != nullptr && item.get_unique()->get_set() != nullptr && this->DeequippingItemBreaksSet(&item)) {
		for (const auto &modifier : item.get_unique()->get_set()->get_modifiers()) {
			RemoveIndividualUpgradeModifier(*this, modifier.get());
		}
	}

	const wyrmgus::item_class item_class = item.Type->get_item_class();
	const wyrmgus::item_slot item_slot = wyrmgus::get_item_class_slot(item_class);
	
	if (item_slot == wyrmgus::item_slot::none) {
		fprintf(stderr, "Trying to de-equip item of type \"%s\", which has no item slot.\n", item.get_type_name().c_str());
		return;
	}
	
	if (game::get()->is_persistency_enabled() && this->get_character() != nullptr && this->Player == CPlayer::GetThisPlayer() && affect_character) {
		const wyrmgus::persistent_item *persistent_item = this->get_character()->get_item(&item);
		if (persistent_item != nullptr) {
			if (this->get_character()->is_item_equipped(persistent_item)) {
				this->get_character()->deequip_item(persistent_item);
				this->get_character()->save();
			} else {
				log::log_error("Item is equipped by character \"" + this->get_character()->get_identifier() + "\"'s unit, but not by the character itself.");
			}
		} else {
			fprintf(stderr, "Item is present in the inventory of the character \"%s\"'s unit, but not in the character's inventory itself.\n", this->get_character()->get_identifier().c_str());
		}
	}

	vector::remove(this->EquippedItems[static_cast<int>(item_slot)], &item);
	
	if (item_slot == wyrmgus::item_slot::weapon && EquippedItems[static_cast<int>(item_slot)].size() == 0) {
		// restore the upgrade modifiers from weapon technologies, and apply ability effects that are weapon class-specific accordingly
		for (const wyrmgus::upgrade_modifier *modifier : wyrmgus::upgrade_modifier::UpgradeModifiers) {
			const CUpgrade *modifier_upgrade = modifier->get_upgrade();
			if (
				(modifier_upgrade->is_weapon() && Player->Allow.Upgrades[modifier->get_upgrade()->get_index()] == 'R' && modifier->applies_to(this->Type))
				|| (modifier_upgrade->is_ability() && this->GetIndividualUpgrade(modifier_upgrade) && modifier_upgrade->WeaponClasses.size() > 0 && std::find(modifier_upgrade->WeaponClasses.begin(), modifier_upgrade->WeaponClasses.end(), this->Type->WeaponClasses[0]) != modifier_upgrade->WeaponClasses.end() && std::find(modifier_upgrade->WeaponClasses.begin(), modifier_upgrade->WeaponClasses.end(), item_class) == modifier_upgrade->WeaponClasses.end())
			) {
				if (this->GetIndividualUpgrade(modifier_upgrade)) {
					for (int i = 0; i < this->GetIndividualUpgrade(modifier_upgrade); ++i) {
						ApplyIndividualUpgradeModifier(*this, modifier);
					}
				} else {
					ApplyIndividualUpgradeModifier(*this, modifier);
				}
			} else if (
				modifier_upgrade->is_ability() && this->GetIndividualUpgrade(modifier_upgrade) && modifier_upgrade->WeaponClasses.size() > 0 && std::find(modifier_upgrade->WeaponClasses.begin(), modifier_upgrade->WeaponClasses.end(), this->Type->WeaponClasses[0]) == modifier_upgrade->WeaponClasses.end() && std::find(modifier_upgrade->WeaponClasses.begin(), modifier_upgrade->WeaponClasses.end(), item_class) != modifier_upgrade->WeaponClasses.end()
			) {
				if (this->GetIndividualUpgrade(modifier_upgrade)) {
					for (int i = 0; i < this->GetIndividualUpgrade(modifier_upgrade); ++i) {
						RemoveIndividualUpgradeModifier(*this, modifier);
					}
				} else {
					RemoveIndividualUpgradeModifier(*this, modifier);
				}
			}
		}
	} else if (item_slot == wyrmgus::item_slot::shield && EquippedItems[static_cast<int>(item_slot)].size() == 0) {
		// restore the upgrade modifiers from shield technologies
		for (const wyrmgus::upgrade_modifier *modifier : wyrmgus::upgrade_modifier::UpgradeModifiers) {
			const CUpgrade *modifier_upgrade = modifier->get_upgrade();
			if (modifier_upgrade->is_shield() && Player->Allow.Upgrades[modifier_upgrade->ID] == 'R' && modifier->applies_to(this->Type)) {
				ApplyIndividualUpgradeModifier(*this, modifier);
			}
		}
	} else if (item_slot == wyrmgus::item_slot::boots && EquippedItems[static_cast<int>(item_slot)].size() == 0) {
		// restore the upgrade modifiers from boots technologies
		for (const wyrmgus::upgrade_modifier *modifier : wyrmgus::upgrade_modifier::UpgradeModifiers) {
			const CUpgrade *modifier_upgrade = modifier->get_upgrade();
			if (modifier_upgrade->is_boots() && Player->Allow.Upgrades[modifier_upgrade->ID] == 'R' && modifier->applies_to(this->Type)) {
				ApplyIndividualUpgradeModifier(*this, modifier);
			}
		}
	} else if (item_slot == wyrmgus::item_slot::arrows && EquippedItems[static_cast<int>(item_slot)].size() == 0) {
		// restore the upgrade modifiers from arrows technologies
		for (const wyrmgus::upgrade_modifier *modifier : wyrmgus::upgrade_modifier::UpgradeModifiers) {
			const CUpgrade *modifier_upgrade = modifier->get_upgrade();
			if (modifier_upgrade->is_arrows() && Player->Allow.Upgrades[modifier_upgrade->ID] == 'R' && modifier->applies_to(this->Type)) {
				ApplyIndividualUpgradeModifier(*this, modifier);
			}
		}
	}
	
	//change variation, if the current one has become forbidden
	const wyrmgus::unit_type_variation *variation = this->GetVariation();
	if (variation != nullptr && !this->can_have_variation(variation)) {
		this->ChooseVariation(); //choose a new variation now
	}
	for (int i = 0; i < MaxImageLayers; ++i) {
		const wyrmgus::unit_type_variation *layer_variation = this->GetLayerVariation(i);

		if (
			layer_variation
			&& (
				layer_variation->item_classes_equipped.contains(item.Type->get_item_class())
				|| std::find(layer_variation->ItemsEquipped.begin(), layer_variation->ItemsEquipped.end(), item.Type) != layer_variation->ItemsEquipped.end()
			)
		) {
			ChooseVariation(nullptr, false, i);
		}
	}

	if (item.GetVariation() != nullptr && !item.can_have_variation(item.GetVariation())) {
		item.ChooseVariation();
	}

	if (item_slot == wyrmgus::item_slot::weapon || item_slot == wyrmgus::item_slot::arrows) {
		this->ChooseButtonIcon(ButtonCmd::Attack);
		this->ChooseButtonIcon(ButtonCmd::StandGround);
	} else if (item_slot == wyrmgus::item_slot::shield) {
		this->ChooseButtonIcon(ButtonCmd::Stop);
	} else if (item_slot == wyrmgus::item_slot::boots) {
		this->ChooseButtonIcon(ButtonCmd::Move);
	}
	this->ChooseButtonIcon(ButtonCmd::Patrol);
}

void CUnit::ReadWork(const CUpgrade *work, bool affect_character)
{
	IndividualUpgradeAcquire(*this, work);
	
	if (game::get()->is_persistency_enabled() && this->get_character() != nullptr && this->Player == CPlayer::GetThisPlayer() && affect_character) {
		if (!vector::contains(this->get_character()->ReadWorks, work)) {
			this->get_character()->ReadWorks.push_back(work);
			this->get_character()->save();
		}
	}
}

void CUnit::ConsumeElixir(const CUpgrade *elixir, bool affect_character)
{
	IndividualUpgradeAcquire(*this, elixir);
	
	if (game::get()->is_persistency_enabled() && this->get_character() != nullptr && this->Player == CPlayer::GetThisPlayer() && affect_character) {
		if (!wyrmgus::vector::contains(this->get_character()->ConsumedElixirs, elixir)) {
			this->get_character()->ConsumedElixirs.push_back(elixir);
			this->get_character()->save();
		}
	}
}

int CUnit::get_aura_range() const
{
	int aura_range = CUnit::aura_range + this->Variable[AURA_RANGE_BONUS_INDEX].Value;
	aura_range -= this->Type->get_tile_width() - 1;
	return std::max(aura_range, 1);
}

void CUnit::ApplyAura(const int aura_index)
{
	if (this->Removed) {
		return;
	}

	if (aura_index == LEADERSHIPAURA_INDEX) {
		if (!this->IsInCombat()) {
			return;
		}
	}
	
	this->ApplyAuraEffect(aura_index);
	
	//apply aura to all appropriate nearby units
	const int aura_range = this->get_aura_range();

	std::vector<CUnit *> table;
	SelectAroundUnit<true>(*this, aura_range, table, MakeOrPredicate(HasSamePlayerAs(*this->Player), IsAlliedWith(*this->Player)));

	for (CUnit *nearby_unit : table) {
		nearby_unit->ApplyAuraEffect(aura_index);
	}
	
	table.clear();

	SelectAroundUnit<true>(*this, aura_range, table, MakeOrPredicate(MakeOrPredicate(HasSamePlayerAs(*this->Player), IsAlliedWith(*this->Player)), HasSamePlayerAs(*CPlayer::get_neutral_player())));

	for (CUnit *nearby_unit : table) {
		if (nearby_unit->has_units_inside()) {
			for (CUnit *uins : nearby_unit->get_units_inside()) {
				if (uins->Player == this->Player || uins->is_allied_with(*this->Player)) {
					uins->ApplyAuraEffect(aura_index);
				}
			}
		}
	}
}

void CUnit::ApplyAuraEffect(const int aura_index)
{
	status_effect status_effect;

	switch (aura_index) {
		case LEADERSHIPAURA_INDEX:
			if (this->Type->BoolFlag[BUILDING_INDEX].value) {
				return;
			}

			status_effect = status_effect::leadership;
			break;
		case REGENERATIONAURA_INDEX:
			if (!this->Type->BoolFlag[ORGANIC_INDEX].value || this->Variable[HP_INDEX].Value >= this->GetModifiedVariable(HP_INDEX, VariableAttribute::Max)) {
				return;
			}

			status_effect = status_effect::regeneration;
			break;
		case HYDRATINGAURA_INDEX:
			if (!this->Type->BoolFlag[ORGANIC_INDEX].value) {
				return;
			}

			this->remove_status_effect(status_effect::dehydration);

			status_effect = status_effect::hydrating;
			break;
		default:
			return;
	}

	this->apply_status_effect(status_effect, CYCLES_PER_SECOND + 1);
}

void CUnit::SetPrefix(const CUpgrade *prefix)
{
	if (Prefix != nullptr) {
		for (const auto &modifier : Prefix->get_modifiers()) {
			RemoveIndividualUpgradeModifier(*this, modifier.get());
		}
		this->Variable[MAGICLEVEL_INDEX].Value -= Prefix->get_magic_level();
		this->Variable[MAGICLEVEL_INDEX].Max -= Prefix->get_magic_level();
	}
	if (game::get()->is_persistency_enabled() && this->Container != nullptr && this->Container->get_character() != nullptr && this->Container->Player == CPlayer::GetThisPlayer() && this->Container->get_character()->has_item(this) && this->Container->get_character()->get_item(this)->Prefix != prefix) {
		//update the persistent item, if applicable and if it hasn't been updated yet
		this->Container->get_character()->get_item(this)->Prefix = prefix;
		this->Container->get_character()->save();
	}
	Prefix = prefix;
	if (Prefix != nullptr) {
		for (const auto &modifier : Prefix->get_modifiers()) {
			ApplyIndividualUpgradeModifier(*this, modifier.get());
		}
		this->Variable[MAGICLEVEL_INDEX].Value += Prefix->get_magic_level();
		this->Variable[MAGICLEVEL_INDEX].Max += Prefix->get_magic_level();
	}
	
	this->UpdateItemName();
}

void CUnit::SetSuffix(const CUpgrade *suffix)
{
	if (Suffix != nullptr) {
		for (const auto &modifier : Suffix->get_modifiers()) {
			RemoveIndividualUpgradeModifier(*this, modifier.get());
		}
		this->Variable[MAGICLEVEL_INDEX].Value -= Suffix->get_magic_level();
		this->Variable[MAGICLEVEL_INDEX].Max -= Suffix->get_magic_level();
	}
	if (game::get()->is_persistency_enabled() && Container && Container->get_character() != nullptr && Container->Player == CPlayer::GetThisPlayer() && Container->get_character()->has_item(this) && Container->get_character()->get_item(this)->Suffix != suffix) {
		//update the persistent item, if applicable and if it hasn't been updated yet
		this->Container->get_character()->get_item(this)->Suffix = suffix;
		this->Container->get_character()->save();
	}
	Suffix = suffix;
	if (Suffix != nullptr) {
		for (const auto &modifier : Suffix->get_modifiers()) {
			ApplyIndividualUpgradeModifier(*this, modifier.get());
		}
		this->Variable[MAGICLEVEL_INDEX].Value += Suffix->get_magic_level();
		this->Variable[MAGICLEVEL_INDEX].Max += Suffix->get_magic_level();
	}
	
	this->UpdateItemName();
}

void CUnit::SetSpell(const wyrmgus::spell *spell)
{
	if (game::get()->is_persistency_enabled() && Container && Container->get_character() != nullptr && Container->Player == CPlayer::GetThisPlayer() && Container->get_character()->has_item(this) && Container->get_character()->get_item(this)->Spell != spell) { //update the persistent item, if applicable and if it hasn't been updated yet
		this->Container->get_character()->get_item(this)->Spell = spell;
		this->Container->get_character()->save();
	}
	Spell = spell;
	
	this->UpdateItemName();
}

void CUnit::SetWork(const CUpgrade *work)
{
	if (this->Work != nullptr) {
		this->Variable[MAGICLEVEL_INDEX].Value -= this->Work->get_magic_level();
		this->Variable[MAGICLEVEL_INDEX].Max -= this->Work->get_magic_level();
	}
	
	if (game::get()->is_persistency_enabled() && Container && Container->get_character() != nullptr && Container->Player == CPlayer::GetThisPlayer() && Container->get_character()->has_item(this) && Container->get_character()->get_item(this)->Work != work) { //update the persistent item, if applicable and if it hasn't been updated yet
		this->Container->get_character()->get_item(this)->Work = work;
		this->Container->get_character()->save();
	}
	
	Work = work;
	
	if (this->Work != nullptr) {
		this->Variable[MAGICLEVEL_INDEX].Value += this->Work->get_magic_level();
		this->Variable[MAGICLEVEL_INDEX].Max += this->Work->get_magic_level();
	}
	
	this->UpdateItemName();
}

void CUnit::SetElixir(const CUpgrade *elixir)
{
	if (this->Elixir != nullptr) {
		this->Variable[MAGICLEVEL_INDEX].Value -= this->Elixir->get_magic_level();
		this->Variable[MAGICLEVEL_INDEX].Max -= this->Elixir->get_magic_level();
	}
	
	if (game::get()->is_persistency_enabled() && Container && Container->get_character() != nullptr && Container->Player == CPlayer::GetThisPlayer() && Container->get_character()->has_item(this) && Container->get_character()->get_item(this)->Elixir != elixir) { //update the persistent item, if applicable and if it hasn't been updated yet
		this->Container->get_character()->get_item(this)->Elixir = elixir;
		this->Container->get_character()->save();
	}
	
	Elixir = elixir;
	
	if (this->Elixir != nullptr) {
		this->Variable[MAGICLEVEL_INDEX].Value += this->Elixir->get_magic_level();
		this->Variable[MAGICLEVEL_INDEX].Max += this->Elixir->get_magic_level();
	}
	
	this->UpdateItemName();
}

void CUnit::set_unique(const wyrmgus::unique_item *unique)
{
	if (this->get_unique() != nullptr && this->get_unique()->get_set() != nullptr) {
		this->Variable[MAGICLEVEL_INDEX].Value -= this->get_unique()->get_set()->get_magic_level();
		this->Variable[MAGICLEVEL_INDEX].Max -= this->get_unique()->get_set()->get_magic_level();
	}
	
	if (unique != nullptr) {
		SetPrefix(unique->get_prefix());
		SetSuffix(unique->get_suffix());
		SetSpell(unique->get_spell());
		SetWork(unique->get_work());
		SetElixir(unique->get_elixir());
		if (unique->get_resources_held() != 0) {
			this->SetResourcesHeld(unique->get_resources_held());
			this->Variable[GIVERESOURCE_INDEX].Value = unique->get_resources_held();
			this->Variable[GIVERESOURCE_INDEX].Max = unique->get_resources_held();
			this->Variable[GIVERESOURCE_INDEX].Enable = 1;
		}
		if (unique->get_set() != nullptr) {
			this->Variable[MAGICLEVEL_INDEX].Value += unique->get_set()->get_magic_level();
			this->Variable[MAGICLEVEL_INDEX].Max += unique->get_set()->get_magic_level();
		}
		this->Name = unique->get_name();
		this->unique = unique;
	} else {
		this->Name.clear();
		this->unique = nullptr;
		SetPrefix(nullptr);
		SetSuffix(nullptr);
		SetSpell(nullptr);
		SetWork(nullptr);
		SetElixir(nullptr);
	}
}

void CUnit::set_site(const wyrmgus::site *site)
{
	if (site == this->get_site()) {
		return;
	}

	if (this->site != nullptr && this->site->get_game_data()->get_site_unit() == this) {
		this->site->get_game_data()->set_site_unit(nullptr);
	}

	this->site = site;

	if (site != nullptr) {
		site->get_game_data()->set_site_unit(this);

		if (!SaveGameLoading) {
			if (this->Type->BoolFlag[TOWNHALL_INDEX].value && site->is_settlement() && this->Player->get_capital_settlement() == nullptr) {
				this->Player->set_capital_settlement(site);
			}
		}
	}
}

void CUnit::set_settlement(const wyrmgus::site *settlement)
{
	if (settlement == this->get_settlement()) {
		return;
	}

	const wyrmgus::site *old_settlement = this->get_settlement();

	this->settlement = settlement;

	if (defines::get()->is_population_enabled() && !this->is_under_construction() && this->IsAlive()) {
		if (old_settlement != nullptr) {
			old_settlement->get_game_data()->on_settlement_building_removed(this);
		}

		if (this->get_settlement() != nullptr) {
			this->get_settlement()->get_game_data()->on_settlement_building_added(this);
		}
	}
}

void CUnit::update_site_owner()
{
	if (this->get_site() == nullptr) {
		return;
	}

	if (this->is_under_construction()) {
		return;
	}

	site_game_data *site_game_data = this->get_site()->get_game_data();

	if (site_game_data->get_site_unit() != this) {
		return;
	}

	if (this->Player->get_index() != PlayerNumNeutral) {
		site_game_data->set_owner(this->Player);
	} else {
		site_game_data->set_owner(nullptr);
	}
}

void CUnit::Identify()
{
	if (game::get()->is_persistency_enabled() && Container && Container->get_character() != nullptr && Container->Player == CPlayer::GetThisPlayer() && Container->get_character()->has_item(this) && !this->Container->get_character()->get_item(this)->is_identified()) { //update the persistent item, if applicable and if it hasn't been updated yet
		this->Container->get_character()->get_item(this)->set_identified(true);
		this->Container->get_character()->save();
	}
	
	this->Identified = true;
	
	if (this->Container != nullptr && this->Container->Player == CPlayer::GetThisPlayer()) {
		this->Container->Player->Notify(notification_type::green, this->Container->tilePos, this->Container->MapLayer->ID, _("%s has identified the %s!"), this->Container->GetMessageName().c_str(), this->GetMessageName().c_str());
	}
}

void CUnit::CheckIdentification()
{
	if (!HasInventory()) {
		return;
	}
	
	for (CUnit *uins : this->get_units_inside()) {
		if (!uins->Type->BoolFlag[ITEM_INDEX].value) {
			continue;
		}
		
		if (!uins->Identified && this->Variable[KNOWLEDGEMAGIC_INDEX].Value >= uins->Variable[MAGICLEVEL_INDEX].Value) {
			uins->Identify();
		}
	}
}

void CUnit::CheckKnowledgeChange(int variable, int change) // this happens after the variable has already been changed
{
	if (!change) {
		return;
	}
	
	if (variable == KNOWLEDGEMAGIC_INDEX) {
		int mana_change = (this->Variable[variable].Value / 5) - ((this->Variable[variable].Value - change) / 5); // +1 max mana for every 5 levels in Knowledge (Magic)
		this->Variable[MANA_INDEX].Max += mana_change;
		if (mana_change < 0) {
			this->Variable[MANA_INDEX].Value += mana_change;
		}
		
		this->CheckIdentification();
	} else if (variable == KNOWLEDGEWARFARE_INDEX) {
		int hp_change = (this->Variable[variable].Value / 5) - ((this->Variable[variable].Value - change) / 5); // +1 max HP for every 5 levels in Knowledge (Warfare)
		this->Variable[HP_INDEX].Max += hp_change;
		this->Variable[HP_INDEX].Value += hp_change;
	} else if (variable == KNOWLEDGEMINING_INDEX) {
		int stat_change = (this->Variable[variable].Value / 25) - ((this->Variable[variable].Value - change) / 25); // +1 mining gathering bonus for every 25 levels in Knowledge (Mining)
		this->Variable[COPPERGATHERINGBONUS_INDEX].Max += stat_change;
		this->Variable[COPPERGATHERINGBONUS_INDEX].Value += stat_change;
		this->Variable[SILVERGATHERINGBONUS_INDEX].Max += stat_change;
		this->Variable[SILVERGATHERINGBONUS_INDEX].Value += stat_change;
		this->Variable[GOLDGATHERINGBONUS_INDEX].Max += stat_change;
		this->Variable[GOLDGATHERINGBONUS_INDEX].Value += stat_change;
		this->Variable[IRONGATHERINGBONUS_INDEX].Max += stat_change;
		this->Variable[IRONGATHERINGBONUS_INDEX].Value += stat_change;
		this->Variable[MITHRILGATHERINGBONUS_INDEX].Max += stat_change;
		this->Variable[MITHRILGATHERINGBONUS_INDEX].Value += stat_change;
		this->Variable[COALGATHERINGBONUS_INDEX].Max += stat_change;
		this->Variable[COALGATHERINGBONUS_INDEX].Value += stat_change;
		this->Variable[GEMSGATHERINGBONUS_INDEX].Max += stat_change;
		this->Variable[GEMSGATHERINGBONUS_INDEX].Value += stat_change;
	}
}

void CUnit::UpdateItemName()
{
	if (this->get_unique() != nullptr) {
		this->Name = _(this->get_unique()->get_name().c_str());
		return;
	}
	
	this->Name.clear();
	if (this->Prefix == nullptr && this->Spell == nullptr && this->Work == nullptr && this->Suffix == nullptr) { //elixirs use the name of their unit type
		return;
	}
	
	if (this->Prefix != nullptr) {
		this->Name += _(this->Prefix->get_name().c_str());
		this->Name += " ";
	}
	if (this->Work != nullptr) {
		this->Name += _(this->Work->get_name().c_str());
	} else {
		this->Name += this->get_type_name();
	}
	if (this->Suffix != nullptr) {
		this->Name += " ";
		this->Name += _(this->Suffix->get_name().c_str());
	} else if (this->Spell != nullptr) {
		this->Name += " ";
		this->Name += _("of");
		this->Name += " ";
		this->Name += _(this->Spell->get_name().c_str());
	}
}

void CUnit::generate_traits()
{
	const int count = CUnit::min_traits - static_cast<int>(this->get_traits().size());

	if (count <= 0) {
		return;
	}

	for (int i = 0; i < count; ++i) {
		this->generate_trait();
	}
}

void CUnit::generate_trait()
{
	std::vector<const CUpgrade *> potential_traits;

	for (const CUpgrade *trait : this->Type->get_traits()) {
		if (vector::contains(this->get_traits(), trait)) {
			continue;
		}

		if (!trait->check_unit_conditions(this)) {
			continue;
		}

		for (int i = 0; i < trait->get_weight(); ++i) {
			potential_traits.push_back(trait);
		}
	}

	if (potential_traits.empty()) {
		return;
	}

	this->add_trait(vector::get_random(potential_traits));
}

void CUnit::add_trait(const CUpgrade *trait)
{
	this->traits.push_back(trait);

	IndividualUpgradeAcquire(*this, trait);

	this->update_epithet();

	//upgrades could change the buttons displayed.
	if (this->Player == CPlayer::GetThisPlayer()) {
		SelectedUnitChanged();
	}
}

void CUnit::remove_traits()
{
	for (const CUpgrade *trait : this->get_traits()) {
		IndividualUpgradeLost(*this, trait);
	}

	this->traits.clear();
}

void CUnit::GenerateDrop()
{
	bool base_based_mission = false;
	for (int p = 0; p < PlayerMax; ++p) {
		if (CPlayer::Players[p]->NumTownHalls > 0 || CPlayer::Players[p]->LostTownHallTimer) {
			base_based_mission = true;
		}
	}
	
	if (this->Type->BoolFlag[ORGANIC_INDEX].value && this->get_character() == nullptr && !this->Type->BoolFlag[FAUNA_INDEX].value && base_based_mission) { //if the unit is organic and isn't a character (and isn't fauna) and this is a base-based mission, don't generate a drop
		return;
	}
	
	Vec2i drop_pos = this->tilePos;
	drop_pos.x += SyncRand(this->Type->get_tile_width());
	drop_pos.y += SyncRand(this->Type->get_tile_height());
	CUnit *droppedUnit = nullptr;
	wyrmgus::unit_type *chosen_drop = nullptr;
	std::vector<wyrmgus::unit_type *> potential_drops;
	for (size_t i = 0; i < this->Type->Drops.size(); ++i) {
		if (check_conditions(this->Type->Drops[i], this)) {
			potential_drops.push_back(this->Type->Drops[i]);
		}
	}

	if (this->Player->AiEnabled) {
		for (size_t i = 0; i < this->Type->AiDrops.size(); ++i) {
			if (check_conditions(this->Type->AiDrops[i], this)) {
				potential_drops.push_back(this->Type->AiDrops[i]);
			}
		}
	}

	if (potential_drops.size() > 0) {
		chosen_drop = potential_drops[SyncRand(potential_drops.size())];
	}
		
	if (chosen_drop != nullptr) {
		const on_top_build_restriction *ontop_b = OnTopDetails(*this->Type, nullptr);
		if (((chosen_drop->BoolFlag[ITEM_INDEX].value || chosen_drop->BoolFlag[POWERUP_INDEX].value) && this->MapLayer->Field(drop_pos)->has_flag(tile_flag::item)) || (ontop_b && ontop_b->ReplaceOnDie)) { //if the dropped unit is an item (and there's already another item there), or if this building is an ontop one (meaning another will appear under it after it is destroyed), search for another spot
			const QPoint res_pos = FindNearestDrop(*chosen_drop, drop_pos, LookingW, this->MapLayer->ID);
			droppedUnit = MakeUnitAndPlace(res_pos, *chosen_drop, CPlayer::get_neutral_player(), this->MapLayer->ID);
		} else {
			droppedUnit = MakeUnitAndPlace(drop_pos, *chosen_drop, CPlayer::get_neutral_player(), this->MapLayer->ID);
		}
			
		if (droppedUnit != nullptr) {
			if (droppedUnit->Type->BoolFlag[FAUNA_INDEX].value) {
				droppedUnit->Name = droppedUnit->Type->generate_personal_name(nullptr, droppedUnit->get_gender());
			}
			
			droppedUnit->generate_special_properties(this, this->Player, true, false, false);
			
			if (droppedUnit->Type->BoolFlag[ITEM_INDEX].value && droppedUnit->get_unique() == nullptr) { //save the initial cycle items were placed in the ground to destroy them if they have been there for too long
				int ttl_cycles = (5 * 60 * CYCLES_PER_SECOND);
				if (droppedUnit->Prefix != nullptr || droppedUnit->Suffix != nullptr || droppedUnit->Spell != nullptr || droppedUnit->Work != nullptr || droppedUnit->Elixir != nullptr) {
					ttl_cycles *= 4;
				}
				droppedUnit->TTL = GameCycle + ttl_cycles;
			}
		}
	}
}

void CUnit::generate_special_properties(const CUnit *dropper, const CPlayer *dropper_player, const bool allow_unique, const bool sold_item, const bool always_magic)
{
	assert_log(dropper_player != nullptr);

	int magic_affix_chance = 10; //10% chance of the unit having a magic prefix or suffix
	int unique_chance = 5; //0.5% chance of the unit being unique
	if (dropper != nullptr) {
		if (dropper->get_character() != nullptr) {
			//if the dropper is a character, multiply the chances of the item being magic or unique by the character's level
			magic_affix_chance *= dropper->get_character()->get_level();
			unique_chance *= dropper->get_character()->get_level();
		} else if (dropper->Type->BoolFlag[BUILDING_INDEX].value) {
			//if the dropper is a building, multiply the chances of the drop being magic or unique by a factor according to whether the building itself is magic/unique
			int chance_multiplier = 2;
			if (dropper->get_unique() != nullptr) {
				chance_multiplier += 8;
			} else {
				if (dropper->Prefix != nullptr) {
					chance_multiplier += 1;
				}
				if (dropper->Suffix != nullptr) {
					chance_multiplier += 1;
				}
			}
			magic_affix_chance *= chance_multiplier;
			unique_chance *= chance_multiplier;
		}
	}
	
	if (sold_item) {
		magic_affix_chance /= 4;
		unique_chance /= 4;
	}

	if (allow_unique) {
		const bool is_unique = SyncRand(1000) >= (1000 - unique_chance);
		if (is_unique) {
			this->generate_unique(dropper, dropper_player);
		}
	}

	if (this->get_unique() == nullptr) {
		bool must_be_magic = false;

		switch (this->Type->get_item_class()) {
			case item_class::scroll:
			case item_class::book:
			case item_class::ring:
			case item_class::amulet:
			case item_class::horn:
			case item_class::trinket:
				//scrolls, books, jewelry, horns and trinkets must always have a property
				must_be_magic = true;
				break;
			default:
				if (always_magic) {
					must_be_magic = true;
				}
				break;
		}

		const bool is_magic = must_be_magic || (SyncRand(100) >= (100 - magic_affix_chance));
		if (is_magic) {
			std::vector<int> magic_types{0, 1, 2, 3};

			while (!magic_types.empty()) {
				const int magic_type = vector::take_random(magic_types);

				switch (magic_type) {
					case 0:
						this->generate_work(dropper, dropper_player);
						break;
					case 1:
						this->generate_prefix(dropper, dropper_player);

						//chance for a suffix to also be generated
						if (SyncRand(100) >= (100 - magic_affix_chance)) {
							this->generate_suffix(dropper, dropper_player);
						}
						break;
					case 2:
						this->generate_suffix(dropper, dropper_player);

						//chance for a prefix to also be generated
						if (SyncRand(100) >= (100 - magic_affix_chance)) {
							this->generate_prefix(dropper, dropper_player);
						}
						break;
					case 3:
						this->generate_spell(dropper, dropper_player);
						break;
				}

				if (this->Prefix != nullptr || this->Suffix != nullptr || this->Work != nullptr || this->Elixir != nullptr || this->Spell != nullptr) {
					break;
				}
			}
		}
	}
	
	if (this->Type->BoolFlag[ITEM_INDEX].value && (this->Prefix != nullptr || this->Suffix != nullptr)) {
		this->Identified = false;
	}
}

void CUnit::generate_prefix(const CUnit *dropper, const CPlayer *dropper_player)
{
	std::vector<const CUpgrade *> potential_prefixes;

	if (dropper_player != nullptr) {
		for (const CUpgrade *affix : this->Type->get_affixes()) {
			if (!affix->is_magic_prefix()) {
				continue;
			}

			//check drop conditions, but not for non-capturable neutral buildings
			if (!dropper_player->is_neutral_player() || !this->Type->BoolFlag[BUILDING_INDEX].value || this->is_capturable()) {
				if (!affix->check_drop_conditions(dropper, dropper_player)) {
					continue;
				}
			}

			potential_prefixes.push_back(affix);
		}

		for (const CUpgrade *upgrade : CUpgrade::get_all()) {
			if (!upgrade->is_magic_prefix()) {
				continue;
			}

			if (this->Type->get_item_class() == item_class::none || !upgrade->has_affixed_item_class(Type->get_item_class())) {
				continue;
			}

			if (!upgrade->check_drop_conditions(dropper, dropper_player)) {
				continue;
			}

			potential_prefixes.push_back(upgrade);
		}
	}
	
	if (!potential_prefixes.empty()) {
		this->SetPrefix(vector::get_random(potential_prefixes));
	}
}

void CUnit::generate_suffix(const CUnit *dropper, const CPlayer *dropper_player)
{
	std::vector<const CUpgrade *> potential_suffixes;

	if (dropper_player != nullptr) {
		for (const CUpgrade *affix : this->Type->get_affixes()) {
			if (!affix->is_magic_suffix()) {
				continue;
			}

			//check drop conditions, but not for non-capturable neutral buildings
			if (!dropper_player->is_neutral_player() || !this->Type->BoolFlag[BUILDING_INDEX].value || this->is_capturable()) {
				if (!affix->check_drop_conditions(dropper, dropper_player)) {
					continue;
				}
			}

			if (this->Prefix != nullptr && affix->is_incompatible_affix(this->Prefix)) {
				//don't allow a suffix incompatible with the prefix to appear
				continue;
			}

			potential_suffixes.push_back(affix);
		}

		for (const CUpgrade *upgrade : CUpgrade::get_all()) {
			if (!upgrade->is_magic_suffix()) {
				continue;
			}

			if (this->Type->get_item_class() == item_class::none || !upgrade->has_affixed_item_class(this->Type->get_item_class())) {
				continue;
			}

			if (!upgrade->check_drop_conditions(dropper, dropper_player)) {
				continue;
			}

			if (this->Prefix != nullptr && upgrade->is_incompatible_affix(this->Prefix)) {
				//don't allow a suffix incompatible with the prefix to appear
				continue;
			}

			potential_suffixes.push_back(upgrade);
		}
	}
	
	if (!potential_suffixes.empty()) {
		this->SetSuffix(vector::get_random(potential_suffixes));
	}
}

void CUnit::generate_spell(const CUnit *dropper, const CPlayer *dropper_player)
{
	std::vector<const spell *> potential_spells;
	if (dropper != nullptr) {
		for (const spell *spell : dropper->Type->DropSpells) {
			if (this->Type->get_item_class() != item_class::none && spell->ItemSpell[static_cast<int>(Type->get_item_class())]) {
				potential_spells.push_back(spell);
			}
		}
	}
	
	if (!potential_spells.empty()) {
		this->SetSpell(vector::get_random(potential_spells));
	}
}

void CUnit::generate_work(const CUnit *dropper, const CPlayer *dropper_player)
{
	if (this->Type->get_item_class() == item_class::none) {
		return;
	}

	std::vector<const CUpgrade *> potential_works;

	if (dropper_player != nullptr) {
		for (const CUpgrade *upgrade : CUpgrade::get_all()) {
			if (upgrade->Work == item_class::none) {
				continue;
			}

			if (upgrade->Work != this->Type->get_item_class()) {
				continue;
			}

			if (upgrade->UniqueOnly) {
				continue;
			}

			if (!upgrade->check_drop_conditions(dropper, dropper_player)) {
				continue;
			}

			potential_works.push_back(upgrade);
		}
	}
	
	if (!potential_works.empty()) {
		this->SetWork(vector::get_random(potential_works));
	}
}

void CUnit::generate_unique(const CUnit *dropper, const CPlayer *dropper_player)
{
	std::vector<const unique_item *> potential_uniques;

	for (const unique_item *unique : unique_item::get_all()) {
		if (this->Type != unique->get_unit_type()) {
			continue;
		}

		if (unique->get_prefix() != nullptr) {
			//the dropper unit must be capable of generating this unique item's prefix to drop the item
			if (dropper_player == nullptr) {
				continue;
			}

			if (!dropper_player->is_neutral_player() || !this->Type->BoolFlag[BUILDING_INDEX].value || this->is_capturable()) {
				if (!unique->get_prefix()->check_drop_conditions(dropper, dropper_player)) {
					continue;
				}
			}
		}

		if (unique->get_suffix() != nullptr) {
			//the dropper unit must be capable of generating this unique item's suffix to drop the item
			if (dropper_player == nullptr) {
				continue;
			}

			if (!dropper_player->is_neutral_player() || !this->Type->BoolFlag[BUILDING_INDEX].value || this->is_capturable()) {
				if (!unique->get_suffix()->check_drop_conditions(dropper, dropper_player)) {
					continue;
				}
			}
		}

		if (unique->get_set() != nullptr) {
			//the dropper unit must be capable of generating this unique item's set to drop the item
			if (dropper_player == nullptr) {
				continue;
			}

			if (!unique->get_set()->check_drop_conditions(dropper, dropper_player)) {
				continue;
			}
		}

		if (unique->get_spell() != nullptr) {
			//the dropper unit must be capable of generating this unique item's spell to drop the item
			if (dropper == nullptr) {
				continue;
			}

			if (!vector::contains(dropper->Type->DropSpells, unique->get_spell())) {
				continue;
			}
		}

		if (unique->get_work() != nullptr) {
			//the dropper unit must be capable of generating this unique item's work to drop the item
			if (dropper_player == nullptr) {
				continue;
			}

			if (!unique->get_work()->check_drop_conditions(dropper, dropper_player)) {
				continue;
			}
		}

		if (unique->get_elixir() != nullptr) {
			//the dropper unit must be capable of generating this unique item's elixir to drop the item
			if (dropper_player == nullptr) {
				continue;
			}

			if (!unique->get_elixir()->check_drop_conditions(dropper, dropper_player)) {
				continue;
			}
		}

		if (!unique->can_drop()) {
			continue;
		}

		potential_uniques.push_back(unique);
	}
	
	if (!potential_uniques.empty()) {
		const unique_item *chosen_unique = vector::get_random(potential_uniques);
		this->set_unique(chosen_unique);
	}
}

void CUnit::UpdateSoldUnits()
{
	static constexpr int recruitable_hero_max = 4;
	static constexpr int sold_item_max = 15;

	if (!this->Type->BoolFlag[RECRUITHEROES_INDEX].value && this->Type->SoldUnits.empty() && this->SoldUnits.empty()) {
		return;
	}
	
	if (this->UnderConstruction == 1 || !CMap::get()->Info->IsPointOnMap(this->tilePos, this->MapLayer) || CEditor::get()->is_running()) {
		return;
	}
	
	for (CUnit *sold_unit : this->SoldUnits) {
		DestroyAllInside(*sold_unit);
		LetUnitDie(*sold_unit);
	}
	this->SoldUnits.clear();
	
	std::vector<const unit_type *> potential_items;
	std::vector<wyrmgus::character *> potential_heroes;

	if (this->Type->BoolFlag[RECRUITHEROES_INDEX].value && !IsNetworkGame()) {
		//allow heroes to be recruited at town halls
		const civilization *civilization = this->get_civilization();
		const faction *faction = this->Player->get_faction();
		
		if (CurrentQuest == nullptr) {
			//give priority to heroes with the building's settlement as their home settlement
			if (this->settlement != nullptr) {
				potential_heroes = this->Player->get_recruitable_heroes_from_list(this->settlement->get_characters());
			}

			//then give priority to heroes belonging to the building's player's faction
			if (faction != nullptr && static_cast<int>(potential_heroes.size()) < recruitable_hero_max) {
				std::vector<wyrmgus::character *> potential_faction_heroes = this->Player->get_recruitable_heroes_from_list(faction->get_characters());

				while (!potential_faction_heroes.empty() && static_cast<int>(potential_heroes.size()) < recruitable_hero_max) {
					wyrmgus::character *hero = vector::take_random(potential_faction_heroes);

					if (vector::contains(potential_heroes, hero)) {
						continue;
					}

					potential_heroes.push_back(hero);
				}
			}

			//if there are still recruitable hero slots available, then try to place characters belonging to the civilization in them
			if (civilization != nullptr && static_cast<int>(potential_heroes.size()) < recruitable_hero_max) {
				std::vector<wyrmgus::character *> potential_civilization_heroes = this->Player->get_recruitable_heroes_from_list(civilization->get_characters());

				while (!potential_civilization_heroes.empty() && static_cast<int>(potential_heroes.size()) < recruitable_hero_max) {
					wyrmgus::character *hero = vector::take_random(potential_civilization_heroes);

					if (vector::contains(potential_heroes, hero)) {
						continue;
					}

					potential_heroes.push_back(hero);
				}
			}
		}

		if (this->Player == CPlayer::GetThisPlayer()) {
			for (wyrmgus::character *custom_hero : character::get_custom_heroes()) {
				if (
					(custom_hero->get_civilization() != nullptr && (custom_hero->get_civilization() == civilization || custom_hero->get_unit_type() == civilization->get_class_unit_type(custom_hero->get_unit_type()->get_unit_class())))
					&& check_conditions(custom_hero->get_unit_type(), this, true) && custom_hero->CanAppear()
				) {
					potential_heroes.push_back(custom_hero);
				}
			}
		}
	} else {
		for (const unit_type *sold_unit_type : this->Type->SoldUnits) {
			if (check_conditions(sold_unit_type, this)) {
				potential_items.push_back(sold_unit_type);
			}
		}

		if (this->Type->BoolFlag[MARKET_INDEX].value) {
			for (const CPlayer *trade_partner : this->Player->get_recent_trade_partners()) {
				const unit_type *market_type = trade_partner->get_class_unit_type(this->Type->get_unit_class());

				if (market_type == nullptr) {
					continue;
				}

				for (const unit_type *sold_unit_type : market_type->SoldUnits) {
					if (!check_conditions(sold_unit_type, trade_partner)) {
						continue;
					}

					if (vector::contains(potential_items, sold_unit_type)) {
						continue;
					}

					potential_items.push_back(sold_unit_type);
				}
			}
		}
	}
	
	if (potential_items.empty() && potential_heroes.empty()) {
		return;
	}
	
	int sold_unit_max = recruitable_hero_max;
	if (!potential_items.empty()) {
		sold_unit_max = sold_item_max;
	}
	
	for (int i = 0; i < sold_unit_max; ++i) {
		CUnit *new_unit = nullptr;
		if (!potential_heroes.empty()) {
			wyrmgus::character *chosen_hero = vector::take_random(potential_heroes);
			new_unit = MakeUnitAndPlace(this->tilePos, *chosen_hero->get_unit_type(), CPlayer::get_neutral_player(), this->MapLayer->ID);
			new_unit->set_character(chosen_hero);
		} else {
			const unit_type *chosen_unit_type = vector::get_random(potential_items);
			new_unit = MakeUnitAndPlace(this->tilePos, *chosen_unit_type, CPlayer::get_neutral_player(), this->MapLayer->ID);
			new_unit->generate_special_properties(this, this->Player, true, true, false);
			new_unit->Identified = true;
			if (new_unit->get_unique() != nullptr && this->Player == CPlayer::GetThisPlayer()) { //send a notification if a unique item is being sold, we don't want the player to have to worry about missing it :)
				this->Player->Notify(notification_type::green, this->tilePos, this->MapLayer->ID, "%s", _("Unique item available for sale"));
			}
		}
		new_unit->Remove(this);
		this->SoldUnits.push_back(new_unit);
		if (potential_heroes.empty() && potential_items.empty()) {
			break;
		}
	}

	if (IsOnlySelected(*this)) {
		UI.ButtonPanel.Update();
	}
}

void CUnit::SellUnit(CUnit *sold_unit, CPlayer *player)
{
	vector::remove(this->SoldUnits, sold_unit);

	sold_unit->drop_out_on_side(sold_unit->Direction, this);

	if (!sold_unit->Type->BoolFlag[ITEM_INDEX].value) {
		sold_unit->ChangeOwner(*player);

		if (player != this->Player) {
			sold_unit->set_player_from(this->Player);
		}
	}

	player->change_resource(defines::get()->get_wealth_resource(), -sold_unit->get_price(), true);

	if (player->AiEnabled && !sold_unit->Type->BoolFlag[ITEM_INDEX].value && !sold_unit->Type->BoolFlag[HARVESTER_INDEX].value) { //add the hero to an AI force, if the hero isn't a harvester
		player->Ai->Force.remove_dead_units();
		player->Ai->Force.Assign(*sold_unit, -1, true);
	}

	if (sold_unit->get_character() != nullptr) {
		player->HeroCooldownTimer = HeroCooldownCycles;
		sold_unit->Variable[MANA_INDEX].Value = 0; //start off with 0 mana
	}

	if (IsOnlySelected(*this)) {
		UI.ButtonPanel.Update();
	}
}

void CUnit::spawn_units()
{
	std::vector<const unit_type *> spawned_units;

	if (!this->Type->get_spawned_units().empty()) {
		spawned_units = this->Type->get_spawned_units();
	}

	if (!this->Type->get_neutral_spawned_units().empty() && this->Player->is_neutral_player()) {
		vector::merge(spawned_units, this->Type->get_neutral_spawned_units());
	}

	//spawn neutral hostile units for minor faction buildings if their settlement has no owner
	if (this->Player->has_neutral_faction_type() && this->Type->BoolFlag[BUILDING_INDEX].value && this->get_settlement() != nullptr && this->get_settlement()->get_game_data()->get_owner() == nullptr) {
		std::vector<const unit_type *> stocked_unit_types;

		for (const auto &[unit_type, unit_stock] : this->get_unit_stocks()) {
			stocked_unit_types.push_back(unit_type);
		}

		for (const auto &[unit_class, unit_stock] : this->get_unit_class_stocks()) {
			const unit_type *unit_type = this->Player->get_class_unit_type(unit_class);

			if (unit_type == nullptr) {
				continue;
			}

			stocked_unit_types.push_back(unit_type);
		}

		std::erase_if(stocked_unit_types, [this](const unit_type *unit_type) {
			if (!unit_type->BoolFlag[NEUTRAL_HOSTILE_INDEX].value) {
				return true;
			}

			if (!check_conditions(unit_type, this->Player, false, false)) {
				return true;
			}

			return false;
		});

		vector::merge(spawned_units, std::move(stocked_unit_types));
	}

	if (spawned_units.empty()) {
		return;
	}

	this->spawn_units(spawned_units);
}

void CUnit::spawn_units(const std::vector<const unit_type *> &spawned_types)
{
	//spawn a random unit type, with the chance for a type to be picked being inversely weighted by demand

	int max_spawned_type_demand = 0;
	for (const unit_type *spawned_type : spawned_types) {
		const int spawned_type_demand = spawned_type->Stats[this->Player->get_index()].Variables[DEMAND_INDEX].Value;

		if (spawned_type_demand > max_spawned_type_demand) {
			max_spawned_type_demand = spawned_type_demand;
		}
	}

	std::vector<const unit_type *> weighted_spawned_types;
	int weighted_average_demand = 0;

	for (const unit_type *spawned_type : spawned_types) {
		const int spawned_type_demand = spawned_type->Stats[this->Player->get_index()].Variables[DEMAND_INDEX].Value;
		const int inverse_demand_weight = (max_spawned_type_demand + 1) - spawned_type_demand;

		weighted_average_demand += spawned_type_demand * inverse_demand_weight;

		for (int i = 0; i < inverse_demand_weight; ++i) {
			weighted_spawned_types.push_back(spawned_type);
		}
	}

	weighted_average_demand /= static_cast<int>(weighted_spawned_types.size());

	//1% chance to spawn a unit in a second, with the chance decreasing proportionally to the weighted average demand
	const int random_chance = 100 * std::max(weighted_average_demand, 1);
	const int random_result = random::get()->generate(random_chance);
	if (random_result != 0) {
		return;
	}

	const int max_spawned_demand = this->Type->get_max_spawned_demand();

	CPlayer *spawned_unit_player = this->Player;
	if (this->Player->has_neutral_faction_type()) {
		spawned_unit_player = CPlayer::get_neutral_player();
	}

	const int spawned_demand = this->get_nearby_spawned_demand(spawned_types, spawned_unit_player);

	if (spawned_demand >= max_spawned_demand) {
		return;
	}

	const unit_type *spawned_type = vector::get_random(weighted_spawned_types);

	CUnit *spawned_unit = MakeUnit(*spawned_type, spawned_unit_player);
	spawned_unit->drop_out_on_side(spawned_unit->Direction, this);

	if (spawned_unit_player != this->Player) {
		spawned_unit->set_player_from(this->Player);
	}
}

int CUnit::get_nearby_spawned_demand(const std::vector<const unit_type *> &spawned_types, const CPlayer *spawned_unit_player) const
{
	static constexpr int nearby_spawned_range = 16;

	int spawned_demand = 0;

	std::vector<CUnit *> nearby_units;
	SelectAroundUnit(*this, nearby_spawned_range, nearby_units, HasSamePlayerAs(*spawned_unit_player));

	for (const CUnit *nearby_unit : nearby_units) {
		if (!vector::contains(spawned_types, nearby_unit->Type)) {
			continue;
		}

		spawned_demand += nearby_unit->Type->Stats[this->Player->get_index()].Variables[DEMAND_INDEX].Value;
	}

	return spawned_demand;
}

/**
**  Produce a resource
**
**  @param resource  Resource to be produced.
*/
void CUnit::ProduceResource(const wyrmgus::resource *resource)
{
	const int resource_index = resource ? resource->get_index() : 0;
	if (resource_index == this->GivesResource) {
		return;
	}

	if (this->Variable[GARRISONED_GATHERING_INDEX].Value > 0 && this->get_given_resource() != nullptr) {
		this->set_garrisoned_gathering_income(0);
	}

	const int old_resource = this->GivesResource;

	if (resource != nullptr) {
		this->GivesResource = resource->get_index();
		this->ResourcesHeld = 10000;
	} else {
		this->GivesResource = 0;
		this->ResourcesHeld = 0;
	}
	
	if (old_resource != 0) {
		for (const std::shared_ptr<unit_ref> &uins_ref : this->Resource.Workers) {
			CUnit *uins = uins_ref->get();
			if (uins->Container == this) {
				uins->CurrentOrder()->Finished = true;
				uins->drop_out_on_side(LookingW, this);
			}
		}
		this->Resource.Active = 0;
	}

	if (this->Variable[GARRISONED_GATHERING_INDEX].Value > 0) {
		if (this->get_given_resource() != nullptr) {
			const std::vector<CUnit *> units_inside = this->get_units_inside();

			for (CUnit *unit : units_inside) {
				if (unit->get_garrisoned_gathering_rate(this->get_given_resource()) == 0) {
					unit->drop_out_on_side(LookingW, this);
				}
			}

			this->update_garrisoned_gathering_income();
		} else {
			if (this->has_units_inside()) {
				DropOutAll(*this);
			}
		}
	}
}

/**
**  Sells 100 of a resource for copper
**
**  @param resource  Resource to be sold.
*/
void CUnit::sell_resource(const resource *resource, const int player)
{
	if (CPlayer::Players[player]->get_resource(resource, resource_storage_type::both) < 100) {
		return;
	}

	CPlayer::Players[player]->change_resource(resource, -100, true);
	CPlayer::Players[player]->change_resource(defines::get()->get_wealth_resource(), this->Player->get_effective_resource_sell_price(resource), true);
	
	this->Player->decrease_resource_price(resource);
}

/**
**  Buy a resource for copper
**
**  @param resource  Resource to be bought.
*/
void CUnit::buy_resource(const resource *resource, const int player)
{
	if (CPlayer::Players[player]->get_resource(defines::get()->get_wealth_resource(), resource_storage_type::both) < this->Player->get_effective_resource_buy_price(resource)) {
		return;
	}

	CPlayer::Players[player]->change_resource(resource, 100, true);
	CPlayer::Players[player]->change_resource(defines::get()->get_wealth_resource(), -this->Player->get_effective_resource_buy_price(resource), true);
	
	this->Player->increase_resource_price(resource);
}

void CUnit::Scout()
{
	int scout_range = std::max(16, this->CurrentSightRange * 2);
			
	QPoint target_pos = this->tilePos;

	target_pos += QPoint(SyncRand(scout_range * 2 + 1) - scout_range, SyncRand(scout_range * 2 + 1) - scout_range);

	// restrict to map
	CMap::get()->clamp(target_pos, this->MapLayer->ID);

	// move if possible
	if (target_pos != this->tilePos) {
		// if the tile the scout is moving to happens to have a layer connector, use it
		bool found_connector = false;
		CUnitCache &unitcache = CMap::get()->Field(target_pos, this->MapLayer->ID)->UnitCache;
		for (CUnitCache::iterator it = unitcache.begin(); it != unitcache.end(); ++it) {
			CUnit *connector = *it;

			if (connector->ConnectingDestination != nullptr && this->CanUseItem(connector)) {
				CommandUse(*this, *connector, FlushCommands);
				found_connector = true;
				break;
			}
		}
		if (found_connector) {
			return;
		}
				
		UnmarkUnitFieldFlags(*this);
		if (UnitCanBeAt(*this, target_pos, this->MapLayer->ID)) {
			MarkUnitFieldFlags(*this);
			CommandMove(*this, target_pos, FlushCommands, this->MapLayer->ID);
			return;
		}
		MarkUnitFieldFlags(*this);
	}
}
//Wyrmgus end

UnitAction CUnit::CurrentAction() const
{
	return this->CurrentOrder()->Action;
}

void CUnit::ClearAction()
{
	Orders[0]->Finished = true;

	if (Selected) {
		SelectedUnitChanged();
	}
}

bool CUnit::IsIdle() const
{
	//Wyrmgus start
//	return Orders.size() == 1 && CurrentAction() == UnitAction::Still;
	return Orders.size() == 1 && CurrentAction() == UnitAction::Still && !this->has_status_effect(status_effect::stun);
	//Wyrmgus end
}

bool CUnit::IsAlive() const
{
	return !Destroyed && CurrentAction() != UnitAction::Die;
}

int CUnit::GetDrawLevel() const
{
	return ((Type->get_corpse_type() != nullptr && CurrentAction() == UnitAction::Die) ?
		Type->get_corpse_type()->get_draw_level() :
	((CurrentAction() == UnitAction::Die) ? Type->get_draw_level() - 10 : Type->get_draw_level()));
}

/**
**  Initialize the unit slot with default values.
**
**  @param type    Unit-type
*/
void CUnit::Init(const wyrmgus::unit_type &type, const bool loading_saved_unit)
{
	if (!loading_saved_unit) {
		//  Set refs to 1. This is the "I am alive ref", lost in ReleaseUnit.
		this->initialize_base_reference();
	}

	//  Build all unit table
	wyrmgus::unit_manager::get()->Add(this);

	//  Initialise unit structure (must be zero filled!)
	Type = &type;

	Seen.Frame = UnitNotSeen; // Unit isn't yet seen

	Frame = type.StillFrame;

	if (UnitTypeVar.GetNumberVariable()) {
		assert_throw(Variable.empty());
		this->Variable = type.DefaultStat.Variables;
	} else {
		this->Variable.clear();
	}

	IndividualUpgrades.clear();

	// Set a heading for the unit if it Handles Directions
	// Don't set a building heading, as only 1 construction direction
	//   is allowed.
	if (type.get_num_directions() > 1 && type.BoolFlag[NORANDOMPLACING_INDEX].value == false && type.Sprite && !type.BoolFlag[BUILDING_INDEX].value) {
		this->Direction = SyncRand(256); // random heading
		UnitUpdateHeading(*this);
	}

	this->autocast_spells = this->Type->get_autocast_spells();

	Active = 1;
	Removed = 1;
	
	// Has StartingResources, Use those
	//Wyrmgus start
//	this->ResourcesHeld = type.StartingResources;
	if (type.get_given_resource() != nullptr) {
		this->GivesResource = type.get_given_resource()->get_index();
		if (type.get_starting_resources().size() > 0) {
			this->ResourcesHeld = vector::get_random(type.get_starting_resources());
		} else {
			this->ResourcesHeld = 0;
		}
	}
	//Wyrmgus end

	assert_throw(Orders.empty());

	Orders.push_back(COrder::NewActionStill());

	assert_throw(NewOrder == nullptr);
	NewOrder = nullptr;
	assert_throw(SavedOrder == nullptr);
	SavedOrder = nullptr;
	assert_throw(CriticalOrder == nullptr);
	CriticalOrder = nullptr;
}

void CUnit::initialize_base_reference()
{
	this->base_ref = std::make_unique<wyrmgus::unit_ref>(this);
	this->ref = this->base_ref;
}

void CUnit::clear_base_reference()
{
	this->base_ref.reset();
}

/**
**  Restore the saved order
**
**  @return      True if the saved order was restored
*/
bool CUnit::RestoreOrder()
{
	if (this->SavedOrder == nullptr) {
		return false;
	}

	if (this->SavedOrder->IsValid() == false) {
		this->SavedOrder.reset();
		return false;
	}

	//cannot delete this->Orders[0] since it is generally that order which calls this method
	this->Orders[0]->Finished = true;

	//copy
	this->Orders.insert(this->Orders.begin() + 1, std::move(this->SavedOrder));

	this->SavedOrder = nullptr;
	return true;
}

/**
**  Check if we can store this order
**
**  @return      True if the order could be saved
*/
bool CUnit::CanStoreOrder(COrder *order)
{
	assert_throw(order != nullptr);

	if ((order && order->Finished == true) || order->IsValid() == false) {
		return false;
	}
	if (this->SavedOrder != nullptr) {
		return false;
	}
	return true;
}

void CUnit::clear_orders()
{
	this->Orders.clear();
	this->Orders.push_back(COrder::NewActionStill());
}

void CUnit::clear_special_orders()
{
	//reset the unit's specially-stored orders
	this->SavedOrder.reset();
	this->NewOrder.reset();
	this->CriticalOrder.reset();
}

/**
**  Assigns a unit to a player, adjusting buildings, food and totals
**
**  @param player  player which have the unit.
*/
void CUnit::AssignToPlayer(CPlayer &player)
{
	const wyrmgus::unit_type &type = *Type;

	this->Stats = &type.Stats[player.get_index()];

	if (!SaveGameLoading) {
		if (UnitTypeVar.GetNumberVariable()) {
			assert_throw(!this->Stats->Variables.empty());
			this->Variable = this->Stats->Variables;
		}
	}

	// Build player unit table
	//Wyrmgus start
//	if (!type.BoolFlag[VANISHES_INDEX].value && CurrentAction() != UnitAction::Die) {
	if (!type.BoolFlag[VANISHES_INDEX].value && CurrentAction() != UnitAction::Die && !this->Destroyed) {
	//Wyrmgus end
		player.AddUnit(*this);
		if (!SaveGameLoading) {
			// If unit is dying, it's already been lost by all players
			// don't count again
			if (type.BoolFlag[BUILDING_INDEX].value) {
				// FIXME: support more races
				//Wyrmgus start
//				if (!type.BoolFlag[WALL_INDEX].value && &type != UnitTypeOrcWall && &type != UnitTypeHumanWall) {
				//Wyrmgus end
					player.TotalBuildings++;
				//Wyrmgus start
//				}
				//Wyrmgus end
			} else {
				player.TotalUnits++;
				
				player.on_unit_built(this);
			}
		}
		
		player.IncreaseCountsForUnit(this);

		player.change_demand(type.Stats[player.get_index()].Variables[DEMAND_INDEX].Value); //food needed
	}

	// Don't Add the building if it's dying, used to load a save game
	//Wyrmgus start
//	if (type.BoolFlag[BUILDING_INDEX].value && CurrentAction() != UnitAction::Die) {
	if (type.BoolFlag[BUILDING_INDEX].value && CurrentAction() != UnitAction::Die && !this->Destroyed && !type.BoolFlag[VANISHES_INDEX].value) {
	//Wyrmgus end
		//Wyrmgus start
//		if (!type.BoolFlag[WALL_INDEX].value && &type != UnitTypeOrcWall && &type != UnitTypeHumanWall) {
		//Wyrmgus end
			player.NumBuildings++;
			//Wyrmgus start
			if (CurrentAction() == UnitAction::Built) {
				player.NumBuildingsUnderConstruction++;
				player.ChangeUnitTypeUnderConstructionCount(&type, 1);
			}
			//Wyrmgus end
		//Wyrmgus start
//		}
		//Wyrmgus end
	}

	this->Player = &player;
	
	//Wyrmgus start
	if (!SaveGameLoading) {
		//assign a gender to the unit
		if (this->get_gender() == gender::none && this->Type->BoolFlag[ORGANIC_INDEX].value && this->get_species() != nullptr && !this->get_species()->is_asexual()) {
			//Gender: 0 = Not Set, 1 = Male, 2 = Female
			this->Variable[GENDER_INDEX].Value = wyrmgus::random::get()->generate(2) + 1;
			this->Variable[GENDER_INDEX].Max = static_cast<int>(gender::count) - 1;
			this->Variable[GENDER_INDEX].Enable = 1;
		}
		
		//generate a personal name for the unit, if applicable
		if (this->get_character() == nullptr) {
			this->UpdatePersonalName();
		}
		
		this->UpdateSoldUnits();

		this->update_site_owner();
	}
	//Wyrmgus end
}

const wyrmgus::player_color *CUnit::get_player_color() const
{
	return this->get_display_player()->get_player_color();
}

const wyrmgus::species *CUnit::get_species() const
{
	if (this->Type->get_species() != nullptr) {
		return this->Type->get_species();
	}

	if (this->Type->BoolFlag[ORGANIC_INDEX].value) {
		const wyrmgus::civilization *civilization = this->get_civilization();
		if (civilization != nullptr) {
			return civilization->get_species();
		}
	}

	return nullptr;
}

const wyrmgus::civilization *CUnit::get_civilization() const
{
	const CPlayer *player = this->Player;

	if (this->get_player_from() != nullptr) {
		player = this->get_player_from();
	}

	return this->Type->get_player_civilization(player);
}

const civilization_base *CUnit::get_civilization_base() const
{
	const civilization_base *civilization = this->get_civilization();
	if (civilization != nullptr) {
		return civilization;
	}

	if (this->Type->get_civilization_group() != nullptr) {
		return this->Type->get_civilization_group();
	}

	return nullptr;
}

/**
**  Create a new unit.
**
**  @param type      Pointer to unit-type.
**  @param player    Pointer to owning player.
**
**  @return          Pointer to created unit.
*/
CUnit *MakeUnit(const wyrmgus::unit_type &type, CPlayer *player)
{
	if (CMap::get()->get_settings()->is_unit_type_disabled(&type)) {
		return nullptr;
	}

	if (!type.get_subtypes().empty()) {
		return MakeUnit(*vector::get_random(type.get_subtypes()), player);
	}

	CUnit *unit = unit_manager::get()->AllocUnit();
	if (unit == nullptr) {
		log::log_error("Unable to allocate unit.");
		return nullptr;
	}

	unit->Init(type, false);
	// Only Assign if a Player was specified
	if (player) {
		unit->AssignToPlayer(*player);

		//Wyrmgus start
		unit->ChooseVariation(nullptr, true);
		for (int i = 0; i < MaxImageLayers; ++i) {
			unit->ChooseVariation(nullptr, true, i);
		}
		unit->UpdateButtonIcons();
		unit->UpdateXPRequired();
		//Wyrmgus end
	}

	//Wyrmgus start
	//grant the unit the civilization/faction upgrades of its respective civilization/faction, so that it is able to pursue its upgrade line in experience upgrades even if it changes hands
	if (unit->Type->get_civilization() != nullptr) {
		const CUpgrade *civilization_upgrade = unit->Type->get_civilization()->get_upgrade();
		if (civilization_upgrade != nullptr) {
			unit->SetIndividualUpgrade(civilization_upgrade, 1);
		}
	}
	if (unit->Type->get_faction() != nullptr && unit->Type->get_faction()->get_upgrade() != nullptr) {
		const CUpgrade *faction_upgrade = unit->Type->get_faction()->get_upgrade();
		unit->SetIndividualUpgrade(faction_upgrade, 1);
	}

	//generate traits for the unit, if any are available (only if the editor isn't running)
	if (!CEditor::get()->is_running() && !unit->Type->get_traits().empty()) {
		unit->generate_traits();
	}
	
	for (const CUpgrade *starting_ability : unit->Type->StartingAbilities) {
		if (check_conditions(starting_ability, unit)) {
			unit->add_bonus_ability(starting_ability);
		}
	}
	
	//set the unit type's elixir, if any
	if (unit->Type->get_elixir() != nullptr) {
		unit->SetElixir(unit->Type->get_elixir());
	}
	
	unit->Variable[MANA_INDEX].Value = 0; //start off with 0 mana
	//Wyrmgus end
	
	if (unit->Type->OnInit) {
		unit->Type->OnInit->pushPreamble();
		unit->Type->OnInit->pushInteger(UnitNumber(*unit));
		unit->Type->OnInit->run();
	}
	
	return unit;
}

/**
**  (Un)Mark on vision table the Sight of the unit
**  (and units inside for transporter (recursively))
**
**  @param unit    Unit to (un)mark.
**  @param pos     coord of first container of unit.
**  @param width   Width of the first container of unit.
**  @param height  Height of the first container of unit.
**  @param f       Function to (un)mark for normal vision.
**  @param f2        Function to (un)mark for cloaking vision.
*/
template <map_marker_func_ptr sight_marker, map_marker_func_ptr detect_cloak_marker, map_marker_func_ptr ethereal_vision_marker>
static void MapMarkUnitSightRec(const CUnit &unit, const Vec2i &pos, int width, int height)
{
	static_assert(sight_marker != nullptr);
	static_assert(detect_cloak_marker != nullptr);
	static_assert(ethereal_vision_marker != nullptr);

	//Wyrmgus start
	/*
	MapSight<sight_marker>(*unit.Player, pos, width, height,
			 unit.GetFirstContainer()->CurrentSightRange);

	if (unit.Type && unit.Type->BoolFlag[DETECTCLOAK_INDEX].value && detect_cloak_marker) {
		MapSight<detect_cloak_marker>(*unit.Player, pos, width, height,
				 unit.GetFirstContainer()->CurrentSightRange);
	}
	*/

	MapSight<sight_marker>(*unit.Player, pos, width, height,
			 unit.Container && unit.Container->CurrentSightRange >= unit.CurrentSightRange ? unit.Container->CurrentSightRange : unit.CurrentSightRange, unit.MapLayer->ID);

	if (unit.Type && unit.Type->BoolFlag[DETECTCLOAK_INDEX].value) {
		MapSight<detect_cloak_marker>(*unit.Player, pos, width, height,
				 unit.Container && unit.Container->CurrentSightRange >= unit.CurrentSightRange ? unit.Container->CurrentSightRange : unit.CurrentSightRange, unit.MapLayer->ID);
	}
	
	if (unit.Variable[ETHEREALVISION_INDEX].Value) {
		MapSight<ethereal_vision_marker>(*unit.Player, pos, width, height,
				 unit.Container && unit.Container->CurrentSightRange >= unit.CurrentSightRange ? unit.Container->CurrentSightRange : unit.CurrentSightRange, unit.MapLayer->ID);
	}
	//Wyrmgus end

	for (const CUnit *unit_inside : unit.get_units_inside()) {
		//Wyrmgus start
//		MapMarkUnitSightRec(*unit_inside, pos, width, height, f, f2);
		MapMarkUnitSightRec<sight_marker, detect_cloak_marker, ethereal_vision_marker>(*unit_inside, pos, width, height);
		//Wyrmgus end
	}
}

/**
**	@brief	Return the topmost container for the unit
**
**	@param	unit	The unit for which to get the topmost container
**
**	@return	The unit's topmost container if present, or the unit itself otherwise; this function should never return null
*/
CUnit *CUnit::GetFirstContainer() const
{
	const CUnit *container = this;

	while (container->Container) {
		container = container->Container;
	}
	
	return const_cast<CUnit *>(container);
}

/**
**  Mark on vision table the Sight of the unit
**  (and units inside for transporter)
**
**  @param unit  unit to unmark its vision.
**  @see MapUnmarkUnitSight.
*/
void MapMarkUnitSight(CUnit &unit)
{
	CUnit *container = unit.GetFirstContainer(); //first container of the unit.
	assert_throw(container->Type != nullptr);

	MapMarkUnitSightRec<MapMarkTileSight, MapMarkTileDetectCloak, MapMarkTileDetectEthereal>(unit, container->tilePos, container->Type->get_tile_width(), container->Type->get_tile_height());

	// Never mark radar, except if the top unit, and unit is usable
	if (&unit == container && !unit.IsUnusable()) {
		if (unit.Stats->Variables[RADAR_INDEX].Value) {
			MapMarkRadar(*unit.Player, unit.tilePos, unit.Type->get_tile_width(),
						 unit.Type->get_tile_height(), unit.Stats->Variables[RADAR_INDEX].Value, unit.MapLayer->ID);
		}
		if (unit.Stats->Variables[RADARJAMMER_INDEX].Value) {
			MapMarkRadarJammer(*unit.Player, unit.tilePos, unit.Type->get_tile_width(),
							   unit.Type->get_tile_height(), unit.Stats->Variables[RADARJAMMER_INDEX].Value, unit.MapLayer->ID);
		}
	}
}

/**
**  Unmark on vision table the Sight of the unit
**  (and units inside for transporter)
**
**  @param unit    unit to unmark its vision.
**  @see MapMarkUnitSight.
*/
void MapUnmarkUnitSight(CUnit &unit)
{
	assert_throw(unit.Type != nullptr);

	CUnit *container = unit.GetFirstContainer();
	assert_throw(container->Type != nullptr);
	MapMarkUnitSightRec<MapUnmarkTileSight, MapUnmarkTileDetectCloak, MapUnmarkTileDetectEthereal>(unit, container->tilePos, container->Type->get_tile_width(), container->Type->get_tile_height());

	// Never mark radar, except if the top unit?
	if (&unit == container && !unit.IsUnusable()) {
		if (unit.Stats->Variables[RADAR_INDEX].Value) {
			MapUnmarkRadar(*unit.Player, unit.tilePos, unit.Type->get_tile_width(), unit.Type->get_tile_height(), unit.Stats->Variables[RADAR_INDEX].Value, unit.MapLayer->ID);
		}
		if (unit.Stats->Variables[RADARJAMMER_INDEX].Value) {
			MapUnmarkRadarJammer(*unit.Player, unit.tilePos, unit.Type->get_tile_width(), unit.Type->get_tile_height(), unit.Stats->Variables[RADARJAMMER_INDEX].Value, unit.MapLayer->ID);
		}
	}
}

/**
**  Update the Unit Current sight range to good value and transported units inside.
**
**  @param unit  unit to update SightRange
**
**  @internal before using it, MapUnmarkUnitSight(unit)
**  and after MapMarkUnitSight(unit)
**  are often necessary.
**
**  FIXME @todo manage differently unit inside with option.
**  (no vision, min, host value, own value, bonus value, ...)
*/
void UpdateUnitSightRange(CUnit &unit)
{
//Wyrmgus start
/*
#if 0 // which is the better ? caller check ?
	if (SaveGameLoading) {
		return;
	}
#else
	assert_throw(!SaveGameLoading);
#endif
*/
//Wyrmgus end

	int unit_sight_range = unit.Variable[SIGHTRANGE_INDEX].Max;
	const wyrmgus::time_of_day *time_of_day = unit.get_center_tile_time_of_day();
	if (time_of_day != nullptr) {
		if (time_of_day->is_day()) {
			unit_sight_range += unit.Variable[DAYSIGHTRANGEBONUS_INDEX].Value;
		} else if (time_of_day->is_night()) {
			unit_sight_range += unit.Variable[NIGHTSIGHTRANGEBONUS_INDEX].Value;
		}
	}
	unit_sight_range = std::max<int>(1, unit_sight_range);
	if (unit.UnderConstruction) { // Units under construction have no sight range.
		unit.CurrentSightRange = 1;
	} else if (!unit.Container) { // proper value.
		unit.CurrentSightRange = unit_sight_range;
	} else { // value of it container.
		//if a unit is inside a container, then use the sight of the unit or the container, whichever is greater
		if (unit_sight_range <= unit.Container->CurrentSightRange) {
			unit.CurrentSightRange = unit.Container->CurrentSightRange;
		} else {
			unit.CurrentSightRange = unit_sight_range;
		}
	}

	for (CUnit *unit_inside : unit.get_units_inside()) {
		UpdateUnitSightRange(*unit_inside);
	}
}

/**
**  Mark the field with the FieldFlags.
**
**  @param unit  unit to mark.
*/
void MarkUnitFieldFlags(const CUnit &unit)
{
	const tile_flag flags = unit.Type->FieldFlags;
	int h = unit.Type->get_tile_height();          // Tile height of the unit.
	const int width = unit.Type->get_tile_width(); // Tile width of the unit.
	unsigned int index = unit.Offset;

	//Wyrmgus start
//	if (unit.Type->BoolFlag[VANISHES_INDEX].value) {
	if (unit.Type->BoolFlag[VANISHES_INDEX].value || unit.CurrentAction() == UnitAction::Die) {
	//Wyrmgus end
		return;
	}

	const bool affects_sight = (unit.Type->FieldFlags & tile_flag::air_impassable) != tile_flag::none;

	std::unique_ptr<nearby_sight_unmarker> sight_unmarker;

	if (affects_sight) {
		sight_unmarker = std::make_unique<nearby_sight_unmarker>(&unit);
	}

	do {
		wyrmgus::tile *mf = unit.MapLayer->Field(index);
		int w = width;
		do {
			mf->Flags |= flags;
			++mf;
		} while (--w);
		index += unit.MapLayer->get_width();
	} while (--h);
}

class _UnmarkUnitFieldFlags final
{
public:
	explicit _UnmarkUnitFieldFlags(const CUnit &unit, wyrmgus::tile *mf) : main(&unit), mf(mf)
	{
	}

	void operator()(CUnit *const unit) const
	{
		if (main != unit && unit->CurrentAction() != UnitAction::Die) {
			mf->Flags |= unit->Type->FieldFlags;
		}
	}
private:
	const CUnit *const main;
	wyrmgus::tile *mf;
};


/**
**  Mark the field with the FieldFlags.
**
**  @param unit  unit to mark.
*/
void UnmarkUnitFieldFlags(const CUnit &unit)
{
	const tile_flag flags = ~unit.Type->FieldFlags;
	const int width = unit.Type->get_tile_width();
	int h = unit.Type->get_tile_height();
	unsigned int index = unit.Offset;

	if (unit.Type->BoolFlag[VANISHES_INDEX].value) {
		return;
	}

	const bool affects_sight = (unit.Type->FieldFlags & tile_flag::air_impassable) != tile_flag::none;

	std::unique_ptr<nearby_sight_unmarker> sight_unmarker;

	if (affects_sight) {
		sight_unmarker = std::make_unique<nearby_sight_unmarker>(&unit);
	}

	do {
		wyrmgus::tile *mf = unit.MapLayer->Field(index);

		int w = width;
		do {
			mf->Flags &= flags;//clean flags
			_UnmarkUnitFieldFlags funct(unit, mf);

			mf->UnitCache.for_each(funct);
			++mf;
		} while (--w);
		index += unit.MapLayer->get_width();
	} while (--h);
}

/**
**  Add unit to a container. It only updates linked list stuff.
**
**  @param host  Pointer to container.
*/
void CUnit::AddInContainer(CUnit &host)
{
	assert_throw(this->Container == nullptr);
	this->Container = &host;
	host.add_unit_inside(this);

	//Wyrmgus start
	if (!SaveGameLoading) {
		//if host has no range by itself, but the unit has range, and the unit can attack from a transporter, change the host's range to the unit's; but don't do this while loading, as it causes a crash (since one unit needs to be loaded before the other, and when this function is processed both won't already have their variables set)
		host.update_for_transported_units();
	}
	//Wyrmgus end
}

/**
**  Remove unit from a container. It only updates linked list stuff.
**
**  @param unit  Pointer to unit.
*/
static void RemoveUnitFromContainer(CUnit &unit)
{
	CUnit *host = unit.Container; //transporter which containd the unit
	assert_throw(unit.Container != nullptr);
	assert_throw(unit.Container->has_units_inside());
	
	host->remove_unit_inside(&unit);
	unit.Container = nullptr;
	//Wyrmgus start
	//reset host attack range
	host->update_for_transported_units();
	//Wyrmgus end
}

void CUnit::set_garrisoned_gathering_income(const int income)
{
	const int old_income = this->get_garrisoned_gathering_income();

	if (income == old_income) {
		return;
	}

	assert_throw(this->get_given_resource() != nullptr);

	this->garrisoned_gathering_income = income;
	this->Player->change_income(this->get_given_resource(), income - old_income);
}

void CUnit::update_garrisoned_gathering_income()
{
	if (this->Variable[GARRISONED_GATHERING_INDEX].Value <= 0) {
		return;
	}

	const resource *resource = this->get_given_resource();

	if (resource == nullptr) {
		return;
	}

	int income = 0;

	if (this->BoardCount > 0) {
		for (const CUnit *garrisoned_unit : this->get_units_inside()) {
			const int unit_income = garrisoned_unit->get_garrisoned_gathering_rate(resource) * this->Player->get_income_modifier(resource) / 100;
			income += unit_income;
		}
	}

	this->set_garrisoned_gathering_income(income);
}

//Wyrmgus start
void CUnit::UpdateContainerAttackRange()
{
	this->best_contained_unit_attack_range = 0;

	//recalculate attack range, if this unit is a transporter (or garrisonable building) from which units can attack
	if (this->Type->CanTransport() && this->Type->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value) {
		if (this->BoardCount > 0) {
			for (const CUnit *boarded_unit : this->get_units_inside()) {
				if (boarded_unit->GetModifiedVariable(ATTACKRANGE_INDEX) > this->best_contained_unit_attack_range && boarded_unit->Type->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value) { //if container has no range by itself, but the unit has range, and the unit can attack from a transporter, change the container's range to the unit's
					this->best_contained_unit_attack_range = boarded_unit->GetModifiedVariable(ATTACKRANGE_INDEX);
				}
			}
		}
	}
}

void CUnit::UpdateXPRequired()
{
	if (!this->Type->can_gain_experience()) {
		return;
	}
	
	this->Variable[XPREQUIRED_INDEX].Value = this->Type->Stats[this->Player->get_index()].Variables[POINTS_INDEX].Value * 4 * this->Type->Stats[this->Player->get_index()].Variables[LEVEL_INDEX].Value;
	int extra_levels = this->Variable[LEVEL_INDEX].Value - this->Type->Stats[this->Player->get_index()].Variables[LEVEL_INDEX].Value;
	for (int i = 1; i <= extra_levels; ++i) {
		this->Variable[XPREQUIRED_INDEX].Value += 50 * 4 * i;
	}
	this->Variable[XPREQUIRED_INDEX].Max = this->Variable[XPREQUIRED_INDEX].Value;
	this->Variable[XPREQUIRED_INDEX].Enable = 1;
	this->Variable[XP_INDEX].Enable = 1;
}

void CUnit::UpdatePersonalName(bool update_settlement_name)
{
	static constexpr bool surname_generation_enabled = false;

	if (this->get_character() != nullptr) {
		return;
	} else if (this->Type->BoolFlag[ITEM_INDEX].value || this->get_unique() != nullptr || this->Prefix || this->Suffix) {
		this->UpdateItemName();
		return;
	}
	
	const civilization *civilization = this->get_civilization();

	const faction *faction = this->Player->get_faction();
	if (this->get_player_from() != nullptr) {
		faction = this->get_player_from()->get_faction();
	}
	
	const language *language = civilization ? civilization->get_language() : nullptr;

	const bool first_name_assignment = this->Name.empty();
	
	if (!this->Type->is_personal_name_valid(this->Name, faction, this->get_gender())) {
		//first see if can translate the current personal name
		std::string new_personal_name = PlayerRaces.TranslateName(this->Name, language);
		if (!new_personal_name.empty()) {
			this->Name = new_personal_name;
		} else {
			this->Name = this->Type->generate_personal_name(faction, this->get_gender());
		}
	}

	if constexpr (surname_generation_enabled) {
		if (civilization != nullptr && this->Type->BoolFlag[ORGANIC_INDEX].value) {
			const name_generator *surname_generator = civilization->get_surname_generator(this->get_gender());

			if (surname_generator != nullptr && (this->get_surname().empty() || !surname_generator->is_name_valid(this->get_surname()))) {
				this->surname = surname_generator->generate_name();
			}
		}
	}

	if (first_name_assignment || this->get_epithet() != nullptr) {
		//only update the epithet if this is the first name assignment, or if the unit already has an epithet which we should check to see if it is still valid
		this->update_epithet();
	}
	
	if (update_settlement_name && (this->Type->BoolFlag[TOWNHALL_INDEX].value || (this->Type->BoolFlag[BUILDING_INDEX].value && !this->settlement))) {
		this->UpdateSettlement();
	}
}

void CUnit::update_epithet()
{
	if (this->get_character() != nullptr || !this->Type->BoolFlag[ORGANIC_INDEX].value || this->Type->BoolFlag[FAUNA_INDEX].value) {
		return;
	}

	if (this->get_traits().empty()) {
		//don't assign an epithet before the unit has a trait
		return;
	}

	const read_only_context ctx = read_only_context::from_scope(this);
	
	if (this->get_epithet() != nullptr) {
		if (this->get_epithet()->get_conditions() != nullptr && !this->get_epithet()->get_conditions()->check(this, ctx)) {
			this->epithet = nullptr;
		} else {
			//already has a valid epithet
			return;
		}
	}

	if (SyncRand(4) != 0) {
		//75% chance that the unit will receive no epithet at all
		return;
	}

	std::vector<const wyrmgus::epithet *> potential_epithets;

	for (const wyrmgus::epithet *epithet : epithet::get_all()) {
		if (epithet->get_conditions() != nullptr && !epithet->get_conditions()->check(this, ctx)) {
			continue;
		}

		for (int i = 0; i < epithet->get_weight(); ++i) {
			potential_epithets.push_back(epithet);
		}
	}
	
	if (!potential_epithets.empty()) {
		this->epithet = vector::get_random(potential_epithets);
	}
}

void CUnit::UpdateSettlement()
{
	if (this->Removed || CEditor::get()->is_running()) {
		return;
	}
	
	if (!this->Type->BoolFlag[BUILDING_INDEX].value || this->Type->get_terrain_type()) {
		return;
	}
	
	if (this->Type->BoolFlag[TOWNHALL_INDEX].value || this->Type == settlement_site_unit_type) {
		if (this->settlement == nullptr && GameEstablishing) {
			const wyrmgus::civilization *civilization = this->get_civilization();
			
			const wyrmgus::faction *faction = this->Type->get_faction();
			if (this->Player->get_faction() != nullptr && this->Player->get_civilization() == civilization && this->Type == this->Player->get_faction()->get_class_unit_type(this->Type->get_unit_class())) {
				faction = this->Player->get_faction();
			}

			std::vector<const wyrmgus::site *> potential_settlements;
			if (civilization != nullptr) {
				for (const wyrmgus::site *site : civilization->sites) {
					if (!site->can_be_randomly_generated_settlement()) {
						continue;
					}

					if (site->get_game_data()->get_site_unit() == nullptr) {
						potential_settlements.push_back(site);
					}
				}
			}
			
			if (potential_settlements.empty() && faction) {
				for (const wyrmgus::site *site : faction->sites) {
					if (!site->can_be_randomly_generated_settlement()) {
						continue;
					}

					if (site->get_game_data()->get_site_unit() == nullptr) {
						potential_settlements.push_back(site);
					}
				}
			}
			
			if (potential_settlements.empty()) {
				for (const wyrmgus::site *site : wyrmgus::site::get_all()) {
					if (!site->can_be_randomly_generated_settlement()) {
						continue;
					}

					if (site->get_game_data()->get_site_unit() == nullptr) {
						potential_settlements.push_back(site);
					}
				}
			}
			
			if (potential_settlements.size() > 0) {
				this->set_settlement(vector::take_random(potential_settlements));
				this->set_site(this->get_settlement());
				CMap::get()->add_settlement_unit(this);

				//set the tiles the settlement unit is located to its settlement
				for (int x = this->tilePos.x; x < (this->tilePos.x + this->Type->get_tile_width()); ++x) {
					for (int y = this->tilePos.y; y < (this->tilePos.y + this->Type->get_tile_height()); ++y) {
						const QPoint tile_pos(x, y);
						this->MapLayer->Field(tile_pos)->set_settlement(this->get_settlement());
					}
				}
			}
		}

		if (this->get_settlement() != nullptr) {
			this->UpdateBuildingSettlementAssignment();
		}
	} else {
		const tile *tile = this->get_center_tile();

		if (this->Player->get_index() == PlayerNumNeutral || this->Player->has_neutral_faction_type()) {
			this->set_settlement(tile->get_settlement());
		} else if (tile->get_owner() == this->Player) {
			this->set_settlement(tile->get_settlement());
		} else {
			this->set_settlement(this->Player->get_nearest_settlement(this->tilePos, this->MapLayer->ID, this->Type->get_tile_size()));
		}
	}
}

void CUnit::UpdateBuildingSettlementAssignment(const wyrmgus::site *old_settlement)
{
	if (CEditor::get()->is_running()) {
		return;
	}
	
	for (const qunique_ptr<CPlayer> &player : CPlayer::Players) {
		if (player->get_index() != PlayerNumNeutral && !player->has_neutral_faction_type() && this->Player != player.get()) {
			continue;
		}

		player->update_building_settlement_assignment(old_settlement, this->MapLayer->ID);
	}
}

void CUnit::on_variable_changed(const int var_index, const int change)
{
	if (change == 0) {
		return;
	}

	switch (var_index) {
		case ATTACKRANGE_INDEX:
			if (this->Container != nullptr && !SaveGameLoading) {
				this->Container->UpdateContainerAttackRange();
			}
			break;
		case LEVEL_INDEX:
			this->UpdateXPRequired();
			break;
		case POINTS_INDEX:
			this->UpdateXPRequired();
			this->Player->change_military_score(change);
			break;
		case XP_INDEX:
			this->XPChanged();
			break;
		case LEVELUP_INDEX:
			this->Player->UpdateLevelUpUnits();
			break;
		case KNOWLEDGEMAGIC_INDEX:
		case KNOWLEDGEWARFARE_INDEX:
		case KNOWLEDGEMINING_INDEX:
			this->CheckKnowledgeChange(var_index, change);
			break;
		case GATHERINGBONUS_INDEX:
		case COPPERGATHERINGBONUS_INDEX:
		case SILVERGATHERINGBONUS_INDEX:
		case GOLDGATHERINGBONUS_INDEX:
		case IRONGATHERINGBONUS_INDEX:
		case MITHRILGATHERINGBONUS_INDEX:
		case LUMBERGATHERINGBONUS_INDEX:
		case STONEGATHERINGBONUS_INDEX:
		case COALGATHERINGBONUS_INDEX:
		case JEWELRYGATHERINGBONUS_INDEX:
		case FURNITUREGATHERINGBONUS_INDEX:
		case LEATHERGATHERINGBONUS_INDEX:
		case GEMSGATHERINGBONUS_INDEX:
			if (this->Container != nullptr && !SaveGameLoading) {
				this->Container->update_garrisoned_gathering_income();
			}
			break;
		default:
			break;
	}
}

void CUnit::XPChanged()
{
	if (!this->Type->can_gain_experience()) {
		return;
	}
	
	if (this->Variable[XPREQUIRED_INDEX].Value == 0) {
		return;
	}
	
	while (this->Variable[XP_INDEX].Value >= this->Variable[XPREQUIRED_INDEX].Value) {
		this->Variable[XP_INDEX].Max -= this->Variable[XPREQUIRED_INDEX].Max;
		this->Variable[XP_INDEX].Value -= this->Variable[XPREQUIRED_INDEX].Value;
		if (this->Player == CPlayer::GetThisPlayer()) {
			this->Player->Notify(notification_type::green, this->tilePos, this->MapLayer->ID, _("%s has leveled up!"), GetMessageName().c_str());
		}
		this->IncreaseLevel(1);
	}
	
	if (game::get()->is_persistency_enabled() && this->get_character() != nullptr && this->Player == CPlayer::GetThisPlayer()) {
		if (this->Variable[LEVEL_INDEX].Value > this->get_character()->get_level()) { //save level, if the unit has a persistent character
			this->get_character()->set_level(this->Variable[LEVEL_INDEX].Value);
		}
		this->get_character()->ExperiencePercent = (this->Variable[XP_INDEX].Value * 100) / this->Variable[XPREQUIRED_INDEX].Value;
		this->get_character()->save();
		achievement::check_achievements(); // check achievements to see if any hero now has a high enough level for a particular achievement to be obtained
	}
}
//Wyrmgus end

/**
**  Affect Tile coord of a unit (with units inside) to tile (x, y).
**
**  @param unit  unit to move.
**  @param pos   map tile position.
**
**  @internal before use it, Map.Remove(unit), MapUnmarkUnitSight(unit)
**  and after Map.Insert(unit), MapMarkUnitSight(unit)
**  are often necessary. Check Flag also for Pathfinder.
*/
static void UnitInXY(CUnit &unit, const Vec2i &pos, const int z)
{
	const time_of_day *old_time_of_day = unit.get_center_tile_time_of_day();
	
	unit.tilePos = pos;
	unit.Offset = CMap::get()->get_pos_index(pos, z);
	unit.MapLayer = CMap::get()->MapLayers[z].get();

	const time_of_day *new_time_of_day = unit.get_center_tile_time_of_day();

	//Wyrmgus start
	if (!SaveGameLoading && old_time_of_day != new_time_of_day) {
		UpdateUnitSightRange(unit);
	}
	//Wyrmgus end

	for (CUnit *unit_inside : unit.get_units_inside()) {
		UnitInXY(*unit_inside, pos, z);
	}
}

/**
**  Move a unit (with units inside) to tile (pos).
**  (Do stuff with vision, cachelist and pathfinding).
**
**  @param pos  map tile position.
**
*/
//Wyrmgus start
//void CUnit::MoveToXY(const Vec2i &pos)
void CUnit::MoveToXY(const Vec2i &pos, const int z)
//Wyrmgus end
{
	MapUnmarkUnitSight(*this);
	CMap::get()->Remove(*this);
	UnmarkUnitFieldFlags(*this);

	//Wyrmgus start
//	assert_throw(UnitCanBeAt(*this, pos));
	assert_throw(UnitCanBeAt(*this, pos, z));
	//Wyrmgus end
	// Move the unit.
	//Wyrmgus start
//	UnitInXY(*this, pos);
	UnitInXY(*this, pos, z);
	//Wyrmgus end

	CMap::get()->Insert(*this);
	MarkUnitFieldFlags(*this);
	//  Recalculate the seen count.
	UnitCountSeen(*this);
	MapMarkUnitSight(*this);

	if (game::get()->is_running()) {
		emit this->MapLayer->unit_tile_pos_changed(UnitNumber(*this), this->tilePos);
	}

	//Wyrmgus start
	// if there is a trap in the new tile, trigger it
	if ((this->Type->get_domain() != unit_domain::air && this->Type->get_domain() != unit_domain::air_low && this->Type->get_domain() != unit_domain::space) || !this->Type->BoolFlag[ORGANIC_INDEX].value) {
		const CUnitCache &cache = CMap::get()->Field(pos, z)->UnitCache;
		for (size_t i = 0; i != cache.size(); ++i) {
			if (!cache[i]) {
				fprintf(stderr, "Error in CUnit::MoveToXY (pos %d, %d): a unit in the tile's unit cache is null.\n", pos.x, pos.y);
			}
			CUnit &unit = *cache[i];
			if (unit.IsAliveOnMap() && unit.Type->BoolFlag[TRAP_INDEX].value) {
				FireMissile(unit, this, this->tilePos, this->MapLayer->ID);
				LetUnitDie(unit);
			}
		}
	}
	//Wyrmgus end
}

/**
**  Place unit on map.
**
**  @param pos  map tile position.
*/
void CUnit::Place(const Vec2i &pos, const int z)
{
	assert_throw(Removed);
	
	const CMapLayer *old_map_layer = this->MapLayer;

	if (Container) {
		MapUnmarkUnitSight(*this);
		RemoveUnitFromContainer(*this);
	}
	if (!SaveGameLoading) {
		UpdateUnitSightRange(*this);
	}
	Removed = 0;
	UnitInXY(*this, pos, z);
	// Pathfinding info.
	MarkUnitFieldFlags(*this);
	// Tha cache list.
	CMap::get()->Insert(*this);
	//  Calculate the seen count.
	UnitCountSeen(*this);
	// Vision
	MapMarkUnitSight(*this);

	//Wyrmgus start
	if (this->IsAlive()) {
		if (this->Type->BoolFlag[BUILDING_INDEX].value) {
			this->UpdateSettlement(); //update the settlement of a building when placing it
		}

		//remove pathways, destroyed walls and decoration units under buildings
		if (this->Type->BoolFlag[BUILDING_INDEX].value && !this->Type->get_terrain_type()) {
			for (int x = this->tilePos.x; x < this->tilePos.x + this->Type->get_tile_width(); ++x) {
				for (int y = this->tilePos.y; y < this->tilePos.y + this->Type->get_tile_height(); ++y) {
					if (!CMap::get()->Info->IsPointOnMap(x, y, this->MapLayer)) {
						continue;
					}
					const QPoint building_tile_pos(x, y);
					wyrmgus::tile &mf = *this->MapLayer->Field(building_tile_pos);
					if (mf.get_overlay_terrain() != nullptr && mf.get_overlay_terrain()->is_constructed()) {
						CMap::get()->RemoveTileOverlayTerrain(building_tile_pos, this->MapLayer->ID);
					}
					//remove decorations if a building has been built here
					std::vector<CUnit *> table;
					Select(building_tile_pos, building_tile_pos, table, this->MapLayer->ID);
					for (size_t i = 0; i != table.size(); ++i) {
						if (table[i] && table[i]->IsAlive() && table[i]->Type->get_domain() == unit_domain::land && table[i]->Type->BoolFlag[DECORATION_INDEX].value) {
							if (!CEditor::get()->is_running()) {
								LetUnitDie(*table[i]);			
							} else {
								EditorActionRemoveUnit(*table[i], false);
							}
						}
					}
				}
			}
		}
		
		const wyrmgus::unit_type_variation *variation = this->GetVariation();
		if (variation != nullptr) {
			// if a unit that is on the tile has a terrain-dependent or season-dependent variation that is not compatible with the new tile, or if this is the first position the unit is being placed in, repick the unit's variation
			if (old_map_layer == nullptr || !this->can_have_variation(variation)) {
				this->ChooseVariation(nullptr, false, -1, false);
			}
		}

		if (this->Type->can_produce_a_resource()) {
			const tile *tile = this->get_center_tile();
			if (tile->get_settlement() != nullptr) {
				tile->get_settlement()->get_game_data()->add_resource_unit(this);
			}
		}

		if (this->Type->BoolFlag[BUILDING_INDEX].value) {
			const tile *tile = this->get_center_tile();
			if (tile->get_settlement() != nullptr) {
				tile->get_settlement()->get_game_data()->add_building(this);
			}
		}
	}
	//Wyrmgus end

	if (game::get()->is_running()) {
		emit this->MapLayer->unit_added(UnitNumber(*this), this->Type, this->GetVariation(), this->Frame, this->get_player_color(), this->tilePos);
	}
}

/**
**  Place a unit on the map nearest to goalPos.
**
**  @param unit  Unit to drop out.
**  @param goalPos Goal map tile position.
**  @param addx  Tile width of unit it's dropping out of.
**  @param addy  Tile height of unit it's dropping out of.
*/
void CUnit::drop_out_nearest(const Vec2i &goal_pos, const CUnit *container)
{
	Vec2i pos;
	Vec2i bestPos;
	int bestd = 99999;
	int addx = 0;
	int addy = 0;
	//Wyrmgus start
	int z;
	//Wyrmgus end

	if (container) {
		assert_throw(this->Removed);
		pos = container->tilePos;
		pos -= Vec2i(this->Type->get_tile_size() - QSize(1, 1));
		addx = container->Type->get_tile_width() + this->Type->get_tile_width() - 1;
		addy = container->Type->get_tile_height() + this->Type->get_tile_height() - 1;
		--pos.x;
		z = container->MapLayer->ID;
	} else {
		pos = this->tilePos;
		z = this->MapLayer->ID;
	}
	// FIXME: if we reach the map borders we can go fast up, left, ...

	for (;;) {
		for (int i = addy; i--; ++pos.y) { // go down
			//Wyrmgus start
//			if (UnitCanBeAt(*this, pos)) {
			if (UnitCanBeAt(*this, pos, z)) {
				//Wyrmgus end
				const int n = point::square_distance_to(goal_pos, pos);

				if (n < bestd) {
					bestd = n;
					bestPos = pos;
				}
			}
		}
		++addx;
		for (int i = addx; i--; ++pos.x) { // go right
			//Wyrmgus start
//			if (UnitCanBeAt(*this, pos)) {
			if (UnitCanBeAt(*this, pos, z)) {
				//Wyrmgus end
				const int n = point::square_distance_to(goal_pos, pos);

				if (n < bestd) {
					bestd = n;
					bestPos = pos;
				}
			}
		}
		++addy;
		for (int i = addy; i--; --pos.y) { // go up
			//Wyrmgus start
//			if (UnitCanBeAt(*this, pos)) {
			if (UnitCanBeAt(*this, pos, z)) {
				//Wyrmgus end
				const int n = point::square_distance_to(goal_pos, pos);

				if (n < bestd) {
					bestd = n;
					bestPos = pos;
				}
			}
		}
		++addx;
		for (int i = addx; i--; --pos.x) { // go left
			//Wyrmgus start
//			if (UnitCanBeAt(*this, pos)) {
			if (UnitCanBeAt(*this, pos, z)) {
				//Wyrmgus end
				const int n = point::square_distance_to(goal_pos, pos);

				if (n < bestd) {
					bestd = n;
					bestPos = pos;
				}
			}
		}
		if (bestd != 99999) {
			//Wyrmgus start
//			this->Place(bestPos);
			this->Place(bestPos, z);
			//Wyrmgus end
			return;
		}
		++addy;
	}
}

template <typename function_type>
void CUnit::drop_out_on_side_base(const int heading, const CUnit *container, const function_type &pos_check_function)
{
	Vec2i pos;
	int addx = 0;
	int addy = 0;
	int z = 0;

	if (container != nullptr) {
		pos = container->tilePos;
		pos -= Vec2i(this->Type->get_tile_size() - QSize(1, 1));
		addx = container->Type->get_tile_width() + this->Type->get_tile_width() - 1;
		addy = container->Type->get_tile_height() + this->Type->get_tile_height() - 1;
		z = container->MapLayer->ID;

		if (heading < LookingNE || heading > LookingNW) {
			pos.x += addx - 1;
			--pos.y;
			goto startn;
		} else if (heading < LookingSE) {
			pos.x += addx;
			pos.y += addy - 1;
			goto starte;
		} else if (heading < LookingSW) {
			pos.y += addy;
			goto starts;
		} else {
			--pos.x;
			goto startw;
		}
	} else {
		pos = this->tilePos;
		z = this->MapLayer->ID;

		if (heading < LookingNE || heading > LookingNW) {
			goto starts;
		} else if (heading < LookingSE) {
			goto startw;
		} else if (heading < LookingSW) {
			goto startn;
		} else {
			goto starte;
		}
	}

	// FIXME: don't search outside of the map
	for (;;) {
	startw:
		for (int i = addy; i--; ++pos.y) {
			if (pos_check_function(this, pos, z)) {
				goto found;
			}
		}
		++addx;
	starts:
		for (int i = addx; i--; ++pos.x) {
			if (pos_check_function(this, pos, z)) {
				goto found;
			}
		}
		++addy;
	starte:
		for (int i = addy; i--; --pos.y) {
			if (pos_check_function(this, pos, z)) {
				goto found;
			}
		}
		++addx;
	startn:
		for (int i = addx; i--; --pos.x) {
			if (pos_check_function(this, pos, z)) {
				goto found;
			}
		}
		++addy;
	}

found:
	this->Place(pos, z);
}

/**
**  Place a unit on the map to the side of a unit.
**
**  @param unit       Unit to drop out.
**  @param heading    Direction in which the unit should appear.
**  @param container  Unit "containing" unit to drop (may be different of unit.Container).
*/
void CUnit::drop_out_on_side(const int heading, const CUnit *container)
{
	this->drop_out_on_side_base(heading, container, [](const CUnit *unit, const Vec2i &pos, const int z) {
		return UnitCanBeAt(*unit, pos, z);
	});
}

std::vector<QPoint> CUnit::get_tile_positions_in_distance_to(const QSize &target_size, const int min_distance, const int max_distance) const
{
	const QPoint max_distance_offset(max_distance, max_distance);

	const QPoint top_left(this->tilePos - max_distance_offset - size::to_point(target_size));
	const QPoint bottom_right(this->tilePos + size::to_point(this->Type->get_tile_size()) + max_distance_offset);

	const QRect rect(top_left, bottom_right);

	std::vector<QPoint> tile_positions;

	const int z = this->MapLayer->ID;

	rect::for_each_point(rect, [&](const QPoint &pos) {
		const int distance = MapDistance(target_size, pos, z, this->Type->get_tile_size(), this->tilePos, z);

		if (distance < min_distance) {
			return;
		}

		if (distance > max_distance) {
			return;
		}

		tile_positions.push_back(pos);
	});

	return tile_positions;
}

/**
**  Create new unit and place on map.
**
**  @param pos     map tile position.
**  @param type    Pointer to unit-type.
**  @param player  Pointer to owning player.
**
**  @return        Pointer to created unit.
*/
CUnit *MakeUnitAndPlace(const Vec2i &pos, const wyrmgus::unit_type &type, CPlayer *player, int z)
{
	CUnit *unit = MakeUnit(type, player);

	if (unit != nullptr) {
		unit->Place(pos, z);
	}

	return unit;
}

//Wyrmgus start
/**
**  Create a new unit and place it on the map, updating its player accordingly.
**
**  @param pos     map tile position.
**  @param type    Pointer to unit-type.
**  @param player  Pointer to owning player.
**
**  @return        Pointer to created unit.
*/
CUnit *CreateUnit(const Vec2i &pos, const unit_type &type, CPlayer *player, const int z, const bool no_building_bordering_impassable, const site *settlement, const bool ignore_ontop)
{
	CUnit *unit = MakeUnit(type, player);

	if (unit != nullptr) {
		unit->MapLayer = CMap::get()->MapLayers[z].get();

		const int heading = SyncRand(256);
		const QPoint res_pos = FindNearestDrop(type, pos, heading, z, no_building_bordering_impassable, ignore_ontop, settlement);
		
		if (type.BoolFlag[BUILDING_INDEX].value && !ignore_ontop) {
			const on_top_build_restriction *b = OnTopDetails(type, nullptr);
			if (b && b->ReplaceOnBuild) {
				CUnitCache &unitCache = CMap::get()->Field(res_pos, z)->UnitCache;
				CUnitCache::iterator it = std::find_if(unitCache.begin(), unitCache.end(), HasSameTypeAs(*b->Parent));

				if (it != unitCache.end()) {
					CUnit &replacedUnit = **it;
					unit->ReplaceOnTop(replacedUnit);
				}
			}
		}
		
		unit->Place(res_pos, z);
		UpdateForNewUnit(*unit, 0);
	}

	return unit;
}

CUnit *CreateResourceUnit(const Vec2i &pos, const unit_type &type, int z, bool allow_unique)
{
	CUnit *unit = CreateUnit(pos, type, CPlayer::get_neutral_player(), z, true);
	unit->generate_special_properties(nullptr, unit->Player, allow_unique, false, false);
			
	// create metal rocks near metal resources
	const unit_type *metal_rock_type = nullptr;
	if (type.get_identifier() == "unit_gold_deposit") {
		metal_rock_type = unit_type::get("unit_gold_rock");
	} else if (type.get_identifier() == "unit_silver_deposit") {
		metal_rock_type = unit_type::get("unit_silver_rock");
	} else if (type.get_identifier() == "unit_copper_deposit") {
		metal_rock_type = unit_type::get("unit_copper_rock");
	} else if (type.get_identifier() == "unit-diamond-deposit") {
		metal_rock_type = unit_type::get("unit_diamond_rock");
	} else if (type.get_identifier() == "unit-emerald-deposit") {
		metal_rock_type = unit_type::get("unit_emerald_rock");
	}

	if (metal_rock_type != nullptr) {
		for (int i = 0; i < 9; ++i) {
			CreateUnit(unit->get_center_tile_pos(), *metal_rock_type, CPlayer::get_neutral_player(), z);
		}
	}
			
	return unit;
}
//Wyrmgus end

/**
**  Find the nearest position at which unit can be placed.
**
**  @param type     Type of the dropped unit.
**  @param goalPos  Goal map tile position.
**  @param resPos   Holds the nearest point.
**  @param heading  preferense side to drop out of.
*/
QPoint FindNearestDrop(const unit_type &type, const QPoint &goal_pos, const int heading, const int z, const bool no_building_bordering_impassable, const bool ignore_ontop, const site *settlement)
{
	int addx = 0;
	int addy = 0;
	Vec2i pos = goal_pos;
	bool searched_any_tile_inside_map = true;

	if (heading < LookingNE || heading > LookingNW) {
		goto starts;
	} else if (heading < LookingSE) {
		goto startw;
	} else if (heading < LookingSW) {
		goto startn;
	} else {
		goto starte;
	}

	for (;;) {
		searched_any_tile_inside_map = false;
startw:
		for (int i = addy; i--; ++pos.y) {
			if (CMap::get()->Info->IsPointOnMap(pos, z)) {
				searched_any_tile_inside_map = true;
			} else {
				continue;
			}

			//Wyrmgus start
//			if (UnitTypeCanBeAt(type, pos)) {
			if (type.can_be_dropped_on_pos(pos, z, no_building_bordering_impassable, ignore_ontop, settlement)) {
			//Wyrmgus end
				return pos;
			}
		}
		++addx;
starts:
		for (int i = addx; i--; ++pos.x) {
			if (CMap::get()->Info->IsPointOnMap(pos, z)) {
				searched_any_tile_inside_map = true;
			} else {
				continue;
			}

			//Wyrmgus start
//			if (UnitTypeCanBeAt(type, pos)) {
			if (type.can_be_dropped_on_pos(pos, z, no_building_bordering_impassable, ignore_ontop, settlement)) {
			//Wyrmgus end
				return pos;
			}
		}
		++addy;
starte:
		for (int i = addy; i--; --pos.y) {
			if (CMap::get()->Info->IsPointOnMap(pos, z)) {
				searched_any_tile_inside_map = true;
			} else {
				continue;
			}

			//Wyrmgus start
//			if (UnitTypeCanBeAt(type, pos)) {
			if (type.can_be_dropped_on_pos(pos, z, no_building_bordering_impassable, ignore_ontop, settlement)) {
			//Wyrmgus end
				return pos;
			}
		}
		++addx;
startn:
		for (int i = addx; i--; --pos.x) {
			if (CMap::get()->Info->IsPointOnMap(pos, z)) {
				searched_any_tile_inside_map = true;
			} else {
				continue;
			}

			//Wyrmgus start
//			if (UnitTypeCanBeAt(type, pos)) {
			if (type.can_be_dropped_on_pos(pos, z, no_building_bordering_impassable, ignore_ontop, settlement)) {
			//Wyrmgus end
				return pos;
			}
		}
		++addy;

		if (!searched_any_tile_inside_map) {
			//we are already fully searching outside the map, so there's no hope of finding a valid position for the unit anymore
			throw std::runtime_error("Failed to find position for unit of type \"" + type.get_identifier() + "\" in FindNearestDrop().");
		}
	}
}

/**
**  Remove unit from map.
**
**  Update selection.
**  Update panels.
**  Update map.
**
**  @param host  Pointer to housing unit.
*/
void CUnit::Remove(CUnit *host)
{
	if (Removed) { // could happen!
		// If unit is removed (inside) and building is destroyed.
		DebugPrint("unit '%s(%d)' already removed\n" _C_ Type->get_identifier().c_str() _C_ UnitNumber(*this));
		return;
	}

	if (this->Type->can_produce_a_resource()) {
		const tile *tile = this->get_center_tile();
		if (tile->get_settlement() != nullptr) {
			tile->get_settlement()->get_game_data()->remove_resource_unit(this);
		}
	}

	if (this->Type->BoolFlag[BUILDING_INDEX].value) {
		const tile *tile = this->get_center_tile();
		if (tile->get_settlement() != nullptr) {
			tile->get_settlement()->get_game_data()->remove_building(this);
		}
	}

	CMap::get()->Remove(*this);
	MapUnmarkUnitSight(*this);
	UnmarkUnitFieldFlags(*this);
	if (host) {
		AddInContainer(*host);
		UpdateUnitSightRange(*this);
		UnitInXY(*this, host->tilePos, host->MapLayer->ID);
		MapMarkUnitSight(*this);
	}

	this->Removed = 1;

	//  Remove unit from the current selection
	if (this->Selected) {
		if (::Selected.size() == 1) { //  Remove building cursor
			CancelBuildingMode();
		}
		UnSelectUnit(*this);
		//Wyrmgus start
//		SelectionChanged();
		if (GameRunning) { // to avoid a crash when SelectionChanged() calls UI.ButtonPanel.Update()
			SelectionChanged();
		}
		//Wyrmgus end
	}
	// Remove unit from team selections
	if (!Selected && TeamSelected) {
		UnSelectUnit(*this);
	}

	// Unit is seen as under cursor
	if (UnitUnderCursor == this) {
		UnitUnderCursor = nullptr;
	}

	if (game::get()->is_running()) {
		emit this->MapLayer->unit_removed(UnitNumber(*this));
	}
}

/**
**  Update information for lost units.
**
**  @param unit  Pointer to unit.
**
**  @note Also called by ChangeUnitOwner
*/
void UnitLost(CUnit &unit)
{
	assert_throw(unit.Player != nullptr);  //the following code doesn't support null player!

	CPlayer &player = *unit.Player;

	//  Call back to AI, for killed or lost units.
	if (!CEditor::get()->is_running()) {
		if (player.AiEnabled) {
			AiUnitKilled(unit);
		} else {
			//  Remove unit from its groups
			if (unit.GroupId) {
				RemoveUnitFromGroups(unit);
			}
		}
	}

	//  Remove the unit from the player's units table.

	const wyrmgus::unit_type &type = *unit.Type;
	if (!type.BoolFlag[VANISHES_INDEX].value) {
		player.RemoveUnit(unit);

		if (type.BoolFlag[BUILDING_INDEX].value) {
			// FIXME: support more races
			//Wyrmgus start
//			if (!type.BoolFlag[WALL_INDEX].value && &type != UnitTypeOrcWall && &type != UnitTypeHumanWall) {
			//Wyrmgus end
				player.NumBuildings--;
				//Wyrmgus start
				if (unit.CurrentAction() == UnitAction::Built) {
					player.NumBuildingsUnderConstruction--;
					player.ChangeUnitTypeUnderConstructionCount(&type, -1);
				}
				//Wyrmgus end
			//Wyrmgus start
//			}
			//Wyrmgus end
		}
		if (unit.CurrentAction() != UnitAction::Built) {
			if (player.AiEnabled && player.Ai) {
				if (std::find(player.Ai->Scouts.begin(), player.Ai->Scouts.end(), &unit) != player.Ai->Scouts.end()) {
					if (player.Ai->Scouting) {
						//if an AI player's scout has been lost, unmark it as "scouting" so that the force can see if it now has a viable target
						player.Ai->Scouting = false;
					}
				}

				player.Ai->remove_transporter(&unit);
			}
			
			player.DecreaseCountsForUnit(&unit);

			if (unit.get_given_resource() != nullptr && unit.get_garrisoned_gathering_income() != 0) {
				player.change_income(unit.get_given_resource(), -unit.get_garrisoned_gathering_income());
			}
			
			if (unit.Variable[LEVELUP_INDEX].Value > 0) {
				player.UpdateLevelUpUnits(); //recalculate level-up units, since this unit no longer should be in that vector
			}
			//Wyrmgus end
		}
	}

	//  Handle unit demand. (Currently only food supported.)
	player.change_demand(-type.Stats[player.get_index()].Variables[DEMAND_INDEX].Value);

	//  Update information.
	if (unit.CurrentAction() != UnitAction::Built) {
		if (unit.Variable[SUPPLY_INDEX].Value != 0) {
			player.change_supply(-unit.Variable[SUPPLY_INDEX].Value);
		}

		if (defines::get()->is_population_enabled()) {
			if (unit.get_settlement() != nullptr) {
				unit.get_settlement()->get_game_data()->on_settlement_building_removed(&unit);
			}
		}

		// Decrease resource limit
		for (const resource *resource : resource::get_all()) {
			if (resource == defines::get()->get_time_resource()) {
				continue;
			}

			if (player.get_max_resource(resource) != -1 && type.Stats[player.get_index()].get_storing(resource) != 0) {
				const int new_max_value = player.get_max_resource(resource) - type.Stats[player.get_index()].get_storing(resource);

				player.set_max_resource(resource, std::max(0, new_max_value));
				player.set_resource(resource, player.get_stored_resource(resource), resource_storage_type::building);
			}
		}

		//  Handle income improvements, look if a player loses a building
		//  which have given him a better income, find the next best
		//  income.
		for (const resource *resource : resource::get_all()) {
			if (resource == defines::get()->get_time_resource()) {
				continue;
			}

			if (player.get_income_modifier(resource) != 0 && type.Stats[player.get_index()].get_improve_income(resource) == player.get_income_modifier(resource)) {
				int m = resource->get_default_income();

				for (int j = 0; j < player.GetUnitCount(); ++j) {
					const CUnit &player_unit = player.GetUnit(j);
					if (!player_unit.IsAlive()) {
						continue;
					}

					m = std::max(m, player_unit.Type->Stats[player.get_index()].get_improve_income(resource));
				}

				player.set_income_modifier(resource, m);
			}
		}

		if (type.Stats[player.get_index()].Variables[TRADECOST_INDEX].Enable) {
			int m = DefaultTradeCost;

			for (int j = 0; j < player.GetUnitCount(); ++j) {
				if (player.GetUnit(j).Type->Stats[player.get_index()].Variables[TRADECOST_INDEX].Enable) {
					m = std::min(m, player.GetUnit(j).Type->Stats[player.get_index()].Variables[TRADECOST_INDEX].Value);
				}
			}
			player.set_trade_cost(m);
		}
		
		//Wyrmgus start
		if (type.BoolFlag[TOWNHALL_INDEX].value) {
			bool lost_town_hall = true;
			for (int j = 0; j < player.GetUnitCount(); ++j) {
				if (player.GetUnit(j).Type->BoolFlag[TOWNHALL_INDEX].value) {
					lost_town_hall = false;
				}
			}
			if (lost_town_hall && CPlayer::GetThisPlayer()->HasContactWith(player)) {
				player.LostTownHallTimer = GameCycle + (30 * CYCLES_PER_SECOND); //30 seconds until being revealed
				for (int j = 0; j < NumPlayers; ++j) {
					if (player.get_index() != j && CPlayer::Players[j]->get_type() != player_type::nobody) {
						CPlayer::Players[j]->Notify(_("%s has lost their last town hall, and will be revealed in thirty seconds!"), player.get_name().c_str());
					} else {
						CPlayer::Players[j]->Notify("%s", _("You have lost your last town hall, and will be revealed in thirty seconds!"));
					}
				}
			}
		}
		//Wyrmgus end
	}

	//  Handle order cancels.
	unit.CurrentOrder()->Cancel(unit);

	DebugPrint("%d: Lost %s(%d)\n" _C_ player.get_index() _C_ type.get_identifier().c_str() _C_ UnitNumber(unit));
}

/**
**  Update for new unit. Food and income ...
**
**  @param unit     New unit pointer.
**  @param upgrade  True unit was upgraded.
*/
void UpdateForNewUnit(const CUnit &unit, int upgrade)
{
	const wyrmgus::unit_type &type = *unit.Type;
	CPlayer &player = *unit.Player;

	// Handle unit supply and max resources.
	// Note an upgraded unit can't give more supply.
	if (!upgrade) {
		if (unit.Variable[SUPPLY_INDEX].Value != 0) {
			player.change_supply(unit.Variable[SUPPLY_INDEX].Value);
		}

		for (const resource *resource : resource::get_all()) {
			if (player.get_max_resource(resource) != -1 && type.Stats[player.get_index()].get_storing(resource) != 0) {
				player.change_max_resource(resource, type.Stats[player.get_index()].get_storing(resource));
			}
		}
	}

	// Update resources
	for (const resource *resource : resource::get_all()) {
		if (resource == defines::get()->get_time_resource()) {
			continue;
		}

		player.set_income_modifier(resource, std::max(player.get_income_modifier(resource), type.Stats[player.get_index()].get_improve_income(resource)));
	}
	
	if (type.Stats[player.get_index()].Variables[TRADECOST_INDEX].Enable) {
		player.set_trade_cost(std::min(player.get_trade_cost(), type.Stats[player.get_index()].Variables[TRADECOST_INDEX].Value));
	}
	
	//Wyrmgus start
	if (player.LostTownHallTimer != 0 && type.BoolFlag[TOWNHALL_INDEX].value && CPlayer::GetThisPlayer()->HasContactWith(player)) {
		player.LostTownHallTimer = 0;
		player.set_revealed(false);
		for (int j = 0; j < NumPlayers; ++j) {
			if (player.get_index() != j && CPlayer::Players[j]->get_type() != player_type::nobody) {
				CPlayer::Players[j]->Notify(_("%s has rebuilt a town hall, and will no longer be revealed!"), player.get_name().c_str());
			} else {
				CPlayer::Players[j]->Notify("%s", _("You have rebuilt a town hall, and will no longer be revealed!"));
			}
		}
	}
	//Wyrmgus end
}

/**
**  Find nearest point of unit.
**
**  @param unit  Pointer to unit.
**  @param pos   tile map position.
**  @param dpos  Out: nearest point tile map position to (tx,ty).
*/
void NearestOfUnit(const CUnit &unit, const Vec2i &pos, Vec2i *dpos)
{
	const int x = unit.tilePos.x;
	const int y = unit.tilePos.y;

	*dpos = pos;
	dpos->x = std::clamp<short int>(dpos->x, x, x + unit.Type->get_tile_width() - 1);
	dpos->y = std::clamp<short int>(dpos->y, y, y + unit.Type->get_tile_height() - 1);
}

/**
**  Copy the unit look in Seen variables. This should be called when
**  buildings go under fog of war for ThisPlayer.
**
**  @param unit  The unit to work on
*/
static void UnitFillSeenValues(CUnit &unit)
{
	// Seen values are undefined for visible units.
	unit.Seen.tilePos = unit.tilePos;
	unit.Seen.pixel_offset = unit.get_pixel_offset();
	unit.Seen.Frame = unit.Frame;
	unit.Seen.Type = unit.Type;
	unit.Seen.UnderConstruction = unit.UnderConstruction;

	unit.CurrentOrder()->FillSeenValues(unit);
}

// Wall unit positions
enum {
	W_NORTH = 0x10,
	W_WEST = 0x20,
	W_SOUTH = 0x40,
	W_EAST = 0x80
};

/**
**  This function should get called when a unit goes under fog of war.
**
**  @param unit    The unit that goes under fog.
**  @param player  The player the unit goes out of fog for.
*/
void UnitGoesUnderFog(CUnit &unit, const CPlayer &player)
{
	if (unit.Type->BoolFlag[VISIBLEUNDERFOG_INDEX].value) {
		if (player.get_type() == player_type::person && !unit.Destroyed) {
			wyrmgus::unit_manager::get()->add_unit_seen_under_fog(&unit);
		}

		//
		// Icky yucky icky Seen.Destroyed trickery.
		// We track for each player if he's seen the unit as destroyed.
		// Remember, a unit is marked Destroyed when it's gone as in
		// completely gone, the corpses vanishes. In that case the unit
		// only survives since some players did NOT see the unit destroyed.
		// Keeping trackof that is hard, mostly due to complex shared vision
		// configurations.
		// A unit does NOT get a reference when it goes under fog if it's
		// Destroyed. Furthermore, it shouldn't lose a reference if it was
		// Seen destroyed. That only happened with complex shared vision, and
		// it's sort of the whole point of this tracking.
		//
		if (unit.Destroyed) {
			unit.Seen.destroyed.insert(player.get_index());
		}
		if (&player == CPlayer::GetThisPlayer()) {
			UnitFillSeenValues(unit);
		}
	}
}

/**
**  This function should get called when a unit goes out of fog of war.
**
**  @param unit    The unit that goes out of fog.
**  @param player  The player the unit goes out of fog for.
**
**  @note For units that are visible under fog (mostly buildings)
**  we use reference counts, from the players that know about
**  the building. When an building goes under fog it gets a refs
**  increase, and when it shows up it gets a decrease. It must
**  not get an decrease the first time it's seen, so we have to
**  keep track of what player saw what units, with SeenByPlayer.
*/
void UnitGoesOutOfFog(CUnit &unit, const CPlayer &player)
{
	if (!unit.Type->BoolFlag[VISIBLEUNDERFOG_INDEX].value) {
		return;
	}
	if (unit.is_seen_by_player(&player)) {
		if (player.get_type() == player_type::person && !unit.is_seen_destroyed_by_player(&player)) {
			unit_manager::get()->remove_unit_seen_under_fog(&unit);
		}
	} else {
		unit.Seen.by_player.insert(player.get_index());
	}
}

/**
**  Recalculates a units visiblity count. This happens really often,
**  Like every time a unit moves. It's really fast though, since we
**  have per-tile counts.
**
**  @param unit  pointer to the unit to check if seen
*/
void UnitCountSeen(CUnit &unit)
{
	assert_throw(unit.Type != nullptr);

	// FIXME: optimize, only work on certain players?
	// This is for instance good for updating shared vision...

	//  Store old values in oldv[p]. This store if the player could see the
	//  unit before this calc.
	std::array<int, PlayerMax> oldv{};
	for (int p = 0; p < PlayerMax; ++p) {
		if (CPlayer::Players[p]->get_type() != player_type::nobody) {
			oldv[p] = unit.IsVisible(*CPlayer::Players[p]);
		}
	}

	//  Calculate new VisCount values.
	const int height = unit.Type->get_tile_height();
	const int width = unit.Type->get_tile_width();

	for (int p = 0; p < PlayerMax; ++p) {
		if (CPlayer::Players[p]->get_type() != player_type::nobody) {
			int newv = 0;
			int y = height;
			unsigned int index = unit.Offset;
			do {
				wyrmgus::tile *mf = unit.MapLayer->Field(index);
				int x = width;
				do {
					if (unit.Type->BoolFlag[PERMANENTCLOAK_INDEX].value && unit.Player != CPlayer::Players[p].get()) {
						if (mf->player_info->VisCloak[p]) {
							newv++;
						}
					//Wyrmgus start
					} else if (unit.Type->BoolFlag[ETHEREAL_INDEX].value && unit.Player != CPlayer::Players[p].get()) {
						if (mf->player_info->VisEthereal[p]) {
							newv++;
						}
					//Wyrmgus end
					} else {
						if (mf->player_info->is_visible(*CPlayer::Players[p])) {
							newv++;
						}
					}
					++mf;
				} while (--x);
				index += unit.MapLayer->get_width();
			} while (--y);
			unit.VisCount[p] = newv;
		}
	}

	//
	// Now here comes the tricky part. We have to go in and out of fog
	// for players. Hopefully this works with shared vision just great.
	//
	for (int p = 0; p < PlayerMax; ++p) {
		if (CPlayer::Players[p]->get_type() != player_type::nobody) {
			int newv = unit.IsVisible(*CPlayer::Players[p]);
			if (!oldv[p] && newv) {
				// Might have revealed a destroyed unit which caused it to
				// be released
				if (!unit.Type) {
					break;
				}
				UnitGoesOutOfFog(unit, *CPlayer::Players[p]);
			}
			if (oldv[p] && !newv) {
				UnitGoesUnderFog(unit, *CPlayer::Players[p]);
			}
		}
	}
}

bool CUnit::IsAgressive() const
{
	//Wyrmgus start
//		return (Type->BoolFlag[CANATTACK_INDEX].value && !Type->BoolFlag[COWARD_INDEX].value
	return (CanAttack() && !Type->BoolFlag[COWARD_INDEX].value && !this->has_status_effect(status_effect::terror)
		//Wyrmgus end
		&& !this->has_status_effect(status_effect::invisible));
}

/**
**  Returns true, if the unit is visible. It check the Viscount of
**  the player and everyone who shares vision with him.
**
**  @note This understands shared vision, and should be used all around.
**
**  @param player  The player to check.
*/
bool CUnit::IsVisible(const CPlayer &player) const
{
	const int player_index = player.get_index();
	if (this->VisCount[player_index]) {
		return true;
	}

	for (const int p : player.get_shared_vision()) {
		if (this->VisCount[p]) {
			if (CPlayer::Players[p]->has_shared_vision_with(player_index)) { //if the shared vision is mutual
				return true;
			}
		}
	}

	for (const int p : CPlayer::get_revealed_player_indexes()) {
		if (this->VisCount[p]) {
			return true;
		}
	}

	return false;
}

bool CUnit::is_invisible(const CPlayer &player) const
{
	return (&player != this->Player && this->has_status_effect(status_effect::invisible)
		&& !player.has_mutual_shared_vision_with(this->Player));
}

/**
**  Returns true, if unit is shown on minimap.
**
**  @warning This is for ::ThisPlayer only.
**  @todo radar support
**
**  @return      True if visible, false otherwise.
*/
bool CUnit::IsVisibleOnMinimap() const
{
	//Wyrmgus start
	if (this->MapLayer != UI.CurrentMapLayer) {
		return false;
	}
	//Wyrmgus end

	// Invisible units.
	if (this->is_invisible(*CPlayer::GetThisPlayer())) {
		return false;
	}

	if (this->IsVisible(*CPlayer::GetThisPlayer()) || ReplayRevealMap || this->IsVisibleOnRadar(*CPlayer::GetThisPlayer())) {
		return this->IsAliveOnMap();
	}

	return this->Type->BoolFlag[VISIBLEUNDERFOG_INDEX].value && Seen.State != 3
		&& this->is_seen_by_player(CPlayer::GetThisPlayer())
		&& !this->is_seen_destroyed_by_player(CPlayer::GetThisPlayer())
		&& !Destroyed
		&& CMap::get()->Info->IsPointOnMap(this->tilePos, this->MapLayer)
		&& this->MapLayer->Field(this->tilePos)->player_info->IsTeamExplored(*CPlayer::GetThisPlayer());
}

/**
**  Returns true, if unit is visible in viewport.
**
**  @warning This is only true for ::ThisPlayer
**
**  @param vp  Viewport pointer.
**
**  @return    True if visible, false otherwise.
*/
bool CUnit::IsVisibleInViewport(const CViewport &vp) const
{
	// Check if the graphic is inside the viewport.

	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();

	QSize frame_size = this->Type->get_frame_size();
	const wyrmgus::unit_type_variation *variation = this->GetVariation();
	if (variation != nullptr && variation->get_frame_size() != QSize(0, 0)) {
		frame_size = variation->get_frame_size();
	}
	frame_size *= scale_factor;

	const QPoint pos = this->tilePos * defines::get()->get_scaled_tile_size() + this->get_scaled_pixel_offset() - size::to_point(frame_size - this->Type->get_tile_size() * defines::get()->get_scaled_tile_width()) / 2 + this->Type->get_offset() * scale_factor;
	const QSize vp_size = vp.get_pixel_size();
	const PixelPos vpTopLeftMapPos = CMap::get()->tile_pos_to_scaled_map_pixel_pos_top_left(vp.MapPos) + vp.Offset;
	const PixelPos vpBottomRightMapPos = vpTopLeftMapPos + vp_size;

	//Wyrmgus start
//	if (x + Type->Width < vpTopLeftMapPos.x || x > vpBottomRightMapPos.x
//		|| y + Type->Height < vpTopLeftMapPos.y || y > vpBottomRightMapPos.y) {
	if (pos.x() + frame_size.width() < vpTopLeftMapPos.x || pos.x() > vpBottomRightMapPos.x
		|| pos.y() + frame_size.height() < vpTopLeftMapPos.y || pos.y() > vpBottomRightMapPos.y) {
	//Wyrmgus end
		return false;
	}

	if (!CPlayer::GetThisPlayer()) {
		//FIXME: ARI: Added here for early game setup state by
		// MakeAndPlaceUnit() from LoadMap(). ThisPlayer not yet set,
		// so don't show anything until first real map-draw.
		DebugPrint("FIXME: ThisPlayer not set yet?!\n");
		return false;
	}

	// Those are never ever visible.
	if (this->is_invisible(*CPlayer::GetThisPlayer())) {
		return false;
	}

	if (IsVisible(*CPlayer::GetThisPlayer()) || ReplayRevealMap) {
		return !Destroyed;
	} else {
		// Unit has to be 'discovered'
		// Destroyed units ARE visible under fog of war, if we haven't seen them like that.
		if (!Destroyed || !this->is_seen_destroyed_by_player(CPlayer::GetThisPlayer())) {
			return this->Type->BoolFlag[VISIBLEUNDERFOG_INDEX].value && this->is_seen_by_player(CPlayer::GetThisPlayer());
		} else {
			return false;
		}
	}
}

/**
**  Change the unit's owner
**
**  @param newplayer  New owning player.
*/
//Wyrmgus start
//void CUnit::ChangeOwner(CPlayer &newplayer)
void CUnit::ChangeOwner(CPlayer &newplayer, bool show_change)
//Wyrmgus end
{
	CPlayer *oldplayer = Player;

	// This shouldn't happen
	if (oldplayer == &newplayer) {
		DebugPrint("Change the unit owner to the same player???\n");
		return;
	}

	// Can't change owner for dead units
	if (this->IsAlive() == false) {
		return;
	}

	// Rescue all units in buildings/transporters.
	//Wyrmgus start
//	for (CUnit *uins : this->get_units_inside()) {
//		uins->ChangeOwner(newplayer);
//	}

	//only rescue units inside if the player is actually a rescuable player (to avoid, for example, unintended worker owner changes when a depot changes hands)
	if (oldplayer->get_type() == player_type::rescue_active || oldplayer->get_type() == player_type::rescue_passive) {
		for (CUnit *uins : this->get_units_inside()) {
			uins->ChangeOwner(newplayer);
		}
	}
	//Wyrmgus end

	if (this->get_settlement() != nullptr && !this->Type->BoolFlag[TOWNHALL_INDEX].value) {
		//set the settlement to null before calling UnitLost() so that the food supply won't be removed twice
		this->set_settlement(nullptr);
	}

	//  Must change food/gold and other.
	UnitLost(*this);

	//  Now the new side!

	if (this->Type->BoolFlag[BUILDING_INDEX].value) {
		//Wyrmgus start
//		if (!Type->BoolFlag[WALL_INDEX].value) {
		//Wyrmgus end
			newplayer.TotalBuildings++;
		//Wyrmgus start
//		}
		//Wyrmgus end
	} else {
		newplayer.TotalUnits++;
	}

	MapUnmarkUnitSight(*this);
	newplayer.AddUnit(*this);
	Stats = &Type->Stats[newplayer.get_index()];

	//  Must change food/gold and other.
	//Wyrmgus start
//	if (Type->GivesResource) {
	if (this->GivesResource) {
	//Wyrmgus end
		DebugPrint("Resource transfer not supported\n");
	}
	newplayer.change_demand(this->Type->Stats[newplayer.get_index()].Variables[DEMAND_INDEX].Value);
	newplayer.change_supply(this->Variable[SUPPLY_INDEX].Value);

	// Increase resource limit
	for (const resource *resource : resource::get_all()) {
		if (newplayer.get_max_resource(resource) != -1 && Type->Stats[newplayer.get_index()].get_storing(resource) != 0) {
			newplayer.change_max_resource(resource, Type->Stats[newplayer.get_index()].get_storing(resource));
		}
	}

	//Wyrmgus start
//	if (Type->BoolFlag[BUILDING_INDEX].value && !Type->BoolFlag[WALL_INDEX].value) {
	if (Type->BoolFlag[BUILDING_INDEX].value) {
	//Wyrmgus end
		newplayer.NumBuildings++;
	}
	//Wyrmgus start
	if (CurrentAction() == UnitAction::Built) {
		newplayer.NumBuildingsUnderConstruction++;
		newplayer.ChangeUnitTypeUnderConstructionCount(this->Type, 1);
	}
	//Wyrmgus end

	//apply upgrades of the new player, if the old one doesn't have that upgrade
	for (const wyrmgus::upgrade_modifier *modifier : wyrmgus::upgrade_modifier::UpgradeModifiers) {
		const CUpgrade *modifier_upgrade = modifier->get_upgrade();
		if (oldplayer->Allow.Upgrades[modifier_upgrade->ID] != 'R' && newplayer.Allow.Upgrades[modifier_upgrade->ID] == 'R' && modifier->applies_to(this->Type)) { //if the old player doesn't have the modifier's upgrade (but the new one does), and the upgrade is applicable to the unit
			//Wyrmgus start
//			ApplyIndividualUpgradeModifier(*this, modifier);
			if ( // don't apply equipment-related upgrades if the unit has an item of that equipment type equipped
				(!modifier_upgrade->is_weapon() || EquippedItems[static_cast<int>(wyrmgus::item_slot::weapon)].size() == 0)
				&& (!modifier_upgrade->is_shield() || EquippedItems[static_cast<int>(wyrmgus::item_slot::shield)].size() == 0)
				&& (!modifier_upgrade->is_boots() || EquippedItems[static_cast<int>(wyrmgus::item_slot::boots)].size() == 0)
				&& (!modifier_upgrade->is_arrows() || EquippedItems[static_cast<int>(wyrmgus::item_slot::arrows)].size() == 0)
				&& !(newplayer.Race != -1 && modifier_upgrade == wyrmgus::civilization::get_all()[newplayer.Race]->get_upgrade())
				&& !(newplayer.Race != -1 && newplayer.get_faction() != nullptr && modifier_upgrade == newplayer.get_faction()->get_upgrade())
			) {
				ApplyIndividualUpgradeModifier(*this, modifier);
			}
			//Wyrmgus end
		}
	}

	newplayer.IncreaseCountsForUnit(this);
	UpdateForNewUnit(*this, 1);
	
	UpdateUnitSightRange(*this);
	MapMarkUnitSight(*this);
		
	//Wyrmgus start
	if (&newplayer == CPlayer::GetThisPlayer() && show_change) {
		this->Blink = 5;
		PlayGameSound(game_sound_set::get()->get_rescue_sound(), MaxSampleVolume);
	}
	//Wyrmgus end

	this->update_site_owner();

	if (this->get_settlement() == nullptr) {
		this->UpdateSettlement();
	}
}

static bool IsMineAssignedBy(const CUnit *mine, const CUnit *worker)
{
	for (const std::shared_ptr<wyrmgus::unit_ref> &unit_ref : mine->Resource.Workers) {
		const CUnit *unit = unit_ref->get();
		if (unit == worker) {
			return true;
		}
	}
	
	return false;
}

void CUnit::AssignWorkerToMine(CUnit &mine)
{
	if (IsMineAssignedBy(&mine, this) == true) {
		return;
	}

#if 0
	DebugPrint("%d: Worker [%d] is adding into %s [%d] on %d pos\n"
			   _C_ this->Player->get_index() _C_ this->Slot
			   _C_ mine.Type->Name.c_str()
			   _C_ mine.Slot
			   _C_ mine.Data.Resource.Assigned);
#endif

	mine.Resource.Workers.push_back(this->acquire_ref());
}

void CUnit::DeAssignWorkerFromMine(CUnit &mine)
{
	if (IsMineAssignedBy(&mine, this) == false) {
		return;
	}

#if 0
	DebugPrint("%d: Worker [%d] is removing from %s [%d] left %d units assigned\n"
			   _C_ this->Player->get_index() _C_ this->Slot
			   _C_ mine.Type->Name.c_str()
			   _C_ mine.Slot
			   _C_ mine.CurrentOrder()->Data.Resource.Assigned);
#endif

	for (size_t i = 0; i < mine.Resource.Workers.size(); ++i) {
		const CUnit *worker = *mine.Resource.Workers[i];

		if (worker == this) {
			mine.Resource.Workers.erase(mine.Resource.Workers.begin() + i);
			break;
		}
	}
}


/**
**  Change the owner of all units of a player.
**
**  @param oldplayer    Old owning player.
**  @param newplayer    New owning player.
*/
static void ChangePlayerOwner(CPlayer &oldplayer, CPlayer &newplayer)
{
	if (&oldplayer == &newplayer) {
		return;
	}

	for (int i = 0; i != oldplayer.GetUnitCount(); ++i) {
		CUnit &unit = oldplayer.GetUnit(i);

		unit.Blink = 5;
		unit.set_player_from(&oldplayer);
	}

	// ChangeOwner remove unit from the player: so change the array.
	while (oldplayer.GetUnitCount() != 0) {
		CUnit &unit = oldplayer.GetUnit(0);

		unit.ChangeOwner(newplayer);
	}
}

/**
**  Rescue units.
**
**  Look through all rescueable players, if they could be rescued.
*/
void RescueUnits()
{
	if (NoRescueCheck) {  // all possible units are rescued
		return;
	}
	NoRescueCheck = true;

	//  Look if player could be rescued.
	for (const qunique_ptr<CPlayer> &p : CPlayer::Players) {
		if (p->get_type() != player_type::rescue_passive && p->get_type() != player_type::rescue_active) {
			continue;
		}
		if (p->GetUnitCount() != 0) {
			NoRescueCheck = false;
			// NOTE: table is changed.
			std::vector<CUnit *> table;
			table.insert(table.begin(), p->UnitBegin(), p->UnitEnd());

			const size_t l = table.size();
			for (size_t j = 0; j != l; ++j) {
				CUnit &unit = *table[j];
				// Do not rescue removed units. Units inside something are
				// rescued by ChangeUnitOwner
				if (unit.Removed) {
					continue;
				}
				std::vector<CUnit *> around;

				SelectAroundUnit(unit, 1, around);
				//  Look if ally near the unit.
				for (size_t i = 0; i != around.size(); ++i) {
					//Wyrmgus start
//					if (around[i]->Type->CanAttack && unit.is_allied_with(*around[i]) && around[i]->Player->get_type() != player_type::rescue_passive && around[i]->Player->get_type() != player_type::rescue_active) {
					if (around[i]->CanAttack() && unit.is_allied_with(*around[i]) && around[i]->Player->get_type() != player_type::rescue_passive && around[i]->Player->get_type() != player_type::rescue_active) {
					//Wyrmgus end
						//  City center converts complete race
						//  NOTE: I use a trick here, centers could
						//        store gold. FIXME!!!
						//Wyrmgus start
//						if (unit.Type->CanStore[GoldCost]) {
						if (unit.Type->BoolFlag[TOWNHALL_INDEX].value) {
						//Wyrmgus end
							ChangePlayerOwner(*p, *around[i]->Player);
							break;
						}

						unit.set_player_from(unit.Player);

						//Wyrmgus start
//						unit.ChangeOwner(*around[i]->Player);
						unit.ChangeOwner(*around[i]->Player, true);
//						unit.Blink = 5;
//						PlayGameSound(wyrmgus::game_sound_set::get()->get_rescue_sound(), MaxSampleVolume);
						//Wyrmgus end
						break;
					}
				}
			}
		}
	}
}

/*----------------------------------------------------------------------------
--  Unit headings
----------------------------------------------------------------------------*/

static std::array<unsigned char, 2608> create_atan_table()
{
	std::array<unsigned char, 2608> atan_table{};

	for (size_t i = 0; i < atan_table.size(); ++i) {
		atan_table[i] = (unsigned char) (atan((double) i / 64) * (64 * 4 / 6.2831853));
	}

	return atan_table;
}

/**
**  Fast arc tangent function.
**
**  @param val  atan argument
**
**  @return     atan(val)
*/
static int myatan(int val)
{
	static const std::array<unsigned char, 2608> atan_table = create_atan_table();

	if (val >= 2608) {
		return 63;
	}

	return atan_table[val];
}

/**
**  Convert direction to heading.
**
**  @param delta  Delta.
**
**  @return         Angle (0..255)
*/
int DirectionToHeading(const Vec2i &delta)
{
	//  Check which quadrant.
	if (delta.x > 0) {
		if (delta.y < 0) { // Quadrant 1?
			return myatan((delta.x * 64) / -delta.y);
		}
		// Quadrant 2?
		return myatan((delta.y * 64) / delta.x) + 64;
	}
	if (delta.y > 0) { // Quadrant 3?
		return myatan((delta.x * -64) / delta.y) + 64 * 2;
	}
	if (delta.x) { // Quadrant 4.
		return myatan((delta.y * -64) / -delta.x) + 64 * 3;
	}
	return 0;
}

/**
**  Update sprite frame for new heading.
*/
void UnitUpdateHeading(CUnit &unit, const bool notify)
{
	//Wyrmgus start
	//fix direction if it does not correspond to one of the defined directions
	int num_dir = std::max<int>(8, unit.Type->get_num_directions());
	if (unit.Direction % (256 / num_dir) != 0) {
		unit.Direction = unit.Direction - (unit.Direction % (256 / num_dir));
	}
	//Wyrmgus end
	
	int dir;
	int nextdir;
	bool neg;

	if (unit.Frame < 0) {
		unit.Frame = -unit.Frame - 1;
		neg = true;
	} else {
		neg = false;
	}
	unit.Frame /= unit.Type->get_num_directions() / 2 + 1;
	unit.Frame *= unit.Type->get_num_directions() / 2 + 1;
	// Remove heading, keep animation frame

	nextdir = 256 / unit.Type->get_num_directions();
	dir = ((unit.Direction + nextdir / 2) & 0xFF) / nextdir;
	if (dir <= LookingS / nextdir) { // north->east->south
		unit.Frame += dir;
	} else {
		unit.Frame += 256 / nextdir - dir;
		unit.Frame = -unit.Frame - 1;
	}
	if (neg && !unit.Frame && unit.Type->BoolFlag[BUILDING_INDEX].value) {
		unit.Frame = -1;
	}

	if (notify && unit.MapLayer != nullptr && !unit.Removed && game::get()->is_running()) {
		emit unit.MapLayer->unit_frame_changed(UnitNumber(unit), unit.Frame);
	}
}

/**
**  Change unit heading/frame from delta direction x, y.
**
**  @param unit  Unit for new direction looking.
**  @param delta  map tile delta direction.
*/
void UnitHeadingFromDeltaXY(CUnit &unit, const Vec2i &delta)
{
	//Wyrmgus start
//	unit.Direction = DirectionToHeading(delta);
	int num_dir = std::max<int>(8, unit.Type->get_num_directions());
	int heading = DirectionToHeading(delta) + ((256 / num_dir) / 2);
	if (heading % (256 / num_dir) != 0) {
		heading = heading - (heading % (256 / num_dir));
	}
	unit.Direction = heading;
	//Wyrmgus end
	UnitUpdateHeading(unit);
}

/*----------------------------------------------------------------------------
  -- Drop out units
  ----------------------------------------------------------------------------*/

/**
**  Drop out all units inside unit.
**
**  @param source  All units inside source are dropped out.
*/
void DropOutAll(const CUnit &source)
{
	//copy the vector since we may modify it
	const std::vector<CUnit *> units_inside = source.get_units_inside();

	for (CUnit *unit : units_inside) {
		unit->drop_out_on_side(LookingW, &source);

		//Wyrmgus start
		if (unit->Type->BoolFlag[ITEM_INDEX].value && unit->get_unique() == nullptr) {
			//save the initial cycle items were placed in the ground to destroy them if they have been there for too long
			int ttl_cycles = (5 * 60 * CYCLES_PER_SECOND);
			if (unit->Prefix != nullptr || unit->Suffix != nullptr || unit->Spell != nullptr || unit->Work != nullptr || unit->Elixir != nullptr) {
				ttl_cycles *= 4;
			}
			unit->TTL = GameCycle + ttl_cycles;
		}
		//Wyrmgus end
	}
}

/*----------------------------------------------------------------------------
  -- Select units
  ----------------------------------------------------------------------------*/

/**
**  Unit on map screen.
**
**  Select units on screen. (x, y are in pixels relative to map 0,0).
**  Not GAMEPLAY safe, uses ReplayRevealMap
**
**  More units on same position.
**    Cycle through units.
**    First take highest unit.
**
**  @param x      X pixel position.
**  @param y      Y pixel position.
**
**  @return       An unit on x, y position.
*/
CUnit *UnitOnScreen(int x, int y)
{
	CUnit *candidate = nullptr;
	bool is_candidate_selected = false;

	const CViewport *vp = UI.SelectedViewport;

	std::vector<CUnit *> screen_units;

	//here we assume that no unit has a box size which would lead it to visually fully contain a tile which it is not on
	const QPoint min_map_pos = vp->MapPos - QPoint(1, 1);
	const QPoint max_map_pos = vp->MapPos + QPoint(vp->MapWidth, vp->MapHeight);
	Select(min_map_pos, max_map_pos, screen_units, UI.CurrentMapLayer->ID);

	for (CUnit *unit : screen_units) {
		if (!ReplayRevealMap && !unit->IsVisibleAsGoal(*CPlayer::GetThisPlayer())) {
			continue;
		}

		const unit_type &type = *unit->Type;
		if (!type.Sprite) {
			continue;
		}

		//
		// Check if mouse is over the unit.
		//
		QPoint unit_sprite_pos = unit->get_scaled_map_pixel_pos_center();
		const centesimal_int &scale_factor = preferences::get()->get_scale_factor();

		unit_sprite_pos -= size::to_point(type.get_box_size()) * scale_factor / 2;
		unit_sprite_pos += type.get_box_offset() * scale_factor;

		if (x >= unit_sprite_pos.x() && x < (unit_sprite_pos.x() + (type.get_box_width() * scale_factor).to_int())
			&& y >= unit_sprite_pos.y() && y < (unit_sprite_pos.y() + (type.get_box_height() * scale_factor).to_int())) {
			if (unit->Type->BoolFlag[ISNOTSELECTABLE_INDEX].value) {
				continue;
			}

			if (!unit->IsAlive()) {
				//don't selected a dead unit
				continue;
			}

			if (candidate != nullptr && !is_candidate_selected) {
				if (unit->Player->get_type() == player_type::neutral && candidate->Player->get_type() != player_type::neutral) {
					//prefer selecting a non-neutral unit over a neutral one
					continue;
				}

				if (unit->Type->BoolFlag[ITEM_INDEX].value && !candidate->Type->BoolFlag[ITEM_INDEX].value) {
					//prefer selecting a non-item unit over an item one
					continue;
				}

				if ((unit->Player->get_type() == player_type::neutral) == (candidate->Player->get_type() == player_type::neutral) && (unit->Type->BoolFlag[ITEM_INDEX].value == candidate->Type->BoolFlag[ITEM_INDEX].value)) {
					continue; //no difference
				}
			}

			if (candidate != nullptr && IsOnlySelected(*unit)) {
				//don't pick the unit if it is already selected
				continue;
			}

			candidate = unit;
			is_candidate_selected = IsOnlySelected(*candidate);

			if (is_candidate_selected || candidate->Player->get_type() == player_type::neutral || candidate->Type->BoolFlag[ITEM_INDEX].value) {
				//check if there are other units in this place
				continue;
			} else {
				break;
			}
		} else {
			continue;
		}
	}

	return candidate;
}

PixelPos CUnit::get_map_pixel_pos_top_left() const
{
	PixelPos pos(this->tilePos.x * defines::get()->get_tile_width() + this->get_pixel_offset().x(), this->tilePos.y * defines::get()->get_tile_height() + this->get_pixel_offset().y());
	return pos;
}

PixelPos CUnit::get_scaled_map_pixel_pos_top_left() const
{
	return QPoint(this->get_map_pixel_pos_top_left()) * preferences::get()->get_scale_factor();
}

PixelPos CUnit::get_map_pixel_pos_center() const
{
	return this->get_map_pixel_pos_top_left() + this->get_half_tile_pixel_size();
}

PixelPos CUnit::get_scaled_map_pixel_pos_center() const
{
	return this->get_scaled_map_pixel_pos_top_left() + this->get_scaled_half_tile_pixel_size();
}

const QSize &CUnit::get_tile_size() const
{
	return this->Type->get_tile_size();
}

//Wyrmgus start
Vec2i CUnit::GetHalfTileSize() const
{
	return this->get_tile_size() / 2;
}

PixelSize CUnit::get_tile_pixel_size() const
{
	return PixelSize(this->get_tile_size()) * defines::get()->get_tile_size();
}

PixelSize CUnit::get_scaled_tile_pixel_size() const
{
	return QSize(this->get_tile_pixel_size()) * preferences::get()->get_scale_factor();
}

PixelSize CUnit::get_half_tile_pixel_size() const
{
	return this->get_tile_pixel_size() / 2;
}

PixelSize CUnit::get_scaled_half_tile_pixel_size() const
{
	return QPoint(this->get_half_tile_pixel_size()) * preferences::get()->get_scale_factor();
}

QPoint CUnit::get_bottom_right_tile_pos() const
{
	const CUnit *first_container = this->GetFirstContainer();
	return first_container->tilePos + size::to_point(first_container->Type->get_tile_size()) - QPoint(1, 1);
}

QPoint CUnit::get_center_tile_pos() const
{
	const CUnit *first_container = this->GetFirstContainer();
	return first_container->tilePos + first_container->Type->get_tile_center_pos_offset();
}

const tile *CUnit::get_center_tile() const
{
	return this->MapLayer->Field(this->get_center_tile_pos());
}

QPoint CUnit::get_scaled_pixel_offset() const
{
	return this->get_pixel_offset() * preferences::get()->get_scale_factor();
}

const resource *CUnit::get_given_resource() const
{
	if (this->GivesResource != 0) {
		return resource::get_all()[this->GivesResource];
	}

	return nullptr;
}

const resource *CUnit::get_current_resource() const
{
	if (this->CurrentResource != 0) {
		return resource::get_all()[this->CurrentResource];
	}

	return nullptr;
}

void CUnit::SetIndividualUpgrade(const CUpgrade *upgrade, int quantity)
{
	if (!upgrade) {
		return;
	}
	
	if (quantity <= 0) {
		if (this->IndividualUpgrades.find(upgrade->ID) != this->IndividualUpgrades.end()) {
			this->IndividualUpgrades.erase(upgrade->ID);
		}
	} else {
		this->IndividualUpgrades[upgrade->ID] = quantity;
	}
}

int CUnit::GetIndividualUpgrade(const CUpgrade *upgrade) const
{
	if (upgrade && this->IndividualUpgrades.find(upgrade->ID) != this->IndividualUpgrades.end()) {
		return this->IndividualUpgrades.find(upgrade->ID)->second;
	} else {
		return 0;
	}
}

int CUnit::GetAvailableLevelUpUpgrades(bool only_units) const
{
	int value = 0;
	int upgrade_value = 0;
	
	if (((int) AiHelpers.ExperienceUpgrades.size()) > Type->Slot) {
		for (size_t i = 0; i != AiHelpers.ExperienceUpgrades[Type->Slot].size(); ++i) {
			if (this->get_character() == nullptr || !wyrmgus::vector::contains(this->get_character()->ForbiddenUpgrades, AiHelpers.ExperienceUpgrades[Type->Slot][i])) {
				int local_upgrade_value = 1;
				
				if (!only_units) {
					local_upgrade_value += AiHelpers.ExperienceUpgrades[Type->Slot][i]->GetAvailableLevelUpUpgrades();
				}
				
				if (local_upgrade_value > upgrade_value) {
					upgrade_value = local_upgrade_value;
				}
			}
		}
	}
	
	value += upgrade_value;
	
	if (!only_units && ((int) AiHelpers.LearnableAbilities.size()) > Type->Slot) {
		for (size_t i = 0; i != AiHelpers.LearnableAbilities[Type->Slot].size(); ++i) {
			value += AiHelpers.LearnableAbilities[Type->Slot][i]->MaxLimit - this->GetIndividualUpgrade(AiHelpers.LearnableAbilities[Type->Slot][i]);
		}
	}
	
	return value;
}

void CUnit::add_bonus_ability(const CUpgrade *ability)
{
	assert_throw(ability->is_ability());

	this->bonus_abilities.push_back(ability);
	IndividualUpgradeAcquire(*this, ability);
}

void CUnit::remove_bonus_ability(const CUpgrade *ability)
{
	assert_throw(ability->is_ability());

	vector::remove_one(this->bonus_abilities, ability);

	if (this->GetIndividualUpgrade(ability) > 0) {
		IndividualUpgradeLost(*this, ability);
	}
}

int CUnit::GetModifiedVariable(const int index, const VariableAttribute variable_type) const
{
	int value = 0;

	switch (variable_type) {
		case VariableAttribute::Value:
			value = this->get_variable_value(index);
			break;
		case VariableAttribute::Max:
			value = this->get_variable_max(index);
			break;
		case VariableAttribute::Increase:
			value = this->get_variable_increase(index);
			break;
		default:
			break;
	}
	
	switch (index) {
		case ATTACKRANGE_INDEX:
			if (this->Container != nullptr && this->Container->Variable[GARRISONEDRANGEBONUS_INDEX].Enable && this->Type->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value) {
				value += this->Container->Variable[GARRISONEDRANGEBONUS_INDEX].Value; //treat the container's attack range as a bonus to the unit's attack range
			}

			//if the unit's current sight range is smaller than its attack range, use it instead
			value = std::min<int>(this->CurrentSightRange, value); 
			break;
		case SPEED_INDEX: {
			const wyrmgus::unit_type *unit_type = this->Type;

			if (!unit_type->CanMove()) {
				break;
			}

			const CMapLayer *map_layer = this->MapLayer;

			if (map_layer == nullptr) {
				break;
			}

			const unit_domain domain = unit_type->get_domain();
			if (domain != unit_domain::air && domain != unit_domain::air_low && domain != unit_domain::space) {
				int movement_cost = 0;
				int tile_speed_bonus = 0;

				for (int x = 0; x < unit_type->get_tile_width(); ++x) {
					for (int y = 0; y < unit_type->get_tile_height(); ++y) {
						const tile *tile = map_layer->Field(this->tilePos + Vec2i(x, y));

						movement_cost += tile->get_movement_cost();

						if (this->Variable[RAIL_SPEED_BONUS_INDEX].Value != 0 && tile->has_flag(tile_flag::railroad)) {
							tile_speed_bonus += this->Variable[RAIL_SPEED_BONUS_INDEX].Value;
						}
					}
				}

				const int tile_count = unit_type->get_tile_width() * unit_type->get_tile_height();
				movement_cost /= tile_count;
				tile_speed_bonus /= tile_count;

				value += DefaultTileMovementCost - movement_cost;
				value += tile_speed_bonus;
			}
			break;
		}
		default:
			break;
	}
	
	return value;
}

int CUnit::GetModifiedVariable(const int index) const
{
	return this->GetModifiedVariable(index, VariableAttribute::Value);
}

int CUnit::get_best_attack_range() const
{
	return std::max(this->GetModifiedVariable(ATTACKRANGE_INDEX), this->best_contained_unit_attack_range);
}

int CUnit::GetReactionRange() const
{
	int reaction_range = this->CurrentSightRange;

	if (this->Player->get_type() == player_type::computer) {
		reaction_range += 2; //+2 reaction range bonus for computer players
	}
	
	return reaction_range;
}

unsigned CUnit::get_item_slot_quantity(const wyrmgus::item_slot item_slot) const
{
	if (!this->HasInventory()) {
		return 0;
	}
	
	if ( //if the item are arrows and the weapon of this unit's type is not a bow, return false
		item_slot == wyrmgus::item_slot::arrows
		&& Type->WeaponClasses[0] != wyrmgus::item_class::bow
	) {
		return 0;
	}
	
	if (item_slot == wyrmgus::item_slot::ring) {
		return 2;
	}
	
	return 1;
}

wyrmgus::item_class CUnit::GetCurrentWeaponClass() const
{
	if (HasInventory() && EquippedItems[static_cast<int>(wyrmgus::item_slot::weapon)].size() > 0) {
		return EquippedItems[static_cast<int>(wyrmgus::item_slot::weapon)][0]->Type->get_item_class();
	}
	
	return Type->WeaponClasses[0];
}

int CUnit::GetItemVariableChange(const CUnit *item, int variable_index, bool increase) const
{
	if (item->Type->get_item_class() == wyrmgus::item_class::none) {
		return 0;
	}
	
	const wyrmgus::item_slot item_slot = wyrmgus::get_item_class_slot(item->Type->get_item_class());
	if (item->Work == nullptr && item->Elixir == nullptr && (item_slot == wyrmgus::item_slot::none || this->get_item_slot_quantity(item_slot) == 0 || !this->can_equip_item_class(item->Type->get_item_class()))) {
		return 0;
	}
	
	int value = 0;
	if (item->Work != nullptr) {
		if (this->GetIndividualUpgrade(item->Work) == 0) {
			for (const auto &modifier : item->Work->get_modifiers()) {
				if (!increase) {
					value += modifier->Modifier.Variables[variable_index].Value;
				} else {
					value += modifier->Modifier.Variables[variable_index].Increase;
				}
			}
		}
	} else if (item->Elixir != nullptr) {
		if (this->GetIndividualUpgrade(item->Elixir) == 0) {
			for (const auto &modifier : item->Elixir->get_modifiers()) {
				if (!increase) {
					value += modifier->Modifier.Variables[variable_index].Value;
				} else {
					value += modifier->Modifier.Variables[variable_index].Increase;
				}
			}
		}
	} else {
		if (!increase) {
			value = item->Variable[variable_index].Value;
		} else {
			value = item->Variable[variable_index].Increase;
		}
		
		if (!item->Identified) { //if the item is unidentified, don't show the effects of its affixes
			for (const wyrmgus::upgrade_modifier *modifier : wyrmgus::upgrade_modifier::UpgradeModifiers) {
				if (
					(item->Prefix != nullptr && modifier->get_upgrade() == item->Prefix)
					|| (item->Suffix != nullptr && modifier->get_upgrade() == item->Suffix)
				) {
					if (!increase) {
						value -= modifier->Modifier.Variables[variable_index].Value;
					} else {
						value -= modifier->Modifier.Variables[variable_index].Increase;
					}
				}
			}
		}
		
		if (item->get_unique() != nullptr && item->get_unique()->get_set() != nullptr) {
			if (this->equipping_item_completes_set(item)) {
				for (const auto &modifier : item->get_unique()->get_set()->get_modifiers()) {
					if (!increase) {
						value += modifier->Modifier.Variables[variable_index].Value;
					} else {
						value += modifier->Modifier.Variables[variable_index].Increase;
					}
				}
			}
		}
		
		if (EquippedItems[static_cast<int>(item_slot)].size() == this->get_item_slot_quantity(item_slot)) {
			int item_slot_used = EquippedItems[static_cast<int>(item_slot)].size() - 1;
			for (size_t i = 0; i < EquippedItems[static_cast<int>(item_slot)].size(); ++i) {
				if (EquippedItems[static_cast<int>(item_slot)][i] == item) {
					item_slot_used = i;
				}
			}

			const CUnit *equipped_item = this->EquippedItems[static_cast<int>(item_slot)][item_slot_used];
			if (!increase) {
				value -= equipped_item->Variable[variable_index].Value;
			} else {
				value -= equipped_item->Variable[variable_index].Increase;
			}
			if (equipped_item != item && equipped_item->get_unique() != nullptr && equipped_item->get_unique()->get_set() != nullptr) {
				if (this->DeequippingItemBreaksSet(equipped_item)) {
					for (const auto &modifier : equipped_item->get_unique()->get_set()->get_modifiers()) {
						if (!increase) {
							value -= modifier->Modifier.Variables[variable_index].Value;
						} else {
							value -= modifier->Modifier.Variables[variable_index].Increase;
						}
					}
				}
			}
		} else if (EquippedItems[static_cast<int>(item_slot)].size() == 0 && (item_slot == wyrmgus::item_slot::weapon || item_slot == wyrmgus::item_slot::shield || item_slot == wyrmgus::item_slot::boots || item_slot == wyrmgus::item_slot::arrows)) {
			for (const wyrmgus::upgrade_modifier *modifier : wyrmgus::upgrade_modifier::UpgradeModifiers) {
				const CUpgrade *modifier_upgrade = modifier->get_upgrade();
				if (
					(
						(
							(modifier_upgrade->is_weapon() && item_slot == wyrmgus::item_slot::weapon)
							|| (modifier_upgrade->is_shield() && item_slot == wyrmgus::item_slot::shield)
							|| (modifier_upgrade->is_boots() && item_slot == wyrmgus::item_slot::boots)
							|| (modifier_upgrade->is_arrows() && item_slot == wyrmgus::item_slot::arrows)
						)
						&& Player->Allow.Upgrades[modifier_upgrade->ID] == 'R' && modifier->applies_to(this->Type)
					)
					|| (item_slot == wyrmgus::item_slot::weapon && modifier_upgrade->is_ability() && this->GetIndividualUpgrade(modifier_upgrade) && modifier_upgrade->WeaponClasses.size() > 0 && modifier_upgrade->WeaponClasses.contains(this->GetCurrentWeaponClass()) && !modifier_upgrade->WeaponClasses.contains(item->Type->get_item_class()))
				) {
					if (this->GetIndividualUpgrade(modifier_upgrade)) {
						for (int i = 0; i < this->GetIndividualUpgrade(modifier_upgrade); ++i) {
							if (!increase) {
								value -= modifier->Modifier.Variables[variable_index].Value;
							} else {
								value -= modifier->Modifier.Variables[variable_index].Increase;
							}
						}
					} else {
						if (!increase) {
							value -= modifier->Modifier.Variables[variable_index].Value;
						} else {
							value -= modifier->Modifier.Variables[variable_index].Increase;
						}
					}
				} else if (
					modifier_upgrade->is_ability() && this->GetIndividualUpgrade(modifier_upgrade) && modifier_upgrade->WeaponClasses.size() > 0 && !modifier_upgrade->WeaponClasses.contains(this->GetCurrentWeaponClass()) && modifier_upgrade->WeaponClasses.contains(item->Type->get_item_class())
				) {
					if (this->GetIndividualUpgrade(modifier_upgrade)) {
						for (int i = 0; i < this->GetIndividualUpgrade(modifier_upgrade); ++i) {
							if (!increase) {
								value += modifier->Modifier.Variables[variable_index].Value;
							} else {
								value += modifier->Modifier.Variables[variable_index].Increase;
							}
						}
					} else {
						if (!increase) {
							value += modifier->Modifier.Variables[variable_index].Value;
						} else {
							value += modifier->Modifier.Variables[variable_index].Increase;
						}
					}
				}
			}
		}
	}
	
	return value;
}

const CPlayer *CUnit::get_display_player() const
{
	if (this->Type->BoolFlag[HIDDENOWNERSHIP_INDEX].value && this->Player != CPlayer::GetThisPlayer()) {
		return CPlayer::get_neutral_player();
	} else if (this->is_rescued()) {
		return this->get_player_from();
	}

	return this->Player;
}

int CUnit::get_price() const
{
	const unit_stats &stats = this->Type->Stats[this->Player->get_index()];

	int cost = stats.get_price();
	
	if (this->Prefix != nullptr) {
		cost += this->Prefix->get_magic_level() * 1000;
	}

	if (this->Suffix != nullptr) {
		cost += this->Suffix->get_magic_level() * 1000;
	}

	if (this->Spell != nullptr) {
		cost += 1000;
	}

	if (this->Work != nullptr) {
		if (this->Type->get_item_class() == item_class::book) {
			cost += 5000;
		} else {
			cost += 1000;
		}
	}

	if (this->Elixir != nullptr) {
		cost += this->Elixir->get_magic_level() * 1000;
	}

	if (this->get_character() != nullptr) {
		cost += (this->Variable[LEVEL_INDEX].Value - stats.Variables[LEVEL_INDEX].Value) * 250;
	}

	if (this->Variable[COST_MODIFIER_INDEX].Value != 0) {
		cost *= 100 + this->Variable[COST_MODIFIER_INDEX].Value;
		cost /= 100;
	}
	
	return cost;
}

bool CUnit::is_rescued() const
{
	if (this->get_player_from() == nullptr) {
		return false;
	}

	switch (this->get_player_from()->get_type()) {
		case player_type::rescue_active:
		case player_type::rescue_passive:
			break;
		default:
			return false;
	}

	return true;
}

int CUnit::get_resource_step(const resource *resource) const
{
	const resource_info *res_info = this->Type->get_resource_info(resource);

	if (res_info == nullptr) {
		throw std::runtime_error("Tried to get the resource step for resource \"" + resource->get_identifier() + "\" for a unit of type \"" + this->Type->get_identifier() + "\", which doesn't support gathering that resource.");
	}

	int resource_step = res_info->ResourceStep;
	
	resource_step += this->Variable[GATHERINGBONUS_INDEX].Value;

	const int res_index = resource->get_index();
	
	if (res_index == CopperCost) {
		resource_step += this->Variable[COPPERGATHERINGBONUS_INDEX].Value;
	} else if (res_index == SilverCost) {
		resource_step += this->Variable[SILVERGATHERINGBONUS_INDEX].Value;
	} else if (res_index == GoldCost) {
		resource_step += this->Variable[GOLDGATHERINGBONUS_INDEX].Value;
	} else if (res_index == IronCost) {
		resource_step += this->Variable[IRONGATHERINGBONUS_INDEX].Value;
	} else if (res_index == MithrilCost) {
		resource_step += this->Variable[MITHRILGATHERINGBONUS_INDEX].Value;
	} else if (res_index == WoodCost) {
		resource_step += this->Variable[LUMBERGATHERINGBONUS_INDEX].Value;
	} else if (res_index == StoneCost || res_index == LimestoneCost) {
		resource_step += this->Variable[STONEGATHERINGBONUS_INDEX].Value;
	} else if (res_index == CoalCost) {
		resource_step += this->Variable[COALGATHERINGBONUS_INDEX].Value;
	} else if (res_index == JewelryCost) {
		resource_step += this->Variable[JEWELRYGATHERINGBONUS_INDEX].Value;
	} else if (res_index == FurnitureCost) {
		resource_step += this->Variable[FURNITUREGATHERINGBONUS_INDEX].Value;
	} else if (res_index == LeatherCost) {
		resource_step += this->Variable[LEATHERGATHERINGBONUS_INDEX].Value;
	} else if (res_index == DiamondsCost || res_index == EmeraldsCost) {
		resource_step += this->Variable[GEMSGATHERINGBONUS_INDEX].Value;
	}
	
	return resource_step;
}

int CUnit::get_garrisoned_gathering_rate(const resource *resource) const
{
	return this->get_resource_step(resource) * 100;
}

int CUnit::GetTotalInsideCount(const CPlayer *player, const bool ignore_items, const bool ignore_saved_cargo, const unit_type *type) const
{
	if (!this->has_units_inside()) {
		return 0;
	}
	
	if (this->Type->BoolFlag[SAVECARGO_INDEX].value && ignore_saved_cargo) {
		return 0;
	}
	
	int inside_count = 0;
	
	for (const CUnit *inside_unit : this->get_units_inside()) {
		if ( //only count units of the faction, ignore items
			(!player || inside_unit->Player == player)
			&& (!ignore_items || !inside_unit->Type->BoolFlag[ITEM_INDEX].value)
			&& (!type || inside_unit->Type == type)
		) {
			inside_count++;
		}

		inside_count += inside_unit->GetTotalInsideCount(player, ignore_items, ignore_saved_cargo);
	}

	return inside_count;
}

bool CUnit::CanAttack(bool count_inside) const
{
	if (this->Type->CanTransport() && this->Type->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value && !this->Type->BoolFlag[CANATTACK_INDEX].value) { //transporters without an attack can only attack through a unit within them
		if (count_inside && this->BoardCount > 0) {
			for (const CUnit *boarded_unit : this->get_units_inside()) {
				if (boarded_unit->GetModifiedVariable(ATTACKRANGE_INDEX) > 1 && boarded_unit->Type->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value) {
					return true;
				}
			}
		}
		return false;
	}
	
	if (this->Container) {
		if (!this->Type->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value || !this->Container->Type->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value) {
			return false;
		}

		//if a neutral unit is inside a container representing a recruitable hero, then it shouldn't attack from it
		if (this->Player->get_type() == player_type::neutral && this->Container->Type->BoolFlag[RECRUITHEROES_INDEX].value) {
			return false;
		}
	}
	
	return this->Type->BoolFlag[CANATTACK_INDEX].value;
}

bool CUnit::IsInCombat() const
{
	// Select all units around the unit
	std::vector<CUnit *> table;
	SelectAroundUnit(*this, this->GetReactionRange(), table, IsEnemyWithUnit(this));

	for (size_t i = 0; i < table.size(); ++i) {
		const CUnit &target = *table[i];

		if (target.IsVisibleAsGoal(*this->Player) && (this->Type->can_target(&target) || target.Type->can_target(this))) {
			return true;
		}
	}
	
	return false;
}

bool CUnit::can_harvest(const resource *resource) const
{
	if (resource == nullptr) {
		return false;
	}

	if (!this->Type->get_resource_info(resource)) {
		return false;
	}

	if (!this->Type->BoolFlag[HARVESTER_INDEX].value) {
		return false;
	}

	if (this->BoardCount > 0) {
		//cannot harvest if carrying units
		return false;
	}

	return true;
}

bool CUnit::can_harvest(const CUnit *dest, const bool only_harvestable) const
{
	if (dest == nullptr) {
		return false;
	}
	
	if (!this->can_harvest(dest->get_given_resource())) {
		return false;
	}
	
	if (!dest->Type->BoolFlag[CANHARVEST_INDEX].value && only_harvestable) {
		return false;
	}
	
	if (dest->GivesResource == TradeCost) {
		if (dest->Player == this->Player) { //can only trade with markets owned by other players
			return false;
		}

		switch (this->Type->get_domain()) {
			case unit_domain::water:
				if (!dest->Type->BoolFlag[SHOREBUILDING_INDEX].value && dest->Type->get_domain() != unit_domain::water) {
					//ships cannot trade with land markets
					return false;
				}

				break;
			default:
				if (dest->Type->BoolFlag[SHOREBUILDING_INDEX].value) {
					//only ships can trade with docks
					return false;
				}

				break;
		}
	} else {
		if (dest->Player != this->Player && !(dest->Player->is_allied_with(*this->Player) && this->Player->is_allied_with(*dest->Player)) && dest->Player->get_index() != PlayerNumNeutral) {
			return false;
		}
	}

	if (dest->Variable[GARRISONED_GATHERING_INDEX].Value > 0) {
		return false;
	}
	
	return true;
}

bool CUnit::can_return_goods_to(const CUnit *dest, const resource *resource) const
{
	if (!dest) {
		return false;
	}
	
	if (resource == nullptr) {
		resource = this->get_current_resource();
	}
	
	if (resource == nullptr) {
		return false;
	}
	
	if (!dest->Type->can_store(resource)) {
		return false;
	}
	
	if (resource->get_index() == TradeCost) {
		if (dest->Player != this->Player) { //can only return trade to markets owned by the same player
			return false;
		}
		
		if (this->Type->get_domain() != unit_domain::water && dest->Type->BoolFlag[SHOREBUILDING_INDEX].value) { //only ships can return trade to docks
			return false;
		}
		if (this->Type->get_domain() == unit_domain::water && !dest->Type->BoolFlag[SHOREBUILDING_INDEX].value && dest->Type->get_domain() != unit_domain::water) { //ships cannot return trade to land markets
			return false;
		}
	} else {
		if (dest->Player != this->Player && !(dest->Player->is_allied_with(*this->Player) && this->Player->is_allied_with(*dest->Player))) {
			return false;
		}
	}
	
	return true;
}

bool CUnit::can_repair() const
{
	return this->Type->can_repair();
}

/**
**	@brief	Get whether a unit can cast a given spell
**
**	@return	True if the unit can cast the given spell, or false otherwise
*/
bool CUnit::CanCastSpell(const wyrmgus::spell *spell, const bool ignore_mana_and_cooldown) const
{
	if (spell->IsAvailableForUnit(*this)) {
		if (!ignore_mana_and_cooldown) {
			if (this->Variable[MANA_INDEX].Value < spell->get_mana_cost()) {
				return false;
			}
			
			if (this->get_spell_cooldown_timer(spell) > 0) {
				return false;
			}
		}
		
		return true;
	} else {
		return false;
	}
}

/**
**	@brief	Get whether a unit can cast any spell
**
**	@return	True if the unit can cast any spell, or false otherwise
*/
bool CUnit::CanCastAnySpell() const
{
	for (size_t i = 0; i < this->Type->Spells.size(); ++i) {
		if (this->CanCastSpell(this->Type->Spells[i], true)) {
			return true;
		}
	}
	
	return false;
}

bool CUnit::is_autocast_spell(const wyrmgus::spell *spell) const
{
	return wyrmgus::vector::contains(this->get_autocast_spells(), spell);
}

void CUnit::add_autocast_spell(const wyrmgus::spell *spell)
{
	this->autocast_spells.push_back(spell);
}

void CUnit::remove_autocast_spell(const wyrmgus::spell *spell)
{
	wyrmgus::vector::remove(this->autocast_spells, spell);
}

/**
**	@brief	Get whether the unit can autocast a given spell
**
**	@param	spell	The spell
**
**	@return	True if the unit can autocast the spell, false otherwise
*/
bool CUnit::CanAutoCastSpell(const wyrmgus::spell *spell) const
{
	if (!spell || !this->is_autocast_spell(spell) || spell->get_autocast_info() == nullptr) {
		return false;
	}
	
	if (!CanCastSpell(spell, false)) {
		return false;
	}
	
	return true;
}

bool CUnit::IsItemEquipped(const CUnit *item) const
{
	const wyrmgus::item_slot item_slot = wyrmgus::get_item_class_slot(item->Type->get_item_class());
	
	if (item_slot == wyrmgus::item_slot::none) {
		return false;
	}
	
	if (wyrmgus::vector::contains(this->EquippedItems[static_cast<int>(item_slot)], item)) {
		return true;
	}
	
	return false;
}

bool CUnit::is_item_class_equipped(const wyrmgus::item_class item_class) const
{
	const wyrmgus::item_slot item_slot = wyrmgus::get_item_class_slot(item_class);
	
	if (item_slot == wyrmgus::item_slot::none) {
		return false;
	}
	
	for (size_t i = 0; i < EquippedItems[static_cast<int>(item_slot)].size(); ++i) {
		if (EquippedItems[static_cast<int>(item_slot)][i]->Type->get_item_class() == item_class) {
			return true;
		}
	}
	
	return false;
}

bool CUnit::is_item_type_equipped(const wyrmgus::unit_type *item_type) const
{
	const wyrmgus::item_slot item_slot = wyrmgus::get_item_class_slot(item_type->get_item_class());
	
	if (item_slot == wyrmgus::item_slot::none) {
		return false;
	}
	
	for (size_t i = 0; i < EquippedItems[static_cast<int>(item_slot)].size(); ++i) {
		if (EquippedItems[static_cast<int>(item_slot)][i]->Type == item_type) {
			return true;
		}
	}
	
	return false;
}

bool CUnit::is_unique_item_equipped(const unique_item *unique) const
{
	const item_slot item_slot = get_item_class_slot(unique->get_unit_type()->get_item_class());
		
	if (item_slot == item_slot::none) {
		return false;
	}
		
	for (const CUnit *equipped_item : this->EquippedItems[static_cast<int>(item_slot)]) {
		if (equipped_item->get_unique() == unique) {
			return true;
		}
	}
	
	return false;
}

bool CUnit::is_equipped() const
{
	if (!this->Type->BoolFlag[ITEM_INDEX].value) {
		return false;
	}

	if (this->Container == nullptr) {
		return false;
	}

	if (!this->Container->HasInventory()) {
		return false;
	}

	if (this->Type->get_item_class() == item_class::none) {
		return false;
	}

	return this->Container->IsItemEquipped(this);
}

bool CUnit::can_equip_item(const CUnit *item) const
{
	if (item->Container != this) {
		return false;
	}
	
	if (!item->Identified) {
		return false;
	}
	
	if (!this->can_equip_item_class(item->Type->get_item_class())) {
		return false;
	}
	
	return true;
}

bool CUnit::can_equip_item_class(const item_class item_class) const
{
	if (item_class == item_class::none) {
		return false;
	}
	
	if (get_item_class_slot(item_class) == item_slot::none) { //can't equip items that don't correspond to an equippable slot
		return false;
	}
	
	if (get_item_class_slot(item_class) == item_slot::weapon && !vector::contains(this->Type->WeaponClasses, item_class)) { //if the item is a weapon and its item class isn't a weapon class used by this unit's type, return false
		return false;
	}
	
	if ( //if the item uses the shield (off-hand) slot, but that slot is unavailable for the weapon (because it is two-handed), return false
		get_item_class_slot(item_class) == item_slot::shield
		&& this->Type->WeaponClasses.size() > 0
		&& (
			this->Type->WeaponClasses[0] == item_class::bow
			// add other two-handed weapons here as necessary
		)
	) {
		return false;
	}
	
	if ( //if the item is a shield and the weapon of this unit's type is incompatible with shields, return false
		item_class == item_class::shield
		&& (
			Type->WeaponClasses.size() == 0
			|| is_shield_incompatible_weapon_item_class(this->Type->WeaponClasses[0])
			|| Type->BoolFlag[HARVESTER_INDEX].value //workers can't use shields
		)
	) {
		return false;
	}
	
	if (this->get_item_slot_quantity(get_item_class_slot(item_class)) == 0) {
		return false;
	}
	
	return true;
}

bool CUnit::CanUseItem(CUnit *item) const
{
	if (item->ConnectingDestination != nullptr) {
		if (item->Type->BoolFlag[ETHEREAL_INDEX].value && !this->Variable[ETHEREALVISION_INDEX].Value) {
			return false;
		}
		
		if (this->Player == item->Player || this->Player->is_allied_with(*item->Player) || item->Player->get_type() == player_type::neutral) {
			return true;
		}
	}
	
	if (!item->Type->BoolFlag[ITEM_INDEX].value && !item->Type->BoolFlag[POWERUP_INDEX].value) {
		return false;
	}
	
	if (item->Type->BoolFlag[ITEM_INDEX].value && !is_consumable_item_class(item->Type->get_item_class())) {
		return false;
	}
	
	if (item->Spell != nullptr) {
		if (!this->HasInventory() || !::CanCastSpell(*this, *item->Spell, this)) {
			return false;
		}
	}
	
	if (item->Work != nullptr) {
		if (!this->HasInventory() || this->GetIndividualUpgrade(item->Work)) {
			return false;
		}
	}
	
	if (item->Elixir != nullptr) {
		if (!this->HasInventory() || this->GetIndividualUpgrade(item->Elixir)) {
			return false;
		}
	}
	
	if (item->Elixir == nullptr) {
		if (item->Variable[HITPOINTHEALING_INDEX].Value > 0 && item->Variable[MANA_RESTORATION_INDEX].Value > 0) {
			if (this->Variable[HP_INDEX].Value >= this->GetModifiedVariable(HP_INDEX, VariableAttribute::Max) && this->Variable[MANA_INDEX].Value >= this->GetModifiedVariable(MANA_INDEX, VariableAttribute::Max)) {
				return false;
			}
		} else if (item->Variable[HITPOINTHEALING_INDEX].Value > 0) {
			if (this->Variable[HP_INDEX].Value >= this->GetModifiedVariable(HP_INDEX, VariableAttribute::Max)) {
				return false;
			}
		} else if (item->Variable[MANA_RESTORATION_INDEX].Value > 0) {
			if (this->Variable[MANA_INDEX].Value >= this->GetModifiedVariable(MANA_INDEX, VariableAttribute::Max)) {
				return false;
			}
		}
	}
	
	return true;
}

bool CUnit::is_item_set_complete(const CUnit *item) const
{
	const unique_item *item_unique = item->get_unique();
	assert_throw(item_unique != nullptr);

	const CUpgrade *item_set = item_unique->get_set();
	assert_throw(item_set != nullptr);

	for (const unique_item *set_unique : item_set->get_set_uniques()) {
		if (!this->is_unique_item_equipped(set_unique)) {
			return false;
		}
	}

	return true;
}

bool CUnit::equipping_item_completes_set(const CUnit *item) const
{
	for (const unique_item *unique_item : item->get_unique()->get_set()->get_set_uniques()) {
		const item_slot item_slot = get_item_class_slot(unique_item->get_unit_type()->get_item_class());
		
		if (item_slot == item_slot::none) {
			return false;
		}
		
		bool has_item_equipped = false;
		for (const CUnit *equipped_item : this->EquippedItems[static_cast<int>(item_slot)]) {
			if (equipped_item->get_unique() == unique_item) {
				has_item_equipped = true;
				break;
			}
		}
		
		if (has_item_equipped && unique_item == item->get_unique()) { //if the unique item is already equipped, it won't complete the set (either the set is already complete, or needs something else)
			return false;
		} else if (!has_item_equipped && unique_item != item->get_unique()) {
			return false;
		}
		
	}

	return true;
}

bool CUnit::DeequippingItemBreaksSet(const CUnit *item) const
{
	if (!this->is_item_set_complete(item)) {
		return false;
	}
	
	const wyrmgus::item_slot item_slot = wyrmgus::get_item_class_slot(item->Type->get_item_class());
		
	if (item_slot == wyrmgus::item_slot::none) {
		return false;
	}
		
	int item_equipped_quantity = 0;
	for (size_t i = 0; i < this->EquippedItems[static_cast<int>(item_slot)].size(); ++i) {
		if (EquippedItems[static_cast<int>(item_slot)][i]->get_unique() == item->get_unique()) {
			item_equipped_quantity += 1;
		}
	}
	
	if (item_equipped_quantity > 1) {
		return false;
	} else {
		return true;
	}
}

bool CUnit::HasInventory() const
{
	if (this->Type->BoolFlag[INVENTORY_INDEX].value) {
		return true;
	}
	
	if (!this->Type->BoolFlag[FAUNA_INDEX].value) {
		if (this->get_character() != nullptr) {
			return true;
		}
		
		if (this->Variable[LEVEL_INDEX].Value >= 3 && this->Type->BoolFlag[ORGANIC_INDEX].value) {
			return true;
		}
	}
	
	return false;
}

template <bool precondition>
bool CUnit::can_learn_ability(const CUpgrade *ability) const
{
	if (ability->get_deity() != nullptr) {
		//if is a deity choice "ability", only allow for custom heroes (but display the icon for already-acquired deities for all heroes)

		if (this->get_character() == nullptr) {
			return false;
		}

		if (!this->get_character()->is_custom() && this->GetIndividualUpgrade(ability) == 0) {
			return false;
		}

		if constexpr (!precondition) {
			if (this->UpgradeRemovesExistingUpgrade(ability)) {
				return false;
			}
		}
	}
	
	if constexpr (!precondition) {
		if (this->GetIndividualUpgrade(ability) >= ability->MaxLimit) { // already learned
			return false;
		}

		if (this->Variable[LEVELUP_INDEX].Value < 1 && ability->is_ability()) {
			return false;
		}
	}
	
	if (!check_conditions<precondition>(ability, this, false)) {
		return false;
	}
	
	return true;
}

template bool CUnit::can_learn_ability<false>(const CUpgrade *ability) const;
template bool CUnit::can_learn_ability<true>(const CUpgrade *ability) const;

bool CUnit::CanEat(const CUnit &unit) const
{
	if (this->Type->BoolFlag[CARNIVORE_INDEX].value && unit.Type->BoolFlag[FLESH_INDEX].value) {
		return true;
	}
	
	if (this->Type->BoolFlag[INSECTIVORE_INDEX].value && unit.Type->BoolFlag[INSECT_INDEX].value) {
		return true;
	}
	
	if (this->Type->BoolFlag[HERBIVORE_INDEX].value && unit.Type->BoolFlag[VEGETABLE_INDEX].value) {
		return true;
	}
	
	if (
		this->Type->BoolFlag[DETRITIVORE_INDEX].value
		&& (
			unit.Type->BoolFlag[DETRITUS_INDEX].value
			|| (unit.CurrentAction() == UnitAction::Die && (unit.Type->BoolFlag[FLESH_INDEX].value || unit.Type->BoolFlag[INSECT_INDEX].value))
		)
	) {
		return true;
	}
		
	return false;
}

int CUnit::level_check_score(const int level) const
{
	if (this->Variable[LEVEL_INDEX].Value == 0) {
		return 0;
	}

	return SyncRand((this->Variable[LEVEL_INDEX].Value * 2) + 1) - level;
}

bool CUnit::LevelCheck(const int level) const
{
	if (this->Variable[LEVEL_INDEX].Value == 0) {
		return false;
	}
	
	return this->level_check_score(level) >= 0;
}

bool CUnit::is_spell_empowered(const wyrmgus::spell *spell) const
{
	const wyrmgus::world *world = this->MapLayer->world;

	if (world != nullptr) {
		if (!world->EmpoweredMagicDomains.empty()) {
			for (const magic_domain *magic_domain : spell->get_magic_domains()) {
				if (vector::contains(world->EmpoweredMagicDomains, magic_domain)) {
					return true;
				}
			}
		}
	}

	return false;
}

/**
**  Check if the upgrade removes an existing individual upgrade of the unit.
**
**  @param upgrade    Upgrade.
*/
bool CUnit::UpgradeRemovesExistingUpgrade(const CUpgrade *upgrade) const
{
	for (const auto &modifier : upgrade->get_modifiers()) {
		for (const CUpgrade *removed_upgrade : modifier->get_removed_upgrades()) {
			if (this->GetIndividualUpgrade(removed_upgrade) > 0) {
				return true;
			}
		}
	}
	
	return false;
}

bool CUnit::HasAdjacentRailForUnitType(const wyrmgus::unit_type *type) const
{
	bool has_adjacent_rail = false;
	Vec2i top_left_pos(this->tilePos - Vec2i(1, 1));
	Vec2i bottom_right_pos(this->tilePos + this->Type->get_tile_size());
			
	for (int x = top_left_pos.x; x <= bottom_right_pos.x; ++x) {
		Vec2i tile_pos(x, top_left_pos.y);
		if (CMap::get()->Info->IsPointOnMap(tile_pos, this->MapLayer) && UnitTypeCanBeAt(*type, tile_pos, this->MapLayer->ID)) {
			has_adjacent_rail = true;
			break;
		}
				
		tile_pos.y = bottom_right_pos.y;
		if (CMap::get()->Info->IsPointOnMap(tile_pos, this->MapLayer) && UnitTypeCanBeAt(*type, tile_pos, this->MapLayer->ID)) {
			has_adjacent_rail = true;
			break;
		}
	}
			
	if (!has_adjacent_rail) {
		for (int y = top_left_pos.y; y <= bottom_right_pos.y; ++y) {
			Vec2i tile_pos(top_left_pos.x, y);
			if (CMap::get()->Info->IsPointOnMap(tile_pos, this->MapLayer) && UnitTypeCanBeAt(*type, tile_pos, this->MapLayer->ID)) {
				has_adjacent_rail = true;
				break;
			}
					
			tile_pos.x = bottom_right_pos.x;
			if (CMap::get()->Info->IsPointOnMap(tile_pos, this->MapLayer) && UnitTypeCanBeAt(*type, tile_pos, this->MapLayer->ID)) {
				has_adjacent_rail = true;
				break;
			}
		}
	}
			
	return has_adjacent_rail;
}

const wyrmgus::animation_set *CUnit::get_animation_set() const
{
	const wyrmgus::unit_type_variation *variation = this->GetVariation();
	if (variation != nullptr && variation->get_animation_set() != nullptr) {
		return variation->get_animation_set();
	} else {
		return this->Type->get_animation_set();
	}
}

const wyrmgus::construction *CUnit::get_construction() const
{
	const unit_type_variation *variation = this->GetVariation();
	if (variation != nullptr && variation->get_construction() != nullptr) {
		return variation->get_construction();
	}

	if (this->Type->get_on_top_construction() != nullptr) {
		const on_top_build_restriction *on_top = OnTopDetails(*this->Type, nullptr);
		if (on_top != nullptr) {
			return this->Type->get_on_top_construction();
		}
	}

	return this->Type->get_construction();
}

const icon *CUnit::get_icon() const
{
	if (this->get_character() != nullptr && this->get_character()->get_level() >= 3 && this->get_character()->get_heroic_icon() != nullptr) {
		return this->get_character()->get_heroic_icon();
	} else if (this->get_character() != nullptr && this->get_character()->get_base_icon() != nullptr) {
		return this->get_character()->get_base_icon();
	} else if (this->get_unique() != nullptr && this->get_unique()->get_icon() != nullptr) {
		return this->get_unique()->get_icon();
	}
	
	const unit_type_variation *variation = this->GetVariation();
	if (variation && variation->Icon.Icon) {
		return variation->Icon.Icon;
	} else {
		return this->Type->get_icon();
	}
}

const wyrmgus::icon *CUnit::GetButtonIcon(const ButtonCmd button_action) const
{
	auto find_iterator = this->ButtonIcons.find(button_action);
	if (find_iterator != this->ButtonIcons.end()) {
		return find_iterator->second;
	}

	if (this->Player == CPlayer::GetThisPlayer() && CPlayer::GetThisPlayer()->get_faction() != nullptr) {
		find_iterator = CPlayer::GetThisPlayer()->get_faction()->ButtonIcons.find(button_action);
		if (find_iterator != CPlayer::GetThisPlayer()->get_faction()->ButtonIcons.end()) {
			return find_iterator->second;
		}
	}

	if (this->Player == CPlayer::GetThisPlayer() && PlayerRaces.ButtonIcons[CPlayer::GetThisPlayer()->Race].find(button_action) != PlayerRaces.ButtonIcons[CPlayer::GetThisPlayer()->Race].end()) {
		return PlayerRaces.ButtonIcons[CPlayer::GetThisPlayer()->Race][button_action].Icon;
	}
	
	return nullptr;
}

MissileConfig CUnit::GetMissile() const
{
	if (this->Variable[FIREDAMAGE_INDEX].Value > 0 && this->Type->FireMissile.Missile) {
		return this->Type->FireMissile;
	} else {
		return this->Type->Missile;
	}
}

const std::shared_ptr<CPlayerColorGraphic> &CUnit::GetLayerSprite(const int image_layer) const
{
	const wyrmgus::unit_type_variation *layer_variation = this->GetLayerVariation(image_layer);
	if (layer_variation && layer_variation->Sprite) {
		return layer_variation->Sprite;
	}
	
	const wyrmgus::unit_type_variation *variation = this->GetVariation();
	if (variation && variation->LayerSprites[image_layer]) {
		return variation->LayerSprites[image_layer];
	} else if (this->Type->LayerSprites[image_layer])  {
		return this->Type->LayerSprites[image_layer];
	} else {
		static std::shared_ptr<CPlayerColorGraphic> null_graphic;
		return null_graphic;
	}
}

const std::string &CUnit::get_simple_name() const
{
	if (GameRunning && this->get_character() != nullptr && this->get_character()->get_deity() != nullptr) {
		if (CPlayer::GetThisPlayer()->get_civilization() != nullptr) {
			const std::string &cultural_name = this->get_character()->get_deity()->get_cultural_name(CPlayer::GetThisPlayer()->get_civilization());
			
			if (!cultural_name.empty()) {
				return cultural_name;
			}
		}
		
		return this->get_character()->get_deity()->get_name();
	}

	if (this->site != nullptr && this->get_unique() == nullptr && this->site->can_use_name_for_site_unit()) {
		//if this unit represents a site which can use its name for the site unit, then use it
		return this->site->get_game_data()->get_current_cultural_name();
	}
	
	return this->Name;
}

std::string CUnit::get_full_name() const
{
	const std::string &name = this->get_simple_name();
	std::string full_name = name;

	if (full_name.empty() && !this->get_surname().empty()) {
		full_name = this->get_surname();
	}

	if (this->get_epithet() != nullptr) {
		if (full_name.empty()) {
			full_name += string::capitalized(this->get_epithet()->get_name());
		} else {
			full_name += " " + this->get_epithet()->get_name();
		}
	} else if (!this->get_surname().empty()) {
		if (!name.empty()) {
			full_name += " " + this->get_surname();
		}
	}

	return full_name;
}

const std::string &CUnit::get_base_type_name() const
{
	const wyrmgus::unit_type_variation *variation = this->GetVariation();
	if (variation != nullptr && !variation->get_type_name().empty()) {
		return variation->get_type_name();
	} else {
		return this->Type->get_name();
	}
}

std::string CUnit::get_type_name() const
{
	const wyrmgus::civilization *civilization = this->get_civilization();
	if (civilization != nullptr) {
		if (civilization != this->Player->get_civilization() && civilization->get_name() != this->get_base_type_name() && this->Type->get_species() == nullptr) {
			return civilization->get_name() + " " + this->get_base_type_name();
		}
	} else {
		const civilization_group *civilization_group = this->Type->get_civilization_group();
		if (civilization_group != nullptr && (this->Player->get_civilization() == nullptr || !this->Player->get_civilization()->is_part_of_group(civilization_group)) && civilization_group->get_name() != this->get_base_type_name() && this->Type->get_species() == nullptr) {
			return civilization_group->get_name() + " " + this->get_base_type_name();
		}
	}

	return this->get_base_type_name();
}

std::string CUnit::GetMessageName() const
{
	std::string name = this->get_full_name();

	if (name.empty()) {
		return this->get_type_name();
	}
	
	if (!this->Identified) {
		return this->get_type_name() + " (" + _("Unidentified") + ")";
	}
	
	if (this->get_unique() == nullptr && this->Work == nullptr && (this->Prefix != nullptr || this->Suffix != nullptr || this->Spell != nullptr)) {
		return name;
	}
	
	return name + " (" + this->get_type_name() + ")";
}
//Wyrmgus end

const wyrmgus::time_of_day *CUnit::get_center_tile_time_of_day() const
{
	if (this->MapLayer == nullptr) {
		return nullptr;
	}

	//get the time of day for the unit's tile
	return this->MapLayer->get_tile_time_of_day(this->get_center_tile_pos());
}

const wyrmgus::site *CUnit::get_center_tile_settlement() const
{
	if (this->MapLayer == nullptr) {
		return nullptr;
	}

	//get the settlement for the unit's tile
	return this->get_center_tile()->get_settlement();
}

const CPlayer *CUnit::get_center_tile_owner() const
{
	if (this->MapLayer == nullptr) {
		return nullptr;
	}

	//get the owner for the unit's tile
	return this->get_center_tile()->get_owner();
}

const landmass *CUnit::get_center_tile_landmass() const
{
	if (this->MapLayer == nullptr) {
		return nullptr;
	}

	//get the landmass for the unit's center tile
	return this->get_center_tile()->get_landmass();
}

const world *CUnit::get_center_tile_world() const
{
	if (this->MapLayer == nullptr) {
		return nullptr;
	}

	//get the world for the unit's center tile
	return this->get_center_tile()->get_world();
}

bool CUnit::is_seen_by_player(const CPlayer *player) const
{
	return this->is_seen_by_player(player->get_index());
}

bool CUnit::is_seen_destroyed_by_player(const CPlayer *player) const
{
	return this->is_seen_destroyed_by_player(player->get_index());
}

bool CUnit::is_in_tile_rect(const QRect &tile_rect, const int z) const
{
	const CUnit *first_container = this->GetFirstContainer();
	const CMapLayer *map_layer = first_container->MapLayer;

	if (z != map_layer->ID) {
		return false;
	}

	for (int x = 0; x < first_container->Type->get_tile_width(); ++x) {
		for (int y = 0; y < first_container->Type->get_tile_height(); ++y) {
			if (tile_rect.contains(first_container->tilePos + QPoint(x, y))) {
				return true;
			}
		}
	}

	return false;
}

bool CUnit::is_in_subtemplate_area(const wyrmgus::map_template *subtemplate) const
{
	const CUnit *first_container = this->GetFirstContainer();
	const CMapLayer *map_layer = first_container->MapLayer;
	const int z = map_layer->ID;

	for (int x = 0; x < first_container->Type->get_tile_width(); ++x) {
		for (int y = 0; y < first_container->Type->get_tile_height(); ++y) {
			if (CMap::get()->is_point_in_subtemplate_area(first_container->tilePos + QPoint(x, y), z, subtemplate)) {
				return true;
			}
		}
	}

	return false;
}

archimedes::gender CUnit::get_gender() const
{
	return static_cast<archimedes::gender>(this->Variable[GENDER_INDEX].Value);
}

bool CUnit::is_capturable() const
{
	return this->Variable[CAPTURE_HP_THRESHOLD_INDEX].Value != 0;
}

bool CUnit::is_near_site(const wyrmgus::site *site) const
{
	if (this->MapLayer == nullptr) {
		return false;
	}

	const site_game_data *site_data = site->get_game_data();

	if (!site_data->is_on_map()) {
		return false;
	}

	const CUnit *site_unit = site_data->get_site_unit();

	if (site_unit != nullptr) {
		if (site_unit->MapLayer == nullptr || this->MapLayer != site_unit->MapLayer) {
			return false;
		}

		return this->MapDistanceTo(*site_unit) <= 1;
	} else {
		if (site_data->get_map_layer() == nullptr || this->MapLayer != site_data->get_map_layer()) {
			return false;
		}

		return this->MapDistanceTo(site_data->get_map_pos(), site_data->get_map_layer()->ID) <= 1;
	}
}

bool CUnit::counts_for_military_score() const
{
	if (!this->IsAlive()) {
		return false;
	}

	if (!this->CanMove()) {
		return false;
	}

	if (this->Type->BoolFlag[HARVESTER_INDEX].value) {
		return false;
	}

	if (!this->CanAttack()) {
		return false;
	}

	return true;
}

/**
**  Let an unit die.
**
**  @param unit    Unit to be destroyed.
*/
void LetUnitDie(CUnit &unit, bool suicide)
{
	unit.Variable[HP_INDEX].Value = std::min<int>(0, unit.Variable[HP_INDEX].Value);
	unit.Moving = 0;
	unit.TTL = 0;
	unit.Anim.Unbreakable = false;

	const wyrmgus::unit_type *type = unit.Type;

	unit.Resource.Workers.clear();

	// removed units, just remove.
	if (unit.Removed) {
		DebugPrint("Killing a removed unit?\n");
		if (unit.has_units_inside()) {
			DestroyAllInside(unit);
		}
		UnitLost(unit);
		unit.clear_orders();
		unit.Release();
		return;
	}

	PlayUnitSound(&unit, unit_sound_type::dying);

	//
	// Catapults,... explodes.
	//
	if (type->ExplodeWhenKilled) {
		const PixelPos pixelPos = unit.get_map_pixel_pos_center();

		MakeMissile(*type->Explosion.Missile, pixelPos, pixelPos, unit.MapLayer->ID);
	}
	if (type->DeathExplosion) {
		const PixelPos pixelPos = unit.get_map_pixel_pos_center();

		type->DeathExplosion->pushPreamble();
		//Wyrmgus start
		type->DeathExplosion->pushInteger(UnitNumber(unit));
		//Wyrmgus end
		type->DeathExplosion->pushInteger(pixelPos.x);
		type->DeathExplosion->pushInteger(pixelPos.y);
		type->DeathExplosion->run();
	}
	if (suicide) {
		const PixelPos pixelPos = unit.get_map_pixel_pos_center();
		
		if (unit.GetMissile().Missile) {
			MakeMissile(*unit.GetMissile().Missile, pixelPos, pixelPos, unit.MapLayer->ID);
		}
	}
	// Handle Teleporter Destination Removal
	if (type->BoolFlag[TELEPORTER_INDEX].value && unit.Goal) {
		unit.Goal->Remove(nullptr);
		UnitLost(*unit.Goal);
		unit.Goal->clear_orders();
		unit.Goal->Release();
		unit.Goal = nullptr;
	}
	
	//Wyrmgus start
	for (size_t i = 0; i < unit.SoldUnits.size(); ++i) {
		DestroyAllInside(*unit.SoldUnits[i]);
		LetUnitDie(*unit.SoldUnits[i]);
	}
	unit.SoldUnits.clear();
	//Wyrmgus end

	// Transporters lose or save their units and buildings their workers
	if (unit.has_units_inside()) {
		if (unit.Type->BoolFlag[SAVECARGO_INDEX].value || (unit.HasInventory() && unit.get_character() == nullptr)) {
			DropOutAll(unit);
		} else {
			DestroyAllInside(unit);
		}
	}

	//Wyrmgus start
	//drop items upon death
	if (!suicide && unit.CurrentAction() != UnitAction::Built && (unit.get_character() != nullptr || unit.Type->BoolFlag[BUILDING_INDEX].value || SyncRand(100) >= 66)) { //66% chance nothing will be dropped, unless the unit is a character or building, in which it case it will always drop an item
		unit.GenerateDrop();
	}
	//Wyrmgus end
	
	unit.Remove(nullptr);
	UnitLost(unit);
	unit.restore_ontop();
	unit.clear_orders();

	// Unit has death animation.

	// Not good: UnitUpdateHeading(unit);
	unit.Orders[0] = COrder::NewActionDie();
	if (type->get_corpse_type() != nullptr) {
#ifdef DYNAMIC_LOAD
		if (!type->Sprite) {
			LoadUnitTypeSprite(type);
		}
#endif
		unit.pixel_offset.setX((type->get_corpse_type()->get_frame_width() - type->get_corpse_type()->Sprite->get_original_frame_size().width()) / 2);
		unit.pixel_offset.setY((type->get_corpse_type()->get_frame_height() - type->get_corpse_type()->Sprite->get_original_frame_size().height()) / 2);

		unit.CurrentSightRange = type->get_corpse_type()->Stats[unit.Player->get_index()].Variables[SIGHTRANGE_INDEX].Max;
	} else {
		unit.CurrentSightRange = 0;
	}

	// If we have a corpse, or a death animation, we are put back on the map
	// This enables us to be tracked.  Possibly for spells (eg raise dead)
	if (type->get_corpse_type() != nullptr || (unit.get_animation_set() && unit.get_animation_set()->Death)) {
		unit.Removed = 0;
		CMap::get()->Insert(unit);

		// FIXME: rb: Maybe we need this here because corpse of cloaked units
		//	may crash Sign code

		// Recalculate the seen count.
		//UnitCountSeen(unit);
	}
	
	MapMarkUnitSight(unit);
	
	//Wyrmgus start
	if (unit.get_settlement() != nullptr && type->BoolFlag[TOWNHALL_INDEX].value) {
		unit.UpdateBuildingSettlementAssignment(unit.get_settlement());
	}
	//Wyrmgus end
}

/**
**  Destroy all units inside unit.
**
**  @param source  container.
*/
void DestroyAllInside(CUnit &source)
{
	//copy the vector since we may modify it
	const std::vector<CUnit *> units_inside = source.get_units_inside();

	// No Corpses, we are inside something, and we can't be seen
	for (CUnit *unit : units_inside) {
		// Transporter inside a transporter?
		if (unit->has_units_inside()) {
			DestroyAllInside(*unit);
		}
		UnitLost(*unit);
		unit->clear_orders();
		unit->Release();
	}
}

/*----------------------------------------------------------------------------
  -- Unit AI
  ----------------------------------------------------------------------------*/

int ThreatCalculate(const CUnit &unit, const CUnit &dest)
{
	const wyrmgus::unit_type &type = *unit.Type;
	const wyrmgus::unit_type &dtype = *dest.Type;
	int cost = 0;

	// Buildings, non-aggressive and invincible units have the lowest priority
	if (dest.IsAgressive() == false || dest.has_status_effect(status_effect::unholy_armor)
		|| dest.Type->BoolFlag[INDESTRUCTIBLE_INDEX].value) {
		if (dest.Type->CanMove() == false) {
			return INT_MAX;
		} else {
			return INT_MAX / 2;
		}
	}

	// Priority 0-255
	cost -= dest.Variable[PRIORITY_INDEX].Value * PRIORITY_FACTOR;
	// Remaining HP (Health) 0-65535
	//Wyrmgus start
//	cost += dest.Variable[HP_INDEX].Value * 100 / dest.Variable[HP_INDEX].Max * HEALTH_FACTOR;
	cost += dest.Variable[HP_INDEX].Value * 100 / dest.GetModifiedVariable(HP_INDEX, VariableAttribute::Max) * HEALTH_FACTOR;
	//Wyrmgus end

	const int d = unit.MapDistanceTo(dest);

	if (d <= unit.get_best_attack_range() && d >= type.MinAttackRange) {
		cost += d * INRANGE_FACTOR;
		cost -= INRANGE_BONUS;
	} else {
		cost += d * DISTANCE_FACTOR;
	}

	for (unsigned int i = 0; i < UnitTypeVar.GetNumberBoolFlag(); i++) {
		if (type.BoolFlag[i].AiPriorityTarget != CONDITION_TRUE) {
			if ((type.BoolFlag[i].AiPriorityTarget == CONDITION_ONLY) & (dtype.BoolFlag[i].value)) {
				cost -= AIPRIORITY_BONUS;
			}
			if ((type.BoolFlag[i].AiPriorityTarget == CONDITION_FALSE) & (dtype.BoolFlag[i].value)) {
				cost += AIPRIORITY_BONUS;
			}
		}
	}

	// Unit can attack back.
	if (dtype.can_target(&unit)) {
		cost -= CANATTACK_BONUS;
	}

	return cost;
}

static void HitUnit_LastAttack(const CUnit *attacker, CUnit &target)
{
	const unsigned long last_attack_cycle = target.Attacked;

	target.Attacked = GameCycle ? GameCycle : 1;
	if (target.Type->BoolFlag[WALL_INDEX].value || (last_attack_cycle && GameCycle <= (last_attack_cycle + 2 * CYCLES_PER_SECOND))) {
		return;
	}
	// NOTE: perhaps this should also be moved into the notify?
	if (target.Player == CPlayer::GetThisPlayer()) {
		// FIXME: Problem with load+save.

		//
		// One help cry each 2 second is enough
		// If on same area ignore it for 2 minutes.
		//
		if (HelpMeLastCycle < GameCycle) {
			if (!HelpMeLastCycle
				|| HelpMeLastCycle + CYCLES_PER_SECOND * 120 < GameCycle
				|| target.tilePos.x < HelpMeLastX - 14
				|| target.tilePos.x > HelpMeLastX + 14
				|| target.tilePos.y < HelpMeLastY - 14
				|| target.tilePos.y > HelpMeLastY + 14) {
				HelpMeLastCycle = GameCycle + CYCLES_PER_SECOND * 2;
				HelpMeLastX = target.tilePos.x;
				HelpMeLastY = target.tilePos.y;
				PlayUnitSound(&target, wyrmgus::unit_sound_type::help);
				target.Player->Notify(notification_type::red, target.tilePos, target.MapLayer->ID, _("%s attacked"), target.GetMessageName().c_str());
			}
		}
	}

	//only trigger this every two minutes for the unit
	if (attacker && (last_attack_cycle == 0 || GameCycle > (last_attack_cycle + 2 * (CYCLES_PER_SECOND * 60)))) {
		if (
			target.Player->AiEnabled
			&& !attacker->Type->BoolFlag[INDESTRUCTIBLE_INDEX].value // don't attack indestructible units back
		) {
			AiHelpMe(attacker->GetFirstContainer(), target);
		}
	}
}

static void HitUnit_Raid(CUnit *attacker, CUnit &target, int damage)
{
	if (!attacker) {
		return;
	}
	
	if (attacker->Player == target.Player || attacker->Player->get_index() == PlayerNumNeutral || target.Player->get_index() == PlayerNumNeutral) {
		return;
	}
	
	int var_index;
	if (target.Type->BoolFlag[BUILDING_INDEX].value) {
		var_index = RAIDING_INDEX;
	} else {
		var_index = MUGGING_INDEX;
	}
	
	if (attacker->Variable[var_index].Value == 0) {
		return;
	}
	
	if (!attacker->Variable[SHIELDPIERCING_INDEX].Value) {
		const int shieldDamage = target.Variable[SHIELDPERMEABILITY_INDEX].Value < 100
							? std::min(target.Variable[SHIELD_INDEX].Value, damage * (100 - target.Variable[SHIELDPERMEABILITY_INDEX].Value) / 100)
							: 0;
		
		damage -= shieldDamage;
	}
	
	damage = std::min(damage, target.Variable[HP_INDEX].Value);
	
	if (damage <= 0) {
		return;
	}
	
	for (const auto &[resource, cost] : target.Type->Stats[target.Player->get_index()].get_costs()) {
		if (cost < 0) {
			continue;
		}

		const int resource_change = cost * damage * attacker->Variable[var_index].Value / target.GetModifiedVariable(HP_INDEX, VariableAttribute::Max) / 100;
		attacker->Player->change_resource(resource, resource_change, false);
		attacker->Player->change_resource_total(resource, resource_change);
	}
}

static bool HitUnit_IsUnitWillDie(const CUnit *attacker, const CUnit &target, int damage)
{
	const int shieldDamage = target.Variable[SHIELDPERMEABILITY_INDEX].Value < 100
					   ? std::min(target.Variable[SHIELD_INDEX].Value, damage * (100 - target.Variable[SHIELDPERMEABILITY_INDEX].Value) / 100)
					   : 0;
	return (target.Variable[HP_INDEX].Value <= damage && attacker && attacker->Variable[SHIELDPIERCING_INDEX].Value)
		   || (target.Variable[HP_INDEX].Value <= damage - shieldDamage)
		   || (target.Variable[HP_INDEX].Value == 0);
}

void HitUnit_IncreaseScoreForKill(CUnit &attacker, CUnit &target, const bool include_contained_units)
{
	attacker.Player->change_score(target.Variable[POINTS_INDEX].Value);

	if (target.Type->BoolFlag[BUILDING_INDEX].value) {
		attacker.Player->TotalRazings++;
	} else {
		attacker.Player->TotalKills++;
	}
	
	attacker.Player->UnitTypeKills[target.Type->Slot]++;
	
	//distribute experience between nearby units belonging to the same player
	if (!target.Type->BoolFlag[BUILDING_INDEX].value) {
		attacker.change_experience(UseHPForXp ? target.Variable[HP_INDEX].Value : target.Variable[POINTS_INDEX].Value, ExperienceRange);
	}
	
	attacker.Variable[KILL_INDEX].Value++;
	attacker.Variable[KILL_INDEX].Max++;
	attacker.Variable[KILL_INDEX].Enable = 1;
	
	attacker.Player->on_unit_destroyed(&target);

	//also increase score for units inside the target that will be destroyed when the target dies
	if (target.has_units_inside() && include_contained_units) {
		for (CUnit *boarded_unit : target.get_units_inside()) {
			if (!boarded_unit->Type->BoolFlag[ITEM_INDEX].value) { //ignore items
				HitUnit_IncreaseScoreForKill(attacker, *boarded_unit, include_contained_units);
			}
		}
	}
}

static void HitUnit_ApplyDamage(CUnit *attacker, CUnit &target, int damage)
{
	if (attacker && attacker->Variable[SHIELDPIERCING_INDEX].Value) {
		target.Variable[HP_INDEX].Value -= damage;
	} else {
		const int shieldDamage = target.Variable[SHIELDPERMEABILITY_INDEX].Value < 100
						   ? std::min(target.Variable[SHIELD_INDEX].Value, damage * (100 - target.Variable[SHIELDPERMEABILITY_INDEX].Value) / 100)
						   : 0;
		if (shieldDamage) {
			target.Variable[SHIELD_INDEX].Value -= shieldDamage;
			target.Variable[SHIELD_INDEX].Value = std::clamp(target.Variable[SHIELD_INDEX].Value, 0, target.Variable[SHIELD_INDEX].Max);
		}
		target.Variable[HP_INDEX].Value -= damage - shieldDamage;
	}
	
	//Wyrmgus start
	//distribute experience between nearby units belonging to the same player

//	if (UseHPForXp && attacker && target.is_enemy_of(*attacker)) {
	if (UseHPForXp && attacker && (target.is_enemy_of(*attacker) || target.Player->get_type() == player_type::neutral) && !target.Type->BoolFlag[BUILDING_INDEX].value) {
		attacker->change_experience(damage, ExperienceRange);
	}
	//Wyrmgus end
	
	//Wyrmgus start
	//use a healing item if any are available
	if (target.HasInventory()) {
		target.auto_use_item();
	}
	//Wyrmgus end
}

static void HitUnit_BuildingCapture(CUnit *attacker, CUnit &target, const int damage)
{
	Q_UNUSED(damage);

	//capture enemy buildings

	if (!target.is_capturable()) {
		return;
	}

	if (target.Variable[HP_INDEX].get_percent_value() >= target.Variable[CAPTURE_HP_THRESHOLD_INDEX].Value) {
		return;
	}

	if (attacker == nullptr || !attacker->is_enemy_of(target)) {
		return;
	}

	attacker->Player->capture_unit(&target);
}

static void HitUnit_ShowDamageMissile(const CUnit &target, const int damage)
{
	const PixelPos targetPixelCenter = target.get_map_pixel_pos_center();

	if ((target.IsVisibleOnMap(*CPlayer::GetThisPlayer()) || ReplayRevealMap) && !DamageMissile.empty()) {
		const missile_type *mtype = missile_type::get(DamageMissile);
		const QPoint pixel_offset(3, -mtype->get_range());

		MakeLocalMissile(*mtype, targetPixelCenter, targetPixelCenter + pixel_offset, target.MapLayer->ID)->Damage = -damage;
	}
}

static void HitUnit_ShowImpactMissile(const CUnit &target)
{
	const PixelPos targetPixelCenter = target.get_map_pixel_pos_center();
	const wyrmgus::unit_type &type = *target.Type;

	if (target.Variable[SHIELD_INDEX].Value > 0
		&& !type.Impact[ANIMATIONS_DEATHTYPES + 1].Name.empty()) { // shield impact
		MakeMissile(*type.Impact[ANIMATIONS_DEATHTYPES + 1].Missile, targetPixelCenter, targetPixelCenter, target.MapLayer->ID);
	} else if (target.DamagedType && !type.Impact[target.DamagedType].Name.empty()) { // specific to damage type impact
		MakeMissile(*type.Impact[target.DamagedType].Missile, targetPixelCenter, targetPixelCenter, target.MapLayer->ID);
	} else if (!type.Impact[ANIMATIONS_DEATHTYPES].Name.empty()) { // generic impact
		MakeMissile(*type.Impact[ANIMATIONS_DEATHTYPES].Missile, targetPixelCenter, targetPixelCenter, target.MapLayer->ID);
	}
}

static void HitUnit_ChangeVariable(CUnit &target, const Missile &missile)
{
	const int var = missile.Type->ChangeVariable;

	const int old_value = target.Variable[var].Value;

	target.Variable[var].Enable = 1;
	target.Variable[var].Value += missile.Type->ChangeAmount;
	if (target.Variable[var].Value > target.Variable[var].Max) {
		if (missile.Type->ChangeMax) {
			target.Variable[var].Max = target.Variable[var].Value;
		//Wyrmgus start
//		} else {
		} else if (target.Variable[var].Value > target.GetModifiedVariable(var, VariableAttribute::Max)) {
		//Wyrmgus end
			//Wyrmgus start
//			target.Variable[var].Value = target.Variable[var].Max;
			target.Variable[var].Value = target.GetModifiedVariable(var, VariableAttribute::Max);
			//Wyrmgus end
		}
	}
	
	target.on_variable_changed(var, target.Variable[var].Value - old_value);
}

static void HitUnit_Burning(CUnit &target)
{
	//Wyrmgus start
//	const int f = (100 * target.Variable[HP_INDEX].Value) / target.Variable[HP_INDEX].Max;
	const int f = (100 * target.Variable[HP_INDEX].Value) / target.GetModifiedVariable(HP_INDEX, VariableAttribute::Max);
	//Wyrmgus end
	const wyrmgus::missile_type *fire = MissileBurningBuilding(f);

	if (fire) {
		const PixelPos targetPixelCenter = target.get_map_pixel_pos_center();
		const PixelDiff offset(0, -wyrmgus::defines::get()->get_tile_height());
		Missile *missile = MakeMissile(*fire, targetPixelCenter + offset, targetPixelCenter + offset, target.MapLayer->ID);

		missile->SourceUnit = target.acquire_ref();
		target.Burning = 1;
	}
}

//Wyrmgus start
void HitUnit_NormalHitSpecialDamageEffects(CUnit &attacker, CUnit &target)
{
	if (attacker.Variable[FIREDAMAGE_INDEX].Value > 0 && attacker.Variable[FIREDAMAGE_INDEX].Value >= attacker.Variable[COLDDAMAGE_INDEX].Value) { // apply only either the fire damage or cold damage effects, but not both at the same time; apply the one with greater value, but if both are equal fire damage takes precedence
		HitUnit_SpecialDamageEffect(target, FIREDAMAGE_INDEX);
	} else if (attacker.Variable[COLDDAMAGE_INDEX].Value > 0) {
		HitUnit_SpecialDamageEffect(target, COLDDAMAGE_INDEX);
	}
	
	if (attacker.Variable[LIGHTNINGDAMAGE_INDEX].Value > 0) {
		HitUnit_SpecialDamageEffect(target, LIGHTNINGDAMAGE_INDEX);
	}
}

void HitUnit_SpecialDamageEffect(CUnit &target, int dmg_var)
{
	if (dmg_var == COLDDAMAGE_INDEX && target.Variable[COLDRESISTANCE_INDEX].Value < 100 && target.Type->BoolFlag[ORGANIC_INDEX].value) {
		//if resistance to cold is 100%, the effect has no chance of being applied
		const int rand_max = 100 * 100 / (100 - target.Variable[COLDRESISTANCE_INDEX].Value);
		if (SyncRand(rand_max) == 0) {
			target.apply_status_effect(status_effect::slow, 200);
		}
	} else if (dmg_var == LIGHTNINGDAMAGE_INDEX && target.Variable[LIGHTNINGRESISTANCE_INDEX].Value < 100 && target.Type->BoolFlag[ORGANIC_INDEX].value) {
		int rand_max = 100 * 100 / (100 - target.Variable[LIGHTNINGRESISTANCE_INDEX].Value);
		if (SyncRand(rand_max) == 0) {
			target.apply_status_effect(status_effect::stun, 50);
		}
	}
}
//Wyrmgus end

//Wyrmgus start
//static void HitUnit_RunAway(CUnit &target, const CUnit &attacker)
void HitUnit_RunAway(CUnit &target, const CUnit &attacker)
//Wyrmgus end
{
	QPoint pos = target.tilePos - attacker.tilePos;
	int d = number::sqrt(pos.x() * pos.x() + pos.y() * pos.y());

	if (!d) {
		d = 1;
	}

	pos.setX(target.tilePos.x + (pos.x() * 5) / d + SyncRand(4));
	pos.setY(target.tilePos.y + (pos.y() * 5) / d + SyncRand(4));

	CMap::get()->clamp(pos, target.MapLayer->ID);

	CommandStopUnit(target);
	CommandMove(target, pos, 0, target.MapLayer->ID);
}

static void HitUnit_AttackBack(CUnit &attacker, CUnit &target)
{
	static constexpr int threshold = 30;

	std::unique_ptr<COrder> saved_order;

	if (target.Player == CPlayer::GetThisPlayer() && target.Player->get_type() != player_type::neutral) { // allow neutral units to strike back
		if (target.CurrentAction() == UnitAction::Attack) {
			COrder_Attack &order = dynamic_cast<COrder_Attack &>(*target.CurrentOrder());
			if (order.IsWeakTargetSelected() == false) {
				return;
			}
		//Wyrmgus start
//		} else {
		} else if (target.CurrentAction() != UnitAction::Still) {
		//Wyrmgus end
			return;
		}
	}
	if (target.CanStoreOrder(target.CurrentOrder())) {
		saved_order = target.CurrentOrder()->Clone();
	}
	CUnit *oldgoal = target.CurrentOrder()->get_goal();
	CUnit *goal, *best = oldgoal;

	if (RevealAttacker && target.Type->can_target(&attacker)) {
		// Reveal Unit that is attacking
		goal = &attacker;
	} else {
		if (target.CurrentAction() == UnitAction::StandGround) {
			goal = AttackUnitsInRange(target);
		} else {
			// Check for any other units in range
			goal = AttackUnitsInReactRange(target);
		}
	}

	// Calculate the best target we could attack
	if (!best || (goal && (ThreatCalculate(target, *goal) < ThreatCalculate(target, *best)))) {
		best = goal;
	}

	if (target.Type->can_target(&attacker)
		&& (!best || (goal != &attacker
					  && (ThreatCalculate(target, attacker) < ThreatCalculate(target, *best))))) {
		best = &attacker;
	}

	//Wyrmgus start
//	if (best && best != oldgoal && best->Player != target.Player && best->is_allied_with(target) == false) {
	if (best && best != oldgoal && (best->Player != target.Player || target.Player->get_type() == player_type::neutral) && best->is_allied_with(target) == false) {
	//Wyrmgus end
		CommandAttack(target, best->tilePos, best, FlushCommands, best->MapLayer->ID);
		// Set threshold value only for aggressive units
		if (best->IsAgressive()) {
			target.Threshold = threshold;
		}
		if (saved_order != nullptr) {
			target.SavedOrder = std::move(saved_order);
		}
	}
}

/**
**  Unit is hit by missile or other damage.
**
**  @param attacker    Unit that attacks.
**  @param target      Unit that is hit.
**  @param damage      How many damage to take.
**  @param missile     Which missile took the damage.
*/
//Wyrmgus start
//void HitUnit(CUnit *attacker, CUnit &target, int damage, const Missile *missile)
void HitUnit(CUnit *attacker, CUnit &target, int damage, const Missile *missile, bool show_damage)
//Wyrmgus end
{
	const wyrmgus::unit_type *type = target.Type;
	if (!damage) {
		// Can now happen by splash damage
		// Multiple places send x/y as damage, which may be zero
		return;
	}

	if (target.has_status_effect(status_effect::unholy_armor) || target.Type->BoolFlag[INDESTRUCTIBLE_INDEX].value) {
		// vladi: units with active UnholyArmour are invulnerable
		return;
	}
	if (target.Removed) {
		DebugPrint("Removed target hit\n");
		return;
	}

	assert_throw(damage != 0 && target.CurrentAction() != UnitAction::Die && !target.Type->BoolFlag[VANISHES_INDEX].value);

	//Wyrmgus start
	if (
		(attacker != nullptr && attacker->Player == CPlayer::GetThisPlayer())
		&& target.Player != CPlayer::GetThisPlayer()
	) {
		// If player is hitting or being hit add tension to our music
		AddMusicTension(1);
	}
	//Wyrmgus end

	if (GodMode) {
		if (attacker && attacker->Player == CPlayer::GetThisPlayer()) {
			damage = target.Variable[HP_INDEX].Value;
		}
		if (target.Player == CPlayer::GetThisPlayer()) {
			damage = 0;
		}
	}
	//Wyrmgus start
//	HitUnit_LastAttack(attacker, target);
	//Wyrmgus end
	if (attacker != nullptr) {
		//Wyrmgus start
		HitUnit_LastAttack(attacker, target); //only trigger the help me notification and AI code if there is actually an attacker
		//Wyrmgus end
		target.DamagedType = ExtraDeathIndex(attacker->Type->DamageType.c_str());
	}

	// OnHit callback
	if (type->OnHit) {
		const int tarSlot = UnitNumber(target);
		const int atSlot = attacker && attacker->IsAlive() ? UnitNumber(*attacker) : -1;

		type->OnHit->pushPreamble();
		type->OnHit->pushInteger(tarSlot);
		type->OnHit->pushInteger(atSlot);
		type->OnHit->pushInteger(damage);
		type->OnHit->run();
	}

	// Increase variables and call OnImpact
	if (missile && missile->Type) {
		if (missile->Type->ChangeVariable != -1) {
			HitUnit_ChangeVariable(target, *missile);
		}
		if (missile->Type->OnImpact) {
			const int attackerSlot = attacker ? UnitNumber(*attacker) : -1;
			const int targetSlot = UnitNumber(target);
			missile->Type->OnImpact->pushPreamble();
			missile->Type->OnImpact->pushInteger(attackerSlot);
			missile->Type->OnImpact->pushInteger(targetSlot);
			missile->Type->OnImpact->pushInteger(damage);
			missile->Type->OnImpact->run();
		}
	}
	
	HitUnit_Raid(attacker, target, damage);

	if (HitUnit_IsUnitWillDie(attacker, target, damage)) { // unit is killed or destroyed
		if (attacker) {
			//  Setting ai threshold counter to 0 so it can target other units
			attacker->Threshold = 0;
		}
		
		CUnit *destroyer = attacker;
		if (!destroyer) {
			int best_distance = 0;
			std::vector<CUnit *> table;
			SelectAroundUnit(target, ExperienceRange, table, IsEnemyWithUnit(&target));
			for (size_t i = 0; i < table.size(); i++) {
				CUnit *potential_destroyer = table[i];
				int distance = target.MapDistanceTo(*potential_destroyer);
				if (!destroyer || distance < best_distance) {
					destroyer = potential_destroyer;
					best_distance = distance;
				}
			}
		}
		if (destroyer) {
			if (target.is_enemy_of(*destroyer) || target.Player->get_type() == player_type::neutral) {
				HitUnit_IncreaseScoreForKill(*destroyer, target, !target.Type->BoolFlag[SAVECARGO_INDEX].value);
			}
		}
		LetUnitDie(target);
		return;
	}

	HitUnit_ApplyDamage(attacker, target, damage);
	HitUnit_BuildingCapture(attacker, target, damage);
	//Wyrmgus start
//	HitUnit_ShowDamageMissile(target, damage);
	if (show_damage) {
		HitUnit_ShowDamageMissile(target, damage);
	}
	//Wyrmgus end

	HitUnit_ShowImpactMissile(target);

	//Wyrmgus start
//	if (type->BoolFlag[BUILDING_INDEX].value && !target.Burning) {
	if (type->BoolFlag[BUILDING_INDEX].value && !target.Burning && !target.UnderConstruction && target.Type->get_tile_width() != 1 && target.Type->get_tile_height() != 1) { //the building shouldn't burn if it's still under construction, or if it's too small
	//Wyrmgus end
		HitUnit_Burning(target);
	}

	/* Target Reaction on Hit */
	if (target.Player->AiEnabled) {
		if (target.CurrentOrder()->OnAiHitUnit(target, attacker, damage)) {
			return;
		}
	}

	if (!attacker) {
		return;
	}

	// Can't attack run away.
	//Wyrmgus start
//	if (!target.IsAgressive() && target.CanMove() && target.CurrentAction() == UnitAction::Still && !target.BoardCount) {
	if (
		(!target.IsAgressive() || attacker->Type->BoolFlag[INDESTRUCTIBLE_INDEX].value)
		&& target.CanMove()
		&& (target.CurrentAction() == UnitAction::Still || target.has_status_effect(status_effect::terror))
		&& !target.BoardCount
	) {
	//Wyrmgus end
		HitUnit_RunAway(target, *attacker);
	}

	const int threshold = 30;

	if (target.Threshold && target.CurrentOrder()->has_goal() && target.CurrentOrder()->get_goal() == attacker) {
		target.Threshold = threshold;
		return;
	}

	//Wyrmgus start
//	if (target.Threshold == 0 && target.IsAgressive() && target.CanMove() && !target.ReCast) {
	if (
		target.Threshold == 0
		&& (target.IsAgressive() || (target.CanAttack() && target.Type->BoolFlag[COWARD_INDEX].value && (attacker->Type->BoolFlag[COWARD_INDEX].value || attacker->Variable[HP_INDEX].Value <= 3))) // attacks back if isn't coward, or if attacker is also coward, or if attacker has 3 HP or less 
		&& target.CanMove()
		&& !target.ReCast
		&& !attacker->Type->BoolFlag[INDESTRUCTIBLE_INDEX].value // don't attack indestructible units back
	) {
	//Wyrmgus end
		// Attack units in range (which or the attacker?)
		// Don't bother unit if it casting repeatable spell
		HitUnit_AttackBack(*attacker->GetFirstContainer(), target); //if the unit is in a container, attack it instead of the unit (which is removed and thus unreachable)
	}

	// What should we do with workers on :
	// case UnitAction::Repair:
	// Drop orders and run away or return after escape?
}

/*----------------------------------------------------------------------------
--  Conflicts
----------------------------------------------------------------------------*/

/**
 **	@brief	Returns the map distance between this unit and another one
 **
 **	@param	dst	The unit the distance to which is to be obtained
 **
 **	@return	The distance to the other unit, in tiles
 */
int CUnit::MapDistanceTo(const CUnit &dst) const
{
	if (this->MapLayer != dst.MapLayer) {
		return 16384;
	}
	
	return MapDistanceBetweenTypes(*this->GetFirstContainer()->Type, this->tilePos, this->MapLayer->ID, *dst.Type, dst.tilePos, dst.MapLayer->ID);
}

/**
 **  Returns the map distance to unit.
 **
 **  @param pos   map tile position.
 **
 **  @return      The distance between in tiles.
 */
int CUnit::MapDistanceTo(const Vec2i &pos, int z) const
{
	//Wyrmgus start
	if (z != this->MapLayer->ID) {
		return 16384;
	}
	//Wyrmgus end
	
	int dx;
	int dy;

	if (pos.x <= tilePos.x) {
		dx = tilePos.x - pos.x;
	//Wyrmgus start
	} else if (this->Container) { //if unit is within another, use the tile size of the transporter to calculate the distance
		dx = std::max<int>(0, pos.x - tilePos.x - this->Container->Type->get_tile_width() + 1);
	//Wyrmgus end
	} else {
		dx = std::max<int>(0, pos.x - tilePos.x - Type->get_tile_width() + 1);
	}
	if (pos.y <= tilePos.y) {
		dy = tilePos.y - pos.y;
	//Wyrmgus start
	} else if (this->Container) {
		dy = std::max<int>(0, pos.y - tilePos.y - this->Container->Type->get_tile_height() + 1);
	//Wyrmgus end
	} else {
		dy = std::max<int>(0, pos.y - tilePos.y - Type->get_tile_height() + 1);
	}
	return number::sqrt(dy * dy + dx * dx);
}

/**
**	@brief	Returns the map distance between two points with unit type
**
**	@param	src		Source unit type
**	@param	pos1	Map tile position of the source (upper-left)
**	@param	dst		Destination unit type to take into account
**	@param	pos2	Map tile position of the destination
**
**	@return	The distance between the types
*/
int MapDistanceBetweenTypes(const wyrmgus::unit_type &src, const Vec2i &pos1, int src_z, const wyrmgus::unit_type &dst, const Vec2i &pos2, int dst_z)
{
	return MapDistance(src.get_tile_size(), pos1, src_z, dst.get_tile_size(), pos2, dst_z);
}

int MapDistance(const Vec2i &src_size, const Vec2i &pos1, int src_z, const Vec2i &dst_size, const Vec2i &pos2, int dst_z)
{
	if (src_z != dst_z) {
		return 16384;
	}
	
	int dx;
	int dy;

	if (pos1.x + src_size.x <= pos2.x) {
		dx = std::max<int>(0, pos2.x - pos1.x - src_size.x + 1);
	} else {
		dx = std::max<int>(0, pos1.x - pos2.x - dst_size.x + 1);
	}
	if (pos1.y + src_size.y <= pos2.y) {
		dy = pos2.y - pos1.y - src_size.y + 1;
	} else {
		dy = std::max<int>(0, pos1.y - pos2.y - dst_size.y + 1);
	}
	return number::sqrt(dy * dy + dx * dx);
}

/**
**  Compute the distance from the view point to a given point.
**
**  @param pos  map tile position.
**
**  @todo FIXME: is it the correct place to put this function in?
*/
int ViewPointDistance(const Vec2i &pos)
{
	const CViewport &vp = *UI.SelectedViewport;
	const Vec2i vpSize(vp.MapWidth, vp.MapHeight);
	const Vec2i middle = vp.MapPos + vpSize / 2;

	return point::distance_to(middle, pos);
}

/**
**  Compute the distance from the view point to a given unit.
**
**  @param dest  Distance to this unit.
**
**  @todo FIXME: is it the correct place to put this function in?
*/
int ViewPointDistanceToUnit(const CUnit &dest)
{
	const CViewport &vp = *UI.SelectedViewport;
	const Vec2i vpSize(vp.MapWidth, vp.MapHeight);
	const Vec2i midPos = vp.MapPos + vpSize / 2;

	//Wyrmgus start
//	return dest.MapDistanceTo(midPos);
	return dest.MapDistanceTo(midPos, UI.CurrentMapLayer->ID);
	//Wyrmgus end
}

/**
**  Can the transporter transport the other unit.
**
**  @param transporter  Unit which is the transporter.
**  @param unit         Unit which wants to go in the transporter.
**
**  @return             1 if transporter can transport unit, 0 else.
*/
int CanTransport(const CUnit &transporter, const CUnit &unit)
{
	if (!transporter.Type->CanTransport()) {
		return 0;
	}

	if (transporter.is_under_construction()) { //under construction
		return 0;
	}

	if (&transporter == &unit) { // Cannot transporter itself.
		return 0;
	}

	//Wyrmgus start
	/*
	if (transporter.BoardCount >= transporter.Type->MaxOnBoard) { // full
		return 0;
	}
	*/
	
	if (transporter.ResourcesHeld > 0 && transporter.CurrentResource) {
		//cannot transport units if already has cargo
		return 0;
	}
	//Wyrmgus end

	if (transporter.BoardCount + unit.Type->BoardSize > transporter.Type->MaxOnBoard) { // too big unit
		return 0;
	}

	if (transporter.Variable[GARRISONED_GATHERING_INDEX].Value > 0) {
		if (transporter.get_given_resource() == nullptr || unit.Type->get_resource_info(transporter.get_given_resource()) == nullptr) {
			return 0;
		}
	}

	// Can transport only allied unit.
	// FIXME : should be parametrable.
	//Wyrmgus start
//	if (!transporter.IsTeamed(unit)) {
	if (!transporter.IsTeamed(unit) && !transporter.is_allied_with(unit) && (transporter.Player->get_type() != player_type::neutral || transporter.Type->BoolFlag[NEUTRAL_HOSTILE_INDEX].value) && unit.Player->get_type() != player_type::neutral) {
	//Wyrmgus end
		return 0;
	}
	for (unsigned int i = 0; i < UnitTypeVar.GetNumberBoolFlag(); i++) {
		if (transporter.Type->BoolFlag[i].CanTransport != CONDITION_TRUE) {
			if ((transporter.Type->BoolFlag[i].CanTransport == CONDITION_ONLY) ^ unit.Type->BoolFlag[i].value) {
				return 0;
			}
		}
	}
	return 1;
}

//Wyrmgus start
/**
**  Can the unit pick up the other unit.
**
**  @param picker		Unit which is the picker.
**  @param unit         Unit which wants to be picked.
**
**  @return             true if picker can pick up unit, false else.
*/
bool CanPickUp(const CUnit &picker, const CUnit &unit)
{
	if (!picker.Type->BoolFlag[ORGANIC_INDEX].value) { //only organic units can pick up power-ups and items
		return false;
	}
	if (!unit.Type->BoolFlag[ITEM_INDEX].value && !unit.Type->BoolFlag[POWERUP_INDEX].value) { //only item and powerup units can be picked up
		return false;
	}
	if (!unit.Type->BoolFlag[POWERUP_INDEX].value && !picker.HasInventory() && !wyrmgus::is_consumable_item_class(unit.Type->get_item_class())) { //only consumable items can be picked up as if they were power-ups for units with no inventory
		return false;
	}
	if (picker.CurrentAction() == UnitAction::Built) { // Under construction
		return false;
	}
	if (&picker == &unit) { // Cannot pick up itself.
		return false;
	}
	if (picker.HasInventory() && unit.Type->BoolFlag[ITEM_INDEX].value && picker.get_units_inside().size() >= UI.InventoryButtons.size()) { //full
		if (picker.Player == CPlayer::GetThisPlayer()) {
			std::string picker_name = picker.Name + "'s (" + picker.get_type_name() + ")";
			picker.Player->Notify(notification_type::red, picker.tilePos, picker.MapLayer->ID, _("%s inventory is full."), picker_name.c_str());
		}
		return false;
	}

	return true;
}
//Wyrmgus end

bool CUnit::is_enemy_of(const CPlayer &player) const
{
	if (this->Player->get_type() == player_type::neutral) {
		if (this->Type->BoolFlag[NEUTRAL_HOSTILE_INDEX].value && player.get_type() != player_type::neutral) {
			return true;
		}
	} else {
		//Wyrmgus start
		if (this->Player != &player && player.get_type() != player_type::neutral && !this->Player->has_building_access(&player) && this->Type->BoolFlag[HIDDENOWNERSHIP_INDEX].value && this->IsAgressive()) {
			return true;
		}
		//Wyrmgus end
	}
	
	return this->Player->is_enemy_of(player);
}

bool CUnit::is_enemy_of(const CUnit &unit) const
{
	switch (this->Player->get_type()) {
		case player_type::neutral:
			if (
				this->Type->BoolFlag[FAUNA_INDEX].value
				&& this->Type->BoolFlag[ORGANIC_INDEX].value
				&& unit.Type->BoolFlag[ORGANIC_INDEX].value
				&& this->Type->Slot != unit.Type->Slot
				&& (!unit.Type->BoolFlag[NEUTRAL_HOSTILE_INDEX].value || unit.Player->get_type() != player_type::neutral)
			) {
				if (
					this->Type->BoolFlag[PREDATOR_INDEX].value
					&& !unit.Type->BoolFlag[PREDATOR_INDEX].value
					&& this->CanEat(unit)
				) {
					return true;
				} else if (
					this->Type->BoolFlag[PEOPLEAVERSION_INDEX].value
					&& !unit.Type->BoolFlag[FAUNA_INDEX].value
					&& unit.Player->get_type() != player_type::neutral
					&& this->MapDistanceTo(unit) <= 1
				) {
					return true;
				}
			}
			break;
		default:
			if (this->Player != unit.Player) {
				if (
					unit.Player->get_type() == player_type::neutral
					&& (unit.Type->BoolFlag[NEUTRAL_HOSTILE_INDEX].value || unit.Type->BoolFlag[PREDATOR_INDEX].value)
				) {
					return true;
				}

				if (
					unit.CurrentAction() == UnitAction::Attack
					&& unit.CurrentOrder()->has_goal()
					&& unit.CurrentOrder()->get_goal()->Player == this->Player
					&& !unit.CurrentOrder()->get_goal()->Type->BoolFlag[HIDDENOWNERSHIP_INDEX].value
				) {
					return true;
				}

				if (
					unit.Player->get_type() != player_type::neutral && !this->Player->has_building_access(unit.Player) && !this->Player->has_neutral_faction_type()
					&& ((this->Type->BoolFlag[HIDDENOWNERSHIP_INDEX].value && this->IsAgressive()) || (unit.Type->BoolFlag[HIDDENOWNERSHIP_INDEX].value && unit.IsAgressive()))
				) {
					return true;
				}
			}
			break;
	}
		
	return this->is_enemy_of(*unit.Player);
}

/**
**  Check if the player is an ally
**
**  @param player  Player to check
*/
bool CUnit::is_allied_with(const CPlayer &player) const
{
	return this->Player->is_allied_with(player);
}

/**
**  Check if the unit is an ally
**
**  @param x  Unit to check
*/
bool CUnit::is_allied_with(const CUnit &unit) const
{
	return this->is_allied_with(*unit.Player);
}

/**
**  Check if unit shares vision with the player
**
**  @param x  Player to check
*/
bool CUnit::has_shared_vision_with(const CPlayer *player) const
{
	return this->Player->has_shared_vision_with(player);
}

/**
**  Check if the unit shares vision with the unit
**
**  @param x  Unit to check
*/
bool CUnit::has_shared_vision_with(const CUnit &unit) const
{
	return this->has_shared_vision_with(unit.Player);
}

/**
**  Check if both players share vision
**
**  @param x  Player to check
*/
bool CUnit::has_mutual_shared_vision_with(const CPlayer *player) const
{
	return this->Player->has_mutual_shared_vision_with(player);
}

/**
**  Check if both units share vision
**
**  @param x  Unit to check
*/
bool CUnit::has_mutual_shared_vision_with(const CUnit &unit) const
{
	return this->has_mutual_shared_vision_with(unit.Player);
}

/**
**  Check if the player is on the same team
**
**  @param x  Player to check
*/
bool CUnit::IsTeamed(const CPlayer &player) const
{
	return (this->Player->Team == player.Team);
}

/**
**  Check if the unit is on the same team
**
**  @param x  Unit to check
*/
bool CUnit::IsTeamed(const CUnit &unit) const
{
	return this->IsTeamed(*unit.Player);
}

/**
**  Check if the unit is unusable (for attacking...)
**  @todo look if correct used (UnitAction::Built is no problem if attacked)?
*/
bool CUnit::IsUnusable(const bool ignore_built_state) const
{
	return (!IsAliveOnMap() || (!ignore_built_state && this->is_under_construction()));
}

/**
**  Check if the unit attacking its goal will result in a ranged attack
*/
//Wyrmgus start
//bool CUnit::IsAttackRanged(CUnit *goal, const Vec2i &goalPos)
bool CUnit::IsAttackRanged(CUnit *goal, const Vec2i &goalPos, int z)
//Wyrmgus end
{
	if (this->Variable[ATTACKRANGE_INDEX].Value <= 1) { //always return false if the units attack range is 1 or lower
		return false;
	}
	
	if (this->Container) { //if the unit is inside a container, the attack will always be ranged
		return true;
	}
	
	const bool is_flying = this->Type->get_domain() == unit_domain::air || this->Type->get_domain() == unit_domain::space;
	const bool is_goal_flying = goal->Type->get_domain() == unit_domain::air || goal->Type->get_domain() == unit_domain::space;
	if (
		goal
		&& goal->IsAliveOnMap()
		&& (this->MapDistanceTo(*goal) > 1 || (is_flying != is_goal_flying))
	) {
		return true;
	}
	
	if (!goal && CMap::get()->Info->IsPointOnMap(goalPos, z) && this->MapDistanceTo(goalPos, z) > 1) {
		return true;
	}
	
	return false;
}

/*----------------------------------------------------------------------------
--  Initialize/Cleanup
----------------------------------------------------------------------------*/

/**
**  Initialize unit module.
*/
void InitUnits()
{
	if (!SaveGameLoading) {
		wyrmgus::unit_manager::get()->init();
	}
}

/**
**  Clean up unit module.
*/
void CleanUnits()
{
	wyrmgus::unit_manager::get()->clean_units();

	HelpMeLastCycle = 0;
}
