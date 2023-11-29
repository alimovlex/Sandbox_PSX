typedef struct {
	DISPENV disp;
	DRAWENV draw;
} Framebuffer;

typedef struct {
	Framebuffer db[2];
	int         db_active;
} RenderContext;

extern RenderContext ctx;

void init_context(RenderContext *ctx);
void display(RenderContext *ctx);