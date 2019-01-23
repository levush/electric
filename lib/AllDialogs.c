/* 3D Options */
static DIALOGITEM us_3ddepthdialogitems[] =
{
 /*  1 */ {0, {552,300,576,380}, BUTTON, N_("OK")},
 /*  2 */ {0, {552,188,576,268}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,8,404,168}, SCROLL, x_("")},
 /*  4 */ {0, {32,184,544,388}, USERDRAWN, x_("")},
 /*  5 */ {0, {412,8,428,88}, MESSAGE, N_("Thickness:")},
 /*  6 */ {0, {412,96,428,168}, EDITTEXT, x_("0")},
 /*  7 */ {0, {8,8,24,324}, MESSAGE, x_("")},
 /*  8 */ {0, {556,9,572,161}, CHECK, N_("Use Perspective")},
 /*  9 */ {0, {436,8,452,88}, MESSAGE, N_("Height:")},
 /* 10 */ {0, {436,96,452,168}, EDITTEXT, x_("0")},
 /* 11 */ {0, {460,8,476,180}, MESSAGE, N_("Separate flat layers by:")},
 /* 12 */ {0, {480,96,496,168}, EDITTEXT, x_("0")},
 /* 13 */ {0, {508,28,532,148}, BUTTON, N_("Clean Up")}
};
static DIALOG us_3ddepthdialog = {{75,75,660,473}, N_("3D Options"), 0, 13, us_3ddepthdialogitems, 0, 0};

/* About Electric */
static DIALOGITEM us_aboutgnudialogitems[] =
{
 /*  1 */ {0, {24,320,48,400}, BUTTON, N_("OK")},
 /*  2 */ {0, {308,12,324,356}, MESSAGE, N_("Electric comes with ABSOLUTELY NO WARRANTY")},
 /*  3 */ {0, {284,12,300,489}, MESSAGE, N_("Copyright (c) 2000 Static Free Software (www.staticfreesoft.com)")},
 /*  4 */ {0, {56,8,72,221}, MESSAGE, N_("Written by Steven M. Rubin")},
 /*  5 */ {0, {8,8,24,295}, MESSAGE, N_("The Electric(tm) Design System")},
 /*  6 */ {0, {32,8,48,246}, MESSAGE, N_("Version XXXX")},
 /*  7 */ {0, {4,420,36,452}, ICON|INACTIVE, (CHAR *)us_icon130},
 /*  8 */ {0, {36,420,68,452}, ICON|INACTIVE, (CHAR *)us_icon129},
 /*  9 */ {0, {4,452,36,484}, ICON|INACTIVE, (CHAR *)us_icon131},
 /* 10 */ {0, {36,452,68,484}, ICON|INACTIVE, (CHAR *)us_icon132},
 /* 11 */ {0, {100,8,273,487}, SCROLL, x_("")},
 /* 12 */ {0, {76,160,94,348}, BUTTON, N_("And a Cast of Thousands")},
 /* 13 */ {0, {332,12,348,330}, MESSAGE, N_("This is free software, and you are welcome to")},
 /* 14 */ {0, {308,358,326,487}, BUTTON, N_("Warranty details")},
 /* 15 */ {0, {344,358,362,487}, BUTTON, N_("Copying details")},
 /* 16 */ {0, {352,12,368,309}, MESSAGE, N_("redistribute it under certain conditions")},
 /* 17 */ {0, {76,388,92,484}, POPUP, x_("")}
};
static DIALOG us_aboutgnudialog = {{50,75,427,574}, 0, 0, 17, us_aboutgnudialogitems, 0, 0};

/* Advanced function selection */
static DIALOGITEM us_advdialogitems[] =
{
 /*  1 */ {0, {8,8,24,268}, MESSAGE, N_("These are advanced commands")},
 /*  2 */ {0, {14,276,38,352}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {152,188,176,348}, BUTTON, N_("Edit Dialogs")},
 /*  4 */ {0, {88,16,112,176}, BUTTON, N_("List Changed Options")},
 /*  5 */ {0, {184,188,208,348}, BUTTON, N_("Examine Memory Arena")},
 /*  6 */ {0, {184,16,208,176}, BUTTON, N_("Dump Name Space")},
 /*  7 */ {0, {152,16,176,176}, BUTTON, N_("Show R-Tree for Cell")},
 /*  8 */ {0, {88,188,112,348}, BUTTON, N_("Save Options as Text")},
 /*  9 */ {0, {120,16,144,176}, BUTTON, N_("Interactive Undo")},
 /* 10 */ {0, {120,188,144,348}, BUTTON, N_("Toggle Internal Errors")},
 /* 11 */ {0, {54,188,78,348}, BUTTON, N_("Edit Variables")},
 /* 12 */ {0, {28,8,44,268}, MESSAGE, N_("And should not normally be used")},
 /* 13 */ {0, {54,16,78,176}, BUTTON, N_("Language Translation")}
};
static DIALOG us_advdialog = {{75,75,292,432}, N_("Advanced Users Only"), 0, 13, us_advdialogitems, 0, 0};

/* Alignment Options */
static DIALOGITEM us_alignmentdialogitems[] =
{
 /*  1 */ {0, {68,340,92,404}, BUTTON, N_("OK")},
 /*  2 */ {0, {68,32,92,96}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,205}, MESSAGE, N_("Alignment of cursor to grid:")},
 /*  4 */ {0, {40,8,56,205}, MESSAGE, N_("Alignment of edges to grid:")},
 /*  5 */ {0, {8,208,24,280}, EDITTEXT, x_("")},
 /*  6 */ {0, {40,208,56,280}, EDITTEXT, x_("")},
 /*  7 */ {0, {16,284,32,426}, MESSAGE, N_("Values of zero will")},
 /*  8 */ {0, {32,284,48,428}, MESSAGE, N_("cause no alignment.")}
};
static DIALOG us_alignmentdialog = {{50,75,154,512}, N_("Alignment Options"), 0, 8, us_alignmentdialogitems, 0, 0};

/* Analyze Cell Network */
static DIALOGITEM net_analyzedialogitems[] =
{
 /*  1 */ {0, {220,236,244,316}, BUTTON, N_("Done")},
 /*  2 */ {0, {28,8,184,272}, SCROLL, x_("")},
 /*  3 */ {0, {8,8,24,120}, MESSAGE, N_("Components:")},
 /*  4 */ {0, {28,280,184,544}, SCROLL, x_("")},
 /*  5 */ {0, {8,280,24,392}, MESSAGE, N_("Networks:")},
 /*  6 */ {0, {188,20,204,120}, BUTTON, N_("Make Unique")},
 /*  7 */ {0, {188,124,204,224}, BUTTON, N_("Not Unique")},
 /*  8 */ {0, {188,292,204,392}, BUTTON, N_("Make Unique")},
 /*  9 */ {0, {188,396,204,496}, BUTTON, N_("Not Unique")}
};
static DIALOG net_analyzedialog = {{75,75,328,628}, N_("Analyze Network"), 0, 9, net_analyzedialogitems, 0, 0};

/* Annular Ring */
static DIALOGITEM us_annringdialogitems[] =
{
 /*  1 */ {0, {268,176,292,240}, BUTTON, N_("OK")},
 /*  2 */ {0, {268,20,292,84}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {164,12,180,160}, MESSAGE, N_("Inner Radius:")},
 /*  4 */ {0, {164,164,180,244}, EDITTEXT, x_("")},
 /*  5 */ {0, {188,12,204,160}, MESSAGE, N_("Outer Radius:")},
 /*  6 */ {0, {188,164,204,244}, EDITTEXT, x_("")},
 /*  7 */ {0, {212,12,228,160}, MESSAGE, N_("Number of segments:")},
 /*  8 */ {0, {212,164,228,244}, EDITTEXT, x_("32")},
 /*  9 */ {0, {236,12,252,160}, MESSAGE, N_("Number of degrees:")},
 /* 10 */ {0, {236,164,252,244}, EDITTEXT, x_("360")},
 /* 11 */ {0, {8,8,24,172}, MESSAGE, N_("Layer to use for ring:")},
 /* 12 */ {0, {28,8,156,244}, SCROLL, x_("")}
};
static DIALOG us_annringdialog = {{75,75,376,330}, N_("Annulus Construction"), 0, 12, us_annringdialogitems, 0, 0};

/* Antenna Rules Options */
static DIALOGITEM erc_antruldialogitems[] =
{
 /*  1 */ {0, {204,208,228,288}, BUTTON, N_("OK")},
 /*  2 */ {0, {204,32,228,112}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {24,8,168,316}, SCROLL, x_("")},
 /*  4 */ {0, {4,4,20,160}, MESSAGE, N_("Arcs in technology:")},
 /*  5 */ {0, {4,160,20,316}, MESSAGE, x_("")},
 /*  6 */ {0, {172,216,188,312}, EDITTEXT, x_("")},
 /*  7 */ {0, {172,8,188,208}, MESSAGE, N_("Maximum antenna ratio:")}
};
static DIALOG erc_antruldialog = {{75,75,312,401}, N_("Antenna Rules Options"), 0, 7, erc_antruldialogitems, 0, 0};

/* Array */
static DIALOGITEM us_arraydialogitems[] =
{
 /*  1 */ {0, {264,412,288,476}, BUTTON, N_("OK")},
 /*  2 */ {0, {264,316,288,380}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {28,160,44,205}, EDITTEXT, x_("1")},
 /*  4 */ {0, {108,160,124,205}, EDITTEXT, x_("1")},
 /*  5 */ {0, {180,160,196,235}, EDITTEXT, x_("0")},
 /*  6 */ {0, {208,160,224,235}, EDITTEXT, x_("0")},
 /*  7 */ {0, {268,4,284,182}, CHECK, N_("Generate array indices")},
 /*  8 */ {0, {4,216,20,393}, CHECK, N_("Flip alternate columns")},
 /*  9 */ {0, {84,216,100,376}, CHECK, N_("Flip alternate rows")},
 /* 10 */ {0, {28,36,44,151}, MESSAGE, N_("X repeat factor:")},
 /* 11 */ {0, {108,36,124,151}, MESSAGE, N_("Y repeat factor:")},
 /* 12 */ {0, {180,4,196,154}, MESSAGE, N_("X edge overlap:")},
 /* 13 */ {0, {208,4,224,154}, MESSAGE, N_("Y centerline distance:")},
 /* 14 */ {0, {160,244,176,480}, RADIO, N_("Space by edge overlap")},
 /* 15 */ {0, {184,244,200,480}, RADIO, N_("Space by centerline distance")},
 /* 16 */ {0, {28,216,44,425}, CHECK, N_("Stagger alternate columns")},
 /* 17 */ {0, {108,216,124,400}, CHECK, N_("Stagger alternate rows")},
 /* 18 */ {0, {208,244,224,480}, RADIO, N_("Space by characteristic spacing")},
 /* 19 */ {0, {232,244,248,480}, RADIO, N_("Space by last measured distance")},
 /* 20 */ {0, {244,4,260,182}, CHECK, N_("Linear diagonal array")},
 /* 21 */ {0, {52,216,68,425}, CHECK, N_("Center about original")},
 /* 22 */ {0, {132,216,148,425}, CHECK, N_("Center about original")},
 /* 23 */ {0, {154,4,155,480}, DIVIDELINE, x_("")},
 /* 24 */ {0, {292,4,308,302}, CHECK, N_("Only place entries that are DRC correct")}
};
static DIALOG us_arraydialog = {{50,75,367,565}, N_("Array Current Objects"), 0, 24, us_arraydialogitems, 0, 0};

/* Artwork Color */
static DIALOGITEM us_artworkdialogitems[] =
{
 /*  1 */ {0, {260,312,284,376}, BUTTON, N_("OK")},
 /*  2 */ {0, {216,312,240,376}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {108,280,124,420}, RADIO, N_("Outlined Pattern")},
 /*  4 */ {0, {34,4,290,260}, USERDRAWN, x_("")},
 /*  5 */ {0, {52,280,68,376}, RADIO, N_("Solid color")},
 /*  6 */ {0, {148,280,164,332}, MESSAGE, N_("Color:")},
 /*  7 */ {0, {80,280,96,376}, RADIO, N_("Use Pattern")},
 /*  8 */ {0, {0,8,32,40}, ICON, (CHAR *)us_icon300},
 /*  9 */ {0, {0,40,32,72}, ICON, (CHAR *)us_icon301},
 /* 10 */ {0, {0,72,32,104}, ICON, (CHAR *)us_icon302},
 /* 11 */ {0, {0,104,32,136}, ICON, (CHAR *)us_icon303},
 /* 12 */ {0, {0,136,32,168}, ICON, (CHAR *)us_icon304},
 /* 13 */ {0, {0,168,32,200}, ICON, (CHAR *)us_icon305},
 /* 14 */ {0, {0,200,32,232}, ICON, (CHAR *)us_icon306},
 /* 15 */ {0, {0,232,32,264}, ICON, (CHAR *)us_icon307},
 /* 16 */ {0, {0,264,32,296}, ICON, (CHAR *)us_icon308},
 /* 17 */ {0, {0,296,32,328}, ICON, (CHAR *)us_icon309},
 /* 18 */ {0, {0,328,32,360}, ICON, (CHAR *)us_icon310},
 /* 19 */ {0, {168,280,184,420}, POPUP, x_("")}
};
static DIALOG us_artworkdialog = {{50,75,349,505}, N_("Set Look of Highlighted"), 0, 19, us_artworkdialogitems, 0, 0};

/* Attribute Report */
static DIALOGITEM us_report_attrdialogitems[] =
{
 /*  1 */ {0, {12,8,28,112}, MESSAGE, N_("Attribute name:")},
 /*  2 */ {0, {12,116,28,224}, EDITTEXT, x_("")},
 /*  3 */ {0, {60,92,84,224}, DEFBUTTON, N_("Generate Report")},
 /*  4 */ {0, {36,32,52,112}, CHECK, N_("To file:")},
 /*  5 */ {0, {36,116,52,224}, EDITTEXT, x_("")},
 /*  6 */ {0, {60,8,84,76}, BUTTON, N_("Cancel")}
};
static DIALOG us_report_attrdialog = {{75,75,168,309}, N_("Attribute Report"), 0, 6, us_report_attrdialogitems, 0, 0};

/* Attributes: Define */
static DIALOGITEM us_attrdialogitems[] =
{
 /*  1 */ {0, {432,28,456,108}, BUTTON, N_("Done")},
 /*  2 */ {0, {148,216,164,332}, MESSAGE, N_("Attribute name:")},
 /*  3 */ {0, {8,8,24,300}, RADIO, N_("Cell")},
 /*  4 */ {0, {32,8,48,300}, RADIO, N_("Node")},
 /*  5 */ {0, {56,8,72,300}, RADIO, N_("Cell port")},
 /*  6 */ {0, {80,8,96,300}, RADIO, N_("Node port")},
 /*  7 */ {0, {104,8,120,300}, RADIO, N_("Arc")},
 /*  8 */ {0, {148,8,300,208}, SCROLL, x_("")},
 /*  9 */ {0, {148,336,164,504}, EDITTEXT, x_("")},
 /* 10 */ {0, {172,276,220,576}, EDITTEXT, x_("")},
 /* 11 */ {0, {320,216,336,364}, CHECK, N_("Instances inherit")},
 /* 12 */ {0, {172,216,188,272}, MESSAGE, N_("Value")},
 /* 13 */ {0, {112,492,128,576}, BUTTON, N_("Make Array")},
 /* 14 */ {0, {380,8,396,132}, BUTTON, N_("Delete Attribute")},
 /* 15 */ {0, {308,8,324,132}, BUTTON, N_("Create Attribute")},
 /* 16 */ {0, {356,8,372,132}, BUTTON, N_("Rename Attribute")},
 /* 17 */ {0, {332,8,348,132}, BUTTON, N_("Change Value")},
 /* 18 */ {0, {272,216,288,296}, MESSAGE, N_("X offset:")},
 /* 19 */ {0, {296,216,312,296}, MESSAGE, N_("Y offset:")},
 /* 20 */ {0, {272,420,304,452}, ICON, (CHAR *)us_icon200},
 /* 21 */ {0, {304,420,336,452}, ICON, (CHAR *)us_icon201},
 /* 22 */ {0, {336,420,368,452}, ICON, (CHAR *)us_icon202},
 /* 23 */ {0, {368,420,400,452}, ICON, (CHAR *)us_icon203},
 /* 24 */ {0, {400,420,432,452}, ICON, (CHAR *)us_icon205},
 /* 25 */ {0, {272,456,288,568}, RADIO, N_("Center")},
 /* 26 */ {0, {288,456,304,568}, RADIO, N_("Bottom")},
 /* 27 */ {0, {304,456,320,568}, RADIO, N_("Top")},
 /* 28 */ {0, {320,456,336,568}, RADIO, N_("Right")},
 /* 29 */ {0, {336,456,352,568}, RADIO, N_("Left")},
 /* 30 */ {0, {352,456,368,568}, RADIO, N_("Lower right")},
 /* 31 */ {0, {368,456,384,568}, RADIO, N_("Lower left")},
 /* 32 */ {0, {384,456,400,568}, RADIO, N_("Upper right")},
 /* 33 */ {0, {400,456,416,568}, RADIO, N_("Upper left")},
 /* 34 */ {0, {8,304,128,484}, SCROLL, x_("")},
 /* 35 */ {0, {440,236,456,404}, POPUP, x_("")},
 /* 36 */ {0, {12,492,28,564}, MESSAGE, N_("Array:")},
 /* 37 */ {0, {32,492,48,576}, BUTTON, N_("Add")},
 /* 38 */ {0, {52,492,68,576}, BUTTON, N_("Remove")},
 /* 39 */ {0, {72,492,88,576}, BUTTON, N_("Add All")},
 /* 40 */ {0, {92,492,108,576}, BUTTON, N_("Remove All")},
 /* 41 */ {0, {248,216,264,272}, MESSAGE, N_("Show:")},
 /* 42 */ {0, {248,276,264,532}, POPUP, x_("")},
 /* 43 */ {0, {224,324,240,576}, MESSAGE, x_("")},
 /* 44 */ {0, {224,216,240,324}, POPUP, x_("")},
 /* 45 */ {0, {136,8,137,576}, DIVIDELINE, x_("")},
 /* 46 */ {0, {272,300,288,380}, EDITTEXT, x_("")},
 /* 47 */ {0, {296,300,312,380}, EDITTEXT, x_("")},
 /* 48 */ {0, {368,248,384,408}, RADIO, N_("Points (max 63)")},
 /* 49 */ {0, {368,196,384,240}, EDITTEXT, x_("")},
 /* 50 */ {0, {392,248,408,408}, RADIO, N_("Lambda (max 127.75)")},
 /* 51 */ {0, {392,196,408,240}, EDITTEXT, x_("")},
 /* 52 */ {0, {416,144,432,228}, MESSAGE, N_("Text font:")},
 /* 53 */ {0, {416,236,432,404}, POPUP, x_("")},
 /* 54 */ {0, {424,456,440,528}, CHECK, N_("Italic")},
 /* 55 */ {0, {444,456,460,520}, CHECK, N_("Bold")},
 /* 56 */ {0, {464,456,480,532}, CHECK, N_("Underline")},
 /* 57 */ {0, {380,140,396,188}, MESSAGE, N_("Size:")},
 /* 58 */ {0, {440,144,456,228}, MESSAGE, N_("Rotation:")},
 /* 59 */ {0, {344,216,360,364}, CHECK, N_("Is Parameter")},
 /* 60 */ {0, {464,236,480,404}, POPUP, x_("")},
 /* 61 */ {0, {464,144,480,228}, MESSAGE, N_("Units:")}
};
static DIALOG us_attrdialog = {{75,75,564,660}, N_("Attributes"), 0, 61, us_attrdialogitems, 0, 0};

/* Attributes: Enumerate */
static DIALOGITEM us_manattrdialogitems[] =
{
 /*  1 */ {0, {72,224,96,304}, BUTTON, N_("OK")},
 /*  2 */ {0, {72,12,96,92}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {12,4,28,192}, MESSAGE, N_("Attributes name to enumerate:")},
 /*  4 */ {0, {12,196,28,316}, EDITTEXT, x_("")},
 /*  5 */ {0, {40,60,56,248}, BUTTON, N_("Check for this attribute")}
};
static DIALOG us_manattrdialog = {{75,75,180,400}, N_("Enumerate Attributes"), 0, 5, us_manattrdialogitems, 0, 0};

/* CDL Options */
static DIALOGITEM io_cdloptdialogitems[] =
{
 /*  1 */ {0, {100,236,124,316}, BUTTON, N_("OK")},
 /*  2 */ {0, {100,64,124,144}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,184}, MESSAGE, N_("Cadence Library Name:")},
 /*  4 */ {0, {8,187,24,363}, EDITTEXT, x_("")},
 /*  5 */ {0, {32,8,48,184}, MESSAGE, N_("Cadence Library Path:")},
 /*  6 */ {0, {32,187,64,363}, EDITTEXT, x_("")},
 /*  7 */ {0, {72,8,88,176}, CHECK, N_("Convert brackets")}
};
static DIALOG io_cdloptdialog = {{75,75,208,447}, N_("CDL Options"), 0, 7, io_cdloptdialogitems, 0, 0};

/* CIF Options */
static DIALOGITEM io_cifoptionsdialogitems[] =
{
 /*  1 */ {0, {224,380,248,452}, BUTTON, N_("OK")},
 /*  2 */ {0, {224,240,248,312}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,248,223}, SCROLL, x_("")},
 /*  4 */ {0, {8,232,24,312}, MESSAGE, N_("CIF Layer:")},
 /*  5 */ {0, {8,316,24,454}, EDITTEXT, x_("")},
 /*  6 */ {0, {32,232,48,454}, CHECK, N_("Output Mimics Display")},
 /*  7 */ {0, {56,232,72,454}, CHECK, N_("Output Merges Boxes")},
 /*  8 */ {0, {148,232,164,454}, CHECK, N_("Input Squares Wires")},
 /*  9 */ {0, {100,232,116,454}, CHECK, N_("Output Instantiates Top Level")},
 /* 10 */ {0, {196,240,212,384}, MESSAGE, N_("Output resolution:")},
 /* 11 */ {0, {196,388,212,454}, EDITTEXT, x_("")},
 /* 12 */ {0, {172,232,188,436}, POPUP, x_("")},
 /* 13 */ {0, {124,232,140,454}, CHECK, N_("Normalize Coordinates")},
 /* 14 */ {0, {76,248,92,454}, MESSAGE, N_("(time consuming)")}
};
static DIALOG io_cifoptionsdialog = {{50,75,307,538}, N_("CIF Options"), 0, 14, io_cifoptionsdialogitems, 0, 0};

/* Cell Lists */
static DIALOGITEM us_faclisdialogitems[] =
{
 /*  1 */ {0, {464,152,488,232}, BUTTON, N_("OK")},
 /*  2 */ {0, {464,32,488,112}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {160,22,176,198}, CHECK, N_("Show only this view:")},
 /*  4 */ {0, {180,38,196,214}, POPUP, x_("")},
 /*  5 */ {0, {200,22,216,198}, CHECK, N_("Also include icon views")},
 /*  6 */ {0, {252,22,268,198}, CHECK, N_("Exclude older versions")},
 /*  7 */ {0, {140,10,156,186}, MESSAGE, N_("View filter:")},
 /*  8 */ {0, {232,10,248,186}, MESSAGE, N_("Version filter:")},
 /*  9 */ {0, {272,22,288,198}, CHECK, N_("Exclude newest versions")},
 /* 10 */ {0, {8,8,24,184}, MESSAGE, N_("Which cells:")},
 /* 11 */ {0, {88,20,104,248}, RADIO, N_("Only those under current cell")},
 /* 12 */ {0, {304,8,320,184}, MESSAGE, N_("Display ordering:")},
 /* 13 */ {0, {324,20,340,248}, RADIO, N_("Order by name")},
 /* 14 */ {0, {344,20,360,248}, RADIO, N_("Order by modification date")},
 /* 15 */ {0, {364,20,380,248}, RADIO, N_("Order by skeletal structure")},
 /* 16 */ {0, {396,8,412,184}, MESSAGE, N_("Destination:")},
 /* 17 */ {0, {416,16,432,244}, RADIO, N_("Display in messages window")},
 /* 18 */ {0, {436,16,452,244}, RADIO, N_("Save to disk")},
 /* 19 */ {0, {48,20,64,248}, RADIO, N_("Only those used elsewhere")},
 /* 20 */ {0, {108,20,124,248}, RADIO, N_("Only placeholder cells")},
 /* 21 */ {0, {28,20,44,248}, RADIO, N_("All cells")},
 /* 22 */ {0, {132,8,133,248}, DIVIDELINE, x_("")},
 /* 23 */ {0, {224,8,225,248}, DIVIDELINE, x_("")},
 /* 24 */ {0, {296,8,297,248}, DIVIDELINE, x_("")},
 /* 25 */ {0, {388,8,389,248}, DIVIDELINE, x_("")},
 /* 26 */ {0, {68,20,84,248}, RADIO, N_("Only those not used elsewhere")}
};
static DIALOG us_faclisdialog = {{75,75,572,333}, N_("Cell Lists"), 0, 26, us_faclisdialogitems, 0, 0};

/* Cell Options */
static DIALOGITEM us_celldialogitems[] =
{
 /*  1 */ {0, {288,512,312,576}, BUTTON, N_("OK")},
 /*  2 */ {0, {232,512,256,576}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {304,24,320,324}, CHECK, N_("Tiny cell instances hashed out")},
 /*  4 */ {0, {8,8,24,90}, MESSAGE, N_("Library:")},
 /*  5 */ {0, {28,4,188,150}, SCROLL, x_("")},
 /*  6 */ {0, {8,92,24,256}, POPUP, x_("")},
 /*  7 */ {0, {196,4,197,584}, DIVIDELINE, x_("")},
 /*  8 */ {0, {208,4,224,116}, MESSAGE, N_("For all cells:")},
 /*  9 */ {0, {28,156,44,496}, CHECK, N_("Disallow modification of anything in this cell")},
 /* 10 */ {0, {4,500,20,584}, MESSAGE, N_("Every cell:")},
 /* 11 */ {0, {52,156,68,496}, CHECK, N_("Disallow modification of instances in this cell")},
 /* 12 */ {0, {148,156,164,344}, MESSAGE|INACTIVE, N_("Characteristic X Spacing:")},
 /* 13 */ {0, {172,156,188,344}, MESSAGE|INACTIVE, N_("Characteristic Y Spacing:")},
 /* 14 */ {0, {148,348,164,424}, EDITTEXT, x_("")},
 /* 15 */ {0, {172,348,188,424}, EDITTEXT, x_("")},
 /* 16 */ {0, {76,156,92,496}, CHECK, N_("Part of a cell-library")},
 /* 17 */ {0, {76,500,92,536}, BUTTON, N_("Set")},
 /* 18 */ {0, {76,544,92,584}, BUTTON, N_("Clear")},
 /* 19 */ {0, {328,24,344,280}, MESSAGE|INACTIVE, N_("Hash cells when scale is more than:")},
 /* 20 */ {0, {328,284,344,344}, EDITTEXT, x_("")},
 /* 21 */ {0, {328,348,344,472}, MESSAGE|INACTIVE, N_("lambda per pixel")},
 /* 22 */ {0, {124,156,140,344}, RADIO, N_("Expand new instances")},
 /* 23 */ {0, {124,360,140,548}, RADIO, N_("Unexpand new instances")},
 /* 24 */ {0, {232,24,248,324}, CHECK, N_("Check cell dates during creation")},
 /* 25 */ {0, {28,544,44,584}, BUTTON, N_("Clear")},
 /* 26 */ {0, {28,500,44,536}, BUTTON, N_("Set")},
 /* 27 */ {0, {52,544,68,584}, BUTTON, N_("Clear")},
 /* 28 */ {0, {52,500,68,536}, BUTTON, N_("Set")},
 /* 29 */ {0, {256,24,272,324}, CHECK, N_("Switch technology to match current cell")},
 /* 30 */ {0, {352,24,368,280}, MESSAGE|INACTIVE, N_("Cell explorer text size:")},
 /* 31 */ {0, {352,284,368,344}, EDITTEXT, x_("")},
 /* 32 */ {0, {280,24,296,324}, CHECK, N_("Place Cell-Center in new cells")},
 /* 33 */ {0, {100,156,116,496}, CHECK, N_("Use technology editor on this cell")},
 /* 34 */ {0, {100,500,116,536}, BUTTON, N_("Set")},
 /* 35 */ {0, {100,544,116,584}, BUTTON, N_("Clear")}
};
static DIALOG us_celldialog = {{50,75,427,669}, N_("Cell Options"), 0, 35, us_celldialogitems, 0, 0};

/* Cell Parameters */
static DIALOGITEM us_paramdialogitems[] =
{
 /*  1 */ {0, {192,424,216,504}, BUTTON, N_("Done")},
 /*  2 */ {0, {36,216,52,332}, MESSAGE, N_("New Parameter:")},
 /*  3 */ {0, {28,8,216,208}, SCROLL, x_("")},
 /*  4 */ {0, {36,336,52,504}, EDITTEXT, x_("")},
 /*  5 */ {0, {60,336,92,504}, EDITTEXT, x_("")},
 /*  6 */ {0, {60,216,76,332}, MESSAGE, N_("Default Value:")},
 /*  7 */ {0, {152,216,176,352}, BUTTON, N_("Create Parameter")},
 /*  8 */ {0, {8,8,24,504}, MESSAGE, N_("Parameters on cell %s:")},
 /*  9 */ {0, {152,368,176,504}, BUTTON, N_("Delete Parameter")},
 /* 10 */ {0, {124,216,140,332}, MESSAGE, N_("Units:")},
 /* 11 */ {0, {124,336,140,504}, POPUP, x_("")},
 /* 12 */ {0, {100,216,116,332}, MESSAGE, N_("Language:")},
 /* 13 */ {0, {100,336,116,504}, POPUP, x_("")}
};
static DIALOG us_paramdialog = {{75,75,300,589}, N_("Cell Parameters"), 0, 13, us_paramdialogitems, 0, 0};

/* Cell Selection */
static DIALOGITEM us_cellselectdialogitems[] =
{
 /*  1 */ {0, {336,208,360,272}, BUTTON, N_("OK")},
 /*  2 */ {0, {336,16,360,80}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {36,8,251,280}, SCROLL, x_("")},
 /*  4 */ {0, {284,8,302,153}, CHECK, N_("Show old versions")},
 /*  5 */ {0, {308,8,326,253}, CHECK, N_("Show cells from Cell-Library")},
 /*  6 */ {0, {8,8,26,67}, MESSAGE, N_("Library:")},
 /*  7 */ {0, {8,72,26,280}, POPUP, x_("")},
 /*  8 */ {0, {260,8,278,205}, CHECK, N_("Show relevant cells only")}
};
static DIALOG us_cellselectdialog = {{50,75,419,365}, N_("Cell List"), 0, 8, us_cellselectdialogitems, 0, 0};

/* Change */
static DIALOGITEM us_changedialogitems[] =
{
 /*  1 */ {0, {264,344,288,416}, BUTTON, N_("OK")},
 /*  2 */ {0, {264,248,288,320}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,232,24,438}, RADIO, N_("Change selected ones only")},
 /*  4 */ {0, {56,232,72,438}, RADIO, N_("Change all in this cell")},
 /*  5 */ {0, {104,232,120,438}, RADIO, N_("Change all in all libraries")},
 /*  6 */ {0, {32,232,48,438}, RADIO, N_("Change all connected to this")},
 /*  7 */ {0, {8,8,264,223}, SCROLL, x_("")},
 /*  8 */ {0, {272,8,288,78}, MESSAGE, N_("Library:")},
 /*  9 */ {0, {272,80,288,218}, POPUP, x_("")},
 /* 10 */ {0, {212,232,228,438}, CHECK, N_("Ignore port names")},
 /* 11 */ {0, {236,232,252,438}, CHECK, N_("Allow missing ports")},
 /* 12 */ {0, {140,232,156,438}, CHECK, N_("Change nodes with arcs")},
 /* 13 */ {0, {164,232,180,438}, CHECK, N_("Show primitives")},
 /* 14 */ {0, {188,232,204,438}, CHECK, N_("Show cells")},
 /* 15 */ {0, {80,232,96,438}, RADIO, N_("Change all in this library")}
};
static DIALOG us_changedialog = {{50,75,347,523}, N_("Change Nodes and Arcs"), 0, 15, us_changedialogitems, 0, 0};

/* Change Cell's View */
static DIALOGITEM us_viewseldialogitems[] =
{
 /*  1 */ {0, {176,108,200,188}, BUTTON, N_("OK")},
 /*  2 */ {0, {176,8,200,88}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,168,188}, SCROLL, x_("")}
};
static DIALOG us_viewseldialog = {{75,75,284,272}, N_("Select View"), 0, 3, us_viewseldialogitems, 0, 0};

/* Change Current Library */
static DIALOGITEM us_chglibrarydialogitems[] =
{
 /*  1 */ {0, {164,220,188,300}, BUTTON, N_("OK")},
 /*  2 */ {0, {116,220,140,300}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {4,4,20,128}, MESSAGE, N_("Current Library:")},
 /*  4 */ {0, {4,132,20,316}, MESSAGE, x_("")},
 /*  5 */ {0, {52,4,196,196}, SCROLL, x_("")},
 /*  6 */ {0, {32,16,48,168}, MESSAGE|INACTIVE, N_("Switch to Library:")}
};
static DIALOG us_chglibrarydialog = {{75,75,280,401}, N_("Set Current Library"), 0, 6, us_chglibrarydialogitems, 0, 0};

/* Change Current Technology */
static DIALOGITEM us_techselectdialogitems[] =
{
 /*  1 */ {0, {96,216,120,280}, BUTTON, N_("OK")},
 /*  2 */ {0, {24,216,48,280}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,153,193}, SCROLL, x_("")},
 /*  4 */ {0, {160,8,208,292}, MESSAGE, x_("")}
};
static DIALOG us_techselectdialog = {{50,75,267,376}, N_("Change Current Technology"), 0, 4, us_techselectdialogitems, 0, 0};

/* Change Text Size */
static DIALOGITEM us_txtmodsizedialogitems[] =
{
 /*  1 */ {0, {200,376,224,456}, BUTTON, N_("OK")},
 /*  2 */ {0, {160,376,184,456}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {156,180,172,340}, RADIO, N_("Points (max 63)")},
 /*  4 */ {0, {156,124,172,172}, EDITTEXT, x_("")},
 /*  5 */ {0, {8,8,24,256}, CHECK, N_("Change size of node text")},
 /*  6 */ {0, {28,8,44,260}, CHECK, N_("Change size of arc text")},
 /*  7 */ {0, {48,8,64,260}, CHECK, N_("Change size of export text")},
 /*  8 */ {0, {88,8,104,260}, CHECK, N_("Change size of instance name text")},
 /*  9 */ {0, {68,8,84,260}, CHECK, N_("Change size of nonlayout text")},
 /* 10 */ {0, {16,264,32,476}, RADIO, N_("Change only selected objects")},
 /* 11 */ {0, {36,264,52,476}, RADIO, N_("Change all in this cell")},
 /* 12 */ {0, {96,264,112,476}, RADIO, N_("Change all in this library")},
 /* 13 */ {0, {180,180,196,340}, RADIO, N_("Lambda (max 127.75)")},
 /* 14 */ {0, {180,124,196,172}, EDITTEXT, x_("")},
 /* 15 */ {0, {204,68,220,152}, MESSAGE, N_("Text font:")},
 /* 16 */ {0, {204,156,220,336}, POPUP, x_("")},
 /* 17 */ {0, {228,68,244,140}, CHECK, N_("Italic")},
 /* 18 */ {0, {228,168,244,232}, CHECK, N_("Bold")},
 /* 19 */ {0, {228,256,244,336}, CHECK, N_("Underline")},
 /* 20 */ {0, {168,68,184,116}, MESSAGE, N_("Size")},
 /* 21 */ {0, {108,8,124,260}, CHECK, N_("Change size of cell text")},
 /* 22 */ {0, {56,264,72,476}, RADIO, N_("Change all cells with view:")},
 /* 23 */ {0, {76,304,92,476}, POPUP, x_("")},
 /* 24 */ {0, {132,8,148,476}, MESSAGE, x_("")}
};
static DIALOG us_txtmodsizedialog = {{75,75,328,561}, N_("Change Text Size"), 0, 24, us_txtmodsizedialogitems, 0, 0};

/* Change Units */
static DIALOGITEM us_unitsdialogitems[] =
{
 /*  1 */ {0, {292,509,316,581}, BUTTON, N_("OK")},
 /*  2 */ {0, {256,509,280,581}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {196,512,212,588}, EDITTEXT, x_("")},
 /*  4 */ {0, {196,308,212,508}, MESSAGE, N_("Lambda size (internal units):")},
 /*  5 */ {0, {32,132,48,292}, POPUP, x_("")},
 /*  6 */ {0, {8,8,24,124}, MESSAGE, N_("Display Units:")},
 /*  7 */ {0, {238,132,254,292}, POPUP, x_("")},
 /*  8 */ {0, {214,8,230,126}, MESSAGE, N_("Internal Units:")},
 /*  9 */ {0, {52,308,188,588}, SCROLL, x_("")},
 /* 10 */ {0, {32,380,48,500}, MESSAGE, N_("Technologies:")},
 /* 11 */ {0, {8,452,24,588}, MESSAGE, x_("")},
 /* 12 */ {0, {8,308,24,452}, MESSAGE, N_("Current library:")},
 /* 13 */ {0, {308,324,324,476}, RADIO, N_("Change all libraries")},
 /* 14 */ {0, {284,324,300,476}, RADIO, N_("Change current library")},
 /* 15 */ {0, {8,300,324,301}, DIVIDELINE, x_("")},
 /* 16 */ {0, {220,512,236,588}, MESSAGE, x_("")},
 /* 17 */ {0, {260,324,276,476}, RADIO, N_("Change no libraries")},
 /* 18 */ {0, {236,308,252,476}, MESSAGE, N_("When changing lambda:")},
 /* 19 */ {0, {32,16,48,132}, MESSAGE, N_("Distance:")},
 /* 20 */ {0, {56,132,72,292}, POPUP, x_("")},
 /* 21 */ {0, {238,16,254,132}, MESSAGE, N_("Distance:")},
 /* 22 */ {0, {56,16,72,132}, MESSAGE, N_("Resistance:")},
 /* 23 */ {0, {80,132,96,292}, POPUP, x_("")},
 /* 24 */ {0, {152,16,168,132}, MESSAGE, N_("Voltage:")},
 /* 25 */ {0, {80,16,96,132}, MESSAGE, N_("Capacitance:")},
 /* 26 */ {0, {104,132,120,292}, POPUP, x_("")},
 /* 27 */ {0, {176,16,192,132}, MESSAGE, N_("Time:")},
 /* 28 */ {0, {104,16,120,132}, MESSAGE, N_("Inductance:")},
 /* 29 */ {0, {128,132,144,292}, POPUP, x_("")},
 /* 30 */ {0, {176,132,192,292}, POPUP, x_("")},
 /* 31 */ {0, {128,16,144,132}, MESSAGE, N_("Current:")},
 /* 32 */ {0, {152,132,168,292}, POPUP, x_("")},
 /* 33 */ {0, {258,16,274,294}, MESSAGE, N_("(read manual before changing this)")}
};
static DIALOG us_unitsdialog = {{50,75,383,672}, N_("Change Units"), 0, 33, us_unitsdialogitems, 0, 0};

/* Compaction Options */
static DIALOGITEM com_optionsdialogitems[] =
{
 /*  1 */ {0, {64,92,88,156}, BUTTON, N_("OK")},
 /*  2 */ {0, {64,8,88,72}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,154}, CHECK, N_("Allow spreading")},
 /*  4 */ {0, {32,8,48,154}, CHECK, N_("Verbose")}
};
static DIALOG com_optionsdialog = {{50,75,147,243}, N_("Compaction Options"), 0, 4, com_optionsdialogitems, 0, 0};

/* Compensation Options */
static DIALOGITEM compen_factorsdialogitems[] =
{
 /*  1 */ {0, {108,176,132,240}, BUTTON, N_("OK")},
 /*  2 */ {0, {108,16,132,80}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,168,24,258}, EDITTEXT, x_("")},
 /*  4 */ {0, {8,8,24,133}, MESSAGE, N_("Metal Thickness:")},
 /*  5 */ {0, {32,168,48,258}, EDITTEXT, x_("")},
 /*  6 */ {0, {32,8,48,145}, MESSAGE, N_("LRS Compensation:")},
 /*  7 */ {0, {56,168,72,258}, EDITTEXT, x_("")},
 /*  8 */ {0, {56,8,72,160}, MESSAGE, N_("Global Compensation:")},
 /*  9 */ {0, {80,168,96,258}, EDITTEXT, x_("")},
 /* 10 */ {0, {80,8,96,165}, MESSAGE, N_("Minimum feature size:")}
};
static DIALOG compen_factorsdialog = {{50,75,197,343}, N_("Compensation Factors"), 0, 10, compen_factorsdialogitems, 0, 0};

/* Component Menu */
static DIALOGITEM us_menuposdialogitems[] =
{
 /*  1 */ {0, {128,168,152,232}, BUTTON, N_("OK")},
 /*  2 */ {0, {88,168,112,232}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,200,48,248}, EDITTEXT, x_("")},
 /*  4 */ {0, {64,8,80,136}, RADIO, N_("Menu at Top")},
 /*  5 */ {0, {88,8,104,136}, RADIO, N_("Menu at Bottom")},
 /*  6 */ {0, {112,8,128,136}, RADIO, N_("Menu on Left")},
 /*  7 */ {0, {136,8,152,136}, RADIO, N_("Menu on Right")},
 /*  8 */ {0, {8,8,24,197}, MESSAGE, N_("Number of Entries Across:")},
 /*  9 */ {0, {32,8,48,197}, MESSAGE, N_("Number of Entries Down:")},
 /* 10 */ {0, {8,200,24,248}, EDITTEXT, x_("")},
 /* 11 */ {0, {160,8,176,100}, RADIO, N_("No Menu")}
};
static DIALOG us_menuposdialog = {{50,75,235,334}, N_("Component Menu Configuration"), 0, 11, us_menuposdialogitems, 0, 0};

/* Convert Library to Technology */
static DIALOGITEM us_tecedlibtotechdialogitems[] =
{
 /*  1 */ {0, {96,284,120,364}, BUTTON, N_("OK")},
 /*  2 */ {0, {96,16,120,96}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {88,120,104,252}, CHECK, N_("Also write C code")},
 /*  4 */ {0, {8,8,24,224}, MESSAGE, N_("Creating new technology:")},
 /*  5 */ {0, {8,228,24,372}, EDITTEXT, x_("")},
 /*  6 */ {0, {40,8,56,372}, MESSAGE, N_("Already a technology with this name")},
 /*  7 */ {0, {64,8,80,224}, MESSAGE, N_("Rename existing technology to:")},
 /*  8 */ {0, {64,228,80,372}, EDITTEXT, x_("")},
 /*  9 */ {0, {112,120,128,272}, CHECK, N_("Also write Java code")}
};
static DIALOG us_tecedlibtotechdialog = {{75,75,212,457}, N_("Convert Library to Technology"), 0, 9, us_tecedlibtotechdialogitems, 0, 0};

/* Copyright Options */
static DIALOGITEM us_crodialogitems[] =
{
 /*  1 */ {0, {168,272,192,352}, BUTTON, N_("OK")},
 /*  2 */ {0, {165,76,189,156}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {4,4,20,432}, MESSAGE, N_("Copyright information can be added to every generated deck.")},
 /*  4 */ {0, {72,4,88,268}, RADIO, N_("Use copyright message from file:")},
 /*  5 */ {0, {96,28,128,292}, EDITTEXT, x_("")},
 /*  6 */ {0, {100,300,124,372}, BUTTON, N_("Browse")},
 /*  7 */ {0, {136,28,152,432}, MESSAGE, N_("Do not put comment characters in this file.")},
 /*  8 */ {0, {48,4,64,268}, RADIO, N_("No copyright message")}
};
static DIALOG us_crodialog = {{75,75,276,517}, N_("Copyright Options"), 0, 8, us_crodialogitems, 0, 0};

/* Create Export */
static DIALOGITEM us_portdialogitems[] =
{
 /*  1 */ {0, {108,256,132,328}, BUTTON, N_("OK")},
 /*  2 */ {0, {108,32,132,104}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,116,24,340}, EDITTEXT, x_("")},
 /*  4 */ {0, {32,176,48,340}, POPUP, x_("")},
 /*  5 */ {0, {8,8,24,112}, MESSAGE, N_("Export name:")},
 /*  6 */ {0, {56,8,72,128}, CHECK, N_("Always drawn")},
 /*  7 */ {0, {32,8,48,175}, MESSAGE, N_("Export characteristics:")},
 /*  8 */ {0, {80,8,96,128}, CHECK, N_("Body only")},
 /*  9 */ {0, {56,176,72,340}, MESSAGE, N_("Reference export:")},
 /* 10 */ {0, {80,176,96,340}, EDITTEXT, x_("")}
};
static DIALOG us_portdialog = {{50,75,191,425}, N_("Create Export on Highlighted Node"), 0, 10, us_portdialogitems, 0, 0};

/* Cross-Library Copy */
static DIALOGITEM us_copycelldialogitems[] =
{
 /*  1 */ {0, {276,172,300,244}, BUTTON, N_("Done")},
 /*  2 */ {0, {8,8,24,168}, POPUP, x_("")},
 /*  3 */ {0, {276,40,300,112}, BUTTON, N_("<< Copy")},
 /*  4 */ {0, {276,296,300,368}, BUTTON, N_("Copy >>")},
 /*  5 */ {0, {32,8,264,172}, SCROLL, x_("")},
 /*  6 */ {0, {8,244,24,408}, POPUP, x_("")},
 /*  7 */ {0, {324,12,340,192}, BUTTON, N_("Examine contents")},
 /*  8 */ {0, {408,8,424,408}, MESSAGE, x_("")},
 /*  9 */ {0, {348,12,364,192}, BUTTON, N_("Examine contents quietly")},
 /* 10 */ {0, {312,248,328,408}, CHECK, N_("Delete after copy")},
 /* 11 */ {0, {384,248,400,408}, CHECK, N_("Copy related views")},
 /* 12 */ {0, {336,248,352,408}, CHECK, N_("Copy subcells")},
 /* 13 */ {0, {32,244,264,408}, SCROLL, x_("")},
 /* 14 */ {0, {32,172,264,244}, SCROLL, x_("")},
 /* 15 */ {0, {372,12,388,192}, BUTTON, N_("List differences")},
 /* 16 */ {0, {360,248,376,408}, CHECK, N_("Use existing subcells")}
};
static DIALOG us_copycelldialog = {{50,75,483,493}, N_("Cross-Library Copy"), 0, 16, us_copycelldialogitems, 0, 0};

/* DEF Options */
static DIALOGITEM io_defoptionsdialogitems[] =
{
 /*  1 */ {0, {64,156,88,228}, BUTTON, N_("OK")},
 /*  2 */ {0, {64,16,88,88}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,8,48,230}, CHECK, N_("Place logical interconnect")},
 /*  4 */ {0, {8,8,24,230}, CHECK, N_("Place physical interconnect")}
};
static DIALOG io_defoptionsdialog = {{50,75,147,314}, N_("DEF Options"), 0, 4, io_defoptionsdialogitems, 0, 0};

/* DRC Options */
static DIALOGITEM dr_optionsdialogitems[] =
{
 /*  1 */ {0, {304,184,328,248}, BUTTON, N_("OK")},
 /*  2 */ {0, {304,44,328,108}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {144,8,160,224}, MESSAGE, N_("Incremental and hierarchical:")},
 /*  4 */ {0, {272,32,288,160}, BUTTON, N_("Edit Rules Deck")},
 /*  5 */ {0, {32,32,48,117}, CHECK, N_("On")},
 /*  6 */ {0, {192,216,208,272}, EDITTEXT, x_("")},
 /*  7 */ {0, {8,8,24,169}, MESSAGE, N_("Incremental DRC:")},
 /*  8 */ {0, {64,8,80,169}, MESSAGE, N_("Hierarchical DRC:")},
 /*  9 */ {0, {248,8,264,169}, MESSAGE, N_("Dracula DRC Interface:")},
 /* 10 */ {0, {112,32,128,192}, BUTTON, N_("Clear valid DRC dates")},
 /* 11 */ {0, {88,32,104,228}, CHECK, N_("Just 1 error per cell")},
 /* 12 */ {0, {216,32,232,296}, CHECK, N_("Ignore center cuts in large contacts")},
 /* 13 */ {0, {192,48,208,212}, MESSAGE, N_("Number of processors:")},
 /* 14 */ {0, {168,32,184,217}, CHECK, N_("Use multiple processors")},
 /* 15 */ {0, {56,8,57,296}, DIVIDELINE, x_("")},
 /* 16 */ {0, {136,8,137,296}, DIVIDELINE, x_("")},
 /* 17 */ {0, {240,8,241,296}, DIVIDELINE, x_("")},
 /* 18 */ {0, {296,8,297,296}, DIVIDELINE, x_("")}
};
static DIALOG dr_optionsdialog = {{50,75,387,381}, N_("DRC Options"), 0, 18, dr_optionsdialogitems, 0, 0};

/* DRC Rules */
static DIALOGITEM dr_rulesdialogitems[] =
{
 /*  1 */ {0, {8,516,32,580}, BUTTON, N_("OK")},
 /*  2 */ {0, {40,516,64,580}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,100,188,278}, SCROLL, x_("")},
 /*  4 */ {0, {32,8,48,95}, RADIO, N_("Layers:")},
 /*  5 */ {0, {212,8,435,300}, SCROLL, x_("")},
 /*  6 */ {0, {8,100,24,294}, MESSAGE|INACTIVE, x_("")},
 /*  7 */ {0, {8,8,24,95}, MESSAGE, N_("Technology:")},
 /*  8 */ {0, {192,8,208,88}, MESSAGE, N_("To Layer:")},
 /*  9 */ {0, {88,452,104,523}, MESSAGE, N_("Size")},
 /* 10 */ {0, {88,528,104,583}, MESSAGE, N_("Rule")},
 /* 11 */ {0, {112,308,128,424}, MESSAGE, N_("Minimum Width:")},
 /* 12 */ {0, {112,454,128,502}, EDITTEXT, x_("")},
 /* 13 */ {0, {112,514,128,596}, EDITTEXT, x_("")},
 /* 14 */ {0, {180,308,196,387}, MESSAGE, N_("Normal:")},
 /* 15 */ {0, {204,324,220,450}, MESSAGE, N_("When connected:")},
 /* 16 */ {0, {204,454,220,502}, EDITTEXT, x_("")},
 /* 17 */ {0, {204,514,220,595}, EDITTEXT, x_("")},
 /* 18 */ {0, {232,324,248,450}, MESSAGE, N_("Not connected:")},
 /* 19 */ {0, {232,454,248,502}, EDITTEXT, x_("")},
 /* 20 */ {0, {232,514,248,595}, EDITTEXT, x_("")},
 /* 21 */ {0, {288,308,304,520}, MESSAGE, N_("Wide (when bigger than this):")},
 /* 22 */ {0, {312,324,328,450}, MESSAGE, N_("When connected:")},
 /* 23 */ {0, {312,454,328,502}, EDITTEXT, x_("")},
 /* 24 */ {0, {312,514,328,595}, EDITTEXT, x_("")},
 /* 25 */ {0, {340,324,356,450}, MESSAGE, N_("Not connected:")},
 /* 26 */ {0, {340,454,356,502}, EDITTEXT, x_("")},
 /* 27 */ {0, {340,514,356,595}, EDITTEXT, x_("")},
 /* 28 */ {0, {368,308,384,448}, MESSAGE, N_("Multiple cuts:")},
 /* 29 */ {0, {392,324,408,450}, MESSAGE, N_("When connected:")},
 /* 30 */ {0, {392,454,408,502}, EDITTEXT, x_("")},
 /* 31 */ {0, {392,514,408,595}, EDITTEXT, x_("")},
 /* 32 */ {0, {420,324,436,450}, MESSAGE, N_("Not connected:")},
 /* 33 */ {0, {420,454,436,502}, EDITTEXT, x_("")},
 /* 34 */ {0, {420,514,436,595}, EDITTEXT, x_("")},
 /* 35 */ {0, {180,452,196,523}, MESSAGE, N_("Distance")},
 /* 36 */ {0, {180,528,196,583}, MESSAGE, N_("Rule")},
 /* 37 */ {0, {24,328,48,488}, BUTTON, N_("Factory Reset of Rules")},
 /* 38 */ {0, {288,526,304,574}, EDITTEXT, x_("")},
 /* 39 */ {0, {260,324,276,450}, MESSAGE, N_("Edge:")},
 /* 40 */ {0, {260,454,276,502}, EDITTEXT, x_("")},
 /* 41 */ {0, {260,514,276,595}, EDITTEXT, x_("")},
 /* 42 */ {0, {192,104,208,300}, CHECK, N_("Show only lines with rules")},
 /* 43 */ {0, {56,8,72,95}, RADIO, N_("Nodes:")},
 /* 44 */ {0, {136,308,152,424}, MESSAGE, N_("Minimum Height:")},
 /* 45 */ {0, {136,454,152,502}, EDITTEXT, x_("")}
};
static DIALOG dr_rulesdialog = {{50,75,495,681}, N_("Design Rules"), 0, 45, dr_rulesdialogitems, 0, 0};

/* DXF Options */
static DIALOGITEM io_dxfoptionsdialogitems[] =
{
 /*  1 */ {0, {144,344,168,416}, BUTTON, N_("OK")},
 /*  2 */ {0, {144,248,168,320}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,16,168,231}, SCROLL, x_("")},
 /*  4 */ {0, {8,240,24,320}, MESSAGE, N_("DXF Layer:")},
 /*  5 */ {0, {8,324,24,430}, EDITTEXT, x_("")},
 /*  6 */ {0, {32,240,48,430}, CHECK, N_("Input flattens hierarchy")},
 /*  7 */ {0, {56,240,72,430}, CHECK, N_("Input reads all layers")},
 /*  8 */ {0, {80,240,96,320}, MESSAGE, N_("DXF Scale:")},
 /*  9 */ {0, {80,324,96,430}, POPUP, x_("")}
};
static DIALOG io_dxfoptionsdialog = {{50,75,228,514}, N_("DXF Options"), 0, 9, io_dxfoptionsdialogitems, 0, 0};

/* Dialog Edit: All dialogs */
static DIALOGITEM us_diaeditalldialogitems[] =
{
 /*  1 */ {0, {116,228,140,284}, DEFBUTTON, N_("Edit")},
 /*  2 */ {0, {8,16,336,228}, SCROLL, x_("")},
 /*  3 */ {0, {52,228,76,284}, BUTTON, N_("Save")},
 /*  4 */ {0, {16,228,40,284}, BUTTON, N_("Done")},
 /*  5 */ {0, {148,228,172,284}, BUTTON, N_("New")},
 /*  6 */ {0, {180,228,204,284}, BUTTON, N_("Delete")},
 /*  7 */ {0, {244,228,268,284}, BUTTON, N_("Title")},
 /*  8 */ {0, {276,228,300,284}, BUTTON, N_("Grid")},
 /*  9 */ {0, {84,228,108,284}, BUTTON, N_("Save .ui")},
 /* 10 */ {0, {212,228,236,284}, BUTTON, N_("Load .ui")}
};
static DIALOG us_diaeditalldialog = {{49,56,394,350}, N_("Dialog Editor"), 0, 10, us_diaeditalldialogitems, 0, 0};

/* special items for the "diaeditall" dialog: */
#define DDIE_EDITDIA   1		/* edit dialog (defbutton) */
#define DDIE_DIALIST   2		/* dialog list (scroll) */
#define DDIE_SAVE      3		/* save dialogs (button) */
#define DDIE_DONE      4		/* exit editor (button) */
#define DDIE_NEWDIA    5		/* new dialog (button) */
#define DDIE_DELDIA    6		/* delete dialog (button) */
#define DDIE_DIATITLE  7		/* set dialog title (button) */
#define DDIE_SETGRID   8		/* set item grid (button) */
#define DDIE_SAVEUI    9		/* save dialog.ui (button) */
#define DDIE_LOADUI   10		/* load dialog.ui (button) */

/* Dialog Edit: Dialog name */
static DIALOGITEM us_diaeditnewdialogitems[] =
{
 /*  1 */ {0, {56,8,76,68}, BUTTON, N_("OK")},
 /*  2 */ {0, {56,168,76,228}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,78}, MESSAGE, N_("Name:")},
 /*  4 */ {0, {8,80,24,228}, EDITTEXT, x_("")},
 /*  5 */ {0, {32,8,48,98}, MESSAGE, N_("Short name:")},
 /*  6 */ {0, {32,100,48,228}, EDITTEXT, x_("")}
};
static DIALOG us_diaeditnewdialog = {{450,56,535,293}, N_("New Dialog"), 0, 6, us_diaeditnewdialogitems, 0, 0};

/* Dialog Edit: Grid alignment */
static DIALOGITEM us_diaeditgriddialogitems[] =
{
 /*  1 */ {0, {32,8,52,68}, BUTTON, N_("OK")},
 /*  2 */ {0, {32,168,52,228}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,58,24,178}, EDITTEXT, x_("")}
};
static DIALOG us_diaeditgriddialog = {{450,56,511,293}, N_("Grid Amount"), 0, 3, us_diaeditgriddialogitems, 0, 0};

/* Dialog Edit: Item details */
static DIALOGITEM us_diaedititemdialogitems[] =
{
 /*  1 */ {0, {104,260,124,320}, BUTTON, N_("OK")},
 /*  2 */ {0, {104,20,124,80}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {56,8,72,332}, EDITTEXT, x_("")},
 /*  4 */ {0, {8,8,24,58}, MESSAGE, N_("Top:")},
 /*  5 */ {0, {8,60,24,90}, EDITTEXT, x_("")},
 /*  6 */ {0, {32,8,48,58}, MESSAGE, N_("Left:")},
 /*  7 */ {0, {32,60,48,90}, EDITTEXT, x_("")},
 /*  8 */ {0, {8,110,24,160}, MESSAGE, N_("Bottom:")},
 /*  9 */ {0, {8,162,24,192}, EDITTEXT, x_("")},
 /* 10 */ {0, {32,110,48,160}, MESSAGE, N_("Right:")},
 /* 11 */ {0, {32,162,48,192}, EDITTEXT, x_("")},
 /* 12 */ {0, {80,8,96,214}, POPUP, x_("")},
 /* 13 */ {0, {80,228,97,332}, CHECK, N_("Inactive")},
 /* 14 */ {0, {20,200,36,332}, CHECK, N_("Width and Height")}
};
static DIALOG us_diaedititemdialog = {{450,56,583,397}, N_("Item Information"), 0, 14, us_diaedititemdialogitems, 0, 0};

/* Dialog Edit: Save */
static DIALOGITEM us_diaeditsavedialogitems[] =
{
 /*  1 */ {0, {8,8,32,68}, BUTTON, N_("Save")},
 /*  2 */ {0, {8,88,32,148}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,168,32,228}, BUTTON, N_("No Save")}
};
static DIALOG us_diaeditsavedialog = {{450,56,491,293}, N_("Dialogs changed.  Save?"), 0, 3, us_diaeditsavedialogitems, 0, 0};

/* Dialog Edit: Set title */
static DIALOGITEM us_diaedittitledialogitems[] =
{
 /*  1 */ {0, {32,8,52,68}, BUTTON, N_("OK")},
 /*  2 */ {0, {32,168,52,228}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,228}, EDITTEXT, x_("")}
};
static DIALOG us_diaedittitledialog = {{450,56,511,293}, N_("Dialog Title"), 0, 3, us_diaedittitledialogitems, 0, 0};

/* Dialog Edit: Single dialog */
static DIALOGITEM us_diaeditsingledialogitems[] =
{
 /*  1 */ {0, {32,4,48,51}, BUTTON, N_("Edit")},
 /*  2 */ {0, {4,4,20,52}, BUTTON, N_("Done")},
 /*  3 */ {0, {60,4,76,52}, BUTTON, N_("New")},
 /*  4 */ {0, {80,4,96,52}, BUTTON, N_("Dup")},
 /*  5 */ {0, {128,4,144,52}, BUTTON, N_("Align")},
 /*  6 */ {0, {4,60,20,156}, CHECK, N_("Numbers")},
 /*  7 */ {0, {4,160,20,256}, CHECK, N_("Outlines")},
 /*  8 */ {0, {25,61,200,300}, USERDRAWN, x_("")},
 /*  9 */ {0, {100,4,116,52}, BUTTON, N_("Del")}
};
static DIALOG us_diaeditsingledialog = {{100,360,309,669}, N_("Dialog"), 0, 9, us_diaeditsingledialogitems, 0, 0};

/* EDIF Options */
static DIALOGITEM io_edifoptionsdialogitems[] =
{
 /*  1 */ {0, {68,120,92,192}, BUTTON, N_("OK")},
 /*  2 */ {0, {68,16,92,88}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,200}, CHECK, N_("Use Schematic View")},
 /*  4 */ {0, {36,8,52,96}, MESSAGE, N_("Input scale:")},
 /*  5 */ {0, {36,100,52,188}, EDITTEXT, x_("")}
};
static DIALOG io_edifoptionsdialog = {{50,75,151,284}, N_("EDIF Options"), 0, 5, io_edifoptionsdialogitems, 0, 0};

/* EMACS warning */
static DIALOGITEM us_emawarndialogitems[] =
{
 /*  1 */ {0, {160,116,184,196}, BUTTON, N_("OK")},
 /*  2 */ {0, {4,4,20,340}, MESSAGE, N_("You have just typed Control-X")},
 /*  3 */ {0, {24,4,40,340}, MESSAGE, N_("Followed quickly by another control character.")},
 /*  4 */ {0, {52,4,68,340}, MESSAGE, N_("Could it be that you think this is EMACS?")},
 /*  5 */ {0, {84,4,100,340}, MESSAGE, N_("The second control character has been ignored.")},
 /*  6 */ {0, {104,4,120,340}, MESSAGE, N_("If you wish to disable this check, click below.")},
 /*  7 */ {0, {128,32,144,284}, BUTTON, N_("Disable EMACS character check")}
};
static DIALOG us_emawarndialog = {{75,75,268,425}, N_("This is Not EMACS"), 0, 7, us_emawarndialogitems, 0, 0};

/* Edit Cell */
static DIALOGITEM us_editcelldialogitems[] =
{
 /*  1 */ {0, {284,208,308,272}, BUTTON, N_("OK")},
 /*  2 */ {0, {284,16,308,80}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,8,208,280}, SCROLL, x_("")},
 /*  4 */ {0, {212,8,228,153}, CHECK, N_("Show old versions")},
 /*  5 */ {0, {260,8,276,231}, CHECK, N_("Make new window for cell")},
 /*  6 */ {0, {284,104,308,187}, BUTTON, N_("New Cell")},
 /*  7 */ {0, {8,8,24,67}, MESSAGE, N_("Library:")},
 /*  8 */ {0, {8,72,24,280}, POPUP, x_("")},
 /*  9 */ {0, {236,8,252,231}, CHECK, N_("Show cells from Cell-Library")},
 /* 10 */ {0, {331,8,347,213}, EDITTEXT, x_("")},
 /* 11 */ {0, {328,216,352,280}, BUTTON, N_("Rename")},
 /* 12 */ {0, {356,144,380,208}, BUTTON, N_("Delete")},
 /* 13 */ {0, {316,8,317,280}, DIVIDELINE, N_("item")},
 /* 14 */ {0, {360,8,376,137}, CHECK, N_("Confirm deletion")}
};
static DIALOG us_editcelldialog = {{50,75,439,365}, N_("Edit Cell"), 0, 14, us_editcelldialogitems, 0, 0};

/* Edit Colors */
static DIALOGITEM us_colormixdialogitems[] =
{
 /*  1 */ {0, {212,400,236,468}, BUTTON, N_("OK")},
 /*  2 */ {0, {212,320,236,388}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {276,8,292,24}, USERDRAWN, x_("")},
 /*  4 */ {0, {276,36,292,472}, RADIO, N_("Entry")},
 /*  5 */ {0, {296,8,312,24}, USERDRAWN, x_("")},
 /*  6 */ {0, {296,36,312,472}, RADIO, N_("Entry")},
 /*  7 */ {0, {316,8,332,24}, USERDRAWN, x_("")},
 /*  8 */ {0, {316,36,332,472}, RADIO, N_("Entry")},
 /*  9 */ {0, {336,8,352,24}, USERDRAWN, x_("")},
 /* 10 */ {0, {336,36,352,472}, RADIO, N_("Entry")},
 /* 11 */ {0, {356,8,372,24}, USERDRAWN, x_("")},
 /* 12 */ {0, {356,36,372,472}, RADIO, N_("Entry")},
 /* 13 */ {0, {376,8,392,24}, USERDRAWN, x_("")},
 /* 14 */ {0, {376,36,392,472}, RADIO, N_("Entry")},
 /* 15 */ {0, {396,8,412,24}, USERDRAWN, x_("")},
 /* 16 */ {0, {396,36,412,472}, RADIO, N_("Entry")},
 /* 17 */ {0, {416,8,432,24}, USERDRAWN, x_("")},
 /* 18 */ {0, {416,36,432,472}, RADIO, N_("Entry")},
 /* 19 */ {0, {436,8,452,24}, USERDRAWN, x_("")},
 /* 20 */ {0, {436,36,452,472}, RADIO, N_("Entry")},
 /* 21 */ {0, {456,8,472,24}, USERDRAWN, x_("")},
 /* 22 */ {0, {456,36,472,472}, RADIO, N_("Entry")},
 /* 23 */ {0, {476,8,492,24}, USERDRAWN, x_("")},
 /* 24 */ {0, {476,36,492,472}, RADIO, N_("Entry")},
 /* 25 */ {0, {496,8,512,24}, USERDRAWN, x_("")},
 /* 26 */ {0, {496,36,512,472}, RADIO, N_("Entry")},
 /* 27 */ {0, {516,8,532,24}, USERDRAWN, x_("")},
 /* 28 */ {0, {516,36,532,472}, RADIO, N_("Entry")},
 /* 29 */ {0, {536,8,552,24}, USERDRAWN, x_("")},
 /* 30 */ {0, {536,36,552,472}, RADIO, N_("Entry")},
 /* 31 */ {0, {556,8,572,24}, USERDRAWN, x_("")},
 /* 32 */ {0, {556,36,572,472}, RADIO, N_("Entry")},
 /* 33 */ {0, {576,8,592,24}, USERDRAWN, x_("")},
 /* 34 */ {0, {576,36,592,472}, RADIO, N_("Entry")},
 /* 35 */ {0, {32,8,244,220}, USERDRAWN, x_("")},
 /* 36 */ {0, {32,228,244,298}, USERDRAWN, x_("")},
 /* 37 */ {0, {8,16,24,144}, MESSAGE, N_("Hue/Saturation:")},
 /* 38 */ {0, {8,228,24,308}, MESSAGE, N_("Intensity:")},
 /* 39 */ {0, {252,160,268,396}, POPUP, x_("")},
 /* 40 */ {0, {36,304,52,388}, MESSAGE, N_("Red:")},
 /* 41 */ {0, {60,304,76,388}, MESSAGE, N_("Green:")},
 /* 42 */ {0, {84,304,100,388}, MESSAGE, N_("Blue:")},
 /* 43 */ {0, {36,393,52,457}, EDITTEXT, x_("")},
 /* 44 */ {0, {60,393,76,457}, EDITTEXT, x_("")},
 /* 45 */ {0, {84,393,100,457}, EDITTEXT, x_("")},
 /* 46 */ {0, {252,8,268,148}, MESSAGE, N_("Colors being edited:")},
 /* 47 */ {0, {176,304,200,472}, BUTTON, N_("Set Transparent Layers")},
 /* 48 */ {0, {116,304,132,388}, MESSAGE, N_("Opacity:")},
 /* 49 */ {0, {140,304,156,388}, MESSAGE, N_("Foreground:")},
 /* 50 */ {0, {116,393,132,457}, EDITTEXT, x_("")},
 /* 51 */ {0, {140,393,156,457}, POPUP, x_("")}
};
static DIALOG us_colormixdialog = {{75,75,676,557}, N_("Color Mixing"), 0, 51, us_colormixdialogitems, 0, 0};

/* Edit Library Dependencies */
static DIALOGITEM us_dependentlibdialogitems[] =
{
 /*  1 */ {0, {208,368,232,432}, BUTTON, N_("OK")},
 /*  2 */ {0, {208,256,232,320}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,8,177,174}, SCROLL, x_("")},
 /*  4 */ {0, {8,8,24,153}, MESSAGE, N_("Dependent Libraries:")},
 /*  5 */ {0, {208,8,224,165}, MESSAGE, N_("Libraries are examined")},
 /*  6 */ {0, {40,192,64,256}, BUTTON, N_("Remove")},
 /*  7 */ {0, {88,192,112,256}, BUTTON, N_("<< Add")},
 /*  8 */ {0, {128,280,144,427}, MESSAGE, N_("Library (if not in list):")},
 /*  9 */ {0, {152,280,168,432}, EDITTEXT, x_("")},
 /* 10 */ {0, {8,272,24,361}, MESSAGE, N_("All Libraries:")},
 /* 11 */ {0, {224,8,240,123}, MESSAGE, N_("from bottom up")},
 /* 12 */ {0, {32,272,118,438}, SCROLL, x_("")},
 /* 13 */ {0, {184,8,200,67}, MESSAGE, N_("Current:")},
 /* 14 */ {0, {184,72,200,254}, MESSAGE, x_("")}
};
static DIALOG us_dependentlibdialog = {{50,75,299,524}, N_("Dependent Library Selection"), 0, 14, us_dependentlibdialogitems, 0, 0};

/* Edit Variables (Technology Edit) */
static DIALOGITEM us_techvarsdialogitems[] =
{
 /*  1 */ {0, {208,472,232,536}, BUTTON, N_("OK")},
 /*  2 */ {0, {208,376,232,440}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {24,8,143,264}, SCROLL, x_("")},
 /*  4 */ {0, {176,16,192,55}, MESSAGE, N_("Type:")},
 /*  5 */ {0, {176,56,192,142}, MESSAGE, x_("")},
 /*  6 */ {0, {152,104,168,536}, MESSAGE, x_("")},
 /*  7 */ {0, {24,280,143,536}, SCROLL, x_("")},
 /*  8 */ {0, {8,16,24,240}, MESSAGE, N_("Current Variables on Technology:")},
 /*  9 */ {0, {8,288,24,419}, MESSAGE, N_("Possible Variables:")},
 /* 10 */ {0, {208,280,232,344}, BUTTON, N_("<< Copy")},
 /* 11 */ {0, {208,24,232,88}, BUTTON, N_("Remove")},
 /* 12 */ {0, {176,216,192,533}, EDITTEXT, x_("")},
 /* 13 */ {0, {208,136,232,237}, BUTTON, N_("Edit Strings")},
 /* 14 */ {0, {176,168,192,212}, MESSAGE, N_("Value:")},
 /* 15 */ {0, {152,16,168,98}, MESSAGE, N_("Description:")}
};
static DIALOG us_techvarsdialog = {{50,75,293,622}, N_("Technology Variables"), 0, 15, us_techvarsdialogitems, 0, 0};

/* FastHenry Arc Info */
static DIALOGITEM sim_fasthenryarcdialogitems[] =
{
 /*  1 */ {0, {88,236,112,316}, BUTTON, N_("OK")},
 /*  2 */ {0, {40,236,64,316}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,8,48,132}, MESSAGE, N_("Thickness:")},
 /*  4 */ {0, {32,136,48,216}, EDITTEXT, x_("")},
 /*  5 */ {0, {56,8,72,132}, MESSAGE, N_("Width:")},
 /*  6 */ {0, {56,136,72,216}, MESSAGE, x_("")},
 /*  7 */ {0, {80,8,96,180}, MESSAGE, N_("Width subdivisions:")},
 /*  8 */ {0, {80,184,96,216}, EDITTEXT, x_("")},
 /*  9 */ {0, {104,8,120,180}, MESSAGE, N_("Height subdivisions:")},
 /* 10 */ {0, {104,184,120,216}, EDITTEXT, x_("")},
 /* 11 */ {0, {232,8,248,36}, MESSAGE, N_("X:")},
 /* 12 */ {0, {232,40,248,136}, MESSAGE, x_("")},
 /* 13 */ {0, {204,8,220,144}, MESSAGE, N_("Head of arc is at:")},
 /* 14 */ {0, {256,8,272,36}, MESSAGE, N_("Y:")},
 /* 15 */ {0, {256,40,272,136}, MESSAGE, x_("")},
 /* 16 */ {0, {280,8,296,36}, MESSAGE, N_("Z:")},
 /* 17 */ {0, {280,40,296,132}, EDITTEXT, x_("")},
 /* 18 */ {0, {232,180,248,208}, MESSAGE, N_("X:")},
 /* 19 */ {0, {232,212,248,308}, MESSAGE, x_("")},
 /* 20 */ {0, {204,180,220,316}, MESSAGE, N_("Tail of arc is at:")},
 /* 21 */ {0, {256,180,272,208}, MESSAGE, N_("Y:")},
 /* 22 */ {0, {256,212,272,308}, MESSAGE, x_("")},
 /* 23 */ {0, {280,180,296,208}, MESSAGE, N_("Z:")},
 /* 24 */ {0, {280,212,296,304}, EDITTEXT, x_("")},
 /* 25 */ {0, {144,8,160,108}, MESSAGE, N_("Group name:")},
 /* 26 */ {0, {144,112,160,316}, POPUP, x_("")},
 /* 27 */ {0, {168,80,184,176}, BUTTON, N_("New Group")},
 /* 28 */ {0, {132,8,133,316}, DIVIDELINE, x_("")},
 /* 29 */ {0, {192,8,193,316}, DIVIDELINE, x_("")},
 /* 30 */ {0, {8,8,24,316}, CHECK, N_("Include this arc in FastHenry analysis")},
 /* 31 */ {0, {304,88,320,168}, MESSAGE, N_("Default Z:")},
 /* 32 */ {0, {304,172,320,268}, MESSAGE, x_("")}
};
static DIALOG sim_fasthenryarcdialog = {{75,75,404,400}, N_("FastHenry Arc Properties"), 0, 32, sim_fasthenryarcdialogitems, 0, 0};

/* FastHenry Options */
static DIALOGITEM sim_fasthenrydialogitems[] =
{
 /*  1 */ {0, {164,392,188,472}, BUTTON, N_("OK")},
 /*  2 */ {0, {164,12,188,92}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,20,48,140}, MESSAGE, N_("Frequency start:")},
 /*  4 */ {0, {32,144,48,200}, EDITTEXT, x_("")},
 /*  5 */ {0, {56,20,72,140}, MESSAGE, N_("Frequency end:")},
 /*  6 */ {0, {56,144,72,200}, EDITTEXT, x_("")},
 /*  7 */ {0, {80,20,96,140}, MESSAGE, N_("Runs per decade:")},
 /*  8 */ {0, {80,144,96,200}, EDITTEXT, x_("")},
 /*  9 */ {0, {8,8,24,200}, CHECK, N_("Use single frequency")},
 /* 10 */ {0, {128,20,144,140}, MESSAGE, N_("Number of poles:")},
 /* 11 */ {0, {128,144,144,200}, EDITTEXT, x_("")},
 /* 12 */ {0, {104,8,120,200}, CHECK, N_("Make multipole subcircuit")},
 /* 13 */ {0, {104,224,120,420}, CHECK, N_("Make PostScript view")},
 /* 14 */ {0, {80,224,96,420}, MESSAGE, N_("Maximum segment length:")},
 /* 15 */ {0, {80,424,96,480}, EDITTEXT, x_("")},
 /* 16 */ {0, {32,224,48,420}, MESSAGE, N_("Default width subdivisions:")},
 /* 17 */ {0, {32,424,48,480}, EDITTEXT, x_("")},
 /* 18 */ {0, {56,224,72,420}, MESSAGE, N_("Default height subdivisions:")},
 /* 19 */ {0, {56,424,72,480}, EDITTEXT, x_("")},
 /* 20 */ {0, {128,224,144,420}, CHECK, N_("Make SPICE subcircuit")},
 /* 21 */ {0, {8,224,24,420}, MESSAGE, N_("Default thickness:")},
 /* 22 */ {0, {8,424,24,480}, EDITTEXT, x_("")},
 /* 23 */ {0, {176,140,192,344}, POPUP, x_("")},
 /* 24 */ {0, {156,176,172,320}, MESSAGE, N_("After writing deck:")}
};
static DIALOG sim_fasthenrydialog = {{75,75,276,565}, N_("FastHenry Options"), 0, 24, sim_fasthenrydialogitems, 0, 0};

/* File Input (UNIX) */
static DIALOGITEM gra_fileindialogitems[] =
{
 /*  1 */ {0, {128,256,152,336}, BUTTON, N_("Open")},
 /*  2 */ {0, {176,256,200,336}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {40,8,216,240}, SCROLL, x_("")},
 /*  4 */ {0, {8,8,40,336}, MESSAGE, x_("")},
 /*  5 */ {0, {80,256,104,336}, BUTTON, N_("Up")}
};
static DIALOG gra_fileindialog = {{50,75,278,421}, 0, 0, 5, gra_fileindialogitems, 0, 0};

/* File Output (UNIX) */
static DIALOGITEM gra_fileoutdialogitems[] =
{
 /*  1 */ {0, {146,256,170,336}, BUTTON, N_("OK")},
 /*  2 */ {0, {194,256,218,336}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {50,8,218,240}, SCROLL, x_("")},
 /*  4 */ {0, {8,8,40,336}, MESSAGE, x_("")},
 /*  5 */ {0, {50,256,74,336}, BUTTON, N_("Up")},
 /*  6 */ {0, {98,256,122,336}, BUTTON, N_("Down")},
 /*  7 */ {0, {226,8,242,336}, EDITTEXT, x_("")}
};
static DIALOG gra_fileoutdialog = {{50,75,301,421}, 0, 0, 7, gra_fileoutdialogitems, 0, 0};

/* Find Options */
static DIALOGITEM us_findoptdialogitems[] =
{
 /*  1 */ {0, {292,8,316,88}, BUTTON, N_("Find")},
 /*  2 */ {0, {24,4,288,372}, SCROLL, x_("")},
 /*  3 */ {0, {4,4,20,84}, MESSAGE, N_("Options:")},
 /*  4 */ {0, {292,292,316,372}, BUTTON, N_("Done")},
 /*  5 */ {0, {296,92,312,288}, EDITTEXT, x_("")}
};
static DIALOG us_findoptdialog = {{75,75,400,457}, N_("Finding Options"), 0, 5, us_findoptdialogitems, 0, 0};

/* Find Text */
static DIALOGITEM us_txtsardialogitems[] =
{
 /*  1 */ {0, {92,4,116,72}, BUTTON, N_("Find")},
 /*  2 */ {0, {124,328,148,396}, BUTTON, N_("Done")},
 /*  3 */ {0, {92,84,116,152}, BUTTON, N_("Replace")},
 /*  4 */ {0, {8,76,24,396}, EDITTEXT, x_("")},
 /*  5 */ {0, {8,8,24,72}, MESSAGE, N_("Find:")},
 /*  6 */ {0, {36,76,52,396}, EDITTEXT, x_("")},
 /*  7 */ {0, {36,8,52,72}, MESSAGE, N_("Replace:")},
 /*  8 */ {0, {92,296,116,396}, BUTTON, N_("Replace All")},
 /*  9 */ {0, {64,216,80,344}, CHECK, N_("Find Reverse")},
 /* 10 */ {0, {64,80,80,208}, CHECK, N_("Case Sensitive")},
 /* 11 */ {0, {92,164,116,284}, BUTTON, N_("Replace and Find")},
 /* 12 */ {0, {128,4,144,96}, MESSAGE, N_("Line number:")},
 /* 13 */ {0, {128,100,144,180}, EDITTEXT, x_("")},
 /* 14 */ {0, {128,188,144,268}, BUTTON, N_("Go To Line")}
};
static DIALOG us_txtsardialog = {{75,75,232,481}, N_("Search and Replace"), 0, 14, us_txtsardialogitems, 0, 0};

/* Font Selection (Mac) */
static DIALOGITEM gra_fontdialogitems[] =
{
 /*  1 */ {0, {136,192,160,256}, BUTTON, N_("OK")},
 /*  2 */ {0, {88,192,112,256}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,24,24,56}, MESSAGE, N_("Font")},
 /*  4 */ {0, {24,200,40,232}, MESSAGE, N_("Size")},
 /*  5 */ {0, {32,24,160,184}, SCROLL, x_("")},
 /*  6 */ {0, {48,192,64,248}, EDITTEXT, x_("")}
};
static DIALOG gra_fontdialog = {{50,75,219,340}, N_("Messages Window Font"), 0, 6, gra_fontdialogitems, 0, 0};

/* Font Selection (UNIX) */
static DIALOGITEM gra_xfontdialogitems[] =
{
 /*  1 */ {0, {312,592,336,652}, BUTTON, N_("OK")},
 /*  2 */ {0, {312,312,336,372}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,88}, MESSAGE, N_("Foundry")},
 /*  4 */ {0, {8,96,24,296}, POPUP, x_("")},
 /*  5 */ {0, {32,8,48,88}, MESSAGE, N_("Family")},
 /*  6 */ {0, {32,96,48,296}, POPUP, x_("")},
 /*  7 */ {0, {56,8,72,88}, MESSAGE, N_("Weight")},
 /*  8 */ {0, {56,96,72,296}, POPUP, x_("")},
 /*  9 */ {0, {80,8,96,88}, MESSAGE, N_("Slant")},
 /* 10 */ {0, {80,96,96,296}, POPUP, x_("")},
 /* 11 */ {0, {104,8,120,88}, MESSAGE, N_("S Width")},
 /* 12 */ {0, {104,96,120,296}, POPUP, x_("")},
 /* 13 */ {0, {128,8,144,88}, MESSAGE, N_("Ad Style")},
 /* 14 */ {0, {128,96,144,296}, POPUP, x_("")},
 /* 15 */ {0, {152,8,168,88}, MESSAGE, N_("Pixel Size")},
 /* 16 */ {0, {152,96,168,296}, POPUP, x_("")},
 /* 17 */ {0, {176,8,192,88}, MESSAGE, N_("Point Size")},
 /* 18 */ {0, {176,96,192,296}, POPUP, x_("")},
 /* 19 */ {0, {200,8,216,88}, MESSAGE, N_("X Resolution")},
 /* 20 */ {0, {200,96,216,296}, POPUP, x_("")},
 /* 21 */ {0, {224,8,240,88}, MESSAGE, N_("Y Resolution")},
 /* 22 */ {0, {224,96,240,296}, POPUP, x_("")},
 /* 23 */ {0, {248,8,264,88}, MESSAGE, N_("Spacing")},
 /* 24 */ {0, {248,96,264,296}, POPUP, x_("")},
 /* 25 */ {0, {272,8,288,88}, MESSAGE, N_("Avg Width")},
 /* 26 */ {0, {272,96,288,296}, POPUP, x_("")},
 /* 27 */ {0, {296,8,312,88}, MESSAGE, N_("Registry")},
 /* 28 */ {0, {296,96,312,296}, POPUP, x_("")},
 /* 29 */ {0, {320,8,336,88}, MESSAGE, N_("Encoding")},
 /* 30 */ {0, {320,96,336,296}, POPUP, x_("")},
 /* 31 */ {0, {8,304,300,656}, SCROLL, x_("")}
};
static DIALOG gra_xfontdialog = {{75,75,421,740}, N_("Font Selection"), 0, 31, gra_xfontdialogitems, 0, 0};

/* Frame Options */
static DIALOGITEM us_drawingoptdialogitems[] =
{
 /*  1 */ {0, {48,456,72,520}, BUTTON, N_("OK")},
 /*  2 */ {0, {12,456,36,520}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {116,4,132,112}, MESSAGE, N_("Company Name:")},
 /*  4 */ {0, {92,116,108,232}, MESSAGE, N_("Default:")},
 /*  5 */ {0, {140,4,156,112}, MESSAGE, N_("Designer Name:")},
 /*  6 */ {0, {164,4,180,112}, MESSAGE, N_("Project Name:")},
 /*  7 */ {0, {44,4,60,100}, MESSAGE, N_("Frame size:")},
 /*  8 */ {0, {44,104,60,220}, POPUP, x_("")},
 /*  9 */ {0, {32,228,48,324}, RADIO, N_("Landscape")},
 /* 10 */ {0, {56,228,72,324}, RADIO, N_("Portrait")},
 /* 11 */ {0, {8,4,24,404}, MESSAGE, N_("For cell:")},
 /* 12 */ {0, {116,116,132,316}, EDITTEXT, x_("")},
 /* 13 */ {0, {80,4,80,524}, DIVIDELINE, x_("")},
 /* 14 */ {0, {140,116,156,316}, EDITTEXT, x_("")},
 /* 15 */ {0, {164,116,180,316}, EDITTEXT, x_("")},
 /* 16 */ {0, {92,324,108,384}, MESSAGE, N_("Library:")},
 /* 17 */ {0, {92,384,108,524}, POPUP, x_("")},
 /* 18 */ {0, {116,324,132,524}, EDITTEXT, x_("")},
 /* 19 */ {0, {140,324,156,524}, EDITTEXT, x_("")},
 /* 20 */ {0, {164,324,180,524}, EDITTEXT, x_("")},
 /* 21 */ {0, {44,332,60,444}, CHECK, N_("Title Box")}
};
static DIALOG us_drawingoptdialog = {{50,75,239,609}, N_("Frame Options"), 0, 21, us_drawingoptdialogitems, 0, 0};

/* GDS Options */
static DIALOGITEM io_gdsoptionsdialogitems[] =
{
 /*  1 */ {0, {308,140,332,212}, BUTTON, N_("OK")},
 /*  2 */ {0, {308,32,332,104}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,296,232}, SCROLL, x_("")},
 /*  4 */ {0, {8,240,24,336}, MESSAGE, N_("GDS Layer(s):")},
 /*  5 */ {0, {8,340,24,444}, EDITTEXT, x_("")},
 /*  6 */ {0, {80,240,96,464}, CHECK, N_("Input Includes Text")},
 /*  7 */ {0, {104,240,120,464}, CHECK, N_("Input Expands Cells")},
 /*  8 */ {0, {128,240,144,464}, CHECK, N_("Input Instantiates Arrays")},
 /*  9 */ {0, {176,240,192,464}, CHECK, N_("Output Merges Boxes")},
 /* 10 */ {0, {272,240,288,428}, MESSAGE, N_("Output Arc Conversion:")},
 /* 11 */ {0, {296,248,312,424}, MESSAGE, N_("Maximum arc angle:")},
 /* 12 */ {0, {296,428,312,496}, EDITTEXT, x_("")},
 /* 13 */ {0, {320,248,336,424}, MESSAGE, N_("Maximum arc sag:")},
 /* 14 */ {0, {320,428,336,496}, EDITTEXT, x_("")},
 /* 15 */ {0, {152,240,168,464}, CHECK, N_("Input Ignores Unknown Layers")},
 /* 16 */ {0, {248,240,264,424}, MESSAGE, N_("Output default text layer:")},
 /* 17 */ {0, {248,428,264,496}, EDITTEXT, x_("")},
 /* 18 */ {0, {32,256,48,336}, MESSAGE, N_("Pin layer:")},
 /* 19 */ {0, {32,340,48,444}, EDITTEXT, x_("")},
 /* 20 */ {0, {56,256,72,336}, MESSAGE, N_("Text layer:")},
 /* 21 */ {0, {56,340,72,444}, EDITTEXT, x_("")},
 /* 22 */ {0, {200,240,216,464}, CHECK, N_("Output Writes Export Pins")},
 /* 23 */ {0, {224,240,240,464}, CHECK, N_("Output All Upper Case")}
};
static DIALOG io_gdsoptionsdialog = {{50,75,395,581}, N_("GDS Options"), 0, 23, io_gdsoptionsdialogitems, 0, 0};

/* General Options */
static DIALOGITEM us_genoptdialogitems[] =
{
 /*  1 */ {0, {180,268,204,348}, BUTTON, N_("OK")},
 /*  2 */ {0, {180,152,204,232}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,216}, CHECK, N_("Beep after long jobs")},
 /*  4 */ {0, {80,8,96,308}, CHECK, N_("Include date and version in output files")},
 /*  5 */ {0, {180,40,204,120}, BUTTON, N_("Advanced")},
 /*  6 */ {0, {128,8,144,308}, MESSAGE, N_("Maximum errors to report (0 for infinite):")},
 /*  7 */ {0, {128,312,144,372}, EDITTEXT, x_("")},
 /*  8 */ {0, {56,8,72,308}, CHECK, N_("Expandable dialogs default to fullsize")},
 /*  9 */ {0, {32,8,48,308}, CHECK, N_("Click sounds when arcs are created")},
 /* 10 */ {0, {152,8,168,252}, MESSAGE, N_("Prevent motion after selection for:")},
 /* 11 */ {0, {152,256,168,304}, EDITTEXT, x_("")},
 /* 12 */ {0, {152,308,168,384}, MESSAGE, N_("seconds")},
 /* 13 */ {0, {104,8,120,384}, CHECK, N_("Show file-selection dialog before writing netlists")}
};
static DIALOG us_genoptdialog = {{75,75,288,469}, N_("General Options"), 0, 13, us_genoptdialogitems, 0, 0};

/* Get Info: Arc */
static DIALOGITEM us_showarcdialogitems[] =
{
 /*  1 */ {0, {148,336,172,408}, BUTTON, N_("OK")},
 /*  2 */ {0, {108,336,132,408}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {188,336,212,408}, BUTTON, N_("Attributes")},
 /*  4 */ {0, {104,88,120,176}, MESSAGE, x_("")},
 /*  5 */ {0, {8,88,24,392}, MESSAGE, x_("")},
 /*  6 */ {0, {32,88,48,392}, MESSAGE, x_("")},
 /*  7 */ {0, {56,88,72,392}, EDITTEXT, x_("")},
 /*  8 */ {0, {80,88,96,172}, EDITTEXT, x_("")},
 /*  9 */ {0, {128,88,144,320}, MESSAGE, x_("")},
 /* 10 */ {0, {80,280,96,364}, MESSAGE, x_("")},
 /* 11 */ {0, {176,88,192,320}, MESSAGE, x_("")},
 /* 12 */ {0, {8,16,24,80}, MESSAGE, N_("Type:")},
 /* 13 */ {0, {32,16,48,80}, MESSAGE, N_("Network:")},
 /* 14 */ {0, {80,16,96,80}, MESSAGE, N_("Width:")},
 /* 15 */ {0, {104,16,120,80}, MESSAGE, N_("Angle:")},
 /* 16 */ {0, {128,16,144,80}, MESSAGE, N_("Head:")},
 /* 17 */ {0, {80,216,96,280}, MESSAGE, N_("Bus size:")},
 /* 18 */ {0, {176,16,192,80}, MESSAGE, N_("Tail:")},
 /* 19 */ {0, {104,196,120,324}, CHECK, N_("Easy to Select")},
 /* 20 */ {0, {252,16,268,112}, CHECK, N_("Negated")},
 /* 21 */ {0, {276,16,292,112}, CHECK, N_("Directional")},
 /* 22 */ {0, {300,16,316,120}, CHECK, N_("Ends extend")},
 /* 23 */ {0, {252,136,268,240}, CHECK, N_("Ignore head")},
 /* 24 */ {0, {276,136,292,232}, CHECK, N_("Ignore tail")},
 /* 25 */ {0, {300,136,316,304}, CHECK, N_("Reverse head and tail")},
 /* 26 */ {0, {252,328,268,424}, CHECK, N_("Temporary")},
 /* 27 */ {0, {276,312,292,416}, CHECK, N_("Fixed-angle")},
 /* 28 */ {0, {56,16,72,80}, MESSAGE, N_("Name:")},
 /* 29 */ {0, {228,136,244,300}, MESSAGE, x_("")},
 /* 30 */ {0, {152,40,168,80}, MESSAGE, N_("At:")},
 /* 31 */ {0, {152,88,168,220}, MESSAGE, x_("")},
 /* 32 */ {0, {152,232,168,272}, BUTTON, N_("See")},
 /* 33 */ {0, {200,40,216,80}, MESSAGE, N_("At:")},
 /* 34 */ {0, {200,88,216,220}, MESSAGE, x_("")},
 /* 35 */ {0, {200,232,216,272}, BUTTON, N_("See")},
 /* 36 */ {0, {152,280,168,320}, BUTTON, N_("Info")},
 /* 37 */ {0, {200,280,216,320}, BUTTON, N_("Info")},
 /* 38 */ {0, {300,312,316,416}, CHECK, N_("Slidable")},
 /* 39 */ {0, {228,312,244,404}, CHECK, N_("Rigid")},
 /* 40 */ {0, {228,16,244,132}, MESSAGE, x_("")}
};
static DIALOG us_showarcdialog = {{50,75,375,509}, N_("Arc Information"), 0, 40, us_showarcdialogitems, 0, 0};

/* Get Info: Capacitance Info */
static DIALOGITEM us_capacitancedialogitems[] =
{
 /*  1 */ {0, {40,176,64,240}, BUTTON, N_("OK")},
 /*  2 */ {0, {40,16,64,80}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,16,24,110}, EDITTEXT, x_("")},
 /*  4 */ {0, {8,120,24,246}, POPUP, x_("")},
 /*  5 */ {0, {40,96,64,160}, BUTTON, N_("More...")}
};
static DIALOG us_capacitancedialog = {{50,75,123,330}, N_("Capacitance"), 0, 5, us_capacitancedialogitems, 0, 0};

/* Get Info: Export */
static DIALOGITEM us_portinfodialogitems[] =
{
 /*  1 */ {0, {268,376,292,448}, BUTTON, N_("OK")},
 /*  2 */ {0, {268,284,292,356}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {120,8,136,56}, RADIO, N_("Left")},
 /*  4 */ {0, {8,112,24,448}, EDITTEXT, x_("")},
 /*  5 */ {0, {88,216,104,264}, EDITTEXT, x_("")},
 /*  6 */ {0, {56,8,72,88}, RADIO, N_("Center")},
 /*  7 */ {0, {72,8,88,80}, RADIO, N_("Bottom")},
 /*  8 */ {0, {88,8,104,56}, RADIO, N_("Top")},
 /*  9 */ {0, {104,8,120,64}, RADIO, N_("Right")},
 /* 10 */ {0, {136,8,152,104}, RADIO, N_("Lower right")},
 /* 11 */ {0, {152,8,168,96}, RADIO, N_("Lower left")},
 /* 12 */ {0, {168,8,184,104}, RADIO, N_("Upper right")},
 /* 13 */ {0, {184,8,200,96}, RADIO, N_("Upper left")},
 /* 14 */ {0, {88,272,104,432}, RADIO, N_("Points (max 63)")},
 /* 15 */ {0, {268,192,292,264}, BUTTON, N_("Attributes")},
 /* 16 */ {0, {40,16,56,107}, MESSAGE, N_("Text corner:")},
 /* 17 */ {0, {208,232,224,296}, EDITTEXT, x_("")},
 /* 18 */ {0, {208,384,224,448}, EDITTEXT, x_("")},
 /* 19 */ {0, {208,160,224,225}, MESSAGE, N_("X offset:")},
 /* 20 */ {0, {208,312,224,377}, MESSAGE, N_("Y offset:")},
 /* 21 */ {0, {56,112,88,144}, ICON, (CHAR *)us_icon200},
 /* 22 */ {0, {88,112,120,144}, ICON, (CHAR *)us_icon201},
 /* 23 */ {0, {120,112,152,144}, ICON, (CHAR *)us_icon202},
 /* 24 */ {0, {152,112,184,144}, ICON, (CHAR *)us_icon203},
 /* 25 */ {0, {184,112,216,144}, ICON, (CHAR *)us_icon204},
 /* 26 */ {0, {8,8,24,104}, MESSAGE, N_("Export name:")},
 /* 27 */ {0, {40,280,56,424}, POPUP, x_("")},
 /* 28 */ {0, {40,160,56,276}, MESSAGE, N_("Characteristics:")},
 /* 29 */ {0, {240,8,256,120}, CHECK, N_("Always drawn")},
 /* 30 */ {0, {216,8,232,100}, CHECK, N_("Body only")},
 /* 31 */ {0, {112,216,128,264}, EDITTEXT, x_("")},
 /* 32 */ {0, {112,272,128,432}, RADIO, N_("Lambda (max 127.75)")},
 /* 33 */ {0, {136,248,152,448}, POPUP, x_("")},
 /* 34 */ {0, {136,160,152,243}, MESSAGE, N_("Text font:")},
 /* 35 */ {0, {184,160,200,231}, CHECK, N_("Italic")},
 /* 36 */ {0, {184,256,200,327}, CHECK, N_("Bold")},
 /* 37 */ {0, {184,352,200,440}, CHECK, N_("Underline")},
 /* 38 */ {0, {100,160,116,208}, MESSAGE, N_("Size")},
 /* 39 */ {0, {268,8,292,80}, BUTTON, N_("See Node")},
 /* 40 */ {0, {268,100,292,172}, BUTTON, N_("Node Info")},
 /* 41 */ {0, {160,248,176,416}, POPUP, x_("")},
 /* 42 */ {0, {160,160,176,237}, MESSAGE, N_("Rotation:")},
 /* 43 */ {0, {232,160,248,449}, MESSAGE, x_("")},
 /* 44 */ {0, {64,160,80,276}, MESSAGE, N_("Reference name:")},
 /* 45 */ {0, {64,280,80,448}, EDITTEXT, x_("")}
};
static DIALOG us_portinfodialog = {{50,75,351,534}, N_("Export Information"), 0, 45, us_portinfodialogitems, 0, 0};

/* Get Info: Inductance */
static DIALOGITEM us_inductancedialogitems[] =
{
 /*  1 */ {0, {40,168,64,232}, BUTTON, N_("OK")},
 /*  2 */ {0, {40,8,64,72}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,16,24,110}, EDITTEXT, x_("")},
 /*  4 */ {0, {8,120,24,237}, POPUP, x_("")},
 /*  5 */ {0, {40,88,64,152}, BUTTON, N_("More...")}
};
static DIALOG us_inductancedialog = {{50,75,126,321}, N_("Inductance"), 0, 5, us_inductancedialogitems, 0, 0};

/* Get Info: Multiple Object */
static DIALOGITEM us_multigetinfodialogitems[] =
{
 /*  1 */ {0, {368,124,392,204}, BUTTON, N_("OK")},
 /*  2 */ {0, {368,16,392,96}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {20,4,360,304}, SCROLLMULTI, x_("")},
 /*  4 */ {0, {0,4,16,304}, MESSAGE, N_("0 Objects:")},
 /*  5 */ {0, {28,308,44,388}, MESSAGE, N_("X Position:")},
 /*  6 */ {0, {28,392,44,472}, EDITTEXT, x_("")},
 /*  7 */ {0, {68,308,84,388}, MESSAGE, N_("Y Position:")},
 /*  8 */ {0, {68,392,84,472}, EDITTEXT, x_("")},
 /*  9 */ {0, {108,308,124,388}, MESSAGE, N_("X Size:")},
 /* 10 */ {0, {108,392,124,472}, EDITTEXT, x_("")},
 /* 11 */ {0, {148,308,164,388}, MESSAGE, N_("Y Size:")},
 /* 12 */ {0, {148,392,164,472}, EDITTEXT, x_("")},
 /* 13 */ {0, {220,308,236,388}, MESSAGE, N_("Width:")},
 /* 14 */ {0, {220,392,236,472}, EDITTEXT, x_("")},
 /* 15 */ {0, {8,308,24,472}, MESSAGE, N_("For all selected nodes:")},
 /* 16 */ {0, {200,308,216,472}, MESSAGE, N_("For all selected arcs:")},
 /* 17 */ {0, {368,232,392,312}, BUTTON, N_("Info")},
 /* 18 */ {0, {368,340,392,420}, BUTTON, N_("Remove")},
 /* 19 */ {0, {344,308,360,472}, POPUP, x_("")},
 /* 20 */ {0, {48,336,64,472}, MESSAGE, x_("")},
 /* 21 */ {0, {88,336,104,472}, MESSAGE, x_("")},
 /* 22 */ {0, {128,336,144,472}, MESSAGE, x_("")},
 /* 23 */ {0, {168,336,184,472}, MESSAGE, x_("")},
 /* 24 */ {0, {240,336,256,472}, MESSAGE, x_("")},
 /* 25 */ {0, {192,308,193,472}, DIVIDELINE, x_("")},
 /* 26 */ {0, {272,308,288,472}, MESSAGE, N_("For all selected exports:")},
 /* 27 */ {0, {264,308,265,472}, DIVIDELINE, x_("")},
 /* 28 */ {0, {292,308,308,472}, POPUP, x_("")},
 /* 29 */ {0, {316,308,317,472}, DIVIDELINE, x_("")},
 /* 30 */ {0, {324,308,340,472}, MESSAGE, N_("For everything:")}
};
static DIALOG us_multigetinfodialog = {{75,75,476,556}, N_("Multiple Object Information"), 0, 30, us_multigetinfodialogitems, 0, 0};

/* Get Info: Node */
static DIALOGITEM us_shownodedialogitems[] =
{
 /*  1 */ {0, {32,8,48,56}, MESSAGE, N_("Name:")},
 /*  2 */ {0, {8,8,24,56}, MESSAGE, N_("Type:")},
 /*  3 */ {0, {32,64,48,392}, EDITTEXT, x_("")},
 /*  4 */ {0, {8,64,24,392}, MESSAGE, x_("")},
 /*  5 */ {0, {76,80,92,176}, EDITTEXT, x_("")},
 /*  6 */ {0, {100,80,116,174}, EDITTEXT, x_("")},
 /*  7 */ {0, {56,76,72,193}, MESSAGE, x_("")},
 /*  8 */ {0, {100,8,116,72}, MESSAGE, N_("Y size:")},
 /*  9 */ {0, {76,8,92,72}, MESSAGE, N_("X size:")},
 /* 10 */ {0, {76,208,92,288}, MESSAGE, N_("X position:")},
 /* 11 */ {0, {76,296,92,392}, EDITTEXT, x_("")},
 /* 12 */ {0, {100,296,116,390}, EDITTEXT, x_("")},
 /* 13 */ {0, {100,208,116,288}, MESSAGE, N_("Y position:")},
 /* 14 */ {0, {56,292,72,392}, MESSAGE, N_("Lower-left:")},
 /* 15 */ {0, {124,8,140,72}, MESSAGE, N_("Rotation:")},
 /* 16 */ {0, {124,80,140,128}, EDITTEXT, x_("")},
 /* 17 */ {0, {124,136,140,246}, AUTOCHECK, N_("Transposed")},
 /* 18 */ {0, {124,264,140,392}, AUTOCHECK, N_("Easy to Select")},
 /* 19 */ {0, {148,32,172,104}, BUTTON, N_("More")},
 /* 20 */ {0, {148,120,172,192}, BUTTON, N_("Close")},
 /* 21 */ {0, {148,208,172,280}, BUTTON, N_("Apply")},
 /* 22 */ {0, {148,300,172,372}, DEFBUTTON, N_("OK")},
 /* 23 */ {0, {180,8,196,104}, RADIOA, N_("Expanded")},
 /* 24 */ {0, {180,112,196,216}, RADIOA, N_("Unexpanded")},
 /* 25 */ {0, {180,220,196,392}, AUTOCHECK, N_("Only Visible Inside Cell")},
 /* 26 */ {0, {229,8,245,156}, MESSAGE, x_("")},
 /* 27 */ {0, {205,8,221,156}, MESSAGE, x_("")},
 /* 28 */ {0, {205,160,221,392}, EDITTEXT, x_("")},
 /* 29 */ {0, {229,160,245,392}, POPUP, x_("")},
 /* 30 */ {0, {256,20,272,108}, RADIOB, N_("Ports:")},
 /* 31 */ {0, {256,124,272,244}, RADIOB, N_("Parameters:")},
 /* 32 */ {0, {256,260,272,380}, RADIOB, N_("Attributes")},
 /* 33 */ {0, {276,8,404,392}, SCROLL, x_("")},
 /* 34 */ {0, {416,12,432,104}, AUTOCHECK, N_("Locked")},
 /* 35 */ {0, {412,212,436,284}, BUTTON, N_("Get Info")},
 /* 36 */ {0, {412,316,436,388}, BUTTON, N_("Attributes")},
 /* 37 */ {0, {445,161,461,393}, EDITTEXT, x_("")},
 /* 38 */ {0, {469,161,485,393}, POPUP, x_("")},
 /* 39 */ {0, {493,161,509,393}, MESSAGE, x_("")},
 /* 40 */ {0, {493,9,509,161}, MESSAGE, x_("")},
 /* 41 */ {0, {469,9,485,161}, MESSAGE, x_("")},
 /* 42 */ {0, {445,9,461,157}, MESSAGE, x_("")}
};
static DIALOG us_shownodedialog = {{50,75,568,478}, N_("Node Information"), 0, 42, us_shownodedialogitems, x_("shownode"), 176};

/* special items for the "shownode" dialog: */
#define DGIN_NAME_L         1		/* Name label (message) */
#define DGIN_TYPE_L         2		/* Type label (message) */
#define DGIN_NAME           3		/* Name (edittext) */
#define DGIN_TYPE           4		/* Type (message) */
#define DGIN_XSIZE          5		/* X size (edittext) */
#define DGIN_YSIZE          6		/* Y size (edittext) */
#define DGIN_SIZE           7		/* Size information (message) */
#define DGIN_YSIZE_L        8		/* Y size label (message) */
#define DGIN_XSIZE_L        9		/* X size label (message) */
#define DGIN_XPOSITION_L   10		/* X position label (message) */
#define DGIN_XPOSITION     11		/* X position (edittext) */
#define DGIN_YPOSITION     12		/* Y position (edittext) */
#define DGIN_YPOSITION_L   13		/* Y position label (message) */
#define DGIN_POSITION      14		/* Position information (message) */
#define DGIN_ROTATTION_L   15		/* Rotation label (message) */
#define DGIN_ROTATION      16		/* Rotation (edittext) */
#define DGIN_TRANSPOSE     17		/* Transposed (autocheck) */
#define DGIN_EASYSELECT    18		/* Easy to Select (autocheck) */
#define DGIN_MOREORLESS    19		/* More/Less (button) */
#define DGIN_CLOSE         20		/* Close (button) */
#define DGIN_APPLY         21		/* Apply changes (button) */
#define DGIN_OK            22		/* OK (defbutton) */
#define DGIN_EXPANDED      23		/* expanded (radioa) */
#define DGIN_UNEXPANDED    24		/* unexpanded (radioa) */
#define DGIN_VISINSIDE     25		/* Only visible inside cell (autocheck) */
#define DGIN_SPECIAL2_L    26		/* Special line 2 title (message) */
#define DGIN_SPECIAL1_L    27		/* Special line 1 title (message) */
#define DGIN_SPECIAL1      28		/* Special line 1 value (edittext) */
#define DGIN_SPECIAL2      29		/* Special line 2 value (popup) */
#define DGIN_SEEPORTS      30		/* Show ports in list (radiob) */
#define DGIN_SEEPARAMETERS 31		/* Show parameters in list (radiob) */
#define DGIN_SEEATTRIBUTES 32		/* Show parameters in list (radiob) */
#define DGIN_CONLIST       33		/* connection list (scroll) */
#define DGIN_NODELOCKED    34		/* Node is locked (autocheck) */
#define DGIN_GETINFO       35		/* Get Info on arc/export (button) */
#define DGIN_ATTRIBUTES    36		/* Attributes (button) */
#define DGIN_PARATTR       37		/* Parameter/attribute value (edittext) */
#define DGIN_PARATTRLAN    38		/* Parameter/attribute language (popup) */
#define DGIN_PARATTREV     39		/* Parameter/attribute evaluation (message) */
#define DGIN_PARATTREV_L   40		/* Parameter/attribute evaluation title (message) */
#define DGIN_PARATTRLAN_L  41		/* Parameter/attribute language title (message) */
#define DGIN_PARATTR_L     42		/* Parameter/attribute name (message) */

/* Get Info: Outline */
static DIALOGITEM us_outlinedialogitems[] =
{
 /*  1 */ {0, {76,208,100,272}, BUTTON, N_("OK")},
 /*  2 */ {0, {20,208,44,272}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,168,192}, SCROLL, x_("")},
 /*  4 */ {0, {184,8,200,28}, MESSAGE, N_("X:")},
 /*  5 */ {0, {184,32,200,104}, EDITTEXT, x_("")},
 /*  6 */ {0, {216,8,232,28}, MESSAGE, N_("Y:")},
 /*  7 */ {0, {216,32,232,104}, EDITTEXT, x_("")},
 /*  8 */ {0, {208,160,232,272}, BUTTON, N_("Duplicate Point")},
 /*  9 */ {0, {176,160,200,272}, BUTTON, N_("Delete Point")},
 /* 10 */ {0, {132,208,156,272}, BUTTON, N_("Apply")}
};
static DIALOG us_outlinedialog = {{50,75,291,356}, N_("Outline Information"), 0, 10, us_outlinedialogitems, 0, 0};

/* Get Info: Resistance */
static DIALOGITEM us_resistancedialogitems[] =
{
 /*  1 */ {0, {40,192,64,256}, BUTTON, N_("OK")},
 /*  2 */ {0, {40,16,64,80}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,24,24,118}, EDITTEXT, x_("")},
 /*  4 */ {0, {8,128,24,239}, POPUP, x_("")},
 /*  5 */ {0, {40,104,64,168}, BUTTON, N_("More...")}
};
static DIALOG us_resistancedialog = {{50,75,124,345}, N_("Resistance"), 0, 5, us_resistancedialogitems, 0, 0};

/* Get Info: Text */
static DIALOGITEM us_showtextdialogitems[] =
{
 /*  1 */ {0, {480,236,504,308}, BUTTON, N_("OK")},
 /*  2 */ {0, {480,20,504,92}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {200,8,216,64}, RADIO, N_("Left")},
 /*  4 */ {0, {40,8,88,340}, EDITTEXT, x_("")},
 /*  5 */ {0, {332,64,348,112}, EDITTEXT, x_("")},
 /*  6 */ {0, {136,8,152,80}, RADIO, N_("Center")},
 /*  7 */ {0, {152,8,168,80}, RADIO, N_("Bottom")},
 /*  8 */ {0, {168,8,184,64}, RADIO, N_("Top")},
 /*  9 */ {0, {184,8,200,72}, RADIO, N_("Right")},
 /* 10 */ {0, {216,8,232,104}, RADIO, N_("Lower right")},
 /* 11 */ {0, {232,8,248,104}, RADIO, N_("Lower left")},
 /* 12 */ {0, {248,8,264,104}, RADIO, N_("Upper right")},
 /* 13 */ {0, {264,8,280,96}, RADIO, N_("Upper left")},
 /* 14 */ {0, {280,8,296,80}, RADIO, N_("Boxed")},
 /* 15 */ {0, {332,120,348,280}, RADIO, N_("Points (max 63)")},
 /* 16 */ {0, {120,16,136,110}, MESSAGE, N_("Text corner:")},
 /* 17 */ {0, {164,244,180,324}, EDITTEXT, x_("")},
 /* 18 */ {0, {188,244,204,324}, EDITTEXT, x_("")},
 /* 19 */ {0, {164,152,180,241}, MESSAGE, N_("X offset:")},
 /* 20 */ {0, {188,152,204,241}, MESSAGE, N_("Y offset:")},
 /* 21 */ {0, {136,112,168,144}, ICON, (CHAR *)us_icon200},
 /* 22 */ {0, {168,112,200,144}, ICON, (CHAR *)us_icon201},
 /* 23 */ {0, {200,112,232,144}, ICON, (CHAR *)us_icon202},
 /* 24 */ {0, {232,112,264,144}, ICON, (CHAR *)us_icon203},
 /* 25 */ {0, {264,112,296,144}, ICON, (CHAR *)us_icon204},
 /* 26 */ {0, {4,8,36,340}, MESSAGE, x_("")},
 /* 27 */ {0, {128,194,148,294}, BUTTON, N_("Edit Text")},
 /* 28 */ {0, {236,152,252,340}, CHECK, N_("Visible only inside cell")},
 /* 29 */ {0, {96,8,112,340}, MESSAGE, x_("")},
 /* 30 */ {0, {212,232,228,340}, POPUP, x_("")},
 /* 31 */ {0, {212,152,228,231}, MESSAGE, N_("Language:")},
 /* 32 */ {0, {308,8,324,88}, MESSAGE, N_("Show:")},
 /* 33 */ {0, {308,92,324,280}, POPUP, x_("")},
 /* 34 */ {0, {356,64,372,112}, EDITTEXT, x_("")},
 /* 35 */ {0, {356,120,372,280}, RADIO, N_("Lambda (max 127.75)")},
 /* 36 */ {0, {380,92,396,280}, POPUP, x_("")},
 /* 37 */ {0, {380,8,396,87}, MESSAGE, N_("Type face:")},
 /* 38 */ {0, {404,8,420,91}, CHECK, N_("Italic")},
 /* 39 */ {0, {404,104,420,187}, CHECK, N_("Bold")},
 /* 40 */ {0, {404,196,420,279}, CHECK, N_("Underline")},
 /* 41 */ {0, {344,8,360,56}, MESSAGE, N_("Size")},
 /* 42 */ {0, {268,260,292,332}, BUTTON, N_("Node Info")},
 /* 43 */ {0, {268,160,292,232}, BUTTON, N_("See Node")},
 /* 44 */ {0, {428,92,444,280}, POPUP, x_("")},
 /* 45 */ {0, {428,8,444,87}, MESSAGE, N_("Rotation:")},
 /* 46 */ {0, {452,92,468,280}, POPUP, x_("")},
 /* 47 */ {0, {452,8,468,87}, MESSAGE, N_("Units:")}
};
static DIALOG us_showtextdialog = {{50,75,563,424}, N_("Information on Highlighted Text"), 0, 47, us_showtextdialogitems, 0, 0};

/* Get Info: Transistor Area */
static DIALOGITEM us_areadialogitems[] =
{
 /*  1 */ {0, {40,184,64,248}, BUTTON, N_("OK")},
 /*  2 */ {0, {40,8,64,72}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,80,24,174}, EDITTEXT, x_("")},
 /*  4 */ {0, {40,96,64,160}, BUTTON, N_("More...")}
};
static DIALOG us_areadialog = {{50,75,124,333}, N_("Area"), 0, 4, us_areadialogitems, 0, 0};

/* Get Info: Transistor Width/Length */
static DIALOGITEM us_widlendialogitems[] =
{
 /*  1 */ {0, {112,184,136,248}, BUTTON, N_("OK")},
 /*  2 */ {0, {112,8,136,72}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,92,24,248}, EDITTEXT, x_("")},
 /*  4 */ {0, {8,8,24,83}, MESSAGE, N_("Width:")},
 /*  5 */ {0, {60,92,76,248}, EDITTEXT, x_("")},
 /*  6 */ {0, {60,8,76,84}, MESSAGE, N_("Length:")},
 /*  7 */ {0, {112,96,136,160}, BUTTON, N_("More")},
 /*  8 */ {0, {32,92,48,247}, POPUP, x_("")},
 /*  9 */ {0, {84,92,100,247}, POPUP, x_("")}
};
static DIALOG us_widlendialog = {{50,75,195,332}, N_("Transistor Information"), 0, 9, us_widlendialogitems, 0, 0};

/* Global Signal */
static DIALOGITEM us_globdialogitems[] =
{
 /*  1 */ {0, {100,196,124,276}, BUTTON, N_("OK")},
 /*  2 */ {0, {100,12,124,92}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,172}, MESSAGE, N_("Global signal name:")},
 /*  4 */ {0, {32,8,48,280}, EDITTEXT, x_("")},
 /*  5 */ {0, {64,8,80,140}, MESSAGE, N_("Characteristics:")},
 /*  6 */ {0, {64,144,80,284}, POPUP, x_("")},
 /*  7 */ {0, {100,104,124,184}, BUTTON, N_("More")}
};
static DIALOG us_globdialog = {{75,75,208,368}, N_("Global Signal"), 0, 7, us_globdialogitems, 0, 0};

/* Grid Options */
static DIALOGITEM us_griddialogitems[] =
{
 /*  1 */ {0, {116,384,140,448}, BUTTON, N_("OK")},
 /*  2 */ {0, {116,12,140,76}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,8,48,269}, MESSAGE, N_("Grid dot spacing (current window):")},
 /*  4 */ {0, {32,272,48,352}, EDITTEXT, x_("")},
 /*  5 */ {0, {8,272,24,366}, MESSAGE, N_("Horizontal:")},
 /*  6 */ {0, {32,372,48,452}, EDITTEXT, x_("")},
 /*  7 */ {0, {60,8,76,269}, MESSAGE, N_("Default grid spacing (new windows):")},
 /*  8 */ {0, {60,272,76,352}, EDITTEXT, x_("")},
 /*  9 */ {0, {8,372,24,466}, MESSAGE, N_("Vertical:")},
 /* 10 */ {0, {60,372,76,452}, EDITTEXT, x_("")},
 /* 11 */ {0, {88,8,104,269}, MESSAGE, N_("Distance between bold dots:")},
 /* 12 */ {0, {88,272,104,352}, EDITTEXT, x_("")},
 /* 13 */ {0, {120,132,136,329}, CHECK, N_("Align grid with circuitry")},
 /* 14 */ {0, {88,372,104,452}, EDITTEXT, x_("")}
};
static DIALOG us_griddialog = {{50,75,199,550}, N_("Grid Options"), 0, 14, us_griddialogitems, 0, 0};

/* Help */
static DIALOGITEM us_helpdialogitems[] =
{
 /*  1 */ {0, {288,376,312,440}, BUTTON, N_("OK")},
 /*  2 */ {0, {8,32,24,91}, MESSAGE, N_("Topics:")},
 /*  3 */ {0, {8,192,280,636}, SCROLL, x_("")},
 /*  4 */ {0, {24,8,309,177}, SCROLL, x_("")}
};
static DIALOG us_helpdialog = {{50,75,378,720}, N_("Help"), 0, 4, us_helpdialogitems, 0, 0};

/* Highlight Layer */
static DIALOGITEM us_highlightlayerdialogitems[] =
{
 /*  1 */ {0, {108,184,132,248}, BUTTON, N_("Done")},
 /*  2 */ {0, {12,184,36,248}, BUTTON, N_("None")},
 /*  3 */ {0, {8,8,136,170}, SCROLL, x_("")}
};
static DIALOG us_highlightlayerdialog = {{50,75,195,334}, N_("Layer to Highlight"), 0, 3, us_highlightlayerdialogitems, 0, 0};

/* Icon Options */
static DIALOGITEM us_iconoptdialogitems[] =
{
 /*  1 */ {0, {216,309,240,389}, BUTTON, N_("OK")},
 /*  2 */ {0, {180,308,204,388}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,88}, MESSAGE, N_("Inputs on:")},
 /*  4 */ {0, {8,92,24,192}, POPUP, x_("")},
 /*  5 */ {0, {32,8,48,88}, MESSAGE, N_("Outputs on:")},
 /*  6 */ {0, {32,92,48,192}, POPUP, x_("")},
 /*  7 */ {0, {56,8,72,88}, MESSAGE, N_("Bidir. on:")},
 /*  8 */ {0, {56,92,72,192}, POPUP, x_("")},
 /*  9 */ {0, {8,208,24,288}, MESSAGE, N_("Power on:")},
 /* 10 */ {0, {8,292,24,392}, POPUP, x_("")},
 /* 11 */ {0, {32,208,48,288}, MESSAGE, N_("Ground on:")},
 /* 12 */ {0, {32,292,48,392}, POPUP, x_("")},
 /* 13 */ {0, {56,208,72,288}, MESSAGE, N_("Clock on:")},
 /* 14 */ {0, {56,292,72,392}, POPUP, x_("")},
 /* 15 */ {0, {88,8,104,116}, CHECK, N_("Draw leads")},
 /* 16 */ {0, {144,8,160,160}, MESSAGE, N_("Export location:")},
 /* 17 */ {0, {144,164,160,264}, POPUP, x_("")},
 /* 18 */ {0, {168,8,184,160}, MESSAGE, N_("Export style:")},
 /* 19 */ {0, {168,164,184,264}, POPUP, x_("")},
 /* 20 */ {0, {112,8,128,116}, CHECK, N_("Draw body")},
 /* 21 */ {0, {113,264,129,340}, EDITTEXT, x_("")},
 /* 22 */ {0, {192,8,208,160}, MESSAGE, N_("Export technology:")},
 /* 23 */ {0, {192,164,208,264}, POPUP, x_("")},
 /* 24 */ {0, {252,8,268,184}, MESSAGE, N_("Instance location:")},
 /* 25 */ {0, {252,188,268,288}, POPUP, x_("")},
 /* 26 */ {0, {224,8,240,184}, CHECK, N_("Reverse export order")},
 /* 27 */ {0, {88,148,104,248}, MESSAGE, N_("Lead length:")},
 /* 28 */ {0, {88,264,104,340}, EDITTEXT, x_("")},
 /* 29 */ {0, {112,148,128,248}, MESSAGE, N_("Lead spacing:")}
};
static DIALOG us_iconoptdialog = {{75,75,352,476}, N_("Icon Options"), 0, 29, us_iconoptdialogitems, 0, 0};

/* Java Options */
static DIALOGITEM us_javaoptdialogitems[] =
{
 /*  1 */ {0, {88,112,112,192}, BUTTON, N_("OK")},
 /*  2 */ {0, {88,16,112,96}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,172}, CHECK, N_("Disable compiler")},
 /*  4 */ {0, {32,8,48,172}, CHECK, N_("Disable evaluation")},
 /*  5 */ {0, {56,8,72,172}, CHECK, N_("Enable Jose")}
};
static DIALOG us_javaoptdialog = {{75,75,196,277}, N_("Java Options"), 0, 5, us_javaoptdialogitems, 0, 0};

/* Layer Display Options */
static DIALOGITEM us_patterndialogitems[] =
{
 /*  1 */ {0, {204,296,228,360}, BUTTON, N_("OK")},
 /*  2 */ {0, {164,296,188,360}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {4,8,20,184}, MESSAGE, N_("Appearance of layer:")},
 /*  4 */ {0, {112,16,368,272}, USERDRAWN, x_("")},
 /*  5 */ {0, {4,184,20,368}, POPUP, x_("")},
 /*  6 */ {0, {64,184,80,336}, CHECK, N_("Outline Pattern")},
 /*  7 */ {0, {64,8,80,160}, CHECK, N_("Use Stipple Pattern")},
 /*  8 */ {0, {28,8,60,40}, ICON, (CHAR *)us_icon300},
 /*  9 */ {0, {28,40,60,72}, ICON, (CHAR *)us_icon301},
 /* 10 */ {0, {28,72,60,104}, ICON, (CHAR *)us_icon302},
 /* 11 */ {0, {28,104,60,136}, ICON, (CHAR *)us_icon303},
 /* 12 */ {0, {28,136,60,168}, ICON, (CHAR *)us_icon304},
 /* 13 */ {0, {28,168,60,200}, ICON, (CHAR *)us_icon305},
 /* 14 */ {0, {28,200,60,232}, ICON, (CHAR *)us_icon306},
 /* 15 */ {0, {28,232,60,264}, ICON, (CHAR *)us_icon307},
 /* 16 */ {0, {28,264,60,296}, ICON, (CHAR *)us_icon308},
 /* 17 */ {0, {28,296,60,328}, ICON, (CHAR *)us_icon309},
 /* 18 */ {0, {28,328,60,360}, ICON, (CHAR *)us_icon310},
 /* 19 */ {0, {88,184,104,368}, POPUP, x_("")},
 /* 20 */ {0, {88,8,104,184}, MESSAGE, N_("Color:")}
};
static DIALOG us_patterndialog = {{50,75,427,453}, N_("Layer Display Options"), 0, 20, us_patterndialogitems, 0, 0};

/* Layer Visibility */
static DIALOGITEM us_visiblelaydialogitems[] =
{
 /*  1 */ {0, {32,8,208,222}, SCROLL, x_("")},
 /*  2 */ {0, {8,8,24,82}, MESSAGE, N_("Layer set:")},
 /*  3 */ {0, {8,88,24,294}, POPUP, x_("")},
 /*  4 */ {0, {40,230,56,382}, MESSAGE, N_("Text visibility options:")},
 /*  5 */ {0, {60,242,76,394}, AUTOCHECK, N_("Node text")},
 /*  6 */ {0, {80,242,96,394}, AUTOCHECK, N_("Arc text")},
 /*  7 */ {0, {100,242,116,394}, AUTOCHECK, N_("Port text")},
 /*  8 */ {0, {120,242,136,394}, AUTOCHECK, N_("Export text")},
 /*  9 */ {0, {140,242,156,394}, AUTOCHECK, N_("Nonlayout text")},
 /* 10 */ {0, {160,242,176,394}, AUTOCHECK, N_("Instance names")},
 /* 11 */ {0, {180,242,196,394}, AUTOCHECK, N_("Cell text")},
 /* 12 */ {0, {212,16,228,222}, MESSAGE, N_("Click to change visibility.")},
 /* 13 */ {0, {228,16,244,222}, MESSAGE, N_("Marked layers are visible.")},
 /* 14 */ {0, {252,8,276,108}, BUTTON, N_("All Visible")},
 /* 15 */ {0, {252,122,276,222}, BUTTON, N_("All Invisible")},
 /* 16 */ {0, {214,316,238,388}, BUTTON, N_("Done")},
 /* 17 */ {0, {250,316,274,388}, DEFBUTTON, N_("Apply")}
};
static DIALOG us_visiblelaydialog = {{50,75,335,479}, N_("Layer Visibility"), 0, 17, us_visiblelaydialogitems, x_("visiblelay"), 0};

/* special items for the "visiblelay" dialog: */
#define DVSL_LAYERLIST     1		/* Layers (scroll) */
#define DVSL_LAYERSET_L    2		/* Layer set (message) */
#define DVSL_LAYERSET      3		/* Set of layers (popup) */
#define DVSL_OPTIONS_L     4		/* Text visibility options (message) */
#define DVSL_NODETEXT      5		/* Show node text (autocheck) */
#define DVSL_ARCTEXT       6		/* Show arc text (autocheck) */
#define DVSL_PORTTEXT      7		/* Show port text (autocheck) */
#define DVSL_EXPORTTEXT    8		/* Show export text (autocheck) */
#define DVSL_NONLAYTEXT    9		/* Show nonlayout text (autocheck) */
#define DVSL_INSTTEXT     10		/* Show instance text (autocheck) */
#define DVSL_CELLTEXT     11		/* Show cell text (autocheck) */
#define DVSL_CLICK_L      12		/* Click to change (message) */
#define DVSL_MARKED_L     13		/* Marked layers (message) */
#define DVSL_ALLVISIBLE   14		/* Make all visible (button) */
#define DVSL_ALLINVISIBLE 15		/* Make all invisible (button) */
#define DVSL_DONE         16		/* Done (button) */
#define DVSL_APPLY        17		/* Apply (defbutton) */

/* Library Options */
static DIALOGITEM io_liboptdialogitems[] =
{
 /*  1 */ {0, {124,148,148,228}, BUTTON, N_("OK")},
 /*  2 */ {0, {124,16,148,96}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,236}, RADIO, N_("No backup of library files")},
 /*  4 */ {0, {32,8,48,236}, RADIO, N_("Backup of last library file")},
 /*  5 */ {0, {56,8,72,236}, RADIO, N_("Backup history of library files")},
 /*  6 */ {0, {92,8,108,236}, CHECK, N_("Check database after write")}
};
static DIALOG io_liboptdialog = {{75,75,232,321}, N_("Library Options"), 0, 6, io_liboptdialogitems, 0, 0};

/* Logical Effort Options */
static DIALOGITEM le_leoptionsdialogitems[] =
{
 /*  1 */ {0, {196,204,220,268}, BUTTON, N_("OK")},
 /*  2 */ {0, {132,204,156,268}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,160}, MESSAGE, N_("Maximum Stage Gain")},
 /*  4 */ {0, {8,168,24,220}, EDITTEXT, x_("")},
 /*  5 */ {0, {36,8,52,244}, CHECK, N_("Display intermediate capacitances")},
 /*  6 */ {0, {64,8,80,192}, CHECK, N_("Highlight components")},
 /*  7 */ {0, {112,8,220,180}, SCROLL, x_("")},
 /*  8 */ {0, {224,8,240,92}, MESSAGE, N_("Wire ratio:")},
 /*  9 */ {0, {224,96,240,180}, EDITTEXT, x_("")},
 /* 10 */ {0, {92,8,108,180}, MESSAGE, N_("Wire ratio for each layer:")}
};
static DIALOG le_leoptionsdialog = {{75,75,324,352}, N_("Logical Effort Options"), 0, 10, le_leoptionsdialogitems, 0, 0};

/* Move Objects By */
static DIALOGITEM us_movebydialogitems[] =
{
 /*  1 */ {0, {64,96,88,160}, BUTTON, N_("OK")},
 /*  2 */ {0, {64,8,88,72}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,48,24,142}, EDITTEXT, x_("")},
 /*  4 */ {0, {8,20,24,42}, MESSAGE, N_("dX:")},
 /*  5 */ {0, {32,48,48,142}, EDITTEXT, x_("")},
 /*  6 */ {0, {32,20,48,42}, MESSAGE, N_("dY:")}
};
static DIALOG us_movebydialog = {{50,75,147,244}, N_("Move By Amount"), 0, 6, us_movebydialogitems, 0, 0};

/* NCC Cell Function */
static DIALOGITEM net_nccfundialogitems[] =
{
 /*  1 */ {0, {308,184,332,264}, BUTTON, N_("OK")},
 /*  2 */ {0, {308,24,332,104}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {4,116,20,288}, POPUP, x_("")},
 /*  4 */ {0, {4,4,20,112}, MESSAGE, N_("Library:")},
 /*  5 */ {0, {24,4,256,288}, SCROLL, x_("")},
 /*  6 */ {0, {260,116,276,288}, POPUP, x_("")},
 /*  7 */ {0, {260,4,276,112}, MESSAGE, N_("Function:")},
 /*  8 */ {0, {280,4,296,288}, CHECK, N_("Override Expansion on Cells with Functions")}
};
static DIALOG net_nccfundialog = {{75,75,416,373}, N_("Assign primitive functions to cells"), 0, 8, net_nccfundialogitems, 0, 0};

/* NCC Control */
static DIALOGITEM net_nccoptionsdialogitems[] =
{
 /*  1 */ {0, {488,224,512,312}, BUTTON, N_("Do NCC")},
 /*  2 */ {0, {488,8,512,72}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {364,4,380,200}, CHECK, N_("Recurse through hierarchy")},
 /*  4 */ {0, {244,4,260,220}, CHECK, N_("Check export names")},
 /*  5 */ {0, {488,96,512,201}, BUTTON, N_("Preanalysis")},
 /*  6 */ {0, {432,16,448,208}, BUTTON, N_("NCC Debugging options...")},
 /*  7 */ {0, {124,4,140,220}, CHECK, N_("Merge parallel components")},
 /*  8 */ {0, {220,4,236,220}, CHECK, N_("Ignore power and ground")},
 /*  9 */ {0, {76,4,92,217}, MESSAGE, N_("For all cells:")},
 /* 10 */ {0, {191,228,387,400}, SCROLL, x_("")},
 /* 11 */ {0, {69,220,480,221}, DIVIDELINE, x_("")},
 /* 12 */ {0, {28,312,44,352}, BUTTON, N_("Set")},
 /* 13 */ {0, {4,312,20,352}, BUTTON, N_("Set")},
 /* 14 */ {0, {28,136,44,305}, EDITTEXT, x_("")},
 /* 15 */ {0, {124,232,140,281}, RADIO, N_("Yes")},
 /* 16 */ {0, {124,284,140,329}, RADIO, N_("No")},
 /* 17 */ {0, {124,332,140,396}, RADIO, N_("Default")},
 /* 18 */ {0, {316,20,332,164}, MESSAGE, N_("Size tolerance (amt):")},
 /* 19 */ {0, {316,168,332,216}, EDITTEXT, x_("")},
 /* 20 */ {0, {68,4,69,401}, DIVIDELINE, x_("")},
 /* 21 */ {0, {76,228,92,400}, MESSAGE, N_("Individual cell overrides:")},
 /* 22 */ {0, {172,12,188,209}, BUTTON, N_("Clear NCC dates this library")},
 /* 23 */ {0, {268,4,284,220}, CHECK, N_("Check component sizes")},
 /* 24 */ {0, {4,136,20,305}, EDITTEXT, x_("")},
 /* 25 */ {0, {28,4,44,132}, MESSAGE, N_("With cell:")},
 /* 26 */ {0, {4,4,20,132}, MESSAGE, N_("Compare cell:")},
 /* 27 */ {0, {292,20,308,164}, MESSAGE, N_("Size tolerance (%):")},
 /* 28 */ {0, {292,168,308,216}, EDITTEXT, x_("")},
 /* 29 */ {0, {488,336,512,400}, BUTTON, N_("Save")},
 /* 30 */ {0, {480,4,481,400}, DIVIDELINE, x_("")},
 /* 31 */ {0, {412,240,428,388}, BUTTON, N_("Remove all overrides")},
 /* 32 */ {0, {148,232,164,281}, RADIO, N_("Yes")},
 /* 33 */ {0, {148,284,164,329}, RADIO, N_("No")},
 /* 34 */ {0, {148,332,164,396}, RADIO, N_("Default")},
 /* 35 */ {0, {148,4,164,220}, CHECK, N_("Merge series transistors")},
 /* 36 */ {0, {100,232,116,281}, RADIO, N_("Yes")},
 /* 37 */ {0, {100,284,116,329}, RADIO, N_("No")},
 /* 38 */ {0, {100,332,116,396}, RADIO, N_("Default")},
 /* 39 */ {0, {100,4,116,220}, CHECK, N_("Expand hierarchy")},
 /* 40 */ {0, {171,228,187,400}, POPUP, x_("")},
 /* 41 */ {0, {392,240,408,388}, BUTTON, N_("List all overrides")},
 /* 42 */ {0, {456,4,472,216}, CHECK, N_("Show 'NCCMatch' tags")},
 /* 43 */ {0, {4,356,20,400}, BUTTON, N_("Next")},
 /* 44 */ {0, {28,356,44,400}, BUTTON, N_("Next")},
 /* 45 */ {0, {196,12,212,209}, BUTTON, N_("Clear NCC dates all libraries")},
 /* 46 */ {0, {48,4,64,400}, MESSAGE, x_("")},
 /* 47 */ {0, {340,4,356,220}, CHECK, N_("Allow no-component nets")},
 /* 48 */ {0, {460,232,476,400}, BUTTON, N_("Remove all forced matches")},
 /* 49 */ {0, {440,232,456,400}, BUTTON, N_("List all forced matches")},
 /* 50 */ {0, {388,4,404,216}, POPUP, x_("")}
};
static DIALOG net_nccoptionsdialog = {{50,75,571,486}, N_("Network Consistency Checker"), 0, 50, net_nccoptionsdialogitems, 0, 0};

/* NCC Debugging */
static DIALOGITEM net_nccdebdialogitems[] =
{
 /*  1 */ {0, {216,217,240,297}, BUTTON, N_("OK")},
 /*  2 */ {0, {216,60,240,140}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {4,4,20,332}, CHECK, N_("Enable local processing after match")},
 /*  4 */ {0, {28,4,44,332}, CHECK, N_("Enable focus on 'fresh' symmetry groups")},
 /*  5 */ {0, {76,4,92,332}, CHECK, N_("Disable unimportant disambiguation messages")},
 /*  6 */ {0, {112,4,128,332}, CHECK, N_("Show gemini iterations graphically")},
 /*  7 */ {0, {136,4,152,332}, CHECK, N_("Show gemini iterations textually")},
 /*  8 */ {0, {160,4,176,332}, CHECK, N_("Show matching progress graphically")},
 /*  9 */ {0, {52,4,68,332}, CHECK, N_("Enable focus on 'promising' symmetry groups")},
 /* 10 */ {0, {184,4,200,332}, CHECK, N_("Show gemini statistics")}
};
static DIALOG net_nccdebdialog = {{75,75,324,417}, N_("NCC Debugging Settings"), 0, 10, net_nccdebdialogitems, 0, 0};

/* NCC preanalysis */
static DIALOGITEM net_nccpredialogitems[] =
{
 /*  1 */ {0, {8,315,24,615}, MESSAGE, N_("Cell2")},
 /*  2 */ {0, {28,8,384,308}, SCROLL, x_("")},
 /*  3 */ {0, {8,8,24,308}, MESSAGE, N_("Cell1")},
 /*  4 */ {0, {28,315,384,615}, SCROLL, x_("")},
 /*  5 */ {0, {392,244,408,392}, RADIOA, N_("Components")},
 /*  6 */ {0, {416,20,432,212}, AUTOCHECK, N_("Tie lists vertically")},
 /*  7 */ {0, {416,244,432,392}, RADIOA, N_("Networks")},
 /*  8 */ {0, {392,20,408,212}, AUTOCHECK, N_("Show only differences")},
 /*  9 */ {0, {400,524,424,604}, DEFBUTTON, N_("Close")},
 /* 10 */ {0, {400,420,424,500}, BUTTON, N_("Compare")}
};
static DIALOG net_nccpredialog = {{75,75,516,700}, N_("NCC Preanalysis Results"), 0, 10, net_nccpredialogitems, x_("nccpre"), 0};

/* special items for the "nccpre" dialog: */
#define DNCP_CELL2NAME      1		/* Cell 2 name (message) */
#define DNCP_CELL1LIST      2		/* Cell 1 list (scroll) */
#define DNCP_CELL1NAME      3		/* Cell 1 name (message) */
#define DNCP_CELL2LIST      4		/* Cell 2 list (scroll) */
#define DNCP_SHOWCOMPS      5		/* Show components (radioa) */
#define DNCP_TIEVERTICALLY  6		/* Tie lists vertically (autocheck) */
#define DNCP_SHOWNETS       7		/* Show networks (radioa) */
#define DNCP_SHOWDIFFS      8		/* Show only differences (autocheck) */
#define DNCP_CLOSE          9		/* Close (defbutton) */
#define DNCP_COMPARE       10		/* Compare (button) */

/* NCC warning */
static DIALOGITEM net_nccwarndialogitems[] =
{
 /*  1 */ {0, {276,340,300,480}, BUTTON, N_("Show Preanalysis")},
 /*  2 */ {0, {244,340,268,480}, BUTTON, N_("Stop Now")},
 /*  3 */ {0, {308,340,332,480}, BUTTON, N_("Do Full NCC")},
 /*  4 */ {0, {8,8,24,512}, MESSAGE, x_("")},
 /*  5 */ {0, {28,8,236,512}, SCROLL, x_("")},
 /*  6 */ {0, {248,56,264,332}, MESSAGE, N_("You may stop the NCC now:")},
 /*  7 */ {0, {280,56,296,332}, MESSAGE, N_("You may request additional detail:")},
 /*  8 */ {0, {312,56,328,332}, MESSAGE, N_("You may continue with NCC:")}
};
static DIALOG net_nccwarndialog = {{75,75,416,597}, N_("NCC Differences Have Been Found"), 0, 8, net_nccwarndialogitems, 0, 0};

/* Network Options */
static DIALOGITEM net_optionsdialogitems[] =
{
 /*  1 */ {0, {288,148,312,212}, BUTTON, N_("OK")},
 /*  2 */ {0, {288,24,312,88}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {28,20,44,226}, CHECK, N_("Unify Power and Ground")},
 /*  4 */ {0, {52,20,68,226}, CHECK, N_("Unify all like-named nets")},
 /*  5 */ {0, {4,4,20,165}, MESSAGE, N_("Network numbering:")},
 /*  6 */ {0, {228,36,244,201}, RADIO, N_("Ascending (0:N)")},
 /*  7 */ {0, {252,36,268,201}, RADIO, N_("Descending (N:0)")},
 /*  8 */ {0, {180,20,196,177}, MESSAGE, N_("Default starting index:")},
 /*  9 */ {0, {180,180,196,226}, POPUP, x_("")},
 /* 10 */ {0, {148,4,149,246}, DIVIDELINE, x_("")},
 /* 11 */ {0, {204,20,220,177}, MESSAGE, N_("Default order:")},
 /* 12 */ {0, {156,4,172,177}, MESSAGE, N_("For busses:")},
 /* 13 */ {0, {76,20,92,226}, CHECK, N_("Ignore Resistors")},
 /* 14 */ {0, {100,20,116,246}, MESSAGE, N_("Unify Networks that start with:")},
 /* 15 */ {0, {124,32,140,246}, EDITTEXT, x_("")}
};
static DIALOG net_optionsdialog = {{50,75,371,331}, N_("Network Options"), 0, 15, net_optionsdialogitems, 0, 0};

/* New Arc Options */
static DIALOGITEM us_defarcdialogitems[] =
{
 /*  1 */ {0, {184,304,208,376}, BUTTON, N_("OK")},
 /*  2 */ {0, {184,64,208,136}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {4,140,20,420}, POPUP, x_("")},
 /*  4 */ {0, {4,4,20,136}, RADIO, N_("Defaults for arc:")},
 /*  5 */ {0, {32,4,48,164}, RADIO, N_("Defaults for all arcs")},
 /*  6 */ {0, {152,360,168,412}, BUTTON, N_("Set pin")},
 /*  7 */ {0, {72,8,88,64}, CHECK, N_("Rigid")},
 /*  8 */ {0, {72,104,88,204}, CHECK, N_("Fixed-angle")},
 /*  9 */ {0, {72,216,88,288}, CHECK, N_("Slidable")},
 /* 10 */ {0, {96,8,112,84}, CHECK, N_("Negated")},
 /* 11 */ {0, {96,104,112,196}, CHECK, N_("Directional")},
 /* 12 */ {0, {96,216,112,336}, CHECK, N_("Ends extended")},
 /* 13 */ {0, {32,228,52,420}, BUTTON, N_("Reset to initial defaults")},
 /* 14 */ {0, {128,64,144,152}, EDITTEXT, x_("")},
 /* 15 */ {0, {128,8,144,56}, MESSAGE, N_("Width:")},
 /* 16 */ {0, {128,176,144,224}, MESSAGE, N_("Angle:")},
 /* 17 */ {0, {128,232,144,296}, EDITTEXT, x_("")},
 /* 18 */ {0, {152,8,168,39}, MESSAGE, N_("Pin:")},
 /* 19 */ {0, {152,40,168,348}, MESSAGE, x_("")}
};
static DIALOG us_defarcdialog = {{50,75,267,505}, N_("New Arc Options"), 0, 19, us_defarcdialogitems, 0, 0};

/* New Cell */
static DIALOGITEM us_newcelldialogitems[] =
{
 /*  1 */ {0, {56,304,80,368}, BUTTON, N_("OK")},
 /*  2 */ {0, {56,12,80,76}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,160,24,367}, EDITTEXT, x_("")},
 /*  4 */ {0, {8,8,24,157}, MESSAGE, N_("Name of new cell:")},
 /*  5 */ {0, {32,160,48,367}, POPUP, x_("")},
 /*  6 */ {0, {32,56,48,149}, MESSAGE, N_("Cell view:")},
 /*  7 */ {0, {60,84,78,297}, CHECK, N_("Make new window for cell")}
};
static DIALOG us_newcelldialog = {{350,75,445,455}, N_("New Cell Creation"), 0, 7, us_newcelldialogitems, 0, 0};

/* New Node Options */
static DIALOGITEM us_defnodedialogitems[] =
{
 /*  1 */ {0, {384,352,408,416}, BUTTON, N_("OK")},
 /*  2 */ {0, {344,352,368,416}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,208,48,318}, EDITTEXT, x_("")},
 /*  4 */ {0, {32,24,48,205}, MESSAGE, N_("X size of new primitives:")},
 /*  5 */ {0, {56,24,72,205}, MESSAGE, N_("Y size of new primitives:")},
 /*  6 */ {0, {56,208,72,318}, EDITTEXT, x_("")},
 /*  7 */ {0, {8,4,24,142}, MESSAGE, N_("For primitive:")},
 /*  8 */ {0, {8,144,24,354}, POPUP, x_("")},
 /*  9 */ {0, {164,24,180,230}, MESSAGE, N_("Rotation of new nodes:")},
 /* 10 */ {0, {164,240,180,293}, EDITTEXT, x_("")},
 /* 11 */ {0, {164,300,180,405}, CHECK, N_("Transposed")},
 /* 12 */ {0, {132,4,133,422}, DIVIDELINE, x_("")},
 /* 13 */ {0, {140,4,156,152}, MESSAGE, N_("For all nodes:")},
 /* 14 */ {0, {188,24,204,338}, CHECK, N_("Disallow modification of locked primitives")},
 /* 15 */ {0, {212,24,228,338}, CHECK, N_("Move after Duplicate")},
 /* 16 */ {0, {236,24,252,338}, CHECK, N_("Duplicate/Array/Extract copies exports")},
 /* 17 */ {0, {108,40,124,246}, MESSAGE, N_("Rotation of new nodes:")},
 /* 18 */ {0, {108,256,124,309}, EDITTEXT, x_("")},
 /* 19 */ {0, {108,317,124,422}, CHECK, N_("Transposed")},
 /* 20 */ {0, {84,24,100,249}, CHECK, N_("Override default orientation")},
 /* 21 */ {0, {292,268,308,309}, EDITTEXT, x_("")},
 /* 22 */ {0, {337,20,453,309}, SCROLL, x_("")},
 /* 23 */ {0, {316,20,332,309}, MESSAGE, N_("Primitive function abbreviations:")},
 /* 24 */ {0, {460,20,476,165}, MESSAGE, N_("Function:")},
 /* 25 */ {0, {460,168,476,309}, EDITTEXT, x_("")},
 /* 26 */ {0, {260,4,261,422}, DIVIDELINE, x_("")},
 /* 27 */ {0, {268,4,284,185}, MESSAGE, N_("Node naming:")},
 /* 28 */ {0, {292,20,308,265}, MESSAGE, N_("Length of cell abbreviations:")},
 /* 29 */ {0, {261,332,476,333}, DIVIDELINE, x_("")}
};
static DIALOG us_defnodedialog = {{50,75,535,507}, N_("New Node Options"), 0, 29, us_defnodedialogitems, 0, 0};

/* New Pure Layer Node */
static DIALOGITEM us_purelayerdialogitems[] =
{
 /*  1 */ {0, {204,116,228,196}, BUTTON, N_("OK")},
 /*  2 */ {0, {204,12,228,92}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {28,8,196,200}, SCROLL, x_("")},
 /*  4 */ {0, {4,12,20,100}, MESSAGE, N_("Technology:")},
 /*  5 */ {0, {4,100,20,200}, MESSAGE, x_("")}
};
static DIALOG us_purelayerdialog = {{75,75,312,284}, N_("Make Pure Layer Node"), 0, 5, us_purelayerdialogitems, 0, 0};

/* New View Type */
static DIALOGITEM us_newviewdialogitems[] =
{
 /*  1 */ {0, {64,232,88,304}, BUTTON, N_("OK")},
 /*  2 */ {0, {64,16,88,88}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,145}, MESSAGE, N_("New view name:")},
 /*  4 */ {0, {32,8,48,145}, MESSAGE, N_("View abbreviation:")},
 /*  5 */ {0, {8,148,24,304}, EDITTEXT, x_("")},
 /*  6 */ {0, {32,148,48,304}, EDITTEXT, x_("")},
 /*  7 */ {0, {68,104,84,213}, CHECK, N_("Textual View")}
};
static DIALOG us_newviewdialog = {{50,75,154,391}, N_("New View"), 0, 7, us_newviewdialogitems, 0, 0};

/* Options Being Saved */
static DIALOGITEM us_optsavedialogitems[] =
{
 /*  1 */ {0, {240,205,264,285}, BUTTON, N_("OK")},
 /*  2 */ {0, {240,16,264,96}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {24,4,208,300}, SCROLL, x_("")},
 /*  4 */ {0, {4,4,20,276}, MESSAGE, N_("These options are being saved:")},
 /*  5 */ {0, {212,4,228,300}, CHECK, N_("Only show options changed this session")}
};
static DIALOG us_optsavedialog = {{75,75,348,385}, N_("Options Being Saved"), 0, 5, us_optsavedialogitems, 0, 0};

/* Port and Export Display Options */
static DIALOGITEM us_portdisplaydialogitems[] =
{
 /*  1 */ {0, {148,208,172,272}, BUTTON, N_("OK")},
 /*  2 */ {0, {148,20,172,84}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {36,8,52,123}, RADIO, N_("Full Names")},
 /*  4 */ {0, {60,8,76,123}, RADIO, N_("Short Names")},
 /*  5 */ {0, {84,8,100,123}, RADIO, N_("Crosses")},
 /*  6 */ {0, {8,8,24,151}, MESSAGE, N_("Ports (on instances):")},
 /*  7 */ {0, {36,156,52,271}, RADIO, N_("Full Names")},
 /*  8 */ {0, {60,156,76,271}, RADIO, N_("Short Names")},
 /*  9 */ {0, {84,156,100,271}, RADIO, N_("Crosses")},
 /* 10 */ {0, {8,156,24,295}, MESSAGE, N_("Exports (in cells):")},
 /* 11 */ {0, {116,8,132,280}, CHECK, N_("Move node with export name")},
 /* 12 */ {0, {108,8,109,292}, DIVIDELINE, x_("")}
};
static DIALOG us_portdisplaydialog = {{133,131,314,435}, N_("Port and Export Options"), 0, 12, us_portdisplaydialogitems, 0, 0};

/* Printing Options */
static DIALOGITEM us_printingoptdialogitems[] =
{
 /*  1 */ {0, {44,400,68,458}, BUTTON, N_("OK")},
 /*  2 */ {0, {8,400,32,458}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,8,48,204}, RADIO, N_("Plot only Highlighted Area")},
 /*  4 */ {0, {8,8,24,142}, RADIO, N_("Plot Entire Cell")},
 /*  5 */ {0, {108,104,124,220}, CHECK, N_("Encapsulated")},
 /*  6 */ {0, {8,228,24,383}, CHECK, N_("Plot Date In Corner")},
 /*  7 */ {0, {308,108,324,166}, RADIO, N_("HPGL")},
 /*  8 */ {0, {308,180,324,254}, RADIO, N_("HPGL/2")},
 /*  9 */ {0, {332,20,348,187}, RADIO, N_("HPGL/2 plot fills page")},
 /* 10 */ {0, {356,20,372,177}, RADIO, N_("HPGL/2 plot fixed at:")},
 /* 11 */ {0, {356,180,372,240}, EDITTEXT, x_("")},
 /* 12 */ {0, {356,244,372,406}, MESSAGE, N_("internal units per pixel")},
 /* 13 */ {0, {260,20,276,172}, CHECK, N_("Synchronize to file:")},
 /* 14 */ {0, {260,172,292,464}, MESSAGE, x_("")},
 /* 15 */ {0, {236,20,252,108}, MESSAGE, N_("EPS Scale:")},
 /* 16 */ {0, {236,124,252,164}, EDITTEXT, x_("2")},
 /* 17 */ {0, {212,8,228,80}, MESSAGE, N_("For cell:")},
 /* 18 */ {0, {212,80,228,464}, MESSAGE, x_("")},
 /* 19 */ {0, {300,8,301,464}, DIVIDELINE, x_("")},
 /* 20 */ {0, {108,8,124,96}, MESSAGE, N_("PostScript:")},
 /* 21 */ {0, {108,312,124,464}, POPUP, x_("")},
 /* 22 */ {0, {100,8,101,464}, DIVIDELINE, x_("")},
 /* 23 */ {0, {308,8,324,98}, MESSAGE, N_("HPGL Level:")},
 /* 24 */ {0, {132,20,148,90}, RADIO, N_("Printer")},
 /* 25 */ {0, {132,100,148,170}, RADIO, N_("Plotter")},
 /* 26 */ {0, {156,20,172,100}, MESSAGE, N_("Width (in):")},
 /* 27 */ {0, {156,104,172,144}, EDITTEXT, x_("2")},
 /* 28 */ {0, {56,228,72,384}, POPUP, x_("")},
 /* 29 */ {0, {32,228,48,384}, MESSAGE, N_("Default printer:")},
 /* 30 */ {0, {156,172,172,260}, MESSAGE, N_("Height (in):")},
 /* 31 */ {0, {156,264,172,304}, EDITTEXT, x_("2")},
 /* 32 */ {0, {180,20,196,256}, POPUP, x_("")},
 /* 33 */ {0, {156,332,172,420}, MESSAGE, N_("Margin (in):")},
 /* 34 */ {0, {156,424,172,464}, EDITTEXT, x_("2")},
 /* 35 */ {0, {80,8,96,280}, MESSAGE, N_("Print and Copy resolution factor:")},
 /* 36 */ {0, {56,8,72,204}, RADIO, N_("Plot only Displayed Window")},
 /* 37 */ {0, {80,284,96,338}, EDITTEXT, x_("")}
};
static DIALOG us_printingoptdialog = {{50,75,431,549}, N_("Printing Options"), 0, 37, us_printingoptdialogitems, 0, 0};

/* Progress (extended) */
static DIALOGITEM us_eprogressdialogitems[] =
{
 /*  1 */ {0, {56,8,73,230}, PROGRESS, x_("")},
 /*  2 */ {0, {32,8,48,230}, MESSAGE, N_("Reading file...")},
 /*  3 */ {0, {8,8,24,230}, MESSAGE, x_("")}
};
static DIALOG us_eprogressdialog = {{50,75,135,314}, 0, 0, 3, us_eprogressdialogitems, 0, 0};

/* Progress (simple) */
static DIALOGITEM us_progressdialogitems[] =
{
 /*  1 */ {0, {32,8,49,230}, PROGRESS, x_("")},
 /*  2 */ {0, {8,8,24,230}, MESSAGE, N_("Reading file...")}
};
static DIALOG us_progressdialog = {{50,75,108,314}, 0, 0, 2, us_progressdialogitems, 0, 0};

/* Project Management: Check In and out */
static DIALOGITEM proj_listdialogitems[] =
{
 /*  1 */ {0, {204,312,228,376}, BUTTON, N_("Done")},
 /*  2 */ {0, {4,4,196,376}, SCROLL, x_("")},
 /*  3 */ {0, {260,4,324,376}, MESSAGE, x_("")},
 /*  4 */ {0, {240,4,256,86}, MESSAGE, N_("Comments:")},
 /*  5 */ {0, {204,120,228,220}, BUTTON, N_("Check It Out")},
 /*  6 */ {0, {236,4,237,376}, DIVIDELINE, x_("")},
 /*  7 */ {0, {204,8,228,108}, BUTTON, N_("Check It In")},
 /*  8 */ {0, {204,232,228,296}, BUTTON, N_("Update")}
};
static DIALOG proj_listdialog = {{50,75,383,462}, N_("Project Management"), 0, 8, proj_listdialogitems, 0, 0};

/* Project Management: Password */
static DIALOGITEM proj_passworddialogitems[] =
{
 /*  1 */ {0, {72,12,96,76}, BUTTON, N_("OK")},
 /*  2 */ {0, {72,132,96,196}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {12,8,28,205}, MESSAGE, N_("Password for")},
 /*  4 */ {0, {40,48,56,165}, EDITTEXT, x_("")}
};
static DIALOG proj_passworddialog = {{298,75,404,292}, N_("User / Password"), 0, 4, proj_passworddialogitems, 0, 0};

/* Project Management: Set User */
static DIALOGITEM proj_usersdialogitems[] =
{
 /*  1 */ {0, {28,184,52,312}, BUTTON, N_("Select User")},
 /*  2 */ {0, {188,212,212,276}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {28,4,220,172}, SCROLL, x_("")},
 /*  4 */ {0, {8,60,24,112}, MESSAGE, N_("Users:")},
 /*  5 */ {0, {136,184,160,312}, BUTTON, N_("Delete User")},
 /*  6 */ {0, {64,184,88,312}, BUTTON, N_("Change Password")},
 /*  7 */ {0, {100,184,124,312}, BUTTON, N_("New User")}
};
static DIALOG proj_usersdialog = {{50,75,279,398}, N_("User Name"), 0, 7, proj_usersdialogitems, 0, 0};

/* Project Management: comments */
static DIALOGITEM proj_commentsdialogitems[] =
{
 /*  1 */ {0, {104,244,128,308}, BUTTON, N_("OK")},
 /*  2 */ {0, {104,48,128,112}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {4,8,20,363}, MESSAGE, N_("Reason for checking out cell")},
 /*  4 */ {0, {28,12,92,358}, EDITTEXT, x_("")}
};
static DIALOG proj_commentsdialog = {{108,75,248,447}, N_("Project Comments"), 0, 4, proj_commentsdialogitems, 0, 0};

/* Project Management: old version */
static DIALOGITEM proj_oldversdialogitems[] =
{
 /*  1 */ {0, {176,224,200,288}, BUTTON, N_("OK")},
 /*  2 */ {0, {176,16,200,80}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {4,8,20,140}, MESSAGE, N_("Old version of cell")},
 /*  4 */ {0, {28,12,166,312}, SCROLL, x_("")},
 /*  5 */ {0, {4,140,20,312}, MESSAGE, x_("")}
};
static DIALOG proj_oldversdialog = {{108,75,318,396}, N_("Get Old Version of Cell"), 0, 5, proj_oldversdialogitems, 0, 0};

/* Prompt: Full Input Dialog */
static DIALOGITEM us_ttyfulldialogitems[] =
{
 /*  1 */ {0, {160,328,184,392}, BUTTON, N_("OK")},
 /*  2 */ {0, {104,328,128,392}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {24,8,56,408}, EDITTEXT, x_("")},
 /*  4 */ {0, {3,8,19,408}, MESSAGE, x_("")},
 /*  5 */ {0, {88,8,212,294}, SCROLL, x_("")},
 /*  6 */ {0, {72,56,88,270}, MESSAGE, N_("Type '?' for a list of options")}
};
static DIALOG us_ttyfulldialog = {{50,75,271,493}, N_("Command Completion"), 0, 6, us_ttyfulldialogitems, 0, 0};

/* Prompt: Input Dialog */
static DIALOGITEM us_ttyinputdialogitems[] =
{
 /*  1 */ {0, {96,200,120,264}, BUTTON, N_("OK")},
 /*  2 */ {0, {96,24,120,88}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {58,38,74,246}, EDITTEXT, x_("")},
 /*  4 */ {0, {3,9,51,281}, MESSAGE, x_("")}
};
static DIALOG us_ttyinputdialog = {{50,75,188,371}, x_(""), 0, 4, us_ttyinputdialogitems, 0, 0};

/* Prompt: List Dialog */
static DIALOGITEM us_listdialogitems[] =
{
 /*  1 */ {0, {160,216,184,280}, BUTTON, N_("OK")},
 /*  2 */ {0, {96,216,120,280}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {56,8,200,206}, SCROLL, x_("")},
 /*  4 */ {0, {8,8,54,275}, MESSAGE, x_("")}
};
static DIALOG us_listdialog = {{50,75,259,369}, x_(""), 0, 4, us_listdialogitems, 0, 0};

/* Prompt: No/Yes */
static DIALOGITEM us_noyesdialogitems[] =
{
 /*  1 */ {0, {80,132,104,204}, BUTTON, N_("No")},
 /*  2 */ {0, {80,60,104,124}, BUTTON, N_("Yes")},
 /*  3 */ {0, {8,8,72,256}, MESSAGE, x_("")}
};
static DIALOG us_noyesdialog = {{50,75,163,341}, N_("Warning"), 0, 3, us_noyesdialogitems, 0, 0};

/* Prompt: No/Yes/Always */
static DIALOGITEM us_noyesalwaysdialogitems[] =
{
 /*  1 */ {0, {64,168,88,248}, BUTTON, N_("No")},
 /*  2 */ {0, {64,8,88,88}, BUTTON, N_("Yes")},
 /*  3 */ {0, {124,88,148,168}, BUTTON, N_("Always")},
 /*  4 */ {0, {8,8,56,248}, MESSAGE, N_("Allow this?")},
 /*  5 */ {0, {100,8,116,248}, MESSAGE, N_("Click ")}
};
static DIALOG us_noyesalwaysdialog = {{75,75,232,332}, N_("Allow this change?"), 0, 5, us_noyesalwaysdialogitems, 0, 0};

/* Prompt: No/Yes/Cancel */
static DIALOGITEM us_noyescanceldialogitems[] =
{
 /*  1 */ {0, {80,108,104,188}, BUTTON, N_("No")},
 /*  2 */ {0, {80,12,104,92}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {80,204,104,284}, BUTTON, N_("Yes")},
 /*  4 */ {0, {8,8,72,284}, MESSAGE, x_("")}
};
static DIALOG us_noyescanceldialog = {{75,75,188,368}, N_("Allow this change?"), 0, 4, us_noyescanceldialogitems, 0, 0};

/* Prompt: OK */
static DIALOGITEM us_okmessagedialogitems[] =
{
 /*  1 */ {0, {72,64,96,144}, BUTTON, N_("OK")},
 /*  2 */ {0, {8,8,56,212}, MESSAGE, x_("")}
};
static DIALOG us_okmessagedialog = {{75,75,180,296}, x_(""), 0, 2, us_okmessagedialogitems, 0, 0};

/* Prompt: Quit */
static DIALOGITEM us_quitdialogitems[] =
{
 /*  1 */ {0, {100,16,124,80}, BUTTON, N_("Yes")},
 /*  2 */ {0, {100,128,124,208}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {136,16,160,80}, BUTTON, N_("No")},
 /*  4 */ {0, {8,16,92,208}, MESSAGE, x_("")},
 /*  5 */ {0, {136,128,160,208}, BUTTON, N_("No to All")}
};
static DIALOG us_quitdialog = {{50,75,219,293}, 0, 0, 5, us_quitdialogitems, 0, 0};

/* Prompt: Session Logging */
static DIALOGITEM us_seslogdialogitems[] =
{
 /*  1 */ {0, {204,100,228,268}, BUTTON, N_("Save All Information")},
 /*  2 */ {0, {240,100,264,268}, BUTTON, N_("Disable Session Logging")},
 /*  3 */ {0, {4,4,20,284}, MESSAGE, N_("Warning: not all information has been saved.")},
 /*  4 */ {0, {36,4,52,356}, MESSAGE, N_("Unless all libraries and options are saved together")},
 /*  5 */ {0, {56,4,72,356}, MESSAGE, N_("It is not possible to reconstruct the session after a crash.")},
 /*  6 */ {0, {88,4,104,356}, MESSAGE, N_("The following information has not been saved:")},
 /*  7 */ {0, {108,4,192,356}, SCROLL, x_("")}
};
static DIALOG us_seslogdialog = {{75,75,348,441}, N_("Session Logging Warning"), 0, 7, us_seslogdialogitems, 0, 0};

/* Prompt: Session Playback */
static DIALOGITEM gra_sesplaydialogitems[] =
{
 /*  1 */ {0, {100,132,124,212}, BUTTON, N_("Yes")},
 /*  2 */ {0, {100,8,124,88}, BUTTON, N_("No")},
 /*  3 */ {0, {8,8,24,232}, MESSAGE, N_("Electric has found a session log file")},
 /*  4 */ {0, {24,8,40,232}, MESSAGE, N_("which may be from a recent crash.")},
 /*  5 */ {0, {52,8,68,232}, MESSAGE, N_("Do you wish to replay this session")},
 /*  6 */ {0, {68,8,84,232}, MESSAGE, N_("and reconstruct the lost work?")}
};
static DIALOG gra_sesplaydialog = {{75,75,208,316}, N_("Replay Log?"), 0, 6, gra_sesplaydialogitems, 0, 0};

/* Prompt: Severe Error */
static DIALOGITEM db_severeerrordialogitems[] =
{
 /*  1 */ {0, {80,8,104,72}, BUTTON, N_("Exit")},
 /*  2 */ {0, {80,96,104,160}, BUTTON, N_("Save")},
 /*  3 */ {0, {80,184,104,256}, BUTTON, N_("Continue")},
 /*  4 */ {0, {8,8,72,256}, MESSAGE, x_("")}
};
static DIALOG db_severeerrordialog = {{50,75,163,341}, N_("Fatal Error"), 0, 4, db_severeerrordialogitems, 0, 0};

/* Prompt: Yes/No */
static DIALOGITEM us_yesnodialogitems[] =
{
 /*  1 */ {0, {64,156,88,228}, BUTTON, N_("Yes")},
 /*  2 */ {0, {64,68,88,140}, BUTTON, N_("No")},
 /*  3 */ {0, {6,15,54,279}, MESSAGE, x_("")}
};
static DIALOG us_yesnodialog = {{50,75,150,369}, N_("Warning"), 0, 3, us_yesnodialogitems, 0, 0};

/* Prompt: Yes/No with "stop asking" */
static DIALOGITEM ro_yesnostopdialogitems[] =
{
 /*  1 */ {0, {64,156,88,228}, BUTTON, N_("Yes")},
 /*  2 */ {0, {64,68,88,140}, BUTTON, N_("No")},
 /*  3 */ {0, {6,15,54,279}, MESSAGE, x_("")},
 /*  4 */ {0, {96,156,120,276}, BUTTON, N_("Yes, then stop")},
 /*  5 */ {0, {96,20,120,140}, BUTTON, N_("No, and stop")}
};
static DIALOG ro_yesnostopdialog = {{50,75,179,363}, N_("Warning"), 0, 5, ro_yesnostopdialogitems, 0, 0};

/* Prompt: different electrical unit */
static DIALOGITEM us_elecunitdialogitems[] =
{
 /*  1 */ {0, {208,212,232,352}, BUTTON, N_("Use new units")},
 /*  2 */ {0, {208,20,232,160}, BUTTON, N_("Use former units")},
 /*  3 */ {0, {8,8,24,276}, MESSAGE, N_("Warning: displayed units have changed")},
 /*  4 */ {0, {36,8,52,108}, MESSAGE, N_("Formerly:")},
 /*  5 */ {0, {36,200,52,358}, MESSAGE, N_("New library:")},
 /*  6 */ {0, {60,16,76,166}, MESSAGE, x_("")},
 /*  7 */ {0, {60,208,76,358}, MESSAGE, x_("")},
 /*  8 */ {0, {84,16,100,166}, MESSAGE, x_("")},
 /*  9 */ {0, {84,208,100,358}, MESSAGE, x_("")},
 /* 10 */ {0, {108,16,124,166}, MESSAGE, x_("")},
 /* 11 */ {0, {108,208,124,358}, MESSAGE, x_("")},
 /* 12 */ {0, {132,16,148,166}, MESSAGE, x_("")},
 /* 13 */ {0, {132,208,148,358}, MESSAGE, x_("")},
 /* 14 */ {0, {156,16,172,166}, MESSAGE, x_("")},
 /* 15 */ {0, {156,208,172,358}, MESSAGE, x_("")},
 /* 16 */ {0, {180,16,196,166}, MESSAGE, x_("")},
 /* 17 */ {0, {180,208,196,358}, MESSAGE, x_("")},
 /* 18 */ {0, {60,168,76,204}, MESSAGE, x_("")},
 /* 19 */ {0, {84,168,100,204}, MESSAGE, x_("")},
 /* 20 */ {0, {108,168,124,204}, MESSAGE, x_("")},
 /* 21 */ {0, {132,168,148,204}, MESSAGE, x_("")},
 /* 22 */ {0, {156,168,172,204}, MESSAGE, x_("")},
 /* 23 */ {0, {180,168,196,204}, MESSAGE, x_("")}
};
static DIALOG us_elecunitdialog = {{75,75,316,442}, 0, 0, 23, us_elecunitdialogitems, 0, 0};

/* Prompt: different lambda values */
static DIALOGITEM io_techadjlamdialogitems[] =
{
 /*  1 */ {0, {232,220,256,300}, BUTTON, N_("Done")},
 /*  2 */ {0, {204,264,220,512}, BUTTON, N_("Use New Size for all Technologies")},
 /*  3 */ {0, {204,8,220,256}, BUTTON, N_("Use Current Size for all Technologies")},
 /*  4 */ {0, {28,300,128,512}, SCROLL, x_("")},
 /*  5 */ {0, {148,8,164,164}, MESSAGE, N_("Current lambda size:")},
 /*  6 */ {0, {148,264,164,416}, MESSAGE, N_("New lambda size:")},
 /*  7 */ {0, {148,164,164,256}, MESSAGE, x_("")},
 /*  8 */ {0, {148,416,164,512}, MESSAGE, x_("")},
 /*  9 */ {0, {176,264,192,512}, BUTTON, N_("<< Use New Size in Current Library")},
 /* 10 */ {0, {176,8,192,256}, BUTTON, N_("Use Current Size in New Library >>")},
 /* 11 */ {0, {80,16,96,292}, MESSAGE, N_("and choose from the actions below.")},
 /* 12 */ {0, {8,8,24,508}, MESSAGE, N_("This new library uses different lambda values than existing libraries.")},
 /* 13 */ {0, {28,8,44,292}, MESSAGE, N_("You should unify the lambda values.")},
 /* 14 */ {0, {60,8,76,292}, MESSAGE, N_("Click on each technology in this list")},
 /* 15 */ {0, {136,8,137,512}, DIVIDELINE, x_("")},
 /* 16 */ {0, {112,8,128,284}, MESSAGE, N_("Use 'Check and Repair Libraries' when done.")}
};
static DIALOG io_techadjlamdialog = {{75,75,340,596}, N_("Lambda Value Adjustment"), 0, 16, io_techadjlamdialogitems, 0, 0};

/* Prompt: different technology options */
static DIALOGITEM us_techdifoptdialogitems[] =
{
 /*  1 */ {0, {140,264,164,440}, BUTTON, N_("Use Requested Options")},
 /*  2 */ {0, {140,32,164,208}, BUTTON, N_("Leave Current Options")},
 /*  3 */ {0, {8,8,24,344}, MESSAGE, N_("This library requests different options for technology:")},
 /*  4 */ {0, {8,348,24,472}, MESSAGE, x_("")},
 /*  5 */ {0, {32,8,48,132}, MESSAGE, N_("Current options:")},
 /*  6 */ {0, {32,136,80,472}, MESSAGE, x_("")},
 /*  7 */ {0, {84,8,100,132}, MESSAGE, N_("Requested options:")},
 /*  8 */ {0, {84,136,132,472}, MESSAGE, x_("")}
};
static DIALOG us_techdifoptdialog = {{75,75,248,556}, N_("Technology Options Conflict"), 0, 8, us_techdifoptdialogitems, 0, 0};

/* Prompt: save options */
static DIALOGITEM us_saveoptsdialogitems[] =
{
 /*  1 */ {0, {172,196,196,276}, BUTTON, N_("Yes")},
 /*  2 */ {0, {172,104,196,184}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {172,12,196,92}, BUTTON, N_("No")},
 /*  4 */ {0, {8,8,24,276}, MESSAGE, N_("These Options have changed.  Save?")},
 /*  5 */ {0, {32,12,164,276}, SCROLL, x_("")}
};
static DIALOG us_saveoptsdialog = {{75,75,280,360}, N_("Save Options?"), 0, 5, us_saveoptsdialogitems, 0, 0};

/* Quick Keys Options */
static DIALOGITEM us_quickkeydialogitems[] =
{
 /*  1 */ {0, {520,320,544,384}, BUTTON, N_("OK")},
 /*  2 */ {0, {520,12,544,76}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {24,4,168,168}, SCROLL, x_("")},
 /*  4 */ {0, {192,20,336,184}, SCROLL, x_("")},
 /*  5 */ {0, {4,4,20,68}, MESSAGE, N_("Menu:")},
 /*  6 */ {0, {172,20,188,136}, MESSAGE, N_("SubMenu/Item:")},
 /*  7 */ {0, {360,36,504,200}, SCROLL, x_("")},
 /*  8 */ {0, {340,36,356,140}, MESSAGE, N_("SubItem:")},
 /*  9 */ {0, {24,228,504,392}, SCROLL, x_("")},
 /* 10 */ {0, {4,228,20,328}, MESSAGE, N_("Quick Key:")},
 /* 11 */ {0, {256,192,280,220}, BUTTON, x_(">>")},
 /* 12 */ {0, {520,236,544,296}, BUTTON, N_("Remove")},
 /* 13 */ {0, {520,96,544,216}, BUTTON, N_("Factory Settings")}
};
static DIALOG us_quickkeydialog = {{75,75,634,478}, N_("Quick Key Options"), 0, 13, us_quickkeydialogitems, 0, 0};

/* ROM Generator */
static DIALOGITEM us_romgdialogitems[] =
{
 /*  1 */ {0, {104,164,128,244}, BUTTON, N_("OK")},
 /*  2 */ {0, {104,48,128,128}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {4,4,20,200}, MESSAGE, N_("ROM personality file:")},
 /*  4 */ {0, {24,4,56,296}, EDITTEXT, x_("")},
 /*  5 */ {0, {4,224,20,272}, BUTTON, N_("Set")},
 /*  6 */ {0, {72,4,88,176}, MESSAGE, N_("Gate size (nanometers):")},
 /*  7 */ {0, {72,180,88,272}, EDITTEXT, x_("")}
};
static DIALOG us_romgdialog = {{75,75,212,381}, N_("ROM Generation"), 0, 7, us_romgdialogitems, 0, 0};

/* Rename Object */
static DIALOGITEM us_rendialogitems[] =
{
 /*  1 */ {0, {196,236,220,316}, BUTTON, N_("OK")},
 /*  2 */ {0, {116,236,140,316}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {56,8,288,220}, SCROLL, x_("")},
 /*  4 */ {0, {292,8,308,88}, MESSAGE, N_("New name:")},
 /*  5 */ {0, {292,84,308,316}, EDITTEXT, x_("")},
 /*  6 */ {0, {8,8,24,316}, POPUP, x_("")},
 /*  7 */ {0, {32,84,48,316}, POPUP, x_("")},
 /*  8 */ {0, {32,8,48,84}, MESSAGE, N_("Library:")}
};
static DIALOG us_rendialog = {{75,75,392,401}, N_("Rename Object"), 0, 8, us_rendialogitems, 0, 0};

/* Routing Options */
static DIALOGITEM ro_optionsdialogitems[] =
{
 /*  1 */ {0, {264,264,288,328}, BUTTON, N_("OK")},
 /*  2 */ {0, {264,12,288,76}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,88,48,272}, POPUP, x_("")},
 /*  4 */ {0, {32,8,48,88}, MESSAGE, N_("Currently:")},
 /*  5 */ {0, {8,8,24,220}, MESSAGE, N_("Arc to use in stitching routers:")},
 /*  6 */ {0, {68,8,84,268}, CHECK, N_("Mimic stitching can unstitch")},
 /*  7 */ {0, {116,24,132,348}, CHECK, N_("Ports must match")},
 /*  8 */ {0, {236,8,252,268}, CHECK, N_("Mimic stitching runs interactively")},
 /*  9 */ {0, {92,8,108,268}, MESSAGE, N_("Mimic stitching restrictions:")},
 /* 10 */ {0, {188,24,204,348}, CHECK, N_("Node types must match")},
 /* 11 */ {0, {164,24,180,348}, CHECK, N_("Nodes sizes must match")},
 /* 12 */ {0, {140,24,156,348}, CHECK, N_("Number of existing arcs must match")},
 /* 13 */ {0, {212,24,228,348}, CHECK, N_("Cannot have other arcs in the same direction")}
};
static DIALOG ro_optionsdialog = {{50,75,347,433}, N_("Routing Options"), 0, 13, ro_optionsdialogitems, 0, 0};

/* SKILL Options */
static DIALOGITEM io_skilloptionsdialogitems[] =
{
 /*  1 */ {0, {260,156,284,228}, BUTTON, N_("OK")},
 /*  2 */ {0, {260,12,284,84}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,168,231}, SCROLL, x_("")},
 /*  4 */ {0, {176,12,192,108}, MESSAGE, N_("SKILL Layer:")},
 /*  5 */ {0, {176,116,192,222}, EDITTEXT, x_("")},
 /*  6 */ {0, {204,12,220,224}, CHECK, N_("Do not include subcells")},
 /*  7 */ {0, {228,12,244,224}, CHECK, N_("Flatten hierarchy")}
};
static DIALOG io_skilloptionsdialog = {{50,75,343,323}, N_("SKILL Options"), 0, 7, io_skilloptionsdialogitems, 0, 0};

/* SKILL library selection */
static DIALOGITEM io_skilllibdialogitems[] =
{
 /*  1 */ {0, {64,136,88,216}, BUTTON, N_("OK")},
 /*  2 */ {0, {64,12,88,92}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,224}, MESSAGE, N_("Library name to use in SKILL file:")},
 /*  4 */ {0, {32,8,48,220}, EDITTEXT, x_("")}
};
static DIALOG io_skilllibdialog = {{75,75,172,308}, N_("SKILL Library Name"), 0, 4, io_skilllibdialogitems, 0, 0};

/* SPICE Options */
static DIALOGITEM sim_spiceoptdialogitems[] =
{
 /*  1 */ {0, {48,420,72,480}, BUTTON, N_("OK")},
 /*  2 */ {0, {8,420,32,480}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {104,8,120,224}, POPUP, x_("")},
 /*  4 */ {0, {8,120,24,223}, POPUP, x_("")},
 /*  5 */ {0, {588,216,604,375}, RADIO, N_("Use Model from File:")},
 /*  6 */ {0, {564,216,580,426}, RADIO, N_("Derive Model from Circuitry")},
 /*  7 */ {0, {32,120,48,164}, POPUP, x_("1")},
 /*  8 */ {0, {588,384,604,451}, BUTTON, N_("Browse")},
 /*  9 */ {0, {612,224,644,480}, EDITTEXT, x_("")},
 /* 10 */ {0, {32,240,48,372}, CHECK, N_("Use Node Names")},
 /* 11 */ {0, {8,240,24,359}, CHECK, N_("Use Parasitics")},
 /* 12 */ {0, {8,8,24,117}, MESSAGE, N_("SPICE Engine:")},
 /* 13 */ {0, {32,8,48,117}, MESSAGE, N_("SPICE Level:")},
 /* 14 */ {0, {352,8,368,192}, RADIO, N_("Use Built-in Header Cards")},
 /* 15 */ {0, {400,8,416,204}, RADIO, N_("Use Header Cards from File:")},
 /* 16 */ {0, {400,208,432,472}, EDITTEXT, x_("")},
 /* 17 */ {0, {448,8,464,132}, RADIO, N_("No Trailer Cards")},
 /* 18 */ {0, {496,8,512,204}, RADIO, N_("Include Trailer from File:")},
 /* 19 */ {0, {496,208,528,472}, EDITTEXT, x_("")},
 /* 20 */ {0, {208,8,224,281}, MESSAGE, x_("")},
 /* 21 */ {0, {416,36,432,204}, BUTTON, N_("Browse Header Card File")},
 /* 22 */ {0, {512,36,528,204}, BUTTON, N_("Browse Trailer Card File")},
 /* 23 */ {0, {564,8,644,206}, SCROLL, x_("")},
 /* 24 */ {0, {544,8,560,80}, MESSAGE, N_("For Cell:")},
 /* 25 */ {0, {352,196,368,480}, BUTTON, N_("Edit Built-in Headers for Technology/Level")},
 /* 26 */ {0, {200,8,201,480}, DIVIDELINE, x_("")},
 /* 27 */ {0, {536,8,537,480}, DIVIDELINE, x_("")},
 /* 28 */ {0, {232,72,312,266}, SCROLL, x_("")},
 /* 29 */ {0, {232,8,248,68}, MESSAGE, N_("Layer:")},
 /* 30 */ {0, {232,272,248,364}, MESSAGE, N_("Resistance:")},
 /* 31 */ {0, {256,272,272,364}, MESSAGE, N_("Capacitance:")},
 /* 32 */ {0, {280,272,296,364}, MESSAGE, N_("Edge Cap:")},
 /* 33 */ {0, {232,372,248,472}, EDITTEXT, x_("")},
 /* 34 */ {0, {256,372,272,472}, EDITTEXT, x_("")},
 /* 35 */ {0, {280,372,296,472}, EDITTEXT, x_("")},
 /* 36 */ {0, {320,8,336,124}, MESSAGE, N_("Min. Resistance:")},
 /* 37 */ {0, {320,128,336,216}, EDITTEXT, x_("")},
 /* 38 */ {0, {320,240,336,380}, MESSAGE, N_("Min. Capacitance:")},
 /* 39 */ {0, {320,384,336,472}, EDITTEXT, x_("")},
 /* 40 */ {0, {56,120,72,224}, POPUP, x_("")},
 /* 41 */ {0, {56,240,72,408}, CHECK, N_("Force Global VDD/GND")},
 /* 42 */ {0, {176,8,192,176}, MESSAGE, N_("SPICE primitive set:")},
 /* 43 */ {0, {176,180,192,408}, POPUP, x_("")},
 /* 44 */ {0, {128,60,144,480}, MESSAGE, x_("")},
 /* 45 */ {0, {128,8,144,56}, MESSAGE, N_("Run:")},
 /* 46 */ {0, {152,60,168,480}, EDITTEXT, x_("")},
 /* 47 */ {0, {152,8,168,56}, MESSAGE, N_("With:")},
 /* 48 */ {0, {80,240,96,408}, CHECK, N_("Use Cell Parameters")},
 /* 49 */ {0, {376,8,392,288}, RADIO, N_("Use Header Cards with extension:")},
 /* 50 */ {0, {376,296,392,392}, EDITTEXT, x_("")},
 /* 51 */ {0, {472,8,488,288}, RADIO, N_("Use Trailer Cards with extension:")},
 /* 52 */ {0, {472,296,488,392}, EDITTEXT, x_("")},
 /* 53 */ {0, {344,8,345,480}, DIVIDELINE, x_("")},
 /* 54 */ {0, {104,240,120,456}, CHECK, N_("Write Trans Sizes in Lambda")},
 /* 55 */ {0, {56,8,72,117}, MESSAGE, N_("Output format:")}
};
static DIALOG sim_spiceoptdialog = {{50,75,703,565}, N_("SPICE Options"), 0, 55, sim_spiceoptdialogitems, 0, 0};

/* Saved Views */
static DIALOGITEM us_windowviewdialogitems[] =
{
 /*  1 */ {0, {256,56,280,166}, BUTTON, N_("Restore View")},
 /*  2 */ {0, {216,8,240,72}, BUTTON, N_("Done")},
 /*  3 */ {0, {32,8,208,234}, SCROLL, x_("")},
 /*  4 */ {0, {8,96,24,229}, EDITTEXT, x_("")},
 /*  5 */ {0, {216,120,240,230}, BUTTON, N_("Save This View")},
 /*  6 */ {0, {8,8,24,90}, MESSAGE, N_("View name:")}
};
static DIALOG us_windowviewdialog = {{50,75,342,318}, N_("Window Views"), 0, 6, us_windowviewdialogitems, 0, 0};

/* Saving Options with Libraries */
static DIALOGITEM us_optionsavingdialogitems[] =
{
 /*  1 */ {0, {280,216,304,288}, BUTTON, N_("OK")},
 /*  2 */ {0, {280,12,304,84}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {28,4,44,294}, MESSAGE, N_("Marked options are saved with the library,")},
 /*  4 */ {0, {92,4,268,294}, SCROLL, x_("")},
 /*  5 */ {0, {68,4,84,294}, MESSAGE, N_("Click an option to change its mark.")},
 /*  6 */ {0, {44,4,60,294}, MESSAGE, N_("and are restored when the library is read.")},
 /*  7 */ {0, {4,4,20,90}, MESSAGE, N_("For library:")},
 /*  8 */ {0, {4,92,20,294}, MESSAGE, x_("")}
};
static DIALOG us_optionsavingdialog = {{50,75,363,378}, N_("Saving Options with Libraries"), 0, 8, us_optionsavingdialogitems, 0, 0};

/* Scalable Transistors */
static DIALOGITEM us_scatrndialogitems[] =
{
 /*  1 */ {0, {128,212,152,292}, BUTTON, N_("OK")},
 /*  2 */ {0, {128,12,152,92}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {128,112,152,192}, BUTTON, N_("More")},
 /*  4 */ {0, {8,8,24,152}, MESSAGE, N_("Number of contacts:")},
 /*  5 */ {0, {8,157,24,209}, POPUP, x_("")},
 /*  6 */ {0, {32,8,48,292}, CHECK, N_("Move contacts half-lambda closer in")},
 /*  7 */ {0, {80,8,96,152}, MESSAGE, N_("Actual width:")},
 /*  8 */ {0, {80,156,96,292}, EDITTEXT, x_("")},
 /*  9 */ {0, {56,8,72,152}, MESSAGE, N_("Maximum width:")},
 /* 10 */ {0, {56,156,72,292}, MESSAGE, x_("")},
 /* 11 */ {0, {104,156,120,292}, MESSAGE, x_("")},
 /* 12 */ {0, {104,8,120,152}, MESSAGE, x_("")}
};
static DIALOG us_scatrndialog = {{75,75,236,376}, N_("Scalable Transistor Information"), 0, 12, us_scatrndialogitems, 0, 0};

/* Select Export/Network/Node/Arc */
static DIALOGITEM us_selnamedialogitems[] =
{
 /*  1 */ {0, {192,60,216,140}, BUTTON, N_("Done")},
 /*  2 */ {0, {8,8,180,192}, SCROLLMULTI, x_("")}
};
static DIALOG us_selnamedialog = {{75,75,300,276}, N_("Select Port"), 0, 2, us_selnamedialogitems, 0, 0};

/* Selection Options */
static DIALOGITEM us_seloptdialogitems[] =
{
 /*  1 */ {0, {108,196,132,276}, BUTTON, N_("OK")},
 /*  2 */ {0, {108,4,132,84}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,4,24,280}, CHECK, N_("Easy selection of cell instances")},
 /*  4 */ {0, {32,4,48,280}, CHECK, N_("Easy selection of annotation text")},
 /*  5 */ {0, {56,4,72,280}, CHECK, N_("Center-based primitives")},
 /*  6 */ {0, {80,4,96,280}, CHECK, N_("Dragging must enclose entire object")}
};
static DIALOG us_seloptdialog = {{75,75,216,365}, N_("Selection Options"), 0, 6, us_seloptdialogitems, 0, 0};

/* Set Node Effort (Logical Effort) */
static DIALOGITEM us_logefforteffortdialogitems[] =
{
 /*  1 */ {0, {40,128,64,192}, BUTTON, N_("OK")},
 /*  2 */ {0, {40,12,64,76}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,116}, MESSAGE, N_("Logical Effort:")},
 /*  4 */ {0, {8,128,24,192}, EDITTEXT, x_("")}
};
static DIALOG us_logefforteffortdialog = {{75,75,149,281}, N_("Logical Effort"), 0, 4, us_logefforteffortdialogitems, 0, 0};

/* Set Paths */
static DIALOGITEM us_librarypathdialogitems[] =
{
 /*  1 */ {0, {76,312,100,376}, BUTTON, N_("OK")},
 /*  2 */ {0, {76,32,100,96}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,16,24,220}, MESSAGE, N_("Location of library files:")},
 /*  4 */ {0, {32,8,64,400}, EDITTEXT, x_("")}
};
static DIALOG us_librarypathdialog = {{50,75,159,485}, N_("Current Library Path"), 0, 4, us_librarypathdialogitems, 0, 0};

/* Silicon Compiler Options */
static DIALOGITEM sc_optionsdialogitems[] =
{
 /*  1 */ {0, {500,268,524,326}, BUTTON, N_("OK")},
 /*  2 */ {0, {500,36,524,94}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,204,24,384}, POPUP, x_("")},
 /*  4 */ {0, {32,204,48,276}, EDITTEXT, x_("")},
 /*  5 */ {0, {8,8,24,180}, MESSAGE, N_("Horizontal routing arc:")},
 /*  6 */ {0, {60,8,76,180}, MESSAGE, N_("Vertical routing arc:")},
 /*  7 */ {0, {32,8,48,174}, MESSAGE, N_("Horizontal wire width:")},
 /*  8 */ {0, {60,204,76,384}, POPUP, x_("")},
 /*  9 */ {0, {84,8,100,174}, MESSAGE, N_("Vertical wire width:")},
 /* 10 */ {0, {84,204,100,276}, EDITTEXT, x_("")},
 /* 11 */ {0, {120,8,136,174}, MESSAGE, N_("Power wire width:")},
 /* 12 */ {0, {120,204,136,276}, EDITTEXT, x_("")},
 /* 13 */ {0, {144,8,160,174}, MESSAGE, N_("Main power wire width:")},
 /* 14 */ {0, {144,204,160,276}, EDITTEXT, x_("")},
 /* 15 */ {0, {204,8,220,199}, MESSAGE, N_("P-Well height (0 for none):")},
 /* 16 */ {0, {168,204,184,352}, POPUP, x_("")},
 /* 17 */ {0, {228,8,244,199}, MESSAGE, N_("P-Well offset from bottom:")},
 /* 18 */ {0, {204,204,220,276}, EDITTEXT, x_("")},
 /* 19 */ {0, {320,8,336,199}, MESSAGE, N_("Via size:")},
 /* 20 */ {0, {232,204,248,276}, EDITTEXT, x_("")},
 /* 21 */ {0, {344,8,360,199}, MESSAGE, N_("Minimum metal spacing:")},
 /* 22 */ {0, {256,204,272,276}, EDITTEXT, x_("")},
 /* 23 */ {0, {384,8,400,199}, MESSAGE, N_("Routing: feed-through size:")},
 /* 24 */ {0, {280,204,296,276}, EDITTEXT, x_("")},
 /* 25 */ {0, {408,8,424,199}, MESSAGE, N_("Routing: min. port distance:")},
 /* 26 */ {0, {320,204,336,276}, EDITTEXT, x_("")},
 /* 27 */ {0, {432,8,448,199}, MESSAGE, N_("Routing: min. active dist.:")},
 /* 28 */ {0, {344,204,360,276}, EDITTEXT, x_("")},
 /* 29 */ {0, {472,8,488,199}, MESSAGE, N_("Number of rows of cells:")},
 /* 30 */ {0, {384,204,400,276}, EDITTEXT, x_("")},
 /* 31 */ {0, {408,204,424,276}, EDITTEXT, x_("")},
 /* 32 */ {0, {168,8,184,180}, MESSAGE, N_("Main power arc:")},
 /* 33 */ {0, {256,8,272,199}, MESSAGE, N_("N-Well height (0 for none):")},
 /* 34 */ {0, {432,204,448,276}, EDITTEXT, x_("")},
 /* 35 */ {0, {280,8,296,199}, MESSAGE, N_("N-Well offset from top:")},
 /* 36 */ {0, {472,204,488,276}, EDITTEXT, x_("")}
};
static DIALOG sc_optionsdialog = {{50,75,583,473}, N_("Silicon Compiler Options"), 0, 36, sc_optionsdialogitems, 0, 0};

/* Simulation Options */
static DIALOGITEM sim_optionsdialogitems[] =
{
 /*  1 */ {0, {312,484,336,548}, BUTTON, N_("OK")},
 /*  2 */ {0, {312,384,336,448}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {124,4,140,201}, CHECK, N_("Resimulate each change")},
 /*  4 */ {0, {148,4,164,201}, CHECK, N_("Auto advance time")},
 /*  5 */ {0, {172,4,188,201}, CHECK, N_("Multistate display")},
 /*  6 */ {0, {52,4,68,201}, CHECK, N_("Show waveform window")},
 /*  7 */ {0, {228,292,244,417}, MESSAGE, N_("Maximum events:")},
 /*  8 */ {0, {228,420,244,501}, EDITTEXT, x_("")},
 /*  9 */ {0, {28,4,44,161}, MESSAGE, N_("Base for bus values:")},
 /* 10 */ {0, {28,164,44,217}, POPUP, x_("")},
 /* 11 */ {0, {76,16,92,201}, MESSAGE, N_("Place waveform window")},
 /* 12 */ {0, {96,60,112,200}, POPUP, x_("")},
 /* 13 */ {0, {4,4,20,129}, MESSAGE, N_("Simulation engine:")},
 /* 14 */ {0, {4,132,20,276}, POPUP, x_("")},
 /* 15 */ {0, {204,4,220,89}, MESSAGE, N_("IRSIM:")},
 /* 16 */ {0, {204,284,220,368}, MESSAGE, N_("ALS:")},
 /* 17 */ {0, {244,28,260,101}, RADIO, N_("Quick")},
 /* 18 */ {0, {244,104,260,177}, RADIO, N_("Local")},
 /* 19 */ {0, {244,180,260,253}, RADIO, N_("Full")},
 /* 20 */ {0, {196,4,197,565}, DIVIDELINE, x_("")},
 /* 21 */ {0, {268,16,284,129}, MESSAGE, N_("Parameter file:")},
 /* 22 */ {0, {288,28,320,273}, EDITTEXT, x_("")},
 /* 23 */ {0, {224,16,240,153}, MESSAGE, N_("Parasitics:")},
 /* 24 */ {0, {268,172,284,229}, BUTTON, N_("Set")},
 /* 25 */ {0, {4,280,345,281}, DIVIDELINE, x_("")},
 /* 26 */ {0, {329,16,345,213}, CHECK, N_("Show commands")},
 /* 27 */ {0, {32,288,48,441}, MESSAGE, N_("Low:")},
 /* 28 */ {0, {32,444,48,565}, POPUP, x_("")},
 /* 29 */ {0, {52,288,68,441}, MESSAGE, N_("High:")},
 /* 30 */ {0, {52,444,68,565}, POPUP, x_("")},
 /* 31 */ {0, {72,288,88,441}, MESSAGE, N_("Undefined (X):")},
 /* 32 */ {0, {72,444,88,565}, POPUP, x_("")},
 /* 33 */ {0, {92,288,108,441}, MESSAGE, N_("Floating (Z):")},
 /* 34 */ {0, {92,444,108,565}, POPUP, x_("")},
 /* 35 */ {0, {112,288,128,441}, MESSAGE, N_("Strength 0 (off):")},
 /* 36 */ {0, {112,444,128,565}, POPUP, x_("")},
 /* 37 */ {0, {132,288,148,441}, MESSAGE, N_("Strength 1 (node):")},
 /* 38 */ {0, {132,444,148,565}, POPUP, x_("")},
 /* 39 */ {0, {152,288,168,441}, MESSAGE, N_("Strength 2 (gate):")},
 /* 40 */ {0, {152,444,168,565}, POPUP, x_("")},
 /* 41 */ {0, {172,288,188,441}, MESSAGE, N_("Strength 3 (power):")},
 /* 42 */ {0, {172,444,188,565}, POPUP, x_("")},
 /* 43 */ {0, {8,288,24,565}, MESSAGE, N_("Waveform window colors:")}
};
static DIALOG sim_optionsdialog = {{50,75,404,650}, N_("Simulation Options"), 0, 43, sim_optionsdialogitems, 0, 0};

/* Simulation Signal Selection */
static DIALOGITEM sim_sigselectdialogitems[] =
{
 /*  1 */ {0, {344,276,368,356}, BUTTON, N_("OK")},
 /*  2 */ {0, {344,184,368,264}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {4,4,336,356}, SCROLLMULTI, x_("")},
 /*  4 */ {0, {348,4,364,176}, CHECK, N_("Show lower levels")}
};
static DIALOG sim_sigselectdialog = {{75,75,452,441}, N_("Select Signal for Simulation"), 0, 4, sim_sigselectdialogitems, 0, 0};

/* Simulation: ALS Clock */
static DIALOGITEM sim_alsclockdialogitems[] =
{
 /*  1 */ {0, {8,320,32,384}, BUTTON, N_("OK")},
 /*  2 */ {0, {40,320,64,384}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {16,8,32,101}, RADIO, N_("Frequency:")},
 /*  4 */ {0, {40,8,56,101}, RADIO, N_("Period:")},
 /*  5 */ {0, {80,8,96,101}, RADIO, N_("Custom:")},
 /*  6 */ {0, {28,208,44,283}, EDITTEXT, x_("")},
 /*  7 */ {0, {28,112,44,199}, MESSAGE, N_("Freq/Period:")},
 /*  8 */ {0, {112,236,128,384}, RADIO, N_("Normal (gate) strength")},
 /*  9 */ {0, {236,232,252,369}, RADIO, N_("Undefined Phase")},
 /* 10 */ {0, {112,8,128,151}, MESSAGE, N_("Random Distribution:")},
 /* 11 */ {0, {112,160,128,223}, EDITTEXT, x_("")},
 /* 12 */ {0, {136,236,152,384}, RADIO, N_("Strong (VDD) strength")},
 /* 13 */ {0, {88,236,104,384}, RADIO, N_("Weak (node) strength")},
 /* 14 */ {0, {212,232,228,328}, RADIO, N_("High Phase")},
 /* 15 */ {0, {292,272,308,363}, BUTTON, N_("Delete Phase")},
 /* 16 */ {0, {164,8,279,221}, SCROLL|INACTIVE, x_("")},
 /* 17 */ {0, {164,224,180,288}, MESSAGE, N_("Duration:")},
 /* 18 */ {0, {264,272,280,363}, BUTTON, N_("Add Phase")},
 /* 19 */ {0, {292,8,308,197}, MESSAGE, N_("Phase Cycles (0 for infinite):")},
 /* 20 */ {0, {292,200,308,247}, EDITTEXT, x_("")},
 /* 21 */ {0, {164,296,180,369}, EDITTEXT, x_("")},
 /* 22 */ {0, {148,8,164,99}, MESSAGE, N_("Phase List:")},
 /* 23 */ {0, {188,232,204,327}, RADIO, N_("Low Phase")},
 /* 24 */ {0, {72,8,73,384}, DIVIDELINE, x_("")}
};
static DIALOG sim_alsclockdialog = {{50,75,367,468}, N_("Clock Specification"), 0, 24, sim_alsclockdialogitems, 0, 0};

/* Simulation: Select Node Index */
static DIALOGITEM sim_selinddialogitems[] =
{
 /*  1 */ {0, {76,120,100,200}, BUTTON, N_("OK")},
 /*  2 */ {0, {76,16,100,96}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {4,4,20,216}, MESSAGE, N_("This node is arrayed")},
 /*  4 */ {0, {24,4,40,216}, MESSAGE, N_("Which entry should be entered?")},
 /*  5 */ {0, {48,4,64,216}, POPUP, x_("")}
};
static DIALOG sim_selinddialog = {{75,75,184,301}, N_("Select Node Index"), 0, 5, sim_selinddialogitems, 0, 0};

/* Simulation: Wide Value */
static DIALOGITEM sim_widedialogitems[] =
{
 /*  1 */ {0, {48,188,72,268}, BUTTON, N_("OK")},
 /*  2 */ {0, {12,188,36,268}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,168}, MESSAGE, N_("Value to set on bus:")},
 /*  4 */ {0, {32,8,48,168}, EDITTEXT, x_("")},
 /*  5 */ {0, {56,8,72,72}, MESSAGE, N_("Base:")},
 /*  6 */ {0, {56,76,72,144}, POPUP, x_("")}
};
static DIALOG sim_widedialog = {{75,75,156,352}, N_("Set Wide Value"), 0, 6, sim_widedialogitems, 0, 0};

/* Size: All Selected Arcs */
static DIALOGITEM us_arcsizedialogitems[] =
{
 /*  1 */ {0, {36,96,60,176}, BUTTON, N_("OK")},
 /*  2 */ {0, {36,4,60,84}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,4,24,84}, MESSAGE|INACTIVE, N_("Width")},
 /*  4 */ {0, {8,92,24,172}, EDITTEXT, x_("")}
};
static DIALOG us_arcsizedialog = {{75,75,144,260}, N_("Set Arc Size"), 0, 4, us_arcsizedialogitems, 0, 0};

/* Size: All Selected Nodes/Arcs */
static DIALOGITEM us_nodesizedialogitems[] =
{
 /*  1 */ {0, {104,132,128,212}, BUTTON, N_("OK")},
 /*  2 */ {0, {104,4,128,84}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,12,24,92}, MESSAGE|INACTIVE, N_("X Size:")},
 /*  4 */ {0, {36,12,52,92}, MESSAGE|INACTIVE, N_("Y Size:")},
 /*  5 */ {0, {8,100,24,200}, EDITTEXT, x_("")},
 /*  6 */ {0, {36,100,52,200}, EDITTEXT, x_("")},
 /*  7 */ {0, {64,4,96,212}, MESSAGE|INACTIVE, x_("")}
};
static DIALOG us_nodesizedialog = {{75,75,212,297}, N_("Set Node Size"), 0, 7, us_nodesizedialogitems, 0, 0};

/* Spread */
static DIALOGITEM us_spreaddialogitems[] =
{
 /*  1 */ {0, {96,128,120,200}, BUTTON, N_("OK")},
 /*  2 */ {0, {96,16,120,88}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {55,15,71,205}, EDITTEXT, x_("")},
 /*  4 */ {0, {20,230,36,380}, RADIO, N_("Spread up")},
 /*  5 */ {0, {45,230,61,380}, RADIO, N_("Spread down")},
 /*  6 */ {0, {70,230,86,380}, RADIO, N_("Spread left")},
 /*  7 */ {0, {95,230,111,380}, RADIO, N_("Spread right")},
 /*  8 */ {0, {25,15,41,180}, MESSAGE, N_("Distance to spread")}
};
static DIALOG us_spreaddialog = {{50,75,188,464}, N_("Spread About Highlighted"), 0, 8, us_spreaddialogitems, 0, 0};

/* Sue Options */
static DIALOGITEM io_sueoptdialogitems[] =
{
 /*  1 */ {0, {36,120,60,192}, BUTTON, N_("OK")},
 /*  2 */ {0, {36,16,60,88}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,200}, CHECK, N_("Make 4-port transistors")}
};
static DIALOG io_sueoptdialog = {{50,75,119,285}, N_("SUE Options"), 0, 3, io_sueoptdialogitems, 0, 0};

/* Technology Edit Reorder */
static DIALOGITEM us_tecedredialogitems[] =
{
 /*  1 */ {0, {376,208,400,288}, BUTTON, N_("OK")},
 /*  2 */ {0, {344,208,368,288}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {28,4,404,200}, SCROLL, x_("")},
 /*  4 */ {0, {4,4,20,284}, MESSAGE, x_("")},
 /*  5 */ {0, {168,208,192,268}, BUTTON, N_("Up")},
 /*  6 */ {0, {212,208,236,268}, BUTTON, N_("Down")},
 /*  7 */ {0, {136,208,160,280}, BUTTON, N_("Far Up")},
 /*  8 */ {0, {244,208,268,280}, BUTTON, N_("Far Down")}
};
static DIALOG us_tecedredialog = {{75,75,488,373}, N_("Reorder Technology Primitives"), 0, 8, us_tecedredialogitems, 0, 0};

/* Technology Options */
static DIALOGITEM us_techsetdialogitems[] =
{
 /*  1 */ {0, {240,328,264,392}, BUTTON, N_("OK")},
 /*  2 */ {0, {240,244,264,308}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,20,48,156}, MESSAGE, N_("Metal layers:")},
 /*  4 */ {0, {32,164,48,312}, POPUP, x_("")},
 /*  5 */ {0, {216,20,232,156}, RADIO, N_("Full Geometry")},
 /*  6 */ {0, {216,164,232,300}, RADIO, N_("Stick Figures")},
 /*  7 */ {0, {88,320,104,424}, MESSAGE, N_("Artwork:")},
 /*  8 */ {0, {112,332,128,468}, CHECK, N_("Arrows filled")},
 /*  9 */ {0, {144,320,160,424}, MESSAGE, N_("Schematics:")},
 /* 10 */ {0, {168,332,184,492}, MESSAGE, N_("Negating Bubble Size")},
 /* 11 */ {0, {168,496,184,556}, EDITTEXT, x_("")},
 /* 12 */ {0, {8,8,24,192}, MESSAGE, N_("MOSIS CMOS:")},
 /* 13 */ {0, {136,320,137,624}, DIVIDELINE, x_("")},
 /* 14 */ {0, {4,316,228,317}, DIVIDELINE, x_("")},
 /* 15 */ {0, {144,20,160,260}, CHECK, N_("Disallow stacked vias")},
 /* 16 */ {0, {56,32,72,300}, RADIO, N_("SCMOS rules (4 metal or less)")},
 /* 17 */ {0, {76,32,92,300}, RADIO, N_("Submicron rules")},
 /* 18 */ {0, {96,32,112,300}, RADIO, N_("Deep rules (5 metal or more)")},
 /* 19 */ {0, {168,20,184,300}, CHECK, N_("Alternate Active and Poly contact rules")},
 /* 20 */ {0, {32,332,48,468}, MESSAGE, N_("Metal layers:")},
 /* 21 */ {0, {32,476,48,612}, POPUP, x_("")},
 /* 22 */ {0, {8,320,24,584}, MESSAGE, N_("MOSIS CMOS Submicron (old):")},
 /* 23 */ {0, {80,320,81,624}, DIVIDELINE, x_("")},
 /* 24 */ {0, {56,332,72,624}, CHECK, N_("Automatically convert to new MOSIS CMOS")},
 /* 25 */ {0, {120,20,136,300}, CHECK, N_("Second Polysilicon layer")},
 /* 26 */ {0, {192,332,208,624}, MESSAGE, N_("Use Lambda values from this Technology:")},
 /* 27 */ {0, {212,388,228,552}, POPUP, x_("")},
 /* 28 */ {0, {192,20,208,300}, CHECK, N_("Show Special transistors")}
};
static DIALOG us_techsetdialog = {{75,75,348,708}, N_("Technology Options"), 0, 28, us_techsetdialogitems, 0, 0};

/* Text (Layout) */
static DIALOGITEM us_spelldialogitems[] =
{
 /*  1 */ {0, {196,192,220,272}, BUTTON, N_("OK")},
 /*  2 */ {0, {196,12,220,92}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,200,24,248}, EDITTEXT, x_("")},
 /*  4 */ {0, {8,8,24,196}, MESSAGE, N_("Size (max 63):")},
 /*  5 */ {0, {164,8,180,72}, MESSAGE, N_("Message:")},
 /*  6 */ {0, {164,76,180,272}, EDITTEXT, x_("")},
 /*  7 */ {0, {136,76,152,272}, POPUP, x_("")},
 /*  8 */ {0, {136,8,152,72}, MESSAGE, N_("Layer:")},
 /*  9 */ {0, {88,76,104,268}, POPUP, x_("")},
 /* 10 */ {0, {88,8,104,72}, MESSAGE, N_("Font:")},
 /* 11 */ {0, {112,8,128,92}, CHECK, N_("Italic")},
 /* 12 */ {0, {112,100,128,168}, CHECK, N_("Bold")},
 /* 13 */ {0, {112,176,128,272}, CHECK, N_("Underline")},
 /* 14 */ {0, {36,200,52,248}, EDITTEXT, x_("")},
 /* 15 */ {0, {36,8,52,196}, MESSAGE, N_("Scale factor:")},
 /* 16 */ {0, {64,200,80,248}, EDITTEXT, x_("")},
 /* 17 */ {0, {64,8,80,196}, MESSAGE, N_("Dot separation (lambda):")}
};
static DIALOG us_spelldialog = {{75,75,304,356}, N_("Create Text Layout"), 0, 17, us_spelldialogitems, 0, 0};

/* Text Options */
static DIALOGITEM us_deftextdialogitems[] =
{
 /*  1 */ {0, {368,328,392,400}, BUTTON, N_("OK")},
 /*  2 */ {0, {368,212,392,284}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {296,32,312,88}, RADIO, N_("Left")},
 /*  4 */ {0, {28,196,44,244}, EDITTEXT, x_("")},
 /*  5 */ {0, {232,32,248,104}, RADIO, N_("Center")},
 /*  6 */ {0, {248,32,264,104}, RADIO, N_("Bottom")},
 /*  7 */ {0, {264,32,280,88}, RADIO, N_("Top")},
 /*  8 */ {0, {280,32,296,96}, RADIO, N_("Right")},
 /*  9 */ {0, {312,32,328,128}, RADIO, N_("Lower right")},
 /* 10 */ {0, {328,32,344,128}, RADIO, N_("Lower left")},
 /* 11 */ {0, {344,32,360,128}, RADIO, N_("Upper right")},
 /* 12 */ {0, {360,32,376,120}, RADIO, N_("Upper left")},
 /* 13 */ {0, {376,32,392,104}, RADIO, N_("Boxed")},
 /* 14 */ {0, {68,8,84,135}, RADIO, N_("Exports & Ports")},
 /* 15 */ {0, {216,8,232,103}, MESSAGE, N_("Text corner:")},
 /* 16 */ {0, {232,136,264,168}, ICON, (CHAR *)us_icon200},
 /* 17 */ {0, {264,136,296,168}, ICON, (CHAR *)us_icon201},
 /* 18 */ {0, {296,136,328,168}, ICON, (CHAR *)us_icon202},
 /* 19 */ {0, {328,136,360,168}, ICON, (CHAR *)us_icon203},
 /* 20 */ {0, {360,136,392,168}, ICON, (CHAR *)us_icon204},
 /* 21 */ {0, {336,308,352,380}, RADIO, N_("Outside")},
 /* 22 */ {0, {248,224,264,296}, RADIO, N_("Off")},
 /* 23 */ {0, {216,204,232,400}, MESSAGE, N_("Smart Vertical Placement:")},
 /* 24 */ {0, {236,308,252,380}, RADIO, N_("Inside")},
 /* 25 */ {0, {260,308,276,380}, RADIO, N_("Outside")},
 /* 26 */ {0, {324,224,340,296}, RADIO, N_("Off")},
 /* 27 */ {0, {292,204,308,400}, MESSAGE, N_("Smart Horizontal Placement:")},
 /* 28 */ {0, {312,308,328,380}, RADIO, N_("Inside")},
 /* 29 */ {0, {184,8,200,280}, CHECK, N_("New text visible only inside cell")},
 /* 30 */ {0, {160,100,176,224}, POPUP, x_("")},
 /* 31 */ {0, {160,8,176,99}, MESSAGE, N_("Text editor:")},
 /* 32 */ {0, {4,12,20,411}, MESSAGE, N_("Default text information for different types of text:")},
 /* 33 */ {0, {52,196,68,244}, EDITTEXT, x_("")},
 /* 34 */ {0, {28,8,44,135}, RADIO, N_("Nodes")},
 /* 35 */ {0, {80,140,96,228}, MESSAGE, N_("Type face:")},
 /* 36 */ {0, {48,8,64,135}, RADIO, N_("Arcs")},
 /* 37 */ {0, {28,252,44,412}, RADIO, N_("Points (max 63)")},
 /* 38 */ {0, {108,8,124,135}, RADIO, N_("Instance names")},
 /* 39 */ {0, {52,252,68,412}, RADIO, N_("Lambda (max 127.75)")},
 /* 40 */ {0, {88,8,104,135}, RADIO, N_("Nonlayout text")},
 /* 41 */ {0, {208,8,209,412}, DIVIDELINE, x_("")},
 /* 42 */ {0, {152,8,153,412}, DIVIDELINE, x_("")},
 /* 43 */ {0, {209,188,392,189}, DIVIDELINE, x_("")},
 /* 44 */ {0, {100,140,116,376}, POPUP, x_("")},
 /* 45 */ {0, {128,140,144,212}, CHECK, N_("Italic")},
 /* 46 */ {0, {128,236,144,296}, CHECK, N_("Bold")},
 /* 47 */ {0, {128,324,144,412}, CHECK, N_("Underline")},
 /* 48 */ {0, {40,140,56,188}, MESSAGE, N_("Size")},
 /* 49 */ {0, {128,8,144,135}, RADIO, N_("Cell text")}
};
static DIALOG us_deftextdialog = {{50,75,451,497}, N_("Text Options"), 0, 49, us_deftextdialogitems, 0, 0};

/* Translate */
static DIALOGITEM us_transdialogitems[] =
{
 /*  1 */ {0, {336,132,360,212}, BUTTON, N_("Next")},
 /*  2 */ {0, {148,8,164,96}, MESSAGE, N_("Translation:")},
 /*  3 */ {0, {148,100,196,528}, EDITTEXT, x_("")},
 /*  4 */ {0, {8,324,24,472}, MESSAGE, N_("Total strings:")},
 /*  5 */ {0, {8,473,24,529}, MESSAGE, x_("")},
 /*  6 */ {0, {28,324,44,472}, MESSAGE, N_("Untranslated strings:")},
 /*  7 */ {0, {28,472,44,528}, MESSAGE, x_("")},
 /*  8 */ {0, {48,324,64,472}, MESSAGE, N_("Fuzzy strings:")},
 /*  9 */ {0, {48,472,64,528}, MESSAGE, x_("")},
 /* 10 */ {0, {92,8,108,96}, MESSAGE, N_("English:")},
 /* 11 */ {0, {92,100,140,528}, EDITTEXT, x_("")},
 /* 12 */ {0, {320,440,344,520}, BUTTON, N_("DONE")},
 /* 13 */ {0, {172,8,188,96}, CHECK, N_("Fuzzy")},
 /* 14 */ {0, {336,220,360,300}, BUTTON, N_("Prev")},
 /* 15 */ {0, {336,12,360,92}, BUTTON, N_("First")},
 /* 16 */ {0, {48,8,80,316}, MESSAGE, x_("")},
 /* 17 */ {0, {288,441,312,521}, BUTTON, N_("Save")},
 /* 18 */ {0, {8,8,24,132}, MESSAGE, N_("Language:")},
 /* 19 */ {0, {8,132,24,188}, POPUP, x_("")},
 /* 20 */ {0, {204,100,264,528}, SCROLL, x_("")},
 /* 21 */ {0, {204,8,220,96}, MESSAGE, N_("Comments:")},
 /* 22 */ {0, {272,12,288,100}, MESSAGE, N_("Choose:")},
 /* 23 */ {0, {272,100,288,300}, POPUP, x_("")},
 /* 24 */ {0, {8,192,24,304}, POPUP, x_("")},
 /* 25 */ {0, {272,328,296,408}, BUTTON, N_("Unix-to-Mac")},
 /* 26 */ {0, {305,328,329,408}, BUTTON, N_("Mac-to-UNIX")},
 /* 27 */ {0, {292,100,308,300}, EDITTEXT, x_("")},
 /* 28 */ {0, {292,24,308,84}, BUTTON, N_("Find:")},
 /* 29 */ {0, {68,324,84,472}, MESSAGE, N_("Found strings:")},
 /* 30 */ {0, {68,472,84,528}, MESSAGE, x_("")},
 /* 31 */ {0, {312,24,328,152}, CHECK, N_("Find in English")},
 /* 32 */ {0, {312,156,328,300}, CHECK, N_("Find in translation")},
 /* 33 */ {0, {337,328,361,408}, BUTTON, N_("Validate")}
};
static DIALOG us_transdialog = {{75,75,444,613}, N_("Translate Electric Strings"), 0, 33, us_transdialogitems, 0, 0};

/* Undo Control */
static DIALOGITEM db_undodialogitems[] =
{
 /*  1 */ {0, {468,608,492,680}, BUTTON, N_("OK")},
 /*  2 */ {0, {32,8,455,690}, SCROLL, x_("")},
 /*  3 */ {0, {468,16,492,88}, BUTTON, N_("Undo")},
 /*  4 */ {0, {8,8,24,241}, MESSAGE, N_("These are the recent changes:")},
 /*  5 */ {0, {8,344,24,456}, CHECK, N_("Show details")},
 /*  6 */ {0, {468,108,492,180}, BUTTON, N_("Redo")}
};
static DIALOG db_undodialog = {{50,75,555,775}, N_("Change History"), 0, 6, db_undodialogitems, 0, 0};

/* VHDL Options */
static DIALOGITEM vhdl_optionsdialogitems[] =
{
 /*  1 */ {0, {336,244,360,308}, BUTTON, N_("OK")},
 /*  2 */ {0, {300,244,324,308}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {308,4,324,166}, CHECK, N_("VHDL stored in cell")},
 /*  4 */ {0, {332,4,348,179}, CHECK, N_("Netlist stored in cell")},
 /*  5 */ {0, {28,8,224,319}, SCROLL, x_("")},
 /*  6 */ {0, {236,4,252,203}, MESSAGE, N_("VHDL for primitive:")},
 /*  7 */ {0, {260,4,276,203}, MESSAGE, N_("VHDL for negated primitive:")},
 /*  8 */ {0, {236,208,252,319}, EDITTEXT, x_("")},
 /*  9 */ {0, {260,208,276,319}, EDITTEXT, x_("")},
 /* 10 */ {0, {8,8,24,183}, MESSAGE, N_("Schematics primitives:")},
 /* 11 */ {0, {292,8,293,319}, DIVIDELINE, x_("")}
};
static DIALOG vhdl_optionsdialog = {{50,75,419,403}, N_("VHDL Options"), 0, 11, vhdl_optionsdialogitems, 0, 0};

/* Variables */
static DIALOGITEM us_variabledialogitems[] =
{
 /*  1 */ {0, {408,344,432,400}, BUTTON, N_("OK")},
 /*  2 */ {0, {352,8,368,56}, MESSAGE, N_("Value:")},
 /*  3 */ {0, {336,8,337,408}, DIVIDELINE, x_("")},
 /*  4 */ {0, {24,8,40,64}, MESSAGE, N_("Object:")},
 /*  5 */ {0, {8,80,24,240}, RADIO, N_("Currently Highlighted")},
 /*  6 */ {0, {56,256,72,408}, RADIO, N_("Current Constraint")},
 /*  7 */ {0, {24,80,40,240}, RADIO, N_("Current Cell")},
 /*  8 */ {0, {40,80,56,240}, RADIO, N_("Current Library")},
 /*  9 */ {0, {8,256,24,408}, RADIO, N_("Current Technology")},
 /* 10 */ {0, {24,256,40,408}, RADIO, N_("Current Tool")},
 /* 11 */ {0, {144,24,160,96}, MESSAGE, N_("Attribute:")},
 /* 12 */ {0, {160,8,304,184}, SCROLL, x_("")},
 /* 13 */ {0, {312,32,328,152}, CHECK, N_("New Attribute:")},
 /* 14 */ {0, {312,160,328,400}, EDITTEXT, x_("")},
 /* 15 */ {0, {216,192,232,251}, CHECK, N_("Array")},
 /* 16 */ {0, {240,200,256,248}, MESSAGE, N_("Index:")},
 /* 17 */ {0, {240,250,256,312}, EDITTEXT, x_("")},
 /* 18 */ {0, {408,192,432,296}, BUTTON, N_("Set Attribute")},
 /* 19 */ {0, {344,80,376,400}, EDITTEXT, x_("")},
 /* 20 */ {0, {408,24,432,144}, BUTTON, N_("Delete Attribute")},
 /* 21 */ {0, {168,192,184,288}, CHECK, N_("Displayable")},
 /* 22 */ {0, {192,192,208,288}, CHECK, N_("Temporary")},
 /* 23 */ {0, {276,224,300,361}, BUTTON, N_("Examine Attribute")},
 /* 24 */ {0, {112,40,128,80}, MESSAGE, N_("Type:")},
 /* 25 */ {0, {112,80,128,216}, MESSAGE, x_("")},
 /* 26 */ {0, {144,184,160,224}, MESSAGE, N_("Type:")},
 /* 27 */ {0, {144,224,160,383}, MESSAGE, x_("")},
 /* 28 */ {0, {136,8,137,408}, DIVIDELINE, x_("")},
 /* 29 */ {0, {80,80,112,408}, MESSAGE, x_("")},
 /* 30 */ {0, {80,32,96,80}, MESSAGE, N_("Name:")},
 /* 31 */ {0, {168,304,184,410}, POPUP, x_("")},
 /* 32 */ {0, {384,80,400,160}, MESSAGE, N_("Evaluation:")},
 /* 33 */ {0, {384,160,400,400}, MESSAGE, x_("")},
 /* 34 */ {0, {232,320,248,366}, BUTTON, N_("Next")},
 /* 35 */ {0, {248,320,264,366}, BUTTON, N_("Prev")},
 /* 36 */ {0, {40,256,56,408}, RADIO, N_("Current Window")}
};
static DIALOG us_variabledialog = {{50,75,492,495}, N_("Variable Control"), 0, 36, us_variabledialogitems, 0, 0};

/* Verilog Options */
static DIALOGITEM sim_verilogoptdialogitems[] =
{
 /*  1 */ {0, {144,396,168,454}, BUTTON, N_("OK")},
 /*  2 */ {0, {144,268,168,326}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {80,216,96,375}, RADIO, N_("Use Model from File:")},
 /*  4 */ {0, {56,216,72,426}, RADIO, N_("Derive Model from Circuitry")},
 /*  5 */ {0, {80,392,96,463}, BUTTON, N_("Browse")},
 /*  6 */ {0, {104,224,136,480}, EDITTEXT, x_("")},
 /*  7 */ {0, {8,256,24,428}, CHECK, N_("Use ASSIGN Construct")},
 /*  8 */ {0, {28,8,168,206}, SCROLL, x_("")},
 /*  9 */ {0, {8,8,24,72}, MESSAGE, N_("Library:")},
 /* 10 */ {0, {8,76,24,206}, POPUP, x_("")},
 /* 11 */ {0, {32,256,48,428}, CHECK, N_("Default wire is Trireg")}
};
static DIALOG sim_verilogoptdialog = {{50,75,227,564}, N_("Verilog Options"), 0, 11, sim_verilogoptdialogitems, 0, 0};

/* Well Check Options */
static DIALOGITEM erc_optionsdialogitems[] =
{
 /*  1 */ {0, {156,296,180,360}, BUTTON, N_("OK")},
 /*  2 */ {0, {156,108,180,172}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,8,48,228}, RADIO, N_("Must have contact in every area")},
 /*  4 */ {0, {56,8,72,228}, RADIO, N_("Must have at least 1 contact")},
 /*  5 */ {0, {80,8,96,228}, RADIO, N_("Do not check for contacts")},
 /*  6 */ {0, {8,44,24,144}, MESSAGE, N_("For P-Well:")},
 /*  7 */ {0, {32,248,48,480}, RADIO, N_("Must have contact in every area")},
 /*  8 */ {0, {56,248,72,480}, RADIO, N_("Must have at least 1 contact")},
 /*  9 */ {0, {80,248,96,480}, RADIO, N_("Do not check for contacts")},
 /* 10 */ {0, {8,284,24,384}, MESSAGE, N_("For N-Well:")},
 /* 11 */ {0, {104,8,120,228}, CHECK, N_("Must connect to Ground")},
 /* 12 */ {0, {104,248,120,480}, CHECK, N_("Must connect to Power")},
 /* 13 */ {0, {128,60,144,448}, CHECK, N_("Find farthest distance from contact to edge")}
};
static DIALOG erc_optionsdialog = {{50,75,239,564}, N_("Well Check Options"), 0, 13, erc_optionsdialogitems, 0, 0};

/* test */
static DIALOGITEM us_testdialogitems[] =
{
 /*  1 */ {0, {172,164,196,244}, BUTTON, N_("Done")},
 /*  2 */ {0, {28,8,44,88}, BUTTON, N_("Button")},
 /*  3 */ {0, {28,100,44,216}, CHECK, N_("Check Box")},
 /*  4 */ {0, {56,8,72,124}, RADIO, N_("Radio 1")},
 /*  5 */ {0, {76,8,92,124}, RADIO, N_("Radio 2")},
 /*  6 */ {0, {96,8,112,124}, RADIO, N_("Radio 3")},
 /*  7 */ {0, {132,8,232,132}, SCROLL, x_("")},
 /*  8 */ {0, {4,4,20,220}, MESSAGE, N_("A Modeless Dialog")},
 /*  9 */ {0, {56,140,72,280}, EDITTEXT, x_("")},
 /* 10 */ {0, {120,8,121,288}, DIVIDELINE, x_("")},
 /* 11 */ {0, {93,140,109,280}, POPUP, x_("")}
};
static DIALOG us_testdialog = {{75,75,316,373}, 0, 0, 11, us_testdialogitems, 0, 0};

