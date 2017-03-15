//---------------------------------------------------------------------------

#ifndef UnitNESBankH
#define UnitNESBankH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <Graphics.hpp>
#include <ComCtrls.hpp>
//---------------------------------------------------------------------------
class TFormBank : public TForm
{
__published:	// IDE-managed Components
	TGroupBox *GroupBoxInfo;
	TImage *ImageCHR1;
	TImage *ImageCHR2;
	TButton *ButtonOK;
	TButton *ButtonCancel;
	TLabel *LabelInfo;
	TEdit *EditBank;
	TUpDown *UpDownBank;
	TLabel *Label1;
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall ButtonCancelClick(TObject *Sender);
	void __fastcall ButtonOKClick(TObject *Sender);
	void __fastcall FormShow(TObject *Sender);
	void __fastcall EditBankChange(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TFormBank(TComponent* Owner);
	void __fastcall DrawBank(TPicture*,unsigned char*);
	void __fastcall WrongFile(void);
	void __fastcall ShowBank(void);
	AnsiString FileName;
	int PRG,CHR,Bank;
	bool OK;
};
//---------------------------------------------------------------------------
extern PACKAGE TFormBank *FormBank;
//---------------------------------------------------------------------------
#endif
