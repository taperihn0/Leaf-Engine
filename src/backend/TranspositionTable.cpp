#include "TranspositionTable.hpp"
#include "Search.hpp"

inline constexpr size_t operator""_MB(ull mb_count) {
	return mb_count * 1024 * 1024;
}

TranspositionTable::TranspositionTable() {
	static_assert(sizeof(TTEntry) == EntryTargetSize);
	static_assert(sizeof(TTBucket) == BucketTargetSize);
	_mem = reinterpret_cast<TTBucket*>(alignedMalloc(1_MB, sizeof(TTBucket)));
	ASSERT(_mem != nullptr, "Failed to allocate memory");
	_buckets_cnt = 1_MB / sizeof(TTBucket);
	clear();
}

TranspositionTable::~TranspositionTable() { 
	alignedFree(_mem);
}

void TranspositionTable::resize(size_t size_mb) {
	alignedFree(_mem);
	_mem = reinterpret_cast<TTBucket*>(alignedMalloc(size_mb * 1024 * 1024, sizeof(TTBucket)));
	ASSERT(_mem != nullptr, "Failed to allocate memory");
	_buckets_cnt = size_mb * 1024 * 1024 / sizeof(TTBucket);
	_generation = 0;
	_hits = 0;
}

void TranspositionTable::clear() {
	alignedMemset(_mem, 0, _buckets_cnt * sizeof(TTBucket));
	_generation = 0;
	_hits = 0;
}

void TranspositionTable::write(uint64_t node_key64, uint8_t node_depth, uint8_t node_ply, 
							   TTEntry::Bound node_bound, Score node_score, Move16b node_move, SearchResults& results) 
{
	//const uint32_t key = static_cast<uint32_t>(node_key64);
	const uint64_t key = node_key64 & 0x3FFFFFFFF;

	TTBucket* bucket = _mem + (node_key64 & (_buckets_cnt - 1));
	int16_t min_relevance = std::numeric_limits<int16_t>::max();
	size_t ind = 0;

	for (size_t i = 0; i < TTBucket::InternalEntriesCnt; i++) {
		if (bucket->entries[i].getHash() == key or bucket->entries[i].isEmpty()) {
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

	if (bucket->entries[ind].getHash() == key and
		bucket->entries[ind].depth > (node_depth * 3) >> 1 and
		node_bound != TTEntry::EXACT)
		return;

	if (bucket->entries[ind].isEmpty())
		_hits++;

	if (bucket->entries[ind].getHash() != key or !node_move.isNull())
		bucket->entries[ind].move = node_move;

	bucket->entries[ind].writeHash(key);

	bucket->entries[ind].score = node_score;
	bucket->entries[ind].depth = node_depth;
	bucket->entries[ind].bound = node_bound;
	bucket->entries[ind].generation = this->_generation;
}

bool TranspositionTable::probe(TTEntry& out_entry, uint64_t key64, Score alpha, Score beta,
							   uint8_t node_depth) const 
{
	const uint64_t key = key64 & 0x3FFFFFFFF;

	const TTBucket* bucket = _mem + (key64 & (_buckets_cnt - 1));

	size_t ind = TTBucket::InternalEntriesCnt;

	for (size_t i = 0; i < TTBucket::InternalEntriesCnt; i++) {
		if (bucket->entries[i].getHash() == key) {
			ind = i;
			break;
		}
	}

	if (ind == TTBucket::InternalEntriesCnt) {
		out_entry.move = Move32b::Null;
		return false;
	}

	const TTEntry* entry = &bucket->entries[ind];

	if (entry->depth < node_depth) {
		out_entry.move = entry->move;
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
