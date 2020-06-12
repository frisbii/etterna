#pragma once
#include <vector>
#include <string>

#include "Etterna/Models/NoteData/NoteDataStructures.h"

/* PRAISE ULBU FOR IT IS ITS GLORY THAT GIVES OUR LIVES MEANING */

enum hands
{
	left_hand,
	right_hand,
	num_hands,
};

static const std::string calc_params_xml = "Save/calc params.xml";
static const std::array<unsigned, num_hands> hand_col_ids = { 3, 12 };

// should be able to handle 1hr 54min easily
static const int max_intervals = 40000;

// holds pattern mods
static thread_local std::array<
  std::array<std::array<float, max_intervals>, NUM_CalcPatternMod>,
  num_hands>
  doot;

inline void
Smooth(std::array<float, max_intervals>& input, float neutral, int end_interval)
{
	float f1;
	float f2 = neutral;
	float f3 = neutral;

	for (int i = 0; i < end_interval; ++i) {
		f1 = f2;
		f2 = f3;
		f3 = input.at(i);
		input.at(i) = (f1 + f2 + f3) / 3.F;
	}
}

inline void
MSSmooth(std::array<float, max_intervals>& input,
		 float neutral,
		 int end_interval)
{
	float f1;
	float f2 = neutral;

	for (int i = 0; i < end_interval; ++i) {
		f1 = f2;
		f2 = input.at(i);
		input.at(i) = (f1 + f2) / 2.F;
	}
}

static const std::vector<CalcPatternMod> agnostic_mods = {
	Stream, JS, HS, CJ, CJDensity, FlamJam, TheThing, TheThing2,
};

static const std::vector<CalcPatternMod> dependent_mods = {
	OHJumpMod,		  Balance,		 Roll,
	OHTrill,		  VOHTrill,		 Chaos,
	WideRangeBalance, WideRangeRoll, WideRangeJumptrill,
	WideRangeAnchor,  RanMan,
};

struct PatternMods
{
	static inline void set_agnostic(const CalcPatternMod& pmod,
									const float& val,
									const int& pos)
	{
		doot.at(left_hand).at(pmod).at(pos) = val;
	}

	static inline void set_dependent(const int& hand,
									 const CalcPatternMod& pmod,
									 const float& val,
									 const int& pos)
	{
		doot.at(hand).at(pmod).at(pos) = val;
	}

	static inline void run_agnostic_smoothing_pass(const int& end_itv)
	{
		for (auto& pmod : agnostic_mods) {
			Smooth(doot.at(left_hand).at(pmod), neutral, end_itv);
		}
	}

	static inline void run_dependent_smoothing_pass(const int& end_itv)
	{
		for (auto& pmod : dependent_mods) {
			for (auto& h : doot) {
				{
					{
						Smooth(h.at(pmod), neutral, end_itv);
					}
				}
			}
		}
	}

	static inline void bruh_they_the_same(const int& end_itv)
	{
		for (auto& pmod : agnostic_mods) {
			for (int i = 0; i < end_itv; i++) {
				doot.at(right_hand).at(pmod).at(i) =
				  doot.at(left_hand).at(pmod).at(i);
			}
		}
	}
};

static thread_local int numitv = 0;

// uhhh
struct RowInfo
{
	unsigned row_notes = 0U;
	int row_count = 0U;
	float row_time = 0.F;
};

static thread_local std::
  array<std::array<RowInfo, max_rows_for_single_interval>, max_intervals>
	adj_ni;

static thread_local std::array<int, max_intervals> itv_size;
static const float interval_span = 0.5F;

// converts time to interval index, if there's an offset to add or a rate to
// scale by, it should be done prior
inline int
time_to_itv_idx(const float& time)
{
	return static_cast<int>(time / interval_span);
}

// checks to see if the noteinfo will fit in our static arrays, if it won't it's
// some garbage joke file and we can throw it out, setting values to 0
inline auto
fastwalk(const vector<NoteInfo>& ni,
		 const float& rate,
		 const float& offset = 0.F) -> bool
{
	// add 1 to convert index to size, we're just using this to guess due to
	// potential float precision differences, the actual numitv will be set at
	// the end
	numitv = time_to_itv_idx(ni.back().rowTime / rate) + 1;

	// are there more intervals than our alloted max
	if (numitv >= max_intervals)
		return true;

	//// for various reasons we actually have to do this, scan the file and make
	//// sure each successive row time is greater than the last
	// for (int i = 1; i < ni.size(); ++i) {
	//	if (ni.at(i - 1).rowTime >= ni.at(i).rowTime) {
	//		return true;
	//	}
	//}

	// now we can attempt to construct notinfo that includes column count and
	// rate adjusted row time, both of which are derived data that both pmod
	// loops require
	int itv = 0;
	int last_itv = 0;
	int row_counter = 0;
	float scaled_time = 0.F;
	for (int i = 0; i < ni.size(); ++i) {

		// it's at least 25 nps per finger, throw it out
		if (row_counter >= max_rows_for_single_interval)
			return true;

		const auto& ri = ni.at(i);

		float zoop = (ni.at(i).rowTime + offset) / rate;

		if (i > 0)
			assert(zoop > scaled_time);

		scaled_time = (ni.at(i).rowTime + offset) / rate;

		// set current interval and current scaled time
		itv = time_to_itv_idx(scaled_time);

		// new interval, reset row counter and set new last interval
		if (itv > last_itv) {

			// we're using static arrays so if we skip over some empty intervals
			// we have to go back and set their row counts to 0
			if (itv - last_itv > 1) {
				for (int j = last_itv + 1; j < itv; ++j) {
					itv_size.at(j) = 0;
				}
			}

			itv_size.at(last_itv) = row_counter;

			last_itv = itv;
			row_counter = 0;
		}

		adj_ni.at(itv).at(row_counter).row_notes = ri.notes;
		adj_ni.at(itv).at(row_counter).row_count = column_count(ri.notes);
		adj_ni.at(itv).at(row_counter).row_time = scaled_time;

		++row_counter;
	}

	// take care to set the proper values for the last row, the set logic block
	// won't be hit on it
	if (itv - last_itv > 1) {
		for (int j = last_itv + 1; j < itv; ++j) {
			itv_size.at(j) = 0;
		}
	}

	itv_size.at(itv) = row_counter;

	// make sure we only set up to the interval/row we actually use
	numitv = itv + 1;
	
	return false;
}
