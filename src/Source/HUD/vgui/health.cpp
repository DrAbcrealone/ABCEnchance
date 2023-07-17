#pragma once
#include <vector>
#include <metahook.h>

#include <vgui/IImage.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/ILocalize.h>
#include <VGUI2/tga_image.h>
#include <vgui_controls/Label.h>
#include "vgui_controls/ImagePanel.h"
#include "vgui_controls/ImageList.h"
#include "vgui_controls/ListViewPanel.h"
#include <vgui_controls/AnimationController.h>

#include "local.h"
#include <vguilocal.h>
#include "Viewport.h"

#include "health.h"
#include <hud.h>

#define VIEWPORT_HEALTH_NAME "HealthPanel"

using namespace vgui;

CHealthPanel::CHealthPanel()
	: BaseClass(nullptr, VIEWPORT_HEALTH_NAME){
	SetProportional(true);
	SetKeyBoardInputEnabled(false);
	SetMouseInputEnabled(false);
	vgui::scheme()->LoadSchemeFromFile(VGUI2_ROOT_DIR "HealthScheme.res", "HealthScheme");
	SetScheme("HealthScheme");

	m_Backround = new ImagePanel(this, "Background");

	m_HealthIcon = new ImagePanel(this, "HealthIcon");
	m_pHealthLable = new Label(this, "Health", "0");
	m_pHealthImagePanel = new ImagePanel(this, "HealthBar");
	m_HealthImageBackround = new ImagePanel(this, "HealthBackground");

	m_ArmorIcon = new ImagePanel(this, "ArmorIcon");
	m_pArmorLable = new Label(this, "Armor", "0");
	m_pArmorImagePanel = new ImagePanel(this, "ArmorBar");
	m_ArmorImageBackround = new ImagePanel(this, "ArmorBackground");

	m_pLongJumpImagePanel = new ImagePanel(this, "Longjump");

	m_pDmgImages = new ListViewPanel(this, "DMGImages");

	LoadControlSettings(VGUI2_ROOT_DIR "HealthPanel.res");
	SetVisible(false);

	m_iRestoredHealWide = m_pHealthImagePanel->GetWide();
	m_iRestoredArmorWide = m_pArmorImagePanel->GetWide();
	m_cRestoredHealth = m_pHealthImagePanel->GetDrawColor();
	m_cRestoredArmor = m_pArmorImagePanel->GetDrawColor();

	dmgimageitem_t some[] = {
		{ "Poison", "icon_poison", DMG_POISON, 0.0f},
		{ "Acid", "icon_acid", DMG_ACID, 0.0f},
		{ "Freeze","icon_freeze", DMG_FREEZE | DMG_SLOWFREEZE, 0.0f},
		{ "Drown", "icon_drown", DMG_DROWN, 0.0f},
		{ "Burn", "icon_burn", DMG_BURN | DMG_SLOWBURN, 0.0f},
		{ "Gas", "icon_gas", DMG_NERVEGAS, 0.0f},
		{ "Radiation", "icon_radiation", DMG_RADIATION, 0.0f},
		{ "Shock", "icon_shock", DMG_SHOCK, 0.0f}
	};
	m_aryDmgImageList = std::vector<dmgimageitem_t>(some, some + sizeof(some) / sizeof(some[0]));
	
}
const char* CHealthPanel::GetName(){
	return VIEWPORT_HEALTH_NAME;
}
void CHealthPanel::Reset(){
	if (!IsVisible())
		ShowPanel(true);
	for (auto iter = m_aryDmgImageList.begin(); iter != m_aryDmgImageList.end(); iter++) {
		iter->fExpire = 0;
	}
	for (size_t i = 0; i < m_pDmgImages->GetItemCount(); i++) {
		m_pDmgImages->RemoveItem(m_pDmgImages->GetItemIDFromPos(i));
	}
	SetLongJump(false);
}
void CHealthPanel::ApplySchemeSettings(vgui::IScheme* pScheme){
	BaseClass::ApplySchemeSettings(pScheme);
	SetBgColor(GetSchemeColor("HealthBar.BgColor", GetSchemeColor("Panel.BgColor", pScheme), pScheme));
	m_pDmgImages->SetBgColor(GetSchemeColor("HealthBar.ListViewBgColor", GetSchemeColor("Panel.BgColor", pScheme), pScheme));
	m_pHealthLable->SetFgColor(GetSchemeColor("HealthBar.HealthFgColor", GetSchemeColor("Label.BgColor", pScheme), pScheme));
	m_pArmorLable->SetFgColor(GetSchemeColor("HealthBar.ArmorFgColor", GetSchemeColor("Label.BgColor", pScheme), pScheme));

	m_cHealthDanger = GetSchemeColor("HealthBar.HealthDangerColor", pScheme);
	m_cArmorDanger = GetSchemeColor("HealthBar.ArmorDangerColor", pScheme);
}
void CHealthPanel::ApplySettings(KeyValues* inResourceData) {
	BaseClass::ApplySettings(inResourceData);
	ImageList* list = new ImageList(true);
	for (size_t i = 0; i < m_aryDmgImageList.size(); i++) {
		auto iter = m_aryDmgImageList[i];
		const char* icon = inResourceData->GetString(iter.szIconKey, nullptr);
		if (icon) {
			iter.iIndex = i;
			CTGAImage* img = new CTGAImage(icon);
			list->AddImage(img);
		}
	}
	m_pDmgImages->SetImageList(list, true);
}
void CHealthPanel::ShowPanel(bool state){
	if (state == IsVisible())
		return;
	SetVisible(state);
}
bool CHealthPanel::IsVisible(){
	return BaseClass::IsVisible();
}
vgui::VPANEL CHealthPanel::GetVPanel(){
	return BaseClass::GetVPanel();
}
void CHealthPanel::SetParent(vgui::VPANEL parent){
	BaseClass::SetParent(parent);
}

void CHealthPanel::UpdateTiles(long bitsDamage) {
	float flTime = ClientTime();
	for (auto iter = m_aryDmgImageList.begin(); iter != m_aryDmgImageList.end(); iter++) {
		if (iter->iDmg & bitsDamage) {
			int id = iter->iDmg;
			iter->fExpire = flTime;
			if (!m_pDmgImages->GetItem(id)) {
				KeyValues* pkv = new KeyValues(iter->szName);
				pkv->SetString("text", iter->szName);
				pkv->SetInt("image", iter->iIndex);
				m_pDmgImages->AddItem(pkv, false, true);
				delete pkv;
			}
		}
	}
}

void CHealthPanel::SetHealth(int health){
	static char buf[32];
	Q_snprintf(buf, "%d", health);
	m_pHealthLable->SetText(buf);

	if (health < gCVars.pDangerHealth->value) {
		float flRatio = health / gCVars.pDangerHealth->value;
		Color drawcolor = m_pHealthImagePanel->GetDrawColor();
		Vector org = Vector(drawcolor.r(), drawcolor.g(), drawcolor.b());
		Vector dng = Vector(m_cHealthDanger.r(), m_cHealthDanger.g(), m_cHealthDanger.b());
		RGBtoHSV(org, org);
		RGBtoHSV(dng, dng);
		org.x += (dng.x - org.x) * flRatio;
		HSVtoRGB(org, org);
		m_pHealthImagePanel->SetDrawColor(Color(org.x, org.y, org.z, drawcolor.a()));
	}
	else
		m_pHealthImagePanel->SetDrawColor(m_cRestoredHealth);
	int newWide = (float)health / 100.0f * m_iRestoredHealWide;
	vgui::GetAnimationController()->RunAnimationCommand(m_pHealthImagePanel, "wide", newWide, 0.0f, 0.15f, vgui::AnimationController::INTERPOLATOR_LINEAR);
}

void CHealthPanel::SetArmor(int armor){
	static char buf[32];
	Q_snprintf(buf, "%d", armor);
	m_pArmorLable->SetText(buf);

	if (armor < gCVars.pDangerArmor->value) {
		float flRatio = armor / gCVars.pDangerHealth->value;
		Color drawcolor = m_pArmorImagePanel->GetDrawColor();
		Vector org = Vector(drawcolor.r(), drawcolor.g(), drawcolor.b());
		Vector dng = Vector(m_cArmorDanger.r(), m_cArmorDanger.g(), m_cArmorDanger.b());
		RGBtoHSV(org, org);
		RGBtoHSV(dng, dng);
		org.x += (dng.x - org.x) * flRatio;
		HSVtoRGB(org, org);
		m_pArmorImagePanel->SetDrawColor(Color(org.x, org.y, org.z, drawcolor.a()));
	}
	else
		m_pArmorImagePanel->SetDrawColor(m_cRestoredArmor);
	int newWide = (float)armor / 100.0f * m_iRestoredArmorWide;
	vgui::GetAnimationController()->RunAnimationCommand(m_pArmorImagePanel, "wide", newWide, 0.0f, 0.15f, vgui::AnimationController::INTERPOLATOR_LINEAR);
}

void CHealthPanel::SetLongJump(bool lj){
	m_pLongJumpImagePanel->SetVisible(lj);
}
void CHealthPanel::SetArmorVisible(bool state) {
	m_pArmorImagePanel->SetVisible(state);
	m_pArmorLable->SetVisible(state);
	m_ArmorImageBackround->SetVisible(state);
}
void CHealthPanel::SetHealthVisible(bool state) {
	m_pHealthImagePanel->SetVisible(state);
	m_pHealthLable->SetVisible(state);
	m_HealthImageBackround->SetVisible(state);
}