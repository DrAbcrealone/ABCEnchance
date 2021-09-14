#include <metahook.h>
#include <triangleapi.h>

#include "vguilocal.h"

char* UnicodeToUtf8(const wchar_t* unicode){
	int len = WideCharToMultiByte(CP_UTF8, 0, unicode, -1, NULL, 0, NULL, NULL);
	char* szUtf8 = (char*)malloc(len + 1);
	if (!szUtf8)
		return "";
	memset(szUtf8, 0, len + 1);
	WideCharToMultiByte(CP_UTF8, 0, unicode, -1, szUtf8, len, NULL, NULL);
	return szUtf8;
}

void DrawSPRIcon(int SprHandle, float x, float y, float w, float h, int r, int g, int b, int a){
	gEngfuncs.pTriAPI->SpriteTexture((struct model_s*)gEngfuncs.GetSpritePointer(SprHandle, SprHandle), 0);
	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);
	gEngfuncs.pTriAPI->Color4ub(r, g, b, a);
	gEngfuncs.pTriAPI->Brightness(1);
	gEngfuncs.pTriAPI->TexCoord2f(0, 0);
	gEngfuncs.pTriAPI->Vertex3f(x, y, 0);
	gEngfuncs.pTriAPI->Brightness(1);
	gEngfuncs.pTriAPI->TexCoord2f(0, 1);
	gEngfuncs.pTriAPI->Vertex3f(x, y + h, 0);
	gEngfuncs.pTriAPI->Brightness(1);
	gEngfuncs.pTriAPI->TexCoord2f(1, 1);
	gEngfuncs.pTriAPI->Vertex3f(x + w, y + h, 0);
	gEngfuncs.pTriAPI->Brightness(1);
	gEngfuncs.pTriAPI->TexCoord2f(1, 0);
	gEngfuncs.pTriAPI->Vertex3f(x + w, y, 0);
	gEngfuncs.pTriAPI->End();
}

void DrawSPRIconPos(int SprHandle, vec2_t p1, vec2_t p2, vec2_t p3, vec2_t p4, int r, int g, int b, int a){
	gEngfuncs.pTriAPI->SpriteTexture((struct model_s*)gEngfuncs.GetSpritePointer(SprHandle, SprHandle), 0);
	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);
	gEngfuncs.pTriAPI->Color4ub(r, g, b, a);
	gEngfuncs.pTriAPI->Brightness(1);
	gEngfuncs.pTriAPI->TexCoord2f(0, 0);
	gEngfuncs.pTriAPI->Vertex3f(p1[0], p1[1], 0);
	gEngfuncs.pTriAPI->Brightness(1);
	gEngfuncs.pTriAPI->TexCoord2f(0, 1);
	gEngfuncs.pTriAPI->Vertex3f(p2[0], p2[1], 0);
	gEngfuncs.pTriAPI->Brightness(1);
	gEngfuncs.pTriAPI->TexCoord2f(1, 1);
	gEngfuncs.pTriAPI->Vertex3f(p3[0], p3[1], 0);
	gEngfuncs.pTriAPI->Brightness(1);
	gEngfuncs.pTriAPI->TexCoord2f(1, 0);
	gEngfuncs.pTriAPI->Vertex3f(p4[0], p4[1], 0);
	gEngfuncs.pTriAPI->End();
}
int GetHudFontHeight(vgui::HFont m_hFont){
	if (!m_hFont)
		return 0;

	return g_pSurface->GetFontTall(m_hFont);
}
void GetStringSize(const wchar_t* string, int* width, int* height, vgui::HFont m_hFont){
	if (width)
		*width = 0;
	if (height)
		*height = 0;
	if (!m_hFont)
		return;

	int len = wcslen(string);

	if (width){
		int a, b, c;
		for (int i = 0; i < len; i++){
			g_pSurface->GetCharABCwide(m_hFont, string[i], a, b, c);
			*width += a + b + c;
		}
	}

	if (height)
		*height = GetHudFontHeight(m_hFont);
}
int DrawVGUI2String(wchar_t* msg, int x, int y, float r, float g, float b, vgui::HFont m_hFont, bool add = false){
	if (r > 1.0)
		r /= 255;
	if (g > 1.0)
		g /= 255;
	if (b > 1.0)
		b /= 255;
	int iOriginalX;
	int iTotalLines;
	int iCurrentLine;
	int w1, w2, w3;
	bool bHorzCenter;
	wchar_t* strTemp;

	if (!m_hFont)
		return 0;

	iCurrentLine = 0;
	iOriginalX = x;
	iTotalLines = 1;
	bHorzCenter = false;

	for (strTemp = msg; *strTemp; strTemp++){
		if (*strTemp == '\r')
			iTotalLines++;
	}

	if (x == -1)
		bHorzCenter = true;

	if (y == -1)
		y = (gScreenInfo.iHeight - g_pSurface->GetFontTall(m_hFont)) / 2;

	for (int i = 0; i < iTotalLines; i++){
		wchar_t line[1024];
		int iWidth;
		int iHeight;
		int iTempCount;
		int j;

		iTempCount = 0;
		iWidth = 0;
		iHeight = 0;

		for (strTemp = &msg[iCurrentLine]; *strTemp; strTemp++, iCurrentLine++){
			if (*strTemp == '\r')
				break;

			if (*strTemp != '\n')
				line[iTempCount++] = *strTemp;
		}

		line[iTempCount] = 0;

		GetStringSize(line, &iWidth, &iHeight, m_hFont);

		if (bHorzCenter)
			x = (gScreenInfo.iWidth - iWidth) / 2;
		else
			x = iOriginalX;

		if (!add){
			gEngfuncs.pfnDrawSetTextColor(0, 0, 0);
			int shadow_x = x;
			for (j = 0; j < iTempCount; j++){
				gEngfuncs.pfnVGUI2DrawCharacter(shadow_x, y, line[j], m_hFont);
				g_pSurface->GetCharABCwide(m_hFont, line[j], w1, w2, w3);
				shadow_x += w1 + w2 + w3;
			}
		}

		gEngfuncs.pfnDrawSetTextColor(r, g, b);

		for (j = 0; j < iTempCount; j++){
			if (add)
				gEngfuncs.pfnVGUI2DrawCharacterAdd(x, y, line[j], r * 255, g * 255, b * 255, m_hFont);
			else
				gEngfuncs.pfnVGUI2DrawCharacter(x, y, line[j], m_hFont);
			g_pSurface->GetCharABCwide(m_hFont, line[j], w1, w2, w3);

			x += w1 + w2 + w3;
		}

		y += iHeight;
		iCurrentLine++;
	}

	return x;
}
void ScaleColors(int& r, int& g, int& b, int a){
	float x = (float)a / 255;
	r = (int)(r * x);
	g = (int)(g * x);
	b = (int)(b * x);
}