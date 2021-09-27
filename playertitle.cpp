#include <metahook.h>
#include <cmath>
#include "mathlib.h"

#include "glew.h"
#include "triangleapi.h"

#include "cvar_hook.h"
#include "local.h"
#include "vguilocal.h"
#include "myconst.h"

#include "hud.h"

#include "weapon.h"
#include "CHudDelegate.h"
#include "drawElement.h"

#include "exportfuncs.h"

#include "playertitle.h"

CHudPlayerTitle m_HudPlayerTitle;
int CHudPlayerTitle::Init(void){
	
	gCVars.pPlayerTitle = CREATE_CVAR("cl_playertitle", "1", FCVAR_VALUE, NULL);
	gCVars.pPlayerTitleLength = CREATE_CVAR("cl_playertitlelength", "196", FCVAR_VALUE, NULL);
	gCVars.pPlayerTitleHeight = CREATE_CVAR("cl_playertitleheight", "48", FCVAR_VALUE, NULL);
	Reset();
	return 0;
}
void CHudPlayerTitle::Reset(){
	VGUI_CREATE_NEWTGA_TEXTURE(iDeathIconTga, "abcenchance/tga/playertitle_deadicon");
	VGUI_CREATE_NEWTGA_TEXTURE(iCrouchIconTga, "abcenchance/tga/playertitle_crouchicon");
	VGUI_CREATE_NEWTGA_TEXTURE(iMedkitIconTga, "abcenchance/tga/playertitle_medkiticon");
	VGUI_CREATE_NEWTGA_TEXTURE(iBackGroundTga, "abcenchance/tga/playertitle_background");
	VGUI_CREATE_NEWTGA_TEXTURE(iHealthBarTga, "abcenchance/tga/playertitle_healthbar");
	VGUI_CREATE_NEWTGA_TEXTURE(iArmorBarTga, "abcenchance/tga/playertitle_armorbar");
}
int CHudPlayerTitle::Draw(float flTime){
	if (gCVars.pPlayerTitle->value > 0){
		cl_entity_t* local = gEngfuncs.GetLocalPlayer();
		float* localColor = gHookFuncs.GetClientColor(local->index);
		GLfloat nowX, nowY;
		vec2_t vecHUD;
		//夹角
		vec3_t vecAngle;
		//夹角点积
		float angledotResult = 0;
		//绘制高度
		vec3_t vecEntityOrigin;
		//绘制相关
		wchar_t wideName[MAX_PLAYER_NAME_LENGTH];
		float* color;
		float fDistance;
		float flArmorRatio;
		float flHealthRatio;
		hud_player_info_t playerinfo = { 0 };
		float flTitleLength = gCVars.pPlayerTitleLength->value;
		float flTitleHeight = gCVars.pPlayerTitleHeight->value;
		//视角角度
		vec3_t vecView;
		gEngfuncs.GetViewAngles(vecView);
		AngleVectors(vecView, vecView, nullptr, nullptr);
		for (int i = 1; i <= 32; i++){
			cl_entity_t* entity = gEngfuncs.GetEntityByIndex(i);
			if (!entity || 
				entity->curstate.messagenum != local->curstate.messagenum || 
				!entity->player || !entity->model || entity == local)
				continue;
			//计算我和目标的相对偏移
			VectorSubtract(entity->curstate.origin, local->curstate.origin, vecAngle);
			fDistance = fsqrt(pow(vecAngle[0], 2) + pow(vecAngle[1], 2) + +pow(vecAngle[2], 2));
			color = gHookFuncs.GetClientColor(i);
			if (fDistance >= 1024 || color != localColor)
				continue;
			VectorNormalize(vecAngle);
			angledotResult = DotProduct(vecAngle, vecView);
			//cos 60
			if (angledotResult > 0.5){
				VectorCopy(entity->curstate.origin, vecEntityOrigin);
				vecEntityOrigin[2] += 45;
				gEngfuncs.pTriAPI->WorldToScreen(vecEntityOrigin, vecHUD);
				vecHUD[0] = (1.0f + vecHUD[0]) * gScreenInfo.iWidth / 2;
				vecHUD[1] = (1.0f - vecHUD[1]) * gScreenInfo.iHeight / 2;
				gEngfuncs.pfnGetPlayerInfo(i, &playerinfo);
				if (playerinfo.name){
					flHealthRatio = clamp((gHudDelegate->m_Playerinfo[i].health / 100.0f), 0.0f, 1.0f);
					flArmorRatio = clamp((gHudDelegate->m_Playerinfo[i].armor / 100.0f), 0.0f, 1.0f);
					
					nowX = vecHUD[0] - flTitleLength / 3;
					nowY = vecHUD[1] - flTitleHeight / 2;
					if (gCVars.pPlayerTitle->value < 2){
						//背景
						glEnable(GL_TEXTURE_2D);
						glEnable(GL_BLEND);
							glColor4ub(255, 255, 255, 255);

							glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
							glBindTexture(GL_TEXTURE_2D, iBackGroundTga);
							glBegin(GL_QUADS);
								glTexCoord2f(0, 0);
								glVertex3f(nowX, nowY, 0);
								glTexCoord2f(0, 1);
								glVertex3f(nowX, nowY + flTitleHeight, 0);
								glTexCoord2f(1, 1);
								glVertex3f(nowX + flTitleLength, nowY + flTitleHeight, 0);
								glTexCoord2f(1, 0);
								glVertex3f(nowX + flTitleLength, nowY, 0);
							glEnd();

							glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
							glBindTexture(GL_TEXTURE_2D, iHealthBarTga);
							glBegin(GL_QUADS);
								glTexCoord2f(0, 0);
								glVertex3f(nowX, nowY, 0);
								glTexCoord2f(0, 1);
								glVertex3f(nowX, nowY + flTitleHeight, 0);
								glTexCoord2f(flHealthRatio, 1);
								glVertex3f(nowX + (flTitleLength * flHealthRatio), nowY + flTitleHeight, 0);
								glTexCoord2f(flHealthRatio, 0);
								glVertex3f(nowX + (flTitleLength * flHealthRatio), nowY, 0);
							glEnd();

							glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
							glBindTexture(GL_TEXTURE_2D, iArmorBarTga);
							glBegin(GL_QUADS);
								glTexCoord2f(0, 0);
								glVertex3f(nowX, nowY, 0);
								glTexCoord2f(0, 1);
								glVertex3f(nowX, nowY + flTitleHeight, 0);
								glTexCoord2f(flArmorRatio, 1);
								glVertex3f(nowX + (flTitleLength * flArmorRatio), nowY + flTitleHeight, 0);
								glTexCoord2f(flArmorRatio, 0);
								glVertex3f(nowX + (flTitleLength * flArmorRatio), nowY, 0);
							glEnd();
						
						gHudDelegate->surface()->DrawSetTexture(-1);
						if (flHealthRatio <= 0.45f || fabs(entity->curstate.maxs[2] - entity->curstate.mins[2]) < 64){
							gHudDelegate->surface()->DrawSetColor(255, 255, 255, 255);
							gHudDelegate->surface()->DrawSetTexture(
								flHealthRatio < 0.4 ? (
									flHealthRatio <= 0 ? iDeathIconTga : iMedkitIconTga) : iCrouchIconTga);
							gHudDelegate->surface()->DrawTexturedRect(nowX, nowY, nowX + flTitleHeight / 2, nowY + flTitleHeight / 2);
						}
						nowX += flTitleHeight / 2;
					}
					pLocalize->ConvertANSIToUnicode(playerinfo.name, wideName, sizeof(wideName));
					vgui::HFont hFont = pScheme->GetFont("MainShitFont", true);
					DrawVGUI2String(wideName, nowX, nowY, color[0], color[1], color[2], hFont);
				}
			}
		}
	}
	return 0;
}

