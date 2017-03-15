//---------------------------------------------------------------------------

#ifndef UnitNametableOffsetH
#define UnitNametableOffsetH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>
//---------------------------------------------------------------------------
class TFormNameOffset : public TForm
{
__published:	// IDE-managed Components
	TGroupBox *GroupBox1;
	TButton *ButtonOk;
	TButton *ButtonCancel;
	TLabel *LabelFromTo;
	TLabel *LabelOffset;
	TLabel *LabelInfo;
	TEdit *EditFrom;
	TEdit *EditTo;
	TEdit *EditOffset;
	void __fastcall ButtonOkClick(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall ButtonCancelClick(TObject *Sender);
	void __fastcall EditFromKeyPress(TObject *Sender, char &Key);
	void __fastcall EditFromChange(TObject *Sender);
	void __fastcall EditFromMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall FormShow(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TFormNameOffset(TComponent* Owner);
	void __fastcall UpdateInfo(void);
	int From;
	int To;
	int Offset;
	bool MakeOffset;
	int Mode;
};
//---------------------------------------------------------------------------
extern PACKAGE TFormNameOffset *FormNameOffset;
//---------------------------------------------------------------------------
#endif
