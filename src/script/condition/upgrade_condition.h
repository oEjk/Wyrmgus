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
//      (c) Copyright 2020-2022 by Andrettin
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

#include "script/condition/upgrade_condition_base.h"

namespace wyrmgus {

class upgrade_condition final : public upgrade_condition_base
{
public:
	upgrade_condition()
	{
	}

	explicit upgrade_condition(const std::string &value)
	{
		this->upgrade = CUpgrade::get(value);
	}

	virtual void ProcessConfigDataProperty(const std::pair<std::string, std::string> &property) override;

	virtual bool check(const civilization *civilization) const override
	{
		return this->check_upgrade(civilization, this->upgrade);
	}

	virtual bool check(const CPlayer *player, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return this->check_upgrade(player, this->upgrade);
	}

	virtual bool check(const CUnit *unit, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return this->check_upgrade(unit, this->upgrade);
	}

	virtual std::string get_string(const size_t indent, const bool links_allowed) const override
	{
		Q_UNUSED(indent)

		return condition::get_object_string(this->upgrade, links_allowed) + " upgrade";
	}

private:
	const CUpgrade *upgrade = nullptr;
};

}
