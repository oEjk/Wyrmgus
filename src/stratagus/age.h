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
//      (c) Copyright 2018-2022 by Andrettin
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

#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

class CPlayer;

namespace wyrmgus {

class resource_icon;

template <typename scope_type>
class condition;

class age final : public named_data_entry, public data_type<age>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::resource_icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(int priority MEMBER priority READ get_priority)

public:
	static constexpr const char *class_identifier = "age";
	static constexpr const char property_class_identifier[] = "wyrmgus::age*";
	static constexpr const char *database_folder = "ages";

	static void initialize_all();

	explicit age(const std::string &identifier);
	virtual ~age() override;
	
	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const resource_icon *get_icon() const
	{
		return this->icon;
	}

	int get_priority() const
	{
		return this->priority;
	}

	const std::unique_ptr<condition<CPlayer>> &get_preconditions() const
	{
		return this->preconditions;
	}

	const std::unique_ptr<condition<CPlayer>> &get_conditions() const
	{
		return this->conditions;
	}

signals:
	void changed();

private:
	resource_icon *icon = nullptr;
	int priority = 0;
	std::unique_ptr<condition<CPlayer>> preconditions;
	std::unique_ptr<condition<CPlayer>> conditions;
};

}
