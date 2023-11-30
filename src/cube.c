#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <psxgpu.h>
#include <psxgte.h>
#include <psxpad.h>
#include <psxapi.h>
#include <psxetc.h>
#include <inline_c.h>
#include "cube.h"

#define FLOOR_SIZE      40
#define ENEMY_COUNT     5

#define SCREEN_XRES		320
#define SCREEN_YRES		240

#define CENTERX			SCREEN_XRES>>1
#define CENTERY			SCREEN_YRES>>1
#define CUBE_FACES 6

MATRIX light_mtx = {
	/* X,  Y,  Z */
	-2048 , -2048 , -2048,
	0	  , 0	  , 0,
	0	  , 0	  , 0
};

MATRIX color_mtx = {
		ONE, 0, 0,	        0, 0, 0,	        ONE, 0, 0 };

typedef struct {
	short v0, v1, v2, v3;
} INDEX;

SVECTOR cube_verts[] = {
		{ -100, -100, -100, 0 },
		{  100, -100, -100, 0 },
		{ -100,  100, -100, 0 },
		{  100,  100, -100, 0 },
		{  100, -100,  100, 0 },
		{ -100, -100,  100, 0 },
		{  100,  100,  100, 0 },
		{ -100,  100,  100, 0 }
};

SVECTOR cube_norms[] = {
		{ 0, 0, -ONE, 0 },
		{ 0, 0, ONE, 0 },
		{ 0, -ONE, 0, 0 },
		{ 0, ONE, 0, 0 },
		{ -ONE, 0, 0, 0 },
		{ ONE, 0, 0, 0 }
};

INDEX cube_indices[] = {
		{ 0, 1, 2, 3 },
		{ 4, 5, 6, 7 },
		{ 5, 4, 0, 1 },
		{ 6, 7, 3, 2 },
		{ 0, 2, 5, 7 },
		{ 3, 1, 6, 4 }
};

DB		db[2];
int		db_active = 0;
char* db_nextpri;
RECT	screen_clip;
//char pad_buff[2][34];

void sort_cube(MATRIX* mtx, VECTOR* pos, SVECTOR* rot, int r, int g, int b) {

	int i, p;
	POLY_F4* pol4;

	MATRIX omtx, lmtx;

	RotMatrix(rot, &omtx);
	TransMatrix(&omtx, pos);

	MulMatrix0(&light_mtx, &omtx, &lmtx);

	gte_SetLightMatrix(&lmtx);

	CompMatrixLV(mtx, &omtx, &omtx);

	PushMatrix();

	gte_SetRotMatrix(&omtx);
	gte_SetTransMatrix(&omtx);

	pol4 = (POLY_F4*)db_nextpri;

	for (i = 0; i < CUBE_FACES; i++) {

		gte_ldv3(
			&cube_verts[cube_indices[i].v0],
			&cube_verts[cube_indices[i].v1],
			&cube_verts[cube_indices[i].v2]);

		gte_rtpt();

		gte_nclip();

		gte_stopz(&p);

		if (p < 0)
			continue;

		gte_avsz3();
		gte_stotz(&p);

		if (((p >> 2) <= 0) || ((p >> 2) >= OT_LEN))
			continue;

		setPolyF4(pol4);

		gte_stsxy0(&pol4->x0);
		gte_stsxy1(&pol4->x1);
		gte_stsxy2(&pol4->x2);

		gte_ldv0(&cube_verts[cube_indices[i].v3]);
		gte_rtps();
		gte_stsxy(&pol4->x3);

		gte_ldrgb(&pol4->r0);

		gte_ldv0(&cube_norms[i]);

		gte_ncs();

		gte_strgb(&pol4->r0);

		gte_avsz4();
		gte_stotz(&p);

		setRGB0(pol4, r, g, b);

		addPrim(db[db_active].ot + (p >> 2), pol4);

		pol4++;

	}

	db_nextpri = (char*)pol4;

	PopMatrix();

}

void init() {

	ResetGraph(0);

	SetDefDispEnv(&db[0].disp, 0, 0, SCREEN_XRES, SCREEN_YRES);
	SetDefDrawEnv(&db[0].draw, SCREEN_XRES, 0, SCREEN_XRES, SCREEN_YRES);

	setRGB0(&db[0].draw, 0, 0, 255);
	db[0].draw.isbg = 1;
	db[0].draw.dtd = 1;


	SetDefDispEnv(&db[1].disp, SCREEN_XRES, 0, SCREEN_XRES, SCREEN_YRES);
	SetDefDrawEnv(&db[1].draw, 0, 0, SCREEN_XRES, SCREEN_YRES);

	setRGB0(&db[1].draw, 0, 0, 255);
	db[1].draw.isbg = 1;
	db[1].draw.dtd = 1;

	PutDrawEnv(&db[0].draw);

	ClearOTagR(db[0].ot, OT_LEN);
	ClearOTagR(db[1].ot, OT_LEN);

	db_nextpri = db[0].p;

	setRECT(&screen_clip, 0, 0, SCREEN_XRES, SCREEN_YRES);


	InitGeom();

	gte_SetGeomOffset(CENTERX, CENTERY);

	gte_SetGeomScreen(CENTERX);

	gte_SetBackColor(0, 255, 0);
	gte_SetColorMatrix(&color_mtx);


	//InitPAD(&pad_buff[0][0], 34, &pad_buff[1][0], 34);

	//StartPAD();

	//ChangeClearPAD(0);

	FntLoad(960, 0);
	FntOpen(0, 8, 320, 216, 0, 100);

}

void display() {

	DrawSync(0);
	VSync(0);

	db_active ^= 1;
	db_nextpri = db[db_active].p;

	ClearOTagR(db[db_active].ot, OT_LEN);

	PutDrawEnv(&db[db_active].draw);
	PutDispEnv(&db[db_active].disp);

	SetDispMask(1);

	DrawOTag(db[1 - db_active].ot + (OT_LEN - 1));

}