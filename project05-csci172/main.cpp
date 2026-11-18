/*
    Nathan Jack Luna
    ID: 302029118
    Project05
    CSCI 172
    11/10/2026
*/

/*
    '.obj' file format notes:

    - v: vertices (x,y,z)
    - vt: texture coords (u,v)
    - vn: vertex normals (x,y,z)
    - f: faces (v index / vt index / vn index) or (v//vn if there is no vt)
*/

#include <string.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <stdlib.h>
#include <iostream>

#include <SOIL.h>
#include <math.h>

#include <vector> // added this library because we will need to create vectors
#include <fstream> // added this library to open an .obj file and read it line by line
#include <sstream> // added this library so I can parse each line in the wavefront files easier


using namespace std;

bool WireFrame= false;

const GLfloat light_ambient[]  = { 0.0f, 0.0f, 0.0f, 1.0f };
const GLfloat light_diffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position[] = { 2.0f, 5.0f, 5.0f, 0.0f };

const GLfloat mat_ambient[]    = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat mat_diffuse[]    = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_specular[]   = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat high_shininess[] = { 100.0f };

// Creating structs for vertices, texture coords, and vertex normals
// vertices and vertex normals will use Vec3, while text coords will use Vec2
// faces will be represented as a vector, with each index holding three Vec3_int's (corners)
struct Vec3_float{ float x, y, z; };
struct Vec3_int{ int v, vt, vn; };
struct Faces{ vector<Vec3_int> corners; };
struct Vec2{ float u, v; };

// These will be the vectors that will be used to store the .obj data into
vector<Vec3_float> vertices;
vector<Vec2> texture_coords;
vector<Vec3_float> vertex_normals;
vector<Faces> faces; // The indices list that will include vertex index, texture index, and normal index for f lines

// Manually-created global variables
GLuint texID_skybox;
GLuint texID_model;
char* models[3] = {"models/cube.obj",
                    "models/ateneam-2.obj",
                    "models/jeep.obj"};
int model_index = 0;

// To control the cube
float cube_radius = 2.0;
GLdouble TranslateX, TranslateY, rotate_modelX, rotate_modelY, rotate_modelZ;
GLdouble scale = 0.8f;

// To control the skybox
GLdouble Rotate_SkyX, Rotate_SkyY, Rotate_SkyZ;

// To control the camera
GLdouble CameraX, CameraY, Zoom;

/* GLUT callback Handlers */
static void resize(int width, int height)
{
    // used "(double)" on both the width and the height to cast it from an int to a double
    double Ratio = (double)width / (double)height;

    // I wasn't a fan of the 1:1 aspect ratio, as it created these black boxes on the
    // sides of the windows if the width > height. I changed the viewport to match the
    // window size. We can now resize the window without the objects stretching out.

    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective (50.0f, Ratio, 0.1f, 100.0f);
}

void loadTexture(char* fileName, GLuint& texID)
{
    glEnable(GL_TEXTURE_2D); // Enable texture format
    glGenTextures(1, &texID); // generate an ID

    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glBindTexture(GL_TEXTURE_2D, texID); // images are 2D arrays of pixels, bound to the GL_TEXTURE_2D target.

    int width, height; // width & height for the image reader
    unsigned char* image;

    image = SOIL_load_image(fileName, &width, &height, 0, SOIL_LOAD_RGBA);
    if(!image)
        cout << fileName << ": File did not load" << endl;

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // binding image data
    SOIL_free_image_data(image); // clear out data

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void loadModel(char* fileName){
    // Clearing out the vectors just in case there was already data inside from a previous model
    // Nothing will happen if you clear() on an empty vector
    vertices.clear();
    texture_coords.clear();
    vertex_normals.clear();
    faces.clear();

    // Opening up the wavefront file to read from it
    ifstream obj_file(fileName);
    if(!obj_file.is_open()){
        cout << "Unable to load wavefront file" << endl;
        return;
    }

    string obj_file_string, corner_str, corner_index;
    string row_type, value1_str, value2_str, value3_str;
    float value1_float, value2_float, value3_float;
    int value1_int, value2_int, value3_int;

    while(getline(obj_file, obj_file_string)){
        istringstream iss(obj_file_string);

        if(!(iss >> row_type)){ continue; }

        if(row_type == "v"){
            iss >> value1_str >> value2_str >> value3_str;
            value1_float = stof(value1_str);
            value2_float = stof(value2_str);
            value3_float = stof(value3_str);
            vertices.push_back({value1_float, value2_float, value3_float});
        }
        else if(row_type == "vt"){
            iss >> value1_str >> value2_str;
            value1_float = stof(value1_str);
            value2_float = stof(value2_str);
            texture_coords.push_back({value1_float, value2_float});

        }
        else if(row_type == "vn"){
            iss >> value1_str >> value2_str >> value3_str;
            value1_float = stof(value1_str);
            value2_float = stof(value2_str);
            value3_float = stof(value3_str);
            vertex_normals.push_back({value1_float, value2_float, value3_float});

        }
        else if(row_type == "f"){
            Faces face;

            while (iss >> corner_str){
                istringstream iss_corner(corner_str);
                string v_str, vt_str, vn_str;
                Vec3_int corner;

                getline(iss_corner, v_str, '/');
                getline(iss_corner, vt_str, '/');
                getline(iss_corner, vn_str, '/');

                corner.v = stoi(v_str) - 1;
                if(!vt_str.empty())
                    corner.vt = stoi(vt_str) - 1;
                else
                    corner.vt = -1;
                corner.vn = stoi(vn_str) - 1;

                face.corners.push_back(corner);
            }
            faces.push_back(face);
        }
        else{}
    }
}

static void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(0,0,10,0.0,0.0,0.0,0.0,1.0,0.0);

    if(WireFrame)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);		//Draw Our Mesh In Wireframe Mesh
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);		//Toggle WIRE FRAME

    // your code here

    // Transformations that affect the whole scene rather than the individuals objects
    glTranslatef(CameraX, CameraY, Zoom);

    // ---------------------------------------- MODEL ----------------------------------------------------
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBindTexture(GL_TEXTURE_2D, texID_model);
    glPushMatrix();
        glRotatef(rotate_modelX, 1.0f, 0.0f, 0.0f);
        if(model_index == 1){
            glRotatef(rotate_modelY - 90.0f, 0.0f, 1.0f, 0.0f);
            glTranslatef(1.0f, 0.0f, 0.0f);
            glScalef(0.001f, 0.001f, 0.001f);
        }
        else if(model_index == 2){
            glRotatef(rotate_modelY, 0.0f, 1.0f, 0.0f);
            glTranslatef(0.0f, -1.0f, 0.0f);
            glScalef(1.5f, 1.5f, 1.5f);
        }
        else{
            glRotatef(rotate_modelY, 0.0f, 1.0f, 0.0f);
            glScalef(2.0f, 2.0f, 2.0f);
        }


    for(int i = 0; i < faces.size(); i++){
            glBegin(GL_TRIANGLES);

            glNormal3f(vertex_normals[faces[i].corners[0].vn].x,
                       vertex_normals[faces[i].corners[0].vn].y,
                       vertex_normals[faces[i].corners[0].vn].z
            );
            if(faces[i].corners[0].vt == -1)
                glTexCoord2f(0.0f, 0.0f);
            else{
                glTexCoord2f(texture_coords[faces[i].corners[0].vt].u,
                             texture_coords[faces[i].corners[0].vt].v
                );
            }
            glVertex3f(vertices[faces[i].corners[0].v].x,
                       vertices[faces[i].corners[0].v].y,
                       vertices[faces[i].corners[0].v].z
            );

            glNormal3f(vertex_normals[faces[i].corners[1].vn].x,
                       vertex_normals[faces[i].corners[1].vn].y,
                       vertex_normals[faces[i].corners[1].vn].z
            );
            if(faces[i].corners[1].vt == -1)
                glTexCoord2f(0.0f, 0.0f);
            else{
                glTexCoord2f(texture_coords[faces[i].corners[1].vt].u,
                             texture_coords[faces[i].corners[1].vt].v
                );
            }
            glVertex3f(vertices[faces[i].corners[1].v].x,
                       vertices[faces[i].corners[1].v].y,
                       vertices[faces[i].corners[1].v].z
            );

            glNormal3f(vertex_normals[faces[i].corners[2].vn].x,
                       vertex_normals[faces[i].corners[2].vn].y,
                       vertex_normals[faces[i].corners[2].vn].z
            );
            if(faces[i].corners[2].vt == -1)
                glTexCoord2f(0.0f, 0.0f);
            else{
                glTexCoord2f(texture_coords[faces[i].corners[2].vt].u,
                             texture_coords[faces[i].corners[2].vt].v
                );
            }
            glVertex3f(vertices[faces[i].corners[2].v].x,
                       vertices[faces[i].corners[2].v].y,
                       vertices[faces[i].corners[2].v].z
            );
        glEnd();
    }
    glPopMatrix();


    // ---------------------------------------- SKY BOX ------------------------------------------------------
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBindTexture(GL_TEXTURE_2D, texID_skybox);

    glPushMatrix();
        glRotatef(Rotate_SkyX, 1.0f, 0.0f, 0.0f);
        glRotatef(Rotate_SkyY, 0.0f, 1.0f, 0.0f);
        glScalef(50, 50, 50);

    glBegin(GL_QUADS);
        glNormal3f(0.0f, 0.0f, -1.0f); // Normal Front Face
        glTexCoord2f(0.25f, 2.0/3.0f); glVertex3f(-1.0f, -1.0f, 1.0f); // Bottom Left Of The Texture and Quad
        glTexCoord2f(0.5f, 2.0/3.0f); glVertex3f( 1.0f, -1.0f, 1.0f); // Bottom Right Of The Texture and Quad
        glTexCoord2f(0.5f, 1.0/3.0f); glVertex3f( 1.0f, 1.0f, 1.0f); // Top Right Of The Texture and Quad
        glTexCoord2f(0.25f, 1.0/3.0f); glVertex3f(-1.0f, 1.0f, 1.0f); // Top Left Of The Texture and Quad
        glNormal3f(0.0f, 0.0f, 1.0f); // Normal Back Face
        glTexCoord2f(1.0f, 2.0/3.0f); glVertex3f(-1.0f, -1.0f, -1.0f); // Bottom Right Of The Texture and Quad
        glTexCoord2f(1.0f, 1.0/3.0f); glVertex3f(-1.0f, 1.0f, -1.0f); // Top Right Of The Texture and Quad
        glTexCoord2f(0.75f, 1.0/3.0f); glVertex3f( 1.0f, 1.0f, -1.0f); // Top Left Of The Texture and Quad
        glTexCoord2f(0.75f, 2.0/3.0f); glVertex3f( 1.0f, -1.0f, -1.0f); // Bottom Left Of The Texture and Quad
        glNormal3f(0.0f, -1.0f, 0.0f); // Normals Top Face
        glTexCoord2f(0.25f, 0.0f); glVertex3f(-1.0f, 1.0f, -1.0f); // Top Left Of The Texture and Quad
        glTexCoord2f(0.25f, 1.0/3.0f); glVertex3f(-1.0f, 1.0f, 1.0f); // Bottom Left Of The Texture and Quad
        glTexCoord2f(0.5f, 1.0/3.0f); glVertex3f( 1.0f, 1.0f, 1.0f); // Bottom Right Of The Texture and Quad
        glTexCoord2f(0.5f, 0.0f); glVertex3f( 1.0f, 1.0f, -1.0f); // Top Right Of The Texture and Quad
        glNormal3f(0.0f, 1.0f, 0.0f); // Bottom Face
        glTexCoord2f(0.25f, 1.0f); glVertex3f(-1.0f, -1.0f, -1.0f); // Top Right Of The Texture and Quad 0.25f, 1.0f
        glTexCoord2f(0.5f, 1.0f); glVertex3f( 1.0f, -1.0f, -1.0f); // Top Left Of The Texture and Quad 0.5f, 1.0f
        glTexCoord2f(0.5f, 2.0/3.0f); glVertex3f( 1.0f, -1.0f, 1.0f); // Bottom Left Of The Texture and Quad 0.5f, 2.0/3.0f
        glTexCoord2f(0.25f, 2.0/3.0f); glVertex3f(-1.0f, -1.0f, 1.0f); // Bottom Right Of The Texture and Quad 0.25f, 2.0/3.0f
        glNormal3f(-1.0f, 0.0f, 0.0f); // Right face
        glTexCoord2f(0.75f, 2.0/3.0f); glVertex3f( 1.0f, -1.0f, -1.0f); // Bottom Right Of The Texture and Quad
        glTexCoord2f(0.75f, 1.0/3.0f); glVertex3f( 1.0f, 1.0f, -1.0f); // Top Right Of The Texture and Quad
        glTexCoord2f(0.5f, 1.0/3.0f); glVertex3f( 1.0f, 1.0f, 1.0f); // Top Left Of The Texture and Quad
        glTexCoord2f(0.5f, 2.0/3.0f); glVertex3f( 1.0f, -1.0f, 1.0f); // Bottom Left Of The Texture and Quad
        glNormal3f(1.0f, 0.0f, 0.0f); // Left Face
        glTexCoord2f(0.0f, 2.0/3.0f); glVertex3f(-1.0f, -1.0f, -1.0f); // Bottom Left Of The Texture and Quad
        glTexCoord2f(0.25f, 2.0/3.0f); glVertex3f(-1.0f, -1.0f, 1.0f); // Bottom Right Of The Texture and Quad
        glTexCoord2f(0.25f, 1.0/3.0f); glVertex3f(-1.0f, 1.0f, 1.0f); // Top Right Of The Texture and Quad
        glTexCoord2f(0.0f, 1.0/3.0f); glVertex3f(-1.0f, 1.0f, -1.0f); // Top Left Of The Texture and Quad
    glEnd();
    glPopMatrix();

    glutSwapBuffers();
}

static void key(unsigned char key, int x, int y)
{
    switch (key)
    {
        case 27 :
        case 'q':
            exit(0);
            break;

        // ZX to zoom in and out of the scene
        case 'z': // Zoom Out
            Zoom -= (float) 0.75f;
            break;
        case 'x': // Zoom In
            Zoom += (float) 0.75f;
            break;

        // IJKL for moving the camera, O to reset camera
        case 'i': // move camera up
            CameraY -= (float) 0.75f;
            break;
        case 'k': // move camera down
            CameraY += (float) 0.75f;
            break;
        case 'j': // move camera left
            CameraX += (float) 0.75f;
            break;
        case 'l': // move camera right
            CameraX -= (float) 0.75f;
            break;

        // Reset the whole scene
        case ' ':
            TranslateX = TranslateY = Zoom = rotate_modelX = rotate_modelY = rotate_modelZ = 0;
            Rotate_SkyX = Rotate_SkyY = Rotate_SkyZ = 0;
            CameraX = CameraY = 0;
            break;
    }
}

void Specialkeys(int key, int x, int y)
{
    switch(key)
    {
        // Arrow keys to rotate the objects
        case GLUT_KEY_UP:
            rotate_modelX = ((int)rotate_modelX - 5)%360;
            break;
        case GLUT_KEY_DOWN:
            rotate_modelX = ((int)rotate_modelX + 5)%360;
            break;
        case GLUT_KEY_LEFT:
            rotate_modelY = ((int)rotate_modelY - 5)%360;
            break;
        case GLUT_KEY_RIGHT:
            rotate_modelY = ((int)rotate_modelY + 5)%360;
            break;
   }
  glutPostRedisplay();
}

// Creating a function for recording left-clicks
void mouse(int button, int state, int x, int y){
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN){
        model_index = (model_index + 1) % 3;
        loadModel(models[model_index]);
        glutPostRedisplay();
    }
}

static void idle(void)
{
    glutPostRedisplay();
}

static void init(void)
{
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);

    glEnable(GL_DEPTH_TEST);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glShadeModel(GL_SMOOTH);

    glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glMaterialfv(GL_FRONT, GL_AMBIENT,   mat_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE,   mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR,  mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);

    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);

    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHTING);

    loadTexture("images/cubemap_sky.png", texID_skybox);
    loadTexture("images/t3.jpg", texID_model);
    loadModel(models[model_index]);
}


/* Program entry point */

int main(int argc, char *argv[])
{
    glutInit(&argc, argv);

    glutInitWindowSize(1280,720);
    glutInitWindowPosition(0,0);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);

    glutCreateWindow("Project05 CSCI 172 - Nathan Luna");
    init();
    glutReshapeFunc(resize);
    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutSpecialFunc(Specialkeys);
    glutMouseFunc(mouse); // Adding my left-click function

    glutIdleFunc(idle);
    glutMainLoop();

    return EXIT_SUCCESS;
}
