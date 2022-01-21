#pragma once
typedef struct buymenuitem_s{
	int id;
	int price;
	int modelindex;
	int sequence;
	char name[64];
}buymenuitem_t;
class CHudEccoBuyMenu {
public:
	int Init();
	void VidInit();
	int Draw(float flTime);
	void Reset();
	void Clear();
	bool AddEntity(int type, cl_entity_s* ent, const char* modelname);
	void AddInfo(buymenuitem_t item);
	buymenuitem_t* GetInfo(int index);
	void OpenMenu();
	void CloseMenu();
	bool SelectMenu();

	std::vector<int> MenuList;
private:
	void CreateLight();
	void ClearTempEnt();
	std::vector<buymenuitem_t> buymenuinfo;
	bool bOpenningMenu = false;
	float m_fAnimateTime = 0;
	int iNowChosenSlot = 0;

	Color TextColor;
	Color ButtonColor;

	cvar_t* pCVarIdealYaw = nullptr;
	cvar_t* pCVarIdealDist = nullptr;
	cvar_t* pCVarFollowAim = nullptr;

	dlight_t* pLight = nullptr;
	TEMPENTITY* pShowEnt = nullptr;
	TEMPENTITY* pWeaponEnt = nullptr;
	int iNowSelectedId = -1;

	float flOldCamYaw;
	float flOldCamDist;
	float flOldCamHeight;
	float flOldCamRight;
	float flOldFollowAim;
	bool bOldCamThirdperson;

	GLuint iBackgroundSpr = 0;
	GLuint iBackgroundTga = 0;

	float BuyMenuAnimateTime = 0;
	int BuyMenuCenterX = 0;
	int BuyMenuCenterY = 0;
	int BuyMenuModelSize = 0;
	int BuyMenuModelX = 0;
	int BuyMenuModelY = 0;
	int BuyMenuOffset = 0;
	int BuyMenuHeight = 0;

	vgui::HFont hFont = 0;
};
extern CHudEccoBuyMenu m_HudEccoBuyMenu;