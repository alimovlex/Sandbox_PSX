/* .VAG header structure */

typedef struct {
	uint32_t magic;			// 0x69474156 ("VAGi") for interleaved files
	uint32_t version;
	uint32_t interleave;	// Little-endian, size of each channel buffer
	uint32_t size;			// Big-endian, in bytes
	uint32_t sample_rate;	// Big-endian, in Hertz
	uint16_t _reserved[5];
	uint16_t channels;		// Little-endian, if 0 the file is mono
	char     name[16];
} VAG_Header;

typedef struct {
	const uint8_t *data;
	int buffer_size, num_chunks, sample_rate, channels;

	volatile int    next_chunk, spu_addr;
	volatile int8_t db_active, buffering;
} StreamContext;

extern StreamContext stream_ctx;
extern const uint8_t stream_data[];

void stop_stream(void);
void start_stream(void);
void init_stream(const VAG_Header *vag);
void reset_spu_channels(void);
void spu_dma_handler(void);
void spu_irq_handler(void);