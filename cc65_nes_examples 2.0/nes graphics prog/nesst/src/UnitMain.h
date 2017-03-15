//---------------------------------------------------------------------------

#ifndef UnitMainH
#define UnitMainH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <Buttons.hpp>
#include <Graphics.hpp>
#include <Dialogs.hpp>
#include <Menus.hpp>
#include <ComCtrls.hpp>

//---------------------------------------------------------------------------

#define REG_SECTION "Software\\Shiru\\NES Screen Tool\\"

#define NAME_MAX_WIDTH		4096
#define NAME_MAX_HEIGHT		4096
#define NAME_MAX_SIZE		(NAME_MAX_WIDTH*NAME_MAX_HEIGHT)
#define ATTR_MAX_SIZE		((NAME_MAX_WIDTH/4)*(NAME_MAX_HEIGHT/4))

#define OAM_FLIP_H	64
#define OAM_FLIP_V	128

enum {
	SAVE_NAMETABLE_NAM=1,
	SAVE_NAMETABLE_BIN,
	SAVE_NAMETABLE_ASM ,
	SAVE_NAMETABLE_H,
	SAVE_NAMETABLE_RLE,
	SAVE_NAMETABLE_RLE_ASM,
	SAVE_NAMETABLE_RLE_H
};

enum {
	SAVE_DATA_BIN=0,
	SAVE_DATA_H,
	SAVE_DATA_ASM
};

const char regWorkingDirectory[]="WorkDir";
const char colBlack=0x0f;

const char sessionIDStr[8]="NSTssTXT";

float emphasis[8][3]={//from nintech.txt
	{100.0,100.0,100.0},
	{ 74.3, 91.5,123.9},
	{ 88.2,108.6, 79.4},
	{ 65.3, 98.0,101.9},
	{127.7,102.6, 90.5},
	{ 97.9, 90.8,102.3},
	{100.1, 98.7, 74.1},
	{ 75.0, 75.0, 75.0}
};

AnsiString FormCaption;


//---------------------------------------------------------------------------

class TFormMain : public TForm
{
	__published:	// IDE-managed Components
	TOpenDialog *OpenDialogChr;
	TMainMenu *MainMenu1;
	TMenuItem *MPatterns;
	TMenuItem *MCHROpen;
	TMenuItem *MCHRSave;
	TOpenDialog *OpenDialogName;
	TSaveDialog *SaveDialogName;
	TMenuItem *MNameTable;
	TMenuItem *MOpenNameTable;
	TMenuItem *MSaveNameTableOnly;
	TMenuItem *MSaveNameTableAttributes;
	TMenuItem *MPalettes;
	TMenuItem *MOpenPalettes;
	TMenuItem *MSavePalettes;
	TOpenDialog *OpenDialogPal;
	TSaveDialog *SaveDialogPal;
	TMenuItem *MImport;
	TMenuItem *MCHREditor;
	TOpenDialog *OpenDialogImport;
	TMenuItem *MAll;
	TOpenDialog *OpenDialogAll;
	TMenuItem *MPutSelectedBlockToClipboard;
	TMenuItem *MCHRClear;
	TMenuItem *N1;
	TMenuItem *MCHRRemoveDoubles;
	TMenuItem *N2;
	TMenuItem *N3;
	TSaveDialog *SaveDialogChr;
	TMenuItem *MCHRInterleave;
	TMenuItem *MCHRDeinterleave;
	TMenuItem *MCHRSwapColors;
	TMenuItem *MAddOffset;
	TMenuItem *MExport;
	TMenuItem *MExportNametableBMP;
	TMenuItem *MExportTilesetBMP;
	TSaveDialog *SaveDialogImage;
	TMenuItem *N4;
	TMenuItem *MPutMetaSpriteToClipboard;
	TMenuItem *MSaveAttributes;
	TSaveDialog *SaveDialogAttr;
	TMenuItem *MCHRFillNumbers;
	TMenuItem *MCHRFindDoubles;
	TMenuItem *N6;
	TMenuItem *N7;
	TMenuItem *N8;
	TMenuItem *MImportBMPNametable;
	TMenuItem *MImportNES;
	TMenuItem *MExportNES;
	TSaveDialog *SaveDialogExportNES;
	TMenuItem *MImportBMPTileset;
	TMenuItem *MCHRSwapBanks;
	TMenuItem *MOpenAll;
	TMenuItem *MSaveAll;
	TMenuItem *N9;
	TMenuItem *MSaveSession;
	TMenuItem *MLoadSession;
	TOpenDialog *OpenDialogSession;
	TSaveDialog *SaveDialogSession;
	TMenuItem *MExportPaletteBMP;
	TMenuItem *N10;
	TMenuItem *MMetaSprites;
	TMenuItem *MOpenMetaSpriteBank;
	TMenuItem *MSaveMetaSpriteBank;
	TMenuItem *MMetaSpritePutBankToClipboardC;
	TMenuItem *N11;
	TOpenDialog *OpenDialogMetaSpriteBank;
	TSaveDialog *SaveDialogMetaSpriteBank;
	TMenuItem *MMetaSpritePutToClipboardC;
	TMenuItem *MCHRFindUnused;
	TMenuItem *MCHRRemoveUnused;
	TMenuItem *N12;
	TMenuItem *MMetaSpritePutToClipboardAsm;
	TMenuItem *MMetaSpritePutBankToClipboardAsm;
	TMenuItem *MCHRSave1K;
	TMenuItem *MCHRSave2K;
	TMenuItem *MCHRSave4K;
	TMenuItem *MCHRSave8K;
	TMenuItem *MCHRSaveSelection;
	TMenuItem *MPutSelectedBlockToClipboardC;
	TMenuItem *MPutSelectedBlockToClipboardASM;
	TMenuItem *MPutMetaSpriteToClipboardCNoFlip;
	TMenuItem *MPutMetaSpriteToClipboardCHFlip;
	TMenuItem *N5;
	TMenuItem *N13;
	TMenuItem *MCHRSaveLatest;
	TMenuItem *MPutPaletteToClipboard;
	TMenuItem *MPutPaletteToClipboardAssembly;
	TMenuItem *MPutPaletteToClipboardASM;
	TMenuItem *MPutPaletteToClipboardC;
	TMenuItem *MPutSelectedBlockToClipboardCRLE;
	TMenuItem *N14;
	TMenuItem *MPutSelectionToMetasprite;
	TMenuItem *MView;
	TMenuItem *MView2x;
	TMenuItem *MView3x;
	TMenuItem *MView4x;
	TMenuItem *MReplaceTile;
	TPanel *PanelToolbar;
	TGroupBox *GroupBoxTiles;
	TGroupBox *GroupBoxPal;
	TPaintBox *PaintBoxPal;
	TSpeedButton *SpeedButtonPal;
	TLabel *Label1;
	TLabel *Label4;
	TLabel *Label5;
	TLabel *Label8;
	TLabel *Label9;
	TSpeedButton *SpeedButtonMaskB;
	TSpeedButton *SpeedButtonMaskG;
	TSpeedButton *SpeedButtonMaskR;
	TSpeedButton *SpeedButtonMaskM;
	TPanel *PanelEditArea;
	TPageControl *PageControlEditor;
	TTabSheet *TabSheetName;
	TImage *ImageName;
	TTabSheet *TabSheetSprite;
	TGroupBox *GroupBoxSpriteList;
	TSpeedButton *SpeedButtonSpriteDel;
	TSpeedButton *SpeedButtonSpriteUp;
	TSpeedButton *SpeedButtonSpriteDown;
	TSpeedButton *SpeedButtonSpriteHFlip;
	TSpeedButton *SpeedButtonSpriteVFlip;
	TSpeedButton *SpeedButtonSpriteBank;
	TSpeedButton *SpeedButtonSpriteAll;
	TSpeedButton *SpeedButtonSpriteNone;
	TListBox *ListBoxSpriteList;
	TGroupBox *GroupBoxMetaSprite;
	TGroupBox *GroupBoxStats;
	TLabel *LabelStats;
	TPanel *PanelTileset;
	TImage *ImageTiles;
	TPanel *PanelTilesetTools;
	TSpeedButton *SpeedButtonTiles;
	TSpeedButton *SpeedButtonChecker;
	TSpeedButton *SpeedButtonTypeIn;
	TLabel *Label6;
	TLabel *Label11;
	TLabel *Label2;
	TStaticText *StaticTextFontOffset;
	TSpeedButton *SpeedButtonSelTiles;
	TLabel *Label3;
	TSpeedButton *SpeedButtonGridAll;
	TSpeedButton *SpeedButtonGridTile;
	TSpeedButton *SpeedButtonGridAtr;
	TSpeedButton *SpeedButtonGridBlock;
	TLabel *Label12;
	TLabel *Label10;
	TSpeedButton *SpeedButtonChrBank1;
	TSpeedButton *SpeedButtonChrBank2;
	TPanel *PanelSpriteToolbar;
	TSpeedButton *SpeedButtonPrevMetaSprite;
	TLabel *LabelMetaSprite;
	TSpeedButton *SpeedButtonNextMetaSprite;
	TSpeedButton *SpeedButtonSpriteGrid;
	TSpeedButton *SpeedButtonClearMetaSprite;
	TSpeedButton *SpeedButtonSpriteSnap;
	TSpeedButton *SpeedButtonMetaSpriteVFlip;
	TSpeedButton *SpeedButtonMetaSpriteHFlip;
	TSpeedButton *SpeedButtonMetaSpritePaste;
	TSpeedButton *SpeedButtonMetaSpriteCopy;
	TLabel *Label7;
	TSpeedButton *SpeedButtonFrameAll;
	TSpeedButton *SpeedButtonFrameSelected;
	TSpeedButton *SpeedButtonFrameNone;
	TSpeedButton *SpeedButtonSprite8x16;
	TPanel *PanelSpriteView;
	TImage *ImageMetaSprite;
	TMenuItem *MImportBestOffsets;
	TMenuItem *MImportLossy;
	TMenuItem *N15;
	TMenuItem *N16;
	TMenuItem *MImportThreshold;
	TMenuItem *MCHRFreqSort;
	TMenuItem *MPutSelectionToMetaspriteAutoInc;
	TMenuItem *N17;
	TMenuItem *MNameTableNew;
	TMenuItem *MSaveMap;
	TSaveDialog *SaveDialogMap;
	TMenuItem *N18;
	TMenuItem *N19;
	void __fastcall FormPaint(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall PaintBoxPalMouseDown(TObject *Sender, TMouseButton Button,
	TShiftState Shift, int X, int Y);
	void __fastcall ImageTilesMouseDown(TObject *Sender, TMouseButton Button,
	TShiftState Shift, int X, int Y);
	void __fastcall SpeedButtonGridAllClick(TObject *Sender);
	void __fastcall MCHROpenClick(TObject *Sender);
	void __fastcall PaintBoxNamePaint(TObject *Sender);
	void __fastcall ImageNameMouseDown(TObject *Sender, TMouseButton Button,
	TShiftState Shift, int X, int Y);
	void __fastcall ImageNameMouseMove(TObject *Sender, TShiftState Shift, int X,
	int Y);
	void __fastcall MOpenNameTableClick(TObject *Sender);
	void __fastcall MSaveNameTableOnlyClick(TObject *Sender);
	void __fastcall MSaveNameTableAttributesClick(TObject *Sender);
	void __fastcall MOpenPalettesClick(TObject *Sender);
	void __fastcall MSavePalettesClick(TObject *Sender);
	void __fastcall PaintBoxPalPaint(TObject *Sender);
	void __fastcall MCHREditorClick(TObject *Sender);
	void __fastcall FormKeyDown(TObject *Sender, WORD &Key, TShiftState Shift);
	void __fastcall ImageTilesDblClick(TObject *Sender);
	void __fastcall FormDestroy(TObject *Sender);
	void __fastcall ImageNameMouseLeave(TObject *Sender);
	void __fastcall ImageTilesMouseLeave(TObject *Sender);
	void __fastcall ImageTilesMouseMove(TObject *Sender, TShiftState Shift, int X,
	int Y);
	void __fastcall MCHRClearClick(TObject *Sender);
	void __fastcall SpeedButtonChrBank1Click(TObject *Sender);
	void __fastcall MCHRRemoveDoublesClick(TObject *Sender);
	void __fastcall PaintBoxPalMouseMove(TObject *Sender, TShiftState Shift, int X,
	int Y);
	void __fastcall PaintBoxPalMouseLeave(TObject *Sender);
	void __fastcall MCHRInterleaveClick(TObject *Sender);
	void __fastcall MCHRDeinterleaveClick(TObject *Sender);
	void __fastcall MCHRSwapColorsClick(TObject *Sender);
	void __fastcall MAddOffsetClick(TObject *Sender);
	void __fastcall SpeedButtonMaskBClick(TObject *Sender);
	void __fastcall SpeedButtonMaskGClick(TObject *Sender);
	void __fastcall SpeedButtonMaskRClick(TObject *Sender);
	void __fastcall SpeedButtonMaskMClick(TObject *Sender);
	void __fastcall MExportNametableBMPClick(TObject *Sender);
	void __fastcall MExportTilesetBMPClick(TObject *Sender);
	void __fastcall SpeedButtonCheckerClick(TObject *Sender);
	void __fastcall MSaveAttributesClick(TObject *Sender);
	void __fastcall MCHRFillNumbersClick(TObject *Sender);
	void __fastcall MCHRFindDoublesClick(TObject *Sender);
	void __fastcall MImportBMPNametableClick(TObject *Sender);
	void __fastcall MImportNESClick(TObject *Sender);
	void __fastcall MExportNESClick(TObject *Sender);
	void __fastcall MImportBMPTilesetClick(TObject *Sender);
	void __fastcall MCHRSwapBanksClick(TObject *Sender);
	void __fastcall MOpenAllClick(TObject *Sender);
	void __fastcall MSaveAllClick(TObject *Sender);
	void __fastcall MLoadSessionClick(TObject *Sender);
	void __fastcall MSaveSessionClick(TObject *Sender);
	void __fastcall MExportPaletteBMPClick(TObject *Sender);
	void __fastcall SpeedButtonTypeInClick(TObject *Sender);
	void __fastcall FormKeyPress(TObject *Sender, char &Key);
	void __fastcall PageControlEditorChange(TObject *Sender);
	void __fastcall ImageMetaSpriteDragOver(TObject *Sender, TObject *Source,
	int X, int Y, TDragState State, bool &Accept);
	void __fastcall ImageMetaSpriteDragDrop(TObject *Sender, TObject *Source,
	int X, int Y);
	void __fastcall ImageMetaSpriteEndDrag(TObject *Sender, TObject *Target, int X,
	int Y);
	void __fastcall SpeedButtonPrevMetaSpriteClick(TObject *Sender);
	void __fastcall SpeedButtonNextMetaSpriteClick(TObject *Sender);
	void __fastcall SpeedButtonClearMetaSpriteClick(TObject *Sender);
	void __fastcall SpeedButtonSpriteDelClick(TObject *Sender);
	void __fastcall ListBoxSpriteListClick(TObject *Sender);
	void __fastcall SpeedButtonFrameSelectedClick(TObject *Sender);
	void __fastcall SpeedButtonSpriteUpClick(TObject *Sender);
	void __fastcall SpeedButtonSpriteDownClick(TObject *Sender);
	void __fastcall ListBoxSpriteListKeyDown(TObject *Sender, WORD &Key,
	TShiftState Shift);
	void __fastcall ImageMetaSpriteMouseDown(TObject *Sender, TMouseButton Button,
	TShiftState Shift, int X, int Y);
	void __fastcall ImageMetaSpriteMouseLeave(TObject *Sender);
	void __fastcall ImageMetaSpriteMouseMove(TObject *Sender, TShiftState Shift,
	int X, int Y);
	void __fastcall ImageMetaSpriteMouseUp(TObject *Sender, TMouseButton Button,
	TShiftState Shift, int X, int Y);
	void __fastcall MOpenMetaSpriteBankClick(TObject *Sender);
	void __fastcall MSaveMetaSpriteBankClick(TObject *Sender);
	void __fastcall MMetaSpritePutBankToClipboardCClick(TObject *Sender);
	void __fastcall MMetaSpritePutToClipboardCClick(TObject *Sender);
	void __fastcall SpeedButtonSpriteHFlipClick(TObject *Sender);
	void __fastcall SpeedButtonSpriteVFlipClick(TObject *Sender);
	void __fastcall SpeedButtonMetaSpriteCopyClick(TObject *Sender);
	void __fastcall SpeedButtonMetaSpritePasteClick(TObject *Sender);
	void __fastcall SpeedButtonMetaSpriteHFlipClick(TObject *Sender);
	void __fastcall SpeedButtonMetaSpriteVFlipClick(TObject *Sender);
	void __fastcall MCHRFindUnusedClick(TObject *Sender);
	void __fastcall MCHRRemoveUnusedClick(TObject *Sender);
	void __fastcall SpeedButtonSprite8x16Click(TObject *Sender);
	void __fastcall SpeedButtonSpriteBankClick(TObject *Sender);
	void __fastcall SpeedButtonSpriteGridClick(TObject *Sender);
	void __fastcall MMetaSpritePutToClipboardAsmClick(TObject *Sender);
	void __fastcall MMetaSpritePutBankToClipboardAsmClick(TObject *Sender);
	void __fastcall StaticTextFontOffsetMouseDown(TObject *Sender,
	TMouseButton Button, TShiftState Shift, int X, int Y);
	void __fastcall StaticTextFontOffsetMouseMove(TObject *Sender,
	TShiftState Shift, int X, int Y);
	void __fastcall ImageTilesDragOver(TObject *Sender, TObject *Source, int X,
	int Y, TDragState State, bool &Accept);
	void __fastcall ImageTilesDragDrop(TObject *Sender, TObject *Source, int X,
	int Y);
	void __fastcall MCHRSave1KClick(TObject *Sender);
	void __fastcall MCHRSave2KClick(TObject *Sender);
	void __fastcall MCHRSave4KClick(TObject *Sender);
	void __fastcall MCHRSave8KClick(TObject *Sender);
	void __fastcall MCHRSaveSelectionClick(TObject *Sender);
	void __fastcall MPutSelectedBlockToClipboardCClick(TObject *Sender);
	void __fastcall MPutSelectedBlockToClipboardASMClick(TObject *Sender);
	void __fastcall MPutMetaSpriteToClipboardCNoFlipClick(TObject *Sender);
	void __fastcall MPutMetaSpriteToClipboardCHFlipClick(TObject *Sender);
	void __fastcall MCHRSaveLatestClick(TObject *Sender);
	void __fastcall MPutPaletteToClipboardAssemblyClick(TObject *Sender);
	void __fastcall MPutPaletteToClipboardASMClick(TObject *Sender);
	void __fastcall MPutPaletteToClipboardCClick(TObject *Sender);
	void __fastcall FormMouseWheel(TObject *Sender, TShiftState Shift,
	int WheelDelta, TPoint &MousePos, bool &Handled);
	void __fastcall MPutSelectedBlockToClipboardCRLEClick(TObject *Sender);
	void __fastcall MPutSelectionToMetaspriteClick(TObject *Sender);
	void __fastcall SpeedButtonSpriteAllClick(TObject *Sender);
	void __fastcall SpeedButtonSpriteNoneClick(TObject *Sender);
	void __fastcall PaintBoxPalDragOver(TObject *Sender, TObject *Source, int X,
	int Y, TDragState State, bool &Accept);
	void __fastcall PaintBoxPalDragDrop(TObject *Sender, TObject *Source, int X,
	int Y);
	void __fastcall MView2xClick(TObject *Sender);
	void __fastcall MReplaceTileClick(TObject *Sender);
	void __fastcall MImportBestOffsetsClick(TObject *Sender);
	void __fastcall MCHRFreqSortClick(TObject *Sender);
	void __fastcall MPutSelectionToMetaspriteAutoIncClick(TObject *Sender);
	void __fastcall MNameTableNewClick(TObject *Sender);
	void __fastcall MSaveMapClick(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TFormMain(TComponent* Owner);
	void __fastcall DrawPalettes(void);
	void __fastcall DrawCol(int,int,int,int,bool);
	void __fastcall DrawBGPal(int,int,int);
	void __fastcall DrawSelection(TImage*,TRect);
	void __fastcall DrawTile(TPicture*,int,int,int,int,int,int,bool);
	void __fastcall DrawTileChecker(TPicture*,int,int,int,int,int,int,bool);
	void __fastcall DrawExportTile16(TPicture*,int,int,int,int,int,int);
	void __fastcall UpdateTiles(bool);
	void __fastcall UpdateNameTable(int,int,bool);
	void __fastcall CopyCHR(bool,bool);
	void __fastcall PasteCHR(void);
	void __fastcall NameTableScrollLeft(bool);
	void __fastcall NameTableScrollRight(bool);
	void __fastcall NameTableScrollUp(bool);
	void __fastcall NameTableScrollDown(bool);
	bool __fastcall OpenCHR(AnsiString);
	void __fastcall InterleaveCHR(bool);
	bool __fastcall OpenNameTable(AnsiString);
	bool __fastcall OpenPalette(AnsiString);
	void __fastcall CopyMap(bool);
	void __fastcall CopyMapCodeASM(void);
	void __fastcall CopyMapCodeC(bool rle);
	void __fastcall PasteMap(void);
	void __fastcall FillMap(void);
	void __fastcall GetSelection(TRect,int&,int&,int&,int&);
	void __fastcall OpenAll(AnsiString);
	void __fastcall UpdateStats(void);
	void __fastcall SetUndo(void);
	void __fastcall Undo(void);
	void __fastcall SetTile(int);
	void __fastcall CopyMetaSpriteCodeC(bool);
	void __fastcall SetBMPPalette(Graphics::TBitmap*);
	bool __fastcall LoadSession1x(AnsiString);
	bool __fastcall LoadSession2x(AnsiString filename);
	bool __fastcall LoadSessionText(AnsiString filename);
	bool __fastcall LoadSession(AnsiString);
	void __fastcall SaveSession(AnsiString);
	void __fastcall UpdateRGBM(void);
	bool __fastcall MouseTypeIn(int,int);
	void __fastcall NameTableTypeIn(int);
	void __fastcall UpdateMetaSprite(void);
	void __fastcall DrawSpriteTile(TPicture*,int,int,int,int,TColor);
	void __fastcall MoveSprite(int,int);
	void __fastcall SelectSprite(int,bool);
	void __fastcall SelectTile(int);
	void __fastcall SelectPalette(int);
	bool __fastcall OpenMetaSprites(AnsiString);
	void __fastcall FindDoublesUnused(bool);
	void __fastcall RemoveDoublesUnused(bool);
	int  __fastcall GetSpriteID(int x,int y);
	void __fastcall SpriteSnap(int id);
	AnsiString __fastcall FilterIndexToExt(int index);
	bool __fastcall OverwritePrompt(AnsiString filename);
	bool __fastcall ImportBMP(AnsiString filename,bool shift,bool squeeze,int thresh);
	void __fastcall SaveCHR(int size);
	void __fastcall SetLatestCHRSize(int size);
	void __fastcall DrawEmptyTile(TPicture *pic,int x,int y,int pal,int tx,int ty,bool sel);
	void __fastcall UpdateUIScale(void);
	int  __fastcall ImportConvertTiles(int wdt,int hgt,int xoff,int yoff,int thresh);
	void __fastcall UpdateAll(void);
	void __fastcall ClearNametable(void);
	int  __fastcall AttrGet(int x,int y);
	void __fastcall AttrSet(int x,int y,int pal);
	void __fastcall CorrectView(void);

	int nameTableWidth;
	int nameTableHeight;
};
//---------------------------------------------------------------------------
extern PACKAGE TFormMain *FormMain;
//---------------------------------------------------------------------------
#endif
