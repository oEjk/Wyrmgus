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
//      (c) Copyright 2020 by Andrettin
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

#include "database/data_type.h"
#include "database/detailed_data_entry.h"

class CUpgrade;

namespace stratagus {

class condition;
class faction;
class icon;

class dynasty final : public detailed_data_entry, public data_type<dynasty>
{
	Q_OBJECT

	Q_PROPERTY(CUpgrade* upgrade MEMBER upgrade READ get_upgrade)
	Q_PROPERTY(stratagus::icon* icon MEMBER icon READ get_icon)
	Q_PROPERTY(QVariantList factions READ get_factions_qvariant_list)

public:
	static constexpr const char *class_identifier = "dynasty";
	static constexpr const char *database_folder = "dynasties";

	explicit dynasty(const std::string &identifier);
	~dynasty();

	virtual void process_sml_scope(const sml_data &scope) override;

	CUpgrade *get_upgrade() const
	{
		return this->upgrade;
	}

	icon *get_icon() const
	{
		return this->icon;
	}

	const std::vector<faction *> &get_factions() const
	{
		return this->factions;
	}

	QVariantList get_factions_qvariant_list() const;

	Q_INVOKABLE void add_faction(faction *faction);
	Q_INVOKABLE void remove_faction(faction *faction);

	const std::unique_ptr<condition> &get_preconditions() const
	{
		return this->preconditions;
	}

	const std::unique_ptr<condition> &get_conditions() const
	{
		return this->conditions;
	}

private:
	CUpgrade *upgrade = nullptr; //dynasty upgrade applied when the dynasty is set
	icon *icon = nullptr;
	std::vector<faction *> factions; //to which factions is this dynasty available
	std::unique_ptr<condition> preconditions;
	std::unique_ptr<condition> conditions;
};

}
