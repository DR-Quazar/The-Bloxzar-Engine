

#include "neslib.h"
#include "bloxzar.h"
const unsigned char palette[16]={ 0x10,0x11,0x21,0x31,0x10,0x0f,0x15,0x25,0x10,0x0f,0x2d,0x30,0x10,0x0f,0x16,0x26 };



const unsigned char palette1[16]={ 0x0f,0x0f,0x19,0x29,0x0f,0x0f,0x15,0x25,0x0f,0x0f,0x19,0x29,0x0f,0x0f,0x2d,0x10 };





unsigned char playfeld[928];




const unsigned char metasprite_data[]={

	- 4,- 4,0x80,0|OAM_FLIP_H,
	128,

	- 4,- 4,0x81,0,
	128,

	- 4,- 4,0x81,0,
	128,

	- 4,- 4,0x81,0|OAM_FLIP_H,
	128,

	- 4,- 4,0x81,0|OAM_FLIP_H,
	128,

	- 4,- 4,0x81,0|OAM_FLIP_H,
	128

};

const unsigned char* const metasprite_list[]={
	metasprite_data+0,
	metasprite_data+5,
	metasprite_data+10,
	metasprite_data+15,
	metasprite_data+20,
	metasprite_data+25
};





static unsigned char clock;

 
static unsigned char y=12*8;
static unsigned char x=10*8,xxx;
static unsigned char player_frame=6;
static unsigned char jump=0;


	static unsigned char h=0;	
		unsigned char addr=0,adr=0;
		
	static unsigned char yy=2*8,jump2,xx=2*8,i,touch2=1,aa,aaa,ii,level=1,touch1=1,lives=4,t,yh,yyh,player2_frame=0,lives2=4,p_space=0,p_space2=0;
 

	
void reserect(unsigned char p){
set_rand(rand8());
if(p==0){
x=rand8();
y=8*8;	

}

if(p==1){

xx=rand8();
yy=8*8;}
sfx_play(1,0);
	
}















void collide(unsigned char x1,unsigned char y1,unsigned char x2,unsigned char y2,short h1,short w1,short h2,short w2,unsigned char a){
	unsigned char touch;	
		if(!(x1+w1< x2 ||
		     x1>=x2+w2||
	         y1+h1< y2 ||
		     y1>=y2+h2)) touch=1; else touch=0;	
		 
 }

unsigned char c=0; 


static unsigned char top_x,bottom_x;


void main(void)
{
	
		static unsigned char top_x,bottom_x;
	
		level=1;
		
	
	pal_bg(palette);
	pal_spr(palette1);
     //music_play(0);


	 


vram_adr(NAMETABLE_A);	
vram_unrle(bloxzar);


vram_adr(NAMETABLE_A);	
vram_read(playfeld,960);

 top_x=0;
	bottom_x=0;			
//put sprite at the screen split point
ppu_on_all();
while(1){
  	 i=pad_poll(1);
		ii=pad_poll(0);

   

   
   
 
   
   
         if(lives2>0){
		oam_meta_spr(xx,yy-3,17*4,metasprite_list[player2_frame]);

		if(ii& PAD_LEFT&&touch2==1){if(clock>6)player2_frame=4;else player2_frame=3;xx=xx-1;}
		if(ii&PAD_RIGHT&&touch2==1){ 	if(clock>6)player2_frame=1;else player2_frame=0;xx=xx+1;}

		
	    if(jump2==0){yy=yy+1;}
		if(ii&PAD_A&&aaa==1&&adr==0){jump2=1; yyh=yy-40;adr=1;}
        if(jump2==1&&yy>yyh){yy--;	}
		if(jump2==1){if(player2_frame<2)player2_frame=2;if(player2_frame>3)player2_frame=5;}
		if(yy<yyh+1){jump2=2;}
        if(jump2==2){yy++;}		
	
		 
		 			if(level==1){	   
if(playfeld[((yy/8)<<5)|(xx/8)]>0&&playfeld[((yy/8)<<5)|(xx/8)]<3||playfeld[((yy/8)<<5)|(xx/8)]==0x15){yy=yy-1;touch2=1;aaa=1;adr=0;}else aaa=0;
		 if(playfeld[((yy/8)<<5)|(xx/8)]==0x19)xx=xx-2;
		 }}

    if(clock<29)clock++;else clock=0;
		 
		scroll(0,0);//set normal scroll, it'll be applied at beginning of a TV frame
		
		ppu_wait_nmi();//not ppu_wait_frame, because every 6th frame would not have the split

		
		++top_x;
		--bottom_x;
		
	




	

	
	
	
	}

}











