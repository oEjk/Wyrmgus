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
//      (c) Copyright 2018-2021 by Andrettin
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

#include "map/map_template_container.h"
#include "vec2i.h"

class CUnit;
struct lua_State;

static int CclStratagusMap(lua_State *l);

namespace wyrmgus {
	class plane;
	class scheduled_season;
	class scheduled_time_of_day;
	class season;
	class season_schedule;
	class tile;
	class time_of_day;
	class time_of_day_schedule;
	class world;
}

class CMapLayer final : public QObject
{
	Q_OBJECT

public:
	explicit CMapLayer(const QSize &size);

	explicit CMapLayer(const int width, const int height) : CMapLayer(QSize(width, height))
	{
	}

	~CMapLayer();
	
	/**
	**	@brief	Get the map field at a given location
	**
	**	@param	index	The index of the map field
	**
	**	@return	The map field
	*/
	wyrmgus::tile *Field(const unsigned int index) const;
	
	/**
	**	@brief	Get the map field at a given location
	**
	**	@param	x	The x coordinate of the map field
	**	@param	y	The y coordinate of the map field
	**
	**	@return	The map field
	*/
	wyrmgus::tile *Field(const int x, const int y) const
	{
		return this->Field(x + y * this->get_width());
	}
	
	/**
	**	@brief	Get the map field at a given location
	**
	**	@param	pos	The coordinates of the map field
	**
	**	@return	The map field
	*/
	wyrmgus::tile *Field(const Vec2i &pos) const
	{
		return this->Field(pos.x, pos.y);
	}
	
	Vec2i GetPosFromIndex(unsigned int index) const
	{
		Vec2i pos;
		pos.x = index % this->get_width();
		pos.y = index / this->get_width();
		return pos;
	}

	const QSize &get_size() const
	{
		return this->size;
	}
	
	int get_width() const
	{
		return this->get_size().width();
	}
	
	int get_height() const
	{
		return this->get_size().height();
	}
	
	void DoPerCycleLoop();
	void DoPerHourLoop();
	void handle_destroyed_overlay_terrain();
	void decay_destroyed_overlay_terrain_tile(const QPoint &pos);
	void regenerate_forests();
	void regenerate_tree_tile(const QPoint &pos);

	const wyrmgus::time_of_day_schedule *get_time_of_day_schedule() const
	{
		return this->time_of_day_schedule;
	}

	void set_time_of_day_schedule(const time_of_day_schedule *schedule)
	{
		this->time_of_day_schedule = schedule;
	}

private:
	void DecrementRemainingTimeOfDayHours();
	void IncrementTimeOfDay();

public:
	void SetTimeOfDayByHours(const unsigned long long hours);

	const scheduled_time_of_day *get_scheduled_time_of_day() const
	{
		return this->time_of_day;
	}

	void SetTimeOfDay(const scheduled_time_of_day *time_of_day);

	const wyrmgus::time_of_day *GetTimeOfDay() const;
	const wyrmgus::time_of_day *get_tile_time_of_day(const int tile_index) const;
	const wyrmgus::time_of_day *get_tile_time_of_day(const QPoint &tile_pos) const;

	const wyrmgus::season_schedule *get_season_schedule() const
	{
		return this->season_schedule;
	}

	void set_season_schedule(const season_schedule *schedule)
	{
		this->season_schedule = schedule;
	}

private:
	void DecrementRemainingSeasonHours();
	void IncrementSeason();

public:
	void SetSeasonByHours(const unsigned long long hours);

	const scheduled_season *get_scheduled_season() const
	{
		return this->season;
	}

	void SetSeason(const scheduled_season *season);
	const wyrmgus::season *GetSeason() const;
	const wyrmgus::season *get_tile_season(const int tile_index) const;
	const wyrmgus::season *get_tile_season(const QPoint &tile_pos) const;

	bool has_subtemplate_area(const wyrmgus::map_template *map_template) const
	{
		return this->subtemplate_areas.contains(map_template);
	}

	const QRect &get_subtemplate_rect(const wyrmgus::map_template *map_template) const
	{
		static QRect empty_rect;

		auto find_iterator = this->subtemplate_areas.find(map_template);
		if (find_iterator != this->subtemplate_areas.end()) {
			return find_iterator->second;
		}

		return empty_rect;
	}
	
	int ID = -1;
private:
	std::unique_ptr<wyrmgus::tile[]> Fields; //fields on the map layer
	QSize size;									/// the size in tiles of the map layer
	const scheduled_time_of_day *time_of_day = nullptr;	/// the time of day for the map layer
	const wyrmgus::time_of_day_schedule *time_of_day_schedule = nullptr; //the time of day schedule for the map layer
public:
	int RemainingTimeOfDayHours = 0;			/// the quantity of hours remaining for the current time of day to end
private:
	const scheduled_season *season = nullptr;			/// the current season for the map layer
	const wyrmgus::season_schedule *season_schedule = nullptr;	/// the season schedule for the map layer
public:
	int RemainingSeasonHours = 0;				/// the quantity of hours remaining for the current season to end
	const wyrmgus::plane *plane = nullptr;			/// the plane pointer (if any) for the map layer
	const wyrmgus::world *world = nullptr;			/// the world pointer (if any) for the map layer
	std::vector<CUnit *> LayerConnectors;		/// connectors in the map layer which lead to other map layers
	wyrmgus::map_template_map<QRect> subtemplate_areas;
	std::vector<QPoint> destroyed_overlay_terrain_tiles; /// destroyed overlay terrain tiles (excluding trees)
	std::vector<QPoint> destroyed_tree_tiles;	/// destroyed tree tiles; this list is used for forest regeneration

	friend int CclStratagusMap(lua_State *l);
};
