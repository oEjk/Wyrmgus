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
//      (c) Copyright 2019-2020 by Andrettin
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

#include "character.h" //for MaxCharacterTitles
#include "database/data_type.h"
#include "database/detailed_data_entry.h"
#include "faction_tier.h"
#include "gender.h"
#include "player.h" //for certain enums

class CAiBuildingTemplate;
class CCurrency;
class CDynasty;
class CForceTemplate;
class CUpgrade;
class LuaCallback;

int CclDefineFaction(lua_State *l);

namespace stratagus {

class character;
class civilization;
class deity;
class icon;
class resource;
class unit_class;
class unit_type;
class upgrade_class;
enum class diplomacy_state;
enum class faction_tier;
enum class government_type;

class faction final : public detailed_data_entry, public data_type<faction>
{
	Q_OBJECT

	Q_PROPERTY(stratagus::civilization* civilization MEMBER civilization READ get_civilization)
	Q_PROPERTY(stratagus::icon* icon MEMBER icon READ get_icon)
	Q_PROPERTY(stratagus::player_color* color MEMBER color READ get_color)
	Q_PROPERTY(stratagus::faction_tier default_tier MEMBER default_tier READ get_default_tier)
	Q_PROPERTY(stratagus::faction_tier min_tier MEMBER min_tier READ get_min_tier)
	Q_PROPERTY(stratagus::faction_tier max_tier MEMBER max_tier READ get_max_tier)
	Q_PROPERTY(stratagus::government_type default_government_type MEMBER default_government_type READ get_default_government_type)
	Q_PROPERTY(stratagus::site* default_capital MEMBER default_capital READ get_default_capital)
	Q_PROPERTY(bool simple_name MEMBER simple_name READ uses_simple_name)
	Q_PROPERTY(bool short_name MEMBER short_name READ uses_short_name)
	Q_PROPERTY(stratagus::faction_tier tier MEMBER tier READ get_tier)
	Q_PROPERTY(stratagus::government_type government_type MEMBER government_type READ get_government_type)
	Q_PROPERTY(stratagus::site* capital MEMBER capital READ get_capital)
	Q_PROPERTY(QVariantList acquired_upgrades READ get_acquired_upgrades_qstring_list)

public:
	using title_name_map = std::map<government_type, std::map<faction_tier, std::string>>;
	using character_title_name_map = std::map<int, std::map<government_type, std::map<faction_tier, std::map<gender, std::string>>>>;

	static constexpr const char *class_identifier = "faction";
	static constexpr const char *database_folder = "factions";

	static faction *add(const std::string &identifier, const stratagus::module *module)
	{
		faction *faction = data_type::add(identifier, module);
		faction->ID = faction::get_all().size() - 1;
		return faction;
	}

	static void process_title_names(title_name_map &title_names, const sml_data &scope);
	static void process_title_name_scope(title_name_map &title_names, const sml_data &scope);
	static void process_character_title_name_scope(character_title_name_map &character_title_names, const sml_data &scope);
	static void process_character_title_name_scope(std::map<government_type, std::map<faction_tier, std::map<gender, std::string>>> &character_title_names, const sml_data &scope);
	static void process_character_title_name_scope(std::map<faction_tier, std::map<gender, std::string>> &character_title_names, const sml_data &scope);

	explicit faction(const std::string &identifier);
	~faction();

	virtual void process_sml_property(const sml_property &property) override;
	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void process_sml_dated_scope(const sml_data &scope, const QDateTime &date) override;
	virtual void initialize() override;
	virtual void check() const override;

	virtual void reset_history() override
	{
		this->tier = this->get_default_tier();
		this->government_type = this->get_default_government_type();
		this->capital = this->get_default_capital();
		this->resources.clear();
		this->diplomacy_states.clear();
		this->acquired_upgrades.clear();
	}

	civilization *get_civilization() const
	{
		return this->civilization;
	}

	icon *get_icon() const
	{
		return this->icon;
	}
	
	CCurrency *GetCurrency() const;

	faction_tier get_default_tier() const
	{
		return this->default_tier;
	}

	faction_tier get_min_tier() const
	{
		return this->min_tier;
	}

	faction_tier get_max_tier() const
	{
		return this->max_tier;
	}

	government_type get_default_government_type() const
	{
		return this->default_government_type;
	}

	player_color *get_color() const
	{
		return this->color;
	}

	site *get_default_capital() const
	{
		return this->default_capital;
	}

	bool uses_simple_name() const;

	bool uses_short_name() const
	{
		return this->short_name;
	}

	std::string_view get_title_name(const government_type government_type, const faction_tier tier) const;
	std::string_view get_character_title_name(const int title_type, const government_type government_type, const faction_tier tier, const gender gender) const;

	int GetUpgradePriority(const CUpgrade *upgrade) const;
	int GetForceTypeWeight(const ForceType force_type) const;
	std::vector<CForceTemplate *> GetForceTemplates(const ForceType force_type) const;
	std::vector<CAiBuildingTemplate *> GetAiBuildingTemplates() const;
	const std::vector<std::string> &get_ship_names() const;

	unit_type *get_class_unit_type(const unit_class *unit_class) const;

	void set_class_unit_type(const unit_class *unit_class, unit_type *unit_type)
	{
		if (unit_type == nullptr) {
			this->class_unit_types.erase(unit_class);
			return;
		}

		this->class_unit_types[unit_class] = unit_type;
	}

	void remove_class_unit_type(unit_type *unit_type)
	{
		for (std::map<const unit_class *, stratagus::unit_type *>::reverse_iterator iterator = this->class_unit_types.rbegin(); iterator != this->class_unit_types.rend(); ++iterator) {
			if (iterator->second == unit_type) {
				this->class_unit_types.erase(iterator->first);
			}
		}
	}

	bool is_class_unit_type(const unit_type *unit_type) const;

	CUpgrade *get_class_upgrade(const upgrade_class *upgrade_class) const;

	void set_class_upgrade(const upgrade_class *upgrade_class, CUpgrade *upgrade)
	{
		if (upgrade == nullptr) {
			this->class_upgrades.erase(upgrade_class);
			return;
		}

		this->class_upgrades[upgrade_class] = upgrade;
	}

	void remove_class_upgrade(CUpgrade *upgrade)
	{
		for (std::map<const upgrade_class *, CUpgrade *>::reverse_iterator iterator = this->class_upgrades.rbegin(); iterator != this->class_upgrades.rend(); ++iterator) {
			if (iterator->second == upgrade) {
				this->class_upgrades.erase(iterator->first);
			}
		}
	}

	const std::vector<CFiller> &get_ui_fillers() const;

	const std::vector<character *> &get_characters() const
	{
		return this->characters;
	}

	void add_character(character *character)
	{
		this->characters.push_back(character);
	}

	faction_tier get_tier() const
	{
		return this->tier;
	}

	government_type get_government_type() const
	{
		return this->government_type;
	}

	site *get_capital() const
	{
		return this->capital;
	}

	const std::map<const resource *, int> &get_resources() const
	{
		return this->resources;
	}

	const std::map<const faction *, diplomacy_state> &get_diplomacy_states() const
	{
		return this->diplomacy_states;
	}

	const std::vector<CUpgrade *> &get_acquired_upgrades() const
	{
		return this->acquired_upgrades;
	}

	QVariantList get_acquired_upgrades_qstring_list() const;

	Q_INVOKABLE void add_acquired_upgrade(CUpgrade *upgrade)
	{
		this->acquired_upgrades.push_back(upgrade);
	}

	Q_INVOKABLE void remove_acquired_upgrade(CUpgrade *upgrade);

	std::string FactionUpgrade;											/// faction upgrade applied when the faction is set
	std::string Adjective;												/// adjective pertaining to the faction
	std::string DefaultAI = "land-attack";
	int ID = -1;														/// faction ID
private:
	stratagus::civilization *civilization = nullptr; //faction civilization
public:
	int Type = FactionTypeNoFactionType;								/// faction type (i.e. tribe or polity)
private:
	faction_tier default_tier;
	faction_tier min_tier;
	faction_tier max_tier;
	government_type default_government_type;
public:
	int ParentFaction = -1;												/// parent faction of this faction
	bool Playable = true;												/// faction playability
	bool DefiniteArticle = false;										/// whether the faction's name should be preceded by a definite article (e.g. "the Netherlands")
	icon *icon = nullptr;												/// Faction's icon
	CCurrency *Currency = nullptr;										/// The faction's currency
	deity *HolyOrderDeity = nullptr;									/// deity this faction belongs to, if it is a holy order
	LuaCallback *Conditions = nullptr;
private:
	player_color *color = nullptr; /// faction color
	site *default_capital = nullptr;
	bool simple_name = false;
	bool short_name = false;
public:
	std::vector<faction *> DevelopsFrom;								/// from which factions can this faction develop
	std::vector<faction *> DevelopsTo;									/// to which factions this faction can develop
	std::vector<CDynasty *> Dynasties;									/// which dynasties are available to this faction
private:
	title_name_map title_names;
	character_title_name_map character_title_names;
public:
	std::map<const CUpgrade *, int> UpgradePriorities;					/// Priority for each upgrade
	std::map<ButtonCmd, IconConfig> ButtonIcons;								/// icons for button actions
private:
	std::map<const unit_class *, unit_type *> class_unit_types; //the unit type slot of a particular class for the faction
	std::map<const upgrade_class *, CUpgrade *> class_upgrades; //the upgrade slot of a particular class for the faction
public:
	std::vector<std::string> ProvinceNames;								/// Province names for the faction
private:
	std::vector<std::string> ship_names;								/// Ship names for the faction
	std::vector<CFiller> ui_fillers;
public:
	std::vector<site *> Cores; /// Core sites of this faction (required to found it)
	std::vector<site *> sites; /// Sites used for this faction if it needs a randomly-generated settlement
private:
	std::vector<character *> characters;
public:
	std::map<ForceType, std::vector<CForceTemplate *>> ForceTemplates;		/// Force templates, mapped to each force type
	std::map<ForceType, int> ForceTypeWeights;								/// Weights for each force type
	std::vector<CAiBuildingTemplate *> AiBuildingTemplates;				/// AI building templates
	std::map<std::tuple<CDate, CDate, int>, character *> HistoricalMinisters;	/// historical ministers of the faction (as well as heads of state and government), mapped to the beginning and end of the rule, and the enum of the title in question
	std::map<std::string, std::map<CDate, bool>> HistoricalUpgrades;	/// historical upgrades of the faction, with the date of change
	std::map<int, faction_tier> HistoricalTiers; /// dates in which this faction's tier changed; faction tier mapped to year
	std::map<int, government_type> HistoricalGovernmentTypes;						/// dates in which this faction's government type changed; government type mapped to year
private:
	faction_tier tier;
	government_type government_type;
	site *capital = nullptr;
	std::map<const resource *, int> resources;
	std::map<const faction *, diplomacy_state> diplomacy_states;
	std::vector<CUpgrade *> acquired_upgrades;
public:
	std::map<std::pair<CDate, faction *>, diplomacy_state> HistoricalDiplomacyStates;	/// dates in which this faction's diplomacy state to another faction changed; diplomacy state mapped to year and faction
	std::map<std::pair<CDate, int>, int> HistoricalResources;	/// dates in which this faction's storage of a particular resource changed; resource quantities mapped to date and resource
	std::vector<std::pair<CDate, std::string>> HistoricalCapitals;		/// historical capitals of the faction; the values are: date and settlement ident
	std::string Mod;							/// To which mod (or map), if any, this faction belongs

	friend int ::CclDefineFaction(lua_State *l);
};

}
