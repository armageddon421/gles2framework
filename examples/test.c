

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>		// usleep
#include <math.h>
#include <kazmath.h>		// matrix manipulation routines

#include "support.h"		// support routines
#include "keys.h"		// defines key indexes for key down boolean array
#include "input.h"

#define RE_RANGE 100
#define RE_PROBABILITY 20
#define RE_TARGET 10
#define RE_MULTIPLY 20		//how often to do random events per frame

#define DAMPING 0.960

#define flare_size 16

#define max_flares 400
//#define max_targets 23
#define reserved_targets 500	//how much memory to reserve for targets

int max_targets = 23;		//actual number of targets


void render();			// func prototype
void think();
void relocate_targets();
void reassign_flares();
void random_events();
void spawn_target(float,float);
void save(void);
void loadfile();
// textures
GLuint flareTex;

float width, height;

font_t *font1;

// matrices and combo matrices
kmMat4 model, view, projection, mvp, vp, mv;

int frame = 0;

int fileid;


bool *keys;
bool lastkeys[256];
int mx,my;

struct flare_t {
	float x,y,vx,vy;
	int target;
};

struct target_t {
	float x,y;
	int flares;
};


struct flare_t flares[max_flares];
struct target_t targets[reserved_targets];

bool file_exists(const char * filename)
{
    FILE *file;
    if (file=fopen(filename, "r")) //I'm sure, you meant for READING =)
    {
        fclose(file);
        return true;
    }
    return false;
}



void save(){

	int fid = 0;
	char buf[20];
	
	do{
		sprintf(buf, "saves/%04i.txt", fid); 
		fid++;
	}while(file_exists(buf));
	
	FILE *outfile;
	outfile = fopen(buf, "w");
	
	for (int i=0; i<max_targets; i++){
		fprintf(outfile,"%i\n%i\n", (int)targets[i].x, (int)targets[i].y);
		
	}

	fclose(outfile);

	printf("saved as %s\n", buf);
}


void loadfile(){
	char fbuf[20];
	sprintf(fbuf, "saves/%04i.txt", fileid); 
	printf("loading %s\n", fbuf);
	if(!file_exists(fbuf)){
		fileid--;
		 return;
	}

	max_targets = 0; 	//reset all targets	
	int state = 0, temp = 0;
	FILE *inputFilePtr;
	inputFilePtr = fopen(fbuf, "r");
	char buf[20];
	
	while(fgets(buf,20,inputFilePtr)){
		//printf("%s", buf);
		int value = atoi(buf);
		if(state == 0){
			temp = value;
			state++;
		}
		else if(state == 1){
			state = 0;
			spawn_target(temp,value);
		}
	}	
	fclose(inputFilePtr);	
	printf("loaded %s\n", fbuf);

	reassign_flares();
}

//float rand_range(float min,float max) {
//    return min + (max - min) * ((float)rand() / RAND_MAX) / 2.0;
//}


void spawn_target(float x, float y){
	if (max_targets >= reserved_targets) return;
	targets[max_targets].x = x;	
	targets[max_targets].y = y;	
	targets[max_targets].flares = 0;
	max_targets++;
}

int main()
{


    // creates a window and GLES context
    if (makeContext() != 0)
        exit(-1);

    // all the shaders have at least texture unit 0 active so
    // activate it now and leave it active
    glActiveTexture(GL_TEXTURE0);

    flareTex = loadPNG("resources/textures/cloud.png");
	
	
    width = getDisplayWidth();
    height = getDisplayHeight();

    glViewport(0, 0, getDisplayWidth(), getDisplayHeight());

    // initialises glprint's matrix, shader and texture
    initGlPrint(getDisplayWidth(), getDisplayHeight());
    font1=createFont("resources/textures/font.png",0,256,16,16,16);
    initSprite(getDisplayWidth(), getDisplayHeight());
	

    fileid = 0;	
    loadfile();
    for (int i=0; i<max_flares; i++) {
        flares[i].x=rand_range(0,getDisplayWidth());
        flares[i].y=rand_range(0,getDisplayHeight());
        flares[i].vx=rand_range(0,10)-5;
        flares[i].vy=rand_range(0,10)-5;
    }
    /*for (int i=0; i<max_targets; i++) {
        targets[i].x=rand_range(0,getDisplayWidth());
        targets[i].y=rand_range(0,getDisplayHeight());
	targets[i].flares = 0;
    }*/

    /*for (int i=0; i<max_flares; i++) {
        flares[i].x=rand_range(0,getDisplayWidth());
        flares[i].y=rand_range(0,getDisplayHeight());
        flares[i].vx=rand_range(0,10)-5;
        flares[i].vy=rand_range(0,10)-5;
	flares[i].target = (int)rand_range(0,max_targets);
	targets[flares[i].target].flares ++;
    }*/


    // we don't want to draw the back of triangles
    // the blending is set up for glprint but disabled
    // while not in use
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0, 0.5, 1, 1);

    // count each frame
    int num_frames = 0;

    // set to true to leave main loop
    bool quit = false;
    
// get a pointer to the key down array
    keys = getKeys();
	int* mouse = getMouse();

    while (!quit) {		// the main loop

        doEvents();	// update mouse and key arrays

        if (keys[KEY_ESC])
            quit = true;	// exit if escape key pressed
        if (keys[KEY_SPACE])
            relocate_targets();	// exit if escape key pressed
        if (keys[KEY_RETURN])
            reassign_flares();	// exit if escape key pressed
        if (keys[KEY_S] && !lastkeys[KEY_S])
            save();	// exit if escape key pressed
        if (keys[KEY_CURSL] && !lastkeys[KEY_CURSL]){
            fileid--;
	    if (fileid <0) fileid = 0;
	    loadfile();	// exit if escape key pressed
	}
        if (keys[KEY_CURSR] && !lastkeys[KEY_CURSR]){
            fileid++;
	    loadfile();	// exit if escape key pressed
	}
        if (keys[KEY_C]){
            max_targets = 1;	// exit if escape key pressed
	    reassign_flares();    
	}
	lastkeys[KEY_S] = keys[KEY_S];
	lastkeys[KEY_CURSL] = keys[KEY_CURSL];
	lastkeys[KEY_CURSR] = keys[KEY_CURSR];
	
	mx = mouse[0];
	my = mouse[1];
	if(mouse[2] != 0){
		spawn_target(mx,my);
	}

	for (int i=0; i<RE_MULTIPLY; i++){
		random_events();
	}
	
	think();
        
        render();	// the render loop

        usleep(1600);	// no need to run cpu/gpu full tilt

    }

    closeContext();		// tidy up

    return 0;
}

void render()
{

    float rad;		// radians of rotation based on frame counter

    // clear the colour (drawing) and depth sort back (offscreen) buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // count the frame and base a rotation on it.
    frame++;
    rad = frame * (0.0175f);

    for (int i=0; i<max_flares; i++) {
        drawSprite( flares[i].x,flares[i].y,flare_size,flare_size,0,flareTex);
    }
       
	
	drawSprite( mx,my,64,64,0,flareTex);

    // see printf documentation for the formatting of variables...
    glPrintf(100, 240, font1,"frame=%i, mousebtn: %i, targets: %i", frame, mx, max_targets);

    // swap the front (visible) buffer for the back (offscreen) buffer
    swapBuffers();

}


float dist2(float ax, float ay, float bx, float by){


	return sqrt(pow(ax-bx,2)+pow(ay-by,2));

}

float dist(int a, int b){

	return sqrt(pow(flares[a].x-flares[b].x,2)+pow(flares[a].y-flares[b].y,2));

}

int nearest(int dex){
	float min = 100000;
	int res = 0;
	
	for (int i = 0; i<max_flares; i++){
		if (i == dex) continue;
		float d = dist(i,dex);
		if (d < min){
			min = d;
			res = i;
		}
	}	
	return res;
}

int nearest_target(int dex){
	float min = 100000;
	int res = 0;
	
	for (int i = 0; i<max_targets; i++){
		if (i == dex) continue;
		float d = dist2(targets[i].x,targets[i].y,targets[dex].x,targets[dex].y);
		if (d < min){
			min = d;
			res = i;
		}
	}	
	return res;
}


void think(){
	
	//think
	for (int i=0; i<max_flares; i++) {
#ifdef COLLISION	
		for (int j=i+1; j<max_flares; j++) {
			//if(i==j) continue;
			float d = dist(i,j);
			if(d<flare_size){
				float dx = flares[j].x - flares[i].x;
				float dy = flares[j].y - flares[i].y;
				dx /= d / 0.8;
				dy /= d / 0.8;
				flares[i].vx -= dx;
				flares[i].vy -= dy;
				flares[j].vx += dx;
				flares[j].vy += dy;
			}
		}
#endif
		float tx = targets[flares[i].target].x;
		float ty = targets[flares[i].target].y;
		float d = dist2(tx, ty, flares[i].x, flares[i].y);
		float dx = tx - flares[i].x;
		float dy = ty - flares[i].y;
		dx /= d / 0.5;
		dy /= d / 0.5;
		flares[i].vx += dx;
		flares[i].vy += dy;
		
	}
	//move
	for (int i=0; i<max_flares; i++) {

            flares[i].x+=flares[i].vx;
            flares[i].y+=flares[i].vy;
            if (flares[i].x<0) {
		flares[i].x = 0;
		flares[i].vx += 5;
            }
            if (flares[i].x>width) {
		flares[i].x = width;
		flares[i].vx -= 5;
            }
            if (flares[i].y<0) {
		flares[i].y = 0;
		flares[i].vy += 5;
            }
            if (flares[i].y>height) {
		flares[i].y = height;
		flares[i].vy -= 5;
            }

		//damping
		flares[i].vx *= DAMPING;	
		flares[i].vy *= DAMPING;	
	
        }

	
	
}


void relocate_targets(){
	
    for (int i=0; i<max_targets; i++) {
        targets[i].x=rand_range(100,getDisplayWidth()-100);
        targets[i].y=rand_range(100,getDisplayHeight()-100);
    }

    reassign_flares();	
}


void reassign_flares(){
    for (int i=0; i<max_targets; i++) {
	targets[i].flares = 0;
    }

    for (int i=0; i<max_flares; i++) {
        flares[i].vx+=rand_range(0,10)-5;
        flares[i].vy+=rand_range(0,10)-5;
	flares[i].target = (int)rand_range(0,max_targets);
	targets[flares[i].target].flares ++;
    }	


}


void random_events(){


/*	int rnd = (int)rand_range(0,max_flares);

	int near = nearest_target(flares[rnd].target);
	targets[flares[rnd].target].flares --;
	flares[rnd].target = near;
	targets[flares[rnd].target].flares ++;
*/

	int rnd = (int)rand_range(0,max_flares);
	
    	for (int j=0; j<max_targets; j++) {
		int i = (int)rand_range(0,max_targets);
		if (i==flares[rnd].target) continue;
		float probmul = 1;
		if (targets[flares[rnd].target].flares > targets[i].flares) probmul *= 2;

		if (targets[i].flares < RE_TARGET) probmul *= 2;
		else if (targets[i].flares > RE_TARGET) probmul *= 0.5;

		if (targets[flares[rnd].target].flares < RE_TARGET) probmul *= 0.5;
		else if (targets[flares[rnd].target].flares > RE_TARGET) probmul *= 2;
		

		if (dist2(flares[rnd].x,flares[rnd].y,targets[i].x,targets[i].y) < RE_RANGE){
			if (rand_range(0,100)<RE_PROBABILITY*probmul){
				targets[flares[rnd].target].flares --;
				flares[rnd].target = i;
				targets[flares[rnd].target].flares ++;
				return;
			}
			
		}
		
		
	}
}
