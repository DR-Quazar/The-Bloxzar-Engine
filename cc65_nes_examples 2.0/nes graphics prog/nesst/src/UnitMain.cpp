//---------------------------------------------------------------------------

#include <vcl.h>
#include <stdio.h>
#include <Clipbrd.hpp>
#pragma hdrstop

#include "UnitMain.h"
#include "UnitCHREditor.h"
#include "UnitSwapColors.h"
#include "UnitNametableOffset.h"
#include "UnitNESBank.h"
#include "UnitSetSize.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TFormMain *FormMain;


#include "palette.h"
#include "smallnums.h"

int basePalette[64];

int outPalette[64];
int ppuMask;

unsigned char bgPal[16];
unsigned char bgPalUndo[16];

int bgPalCur;
int palActive;
int tileActive;
int bankActive;
int nameXC;
int nameYC;
int tileXC;
int tileYC;
int palHover;
int palColHover;
int colHover;
int palDragColor;

unsigned char chr[8192];
unsigned char chrCopy[4096];
unsigned char nameTable[NAME_MAX_SIZE];
unsigned char attrTable[ATTR_MAX_SIZE];

int nameTableViewX;
int nameTableViewY;

TRect nameSelection;
int nameCopyWidth;
int nameCopyHeight;

unsigned char nameCopy[NAME_MAX_SIZE];
unsigned char attrCopy[NAME_MAX_SIZE];

unsigned char undoChr[8192];
unsigned char undoNameTable[NAME_MAX_SIZE];
unsigned char undoAttrTable[ATTR_MAX_SIZE];

TRect chrSelection;
int chrCopyWidth;
int chrCopyHeight;
bool chrCopyRect;
unsigned char chrSelected[256];
bool chrSelectRect;

int typeInFontOffset;
int prevMouseY;

unsigned char metaSprites[256*64*4];

int spriteActive;
int metaSpriteActive;

int spriteGridX;
int spriteGridY;

int spriteHover;
int spriteDrag;
int spriteDragX;
int spriteDragY;

unsigned char metaSpriteCopy[64*4];

int CHRLatestSize;

int uiScale;



AnsiString reg_load_str(const char *name,AnsiString def)
{
	HKEY key;
	DWORD type,size;
	char *str;
	AnsiString ret;

	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,REG_SECTION,0,KEY_READ,&key)!=ERROR_SUCCESS) return def;

	type=REG_SZ;
	size=1024;
	str=(char*)malloc(size);
	if(RegQueryValueEx(key,name,NULL,&type,(unsigned char*)str,&size)!=ERROR_SUCCESS) ret=def; else ret=str;
	free(str);
	RegCloseKey(key);

	return ret;
}



void reg_save_str(const char *name,AnsiString str)
{
	HKEY key;
	DWORD disp;

	if(RegCreateKeyEx(HKEY_LOCAL_MACHINE,REG_SECTION,0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&key,&disp)!=ERROR_SUCCESS) return;

	RegSetValueEx(key,name,0,REG_SZ,str.c_str(),strlen(str.c_str())+1);
	RegCloseKey(key);
}



AnsiString num_to_3char(int i)
{
	AnsiString str;

	if(i<0) str="-"; else str=" ";

	i=abs(i);

	if(i>=100) str+=IntToStr((i/100)%10);
	if(i>=10 ) str+=IntToStr((i/10 )%10); else str+=" ";

	str+=IntToStr(i%10);

	return str;
}



int get_file_size(FILE *file)
{
	int size;

	fseek(file,0,SEEK_END);
	size=ftell(file);
	fseek(file,0,SEEK_SET);

	return size;
}



int read_dword(unsigned char *data)
{
	return data[0]+(data[1]<<8)+(data[2]<<16)+(data[3]<<24);
}


int __fastcall TFormMain::AttrGet(int x,int y)
{
	int pal;

	pal=attrTable[y/4*((nameTableWidth+3)/4)+x/4];

	if(x&2) pal>>=2;
	if(y&2) pal>>=4;

	return pal&3;
}



void __fastcall TFormMain::AttrSet(int x,int y,int pal)
{
	int pp,mask;

	pp=y/4*((nameTableWidth+3)/4)+x/4;

	mask=3;
	pal=pal&3;

	if(x&2)
	{
		pal<<=2;
		mask<<=2;
	}
	if(y&2)
	{
		pal<<=4;
		mask<<=4;
	}

	attrTable[pp]=(attrTable[pp]&(mask^255))|pal;
}



int encode_rle(unsigned char *src,int size,unsigned char *dst)
{
	int stat[256];
	int i,tag,sym,sym_prev,len,ptr;

	memset(stat,0,sizeof(stat));

	for(i=0;i<size;++i) ++stat[src[i]];

	tag=-1;

	for(i=0;i<256;++i)
	{
		if(!stat[i])
		{
			tag=i;
			break;
		}
	}

	if(tag<0) return -1;

	ptr=0;
	len=1;
	sym_prev=-1;

	dst[ptr++]=tag;

	for(i=0;i<size;++i)
	{
		sym=src[i];

		if(sym_prev!=sym||len>=255||i==size-1)
		{
			if(len>1)
			{
				if(len==2)
				{
					dst[ptr++]=sym_prev;
				}
				else
				{
					dst[ptr++]=tag;
					dst[ptr++]=len-1;
				}
			}

			dst[ptr++]=sym;

			sym_prev=sym;

			len=1;
		}
		else
		{
			++len;
		}
	}

	dst[ptr++]=tag;	//end of file marked with zero length rle
	dst[ptr++]=0;

	return ptr;
}



int name_size(void)
{
	return FormMain->nameTableWidth*FormMain->nameTableHeight;
}



int attr_size(void)
{
	return (FormMain->nameTableWidth+3)/4*((FormMain->nameTableHeight+3)/4);
}



AnsiString save_data(const char *name,unsigned char *src,int size,int filter)
{
	char arrname[256],temp[256];
	FILE *file;
	int i,tag,pp,len,sym,sym_prev,type;
	unsigned char *dst;
	bool rle;

	switch(filter)
	{
	case SAVE_NAMETABLE_NAM:     type=SAVE_DATA_BIN; rle=false; break;
	case SAVE_NAMETABLE_BIN:     type=SAVE_DATA_BIN; rle=false; break;
	case SAVE_NAMETABLE_ASM:     type=SAVE_DATA_ASM; rle=false; break;
	case SAVE_NAMETABLE_H:       type=SAVE_DATA_H;   rle=false; break;
	case SAVE_NAMETABLE_RLE:     type=SAVE_DATA_BIN; rle=true; break;
	case SAVE_NAMETABLE_RLE_ASM: type=SAVE_DATA_ASM; rle=true; break;
	case SAVE_NAMETABLE_RLE_H:   type=SAVE_DATA_H;   rle=true; break;
	}

	if(rle)
	{
		dst=(unsigned char*)malloc(size*2);

		size=encode_rle(src,size,dst);

		if(size<0) return "No unused tiles found, can't be saved as RLE (limitation of this RLE format)";
	}
	else
	{
		dst=src;
	}

	switch(type)
	{
	case SAVE_DATA_H:
		{
			file=fopen(name,"wt");

			if(!file) break;

			i=strlen(name);

			while(i)
			{
				if(name[i]=='\\'||name[i]=='/')
				{
					i++;
					break;
				}
				i--;
			}

			strcpy(arrname,&name[i]);

			for(i=strlen(arrname)-1;i>=0;i--)
			{
				if(arrname[i]=='.')
				{
					arrname[i]=0;
					break;
				}
			}

			if(arrname[0]!='_'&&!(arrname[0]>='a'&&arrname[0]<='z')&&!(arrname[0]>='A'&&arrname[0]<='Z'))
			{
				strcpy(temp,arrname);
				strcpy(arrname,"n");
				strcat(arrname,temp);
			}

			fprintf(file,"const unsigned char %s[%i]={\n",arrname,size);

			for(i=0;i<size;i++)
			{
				fprintf(file,"0x%2.2x",dst[i]);
				if(i<size-1) fprintf(file,",");
				if((i&15)==15||i==(size-1)) fprintf(file,"\n");
			}

			fprintf(file,"};\n");
			fclose(file);
		}
		break;

	case SAVE_DATA_ASM:
		{

		}
		break;

	case SAVE_DATA_BIN:
		{
			file=fopen(name,"wb");

			if(!file) break;

			fwrite(dst,size,1,file);
			fclose(file);
		}
		break;
	}

	if(rle) free(dst);

	return "";
}



void mem_exchange(unsigned char *src,unsigned char *dst,int len)
{
	int temp;

	while(len)
	{
		temp=*src;
		*src++=*dst;
		*dst++=temp;
		len--;
	}
}



void pal_validate(void)
{
	int i,j;

	for(i=0;i<4;i++)
	{
		for(j=0;j<4;j++)
		{
			switch(bgPal[i*4+j])
			{
			case 0x0d:
			case 0x1d:
			case 0x0e:
			case 0x1e:
			case 0x2e:
			case 0x3e:
			case 0x1f:
			case 0x2f:
			case 0x3f:
				bgPal[i*4+j]=colBlack;
				break;
			}
		}
	}
}



void palette_calc(void)
{
	int i;
	float r,g,b;

	if(!(ppuMask&0x01))
	{
		for(i=0;i<64;i++)
		{
			r=((float)((basePalette[i]>>16)&0xff))/255.0;
			g=((float)((basePalette[i]>>8)&0xff))/255.0;
			b=((float)(basePalette[i]&0xff))/255.0;
			r=r*emphasis[ppuMask>>5][0]/100.0;
			g=g*emphasis[ppuMask>>5][1]/100.0;
			b=b*emphasis[ppuMask>>5][2]/100.0;
			if(r>1.0) r=1.0;
			if(g>1.0) g=1.0;
			if(b>1.0) b=1.0;
			outPalette[i]=(((int)(255.0*r))<<16)|(((int)(255.0*g))<<8)|((int)(255.0*b));
		}
	}
	else
	{
		for(i=0;i<64;i++)
		{
			outPalette[i]=basePalette[i&0xf0];
		}
	}
}



AnsiString RemoveExt(AnsiString name)
{
	return ChangeFileExt(name,"");
}



AnsiString GetExt(AnsiString name)
{
	name=ExtractFileName(name);

	return name.SubString(name.LastDelimiter(".")+1,name.Length()-name.LastDelimiter(".")).LowerCase();
}



void __fastcall TFormMain::ClearNametable(void)
{
	memset(nameTable,0,sizeof(nameTable));
	memset(attrTable,0,sizeof(attrTable));
}



void __fastcall TFormMain::DrawCol(int x,int y,int size,int c,bool sel)
{
	TRect r;

	r.left  =x;
	r.top   =y;
	r.right =x+size;
	r.Bottom=y+size;

	PaintBoxPal->Canvas->Brush->Color=TColor(outPalette[c]);
	PaintBoxPal->Canvas->FillRect(r);

	if(sel)
	{
		PaintBoxPal->Canvas->Pen->Color=TColor(0xffffff);
		PaintBoxPal->Canvas->Rectangle(r);
		PaintBoxPal->Canvas->Pen->Color=TColor(0);

		r.left  +=1;
		r.top   +=1;
		r.right -=1;
		r.bottom-=1;

		PaintBoxPal->Canvas->Rectangle(r);
	}
}



void __fastcall TFormMain::DrawBGPal(int x,int y,int pal)
{
	DrawCol(x   ,y,20,bgPal[pal*4+0],pal==palActive&&bgPalCur==0?true:false);
	DrawCol(x+20,y,20,bgPal[pal*4+1],pal==palActive&&bgPalCur==1?true:false);
	DrawCol(x+40,y,20,bgPal[pal*4+2],pal==palActive&&bgPalCur==2?true:false);
	DrawCol(x+60,y,20,bgPal[pal*4+3],pal==palActive&&bgPalCur==3?true:false);
}



void __fastcall TFormMain::DrawPalettes(void)
{
	int i,j,x,y,pp,col;

	if(!Visible) return;

	DrawBGPal( 16, 0,0);
	DrawBGPal(128, 0,1);
	DrawBGPal( 16,32,2);
	DrawBGPal(128,32,3);

	y=64;
	pp=0;

	col=bgPal[palActive*4+bgPalCur];

	if(col==0x0f) col=0x0d;

	for(i=0;i<4;i++)
	{
		x=0;

		for(j=0;j<14;j++)
		{
			DrawCol(x,y,16,pp,pp==col);

			pp++;
			x+=16;
		}

		pp+=2;
		y+=16;
	}
}



void __fastcall TFormMain::DrawTile(TPicture *pic,int x,int y,int tile,int pal,int tx,int ty,bool sel)
{
	int j,k,l,m,pp,col,r,g,b;
	unsigned char *dst;
	int rr[8],gg[8],bb[8];
	int hgrid,vgrid,hcol,vcol;

	pp=tile*16+bankActive;

	if(chrSelectRect) sel=false;

	for(j=0;j<8;++j)
	{
		for(k=0;k<8;++k)
		{
			col=(((chr[pp+j]<<k)&128)>>7)|(((chr[pp+j+8]<<k)&128)>>6);
			col=outPalette[bgPal[pal*4+col]];

			rr[k]=(col>>16)&0xff;
			gg[k]=(col>>8)&0xff;
			bb[k]= col&0xff;
		}

		for(l=0;l<uiScale;++l)
		{
			dst=(unsigned char*)pic->Bitmap->ScanLine[y+j*uiScale+l]+x*3;

			for(k=0;k<8;++k)
			{
				r=rr[k];
				g=gg[k];
				b=bb[k];

				for(m=0;m<uiScale;++m)
				{
					*dst++=r;
					*dst++=g;
					*dst++=b;
				}
			}
		}
	}

	hgrid=0;
	vgrid=0;
	hcol=64;
	vcol=64;

	if(tx<0&&ty<0)
	{
		if(SpeedButtonGridTile->Down||SpeedButtonGridAtr->Down||SpeedButtonGridBlock->Down)
		{
			hgrid=2;
			vgrid=2;
		}
	}
	else
	{
		if(SpeedButtonGridTile->Down)
		{
			hgrid=2;
			vgrid=2;
		}
		if(SpeedButtonGridAtr->Down)
		{
			if(!(ty&1))
			{
				hcol=96;
				hgrid=1;
			}
			if(!(tx&1))
			{
				vcol=96;
				vgrid=1;
			}
		}
		if(SpeedButtonGridBlock->Down)
		{
			if(!(ty&3))
			{
				hcol=128;
				hgrid=1;
			}
			if(!(tx&3))
			{
				vcol=128;
				vgrid=1;
			}
		}
	}

	if(hgrid)
	{
		dst=(unsigned char*)pic->Bitmap->ScanLine[y]+x*3;

		for(j=0;j<8*uiScale;j+=hgrid)
		{
			*dst+++=hcol;
			*dst+++=hcol;
			*dst+++=hcol;

			if(hgrid>1) dst+=(hgrid-1)*3;
		}
	}

	if(vgrid)
	{
		for(j=1;j<8*uiScale;j+=vgrid)
		{
			dst=(unsigned char*)pic->Bitmap->ScanLine[y+j]+x*3;

			*dst+++=vcol;
			*dst+++=vcol;
			*dst+++=vcol;
		}

		*dst=*dst;//to prevent warning
	}
}



void __fastcall TFormMain::DrawTileChecker(TPicture *pic,int x,int y,int tile,int pal,int tx,int ty,bool sel)
{
	const unsigned char checker[16]={
		0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,
		0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff
	};
	int j,k,l,m,col,r,g,b;
	int rr[8],gg[8],bb[8];
	unsigned char *dst;
	int hgrid,vgrid,hcol,vcol;

	for(j=0;j<8;j++)
	{
		for(k=0;k<8;k++)
		{
			col=(((checker[j]<<k)&128)>>7)|(((checker[j+8]<<k)&128)>>6);
			col=outPalette[bgPal[pal*4+col]];

			rr[k]=(col>>16)&0xff;
			gg[k]=(col>>8)&0xff;
			bb[k]=col&0xff;
		}

		for(l=0;l<uiScale;++l)
		{
			dst=(unsigned char*)pic->Bitmap->ScanLine[y+j*uiScale+l]+x*3;

			for(k=0;k<8;++k)
			{
				r=rr[k];
				g=gg[k];
				b=bb[k];

				for(m=0;m<uiScale;++m)
				{
					*dst++=r;
					*dst++=g;
					*dst++=b;
				}
			}
		}
	}
}



void __fastcall TFormMain::DrawEmptyTile(TPicture *pic,int x,int y,int pal,int tx,int ty,bool sel)
{
	int j,k,l,m,pp,col;
	unsigned char *dst;

	for(j=0;j<8;++j)
	{
		for(l=0;l<uiScale;++l)
		{
			dst=(unsigned char*)pic->Bitmap->ScanLine[y+j*uiScale+l]+x*3;

			for(k=0;k<8;++k)
			{
				if((j^k)&4) col=0x80; else col=0xc0;

				for(m=0;m<uiScale;++m)
				{
					*dst++=col;
					*dst++=col;
					*dst++=col;
				}
			}
		}
	}
}



void __fastcall TFormMain::DrawSpriteTile(TPicture *pic,int x,int y,int tile,int attr,TColor frame)
{
	int tx,ty,j,k,l,m,pp,col,r,g,b;
	int rr[8],gg[8],bb[8];
	unsigned char *dst;
	unsigned char chrt[16][8];

	if(!SpeedButtonSprite8x16->Down)
	{
		pp=(tile&255)*16+bankActive;

		for(j=0;j<8;++j)
		{
			for(k=0;k<8;++k)
			{
				if(attr&128) ty=7-j; else ty=j;//vflip
				if(attr&64 ) tx=7-k; else tx=k;//hflip

				chrt[ty][tx]=(((chr[pp+j]<<k)&128)>>7)|(((chr[pp+j+8]<<k)&128)>>6);
			}
		}
	}
	else
	{
		for(j=0;j<16;++j)
		{
			if(j==0) pp=(tile&254)*16+((tile&1)*4096);
			if(j==8) pp=(tile&254)*16+16+((tile&1)*4096);

			for(k=0;k<8;++k)
			{
				if(attr&128) ty=15-j; else ty=j;//vflip
				if(attr&64 ) tx= 7-k; else tx=k;//hflip

				chrt[ty][tx]=(((chr[pp+(j&7)]<<k)&128)>>7)|(((chr[pp+(j&7)+8]<<k)&128)>>6);
			}
		}
	}

	for(j=0;j<(SpeedButtonSprite8x16->Down?16:8);++j)
	{
		for(k=0;k<8;++k)
		{
			col=chrt[j][k];

			if(col)
			{
				col=outPalette[bgPal[(attr&3)*4+col]];

				rr[k]=(col>>16)&0xff;
				gg[k]=(col>>8)&0xff;
				bb[k]= col&0xff;
			}
			else
			{
				rr[k]=-1;
			}
		}

		for(l=0;l<uiScale;++l)
		{
			if(y+j*uiScale+l>=pic->Bitmap->Height) break;
			
			dst=(unsigned char*)pic->Bitmap->ScanLine[y+j*uiScale+l]+x*3;

			for(k=0;k<8;++k)
			{
				r=rr[k];
				g=gg[k];
				b=bb[k];

				if(r>=0)
				{
					for(m=0;m<uiScale;++m)
					{
						*dst++=r;
						*dst++=g;
						*dst++=b;
					}
				}
				else
				{
					dst+=3*uiScale;
				}
			}
		}
	}

	if(frame!=clBlack)
	{
		pic->Bitmap->Canvas->Brush->Style=bsClear;
		pic->Bitmap->Canvas->Pen->Color=frame;
		pic->Bitmap->Canvas->Rectangle(x,y,x+8*uiScale,y+(SpeedButtonSprite8x16->Down?16*uiScale:8*uiScale));
	}
}



void __fastcall TFormMain::DrawExportTile16(TPicture *pic,int x,int y,int tile,int pal,int tx,int ty)
{
	int j,k,pp;;
	unsigned char *dst;
	int col1,col2;

	pp=tile*16+bankActive;

	for(j=0;j<8;j++)
	{
		dst=(unsigned char*)pic->Bitmap->ScanLine[y+j]+x/2;

		for(k=0;k<8;k+=2)
		{
			col1=(pal<<2)|(((chr[pp+j]<<k)&128)>>7)|(((chr[pp+j+8]<<k)&128)>>6);
			col2=(pal<<2)|(((chr[pp+j]<<(k+1))&128)>>7)|(((chr[pp+j+8]<<(k+1))&128)>>6);

			*dst++=col2|(col1<<4);
		}
	}
}



void __fastcall TFormMain::DrawSelection(TImage *image,TRect rect)
{
	TRect r;

	r.left  =rect.left  *8*uiScale;
	r.top   =rect.top   *8*uiScale;
	r.right =rect.right *8*uiScale;
	r.bottom=rect.bottom*8*uiScale;

	image->Canvas->Brush->Style=bsClear;
	image->Canvas->Pen->Color=TColor(0xffffff);
	image->Canvas->Rectangle(r);

	r.left  +=1;
	r.top   +=1;
	r.right -=1;
	r.bottom-=1;

	image->Canvas->Pen->Color=TColor(0x000000);
	image->Canvas->Rectangle(r);

	r.left  -=2;
	r.top   -=2;
	r.right +=2;
	r.bottom+=2;
	
	image->Canvas->Pen->Color=TColor(0x000000);
	image->Canvas->Rectangle(r);
}



void __fastcall TFormMain::UpdateTiles(bool updchr)
{
	int i,x,y;
	TRect rect;

	if(!Visible) return;

	x=0;
	y=0;

	for(i=0;i<256;i++)
	{
		DrawTile(ImageTiles->Picture,x,y,i,palActive,-1,-1,chrSelected[i]);

		x+=8*uiScale;

		if(x>=128*uiScale)
		{
			x=0;
			y+=8*uiScale;
		}
	}

	if(!chrSelectRect)
	{
		for(i=0;i<256;i++)
		{
			if(chrSelected[i])
			{
				rect.Left=i&15;
				rect.Right=rect.Left+1;
				rect.Top=i/16;
				rect.Bottom=rect.Top+1;

				DrawSelection(ImageTiles,rect);
			}
		}
	}
	else
	{
		DrawSelection(ImageTiles,chrSelection);
	}

	ImageTiles->Repaint();

	if(updchr) FormCHREditor->PaintBoxChr->Repaint();
}



void __fastcall TFormMain::UpdateNameTable(int tx,int ty,bool repaint)
{
	int i,j,x,y,nx,ny,tile;
	TRect r;

	if(!Visible) return;

	if(tx<0||ty<0)
	{
		//redraw whole viewport

		y=0;

		for(i=0;i<32;i++)
		{
			x=0;

			for(j=0;j<32;j++)
			{
				nx=j+nameTableViewX;
				ny=i+nameTableViewY;

				if(nx>=0&&nx<nameTableWidth&&ny>=0&&ny<nameTableHeight)
				{
					tile=nameTable[ny*nameTableWidth+nx];

					if(SpeedButtonChecker->Down)
					{
						DrawTileChecker(ImageName->Picture,x,y,tile,AttrGet(nx,ny),nx,ny,false);
					}
					else
					{
						if(!SpeedButtonSelTiles->Down||chrSelected[tile])
						{
							DrawTile(ImageName->Picture,x,y,tile,AttrGet(nx,ny),nx,ny,false);
						}
						else
						{
							DrawEmptyTile(ImageName->Picture,x,y,AttrGet(nx,ny),nx,ny,false);
						}
					}
				}
				else
				{
					DrawEmptyTile(ImageName->Picture,x,y,0,nx,ny,false);
				}

				x+=8*uiScale;
			}

			y+=8*uiScale;
		}
	}
	else
	{
		//redraw one attribute cell

		tx&=~1;
		ty&=~1;

		y=(ty-nameTableViewY)*8*uiScale;

		for(i=0;i<2;i++)
		{
			x=(tx-nameTableViewX)*8*uiScale;

			for(j=0;j<2;j++)
			{
				nx=j+tx;
				ny=i+ty;

				if(nx>=0&&nx<nameTableWidth&&ny>=0&&ny<nameTableHeight)
				{
					tile=nameTable[ny*nameTableWidth+nx];

					if(SpeedButtonChecker->Down)
					{
						DrawTileChecker(ImageName->Picture,x,y,tile,AttrGet(nx,ny),nx,ny,false);
					}
					else
					{
						if(!SpeedButtonSelTiles->Down||chrSelected[tile])
						{
							DrawTile(ImageName->Picture,x,y,tile,AttrGet(nx,ny),nx,ny,false);
						}
						else
						{
							DrawEmptyTile(ImageName->Picture,x,y,AttrGet(nx,ny),nx,ny,false);
						}
					}
				}
				else
				{
					DrawEmptyTile(ImageName->Picture,x,y,0,nx,ny,false);
				}

				x+=8*uiScale;
			}

			y+=8*uiScale;
		}
	}

	if(nameSelection.left>=0&&nameSelection.top>=0)
	{
		r.left  =nameSelection.left  -nameTableViewX;
		r.right =nameSelection.right -nameTableViewX;
		r.top   =nameSelection.top   -nameTableViewY;
		r.bottom=nameSelection.bottom-nameTableViewY;

		DrawSelection(ImageName,r);
	}

	if(repaint) ImageName->Repaint();
}



void __fastcall TFormMain::CopyCHR(bool copy,bool cut)
{
	int i,j,k,pp,ps,x,y,w,h;

	if(chrSelectRect)
	{
		GetSelection(chrSelection,x,y,w,h);
		if(w&&h&&cut) SetUndo();

		pp=0;

		for(i=0;i<h;i++)
		{
			for(j=0;j<w;j++)
			{
				for(k=0;k<16;k++)
				{
					ps=bankActive+(x+j)*16+(y+i)*256+k;
					if(copy) chrCopy[pp++]=chr[ps];
					if(cut) chr[ps]=0;
				}
			}
		}

		if(copy)
		{
			chrCopyWidth=w;
			chrCopyHeight=h;
			chrCopyRect=true;
		}
	}
	else
	{
		if(cut) SetUndo();

		pp=0;
		w=0;
		h=-1;

		for(i=0;i<256;i++)
		{
			if(chrSelected[i])
			{
				for(j=0;j<16;j++)
				{
					ps=bankActive+i*16+j;
					if(copy) chrCopy[pp++]=chr[ps];
					if(cut) chr[ps]=0;
				}
				w++;
			}
		}

		if(copy)
		{
			chrCopyWidth=w;
			chrCopyHeight=h;
			chrCopyRect=false;
		}
	}

	if(cut)
	{
		UpdateTiles(true);
		UpdateMetaSprite();

		UpdateNameTable(-1,-1,true);
	}
}



void __fastcall TFormMain::PasteCHR(void)
{
	int i,j,k,pp,pd,x,y,w,h;

	if(chrCopyRect)
	{
		if(chrCopyWidth<1||chrCopyHeight<1) return;

		GetSelection(chrSelection,x,y,w,h);
		SetUndo();

		pp=0;

		for(i=0;i<chrCopyHeight;i++)
		{
			for(j=0;j<chrCopyWidth;j++)
			{
				if(x+j<16&&y+i<16)
				{
					for(k=0;k<16;k++)
					{
						pd=bankActive+(x+j)*16+(y+i)*256+k;
						chr[pd]=chrCopy[pp+k];
					}
				}
				pp+=16;
			}
		}
	}
	else
	{
		if(chrCopyWidth<1) return;

		pp=0;
		pd=tileActive*16;

		for(i=0;i<chrCopyWidth;i++)
		{
			pd=bankActive+(pd&0x0fff);
			for(j=0;j<16;j++)
			{
				chr[pd++]=chrCopy[pp++];
			}
		}
	}

	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	UpdateMetaSprite();
}



void __fastcall TFormMain::NameTableScrollLeft(bool all)
{
	unsigned char temp[1024];
	int i,j,k;

	for(k=0;k<(all?2:1);k++)
	{
		for(i=0;i<nameTableHeight;i++) temp[i]=nameTable[i*nameTableWidth];

		for(i=0;i<nameTableWidth-1;i++)
		{
			for(j=0;j<nameTableHeight;j++)
			{
				nameTable[j*nameTableWidth+i]=nameTable[j*nameTableWidth+i+1];
			}
		}

		for(i=0;i<nameTableHeight;i++) nameTable[i*nameTableWidth+nameTableWidth-1]=temp[i];
	}

	if(all)
	{
		for(i=0;i<nameTableHeight/2;i++) temp[i]=AttrGet(0,i*2);

		for(i=0;i<nameTableWidth/2-1;i++)
		{
			for(j=0;j<nameTableHeight/2;j++)
			{
				AttrSet(i*2,j*2,AttrGet(i*2+2,j*2));
			}
		}

		for(i=0;i<nameTableHeight/2;i++) AttrSet(nameTableWidth-2,i*2,temp[i]);
	}

	UpdateNameTable(-1,-1,true);
}



void __fastcall TFormMain::NameTableScrollRight(bool all)
{
	unsigned char temp[1024];
	int i,j,k;

	for(k=0;k<(all?2:1);k++)
	{
		for(i=0;i<nameTableHeight;i++) temp[i]=nameTable[i*nameTableWidth+nameTableWidth-1];

		for(i=nameTableWidth-1;i>0;i--)
		{
			for(j=0;j<nameTableHeight;j++)
			{
				nameTable[j*nameTableWidth+i]=nameTable[j*nameTableWidth+i-1];
			}
		}

		for(i=0;i<nameTableHeight;i++) nameTable[i*nameTableWidth]=temp[i];
	}

	if(all)
	{
		for(i=0;i<nameTableHeight/2;i++) temp[i]=AttrGet(nameTableWidth-2,i*2);

		for(i=nameTableWidth/2-1;i>0;i--)
		{
			for(j=0;j<nameTableHeight/2;j++)
			{
				AttrSet(i*2,j*2,AttrGet(i*2-2,j*2));
			}
		}

		for(i=0;i<nameTableHeight/2;i++) AttrSet(0,i*2,temp[i]);
	}

	UpdateNameTable(-1,-1,true);
}



void __fastcall TFormMain::NameTableScrollUp(bool all)
{
	unsigned char temp[1024];
	int i,j,k;

	for(k=0;k<(all?2:1);k++)
	{
		for(i=0;i<nameTableWidth;i++) temp[i]=nameTable[i];

		for(i=0;i<nameTableHeight-1;i++)
		{
			for(j=0;j<nameTableWidth;j++)
			{
				nameTable[i*nameTableWidth+j]=nameTable[(i+1)*nameTableWidth+j];
			}
		}

		for(i=0;i<nameTableWidth;i++) nameTable[nameTableWidth*nameTableHeight-nameTableWidth+i]=temp[i];
	}

	if(all)
	{
		for(i=0;i<nameTableWidth/2;i++) temp[i]=AttrGet(i*2,0);

		for(i=0;i<nameTableHeight/2-1;i++)
		{
			for(j=0;j<nameTableWidth/2;j++)
			{
				AttrSet(j*2,i*2,AttrGet(j*2,i*2+2));
			}
		}

		for(i=0;i<nameTableWidth/2;i++) AttrSet(i*2,nameTableHeight-2,temp[i]);
	}

	UpdateNameTable(-1,-1,true);
}



void __fastcall TFormMain::NameTableScrollDown(bool all)
{
	unsigned char temp[1024];
	int i,j,k;

	for(k=0;k<(all?2:1);k++)
	{
		for(i=0;i<nameTableWidth;i++) temp[i]=nameTable[nameTableWidth*nameTableHeight-nameTableWidth+i];

		for(i=nameTableHeight-1;i>0;i--)
		{
			for(j=0;j<nameTableWidth;j++)
			{
				nameTable[i*nameTableWidth+j]=nameTable[(i-1)*nameTableWidth+j];
			}
		}

		for(i=0;i<nameTableWidth;i++) nameTable[i]=temp[i];
	}

	if(all)
	{
		for(i=0;i<nameTableWidth/2;i++) temp[i]=AttrGet(i*2,nameTableHeight-2);

		for(i=nameTableHeight/2-1;i>0;i--)
		{
			for(j=0;j<nameTableWidth/2;j++)
			{
				AttrSet(j*2,i*2,AttrGet(j*2,i*2-2));
			}
		}

		for(i=0;i<nameTableWidth/2;i++) AttrSet(i*2,0,temp[i]);
	}

	UpdateNameTable(-1,-1,true);
}



bool __fastcall TFormMain::OpenCHR(AnsiString name)
{
	unsigned char buf[4096];
	FILE *file;
	int i,pp,size,type;

	file=fopen(name.c_str(),"rb");

	type=-1;

	if(file)
	{
		size=get_file_size(file);

		switch(size)
		{
		case 8192:
			fread(chr,8192,1,file);
			type=4;
			break;

		case 4096:
			fread(chr+bankActive,4096,1,file);
			type=3;
			break;

		default:
			if(size<4096&&!(size&15))
			{
				fread(buf,size,1,file);

				pp=tileActive*16;

				for(i=0;i<size;i++)
				{
					chr[bankActive+pp++]=buf[i];
					if(pp>=4096) pp=0;
				}

				if(size==2048) type=2; else if(size==1024) type=1; else type=0;
			}
			else
			{
				Application->MessageBox("Wrong file size","Error",MB_OK);
				fclose(file);
				return false;
			}
		}
	}

	fclose(file);

	if(type<0) return false;

	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	UpdateMetaSprite();

	return true;
}



void __fastcall TFormMain::SaveCHR(int size)
{
	unsigned char buf[8192];
	FILE *file;
	int i,pp;
	AnsiString name;

	if(!SaveDialogChr->Execute()) return;

	name=RemoveExt(SaveDialogChr->FileName)+".chr";

	if(!OverwritePrompt(name)) return;

	file=fopen(name.c_str(),"rb");

	if(file)
	{
		fseek(file,0,SEEK_END);
		i=ftell(file);
		fclose(file);

		if(size!=i)
		{
			if(Application->MessageBox(("Previous file has different size ("+IntToStr(i)+" bytes)!\nDo you really want to overwrite?").c_str(),"Confirm",MB_YESNO)!=IDYES) return;
		}
	}

	file=fopen(name.c_str(),"wb");

	if(file)
	{
		if(size==4096||size==8192)
		{
			if(size==4096)
			{
				memcpy(buf,chr+bankActive,4096);
			}
			else
			{
				memcpy(buf,chr,8192);
			}
		}
		else
		{
			pp=tileActive*16+bankActive;

			for(i=0;i<size;i++)
			{
				buf[i]=chr[pp++];

				if(pp>=4096) pp=0;
			}
		}

		fwrite(buf,size,1,file);
		fclose(file);
	}

	SetLatestCHRSize(size);
}



void __fastcall TFormMain::InterleaveCHR(bool dir)
{
	unsigned char buf[4096];
	int table[256];
	int i,j,pp,pd,num;

	SetUndo();

	for(i=0;i<16;i++)
	{
		num=(i/2)*32+(i&1);

		for(j=0;j<16;j++)
		{
			if(dir) table[i*16+j]=num; else table[num]=i*16+j;
			num+=2;
		}
	}

	pp=bankActive;

	for(i=0;i<256;i++)
	{
		pd=table[i]*16;

		for(j=0;j<16;j++)
		{
			buf[pd++]=chr[pp++];
		}
	}

	memcpy(chr+bankActive,buf,4096);

	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	UpdateMetaSprite();
}



bool __fastcall TFormMain::OpenNameTable(AnsiString name)
{
	FILE *file;
	unsigned char src[2048],dst[1024];
	unsigned char meta[4];
	int i,tag,pp,pd,size;
	AnsiString ext;

	ext=GetExt(name);

	file=fopen(name.c_str(),"rb");

	if(!file) return false;

	if(ext=="map")
	{
		fseek(file,-4,SEEK_END);
		fread(meta,sizeof(meta),1,file);

		nameTableWidth =meta[0]+meta[1]*256;
		nameTableHeight=meta[2]+meta[3]*256;

		fseek(file,0,SEEK_SET);
		fread(nameTable,name_size(),1,file);
		fread(attrTable,attr_size(),1,file);
		fclose(file);
	}
	else
	{
		if(ext!="rle")
		{
			size=get_file_size(file);

			if(size==960||size==1024)
			{
				fread(nameTable,960,1,file);

				if(size==1024) fread(attrTable,64,1,file);

				fclose(file);
			}
			else
			{
				Application->MessageBox("Nametable should be 960 or 1024 bytes long","Error",MB_OK);

				fclose(file);
				return false;
			}
		}
		else
		{
			size=get_file_size(file);

			if(size<2048)
			{
				fread(src,size,1,file);
				fclose(file);

				tag=src[0];
				pp=1;
				pd=0;

				while(pp<size)
				{
					if(src[pp]==tag)
					{
						pp++;
						if(src[pp]==0) break;
						for(i=0;i<src[pp];i++) dst[pd++]=dst[pd-1];
						pp++;
					}
					else
					{
						dst[pd++]=src[pp++];
					}
				}
			}

			if(pd==959||pd==1023) pd++;//for really old files saved when RLE packer had bug
			if(pd==960||pd==1024) memcpy(nameTable,dst,960);
			if(pd==1024) memcpy(attrTable,dst+960,64);
			if(pd!=960&&pd!=1024) Application->MessageBox("Can't decode RLE","Error",MB_OK);
		}

		nameTableWidth=32;
		nameTableHeight=30;
	}

	UpdateAll();

	return true;
}



bool __fastcall TFormMain::OpenPalette(AnsiString name)
{
	FILE *file;
	unsigned char pal[16];
	int i;

	file=fopen(name.c_str(),"rb");

	if(file)
	{
		if(get_file_size(file)==16)
		{
			fread(pal,16,1,file);
			fclose(file);

			for(i=0;i<4;i++)
			{
				bgPal[0*4+i]=pal[i+0];
				bgPal[1*4+i]=pal[i+4];
				bgPal[2*4+i]=pal[i+8];
				bgPal[3*4+i]=pal[i+12];
			}

			UpdateAll();

			return true;
		}
		else
		{
			Application->MessageBox("Palette file should be 16 bytes long","Error",MB_OK);
		}
	}

	return false;
}



void __fastcall TFormMain::GetSelection(TRect r,int &x,int &y,int &w,int &h)
{
	if(r.left<r.right)
	{
		x=r.left;
		w=r.right-x;
	}
	else
	{
		x=r.right;
		w=r.left-x;
	}
	if(r.top<r.bottom)
	{
		y=r.top;
		h=r.bottom-y;
	}
	else
	{
		y=r.bottom;
		h=r.top-y;
	}
}



void __fastcall TFormMain::CopyMap(bool cut)
{
	int i,j,x,y,w,h,pp;

	GetSelection(nameSelection,x,y,w,h);

	if(w&&h&&cut) SetUndo();

	pp=0;

	for(i=0;i<h;++i)
	{
		for(j=0;j<w;++j)
		{
			nameCopy[pp]=nameTable[y*nameTableWidth+x+j];
			attrCopy[pp]=AttrGet(x+j,y);

			if(cut&&SpeedButtonTiles->Down) nameTable[y*nameTableWidth+x+j]=0;

			++pp;
		}

		++y;
	}

	nameCopyWidth=w;
	nameCopyHeight=h;

	if(cut) UpdateNameTable(-1,-1,true);
}



void __fastcall TFormMain::CopyMapCodeASM(void)
{
	char str[65536],buf[1024];
	int i,j,x,y,w,h;

	if(nameSelection.left>=0&&nameSelection.top>=0)
	{
		GetSelection(nameSelection,x,y,w,h);

		strcpy(str,"");

		for(i=0;i<h;i++)
		{
			strcat(str,"\t.db ");

			for(j=0;j<w;j++)
			{
				sprintf(buf,"$%2.2x%c",nameTable[(y+i)*nameTableWidth+x+j],j<w-1?',':'\n');
				strcat(str,buf);
			}
		}

		Clipboard()->SetTextBuf(str);
	}
}



void __fastcall TFormMain::CopyMapCodeC(bool rle)
{
	char str[65536],buf[1024],src[65536],dst[65536];
	int i,j,x,y,w,h,ptr,size;

	if(nameSelection.left>=0&&nameSelection.top>=0)
	{
		GetSelection(nameSelection,x,y,w,h);

		if(!rle)
		{
			sprintf(str,"const unsigned char nametable[%i*%i]={\n",w,h);

			for(i=0;i<h;i++)
			{
				strcat(str,"\t");

				for(j=0;j<w;j++)
				{
					sprintf(buf,"0x%2.2x",nameTable[(y+i)*nameTableWidth+x+j]);

					strcat(str,buf);

					if(i*w+j<w*h-1) strcat(str,",");
				}

				strcat(str,"\n");
			}
		}
		else
		{
			ptr=0;

			for(i=0;i<h;i++) for(j=0;j<w;j++) src[ptr++]=nameTable[(y+i)*nameTableWidth+x+j];

			size=encode_rle(src,ptr,dst);

			if(size<0)
			{
				Application->MessageBox("No unused tiles found, can't be encoded with RLE","Error",MB_OK);

				return;
			}

			sprintf(str,"const unsigned char nametable[%i]={\n",size);

			for(i=0;i<size;++i)
			{
				sprintf(buf,"0x%2.2x",dst[i]);

				strcat(str,buf);

				if(i<size-1) strcat(str,",");

				if(((i&15)==15)||(i==size-1)) strcat(str,"\n");
			}
		}

		strcat(str,"};\n\n");

		Clipboard()->SetTextBuf(str);
	}
}



void __fastcall TFormMain::CopyMetaSpriteCodeC(bool hflip)
{
	char str[65536],buf[1024];
	int i,j,x,y,w,h;

	if(nameSelection.left>=0&&nameSelection.top>=0)
	{
		GetSelection(nameSelection,x,y,w,h);

		sprintf(str,"const unsigned char metasprite[]={\n");

		if(!hflip)
		{
			for(i=0;i<h;i++)
			{
				for(j=0;j<w;j++)
				{
					sprintf(buf,"\t%s,%s,0x%2.2x,%i,\n",num_to_3char(j*8),num_to_3char(i*8),nameTable[(y+i)*nameTableWidth+x+j],AttrGet(x+j,y+i));
					strcat(str,buf);
				}
			}
		}
		else
		{
			for(i=0;i<h;i++)
			{
				for(j=0;j<w;j++)
				{
					sprintf(buf,"\t%s,%s,0x%2.2x,%i|OAM_FLIP_H,\n",num_to_3char(j*8),num_to_3char(i*8),nameTable[(y+i)*nameTableWidth+x+(w-1-j)],AttrGet(x+(w-1-j),y+i));
					strcat(str,buf);
				}
			}
		}

		strcat(str,"\t128\n};\n\n");

		Clipboard()->SetTextBuf(str);
	}
}



void __fastcall TFormMain::PasteMap(void)
{
	int i,j,x,y,w,h,pp;

	if(nameCopyHeight<1||nameCopyWidth<1) return;

	GetSelection(nameSelection,x,y,w,h);
	if(w&&h) SetUndo();

	pp=0;

	for(i=0;i<nameCopyHeight;i++)
	{
		for(j=0;j<nameCopyWidth;j++)
		{
			if(x+j<nameTableWidth&&y<nameTableHeight)
			{
				if(SpeedButtonTiles->Down) nameTable[y*nameTableWidth+x+j]=nameCopy[pp];
				if(SpeedButtonPal->Down) AttrSet(x+j,y,attrCopy[pp]);
			}
			pp++;
		}
		y++;
	}

	UpdateNameTable(-1,-1,true);
}



void __fastcall TFormMain::FillMap(void)
{
	int i,j,x,y,w,h;

	GetSelection(nameSelection,x,y,w,h);
	if(w&&h) SetUndo();

	for(i=0;i<h;i++)
	{
		for(j=0;j<w;j++)
		{
			if(SpeedButtonTiles->Down) nameTable[y*nameTableWidth+x+j]=tileActive;
		}
		y++;
	}

	UpdateNameTable(-1,-1,true);
}



void __fastcall TFormMain::OpenAll(AnsiString name)
{
	if(OpenCHR(name+".chr"))
	{
		SaveDialogChr->FileName=name;
	}
	else
	{
		if(OpenCHR(name+".bin")) SaveDialogChr->FileName=name;
	}

	if(OpenNameTable(name+".nam"))
	{
		SaveDialogName->FileName=name;
		SaveDialogAttr->FileName=name;
	}
	else
	{
		if(OpenNameTable(name+".rle"))
		{
			SaveDialogName->FileName=name;
			SaveDialogAttr->FileName=name;
		}
		else
		{
			if(OpenNameTable(name+".map"))
			{
				SaveDialogMap->FileName=name;
			}
		}
	}

	if(OpenPalette(name+".pal"))
	{
		SaveDialogPal->FileName=name;
	}

	if(OpenMetaSprites(name+".msp"))
	{
		SaveDialogMetaSpriteBank->FileName=name;
	}
}



void __fastcall TFormMain::UpdateStats(void)
{
	AnsiString str;
	int i,vx,vy,cnt,tile,off,col,sel;

	str="---";

	if(tileXC>=0&&tileYC>=0)
	{
		tile=tileYC*16+tileXC;
		cnt=0;

		for(i=0;i<nameTableWidth*nameTableHeight;i++) if(nameTable[i]==tile) ++cnt;

		sel=0;

		for(i=0;i<256;i++) if(chrSelected[i]) ++sel;

		str="Tile:$"+IntToHex(tile,2)+" ("+IntToStr(cnt)+" entries)";

		if(sel) str+=" "+IntToStr(sel)+" selected";
	}

	if(nameXC>=0&&nameXC<nameTableWidth&&nameYC>=0&&nameYC<nameTableHeight)
	{
		off=nameYC*nameTableWidth+nameXC;
		
		str="XY:"+IntToStr(nameXC)+","+IntToStr(nameYC)+" Off:$"+IntToHex(off,4)+"=$"+IntToHex(nameTable[off],2);

		str+=" AtXY:"+IntToStr(nameXC/2)+","+IntToStr(nameYC/2)+" AtOff:$"+IntToHex(nameYC/4*8+nameXC/4+nameTableWidth*nameTableHeight,4)+"."+IntToStr((nameXC&2)+(nameYC&2)*2)+"="+IntToStr(AttrGet(nameXC,nameYC))+" ($"+IntToHex(attrTable[nameYC/4*8+nameXC/4],2)+")";

		if(nameSelection.left>=0) str+=" WH:"+IntToStr(nameSelection.right-nameSelection.left)+"x"+IntToStr(nameSelection.bottom-nameSelection.top);

		str+=" MetaSpr:"+IntToStr(metaSpriteActive);
	}

	if(palHover>=0)
	{
		str="Pal:"+IntToStr(palHover)+" Entry:"+IntToStr(palColHover)+" Adr:"+IntToHex(0x3f00+palHover*4+palColHover,4)+" Color:$"+IntToHex(bgPal[palHover*4+palColHover],2);
	}

	if(colHover>=0)
	{
		col=colHover;

		if(col==0x0d||col==0x1d) col=0x0f;

		str="Color:$"+IntToHex(col,2);
	}

	if(spriteHover>=0)
	{
		off=metaSpriteActive*64*4+spriteHover*4;

		str="Sprite:"+IntToStr(spriteHover)+" X:"+IntToStr(metaSprites[off+3]-spriteGridX)+" Y:"+IntToStr(metaSprites[off]-spriteGridY)+" Tile:$"+IntToHex(metaSprites[off+1],2)+" Pal:"+IntToStr(metaSprites[off+2]&3);

		if(metaSprites[off+2]&OAM_FLIP_H) str+=" HFlip";
		if(metaSprites[off+2]&OAM_FLIP_V) str+=" VFlip";
	}

	LabelStats->Caption=str;

	str=FormCaption+"  ";

	if(nameTableWidth==32&&nameTableHeight==30)
	{
		str+="[std nametable]";
	}
	else
	{
		vx=nameTableViewX;
		vy=nameTableViewY;

		if(vx<0) vx=0;
		if(vy<0) vy=0;

		str+="[map "+IntToStr(nameTableWidth)+"x"+IntToStr(nameTableHeight)+"]";
		str+="["+IntToStr(vx)+","+IntToStr(vy)+"]";
	}

	Caption=str;
}



void __fastcall TFormMain::SetUndo(void)
{
	memcpy(bgPalUndo,bgPal,sizeof(bgPal));
	memcpy(undoChr,chr,8192);
	memcpy(undoNameTable,nameTable,name_size());
	memcpy(undoAttrTable,attrTable,attr_size());
}



void __fastcall TFormMain::Undo(void)
{
	mem_exchange(bgPalUndo,bgPal,sizeof(bgPal));
	mem_exchange(undoChr,chr,8192);
	mem_exchange(undoNameTable,nameTable,name_size());
	mem_exchange(undoAttrTable,attrTable,attr_size());

	UpdateAll();
}



void __fastcall TFormMain::SetTile(int tile)
{
	int i;

	tileActive=tile;
	chrSelection.left=tile&15;
	chrSelection.top=tile/16;
	chrSelection.right=chrSelection.left+1;
	chrSelection.bottom=chrSelection.top+1;

	for(i=0;i<256;++i) chrSelected[i]=0;

	chrSelected[tile]=1;

	chrSelectRect=true;

	UpdateTiles(true);
}



void __fastcall TFormMain::SetBMPPalette(Graphics::TBitmap* bmp)
{
	PALETTEENTRY pal[16];
	int i,col;

	for(i=0;i<16;i++)
	{
		col=outPalette[bgPal[(i>>2)*4+(i&3)]];

		pal[i].peBlue =(col>>16)&0xff;
		pal[i].peGreen=(col>>8)&0xff;
		pal[i].peRed  =(col&0xff);
	}

	SetPaletteEntries(bmp->Palette,0,16,pal);
}





int fget_i32(FILE *file)
{
	unsigned char buf[4];

	fread(buf,4,1,file);

	return buf[0]|(buf[1]<<8)|(buf[2]<<16)|(buf[3]<<24);
}



bool fget_bool(FILE *file)
{
	return fgetc(file)?true:false;
}



AnsiString fget_ansistring(FILE *file)
{
	AnsiString str;
	char *temp;
	int len;

	len=fget_i32(file);

	temp=(unsigned char*)malloc(len+1);
	fread(temp,len,1,file);
	temp[len]=0;
	str=temp;
	free(temp);

	return str;
}



void nss_put_bytes(FILE *file,const char *name,unsigned char *data,int size)
{
	int i,byte,cnt;

	fprintf(file,"%s",name);

	byte=-1;
	cnt=0;

	for(i=0;i<size;++i)
	{
		if(byte!=data[i])
		{
			if(cnt>1) fprintf(file,"[%x]",cnt);

			byte=data[i];

			fprintf(file,"%2.2x",byte);

			cnt=1;
		}
		else
		{
			++cnt;
		}
	}

	if(cnt>1) fprintf(file,"[%x]",cnt);

	fprintf(file,"\n");
}



bool nss_get_bytes(char* data,int size,const char* tag,unsigned char *dst,int dst_size)
{
	char c1,c2;
	int ptr,len,prev;

	len=strlen(tag);
	ptr=0;

	while(size)
	{
		if(!memcmp(data,tag,len))
		{
			data+=len;
			prev=0;

			while(ptr<dst_size)
			{
				c1=*data++;

				if(c1=='[')	//repeat tag
				{
					len=0;

					while(1)
					{
						c1=*data++;

						if(c1==']') break;

						if(c1>='0'&&c1<='9') c1=c1-'0'; else if(c1>='a'&&c1<='f') c1=c1-'a'+10; else return false;

						len=(len<<4)|c1;
					}

					while(len>1)
					{
						dst[ptr++]=prev;

						--len;
					}
					
					continue;
				}

				c2=*data++;

				if(c1>='0'&&c1<='9') c1=c1-'0'; else if(c1>='a'&&c1<='f') c1=c1-'a'+10; else return false;
				if(c2>='0'&&c2<='9') c2=c2-'0'; else if(c2>='a'&&c2<='f') c2=c2-'a'+10; else return false;

				prev=(c1<<4)+c2;

				dst[ptr++]=prev;
			}

			return true;
		}

		++data;
		--size;
	}

	return false;
}



char* nss_get_str(char* data,int size,const char* tag)
{
	static char str[16384];
	char c;
	int ptr,len;

	len=strlen(tag);
	ptr=0;

	while(size)
	{
		if(!memcmp(data,tag,len))
		{
			data+=len;

			while(size)
			{
				c=*data++;
				if(c<32) break;

				str[ptr++]=c;
			}

			str[ptr]=0;

			return str;
		}

		++data;
		--size;
	}

	return NULL;
}



int nss_get_int(char* data,int size,const char* tag)
{
	char c;
	int len,n,sign;

	sign=1;

	len=strlen(tag);

	while(size)
	{
		if(!memcmp(data,tag,len))
		{
			data+=len;
			n=0;

			c=*data;

			if(c=='-')
			{
				sign=-1;
				*data++;
			}

			while(size)
			{
				c=*data++;
				if(c<'0'||c>'9') break;

				n=n*10+(c-'0');

				--size;
			}

			return n*sign;
		}

		++data;
		--size;
	}

	return 0;
}



bool __fastcall TFormMain::LoadSession1x(AnsiString filename)
{
	FILE *file;
	unsigned char temp[16];
	int i,j;

	file=fopen(filename.c_str(),"rb");

	if(!file) return false;

	//signature

	fread(temp,sizeof(sessionIDStr),1,file);

	if(memcmp(temp,sessionIDStr,sizeof(sessionIDStr)))
	{
		fclose(file);
		Application->MessageBox("Session data does not match current version of the program or has been corrupted","Error",MB_OK);

		return false;
	}

	//arrays

	fread(chr      ,sizeof(chr)      ,1,file);
	fread(chrCopy  ,sizeof(chrCopy)  ,1,file);
	fread(nameTable,sizeof(nameTable),1,file);
	fread(attrTable,sizeof(attrTable),1,file);
	fread(nameCopy ,sizeof(nameCopy) ,1,file);
	fread(attrCopy ,sizeof(attrCopy) ,1,file);
	fread(undoChr  ,sizeof(undoChr)  ,1,file);
	fread(undoNameTable,sizeof(undoNameTable),1,file);
	fread(undoAttrTable,sizeof(undoAttrTable),1,file);

	//palette

	for(i=0;i<4;i++)
	{
		for(j=0;j<4;j++)
		{
			bgPal[i*4+j]=fget_i32(file);
		}
	}

	//screen buttons state

	SpeedButtonTiles    ->Down=fget_bool(file);
	SpeedButtonChecker  ->Down=fget_bool(file);
	SpeedButtonChrBank1 ->Down=fget_bool(file);
	SpeedButtonChrBank2 ->Down=fget_bool(file);
	SpeedButtonGridAll  ->Down=fget_bool(file);
	SpeedButtonGridTile ->Down=fget_bool(file);
	SpeedButtonGridAtr  ->Down=fget_bool(file);
	SpeedButtonGridBlock->Down=fget_bool(file);
	SpeedButtonPal      ->Down=fget_bool(file);

	//variables

	bgPalCur  =fget_i32(file);
	palActive =fget_i32(file);
	tileActive=fget_i32(file);
	bankActive=fget_i32(file);
	ppuMask   =fget_i32(file);

	nameSelection.Left  =fget_i32(file);
	nameSelection.Right =fget_i32(file);
	nameSelection.Top   =fget_i32(file);
	nameSelection.Bottom=fget_i32(file);
	nameCopyWidth       =fget_i32(file);
	nameCopyHeight      =fget_i32(file);

	chrSelection.Left  =fget_i32(file);
	chrSelection.Right =fget_i32(file);
	chrSelection.Top   =fget_i32(file);
	chrSelection.Bottom=fget_i32(file);
	chrCopyWidth       =fget_i32(file);
	chrCopyHeight      =fget_i32(file);
	chrCopyRect        =fget_bool(file);
	chrSelectRect      =fget_bool(file);

	for(i=0;i<256;i++) chrSelected[i]=fget_bool(file);


	//save dialogs settings

	SaveDialogChr ->FilterIndex=fget_i32(file);
	SaveDialogName->FilterIndex=fget_i32(file);
	SaveDialogAttr->FilterIndex=fget_i32(file);

	SaveDialogChr ->FileName=fget_ansistring(file);
	SaveDialogName->FileName=fget_ansistring(file);
	SaveDialogAttr->FileName=fget_ansistring(file);
	SaveDialogPal ->FileName=fget_ansistring(file);

	fclose(file);

	return true;
}



bool __fastcall TFormMain::LoadSession2x(AnsiString filename)
{
	FILE *file;
	unsigned char temp[16];
	int i,j;

	file=fopen(filename.c_str(),"rb");

	if(!file) return false;

	//signature

	fread(temp,sizeof(sessionIDStr),1,file);

	//arrays

	fread(chr      ,sizeof(chr)      ,1,file);
	fread(chrCopy  ,sizeof(chrCopy)  ,1,file);
	fread(nameTable,sizeof(nameTable),1,file);
	fread(attrTable,sizeof(attrTable),1,file);
	fread(nameCopy ,sizeof(nameCopy) ,1,file);
	fread(attrCopy ,sizeof(attrCopy) ,1,file);
	fread(undoChr  ,sizeof(undoChr)  ,1,file);
	fread(undoNameTable,sizeof(undoNameTable),1,file);
	fread(undoAttrTable,sizeof(undoAttrTable),1,file);
	fread(metaSprites  ,sizeof(metaSprites)  ,1,file);

	//palette

	for(i=0;i<4;i++)
	{
		for(j=0;j<4;j++)
		{
			bgPal[i*4+j]=fget_i32(file);
		}
	}

	//screen buttons state

	SpeedButtonTiles    ->Down=fget_bool(file);
	SpeedButtonChecker  ->Down=fget_bool(file);
	SpeedButtonSelTiles ->Down=fget_bool(file);
	SpeedButtonChrBank1 ->Down=fget_bool(file);
	SpeedButtonChrBank2 ->Down=fget_bool(file);
	SpeedButtonGridAll  ->Down=fget_bool(file);
	SpeedButtonGridTile ->Down=fget_bool(file);
	SpeedButtonGridAtr  ->Down=fget_bool(file);
	SpeedButtonGridBlock->Down=fget_bool(file);
	SpeedButtonPal      ->Down=fget_bool(file);
	SpeedButtonTypeIn   ->Down=fget_bool(file);
	SpeedButtonFrameAll ->Down=fget_bool(file);
	SpeedButtonFrameSelected->Down=fget_bool(file);
	SpeedButtonFrameNone    ->Down=fget_bool(file);
	SpeedButtonSpriteSnap   ->Down=fget_bool(file);
	SpeedButtonSprite8x16   ->Down=fget_bool(file);

	SpeedButtonSpriteBank->Enabled=SpeedButtonSprite8x16->Down;

	//variables

	bgPalCur  =fget_i32(file);
	palActive =fget_i32(file);
	tileActive=fget_i32(file);
	bankActive=fget_i32(file);
	ppuMask   =fget_i32(file);

	metaSpriteActive=fget_i32(file);
	spriteActive=fget_i32(file);
	spriteGridX =fget_i32(file);
	spriteGridY =fget_i32(file);

	nameSelection.Left  =fget_i32(file);
	nameSelection.Right =fget_i32(file);
	nameSelection.Top   =fget_i32(file);
	nameSelection.Bottom=fget_i32(file);
	nameCopyWidth       =fget_i32(file);
	nameCopyHeight      =fget_i32(file);

	chrSelection.Left  =fget_i32(file);
	chrSelection.Right =fget_i32(file);
	chrSelection.Top   =fget_i32(file);
	chrSelection.Bottom=fget_i32(file);
	chrCopyWidth       =fget_i32(file);
	chrCopyHeight      =fget_i32(file);
	chrCopyRect        =fget_bool(file);
	chrSelectRect      =fget_bool(file);

	for(i=0;i<256;i++) chrSelected[i]=fget_bool(file);

	//save dialogs settings

	SaveDialogChr ->FilterIndex=fget_i32(file);
	SaveDialogName->FilterIndex=fget_i32(file);
	SaveDialogAttr->FilterIndex=fget_i32(file);

	SaveDialogChr ->FileName=fget_ansistring(file);
	SaveDialogName->FileName=fget_ansistring(file);
	SaveDialogAttr->FileName=fget_ansistring(file);
	SaveDialogPal ->FileName=fget_ansistring(file);
	SaveDialogMetaSpriteBank->FileName=fget_ansistring(file);

	fclose(file);

	return true;
}



bool __fastcall TFormMain::LoadSessionText(AnsiString filename)
{
	FILE *file;
	char *text;
	int i,j,size;

	file=fopen(filename.c_str(),"rb");

	if(!file) return false;

	fseek(file,0,SEEK_END);
	size=ftell(file);
	fseek(file,0,SEEK_SET);

	text=(char*)malloc(size+1);
	text[size]=0;

	fread(text,size,1,file);
	fclose(file);

	//palette

	nss_get_bytes(text,size,"Palette=",bgPal,sizeof(bgPal));

	//screen buttons state

	SpeedButtonTiles    ->Down=nss_get_int(text,size,"BtnTiles=")?true:false;
	SpeedButtonChecker  ->Down=nss_get_int(text,size,"BtnChecker=")?true:false;
	SpeedButtonSelTiles ->Down=nss_get_int(text,size,"BtnSelTiles=")?true:false;
	SpeedButtonChrBank1 ->Down=nss_get_int(text,size,"BtnChrBank1=")?true:false;
	SpeedButtonChrBank2 ->Down=nss_get_int(text,size,"BtnChrBank2=")?true:false;
	SpeedButtonGridAll  ->Down=nss_get_int(text,size,"BtnGridAll=")?true:false;
	SpeedButtonGridTile ->Down=nss_get_int(text,size,"BtnGridTile=")?true:false;
	SpeedButtonGridAtr  ->Down=nss_get_int(text,size,"BtnGridAtr=")?true:false;
	SpeedButtonGridBlock->Down=nss_get_int(text,size,"BtnGridBlock=")?true:false;
	SpeedButtonPal      ->Down=nss_get_int(text,size,"BtnPal=")?true:false;
	SpeedButtonTypeIn   ->Down=nss_get_int(text,size,"BtnTypeIn=")?true:false;
	SpeedButtonFrameAll ->Down=nss_get_int(text,size,"BtnFrameAll=")?true:false;
	SpeedButtonFrameSelected->Down=nss_get_int(text,size,"BtnFrameSelected=")?true:false;
	SpeedButtonFrameNone    ->Down=nss_get_int(text,size,"BtnFrameNone=")?true:false;
	SpeedButtonSpriteSnap   ->Down=nss_get_int(text,size,"BtnSpriteSnap=")?true:false;
	SpeedButtonSprite8x16   ->Down=nss_get_int(text,size,"BtnSprite8x16=")?true:false;

	MImportBestOffsets              ->Checked=nss_get_int(text,size,"MenuBestOffsets=")?true:false;
	MImportLossy                    ->Checked=nss_get_int(text,size,"MenuLossy=")?true:false;
	MImportThreshold                ->Checked=nss_get_int(text,size,"MenuThreshold=")?true:false;
	MPutSelectionToMetaspriteAutoInc->Checked=nss_get_int(text,size,"MenuMetaSprAutoInc=")?true:false;

	SpeedButtonSpriteBank->Enabled=SpeedButtonSprite8x16->Down;

	//variables

	bgPalCur  =nss_get_int(text,size,"VarBgPalCur=");
	palActive =nss_get_int(text,size,"VarPalActive=");
	tileActive=nss_get_int(text,size,"VarTileActive=");
	bankActive=nss_get_int(text,size,"VarBankActive=");
	ppuMask   =nss_get_int(text,size,"VarPPUMask=");

	metaSpriteActive=nss_get_int(text,size,"VarMetaSpriteActive=");
	spriteActive=nss_get_int(text,size,"VarSpriteActive=");
	spriteGridX =nss_get_int(text,size,"VarSpriteGridX=");
	spriteGridY =nss_get_int(text,size,"VarSpriteGridY=");

	nameTableWidth      =nss_get_int(text,size,"VarNameW=");
	nameTableHeight     =nss_get_int(text,size,"VarNameH=");
	nameTableViewX      =nss_get_int(text,size,"VarNameViewX=");
	nameTableViewY      =nss_get_int(text,size,"VarNameViewY=");
	nameSelection.Left  =nss_get_int(text,size,"VarNameSelectionL=");
	nameSelection.Right =nss_get_int(text,size,"VarNameSelectionR=");
	nameSelection.Top   =nss_get_int(text,size,"VarNameSelectionT=");
	nameSelection.Bottom=nss_get_int(text,size,"VarNameSelectionB=");
	nameCopyWidth       =nss_get_int(text,size,"VarNameCopyW=");
	nameCopyHeight      =nss_get_int(text,size,"VarNameCopyH=");

	chrSelection.Left  =nss_get_int(text,size,"VarCHRSelectionL=");
	chrSelection.Right =nss_get_int(text,size,"VarCHRSelectionR=");
	chrSelection.Top   =nss_get_int(text,size,"VarCHRSelectionT=");
	chrSelection.Bottom=nss_get_int(text,size,"VarCHRSelectionB=");
	chrCopyWidth       =nss_get_int(text,size,"VarCHRCopyW=");
	chrCopyHeight      =nss_get_int(text,size,"VarCHRCopyH=");
	chrCopyRect        =nss_get_int(text,size,"VarCHRCopyRect=")?true:false;
	chrSelectRect      =nss_get_int(text,size,"VarCHRSelectRect=")?true:false;

	nss_get_bytes(text,size,"VarCHRSelected=",chrSelected,sizeof(chrSelected));

	//arrays

	nss_get_bytes(text,size,"PalUndo="    ,bgPalUndo    ,sizeof(bgPalUndo));

	nss_get_bytes(text,size,"CHRMain="    ,chr          ,sizeof(chr));
	nss_get_bytes(text,size,"CHRCopy="    ,chrCopy      ,sizeof(chrCopy));
	nss_get_bytes(text,size,"CHRUndo="    ,undoChr      ,sizeof(undoChr));

	nss_get_bytes(text,size,"NameTable="  ,nameTable    ,name_size());
	nss_get_bytes(text,size,"NameCopy="   ,nameCopy     ,name_size());
	nss_get_bytes(text,size,"NameUndo="   ,undoNameTable,name_size());

	nss_get_bytes(text,size,"AttrTable="  ,attrTable    ,attr_size());
	nss_get_bytes(text,size,"AttrCopy="   ,attrCopy     ,attr_size());
	nss_get_bytes(text,size,"AttrUndo="   ,undoAttrTable,attr_size());

	nss_get_bytes(text,size,"MetaSprites=",metaSprites,sizeof(metaSprites));

	//save dialogs settings

	SaveDialogChr ->FilterIndex=nss_get_int(text,size,"FilterCHR=");
	SaveDialogName->FilterIndex=nss_get_int(text,size,"FilterName=");
	SaveDialogAttr->FilterIndex=nss_get_int(text,size,"FilterAttr=");

	SaveDialogChr ->FileName=nss_get_str(text,size,"FileNameCHR=");
	SaveDialogName->FileName=nss_get_str(text,size,"FileNameName=");
	SaveDialogAttr->FileName=nss_get_str(text,size,"FileNameAttr=");
	SaveDialogPal ->FileName=nss_get_str(text,size,"FileNamePal=");
	SaveDialogMetaSpriteBank->FileName=nss_get_str(text,size,"FileNameMetaSpriteBank=");

	return true;
}



bool __fastcall TFormMain::LoadSession(AnsiString filename)
{
	FILE *file;
	unsigned char temp[16];
	int i,j,ver;
	bool r;

	file=fopen(filename.c_str(),"rb");

	if(!file) return false;

	fread(temp,sizeof(sessionIDStr),1,file);
	fclose(file);

	ver=0;

	if(!memcmp(temp,"NESSTses",8)) ver=1;//load legacy binary format
	if(!memcmp(temp,"NSTses00",8)) ver=2;//load old binary format
	if(!memcmp(temp,sessionIDStr,8)) ver=3;//current text format

	switch(ver)
	{
	case 1: r=LoadSession1x(filename); break;
	case 2: r=LoadSession2x(filename); break;
	case 3: r=LoadSessionText(filename); break;
	}

	if(r)
	{
		nameXC=-1;
		nameYC=-1;
		tileXC=-1;
		tileYC=-1;
		palHover=-1;
		palColHover=-1;
		colHover=-1;

		UpdateRGBM();
		palette_calc();
		DrawPalettes();
		UpdateTiles(true);
		UpdateNameTable(-1,-1,true);

		return true;
	}

	Application->MessageBox("Unknown or corruption session data format","Error",MB_OK);

	return false;
}



void __fastcall TFormMain::SaveSession(AnsiString filename)
{
	FILE *file;
	int i,j;
	unsigned char pal[16];

	file=fopen(filename.c_str(),"wt");

	if(!file)
	{
		Application->MessageBox("Can't save session","Error",MB_OK);
		return;
	}

	//signature

	fprintf(file,"%s\n\n",sessionIDStr);

	//screen buttons state

	fprintf(file,"BtnTiles=%i\n"        ,SpeedButtonTiles        ->Down?1:0);
	fprintf(file,"BtnChecker=%i\n"      ,SpeedButtonChecker      ->Down?1:0);
	fprintf(file,"BtnSelTiles=%i\n"     ,SpeedButtonSelTiles     ->Down?1:0);
	fprintf(file,"BtnChrBank1=%i\n"     ,SpeedButtonChrBank1     ->Down?1:0);
	fprintf(file,"BtnChrBank2=%i\n"     ,SpeedButtonChrBank2     ->Down?1:0);
	fprintf(file,"BtnGridAll=%i\n"      ,SpeedButtonGridAll      ->Down?1:0);
	fprintf(file,"BtnGridTile=%i\n"     ,SpeedButtonGridTile     ->Down?1:0);
	fprintf(file,"BtnGridAtr=%i\n"      ,SpeedButtonGridAtr      ->Down?1:0);
	fprintf(file,"BtnGridBlock=%i\n"    ,SpeedButtonGridBlock    ->Down?1:0);
	fprintf(file,"BtnPal=%i\n"          ,SpeedButtonPal          ->Down?1:0);
	fprintf(file,"BtnTypeIn=%i\n"       ,SpeedButtonTypeIn       ->Down?1:0);
	fprintf(file,"BtnFrameAll=%i\n"     ,SpeedButtonFrameAll     ->Down?1:0);
	fprintf(file,"BtnFrameSelected=%i\n",SpeedButtonFrameSelected->Down?1:0);
	fprintf(file,"BtnFrameNone=%i\n"    ,SpeedButtonFrameNone    ->Down?1:0);
	fprintf(file,"BtnSpriteSnap=%i\n"   ,SpeedButtonSpriteSnap   ->Down?1:0);
	fprintf(file,"BtnSprite8x16=%i\n"   ,SpeedButtonSprite8x16   ->Down?1:0);

	fprintf(file,"MenuBestOffsets=%i\n"   ,MImportBestOffsets->Checked?1:0);
	fprintf(file,"MenuLossy=%i\n"         ,MImportLossy->Checked?1:0);
	fprintf(file,"MenuThreshold=%i\n"     ,MImportThreshold->Checked?1:0);
	fprintf(file,"MenuMetaSprAutoInc=%i\n",MPutSelectionToMetaspriteAutoInc->Checked?1:0);

	//variables

	fprintf(file,"VarBgPalCur=%i\n"  ,bgPalCur);
	fprintf(file,"VarPalActive=%i\n" ,palActive);
	fprintf(file,"VarTileActive=%i\n",tileActive);
	fprintf(file,"VarBankActive=%i\n",bankActive);
	fprintf(file,"VarPPUMask=%i\n"   ,ppuMask);

	fprintf(file,"VarMetaSpriteActive=%i\n",metaSpriteActive);
	fprintf(file,"VarSpriteActive=%i\n"    ,spriteActive);
	fprintf(file,"VarSpriteGridX=%i\n"     ,spriteGridX);
	fprintf(file,"VarSpriteGridY=%i\n"     ,spriteGridY);

	fprintf(file,"VarNameW=%i\n"         ,nameTableWidth);
	fprintf(file,"VarNameH=%i\n"         ,nameTableHeight);
	fprintf(file,"VarNameViewX=%i\n"     ,nameTableViewX);
	fprintf(file,"VarNameViewY=%i\n"     ,nameTableViewY);
	fprintf(file,"VarNameSelectionL=%i\n",nameSelection.Left);
	fprintf(file,"VarNameSelectionR=%i\n",nameSelection.Right);
	fprintf(file,"VarNameSelectionT=%i\n",nameSelection.Top);
	fprintf(file,"VarNameSelectionB=%i\n",nameSelection.Bottom);
	fprintf(file,"VarNameCopyW=%i\n"     ,nameCopyWidth);
	fprintf(file,"VarNameCopyH=%i\n"     ,nameCopyHeight);

	fprintf(file,"VarCHRSelectionL=%i\n",chrSelection.Left);
	fprintf(file,"VarCHRSelectionR=%i\n",chrSelection.Right);
	fprintf(file,"VarCHRSelectionT=%i\n",chrSelection.Top);
	fprintf(file,"VarCHRSelectionB=%i\n",chrSelection.Bottom);
	fprintf(file,"VarCHRCopyW=%i\n"     ,chrCopyWidth);
	fprintf(file,"VarCHRCopyH=%i\n"     ,chrCopyHeight);
	fprintf(file,"VarCHRCopyRect=%i\n"  ,chrCopyRect?1:0);
	fprintf(file,"VarCHRSelectRect=%i\n",chrSelectRect?1:0);

	nss_put_bytes(file,"VarCHRSelected=",chrSelected,sizeof(chrSelected));

	//arrays

	nss_put_bytes(file,"Palette="  ,bgPal        ,sizeof(bgPal));
	nss_put_bytes(file,"PalUndo="  ,bgPalUndo    ,sizeof(bgPalUndo));

	nss_put_bytes(file,"CHRMain="  ,chr          ,sizeof(chr));
	nss_put_bytes(file,"CHRCopy="  ,chrCopy      ,sizeof(chrCopy));
	nss_put_bytes(file,"CHRUndo="  ,undoChr      ,sizeof(undoChr));

	nss_put_bytes(file,"NameTable=",nameTable    ,name_size());
	nss_put_bytes(file,"NameCopy=" ,nameCopy     ,name_size());
	nss_put_bytes(file,"NameUndo=" ,undoNameTable,name_size());

	nss_put_bytes(file,"AttrTable=",attrTable    ,attr_size());
	nss_put_bytes(file,"AttrCopy=" ,attrCopy     ,attr_size());
	nss_put_bytes(file,"AttrUndo=" ,undoAttrTable,attr_size());

	nss_put_bytes(file,"MetaSprites=",metaSprites,sizeof(metaSprites));

	//save dialogs settings

	fprintf(file,"FilterCHR=%i\n" ,SaveDialogChr ->FilterIndex);
	fprintf(file,"FilterName=%i\n",SaveDialogName->FilterIndex);
	fprintf(file,"FilterAttr=%i\n",SaveDialogAttr->FilterIndex);

	fprintf(file,"FileNameCHR=%s\n"           ,SaveDialogChr           ->FileName.c_str());
	fprintf(file,"FileNameName=%s\n"          ,SaveDialogName          ->FileName.c_str());
	fprintf(file,"FileNameAttr=%s\n"          ,SaveDialogAttr          ->FileName.c_str());
	fprintf(file,"FileNamePal=%s\n"           ,SaveDialogPal           ->FileName.c_str());
	fprintf(file,"FileNameMetaSpriteBank=%s\n",SaveDialogMetaSpriteBank->FileName.c_str());

	fclose(file);
}



void __fastcall TFormMain::UpdateRGBM(void)
{
	SpeedButtonMaskB->Caption="B"+IntToStr((ppuMask&0x80?1:0));
	SpeedButtonMaskG->Caption="G"+IntToStr((ppuMask&0x40?1:0));
	SpeedButtonMaskR->Caption="R"+IntToStr((ppuMask&0x20?1:0));
	SpeedButtonMaskM->Caption="M"+IntToStr((ppuMask&0x01?1:0));
}



bool __fastcall TFormMain::MouseTypeIn(int X,int Y)
{
	if(SpeedButtonTypeIn->Down)
	{
		nameSelection.left  =nameTableViewX+X/(8*uiScale);
		nameSelection.top   =nameTableViewY+Y/(8*uiScale);
		nameSelection.right =nameSelection.left+1;
		nameSelection.bottom=nameSelection.top +1;

		UpdateNameTable(-1,-1,true);
		UpdateStats();

		return true;
	}

	return false;
}



void __fastcall TFormMain::NameTableTypeIn(int tile)
{
	int dx,dy;

	if(nameSelection.left>=0&&nameSelection.right>=0)
	{
		dx=nameSelection.left;
		dy=nameSelection.top;

		SetUndo();

		if(SpeedButtonTiles->Down) nameTable[dy*nameTableWidth+dx]=(tile+typeInFontOffset)&255;

		if(SpeedButtonPal->Down) AttrSet(dx,dy,palActive);

		++nameSelection.left;

		if(nameSelection.left>=nameTableWidth) nameSelection.left=nameTableWidth-1;
		if(nameSelection.left<0) nameSelection.left=0;

		nameSelection.right =nameSelection.left+1;
		nameSelection.bottom=nameSelection.top+1;

		UpdateNameTable(dx,dy,false);
	}
}



void __fastcall TFormMain::UpdateMetaSprite(void)
{
	int i,j,x,y,cnt,tile,attr,pp,hcol,vcol,bank;
	char str[128];
	TColor frame;
	unsigned char *d1;
	bool sel[64];

	//clear

	ImageMetaSprite->Picture->Bitmap->Canvas->Brush->Style=bsSolid;
	ImageMetaSprite->Picture->Bitmap->Canvas->Brush->Color=(TColor)outPalette[bgPal[0]];
	ImageMetaSprite->Picture->Bitmap->Canvas->FillRect(TRect(0,0,128*uiScale,128*uiScale));

	//draw grid

	if(SpeedButtonSpriteGrid->Down)
	{
		for(i=0;i<ImageMetaSprite->Width;i+=8*uiScale)
		{
			d1=(unsigned char*)ImageMetaSprite->Picture->Bitmap->ScanLine[i];

			for(j=0;j<ImageMetaSprite->Width;j+=2)
			{
				hcol=(spriteGridY*uiScale==i)?128:64;

				*d1+++=hcol;
				*d1+++=hcol;
				*d1+++=hcol;

				d1+=3;
			}

			for(j=1;j<ImageMetaSprite->Height;j+=2)
			{
				d1=(unsigned char*)ImageMetaSprite->Picture->Bitmap->ScanLine[j]+i*3;

				vcol=(spriteGridX*uiScale==i)?128:64;

				*d1+++=vcol;
				*d1+++=vcol;
				*d1+++=vcol;
			}

			*d1=*d1;//to prevent warning
		}
	}

	//draw sprites

	pp=metaSpriteActive*64*4+63*4;

	for(i=63;i>=0;--i)//reverse order to make proper sprites drawing priority
	{
		y   =metaSprites[pp+0];
		tile=metaSprites[pp+1];
		attr=metaSprites[pp+2];
		x   =metaSprites[pp+3];

		if(y<255)
		{
			frame=SpeedButtonFrameAll->Down?clGray:clBlack;

			if(!SpeedButtonFrameNone->Down&&(spriteActive==i)) frame=clWhite;

			DrawSpriteTile(ImageMetaSprite->Picture,x*uiScale,y*uiScale,tile,attr,frame);
		}

		pp-=4;
	}

	ImageMetaSprite->Repaint();

	//update list

	pp=metaSpriteActive*64*4;
	cnt=0;

	for(i=0;i<64;++i)
	{
		if(metaSprites[pp]<255) ++cnt;

		pp+=4;
	}

	if(spriteActive<0) spriteActive=0;

	if(spriteActive>cnt-1) spriteActive=cnt-1;

	for(i=0;i<64;++i)
	{
		if(i<ListBoxSpriteList->Items->Count) sel[i]=ListBoxSpriteList->Selected[i]; else sel[i]=false;
	}

	sel[spriteActive]=true;

	ListBoxSpriteList->Clear();

	pp=metaSpriteActive*64*4;

	for(i=0;i<64;++i)
	{
		if(metaSprites[pp]<255)
		{
			x   =metaSprites[pp+3]-spriteGridX;
			y   =metaSprites[pp+0]-spriteGridY;
			tile=metaSprites[pp+1];
			bank=metaSprites[pp+1]&1;

			if(SpeedButtonSprite8x16->Down) tile&=0xfe;

			sprintf(str,"%2.2x: X%c%3.3i Y%c%3.3i Tile $%2.2x Pal %i",i,x<0?'-':' ',abs(x),y<0?'-':' ',abs(y),tile,metaSprites[pp+2]&3);

			if(SpeedButtonSprite8x16->Down) strcat(str,!bank?" A":" B");

			if(metaSprites[pp+2]&OAM_FLIP_H) strcat(str," H");
			if(metaSprites[pp+2]&OAM_FLIP_V) strcat(str," V");

			ListBoxSpriteList->Items->Add(str);

			ListBoxSpriteList->Selected[ListBoxSpriteList->Items->Count-1]=sel[i];
		}

		pp+=4;
	}

	ListBoxSpriteList->ItemIndex=spriteActive;

	LabelMetaSprite->Caption="Metasprite "+IntToStr(metaSpriteActive);
}



void squeeze_sprites(void)
{
	int i,j,k,pp;

	pp=0;

	for(i=0;i<256;++i)
	{
		for(j=0;j<63;++j)
		{
			if(metaSprites[pp+j*4]==255)
			{
				memcpy(&metaSprites[pp+j*4],&metaSprites[pp+j*4+4],64*4-j*4-4);

				for(k=0;k<4;++k) metaSprites[pp+63*4+k]=255;
			}
		}

		pp+=64*4;
	}

	pp=metaSpriteActive*64*4+spriteActive*4;

	while(pp>=4)
	{
		if(metaSprites[pp]==255) --spriteActive; else break;

		pp-=4;
	}
}



void __fastcall TFormMain::MoveSprite(int dx,int dy)
{
	int i,off;

	if(SpeedButtonSpriteSnap->Down)
	{
		dx*=8;
		dy*=8;
	}

	for(i=0;i<ListBoxSpriteList->Items->Count;++i)
	{
		if(ListBoxSpriteList->Selected[i])
		{
			off=metaSpriteActive*64*4+i*4;

			if(metaSprites[off]<255)
			{
				if(SpeedButtonSpriteSnap->Down)
				{
					if(dy!=0)
					{
						if(metaSprites[off+0]&7) metaSprites[off+0]&=~7; else metaSprites[off+0]+=dy;
					}

					if(dx!=0)
					{
						if(metaSprites[off+3]&7) metaSprites[off+3]&=~7; else metaSprites[off+3]+=dx;
					}
				}
				else
				{
					metaSprites[off+0]+=dy;
					metaSprites[off+3]+=dx;
				}
			}
		}
	}

	UpdateMetaSprite();
}




int __fastcall TFormMain::GetSpriteID(int x,int y)
{
	int i,pp,hgt;

	if(SpeedButtonSprite8x16->Down) hgt=16; else hgt=8;

	pp=metaSpriteActive*64*4;

	x/=uiScale;
	y/=uiScale;

	for(i=0;i<64;++i)
	{
		if(y>=metaSprites[pp]&&y<metaSprites[pp]+hgt&&x>=metaSprites[pp+3]&&x<metaSprites[pp+3]+8) return i;

		pp+=4;
	}

	return -1;
}



void __fastcall TFormMain::SpriteSnap(int id)
{
	int x,y,off;

	off=metaSpriteActive*64*4+id*4;

	if(metaSprites[off]<255)
	{
		x=metaSprites[off+3];
		y=metaSprites[off+0];

		if((x&7)<4) x&=~7; else x=(x&~7)+8;
		if((y&7)<4) y&=~7; else y=(y&~7)+8;

		metaSprites[off+3]=x;
		metaSprites[off+0]=y;
	}
}



void __fastcall TFormMain::SelectSprite(int id,bool multi)
{
	int i,off;

	spriteActive=id;

	if(!multi)
	{
		for(i=0;i<ListBoxSpriteList->Items->Count;++i)
		{
			ListBoxSpriteList->Selected[i]=(i==id)?true:false;
		}
	}

	if(spriteActive>=0)
	{
		off=metaSpriteActive*64*4+spriteActive*4;

		if(metaSprites[off]<255)
		{
			SelectTile   (metaSprites[off+1]);
			SelectPalette(metaSprites[off+2]&3);

			UpdateTiles(true);
		}
	}

	UpdateMetaSprite();
}



void __fastcall TFormMain::SelectTile(int id)
{
	int i;

	tileActive=id;

	chrSelection.left  =tileActive&15;
	chrSelection.top   =tileActive/16;
	chrSelection.right =chrSelection.left+1;
	chrSelection.bottom=chrSelection.top +1;

	for(i=0;i<256;++i) chrSelected[i]=0;

	chrSelected[tileActive]=1;
	chrSelectRect=true;
}



void __fastcall TFormMain::SelectPalette(int id)
{
	palActive=id;
	DrawPalettes();
}



bool __fastcall TFormMain::OpenMetaSprites(AnsiString name)
{
	FILE *file;
	int size;
	unsigned char data[2];

	file=fopen(name.c_str(),"rb");

	if(!file) return false;

	fseek(file,0,SEEK_END);
	size=ftell(file);
	fseek(file,0,SEEK_SET);

	if(size!=256*64*4+2) return false;

	fread(data,2,1,file);
	fread(metaSprites,256*64*4,1,file);
	fclose(file);

	spriteGridX=data[0];
	spriteGridY=data[1];

	UpdateMetaSprite();

	return true;
}



void __fastcall TFormMain::FindDoublesUnused(bool unused)
{
	int i,j,cnt,used;

	cnt=0;

	for(i=0;i<256;++i)
	{
		chrSelected[i]=0;

		if(unused)
		{
			used=0;

			for(j=0;j<nameTableWidth*nameTableHeight;++j) if(nameTable[j]==i) ++used;

			if(!used)
			{
				chrSelected[i]=1;
				++cnt;
			}
		}
		else
		{
			if(!memcmp(chr+bankActive+(tileActive<<4),chr+bankActive+(i<<4),16))
			{
				chrSelected[i]=1;
				++cnt;
			}
		}
	}

	if(cnt)
	{
		chrSelectRect=false;
		nameSelection.left=-1;
		nameSelection.top=-1;

		UpdateTiles(true);
		UpdateNameTable(-1,-1,true);
		UpdateStats();
	}
}



void __fastcall TFormMain::RemoveDoublesUnused(bool unused)
{
	int i,j,k,pp,x,y,w,h,used;
	bool clear[256],skip[256];

	SetUndo();

	GetSelection(chrSelection,x,y,w,h);

	for(i=0;i<256;++i)
	{
		clear[i]=false;
		skip[i]=false;
	}

	if(chrSelectRect)
	{
		if(w>1||h>1)
		{
			for(i=0;i<16;++i)
			{
				for(j=0;j<16;++j)
				{
					skip[i*16+j]=(i>=y&&i<y+h&&j>=x&&j<x+w)?true:false;
				}
			}
		}
	}
	else
	{
		for(i=0;i<256;++i) skip[i]=chrSelected[i];
	}

	for(i=0;i<256;++i)
	{
		if(!clear[i]&&!skip[i])
		{
			if(unused)
			{
				used=0;

				for(j=0;j<nameTableWidth*nameTableHeight;++j) if(nameTable[j]==i) ++used;

				if(!used) clear[i]=true;
			}
			else
			{
				for(j=i+1;j<256;j++)
				{
					if(!memcmp(&chr[bankActive+i*16],&chr[bankActive+j*16],16)&&!skip[j])
					{
						clear[j]=true;

						for(k=0;k<nameTableWidth*nameTableHeight;k++) if(nameTable[k]==j) nameTable[k]=i;
					}
				}
			}
		}
	}

	pp=0;

	for(i=0;i<256;++i)
	{
		if(!clear[i]&&!skip[i])
		{
			while(skip[pp/16]) pp+=16;

			for(j=0;j<nameTableWidth*nameTableHeight;++j) if(nameTable[j]==i) nameTable[j]=pp/16;

			for(j=0;j<16;++j)
			{
				chr[bankActive+pp]=chr[bankActive+i*16+j];

				++pp;
			}
		}
	}

	for(;pp<4096;pp++) if(!skip[pp/16]) chr[bankActive+pp]=0;

	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	UpdateMetaSprite();
}



AnsiString __fastcall TFormMain::FilterIndexToExt(int index)
{
	switch(index)
	{
	case SAVE_NAMETABLE_NAM: return ".nam";
	case SAVE_NAMETABLE_BIN: return ".bin";
	case SAVE_NAMETABLE_ASM: return ".asm";
	case SAVE_NAMETABLE_H:   return ".h";
	case SAVE_NAMETABLE_RLE: return ".rle";
	case SAVE_NAMETABLE_RLE_ASM: return ".asm";
	case SAVE_NAMETABLE_RLE_H: return ".h";
	}

	return "";
}



bool __fastcall TFormMain::OverwritePrompt(AnsiString filename)
{
	if(FileExists(filename)) if(Application->MessageBox(("File "+filename+" is already exist, overwrite?").c_str(),"Confirm",MB_YESNO)!=IDYES) return false;

	return true;
}



void __fastcall TFormMain::SetLatestCHRSize(int size)
{
	AnsiString str;

	CHRLatestSize=size;

	str="Latest ";

	switch(size)
	{
	case 1024: str+="(1K)"; break;
	case 2048: str+="(2K)"; break;
	case 4096: str+="(4K)"; break;
	case 8192: str+="(8K)"; break;
	default: str+="(Selection)"; break;
	}

	MCHRSaveLatest->Caption=str;
}



void __fastcall TFormMain::UpdateAll(void)
{
	bool normal;

	if(nameTableWidth==32&&nameTableHeight==30) normal=true; else normal=false;

	MSaveNameTableAttributes->Enabled=normal;
	MSaveNameTableOnly->Enabled=normal;
	MSaveAttributes->Enabled=normal;
	
	pal_validate();

	CorrectView();
	DrawPalettes();
	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	UpdateMetaSprite();
	UpdateStats();
}



void __fastcall TFormMain::CorrectView(void)
{
	if(nameTableWidth<32)
	{
		nameTableViewX=-(32-nameTableWidth)/2;
	}
	else
	{
		if(nameTableViewX<0) nameTableViewX=0;
		if(nameTableViewX>=nameTableWidth-32) nameTableViewX=nameTableWidth-32;
	}

	if(nameTableHeight<32)
	{
		nameTableViewY=-(32-nameTableHeight)/2;
	}
	else
	{
		if(nameTableViewY<0) nameTableViewY=0;
		if(nameTableViewY>=nameTableHeight-32) nameTableViewY=nameTableHeight-32;
	}
}



//---------------------------------------------------------------------------
__fastcall TFormMain::TFormMain(TComponent* Owner)
: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TFormMain::FormPaint(TObject *Sender)
{
	UpdateAll();
}
//---------------------------------------------------------------------------

int FormOriginalWidth;
int FormOriginalHeight;
int PanelEditAreaOriginalWidth;
int PanelEditAreaOriginalHeight;
int PanelToolbarOriginalWidth;
int PanelToolbarOriginalHeight;
int PanelTilesetOriginalHeight;
int GroupBoxTilesOriginalHeight;
int PageControlEditorOriginalHeight;
int GroupBoxMetaSpriteOriginalWidth;
int PanelSpriteViewOriginalHeight;

void __fastcall TFormMain::UpdateUIScale(void)
{
	MView2x->Checked=(uiScale==2)?true:false;
	MView3x->Checked=(uiScale==3)?true:false;
	MView4x->Checked=(uiScale==4)?true:false;

	Width =FormOriginalWidth -512-256+256*uiScale+128*uiScale;
	Height=FormOriginalHeight-512+256*uiScale;

	PanelEditArea->Width =PanelEditAreaOriginalWidth -512+256*uiScale;
	PanelToolbar ->Width =PanelToolbarOriginalWidth  -256+128*uiScale;
	PanelTileset ->Height=PanelTilesetOriginalHeight -256+128*uiScale;
	GroupBoxTiles->Height=GroupBoxTilesOriginalHeight-256+128*uiScale;
	PageControlEditor ->Height=PageControlEditorOriginalHeight-512+256*uiScale;
	GroupBoxMetaSprite->Width=GroupBoxMetaSpriteOriginalWidth -256+128*uiScale;
	PanelSpriteView   ->Height=PanelSpriteViewOriginalHeight  -256+128*uiScale;

	ImageName      ->Width =256*uiScale;
	ImageName      ->Height=256*uiScale;
	ImageTiles     ->Width =128*uiScale;
	ImageTiles     ->Height=128*uiScale;
	ImageMetaSprite->Width =128*uiScale;
	ImageMetaSprite->Height=128*uiScale;

	ImageName      ->Picture->Bitmap->SetSize(ImageName      ->Width,ImageName      ->Height);
	ImageTiles     ->Picture->Bitmap->SetSize(ImageTiles     ->Width,ImageTiles     ->Height);
	ImageMetaSprite->Picture->Bitmap->SetSize(ImageMetaSprite->Width,ImageMetaSprite->Height);

	UpdateAll();
}

void __fastcall TFormMain::FormCreate(TObject *Sender)
{
	FILE *file;
	int i,pp;
	unsigned char buf[192];
	AnsiString dir,name;

	FormCaption=Caption;

	bgPal[0x00]=15;
	bgPal[0x01]=0;
	bgPal[0x02]=16;
	bgPal[0x03]=48;
	bgPal[0x04]=15;
	bgPal[0x05]=1;
	bgPal[0x06]=33;
	bgPal[0x07]=49;
	bgPal[0x08]=15;
	bgPal[0x09]=6;
	bgPal[0x0a]=22;
	bgPal[0x0b]=38;
	bgPal[0x0c]=15;
	bgPal[0x0d]=9;
	bgPal[0x0e]=25;
	bgPal[0x0f]=41;

	nameTableWidth=32;
	nameTableHeight=30;

	nameTableViewX=0;
	nameTableViewY=0;
	
	ImageName->Picture=new TPicture();
	ImageName->Picture->Bitmap=new Graphics::TBitmap();
	ImageName->Picture->Bitmap->PixelFormat=pf24bit;

	ImageTiles->Picture=new TPicture();
	ImageTiles->Picture->Bitmap=new Graphics::TBitmap();
	ImageTiles->Picture->Bitmap->PixelFormat=pf24bit;

	ImageMetaSprite->Picture=new TPicture();
	ImageMetaSprite->Picture->Bitmap=new Graphics::TBitmap();
	ImageMetaSprite->Picture->Bitmap->PixelFormat=pf24bit;

	FormOriginalWidth          =Width;
	FormOriginalHeight         =Height;
	PanelEditAreaOriginalWidth =PanelEditArea->Width;
	PanelEditAreaOriginalHeight=PanelEditArea->Height;
	PanelToolbarOriginalWidth  =PanelToolbar->Width;
	PanelToolbarOriginalHeight =PanelToolbar->Height;
	PanelTilesetOriginalHeight =PanelTileset->Height;
	GroupBoxTilesOriginalHeight=GroupBoxTiles->Height;
	PageControlEditorOriginalHeight=PageControlEditor->Height;
	GroupBoxMetaSpriteOriginalWidth=GroupBoxMetaSprite->Width;
	PanelSpriteViewOriginalHeight=PanelSpriteView->Height;

	uiScale=2;

	UpdateUIScale();

	bgPalCur=0;
	palActive=0;
	tileActive=0;
	bankActive=0;
	metaSpriteActive=0;

	spriteGridX=64;
	spriteGridY=64;

	typeInFontOffset=0;

	memset(nameTable  ,0  ,sizeof(nameTable));
	memset(attrTable  ,0  ,sizeof(attrTable));
	memset(chr        ,0  ,sizeof(chr));
	memset(chrCopy    ,0  ,sizeof(chrCopy));
	memset(metaSprites,255,sizeof(metaSprites));
	memset(metaSpriteCopy,255,sizeof(metaSpriteCopy));

	dir=ParamStr(0).SubString(0,ParamStr(0).LastDelimiter("\\/"));

	file=fopen((dir+"nes.pal").c_str(),"rb");

	if(file)
	{
		if(get_file_size(file)==192)
		{
			fread(buf,192,1,file);
			fclose(file);

			pp=0;

			for(i=0;i<64;i++)
			{
				basePalette[i]=(buf[pp+2]<<16)|(buf[pp+1]<<8)|buf[pp];
				pp+=3;
			}
		}
	}
	else
	{
		pp=0;

		for(i=0;i<64;i++)
		{
			basePalette[i]=(palette[pp+2]<<16)|(palette[pp+1]<<8)|palette[pp];
			pp+=3;
		}
	}

	ppuMask=0;
	palette_calc();

	nameSelection.left=-1;
	nameSelection.top=-1;
	nameCopyWidth=-1;
	nameCopyHeight=-1;

	chrSelection.left=0;
	chrSelection.right=0;
	chrSelection.right=1;
	chrSelection.bottom=1;
	chrCopyWidth=-1;
	chrCopyHeight=-1;
	chrCopyRect=true;

	for(i=0;i<256;i++) chrSelected[i]=0;

	chrSelected[tileActive]=1;
	chrSelectRect=true;

	tileXC=-1;
	tileYC=-1;
	nameXC=-1;
	nameYC=-1;
	palHover=-1;
	palColHover=-1;
	colHover=-1;
	spriteHover=-1;
	spriteDrag=-1;

	dir=reg_load_str(regWorkingDirectory,"");

	if(ParamStr(1)!="")
	{
		name=RemoveExt(ParamStr(1));

		if(!ImportBMP(name+".bmp",false,false,-1))
		{
			if(!LoadSession(name+".nss"))
			{
				OpenAll(name);
			}
		}

		dir=ParamStr(1);
	}

	SetCurrentDirectory(dir.c_str());

	SetLatestCHRSize(8192);

	SetUndo();
}
//---------------------------------------------------------------------------

int check_palette_mouse(int X,int Y)
{
	int i,x,y,s;

	s=2;//uiScale;
	y=0;
	x=8*s;

	//check BG palettes

	for(i=0;i<4;i++)
	{
		if(Y>=y&&Y<y+10*s)
		{
			if(X>=x+ 0*s&&X<x+10*s) return 0x00|i;
			if(X>=x+10*s&&X<x+20*s) return 0x10|i;
			if(X>=x+20*s&&X<x+30*s) return 0x20|i;
			if(X>=x+30*s&&X<x+40*s) return 0x30|i;
		}

		x+=(64-8)*s;

		if(x>(64*s))
		{
			x=8*s;
			y=16*s;
		}
	}

	//check the main palette

	if(X>=0&&X<(7*16*s)&&Y>=(32*s)&&Y<(32*s+4*8*s))
	{
		return 0x100|(X/(8*s)+(Y-32*s)/(8*s)*16);
	}

	return -1;
}



void __fastcall TFormMain::PaintBoxPalMouseDown(TObject *Sender,
TMouseButton Button, TShiftState Shift, int X, int Y)
{
	int i,n,col,off;

	n=check_palette_mouse(X,Y);

	if(Shift.Contains(ssLeft))
	{
		if(n>=0)
		{
			if(n&0x100)
			{
				SetUndo();

				bgPal[palActive*4+bgPalCur]=n&0xff;

				col=bgPal[palActive*4+0];

				bgPal[0*4+0]=col;
				bgPal[1*4+0]=col;
				bgPal[2*4+0]=col;
				bgPal[3*4+0]=col;
			}
			else
			{
				palActive=n&3;
				bgPalCur=n>>4;
			}
		}
	}

	if(PageControlEditor->ActivePage==TabSheetSprite)
	{
		for(i=0;i<ListBoxSpriteList->Items->Count;++i)
		{
			if(ListBoxSpriteList->Selected[i])
			{
				off=metaSpriteActive*64*4+i*4;

				if(metaSprites[off]<255) metaSprites[off+2]=(metaSprites[off+2]&~3)+palActive;
			}
		}

		UpdateMetaSprite();
	}

	if(Shift.Contains(ssRight))
	{
		if(n>=0)
		{
			if(n&0x100) palDragColor=n&0xff; else palDragColor=bgPal[(n&3)*4+(n>>4)];

			PaintBoxPal->BeginDrag(false,-1);
		}
	}

	pal_validate();

	DrawPalettes();
	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	UpdateMetaSprite();
}
//---------------------------------------------------------------------------


void __fastcall TFormMain::ImageTilesMouseDown(TObject *Sender,
TMouseButton Button, TShiftState Shift, int X, int Y)
{
	int i,off;

	if(!(X>=0&&X<(128*uiScale)&&Y>=0&&Y<(128*uiScale))) return;

	if(Shift.Contains(ssCtrl)&&(Shift.Contains(ssLeft)||Shift.Contains(ssRight)))
	{
		chrSelected[Y/(8*uiScale)*16+X/(8*uiScale)]=Shift.Contains(ssLeft)?1:0;
		chrSelectRect=false;

		UpdateTiles(true);
	}
	else
	{
		SelectTile(Y/(8*uiScale)*16+X/(8*uiScale));

		UpdateTiles(true);
	}

	if(SpeedButtonTypeIn->Down)
	{
		NameTableTypeIn(tileActive);
	}
	else
	{
		nameSelection.left=-1;
		nameSelection.top=-1;

		UpdateTiles(true);
	}

	UpdateNameTable(-1,-1,true);
	UpdateStats();

	if(PageControlEditor->ActivePage==TabSheetName)
	{
		if(Shift.Contains(ssRight))
		{
			ImageTiles->BeginDrag(false,-1);
		}
	}

	if(PageControlEditor->ActivePage==TabSheetSprite)
	{
		if(Shift.Contains(ssRight))
		{
			ImageTiles->BeginDrag(false,-1);
		}

		if(!Shift.Contains(ssRight))
		{
			if(spriteActive>=0)
			{
				off=metaSpriteActive*64*4+spriteActive*4;

				if(metaSprites[off]<255)
				{
					if(!SpeedButtonSprite8x16->Down)
					{
						metaSprites[off+1]=tileActive;
					}
					else
					{
						metaSprites[off+1]=(tileActive&0xfe)|(metaSprites[off+1]&1);
					}

					UpdateMetaSprite();
				}
			}
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonGridAllClick(TObject *Sender)
{
	if(((TSpeedButton*)Sender)->GroupIndex==10)
	{
		SpeedButtonGridTile->Down=SpeedButtonGridAll->Down;
		SpeedButtonGridAtr->Down=SpeedButtonGridAll->Down;
		SpeedButtonGridBlock->Down=SpeedButtonGridAll->Down;
	}
	if(!SpeedButtonGridTile->Down||!SpeedButtonGridAtr->Down||!SpeedButtonGridBlock->Down) SpeedButtonGridAll->Down=false;
	if(SpeedButtonGridTile->Down&&SpeedButtonGridAtr->Down&&SpeedButtonGridBlock->Down) SpeedButtonGridAll->Down=true;

	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHROpenClick(TObject *Sender)
{
	if(!OpenDialogChr->Execute()) return;

	if(OpenCHR(OpenDialogChr->FileName))
	{
		OpenDialogChr->FileName=RemoveExt(OpenDialogChr->FileName);

		SaveDialogChr->FileName=OpenDialogChr->FileName;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::PaintBoxNamePaint(TObject *Sender)
{
	UpdateNameTable(-1,-1,true);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ImageNameMouseDown(TObject *Sender, TMouseButton Button,
TShiftState Shift, int X, int Y)
{
	int i,nx,ny;

	nx=X/(8*uiScale)+nameTableViewX;
	ny=Y/(8*uiScale)+nameTableViewY;

	if(nameXC<0||nameXC>=nameTableWidth||nameYC<0||nameYC>=nameTableHeight) return;

	if(Shift.Contains(ssLeft)) if(MouseTypeIn(X,Y)) return;

	if((Shift.Contains(ssShift)||GetKeyState(VK_CAPITAL))&&Shift.Contains(ssLeft))
	{
		nameSelection.left  =nx;
		nameSelection.top   =ny;
		nameSelection.right =nameSelection.left+1;
		nameSelection.bottom=nameSelection.top +1;

		chrSelection.right =chrSelection.left+1;
		chrSelection.bottom=chrSelection.top +1;

		for(i=0;i<256;++i) chrSelected[i]=0;

		chrSelected[tileActive]=1;
		chrSelectRect=true;

		UpdateTiles(true);
		UpdateNameTable(-1,-1,true);
	}
	else
	{
		if(Shift.Contains(ssLeft)) SetUndo();

		ImageNameMouseMove(Sender,Shift,X,Y);

		if(nameSelection.left>=0||nameSelection.top>=0)
		{
			nameSelection.left=-1;
			nameSelection.top =-1;

			UpdateNameTable(-1,-1,true);
		}
	}

	UpdateStats();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ImageNameMouseMove(TObject *Sender, TShiftState Shift,
int X, int Y)
{
	int i,j,dx,dy;
	int xmin,ymin,xmax,ymax;

	nameXC=X/(8*uiScale)+nameTableViewX;
	nameYC=Y/(8*uiScale)+nameTableViewY;

	if(nameXC<0||nameXC>=nameTableWidth||nameYC<0||nameYC>=nameTableHeight)
	{
		nameXC=-1;
		nameYC=-1;

		UpdateStats();

		return;
	}

	if(Shift.Contains(ssLeft)) if(MouseTypeIn(X,Y)) return;

	if(Shift.Contains(ssShift)||GetKeyState(VK_CAPITAL))
	{
		if(Shift.Contains(ssLeft))
		{
			nameSelection.right =nameXC+(nameXC>=nameSelection.left?1:0);
			nameSelection.bottom=nameYC+(nameYC>=nameSelection.top ?1:0);
		}

		if(Shift.Contains(ssRight))
		{
			nameSelection.left=-1;
			nameSelection.top =-1;
		}

		UpdateNameTable(-1,-1,true);
		UpdateStats();

		return;
	}

	if(Shift.Contains(ssLeft)&&nameSelection.left<0&&nameSelection.top<0)
	{
		if(chrSelectRect)
		{
			for(i=0;i<chrSelection.bottom-chrSelection.top;++i)
			{
				for(j=0;j<chrSelection.right-chrSelection.left;++j)
				{
					dx=nameXC+j;
					dy=nameYC+i;

					if(dx>=0&&dx<nameTableWidth&&dy>=0&&dy<nameTableHeight)
					{
						if(SpeedButtonTiles->Down) nameTable[dy*nameTableWidth+dx]=((chrSelection.top+i)<<4)+chrSelection.left+j;

						if(SpeedButtonPal->Down) AttrSet(dx,dy,palActive);

						UpdateNameTable(dx,dy,false);
					}
				}
			}
		}
		else
		{
			xmin=16;
			ymin=16;
			xmax=0;
			ymax=0;

			for(i=0;i<16;++i)
			{
				for(j=0;j<16;++j)
				{
					if(!chrSelected[i*16+j]) continue;

					if(j<xmin) xmin=j;
					if(j>xmax) xmax=j;
					if(i<ymin) ymin=i;
					if(i>ymax) ymax=i;
				}
			}

			for(i=ymin;i<=ymax;++i)
			{
				for(j=xmin;j<=xmax;++j)
				{
					if(!chrSelected[i*16+j]) continue;

					dx=nameXC+j-xmin;
					dy=nameYC+i-ymin;

					if(dx>=0&&dx<nameTableWidth&&dy>=0&&dy<nameTableHeight)
					{
						if(SpeedButtonTiles->Down) nameTable[dy*nameTableWidth+dx]=i*16+j;

						if(SpeedButtonPal->Down) AttrSet(dx,dy,palActive);

						UpdateNameTable(dx,dy,false);
					}
				}
			}
		}

		UpdateNameTable(nameXC,nameYC,true);
	}

	if(Shift.Contains(ssRight))
	{
		SelectTile(nameTable[nameYC*nameTableWidth+nameXC]);
		SelectPalette(AttrGet(nameXC,nameYC));

		UpdateTiles(true);
		UpdateNameTable(-1,-1,true);
	}

	UpdateStats();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MOpenNameTableClick(TObject *Sender)
{
	if(!OpenDialogName->Execute()) return;

	if(OpenNameTable(OpenDialogName->FileName))
	{
		OpenDialogName->FileName=RemoveExt(OpenDialogName->FileName);
		SaveDialogName->FileName=OpenDialogName->FileName;
		SaveDialogMap ->FileName=OpenDialogName->FileName;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MSaveNameTableOnlyClick(TObject *Sender)
{
	AnsiString name;

	SaveDialogName->FileName=RemoveExt(SaveDialogName->FileName);
	SaveDialogName->Title="Save nametable only";

	if(!SaveDialogName->Execute()) return;

	name=RemoveExt(SaveDialogName->FileName)+FilterIndexToExt(SaveDialogName->FilterIndex);

	if(!OverwritePrompt(name)) return;

	name=save_data(name.c_str(),nameTable,nameTableWidth*nameTableHeight,SaveDialogName->FilterIndex);

	if(name!="") Application->MessageBox(name.c_str(),"Error",MB_OK);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MSaveNameTableAttributesClick(TObject *Sender)
{
	unsigned char *buf;
	AnsiString name;

	SaveDialogName->FileName=RemoveExt(SaveDialogName->FileName);
	SaveDialogName->Title="Save nametable and attributes";

	if(!SaveDialogName->Execute()) return;

	name=RemoveExt(SaveDialogName->FileName)+FilterIndexToExt(SaveDialogName->FilterIndex);

	if(FileExists(name))
	{
		if(Application->MessageBox(("File "+name+" is already exist, overwrite?").c_str(),"Confirm",MB_YESNO)!=IDYES)return;
	}

	buf=(unsigned char*)malloc(name_size()+attr_size());

	memcpy(buf,nameTable,name_size());
	memcpy(buf+name_size(),attrTable,attr_size());

	name=save_data(name.c_str(),buf,name_size()+attr_size(),SaveDialogName->FilterIndex);

	free(buf);

	if(name!="") Application->MessageBox(name.c_str(),"Error",MB_OK);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MOpenPalettesClick(TObject *Sender)
{
	if(!OpenDialogPal->Execute()) return;

	if(OpenPalette(OpenDialogPal->FileName))
	{
		OpenDialogPal->FileName=RemoveExt(OpenDialogPal->FileName);

		SaveDialogPal->FileName=OpenDialogPal->FileName;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MSavePalettesClick(TObject *Sender)
{
	FILE *file;
	unsigned char pal[16];
	int i;
	AnsiString name;

	if(!SaveDialogPal->Execute()) return;

	name=RemoveExt(SaveDialogPal->FileName)+".pal";

	if(!OverwritePrompt(name)) return;

	pal_validate();

	for(i=0;i<4;i++)
	{
		pal[i+ 0]=bgPal[0*4+i];
		pal[i+ 4]=bgPal[1*4+i];
		pal[i+ 8]=bgPal[2*4+i];
		pal[i+12]=bgPal[3*4+i];
	}

	file=fopen(name.c_str(),"wb");

	if(file)
	{
		fwrite(pal,16,1,file);
		fclose(file);
	}
}
//---------------------------------------------------------------------------


void __fastcall TFormMain::PaintBoxPalPaint(TObject *Sender)
{
	DrawPalettes();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHREditorClick(TObject *Sender)
{
	FormCHREditor->Visible^=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::FormKeyDown(TObject *Sender, WORD &Key,
TShiftState Shift)
{
	if(Key==VK_F1) PageControlEditor->ActivePage=TabSheetName;
	if(Key==VK_F2) PageControlEditor->ActivePage=TabSheetSprite;

	if(Shift.Contains(ssCtrl))
	{
		if(Key>='2'&&Key<='4')
		{
			uiScale=Key-'0';

			UpdateUIScale();
		}

		if(Key=='Z') Undo();

		if(nameSelection.left<0&&nameSelection.top<0)
		{
			if(Key=='X') CopyCHR(true,true);
			if(Key=='C') CopyCHR(true,false);
			if(Key=='V') PasteCHR();
		}
		else
		{
			if(Key=='X') CopyMap(true);
			if(Key=='C') CopyMap(false);
			if(Key=='V') PasteMap();
			if(Key=='F') FillMap();
		}

		if(Key=='M') MPutSelectionToMetaspriteClick(Sender);
	}
	else
	{
		if(nameSelection.left<0&&nameSelection.top<0)
		{
			if(Key==VK_DELETE) CopyCHR(false,true);

			if(Key=='H') FormCHREditor->MirrorHorizontal();
			if(Key=='V') FormCHREditor->MirrorVertical();
			if(Key=='R') FormCHREditor->Flip90(false);
			if(Key=='L') FormCHREditor->Flip90(true);
		}
	}

	if(SpeedButtonTypeIn->Down&&PageControlEditor->ActivePage==TabSheetName)
	{
		if(nameSelection.left>=0)
		{
			switch(Key)
			{
			case VK_BACK:
			case VK_LEFT:  --nameSelection.left; break;
			case VK_RIGHT: ++nameSelection.left; break;
			case VK_UP:    --nameSelection.top;  break;
			case VK_RETURN:
			case VK_DOWN:  ++nameSelection.top;  break;
			}

			if(nameSelection.left<0) nameSelection.left=0;
			if(nameSelection.left>=nameTableWidth) nameSelection.left=nameTableWidth-1;
			if(nameSelection.top<0) nameSelection.top=0;
			if(nameSelection.top>=nameTableHeight) nameSelection.top=nameTableHeight-1;

			nameSelection.right =nameSelection.left+1;
			nameSelection.bottom=nameSelection.top +1;

			UpdateNameTable(-1,-1,false);
		}

		return;
	}

	if(Shift.Contains(ssShift))
	{
		if(Key=='C')
		{
			CopyMap(false);
			CopyMapCodeC(false);
			return;
		}

		if(Key=='M')
		{
			CopyMetaSpriteCodeC(false);
			return;
		}

		if(Key=='N')
		{
			CopyMetaSpriteCodeC(true);
			return;
		}
	}

	if(Key==VK_OEM_4) SpeedButtonPrevMetaSpriteClick(Sender);// [
	if(Key==VK_OEM_6) SpeedButtonNextMetaSpriteClick(Sender);// ]

	if(PageControlEditor->ActivePage==TabSheetName)
	{
		if(!Shift.Contains(ssAlt))
		{
			if(!Shift.Contains(ssCtrl))
			{
				if(Key==VK_LEFT)  nameTableViewX-=4;
				if(Key==VK_RIGHT) nameTableViewX+=4;
				if(Key==VK_UP)    nameTableViewY-=4;
				if(Key==VK_DOWN)  nameTableViewY+=4;

				CorrectView();
				UpdateNameTable(-1,-1,true);
			}
			else
			{
				if(Key==VK_LEFT)  NameTableScrollLeft (Shift.Contains(ssShift));
				if(Key==VK_RIGHT) NameTableScrollRight(Shift.Contains(ssShift));
				if(Key==VK_UP)    NameTableScrollUp   (Shift.Contains(ssShift));
				if(Key==VK_DOWN)  NameTableScrollDown (Shift.Contains(ssShift));
			}
		}
		else
		{
			if(nameSelection.left>=0)
			{
				if(Key==VK_LEFT)
				{
					if(nameSelection.left>0)
					{
						--nameSelection.left;
						--nameSelection.right;
					}
				}

				if(Key==VK_RIGHT)
				{
					if(nameSelection.right<nameTableWidth)
					{
						++nameSelection.left;
						++nameSelection.right;
					}
				}

				if(Key==VK_UP)
				{
					if(nameSelection.top>0)
					{
						--nameSelection.top;
						--nameSelection.bottom;
					}
				}

				if(Key==VK_DOWN)
				{
					if(nameSelection.bottom<nameTableHeight)
					{
						++nameSelection.top;
						++nameSelection.bottom;
					}
				}

				UpdateNameTable(-1,-1,false);
			}
		}
	}
	else
	{
		if(Key==VK_LEFT)  MoveSprite(-1, 0);
		if(Key==VK_RIGHT) MoveSprite( 1, 0);
		if(Key==VK_UP)    MoveSprite( 0,-1);
		if(Key==VK_DOWN)  MoveSprite( 0, 1);
	}

	if(Key=='A')
	{
		SpeedButtonChecker->Down^=true;
		SpeedButtonCheckerClick(Sender);
	}

	UpdateStats();
}
//---------------------------------------------------------------------------



void __fastcall TFormMain::ImageTilesDblClick(TObject *Sender)
{
	FormCHREditor->Show();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::FormDestroy(TObject *Sender)
{
	char path[MAX_PATH];
	int len;

	GetCurrentDirectory(len,path);
	reg_save_str(regWorkingDirectory,path);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ImageNameMouseLeave(TObject *Sender)
{
	nameXC=-1;
	nameYC=-1;
	UpdateStats();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ImageTilesMouseLeave(TObject *Sender)
{
	tileXC=-1;
	tileYC=-1;

	UpdateStats();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ImageTilesMouseMove(TObject *Sender,
TShiftState Shift, int X, int Y)
{
	int i,j,tx,ty;
	int xs,ys,wdt,hgt;

	if(SpeedButtonTypeIn->Down) return;

	if(X>=0&&X<(128*uiScale)&&Y>=0&&Y<(128*uiScale))
	{
		tx=X/(8*uiScale);
		ty=Y/(8*uiScale);
		tileXC=tx;
		tileYC=ty;

		if(Shift.Contains(ssShift)&&!Shift.Contains(ssCtrl))
		{
			if(Shift.Contains(ssLeft))
			{
				chrSelection.right =tx+(tx>=chrSelection.left?1:0);
				chrSelection.bottom=ty+(ty>=chrSelection.top ?1:0);

				if(abs(chrSelection.left-chrSelection.right)>1||abs(chrSelection.top-chrSelection.bottom)>1)
				{
					nameSelection.left=-1;
					nameSelection.right=-1;

					UpdateNameTable(-1,-1,true);
				}

				for(i=0;i<256;i++) chrSelected[i]=false;

				xs=chrSelection.left<chrSelection.right ?chrSelection.left:chrSelection.right;
				ys=chrSelection.top <chrSelection.bottom?chrSelection.top :chrSelection.bottom;

				wdt=abs(chrSelection.right -chrSelection.left);
				hgt=abs(chrSelection.bottom-chrSelection.top);

				for(i=0;i<hgt;i++)
				{
					for(j=0;j<wdt;j++)
					{
						chrSelected[(i+ys)*16+j+xs]=1;
					}
				}

				chrSelectRect=true;
			}
		}

		if(Shift.Contains(ssCtrl)&&(Shift.Contains(ssLeft)||Shift.Contains(ssRight)))
		{
			chrSelected[ty*16+tx]=Shift.Contains(ssLeft)?1:0;
			chrSelectRect=false;

			if(SpeedButtonSelTiles->Down) UpdateNameTable(-1,-1,true);
		}
	}
	else
	{
		tileXC=-1;
		tileYC=-1;
	}

	UpdateTiles(false);
	UpdateStats();
}
//---------------------------------------------------------------------------


void __fastcall TFormMain::MCHRClearClick(TObject *Sender)
{
	int i;

	if(Application->MessageBox("Do you really want to clear 8K CHR?","Confirm",MB_YESNO)==ID_YES)
	{
		SetUndo();
		for(i=0;i<8192;i++) chr[i]=0;
		UpdateTiles(true);
		UpdateNameTable(-1,-1,true);
		UpdateMetaSprite();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonChrBank1Click(TObject *Sender)
{
	bankActive=SpeedButtonChrBank1->Down?0:4096;
	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHRRemoveDoublesClick(TObject *Sender)
{
	RemoveDoublesUnused(false);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::PaintBoxPalMouseMove(TObject *Sender,
TShiftState Shift, int X, int Y)
{
	int n;

	palHover=-1;
	palColHover=-1;
	colHover=-1;

	n=check_palette_mouse(X,Y);

	if(n>=0)
	{
		if(n&0x100)
		{
			colHover=n&0xff;
		}
		else
		{
			palHover=n&3;
			palColHover=n>>4;
		}
	}

	UpdateStats();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::PaintBoxPalMouseLeave(TObject *Sender)
{
	palHover=-1;
	palColHover=-1;
	colHover=-1;
	UpdateStats();
}
//---------------------------------------------------------------------------


void __fastcall TFormMain::MCHRInterleaveClick(TObject *Sender)
{
	InterleaveCHR(true);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHRDeinterleaveClick(TObject *Sender)
{
	InterleaveCHR(false);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHRSwapColorsClick(TObject *Sender)
{
	bool swap[256*2];
	int i,j,k,pp,col,bit;
	unsigned char paltemp[4];

	FormSwapColors->ShowModal();

	if(FormSwapColors->Swap)
	{
		SetUndo();

		for(i=0;i<256*2;i++) swap[i]=false;

		if(FormSwapColors->Selection)
		{
			for(i=0;i<256;i++) swap[(bankActive/4096*256)+i]=chrSelected[i];
		}
		else
		{
			if(FormSwapColors->WholeCHR)
			{
				for(i=0;i<256*2;i++) swap[i]=true;
			}
			else
			{
				for(i=0;i<256;i++) swap[(bankActive/4096*256)+i]=true;
			}
		}

		for(i=0;i<256*2;i++)
		{
			if(swap[i])
			{
				pp=i*16;

				for(j=0;j<8;j++)
				{
					for(k=0;k<8;k++)
					{
						bit=1<<k;
						col=((chr[pp]&bit)?1:0)|((chr[pp+8]&bit)?2:0);
						col=FormSwapColors->Map[col];
						chr[pp]=(chr[pp]&~bit)|((col&1)<<k);
						chr[pp+8]=(chr[pp+8]&~bit)|((col>>1)<<k);
					}
					pp++;
				}
			}
		}

		if(FormSwapColors->RemapPalette)
		{
			for(i=0;i<4;i++)
			{
				for(j=0;j<4;j++) paltemp[FormSwapColors->Map[j]]=bgPal[i*4+j];
				for(j=0;j<4;j++) bgPal[i*4+j]=paltemp[j];
			}

			bgPal[1*4+0]=bgPal[0];
			bgPal[2*4+0]=bgPal[0];
			bgPal[3*4+0]=bgPal[0];
		}

		pal_validate();

		UpdateTiles(true);
		UpdateNameTable(-1,-1,true);
		DrawPalettes();
		UpdateMetaSprite();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MAddOffsetClick(TObject *Sender)
{
	int i;

	FormNameOffset->Mode=0;
	FormNameOffset->ShowModal();

	if(FormNameOffset->MakeOffset)
	{
		SetUndo();

		for(i=0;i<nameTableWidth*nameTableHeight;++i)
		{
			if(nameTable[i]>=FormNameOffset->From&&nameTable[i]<=FormNameOffset->To) nameTable[i]+=FormNameOffset->Offset;
		}

		UpdateNameTable(-1,-1,true);
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonMaskBClick(TObject *Sender)
{
	ppuMask^=0x80;
	UpdateRGBM();
	palette_calc();
	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	DrawPalettes();
	UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonMaskGClick(TObject *Sender)
{
	ppuMask^=0x40;
	UpdateRGBM();
	palette_calc();
	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	DrawPalettes();
	UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonMaskRClick(TObject *Sender)
{
	ppuMask^=0x20;
	UpdateRGBM();
	palette_calc();
	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	DrawPalettes();
	UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonMaskMClick(TObject *Sender)
{
	ppuMask^=0x01;
	UpdateRGBM();
	palette_calc();
	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	DrawPalettes();
	UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MExportNametableBMPClick(TObject *Sender)
{
	TPicture *picture;
	Graphics::TBitmap *bmp;
	int i,j,x,y;
	AnsiString str;

	str=RemoveExt(SaveDialogName->FileName);

	if(str=="") str="nametable";

	SaveDialogImage->FileName=str+".bmp";

	if(!SaveDialogImage->Execute()) return;

	picture=new TPicture();
	bmp=new Graphics::TBitmap();
	bmp->SetSize(nameTableWidth*8,nameTableHeight*8);
	bmp->PixelFormat=pf4bit;
	SetBMPPalette(bmp);
	picture->Bitmap=bmp;

	y=0;

	for(i=0;i<nameTableHeight;i++)
	{
		x=0;

		for(j=0;j<nameTableWidth;j++)
		{
			DrawExportTile16(picture,x,y,nameTable[i*nameTableWidth+j],AttrGet(j,i),j,i);

			x+=8;
		}

		y+=8;
	}

	picture->SaveToFile(SaveDialogImage->FileName);

	delete bmp;
	delete picture;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MExportTilesetBMPClick(TObject *Sender)
{
	TPicture *picture;
	Graphics::TBitmap *bmp;
	int i,x,y;
	AnsiString str;

	str=RemoveExt(SaveDialogChr->FileName);

	if(str=="") str="tileset";

	SaveDialogImage->FileName=str+".bmp";

	if(!SaveDialogImage->Execute()) return;

	picture=new TPicture();
	bmp=new Graphics::TBitmap();
	bmp->SetSize(128,128);
	bmp->PixelFormat=pf4bit;
	SetBMPPalette(bmp);
	picture->Bitmap=bmp;

	x=0;
	y=0;

	for(i=0;i<256;i++)
	{
		DrawExportTile16(picture,x,y,i,palActive,-1,-1);

		x+=8;
		
		if(x>=128)
		{
			x=0;
			y+=8;
		}
	}

	picture->SaveToFile(SaveDialogImage->FileName);

	delete bmp;
	delete picture;
}
//---------------------------------------------------------------------------



void __fastcall TFormMain::SpeedButtonCheckerClick(TObject *Sender)
{
	UpdateNameTable(-1,-1,true);
}
//---------------------------------------------------------------------------




void __fastcall TFormMain::MSaveAttributesClick(TObject *Sender)
{
	AnsiString name;

	SaveDialogAttr->FileName=RemoveExt(SaveDialogAttr->FileName);
	SaveDialogAttr->Title="Save attributes only";

	if(!SaveDialogAttr->Execute()) return;

	name=RemoveExt(SaveDialogAttr->FileName);

	switch(SaveDialogAttr->FilterIndex)
	{
	case 2:  name+=".bin"; break;
	case 3:  name+=".rle"; break;
	case 4:  name+=".h"; break;
	case 5: name+=".asm"; break;
	default: name+=".atr";
	}

	if(FileExists(name))
	{
		if(Application->MessageBox(("File "+name+" is already exist, overwrite?").c_str(),"Confirm",MB_YESNO)!=IDYES) return;
	}

	name=save_data(name.c_str(),attrTable,64,SaveDialogAttr->FilterIndex);

	if(name!="") Application->MessageBox(name.c_str(),"Error",MB_OK);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHRFillNumbersClick(TObject *Sender)
{
	int i,j,k,pp;

	if(Application->MessageBox("Do you really want to fill 8K CHR with numbers?","Confirm",MB_YESNO)==ID_YES)
	{
		SetUndo();

		pp=0;

		for(i=0;i<16;i++)
		{
			for(j=0;j<16;j++)
			{
				for(k=0;k<16;k++)
				{
					chr[pp++]=(smallnums_data[(i<<4)+k]&0xf0)|(smallnums_data[(j<<4)+k]&0x0f);
				}
			}
		}

		memcpy(&chr[4096],chr,4096);

		UpdateTiles(true);
		UpdateNameTable(-1,-1,true);
		UpdateMetaSprite();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHRFindDoublesClick(TObject *Sender)
{
	FindDoublesUnused(false);
}
//---------------------------------------------------------------------------



int           *import_name;
unsigned char *import_chr;
unsigned char *import_bmp;
int           *import_stat;

int import_chr_size;
int import_name_size;

int import_bmp_width;
int import_bmp_height;

int import_name_width;
int import_name_height;

int import_stat_size;



//compare two tiles by counting number of matching pixels

inline int tile_lossy_compare(int tile1,int tile2)
{
	int i,j,col1,col2,mask,match;

	tile1<<=4;
	tile2<<=4;

	match=0;

	for(i=0;i<8;++i)
	{
		col1=(import_chr[tile1]|(import_chr[tile1+8]<<8));
		col2=(import_chr[tile2]|(import_chr[tile2+8]<<8));

		mask=0x0101;

		for(j=0;j<8;++j)
		{
			if(!((col1^col2)&mask)) ++match;

			mask<<=1;
		}

		++tile1;
		++tile2;
	}

	return match;
}



int __fastcall TFormMain::ImportConvertTiles(int wdt,int hgt,int xoff,int yoff,int thresh)
{
	unsigned char tile[16];
	int i,j,k,l,x,y,c1,c2,col,pp,ptr,tile_count,pixel_count;
	bool add;

	memset(import_chr ,0,import_chr_size);
	memset(import_name,0,import_name_size);

	ptr=0;
	tile_count=0;

	for(i=0;i<hgt/8;++i)
	{
		for(j=0;j<wdt/8;++j)
		{
			memset(tile,0,sizeof(tile));

			pp=0;

			y=((i<<3)-yoff);

			pixel_count=0;

			for(k=0;k<8;++k)
			{
				x=((j<<3)-xoff);

				for(l=0;l<8;++l)
				{
					if(x<0) x+=wdt;
					if(x>wdt) x-=wdt;
					if(y<0) y+=hgt;
					if(y>hgt) y-=hgt;

					col=import_bmp[y*import_bmp_width+x];

					c1=(col&1)?(1<<(7-l)):0;
					c2=(col&2)?(1<<(7-l)):0;

					tile[pp+0]|=c1;
					tile[pp+8]|=c2;

					if(c1|c2) ++pixel_count;

					++x;
				}

				++pp;
				++y;
			}

			add=false;

			if(thresh<0||(!pixel_count||pixel_count>thresh)) add=true;

			if(add)
			{
				for(k=0;k<tile_count;++k)
				{
					if(memcmp(tile,&import_chr[k*16],16)==0)
					{
						import_name[i*import_name_width+j]=k;
						add=false;
						break;
					}
				}
			}

			if(add)
			{
				memcpy(&import_chr[ptr],tile,16);
				ptr+=16;
				import_name[i*import_name_width+j]=tile_count;

				++tile_count;
			}
		}
	}

	return tile_count;
}


struct matchStruct {
	unsigned char match;
	int tile1;
	int tile2;
};

bool __fastcall TFormMain::ImportBMP(AnsiString filename,bool shift,bool squeeze,int thresh)
{
	FILE *file;
	unsigned char *data;
	int i,j,k,l,x,y,n,size,wdt,hgt,pp,ps,off,ptr,bpp;
	int cnt,fi,min,max,id,rx,gx,bx,pitch;
	int tile_count,tiles_min,tile1,tile2,shift_x,shift_y,best_x,best_y;
	int attr[4];
	AnsiString str;

	file=fopen(filename.c_str(),"rb");

	if(!file) return false;

	size=get_file_size(file);
	data=(unsigned char*)malloc(size);
	fread(data,size,1,file);
	fclose(file);

	bpp=data[28];

	if(data[0]!='B'||data[1]!='M'||(bpp!=4&&bpp!=8)||data[29]!=0||read_dword(&data[30]))
	{
		Application->MessageBox("Wrong BMP format, only uncompressed 16 or 256 color images supported","Error",MB_OK);
		free(data);
		return false;
	}

	off=read_dword(&data[10]);
	wdt=read_dword(&data[18]);
	hgt=read_dword(&data[22]);

	if(wdt<32||wdt>8*NAME_MAX_WIDTH||labs(hgt)<32||labs(hgt)>8*NAME_MAX_HEIGHT)
	{
		Application->MessageBox(("BMP should not be smaller than 32x32 or larger than "+IntToStr(NAME_MAX_WIDTH*8)+"x"+IntToStr(NAME_MAX_HEIGHT)+" pixels").c_str(),"Error",MB_OK);
		free(data);
		return false;
	}

	//find similar colors in the NES palette

	for(i=0;i<16;i++)
	{
		min=0x01000000;
		id=0;

		for(j=0;j<64;j++)
		{
			if(j==0x1d||j==0x2d) continue;

			rx=((outPalette[j]>>16)&255)-data[i*4+54];
			gx=((outPalette[j]>>8 )&255)-data[i*4+55];
			bx=((outPalette[j]    )&255)-data[i*4+56];

			fi=30*rx*rx+59*gx*gx+11*bx*bx;

			if(fi<min)
			{
				min=fi;
				id=j;
			}
		}

		if(id==0x20) id=0x30;

		if(i!=4&&i!=8&&i!=12) bgPal[(i>>2)*4+(i&3)]=id;
	}

	bgPal[1*4+0]=bgPal[0];
	bgPal[2*4+0]=bgPal[0];
	bgPal[3*4+0]=bgPal[0];

	pal_validate();

	//put bitmap into an array to make it easier to work with

	import_bmp_width =wdt;
	import_bmp_height=hgt;

	import_bmp=(unsigned char*)malloc(import_bmp_width*import_bmp_height*sizeof(unsigned char));

	memset(import_bmp,0,import_bmp_width*import_bmp_height*sizeof(unsigned char));

	switch(bpp)
	{
	case 4:
		{
			pitch=wdt&~7;

			if(wdt&7) pitch+=8;

			pitch>>=1;

			for(i=0;i<hgt;++i)
			{
				ps=off+(hgt-1-i)*pitch;

				for(j=0;j<wdt;++j)
				{
					if(!(j&1))
					{
						import_bmp[i*import_bmp_width+j]=data[ps]>>4;
					}
					else
					{
						import_bmp[i*import_bmp_width+j]=data[ps++]&15;
					}
				}
			}
		}
		break;

	case 8:
		{
			pitch=wdt&~3;

			if(wdt&3) pitch+=4;

			for(i=0;i<hgt;++i)
			{
				ps=off+(hgt-1-i)*pitch;

				for(j=0;j<wdt;++j) import_bmp[i*import_bmp_width+j]=data[ps++];
			}
		}
		break;
	}

	free(data);

	//convert graphics into tiles

	if(wdt&7) wdt=(wdt&~7)+8;
	if(hgt&7) hgt=(hgt&~7)+8;

	import_name_width =wdt/8;
	import_name_height=hgt/8;

	import_name_size=import_name_width*import_name_height*sizeof(int);
	import_chr_size =import_bmp_width *import_bmp_height *sizeof(unsigned char);

	import_name=(int*)malloc(import_name_size);
	import_chr =(unsigned char*)malloc(import_chr_size);

	tiles_min=import_name_width*import_name_height;

	best_x=0;
	best_y=0;

	if(shift)
	{
		for(shift_y=0;shift_y<8;++shift_y)
		{
			for(shift_x=0;shift_x<8;++shift_x)
			{
				tile_count=ImportConvertTiles(wdt,hgt,shift_x,shift_y,thresh);

				if(tile_count<tiles_min)
				{
					tiles_min=tile_count;
					best_x=shift_x;
					best_y=shift_y;
				}
			}
		}
	}

	tile_count=ImportConvertTiles(wdt,hgt,best_x,best_y,thresh);

	if(!squeeze&&tile_count>256)
	{
		Application->MessageBox(("Too many unique tiles (256+"+IntToStr(tile_count-256)+"="+IntToStr(tile_count)+"), extra tiles will be skipped").c_str(),"Warning",MB_OK);
	}

	if(tile_count>256&&squeeze)
	{
		str=Caption;

		import_stat_size=tile_count*sizeof(int);
		import_stat=(int*)malloc(import_stat_size);

		while(tile_count>256)
		{
			Caption="Wait... ["+IntToStr(tile_count)+" tiles]";

			//count tile use

			memset(import_stat,0,import_stat_size);

			for(i=0;i<import_name_width*import_name_height;++i) ++import_stat[import_name[i]];

			//find a pair of tiles with max similarity

			max=0;
			tile1=0;
			tile2=0;

			for(i=0;i<tile_count-1;++i)
			{
				for(j=i+1;j<tile_count;++j)
				{
					n=tile_lossy_compare(i,j)*65536/import_stat[j];

					if(n>max)
					{
						max=n;
						tile1=i;
						tile2=j;
					}
				}
			}

			//remove one of tiles

			for(i=0;i<import_name_width*import_name_height;++i)
			{
				if(import_name[i]==tile2) import_name[i]=tile1; else if(import_name[i]>tile2) --import_name[i];
			}

			memcpy(&import_chr[tile2*16],&import_chr[tile2*16+16],(tile_count-tile2-1)*16);

			--tile_count;
		}

		free(import_stat);

		Caption=str;
	}

	memcpy(chr+bankActive,import_chr,4096);

	free(import_chr);

	for(i=0;i<import_name_width*import_name_height;++i)
	{
		n=import_name[i];

		if(n>255) n=255;

		nameTable[i]=n;
	}

	free(import_name);

	//try to set proper attributes

	nameTableWidth =import_name_width;
	nameTableHeight=import_name_height;
	
	for(i=0;i<import_bmp_height;i+=16)
	{
		for(j=0;j<import_bmp_width;j+=16)
		{
			for(k=0;k<4;++k) attr[k]=0;

			for(k=i;k<i+16;++k)
			{
				for(l=j;l<j+16;++l)
				{
					if(import_bmp[k*import_bmp_width+l]&3) ++attr[import_bmp[k*import_bmp_width+l]>>2];
				}
			}

			id=0;
			max=0;

			for(k=0;k<4;++k)
			{
				if(attr[k]>max)
				{
					max=attr[k];
					id=k;
				}
			}

			AttrSet(j>>3,i>>3,id);
		}
	}

	free(import_bmp);

	return true;
}



void __fastcall TFormMain::MImportBMPNametableClick(TObject *Sender)
{
	OpenDialogImport->Title="Import BMP file as nametable";
	OpenDialogImport->Filter="Windows bitmap files (*.bmp)|*.bmp|All files (*.*)|*.*";
	OpenDialogImport->DefaultExt="bmp";

	if(OpenDialogImport->Execute())
	{
		SetUndo();

		if(!ImportBMP(OpenDialogImport->FileName,MImportBestOffsets->Checked,MImportLossy->Checked,MImportThreshold->Checked?8:-1)) return;

		UpdateAll();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MImportNESClick(TObject *Sender)
{
	FILE *file;
	unsigned char *data;

	OpenDialogImport->Title="Import NES file";
	OpenDialogImport->Filter="NES cartridge dump|*.nes|All files (*.*)|*.*";
	OpenDialogImport->DefaultExt="nes";

	if(OpenDialogImport->Execute())
	{
		FormBank->Caption="Import 8K CHR bank from NES file";
		FormBank->FileName=OpenDialogImport->FileName;
		FormBank->ShowModal();

		if(FormBank->OK)
		{
			file=fopen(OpenDialogImport->FileName.c_str(),"rb");

			if(file)
			{
				fseek(file,0,SEEK_END);
				fseek(file,16+FormBank->PRG*16384+FormBank->Bank*8192,SEEK_SET);
				fread(chr,8192,1,file);
				fclose(file);
			}

			UpdateTiles(true);
			UpdateNameTable(-1,-1,true);
			UpdateMetaSprite();
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MExportNESClick(TObject *Sender)
{
	FILE *file;
	unsigned char *nes;
	int size;

	if(!SaveDialogExportNES->Execute()) return;

	FormBank->Caption="Export 8K CHR bank to NES file";
	FormBank->FileName=SaveDialogExportNES->FileName;
	FormBank->ShowModal();

	if(FormBank->OK)
	{
		file=fopen(SaveDialogExportNES->FileName.c_str(),"rb");

		if(!file) return;

		fseek(file,0,SEEK_END);
		size=ftell(file);
		fseek(file,0,SEEK_SET);

		nes=(unsigned char*)malloc(size);
		fread(nes,size,1,file);
		fclose(file);

		file=fopen((SaveDialogExportNES->FileName+".bak").c_str(),"wb");

		if(!file)
		{
			free(nes);
			return;
		}

		fwrite(nes,size,1,file);
		fclose(file);

		memcpy(nes+16+FormBank->PRG*16384+FormBank->Bank*8192,chr,8192);

		file=fopen(SaveDialogExportNES->FileName.c_str(),"wb");

		if(!file)
		{
			free(nes);
			return;
		}

		fwrite(nes,size,1,file);
		fclose(file);
		free(nes);
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MImportBMPTilesetClick(TObject *Sender)
{
	FILE *file;
	unsigned char *data;
	int i,j,k,l,size,wdt,hgt,pp,ps,off,ptr,bpp;
	unsigned char tile[16];

	OpenDialogImport->Title="Import BMP file as tileset";
	OpenDialogImport->Filter="Windows bitmap files (*.bmp)|*.bmp|All files (*.*)|*.*";
	OpenDialogImport->DefaultExt="bmp";

	if(!OpenDialogImport->Execute()) return;

	file=fopen(OpenDialogImport->FileName.c_str(),"rb");

	if(file)
	{
		size=get_file_size(file);
		data=(unsigned char*)malloc(size);
		fread(data,size,1,file);
		fclose(file);

		bpp=data[28];

		if(data[0]!='B'||data[1]!='M'||(bpp!=4&&bpp!=8)||data[29]!=0||read_dword(&data[30]))
		{
			Application->MessageBox("Wrong BMP format, only 4bpp and 8bpp files without compression are supported","Error",MB_OK);
			free(data);
			return;
		}

		off=read_dword(&data[10]);
		wdt=read_dword(&data[18]);
		hgt=read_dword(&data[22]);

		if(wdt>128||(hgt>128&&hgt<-128)||(wdt&7)||(hgt&7))
		{
			Application->MessageBox("BMP should be 128x128 pixels or smaller, with dimensions being a multiply of 8","Error",MB_OK);
			free(data);
			return;
		}

		ptr=bankActive;

		if(!(wdt==128&&hgt==128)) ptr+=tileActive*16;
		
		for(i=0;i<hgt/8;i++)
		{
			for(j=0;j<wdt/8;j++)
			{
				memset(tile,0,sizeof(tile));
				pp=0;

				if(bpp==4)
				{
					for(k=0;k<8;k++)
					{
						/*if(hgt>0) ps=off+(i*8+k)*128+j*4; else */ps=off+(hgt-1-i*8-k)*(wdt/2)+j*4;
						for(l=0;l<8;l++)
						{
							tile[pp]|=(data[ps]&(l&1?0x01:0x10))?1<<(7-l):0;
							tile[pp+8]|=(data[ps]&(l&1?0x02:0x20))?1<<(7-l):0;

							if(l&1) ps++;
						}

						pp++;
					}
				}
				else
				{
					for(k=0;k<8;k++)
					{
						/*if(hgt>0) ps=off+(i*8+k)*128+j*4; else */ps=off+(hgt-1-i*8-k)*wdt+j*8;
						for(l=0;l<8;l++)
						{
							tile[pp]|=(data[ps]&1)?1<<(7-l):0;
							tile[pp+8]|=(data[ps]&2)?1<<(7-l):0;

							ps++;
						}

						pp++;
					}
				}

				memcpy(&chr[ptr+j*16],tile,16);
			}

			ptr+=16*16;
		}

		free(data);
	}

	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHRSwapBanksClick(TObject *Sender)
{
	unsigned char temp[4096];

	memcpy(temp,chr,4096);
	memcpy(chr,chr+4096,4096);
	memcpy(chr+4096,temp,4096);

	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MOpenAllClick(TObject *Sender)
{
	if(OpenDialogAll->Execute()) OpenAll(RemoveExt(OpenDialogAll->FileName));
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MSaveAllClick(TObject *Sender)
{
	SaveCHR(8192);

	if(SaveDialogName->FileName=="") SaveDialogName->FileName=SaveDialogChr->FileName;
	if(SaveDialogMap ->FileName=="") SaveDialogMap ->FileName=SaveDialogChr->FileName;

	if(nameTableWidth==32&&nameTableHeight==30)
	{
		MSaveNameTableAttributesClick(this);
	}
	else
	{
		MSaveMapClick(this);
	}

	if(SaveDialogPal->FileName=="") SaveDialogPal->FileName=SaveDialogName->FileName;

	MSavePalettesClick(this);

	MSaveMetaSpriteBankClick(this);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MLoadSessionClick(TObject *Sender)
{
	if(!OpenDialogSession->Execute()) return;

	if(!LoadSession(OpenDialogSession->FileName)) Application->MessageBox("Can't load session","Error",MB_OK);

	UpdateAll();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MSaveSessionClick(TObject *Sender)
{
	if(!SaveDialogSession->Execute()) return;

	SaveSession(SaveDialogSession->FileName);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MExportPaletteBMPClick(TObject *Sender)
{
	TPicture *picture;
	Graphics::TBitmap *bmp;
	int i;
	unsigned char* dst;
	AnsiString str;

	str=RemoveExt(SaveDialogPal->FileName);

	if(str=="") str="palette";

	SaveDialogImage->FileName=str+".bmp";

	if(!SaveDialogImage->Execute()) return;

	picture=new TPicture();
	bmp=new Graphics::TBitmap();
	bmp->SetSize(16,1);
	bmp->PixelFormat=pf4bit;
	SetBMPPalette(bmp);
	picture->Bitmap=bmp;

	dst=(unsigned char*)picture->Bitmap->ScanLine[0];

	for(i=0;i<16;i+=2) *dst++=((i+1)|(i<<4));

	picture->SaveToFile(SaveDialogImage->FileName);

	delete bmp;
	delete picture;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonTypeInClick(TObject *Sender)
{
	if(SpeedButtonTypeIn->Down)
	{
		if(nameSelection.left>=0)
		{
			nameSelection.right =nameSelection.left+1;
			nameSelection.bottom=nameSelection.top +1;

			UpdateNameTable(-1,-1,true);
		}
	}

	StaticTextFontOffset->Caption="$"+IntToHex(typeInFontOffset,2);
	StaticTextFontOffset->Visible=SpeedButtonTypeIn->Down;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::FormKeyPress(TObject *Sender, char &Key)
{
	if(SpeedButtonTypeIn->Down)
	{
		if(Key>=32)
		{
			NameTableTypeIn(Key-32);
		}
	}
}
//---------------------------------------------------------------------------


void __fastcall TFormMain::PageControlEditorChange(TObject *Sender)
{
	UpdateMetaSprite();
	ActiveControl=NULL;//to prevent ListBoxSpriteList grab focus while Nametable tab is active
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ImageMetaSpriteDragOver(TObject *Sender,
TObject *Source, int X, int Y, TDragState State, bool &Accept)
{
	if(Source->InheritsFrom(__classid(TImage))) Accept=true; else Accept=false;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ImageMetaSpriteDragDrop(TObject *Sender,
TObject *Source, int X, int Y)
{
	int i,pp,tile;

	pp=metaSpriteActive*64*4;

	for(i=0;i<64;++i)
	{
		if(metaSprites[pp]==255)
		{
			tile=tileActive;

			if(SpeedButtonSprite8x16->Down) tile=(tileActive&0xfe)+(bankActive?1:0);

			metaSprites[pp+0]=Y/uiScale;
			metaSprites[pp+1]=tile;
			metaSprites[pp+2]=palActive;
			metaSprites[pp+3]=X/uiScale;

			if(SpeedButtonSpriteSnap->Down) SpriteSnap(i);

			SelectSprite(i,false);

			break;
		}

		pp+=4;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ImageMetaSpriteEndDrag(TObject *Sender,
TObject *Target, int X, int Y)
{
	if(Target) UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonPrevMetaSpriteClick(TObject *Sender)
{
	if(metaSpriteActive)
	{
		--metaSpriteActive;

		UpdateMetaSprite();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonNextMetaSpriteClick(TObject *Sender)
{
	if(metaSpriteActive<255)
	{
		++metaSpriteActive;

		UpdateMetaSprite();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonClearMetaSpriteClick(TObject *Sender)
{
	int i,off;

	if(Application->MessageBox("Metasprite will be cleared out!","Are you sure?",MB_YESNO)==IDYES)
	{
		off=metaSpriteActive*64*4;

		for(i=0;i<64*4;i+=4) metaSprites[off+i]=255;

		UpdateMetaSprite();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonSpriteDelClick(TObject *Sender)
{
	int i,j,off,cnt;

	if(Application->MessageBox("Sprite(s) will be deleted!","Are you sure?",MB_YESNO)==IDYES)
	{
		cnt=0;

		for(i=0;i<ListBoxSpriteList->Items->Count;++i)
		{
			if(ListBoxSpriteList->Selected[i])
			{
				off=metaSpriteActive*64*4+i*4;

				for(j=0;j<4;++j) metaSprites[off+j]=255;

				++cnt;
			}
		}

		for(i=0;i<cnt;++i) squeeze_sprites();

		for(i=0;i<ListBoxSpriteList->Items->Count;++i) ListBoxSpriteList->Selected[i]=false;

		UpdateMetaSprite();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ListBoxSpriteListClick(TObject *Sender)
{
	SelectSprite(ListBoxSpriteList->ItemIndex,true);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonFrameSelectedClick(TObject *Sender)
{
	UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonSpriteUpClick(TObject *Sender)
{
	unsigned char temp[4];
	int off;

	off=metaSpriteActive*64*4+spriteActive*4;

	if(spriteActive>0)
	{
		if(metaSprites[off]<255)
		{
			memcpy( temp              ,&metaSprites[off-4],4);
			memcpy(&metaSprites[off-4],&metaSprites[off  ],4);
			memcpy(&metaSprites[off  ], temp              ,4);

			--spriteActive;

			UpdateMetaSprite();
		}
	}

	SelectSprite(spriteActive,false);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonSpriteDownClick(TObject *Sender)
{
	unsigned char temp[4];
	int off;

	off=metaSpriteActive*64*4+spriteActive*4;

	if(spriteActive<63)
	{
		if(metaSprites[off+4]<255)
		{
			memcpy( temp              ,&metaSprites[off  ],4);
			memcpy(&metaSprites[off  ],&metaSprites[off+4],4);
			memcpy(&metaSprites[off+4], temp              ,4);

			++spriteActive;

			UpdateMetaSprite();
		}
	}

	SelectSprite(spriteActive,false);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ListBoxSpriteListKeyDown(TObject *Sender, WORD &Key,
TShiftState Shift)
{
	FormKeyDown(Sender,Key,Shift);
	Key=0;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ImageMetaSpriteMouseDown(TObject *Sender,
TMouseButton Button, TShiftState Shift, int X, int Y)
{
	int i,off;

	if(Shift.Contains(ssLeft)&&!Shift.Contains(ssCtrl))
	{
		i=GetSpriteID(X,Y);

		if(i>=0)
		{
			SelectSprite(i,false);
		}
	}

	if(Shift.Contains(ssRight))
	{
		i=GetSpriteID(X,Y);

		if(i>=0)
		{
			SelectSprite(i,false);
		}

		off=metaSpriteActive*64*4+spriteActive*4;

		if(metaSprites[off]<255)
		{
			spriteDragX=metaSprites[off+3]-X/uiScale;
			spriteDragY=metaSprites[off+0]-Y/uiScale;
		}

		spriteDrag=i;
	}

	ImageMetaSpriteMouseMove(Sender,Shift,X,Y);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ImageMetaSpriteMouseLeave(TObject *Sender)
{
	spriteHover=-1;
	spriteDrag=-1;
	UpdateStats();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ImageMetaSpriteMouseMove(TObject *Sender,
TShiftState Shift, int X, int Y)
{
	int x,y,off;

	if(Shift.Contains(ssRight)&&spriteDrag>=0)
	{
		off=metaSpriteActive*64*4+spriteDrag*4;

		if(metaSprites[off]<255)
		{
			x=X/uiScale+spriteDragX;
			y=Y/uiScale+spriteDragY;

			if(x<0)   x=0;
			if(x>120) x=120;
			if(y<0)   y=0;
			if(y>120) y=120;

			metaSprites[off+3]=x;
			metaSprites[off+0]=y;

			UpdateMetaSprite();
		}
	}

	if(Shift.Contains(ssLeft)&&Shift.Contains(ssCtrl))
	{
		spriteGridX=(X/uiScale)&~7;
		spriteGridY=(Y/uiScale)&~7;

		if(spriteGridX<8) spriteGridX=8;
		if(spriteGridY<8) spriteGridY=8;

		UpdateMetaSprite();
	}

	spriteHover=GetSpriteID(X,Y);

	UpdateStats();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ImageMetaSpriteMouseUp(TObject *Sender,
TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if(SpeedButtonSpriteSnap->Down)
	{
		SpriteSnap(spriteActive);

		UpdateMetaSprite();
	}

	spriteDrag=-1;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MOpenMetaSpriteBankClick(TObject *Sender)
{
	if(!OpenDialogMetaSpriteBank->Execute()) return;

	if(OpenMetaSprites(OpenDialogMetaSpriteBank->FileName))
	{
		OpenDialogMetaSpriteBank->FileName=RemoveExt(OpenDialogMetaSpriteBank->FileName);

		SaveDialogMetaSpriteBank->FileName=OpenDialogMetaSpriteBank->FileName;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MSaveMetaSpriteBankClick(TObject *Sender)
{
	FILE *file;
	unsigned char data[2];
	AnsiString name;

	if(!SaveDialogMetaSpriteBank->Execute()) return;

	name=RemoveExt(SaveDialogMetaSpriteBank->FileName)+".msb";

	if(!OverwritePrompt(name)) return;

	file=fopen(name.c_str(),"wb");

	if(file)
	{
		data[0]=spriteGridX;
		data[1]=spriteGridY;

		fwrite(data,2,1,file);
		fwrite(metaSprites,256*64*4,1,file);
		fclose(file);
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MMetaSpritePutBankToClipboardCClick(TObject *Sender)
{
	char str[65536],buf[1024];
	int i,j,pp,off,size;
	bool first;

	str[0]=0;

	strcat(str,"const unsigned char metasprite_data[]={\n");

	first=true;

	for(i=0;i<256;++i)
	{
		off=i*64*4;

		if(metaSprites[off]<255)
		{
			if(!first) strcat(str,",\n");

			first=false;

			sprintf(buf,"\n",i);
			strcat(str,buf);

			for(j=0;j<64;++j)
			{
				if(metaSprites[off]==255) continue;

				if(j) strcat(str,",\n");

				sprintf(buf,"\t%s,%s,0x%2.2x,%i",num_to_3char(metaSprites[off+3]-spriteGridX),num_to_3char(metaSprites[off+0]-spriteGridY),metaSprites[off+1],metaSprites[off+2]&3);

				strcat(str,buf);

				if(metaSprites[off+2]&OAM_FLIP_H) strcat(str,"|OAM_FLIP_H");
				if(metaSprites[off+2]&OAM_FLIP_V) strcat(str,"|OAM_FLIP_V");

				off+=4;
			}

			strcat(str,",\n\t128");
		}
	}

	strcat(str,"\n\n};\n\n");

	strcat(str,"const unsigned char* const metasprite_list[]={\n");

	pp=0;

	for(i=0;i<256;++i)
	{
		off=i*64*4;

		if(metaSprites[off]<255)
		{
			if(pp) strcat(str,",\n");

			sprintf(buf,"\tmetasprite_data+%i",pp);
			strcat(str,buf);

			size=1;

			for(j=0;j<64;++j)
			{
				if(metaSprites[off]==255) break;

				off+=4;
				size+=4;
			}

			pp+=size;
		}
	}

	strcat(str,"\n};\n\n");

	Clipboard()->SetTextBuf(str);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MMetaSpritePutToClipboardCClick(TObject *Sender)
{
	char str[65536],buf[1024];
	int i,j,x,y,off;

	off=metaSpriteActive*64*4;

	if(metaSprites[off]<255)
	{
		sprintf(str,"const unsigned char metasprite[]={\n");

		for(i=0;i<64;++i)
		{
			if(i) strcat(str,",\n");

			if(metaSprites[off]==255) break;

			sprintf(buf,"\t%s,%s,0x%2.2x,%i",num_to_3char(metaSprites[off+3]-spriteGridX),num_to_3char(metaSprites[off]-spriteGridY),metaSprites[off+1],metaSprites[off+2]&3);

			strcat(str,buf);

			if(metaSprites[off+2]&OAM_FLIP_H) strcat(str,"|OAM_FLIP_H");
			if(metaSprites[off+2]&OAM_FLIP_V) strcat(str,"|OAM_FLIP_V");

			off+=4;
		}

		strcat(str,"\t128\n};\n\n");

		Clipboard()->SetTextBuf(str);
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonSpriteHFlipClick(TObject *Sender)
{
	int i,off;

	for(i=0;i<ListBoxSpriteList->Items->Count;++i)
	{
		if(ListBoxSpriteList->Selected[i])
		{
			off=metaSpriteActive*64*4+i*4;

			if(metaSprites[off]<255) metaSprites[off+2]^=OAM_FLIP_H;
		}
	}

	UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonSpriteVFlipClick(TObject *Sender)
{
	int i,off;

	for(i=0;i<ListBoxSpriteList->Items->Count;++i)
	{
		if(ListBoxSpriteList->Selected[i])
		{
			off=metaSpriteActive*64*4+i*4;

			if(metaSprites[off]<255) metaSprites[off+2]^=OAM_FLIP_V;
		}
	}

	UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonMetaSpriteCopyClick(TObject *Sender)
{
	memcpy(metaSpriteCopy,&metaSprites[metaSpriteActive*64*4],sizeof(metaSpriteCopy));
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonMetaSpritePasteClick(TObject *Sender)
{
	if(metaSpriteCopy[0]<255)
	{
		memcpy(&metaSprites[metaSpriteActive*64*4],metaSpriteCopy,sizeof(metaSpriteCopy));

		UpdateMetaSprite();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonMetaSpriteHFlipClick(TObject *Sender)
{
	int i,x,off;

	off=metaSpriteActive*64*4;

	for(i=0;i<64;++i)
	{
		if(metaSprites[off]==255) break;

		x=-8-(metaSprites[off+3]-spriteGridX);

		metaSprites[off+3]=spriteGridX+x;
		metaSprites[off+2]^=OAM_FLIP_H;

		off+=4;
	}

	UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonMetaSpriteVFlipClick(TObject *Sender)
{
	int i,y,off;

	off=metaSpriteActive*64*4;

	for(i=0;i<64;++i)
	{
		if(metaSprites[off]==255) break;

		y=-8-(metaSprites[off+0]-spriteGridY);

		metaSprites[off+0]=spriteGridY+y;
		metaSprites[off+2]^=OAM_FLIP_V;

		off+=4;
	}

	UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHRFindUnusedClick(TObject *Sender)
{
	FindDoublesUnused(true);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHRRemoveUnusedClick(TObject *Sender)
{
	RemoveDoublesUnused(true);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonSprite8x16Click(TObject *Sender)
{
	SpeedButtonSpriteBank->Enabled=SpeedButtonSprite8x16->Down;

	UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonSpriteBankClick(TObject *Sender)
{
	int i,off;

	for(i=0;i<ListBoxSpriteList->Items->Count;++i)
	{
		if(ListBoxSpriteList->Selected[i])
		{
			off=metaSpriteActive*64*4+i*4;

			if(metaSprites[off]<255) metaSprites[off+1]^=1;
		}
	}

	UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonSpriteGridClick(TObject *Sender)
{
	UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MMetaSpritePutToClipboardAsmClick(TObject *Sender)
{
	char str[65536],buf[1024];
	int i,j,x,y,off;

	off=metaSpriteActive*64*4;

	if(metaSprites[off]<255)
	{
		sprintf(str,"metasprite:\n");

		for(i=0;i<64;++i)
		{
			strcat(str,"\n");

			if(metaSprites[off]==255) break;

			sprintf(buf,"\t.byte %s,%s,$%2.2x,%i",num_to_3char(metaSprites[off+3]-spriteGridX),num_to_3char(metaSprites[off]-spriteGridY),metaSprites[off+1],metaSprites[off+2]&3);

			strcat(str,buf);

			if(metaSprites[off+2]&OAM_FLIP_H) strcat(str,"|OAM_FLIP_H");
			if(metaSprites[off+2]&OAM_FLIP_V) strcat(str,"|OAM_FLIP_V");

			off+=4;
		}

		strcat(str,"\n\n");

		Clipboard()->SetTextBuf(str);
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MMetaSpritePutBankToClipboardAsmClick(
TObject *Sender)
{
	char str[65536],buf[1024];
	int i,j,pp,off,size;
	bool first;

	str[0]=0;

	strcat(str,"metasprite_data:\n");

	first=true;

	for(i=0;i<256;++i)
	{
		off=i*64*4;

		if(metaSprites[off]<255)
		{
			if(!first) strcat(str,"\n");

			first=false;

			sprintf(buf,"\n",i);
			strcat(str,buf);

			for(j=0;j<64;++j)
			{
				if(metaSprites[off]==255) continue;

				if(j) strcat(str,"\n");

				sprintf(buf,"\t.byte %s,%s,$%2.2x,%i",num_to_3char(metaSprites[off+3]-spriteGridX),num_to_3char(metaSprites[off+0]-spriteGridY),metaSprites[off+1],metaSprites[off+2]&3);

				strcat(str,buf);

				if(metaSprites[off+2]&OAM_FLIP_H) strcat(str,"|OAM_FLIP_H");
				if(metaSprites[off+2]&OAM_FLIP_V) strcat(str,"|OAM_FLIP_V");

				off+=4;
			}

			strcat(str,"\n\t.byte 128");
		}
	}

	strcat(str,"\n\n");

	strcat(str,"metasprite_pointers:\n");

	pp=0;

	for(i=0;i<256;++i)
	{
		off=i*64*4;

		if(metaSprites[off]<255)
		{
			strcat(str,"\n");

			sprintf(buf,"\t.word metasprite_data+%i",pp);
			strcat(str,buf);

			size=1;

			for(j=0;j<64;++j)
			{
				if(metaSprites[off]==255) break;

				off+=4;
				size+=4;
			}

			pp+=size;
		}
	}

	strcat(str,"\n\n");

	Clipboard()->SetTextBuf(str);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::StaticTextFontOffsetMouseDown(TObject *Sender,
TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if(Shift.Contains(ssLeft)) prevMouseY=Y;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::StaticTextFontOffsetMouseMove(TObject *Sender,
TShiftState Shift, int X, int Y)
{
	int delta;

	if(Shift.Contains(ssLeft))
	{
		delta=prevMouseY-Y;

		typeInFontOffset+=16*(delta/2);

		if(typeInFontOffset<0x00) typeInFontOffset=0x00;
		if(typeInFontOffset>0xf0) typeInFontOffset=0xf0;

		StaticTextFontOffset->Caption="$"+IntToHex(typeInFontOffset,2);

		prevMouseY=Y;
	}
}
//---------------------------------------------------------------------------


void __fastcall TFormMain::ImageTilesDragOver(TObject *Sender, TObject *Source,
int X, int Y, TDragState State, bool &Accept)
{
	Accept=false;

	if(X>=0&&X<(128*uiScale)&&Y>=0&&Y<(128*uiScale))
	{
		tileXC=X/(8*uiScale);
		tileYC=Y/(8*uiScale);
	}

	UpdateStats();

	if(PageControlEditor->ActivePage==TabSheetName&&Source==ImageTiles) Accept=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ImageTilesDragDrop(TObject *Sender, TObject *Source,
int X, int Y)
{
	int tile,ps,pd;
	unsigned char tempchr[16];

	X/=(8*uiScale);
	Y/=(8*uiScale);

	if(X<0||X>=(8*uiScale)||Y<0||Y>=(8*uiScale)) return;

	tile=Y*16+X;

	ps=tileActive*16+bankActive;
	pd=tile*16+bankActive;

	memcpy(tempchr,&chr[pd],16);
	memcpy(&chr[pd],&chr[ps],16);
	memcpy(&chr[ps],tempchr,16);

	for(ps=0;ps<nameTableWidth*nameTableHeight;++ps)
	{
		if(nameTable[ps]==tileActive) nameTable[ps]=tile; else if(nameTable[ps]==tile) nameTable[ps]=tileActive;
	}

	SetTile(tile);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHRSave1KClick(TObject *Sender)
{
	SaveCHR(1024);	
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHRSave2KClick(TObject *Sender)
{
	SaveCHR(2048);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHRSave4KClick(TObject *Sender)
{
	SaveCHR(4096);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHRSave8KClick(TObject *Sender)
{
	SaveCHR(8192);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHRSaveSelectionClick(TObject *Sender)
{
	FILE *file;
	int i,size;
	AnsiString name;

	if(!SaveDialogChr->Execute()) return;

	name=RemoveExt(SaveDialogChr->FileName)+".chr";

	size=0;

	for(i=0;i<256;++i) if(chrSelected[i]) size+=16;

	file=fopen(name.c_str(),"rb");

	if(file)
	{
		fseek(file,0,SEEK_END);
		i=ftell(file);
		fclose(file);

		if(Application->MessageBox((size==i?"Overwrite?":"Previous file has different size! Overwrite?"),"Confirm",MB_YESNO)!=IDYES) return;
	}

	file=fopen(name.c_str(),"wb");

	if(file)
	{
		for(i=0;i<256;i++)
		{
			if(chrSelected[i]) fwrite(chr+i*16+bankActive,16,1,file);
		}

		fclose(file);
	}

	SetLatestCHRSize(size);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MPutSelectedBlockToClipboardCClick(TObject *Sender)
{
	CopyMapCodeC(false);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MPutSelectedBlockToClipboardASMClick(TObject *Sender)
{
	CopyMapCodeASM();	
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MPutMetaSpriteToClipboardCNoFlipClick(TObject *Sender)
{
	CopyMetaSpriteCodeC(false);	
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MPutMetaSpriteToClipboardCHFlipClick(TObject *Sender)
{
	CopyMetaSpriteCodeC(true);	
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHRSaveLatestClick(TObject *Sender)
{
	switch(CHRLatestSize)
	{
	case 1024: SaveCHR(1024); break;
	case 2048: SaveCHR(2048); break;
	case 4096: SaveCHR(4096); break;
	case 8192: SaveCHR(8192); break;
	default: MCHRSaveSelectionClick(Sender);
	}	
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MPutPaletteToClipboardAssemblyClick(TObject *Sender)
{
	char str[1024],buf[1024];
	int i,j;

	pal_validate();

	sprintf(str,"\tlda #$3f\n\tsta $2006\n\tlda #$00\n\tsta $2006\n\tldx #$%2.2x\n\tstx $2007\n",bgPal[0]);

	for(i=0;i<4;i++)
	{
		if(i) strcat(str,"\tstx $2007\n");
		for(j=1;j<4;j++)
		{
			sprintf(buf,"\tlda #$%2.2x\n\tsta $2007\n",bgPal[i*4+j]);
			strcat(str,buf);
		}
	}

	Clipboard()->SetTextBuf(str);	
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MPutPaletteToClipboardASMClick(TObject *Sender)
{
	char str[1024],buf[1024];
	int i,j;

	strcpy(str,".db ");

	for(i=0;i<4;i++)
	{
		for(j=0;j<4;j++)
		{
			sprintf(buf,"$%2.2x%c",bgPal[i*4+j],i*4+j<15?',':'\n');
			strcat(str,buf);
		}
	}

	Clipboard()->SetTextBuf(str);	
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MPutPaletteToClipboardCClick(TObject *Sender)
{
	char str[1024],buf[1024];
	int i,j;

	strcpy(str,"const unsigned char palette[16]={ ");

	for(i=0;i<4;i++)
	{
		for(j=0;j<4;j++)
		{
			sprintf(buf,"0x%2.2x%c",bgPal[i*4+j],i*4+j<15?',':' ');
			strcat(str,buf);
		}
	}

	strcat(str,"};\n\n");

	Clipboard()->SetTextBuf(str);	
}
//---------------------------------------------------------------------------




void __fastcall TFormMain::FormMouseWheel(TObject *Sender, TShiftState Shift,
int WheelDelta, TPoint &MousePos, bool &Handled)
{
	if(WheelDelta<0)
	{
		palActive=(palActive-1)&3;
	}
	else
	{
		palActive=(palActive+1)&3;
	}

	pal_validate();

	DrawPalettes();
	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	UpdateMetaSprite();

	Handled=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MPutSelectedBlockToClipboardCRLEClick(
TObject *Sender)
{
	CopyMapCodeC(true);
}
//---------------------------------------------------------------------------


void __fastcall TFormMain::MPutSelectionToMetaspriteClick(TObject *Sender)
{
	int i,j,x,y,w,h,sx,sy,tx,ty,ptr,spr;

	if(nameSelection.left>=0&&nameSelection.top>=0)
	{
		GetSelection(nameSelection,x,y,w,h);

		spr=0;
		ptr=metaSpriteActive*64*4;

		sy=64-(h*8/2);

		for(i=0;i<h;i++)
		{
			sx=64-(w*8/2);

			for(j=0;j<w;j++)
			{
				tx=x+j;
				ty=y+i;

				if(spr<64)
				{
					metaSprites[ptr+0]=sy;
					metaSprites[ptr+1]=nameTable[ty*nameTableWidth+tx];
					metaSprites[ptr+2]=AttrGet(tx,ty);
					metaSprites[ptr+3]=sx;

					ptr+=4;

					++spr;
				}

				sx+=8;
			}

			sy+=8;
		}

		if(MPutSelectionToMetaspriteAutoInc->Checked) if(metaSpriteActive<255) ++metaSpriteActive;
		
		UpdateMetaSprite();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonSpriteAllClick(TObject *Sender)
{
	int i;

	for(i=0;i<ListBoxSpriteList->Items->Count;++i) ListBoxSpriteList->Selected[i]=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonSpriteNoneClick(TObject *Sender)
{
	int i;

	for(i=0;i<ListBoxSpriteList->Items->Count;++i) ListBoxSpriteList->Selected[i]=false;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::PaintBoxPalDragOver(TObject *Sender, TObject *Source,
int X, int Y, TDragState State, bool &Accept)
{
	UpdateStats();

	if(Source==PaintBoxPal) Accept=true; else Accept=false;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::PaintBoxPalDragDrop(TObject *Sender, TObject *Source,
int X, int Y)
{
	int n;

	n=check_palette_mouse(X,Y);

	if(n>=0&&!(n&0x100))
	{
		SetUndo();
		
		bgPal[(n&3)*4+(n>>4)]=palDragColor;

		pal_validate();

		DrawPalettes();
		UpdateTiles(true);
		UpdateNameTable(-1,-1,true);
		UpdateMetaSprite();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MView2xClick(TObject *Sender)
{
	if(Sender==MView2x) uiScale=2;
	if(Sender==MView3x) uiScale=3;
	if(Sender==MView4x) uiScale=4;

	UpdateUIScale();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MReplaceTileClick(TObject *Sender)
{
	int i;

	FormNameOffset->Mode=1;
	FormNameOffset->ShowModal();

	if(FormNameOffset->MakeOffset)
	{
		SetUndo();

		for(i=0;i<nameTableWidth*nameTableHeight;++i)
		{
			if(nameTable[i]==FormNameOffset->From) nameTable[i]=FormNameOffset->Offset;
		}

		UpdateNameTable(-1,-1,true);
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MImportBestOffsetsClick(TObject *Sender)
{
	((TMenuItem*)Sender)->Checked^=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHRFreqSortClick(TObject *Sender)
{
	int i,j,temp;
	static int sort[256],freq[256];
	static unsigned char chrt[256*16];

	SetUndo();

	memset(freq,0,sizeof(freq));

	for(i=0;i<nameTableHeight*nameTableWidth;++i) ++freq[nameTable[i]];

	for(i=0;i<256;++i) sort[i]=i;

	for(i=0;i<256-1;++i)
	{
		for(j=0;j<256-i-1;++j)
		{
			if(freq[j]<freq[j+1])
			{
				temp=freq[j];
				freq[j]=freq[j+1];
				freq[j+1]=temp;

				temp=sort[j];
				sort[j]=sort[j+1];
				sort[j+1]=temp;
			}
		}
	}

	for(i=0;i<nameTableWidth*nameTableHeight;++i)
	{
		for(j=0;j<256;++j)
		{
			if(sort[j]==nameTable[i])
			{
				nameTable[i]=j;
				break;
			}
		}
	}

	memcpy(chrt,&chr[bankActive],sizeof(chrt));

	for(i=0;i<256;++i)
	{
		memcpy(&chr[bankActive+i*16],&chrt[sort[i]*16],16);
	}

	UpdateAll();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MPutSelectionToMetaspriteAutoIncClick(
TObject *Sender)
{
	MPutSelectionToMetaspriteAutoInc->Checked^=true;	
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MNameTableNewClick(TObject *Sender)
{
	FormSetSize->ShowModal();

	if(!FormSetSize->Confirm) return;

	nameTableWidth =FormSetSize->NewWidth;
	nameTableHeight=FormSetSize->NewHeight;

	ClearNametable();

	nameSelection.left=-1;
	nameSelection.top=-1;
	nameCopyWidth=-1;
	nameCopyHeight=-1;

	SetUndo();

	UpdateAll();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MSaveMapClick(TObject *Sender)
{
	unsigned char meta[4];
	AnsiString name;
	FILE *file;

	if(!SaveDialogMap->Execute()) return;

	name=RemoveExt(SaveDialogMap->FileName)+".map";

	if(FileExists(name))
	{
		if(Application->MessageBox(("File "+name+" is already exist, overwrite?").c_str(),"Confirm",MB_YESNO)!=IDYES)return;
	}

	meta[0]=nameTableWidth&255;
	meta[1]=nameTableWidth/256;
	meta[2]=nameTableHeight&255;
	meta[3]=nameTableHeight/256;

	file=fopen(name.c_str(),"wb");

	if(!file) return;

	fwrite(nameTable,name_size(),1,file);
	fwrite(attrTable,attr_size(),1,file);
	fwrite(meta,sizeof(meta),1,file);
	fclose(file);
}
//---------------------------------------------------------------------------


