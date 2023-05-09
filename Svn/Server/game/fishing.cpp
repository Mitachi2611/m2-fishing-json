// Srcs/Server/game/src/fishing.cpp

// search:

#include "unique_item.h"

// and add below, this:

#include <fstream>
#include <filesystem>
#include <array>
#include <fmt/core.h>
#include <nlohmann/json.hpp>

// search:
		FISH_NAME_MAX_LEN = 64,
		MAX_PROB = 4,

// and add below, this:

		LENGTH_RANGE = 3,

// search:
	struct SFishInfo
	{
		[...]
	};

// and replace with:

	struct SFishInfo
	{
		char name[FISH_NAME_MAX_LEN];

		DWORD vnum;
		DWORD dead_vnum;
		DWORD grill_vnum;

		std::array<int, MAX_PROB> prob;

		int difficulty;
		int time_type;

		std::array<int, LENGTH_RANGE> length_range; // MIN MAX EXTRA_MAX : 99% MIN~MAX, 1% MAX~EXTRA_MAX
		std::array<int, NUM_USE_RESULT_COUNT> used_table;
		// 6000 2000 1000 500 300 100 50 30 10 5 4 1
	};

// search:

	void Initialize()
	{
		[...]
	}

// and replace the whole function with:

	void Initialize()
	{
		// Make a backup
		SFishInfo fish_info_bak[MAX_FISH]{};
		std::memcpy(fish_info_bak, fish_info, sizeof(fish_info));

		// Load json
		const std::filesystem::path fishing_file_path = LocaleService_GetBasePath() + "/fishing.json";
		if (!std::filesystem::exists(fishing_file_path)) {
			sys_err("error! cannot open fishing.json");
			return;
		}

		// Clean data
		std::memset(fish_info, 0, sizeof(fish_info));

		// Reading json
		std::ifstream file(fishing_file_path);
		nlohmann::json data;
		try {
			file >> data;
		} // If can't load json and have backups, restore them
		catch (const std::exception&) {
			if (std::any_of(std::begin(fish_info_bak), std::end(fish_info_bak), [](const SFishInfo& info) { return info.name[0] != '\0'; })) {
				std::memcpy(fish_info, fish_info_bak, sizeof(fish_info));
				SendLog("Restoring backup");
			}
			return;
		}

		// Emplace data
		int idx = 0;
		for (const auto& [key, value] : data.items())
		{
			std::strncpy(fish_info[idx].name, value["name"].get<std::string>().c_str(), sizeof(fish_info[idx].name) - 1);
			fish_info[idx].name[sizeof(fish_info[idx].name) - 1] = '\0';

			fish_info[idx].vnum = value["vnum"].get<unsigned long>();
			fish_info[idx].dead_vnum = value["dead_vnum"].get<unsigned long>();
			fish_info[idx].grill_vnum = value["grill_vnum"].get<unsigned long>();

			std::copy_n(value["prob"].get<std::array<int, MAX_PROB>>().begin(),
						MAX_PROB,
						fish_info[idx].prob.begin());

			fish_info[idx].difficulty = value["difficulty"].get<int>();
			fish_info[idx].time_type = value["time_type"].get<int>();

			std::copy_n(value["length_range"].get<std::array<int, LENGTH_RANGE>>().begin(),
						LENGTH_RANGE,
						fish_info[idx].length_range.begin());


			std::copy_n(value["used_table"].get<std::array<int, NUM_USE_RESULT_COUNT>>().begin(),
						NUM_USE_RESULT_COUNT,
						fish_info[idx].used_table.begin());

			++idx;
			if (idx == MAX_FISH)
				break;
		}

		// Print data
		for (int i = 0; i < MAX_FISH; ++i) {
			sys_log(0, "FISH: %-24s vnum %5lu prob %4d %4d %4d %4d len %d %d %d",
				fish_info[i].name,
				fish_info[i].vnum,
				fish_info[i].prob[0],
				fish_info[i].prob[1],
				fish_info[i].prob[2],
				fish_info[i].prob[3],
				fish_info[i].length_range[0],
				fish_info[i].length_range[1],
				fish_info[i].length_range[2]);
		}

		// Prob table
		for (int j = 0; j < MAX_PROB; ++j)
		{
			g_prob_accumulate[j][0] = fish_info[0].prob[j];

			for (int i = 1; i < MAX_FISH; ++i)
				g_prob_accumulate[j][i] = fish_info[i].prob[j] + g_prob_accumulate[j][i - 1];

			g_prob_sum[j] = g_prob_accumulate[j][MAX_FISH - 1];
			sys_log(0, "FISH: prob table %d %d", j, g_prob_sum[j]);
		}
	}