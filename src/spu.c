#include <stdint.h>
#include <stddef.h>
#include <psxetc.h>
#include <psxapi.h>
#include <psxgpu.h>
#include <psxpad.h>
#include <psxspu.h>
#include <hwregs_c.h>
#include "spu.h"

#define SWAP_ENDIAN(x) ( \
	(((uint32_t) (x) & 0x000000ff) << 24) | \
	(((uint32_t) (x) & 0x0000ff00) <<  8) | \
	(((uint32_t) (x) & 0x00ff0000) >>  8) | \
	(((uint32_t) (x) & 0xff000000) >> 24) \
)

/* Interrupt callbacks */

// The first 4 KB of SPU RAM are reserved for capture buffers and psxspu
// additionally uploads a dummy sample (16 bytes) at 0x1000 by default, so the
// chunks must be placed after those. The dummy sample is going to be used to
// keep unused SPU channels busy, preventing them from accidentally triggering
// the SPU IRQ and throwing off the timing (all channels are always reading
// from SPU RAM, even when "stopped").
// https://problemkaputt.de/psx-spx.htm#spuinterrupt
#define DUMMY_BLOCK_ADDR  0x1000
#define BUFFER_START_ADDR 0x1010

void spu_irq_handler(void) {
	// Acknowledge the interrupt to ensure it can be triggered again. The only
	// way to do this is actually to disable the interrupt entirely; we'll
	// enable it again once the chunk is ready.
	SPU_CTRL &= ~(1 << 6);

	int chunk_size = stream_ctx.buffer_size * stream_ctx.channels;
	int chunk      = (stream_ctx.next_chunk + 1) % (uint32_t) stream_ctx.num_chunks;

	stream_ctx.db_active ^= 1;
	stream_ctx.buffering  = 1;
	stream_ctx.next_chunk = chunk;

	// Configure to SPU to trigger an IRQ once the chunk that is going to be
	// filled now starts playing (so the next buffer can be loaded) and
	// override both channels' loop addresses to make them "jump" to the new
	// buffers, rather than actually looping when they encounter the loop flag
	// at the end of the currently playing buffers.
	int addr = BUFFER_START_ADDR + (stream_ctx.db_active ? chunk_size : 0);
	stream_ctx.spu_addr = addr;

	SPU_IRQ_ADDR = getSPUAddr(addr);
	for (int i = 0; i < stream_ctx.channels; i++)
		SPU_CH_LOOP_ADDR(i) = getSPUAddr(addr + stream_ctx.buffer_size * i);

	// Start uploading the next chunk to the SPU.
	SpuSetTransferStartAddr(addr);
	SpuWrite((const uint32_t *) &stream_ctx.data[chunk * chunk_size], chunk_size);
}

void spu_dma_handler(void) {
	// Re-enable the SPU IRQ once the new chunk has been fully uploaded.
	SPU_CTRL |= 1 << 6;

	stream_ctx.buffering = 0;
}

/* Helper functions */

// This isn't actually required for this example, however it is necessary if
// you want to allocate the stream buffers into a region of SPU RAM that was
// previously used (to make sure the IRQ isn't going to be triggered by any
// inactive channels).
void reset_spu_channels(void) {
	SpuSetKey(0, 0x00ffffff);

	for (int i = 0; i < 24; i++) {
		SPU_CH_ADDR(i) = getSPUAddr(DUMMY_BLOCK_ADDR);
		SPU_CH_FREQ(i) = 0x1000;
	}

	SpuSetKey(1, 0x00ffffff);
}

void init_stream(const VAG_Header *vag) {
	EnterCriticalSection();
	InterruptCallback(IRQ_SPU, &spu_irq_handler);
	DMACallback(DMA_SPU, &spu_dma_handler);
	ExitCriticalSection();

	int buf_size = vag->interleave;

	stream_ctx.data        = &((const uint8_t *) vag)[2048];
	stream_ctx.buffer_size = buf_size;
	stream_ctx.num_chunks  = (SWAP_ENDIAN(vag->size) + buf_size - 1) / buf_size;
	stream_ctx.sample_rate = SWAP_ENDIAN(vag->sample_rate);
	stream_ctx.channels    = vag->channels ? vag->channels : 1;

	stream_ctx.db_active  =  1;
	stream_ctx.next_chunk = -1;

	// Ensure at least one chunk is in SPU RAM by invoking the IRQ handler
	// manually and blocking until the chunk has loaded.
	spu_irq_handler();
	while (stream_ctx.buffering)
		__asm__ volatile("");
}

void start_stream(void) {
	int bits = 0x00ffffff >> (24 - stream_ctx.channels);

	// Disable the IRQ as we're going to call spu_irq_handler() manually (due
	// to finicky SPU timings).
	SPU_CTRL &= ~(1 << 6);

	for (int i = 0; i < stream_ctx.channels; i++) {
		SPU_CH_ADDR(i)  = getSPUAddr(stream_ctx.spu_addr + stream_ctx.buffer_size * i);
		SPU_CH_FREQ(i)  = getSPUSampleRate(stream_ctx.sample_rate);
		SPU_CH_ADSR1(i) = 0x00ff;
		SPU_CH_ADSR2(i) = 0x0000;
	}

	// Unmute the channels and route them for stereo output. You'll want to
	// edit this if you are using more than 2 channels, and/or if you want to
	// provide an option to output mono audio instead of stereo.
	SPU_CH_VOL_L(0) = 0x3fff;
	SPU_CH_VOL_R(0) = 0x0000;
	SPU_CH_VOL_L(1) = 0x0000;
	SPU_CH_VOL_R(1) = 0x3fff;

	SpuSetKey(1, bits);
	spu_irq_handler();
}

// This is basically a variant of reset_spu_channels() that only resets the
// channels used to play the stream, to (again) prevent them from triggering
// the SPU IRQ while the stream is paused.
void stop_stream(void) {
	int bits = 0x00ffffff >> (24 - stream_ctx.channels);

	SpuSetKey(0, bits);

	for (int i = 0; i < stream_ctx.channels; i++)
		SPU_CH_ADDR(i) = getSPUAddr(DUMMY_BLOCK_ADDR);

	SpuSetKey(1, bits);
}