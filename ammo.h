#pragma once
#include "IWeaponSelect.h"
class CHudCustomAmmo{
public:
	void GLInit();
	int Init(void);
	int VidInit(void);
	int Draw(float flTime);
	void Reset(void);
	void SlotInput(int iSlot, int fAdvance);
	void ChosePlayerWeapon(void);
	void ClientMove(struct playermove_s* ppmove, qboolean server);
	void IN_Accumulate();
	void Clear();

	bool m_bIsOnTarget = false;
	WEAPON* m_pWeapon;
	IWeaponSelect* m_pNowSelectMenu;

	bool m_bAcceptDeadMessage = false;
private:
	void DrawSelectIcon(WEAPON* wp, int a, int xpos, int ypos, int index);
	int DrawWList(float flTime);
	void SyncWeapon();
	float m_fNextSyncTime = 0;

	float IconSize = 0.5F;
	float ElementGap = 0.2F;
	float BackGroundY = 0.95F;
	float BackGroundLength = 3.0F;

	GLint iBackGroundTga = 0;

	Color Ammo1IconColor;
	Color Ammo1BigTextColor;
	Color Ammo1TextColor;
	Color Ammo2IconColor;
	Color Ammo2BigTextColor;
	Color Ammo2TextColor;

	vgui::HFont HUDFont = 0;
	vgui::HFont HUDSmallFont = 0;
};
extern CHudCustomAmmo m_HudCustomAmmo;