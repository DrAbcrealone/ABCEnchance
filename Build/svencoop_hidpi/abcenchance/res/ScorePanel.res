"abcenchance/res/ScorePanel.res"
{
"ScorePanel"
{
	"ControlName"		"CScorePanel"
	"fieldName"		"ScorePanel"
	"xpos"	"c-367"
	"ypos"	"c-431"
	"wide"	"735"
	"tall"	"650"
	"autoResize"		"0"
	"pinCorner"		"0"
	"visible"		"1"
	"enabled"		"1"
	"tabPosition"		"0"
	"settitlebarvisible"		"0"
	"bgcolor_override" "0 0 0 64"

	// Column widths
	// Sum of these must equal width of PlayerList
	"column_avatar"		"24"
	"column_donor"		"18"
	"column_name"		"164"
	"column_steamid"	"92"
	"column_health"		"47"
	"column_armor"		"47"
	"column_frags"		"47"
	"column_deaths"		"47"
	"column_ping"		"80"

	// Icon of muted players (without .tga)
	"muted_icon"		"abcenchance/tga/muted_icon32"
	"donor1_icon"		"abcenchance/tga/donor_1"
	"donor2_icon"		"abcenchance/tga/donor_2"
	"donor3_icon"		"abcenchance/tga/donor_3"
	"donor4_icon"		"abcenchance/tga/donor_4"
	"donor5_icon"		"abcenchance/tga/donor_5"
	"donor6_icon"		"abcenchance/tga/donor_6"
	"admin_icon"		"abcenchance/tga/admin_icon32"
	"server_icon"		"abcenchance/tga/serverowner_icon32"

	// Minimum height of the score panel
	"min_height"		"300"

	// Minimum distance between vertical edges of the score panel and vertical edges of the screen
	"vertical_margin"	"30"

	// Distance from bottom edge of player list to bottom edge of score panel
	"bottom_padding"	"4"
}
"ServerName"
{
	"ControlName"		"Label"
	"fieldName"		"ServerName"
	"xpos"	"10"
	"ypos"	"3"
	"wide"	"666"
	"tall"	"13"
	"autoResize"		"1"
	"pinCorner"		"0"
	"visible"		"1"
	"enabled"		"1"
	"tabPosition"		"0"
	"labelText"		"Sven Co-op Server"
	"textAlignment"		"west"
	"dulltext"		"0"
	"brighttext"		"1"
	"font"		"Default"
	"wrap"		"0"
}
"NextMap"
{
	"ControlName"		"Label"
	"fieldName"		"NextMap"
	"xpos"	"399"
	"ypos"	"15"
	"wide"	"255"
	"tall"	"13"
	"autoResize"		"0"
	"pinCorner"		"1"
	"visible"		"1"
	"enabled"		"1"
	"tabPosition"		"0"
	"labelText"		""
	"textAlignment"		"east"
	"dulltext"		"0"
	"brighttext"		"0"
	"wrap"		"0"
}
"TimeEnd"
{
	"ControlName"		"Label"
	"fieldName"		"TimeEnd"
	"xpos"	"661"
	"ypos"	"15"
	"wide"	"63"
	"tall"	"10"
	"autoResize"		"0"
	"pinCorner"		"0"
	"visible"		"1"
	"enabled"		"1"
	"tabPosition"		"0"
	"labelText"		"666:666"
	"textAlignment"		"east"
	"dulltext"		"0"
	"brighttext"		"0"
	"wrap"		"0"
}
"MapName"
{
	"ControlName"		"Label"
	"fieldName"		"MapName"
	"xpos"	"10"
	"ypos"	"15"
	"wide"	"255"
	"tall"	"13"
	"autoResize"		"0"
	"pinCorner"		"0"
	"visible"		"1"
	"enabled"		"1"
	"tabPosition"		"0"
	"labelText"		""
	"textAlignment"		"west"
	"dulltext"		"0"
	"brighttext"		"0"
	"wrap"		"0"
}
"PlayerCount"
{
	"ControlName"		"Label"
	"fieldName"		"PlayerCount"
	"xpos"	"661"
	"ypos"	"2"
	"wide"	"63"
	"tall"	"10"
	"autoResize"		"0"
	"pinCorner"		"0"
	"visible"		"1"
	"enabled"		"1"
	"tabPosition"		"0"
	"labelText"		"666/666"
	"textAlignment"		"east"
	"dulltext"		"0"
	"brighttext"		"0"
	"wrap"		"0"
}
"PlayerList"
{
	"ControlName"		"SectionedListPanel"
	"fieldName"		"PlayerList"
	"xpos"	"5"
	"ypos"	"31"
	"wide"	"725"
	"tall"	"853"
	"autoResize"		"0"
	"pinCorner"		"0"
	"visible"		"1"
	"enabled"		"1"
	"tabPosition"		"0"
	"sectiongap"	"4"
	"bgcolor_override" "0 0 0 84"
}
}
