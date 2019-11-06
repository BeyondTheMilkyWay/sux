/*
 * Sux: Succinct data structures
 *
 * Copyright (C) 2019 Stefano Marchini
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as published by the Free
 *  Software Foundation; either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  This library is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "../util/Vector.hpp"
#include "DynamicBitVector.hpp"
#include "Rank.hpp"
#include "Select.hpp"
#include "SelectZero.hpp"

namespace sux::bits {

/** Ranking and selection in a dynamic bit vector by means
 * of a searchable prefix-sum data structure and broadword
 * operations on a single word.
 *
 * The constructors of this class only store a reference
 * to a provided bit vector. The content of the bit vector
 * should be changed only through the mutation methods of
 * this class, or the results will be unpredictable.
 *
 * **Warning**: if you plan an calling rank(size_t) with
 * argument size(), you must have at least one additional
 * free bit at the end of the provided bit vector.
 *
 * @tparam SPS underlying sux::util::SearchablePrefixSums implementation.
 * @tparam AT a type of memory allocation for the underlying structure.
 */

template <template <size_t, util::AllocType AT> class SPS, util::AllocType AT = util::AllocType::MALLOC> class WordDynRankSel : public DynamicBitVector, public Rank, public Select, public SelectZero {
  private:
	static constexpr size_t BOUND = 64;
	uint64_t *const Vector;
	size_t Size;
	SPS<BOUND, AT> SrcPrefSum;

  public:
	/** Creates a new instance using a given bit vector.
	 *
	 * Thus constructor only store a reference
	 * to the provided bit vector. The content of the bit vector
	 * should be changed only through the mutation methods of
	 * this class, or the results will be unpredictable.
	 *
	 * **Warning**: if you plan an calling rank(size_t) with
	 * argument size(), you must have at least one additional
	 * free bit at the end of the provided bit vector.
	 *
	 * @param bitvector a bit vector of 64-bit words.
	 * @param size the length (in bits) of the bit vector.
	 */
	WordDynRankSel(uint64_t bitvector[], size_t size) : Vector(bitvector), Size(size), SrcPrefSum(buildSrcPrefSum(bitvector, divRoundup(size, BOUND))) {}

	uint64_t *bitvector() const { return Vector; }

	using Rank::rank;
	using Rank::rankZero;
	virtual uint64_t rank(size_t pos) { return SrcPrefSum.prefix(pos / 64) + nu(Vector[pos / 64] & ((1ULL << (pos % 64)) - 1)); }

	virtual size_t select(uint64_t rank) {
		size_t idx = SrcPrefSum.find(&rank);
		uint64_t rank_chunk = nu(Vector[idx]);
		if (rank < rank_chunk) return idx * 64 + select64(Vector[idx], rank);

		return SIZE_MAX;
	}

	virtual size_t selectZero(uint64_t rank) {
		const size_t idx = SrcPrefSum.compFind(&rank);

		uint64_t rank_chunk = nu(~Vector[idx]);
		if (rank < rank_chunk) return idx * 64 + select64(~Vector[idx], rank);

		return SIZE_MAX;
	}

	virtual uint64_t update(size_t index, uint64_t word) {
		uint64_t old = Vector[index];
		Vector[index] = word;
		SrcPrefSum.add(index + 1, nu(word) - nu(old));

		return old;
	}

	virtual bool set(size_t index) {
		uint64_t old = Vector[index / 64];
		Vector[index / 64] |= uint64_t(1) << (index % 64);

		if (Vector[index / 64] != old) {
			SrcPrefSum.add(index / 64 + 1, 1);
			return false;
		}

		return true;
	}

	virtual bool clear(size_t index) {
		uint64_t old = Vector[index / 64];
		Vector[index / 64] &= ~(uint64_t(1) << (index % 64));

		if (Vector[index / 64] != old) {
			SrcPrefSum.add(index / 64 + 1, -1);
			return true;
		}

		return false;
	}

	virtual bool toggle(size_t index) {
		uint64_t old = Vector[index / 64];
		Vector[index / 64] ^= uint64_t(1) << (index % 64);
		bool was_set = Vector[index / 64] < old;
		SrcPrefSum.add(index / 64 + 1, was_set ? -1 : 1);

		return was_set;
	}

	virtual size_t size() const { return Size; }

	virtual size_t bitCount() const { return SrcPrefSum.bitCount() - sizeof(SrcPrefSum) * 8 + sizeof(this) * 8 + ((Size + 63) & ~63); }

  private:
	static size_t divRoundup(size_t x, size_t y) { return (x + y - 1) / y; }

	SPS<BOUND, AT> buildSrcPrefSum(const uint64_t bitvector[], size_t size) {
		unique_ptr<uint64_t[]> sequence = make_unique<uint64_t[]>(size);
		for (size_t i = 0; i < size; i++) sequence[i] = nu(bitvector[i]);
		return SPS<BOUND, AT>(sequence.get(), size);
	}

	friend std::ostream &operator<<(std::ostream &os, const WordDynRankSel<SPS, AT> &bv) {
		os.write((char *)&bv.Size, sizeof(uint64_t));
		return os << bv.SrcPrefSum << bv.Vector;
	}

	friend std::istream &operator>>(std::istream &is, WordDynRankSel<SPS, AT> &bv) {
		is.read((char *)&bv.Size, sizeof(uint64_t));
		return is >> bv.SrcPrefSum >> bv.Vector;
	}
};

} // namespace sux::bits
