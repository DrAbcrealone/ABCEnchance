"abcenchance/res/HealthPanel.res"
{
"HealthPanel"
{
	"ControlName"		"CHealthPanel"
	"fieldName"		"HealthPanel"
	"xpos"	"0"
	"ypos"		"rs1.0"
	"wide"	"399"
	"tall"	"39"

	"autoResize"		"1"
	"visible"		"1"
	"enabled"		"1"
	"proportionalToParent" "1"
}

"Background"
{
	"ControlName"		"ImagePanel"
	"fieldName"		"Background"
	"xpos"	"0"
	"ypos"		"rs1.0"
	"wide"		"f0"
	"tall"		"f0"
	"visible"		"1"
	"scaleImage"		"1"
	"proportionalToParent" "1"
	"image"			"abcenchance/tga/healthpanel_background"
}

"HealthIcon"
{
	"ControlName"		"ImagePanel"
	"fieldName"		"HealthIcon"
	"xpos"	"7"
	"ypos"		"cs-0.5"
	"wide"	"19"
	"tall"	"19"
	"visible"		"1"
	"proportionalToParent" "1"
	"image"			"abcenchance/tga/healthbar_icon"
	"scaleImage"		"1"
}

"Health"
{
	"ControlName"		"Label"
	"fieldName"		"Health"
	"xpos"	"18"
	"ypos"		"cs-0.5"
	"wide"	"63"
	"tall"	"39"
	"visible"		"1"
	"enabled"		"1"
	"labelText"		"0"
	"proportionalToParent" "1"
	"textAlignment"		"center"
	"brighttext"		"1"
	"font"		"HealthBar"
	"wrap"		"0"
}
"HealthBar"
{
	"ControlName"		"ImageClipPanel"
	"fieldName"		"HealthBar"
	"xpos"	"73"
	"ypos"		"cs-0.5"
	"wide"	"74"
	"tall"	"14"
	"visible"		"1"
	"proportionalToParent" "1"
	"image"			"abcenchance/tga/healthbar"
	"scaleImage"		"1"
	"zpos"		"999"
}
"HealthBackground"
{
	"ControlName"		"ImagePanel"
	"fieldName"		"HealthBackground"
	"xpos"	"73"
	"ypos"		"cs-0.5"
	"wide"	"74"
	"tall"	"14"
	"visible"		"1"
	"proportionalToParent" "1"
	"image"			"abcenchance/tga/healthbar_bg"
	"scaleImage"		"1"
	"zpos"		"0"
}

"ArmorIcon"
{
	"ControlName"		"ImagePanel"
	"fieldName"		"ArmorIcon"
	"xpos"	"159"
	"ypos"		"cs-0.5"
	"wide"	"19"
	"tall"	"19"
	"visible"		"1"
	"proportionalToParent" "1"
	"image"			"abcenchance/tga/armorbar_icon"
	"scaleImage"		"1"
}
"Armor"
{
	"ControlName"		"Label"
	"fieldName"		"Armor"
	"xpos"	"170"
	"ypos"		"cs-0.5"
	"wide"	"63"
	"tall"	"39"
	"visible"		"1"
	"enabled"		"1"
	"labelText"		"0"
	"proportionalToParent" "1"
	"textAlignment"		"center"
	"brighttext"		"1"
	"font"		"HealthBar"
	"wrap"		"0"
}
"ArmorBar"
{
	"ControlName"		"ImageClipPanel"
	"fieldName"		"ArmorBar"
	"xpos"	"225"
	"ypos"		"cs-0.5"
	"wide"	"74"
	"tall"	"14"
	"visible"		"1"
	"proportionalToParent" "1"
	"image"			"abcenchance/tga/armorbar"
	"scaleImage"		"1"
	"zpos"		"999"
}
"ArmorBackground"
{
	"ControlName"		"ImagePanel"
	"fieldName"		"ArmorBackground"
	"xpos"	"225"
	"ypos"		"cs-0.5"
	"wide"	"74"
	"tall"	"14"
	"visible"		"1"
	"proportionalToParent" "1"
	"image"			"abcenchance/tga/armorbar_bg"
	"scaleImage"		"1"
	"zpos"		"0"
}

"Longjump"
{
	"ControlName"		"ImagePanel"
	"fieldName"		"Longjump"
	"xpos"	"330"
	"ypos"		"cs-0.5"
	"wide"	"26"
	"tall"	"26"
	"visible"		"1"
	"proportionalToParent" "1"
	"scaleImage"		"1"
	"image"			"abcenchance/tga/longjump"
}
}
