/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "cupgame.h"

#include "psx/mem.h"
#include "psx/archive.h"
#include "stage/stage.h"
#include "psx/main.h"

//CupGaming character structure
enum
{
	CupGaming_ArcMain_Game,
	
	CupGaming_Arc_Max,
};

typedef struct
{
	//Character base structure
	Character character;
	
	//Render data and state
	IO_Data arc_main;
	IO_Data arc_ptr[CupGaming_Arc_Max];
	
	Gfx_Tex tex;
	u8 frame, tex_id;
} Char_CupGaming;

//CupGaming character definitions
static const CharFrame char_cupgame_frame[] = {
	{CupGaming_ArcMain_Game , {  0,   0,  83,  86}, { 42, 100}}, //0 idle 1
	{CupGaming_ArcMain_Game , { 83,   0,  83,  86}, { 42, 100}}, //1 idle 2
	{CupGaming_ArcMain_Game , {166,   0,  83,  86}, { 41, 100}}, //2 idle 3
	{CupGaming_ArcMain_Game , {  0,  86,  83,  85}, { 42,  98}}, //3 idle 4
	{CupGaming_ArcMain_Game , { 83,  86,  83,  85}, { 42,  98}}, //4 idle 5
	{CupGaming_ArcMain_Game , {166,  86,  82,  86}, { 40,  99}}, //5 idle 6
};

static const Animation char_cupgame_anim[CharAnim_Max] = {
	{1, (const u8[]){ 0,  0,  1,  2,  3,  4,  5,  5, ASCR_CHGANI, CharAnim_Idle}}, //CharAnim_Idle
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},         //CharAnim_Left
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_LeftAlt
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},         //CharAnim_Down
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_DownAlt
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},        //CharAnim_Up
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_UpAlt
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},         //CharAnim_Right
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_RightAlt

	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_Special1
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_Special2
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_Special3
};

//CupGaming character functions
void Char_CupGaming_SetFrame(void *user, u8 frame)
{
	Char_CupGaming *this = (Char_CupGaming*)user;
	
	//Check if this is a new frame
	if (frame != this->frame)
	{
		//Check if new art shall be loaded
		const CharFrame *cframe = &char_cupgame_frame[this->frame = frame];
		if (cframe->tex != this->tex_id)
			Gfx_LoadTex(&this->tex, this->arc_ptr[this->tex_id = cframe->tex], 0);
	}
}

void Char_CupGaming_Tick(Character *character)
{
	Char_CupGaming *this = (Char_CupGaming*)character;
	
	Character_CheckAnimationUpdate(character);
	
	//Animate and draw
	Animatable_Animate(&character->animatable, (void*)this, Char_CupGaming_SetFrame);
	Character_Draw(character, &this->tex, &char_cupgame_frame[this->frame]);
}

void Char_CupGaming_SetAnim(Character *character, u8 anim)
{
	//Set animation
	Animatable_SetAnim(&character->animatable, anim);
	Character_CheckStartSing(character);
}

void Char_CupGaming_Free(Character *character)
{
	Char_CupGaming *this = (Char_CupGaming*)character;
	
	//Free art
	Mem_Free(this->arc_main);
}

Character *Char_CupGaming_New(fixed_t x, fixed_t y)
{
	//Allocate cupgame object
	Char_CupGaming *this = Mem_Alloc(sizeof(Char_CupGaming));
	if (this == NULL)
	{
		sprintf(error_msg, "[Char_CupGaming_New] Failed to allocate cupgame object");
		ErrorLock();
		return NULL;
	}
	
	//Initialize character
	this->character.tick = Char_CupGaming_Tick;
	this->character.set_anim = Char_CupGaming_SetAnim;
	this->character.free = Char_CupGaming_Free;
	
	Animatable_Init(&this->character.animatable, char_cupgame_anim);
	Character_Init((Character*)this, x, y);
	
	//Set character information
	this->character.spec = 0;
	
	//Health Icon
	this->character.health_i = 1;

	//Health Bar
	this->character.health_b = 0xFFAE67D0;

	//Character scale
	this->character.scale = FIXED_DEC(12,10);
	
	this->character.focus_x = FIXED_DEC(65,1);
	this->character.focus_y = FIXED_DEC(-115,1);
	this->character.focus_zoom = FIXED_DEC(1,1);
	
	//Load art
	this->arc_main = IO_Read("\\CHAR\\CUPGAME.TIM;1");
	
	IO_Data *arc_ptr = this->arc_ptr;
	*arc_ptr = this->arc_main;
	
	//Initialize render state
	this->tex_id = this->frame = 0xFF;
	
	return (Character*)this;
}
