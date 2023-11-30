
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <psxgpu.h>
#include <psxgte.h>
#include <psxetc.h>
#include <psxpad.h>
#include <psxapi.h>
#include <psxspu.h>
#include <psxetc.h>
#include <inline_c.h>
#include <hwregs_c.h>

#include "clip.h"
#include "lookat.h"
#include "cube.h"
#include "spu.h"

#define FLOOR_SIZE      40

int main() {

	int i, p, xy_temp;
	int px, py;

	SVECTOR	rot;			    VECTOR	pos;
	SVECTOR verts[FLOOR_SIZE + 1][FLOOR_SIZE + 1];
	VECTOR	cam_pos;		    VECTOR	cam_rot;		    int		cam_mode;
	VECTOR	tpos;			    SVECTOR	trot;			    MATRIX	mtx, lmtx;
	//PADTYPE* pad;
	POLY_F4* pol4;

	init();

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

	pos.vx = 0;
	pos.vz = 0;
	pos.vy = -125;

	rot.vx = 0;
	rot.vy = 0;
	rot.vz = 0;

	for (py = 0; py < FLOOR_SIZE + 1; py++) {
		for (px = 0; px < FLOOR_SIZE + 1; px++) {

			setVector(&verts[py][px],
				(100 * (px - 8)) - 50,
				0,
				(100 * (py - 8)) - 50);

		}
	}


	setVector(&cam_pos, 0, ONE * -250, 0);
	setVector(&cam_rot, 0, 0, 0);

	while (1) {

		cam_mode = 0;

		trot.vx = cam_rot.vx >> 12;
		trot.vy = cam_rot.vy >> 12;
		trot.vz = cam_rot.vz >> 12;

		SVECTOR up = { 0, -ONE, 0 };

		tpos.vx = cam_pos.vx >> 12;
		tpos.vy = cam_pos.vy >> 12;
		tpos.vz = cam_pos.vz >> 12;

		LookAt(&tpos, &pos, &up, &mtx);

		gte_SetRotMatrix(&mtx);
		gte_SetTransMatrix(&mtx);

		pol4 = (POLY_F4*)db_nextpri;

		for (py = 0; py < FLOOR_SIZE; py++) {
			for (px = 0; px < FLOOR_SIZE; px++) {

				gte_ldv3(
					&verts[py][px],
					&verts[py][px + 1],
					&verts[py + 1][px]);

				gte_rtpt();

				gte_avsz3();
				gte_stotz(&p);

				if (((p >> 2) >= OT_LEN) || ((p >> 2) <= 0))
					continue;

				setPolyF4(pol4);

				gte_stsxy0(&pol4->x0);
				gte_stsxy1(&pol4->x1);
				gte_stsxy2(&pol4->x2);

				gte_ldv0(&verts[py + 1][px + 1]);
				gte_rtps();
				gte_stsxy(&pol4->x3);

				if (quad_clip(&screen_clip,
					(DVECTOR*)&pol4->x0, (DVECTOR*)&pol4->x1,
					(DVECTOR*)&pol4->x2, (DVECTOR*)&pol4->x3))
					continue;

				gte_avsz4();
				gte_stotz(&p);

				if ((px + py) & 0x1) {
					setRGB0(pol4, 128, 128, 128);
				}
				else {
					setRGB0(pol4, 255, 255, 255);
				}

				addPrim(db[db_active].ot + (p >> 2), pol4);
				pol4++;

			}
		}

		db_nextpri = (char*)pol4;

		sort_cube(&mtx, &pos, &rot, 0, 200, 0);

		cam_pos.vx = (pos.vx + (isin(rot.vy) - 1) / 10) << 12;
		cam_pos.vz = (pos.vz + (icos(rot.vy) - 1) / 10) << 12;
		FntPrint(-1, "BUFFER: %d\n", stream_ctx.db_active);
		FntPrint(-1, "POSITION: %d/%d\n",  stream_ctx.next_chunk, stream_ctx.num_chunks);
		FntPrint(-1, "SMP RATE: %5d HZ\n", (sample_rate * 44100) >> 12);
		FntPrint(-1, "[START]      %s\n", paused ? "RESUME" : "PAUSE");
		FntFlush(-1);

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
		/*
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
		*/
		last_buttons = pad->btn;

		display();

	}

	return 0;

}

