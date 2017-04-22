

#include "neslib.h"
#include "Level_Data.h"
#include "title.h"
const unsigned char palette[16]={ 0x31,0x0f,0x00,0x31,0x31,0x0f,0x15,0x25,0x31,0x0f,0x00,0x10,0x31,0x02,0x12,0x30 };


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
	128,

	- 8,  0,0x98,3,
	  0,  0,0x99,3,
	128,

	- 8,  0,0x9a,3,
	  0,  0,0x9b,3,
	128,

	- 8,  0,0x9c,3,
	  0,  0,0x9d,3,
	128,

	  0,  0,0x98,3|OAM_FLIP_H,
	- 8,  0,0x99,3|OAM_FLIP_H,
	128,

	  0,  0,0x9a,3|OAM_FLIP_H,
	- 8,  0,0x9b,3|OAM_FLIP_H,
	128,

	  0,  0,0x9c,3|OAM_FLIP_H,
	- 8,  0,0x9d,3|OAM_FLIP_H,
	128,

	- 4,- 7,0x87,2,
	- 4,  0,0x86,1,
	128

};

const unsigned char* const metasprite_list[]={
	metasprite_data+0,
	metasprite_data+13,
	metasprite_data+30,
	metasprite_data+35,
	metasprite_data+40,
	metasprite_data+49,
	metasprite_data+58,
	metasprite_data+67,
	metasprite_data+76,
	metasprite_data+85,
	metasprite_data+94,
	metasprite_data+103
};








static unsigned char clock;

 
static unsigned char y=12*8;
static unsigned char x=10*8;


	
		unsigned char addr=0,adr=0;
		
	static unsigned char yy=20*8,jump2,xx=5*8,i,touch2=1,aaa,ii,level=7,yyh,player2_frame=3,lives2=8,x3=12*8,y3=21*8;
 
	
void reserect(unsigned char p){

if(p==0){
x=rand8();
y=9*8;	

}

if(p==1){

xx=6*8;
yy=19*8;}
sfx_play(1,0);

	}













	unsigned char direction=0,invert=0;
unsigned char player_Y_offset=3,Player_PowerUp_state=0,mech1_frame=5,mech2_frame=5,mech3_frame=5,left,right,m_offset;

int collide(unsigned char x1,unsigned char y1,unsigned char x2,unsigned char y2,short h1,short w1,short h2,short w2,unsigned a){
	unsigned char touch;	
		if(!(x1+w1< x2 ||
		     x1>=x2+w2||
	         y1+h1< y2 ||
		     y1>=y2+h2)) touch=1; else touch=0;	
		 if(touch==1&&a==1){touch2=1;aaa=1;adr=0;yy--;if(direction==1){yy--;}if(direction==2){xx++;}if(direction==3){yy++;}if(direction==4){xx--;}}
	return touch;	
	
}

 unsigned char c=0,iii;

// obj 1: tracked platform
// obj 2: tracked buzzsaw
// obj 3: lobe enemy 
// obj 4: spud enemy 
// obj 5: boss enemy 
void draw_nam(const unsigned char *nam){

	ppu_off();
vram_adr(NAMETABLE_A);	
vram_unrle(nam);


vram_adr(NAMETABLE_A);	
vram_read(playfeld,960);

ppu_on_all();
	
	
}

void main(void)
{
short a_num,a_num1;
unsigned char x=a_num%32,y=a_num/32,hold=0,ww=0,w=0,www=0,x1=a_num1%32,y1=a_num1/32,p_space=0,p_space1=0;


	
		level=0;
		
	
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

   

   set_rand(rand8());
         

        
   
         if(lives2>1){
		oam_meta_spr(xx,yy-player_Y_offset,0,metasprite_list[player2_frame]);

		 if(ii& PAD_LEFT&&xx>8){m_offset=0; left=1; player2_frame=2;xx--;if(Player_PowerUp_state==1){if(clock/2>4)mech1_frame=9; else mech1_frame=10; }}else left=0;
		if(ii&PAD_RIGHT&&xx<254){ m_offset=1; right=1; player2_frame=3;xx++;if(Player_PowerUp_state==1){if(clock/2>4)mech1_frame=6; else mech1_frame=7;}}else right=0;
 	
	if(right==0&&left==0)mech1_frame=5;

		
	    if(jump2==0){yy=yy+1;}
		if(ii&PAD_A&&aaa==1&&adr==0){sfx_play(1,0); jump2=1;touch2=0; yyh=yy-24;adr=1;}
        if(jump2==1){yy--;}
		if(yy<yyh-1){jump2=0;hold=0;}
    	if(ii&PAD_A)hold++;
	if(www==2)www=0;
if(level==1&&www==0){draw_nam(Level_Data1);www=1;c=0;}
if(level==2&&www==1){draw_nam(Level_Data2);www=0;c=0;}
if(level==3&&www==0){draw_nam(Level_Data3);www=1;c=0;}
if(level==4&&www==1){draw_nam(Level_Data4);www=0;c=0;}
if(level==5&&www==0){draw_nam(Level_Data5);www=1;c=0;}
if(level==6&&www==1){draw_nam(Level_Data6);www=0;c=0;}
if(level==7&&www==0){draw_nam(Level_Data7);www=1;c=0;}
if(level==8&&www==1){draw_nam(Level_Data8);www=0;c=0;}
if(level==9&&www==0){draw_nam(Level_Data9);www=1;c=0;}
if(level==7){oam_meta_spr(x3,y3,20,metasprite_list[5]);if(Player_PowerUp_state<1)if(collide(x3,y3+8,xx,yy+2,6,16,8,8,0)){Player_PowerUp_state=1;sfx_play(0,0); oam_hide_rest(20);}}
if(level==3){oam_meta_spr(x3+8,y3+16,20,metasprite_list[5]);if(Player_PowerUp_state<1)if(collide(x3+8,y3+24,xx,yy+2,6,16,8,8,0)){Player_PowerUp_state=1;sfx_play(0,0); oam_hide_rest(20);}}
if(level==0){oam_meta_spr(x3,y3,20,metasprite_list[5]);if(Player_PowerUp_state<1)if(collide(x3,y3+8,xx,yy+2,6,16,8,8,0)){Player_PowerUp_state=1;sfx_play(0,0); oam_hide_rest(20);}}
if(Player_PowerUp_state==0&&level!=0&&level!=7&&level!=3){ oam_hide_rest(20);player_Y_offset=3;}
if(Player_PowerUp_state==1){player_Y_offset=11; oam_meta_spr(xx-m_offset,yy-8,20,metasprite_list[mech1_frame]);}

 
		 
if(playfeld[((yy/8)<<5)|(xx/8)]==0x08||playfeld[((yy/8)<<5)|(xx/8)]==0x09||playfeld[((yy/8)<<5)|(xx/8)]==0x18||playfeld[((yy/8)<<5)|(xx/8)]==0x19){music_pause(1);for(iii=4; iii<9; iii++){pal_bright(iii);ppu_wait_nmi();delay(4);sfx_play(1,2); }reserect(1);c=0;delay(20);level++;oam_clear();for(iii=8; iii>3; iii--){pal_bright(iii);ppu_wait_nmi();delay(4);sfx_play(3,1);}delay(40);music_pause(0);}						
if(playfeld[((yy/8)<<5)|(xx/8)]>0&&playfeld[((yy/8)<<5)|(xx/8)]<3||playfeld[((yy/8)<<5)|(xx/8)]==0x15||playfeld[((yy/8)<<5)|(xx/8)]==0x10){yy=yy-1;touch2=1;aaa=1;adr=0;}else aaa=0;
if(playfeld[((yy/8)<<5)|(xx/8)]>0x59&&playfeld[((yy/8)<<5)|(xx/8)]<0x73){if(Player_PowerUp_state>0)Player_PowerUp_state--; player_Y_offset=3; reserect(1);sfx_play(2,2);lives2--;jump2=0;}
if(playfeld[((yy/8)<<5)|(xx/8)]==0x12)xx++;	
if(playfeld[((yy/8)<<5)|(xx/8)]==0x11)xx--;		 
		 }else {oam_clear();if(www==0||www==1){   music_play(1);draw_nam(game_over);www=2;c=0;}if(ii&PAD_START){level=0; Player_PowerUp_state=0; music_play(0); draw_nam(Level_Data); lives2=8;} }

    if(clock<20)clock++;else clock=0;

		ppu_wait_nmi();

		if(clock>10){bank_bg(1);bank_spr(1);} else {bank_bg(0);bank_spr(0);}
		
		
if(a_num==927&&c==0){a_num=420;}else if(c==0){a_num++;a_num++;a_num++; x=(a_num%32)*8;y=(a_num/32)*8;}
if(playfeld[a_num]==0x74){c=1;}
if(playfeld[a_num]==0x75){c=2;}

if(c==2){oam_meta_spr(x+8,y+2,4,metasprite_list[1]);if(collide(x,y+3,xx,yy,16,16,8,8,2)){Player_PowerUp_state--; reserect(1);sfx_play(2,2);lives2--;jump2=0;}}

if(c==1){oam_meta_spr(x+8,y,8,metasprite_list[0]);collide(x,y+3,xx,yy,8,24,8,8,1);}
if(c==0)oam_clear();

if(direction==1){y--;}if(direction==2){x++;}if(direction==3){y++;}if(direction==4){x--;}
	

	
		
		 if(direction==5){direction=1;}
		 if(playfeld[((y/8)<<5)|(x/8)]==0x7D)invert=1;
		if(invert==1)direction=4; else direction=2;
      		
         if(playfeld[((y/8)<<5)|(x/8)]==0x76)invert=0;
		          if(playfeld[((y/8)<<5)|(x/8)]==0x77)invert=1;
		 
		 

	
	}

}











