#include <metahook.h>

#include "local.h"
#include "vguilocal.h"
#include "IFileSystem.h"
#include "IVanilliaPanel.h"

#include <vector>
#include <string>
#include <thread>
#include <atomic>

#include "vpx/vpx_decoder.h"
#include "vpx/video_reader.h"
#include "vpx/tools_common.h"

#include "Controls.h"

#include "BasePanel.h"

#include "exportfuncs.h"
#include "plugins.h"

int g_iTextureID;

IVanilliaPanel* g_pBasePanel;
IVanilliaPanel* g_pLoadingDialog;

typedef struct backgroundinfo_s {
	char video[MAX_PATH];
	char audio[MAX_PATH];
} backgroundinfo_t;
backgroundinfo_t* g_pNowChose;
std::vector<backgroundinfo_t*> g_aryBackGrounds;

std::atomic <const VpxVideoInfo*> g_pInfo;
std::atomic<vpx_codec_ctx_t*> g_pCodec;
std::atomic<CIVFVideoReader*> g_pReader;

typedef struct asyncResult {
	byte* data;
	uint wide;
	uint tall;
};
std::atomic<asyncResult*> g_pVideoResult;
std::thread g_pDecodeThread;
std::atomic_bool g_pThreadStop(false);

std::atomic_bool g_bPauseDecode;

void ReadBackGroundList() {
	char buffer2[MAX_PATH];
	char buffer1[MAX_PATH];

	char* pfile = (char*)gEngfuncs.COM_LoadFile(const_cast<char*>("abcenchance/scence/background.txt"), 5, nullptr);
	size_t c = 0;
	while (true) {
		if (c >= 1)
			strcpy(buffer1, buffer2);
		pfile = gEngfuncs.COM_ParseFile(pfile, buffer2);
		if (!pfile)
			break;
		if (c >= 1) {
			backgroundinfo_t* info = new backgroundinfo_t();
			g_pFileSystem->GetLocalPath(buffer1, info->video, MAX_PATH);
			strcpy(info->audio, buffer2);
			g_aryBackGrounds.push_back(info);
			c = 0;
		}
		else
			c++;
	}
	gEngfuncs.COM_FreeFile(pfile);
}
void OpenVideo() {
	static CIVFVideoReader pReader = CIVFVideoReader(g_pNowChose->video);
	g_pInfo = pReader.GetInfo();
	const VpxInterface* pDecoder = get_vpx_decoder_by_fourcc(g_pInfo.load()->codec_fourcc);
	static vpx_codec_ctx_t s_Codec;
	vpx_codec_dec_init(&s_Codec, pDecoder->codec_interface(), nullptr, 0);
	g_pCodec = &s_Codec;
	g_pReader = &pReader;
}
void CloseVideo() {
	vpx_codec_destroy(g_pCodec.load());
	g_pReader.load()->Close();
}
void PlayMp3() {
	char soundcmd[MAX_PATH + 8];
	Q_snprintf(soundcmd, "mp3 stop;mp3 loop %s ui", g_pNowChose->audio);
	EngineClientCmd(soundcmd);
}
void StopMp3() {
	EngineClientCmd("mp3 stop");
}
void DecodeVideo() {
	do {
		if (g_bPauseDecode)
			continue;
		auto start = std::chrono::high_resolution_clock::now();
		int result = g_pReader.load()->ReadFrame();
		if (!result) {
			g_pReader.load()->ResetToBegine();
			g_pReader.load()->ReadFrame();
		}
		size_t frame_size = 0;
		const byte* frame = g_pReader.load()->GetFrame(&frame_size);
		vpx_codec_err_t err = vpx_codec_decode(g_pCodec.load(), frame, frame_size, nullptr, 0);
		vpx_codec_iter_t iter = nullptr;
		vpx_image_t* img = vpx_codec_get_frame(g_pCodec.load(), &iter);
		if (img) {
			//not 444, fuck it
			if ((img->fmt != VPX_IMG_FMT_I444) && (img->fmt != VPX_IMG_FMT_I44416))
				return;
			static int s_iArea;
			static byte* s_pBuf;
			if (s_iArea < img->d_w * img->d_h) {
				s_iArea = img->d_w * img->d_h;
				delete[] s_pBuf;
				s_pBuf = new byte[img->d_w * img->d_h * 4];
			}
			size_t c = 0;
			size_t enumW = img->stride[VPX_PLANE_Y];
			size_t enumH = img->h;
			asyncResult returnVal;
			const static constexpr auto YUV2RGB = [](int Y, int U, int V, int* R, int* G, int* B) {
				int iTmpR = 0;
				int iTmpG = 0;
				int iTmpB = 0;

				iTmpR = (((int)Y) << 14) + 22970 * (((int)V) - 128);
				iTmpG = (((int)Y) << 14) - 5638 * (((int)U) - 128) - 11700 * (((int)V) - 128);
				iTmpB = (((int)Y) << 14) + 29032 * (((int)U) - 128);
				const static constexpr auto RoundShr = [](int d, int s) -> int {
					return d >= 0 ?
						-((-d & (1 << (s - 1))) ? ((-(d)) >> (s)) + 1 : ((-(d)) >> (s))) :
						d & (1 << ((s)-1)) ? (d >> s) + 1 : (d >> s);
					};
				iTmpR = RoundShr(iTmpR, 14);
				iTmpG = RoundShr(iTmpG, 14);
				iTmpB = RoundShr(iTmpB, 14);

				*R = clamp<int>(iTmpR, 0, 255);
				*G = clamp<int>(iTmpG, 0, 255);
				*B = clamp<int>(iTmpB, 0, 255);
				};
			for (size_t h = 0; h < enumH; h++) {
				if (h >= img->d_h)
					continue;
				for (size_t w = 0; w < enumW; w++) {
					if (w >= img->d_w)
						break;
					size_t i = w + (h * enumW);
					//use int damit!
					int r, g, b, a;
					YUV2RGB(
						img->planes[VPX_PLANE_Y][i],
						img->planes[VPX_PLANE_U][i],
						img->planes[VPX_PLANE_V][i],
						&r, &g, &b
					);
					a = img->planes[VPX_PLANE_ALPHA] ? img->planes[VPX_PLANE_ALPHA][i] : 255;
					s_pBuf[c * 4 + 0] = r;
					s_pBuf[c * 4 + 1] = g;
					s_pBuf[c * 4 + 2] = b;
					s_pBuf[c * 4 + 3] = a;
					c++;
				}
			}
			returnVal.data = s_pBuf;
			returnVal.wide = img->d_w;
			returnVal.tall = img->d_h;
			g_pVideoResult = &returnVal;
		};
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);
		auto tbr = std::chrono::milliseconds(g_pInfo.load()->time_base.numerator / g_pInfo.load()->time_base.denominator);
		std::this_thread::sleep_for(std::chrono::milliseconds(duration > tbr ? std::chrono::milliseconds(0) : tbr - duration));
	} while (!g_pThreadStop);
	if (!g_pThreadStop)
		CloseVideo();
}
void BackGroundVideoInit() {
	gCVars.pDynamicBackground = CREATE_CVAR("hud_dynamic_background", "1", FCVAR_VALUE, [](cvar_t* cvar) {
		if (cvar->value > 0){
			OpenVideo();
			PlayMp3();
			g_pThreadStop = false;
			g_pDecodeThread = std::thread(DecodeVideo);
			g_pDecodeThread.detach();
		}
		else {
			g_pThreadStop = true;
			CloseVideo();
			EngineClientCmd("mp3 stop;mp3 loop media/gamestartup.mp3 ui");
		}
	});
	ReadBackGroundList();
	g_pNowChose = g_aryBackGrounds[gEngfuncs.pfnRandomLong(0, g_aryBackGrounds.size()-1)];
}
void BackGroundVideoClose() {
	g_pThreadStop = true;
	for (auto iter = g_aryBackGrounds.begin(); iter != g_aryBackGrounds.end(); iter++) {
		delete (*iter);
	}
	g_aryBackGrounds.clear();
}
void BackGroundPushFrame() {
	if (!g_pBasePanel->IsVisible() || gCVars.pDynamicBackground->value <= 0) {
		g_bPauseDecode = true;
		return;
	}
	if (g_pLoadingDialog != nullptr && g_pLoadingDialog->IsVisible()) {
		g_bPauseDecode = true;
		return;
	}
	const static auto IsInLevel = []() -> bool {
		const char* levelName = gEngfuncs.pfnGetLevelName();
		if (strlen(levelName) > 0)
			return true;
		return false;
	};
	bool inLevel = IsInLevel();
	static bool s_bOldInLevel = false;
	//back to main menu
	if (s_bOldInLevel && !inLevel) {
		g_pReader.load()->ResetToBegine();
		PlayMp3();
	}
	else if (!s_bOldInLevel && inLevel)
		StopMp3();
	s_bOldInLevel = inLevel;
	g_bPauseDecode = inLevel;
}
void __fastcall CGameUI_Start(void* pthis, int dummy, void* engfuncs, int idoncare, void* ibasesystem) {
	gHookFuncs.CGameUI_Start(pthis, dummy, engfuncs, idoncare, ibasesystem);
	if (gCVars.pDynamicBackground->value > 0) 
		PlayMp3();
}
void* __fastcall CBasePanel_ctor(void* pthis, int dummy) {
	g_pBasePanel = static_cast<IVanilliaPanel*>(gHookFuncs.CBasePanel_ctor(pthis, dummy));
	g_iTextureID = vgui::surface()->CreateNewTextureID(true);
	return g_pBasePanel;
}
void* __fastcall CLoadingDialog_ctor(void* pthis, int dummy, void* pPanel) {
	g_pLoadingDialog = static_cast<IVanilliaPanel*>(gHookFuncs.CLoadingDialog_ctor(pthis, dummy, pPanel));
	return g_pLoadingDialog;
}
void* __fastcall CLoadingDialog_dtor(void* pthis, int dummy, byte idoncare) {
	g_pLoadingDialog = nullptr;
	return gHookFuncs.CLoadingDialog_dtor(pthis, dummy, idoncare);
}
void __fastcall CBasePanel_PaintBackground(void* pthis, int dummy) {
	if (gCVars.pDynamicBackground->value <= 0) {
		gHookFuncs.CBasePanel_PaintBackground(pthis, dummy);
		return;
	}
	if (g_pVideoResult.is_lock_free()) {
		asyncResult* value = g_pVideoResult.load();
		if (value)
			vgui::surface()->DrawSetTextureRGBA(g_iTextureID, value->data, value->wide, value->tall, true, false);
	}
	vgui::surface()->DrawSetColor(255, 255, 255, 255);
	vgui::surface()->DrawSetTexture(g_iTextureID);
	vgui::surface()->DrawTexturedRect(0, 0, ScreenWidth, ScreenHeight);
}
void BasePanel_InstallHook(void){
	HINTERFACEMODULE hGameUI = (HINTERFACEMODULE)GetModuleHandle("GameUI.dll");
	if (hGameUI) {
		CreateInterfaceFn fnCreateInterface = Sys_GetFactory(hGameUI);
		DWORD moduleSize = g_pMetaHookAPI->GetModuleSize(hGameUI);
		if (fnCreateInterface) {
#define SC_CBASEPANEL_PAINTBACKGROUNDIMAGE_SIG "\x55\x8B\xEC\x83\xEC\x38\x53\x8D\x45\xCC\x8B\xD9\x50\x8D\x45\xC8\x89\x5D\xD0\x50\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x8D\x4D\xD4\x51"
			Fill_Sig(SC_CBASEPANEL_PAINTBACKGROUNDIMAGE_SIG, hGameUI, moduleSize, CBasePanel_PaintBackground);
			Install_InlineHook(CBasePanel_PaintBackground);
#define SC_CBASEPANEL_CTOR_SIG "\x55\x8B\xEC\x51\x56\x68\x2A\x2A\x2A\x2A\x8B\xF1\x6A\x00\x89\x75\xFC\xE8\x2A\x2A\x2A\x2A\xC7"
			Fill_Sig(SC_CBASEPANEL_CTOR_SIG, hGameUI, moduleSize, CBasePanel_ctor);
			Install_InlineHook(CBasePanel_ctor);
#define SC_CLOADINGDIALOG_CTOR_SIG "\x55\x8B\xEC\x6A\xFF\x68\x2A\x2A\x2A\x2A\x64\xA1\x2A\x2A\x2A\x2A\x50\x51\x56\xA1\x2A\x2A\x2A\x2A\x33\xC5\x50\x8D\x45\xF4\x64\xA3\x2A\x2A\x2A\x2A\x8B\xF1\x89\x75\xF0\x6A\x01\x68"
			Fill_Sig(SC_CLOADINGDIALOG_CTOR_SIG, hGameUI, moduleSize, CLoadingDialog_ctor);
			Install_InlineHook(CLoadingDialog_ctor);
#define SC_CLOADINGDIALOG_DTOR_SIG "\x55\x8B\xEC\x56\x8B\xF1\xC7\x06\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xF6\x45\x08\x01\x74\x0E\x68\x80\x01\x00\x00"
			Fill_Sig(SC_CLOADINGDIALOG_DTOR_SIG, hGameUI, moduleSize, CLoadingDialog_dtor);
			Install_InlineHook(CLoadingDialog_dtor);
#define SC_GAMEUI_START_SIG "\x55\x8B\xEC\x6A\xFF\x68\x2A\x2A\x2A\x2A\x64\xA1\x2A\x2A\x2A\x2A\x50\x81\xEC\x30\x03\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xF0\x53"
			Fill_Sig(SC_GAMEUI_START_SIG, hGameUI, moduleSize, CGameUI_Start);
			Install_InlineHook(CGameUI_Start);
		}
	}
	else
		g_pMetaHookAPI->SysError("[ABCEnchace] Can not find game ui for dynamic background.");

	HMODULE hVPX= LoadLibrary("vpx.dll");
	if (hVPX) {
#define GetFuncVPX(name) name = (decltype(name))(GetProcAddress(hVPX, #name))
		GetFuncVPX(vpx_codec_dec_init_ver);
		GetFuncVPX(vpx_codec_peek_stream_info);
		GetFuncVPX(vpx_codec_get_stream_info);
		GetFuncVPX(vpx_codec_decode);
		GetFuncVPX(vpx_codec_get_frame);
		GetFuncVPX(vpx_codec_register_put_frame_cb);
		GetFuncVPX(vpx_codec_register_put_slice_cb);
		GetFuncVPX(vpx_codec_set_frame_buffer_functions);
		GetFuncVPX(vpx_codec_vp8_dx);
		GetFuncVPX(vpx_codec_vp9_dx);
		GetFuncVPX(vpx_codec_destroy);
		GetFuncVPX(vpx_codec_error);
		GetFuncVPX(vpx_codec_error_detail);
		GetFuncVPX(vpx_img_wrap);
		GetFuncVPX(vpx_img_free);
#undef GetFuncVPX
	}
	else
		g_pMetaHookAPI->SysError("[ABCEnchace] Can not open vpx.dll!");
}