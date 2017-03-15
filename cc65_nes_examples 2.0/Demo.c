

#include "neslib.h"
#include "Level_Data.h"
const unsigned char palette[16]={ 0x30,0x0f,0x21,0x31,0x30,0x0f,0x15,0x25,0x30,0x0f,0x2d,0x20,0x30,0x0f,0x2d,0x10 };



const unsigned char palette1[16]={ 0x30,0x0f,0x19,0x29,0x30,0x0f,0x15,0x25,0x30,0x0f,0x2d,0x30,0x30,0x0f,0x16,0x26 };






unsigned char playfeld[928];

const unsigned char metasprite_data[]={

	-12,- 4,0x10,3,
	- 4,- 4,0x10,3,
	  4,- 4,0x10,3,
	128,

	- 8,- 8,0x61,2,
	  0,- 8,0x62,2,
	- 8,  0,0x71,2,
	  0,  0,0x72,2,
	128,

	- 4,- 4,0x80,0,
	128,

	- 4,- 4,0x80,0|OAM_FLIP_H,
	128,

	- 8,- 4,0x84,1,
	  0,- 4,0x85,1,
	128

};

const unsigned char* const metasprite_list[]={
	metasprite_data+0,
	metasprite_data+13,
	metasprite_data+30,
	metasprite_data+35,
	metasprite_data+40
};





static unsigned char clock;

 
static unsigned char y=12*8;
static unsigned char x=10*8;


	
		unsigned char addr=0,adr=0;
		
	static unsigned char yy=2*8,jump2,xx=2*8,i,touch2=1,aaa,ii,level=1,yyh,player2_frame=3,lives2=4;
 
	
void reserect(unsigned char p){
set_rand(rand8());
if(p==0){
x=rand8();
y=9*8;	

}

if(p==1){

xx=rand8();
yy=9*8;}
sfx_play(1,0);
	
}















void collide(unsigned char x1,unsigned char y1,unsigned char x2,unsigned char y2,short h1,short w1,short h2,short w2){
	unsigned char touch;	
		if(!(x1+w1< x2 ||
		     x1>=x2+w2||
	         y1+h1< y2 ||
		     y1>=y2+h2)) touch=1; else touch=0;	
		 if(touch==1&&yy<y2+1)y--;
 }

 unsigned char c=0;

void scan_nam(){
	unsigned char num;
	unsigned char direction;
	unsigned char a;
	 unsigned char x=(num % 32)*8;
	 unsigned char y=(num / 32)*8;
if(playfeld[num]==0x7d){oam_meta_spr(x,y,4,metasprite_list[0]);collide(xx,yy,x,y,8,8,8,24);if(playfeld[((y/8)<<5)|(x/8)]==0x74){if(a==0)direction=4 else }}
	
}
// obj 1: tracked platform
// obj 2: tracked buzzsaw
// obj 3: lobe enemy 
// obj 4: spud enemy 
// obj 5: boss enemy 

void main(void)
{
	
	
		level=1;
		
	
	pal_bg(palette);
	pal_spr(palette1);
    music_play(0);


	 


vram_adr(NAMETABLE_A);	
vram_unrle(Level_Data);


vram_adr(NAMETABLE_A);	
vram_read(playfeld,960);

ppu_on_all();
while(1){
  	 i=pad_poll(1);
		ii=pad_poll(0);

   

   
         
 
        
   
         if(lives2>1){
		oam_meta_spr(xx+4,yy-3,0,metasprite_list[player2_frame]);

		if(ii& PAD_LEFT&&touch2==1){ player2_frame=2;xx=xx-1;}
		if(ii&PAD_RIGHT&&touch2==1){  player2_frame=3;xx=xx+1;}

		
	    if(jump2==0){yy=yy+1;}
		if(ii&PAD_A&&aaa==1&&adr==0){jump2=1; yyh=yy-40;adr=1;}
        if(jump2==1&&yy>yyh){yy--;	}
		if(jump2==1){if(player2_frame<2)player2_frame=2;if(player2_frame>3)player2_frame=5;}
		if(yy<yyh+1){jump2=2;}
        if(jump2==2){yy++;}		
	
		 
		 			if(level==1){	   
if(playfeld[((yy/8)<<5)|(xx/8)]>0&&playfeld[((yy/8)<<5)|(xx/8)]<3||playfeld[((yy/8)<<5)|(xx/8)]==0x15||playfeld[((yy/8)<<5)|(xx/8)]==0x10){yy=yy-1;touch2=1;aaa=1;adr=0;}else aaa=0;
if(playfeld[((yy/8)<<5)|(xx/8)]>0x59&&playfeld[((yy/8)<<5)|(xx/8)]<0x73){reserect(1);sfx_play(2,2);lives2--;}
		 }}

    if(clock<20)clock++;else clock=0;
		 
		scroll(0,0);//set normal scroll, it'll be applied at beginning of a TV frame
		
		ppu_wait_nmi();//not ppu_wait_frame, because every 6th frame would not have the split

		if(clock>10)bank_bg(1); else bank_bg(0);
		

		
	




	

	
	
	
	}

}











