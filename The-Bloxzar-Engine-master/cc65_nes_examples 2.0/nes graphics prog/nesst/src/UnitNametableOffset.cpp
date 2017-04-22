//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "UnitNametableOffset.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TFormNameOffset *FormNameOffset;

void __fastcall TFormNameOffset::UpdateInfo(void)
{
	AnsiString str,hex;

	if(Offset>=0) hex=IntToHex(Offset,2); else hex="-"+IntToHex(-Offset,2);

	if(Mode==0)
	{
		str="Range hex $"+IntToHex(From,2)+"...$"+IntToHex(To,2)+", offset $"+hex;
		str+="\nRange dec "+IntToStr(From)+"..."+IntToStr(To)+", offset "+IntToStr(Offset);
	}
	else
	{
		str="Replace hex $"+IntToHex(From,2)+" to $"+hex;
		str+="\nReplace dec "+IntToStr(From)+" to "+IntToStr(Offset);
    }

	LabelInfo->Caption=str;
}


//---------------------------------------------------------------------------
__fastcall TFormNameOffset::TFormNameOffset(TComponent* Owner)
: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TFormNameOffset::FormCreate(TObject *Sender)
{
	From=0;
	To=255;
	Offset=0;
	MakeOffset=false;

	UpdateInfo();
}
//---------------------------------------------------------------------------
void __fastcall TFormNameOffset::ButtonOkClick(TObject *Sender)
{
	MakeOffset=true;
	Close();
}
//---------------------------------------------------------------------------
void __fastcall TFormNameOffset::ButtonCancelClick(TObject *Sender)
{
	MakeOffset=false;
	Close();
}
//---------------------------------------------------------------------------

void __fastcall TFormNameOffset::EditFromKeyPress(TObject *Sender, char &Key)
{
	if((Key>='0'&&Key<='9')||(Key>='A'&&Key<='F')||(Key>='a'&&Key<='f')||Key=='-'||Key=='$'||Key==VK_DELETE||Key==VK_BACK) return;

	if(Key==VK_ESCAPE) ButtonCancelClick(Sender);
	if(Key==VK_RETURN) ButtonOkClick(Sender);

	Key=0;
}
//---------------------------------------------------------------------------

void __fastcall TFormNameOffset::EditFromChange(TObject *Sender)
{
	TEdit *e;
	char str_in[16],str_out[16];
	unsigned int i;
	int n,c,ptr,pos;
	bool neg,hex,num;

	e=((TEdit*)Sender);

	strcpy(str_in,e->Text.c_str());

	pos=e->SelStart;

	neg=false;
	hex=false;
	num=false;

	for(i=0;i<strlen(str_in);++i)
	{
		c=str_in[i];

		if(!c) break;
		if(c=='-') neg=true;
		if(c>='a'&&c<='f') str_in[i]-=32;
		if(c>='A'&&c<='F') { hex=true; num=true; }
		if(c=='$') hex=true;
		if(c>='0'&&c<='9') num=true;
	}

	if(Sender==EditFrom||Sender==EditTo) neg=false;

	memset(str_out,0,sizeof(str_out));

	ptr=0;

	if(neg) str_out[ptr++]='-';
	if(hex) str_out[ptr++]='$';

	for(i=0;i<strlen(str_in);++i)
	{
		c=str_in[i];

		if(!c) break;
		if(c=='-'||c=='$') continue;

		str_out[ptr++]=c;
	}

	if(!num) str_out[ptr]='0';

	e->Text=str_out;
	e->SelStart=pos;

	n=0;

	for(i=0;i<strlen(str_out);++i)
	{
		c=str_out[i];

		if(!hex)
		{
			if(c>='0'&&c<='9') n=n*10+(c-'0');
		}
		else
		{
			if(c>='0'&&c<='9'||c>='A'&&c<='F')
			{
				if(c>='0'&&c<='9') n=(n<<4)+(c-'0');
				if(c>='A'&&c<='F') n=(n<<4)+(c-'A')+10;
            }
		}
	}

	if(neg) n=-n;

	if(Sender==EditFrom)   From=n;
	if(Sender==EditTo)	   To=n;
	if(Sender==EditOffset) Offset=n;

	UpdateInfo();
}
//---------------------------------------------------------------------------

void __fastcall TFormNameOffset::EditFromMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	((TEdit*)Sender)->SelectAll();
}
//---------------------------------------------------------------------------

void __fastcall TFormNameOffset::FormShow(TObject *Sender)
{
	if(Mode==0)
	{
		Caption="Add offset";

		LabelFromTo->Caption="Tile numbers range:";
		LabelOffset->Caption="Offset:";

		EditTo->Visible=true;
	}
	else
	{
		Caption="Replace tile";

		LabelFromTo->Caption="Tile to replace:";
		LabelOffset->Caption="Replace with:";

		EditTo->Visible=false;
	}

	UpdateInfo();
}
//---------------------------------------------------------------------------

