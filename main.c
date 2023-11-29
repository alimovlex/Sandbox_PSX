
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <psxgpu.h>
#include <psxgte.h>
#include <psxpad.h>
#include <psxapi.h>
#include <psxetc.h>
#include <inline_c.h>

#include "clip.h"
#include "lookat.h"
#include "cube.h"

#define FLOOR_SIZE      40
#define ENEMY_COUNT     5

typedef struct {
	VECTOR pos;
	SVECTOR rot;
} ENEMY;

int main() {
	int		db_active = 0;

	int i, p, xy_temp;
	int px, py;

	int lost = 0;
	int win = 0;

	int timer = 0;

	SVECTOR	rot;			    VECTOR	pos;
	SVECTOR verts[FLOOR_SIZE + 1][FLOOR_SIZE + 1];
	VECTOR	cam_pos;		    VECTOR	cam_rot;		    int		cam_mode;
	VECTOR	tpos;			    SVECTOR	trot;			    MATRIX	mtx, lmtx;
	PADTYPE* pad;
	POLY_F4* pol4;
	VECTOR victory_pos;
	SVECTOR victory_rot;

	victory_pos.vx = rand() % 2000 + 1000;
	victory_pos.vz = rand() % 2000 + 1000;
	victory_pos.vy = -125;

	victory_rot.vx = 0;
	victory_rot.vy = 0;

	ENEMY enemies[ENEMY_COUNT];

	for (int j = 0; j < ENEMY_COUNT; ++j) {
		enemies[j].pos.vx = rand() % 1500;
		enemies[j].pos.vz = rand() % 1500;
		enemies[j].pos.vy = -150;

		enemies[j].rot.vx = 0;
		enemies[j].rot.vz = 0;
		enemies[j].rot.vy = rand() % 4096;
	}


	init();

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

	int bounce = 0;

	while (1) {

		if (win || lost) {
			timer++;
			if (timer >= 300) {
				((void (*)())0xBFC00000)();
			}
		}

		pad = (PADTYPE*)&pad_buff[0][0];

		cam_mode = 0;

		trot.vx = cam_rot.vx >> 12;
		trot.vy = cam_rot.vy >> 12;
		trot.vz = cam_rot.vz >> 12;

		if (pad->stat == 0 && !lost && !win) {
			if ((pad->type == 0x5) || (pad->type == 0x7)) {

				if (((pad->ls_y - 128) < -16) || ((pad->ls_y - 128) > 16)) {
					pos.vz -= (((pad->ls_y - 128) / 64) * (isin(-(rot.vy + 1024)) / 1000));
					pos.vx -= (((pad->ls_y - 128) / 64) * (icos(-(rot.vy + 1024)) / 1000));
				}

				if (((pad->ls_x - 128) < -16) || ((pad->ls_x - 128) > 16)) {
					pos.vz -= (((pad->ls_x - 128) / 64) * (isin(-(rot.vy)) / 1000));
					pos.vx -= (((pad->ls_x - 128) / 64) * (icos(-(rot.vy)) / 1000));
				}

				if (((pad->rs_x - 128) < -16) || ((pad->rs_x - 128) > 16)) {

					rot.vy += (pad->rs_x - 128) / 10;
				}

			}

		}

		if (lost) {
			FntPrint(-1, "\n\n               GAME OVER!\n      You've been hit by an enemy!");
		}
		else if (win) {
			FntPrint(-1, "\n\n                YOU WON!\n     You managed to get to the end!");
		}



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

		sort_cube(&mtx, &victory_pos, &victory_rot, 200, 200, 0);

		for (int j = 0; j < 4; ++j) {
			sort_cube(&mtx, &enemies[j].pos, &enemies[j].rot, 200, 0, 0);
			enemies[j].pos.vz -= (0.5 * (isin(-(enemies[j].rot.vy + 1024)) / 1000));
			enemies[j].pos.vx -= (0.5 * (icos(-(enemies[j].rot.vy + 1024)) / 1000));
			enemies[j].rot.vy += (rand() % 100) - 50;

			int distance;

			distance = SquareRoot0(abs((pos.vx - enemies[j].pos.vx) * (pos.vx - enemies[j].pos.vx)) + abs((pos.vz - enemies[j].pos.vz) * (pos.vz - enemies[j].pos.vz)));
			if (distance < 200 && !win) {
				lost = 1;
			}
		}

		int distance;
		distance = SquareRoot0(abs((pos.vx - victory_pos.vx) * (pos.vx - victory_pos.vx)) + abs((pos.vz - victory_pos.vz) * (pos.vz - victory_pos.vz)));
		if (distance < 200) {
			win = 1;
		}

		cam_pos.vx = (pos.vx + (isin(rot.vy) - 1) / 10) << 12;
		cam_pos.vz = (pos.vz + (icos(rot.vy) - 1) / 10) << 12;

		victory_rot.vx = victory_rot.vx + 10;
		victory_rot.vy = victory_rot.vy + 10;

		victory_pos.vy = -200 + isin(bounce * 30) / 50;

		bounce++;

		FntFlush(-1);

		display();

	}

	return 0;

}

