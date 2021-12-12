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
//      (c) Copyright 2021 by Andrettin
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

#include "economy/resource.h"
#include "economy/resource_storage_type.h"
#include "script/effect/effect.h"
#include "util/string_util.h"

namespace wyrmgus {

class resource;

class experience_effect final : public effect<CUnit>
{
public:
	explicit experience_effect(const std::string &value, const sml_operator effect_operator)
		: effect(effect_operator)
	{
		this->quantity = std::stoi(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "experience";
		return identifier;
	}

	virtual void do_assignment_effect(CUnit *unit) const override
	{
		unit->set_experience(this->quantity);
	}

	virtual void do_addition_effect(CUnit *unit) const override
	{
		unit->change_experience(this->quantity);
	}

	virtual void do_subtraction_effect(CUnit *unit) const override
	{
		unit->change_experience(-this->quantity);
	}

	virtual std::string get_assignment_string() const override
	{
		return "Set experience to " + std::to_string(this->quantity);
	}

	virtual std::string get_addition_string() const override
	{
		return "Gain " + std::to_string(this->quantity) + " experience";
	}

	virtual std::string get_subtraction_string() const override
	{
		return "Lose " + std::to_string(this->quantity) + " experience";
	}

private:
	int quantity = 0;
};

}
