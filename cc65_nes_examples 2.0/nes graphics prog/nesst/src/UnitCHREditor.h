//---------------------------------------------------------------------------

#ifndef UnitCHREditorH
#define UnitCHREditorH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <Buttons.hpp>
//---------------------------------------------------------------------------
class TFormCHREditor : public TForm
{
__published:	// IDE-managed Components
	TGroupBox *GroupBox1;
	TPaintBox *PaintBoxChr;
	TGroupBox *GroupBox2;
	TSpeedButton *SpeedButtonHFlip;
	TSpeedButton *SpeedButtonVFlip;
	TSpeedButton *SpeedButtonRotateCCW;
	TSpeedButton *SpeedButtonRotateCW;
	void __fastcall PaintBoxChrPaint(TObject *Sender);
	void __fastcall PaintBoxChrMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall PaintBoxChrMouseMove(TObject *Sender, TShiftState Shift, int X,
          int Y);
	void __fastcall FormKeyDown(TObject *Sender, WORD &Key, TShiftState Shift);
	void __fastcall SpeedButtonHFlipClick(TObject *Sender);
	void __fastcall SpeedButtonVFlipClick(TObject *Sender);
	void __fastcall SpeedButtonRotateCCWClick(TObject *Sender);
	void __fastcall SpeedButtonRotateCWClick(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormMouseWheel(TObject *Sender, TShiftState Shift,
          int WheelDelta, TPoint &MousePos, bool &Handled);
private:	// User declarations
public:		// User declarations
	__fastcall TFormCHREditor(TComponent* Owner);
	void __fastcall DrawCHR(int,int,int);
	void __fastcall Draw(void);
	void __fastcall ScrollLeft(void);
	void __fastcall ScrollRight(void);
	void __fastcall ScrollUp(void);
	void __fastcall ScrollDown(void);
	void __fastcall MirrorHorizontal(void);
	void __fastcall MirrorVertical(void);
	void __fastcall Fill(int,int);
	void __fastcall TileChange(int,int);
	void __fastcall Flip90(bool);
};
//---------------------------------------------------------------------------
extern PACKAGE TFormCHREditor *FormCHREditor;
//---------------------------------------------------------------------------
#endif
