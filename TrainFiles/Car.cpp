#include "Car.h"

#include <windows.h>
#include <math.h>
#include "GL/gl.h"
#include "GL/glu.h"

Car::Car()
{
}

void Car::draw(bool doingShadows)
{
	float angle = angle3;
	float M_PI = 3.1415926535;
	float rad = 10;

	float ax = (-rad)*cosf((90-angle)*M_PI/180.0) - (-rad)*sinf((270-angle)*M_PI/180.0);
	float ay = (-rad)*cosf((90-angle)*M_PI/180.0) + (-rad)*sinf((270-angle)*M_PI/180.0);

	glPushMatrix();

	glPushMatrix();
	glTranslated(ax-60, 15+ay, 70);
	
	glPushMatrix();
	glRotatef(270, 1.0f, 0.0f, 0.0f);
	glColor3f(1, 1, 1);
	glBegin(GL_POLYGON);
	GLUquadricObj *seat = gluNewQuadric();
	gluCylinder(seat, 3, 4, 4, 20, 30);
	glEnd();
	glPopMatrix();

	glPushMatrix();
	glTranslated(2, 4, 2);
	glRotatef(270, 1.0f, 0.0f, 0.0f);
	glColor3f(1, 1, 1);
	glBegin(GL_POLYGON);
	GLUquadricObj *col1 = gluNewQuadric();
	gluCylinder(col1, 0.5, 0.5, 4, 20, 30);
	glEnd();
	glPopMatrix();

	glPushMatrix();
	glTranslated(-2, 4, 2);
	glRotatef(270, 1.0f, 0.0f, 0.0f);
	glColor3f(1, 1, 1);
	glBegin(GL_POLYGON);
	GLUquadricObj *col2 = gluNewQuadric();
	gluCylinder(col2, 0.5, 0.5, 4, 20, 30);
	glEnd();
	glPopMatrix();

	glPushMatrix();
	glTranslated(2, 4, -2);
	glRotatef(270, 1.0f, 0.0f, 0.0f);
	glColor3f(1, 1, 1);
	glBegin(GL_POLYGON);
	GLUquadricObj *col3 = gluNewQuadric();
	gluCylinder(col3, 0.5, 0.5, 4, 20, 30);
	glEnd();
	glPopMatrix();

	glPushMatrix();
	glTranslated(-2, 4, -2);
	glRotatef(270, 1.0f, 0.0f, 0.0f);
	glColor3f(1, 1, 1);
	glBegin(GL_POLYGON);
	GLUquadricObj *col4 = gluNewQuadric();
	gluCylinder(col4, 0.5, 0.5, 4, 20, 30);
	glEnd();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0, 8, 0);
	glRotatef(270, 1.0f, 0.0f, 0.0f);
	glPushMatrix();
	glColor3f(tcolor[0], tcolor[1], tcolor[2]);
	glBegin(GL_POLYGON);
	GLUquadricObj *ceil = gluNewQuadric();
	gluCylinder(ceil, 4, 0, 2, 20, 30);
	glEnd();
	glPopMatrix();
	glPopMatrix();

	glPopMatrix();

	// linkage
	
	glPushMatrix();
		glRotatef(270, 0.0f, 1.0f, 0.0f);
		glTranslated(70, 20, 60);
		glRotatef(angle+20, 1.0f, 0.0f, 0.0f);

		glPushMatrix();
		glTranslated(0, 0, 0);
		glColor3f(1, 1, 1);
		glBegin(GL_POLYGON);
		GLUquadricObj *col = gluNewQuadric();
		gluCylinder(col, 1, 1, 10, 20, 30);
		glEnd();
		glPopMatrix();
	
	glPopMatrix();

	glPopMatrix();

}