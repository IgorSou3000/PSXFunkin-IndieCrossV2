/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "menu.h"

#include "psx/mem.h"
#include "psx/main.h"
#include "psx/timer.h"
#include "psx/io.h"
#include "psx/gfx.h"
#include "psx/audio.h"
#include "psx/pad.h"
#include "psx/archive.h"
#include "psx/mutil.h"
#include "psx/save.h"

#include "psx/font.h"
#include "psx/trans.h"
#include "psx/loadscr.h"

#include "stage/stage.h"
#include "characters/cupgame.h"

//Menu messages
static const char *funny_messages[][2] = {
	{"PSX PORT BY CUCKYDEV", "YOU KNOW IT"},
	{"PORTED BY CUCKYDEV", "WHAT YOU GONNA DO"},
	{"FUNKIN", "FOREVER"},
	{"WHAT THE HELL", "RITZ PSX"},
	{"LIKE PARAPPA", "BUT COOLER"},
	{"THE JAPI", "EL JAPI"},
	{"PICO FUNNY", "PICO FUNNY"},
	{"OPENGL BACKEND", "BY CLOWNACY"},
	{"CUCKYFNF", "SETTING STANDARDS"},
	{"lool", "inverted colours"},
	{"NEVER LOOK AT", "THE ISSUE TRACKER"},
	{"PSXDEV", "HOMEBREW"},
	{"ZERO POINT ZERO TWO TWO EIGHT", "ONE FIVE NINE ONE ZERO FIVE"},
	{"DOPE ASS GAME", "PLAYSTATION MAGAZINE"},
	{"NEWGROUNDS", "FOREVER"},
	{"NO FPU", "NO PROBLEM"},
	{"OK OKAY", "WATCH THIS"},
	{"ITS MORE MALICIOUS", "THAN ANYTHING"},
	{"USE A CONTROLLER", "LOL"},
	{"SNIPING THE KICKSTARTER", "HAHA"},
	{"SHITS UNOFFICIAL", "NOT A PROBLEM"},
	{"SYSCLK", "RANDOM SEED"},
	{"THEY DIDNT HIT THE GOAL", "STOP"},
	{"FCEFUWEFUETWHCFUEZDSLVNSP", "PQRYQWENQWKBVZLZSLDNSVPBM"},
	{"THE FLOORS ARE", "THREE DIMENSIONAL"},
	{"PSXFUNKIN BY CUCKYDEV", "SUCK IT DOWN"},
	{"PLAYING ON EPSXE HUH", "YOURE THE PROBLEM"},
	{"NEXT IN LINE", "ATARI"},
	{"HAXEFLIXEL", "COME ON"},
	{"HAHAHA", "I DONT CARE"},
	{"GET ME TO STOP", "TRY"},
	{"FNF MUKBANG GIF", "THATS UNRULY"},
	{"OPEN SOURCE", "FOREVER"},
	{"ITS A PORT", "ITS WORSE"},
	{"WOW GATO", "WOW GATO"},
	{"BALLS FISH", "BALLS FISH"},
};

//Menu Characters enum
enum
{
	Character_Cuphead,

	Character_Max,
};

//Menu state
const u8 play_buttom_lines_limit = 1;
static struct
{
	//Menu state
	u8 page, next_page;
	boolean page_swap;
	u8 select, next_select;
	
	fixed_t scroll;
	fixed_t trans_time;

	SFX sounds[10];
	
	//Page specific state
	union
	{
		struct
		{
			u8 funny_message;
		} opening;
		struct
		{
			fixed_t logo_bump;
			fixed_t fade, fadespd;
			fixed_t play_lines_opacity;
			u16 play_lines_x;
		} title;
		struct
		{
			fixed_t option_x;
		} main;
		struct
		{
			fixed_t fade, fadespd;
		} story;
	} page_state;
	
	union
	{
		struct
		{
			u8 id, diff;
			boolean story;
		} stage;
	} page_param;
	
	//Menu assets
	Gfx_Tex tex_back, tex_titlebg, tex_title, tex_main, tex_leftbg, tex_storyhud, tex_storybg, tex_freeplay_select;
	FontData font_bold, font_arial, font_cdr;

	Character *characters[Character_Max];
} menu;

//Internal menu functions
char menu_text_buffer[0x100];

static const char *Menu_LowerIf(const char *text, boolean lower)
{
	//Copy text
	char *dstp = menu_text_buffer;
	if (lower)
	{
		for (const char *srcp = text; *srcp != '\0'; srcp++)
		{
			if (*srcp >= 'A' && *srcp <= 'Z')
				*dstp++ = *srcp | 0x20;
			else
				*dstp++ = *srcp;
		}
	}
	else
	{
		for (const char *srcp = text; *srcp != '\0'; srcp++)
		{
			if (*srcp >= 'a' && *srcp <= 'z')
				*dstp++ = *srcp & ~0x20;
			else
				*dstp++ = *srcp;
		}
	}
	
	//Terminate text
	*dstp++ = '\0';
	return menu_text_buffer;
}

//Since the psxfunkin scroll code is basically the same in every mode, i making this be a function
u8 Menu_Scroll(u8 select, u8 optionsn, SFX* scroll_sfx)
{
	//Change options
	if (pad_state.press & PAD_UP)
	{
		menu.page_state.main.option_x = 0;
		//Play Scroll Sound
		Audio_PlaySFX(*scroll_sfx, 80);
		if (select > 0)
				select--;
		else
				select = optionsn;
	}

	if (pad_state.press & PAD_DOWN)
	{
		menu.page_state.main.option_x = 0;
		//Play Scroll Sound
		Audio_PlaySFX(*scroll_sfx, 80);
		if (select < optionsn)
			select++;
		else
			select = 0;
	}
	return select;
}

static void Menu_DrawBack(boolean flash, s32 scroll, u8 r0, u8 g0, u8 b0, u8 r1, u8 g1, u8 b1)
{
	RECT back_src = {0, 0, 255, 255};
	RECT back_dst = {0, -scroll - SCREEN_WIDEADD2, SCREEN_WIDTH, SCREEN_WIDTH * 4 / 5};
	
	if (flash || (animf_count & 4) == 0)
		Gfx_DrawTexCol(&menu.tex_back, &back_src, &back_dst, r0, g0, b0);
	else
		Gfx_DrawTexCol(&menu.tex_back, &back_src, &back_dst, r1, g1, b1);
}

static void Menu_DifficultySelector(s32 x, s32 y)
{
	//Change difficulty
	if (menu.next_page == menu.page && Trans_Idle())
	{
		if (pad_state.press & PAD_LEFT)
		{
			if (menu.page_param.stage.diff > StageDiff_Easy)
				menu.page_param.stage.diff--;
			else
				menu.page_param.stage.diff = StageDiff_Hard;
		}
		if (pad_state.press & PAD_RIGHT)
		{
			if (menu.page_param.stage.diff < StageDiff_Hard)
				menu.page_param.stage.diff++;
			else
				menu.page_param.stage.diff = StageDiff_Easy;
		}
	}
	
	//Draw difficulty
	static const RECT diff_srcs[] = {
		{  0,  0,118, 10},
		{  0, 10,118, 10},
		{  0, 20,118, 10},
	};
	
	const RECT *diff_src = &diff_srcs[menu.page_param.stage.diff];

	RECT diff_dst = {
		x ,
		y - 9,
		diff_src->w,
		diff_src->h,
	};

	Gfx_DrawTex(&menu.tex_storyhud, diff_src, &diff_dst);
}

static void Menu_DrawWeek(const char *week, s32 x, s32 y, u32 col, boolean is_selected)
{
	u8 r, g, b;

	r = (((col >> 16) & 0xFF) >> 1) / (2 - is_selected);
	g = (((col >> 8) & 0xFF) >> 1) / (2 - is_selected);
	b = (((col >> 0) & 0xFF) >> 1) / (2 - is_selected);

	//Draw label
	RECT label_src = {0, 144, 49, 16};
	RECT label_dst = {x, y,   49, 16};

	Gfx_DrawTexCol(&menu.tex_storyhud, &label_src, &label_dst, r, g, b);
		
	//Number
	x += 50;
	for (; *week != '\0'; week++)
	{
		//Draw number
		u8 i = *week - '1';
			
		if (*week == '?')
		{
			//Draw two ?
			RECT num_src = {0, 160, 16, 16};
			RECT num_dst = {x, y, 16, 16};

			Gfx_DrawTexCol(&menu.tex_storyhud, &num_src, &num_dst, r, g, b);
			num_dst.x += 10;
			Gfx_DrawTexCol(&menu.tex_storyhud, &num_src, &num_dst, r, g, b);
		}

		else
		{
			RECT num_src = {64 + (i << 4), 144, 16, 16};
			RECT num_dst = {x, y, 16, 16};

			Gfx_DrawTexCol(&menu.tex_storyhud, &num_src, &num_dst, r, g, b);

		}
		x += 16;
	}
}

static void Menu_DrawHealth(u8 i, s16 x, s16 y, boolean is_selected)
{
	//Icon Size
	u8 icon_size = 38;

	//Get src and dst
	RECT src = {
		(i % 6) * icon_size,
		(i / 6) * icon_size,
		icon_size,
		icon_size
	};
	RECT dst = {
		x,
		y,
		38,
		38
	};

	u8 col = (is_selected) ? 128 : 64;
	
	//Draw health icon
	Gfx_DrawTexCol(&stage.tex_hud1, &src, &dst, col, col, col);
}

//Menu functions
void Menu_Load(MenuPage page)
{
	//Initialize with a special stage id for not trigger any stage events (like gf cheer)
	stage.stage_id = StageId_Max;

	//Load menu assets
	IO_Data menu_arc = IO_Read("\\MENU\\MENU.ARC;1");
	Gfx_LoadTex(&menu.tex_back,  		Archive_Find(menu_arc, "back.tim"),  0);
	Gfx_LoadTex(&menu.tex_titlebg,  Archive_Find(menu_arc, "titlebg.tim"), 0);
	Gfx_LoadTex(&menu.tex_title, 		Archive_Find(menu_arc, "title.tim"), 0);
	Gfx_LoadTex(&menu.tex_main, 		Archive_Find(menu_arc, "main.tim"), 0);
	Gfx_LoadTex(&menu.tex_leftbg,   Archive_Find(menu_arc, "leftbg.tim"), 0);
	Gfx_LoadTex(&menu.tex_storyhud, Archive_Find(menu_arc, "storyhud.tim"), 0);
	Gfx_LoadTex(&menu.tex_storybg, Archive_Find(menu_arc, "BG.tim"), 0);
	Gfx_LoadTex(&menu.tex_freeplay_select, Archive_Find(menu_arc, "freesel.tim"), 0);
	Gfx_LoadTex(&stage.tex_hud1, 		Archive_Find(menu_arc, "hud1.tim"), 0);
	Mem_Free(menu_arc);
	
	FontData_Load(&menu.font_bold, Font_Bold, false);
	FontData_Load(&menu.font_arial, Font_Arial, false);
	FontData_Load(&menu.font_cdr, Font_CDR, false);
	
	//Initialize Girlfriend and stage
	menu.characters[Character_Cuphead] = Char_CupGaming_New(FIXED_DEC(102,1), FIXED_DEC(118,1));

	stage.camera.x = stage.camera.y = FIXED_DEC(0,1);
	stage.camera.bzoom = FIXED_UNIT;
	stage.gf_speed = 4;
	menu.page_state.title.play_lines_x = 0;
	menu.page_state.title.play_lines_opacity = 100;
	menu.page_state.main.option_x = 0;
	
	//Initialize menu state
	menu.select = menu.next_select = 0;
	
	menu.page = menu.next_page = page;
	
	menu.page_swap = true;
	
	menu.trans_time = 0;
	Trans_Clear();
	
	stage.song_step = 0;

	//Clear Alloc for sound effect work (otherwise the sounds will not work)
	Audio_ClearAlloc();

	//Sound Effects Path
	const char* sfx_path[] = {
		"\\SOUNDS\\SCROLL.VAG;1",
		"\\SOUNDS\\CONFIRM.VAG;1",
		"\\SOUNDS\\CANCEL.VAG;1",
		"\\SOUNDS\\CUPTRANS.VAG;1"
	};

	//Load sound effects
	for (u8 i = 0; i < COUNT_OF(sfx_path); i++)
	{
		menu.sounds[i] = Audio_LoadSFX(sfx_path[i]);
	}
	
	//Play menu music
	Audio_PlayXA_Track(XA_GettinFreaky, 0x40, 0, true);
	
	//Set background colour
	Gfx_SetClear(0, 0, 0);
}

void Menu_Unload(void)
{
	//Free Menu Characters
	for (u8 i = 0; i < Character_Max; i++)
	{
		Character_Free(menu.characters[i]);
	}
}

void Menu_ToStage(StageId id, StageDiff diff, boolean story)
{
	menu.next_page = MenuPage_Stage;
	menu.page_param.stage.id = id;
	menu.page_param.stage.story = story;
	menu.page_param.stage.diff = diff;
	Trans_Start();
}

void Menu_Tick(void)
{
	//Clear per-frame flags
	stage.flag &= ~STAGE_FLAG_JUST_STEP;
	
	//Get song position
	u16 bpm = 117; // Freaky BPM(117)

	//Weird formula that make the xa milliseconds be a bpm
	u16 next_step = Audio_TellXA_Milli() / (256 * 60 / bpm);

	if (next_step != stage.song_step)
	{
		if (next_step >= stage.song_step)
			stage.flag |= STAGE_FLAG_JUST_STEP;
		stage.song_step = next_step;
		stage.song_beat = stage.song_step / 4;
	}
	
	//Handle transition out
	if (Trans_Tick())
	{
		//Play settings music
		if (menu.next_page == MenuPage_Options && menu.page == MenuPage_Main)
				Audio_PlayXA_Track(XA_Settings, 0x40, 1, true);

		//Play main menu music
		if (menu.next_page == MenuPage_Main && menu.page == MenuPage_Options)
				Audio_PlayXA_Track(XA_GettinFreaky, 0x40, 0, true);

		//Change to set next page
		menu.page_swap = true;
		menu.page = menu.next_page;
		menu.select = menu.next_select;
	}
	
	//Tick menu page
	MenuPage exec_page;
	switch (exec_page = menu.page)
	{
		case MenuPage_Title:
		{
			//Initialize page
			if (menu.page_swap)
			{
				menu.page_state.title.logo_bump = (FIXED_DEC(6,1) / 24) - 1;
			}
			
			//Draw white fade
			if (menu.page_state.title.fade > 0)
			{
				static const RECT flash = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
				u8 flash_col = menu.page_state.title.fade >> FIXED_SHIFT;
				Gfx_BlendRect(&flash, flash_col, flash_col, flash_col, 1);
				menu.page_state.title.fade -= FIXED_MUL(menu.page_state.title.fadespd, timer_dt);
			}
			
			//Go to main menu when start is pressed
			if (menu.trans_time > 0 && (menu.trans_time -= timer_dt) <= 0)
				Trans_Start();
			
			if ((pad_state.press & PAD_START) && menu.next_page == menu.page && Trans_Idle())
			{
				//Play Confirm Sound
				Audio_PlaySFX(menu.sounds[1], 80);
				menu.trans_time = FIXED_UNIT;
				menu.page_state.title.fade = FIXED_DEC(255,1);
				menu.page_state.title.fadespd = FIXED_DEC(300,1);
				menu.next_page = MenuPage_Main;
				menu.next_select = 0;
			}
			
			//Draw Friday Night Funkin' logo
			if ((stage.flag & STAGE_FLAG_JUST_STEP) && (stage.song_step & 0x3) == 0 && menu.page_state.title.logo_bump == 0)
				menu.page_state.title.logo_bump = (FIXED_DEC(6,1) / 24) - 1;
			
			static const fixed_t logo_scales[] = {
				FIXED_DEC(101,100),
				FIXED_DEC(102,100),
				FIXED_DEC(103,100),
				FIXED_DEC(105,100),
				FIXED_DEC(110,100),
				FIXED_DEC(1,1),
			};

			const u16 logo_width = 180;
			const u16 logo_height = 130;

			fixed_t logo_scale = logo_scales[(menu.page_state.title.logo_bump * 24) >> FIXED_SHIFT];
			u32 x_rad = (logo_scale * (logo_width / 2)) >> FIXED_SHIFT;
			u32 y_rad = (logo_scale * (logo_height / 2)) >> FIXED_SHIFT;
			
			RECT logo_src = {0, 0, logo_width, logo_height};
			RECT logo_dst = {
				100 - x_rad + (SCREEN_WIDEADD2 / 2),
				SCREEN_HEIGHT2 - y_rad,
				x_rad * 2,
				y_rad * 2
			};
			Gfx_DrawTex(&menu.tex_title, &logo_src, &logo_dst);
			
			if (menu.page_state.title.logo_bump > 0)
				if ((menu.page_state.title.logo_bump -= timer_dt) < 0)
					menu.page_state.title.logo_bump = 0;
			
			//Draw "Play"
			RECT playtext_src = {0, 192, 62, 17};
			RECT playtext_dst = {SCREEN_WIDTH2 + 38, SCREEN_HEIGHT - 40, 62, 17};
			Gfx_DrawTex(&menu.tex_title, &playtext_src, &playtext_dst);

			RECT playbuttom_src = {0, 223, 89, 29};
			RECT playbuttom_dst = {SCREEN_WIDTH2 + 28, SCREEN_HEIGHT - 45, 89, 29};
			Gfx_DrawTex(&menu.tex_title, &playbuttom_src, &playbuttom_dst);

			if (menu.page_state.title.play_lines_x++ >= 50)
				menu.page_state.title.play_lines_x = 0;

			for (u8 i = 0; i < play_buttom_lines_limit; i++)
			{
				RECT play_lines_src = {91, 223, 12, 29};
				RECT play_lines_dst = {
					SCREEN_WIDTH2 + 112 + menu.page_state.title.play_lines_x + (i * 16), 
					SCREEN_HEIGHT - 45,
					 12, 
					 29
				};

				menu.page_state.title.play_lines_opacity = menu.page_state.title.play_lines_x * 4;
				u16 col = menu.page_state.title.play_lines_opacity;
				FntPrint("%d",col);

				Gfx_DrawTexCol(&menu.tex_title, &play_lines_src, &play_lines_dst, 255 - col, 255 - col, 255 - col);
			}

			//Draw Title background
			RECT titlebg_src = {0, 0, 255, 240};
			RECT titlebg_dst = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

			Gfx_DrawTex(&menu.tex_titlebg, &titlebg_src, &titlebg_dst);

			break;
		}
		case MenuPage_Main:
		{
			static const char *menu_options[] = {
				"STORY MODE",
				"FREEPLAY",
				"OPTIONS",
				"CREDITS",
			};

			//Draw white fade
			if (menu.page_state.title.fade > 0)
			{
				static const RECT flash = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
				u8 flash_col = menu.page_state.title.fade >> FIXED_SHIFT;
				Gfx_BlendRect(&flash, flash_col, flash_col, flash_col, 1);
				menu.page_state.title.fade -= FIXED_MUL(menu.page_state.title.fadespd, timer_dt);
			}
			
			//Initialize page
			if (menu.page_swap)
				menu.scroll = menu.select * FIXED_DEC(8,1);
			
			//Handle option and selection
			if (menu.trans_time > 0 && (menu.trans_time -= timer_dt) <= 0)
				Trans_Start();
			
			if (menu.next_page == menu.page && Trans_Idle())
			{
				//Change option
				menu.select = Menu_Scroll(menu.select, COUNT_OF(menu_options) - 1, &menu.sounds[0]);
				
				//Select option if cross is pressed
				if (pad_state.press & (PAD_START | PAD_CROSS))
				{
					switch (menu.select)
					{
						case 0: //Story Mode
							menu.next_page = MenuPage_Story;
							break;
						case 1: //Freeplay
							menu.next_page = MenuPage_Freeplay_Selection;
							break;
						case 2: //Options
							menu.next_page = MenuPage_Options;
							break;
						case 3: //Credits
							menu.next_page = MenuPage_Credits;
							break;
					}
					//Play Confirm Sound
					Audio_PlaySFX(menu.sounds[1], 80);

					menu.page_state.main.option_x = 0;
					menu.next_select = 0;
					menu.trans_time = FIXED_UNIT;
					menu.page_state.title.fade = FIXED_DEC(255,1);
					menu.page_state.title.fadespd = FIXED_DEC(300,1);
				}
				
				//Return to title screen if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					//Play Cancel Sound
					Audio_PlaySFX(menu.sounds[2], 80);
					menu.next_page = MenuPage_Title;
					Trans_Start();
				}
			}
			
			//Draw options
			s32 next_scroll = menu.select * FIXED_DEC(8,1);

			menu.scroll += (next_scroll - menu.scroll) / 4;

			fixed_t next_option = FIXED_DEC(22,1);
			menu.page_state.main.option_x += (next_option - menu.page_state.main.option_x) / 4;

			static RECT option_dst[4];
			
			if (menu.next_page == menu.page || menu.next_page == MenuPage_Title)
			{
				//Draw all options
				for (u8 i = 0; i < COUNT_OF(menu_options); i++)
				{
					RECT option_src = {0, i * 29, 182, 29};

					option_dst[i].x = (i == menu.select) ? -22 + (menu.page_state.main.option_x >> FIXED_SHIFT) : -22; 

					option_dst[i].y = SCREEN_HEIGHT2 - 29 + i * 32; 
					option_dst[i].w = 182; 
					option_dst[i].h = 29;

					u8 col = (i == menu.select) ? 128 : 64;
			
					Gfx_DrawTexCol(&menu.tex_main, &option_src, &option_dst[i], col, col, col);
				}
			}
			else if (animf_count & 2)
			{
				RECT option_src = {0, menu.select * 29, 182, 29};
				option_dst[menu.select].x = -22 + (menu.page_state.main.option_x >> FIXED_SHIFT);
				option_dst[menu.select].y = SCREEN_HEIGHT2 - 29 + menu.select * 32; 
				option_dst[menu.select].w = 182; 
				option_dst[menu.select].h = 29;

				Gfx_DrawTex(&menu.tex_main, &option_src, &option_dst[menu.select]);
			}
			
			//Draw background
			Menu_DrawBack(
				true, 
				8,
				128, 128, 128,
				0, 0, 0
			);
			break;
		}
		case MenuPage_Story:
		{
			static fixed_t scale = FIXED_DEC(20,1);
			static RECT trans_src = {0, 0, 160, 120};
			static RECT_FIXED trans_dst = {
				FIXED_DEC(-80,1), 
				FIXED_DEC(-60,1), 
				FIXED_DEC(160,1), 
				FIXED_DEC(120,1)
			};

			if (scale > FIXED_DEC(2,1))
				Stage_DrawTex(&cuphead_trans_tex, &trans_src, &trans_dst, scale);
			else
			{
				RECT screen = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
				Gfx_DrawRect(&screen, 0, 0, 0);
			}

			static const struct
			{
				const char *week;
				StageId stage;
				u32 col;
				u32 selected_col;
			} menu_options[] = {
				{"1", StageId_Snake_Eyes, 0xFFF82062, 0xFFFFDCC9},
				{"?", StageId_Snake_Eyes, 0xFF20B2F8, 0xFFC9F9FF},
				{"?", StageId_Snake_Eyes, 0xFFF7A333, 0xFFFFDFCE},
			};
			
			//Initialize page
			if (menu.page_swap)
			{
				menu.scroll = 0;
				menu.page_param.stage.diff = StageDiff_Normal;
			}
			
			//Draw difficulty selector
			Menu_DifficultySelector(0, 60);
			
			//Handle option and selection
			if ((menu.trans_time > 0 && (menu.trans_time -= timer_dt) <= 0) || (scale < FIXED_DEC(2,1)))
				Trans_Start();
			
			if (menu.next_page == menu.page && Trans_Idle())
			{
				//Change option
				menu.select = Menu_Scroll(menu.select, COUNT_OF(menu_options) - 1, &menu.sounds[0]);
				
				//Select option if cross is pressed
				if (pad_state.press & (PAD_START | PAD_CROSS) && *menu_options[menu.select].week != '?')
				{
					if (menu.select == 0)
						Audio_PlaySFX(menu.sounds[3], 80); //Cuphead transition
					else
						Audio_PlaySFX(menu.sounds[1], 80); //Normal transition

					menu.next_page = MenuPage_Stage;
					menu.page_param.stage.id = menu_options[menu.select].stage;
					menu.page_param.stage.story = true;
					menu.trans_time = FIXED_UNIT;
				}
				
				//Return to main menu if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					//Play Cancel Sound
					Audio_PlaySFX(menu.sounds[2], 80);

					menu.next_page = MenuPage_Main;
					menu.next_select = 0; //Story Mode
					Trans_Start();
				}
			}
			
			//Draw all options
			for (u8 i = 0; i < COUNT_OF(menu_options); i++)
			{
				s32 y = SCREEN_HEIGHT2 - 16 + (i * 22);
				if (y <= 16)
					continue;
				if (y >= SCREEN_HEIGHT)
					break;

				//Draw all options
				Menu_DrawWeek(menu_options[i].week, 28, y, (i == menu.select) ? menu_options[i].selected_col : menu_options[i].col, (i == menu.select));
			}

			//Draw "Story Mode"
			RECT story_src = {0, 0, 182, 29};
			RECT story_dst = {-12, 10, 182, 29};

			Gfx_DrawTex(&menu.tex_main, &story_src, &story_dst);

			RECT leftbg_src = {0, 0, 240, 240};
			RECT leftbg_dst = {0, 0, SCREEN_HEIGHT, SCREEN_HEIGHT};
			Gfx_DrawTex(&menu.tex_leftbg, &leftbg_src, &leftbg_dst);

			u16 v0 = ((animf_count % 2) != 0);

			if (menu.next_page == MenuPage_Stage && v0 && scale >= FIXED_DEC(2,1))
			{
				scale -= FIXED_DEC(1,1);
			}

			if (menu.select == 0)
			{
				//Draw Cuphead
				menu.characters[Character_Cuphead]->tick(menu.characters[Character_Cuphead]);

				//Draw Cuphead Background
				RECT cupbg_src = {0, 0, 240, 189};
				RECT cupbg_dst = {100, 0, SCREEN_HEIGHT, 252};
				Gfx_DrawTex(&menu.tex_storybg, &cupbg_src, &cupbg_dst);
			}

			break;
	}
		case MenuPage_Freeplay_Selection:
		{
			static const struct
			{
				const char* text;
				MenuPage page;
			} menu_options[] = {
				{"story", MenuPage_Freeplay_Story},
				{"?", MenuPage_Freeplay_Story},
				{"?", MenuPage_Freeplay_Story},
			};

			//Draw white fade
			if (menu.page_state.title.fade > 0)
			{
				static const RECT flash = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
				u8 flash_col = menu.page_state.title.fade >> FIXED_SHIFT;
				Gfx_BlendRect(&flash, flash_col, flash_col, flash_col, 1);
				menu.page_state.title.fade -= FIXED_MUL(menu.page_state.title.fadespd, timer_dt);
			}

			//Handle option and selection
			if ((menu.trans_time > 0 && (menu.trans_time -= timer_dt) <= 0))
				Trans_Start();
			
			if (menu.next_page == menu.page && Trans_Idle())
			{
				//Select option if cross is pressed
				if (pad_state.press & (PAD_START | PAD_CROSS) && *menu_options[menu.select].text != '?')
				{
					menu.next_page = menu_options[menu.select].page;
					//Play Confirm Sound
					Audio_PlaySFX(menu.sounds[1], 80);

					menu.page_state.main.option_x = 0;
					menu.next_select = 0;
					menu.trans_time = FIXED_UNIT;
					menu.page_state.title.fade = FIXED_DEC(255,1);
					menu.page_state.title.fadespd = FIXED_DEC(300,1);
				}

				//Change options
				if (pad_state.press & PAD_LEFT)
				{
					menu.page_state.main.option_x = 0;
					//Play Scroll Sound
					Audio_PlaySFX(menu.sounds[0], 80);
					if (menu.select > 0)
							menu.select--;
					else
							menu.select = COUNT_OF(menu_options) - 1;
				}

				if (pad_state.press & PAD_RIGHT)
				{
					menu.page_state.main.option_x = 0;
					//Play Scroll Sound
					Audio_PlaySFX(menu.sounds[0], 80);
					if (menu.select < COUNT_OF(menu_options) - 1)
						menu.select++;
					else
						menu.select = 0;
				}

				//Return to main menu if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					//Play Cancel Sound
					Audio_PlaySFX(menu.sounds[2], 80);

					menu.next_page = MenuPage_Main;
					menu.next_select = 1; //Freeplay
					Trans_Start();
				}
			}

			if (menu.next_page == menu.page || menu.next_page == MenuPage_Main)
			{
				for (u8 i = 0; i < COUNT_OF(menu_options); i++)
				{
					RECT option_src = {60 + (i * 60), 0, 60, 148};
					RECT option_dst = {
						30 + i * 100, 
						5,
					  66, //60 * 1.1 = 66
					  163, //148 * 1.1 = 163
					};

					if (*menu_options[i].text == '?')
						option_src.x = 0;

					u8 col = (i == menu.select) ? 128 : 64;

					Gfx_DrawTexCol(&menu.tex_freeplay_select, &option_src, &option_dst, col, col, col);

					RECT option_name_src = {0, 150 + (i * 30), 116, 30};
					RECT option_name_dst = {(i * 103), 165, 116, 30};

					if (*menu_options[i].text != '?')
						Gfx_DrawTexCol(&menu.tex_freeplay_select, &option_name_src, &option_name_dst, col, col, col);
				}
			}
			else
			{
				RECT option_src = {60 * (menu.select + 1), 0, 60, 148};
				RECT option_dst = {
					30 + menu.select * 100, 
					5,
					66, //60 * 1.1 = 66
					163, //148 * 1.1 = 163
				};
				Gfx_DrawTex(&menu.tex_freeplay_select, &option_src, &option_dst);
			}

			//Draw background
			Menu_DrawBack(
				true, 
				8,
				128, 128, 128,
				0, 0, 0
			);
			break;
		}
		case MenuPage_Freeplay_Story:
		{
			static const struct
			{
				StageId stage; //The stage
				const char *text; //The text of the song
				u8 icon; //The character icon
			} menu_options[] = {
				{StageId_Snake_Eyes, "SNAKE EYES", 1},
				{StageId_Technicolor_Tussle, "TECNHICOLOR TUSSLE", 1},
				{StageId_Knockout, "KNOCKOUT", 1},
			};
			
			//Initialize page
			if (menu.page_swap)
			{
				menu.scroll = COUNT_OF(menu_options) * FIXED_DEC(30 + SCREEN_HEIGHT2,1);
				menu.page_param.stage.diff = StageDiff_Normal;
			}
			
			//Draw difficulty selector
			Menu_DifficultySelector(SCREEN_WIDTH - 100, SCREEN_HEIGHT2 - 48);
			
			//Handle option and selection
			if (menu.next_page == menu.page && Trans_Idle())
			{
				//Change option
				menu.select = Menu_Scroll(menu.select, COUNT_OF(menu_options) - 1, &menu.sounds[0]);
				
				//Select option if cross is pressed
				if (pad_state.press & (PAD_START | PAD_CROSS))
				{
					menu.next_page = MenuPage_Stage;
					menu.page_param.stage.id = menu_options[menu.select].stage;
					menu.page_param.stage.story = false;
					Trans_Start();
				}
				
				//Return to main menu if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					//Play Cancel Sound
					Audio_PlaySFX(menu.sounds[2], 80);

					menu.next_page = MenuPage_Freeplay_Selection;
					menu.next_select = 0; //Story songs
					Trans_Start();
				}
			}

			//Draw Score
			char scoredisp[0x100];
			sprintf(scoredisp, "PERSONAL BEST: %d", stage.save.savescore[menu_options[menu.select].stage][menu.page_param.stage.diff] * 10);

			#ifndef NOSAVE
				menu.font_arial.draw(&menu.font_arial,
					scoredisp,
					SCREEN_WIDTH - 170,
					SCREEN_HEIGHT / 2 - 75,
					FontAlign_Left
				);
			#endif
			
			//Draw options
			s32 next_scroll = menu.select * FIXED_DEC(30,1);
			menu.scroll += (next_scroll - menu.scroll) / 16;
			
			for (u8 i = 0; i < COUNT_OF(menu_options); i++)
			{
				//Get position on screen
				s32 y = (i * 30) - (menu.scroll >> FIXED_SHIFT);
				if (y <= -SCREEN_HEIGHT2 - 8)
					continue;
				if (y >= SCREEN_HEIGHT2 + 8)
					break;

				//Draw Icon
				Menu_DrawHealth(menu_options[i].icon, strlen(menu_options[i].text) * 13 + 38 + 4 + (y / 6), SCREEN_HEIGHT2 + y - 38, i == menu.select);
				
				//Draw text
				menu.font_bold.draw(&menu.font_bold,
					Menu_LowerIf(menu_options[i].text, menu.select != i),
					48 + (y / 6),
					SCREEN_HEIGHT2 - 16 + y - 8,
					FontAlign_Left
				);
			}
			
			//Draw background
			Menu_DrawBack(
				true,
				8,
				128, 128, 128,
				0, 0, 0
			);
			break;
		}
		case MenuPage_Credits:
		{
			static const struct
			{
				const char *text; //The text (NULL skip that text)
			} menu_options[] = {
				{"potage engine by"},
				{NULL},
				{"IGORSOU"},
				{"LUCKY"},
				{"UNSTOPABLE"},
				{"SPICYJPEG"},
				{"SPARK"},
				{NULL},
				{"special thanks"},
				{NULL},
				{"MAXDEV"},
				{"CUCKYDEV"},
			};
			
			//Initialize page
			if (menu.page_swap)
				menu.scroll = COUNT_OF(menu_options) * FIXED_DEC(24 + SCREEN_HEIGHT2,1);
			
			//Handle option and selection
			if (menu.next_page == menu.page && Trans_Idle())
			{
				//skip by 1 if next or previous text be NULL
				s8 skip;

				if (menu_options[menu.select - 1].text == NULL && pad_state.press & (PAD_UP))
					skip = -1;

				else if (menu_options[menu.select + 1].text == NULL && pad_state.press & (PAD_DOWN))
					skip = 1;

				else
					skip = 0;

				//Change options
				menu.select = Menu_Scroll(menu.select, COUNT_OF(menu_options) - 1, &menu.sounds[0]) + skip;
				
				//Return to main menu if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					//Play Cancel Sound
					Audio_PlaySFX(menu.sounds[2], 80);

					menu.next_page = MenuPage_Main;
					menu.next_select = 3; //Credits
					Trans_Start();
				}
			}
			
			//Draw options
			s32 next_scroll = menu.select * FIXED_DEC(24,1);
			menu.scroll += (next_scroll - menu.scroll) / 16;
			
			for (u8 i = 0; i < COUNT_OF(menu_options); i++)
			{
				//Get position on screen
				s32 y = (i * 24) - 8 - (menu.scroll >> FIXED_SHIFT);
				if (y <= -SCREEN_HEIGHT2 - 8)
					continue;
				if (y >= SCREEN_HEIGHT2 + 8)
					break;
				
				//Draw text
				menu.font_bold.draw(&menu.font_bold,
					Menu_LowerIf(menu_options[i].text, menu.select != i),
					SCREEN_WIDTH2,
					SCREEN_HEIGHT2 + y - 8,
					FontAlign_Center
				);
			}
			
			//Draw background
			Menu_DrawBack(
				true,
				8,
				128, 128, 128,
				0, 0, 0
			);
			break;
		}
		case MenuPage_Options:
		{
			static const struct
			{
				const char *text;
				u8 page;
			} menu_options[] = {
				{"VISUAL", MenuOptions_Visual},
				{"GAMEPLAY", MenuOptions_Gameplay},
			};

			//Initialize page
			if (menu.page_swap)
			{
				menu.select = 0;
				menu.scroll = COUNT_OF(menu_options) * FIXED_DEC(24 + SCREEN_HEIGHT2,1);
			}
			
			//Handle option and selection
			if (menu.next_page == menu.page && Trans_Idle())
			{
				menu.select = Menu_Scroll(menu.select, COUNT_OF(menu_options) - 1, &menu.sounds[0]);
				
				//Go to option when cross is pressed
				if (pad_state.press & (PAD_CROSS | PAD_START))
				{
					menu.page = menu.next_page = menu_options[menu.select].page;
				}

				//Return to main menu if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{				
					//Play Cancel Sound
					Audio_PlaySFX(menu.sounds[2], 80);

					menu.next_page = MenuPage_Main;
					menu.next_select = 2; //Options
					Trans_Start();
				}
			}

			//Save your game
			#ifndef NOSAVE
				if (pad_state.press & PAD_SELECT)
					WriteSave();

				RECT save_src = {0,120, 49, 7};
				RECT save_dst = {20, 23, 49*2, 7 *2};
				Gfx_DrawTex(&menu.tex_story, &save_src, &save_dst);

				//Reset Your Save
				if (pad_state.press & PAD_TRIANGLE)
					DefaultSettings();

				RECT reset_src = {0, 64, 57, 14};
				RECT reset_dst = {57*2 + 20, 9, 57*2, 14*2};
				Gfx_DrawTex(&menu.tex_story, &reset_src, &reset_dst);
		  #endif
			
			//Draw options
			for (u8 i = 0; i < COUNT_OF(menu_options); i++)
			{
				//Get position on screen
				s32 y = (i * 24) - 8;
				if (y <= -SCREEN_HEIGHT2 - 8)
					continue;
				if (y >= SCREEN_HEIGHT2 + 8)
					break;
				
				//Draw text
				menu.font_bold.draw(&menu.font_bold,
					Menu_LowerIf(menu_options[i].text, menu.select != i),
					SCREEN_WIDTH2,
					SCREEN_HEIGHT2 + y - 8,
					FontAlign_Center
				);
			}
			
			//Draw background
			Menu_DrawBack(
				true,
				8,
				128, 128, 128,
				0, 0, 0
			);
			break;
		}
		case MenuOptions_Visual:
		{
			static const struct
			{
				enum
				{
					OptType_Boolean,
				} type;
				const char *text;
				void *value;
				union
				{
					struct
					{
						int a;
					} spec_boolean;
				} spec;
			} menu_options[] = {
				{OptType_Boolean, "CAMERA ZOOM", &stage.save.canbump, {.spec_boolean = {0}}},
				{OptType_Boolean, "SPLASH", &stage.save.splash, {.spec_boolean = {0}}},
			};
			
			//Initialize page
			if (menu.page_swap)
			{
				menu.select = 0;
				menu.scroll = COUNT_OF(menu_options) * FIXED_DEC(24 + SCREEN_HEIGHT2,1);
			}
			
			//Draw page label
			menu.font_bold.draw(&menu.font_bold,
				"VISUALS AND UI",
				16,
				16,
				FontAlign_Left
			);
			
			//Handle option and selection
			if (menu.next_page == menu.page && Trans_Idle())
			{
				menu.select = Menu_Scroll(menu.select, COUNT_OF(menu_options) - 1, &menu.sounds[0]);
				
				//Handle option changing
				if (pad_state.press & (PAD_CROSS | PAD_LEFT | PAD_RIGHT))
					*((boolean*)menu_options[menu.select].value) ^= 1;
				
				//Return to main menu if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					//Play Cancel Sound
					Audio_PlaySFX(menu.sounds[2], 80);

					menu.page = menu.next_page = MenuPage_Options;
				}
			}

			//Draw options
			s32 next_scroll = menu.select * FIXED_DEC(24,1);
			menu.scroll += (next_scroll - menu.scroll) / 16;
			
			for (u8 i = 0; i < COUNT_OF(menu_options); i++)
			{
				//Get position on screen
				s32 y = (i * 24) - 8 - (menu.scroll >> FIXED_SHIFT);
				if (y <= -SCREEN_HEIGHT2 - 8)
					continue;
				if (y >= SCREEN_HEIGHT2 + 8)
					break;
				
				//Draw text
				char text[0x80];
				sprintf(text, "%s %s", menu_options[i].text, *((boolean*)menu_options[i].value) ? "ON" : "OFF");

				//Draw text
				menu.font_bold.draw(&menu.font_bold,
					Menu_LowerIf(text, menu.select != i),
					48 + (y / 4),
					SCREEN_HEIGHT2 - 16 + y - 8,
					FontAlign_Left
				);
			}
			
			//Draw background
			Menu_DrawBack(
				true,
				8,
				128, 128, 128,
				0, 0, 0
			);
			break;
		}
		case MenuOptions_Gameplay:
		{
			static const char *gamemode_strs[] = {"NORMAL", "SWAP", "TWO PLAYER"};
			static const struct
			{
				enum
				{
					OptType_Boolean,
					OptType_Enum,
				} type;
				const char *text;
				void *value;
				union
				{
					struct
					{
						int a;
					} spec_boolean;
					struct
					{
						s32 max;
						const char **strs;
					} spec_enum;
				} spec;
			} menu_options[] = {
				{OptType_Enum,    "GAMEMODE", &stage.mode, {.spec_enum = {COUNT_OF(gamemode_strs), gamemode_strs}}},
				{OptType_Boolean, "GHOST TAP ", &stage.save.ghost, {.spec_boolean = {0}}},
				{OptType_Boolean, "DOWNSCROLL", &stage.save.downscroll, {.spec_boolean = {0}}},
				{OptType_Boolean, "MIDDLESCROLL", &stage.save.middlescroll, {.spec_boolean = {0}}},
				{OptType_Boolean, "BOTPLAY", &stage.save.botplay, {.spec_boolean = {0}}},
				{OptType_Boolean, "SHOW TIMER", &stage.save.showtimer, {.spec_boolean = {0}}},
			};
			
			//Initialize page
			if (menu.page_swap)
			{
				menu.select = 0;
				menu.scroll = COUNT_OF(menu_options) * FIXED_DEC(24 + SCREEN_HEIGHT2,1);
			}
			
			//Draw page label
			menu.font_bold.draw(&menu.font_bold,
				"GAMEPLAY",
				16,
				16,
				FontAlign_Left
			);
			
			//Handle option and selection
			if (menu.next_page == menu.page && Trans_Idle())
			{
				menu.select = Menu_Scroll(menu.select, COUNT_OF(menu_options) - 1, &menu.sounds[0]);
				
				//Handle option changing
				switch (menu_options[menu.select].type)
				{
					case OptType_Boolean:
						if (pad_state.press & (PAD_CROSS | PAD_LEFT | PAD_RIGHT))
							*((boolean*)menu_options[menu.select].value) ^= 1;
						break;
					case OptType_Enum:
						if (pad_state.press & PAD_LEFT)
							if (--*((s32*)menu_options[menu.select].value) < 0)
								*((s32*)menu_options[menu.select].value) = menu_options[menu.select].spec.spec_enum.max - 1;
						if (pad_state.press & PAD_RIGHT)
							if (++*((s32*)menu_options[menu.select].value) >= menu_options[menu.select].spec.spec_enum.max)
								*((s32*)menu_options[menu.select].value) = 0;
						break;
				}
				
				//Return to main menu if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					//Play Cancel Sound
					Audio_PlaySFX(menu.sounds[2], 80);

					menu.page = menu.next_page = MenuPage_Options;
				}
			}
			
			//Draw options
			s32 next_scroll = menu.select * FIXED_DEC(24,1);
			menu.scroll += (next_scroll - menu.scroll) / 16;
			
			for (u8 i = 0; i < COUNT_OF(menu_options); i++)
			{
				//Get position on screen
				s32 y = (i * 24) - 8 - (menu.scroll >> FIXED_SHIFT);
				if (y <= -SCREEN_HEIGHT2 - 8)
					continue;
				if (y >= SCREEN_HEIGHT2 + 8)
					break;
				
				//Draw text
				char text[0x80];
				switch (menu_options[i].type)
				{
					case OptType_Boolean:
						sprintf(text, "%s %s", menu_options[i].text, *((boolean*)menu_options[i].value) ? "ON" : "OFF");
						break;
					case OptType_Enum:
						sprintf(text, "%s %s", menu_options[i].text, menu_options[i].spec.spec_enum.strs[*((s32*)menu_options[i].value)]);
						break;
				}
				menu.font_bold.draw(&menu.font_bold,
					Menu_LowerIf(text, menu.select != i),
					48 + (y / 4),
					SCREEN_HEIGHT2 - 16 + y - 8,
					FontAlign_Left
				);
			}
			
			//Draw background
			Menu_DrawBack(
				true,
				8,
				128, 128, 128,
				0, 0, 0
			);
			break;
		}

		case MenuPage_Stage:
		{
			//Unload menu state
			Menu_Unload();
			
			//Load new stage
			LoadScr_Start();
			Stage_Load(menu.page_param.stage.id, menu.page_param.stage.diff, menu.page_param.stage.story);
			gameloop = GameLoop_Stage;
			LoadScr_End();
			break;
		}
		default:
			break;
	}
	
	//Clear page swap flag
	menu.page_swap = menu.page != exec_page;
}
