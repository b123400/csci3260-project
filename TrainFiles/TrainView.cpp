//  Train Project
// TrainView class implementation
// see the header for details
// look for TODO: to see things you want to add/change
// 

//////////////////////////////////////////////////////////////////////////
//Please fill your name and student ID
//Name: 
//StuID: 
//////////////////////////////////////////////////////////////////////////

#include "TrainView.H"
#include "TrainWindow.H"

#include "Utilities/3DUtils.H"

#include <Fl/fl.h>
#include <math.h>

// we will need OpenGL, and OpenGL needs windows.h
#include <windows.h>
#include "GL/gl.h"
#include "GL/glu.h"

// yk
#include "../Utilities/bitmap.h"
int texground_w, texground_h;
int texball_w, texball_h;
int texcar_w, texcar_h;

float angle1 = 0;
float angle2 = 0;
bool angleFlag1 = false;
bool angleFlag2 = false;
float angle3 = 0;
float M_PI = 3.1415926535;
static bool isFoggy = false;

float rot_ball = 0;
GLUquadricObj *ball = gluNewQuadric();

void TrainView::toggleFog()
{
	printf("fog toggle\n");
	isFoggy = !isFoggy;
};

//Load the BMP file
GLubyte* TextureLoadBitmap(char *filename, int *w, int *h)		/* I - Bitmap file to load */
{
	BITMAPINFO	*info;				/* Bitmap information */
	void		*bits;				/* Bitmap pixel bits */
	GLubyte	*rgb;				/* Bitmap RGB pixels */
	GLubyte   err = '0';

	/*
	* Try loading the bitmap and converting it to RGB...
	*/

	bits = LoadDIBitmap(filename, &info);
	if(bits==NULL) 
		return(NULL);
	rgb = ConvertRGB(info, bits);
	if (rgb == NULL)
	{
		free(info);
		free(bits);
	};

	printf("%s: %d %d\n", filename, info->bmiHeader.biWidth, info->bmiHeader.biHeight);
	printf("read %s successfully\n", filename);
	*w = info->bmiHeader.biWidth;
	*h = info->bmiHeader.biHeight;

	/*
	* Free the bitmap and RGB images, then return 0 (no errors).
	*/

	free(info);
	free(bits);
	return (rgb);

}

GLubyte *texground = TextureLoadBitmap("grass.bmp", &texground_w, &texground_h);
GLubyte *texball = TextureLoadBitmap("ball.bmp", &texball_w, &texball_h);
GLubyte *texcar = TextureLoadBitmap("car.bmp", &texcar_w, &texcar_h);

TrainView::TrainView(int x, int y, int w, int h, const char* l) : Fl_Gl_Window(x,y,w,h,l)
{
	mode( FL_RGB|FL_ALPHA|FL_DOUBLE | FL_STENCIL );

	resetArcball();
	curveMode = CurveModeCatmull;
	trackMode = TrackModeNormal;
	trainHeadPoint = NULL;
	trainHeadTarget = NULL;
}

void TrainView::resetArcball()
{
	// set up the camera to look at the world
	// these parameters might seem magical, and they kindof are
	// a little trial and error goes a long way
	arcball.setup(this,40,250,.2f,.4f,0);
}

// FlTk Event handler for the window
// TODO: if you want to make the train respond to other events 
// (like key presses), you might want to hack this.
int TrainView::handle(int event)
{
	// see if the ArcBall will handle the event - if it does, then we're done
	// note: the arcball only gets the event if we're in world view
	if (tw->worldCam->value())
		if (arcball.handle(event)) return 1;

	// remember what button was used
	static int last_push;

	switch(event) {
		case FL_PUSH:
			last_push = Fl::event_button();
			if (last_push == 1) {
				doPick();
				damage(1);
				return 1;
			};
			break;
		case FL_RELEASE:
			damage(1);
			last_push=0;
			return 1;
		case FL_DRAG:
			if ((last_push == 1) && (selectedCube >=0)) {
				ControlPoint &cp = world->points[selectedCube];

				double r1x, r1y, r1z, r2x, r2y, r2z;
				getMouseLine(r1x,r1y,r1z, r2x,r2y,r2z);

				double rx, ry, rz;
				mousePoleGo(r1x,r1y,r1z, r2x,r2y,r2z, 
						  static_cast<double>(cp.pos.x), 
						  static_cast<double>(cp.pos.y),
						  static_cast<double>(cp.pos.z),
						  rx, ry, rz,
						  (Fl::event_state() & FL_CTRL) != 0);
				cp.pos.x = (float) rx;
				cp.pos.y = (float) ry;
				cp.pos.z = (float) rz;
				damage(1);
			}
			break;
			// in order to get keyboard events, we need to accept focus
		case FL_FOCUS:
			return 1;
		case FL_ENTER:	// every time the mouse enters this window, aggressively take focus
				focus(this);
				break;
		case FL_KEYBOARD:
		 		int k = Fl::event_key();
				int ks = Fl::event_state();
				if (k=='p') {
					if (selectedCube >=0) 
						printf("Selected(%d) (%g %g %g) (%g %g %g)\n",selectedCube,
							world->points[selectedCube].pos.x,world->points[selectedCube].pos.y,world->points[selectedCube].pos.z,
							world->points[selectedCube].orient.x,world->points[selectedCube].orient.y,world->points[selectedCube].orient.z);
					else
						printf("Nothing Selected\n");
					return 1;
				};
				break;
	}

	return Fl_Gl_Window::handle(event);
}

void TrainView::drawGround()
{
	// ground
	glPushMatrix();
	glEnable(GL_TEXTURE_2D);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, texground_w, texground_h, 0, GL_RGB, GL_UNSIGNED_BYTE, texground);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT ); // no
	glBegin(GL_QUADS);
		glTexCoord2f(0.0,0.0); glVertex3f(-100, 0, -100);
		glTexCoord2f(0.0,1.0); glVertex3f(-100, 0, 100);
		glTexCoord2f(1.0,1.0); glVertex3f(100, 0, 100);
		glTexCoord2f(1.0,0.0); glVertex3f(100, 0, -100);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	glPopMatrix();
}

// this is the code that actually draws the window
// it puts a lot of the work into other routines to simplify things
void TrainView::draw()
{

	glViewport(0,0,w(),h());

	// clear the window, be sure to clear the Z-Buffer too
	glClearColor(0,0,.3f,0);		// background should be blue
	// we need to clear out the stencil buffer since we'll use
	// it for shadows
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glEnable(GL_DEPTH);

	// Blayne prefers GL_DIFFUSE
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	// prepare for projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	setProjection();		// put the code to set up matrices here

	// TODO: you might want to set the lighting up differently
	// if you do, 
	// we need to set up the lights AFTER setting up the projection

	// enable the lighting
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	// top view only needs one light
	if (tw->topCam->value()) {
		glDisable(GL_LIGHT1);
		glDisable(GL_LIGHT2);
	} else {
		glEnable(GL_LIGHT1);
		glEnable(GL_LIGHT2);
	}
	// set the light parameters
	GLfloat lightPosition1[] = {0,1,1,0}; // {50, 200.0, 50, 1.0};
	GLfloat lightPosition2[] = {1, 0, 0, 0};
	GLfloat lightPosition3[] = {0, -1, 0, 0};
	GLfloat yellowLight[] = {0.5f, 0.5f, .1f, 1.0};
	GLfloat whiteLight[] = {1.0f, 1.0f, 1.0f, 1.0};
	GLfloat blueLight[] = {.1f,.1f,.3f,1.0};
	GLfloat grayLight[] = {.3f, .3f, .3f, 1.0};

	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition1);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, whiteLight);
	glLightfv(GL_LIGHT0, GL_AMBIENT, grayLight);

	glLightfv(GL_LIGHT1, GL_POSITION, lightPosition2);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, yellowLight);

	glLightfv(GL_LIGHT2, GL_POSITION, lightPosition3);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, blueLight);

	// yk texture

	// Texture mapping setting for Microsoft's OpenGL implementation
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);

	// Texture mapping parameters for filter and repeatance
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);  
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	// now draw the ground plane
	setupFloor();
	//glDisable(GL_LIGHTING);
	//drawFloor(200,10);
	glEnable(GL_LIGHTING);
	setupObjects();

	// we draw everything twice - once for real, and then once for
	// shadows
	drawStuff();
	// ground
	drawGround();

	// this time drawing is for shadows (except for top view)
	if (!tw->topCam->value()) {
		setupShadows();
		drawStuff(true);
		// ground
		drawGround();
		unsetupShadows();
	}

	float fogColor[] = {0.7, 0.7, 0.7, 0.1};
	glFogf(GL_FOG_MODE, GL_EXP);
	if (isFoggy == true) {
		//printf("fog on\n");
		glEnable(GL_FOG);
	} else {
		//printf("fog off\n");
		glDisable(GL_FOG);
	}
	glFogfv(GL_FOG_COLOR, fogColor);
	glFogf(GL_FOG_DENSITY, 0.02f);
	glFogf(GL_FOG_START, 20);
	glFogf(GL_FOG_END, 200);

		// yk running
		
		// angle 1
		if (rand() % 10 == 1)  angleFlag1 = !angleFlag1;
		if (angleFlag1 == 0)
		{
			angle1 += 0.1;
		}
		else
		{
			angle1 -= 0.1;
		}
		if (angle1 > 10 || angle1 < -10) angleFlag1 = !angleFlag1;

		// angle 2
		if (rand() % 10 == 1)  angleFlag2 = !angleFlag2;
		if (angleFlag2 == 0)
		{
			angle2 += 0.1;
		}
		else
		{
			angle2 -= 0.1;
		}
		if (angle2 > 10 || angle2 < -10) angleFlag2 = !angleFlag2;

		// angle3
		angle3 += 0.6;
		if (angle3 >= 360) angle3 = 0;

		glFlush();
}

// note: this sets up both the Projection and the ModelView matrices
// HOWEVER: it doesn't clear the projection first (the caller handles
// that) - its important for picking
void TrainView::setProjection()
{
	// compute the aspect ratio (we'll need it)
	float aspect = static_cast<float>(w()) / static_cast<float>(h());

	if (tw->worldCam->value()) {
		arcball.setProjection(false);
	} else if (tw->topCam->value()) {
		float wi,he;
		if (aspect >= 1) {
			wi = 110;
			he = wi/aspect;
		} else {
			he = 110;
			wi = he*aspect;
		}
		glMatrixMode(GL_PROJECTION);
		glOrtho(-wi,wi,-he,he,200,-200);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(-90,1,0,0);
	} else {
		// TODO: put code for train view projection here!
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45.0, aspect, 1.0, 900.0);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt(
			(*trainHeadPoint).x, (*trainHeadPoint).y+1, (*trainHeadPoint).z, // eye
			(*trainHeadTarget).x, (*trainHeadTarget).y+1, (*trainHeadTarget).z, // center
			0, 1, 0); // up vector
	}
}

float catmullRom( float u, float x0, float x1, float x2, float x3 )
{
    float u2 = u * u;
    float u3 = u2 * u;
    return ((2 * x1) + 
           (-x0 + x2) * u + 
           (2*x0 - 5*x1 + 4*x2 - x3) * u2 + 
           (-x0 + 3*x1 - 3*x2 + x3) * u3) * 0.5f;
}

Pnt3f* catmullPoint( float u, Pnt3f p1, Pnt3f p2, Pnt3f p3, Pnt3f p4 )
{
	return new Pnt3f(
		catmullRom(u, p1.x, p2.x, p3.x, p4.x),
		catmullRom(u, p1.y, p2.y, p3.y, p4.y),
		catmullRom(u, p1.z, p2.z, p3.z, p4.z));
}


void drawCar(int index, bool drawingShadow) {
	switch (index % 3) {
	case 0:
		glColor3f(1.0, 0.0, 0.0);
		break;
	case 1:
		glColor3f(0.0, 1.0, 0.0);
		break;
	case 2:
		glColor3f(0.0, 0.0, 1.0);
		break;
	}
	if (drawingShadow) {
		glColor3f(0.1, 0.1, 0.1);
	}
	glTranslated(0, 0.5, 0);
	
	if (index == 1) {
		glEnable(GL_TEXTURE_2D);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, texball_w, texball_h, 0, GL_RGB, GL_UNSIGNED_BYTE, texball);
		GLUquadricObj *quadratic;
		quadratic = gluNewQuadric();
		gluQuadricTexture(quadratic, GL_TRUE);
		drawCube(0, 0, 0, 1);
		glScaled(2, 0.5, 2);
		glRotated(-90.0f, 1.0f, 0.0f, 0.0f);
		gluCylinder(quadratic,0.1f,0.1f,3.0f,32,32);
		glDisable(GL_TEXTURE_2D);
	} else {
		drawCube(0, 0, 0, 1);
	}

	glPopMatrix();
}

void drawTrackLine(Pnt3f fromPoint, Pnt3f toPoint, double *accumulatedDistance, TrackMode trackMode, bool drawingShadow)
{
	Pnt3f *middle = new Pnt3f(
		(fromPoint.x + toPoint.x)/2.0,
		(fromPoint.y + toPoint.y)/2.0,
		(fromPoint.z + toPoint.z)/2.0);
	Pnt3f *from = new Pnt3f(
		(fromPoint.x - (*middle).x),
		(fromPoint.y - (*middle).y),
		(fromPoint.z - (*middle).z));

	glPushMatrix();

	if (trackMode == TrackModeNormal) {
		glBegin(GL_LINES);
		glVertex3f(fromPoint.x,fromPoint.y,fromPoint.z);
		glVertex3f(toPoint.x,toPoint.y,toPoint.z);
		glEnd();
	} else if (trackMode == TrackModeDouble) {
		glBegin(GL_LINES);
		glVertex3f(fromPoint.x+3,fromPoint.y,fromPoint.z);
		glVertex3f(toPoint.x+3,toPoint.y,toPoint.z);
		glEnd();

		glBegin(GL_LINES);
		glVertex3f(fromPoint.x-3,fromPoint.y,fromPoint.z);
		glVertex3f(toPoint.x-3,toPoint.y,toPoint.z);
		glEnd();
	}
	glPopMatrix();

	glColor3f(211/255.0, 157/255.0, 7/255.0);
	glLineWidth(5);
	if (drawingShadow) {
		glColor3f(0.1, 0.1, 0.1);
	}

	if (*accumulatedDistance > 5) {
			
		glPushMatrix();
		glTranslated((*middle).x, (*middle).y, (*middle).z);
		glRotated(90, 0, 1, 0);
		glScaled(5, 5, 5);
		glBegin(GL_LINES);
		glVertex3f((*from).x, (*from).y, (*from).z);
		glVertex3f(-(*from).x, -(*from).y, -(*from).z);
		glEnd();
		glPopMatrix();

		*accumulatedDistance = 0;
	}
}

// TODO: function that draws the track
void TrainView::drawTrack(bool doingShadows)
{
	trackLength = 0;
	float step = 50.0f;
	float size = 1.0;
	//draw track
	glLineWidth(size);
	glColor3f(0.0, 1.0, 0.0);
	double accumulatedDistance = 0;

	for (size_t i=0; i<world->points.size(); i++) {

		if (curveMode == CurveModeLinear) {
			ControlPoint thisPoint = world->points[i];
			ControlPoint nextPoint = world->points[(i+1) % world->points.size()];

			for (int j = 0; j < step; j++) {
				float from = j*(1.0f/step);
				float to = (j+1)*(1.0f/step);

				Pnt3f *fromPoint = new Pnt3f(
					thisPoint.pos.x + (nextPoint.pos.x - thisPoint.pos.x) * from,
					thisPoint.pos.y + (nextPoint.pos.y - thisPoint.pos.y) * from,
					thisPoint.pos.z + (nextPoint.pos.z - thisPoint.pos.z) * from);
				Pnt3f *toPoint = new Pnt3f(
					thisPoint.pos.x + (nextPoint.pos.x - thisPoint.pos.x) * to,
					thisPoint.pos.y + (nextPoint.pos.y - thisPoint.pos.y) * to,
					thisPoint.pos.z + (nextPoint.pos.z - thisPoint.pos.z) * to);

				double thisDistance = pow(
					pow((*fromPoint).x - (*toPoint).x, 2) +
					pow((*fromPoint).y - (*toPoint).y, 2) +
					pow((*fromPoint).z - (*toPoint).z, 2), 0.5);
				trackLength += thisDistance;
				accumulatedDistance += thisDistance;

				drawTrackLine(*fromPoint, *toPoint, &accumulatedDistance, trackMode, doingShadows);
			}

		} else if (curveMode == CurveModeCatmull) {
			ControlPoint point1 = world->points[i];
			ControlPoint point2 = world->points[(i+1) % world->points.size()];
			ControlPoint point3 = world->points[(i+2) % world->points.size()];
			ControlPoint point4 = world->points[(i+3) % world->points.size()];

			for (int j = 0; j < step; j++) {
				float from = j*(1.0f/step);
				float to = (j+1)*(1.0f/step);

				Pnt3f *fromPoint = catmullPoint(from, point1.pos, point2.pos, point3.pos, point4.pos);
				Pnt3f *toPoint = catmullPoint(to, point1.pos, point2.pos, point3.pos, point4.pos);

				double thisDistance = pow(
					pow((*fromPoint).x - (*toPoint).x, 2) +
					pow((*fromPoint).y - (*toPoint).y, 2) +
					pow((*fromPoint).z - (*toPoint).z, 2), 0.5);
				trackLength += thisDistance;
				accumulatedDistance += thisDistance;

				drawTrackLine(*fromPoint, *toPoint, &accumulatedDistance, trackMode, doingShadows);
			}
		}
	}
}

//TODO: function that draw the train
void TrainView::drawTrain(bool doingShadows)
{
	float step = 50.0f;
	float size = 15.0;
	int carCount = 4;
	//draw track
	glLineWidth(size);
	
	int drawCount = 0;
	double accumulatedDistance = -1;
	bool skippedShift = false;

	size_t i = 0;
	while (true) {
		if (drawCount >= carCount) {
			break;
		}
		if (curveMode == CurveModeLinear) {
			ControlPoint thisPoint = world->points[i % world->points.size()];
			ControlPoint nextPoint = world->points[(i+1) % world->points.size()];
			
			
			for (int j = 0; j < step; j++) {
				float from = j*(1.0f/step);
				float to = (j+1)*(1.0f/step);

				Pnt3f *fromPoint = new Pnt3f(
					thisPoint.pos.x + (nextPoint.pos.x - thisPoint.pos.x) * from,
					thisPoint.pos.y + (nextPoint.pos.y - thisPoint.pos.y) * from,
					thisPoint.pos.z + (nextPoint.pos.z - thisPoint.pos.z) * from);
				Pnt3f *toPoint = new Pnt3f(
					thisPoint.pos.x + (nextPoint.pos.x - thisPoint.pos.x) * to,
					thisPoint.pos.y + (nextPoint.pos.y - thisPoint.pos.y) * to,
					thisPoint.pos.z + (nextPoint.pos.z - thisPoint.pos.z) * to);

				double thisDistance = pow(
					pow((*fromPoint).x - (*toPoint).x, 2) +
					pow((*fromPoint).y - (*toPoint).y, 2) +
					pow((*fromPoint).z - (*toPoint).z, 2), 0.5);

				if (skippedShift &&
					drawCount < carCount &&
					(accumulatedDistance == -1 /* first time*/ || accumulatedDistance > size)) {
					// can draw

						float x = (*toPoint).x - (*fromPoint).x;
						float y = (*toPoint).y - (*fromPoint).y;
						float z = (*toPoint).z - (*fromPoint).z;

						float xAngle = atanf(y/z);
						float yAngle = atanf(-z/x);
						float zAngle = atanf(y/x);
						float pi = atan(1)*4;

						glPushMatrix();
						glTranslated((*fromPoint).x,(*fromPoint).y,(*fromPoint).z);
						glRotated(xAngle/pi*180, 1, 0, 0);
						glRotated(yAngle/pi*180, 0, 1, 0);
						glRotated(zAngle/pi*180, 0, 0, 1);
						glScaled(size, size, size);
						drawCar(carCount - drawCount, doingShadows);
						glPopMatrix();
						
						accumulatedDistance = 0;
						drawCount++;
				} else {
					accumulatedDistance += thisDistance;
					if (!skippedShift && accumulatedDistance >= world->trainU) {
						skippedShift = true;
						accumulatedDistance = -1;
					}
				}
			}

		} else if (curveMode == CurveModeCatmull) {
			ControlPoint point1 = world->points[i % world->points.size()];
			ControlPoint point2 = world->points[(i+1) % world->points.size()];
			ControlPoint point3 = world->points[(i+2) % world->points.size()];
			ControlPoint point4 = world->points[(i+3) % world->points.size()];

			for (int j = 0; j < step; j++) {
				float from = j*(1.0f/step);
				float to = (j+1)*(1.0f/step);

				Pnt3f *fromPoint = catmullPoint(from, point1.pos, point2.pos, point3.pos, point4.pos);
				Pnt3f *toPoint = catmullPoint(to, point1.pos, point2.pos, point3.pos, point4.pos);

				double thisDistance = pow(
					pow((*fromPoint).x - (*toPoint).x, 2) +
					pow((*fromPoint).y - (*toPoint).y, 2) +
					pow((*fromPoint).z - (*toPoint).z, 2), 0.5);

				if (skippedShift &&
					drawCount < carCount &&
					(accumulatedDistance == -1 /* first time*/ || accumulatedDistance > size)) {
					// can draw
						float x = (*toPoint).x - (*fromPoint).x;
						float y = (*toPoint).y - (*fromPoint).y;
						float z = (*toPoint).z - (*fromPoint).z;

						float xAngle = atanf(y/z);
						float yAngle = atanf(-z/x);
						float zAngle = atanf(y/x);
						float pi = atan(1)*4;

						glPushMatrix();
						glTranslated((*fromPoint).x,(*fromPoint).y,(*fromPoint).z);
						glRotated(xAngle/pi*180, 1, 0, 0);
						glRotated(yAngle/pi*180, 0, 1, 0);
						glRotated(zAngle/pi*180, 0, 0, 1);
						glScaled(size, size, size);
						drawCar(carCount - drawCount, doingShadows);
						glPopMatrix();

						accumulatedDistance = 0;
						drawCount++;
				} else {
					accumulatedDistance += thisDistance;
					if (!skippedShift && accumulatedDistance >= world->trainU) {
						skippedShift = true;
						accumulatedDistance = -1;
					}
				}
			}
		}

		i++;
	}
}

//TODO: function that draw the train
void TrainView::calcTrain()
{
	float step = 50.0f;
	float size = 15.0;
	int carCount = 4;
	//draw track
	glLineWidth(size);
	
	int drawCount = 0;
	double accumulatedDistance = -1;
	bool skippedShift = false;

	size_t i = 0;
	while (true) {
		if (drawCount >= carCount) {
			break;
		}
		if (curveMode == CurveModeLinear) {
			ControlPoint thisPoint = world->points[i % world->points.size()];
			ControlPoint nextPoint = world->points[(i+1) % world->points.size()];
			
			
			for (int j = 0; j < step; j++) {
				float from = j*(1.0f/step);
				float to = (j+1)*(1.0f/step);

				Pnt3f *fromPoint = new Pnt3f(
					thisPoint.pos.x + (nextPoint.pos.x - thisPoint.pos.x) * from,
					thisPoint.pos.y + (nextPoint.pos.y - thisPoint.pos.y) * from,
					thisPoint.pos.z + (nextPoint.pos.z - thisPoint.pos.z) * from);
				Pnt3f *toPoint = new Pnt3f(
					thisPoint.pos.x + (nextPoint.pos.x - thisPoint.pos.x) * to,
					thisPoint.pos.y + (nextPoint.pos.y - thisPoint.pos.y) * to,
					thisPoint.pos.z + (nextPoint.pos.z - thisPoint.pos.z) * to);

				double thisDistance = pow(
					pow((*fromPoint).x - (*toPoint).x, 2) +
					pow((*fromPoint).y - (*toPoint).y, 2) +
					pow((*fromPoint).z - (*toPoint).z, 2), 0.5);

				if (skippedShift &&
					drawCount < carCount &&
					(accumulatedDistance == -1 /* first time*/ || accumulatedDistance > size)) {
					// can draw

						float x = (*toPoint).x - (*fromPoint).x;
						float y = (*toPoint).y - (*fromPoint).y;
						float z = (*toPoint).z - (*fromPoint).z;

						float xAngle = atanf(y/z);
						float yAngle = atanf(-z/x);
						float zAngle = atanf(y/x);
						float pi = atan(1)*4;

						if (carCount - drawCount == 1) {
							trainHeadPoint = fromPoint;
							trainHeadTarget = toPoint;
						}

						accumulatedDistance = 0;
						drawCount++;
				} else {
					accumulatedDistance += thisDistance;
					if (!skippedShift && accumulatedDistance >= world->trainU) {
						skippedShift = true;
						accumulatedDistance = -1;
					}
				}
			}

		} else if (curveMode == CurveModeCatmull) {
			ControlPoint point1 = world->points[i % world->points.size()];
			ControlPoint point2 = world->points[(i+1) % world->points.size()];
			ControlPoint point3 = world->points[(i+2) % world->points.size()];
			ControlPoint point4 = world->points[(i+3) % world->points.size()];

			for (int j = 0; j < step; j++) {
				float from = j*(1.0f/step);
				float to = (j+1)*(1.0f/step);

				Pnt3f *fromPoint = catmullPoint(from, point1.pos, point2.pos, point3.pos, point4.pos);
				Pnt3f *toPoint = catmullPoint(to, point1.pos, point2.pos, point3.pos, point4.pos);

				double thisDistance = pow(
					pow((*fromPoint).x - (*toPoint).x, 2) +
					pow((*fromPoint).y - (*toPoint).y, 2) +
					pow((*fromPoint).z - (*toPoint).z, 2), 0.5);

				if (skippedShift &&
					drawCount < carCount &&
					(accumulatedDistance == -1 /* first time*/ || accumulatedDistance > size)) {
					// can draw
						float x = (*toPoint).x - (*fromPoint).x;
						float y = (*toPoint).y - (*fromPoint).y;
						float z = (*toPoint).z - (*fromPoint).z;

						float xAngle = atanf(y/z);
						float yAngle = atanf(-z/x);
						float zAngle = atanf(y/x);
						float pi = atan(1)*4;

						if (carCount - drawCount == 1) {
							trainHeadPoint = fromPoint;
							trainHeadTarget = toPoint;
						}

						accumulatedDistance = 0;
						drawCount++;
				} else {
					accumulatedDistance += thisDistance;
					if (!skippedShift && accumulatedDistance >= world->trainU) {
						skippedShift = true;
						accumulatedDistance = -1;
					}
				}
			}
		}
		i++;
	}
}

// this draws all of the stuff in the world
// NOTE: if you're drawing shadows, DO NOT set colors 
// (otherwise, you get colored shadows)
// this gets called twice per draw - once for the objects, once for the shadows
// TODO: if you have other objects in the world, make sure to draw them
void TrainView::drawStuff(bool doingShadows)
{
	// draw the control points
	// don't draw the control points if you're driving 
	// (otherwise you get sea-sick as you drive through them)
	if (!tw->trainCam->value()) {
		for(size_t i=0; i<world->points.size(); ++i) {
			if (!doingShadows) {
				if ( ((int) i) != selectedCube)
					glColor3ub(240,60,60);
				else
					glColor3ub(240,240,30);
			}
			world->points[i].draw();
		}
	}
	// draw the track
	// TODO: call your own track drawing code
	drawTrack(doingShadows);


	// draw the train
	// TODO: call your own train drawing code
	// don't draw the train if you're looking out the front window
	calcTrain();
	if (!tw->trainCam->value()) {
		drawTrain(doingShadows);
	}

	// draw trees

	t1 = new Tree();
	t1->x = -51.0f;
	t1->z = -54.0f;
	t1->angle1 = angle1;
	t1->angle2 = angle2;
	t1->draw(doingShadows);

	t2 = new Tree();
	t2->x = -58.0f;
	t2->z = -38.0f;
	t2->angle1 = angle1;
	t2->angle2 = -angle2;
	t2->draw(doingShadows);

	t3 = new Tree();
	t3->x = -45.0f;
	t3->z = -42.0f;
	t3->angle1 = -angle2;
	t3->angle2 = angle1;
	t3->draw(doingShadows);

	t4 = new Tree();
	t4->x = -13.0f;
	t4->z = -12.0f;
	t4->angle1 = angle2;
	t4->angle2 = -angle1;
	t4->draw(doingShadows);

	// draw ferris wheel cars

	car1 = new Car();
	car1->tcolor[0] = 1;
	car1->tcolor[1] = 0.5;
	car1->tcolor[2] = 1;
	car1->angle3 = angle3;
	car1->draw(doingShadows);

	car2 = new Car();
	car2->tcolor[0] = 1;
	car2->tcolor[1] = 1;
	car2->tcolor[2] = 0.5;
	car2->angle3 = angle3+60;
	car2->draw(doingShadows);

	car3 = new Car();
	car3->tcolor[0] = 0.5;
	car3->tcolor[1] = 1;
	car3->tcolor[2] = 1;
	car3->angle3 = angle3+120;
	car3->draw(doingShadows);

	car4 = new Car();
	car4->tcolor[0] = 1;
	car4->tcolor[1] = 0.5;
	car4->tcolor[2] = 1;
	car4->angle3 = angle3+180;
	car4->draw(doingShadows);

	car5 = new Car();
	car5->tcolor[0] = 1;
	car5->tcolor[1] = 1;
	car5->tcolor[2] = 0.5;
	car5->angle3 = angle3+240;
	car5->draw(doingShadows);

	car6 = new Car();
	car6->tcolor[0] = 0.5;
	car6->tcolor[1] = 1;
	car6->tcolor[2] = 1;
	car6->angle3 = angle3+300;
	car6->draw(doingShadows);
	
	// base of ferris wheel
	glPushMatrix();
		glTranslated(-60, 20, 70);

		glPushMatrix();
		glRotatef(120, 1.0f, 0.0f, 0.0f);
		glColor3f(0.8, 1, 0.8);
		glBegin(GL_POLYGON);
		GLUquadricObj *cola = gluNewQuadric();
		gluCylinder(cola, 1, 1, 30, 20, 30);
		glEnd();
		glPopMatrix();

		glPushMatrix();
		glRotatef(60, 1.0f, 0.0f, 0.0f);
		glColor3f(0.8, 1, 0.8);
		glBegin(GL_POLYGON);
		GLUquadricObj *colb = gluNewQuadric();
		gluCylinder(colb, 1, 1, 30, 20, 30);
		glEnd();
		glPopMatrix();
	
	glPopMatrix();

	// hemisphere
	int rad = 45;
	glPushMatrix();
		glTranslated(45, 0, 5);
		glRotatef(270, 1.0f, 0.0f, 0.0f);
	
	glEnable(GL_TEXTURE_2D);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, texcar_w, texcar_h, 0, GL_RGB, GL_UNSIGNED_BYTE, texcar);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT ); // no
		glBegin(GL_QUADS);
		float ux,lx,uy,ly;
		int i,j;
		for(i = 0; i < 360; i += 18){
			if (i > 80 && i < 115) continue;
			if (i > 205 && i < 240) continue;
			for (j = 0; j < 90; j += 18)
			{	
				ux = i * (M_PI / 180);
				lx = (i + 18) * (M_PI / 180);
				uy = j * (M_PI / 180);
				ly = (j + 18) * (M_PI / 180);
				glNormal3f(cos(ux)*cos(uy),cos(ux)*sin(uy),sin(uy));
				glVertex3f(rad*cos(ux)*sin(ly),rad*sin(ux)*sin(ly),rad*cos(ly));
				glVertex3f(rad*cos(lx)*sin(ly),rad*sin(lx)*sin(ly),rad*cos(ly));
				glVertex3f(rad*cos(lx)*sin(uy),rad*sin(lx)*sin(uy),rad*cos(uy));
				glVertex3f(rad*cos(ux)*sin(uy),rad*sin(ux)*sin(uy),rad*cos(uy));
			}
		}
		glEnd();
	glDisable(GL_TEXTURE_2D);
	glPopMatrix();
}

// this tries to see which control point is under the mouse
// (for when the mouse is clicked)
// it uses OpenGL picking - which is always a trick
// TODO: if you want to pick things other than control points, or you
// changed how control points are drawn, you might need to change this
void TrainView::doPick()
{
	make_current();		// since we'll need to do some GL stuff

	int mx = Fl::event_x(); // where is the mouse?
	int my = Fl::event_y();

	// get the viewport - most reliable way to turn mouse coords into GL coords
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	// set up the pick matrix on the stack - remember, FlTk is
	// upside down!
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity ();
	gluPickMatrix((double)mx, (double)(viewport[3]-my), 5, 5, viewport);

	// now set up the projection
	setProjection();

	// now draw the objects - but really only see what we hit
	GLuint buf[100];
	glSelectBuffer(100,buf);
	glRenderMode(GL_SELECT);
	glInitNames();
	glPushName(0);

	// draw the cubes, loading the names as we go
	for(size_t i=0; i<world->points.size(); ++i) {
		glLoadName((GLuint) (i+1));
		world->points[i].draw();
	}

	// go back to drawing mode, and see how picking did
	int hits = glRenderMode(GL_RENDER);
	if (hits) {
		// warning; this just grabs the first object hit - if there
		// are multiple objects, you really want to pick the closest
		// one - see the OpenGL manual 
		// remember: we load names that are one more than the index
		selectedCube = buf[3]-1;
	} else // nothing hit, nothing selected
		selectedCube = -1;

	printf("Selected Cube %d\n",selectedCube);
}

// CVS Header - if you don't know what this is, don't worry about it
// This code tells us where the original came from in CVS
// Its a good idea to leave it as-is so we know what version of
// things you started with
// $Header: /p/course/-gleicher/private/CVS/TrainFiles/TrainView.cpp,v 1.9 2008/10/21 14:46:45 gleicher Exp $
