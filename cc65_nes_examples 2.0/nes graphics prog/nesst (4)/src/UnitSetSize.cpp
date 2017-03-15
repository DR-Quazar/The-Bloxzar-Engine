//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "UnitMain.h"
#include "UnitSetSize.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TFormSetSize *FormSetSize;
//---------------------------------------------------------------------------
__fastcall TFormSetSize::TFormSetSize(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TFormSetSize::ButtonOKClick(TObject *Sender)
{
	if(Application->MessageBox("Are you sure? Nametable content will be lost!","Confirm",MB_YESNO)!=IDYES) return;

	Confirm=true;

	if(RadioButtonNormal->Checked)
	{
		NewWidth=32;
		NewHeight=30;
	}
	else
	{
		NewWidth=UpDownWidth->Position;
		NewHeight=UpDownHeight->Position;
	}

	Close();
}
//---------------------------------------------------------------------------
void __fastcall TFormSetSize::ButtonCancelClick(TObject *Sender)
{
	Confirm=false;

	Close();	
}
//---------------------------------------------------------------------------
void __fastcall TFormSetSize::FormShow(TObject *Sender)
{
	UpDownWidth ->Max=NAME_MAX_WIDTH;
    UpDownHeight->Max=NAME_MAX_HEIGHT;
	UpDownWidth ->Position=NewWidth;
	UpDownHeight->Position=NewHeight;
}
//---------------------------------------------------------------------------


void __fastcall TFormSetSize::EditWidthKeyPress(TObject *Sender, char &Key)
{
	if(!((Key>='0'&&Key<='9')||Key==VK_BACK||Key==VK_DELETE)) Key=0;	
}
//---------------------------------------------------------------------------

void __fastcall TFormSetSize::EditWidthExit(TObject *Sender)
{
	int n;

	if(!TryStrToInt(EditWidth->Text,n)) n=0;

	if(n<4) n=4;

	if(n>NAME_MAX_WIDTH) n=NAME_MAX_WIDTH;

	n=(n+3)/4*4;

	EditWidth->Text=IntToStr(n);
}
//---------------------------------------------------------------------------

void __fastcall TFormSetSize::EditHeightExit(TObject *Sender)
{
	int n;

	if(!TryStrToInt(EditHeight->Text,n)) n=0;

	if(n<4) n=4;

	if(n>NAME_MAX_HEIGHT) n=NAME_MAX_HEIGHT;

	n=(n+3)/4*4;

	EditHeight->Text=IntToStr(n);
}
//---------------------------------------------------------------------------

void __fastcall TFormSetSize::RadioButtonNormalClick(TObject *Sender)
{
	UpDownWidth ->Enabled=RadioButtonUser->Checked;
	UpDownHeight->Enabled=RadioButtonUser->Checked;
	EditWidth   ->Enabled=RadioButtonUser->Checked;
	EditHeight  ->Enabled=RadioButtonUser->Checked;
}
//---------------------------------------------------------------------------

void __fastcall TFormSetSize::FormCreate(TObject *Sender)
{
	NewWidth=256;
	NewHeight=256;

	RadioButtonNormalClick(Sender);
}
//---------------------------------------------------------------------------

void __fastcall TFormSetSize::EditWidthClick(TObject *Sender)
{
	((TEdit*)Sender)->SelectAll();
}
//---------------------------------------------------------------------------

