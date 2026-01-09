#include "TranspositionTable.hpp"
#include "Search.hpp"

TranspositionTable::TranspositionTable(size_t mb_size) {
	ASSERT(isPow2(mb_size), "Transposition table must be size of 2 power");
	_mem = reinterpret_cast<TTBucket*>(alignedMalloc(mb_size, sizeof(TTBucket)));
	ASSERT(_mem != nullptr, "Failed to allocate memory");
	_buckets_cnt = mb_size / sizeof(TTBucket);
	_buckets_pow_2 = get2pow(_buckets_cnt);
	clear();
}

TranspositionTable::TranspositionTable(TranspositionTable&& tt) {
	_mem = tt._mem;
	_buckets_cnt = tt._buckets_cnt;
	_buckets_pow_2 = tt._buckets_pow_2;
	_generation = tt._generation;
	_hits = tt._hits;
	std::memset(reinterpret_cast<void*>(&tt), 0, sizeof(tt));
}

TranspositionTable::~TranspositionTable() { 
	alignedFree(_mem);
}

void TranspositionTable::resize(size_t size_mb) {
	ASSERT(isPow2(size_mb), "Transposition table must be size of 2 power");
	alignedFree(_mem);
	_mem = reinterpret_cast<TTBucket*>(alignedMalloc(size_mb * 1024 * 1024, sizeof(TTBucket)));
	ASSERT(_mem != nullptr, "Failed to allocate memory");
	_buckets_cnt = size_mb * 1024 * 1024 / sizeof(TTBucket);
	_buckets_pow_2 = get2pow(_buckets_cnt);
	_generation = 0;
	_hits = 0;
}

void TranspositionTable::clear() {
	alignedMemset(_mem, 0, _buckets_cnt * sizeof(TTBucket));
	_generation = 0;
	_hits = 0;
}

void TranspositionTable::write(uint64_t node_key64, uint8_t node_depth, 
							   uint8_t node_ply, TTEntry::Bound node_bound, 
							   Score node_score, Move16b node_move, Score node_eval,
							   SearchResults& results) 
{
	_declUnused(results);
	_declUnused(node_ply); // unused for now

	assert(get2pow(_buckets_cnt) == _buckets_pow_2);

	TTBucket* bucket = _mem + (node_key64 & (_buckets_cnt - 1));

	const uint32_t keyhi = static_cast<uint32_t>((node_key64 >> _buckets_pow_2) & 0x3FFFF);

	int16_t min_relevance = std::numeric_limits<int16_t>::max();
	size_t ind = 0;

	for (size_t i = 0; i < TTBucket::InternalEntriesCnt; i++) {
		if (bucket->entries[i].getHash() == keyhi or bucket->entries[i].isEmpty()) {
			ind = i;
			break;
		}

		uint8_t age = this->_generation - bucket->entries[i].generation;
		int16_t relevance = (int16_t)bucket->entries[i].depth - (int16_t)age;

		if (relevance < min_relevance) {
			min_relevance = relevance;
			ind = i;
		}
	}

	if (bucket->entries[ind].getHash() == keyhi and
		bucket->entries[ind].depth > (node_depth * 3) >> 1 and
		node_bound != TTEntry::EXACT) 
	{	
		if (!bucket->entries[ind].eval.isValid())
			bucket->entries[ind].eval = node_eval;

		return;
	}

	if (bucket->entries[ind].isEmpty())
		_hits++;

	if (bucket->entries[ind].getHash() != keyhi or !node_move.isNull())
		bucket->entries[ind].move = node_move;

	bucket->entries[ind].writeHash(keyhi);
	assert(bucket->entries[ind].getHash() == keyhi);

	if (!bucket->entries[ind].eval.isValid())
		bucket->entries[ind].eval = node_eval;

	bucket->entries[ind].score = node_score;
	bucket->entries[ind].depth = node_depth;
	bucket->entries[ind].bound = node_bound;
	bucket->entries[ind].generation = this->_generation;
}

bool TranspositionTable::probe(TTEntry& out_entry, 
							   uint64_t key64, 
							   Score alpha, Score beta,
							   uint8_t node_depth) const 
{
	assert(get2pow(_buckets_cnt) == _buckets_pow_2);

	const TTBucket* bucket = _mem + (key64 & (_buckets_cnt - 1));

	const uint32_t keyhi = static_cast<uint32_t>((key64 >> _buckets_pow_2) & 0x3FFFF);

	size_t ind = TTBucket::InternalEntriesCnt;

	for (size_t i = 0; i < TTBucket::InternalEntriesCnt; i++) {
		if (bucket->entries[i].getHash() == keyhi) {
			ind = i;
			break;
		}
	}

	if (ind == TTBucket::InternalEntriesCnt) {
		out_entry.move = Move32b::Null;
		out_entry.eval = Score::Undef;
		return false;
	}

	const TTEntry* entry = &bucket->entries[ind];

	if (entry->depth < node_depth) {
		out_entry.move = entry->move;
		out_entry.eval = entry->eval;
		return false;
	}

	switch (entry->bound) {
	case TTEntry::EXACT: {
		out_entry = *entry;
		return true;
	}
	case TTEntry::LOWERBOUND: {
		out_entry = *entry;
		out_entry.score = alpha;
		return alpha >= entry->score;
	}
	case TTEntry::UPPERBOUND: {
		out_entry = *entry;
		out_entry.score = beta;
		return beta <= entry->score;
	}
	default: break; 
	}

	return false;
}

void TranspositionTable::prefetchBucket(uint64_t key64) const {
	assert(get2pow(_buckets_cnt) == _buckets_pow_2);
	prefetch(reinterpret_cast<const void*>(_mem + (key64 & (_buckets_cnt - 1))));
}

#if defined(DEBUG)
void TranspositionTable::printDebug() {
	std::cout << "Hash size: " << _buckets_cnt * sizeof(TTBucket) / 1024 / 1024 << "MB\n";
}
#endif

size_t TranspositionTable::getEntriesCount() const {
	return _buckets_cnt * TTBucket::InternalEntriesCnt;
}

uint16_t TranspositionTable::getHashfull() const {
	return static_cast<uint16_t>(static_cast<float>(_hits) / getEntriesCount() * 1000);
}

void TranspositionTable::newGeneration() {
	_generation++;
}

void TranspositionTable::clearHashfull() {
	_hits = 0;
}
