#define FD1094_STATE_RESET	0x0100
#define FD1094_STATE_IRQ	0x0200
#define FD1094_STATE_RTE	0x0300

int fd1094_set_state(unsigned char *key,int state);
int fd1094_decode(int address,int val,unsigned char *key,int vector_fetch);

#ifdef MAME_DEBUG

typedef struct _fd1094_constraint fd1094_constraint;
struct _fd1094_constraint
{
	offs_t	pc;
	UINT16	state;
	UINT16	value;
	UINT16	mask;
};

void fd1094_generate_full_key(UINT8 *key, UINT32 global, UINT32 seed);
int fd1094_is_valid_prng_sequence(const UINT8 *base, int length, UINT32 *seedptr);
UINT32 fd1094_find_global_key_matches(const UINT16 *encrypted, const UINT16 *desired, const UINT16 *desiredmask, UINT32 startwith, UINT16 *output);
void fd1094_find_global_keys(const UINT16 *base, UINT8 *key);
#endif
