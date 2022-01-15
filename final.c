/*
HW4 Perspectives
use 1,2,3 to change between the perspectives
while in first person, use wasd to move
cursers will always change your angle of perspective (up down, left right)
hold 't' to move the balls
0 resets your perspective

*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#ifdef USEGLEW
#include <GL/glew.h>
#endif
//  OpenGL with prototypes for glext
#define GL_GLEXT_PROTOTYPES
#ifdef __APPLE__
#include <GLUT/glut.h>
// Tell Xcode IDE to not gripe about OpenGL deprecation
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#else
#include <GL/glut.h>
#endif
//  Default resolution
//  For Retina displays compile with -DRES=2
#ifndef RES
#define RES 1
#endif


int mode=1;       //  0 is ortho, 1 is perspective, 2 is first person
int move=1;       //  Move light
int th=0;         //  Azimuth of view angle
int ph=0;         //  Elevation of view angle
int fov=55;       //  Field of view (for perspective)
double asp=1;     //  Aspect ratio
double dim=3.0;   //  Size of world
float zmin=+2;       //  DEM lowest location
float zmax=-1;       //  DEM highest location
float zmag=1;          //  DEM magnification
// Light values
int light     =   1;  // Lighting
int one       =   1;  // Unit value
int distance  =   4;  // Light distance
int inc       =  10;  // Ball increment
int smooth    =   1;  // Smooth/Flat shading
int local     =   0;  // Local Viewer Model
int emission  =   0;  // Emission intensity (%)
int ambient   =  45;  // Ambient intensity (%)
int diffuse   =  50;  // Diffuse intensity (%)
int specular  =   0;  // Specular intensity (%)
int shininess =   0;  // Shininess (power of two)
float shiny   =   1;  // Shininess (value)
int zh        =  90;  // Light azimuth
float ylight  =   0;  // Elevation of light
unsigned int texture[6]; // Texture names
unsigned char* image = 0;
float grass[48][48];
int    sky[2];   //  Sky textures
int alpha = 10;
float terrain [64][64];
double quadMul = 1.0;
int grassMul = 1;
//  Macro for sin & cos in degrees
#define Cos(th) cos(3.14159265/180*(th))
#define Sin(th) sin(3.14159265/180*(th))

/*
 *  Convenience routine to output raster text
 *  Use VARARGS to make this more flexible
 */
#define LEN 8192  //  Maximum length of text string
void Print(const char* format , ...) //from class examples
{
   char    buf[LEN];
   char*   ch=buf;
   va_list args;
   //  Turn the parameters into a character string
   va_start(args,format);
   vsnprintf(buf,LEN,format,args);
   va_end(args);
   //  Display the characters one at a time at the current raster position
   while (*ch)
      glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,*ch++);
}

/*
 *  Check for OpenGL errors
 */
void ErrCheck(const char* where)//from class examples
{
   int err = glGetError();
   if (err) fprintf(stderr,"ERROR: %s [%s]\n",gluErrorString(err),where);
}

/*
 *  Print message to stderr and exit
 */
void Fatal(const char* format , ...)//from class examples
{
   va_list args;
   va_start(args,format);
   vfprintf(stderr,format,args);
   va_end(args);
   exit(1);
}
void idle()//from class examples
{
   //  Elapsed time in seconds
   double t = glutGet(GLUT_ELAPSED_TIME)/1000.0;
   zh = fmod(90*t,360.0);
   //  Tell GLUT it is necessary to redisplay the scene
   glutPostRedisplay();
}

//generates a 64x64 array of random numbers between  roughly 0-2
void createTerrain()
{
  for (int i = 0; i < 64; i++){
    for (int j = 0; j < 64; j++){
      terrain[i][j] = (rand() % 10)/6;
    }
  }
}

/*
 *  Set projection
 */
static void Project() //from class examples
{
   //  Tell OpenGL we want to manipulate the projection matrix
   glMatrixMode(GL_PROJECTION);
   //  Undo previous transformations
   glLoadIdentity();

   //  Orthogonal projection

   gluPerspective(fov,asp,dim/4,24*dim);

   //  Switch to manipulating the model matrix
   glMatrixMode(GL_MODELVIEW);
   //  Undo previous transformations
   glLoadIdentity();
}

static void Reverse(void* x,const int n) //from class examples
{
   int k;
   char* ch = (char*)x;
   for (k=0;k<n/2;k++)
   {
      char tmp = ch[k];
      ch[k] = ch[n-1-k];
      ch[n-1-k] = tmp;
   }
}

//
//  Load texture from BMP file
//
unsigned int LoadTexBMP(const char* file) //from class examples
{
   unsigned int   texture;    // Texture name
   FILE*          f;          // File pointer
   unsigned short magic;      // Image magic
   unsigned int   dx,dy,size; // Image dimensions
   unsigned short nbp,bpp;    // Planes and bits per pixel
   unsigned char* image;      // Image data
   unsigned int   off;        // Image offset
   unsigned int   k;          // Counter
   unsigned int   max;        // Maximum texture dimensions

   //  Open file
   f = fopen(file,"rb");
   if (!f) Fatal("Cannot open file %s\n",file);
   //  Check image magic
   if (fread(&magic,2,1,f)!=1) Fatal("Cannot read magic from %s\n",file);
   if (magic!=0x4D42 && magic!=0x424D) Fatal("Image magic not BMP in %s\n",file);
   //  Read header
   if (fseek(f,8,SEEK_CUR) || fread(&off,4,1,f)!=1 ||
       fseek(f,4,SEEK_CUR) || fread(&dx,4,1,f)!=1 || fread(&dy,4,1,f)!=1 ||
       fread(&nbp,2,1,f)!=1 || fread(&bpp,2,1,f)!=1 || fread(&k,4,1,f)!=1)
     Fatal("Cannot read header from %s\n",file);
   //  Reverse bytes on big endian hardware (detected by backwards magic)
   if (magic==0x424D)
   {
      Reverse(&off,4);
      Reverse(&dx,4);
      Reverse(&dy,4);
      Reverse(&nbp,2);
      Reverse(&bpp,2);
      Reverse(&k,4);
   }
   //  Check image parameters
   glGetIntegerv(GL_MAX_TEXTURE_SIZE,(int*)&max);
   if (dx<1 || dx>max) Fatal("%s image width %d out of range 1-%d\n",file,dx,max);
   if (dy<1 || dy>max) Fatal("%s image height %d out of range 1-%d\n",file,dy,max);
   if (nbp!=1)  Fatal("%s bit planes is not 1: %d\n",file,nbp);
   if (bpp!=24) Fatal("%s bits per pixel is not 24: %d\n",file,bpp);
   if (k!=0)    Fatal("%s compressed files not supported\n",file);
#ifndef GL_VERSION_2_0
   //  OpenGL 2.0 lifts the restriction that texture size must be a power of two
   for (k=1;k<dx;k*=2);
   if (k!=dx) Fatal("%s image width not a power of two: %d\n",file,dx);
   for (k=1;k<dy;k*=2);
   if (k!=dy) Fatal("%s image height not a power of two: %d\n",file,dy);
#endif

   //  Allocate image memory
   size = 3*dx*dy;
   image = (unsigned char*) malloc(size);
   if (!image) Fatal("Cannot allocate %d bytes of memory for image %s\n",size,file);
   //  Seek to and read image
   if (fseek(f,off,SEEK_SET) || fread(image,size,1,f)!=1) Fatal("Error reading data from image %s\n",file);
   fclose(f);
   //  Reverse colors (BGR -> RGB)
   for (k=0;k<size;k+=3)
   {
      unsigned char temp = image[k];
      image[k]   = image[k+2];
      image[k+2] = temp;
   }

   //  Sanity check
   ErrCheck("LoadTexBMP");
   //  Generate 2D texture
   glGenTextures(1,&texture);
   glBindTexture(GL_TEXTURE_2D,texture);
   //  Copy image
   glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,dx,dy,0,GL_RGB,GL_UNSIGNED_BYTE,image);
   if (glGetError()) Fatal("Error in glTexImage2D %s %dx%d\n",file,dx,dy);
   //  Scale linearly when image size doesn't match
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

   //  Free image memory
   free(image);
   //  Return texture name
   return texture;
}

static void Vertex(double th,double ph) //from class examples
{
   double x = Sin(th)*Cos(ph);
   double y = Cos(th)*Cos(ph);
   double z =         Sin(ph);
   //  For a sphere at the origin, the position
   //  and normal vectors are the same
   glNormal3d(x,y,z);
   glVertex3d(x,y,z);
}

//the following function is from https://newbedev.com/drawing-circle-with-opengl, all credit to them. I changed it from 2d to 3d though
static void DrawCircle(float cx, float cy, float cz, float r, int num_segments, int axis) {
  //draw in x/y
  if (axis == 1){
    glBegin(GL_LINE_LOOP);
    for (int ii = 0; ii < num_segments; ii++)   {
        float theta = 2.0 * 3.1415926 * (float)ii / (float)num_segments;//get the current angle
        float x = r * cosf(theta);//calculate the x component
        float y = r * sinf(theta);//calculate the y component
        glVertex3f(x + cx, y + cy, cz);//output vertex
    }
    glEnd();
  }
  //draw in x/z
  else if (axis == 2){
    glBegin(GL_LINE_LOOP);
    for (int ii = 0; ii < num_segments; ii++)   {
        float theta = 2.0 * 3.1415926 * (float)ii / (float)num_segments;//get the current angle
        float x = r * cosf(theta);//calculate the x component
        float z = r * sinf(theta);//calculate the y component
        glVertex3f(x + cx,cy,z + cz);//output vertex
    }
    glEnd();
  }
  //draw in y/z
  else if (axis == 3){
    glBegin(GL_LINE_LOOP);
    for (int ii = 0; ii < num_segments; ii++)   {
        float theta = 2.0 * 3.1415926 * (float)ii / (float)num_segments;//get the current angle
        float z = r * cosf(theta);//calculate the x component
        float y = r * sinf(theta);//calculate the y component
        glVertex3f(cx, y + cy,z + cz);//output vertex
    }
    glEnd();
  }

}

//this draws the light source ball, from example 13
static void ball(double x,double y,double z,double r)
{
   //  Save transformation
   glPushMatrix();
   //  Offset, scale and rotate
   glTranslated(x,y,z);
   glScaled(r,r,r);
   //  White ball with yellow specular
   float yellow[]   = {1.0,1.0,0.0,1.0};
   float Emission[] = {0.0,0.0,0.01*emission,1.0};
   glColor3f(1,1,1);
   glMaterialf(GL_FRONT,GL_SHININESS,shiny);
   glMaterialfv(GL_FRONT,GL_SPECULAR,yellow);
   glMaterialfv(GL_FRONT,GL_EMISSION,Emission);
   //  Bands of latitude
   for (int ph=-90;ph<90;ph+=inc)
   {
      glBegin(GL_QUAD_STRIP);
      for (int th=0;th<=360;th+=2*inc)
      {
         Vertex(th,ph);
         Vertex(th,ph+inc);
      }
      glEnd();
   }
   //  Undo transofrmations
   glPopMatrix();
}

//This is taken from class example 24
static void Sky(double D)
{
   //  Textured white box dimension (-D,+D)
   glPushMatrix();
   glScaled(D,D,D);
   glEnable(GL_TEXTURE_2D);
   glColor3f(1,1,1);

   //  Sides
   glBindTexture(GL_TEXTURE_2D,sky[0]);
   glBegin(GL_QUADS);
   glTexCoord2f(0.00,0); glVertex3f(-1,-1,-1);
   glTexCoord2f(0.25,0); glVertex3f(+1,-1,-1);
   glTexCoord2f(0.25,1); glVertex3f(+1,+1,-1);
   glTexCoord2f(0.00,1); glVertex3f(-1,+1,-1);

   glTexCoord2f(0.25,0); glVertex3f(+1,-1,-1);
   glTexCoord2f(0.50,0); glVertex3f(+1,-1,+1);
   glTexCoord2f(0.50,1); glVertex3f(+1,+1,+1);
   glTexCoord2f(0.25,1); glVertex3f(+1,+1,-1);

   glTexCoord2f(0.50,0); glVertex3f(+1,-1,+1);
   glTexCoord2f(0.75,0); glVertex3f(-1,-1,+1);
   glTexCoord2f(0.75,1); glVertex3f(-1,+1,+1);
   glTexCoord2f(0.50,1); glVertex3f(+1,+1,+1);

   glTexCoord2f(0.75,0); glVertex3f(-1,-1,+1);
   glTexCoord2f(1.00,0); glVertex3f(-1,-1,-1);
   glTexCoord2f(1.00,1); glVertex3f(-1,+1,-1);
   glTexCoord2f(0.75,1); glVertex3f(-1,+1,+1);
   glEnd();

   //  Top and bottom
   glBindTexture(GL_TEXTURE_2D,sky[1]);
   glBegin(GL_QUADS);
   glTexCoord2f(0.0,0); glVertex3f(+1,+1,-1);
   glTexCoord2f(0.5,0); glVertex3f(+1,+1,+1);
   glTexCoord2f(0.5,1); glVertex3f(-1,+1,+1);
   glTexCoord2f(0.0,1); glVertex3f(-1,+1,-1);

   glTexCoord2f(1.0,1); glVertex3f(-1,-1,+1);
   glTexCoord2f(0.5,1); glVertex3f(+1,-1,+1);
   glTexCoord2f(0.5,0); glVertex3f(+1,-1,-1);
   glTexCoord2f(1.0,0); glVertex3f(-1,-1,-1);
   glEnd();

   //  Undo
   glDisable(GL_TEXTURE_2D);
   glPopMatrix();
}



//The following is taken from example 17, but only the portion where the DEM map is utilized with the shader
static void grassArea(double x, double y, double z)
{
  double z0 = (zmin+zmax)/2;
  glPushMatrix();
  glTranslated(x,y,z);
  glColor3f(1,1,1);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D,texture[4]);
  for (int i=0;i<63;i++)
    for (int j=0;j<63;j++)
    {
       float x=(16*i-512);
       float z=(16*j-512);
       glBegin(GL_QUADS);
       glTexCoord2f(0,0); glVertex3d(x+ 0,zmag*(terrain[i+0][j+0]-z0),z+0);
       glTexCoord2f(2,0); glVertex3d(x+16,zmag*(terrain[i+1][j+0]-z0),z+0);
       glTexCoord2f(2,2); glVertex3d(x+16,zmag*(terrain[i+1][j+1]-z0),z+16);
       glTexCoord2f(0,2); glVertex3d(x+ 0,zmag*(terrain[i+0][j+1]-z0),z+16);
       glEnd();
    }
  glPopMatrix();
  glDisable(GL_TEXTURE_2D);
}

static void grass2D(double y){
  /*
  This populates the grass plane with 2d tufts of grass to give it a 3d feeling
  all the grass points toward the origin
  */
  //avoid (-8,8)x and -10,10z (x,z)
  float grey[] = {1,1,1,1};
  float black[] = {0,0,0,1};
  glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,0.8);
  glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,grey);
  glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,black);

  glEnable(GL_TEXTURE_2D);
  glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  glEnable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  glBindTexture(GL_TEXTURE_2D,texture[5]);
  glBegin(GL_QUADS);
  glColor3f(.2,.2,.2);
  double ang;
  int dirX;//for surface normals
  int dirZ;
  double x1, x2;
  double z1, z2;
  double y2;
  for (int x = -24; x < 24; x++){//x
    for (int z = -24; z < 24; z++){//z
        if (x <= 9.5 && x >=-9.5 && z <= 11.5 && z >= -11.5) continue;
        x1 = x  + terrain[x+24][z+24]; //terrain adds some random values to the grass
        z1 = z  + terrain[x+24][z+24];
        ang = atan2(z1, x1) * (180/3.14159265);
        x2 = x1 + 2*sin(ang);
        z2 = z1 + 2*cos(ang);
        y2 = y+1;
        if (x < 0) dirX = 1;
        else dirX = -1;
        if (z < 0) dirZ = 1;
        else dirZ = -1;
        glNormal3d(dirX*abs(x2-x1), .3, dirZ*abs(z2-z1));
        glTexCoord2f(0,0); glVertex3d(x1, y, z1);
        glTexCoord2f(1,0); glVertex3d(x2, y, z2);
        glTexCoord2f(1,1); glVertex3d(x2, y2, z2);
        glTexCoord2f(0,1); glVertex3d(x1, y2, z1);
    }
  }
  glEnd();
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_DEPTH_TEST);
}
static void court(double x, double y, double z, double dx, double dy, double dz, double th){
  float white[] = {1,1,1,1};
  float black[] = {0,0,0,1};
  glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,0.2);
  glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
  glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,black);
  //  Save transformation
  glPushMatrix();
  //  Offset, scale and rotate
  glTranslated(x,y,z);
  glRotated(th,0,1,0);
  glScaled(dx,dy,dz);
  glEnable(GL_TEXTURE_2D);
  glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  glColor4f(1,1,1, 0.01*alpha);
  glBindTexture(GL_TEXTURE_2D,texture[0]);
  //  Cube
  glBegin(GL_QUADS);
  //  Front
  double wid = 4/quadMul;
  double dep = 8/quadMul;
  for (int i = 0; i < quadMul; i++){
    for (int j = 0; j < quadMul; j++){
      glNormal3f( 0, 1, 0);
      glTexCoord2f(0,0); glVertex3f((-2+wid*(i)),0, (-4+dep*(j+1)));
      glTexCoord2f(1,0); glVertex3f((-2+wid*(i+1)),0, (-4+dep*(j+1)));
      glTexCoord2f(1,1); glVertex3f((-2+wid*(i+1)),0, (-4+dep*(j)));
      glTexCoord2f(0,1); glVertex3f((-2+wid*(i)),0, (-4+dep*(j)));
    }
  }
  glEnd();
  glPopMatrix();
  glDisable(GL_TEXTURE_2D);
}
static void backboard(double x,double y,double z,
                 double dx,double dy,double dz,
                 double th, double r, double g, double b)
{
   //  Set specular color to white
   float white[] = {1,1,1,1};
   float black[] = {0,0,0,1};
   glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,0.4);
   glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
   glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,black);
   //  Save transformation
   glPushMatrix();
   //  Offset, scale and rotate
   glTranslated(x,y,z);
   glRotated(th,0,1,0);
   glScaled(dx,dy,dz);
   //glEnable(GL_TEXTURE_2D);
   //glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
   //glBindTexture(GL_TEXTURE_2D,texture[4]);
   //blending (from example 22)

   //  Cube
   glBegin(GL_QUADS);
   //  Right
   glColor3f(.8,.8,.8);
   glNormal3f(+1, 0, 0);
   glTexCoord2f(0,0); glVertex3f(+1,-1,+1);
   glTexCoord2f(3,0); glVertex3f(+1,-1,-1);
   glTexCoord2f(3,3); glVertex3f(+1,+1,-1);
   glTexCoord2f(0,3); glVertex3f(+1,+1,+1);
   //  Left
   glColor3f(.8,.8,.8);
   glNormal3f(-1, 0, 0);
   glTexCoord2f(0,0); glVertex3f(-1,-1,-1);
   glTexCoord2f(3,0); glVertex3f(-1,-1,+1);
   glTexCoord2f(3,3); glVertex3f(-1,+1,+1);
   glTexCoord2f(0,3); glVertex3f(-1,+1,-1);
   //  Top
   glColor3f(.8,.8,.8);
   glNormal3f( 0,+1, 0);
   glTexCoord2f(0,0); glVertex3f(-1,+1,+1);
   glTexCoord2f(3,0); glVertex3f(+1,+1,+1);
   glTexCoord2f(3,3); glVertex3f(+1,+1,-1);
   glTexCoord2f(0,3); glVertex3f(-1,+1,-1);
   //  Bottom
   glColor3f(.8,.8,.8);
   glNormal3f( 0,-1, 0);
   glTexCoord2f(0,0); glVertex3f(-1,-1,-1);
   glTexCoord2f(3,0); glVertex3f(+1,-1,-1);
   glTexCoord2f(3,3); glVertex3f(+1,-1,+1);
   glTexCoord2f(0,3); glVertex3f(-1,-1,+1);

   //  End
   //White Stripes around edge
   //right side
   glNormal3f( 0, 0, 1);
   glVertex3f(+1,-1,+1);
   glVertex3f(+0.93,-1,+1);
   glVertex3f(+0.93,+1,+1);
   glVertex3f(+1,+1,+1);
   //left side
   glNormal3f( 0, 0, 1);
   glVertex3f(-1,-1,+1);
   glVertex3f(-0.93,-1,+1);
   glVertex3f(-0.93,+1,+1);
   glVertex3f(-1,+1,+1);
   //top
   glNormal3f( 0, 0, 1);
   glVertex3f(-1,+1,+1);
   glVertex3f(-1,+.93,+1);
   glVertex3f(+1,+.93,+1);
   glVertex3f(+1,+1,+1);
   //Bottom
   glNormal3f( 0, 0, 1);
   glVertex3f(-1,-1,+1);
   glVertex3f(-1,-.93,+1);
   glVertex3f(+1,-.93,+1);
   glVertex3f(+1,-1,+1);

   glEnd();
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA,GL_ONE);
   glBegin(GL_QUADS);
   //  Front
   glColor4f(1,1,1, 0.01*alpha);
   glNormal3f( 0, 0, 1);
   glTexCoord2f(0,0); glVertex3f(-1,-1, 1);
   glTexCoord2f(3,0); glVertex3f(+1,-1, 1);
   glTexCoord2f(3,3); glVertex3f(+1,+1, 1);
   glTexCoord2f(0,3); glVertex3f(-1,+1, 1);
   //  Back
   glNormal3f( 0, 0,-1);
   glTexCoord2f(0,0); glVertex3f(+1,-1,-1);
   glTexCoord2f(3,0); glVertex3f(-1,-1,-1);
   glTexCoord2f(3,3); glVertex3f(-1,+1,-1);
   glTexCoord2f(0,3); glVertex3f(+1,+1,-1);
   glEnd();

   //  Undo transofrmations
   glPopMatrix();
   //glDisable(GL_TEXTURE_2D);
   glDisable(GL_BLEND);
}

static void pole(double x,double y,double z,
                 double dx,double dy,double dz,
                 double th)
{
  float white[] = {1,1,1,1};
  float black[] = {0,0,0,1};
  glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,0.4);
  glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
  glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,black);
  //  Save transformation
  glPushMatrix();
  //  Offset, scale and rotate
  glTranslated(x,y,z);
  glRotated(th,0,1,0);
  glScaled(dx,dy,dz);
  glEnable(GL_TEXTURE_2D);
  glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);

  glBindTexture(GL_TEXTURE_2D,texture[2]);
  //Slimmer Inner Pole
  glBegin(GL_QUADS);
  glColor3f(.5,.5,.5);
  //Front
  glNormal3f( 0, 0, 1);
  glTexCoord2f(0,0); glVertex3f(-.18,0, .18);
  glTexCoord2f(1,0); glVertex3f(+.18,0, .18);
  glTexCoord2f(1,1); glVertex3f(+.18,+4.5, .18);
  glTexCoord2f(0,1); glVertex3f(-.18,+4.5, .18);
  //Right
  glNormal3f( 1, 0, 0);
  glTexCoord2f(0,0); glVertex3f(+.18,-0, +.18);
  glTexCoord2f(1,0); glVertex3f(+.18,-0, -.18);
  glTexCoord2f(1,1); glVertex3f(+.18,+4.5, -.18);
  glTexCoord2f(0,1); glVertex3f(+.18,+4.5, +.18);
  //Left
  glNormal3f( -1, 0, 0);
  glTexCoord2f(0,0); glVertex3f(-.18,-0, +.18);
  glTexCoord2f(1,0); glVertex3f(-.18,-0, -.18);
  glTexCoord2f(1,1); glVertex3f(-.18,+4.5, -.18);
  glTexCoord2f(0,1); glVertex3f(-.18,+4.5, +.18);
  //back
  glNormal3f( 0, 0, -1);
  glTexCoord2f(0,0); glVertex3f(-.18,0, -.18);
  glTexCoord2f(1,0); glVertex3f(+.18,0, -.18);
  glTexCoord2f(1,1); glVertex3f(+.18,+4.5, -.18);
  glTexCoord2f(0,1); glVertex3f(-.18,+4.5, -.18);
  //Top
  glNormal3f( 0, 1, 0);
  glTexCoord2f(0,0); glVertex3f(-.18,4.5, -.18);
  glTexCoord2f(1,0); glVertex3f(+.18,4.5, -.18);
  glTexCoord2f(1,1); glVertex3f(+.18,+4.5, +.18);
  glTexCoord2f(0,1); glVertex3f(-.18,+4.5, +.18);


  //Thicker lower pole
  glColor3f(.5,.5,.5);
  //Front
  glNormal3f( 0, 0, 1);
  glTexCoord2f(0,0); glVertex3f(-.25,0, .25);
  glTexCoord2f(1,0); glVertex3f(+.25,0, .25);
  glTexCoord2f(1,1); glVertex3f(+.25,+2.8, .25);
  glTexCoord2f(0,1); glVertex3f(-.25,+2.8, .25);
  //Right
  glNormal3f( 1, 0, 0);
  glTexCoord2f(0,0); glVertex3f(+.25,-0, +.25);
  glTexCoord2f(1,0); glVertex3f(+.25,-0, -.25);
  glTexCoord2f(1,1); glVertex3f(+.25,+2.8, -.25);
  glTexCoord2f(0,1); glVertex3f(+.25,+2.8, +.25);
  //Left
  glNormal3f( -1, 0, 0);
  glTexCoord2f(0,0); glVertex3f(-.25,-0, +.25);
  glTexCoord2f(1,0); glVertex3f(-.25,-0, -.25);
  glTexCoord2f(1,1); glVertex3f(-.25,+2.8, -.25);
  glTexCoord2f(0,1); glVertex3f(-.25,+2.8, +.25);
  //back
  glNormal3f( 0, 0, -1);
  glTexCoord2f(0,0); glVertex3f(-.25,0, -.25);
  glTexCoord2f(1,0); glVertex3f(+.25,0, -.25);
  glTexCoord2f(1,1); glVertex3f(+.25,+2.8, -.25);
  glTexCoord2f(0,1); glVertex3f(-.25,+2.8, -.25);
  //Top
  glNormal3f( 0, 1, 0);
  glTexCoord2f(0,0); glVertex3f(-.25,2.8, -.25);
  glTexCoord2f(1,0); glVertex3f(+.25,2.8, -.25);
  glTexCoord2f(1,1); glVertex3f(+.25,+2.8, +.25);
  glTexCoord2f(0,1); glVertex3f(-.25,+2.8, +.25);

  //Flared base
  //Front
  glColor3f(.5,.5,.5);
  glTexCoord2f(0,0); glVertex3f(-.4,0, .4);
  glTexCoord2f(1,0); glVertex3f(+.4,0, .4);
  glTexCoord2f(1,1); glVertex3f(+.25,+1, .25);
  glTexCoord2f(0,1); glVertex3f(-.25,+1, .25);
  //Right
  glNormal3f( 1, .2, 0);
  glTexCoord2f(0,0); glVertex3f(+.4,-0, +.4);
  glTexCoord2f(1,0); glVertex3f(+.4,-0, -.4);
  glTexCoord2f(1,1); glVertex3f(+.25,+1, -.25);
  glTexCoord2f(0,1); glVertex3f(+.25,+1, +.25);
  //Left
  glNormal3f( -1, .2, 0);
  glTexCoord2f(0,0); glVertex3f(-.4,-0, +.4);
  glTexCoord2f(1,0); glVertex3f(-.4,-0, -.4);
  glTexCoord2f(1,1); glVertex3f(-.25,+1, -.25);
  glTexCoord2f(0,1); glVertex3f(-.25,+1, +.25);
  //back
  glNormal3f( 0, .2, -1);
  glTexCoord2f(0,0); glVertex3f(-.4,0, -.4);
  glTexCoord2f(1,0); glVertex3f(+.4,0, -.4);
  glTexCoord2f(1,1); glVertex3f(+.25,+1, -.25);
  glTexCoord2f(0,1); glVertex3f(-.25,+1, -.25);
  glEnd();
  glDisable(GL_TEXTURE_2D);
  glBegin(GL_QUADS);
  //support bars from pole to backboard
  glColor3f(.2,.2,.2);
  //left
  //topleft
  glNormal3f(0,3.5,-1.3);
  glTexCoord2f(0,0); glVertex3f(-.18,2.9, -.6);
  glTexCoord2f(3,0); glVertex3f(-.28,2.9, -.6);
  glTexCoord2f(3,3); glVertex3f(-.28,5.4, +.9);
  glTexCoord2f(0,3); glVertex3f(-.18,5.4, +.9);
    //bottomleft
  glNormal3f(0,-3.5,1.3);
  glTexCoord2f(0,0); glVertex3f(-.18,2.8, -.6);
  glTexCoord2f(3,0); glVertex3f(-.28,2.8, -.6);
  glTexCoord2f(3,3); glVertex3f(-.28,5.3, +.9);
  glTexCoord2f(0,3); glVertex3f(-.18,5.3, +.9);
    //leftleft
  glNormal3f(-1,0,0);
  glTexCoord2f(0,0); glVertex3f(-.28,2.9, -.6);
  glTexCoord2f(3,0); glVertex3f(-.28,2.8, -.6);
  glTexCoord2f(3,3); glVertex3f(-.28,5.3, +.9);
  glTexCoord2f(0,3); glVertex3f(-.28,5.4, +.9);
  //rightleft
  glNormal3f(1,0,0);
  glTexCoord2f(0,0); glVertex3f(-.18,2.9, -.6);
  glTexCoord2f(3,0); glVertex3f(-.18,2.8, -.6);
  glTexCoord2f(3,3); glVertex3f(-.18,5.3, +.9);
  glTexCoord2f(0,3); glVertex3f(-.18,5.4, +.9);
  //Right
  //topright
  glNormal3f(0,3.5,-1.3);
  glTexCoord2f(0,0); glVertex3f(.18,2.9, -.6);
  glTexCoord2f(3,0); glVertex3f(.28,2.9, -.6);
  glTexCoord2f(3,3); glVertex3f(.28,5.4, +.9);
  glTexCoord2f(0,3); glVertex3f(.18,5.4, +.9);
    //bottomright
  glNormal3f(0,-3.5,1.3);
  glTexCoord2f(0,0); glVertex3f(.18,2.8, -.6);
  glTexCoord2f(3,0); glVertex3f(.28,2.8, -.6);
  glTexCoord2f(3,3); glVertex3f(.28,5.3, +.9);
  glTexCoord2f(0,3); glVertex3f(.18,5.3, +.9);
    //leftright
  glNormal3f(1,0,0);
  glTexCoord2f(0,0); glVertex3f(.28,2.9, -.6);
  glTexCoord2f(3,0); glVertex3f(.28,2.8, -.6);
  glTexCoord2f(3,3); glVertex3f(.28,5.3, +.9);
  glTexCoord2f(0,3); glVertex3f(.28,5.4, +.9);
  //rightright
  glNormal3f(-1,0,0);
  glTexCoord2f(0,0); glVertex3f(.18,2.9, -.6);
  glTexCoord2f(3,0); glVertex3f(.18,2.8, -.6);
  glTexCoord2f(3,3); glVertex3f(.18,5.3, +.9);
  glTexCoord2f(0,3); glVertex3f(.18,5.4, +.9);

  //backboard plate
  //front
  glNormal3f(0,0,1);
  glTexCoord2f(0,0); glVertex3f(-.28,5.1, +.9);
  glTexCoord2f(3,0); glVertex3f(.28,5.1, +.9);
  glTexCoord2f(3,3); glVertex3f(.28,5.6, +.9);
  glTexCoord2f(0,3); glVertex3f(-.28,5.6, +.9);
  //back
  glNormal3f(0,0,-1);
  glTexCoord2f(0,0); glVertex3f(-.28,5.1, +.87);
  glTexCoord2f(3,0); glVertex3f(.28,5.1, +.87);
  glTexCoord2f(3,3); glVertex3f(.28,5.6, +.87);
  glTexCoord2f(0,3); glVertex3f(-.28,5.6, +.87);
  //sides
  glNormal3f(0,0,-1);
  glTexCoord2f(0,0); glVertex3f(.28,5.1, +.9);
  glTexCoord2f(3,0); glVertex3f(.28,5.1, +.87);
  glTexCoord2f(3,3); glVertex3f(.28,5.6, +.87);
  glTexCoord2f(0,3); glVertex3f(.28,5.6, +.9);
  glNormal3f(0,0,-1);
  glTexCoord2f(0,0); glVertex3f(-.28,5.1, +.9);
  glTexCoord2f(3,0); glVertex3f(-.28,5.1, +.87);
  glTexCoord2f(3,3); glVertex3f(-.28,5.6, +.87);
  glTexCoord2f(0,3); glVertex3f(-.28,5.6, +.9);
  glNormal3f(0,0,-1);
  glTexCoord2f(0,0); glVertex3f(-.28,5.6, +.9);
  glTexCoord2f(3,0); glVertex3f(-.28,5.6, +.87);
  glTexCoord2f(3,3); glVertex3f(.28,5.6, +.87);
  glTexCoord2f(0,3); glVertex3f(.28,5.6, +.9);
  glNormal3f(0,0,-1);
  glTexCoord2f(0,0); glVertex3f(-.28,5.1, +.9);
  glTexCoord2f(3,0); glVertex3f(-.28,5.1, +.87);
  glTexCoord2f(3,3); glVertex3f(.28,5.1, +.87);
  glTexCoord2f(0,3); glVertex3f(.28,5.1, +.9);

  glEnd();

  //screws in poles
  glColor3f(1,1,1);
  DrawCircle(-.28, 3.8, 0.0, .05, 10, 3);
  DrawCircle(.28, 3.8, 0.0, .05, 10, 3);
  glPopMatrix();
}

static void basketballHoop(double x, double y, double z, double dx, double dy, double dz, double th)
{


  //bH is backboard Height
  int bH = y + 1;
  //bD is backboard depth
  int bD = z + 1;
  float white[] = {1,1,1,1};
  float black[] = {0,0,0,1};
  glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,shiny);
  glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
  glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,black);
  glEnable(GL_TEXTURE_2D);
  glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  glColor3f(1,1,1);
  glBindTexture(GL_TEXTURE_2D,texture[2]);
  //draw the hinge for the rim
  glPushMatrix();
  glTranslated(x,bH,bD);
  glRotated(th,0,1,0);
  glScaled(dx,dy,dz);
  glPolygonOffset(1,1);
  glBegin(GL_QUADS);
  //top of hinge
  glColor3f(1,.5,0);
  glNormal3f(0, 1, 0);
  glTexCoord2f(0,0); glVertex3f(-.4,4.4, .3);
  glTexCoord2f(1,0); glVertex3f(-.4,4.4, .1);
  glTexCoord2f(1,1); glVertex3f(.4,4.4, .1);
  glTexCoord2f(0,1); glVertex3f(.4,4.4, .3);
  //front side of hinge
  glNormal3f(0, -.4, .2);
  glTexCoord2f(0,0); glVertex3f(-.4,4.4, .3);
  glTexCoord2f(1,0); glVertex3f(-.4,4.0, .1);
  glTexCoord2f(1,1); glVertex3f(.4,4.0, .1);
  glTexCoord2f(0,1); glVertex3f(.4,4.4, .3);
  glEnd();

  //draw the protruding part of the hinge
  glBegin( GL_TRIANGLES );
  //left
  glColor3f(1,.5,0);
  glNormal3f(-1, 0, 0);
  glTexCoord2f(0.0  ,0.0); glVertex3f(-.4,4.4,0.1);
  glTexCoord2f(1  ,0.0); glVertex3f(-.4,4,0.1);
  glTexCoord2f(1/2,1); glVertex3f(-.4,4.4,0.3);
  //right
  glNormal3f(1, 0, 0);
  glTexCoord2f(0.0  ,0.0); glVertex3f(.4,4.4,0.1);
  glTexCoord2f(1  ,0.0); glVertex3f(.4,4,0.1);
  glTexCoord2f(1/2,1); glVertex3f(.4,4.4,0.3);
  glEnd();

  glBegin(GL_QUAD_STRIP);
  glColor3f(1,.5,0);
  double hoopRadius = 0.49;
  double ringRadius = 0.04;
  //the following draws the hoop, I made the point/normal math myself
 //the code draws a complete loop using quadstrips
  for(int ringAngle = 0; ringAngle < 360 ; ringAngle += 10)//represents the orbital around the ring itself
  { //this for loop determines the angle of the two points being drawn with respect to the outer ring
    //this is for the loops made around the band, not the larger ring itself

    //each quadstrip makes a complete loop at a constant elevation/radius from the center of the ring
      for(int innerAngle = 0; innerAngle <= 360; innerAngle += 10)
      {//this for loop finds the x,y, and z points that correlate to the current angle relative to the center of the ring, while also considering the angle around the band
        float X[] = {Cos(innerAngle)*(hoopRadius + ringRadius*Cos(ringAngle)), Cos(innerAngle)*(hoopRadius + ringRadius*Cos(ringAngle+11))};
        float Y[] = {Sin(ringAngle)*ringRadius + 4.4, Sin(ringAngle+11)*ringRadius + 4.4};
        float Z[] = {Sin(innerAngle)*(hoopRadius + ringRadius*Cos(ringAngle))+.75, Sin(innerAngle)*(hoopRadius + ringRadius*Cos(ringAngle+11))+.75};
        glNormal3d(Cos(innerAngle)*Cos(ringAngle), Sin(ringAngle),Sin(innerAngle)*Sin(ringAngle));
        if (innerAngle % 20 == 0){
          glTexCoord2f(0,0); glVertex3d(X[0], Y[0], Z[0]);
          glTexCoord2f(.05,0); glVertex3d(X[1], Y[1], Z[1]);
        }
        else{
          glTexCoord2f(.05,.05); glVertex3d(X[0], Y[0], Z[0]);
          glTexCoord2f(0,.05); glVertex3d(X[1], Y[1], Z[1]);
        }
      }
  }
  glEnd();
  //draw the backboard square
    //left side of square
  glBindTexture(GL_TEXTURE_2D, texture[3]);
  glBegin(GL_QUADS);
  glColor3f(1,1,1);
  glNormal3f(0, 0, 1);
  glTexCoord2f(0,0); glVertex3f(-.65,4.3, .14);
  glTexCoord2f(1,0); glVertex3f(-.55,4.3, .14);
  glTexCoord2f(1,1); glVertex3f(-.55,5.2, .14);
  glTexCoord2f(0,1); glVertex3f(-.65,5.2, .14);
    //right side of square
  glNormal3f(0, 0, 1);
  glTexCoord2f(0,0); glVertex3f(.65,4.3, .14);
  glTexCoord2f(1,0); glVertex3f(.55,4.3, .14);
  glTexCoord2f(1,1); glVertex3f(.55,5.2, .14);
  glTexCoord2f(0,1); glVertex3f(.65,5.2, .14);
    //top of square
  glNormal3f(0, 0, 1);
  glTexCoord2f(0,0); glVertex3f(-.65,5.2, .14);
  glTexCoord2f(1,0); glVertex3f(.65,5.2, .14);
  glTexCoord2f(1,1); glVertex3f(.65,5.3, .14);
  glTexCoord2f(0,1); glVertex3f(-.65,5.3, .14);
    //bottom of square
  glNormal3f(0, 0, 1);
  glTexCoord2f(0,0); glVertex3f(-.65,4.3, .14);
  glTexCoord2f(1,0); glVertex3f(.65,4.3, .14);
  glTexCoord2f(1,1); glVertex3f(.65,4.4, .14);
  glTexCoord2f(0,1); glVertex3f(-.65,4.4, .14);
  glEnd();
  glDisable(GL_TEXTURE_2D);
  /*
   * draw the hoop (ring)
   * the implementation was found at https://stackoverflow.com/questions/13789269/draw-a-ring-with-different-color-sector
   * I made a slight change to make it 3d instead of a 2d hoop by adding a third vertex with a lower y value every loop
   */

  //net


  //Make the pole
  //TODO: TEXTURE FOR POLE
  //double wid = 0.12;
  // glBegin(GL_QUAD_STRIP);
  // glColor3f(.8,.8,.8);
  // for (int th=0;th<=360;th+=30)
  // {
  //   glNormal3d(wid*Cos(th), 0,wid*Sin(th));
  //   if (th % 20 == 0){
  //     glTexCoord2f(0,0); glVertex3d(wid*Cos(th), 0,wid*Sin(th) - (wid*2));
  //     glTexCoord2f(1,0); glVertex3d(wid*Cos(th), 3,wid*Sin(th) - (wid*2));
  //   }
  //   else{
  //     glTexCoord2f(1,1); glVertex3d(wid*Cos(th), 0,wid*Sin(th) - (wid*2));
  //     glTexCoord2f(0,1); glVertex3d(wid*Cos(th), 3,wid*Sin(th) - (wid*2));
  //   }
  // }

  //  Undo transofrmations
  glPopMatrix();
  pole(x,y,z,dx,dy,dz,th);
  backboard(x,bH+(5*dy),bD,dx*2,dy*1.5,dz*0.1,th, 0.9, 0.9, 0.9);

  glPushMatrix();
  glTranslated(x,bH,bD);
  glRotated(th,0,1,0);
  glScaled(dx,dy,dz);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, texture[1]);
  glEnable(GL_BLEND);
  glBegin(GL_QUAD_STRIP);
  glColor3f(.8,.8,.8);
  for (int th=0;th<=360;th+=30)
  {
    glNormal3d(hoopRadius*Cos(th), 0,hoopRadius*Sin(th));
    if (th % 20 == 0){
      glTexCoord2f(th/360.0,1); glVertex3d(hoopRadius*Cos(th), 3.6,1.75+hoopRadius*Sin(th) - (hoopRadius*2));
      glTexCoord2f(th/360.0,0); glVertex3d(hoopRadius*Cos(th), 4.4,1.75+hoopRadius*Sin(th) - (hoopRadius*2));
    }
    else{

      glTexCoord2f((th)/360.0,1); glVertex3d(hoopRadius*Cos(th), 3.6,1.75+hoopRadius*Sin(th) - (hoopRadius*2));
      glTexCoord2f((th)/360.0,0); glVertex3d(hoopRadius*Cos(th), 4.4,1.75+hoopRadius*Sin(th) - (hoopRadius*2));
    }
  }
  glEnd();
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
  glPopMatrix();
}
/*
 *  OpenGL (GLUT) calls this routine to display the scene
 */
void display()
{

   //  Set background color, then erase the window and the depth buffer
   //glClearColor(0.435,0.76,0.918,0.8);
   glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

   //  Enable Z-buffering in OpenGL
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_POLYGON_OFFSET_FILL);
   //  Undo previous transformations
   glLoadIdentity();


   //  close or far perspective
  if (mode == 0){
    double Ex = -6*dim*Sin(th)*Cos(ph);
    double Ey = +6*dim        *Sin(ph);
    double Ez = +6*dim*Cos(th)*Cos(ph);
    gluLookAt(Ex,Ey,Ez , 0,0,0 , 0,Cos(ph),0);
  }
  else if (mode == 1){
    double Ex = -2*dim*Sin(th)*Cos(ph);
    double Ey = +2*dim        *Sin(ph);
    double Ez = +2*dim*Cos(th)*Cos(ph);
    gluLookAt(Ex,Ey,Ez , 0,0,0 , 0,Cos(ph),0);
  }

  //sets the fpp x and z values so you are put in the correct space when switching to fpp
  Sky(10*dim);//24x24
   //taken from ex13, creates the ball light source
   if (light)
   {
      //  Translate intensity to color vectors
      float Ambient[]   = {0.01*ambient ,0.01*ambient ,0.01*ambient ,1.0};
      float Diffuse[]   = {0.01*diffuse ,0.01*diffuse ,0.01*diffuse ,1.0};
      float Specular[]  = {0.01*specular,0.01*specular,0.01*specular,1.0};
      //  Light position
      float Position[]  = {distance*Cos(zh),ylight,distance*Sin(zh),1.0};
      //  Draw light position as ball (still no lighting here)
      glColor3f(1,1,1);
      ball(Position[0],Position[1],Position[2] , 0.1);
      //  OpenGL should normalize normal vectors
      glEnable(GL_NORMALIZE);
      //  Enable lighting
      glEnable(GL_LIGHTING);
      //  Location of viewer for specular calculations
      glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,local);
      //  glColor sets ambient and diffuse color materials
      glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
      glEnable(GL_COLOR_MATERIAL);
      //  Enable light 0
      glEnable(GL_LIGHT0);
      //  Set ambient, diffuse, specular components and position of light 0
      glLightfv(GL_LIGHT0,GL_AMBIENT ,Ambient);
      glLightfv(GL_LIGHT0,GL_DIFFUSE ,Diffuse);
      glLightfv(GL_LIGHT0,GL_SPECULAR,Specular);
      glLightfv(GL_LIGHT0,GL_POSITION,Position);
   }

   //draw the objects
   court(0,-4,0,4,1,2.5,0); //16*20
   grassArea(0,-4.8,0);
   grass2D(-4.8);
   basketballHoop(0,-4,-6,1,.8,1,0);

   //  Display parameters
   glWindowPos2i(5,25);
   Print("Angle=%d,%d FOV=%d Light=%s",
     th,ph,fov,light?"On":"Off");
   if (light)
   {
      glWindowPos2i(5,5);
      Print("Ambient=%d  Diffuse=%d Specular=%d Emission=%d Shininess=%.0f",ambient,diffuse,specular,emission,shiny);
   }
   //  Render the scene and make it visible
   ErrCheck("display");
   glFlush();
   glutSwapBuffers();
   //I put these here because the light wouldn't start moving until I pressed a button, unless I put the following functions below
   Project();
   glutPostRedisplay();
   glutIdleFunc(move?idle:NULL);
}

/*
 *  GLUT calls this routine when an arrow key is pressed
 */
void special(int key,int x,int y) //From class examples
{
   //  Right arrow key - increase angle by 5 degrees
   if (key == GLUT_KEY_RIGHT)
      th += 5;
   //  Left arrow key - decrease angle by 5 degrees
   else if (key == GLUT_KEY_LEFT)
      th -= 5;
   //  Up arrow key - increase elevation by 5 degrees
   else if (key == GLUT_KEY_UP && ph <= 85)
      ph += 5;
   //  Down arrow key - decrease elevation by 5 degrees
   else if (key == GLUT_KEY_DOWN && ph >= 10)
      ph -= 5;
   //  PageUp key - increase dim
   else if (key == GLUT_KEY_PAGE_UP)
      dim += 0.1;
   //  PageDown key - decrease dim
   else if (key == GLUT_KEY_PAGE_DOWN && dim>1)
      dim -= 0.1;
   //  Toggle ball increment
   else if (key == GLUT_KEY_F8)
     inc = (inc==10)?3:10;
   //  Keep angles to +/-360 degrees
   th %= 360;
   ph %= 360;
   //  Update projection
   Project();
   //  Tell GLUT it is necessary to redisplay the scene
   glutPostRedisplay();
   glutIdleFunc(move?idle:NULL);
}

/*
 *  GLUT calls this routine when a key is pressed
 */
void key(unsigned char ch,int x,int y)
{
  //these are reused from ex13 (at least the format of them, some variables are different)
  //  Exit on ESC
  if (ch == 27)
     exit(0);
  //  Reset view angle
  else if (ch == '0')
     th = ph = 0;
  //  Toggle lighting
  else if (ch == 'l' || ch == 'L')
     light = 1-light;
  //  Toggle light movement
  else if (ch == 'm' || ch == 'M')
     move = 1-move;
  else if (ch == '2')
     mode = 0;
  else if (ch == '1')
    mode = 1;
  //  Change field of view angle
  else if (ch == '-' && ch>1)
     fov--;
  else if (ch == '+' && ch<179)
     fov++;
  //  Light elevation
  else if (ch=='[')
    ylight -= 1;
  else if (ch==']')
     ylight += 1;
  //  Ambient level
  else if (ch=='a' && ambient>0)
     ambient -= 5;
  else if (ch=='A' && ambient<100)
     ambient += 5;
  //  Diffuse level
  else if (ch=='d' && diffuse>0)
     diffuse -= 5;
  else if (ch=='D' && diffuse<100)
     diffuse += 5;
  //  Specular level
  else if (ch=='s' && specular>0)
     specular -= 5;
  else if (ch=='S' && specular<100)
     specular += 5;
  //  Emission level
  else if (ch=='e' && emission>0)
     emission -= 5;
  else if (ch=='E' && emission<100)
     emission += 5;
  else if (ch=='y' && quadMul>=2.0)
     quadMul -= 1.0;
  else if (ch=='Y' && quadMul<=10)
     quadMul += 1.0;
  //  Translate shininess power to value (-1 => 0)
  shiny = shininess<0 ? 0 : pow(2.0,shininess);
  //  Reproject
  Project();
  //  Animate if requested
  glutIdleFunc(move?idle:NULL);
  //  Tell GLUT it is necessary to redisplay the scene
  glutPostRedisplay();
}

/*
 *  GLUT calls this routine when the window is resized
 */
void reshape(int width,int height) //from class examples
{
   //  Ratio of the width to the height of the window
   asp = (height>0) ? (double)width/height : 1;
   //  Set the viewport to the entire window
   glViewport(0,0, RES*width,RES*height);
   //  Set projection
   Project();
}

/*
 *  Start up GLUT and tell it what to do
 */
int main(int argc,char* argv[])
{
   //  Initialize GLUT
   glutInit(&argc,argv);
   //  Request double buffered, true color window with Z buffering at 600x600
   glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
   glutInitWindowSize(600,600);
   glutCreateWindow("Final");
#ifdef USEGLEW
   //  Initialize GLEW
   if (glewInit()!=GLEW_OK) Fatal("Error initializing GLEW\n");
#endif
   //  Set callbacks
   glutDisplayFunc(display);
   glutReshapeFunc(reshape);
   glutSpecialFunc(special);
   glutKeyboardFunc(key);
   createTerrain();
   //  Pass control to GLUT so it can interact with the user


   texture[0] = LoadTexBMP("asphalt.bmp");
   texture[1] = LoadTexBMP("net.bmp");
   texture[2] = LoadTexBMP("metal.bmp");
   texture[3] = LoadTexBMP("paint.bmp");
   texture[4] = LoadTexBMP("grass.bmp");
   texture[5] = LoadTexBMP("grassy.bmp");
   sky[0] = LoadTexBMP("skyLong.bmp");
   sky[1] = LoadTexBMP("skyTop.bmp");
   glutMainLoop();
   return 0;
}
