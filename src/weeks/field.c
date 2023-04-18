/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "field.h"

#include "cuphead/vhs.h"
#include "psx/archive.h"
#include "psx/mem.h"
#include "stage/stage.h"

//Field background structure
typedef struct
{
	//Field background base structure
	StageBack back;
	
	//Textures
	Gfx_Tex tex_back0; //Background
	Gfx_Tex tex_back1; //Trees
	Gfx_Tex tex_back2; //Foreground
} Back_Field;

//Field functions
void Back_Field_Tick(StageBack *back)
{
	//Back_Field *this = (Back_Field*)back;
	(void)back; //Only for remove warning
}

void Back_Field_DrawFG(StageBack *back)
{
	//Back_Field *this = (Back_Field*)back;
	(void)back; //Only for remove warning

	VHS_Draw();
}
void Back_Field_DrawBG(StageBack *back)
{
	Back_Field *this = (Back_Field*)back;
	
	fixed_t fx, fy, fscroll;

	//Draw foreground
	fx = stage.camera.x;
	fy = stage.camera.y;
	
	RECT foreground_src = {0, 0, 255, 255};
	RECT_FIXED foreground_dst = {
		FIXED_DEC(-525,1) - fx,
		FIXED_DEC(-314,1) - fy,
		FIXED_DEC(864,1),
		FIXED_DEC(486,1)
	};

	Debug_MoveTexture(&foreground_dst, 0, "foreground", fx, fy);
	
	Stage_DrawTex(&this->tex_back2, &foreground_src, &foreground_dst, stage.camera.bzoom);

	//Draw trees
	fscroll = FIXED_DEC(45,100);
	fx = FIXED_MUL(stage.camera.x, fscroll);
	fy = FIXED_MUL(stage.camera.y, fscroll);
	
	RECT trees_src = {0, 0, 255, 255};
	RECT_FIXED trees_dst = {
		FIXED_DEC(-276,1) - fx,
		FIXED_DEC(-225,1) - fy,
		FIXED_DEC(672,1),
		FIXED_DEC(378,1)
	};

	Debug_MoveTexture(&trees_dst, 1, "trees", fx, fy);
	
	Stage_DrawTex(&this->tex_back1, &trees_src, &trees_dst, stage.camera.bzoom);

	//Draw background
	fscroll = FIXED_DEC(2,10);
	fx = FIXED_MUL(stage.camera.x, fscroll);
	fy = FIXED_MUL(stage.camera.y, fscroll);
	
	RECT background_src = {0, 0, 228, 255};
	RECT_FIXED background_dst = {
		FIXED_DEC(-250,1) - fx,
		FIXED_DEC(-181,1) - fy,
		FIXED_DEC(672,1),
		FIXED_DEC(378,1)
	};

	Debug_MoveTexture(&background_dst, 2, "background", fx, fy);
	
	Stage_DrawTex(&this->tex_back0, &background_src, &background_dst, stage.camera.bzoom);
}

void Back_Field_Free(StageBack *back)
{
	Back_Field *this = (Back_Field*)back;
	
	//Free structure
	Mem_Free(this);
}

StageBack *Back_Field_New(void)
{
	//Allocate background structure
	Back_Field *this = (Back_Field*)Mem_Alloc(sizeof(Back_Field));
	if (this == NULL)
		return NULL;
	
	//Set background functions
	this->back.tick = Back_Field_Tick;
	this->back.draw_fg = Back_Field_DrawFG;
	this->back.draw_md = NULL;
	this->back.draw_bg = Back_Field_DrawBG;
	this->back.free = Back_Field_Free;

	stage.defaultzoom = FIXED_DEC(8,10);
	
	//Load background textures
	IO_Data arc_back = IO_Read("\\FIELD\\BACK.ARC;1");
	Gfx_LoadTex(&this->tex_back0, Archive_Find(arc_back, "back0.tim"), 0);
	Gfx_LoadTex(&this->tex_back1, Archive_Find(arc_back, "back1.tim"), 0);
	Gfx_LoadTex(&this->tex_back2, Archive_Find(arc_back, "back2.tim"), 0);
	Mem_Free(arc_back);
	
	return (StageBack*)this;
}
