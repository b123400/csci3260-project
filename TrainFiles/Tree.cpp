#include "Tree.h"

#include <windows.h>
#include "GL/gl.h"
#include "GL/glu.h"

Tree::Tree()
{
}

void Tree::draw(bool doingShadows)
{
	angle1 = angle1 * 0.5;
	angle2 = angle2 * 0.5;

	glPushMatrix();
	glTranslated(x, 0, z);
	glRotatef(270, 1.0f, 0.0f, 0.0f);
	glScalef(0.3, 0.3, 0.8);

	glPushMatrix();
	glColor3f(0.78, 0.38, 0.08);
	glBegin(GL_POLYGON);
	GLUquadricObj *tree2gun = gluNewQuadric();
	gluCylinder(tree2gun, 5, 5, 20, 20, 30);
	glEnd();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0, 0, 10);
	glColor3f(0.1, 0.4, 0.1);
	glBegin(GL_POLYGON);
	GLUquadricObj *tree2leaves1 = gluNewQuadric();
	gluCylinder(tree2leaves1, 15, 0, 20, 20, 30);
	glEnd();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0, 0, 22);
	glRotatef(-angle2, 1.0f, 0.0f, 0.0f);
	glRotatef(angle1, 0.0f, 1.0f, 0.0f);
	glColor3f(0.1, 0.4, 0.1);
	glBegin(GL_POLYGON);
	GLUquadricObj *tree2leaves2 = gluNewQuadric();
	gluCylinder(tree2leaves2, 14, 0, 20, 20, 30);
	glEnd();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0, 0, 33);
	glRotatef(angle1, 1.0f, 0.0f, 0.0f);
	glRotatef(angle2, 0.0f, 1.0f, 0.0f);
	glColor3f(0.1, 0.4, 0.1);
	glBegin(GL_POLYGON);
	GLUquadricObj *tree2leaves3 = gluNewQuadric();
	gluCylinder(tree2leaves3, 10, 0, 20, 20, 30);
	glEnd();
	glPopMatrix();

	glBegin(GL_TRIANGLES);
	if (!doingShadows)
		glColor3d((float)0x45 / 255, (float)0x8B / 255, 0);
	glVertex3f(-2.0f, 2, 2);
	glVertex3f(2.0f, 2, 2);
	glVertex3f(0.0f, 10, 0);
	glEnd();

	glPopMatrix();

}