#include "vhs.h"

#include "psx/gfx.h"
#include "psx/random.h"
#include "psx/timer.h"

//Lines max that can appear in screen
#define LINES_LIMIT 5

static s16 vhs_x[LINES_LIMIT], vhs_y[LINES_LIMIT], vhs_w[LINES_LIMIT], vhs_h[LINES_LIMIT];

void VHS_Randomize(void)
{
	for (u8 i = 0; i < LINES_LIMIT; i++)
	{
		vhs_x[i] = RandomRange(0, SCREEN_WIDTH);
		vhs_y[i] = RandomRange(0, SCREEN_HEIGHT);
		vhs_w[i] = RandomRange(1, 2);
		vhs_h[i] = RandomRange(1, 4);
	}
}
void VHS_Draw(void)
{
	//Randomize lines each 12 frames
	if (animf_count & 2)
		VHS_Randomize();

	for (u8 i = 0; i < LINES_LIMIT; i++)
	{
		RECT vhs_dst = {
			vhs_x[i], 
			vhs_y[i],
			vhs_w[i],
			vhs_h[i],
		};

		Gfx_DrawRect(&vhs_dst, 255, 255, 255);
	}
}