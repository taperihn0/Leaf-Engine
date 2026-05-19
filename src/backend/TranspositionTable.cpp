#include "TranspositionTable.hpp"
#include "Search.hpp"

TTEntry::TTEntry()
	: key16(0)
	, key18(0)
	, generation(0)
	, bound(Bound::NONE)
	, depth(0)
	, score(Score::Undef)
	, move(Move16b::Null)
	, eval(Score::Undef)
{}

TranspositionTable::TranspositionTable(size_t mb_size)
	: _mem(reinterpret_cast<TTBucket*>(alignedMalloc(mb_size, sizeof(TTBucket)))) 
{
	ASSERT(isPow2(mb_size), "Transposition table must be size of 2 power");
	ASSERT(_mem.get() != nullptr, "Failed to allocate memory");
	_buckets_cnt = mb_size / sizeof(TTBucket);
	_buckets_pow_2 = getExp2(_buckets_cnt);
	clear();
}

TranspositionTable::TranspositionTable(TranspositionTable&& tt) {
	_mem = std::move(tt._mem);
	_buckets_cnt = tt._buckets_cnt;
	_buckets_pow_2 = tt._buckets_pow_2;
	_generation = tt._generation;
	_hits = tt._hits;
}

void TranspositionTable::resize(size_t size_mb) {
	ASSERT(isPow2(size_mb), "Transposition table must be size of 2 power");

	_mem = uniq_ptr_buf(
		reinterpret_cast<TTBucket*>(alignedMalloc(size_mb, sizeof(TTBucket)))
	);
	
	ASSERT(_mem.get() != nullptr, "Failed to allocate memory");
	_buckets_cnt = size_mb / sizeof(TTBucket);
	_buckets_pow_2 = getExp2(_buckets_cnt);
	_generation = 0;
	_hits = 0;
}

void TranspositionTable::clear() {
	alignedMemset(_mem.get(), 0, _buckets_cnt * sizeof(TTBucket));
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

	assert(getExp2(_buckets_cnt) == _buckets_pow_2);

	TTBucket* bucket = _mem.get() + (node_key64 & (_buckets_cnt - 1));

	const uint32_t keyhi = static_cast<uint32_t>((node_key64 >> _buckets_pow_2) & 0x3FFFF);

	int16_t min_relevance = maxof<int16_t>();
	size_t ind = 0;

	for (size_t i = 0; i < TTBucket::InternalEntriesCnt; i++) {
		if (bucket->entries[i].getHash() == keyhi or bucket->entries[i].isEmpty()) {
			ind = i;
			break;
		}

		const int16_t age = static_cast<int16_t>(this->_generation) - bucket->entries[i].generation;
		const int16_t relevance = static_cast<int16_t>(bucket->entries[i].depth) - age;

		if (relevance < min_relevance) {
			min_relevance = relevance;
			ind = i;
		}
	}

	const uint32_t entry_keyhi = bucket->entries[ind].getHash();

	if (entry_keyhi != keyhi or node_eval.isValid())
		bucket->entries[ind].eval = node_eval;

	if (entry_keyhi == keyhi and
		bucket->entries[ind].depth > (node_depth * 3) >> 1 and
		node_bound != TTEntry::EXACT) 
		return;

	if (bucket->entries[ind].isEmpty())
		_hits++;

	if (entry_keyhi != keyhi or !node_move.isNull())
		bucket->entries[ind].move = node_move;

	bucket->entries[ind].writeHash(keyhi);
	assert(bucket->entries[ind].getHash() == keyhi);

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
	assert(getExp2(_buckets_cnt) == _buckets_pow_2);

	const TTBucket* bucket = _mem.get() + (key64 & (_buckets_cnt - 1));

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
	assert(getExp2(_buckets_cnt) == _buckets_pow_2);
	prefetch(reinterpret_cast<const void*>(_mem.get() + (key64 & (_buckets_cnt - 1))));
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
