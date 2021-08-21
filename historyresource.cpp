#include <metahook.h>
#include "vguilocal.h"
#include "hud.h"
#include "weapon.h"
#include "weaponbank.h"
#include "drawElement.h"
#include "CHudDelegate.h"
#include "historyresource.h"


HistoryResource gHR;

#define AMMO_PICKUP_GAP (gHR.iHistoryGap+5)
#define AMMO_PICKUP_PICK_HEIGHT		(32 + (gHR.iHistoryGap * 2))
#define AMMO_PICKUP_HEIGHT_MAX		(ScreenHeight - 100)

#define MAX_ITEM_NAME	32
int HISTORY_DRAW_TIME = 5;

// keep a list of items
struct ITEM_INFO
{
	char szName[MAX_ITEM_NAME];
	HSPRITE spr;
	wrect_t rect;
};

void HistoryResource::AddToHistory(int iType, int iId, int iCount)
{
	if (iType == HISTSLOT_AMMO && !iCount)
		return;  // no amount, so don't add

	if ((((AMMO_PICKUP_GAP * iCurrentHistorySlot) + AMMO_PICKUP_PICK_HEIGHT) > AMMO_PICKUP_HEIGHT_MAX) || (iCurrentHistorySlot >= MAX_HISTORY))
	{	// the pic would have to be drawn too high
		// so start from the bottom
		iCurrentHistorySlot = 0;
	}

	HIST_ITEM* freeslot = &rgAmmoHistory[iCurrentHistorySlot++];  // default to just writing to the first slot
	HISTORY_DRAW_TIME = CVAR_GET_FLOAT("hud_drawhistory_time");

	freeslot->type = iType;
	freeslot->iId = iId;
	freeslot->iCount = iCount;
	freeslot->DisplayTime = gEngfuncs.GetClientTime() + HISTORY_DRAW_TIME;
}

void HistoryResource::AddToHistory(int iType, const char* szName, int iCount)
{
	if (iType != HISTSLOT_ITEM)
		return;

	if ((((AMMO_PICKUP_GAP * iCurrentHistorySlot) + AMMO_PICKUP_PICK_HEIGHT) > AMMO_PICKUP_HEIGHT_MAX) || (iCurrentHistorySlot >= MAX_HISTORY))
	{	// the pic would have to be drawn too high
		// so start from the bottom
		iCurrentHistorySlot = 0;
	}

	HIST_ITEM* freeslot = &rgAmmoHistory[iCurrentHistorySlot++];  // default to just writing to the first slot

	// I am really unhappy with all the code in this file

	int i = gHudDelegate->GetSpriteIndex(szName);
	if (i == -1)
		return;  // unknown sprite name, don't add it to history

	freeslot->iId = i;
	freeslot->type = iType;
	freeslot->iCount = iCount;

	HISTORY_DRAW_TIME = CVAR_GET_FLOAT("hud_drawhistory_time");
	freeslot->DisplayTime = gEngfuncs.GetClientTime() + HISTORY_DRAW_TIME;
}


void HistoryResource::CheckClearHistory(void)
{
	for (int i = 0; i < MAX_HISTORY; i++)
	{
		if (rgAmmoHistory[i].type)
			return;
	}

	iCurrentHistorySlot = 0;
}

//
// Draw Ammo pickup history
//
int HistoryResource::DrawAmmoHistory(float flTime)
{
	for (int i = 0; i < MAX_HISTORY; i++)
	{
		if (rgAmmoHistory[i].type)
		{
			rgAmmoHistory[i].DisplayTime = min(rgAmmoHistory[i].DisplayTime, gEngfuncs.GetClientTime() + HISTORY_DRAW_TIME);

			if (rgAmmoHistory[i].DisplayTime <= flTime)
			{  // pic drawing time has expired
				memset(&rgAmmoHistory[i], 0, sizeof(HIST_ITEM));
				CheckClearHistory();
			}
			else if (rgAmmoHistory[i].type == HISTSLOT_AMMO)
			{
				wrect_t rcPic;
				HSPRITE* spr = gWR.GetAmmoPicFromWeapon(rgAmmoHistory[i].iId, rcPic);

				int r, g, b;
				UnpackRGB(r, g, b, RGB_YELLOWISH);
				float scale = (rgAmmoHistory[i].DisplayTime - flTime) * 80;
				ScaleColors(r, g, b, min(scale, 255));

				// Draw the pic
				int ypos = ScreenHeight - (AMMO_PICKUP_PICK_HEIGHT + (AMMO_PICKUP_GAP * i));
				int xpos = ScreenWidth - 24;
				if (spr && *spr)    // weapon isn't loaded yet so just don't draw the pic
				{ // the dll has to make sure it has sent info the weapons you need
					SPR_Set(*spr, r, g, b);
					SPR_DrawAdditive(0, xpos, ypos, &rcPic);
				}

				// Draw the number
				//gHUD.DrawHudNumberString(xpos - 10, ypos, xpos - 100, rgAmmoHistory[i].iCount, r, g, b);
			}
			else if (rgAmmoHistory[i].type == HISTSLOT_WEAP)
			{
				WEAPON* weap = gWR.GetWeapon(rgAmmoHistory[i].iId);

				if (!weap)
					return 1;  // we don't know about the weapon yet, so don't draw anything

				int r, g, b;
				UnpackRGB(r, g, b, RGB_YELLOWISH);

				if (!gWR.HasAmmo(weap))
					UnpackRGB(r, g, b, RGB_REDISH);	// if the weapon doesn't have ammo, display it as red

				float scale = (rgAmmoHistory[i].DisplayTime - flTime) * 80;
				ScaleColors(r, g, b, min(scale, 255));

				int ypos = ScreenHeight - (AMMO_PICKUP_PICK_HEIGHT + (AMMO_PICKUP_GAP * i));
				int xpos = ScreenWidth - (weap->rcInactive.right - weap->rcInactive.left);
				SPR_Set(weap->hInactive, r, g, b);
				SPR_DrawAdditive(0, xpos, ypos, &weap->rcInactive);
			}
			else if (rgAmmoHistory[i].type == HISTSLOT_ITEM)
			{
				int r, g, b;

				if (!rgAmmoHistory[i].iId)
					continue;  // sprite not loaded

				wrect_t rect = gHudDelegate->GetSpriteRect(rgAmmoHistory[i].iId);

				UnpackRGB(r, g, b, RGB_YELLOWISH);
				float scale = (rgAmmoHistory[i].DisplayTime - flTime) * 80;
				ScaleColors(r, g, b, min(scale, 255));

				int ypos = ScreenHeight - (AMMO_PICKUP_PICK_HEIGHT + (AMMO_PICKUP_GAP * i));
				int xpos = ScreenWidth - (rect.right - rect.left) - 10;

				SPR_Set(gHudDelegate->GetSprite(rgAmmoHistory[i].iId), r, g, b);
				SPR_DrawAdditive(0, xpos, ypos, &rect);
			}
		}
	}


	return 1;
}