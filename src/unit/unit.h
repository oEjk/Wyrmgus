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
//      (c) Copyright 1998-2020 by Lutz Sammer, Jimmy Salmon and Andrettin
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
//

#pragma once

#include "item/item_slot.h"
#include "player.h"
#include "player_container.h"
#include "unit/unit_type.h"
#include "unit/unit_variable.h"
#include "vec2i.h"

class CAnimation;
class CBuildRestrictionOnTop;
class CFile;
class Missile;
class COrder;
class CPlayer;
class CUnit;
class CUnitStats;
class CUpgrade;
class CViewport;
class PathFinderData;
enum class UnitAction : char;
enum class VariableAttribute;
struct lua_State;

namespace wyrmgus {
	class animation_set;
	class character;
	class construction;
	class construction_frame;
	class map_template;
	class player_color;
	class spell;
	class tile;
	class time_of_day;
	class unique_item;
	class unit_manager;
	class unit_ref;
	class unit_type;
	class unit_type_variation;
	enum class gender;
	enum class item_class;
	enum class item_slot;
}

/*
** Configuration of the small (unit) AI.
*/
constexpr int PRIORITY_FACTOR = 0x00080000;
constexpr int HEALTH_FACTOR = 0x00000001;
constexpr int DISTANCE_FACTOR = 0x00010000;
constexpr int INRANGE_FACTOR = 0x00008000;
constexpr int INRANGE_BONUS = 0x01000000;
constexpr int CANATTACK_BONUS = 0x00080000;
constexpr int AIPRIORITY_BONUS = 0x04000000;

//the range in which experience is distributed
constexpr int ExperienceRange = 6;

/// Called whenever the selected unit was updated
extern void SelectedUnitChanged();

/// Returns the map distance between to unittype as locations
extern int MapDistanceBetweenTypes(const wyrmgus::unit_type &src, const Vec2i &pos1, int src_z, const wyrmgus::unit_type &dst, const Vec2i &pos2, int dst_z);
								   
extern int MapDistance(const Vec2i &src_size, const Vec2i &pos1, int src_z, const Vec2i &dst_size, const Vec2i &pos2, int dst_z);

/**
**  Unit/Missile headings.
**          N
**  NW              NE
**  W                E
**  SW              SE
**          S
*/
enum _directions_ {
	LookingN  = 0 * 32,      /// Unit looking north
	LookingNE = 1 * 32,      /// Unit looking north east
	LookingE  = 2 * 32,      /// Unit looking east
	LookingSE = 3 * 32,      /// Unit looking south east
	LookingS  = 4 * 32,      /// Unit looking south
	LookingSW = 5 * 32,      /// Unit looking south west
	LookingW  = 6 * 32,      /// Unit looking west
	LookingNW = 7 * 32       /// Unit looking north west
};

constexpr int NextDirection = 32;        /// Next direction N->NE->E...
#define UnitNotSeen 0x7fffffff  /// Unit not seen, used by CUnit::SeenFrame

/// The big unit structure
class CUnit final
{
public:
	static constexpr unsigned char max_step_count = 10;

	CUnit();
	~CUnit();

	void Init();

	std::shared_ptr<wyrmgus::unit_ref> acquire_ref();

	int get_ref_count() const
	{
		return this->ref.use_count();
	}

	COrder *CurrentOrder() const
	{
		return this->Orders.front().get();
	}

	UnitAction CurrentAction() const;

	bool IsIdle() const;

	void ClearAction();

	/// Initialize unit structure with default values
	void Init(const wyrmgus::unit_type &type);
	void initialize_base_reference();

	/// Assign unit to player
	void AssignToPlayer(CPlayer &player);

	const wyrmgus::player_color *get_player_color() const;

	const wyrmgus::species *get_species() const;
	const wyrmgus::civilization *get_civilization() const;

	/// Draw a single unit
	void Draw(const CViewport &vp) const;
	/// Place a unit on map
	//Wyrmgus start
//	void Place(const Vec2i &pos);
	void Place(const Vec2i &pos, int z);
	//Wyrmgus end

	/// Move unit to tile(pos). (Do special stuff : vision, cachelist, pathfinding)
	//Wyrmgus start
//	void MoveToXY(const Vec2i &pos);
	void MoveToXY(const Vec2i &pos, int z);
	//Wyrmgus end
	/// Add a unit inside a container. Only deal with list stuff.
	void AddInContainer(CUnit &host);
	//Wyrmgus start
	void UpdateContainerAttackRange();
	void UpdateXPRequired();
	void UpdatePersonalName(bool update_settlement_name = true);
	void UpdateExtraName();
	void UpdateSettlement();
	void UpdateBuildingSettlementAssignment(const wyrmgus::site *old_settlement = nullptr); //update the settlement assignment of surrounding buildings for this town hall
	void XPChanged();
	//Wyrmgus end
	/// Change owner of unit
	//Wyrmgus start
//	void ChangeOwner(CPlayer &newplayer);
	void ChangeOwner(CPlayer &newplayer, bool show_change = false);
	//Wyrmgus end

	/// Remove unit from map/groups/...
	#ifdef __MORPHOS__
	#undef Remove
	#endif
	void Remove(CUnit *host);

	void AssignWorkerToMine(CUnit &mine);
	void DeAssignWorkerFromMine(CUnit &mine);

	/// Release a unit
	void Release(bool final = false);
	
	//Wyrmgus start
	void SetResourcesHeld(int quantity);
	void ChangeResourcesHeld(int quantity);
	void ReplaceOnTop(CUnit &replaced_unit);
	void ChangeExperience(int amount, int around_range = 0); //around_range is the range around the unit that other units of the same player will receive experience
	void IncreaseLevel(int level_quantity, bool automatic_learning = true);
	void Retrain();
	void HealingItemAutoUse();
	void set_character(wyrmgus::character *character);
	void SetCharacter(const std::string &character_identifier, const bool custom_hero = false);
	void apply_character_properties();
	bool CheckTerrainForVariation(const wyrmgus::unit_type_variation *variation) const;
	bool CheckSeasonForVariation(const wyrmgus::unit_type_variation *variation) const;
	void ChooseVariation(const wyrmgus::unit_type *new_type = nullptr, bool ignore_old_variation = false, int image_layer = -1);
	void SetVariation(const wyrmgus::unit_type_variation *new_variation, int image_layer = -1);
	const wyrmgus::unit_type_variation *GetVariation() const;
	const wyrmgus::unit_type_variation *GetLayerVariation(const unsigned int image_layer) const;
	void UpdateButtonIcons();
	void ChooseButtonIcon(const ButtonCmd button_action);
	void EquipItem(CUnit &item, bool affect_character = true);
	void DeequipItem(CUnit &item, bool affect_character = true);
	void ReadWork(const CUpgrade *work, bool affect_character = true);
	void ConsumeElixir(const CUpgrade *elixir, bool affect_character = true);
	void ApplyAura(int aura_index);
	void ApplyAuraEffect(int aura_index);
	void SetPrefix(const CUpgrade *prefix);
	void SetSuffix(const CUpgrade *suffix);
	void SetSpell(const wyrmgus::spell *spell);
	void SetWork(const CUpgrade *work);
	void SetElixir(const CUpgrade *elixir);

	const wyrmgus::unique_item *get_unique() const
	{
		return this->unique;
	}

	void set_unique(const wyrmgus::unique_item *unique);

	const wyrmgus::character *get_character() const
	{
		return this->character;
	}

	wyrmgus::character *get_character()
	{
		return this->character;
	}

	void Identify();
	void CheckIdentification();
	void CheckKnowledgeChange(int variable, int change);
	void UpdateItemName();
	void GenerateDrop();
	void GenerateSpecialProperties(CUnit *dropper = nullptr, CPlayer *dropper_player = nullptr, bool allow_unique = true, bool sold_item = false, bool always_magic = false);
	void GeneratePrefix(CUnit *dropper, CPlayer *dropper_player);
	void GenerateSuffix(CUnit *dropper, CPlayer *dropper_player);
	void GenerateSpell(CUnit *dropper, CPlayer *dropper_player);
	void GenerateWork(CUnit *dropper, CPlayer *dropper_player);
	void GenerateUnique(CUnit *dropper, CPlayer *dropper_player);
	void UpdateSoldUnits();
	void SellUnit(CUnit *sold_unit, int player);
	void ProduceResource(const int resource);
	void SellResource(const int resource, const int player);
	void BuyResource(const int resource, const int player);
	void Scout();
	//Wyrmgus end
	
	bool RestoreOrder();
	bool CanStoreOrder(COrder *order);

	void clear_orders();
	void clear_special_orders();

	void clear_all_orders()
	{
		this->clear_orders();
		this->clear_special_orders();
	}

	// Cowards and invisible units don't attack unless ordered.
	bool IsAgressive() const
	{
		//Wyrmgus start
//		return (Type->BoolFlag[CANATTACK_INDEX].value && !Type->BoolFlag[COWARD_INDEX].value
		return (CanAttack() && !Type->BoolFlag[COWARD_INDEX].value && Variable[TERROR_INDEX].Value == 0
		//Wyrmgus end
				&& Variable[INVISIBLE_INDEX].Value == 0);
	}

	/// Returns true, if unit is directly seen by an allied unit.
	bool IsVisible(const CPlayer &player) const;

	bool IsInvisibile(const CPlayer &player) const
	{
		return (&player != Player && !!Variable[INVISIBLE_INDEX].Value
				&& !player.has_mutual_shared_vision_with(*Player));
	}

	/**
	**  Returns true if unit is alive.
	**  Another unit can interact only with alive map units.
	**
	**  @return        True if alive, false otherwise.
	*/
	bool IsAlive() const;

	/**
	**  Returns true if unit is alive and on the map.
	**  Another unit can interact only with alive map units.
	**
	**  @return        True if alive, false otherwise.
	*/
	bool IsAliveOnMap() const
	{
		return !Removed && IsAlive();
	}

	/**
	**  Returns true, if unit is visible as an action goal for a player on the map.
	**
	**  @param player  Player to check for.
	**
	**  @return        True if visible, false otherwise.
	*/
	bool IsVisibleAsGoal(const CPlayer &player) const
	{
		// Invisibility
		if (IsInvisibile(player)) {
			return false;
		}
		// Don't attack revealers
		if (this->Type->BoolFlag[REVEALER_INDEX].value) {
			return false;
		}
		//Wyrmgus start
//		if ((player.Type == PlayerComputer && !this->Type->BoolFlag[PERMANENTCLOAK_INDEX].value)
		if (
		//Wyrmgus end
			//Wyrmgus start
//			|| IsVisible(player) || IsVisibleOnRadar(player)) {
			IsVisible(player) || IsVisibleOnRadar(player)) {
			//Wyrmgus end
			return IsAliveOnMap();
		} else {
			return Type->BoolFlag[VISIBLEUNDERFOG_INDEX].value
				   && this->is_seen_by_player(&player)
				   && !this->is_seen_destroyed_by_player(&player);
		}
	}

	/**
	**  Returns true, if unit is visible for this player on the map.
	**  The unit has to be out of fog of war and alive
	**
	**  @param player  Player to check for.
	**
	**  @return        True if visible, false otherwise.
	*/
	bool IsVisibleOnMap(const CPlayer &player) const
	{
		return IsAliveOnMap() && !IsInvisibile(player) && IsVisible(player);
	}

	/// Returns true if unit is visible on minimap. Only for ThisPlayer.
	bool IsVisibleOnMinimap() const;

	// Returns true if unit is visible under radar (By player, or by shared vision)
	bool IsVisibleOnRadar(const CPlayer &pradar) const;

	/// Returns true if unit is visible in a viewport. Only for ThisPlayer.
	bool IsVisibleInViewport(const CViewport &vp) const;

	bool IsEnemy(const CPlayer &player) const;
	bool IsEnemy(const CUnit &unit) const;
	bool IsAllied(const CPlayer &player) const;
	bool IsAllied(const CUnit &unit) const;
	bool has_shared_vision_with(const CPlayer &player) const;
	bool has_shared_vision_with(const CUnit &unit) const;
	bool has_mutual_shared_vision_with(const CPlayer &player) const;
	bool has_mutual_shared_vision_with(const CUnit &unit) const;
	bool IsTeamed(const CPlayer &player) const;
	bool IsTeamed(const CUnit &unit) const;

	bool IsUnusable(bool ignore_built_state = false) const;

	bool is_ai_active() const
	{
		return this->Active != 0;
	}

	int MapDistanceTo(const CUnit &dst) const;

	int MapDistanceTo(const Vec2i &pos, int z) const;

	/**
	**  Test if unit can move.
	**  For the moment only check for move animation.
	**
	**  @return true if unit can move.
	*/
	bool CanMove() const { return Type->CanMove(); }

	int GetDrawLevel() const;

	bool IsAttackRanged(CUnit *goal, const Vec2i &goalPos, int z);

	PixelPos get_map_pixel_pos_top_left() const;
	PixelPos get_scaled_map_pixel_pos_top_left() const;
	PixelPos get_map_pixel_pos_center() const;
	PixelPos get_scaled_map_pixel_pos_center() const;
	
	//Wyrmgus start
	Vec2i GetTileSize() const;
	Vec2i GetHalfTileSize() const;
	PixelSize get_tile_pixel_size() const;
	PixelSize get_scaled_tile_pixel_size() const;
	PixelSize get_half_tile_pixel_size() const;
	PixelSize get_scaled_half_tile_pixel_size() const;
	QPoint get_bottom_right_tile_pos() const;
	QPoint get_center_tile_pos() const;
	const wyrmgus::tile *get_center_tile() const;

	QRect get_tile_rect() const
	{
		return QRect(this->tilePos, this->get_bottom_right_tile_pos());
	}

	const QPoint &get_pixel_offset() const
	{
		return this->pixel_offset;
	}

	QPoint get_scaled_pixel_offset() const;
	
	CUnit *GetFirstContainer() const;

	void SetIndividualUpgrade(const CUpgrade *upgrade, int quantity);
	int GetIndividualUpgrade(const CUpgrade *upgrade) const;
	int GetAvailableLevelUpUpgrades(bool only_units = false) const;

	int get_variable_value(const int var_index) const
	{
		return this->Variable.at(var_index).Value;
	}

	void set_variable_value(const int var_index, const int value)
	{
		this->Variable.at(var_index).Value = value;
	}

	void change_variable_value(const int var_index, const int change)
	{
		this->set_variable_value(var_index, this->get_variable_value(var_index) + change);
	}

	int get_variable_max(const int var_index) const
	{
		return this->Variable.at(var_index).Max;
	}

	void set_variable_max(const int var_index, const int max)
	{
		this->Variable.at(var_index).Max = max;
	}

	char get_variable_increase(const int var_index) const
	{
		return this->Variable.at(var_index).Increase;
	}

	int GetModifiedVariable(const int index, const VariableAttribute variable_type) const;
	int GetModifiedVariable(const int index) const;

	int GetReactionRange() const;
	unsigned get_item_slot_quantity(const wyrmgus::item_slot item_slot) const;
	wyrmgus::item_class GetCurrentWeaponClass() const;
	int GetItemVariableChange(const CUnit *item, int variable_index, bool increase = false) const;
	int GetDisplayPlayer() const;
	int GetPrice() const;
	int GetUnitStock(const wyrmgus::unit_type *unit_type) const;
	void SetUnitStock(const wyrmgus::unit_type *unit_type, const int quantity);
	void ChangeUnitStock(const wyrmgus::unit_type *unit_type, const int quantity);
	int GetUnitStockReplenishmentTimer(const wyrmgus::unit_type *unit_type) const;
	void SetUnitStockReplenishmentTimer(const wyrmgus::unit_type *unit_type, int quantity);
	void ChangeUnitStockReplenishmentTimer(const wyrmgus::unit_type *unit_type, int quantity);
	int GetResourceStep(const int resource) const;
	int GetTotalInsideCount(const CPlayer *player = nullptr, const bool ignore_items = true, const bool ignore_saved_cargo = false, const wyrmgus::unit_type *type = nullptr) const;
	bool CanAttack(bool count_inside = true) const;
	bool IsInCombat() const;
	bool CanHarvest(const CUnit *dest, bool only_harvestable = true) const;
	bool CanReturnGoodsTo(const CUnit *dest, int resource = 0) const;
	bool CanCastSpell(const wyrmgus::spell *spell, const bool ignore_mana_and_cooldown) const;
	bool CanCastAnySpell() const;

	const std::vector<const wyrmgus::spell *> &get_autocast_spells() const
	{
		return this->autocast_spells;
	}

	bool is_autocast_spell(const wyrmgus::spell *spell) const;
	void add_autocast_spell(const wyrmgus::spell *spell);
	void remove_autocast_spell(const wyrmgus::spell *spell);
	bool CanAutoCastSpell(const wyrmgus::spell *spell) const;
	bool IsItemEquipped(const CUnit *item) const;
	bool is_item_class_equipped(const wyrmgus::item_class item_class) const;
	bool IsItemTypeEquipped(const wyrmgus::unit_type *item_type) const;
	bool IsUniqueItemEquipped(const wyrmgus::unique_item *unique) const;
	bool can_equip_item(const CUnit *item) const;
	bool can_equip_item_class(const wyrmgus::item_class item_class) const;
	bool CanUseItem(CUnit *item) const;
	bool IsItemSetComplete(const CUnit *item) const;
	bool EquippingItemCompletesSet(const CUnit *item) const;
	bool DeequippingItemBreaksSet(const CUnit *item) const;
	bool HasInventory() const;

	template <bool precondition = false>
	bool can_learn_ability(const CUpgrade *ability) const;

	bool can_hire_mercenary(const wyrmgus::unit_type *type, const wyrmgus::civilization *civilization = nullptr) const;
	bool CanEat(const CUnit &unit) const;
	bool LevelCheck(const int level) const;
	bool is_spell_empowered(const wyrmgus::spell *spell) const;
	bool UpgradeRemovesExistingUpgrade(const CUpgrade *upgrade) const;
	bool HasAdjacentRailForUnitType(const wyrmgus::unit_type *type) const;
	const wyrmgus::animation_set *get_animation_set() const;
	const wyrmgus::construction *get_construction() const;
	const wyrmgus::icon *get_icon() const;
	const wyrmgus::icon *GetButtonIcon(const ButtonCmd button_action) const;
	MissileConfig GetMissile() const;
	const std::shared_ptr<CPlayerColorGraphic> &GetLayerSprite(int image_layer) const;
	std::string GetName() const;
	std::string GetTypeName() const;
	std::string GetMessageName() const;
	//Wyrmgus end

	const std::string &get_surname() const
	{
		return this->surname;
	}

	const wyrmgus::time_of_day *get_center_tile_time_of_day() const;

	bool is_seen_by_player(const CPlayer *player) const;

	bool is_seen_by_player(const int index) const
	{
		return this->Seen.by_player.contains(index);
	}

	bool is_seen_destroyed_by_player(const CPlayer *player) const;

	bool is_seen_destroyed_by_player(const int index) const
	{
		return this->Seen.destroyed.contains(index);
	}

	bool is_in_tile_rect(const QRect &tile_rect, int z) const;
	bool is_in_subtemplate_area(const wyrmgus::map_template *subtemplate) const;

	wyrmgus::gender get_gender() const;

	unsigned char get_step_count() const
	{
		return this->step_count;
	}

	void increment_step_count()
	{
		++this->step_count;
		this->step_count = std::min(this->get_step_count(), CUnit::max_step_count);
	}

	void reset_step_count()
	{
		this->step_count = 0;
	}

public:
	class CUnitManagerData final
	{
	public:
		int GetUnitId() const
		{
			return this->slot;
		}

	private:
		int slot = -1;           /// index in UnitManager::unitSlots
		int unitSlot = -1;       /// index in UnitManager::units

		friend class wyrmgus::unit_manager;
	};

private:
	std::shared_ptr<wyrmgus::unit_ref> base_ref; //base reference for the unit
	std::weak_ptr<wyrmgus::unit_ref> ref; //the handle to the unit's reference object
public:
	// @note int is faster than shorts
	unsigned int     ReleaseCycle; /// When this unit could be recycled
	CUnitManagerData UnitManagerData;
	size_t PlayerSlot;  /// index in Player->Units

	int    InsideCount;   /// Number of units inside.
	int    BoardCount;    /// Number of units transported inside.
	CUnit *UnitInside;    /// Pointer to one of the units inside.
	CUnit *Container;     /// Pointer to the unit containing it (or 0)
	CUnit *NextContained; /// Next unit in the container.
	CUnit *PrevContained; /// Previous unit in the container.

	struct {
		std::vector<std::shared_ptr<wyrmgus::unit_ref>> Workers; ///references to the workers assigned to this resource.
		int Active = 0; /// how many units are harvesting from the resource.
	} Resource; /// Resource still

	//Wyrmgus start
	std::vector<CUnit *> EquippedItems[static_cast<int>(wyrmgus::item_slot::count)];	/// Pointer to unit's equipped items, per slot
	std::vector<CUnit *> SoldUnits;						/// units available for sale at this unit
	//Wyrmgus end
	
	Vec2i tilePos = Vec2i(-1, -1); /// Map position X
	//Wyrmgus start
	Vec2i RallyPointPos = Vec2i(-1, -1);			/// used for storing the rally point position (where units trained by this unit will be sent to)
	CMapLayer *MapLayer = nullptr;			/// in which map layer the unit is
	CMapLayer *RallyPointMapLayer = nullptr;	/// in which map layer the unit's rally point is
	//Wyrmgus end

	unsigned int Offset;/// Map position as flat index offset (x + y * w)

	const wyrmgus::unit_type *Type;        /// Pointer to unit-type (peon,...)
	CPlayer    *Player;            /// Owner of this unit
	const CUnitStats *Stats;       /// Current unit stats
	int         CurrentSightRange; /// Unit's Current Sight Range

	// Pathfinding stuff:
	std::unique_ptr<PathFinderData> pathFinderData;

	// DISPLAY:
	int         Frame;      /// Image frame: <0 is mirrored
	//Wyrmgus start
	std::string Name;		/// Unit's personal/proper name (if any)
	std::string ExtraName;	/// Unit's "extra" name (i.e. a nickname)
private:
	std::string surname;	/// Unit's surname
	wyrmgus::character *character = nullptr; //character represented by this unit
public:
	const wyrmgus::site *settlement = nullptr;	/// Settlement (for if the unit is a town hall or a building associated to a settlement)
	const wyrmgus::site *site = nullptr; //the site to which the unit belongs, if it is a site unit (not necessarily the same as the settlement, e.g. if the site is a non-major one)
	CUpgrade *Trait;	/// Unit's trait
	int Variation;      /// Which of the variations of its unit type this unit has
	int LayerVariation[MaxImageLayers];	/// Which layer variations this unit has
	const CUpgrade *Prefix = nullptr;	/// Item unit's prefix
	const CUpgrade *Suffix = nullptr;	/// Item unit's suffix
	const wyrmgus::spell *Spell = nullptr; /// Item unit's spell
	const CUpgrade *Work = nullptr;		/// Item unit's literary work
	const CUpgrade *Elixir = nullptr;	/// Item unit's elixir
private:
	const wyrmgus::unique_item *unique = nullptr;		/// Whether the item is unique
public:
	bool Bound;			/// Whether the item is bound to its owner
	bool Identified;	/// Whether the item has been identified
	CUnit *ConnectingDestination;	/// Which connector this unit connects to (if any)
	std::map<ButtonCmd, const wyrmgus::icon *> ButtonIcons;				/// icons for button actions
	//Wyrmgus end
	std::map<int, int> IndividualUpgrades;      /// individual upgrades which the unit has (and how many of it the unit has)

	QPoint pixel_offset;         /// pixel image displacement to map position
	unsigned char Direction; //: 8; /// angle (0-255) unit looking
	//Wyrmgus start
	unsigned char GivesResource;	/// The resource currently given by the unit
	//Wyrmgus end
	unsigned char CurrentResource;
	int ResourcesHeld;      /// Resources Held by a unit
	std::map<int, int> UnitStock; 						/// How many of each unit type this unit has stocked
	wyrmgus::unit_type_map<int> UnitStockReplenishmentTimers; 	/// Replenishment timer for each unit type stock

	unsigned char DamagedType;   /// Index of damage type of unit which damaged this unit
	unsigned long Attacked;      /// gamecycle unit was last attacked
	unsigned Blink : 3;          /// Let selection rectangle blink
	unsigned Moving : 1;         /// The unit is moving
	unsigned ReCast : 1;         /// Recast again next cycle
	unsigned AutoRepair : 1;     /// True if unit tries to repair on still action.

	unsigned Burning : 1;        /// unit is burning
	unsigned Destroyed : 1;      /// unit is destroyed pending reference
	unsigned Removed : 1;        /// unit is removed (not on map)
	unsigned Selected : 1;       /// unit is selected

	unsigned UnderConstruction : 1;    /// Unit is in construction
	unsigned Active : 1;         /// Unit is active for AI
	unsigned Boarded : 1;        /// Unit is on board a transporter.
	unsigned CacheLock : 1;      /// Unit is on lock by unitcache operations.

	unsigned Summoned : 1;       /// Unit is summoned using spells.
	unsigned Waiting : 1;        /// Unit is waiting and playing its still animation
	unsigned MineLow : 1;        /// This mine got a notification about its resources being low
	
	unsigned TeamSelected;  /// unit is selected by a team member.
	CPlayer *RescuedFrom;        /// The original owner of a rescued unit.
	/// null if the unit was not rescued.
	/* Seen stuff. */
	int VisCount[PlayerMax];     /// Unit visibility counts
	struct _seen_stuff_ {
		const wyrmgus::construction_frame *cframe = nullptr; /// Seen construction frame
		int Frame = 0; /// last seen frame/stage of buildings
		const wyrmgus::unit_type *Type = nullptr; /// Pointer to last seen unit-type
		Vec2i tilePos = Vec2i(-1, -1); /// Last unit->tilePos Seen
		QPoint pixel_offset = QPoint(0, 0); /// seen pixel image displacement to map position
		unsigned UnderConstruction : 1 = 0; /// Unit seen construction
		unsigned State : 3 = 0; /// Unit seen build/upgrade state
		wyrmgus::player_index_set destroyed;  /// Unit seen destroyed or not
		wyrmgus::player_index_set by_player;   /// Track unit seen by player
	} Seen;

	std::vector<wyrmgus::unit_variable> Variable; /// array of User Defined variables.

	unsigned long TTL;  /// time to live

	unsigned int GroupId;       /// unit belongs to this group id
	unsigned int LastGroup;     /// unit belongs to this last group

	unsigned int Wait;          /// action counter
	int Threshold;              /// The counter while ai unit couldn't change target.
	
private:
	unsigned char step_count = 0;	/// How many steps the unit has taken without stopping (maximum 10)

public:
	struct _unit_anim_ {
		const CAnimation *Anim;      /// Anim
		const CAnimation *CurrAnim;  /// CurrAnim
		int Wait;                    /// Wait
		int Unbreakable;             /// Unbreakable
	} Anim, WaitBackup;


	std::vector<std::unique_ptr<COrder>> Orders; /// orders to process
	std::unique_ptr<COrder> SavedOrder;         /// order to continue after current
	std::unique_ptr<COrder> NewOrder;           /// order for new trained units
	std::unique_ptr<COrder> CriticalOrder;      /// order to do as possible in breakable animation.

private:
	std::vector<const wyrmgus::spell *> autocast_spells; //the list of autocast spells
	std::vector<bool> spell_autocast; //spells to auto cast, mapped to their spell IDs

public:
	std::unique_ptr<int[]> SpellCoolDownTimers;   /// how much time unit need to wait before spell will be ready

	CUnit *Goal; /// Generic/Teleporter goal pointer

	friend static int CclUnit(lua_State *l);
};

#define NoUnitP (CUnit *)0        /// return value: for no unit found

/**
**  Returns unit number (unique to this unit)
*/
inline int UnitNumber(const CUnit &unit)
{
	return unit.UnitManagerData.GetUnitId();
}

/**
**  User preference.
*/
class CPreference
{
public:
	CPreference() : ShowSightRange(false), ShowReactionRange(false),
		ShowAttackRange(false), ShowMessages(true), BigScreen(false),
		PauseOnLeave(true), AiExplores(true), GrayscaleIcons(false),
		IconsShift(false), StereoSound(true), MineNotifications(false),
		DeselectInMine(false),
		//Wyrmgus start
		PlayerColorCircle(false), SepiaForGrayscale(false),
		ShowPathlines(false),
//		ShowOrders(0), ShowNameDelay(0), ShowNameTime(0), AutosaveMinutes(5) {};
		ShowOrders(0), ShowNameDelay(0), ShowNameTime(0), AutosaveMinutes(5), HotkeySetup(0) {};
		//Wyrmgus end

	bool ShowSightRange;     /// Show sight range.
	bool ShowReactionRange;  /// Show reaction range.
	bool ShowAttackRange;    /// Show attack range.
	bool ShowMessages;		 /// Show messages.
	bool BigScreen;			 /// If true, shows the big screen(without panels)
	bool PauseOnLeave;       /// If true, game pauses when cursor is gone
	bool AiExplores;         /// If true, AI sends explorers to search for resources (almost useless thing)
	bool GrayscaleIcons;     /// Use grayscaled icons for unavailable units, upgrades, etc
	bool IconsShift;         /// Shift icons slightly when you press on them
	bool StereoSound;        /// Enables/disables stereo sound effects	
	bool MineNotifications;  /// Show mine is running low/depleted messages
	bool DeselectInMine;     /// Deselect peasants in mines
	//Wyrmgus start
	bool SepiaForGrayscale;		/// Use a sepia filter for grayscale icons
	bool PlayerColorCircle;		/// Show a player color circle below each unit
	bool ShowPathlines;			/// Show order pathlines
	//Wyrmgus end

	int ShowOrders;			/// How many second show orders of unit on map.
	int ShowNameDelay;		/// How many cycles need to wait until unit's name popup will appear.
	int ShowNameTime;		/// How many cycles need to show unit's name popup.
	int AutosaveMinutes;	/// Autosave the game every X minutes; autosave is disabled if the value is 0
	//Wyrmgus start
	int HotkeySetup;			/// Hotkey layout (0 = default, 1 = position-based, 2 = position-based (except commands))
	//Wyrmgus end
	std::string SF2Soundfont;/// Path to SF2 soundfont
};

extern CPreference Preference;

// in unit_draw.c
/// @todo could be moved into the user interface ?
extern unsigned long ShowOrdersCount;   /// Show orders for some time
extern unsigned long ShowNameDelay;     /// Delay to show unit's name
extern unsigned long ShowNameTime;      /// Show unit's name for some time
extern bool EnableTrainingQueue;               /// Config: training queues enabled
extern bool EnableBuildingCapture;             /// Config: building capture enabled
extern bool RevealAttacker;                    /// Config: reveal attacker enabled
extern int ResourcesMultiBuildersMultiplier;   /// Config: spend resources for building with multiple workers
extern const CViewport *CurrentViewport; /// CurrentViewport
extern void DrawUnitSelection(const CViewport &vp, const CUnit &unit);
extern void (*DrawSelection)(IntColor, int, int, int, int);

extern unsigned int MaxSelectable;    /// How many units could be selected
extern std::vector<CUnit *> Selected; /// currently selected units

/// Mark the field with the FieldFlags.
void MarkUnitFieldFlags(const CUnit &unit);
/// Unmark the field with the FieldFlags.
void UnmarkUnitFieldFlags(const CUnit &unit);
/// Update unit->CurrentSightRange.
void UpdateUnitSightRange(CUnit &unit);
/// Create a new unit
extern CUnit *MakeUnit(const wyrmgus::unit_type &type, CPlayer *player);
/// Create a new unit and place on map
extern CUnit *MakeUnitAndPlace(const Vec2i &pos, const wyrmgus::unit_type &type, CPlayer *player, int z);
/// Create a new unit and place it on the map, and update its player accordingly
extern CUnit *CreateUnit(const Vec2i &pos, const wyrmgus::unit_type &type, CPlayer *player, int z, bool no_bordering_building = false, const wyrmgus::site *settlement = nullptr);
extern CUnit *CreateResourceUnit(const Vec2i &pos, const wyrmgus::unit_type &type, int z, bool allow_unique = true);
/// Find the nearest position at which unit can be placed.
void FindNearestDrop(const wyrmgus::unit_type &type, const Vec2i &goalPos, Vec2i &resPos, int heading, int z, bool no_bordering_building = false, bool ignore_construction_requirements = false, const wyrmgus::site *settlement = nullptr);
/// Handle the loss of a unit (food,...)
extern void UnitLost(CUnit &unit);
/// @todo more docu
extern void UpdateForNewUnit(const CUnit &unit, int upgrade);
/// @todo more docu
extern void NearestOfUnit(const CUnit &unit, const Vec2i &pos, Vec2i *dpos);

/// Call when an Unit goes under fog.
extern void UnitGoesUnderFog(CUnit &unit, const CPlayer &player);
/// Call when an Unit goes out of fog.
extern void UnitGoesOutOfFog(CUnit &unit, const CPlayer &player);

/// Does a recount for VisCount
extern void UnitCountSeen(CUnit &unit);

/// Check for rescue each second
extern void RescueUnits();

/// Convert direction (dx,dy) to heading (0-255)
extern int DirectionToHeading(const Vec2i &dir);
/// Convert direction (dx,dy) to heading (0-255)
extern int DirectionToHeading(const PixelDiff &dir);

///Correct directions for placed wall.
extern void CorrectWallDirections(CUnit &unit);
/// Correct the surrounding walls.
extern void CorrectWallNeighBours(CUnit &unit);

/// Update frame from heading
extern void UnitUpdateHeading(CUnit &unit);
/// Heading and frame from delta direction
extern void UnitHeadingFromDeltaXY(CUnit &unit, const Vec2i &delta);

/// @todo more docu
extern void DropOutOnSide(CUnit &unit, int heading, const CUnit *container);
/// @todo more docu
extern void DropOutNearest(CUnit &unit, const Vec2i &goalPos, const CUnit *container);

/// Drop out all units in the unit
extern void DropOutAll(const CUnit &unit);

/// Return the rule used to build this building.
extern const CBuildRestrictionOnTop *OnTopDetails(const wyrmgus::unit_type &type, const wyrmgus::unit_type *parent);
/// @todo more docu
extern CUnit *CanBuildHere(const CUnit *unit, const wyrmgus::unit_type &type, const QPoint &pos, const int z, const bool no_bordering_building = false);
/// @todo more docu
extern bool CanBuildOn(const QPoint &pos, const int mask, const int z, const CPlayer *player, const wyrmgus::unit_type *unit_type);
/// FIXME: more docu
extern CUnit *CanBuildUnitType(const CUnit *unit, const wyrmgus::unit_type &type, const QPoint &pos, const int real, const bool ignore_exploration, const int z, const bool no_bordering_building = false);
/// Get the suitable animation frame depends of unit's damaged type.
extern int ExtraDeathIndex(const char *death);

/// Get unit under cursor
extern CUnit *UnitOnScreen(int x, int y);

/// Let a unit die
extern void LetUnitDie(CUnit &unit, bool suicide = false);
/// Destroy all units inside another unit
extern void DestroyAllInside(CUnit &source);
/// Calculate some value to measure the unit's priority for AI
extern int ThreatCalculate(const CUnit &unit, const CUnit &dest);
/// Hit unit with damage, if destroyed give attacker the points
//Wyrmgus start
//extern void HitUnit(CUnit *attacker, CUnit &target, int damage, const Missile *missile = nullptr);
extern void HitUnit(CUnit *attacker, CUnit &target, int damage, const Missile *missile = nullptr, bool show_damage = true);
extern void HitUnit_NormalHitSpecialDamageEffects(CUnit &attacker, CUnit &target);
extern void HitUnit_SpecialDamageEffect(CUnit &target, int dmg_var);
extern void HitUnit_RunAway(CUnit &target, const CUnit &attacker);
//Wyrmgus end

/// Calculate the distance from current view point to coordinate
extern int ViewPointDistance(const Vec2i &pos);
/// Calculate the distance from current view point to unit
extern int ViewPointDistanceToUnit(const CUnit &dest);

/// Can this unit-type attack the other (destination)
extern int CanTarget(const wyrmgus::unit_type &type, const wyrmgus::unit_type &dest);
/// Can transporter transport the other unit
extern int CanTransport(const CUnit &transporter, const CUnit &unit);
//Wyrmgus start
/// Can the unit pick up the other unit
extern bool CanPickUp(const CUnit &picker, const CUnit &unit);
//Wyrmgus end

/// Generate a unit reference, a printable unique string for unit
extern std::string UnitReference(const CUnit *unit);

/// save unit-structure
extern void SaveUnit(const CUnit &unit, CFile &file);

/// Initialize unit module
extern void InitUnits();
/// Clean unit module
extern void CleanUnits();

// in unit_draw.c
//--------------------
/// Draw nothing around unit
extern void DrawSelectionNone(IntColor, int, int, int, int);
/// Draw circle around unit
extern void DrawSelectionCircle(IntColor, int, int, int, int);
/// Draw circle filled with alpha around unit
extern void DrawSelectionCircleWithTrans(IntColor, int, int, int, int);
/// Draw rectangle around unit
extern void DrawSelectionRectangle(IntColor, int, int, int, int);
/// Draw rectangle filled with alpha around unit
extern void DrawSelectionRectangleWithTrans(IntColor, int, int, int, int);
/// Draw corners around unit
extern void DrawSelectionCorners(IntColor, int, int, int, int);

/// Register CCL decorations features
extern void DecorationCclRegister();
/// Get the amount of decorations
extern int GetDecorationsCount();
/// Load the decorations (health,mana) of units
extern void LoadDecorations();
/// Clean the decorations (health,mana) of units
extern void CleanDecorations();

/// Draw unit's shadow
extern void DrawShadow(const wyrmgus::unit_type &type, const std::shared_ptr<CGraphic> &sprite, int frame, const PixelPos &screenPos);
//Wyrmgus start
/// Draw unit's overlay
extern void DrawPlayerColorOverlay(const wyrmgus::unit_type &type, const std::shared_ptr<CPlayerColorGraphic> &sprite, const int player, int frame, const PixelPos &screenPos, const wyrmgus::time_of_day *time_of_day);
extern void DrawOverlay(const wyrmgus::unit_type &type, const std::shared_ptr<CGraphic> &sprite, int player, int frame, const PixelPos &screenPos, const wyrmgus::time_of_day *time_of_day);
//Wyrmgus end
/// Draw all units visible on map in viewport
extern int FindAndSortUnits(const CViewport &vp, std::vector<CUnit *> &table);

/// Show a unit's orders.
extern void ShowOrder(const CUnit &unit);

// in groups.c

/// Save groups
extern void SaveGroups(CFile &file);
/// Cleanup groups
extern void CleanGroups();
/// Get the array of units of a particular group
extern const std::vector<CUnit *> &GetUnitsOfGroup(int num);

/// Remove all units from a group
extern void ClearGroup(int num);
/// Add the array of units to the group
extern void AddToGroup(CUnit **units, unsigned int nunits, int num);
/// Set the contents of a particular group with an array of units
extern void SetGroup(CUnit **units, unsigned int nunits, int num);
/// Remove a unit from a group
extern void RemoveUnitFromGroups(CUnit &unit);
//Wyrmgus start
/// Remove a unit from a group which has more than one unit in it
extern void RemoveUnitFromNonSingleGroups(CUnit &unit);
//Wyrmgus end
/// Register CCL group features
extern void GroupCclRegister();
extern bool IsGroupTainted(int num);

// in selection.c

/// Check if unit is the currently only selected
extern bool IsOnlySelected(const CUnit &unit);

///  Save selection to restore after.
extern void SaveSelection();
///  Restore selection.
extern void RestoreSelection();
/// Clear current selection
extern void UnSelectAll();
/// Changed TeamUnit Selection
extern void ChangeTeamSelectedUnits(CPlayer &player, const std::vector<CUnit *> &units);
/// Add a unit to selection
extern int SelectUnit(CUnit &unit);
/// Select one unit as selection
extern void SelectSingleUnit(CUnit &unit);
/// Remove a unit from selection
extern void UnSelectUnit(CUnit &unit);
//Wyrmgus start
/// Check whether two units can be selected together
extern bool UnitCanBeSelectedWith(const CUnit &first_unit, const CUnit &second_unit);
//Wyrmgus end
/// Add a unit to selected if not already selected, remove it otherwise
extern int ToggleSelectUnit(CUnit &unit);
/// Select units from the same type (if selectable by rectangle)
//Wyrmgus start
//extern int SelectUnitsByType(CUnit &base);
extern int SelectUnitsByType(CUnit &base, bool only_visible = true);
//Wyrmgus end
/// Toggle units from the same type (if selectable by rectangle)
extern int ToggleUnitsByType(CUnit &base);
/// Select the units belonging to a particular group
extern int SelectGroup(int group_number, GroupSelectionMode mode = GroupSelectionMode::SELECTABLE_BY_RECTANGLE_ONLY);
/// Add the units from the same group as the one in parameter
extern int AddGroupFromUnitToSelection(const CUnit &unit);
/// Select the units from the same group as the one in parameter
extern int SelectGroupFromUnit(const CUnit &group_unit);
//Wyrmgus start
/// Select entire army
extern int SelectArmy();
//Wyrmgus end
/// Select the units in the selection rectangle
extern int SelectUnitsInRectangle(const PixelPos &corner_topleft, const PixelPos &corner_bottomright);
/// Select ground units in the selection rectangle
extern int SelectGroundUnitsInRectangle(const PixelPos &corner_topleft, const PixelPos &corner_bottomright);
/// Select flying units in the selection rectangle
extern int SelectAirUnitsInRectangle(const PixelPos &corner_topleft, const PixelPos &corner_bottomright);
/// Add the units in the selection rectangle to the current selection
extern int AddSelectedUnitsInRectangle(const PixelPos &corner_topleft, const PixelPos &corner_bottomright);
/// Add ground units in the selection rectangle to the current selection
extern int AddSelectedGroundUnitsInRectangle(const PixelPos &corner_topleft, const PixelPos &corner_bottomright);
/// Add flying units in the selection rectangle to the current selection
extern int AddSelectedAirUnitsInRectangle(const PixelPos &corner_topleft, const PixelPos &corner_bottomright);

/// Save current selection state
extern void SaveSelections(CFile &file);
/// Clean up selections
extern void CleanSelections();
/// Register CCL selection features
extern void SelectionCclRegister();

// in ccl_unit.c

/// register CCL units features
extern void UnitCclRegister();
