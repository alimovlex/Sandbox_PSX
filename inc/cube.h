#define OT_LEN			4096
#define PACKET_LEN		161384
extern char pad_buff[2][34];
extern char* db_nextpri;
extern RECT	screen_clip;
//extern ;

typedef struct {
	DISPENV	disp;
	DRAWENV	draw;
	u_long 	ot[OT_LEN];
	char 	p[PACKET_LEN];
} DB;


extern DB		db[2];

void init();
void display();
void sort_cube(MATRIX* mtx, VECTOR* pos, SVECTOR* rot, int r, int g, int b);