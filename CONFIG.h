#define WORD_WIDTH 24
#define DWORD_WIDTH (2 * WORD_WIDTH)

#define word_t std::bitset<WORD_WIDTH>
#define dword_t std::bitset<DWORD_WIDTH>

#define MIN_MEM_SIZE 1026