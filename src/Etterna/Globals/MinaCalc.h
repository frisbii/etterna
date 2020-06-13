#pragma once
#include "Etterna/Models/NoteData/NoteDataStructures.h"
#include <vector>

// For internal, must be preprocessor defined
#if defined(MINADLL_COMPILE) && defined(_WIN32)
#define MINACALC_API __declspec(dllexport)
#endif

// For Stepmania
#ifndef MINACALC_API
#define MINACALC_API
#endif

typedef std::vector<std::vector<float>> MinaSD;

class Hand
{
  public:
	// I don't know why this was ever being done in the internal loop, only the
	// stam adjusted difficulties were dependent on a player_skill input, these
	// values are static. Just calculate them for each skillset after pattern
	// mods are done. For reasons we want to calculate stam mod on a different
	// vector than what we apply the stam mod to, so calculate those as well.
	// Yes this makes sense.
	void InitAdjDiff();

	/*	The stamina model works by asserting a minimum difficulty relative to
	the supplied player skill level for which the player's stamina begins to
	wane. Experience in both gameplay and algorithm testing has shown the
	appropriate value to be around 0.8. The multiplier is scaled to the
	proportionate difference in player skill. */
	// just recycle the stam_adj_diff vector directly in this function
	// sometimes
	void StamAdjust(float x, int ss, bool debug = false);

	/*	For a given player skill level x, invokes the function used by wife
	scoring to assert the average of the distribution of point gain for each
	interval and then tallies up the result to produce an average total number
	of points achieved by this hand. */
	// return value is true if a player can no longer reach scoregoal even if
	// they get 100% of the remaining points
	void CalcInternal(float& gotpoints,
					  float& x,
					  int ss,
					  bool stam,
					  bool debug = false);

	std::vector<std::vector<std::vector<float>>> debugValues;

	int hi = 0;
};

class Calc
{
  public:
	/*	Primary calculator function that wraps everything else. Initializes the
	hand objects and then runs the chisel function under varying circumstances
	to estimate difficulty for each different skillset. Currently only
	overall/stamina are being produced. */
	auto CalcMain(const std::vector<NoteInfo>& NoteInfo,
				  float music_rate,
				  float score_goal) -> std::vector<float>;

	bool debugmode = false;
	bool ssr = true; // set to true for scores, false for cache

	/*	Splits up the chart by each hand and calls ProcessFinger on each "track"
	before passing the results to the hand initialization functions. Also passes
	the input timingscale value. Hardcode a limit for nps and if we hit it just
	return max value for the calc and move on, there's no point in calculating
	values for 500 nps joke files. Mem optimization can be better if we only
	allow 100 maximum notes for a single half second interval, return value is
	whether or not we should continue calculation */
	auto InitializeHands(const std::vector<NoteInfo>& NoteInfo,
						 float music_rate,
						 float offset) -> bool;

	/*	Returns estimate of player skill needed to achieve score goal on chart.
	 *  The player_skill parameter gives an initial guess and floor for player
	 * skill. Resolution relates to how precise the answer is. Additional
	 * parameters give specific skill sets being tested for.*/
	auto Chisel(float player_skill,
				float resolution,
				float score_goal,
				int ss, // skillset
				bool stamina,
				bool debugoutput = false) -> float;
	Hand l_hand;
	Hand r_hand;

  private:
	// Derivative calc params
	int MaxPoints = 0;	 // Total points achievable in the file
	void TotalMaxPoints(); // Counts up the total points and assigns it
};

MINACALC_API auto
MinaSDCalc(const std::vector<NoteInfo>& NoteInfo, float musicrate, float goal)
  -> std::vector<float>;
MINACALC_API auto
MinaSDCalc(const std::vector<NoteInfo>& NoteInfo) -> MinaSD;
MINACALC_API void
MinaSDCalcDebug(
  const std::vector<NoteInfo>& NoteInfo,
  float musicrate,
  float goal,
  std::vector<std::vector<std::vector<std::vector<float>>>>& handInfo);
MINACALC_API auto
GetCalcVersion() -> int;
