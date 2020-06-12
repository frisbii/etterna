#pragma once
#include <array>
#include "PatternModHelpers.h"

/* custom moving window container that can do basic statistical operations on a
 * dynamic window */

static const int max_moving_window_size = 6;

// applies to acca and cccccc as well
static const int ccacc_timing_check_size = 3;

template<typename T>
struct CalcMovingWindow
{
	// ok there's actually a good reason for indexing this way because it's more
	// intuitive since we are scanning row by row the earliest values in the
	// window are the oldest
	inline void operator()(const T& new_val)
	{
		// update the window
		for (int i = 1; i < max_moving_window_size; ++i) {
			_itv_vals.at(i - 1) = _itv_vals.at(i);
		}

		// set new value at size - 1
		_itv_vals.at(max_moving_window_size - 1) = new_val;
	}

	// return type T
	inline auto operator[](const int& pos) const -> T
	{
		assert(pos > 0 && pos < max_moving_window_size);
		return _itv_vals.at(pos);
	}

	// return type T
	[[nodiscard]] inline auto get_now() const -> T { return _itv_vals.back(); }
	[[nodiscard]] inline auto get_last() const -> T
	{
		return _itv_vals.at(max_moving_window_size - 2);
	}

	// return type T
	[[nodiscard]] inline auto get_total_for_window(const int& window) const -> T
	{
		T o = static_cast<T>(0);
		int i = max_moving_window_size;
		while (i > max_moving_window_size - window) {
			--i;
			o += _itv_vals.at(i);
		}

		return o;
	}

	// return type T
	[[nodiscard]] inline auto get_max_for_window(const int& window) const -> T
	{
		T o = static_cast<T>(0);
		int i = max_moving_window_size;
		while (i > max_moving_window_size - window) {
			--i;
			o = _itv_vals.at(i) > o ? _itv_vals.at(i) : o;
		}

		return o;
	}

	// return type float
	[[nodiscard]] inline auto get_mean_of_window(const int& window) const
	  -> float
	{
		T o = static_cast<T>(0);

		int i = max_moving_window_size;
		while (i > max_moving_window_size - window) {
			--i;
			o += _itv_vals.at(i);
		}

		return static_cast<float>(o) / static_cast<float>(window);
	}

	// return type float
	[[nodiscard]] inline auto get_total_for_windowf(const int& window) const
	  -> float
	{
		float o = 0.F;
		int i = max_moving_window_size;
		while (i > max_moving_window_size - window) {
			--i;
			o += _itv_vals.at(i);
		}

		return o;
	}

	// return type float
	[[nodiscard]] inline auto get_cv_of_window(const int& window) const -> float
	{
		float sd = 0.F;
		float avg = get_mean_of_window(window);

		assert(avg > 0.F);

		// if window is 4, we check values 6/5/4/3, since this window is always
		// 6
		int i = max_moving_window_size;
		while (i > max_moving_window_size - window) {
			--i;
			sd += (static_cast<float>(_itv_vals.at(i)) - avg) *
				  (static_cast<float>(_itv_vals.at(i)) - avg);
		}

		return fastsqrt(sd / static_cast<float>(window)) / avg;
	}

	/* cv checks for pattern detection */

	// comment moved from wrjt
	/* we don't want to suppress actual streams that use this pattern, so we
	 * will keep a fairly tight requirement on the ms variance we are currently
	 * assuming we have xyyx always, and are not interested in xxyy as a part of
	 * xxyyxxyyxx transitions, given these conditions we can do some pretty neat
	 * stuff seq_ms 0 and 2 will both be cross column ms values, or left->right
	 * / right->left, seq_ms 1 will always be an anchor value, so, right->right
	 * for example. our interest is in hard nerfing long chains of xyyx patterns
	 * that won't get picked up by any of the roll scalers or balance scalers,
	 * but are still jumptrillable, for this condition to be true the anchor ms
	 * length has to be within a certain ratio of the cross column ms lengths,
	 * enabling the cross columns to be hit like jumptrilled flams, the optimal
	 * ratio for inflating nps is about 3:1, this is short enough that the nps
	 * boost is still high, but long enough that it doesn't become endless
	 * minijacking on the anchor given these conditions we can divide seq_ms[1]
	 * by 3 and cv check the array, with 2 identical values cc values, even if
	 * the anchor ratio floats between 2:1 and 4:1 the cv should still be below
	 * 0.25, which is a sensible cutoff that should avoid punishing
	 * happenstances of this pattern in just regular files */

	// perform cv check internally
	[[nodiscard]] inline auto ccacc_timing_check(const float& factor,
												 const float& threshold) -> bool
	{
		// anchor in the center, divide by factor, 4 is the middle value of the
		// last 3
		_itv_vals[4] /= factor;

		// ccacc is always window of 3
		float o = get_cv_of_window(ccacc_timing_check_size);

		// set value back
		_itv_vals[4] *= factor;

		return o < threshold;
	}

	// if using these functions, it's the responsibility of whatever is filling
	// this container to ensure that the values it's filling it with will
	// actually produce usable results

	// perform cv check internally
	[[nodiscard]] inline auto acca_timing_check(const float& factor,
												const float& threshold) -> bool
	{
		// cc in the center, multiply by factor
		_itv_vals[4] *= factor;
		float o = get_cv_of_window(ccacc_timing_check_size);
		_itv_vals[4] /= factor;
		return o < threshold;
	}

	// perform cv check internally
	[[nodiscard]] inline auto roll_timing_check(const float& factor,
												const float& threshold) -> bool
	{
		// we are looking at cccccc formation, which could be a roll or an oht,
		// we don't know yet, but presumably whatever is calling this only cares
		// about whether or not this is a roll, since the oht check requires no
		// manipulation, just a vanilla cv call

		// we can basically just branch to ccacc or acca checks depending on
		// which value is higher
		bool o = false;
		if (_itv_vals[4] > _itv_vals[5]) {
			// if middle is higher, run the ccacc check that will divide it
			o = ccacc_timing_check(factor, threshold);
		} else {
			// otherwise run acca and multiply the center
			o = acca_timing_check(factor, threshold);
		}

		return o;
	}

	// set everything to zero
	inline void zero() { _itv_vals.fill(static_cast<T>(0)); }
	inline void fill(const T& val) { _itv_vals.fill(val); }

  protected:
	std::array<T, max_moving_window_size> _itv_vals = { 0, 0, 0, 0, 0, 0 };
};
