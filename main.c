/*
 * PSn00bSDK SPU .VAG streaming example
 * (C) 2022 spicyjpeg - MPL licensed
 *
 * This example shows how to play arbitrarily long sounds, which normally would
 * not fit into SPU RAM in their entirety, by streaming them to the SPU from
 * main RAM. In this example audio data is streamed from an in-memory file,
 * however the code can easily be modified to stream from the CD instead (see
 * the cdstream example).
 *
 * The way SPU streaming works is by splitting the audio data into a series of
 * small "chunks", each of which in turn is an array of concatenated buffers
 * holding SPU ADPCM data (one for each channel, so a stereo stream would have
 * 2 buffers per chunk). All buffers in a chunk are played simultaneously using
 * multiple SPU channels; each buffer has the loop flag set at the end, so the
 * SPU will jump to the loop point set in the SPU_CH_LOOP_ADDR registers after
 * the chunk is played.
 *
 * As the loop point doesn't necessarily have to be within the chunk itself, it
 * can be used to "queue" another chunk to be played immediately after the
 * current one. This allows for double buffering: two chunks are always kept in
 * SPU RAM and one is overwritten with a new chunk while the other is playing.
 * Chunks are laid out in SPU RAM as follows:
 *
 *           ________________________________________________
 *          /               __________________               \
 *          |              /                  \              |
 *          v Loop point   | Loop flag        v Loop point   | Loop flag
 * +-------+----------------+----------------+----------------+----------------+
 * | Dummy | Left buffer 0  | Right buffer 0 | Left buffer 1  | Right buffer 1 |
 * +-------+----------------+----------------+----------------+----------------+
 *          \____________Chunk 0____________/ \____________Chunk 1____________/
 *
 * In order to keep streaming continuously we need to know when each chunk
 * actually starts playing. The SPU can be configured to trigger an interrupt
 * whenever a specific address in SPU RAM is read by a channel, so we can just
 * point it to the beginning of the buffered chunk's first buffer and wait
 * until the IRQ is fired before loading the next chunk.
 *
 * Chunks are read from a special type of .VAG file which has been interleaved
 * ahead-of-time and already contains the loop flags required to make streaming
 * work. A Python script is provided to generate such file from one or more
 * mono .VAG files.
 */
#include <stdint.h>
#include <stddef.h>
#include <psxetc.h>
#include <psxapi.h>
#include <psxgpu.h>
#include <psxpad.h>
#include <psxspu.h>
#include <hwregs_c.h>
#include "spu.h"
#include "gpu.h"

extern const uint8_t stream_data[];

static RenderContext ctx;

int main(int argc, const char* argv[]) {
	init_context(&ctx);
	SpuInit();
	reset_spu_channels();

	// Set up controller polling.
	uint8_t pad_buff[2][34];
	InitPAD(pad_buff[0], 34, pad_buff[1], 34);
	StartPAD();
	ChangeClearPAD(0);

	init_stream((const VAG_Header *) stream_data);
	start_stream();

	int paused = 0, sample_rate = getSPUSampleRate(stream_ctx.sample_rate);

	uint16_t last_buttons = 0xffff;

	while (1) {
		FntPrint(-1, "PLAYING SPU STREAM\n\n");
		FntPrint(-1, "BUFFER: %d\n", stream_ctx.db_active);
		FntPrint(-1, "STATUS: %s\n\n", stream_ctx.buffering ? "BUFFERING" : "IDLE");

		FntPrint(-1, "POSITION: %d/%d\n",  stream_ctx.next_chunk, stream_ctx.num_chunks);
		FntPrint(-1, "SMP RATE: %5d HZ\n\n", (sample_rate * 44100) >> 12);

		FntPrint(-1, "[START]      %s\n", paused ? "RESUME" : "PAUSE");
		FntPrint(-1, "[LEFT/RIGHT] SEEK\n");
		FntPrint(-1, "[O]          RESET POSITION\n");
		FntPrint(-1, "[UP/DOWN]    CHANGE SAMPLE RATE\n");
		FntPrint(-1, "[X]          RESET SAMPLE RATE\n");

		FntFlush(-1);
		display(&ctx);

		// Check if a compatible controller is connected and handle button
		// presses.
		PADTYPE *pad = (PADTYPE *) pad_buff[0];
		if (pad->stat)
			continue;
		if (
			(pad->type != PAD_ID_DIGITAL) &&
			(pad->type != PAD_ID_ANALOG_STICK) &&
			(pad->type != PAD_ID_ANALOG)
		)
			continue;

		if ((last_buttons & PAD_START) && !(pad->btn & PAD_START)) {
			paused ^= 1;
			if (paused)
				stop_stream();
			else
				start_stream();
		}

		if (!(pad->btn & PAD_LEFT))
			stream_ctx.next_chunk--;
		if (!(pad->btn & PAD_RIGHT))
			stream_ctx.next_chunk++;
		if ((last_buttons & PAD_CIRCLE) && !(pad->btn & PAD_CIRCLE))
			stream_ctx.next_chunk = -1;

		if (!(pad->btn & PAD_DOWN) && (sample_rate > 0x400))
			sample_rate -= 0x40;
		if (!(pad->btn & PAD_UP) && (sample_rate < 0x2000))
			sample_rate += 0x40;
		if ((last_buttons & PAD_CROSS) && !(pad->btn & PAD_CROSS))
			sample_rate = getSPUSampleRate(stream_ctx.sample_rate);

		// Only set the sample rate registers if necessary.
		if (pad->btn != 0xffff) {
			for (int i = 0; i < stream_ctx.channels; i++)
				SPU_CH_FREQ(i) = sample_rate;
		}

		last_buttons = pad->btn;
	}

	return 0;
}
