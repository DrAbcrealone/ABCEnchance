#pragma once
class CHudVote {
public:
	int Init();
	int Draw(float flTime);
	void Reset();
	int HUD_KeyEvent(int eventcode, int keynum, const char* pszCurrentBinding);
	int StartVote();

	float m_flKeepTime;
	wchar_t m_VoteContend[128];
	wchar_t m_VoteYes[128];
	wchar_t m_VoteNo[128];

	bool m_bInVoting;
private:
	void DrawMultiLineStr(const wchar_t* str, int limite, int x, int y, int r, int g, int b);
	GLint m_iYesIconTga;
	GLint m_iNoIconTga;

	float XOffset;
	float YOffset;
	float Height;
	float Width;

	Color OutlineColor;
	Color BackGoundColor;

	wchar_t* VoteTitle;
	wchar_t* DefaultYes;
	wchar_t* DefaultNo;

	vgui::HFont HudFont;
};
extern CHudVote m_HudVote;