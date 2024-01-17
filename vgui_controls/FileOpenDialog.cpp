//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implementation of vgui generic open file dialog
//
// $NoKeywords: $
//===========================================================================//


#define PROTECTED_THINGS_DISABLE

#if !defined( _X360 ) && defined( WIN32 )
#include "winlite.h"
#include <shellapi.h>
#elif defined( POSIX )
#include <stdlib.h>
#define _stat stat
#define _wcsnicmp wcsncmp
#elif defined( _X360 )
#else
#error
#endif

#undef GetCurrentDirectory
#include <algorithm>
#include <filesystem>
#include <map>

#include "filesystem_helpers.h"

#include "tier1/utldict.h"
#include "tier1/utlstring.h"

#include <KeyValues.h>

#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/IVGui.h>
#include <vgui/ILocalize.h>
#include <vgui/IInput.h>

#include <vgui_controls/FileOpenDialog.h>

#include <vgui_controls/Button.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/InputDialog.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ListPanel.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/MenuItem.h>
#include <vgui_controls/Tooltip.h>

#if defined( _X360 )
#include "xbox/xbox_win32stubs.h"
#undef GetCurrentDirectory
#endif
#include <codecvt>

using namespace vgui;
namespace fs = std::filesystem;
extern bool IsOSX();

static int s_nLastSortColumn = 0;

static int ListFileNameSortFunc(ListPanel* pPanel, const ListPanelItem& item1, const ListPanelItem& item2)
{
	NOTE_UNUSED(pPanel);

	bool dir1 = item1.kv->GetInt("directory") == 1;
	bool dir2 = item2.kv->GetInt("directory") == 1;

	// if they're both not directories of files, return if dir1 is a directory (before files)
	if (dir1 != dir2)
	{
		return dir1 ? -1 : 1;
	}

	const char* string1 = item1.kv->GetString("text");
	const char* string2 = item2.kv->GetString("text");

	// YWB:  Mimic windows behavior where filenames starting with numbers are sorted based on numeric part
	int num1 = Q_atoi(string1);
	int num2 = Q_atoi(string2);

	if (num1 != 0 &&
		num2 != 0)
	{
		if (num1 < num2)
			return -1;
		else if (num1 > num2)
			return 1;
	}

	// Push numbers before everything else
	if (num1 != 0)
	{
		return -1;
	}

	// Push numbers before everything else
	if (num2 != 0)
	{
		return 1;
	}

	return Q_stricmp(string1, string2);
}

static int ListBaseStringSortFunc(ListPanel* pPanel, const ListPanelItem& item1, const ListPanelItem& item2, char const* fieldName)
{
	bool dir1 = item1.kv->GetInt("directory") == 1;
	bool dir2 = item2.kv->GetInt("directory") == 1;

	// if they're both not directories of files, return if dir1 is a directory (before files)
	if (dir1 != dir2)
	{
		return -1;
	}

	const char* string1 = item1.kv->GetString(fieldName);
	const char* string2 = item2.kv->GetString(fieldName);
	int cval = Q_stricmp(string1, string2);
	if (cval == 0)
	{
		// Use filename to break ties
		return ListFileNameSortFunc(pPanel, item1, item2);
	}

	return cval;
}

static int ListBaseIntegerSortFunc(ListPanel* pPanel, const ListPanelItem& item1, const ListPanelItem& item2, char const* fieldName)
{
	bool dir1 = item1.kv->GetInt("directory") == 1;
	bool dir2 = item2.kv->GetInt("directory") == 1;

	// if they're both not directories of files, return if dir1 is a directory (before files)
	if (dir1 != dir2)
	{
		return -1;
	}

	int i1 = item1.kv->GetInt(fieldName);
	int i2 = item2.kv->GetInt(fieldName);
	if (i1 == i2)
	{
		// Use filename to break ties
		return ListFileNameSortFunc(pPanel, item1, item2);
	}

	return (i1 < i2) ? -1 : 1;
}

static int ListBaseInteger64SortFunc(ListPanel* pPanel, const ListPanelItem& item1, const ListPanelItem& item2, char const* lowfield, char const* highfield)
{
	bool dir1 = item1.kv->GetInt("directory") == 1;
	bool dir2 = item2.kv->GetInt("directory") == 1;

	// if they're both not directories of files, return if dir1 is a directory (before files)
	if (dir1 != dir2)
	{
		return dir1 ? -1 : 1;
	}

	uint32 l1 = item1.kv->GetInt(lowfield);
	uint32 h1 = item1.kv->GetInt(highfield);
	uint32 l2 = item2.kv->GetInt(lowfield);
	uint32 h2 = item2.kv->GetInt(highfield);
	uint64 i1 = (uint64)((uint64)l1 | ((uint64)h1 << 32));
	uint64 i2 = (uint64)((uint64)l2 | ((uint64)h2 << 32));

	if (i1 == i2)
	{
		// Use filename to break ties
		return ListFileNameSortFunc(pPanel, item1, item2);
	}

	return (i1 < i2) ? -1 : 1;
}


static int ListFileSizeSortFunc(ListPanel* pPanel, const ListPanelItem& item1, const ListPanelItem& item2)
{
	return ListBaseIntegerSortFunc(pPanel, item1, item2, "filesizeint");
}

static int ListFileModifiedSortFunc(ListPanel* pPanel, const ListPanelItem& item1, const ListPanelItem& item2)
{
	// NOTE: Backward order to get most recent files first
	return ListBaseInteger64SortFunc(pPanel, item2, item1, "modifiedint_low", "modifiedint_high");
}
static int ListFileCreatedSortFunc(ListPanel* pPanel, const ListPanelItem& item1, const ListPanelItem& item2)
{
	// NOTE: Backward order to get most recent files first
	return ListBaseInteger64SortFunc(pPanel, item2, item1, "createdint_low", "createdint_high");
}
static int ListFileAttributesSortFunc(ListPanel* pPanel, const ListPanelItem& item1, const ListPanelItem& item2)
{
	return ListBaseStringSortFunc(pPanel, item1, item2, "attributes");
}
static int ListFileTypeSortFunc(ListPanel* pPanel, const ListPanelItem& item1, const ListPanelItem& item2)
{
	return ListBaseStringSortFunc(pPanel, item1, item2, "type");
}

std::wstring UTF8ToWString(const std::string& str){
	std::wstring_convert< std::codecvt_utf8<wchar_t> > strCnv;
	return strCnv.from_bytes(str);
}
template <typename TP>
std::time_t ToTimeT(TP tp){
	using namespace std::chrono;
	auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now()
		+ system_clock::now());
	return system_clock::to_time_t(sctp);
}
bool WildMatch(const std::string& str, const std::string& pat) {
	std::string::const_iterator str_it = str.begin();
	for (std::string::const_iterator pat_it = pat.begin(); pat_it != pat.end();
		++pat_it) {
		switch (*pat_it) {
		case '?':
			if (str_it == str.end()) {
				return false;
			}

			++str_it;
			break;
		case '*': {
			if (pat_it + 1 == pat.end()) {
				return true;
			}

			const size_t max = strlen(&*str_it);
			for (size_t i = 0; i < max; ++i) {
				if (WildMatch(&*(pat_it + 1), &*(str_it + i))) {
					return true;
				}
			}

			return false;
		}
		default:
			if (*str_it != *pat_it) {
				return false;
			}

			++str_it;
		}
	}

	return str_it == str.end();
}
class IconImage : public IImage
{
public:
	IconImage(unsigned char* data, size_t w, size_t h) {
		_id = surface()->CreateNewTextureID();
		_wide = w;
		_tall = h;
		_color = Color(255, 255, 255, 255);
		surface()->DrawSetTextureRGBA(_id, data, w, h, 1, 0);
	}

public:
	virtual void Paint(void) {
		if (!_id)
			_id = surface()->CreateNewTextureID();
		surface()->DrawSetColor(_color[0], _color[1], _color[2], _color[3]);
		surface()->DrawSetTexture(_id);
		if (_wide == 0)
			GetSize(_wide, _tall);
		surface()->DrawTexturedRect(_pos[0], _pos[1], _pos[0] + _wide, _pos[1] + _tall);
	}
	virtual void GetSize(int& wide, int& tall) {
		wide = 0;
		tall = 0;
		if (0 == _wide && 0 == _tall)
			surface()->DrawGetTextureSize(_id, _wide, _tall);
		wide = _wide;
		tall = _tall;
	}
	virtual void GetContentSize(int& wide, int& tall) {
		GetSize(wide, tall);
	}
	virtual void SetSize(int x, int y) {
		_wide = x;
		_tall = y;
	}
	virtual void SetPos(int x, int y) {
		_pos[0] = x;
		_pos[1] = y;
	}

	virtual void SetColor(Color col) {
		_color = col;
	}
private:
	HTexture _id;
	int _pos[2];
	Color _color;
	int _wide, _tall;
};

#ifdef WIN32	
const char* Win32GetAttributesAsString(DWORD dwAttributes)
{
	static char out[256];
	out[0] = 0;
	if (dwAttributes & FILE_ATTRIBUTE_ARCHIVE)
	{
		Q_strncat(out, "A", sizeof(out), COPY_ALL_CHARACTERS);
	}
	if (dwAttributes & FILE_ATTRIBUTE_COMPRESSED)
	{
		Q_strncat(out, "C", sizeof(out), COPY_ALL_CHARACTERS);
	}
	if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		Q_strncat(out, "D", sizeof(out), COPY_ALL_CHARACTERS);
	}
	if (dwAttributes & FILE_ATTRIBUTE_HIDDEN)
	{
		Q_strncat(out, "H", sizeof(out), COPY_ALL_CHARACTERS);
	}
	if (dwAttributes & FILE_ATTRIBUTE_READONLY)
	{
		Q_strncat(out, "R", sizeof(out), COPY_ALL_CHARACTERS);
	}
	if (dwAttributes & FILE_ATTRIBUTE_SYSTEM)
	{
		Q_strncat(out, "S", sizeof(out), COPY_ALL_CHARACTERS);
	}
	if (dwAttributes & FILE_ATTRIBUTE_TEMPORARY)
	{
		Q_strncat(out, "T", sizeof(out), COPY_ALL_CHARACTERS);
	}
	return out;
}
const char* Win32GetFileTimetamp(FILETIME ft){
	SYSTEMTIME local;
	FILETIME localFileTime;
	FileTimeToLocalFileTime(&ft, &localFileTime);
	FileTimeToSystemTime(&localFileTime, &local);

	static char out[256];

	bool am = true;
	WORD hour = local.wHour;
	if (hour >= 12)
	{
		am = false;
		// 12:42 pm displays as 12:42 pm
		// 13:42 pm displays as 1:42 pm
		if (hour > 12)
		{
			hour -= 12;
		}
	}
	Q_snprintf(out, sizeof(out), "%d/%02d/%04d %d:%02d %s",
		local.wMonth,
		local.wDay,
		local.wYear,
		hour,
		local.wMinute,
		am ? "AM" : "PM" // TODO: Localize this?
	);
	return out;
}
std::string Win32GetStringValueFromRegistry(HKEY hKey, const std::string& subKey, const std::string& valueName) {
	std::string result;
	DWORD size = 0;
	if (RegGetValueA(hKey, subKey.c_str(), valueName.c_str(), RRF_RT_REG_SZ, NULL, NULL, &size) == ERROR_SUCCESS) {
		char* buffer = new char[size];
		if (RegGetValueA(hKey, subKey.c_str(), valueName.c_str(), RRF_RT_REG_SZ, NULL, buffer, &size) == ERROR_SUCCESS)
			result = std::string(buffer, size - 1);
		delete[] buffer;
	}
	return result;
}
std::string Win32GetDefaultValueFromRegistry(HKEY hKey, const std::string& subKey) {
	return Win32GetStringValueFromRegistry(hKey, subKey, "");
}
std::string Win32GetFileDescriptionByExtension(const std::string& extension) {
	HKEY hKey;
	if (RegOpenKeyExA(HKEY_CLASSES_ROOT, NULL, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		std::string subKey = Win32GetDefaultValueFromRegistry(hKey, extension);
		std::string description = Win32GetDefaultValueFromRegistry(hKey, subKey);
		RegCloseKey(hKey);
		return description;
	}
	return "";
}
HICON Win32GetFileIconByExtension(const std::string& extension) {
	HKEY hKey;
	if (RegOpenKeyExA(HKEY_CLASSES_ROOT, NULL, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		std::string subKey = Win32GetDefaultValueFromRegistry(hKey, extension);
		std::string iconPath = Win32GetDefaultValueFromRegistry(hKey, subKey + "\\DefaultIcon");
		RegCloseKey(hKey);
		if (iconPath.size() == 0)
			return NULL;
		size_t pos = iconPath.find_last_of(',');
		std::string path = iconPath.substr(0, pos);
		int index = std::stoi(iconPath.substr(pos + 1));
		char expandedPath[MAX_PATH];
		ExpandEnvironmentStringsA(path.c_str(), expandedPath, MAX_PATH);
		HICON smallIcon;
		ExtractIconExW(std::wstring(expandedPath, expandedPath + strlen(expandedPath)).c_str(), index, NULL, &smallIcon, 1);
		return smallIcon;
	}
	return NULL;
}
BITMAP Win32GetBitmapInfo(HICON hIcon) {
	BITMAP bmp;
	ICONINFO iconInfo;
	GetIconInfo(hIcon, &iconInfo);
	GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmp);
	return bmp;
}
std::vector<unsigned char> Win32ConvertHICONToRGBA(HICON hIcon, size_t& ww, size_t& hh) {
	BITMAP bmp = Win32GetBitmapInfo(hIcon);
	int width = bmp.bmWidth;
	int height = bmp.bmHeight;
	ww = width;
	hh = height;
	HDC hdc = CreateCompatibleDC(NULL);
	BITMAPINFO bmi;
	ZeroMemory(&bmi, sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = -height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	unsigned char* bits = new unsigned char[width * height * 4];
	HBITMAP hBmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
	SelectObject(hdc, hBmp);
	DrawIconEx(hdc, 0, 0, hIcon, width, height, 0, NULL, DI_NORMAL);
	std::vector<unsigned char> rgba;
	for (int i = 0; i < width * height * 4; i += 4) {
		rgba.push_back(bits[i + 2]);
		rgba.push_back(bits[i + 1]);
		rgba.push_back(bits[i]);
		rgba.push_back(bits[i + 3]);
	}
	DeleteObject(hBmp);
	DeleteDC(hdc);
	return rgba;
}
#endif

namespace vgui
{
	static std::map<std::string, IconImage*> g_dicExtensionIcons;

	class FileCompletionMenu : public Menu
	{
	public:
		FileCompletionMenu(Panel* parent, const char* panelName) : Menu(parent, panelName)
		{
		}

		// override it so it doesn't request focus
		virtual void SetVisible(bool state)
		{
			Panel::SetVisible(state);
		}

	};


	//-----------------------------------------------------------------------------
	// File completion edit text entry
	//-----------------------------------------------------------------------------
	class FileCompletionEdit : public TextEntry
	{
		DECLARE_CLASS_SIMPLE(FileCompletionEdit, TextEntry);

	public:
		FileCompletionEdit(Panel* parent);
		~FileCompletionEdit();

		int AddItem(const char* itemText, KeyValues* userData);
		int AddItem(const wchar_t* itemText, KeyValues* userData);
		void DeleteAllItems();
		int GetItemCount();
		int GetItemIDFromRow(int row);
		int GetRowFromItemID(int itemID);
		virtual void PerformLayout();
		void OnSetText(const wchar_t* newtext);
		virtual void OnKillFocus();
		void HideMenu(void);
		void ShowMenu(void);
		virtual void OnKeyCodeTyped(KeyCode code);
		MESSAGE_FUNC_INT(OnMenuItemHighlight, "MenuItemHighlight", itemID);

	private:
		FileCompletionMenu* m_pDropDown;
	};

	FileCompletionEdit::FileCompletionEdit(Panel* parent) : TextEntry(parent, NULL)
	{
		m_pDropDown = new FileCompletionMenu(this, NULL);
		m_pDropDown->AddActionSignalTarget(this);
	}

	FileCompletionEdit::~FileCompletionEdit()
	{
		delete m_pDropDown;
	}

	int FileCompletionEdit::AddItem(const char* itemText, KeyValues* userData)
	{
		// when the menu item is selected it will send the custom message "SetText"
		return m_pDropDown->AddMenuItem(itemText, new KeyValues("SetText", "text", itemText), this, userData);
	}
	int FileCompletionEdit::AddItem(const wchar_t* itemText, KeyValues* userData)
	{
		// add the element to the menu
		// when the menu item is selected it will send the custom message "SetText"
		KeyValues* kv = new KeyValues("SetText");
		kv->SetWString("text", itemText);

		// get an ansi version for the menuitem name
		char ansi[128];
		g_pVGuiLocalize->ConvertUnicodeToANSI(itemText, ansi, sizeof(ansi));
		return m_pDropDown->AddMenuItem(ansi, kv, this, userData);
	}

	void FileCompletionEdit::DeleteAllItems()
	{
		m_pDropDown->DeleteAllItems();
	}

	int FileCompletionEdit::GetItemCount()
	{
		return m_pDropDown->GetItemCount();
	}

	int FileCompletionEdit::GetItemIDFromRow(int row)
	{
		// valid from [0, GetItemCount)
		return m_pDropDown->GetMenuID(row);
	}

	int FileCompletionEdit::GetRowFromItemID(int itemID)
	{
		int i;
		for (i = 0; i < GetItemCount(); i++)
		{
			if (m_pDropDown->GetMenuID(i) == itemID)
				return i;
		}
		return -1;
	}

	void FileCompletionEdit::PerformLayout()
	{
		BaseClass::PerformLayout();

		m_pDropDown->PositionRelativeToPanel(this, Menu::DOWN, 0);

		// reset the width of the drop down menu to be the width of this edit box
		m_pDropDown->SetFixedWidth(GetWide());
		m_pDropDown->ForceCalculateWidth();
	}

	void FileCompletionEdit::OnSetText(const wchar_t* newtext)
	{
		// see if the combobox text has changed, and if so, post a message detailing the new text
		wchar_t wbuf[255];
		GetText(wbuf, 254);

		if (wcscmp(wbuf, newtext))
		{
			// text has changed
			SetText(newtext);

			// fire off that things have changed
			PostActionSignal(new KeyValues("TextChanged", "text", newtext));
			Repaint();
		}
	}

	void FileCompletionEdit::OnKillFocus()
	{
		HideMenu();
		BaseClass::OnKillFocus();
	}

	void FileCompletionEdit::HideMenu(void)
	{
		// hide the menu
		m_pDropDown->SetVisible(false);
	}

	void FileCompletionEdit::ShowMenu(void)
	{
		// reset the dropdown's position
		m_pDropDown->InvalidateLayout();

		// make sure we're at the top of the draw order (and therefore our children as well)
		// this important to make sure the menu will be drawn in the foreground
		MoveToFront();

		// reset the drop down
		m_pDropDown->ClearCurrentlyHighlightedItem();

		// limit it to only 6
		if (m_pDropDown->GetItemCount() > 6)
		{
			m_pDropDown->SetNumberOfVisibleItems(6);
		}
		else
		{
			m_pDropDown->SetNumberOfVisibleItems(m_pDropDown->GetItemCount());
		}
		// show the menu
		m_pDropDown->SetVisible(true);

		Repaint();
	}

	void FileCompletionEdit::OnKeyCodeTyped(KeyCode code)
	{
		if (code == KEY_DOWN)
		{
			if (m_pDropDown->GetItemCount() > 0)
			{
				int menuID = m_pDropDown->GetCurrentlyHighlightedItem();
				int row = -1;
				if (menuID == -1)
				{
					row = m_pDropDown->GetItemCount() - 1;
				}
				else
				{
					row = GetRowFromItemID(menuID);
				}
				row++;
				if (row == m_pDropDown->GetItemCount())
				{
					row = 0;
				}
				menuID = GetItemIDFromRow(row);
				m_pDropDown->SetCurrentlyHighlightedItem(menuID);
				return;
			}
		}
		else if (code == KEY_UP)
		{
			if (m_pDropDown->GetItemCount() > 0)
			{
				int menuID = m_pDropDown->GetCurrentlyHighlightedItem();
				int row = -1;
				if (menuID == -1)
				{
					row = 0;
				}
				else
				{
					row = GetRowFromItemID(menuID);
				}
				row--;
				if (row < 0)
				{
					row = m_pDropDown->GetItemCount() - 1;
				}
				menuID = GetItemIDFromRow(row);
				m_pDropDown->SetCurrentlyHighlightedItem(menuID);
				return;
			}
		}
		else if (code == KEY_ESCAPE)
		{
			if (m_pDropDown->IsVisible())
			{
				HideMenu();
				return;
			}
		}
		BaseClass::OnKeyCodeTyped(code);
		return;
	}

	void FileCompletionEdit::OnMenuItemHighlight(int itemID)
	{
		char wbuf[80];
		if (m_pDropDown->IsValidMenuID(itemID))
		{
			m_pDropDown->GetMenuItem(itemID)->GetText(wbuf, 80);
		}
		else
		{
			wbuf[0] = 0;
		}
		SetText(wbuf);
		RequestFocus();
		GotoTextEnd();
	}


} // namespace vgui


//-----------------------------------------------------------------------------
// Dictionary of start dir contexts 
//-----------------------------------------------------------------------------
static CUtlDict< CUtlString, unsigned short > s_StartDirContexts;

struct ColumnInfo_t
{
	char const* columnName;
	char const* columnText;
	int			startingWidth;
	int			minWidth;
	int			maxWidth;
	int			flags;
	SortFunc* pfnSort;
	Label::Alignment alignment;
};

static ColumnInfo_t g_ColInfo[] =
{
	{	"text",				"#FileOpenDialog_Col_Name",				175,	20, 10000, ListPanel::COLUMN_UNHIDABLE,		&ListFileNameSortFunc			, Label::a_west },
	{	"filesize",			"#FileOpenDialog_Col_Size",				100,	20, 10000, 0,								&ListFileSizeSortFunc			, Label::a_east },
	{	"type",				"#FileOpenDialog_Col_Type",				150,	20, 10000, 0,								&ListFileTypeSortFunc			, Label::a_west },
	{	"modified",			"#FileOpenDialog_Col_DateModified",		125,	20, 10000, 0,								&ListFileModifiedSortFunc		, Label::a_west },
	//	{	"created",			"#FileOpenDialog_Col_DateCreated",		125,	20, 10000, ListPanel::COLUMN_HIDDEN,		&ListFileCreatedSortFunc		, Label::a_west },
		{	"attributes",		"#FileOpenDialog_Col_Attributes",		50,		20, 10000, ListPanel::COLUMN_HIDDEN,		&ListFileAttributesSortFunc		, Label::a_west },
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
FileOpenDialog::FileOpenDialog(Panel* parent, const char* title, bool bOpenOnly, KeyValues* pContextKeyValues) :
	Frame(parent, "FileOpenDialog")
{
	m_DialogType = bOpenOnly ? FOD_OPEN : FOD_SAVE;
	Init(title, pContextKeyValues);
}


FileOpenDialog::FileOpenDialog(Panel* parent, const char* title, FileOpenDialogType_t type, KeyValues* pContextKeyValues) :
	Frame(parent, "FileOpenDialog")
{
	m_DialogType = type;
	Init(title, pContextKeyValues);
}

void FileOpenDialog::Init(const char* title, KeyValues* pContextKeyValues)
{
	m_bFileSelected = false;
	SetTitle(title, true);
	SetMinimizeButtonVisible(false);

#ifdef POSIX
	Q_strncpy(m_szLastPath, "/", sizeof(m_szLastPath));
#else
	Q_strncpy(m_szLastPath, "c:\\", sizeof(m_szLastPath));
#endif	

	m_pContextKeyValues = pContextKeyValues;

	// Get the list of available drives and put them in a menu here.
	// Start with the directory we are in.
	m_pFullPathEdit = new ComboBox(this, "FullPathEdit", 6, false);
	m_pFullPathEdit->GetTooltip()->SetTooltipFormatToSingleLine();

	// list panel
	m_pFileList = new ListPanel(this, "FileList");
	for (int i = 0; i < ARRAYSIZE(g_ColInfo); ++i)
	{
		const ColumnInfo_t& info = g_ColInfo[i];

		m_pFileList->AddColumnHeader(i, info.columnName, info.columnText, info.startingWidth, info.minWidth, info.maxWidth, info.flags);
		m_pFileList->SetSortFunc(i, info.pfnSort);
		m_pFileList->SetColumnTextAlignment(i, info.alignment);
	}

	m_pFileList->SetSortColumn(s_nLastSortColumn);
	m_pFileList->SetMultiselectEnabled(false);

	// file name edit box
	m_pFileNameEdit = new FileCompletionEdit(this);
	m_pFileNameEdit->AddActionSignalTarget(this);

	m_pFileTypeCombo = new ComboBox(this, "FileTypeCombo", 6, false);

	switch (m_DialogType)
	{
	case FOD_OPEN:
		m_pOpenButton = new Button(this, "OpenButton", "#FileOpenDialog_Open", this);
		break;
	case FOD_SAVE:
		m_pOpenButton = new Button(this, "OpenButton", "#FileOpenDialog_Save", this);
		break;
	case FOD_SELECT_DIRECTORY:
		m_pOpenButton = new Button(this, "OpenButton", "#FileOpenDialog_Select", this);
		m_pFileTypeCombo->SetVisible(false);
		break;
	}

	m_pCancelButton = new Button(this, "CancelButton", "#FileOpenDialog_Cancel", this);
	m_pFolderUpButton = new Button(this, "FolderUpButton", "", this);
	m_pFolderUpButton->GetTooltip()->SetText("#FileOpenDialog_ToolTip_Up");
	m_pNewFolderButton = new Button(this, "NewFolderButton", "", this);
	m_pNewFolderButton->GetTooltip()->SetText("#FileOpenDialog_ToolTip_NewFolder");
	m_pOpenInExplorerButton = new Button(this, "OpenInExplorerButton", "", this);

#if defined ( OSX )	
	m_pOpenInExplorerButton->GetTooltip()->SetText("#FileOpenDialog_ToolTip_OpenInFinderButton");
#elif defined ( POSIX )
	m_pOpenInExplorerButton->GetTooltip()->SetText("#FileOpenDialog_ToolTip_OpenInDesktopManagerButton");
#else // Assume Windows / Explorer
	m_pOpenInExplorerButton->GetTooltip()->SetText("#FileOpenDialog_ToolTip_OpenInExplorerButton");
#endif

	Label* lookIn = new Label(this, "LookInLabel", "#FileOpenDialog_Look_in");
	Label* fileName = new Label(this, "FileNameLabel",
		(m_DialogType != FOD_SELECT_DIRECTORY) ? "#FileOpenDialog_File_name" : "#FileOpenDialog_Directory_Name");

	m_pFolderIcon = new ImagePanel(NULL, "FolderIcon");

	// set up the control's initial positions
	SetSize(600, 260);

	int nFileEditLeftSide = (m_DialogType != FOD_SELECT_DIRECTORY) ? 84 : 100;
	int nFileNameWidth = (m_DialogType != FOD_SELECT_DIRECTORY) ? 72 : 82;

	m_pFullPathEdit->SetBounds(67, 32, 310, 24);
	m_pFolderUpButton->SetBounds(362, 32, 24, 24);
	m_pNewFolderButton->SetBounds(392, 32, 24, 24);
	m_pOpenInExplorerButton->SetBounds(332, 32, 24, 24);
	m_pFileList->SetBounds(10, 60, 406, 130);
	m_pFileNameEdit->SetBounds(nFileEditLeftSide, 194, 238, 24);
	m_pFileTypeCombo->SetBounds(nFileEditLeftSide, 224, 238, 24);
	m_pOpenButton->SetBounds(336, 194, 74, 24);
	m_pCancelButton->SetBounds(336, 224, 74, 24);
	lookIn->SetBounds(10, 32, 55, 24);
	fileName->SetBounds(10, 194, nFileNameWidth, 24);

	// set autolayout parameters
	m_pFullPathEdit->SetAutoResize(Panel::PIN_TOPLEFT, Panel::AUTORESIZE_RIGHT, 67, 32, -100, 0);
	m_pFileNameEdit->SetAutoResize(Panel::PIN_BOTTOMLEFT, Panel::AUTORESIZE_RIGHT, nFileEditLeftSide, -42, -104, 0);
	m_pFileTypeCombo->SetAutoResize(Panel::PIN_BOTTOMLEFT, Panel::AUTORESIZE_RIGHT, nFileEditLeftSide, -12, -104, 0);
	m_pFileList->SetAutoResize(Panel::PIN_TOPLEFT, Panel::AUTORESIZE_DOWNANDRIGHT, 10, 60, -10, -70);

	m_pFolderUpButton->SetPinCorner(Panel::PIN_TOPRIGHT, -40, 32);
	m_pNewFolderButton->SetPinCorner(Panel::PIN_TOPRIGHT, -10, 32);
	m_pOpenInExplorerButton->SetPinCorner(Panel::PIN_TOPRIGHT, -70, 32);
	m_pOpenButton->SetPinCorner(Panel::PIN_BOTTOMRIGHT, -16, -42);
	m_pCancelButton->SetPinCorner(Panel::PIN_BOTTOMRIGHT, -16, -12);
	lookIn->SetPinCorner(Panel::PIN_TOPLEFT, 10, 32);
	fileName->SetPinCorner(Panel::PIN_BOTTOMLEFT, 10, -42);

	// label settings
	lookIn->SetContentAlignment(Label::a_west);
	fileName->SetContentAlignment(Label::a_west);

	lookIn->SetAssociatedControl(m_pFullPathEdit);
	fileName->SetAssociatedControl(m_pFileNameEdit);

	if (m_DialogType != FOD_SELECT_DIRECTORY)
	{
		Label* fileType = new Label(this, "FileTypeLabel", "#FileOpenDialog_File_type");
		fileType->SetBounds(10, 224, 72, 24);
		fileType->SetPinCorner(Panel::PIN_BOTTOMLEFT, 10, -12);
		fileType->SetContentAlignment(Label::a_west);
		fileType->SetAssociatedControl(m_pFileTypeCombo);
	}

	// set tab positions
	GetFocusNavGroup().SetDefaultButton(m_pOpenButton);

	m_pFileNameEdit->SetTabPosition(1);
	m_pFileTypeCombo->SetTabPosition(2);
	m_pOpenButton->SetTabPosition(3);
	m_pCancelButton->SetTabPosition(4);
	m_pFullPathEdit->SetTabPosition(5);
	m_pFileList->SetTabPosition(6);

	m_pOpenButton->SetCommand((m_DialogType != FOD_SELECT_DIRECTORY) ? new KeyValues("OnOpen") : new KeyValues("SelectFolder"));
	m_pCancelButton->SetCommand("CloseModal");
	m_pFolderUpButton->SetCommand(new KeyValues("OnFolderUp"));
	m_pNewFolderButton->SetCommand(new KeyValues("OnNewFolder"));
	m_pOpenInExplorerButton->SetCommand(new KeyValues("OpenInExplorer"));

	SetSize(600, 384);

	m_nStartDirContext = s_StartDirContexts.InvalidIndex();

	// Set our starting path to the current directory
	char pLocalPath[255];
	g_pFullFileSystem->GetCurrentDirectory(pLocalPath, 255);
	if (!pLocalPath[0] || (IsOSX() && V_strlen(pLocalPath) <= 2))
	{
		const char* pszHomeDir = getenv("HOME");
		V_strcpy(pLocalPath, pszHomeDir);
	}
	SetStartDirectory(pLocalPath);

	// Because these call through virtual functions, we can't issue them in the constructor, so we post a message to ourselves instead!!
	PostMessage(GetVPanel(), new KeyValues("PopulateFileList"));
	PostMessage(GetVPanel(), new KeyValues("PopulateDriveList"));
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
FileOpenDialog::~FileOpenDialog()
{
	s_nLastSortColumn = m_pFileList->GetSortColumn();
	if (m_pContextKeyValues)
	{
		m_pContextKeyValues->deleteThis();
		m_pContextKeyValues = NULL;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Apply scheme settings
//-----------------------------------------------------------------------------
void FileOpenDialog::ApplySchemeSettings(IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	m_pFolderIcon->SetImage(scheme()->GetImage("resource/icon_folder", false));
	m_pFolderUpButton->AddImage(scheme()->GetImage("resource/icon_folderup", false), -3);
	m_pNewFolderButton->AddImage(scheme()->GetImage("resource/icon_newfolder", false), -3);
	m_pOpenInExplorerButton->AddImage(scheme()->GetImage("resource/icon_play_once", false), -3);

	ImageList* imageList = new ImageList(false);
	imageList->AddImage(scheme()->GetImage("resource/icon_file", false));
	imageList->AddImage(scheme()->GetImage("resource/icon_folder", false));
	imageList->AddImage(scheme()->GetImage("resource/icon_folder_selected", false));

	m_pFileList->SetImageList(imageList, true);
}


//-----------------------------------------------------------------------------
// Prevent default button ('select') from getting triggered
// when selecting directories. Instead, open the directory
//-----------------------------------------------------------------------------
void FileOpenDialog::OnKeyCodeTyped(KeyCode code)
{
	if (m_DialogType == FOD_SELECT_DIRECTORY && code == KEY_ENTER)
	{
		OnOpen();
	}
	else
	{
		BaseClass::OnKeyCodeTyped(code);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void FileOpenDialog::PopulateDriveList()
{
	char fullpath[MAX_PATH * 4];
	char subDirPath[MAX_PATH * 4];
	GetCurrentDirectory(fullpath, sizeof(fullpath) - MAX_PATH);
	Q_strncpy(subDirPath, fullpath, sizeof(subDirPath));

	m_pFullPathEdit->DeleteAllItems();

#ifdef WIN32
	// populate the drive list
	char buf[512];
	int len = system()->GetAvailableDrives(buf, 512);
	char* pBuf = buf;
	for (int i = 0; i < len / 4; i++)
	{
		m_pFullPathEdit->AddItem(pBuf, NULL);

		// is this our drive - add all subdirectories
		if (!_strnicmp(pBuf, fullpath, 2))
		{
			int indent = 0;
			char* pData = fullpath;
			while (*pData)
			{
				if (*pData == CORRECT_PATH_SEPARATOR)
				{
					if (indent > 0)
					{
						memset(subDirPath, ' ', indent);
						memcpy(subDirPath + indent, fullpath, pData - fullpath);
						subDirPath[indent + pData - fullpath] = 0;

						m_pFullPathEdit->AddItem(subDirPath, NULL);
					}
					indent += 2;
				}
				pData++;
			}
		}
		pBuf += 4;
	}
#else
	m_pFullPathEdit->AddItem("/", NULL);

	char* pData = fullpath;
	int indent = 0;
	while (*pData)
	{
		if (*pData == '/' && (pData[1] != '\0'))
		{
			if (indent > 0)
			{
				memset(subDirPath, ' ', indent);
				memcpy(subDirPath + indent, fullpath, pData - fullpath);
				subDirPath[indent + pData - fullpath] = 0;

				m_pFullPathEdit->AddItem(subDirPath, NULL);
			}
			indent += 2;
		}
		pData++;
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Delete self on close
//-----------------------------------------------------------------------------
void FileOpenDialog::OnClose()
{
	s_nLastSortColumn = m_pFileList->GetSortColumn();
	if (!m_bFileSelected)
	{
		KeyValues* pKeyValues = new KeyValues("FileSelectionCancelled");
		PostActionSignal(pKeyValues);
		m_bFileSelected = true;
	}

	m_pFileNameEdit->SetText("");
	m_pFileNameEdit->HideMenu();

	if (vgui::input()->GetAppModalSurface() == GetVPanel())
	{
		input()->SetAppModalSurface(NULL);
	}

	BaseClass::OnClose();
}

void FileOpenDialog::OnFolderUp()
{
	MoveUpFolder();
	OnOpen();
}

void FileOpenDialog::OnInputCompleted(KeyValues* data)
{
	if (m_hInputDialog.Get())
	{
		delete m_hInputDialog.Get();
	}

	input()->SetAppModalSurface(m_SaveModal);
	m_SaveModal = 0;

	NewFolder(data->GetString("text"));
	OnOpen();
}

void FileOpenDialog::OnInputCanceled()
{
	input()->SetAppModalSurface(m_SaveModal);
	m_SaveModal = 0;
}

void FileOpenDialog::OnNewFolder()
{
	if (m_hInputDialog.Get())
		delete m_hInputDialog.Get();

	m_hInputDialog = new InputDialog(this, "#FileOpenDialog_NewFolder_InputTitle", "#FileOpenDialog_NewFolderPrompt", "#FileOpenDialog_NewFolder_DefaultName");
	if (m_hInputDialog.Get())
	{
		m_SaveModal = input()->GetAppModalSurface();

		KeyValues* pContextKeyValues = new KeyValues("NewFolder");
		m_hInputDialog->SetSmallCaption(true);
		m_hInputDialog->SetMultiline(false);
		m_hInputDialog->DoModal(pContextKeyValues);
	}
}


//-----------------------------------------------------------------------------
// Opens the current file/folder in explorer
//-----------------------------------------------------------------------------
void FileOpenDialog::OnOpenInExplorer()
{
	char pCurrentDirectory[MAX_PATH];
	GetCurrentDirectory(pCurrentDirectory, sizeof(pCurrentDirectory));
#if !defined( _X360 ) && defined( WIN32 )
	ShellExecute(NULL, NULL, pCurrentDirectory, NULL, NULL, SW_SHOWNORMAL);
#elif defined( OSX )
	char szCmd[MAX_PATH * 2];
	Q_snprintf(szCmd, sizeof(szCmd), "/usr/bin/open \"%s\"", pCurrentDirectory);
	[[maybe_unused]] int result = ::system(szCmd);
#elif defined( LINUX )
	char szCmd[MAX_PATH * 2];
	Q_snprintf(szCmd, sizeof(szCmd), "xdg-open \"%s\" &", pCurrentDirectory);
	[[maybe_unused]] int result = ::system(szCmd);
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Handle for button commands
//-----------------------------------------------------------------------------
void FileOpenDialog::OnCommand(const char* command)
{
	if (!stricmp(command, "Cancel"))
	{
		Close();
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}


//-----------------------------------------------------------------------------
// Sets the start directory context (and resets the start directory in the process)
//-----------------------------------------------------------------------------
void FileOpenDialog::SetStartDirectoryContext(const char* pStartDirContext, const char* pDefaultDir)
{
	bool bUseCurrentDirectory = true;
	if (pStartDirContext)
	{
		m_nStartDirContext = s_StartDirContexts.Find(pStartDirContext);
		if (m_nStartDirContext == s_StartDirContexts.InvalidIndex())
		{
			m_nStartDirContext = s_StartDirContexts.Insert(pStartDirContext, pDefaultDir);
			bUseCurrentDirectory = (pDefaultDir == NULL);
		}
		else
		{
			bUseCurrentDirectory = false;
		}
	}
	else
	{
		m_nStartDirContext = s_StartDirContexts.InvalidIndex();
	}

	if (!bUseCurrentDirectory)
	{
		SetStartDirectory(s_StartDirContexts[m_nStartDirContext].Get());
	}
	else
	{
		// Set our starting path to the current directory
		char pLocalPath[255];
		g_pFullFileSystem->GetCurrentDirectory(pLocalPath, 255);
		SetStartDirectory(pLocalPath);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Set the starting directory of the file search.
//-----------------------------------------------------------------------------
void FileOpenDialog::SetStartDirectory(const char* dir)
{
	m_pFullPathEdit->SetText(dir);

	// ensure it's validity
	ValidatePath();

	// Store this in the start directory list
	if (m_nStartDirContext != s_StartDirContexts.InvalidIndex())
	{
		char pDirBuf[MAX_PATH];
		GetCurrentDirectory(pDirBuf, sizeof(pDirBuf));
		s_StartDirContexts[m_nStartDirContext] = pDirBuf;
	}

	PopulateDriveList();
}


//-----------------------------------------------------------------------------
// Purpose: Add filters for the drop down combo box
//-----------------------------------------------------------------------------
void FileOpenDialog::AddFilter(const char* filter, const char* filterName, bool bActive, const char* pFilterInfo)
{
	KeyValues* kv = new KeyValues("item");
	kv->SetString("filter", filter);
	kv->SetString("filterinfo", pFilterInfo);
	int itemID = m_pFileTypeCombo->AddItem(filterName, kv);
	if (bActive)
	{
		m_pFileTypeCombo->ActivateItem(itemID);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Activate the dialog
//-----------------------------------------------------------------------------
void FileOpenDialog::DoModal(bool bUnused)
{
	m_bFileSelected = false;
	m_pFileNameEdit->RequestFocus();
	BaseClass::DoModal();
}


//-----------------------------------------------------------------------------
// Purpose: Gets the directory this is currently in
//-----------------------------------------------------------------------------
void FileOpenDialog::GetCurrentDirectory(char* buf, int bufSize)
{
	// get the text from the text entry
	m_pFullPathEdit->GetText(buf, bufSize);
}


//-----------------------------------------------------------------------------
// Purpose: Get the last selected file name
//-----------------------------------------------------------------------------
void FileOpenDialog::GetSelectedFileName(char* buf, int bufSize)
{
	m_pFileNameEdit->GetText(buf, bufSize);
}

//-----------------------------------------------------------------------------
// Creates a new folder
//-----------------------------------------------------------------------------
void FileOpenDialog::NewFolder(char const* folderName)
{
	char pCurrentDirectory[MAX_PATH];
	GetCurrentDirectory(pCurrentDirectory, sizeof(pCurrentDirectory));

	char pFullPath[MAX_PATH];
	char pNewFolderName[MAX_PATH];
	Q_strncpy(pNewFolderName, folderName, sizeof(pNewFolderName));
	int i = 2;
	do
	{
		Q_MakeAbsolutePath(pFullPath, sizeof(pFullPath), pNewFolderName, pCurrentDirectory);

		std::wstring widePath = UTF8ToWString(pFullPath);
		if (!fs::exists(widePath) && !fs::is_directory(widePath)){
			fs::create_directories(widePath);
			m_pFileNameEdit->SetText(pNewFolderName);
			return;
		}

		Q_snprintf(pNewFolderName, sizeof(pNewFolderName), "%s%d", folderName, i);
		++i;
	} while (i <= 999);
}
IImage* FileOpenDialog::GetIconFromExtension(const char* extension){
#ifdef WIN32
	auto it = g_dicExtensionIcons.find(extension);
	if (it != g_dicExtensionIcons.end())
		return it->second;
	else {
		HICON icon = Win32GetFileIconByExtension(extension);
		if (icon == NULL)
			return nullptr;
		size_t iw, ih;
		auto bits = Win32ConvertHICONToRGBA(icon, iw, ih);
		DestroyIcon(icon);
		IconImage* img = new IconImage(bits.data(), iw, ih);
		std::string key = extension;
		g_dicExtensionIcons.insert(std::make_pair(key, img));
		return img;
	}
#else
	return nullptr;
#endif // WIN32
}

void FileOpenDialog::GetFileDescriptionByExtension(const char* extension, char* buf){
#ifdef WIN32
	std::string type = Win32GetFileDescriptionByExtension(extension);
	std::strcpy(buf, type.c_str());
#else
	returen extension;
#endif // WIN32
}

//-----------------------------------------------------------------------------
// Purpose: Move the directory structure up
//-----------------------------------------------------------------------------
void FileOpenDialog::MoveUpFolder()
{
	char fullpath[MAX_PATH * 4];
	GetCurrentDirectory(fullpath, sizeof(fullpath) - MAX_PATH);

	Q_StripLastDir(fullpath, sizeof(fullpath));
	// append a trailing slash
	Q_AppendSlash(fullpath, sizeof(fullpath));

	SetStartDirectory(fullpath);
	PopulateFileList();
	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Validate that the current path is valid
//-----------------------------------------------------------------------------
void FileOpenDialog::ValidatePath()
{
	char thingfullpath[MAX_PATH * 4];
	GetCurrentDirectory(thingfullpath, sizeof(thingfullpath) - MAX_PATH);
	std::wstring w = UTF8ToWString(thingfullpath);
	wchar_t fullpath[MAX_PATH * 4];
	wcsncpy(fullpath, w.c_str(), MAX_PATH * 4);
	Q_RemoveDotSlashes(fullpath);

	// when statting a directory on Windows, you want to include
	// the terminal slash exactly when you are statting a root
	// directory. PKMN.
#ifdef _WIN32
	if (wcslen(fullpath) != 3)
	{
		Q_StripTrailingSlash(fullpath);
	}
#endif
	// cleanup the path, we format tabs into the list to make it pretty in the UI
	Q_StripPrecedingAndTrailingWhitespace(fullpath);
	if (fs::is_directory(fullpath))
	{
		Q_AppendSlash(fullpath, sizeof(fullpath));
		Q_UnicodeToUTF8(fullpath, m_szLastPath, sizeof(m_szLastPath));
	}
	else
	{
		// failed to load file, use the previously successful path
	}

	m_pFullPathEdit->SetText(m_szLastPath);
	m_pFullPathEdit->GetTooltip()->SetText(m_szLastPath);
}

//-----------------------------------------------------------------------------
// Purpose: Fill the filelist with the names of all the files in the current directory
//-----------------------------------------------------------------------------
#define MAX_FILTER_LENGTH 255
void FileOpenDialog::PopulateFileList()
{
	// clear the current list
	m_pFileList->DeleteAllItems();
	// get the current directory
	char currentDir[MAX_PATH * 4];
	char filterList[MAX_FILTER_LENGTH + 1];
	GetCurrentDirectory(currentDir, sizeof(currentDir));
	auto dirIterator = fs::directory_iterator(UTF8ToWString(currentDir));
	KeyValues* combokv = m_pFileTypeCombo->GetActiveItemUserData();
	if (combokv)
		Q_strncpy(filterList, combokv->GetString("filter", "*"), MAX_FILTER_LENGTH);
	else
		// add wildcard for search
		Q_strncpy(filterList, "*\0", MAX_FILTER_LENGTH);

	std::vector<std::string> aryFilters = {};
	char* filterPtr = filterList;
	if (m_DialogType != FOD_SELECT_DIRECTORY){
		while ((filterPtr != NULL) && (*filterPtr != 0)){
			// parse the next filter in the list.
			char curFilter[MAX_FILTER_LENGTH];
			curFilter[0] = 0;
			int i = 0;
			while ((filterPtr != NULL) && ((*filterPtr == ',') || (*filterPtr == ';') || (*filterPtr <= ' '))){
				++filterPtr;
			}
			while ((filterPtr != NULL) && (*filterPtr != ',') && (*filterPtr != ';') && (*filterPtr > ' ')){
				curFilter[i++] = std::tolower(*(filterPtr++));
			}
			curFilter[i] = 0;
			if (curFilter[0] == 0)
				break;
			aryFilters.push_back(curFilter);
		}
	}
	// find all the directories
	KeyValues* kv = new KeyValues("item");
	for(auto& iter : dirIterator){
		std::string dirname = iter.path().filename().u8string();
		if (!iter.is_directory()) {
			std::string extension = iter.path().extension().u8string();
			if (aryFilters.size() > 0) {
				std::transform(extension.begin(), extension.end(), extension.begin(), static_cast<int(*)(int)>(std::tolower));
				if (std::find_if(aryFilters.begin(), aryFilters.end(), [&extension](std::string& a) -> bool {return WildMatch(extension, a); }) == aryFilters.end())
					continue;
			}
			IImage* img = GetIconFromExtension(extension.c_str());
			if (img)
				kv->SetPtr("iconImage", (void*)img);
			kv->SetInt("image", 1);
			kv->SetInt("imageSelected", 1);
			kv->SetInt("directory", 0);
			kv->SetString("filesize", Q_pretifymem(iter.file_size(), 0, true));
			char icon[64];
			GetFileDescriptionByExtension(extension.c_str(), icon);
			kv->SetString("type", icon);
		}
		else {
			if (dirname[0] != '.')
			{
				kv->SetPtr("iconImage", (void*)nullptr);
				kv->SetInt("image", 2);
				kv->SetInt("imageSelected", 3);
				kv->SetInt("directory", 1);
				kv->SetString("filesize", "");
				kv->SetString("type", "#FileOpenDialog_FileType_Folder");
			}
		}
		kv->SetString("text", dirname.c_str());
		bool filewritable = (iter.status().permissions() & fs::perms::owner_write) == fs::perms::others_write;
		kv->SetString("attributes", filewritable ? "WR" : "R");
		std::time_t tt = ToTimeT(iter.last_write_time());
		tm* tm = localtime(&tt);
		char buffer[80];
		strftime(buffer, 80, "%F %T", tm);

		kv->SetString("modified", buffer);
		kv->SetString("created", buffer);
		m_pFileList->AddItem(kv, 0, false, false);
	}
	kv->deleteThis();
	m_pFileList->SortList();
}


//-----------------------------------------------------------------------------
// Does the specified extension match something in the filter list?
//-----------------------------------------------------------------------------
bool FileOpenDialog::ExtensionMatchesFilter(const char* pExt)
{
	KeyValues* combokv = m_pFileTypeCombo->GetActiveItemUserData();
	if (!combokv)
		return true;

	char filterList[MAX_FILTER_LENGTH + 1];
	Q_strncpy(filterList, combokv->GetString("filter", "*"), MAX_FILTER_LENGTH);

	char* filterPtr = filterList;
	while ((filterPtr != NULL) && (*filterPtr != 0))
	{
		// parse the next filter in the list.
		char curFilter[MAX_FILTER_LENGTH];
		curFilter[0] = 0;
		int i = 0;
		while ((filterPtr != NULL) && ((*filterPtr == ',') || (*filterPtr == ';') || (*filterPtr <= ' ')))
		{
			++filterPtr;
		}
		while ((filterPtr != NULL) && (*filterPtr != ',') && (*filterPtr != ';') && (*filterPtr > ' '))
		{
			curFilter[i++] = *(filterPtr++);
		}
		curFilter[i] = 0;

		if (curFilter[0] == 0)
			break;

		if (!Q_stricmp(curFilter, "*") || !Q_stricmp(curFilter, "*.*"))
			return true;

		// FIXME: This isn't exactly right, but tough cookies;
		// it assumes the first two characters of the filter are *.
		Assert(curFilter[0] == '*' && curFilter[1] == '.');
		if (!Q_stricmp(&curFilter[2], pExt))
			return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Choose the first non *.* filter in the filter list
//-----------------------------------------------------------------------------
void FileOpenDialog::ChooseExtension(char* pExt, int nBufLen)
{
	pExt[0] = 0;

	KeyValues* combokv = m_pFileTypeCombo->GetActiveItemUserData();
	if (!combokv)
		return;

	char filterList[MAX_FILTER_LENGTH + 1];
	Q_strncpy(filterList, combokv->GetString("filter", "*"), MAX_FILTER_LENGTH);

	char* filterPtr = filterList;
	while ((filterPtr != NULL) && (*filterPtr != 0))
	{
		// parse the next filter in the list.
		char curFilter[MAX_FILTER_LENGTH];
		curFilter[0] = 0;
		int i = 0;
		while ((filterPtr != NULL) && ((*filterPtr == ',') || (*filterPtr == ';') || (*filterPtr <= ' ')))
		{
			++filterPtr;
		}
		while ((filterPtr != NULL) && (*filterPtr != ',') && (*filterPtr != ';') && (*filterPtr > ' '))
		{
			curFilter[i++] = *(filterPtr++);
		}
		curFilter[i] = 0;

		if (curFilter[0] == 0)
			break;

		if (!Q_stricmp(curFilter, "*") || !Q_stricmp(curFilter, "*.*"))
			continue;

		// FIXME: This isn't exactly right, but tough cookies;
		// it assumes the first two characters of the filter are *.
		Assert(curFilter[0] == '*' && curFilter[1] == '.');
		Q_strncpy(pExt, &curFilter[1], nBufLen);
		break;
	}
}


//-----------------------------------------------------------------------------
// Saves the file to the start dir context
//-----------------------------------------------------------------------------
void FileOpenDialog::SaveFileToStartDirContext(const char* pFullPath)
{
	if (m_nStartDirContext == s_StartDirContexts.InvalidIndex())
		return;

	char pPath[MAX_PATH];
	pPath[0] = 0;
	Q_ExtractFilePath(pFullPath, pPath, sizeof(pPath));
	s_StartDirContexts[m_nStartDirContext] = pPath;
}


//-----------------------------------------------------------------------------
// Posts a file selected message
//-----------------------------------------------------------------------------
void FileOpenDialog::PostFileSelectedMessage(const char* pFileName)
{
	m_bFileSelected = true;

	// open the file!
	KeyValues* pKeyValues = new KeyValues("FileSelected", "fullpath", pFileName);
	KeyValues* pFilterKeys = m_pFileTypeCombo->GetActiveItemUserData();
	const char* pFilterInfo = pFilterKeys ? pFilterKeys->GetString("filterinfo", NULL) : NULL;
	if (pFilterInfo)
	{
		pKeyValues->SetString("filterinfo", pFilterInfo);
	}
	if (m_pContextKeyValues)
	{
		pKeyValues->AddSubKey(m_pContextKeyValues);
		m_pContextKeyValues = NULL;
	}
	PostActionSignal(pKeyValues);
	CloseModal();
}


//-----------------------------------------------------------------------------
// Selects the current folder
//-----------------------------------------------------------------------------
void FileOpenDialog::OnSelectFolder()
{
	ValidatePath();

	// construct a file path
	char pFileName[MAX_PATH];
	GetSelectedFileName(pFileName, sizeof(pFileName));

	Q_StripTrailingSlash(pFileName);

	if (!stricmp(pFileName, ".."))
	{
		MoveUpFolder();

		// clear the name text
		m_pFileNameEdit->SetText("");
		return;
	}

	if (!stricmp(pFileName, "."))
	{
		// clear the name text
		m_pFileNameEdit->SetText("");
		return;
	}

	// Compute the full path
	char pFullPath[MAX_PATH * 4];
	if (!Q_IsAbsolutePath(pFileName))
	{
		GetCurrentDirectory(pFullPath, sizeof(pFullPath) - MAX_PATH);
		strcat(pFullPath, pFileName);
		if (!pFileName[0])
		{
			Q_StripTrailingSlash(pFullPath);
		}
	}
	else
	{
		Q_strncpy(pFullPath, pFileName, sizeof(pFullPath));
	}

	if (fs::exists(UTF8ToWString(pFullPath)))
	{
		// open the file!
		SaveFileToStartDirContext(pFullPath);
		PostFileSelectedMessage(pFullPath);
		return;
	}

	PopulateDriveList();
	PopulateFileList();
	InvalidateLayout();
}


//-----------------------------------------------------------------------------
// Purpose: Handle the open button being pressed
//			checks on what has changed and acts accordingly
//-----------------------------------------------------------------------------
void FileOpenDialog::OnOpen()
{
	ValidatePath();

	// construct a file path
	char pFileName[MAX_PATH];
	GetSelectedFileName(pFileName, sizeof(pFileName));

	int nLen = Q_strlen(pFileName);
	bool bSpecifiedDirectory = (pFileName[nLen - 1] == '/' || pFileName[nLen - 1] == '\\') && (!IsOSX() || (IsOSX() && !Q_stristr(pFileName, ".app")));
	Q_StripTrailingSlash(pFileName);

	if (!stricmp(pFileName, ".."))
	{
		MoveUpFolder();

		// clear the name text
		m_pFileNameEdit->SetText("");
		return;
	}

	if (!stricmp(pFileName, "."))
	{
		// clear the name text
		m_pFileNameEdit->SetText("");
		return;
	}

	// Compute the full path
	char pFullPath[MAX_PATH * 4];
	if (!Q_IsAbsolutePath(pFileName))
	{
		GetCurrentDirectory(pFullPath, sizeof(pFullPath) - MAX_PATH);
		Q_AppendSlash(pFullPath, sizeof(pFullPath));
		strcat(pFullPath, pFileName);
		if (!pFileName[0])
		{
			Q_StripTrailingSlash(pFullPath);
		}
	}
	else
	{
		Q_strncpy(pFullPath, pFileName, sizeof(pFullPath));
	}

	Q_StripTrailingSlash(pFullPath);

	// when statting a directory on Windows, you want to include
	// the terminal slash exactly when you are statting a root
	// directory. PKMN.
#ifdef _WIN32
	if (Q_strlen(pFullPath) == 2)
	{
		Q_AppendSlash(pFullPath, Q_ARRAYSIZE(pFullPath));
	}
#endif

	// If the name specified is a directory, then change directory
	if (fs::is_directory(UTF8ToWString(pFullPath)) && (!IsOSX() || (IsOSX() && !Q_stristr(pFullPath, ".app"))))
	{
		// it's a directory; change to the specified directory
		if (!bSpecifiedDirectory)
		{
			Q_AppendSlash(pFullPath, Q_ARRAYSIZE(pFullPath));
		}
		SetStartDirectory(pFullPath);

		// clear the name text
		m_pFileNameEdit->SetText("");
		m_pFileNameEdit->HideMenu();

		PopulateDriveList();
		PopulateFileList();
		InvalidateLayout();
		return;
	}
	else if (bSpecifiedDirectory)
	{
		PopulateDriveList();
		PopulateFileList();
		InvalidateLayout();
		return;
	}

	// Append suffix of the first filter that isn't *.*
	char extension[512];
	Q_ExtractFileExtension(pFullPath, extension, sizeof(extension));
	if (!ExtensionMatchesFilter(extension))
	{
		ChooseExtension(extension, sizeof(extension));
		Q_SetExtension(pFullPath, extension, sizeof(pFullPath));
	}

	if (fs::exists(UTF8ToWString(pFullPath)))
	{
		// open the file!
		SaveFileToStartDirContext(pFullPath);
		PostFileSelectedMessage(pFullPath);
		return;
	}

	// file not found
	if ((m_DialogType == FOD_SAVE) && pFileName[0])
	{
		// open the file!
		SaveFileToStartDirContext(pFullPath);
		PostFileSelectedMessage(pFullPath);
		return;
	}

	PopulateDriveList();
	PopulateFileList();
	InvalidateLayout();
}


//-----------------------------------------------------------------------------
// Purpose: using the file edit box as a prefix, create a menu of all possible files 
//-----------------------------------------------------------------------------
void FileOpenDialog::PopulateFileNameCompletion()
{
	char buf[80];
	m_pFileNameEdit->GetText(buf, 80);
	wchar_t wbuf[80];
	m_pFileNameEdit->GetText(wbuf, 80);
	int bufLen = wcslen(wbuf);

	// delete all items before we check if there's even a string
	m_pFileNameEdit->DeleteAllItems();

	// no string at all - don't show even bother showing it
	if (bufLen == 0)
	{
		m_pFileNameEdit->HideMenu();
		return;
	}

	// what files use current string as a prefix?
	int nCount = m_pFileList->GetItemCount();
	int i;
	for (i = 0; i < nCount; i++)
	{
		KeyValues* kv = m_pFileList->GetItem(m_pFileList->GetItemIDFromRow(i));
		const wchar_t* wszString = kv->GetWString("text");
		if (!_wcsnicmp(wbuf, wszString, bufLen))
		{
			m_pFileNameEdit->AddItem(wszString, NULL);
		}
	}

	// if there are any items - show the menu
	if (m_pFileNameEdit->GetItemCount() > 0)
	{
		m_pFileNameEdit->ShowMenu();
	}
	else
	{
		m_pFileNameEdit->HideMenu();
	}

	m_pFileNameEdit->InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Handle an item in the list being selected
//-----------------------------------------------------------------------------
void FileOpenDialog::OnItemSelected()
{
	// make sure only one item is selected
	if (m_pFileList->GetSelectedItemsCount() != 1)
	{
		m_pFileNameEdit->SetText("");
	}
	else
	{
		// put the file name into the text edit box
		KeyValues* data = m_pFileList->GetItem(m_pFileList->GetSelectedItem(0));
		m_pFileNameEdit->SetText(data->GetString("text"));
	}

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Handle an item in the Drive combo box being selected
//-----------------------------------------------------------------------------
void FileOpenDialog::OnTextChanged(KeyValues* kv)
{
	Panel* pPanel = (Panel*)kv->GetPtr("panel", NULL);

	// first check which control had its text changed!
	if (pPanel == m_pFullPathEdit)
	{
		m_pFileNameEdit->HideMenu();
		m_pFileNameEdit->SetText("");
		OnOpen();
	}
	else if (pPanel == m_pFileNameEdit)
	{
		PopulateFileNameCompletion();
	}
	else if (pPanel == m_pFileTypeCombo)
	{
		m_pFileNameEdit->HideMenu();
		PopulateFileList();
	}
}