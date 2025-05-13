/*
* Title: Linear Transformations: Module 4 - Transformations
* Description: This program simulates the solar system
* Author: Halmuhammet Muhamedorazov
* Date: 10/09/2024
* Version number: g++ 13.2.0, gcc 11.4.0
* Requirements: This program requires GLAD, GLFW, GLM, gif, and ImGui libraries
* Note: The user can change planet/object properties using the drop-down menu in the screen
* Version requirement: This program requires GLFW 3.3 or above
*/

// Import the libraries that will be used in this program
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "gif.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define _USE_MATH_DEFINES
#include <iostream>
#include <cmath>
#include <numbers>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
using namespace std;

// Constants
float ASTERIOD_BELT_RADIUS_X = 0.42f;
float ASTERIOD_BELT_RADIUS_Y = 0.40f;
float ASTERIOD_BELT_RADIUS2_X = 0.41f;
float ASTERIOD_BELT_RADIUS2_Y = 0.39f;
float ASTERIOD_BELT_RADIUS3_X = 0.4f;
float ASTERIOD_BELT_RADIUS3_Y = 0.38f;
float EARTH_MOON_DISTANCE = 0.036f;

 /*Texture coordinate for background that covers the entire screen
 It forms two triangles with 3 vertices, each with its texture coordinates*/

float backgroundVertices[] = {
	// positions        // texture coords
	-1.0f,  1.0f,       0.0f, 1.0f,
	-1.0f, -1.0f,       0.0f, 0.0f,
	 1.0f, -1.0f,       1.0f, 0.0f,

	-1.0f,  1.0f,       0.0f, 1.0f,
	 1.0f, -1.0f,       1.0f, 0.0f,
	 1.0f,  1.0f,       1.0f, 1.0f
};

//Celestial Bodies struct that will be used to update body properties in the rendering loop

struct CelestialBodies {
	unsigned int VAO;         // Vertex Array Object for the planet
	float moveSpeed;          // Speed at which the planet moves
	float orbitRadiusX;       // Max X-axis radius of the orbit
	float orbitRadiusY;       // Max Y-axis radius of the orbit
	float scale;              // Scale of the planet
	int segments;             // Number of segments for rendering
	float updatePosX;         // Updated X position for the planet
	float updatePosY;         // Updated Y position for the planet
	float rotationSpeed;	  // Rotation speed of planet/object
	bool isScale;             // Flag for scaling
	bool isTranslate;         // Flag for translation
	bool isRotate;            // Flag for rotation
	bool isVisible;			  // Flag to allow the user add or remove planet
	bool isDrawAsRing;        // Flag for drawing the orbital as ring
	glm::vec4 color;          // Color of the planet
	unsigned int textureID;	  // ID of the planet texture
};

/*--------------------------------------------------------------
Function prototypes which are defined at the end of this program
---------------------------------------------------------------*/

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window, unsigned int shaderProgram);
unsigned int loadTexture(char const* path);
void setupObjectBuffer(GLuint& VAO, GLuint& VBO, float radius_x_axis, float radius_y_axis, int segments);
void useBackgroundTexture(unsigned int shaderProgram, GLuint backgroundVAO, unsigned int backgroundTextureID);
void setupBackgroundBuffers(GLuint& backgroundVAO, GLuint& backgroundVBO, float* backgroundVertices, size_t vertexCount);
GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource);
void drawAsteroidBelt(unsigned int shaderProgram, GLuint asteroidBeltVAO, float asteroidBeltSpeed);
void processInput(GLFWwindow* window, unsigned int shaderProgram, int& selectedObject,
	CelestialBodies& sun, CelestialBodies& mercury, CelestialBodies& venus, CelestialBodies& earth,
	CelestialBodies& mars, CelestialBodies& jupiter, CelestialBodies& saturn, CelestialBodies& uranus, CelestialBodies& neptune,
	CelestialBodies& moon, CelestialBodies& jupiterMoonIo, CelestialBodies& jupiterMoonCallisto, CelestialBodies& comet, bool& isDrawAsteroidBelt, float& asteroidBeltMoveSpeed);


/*---------------------------------------------
Celestial Object Shader Program Source Code
-----------------------------------------------*/

// vertex shader source code - defines where in the screen the object and its texture need to be rendered
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
uniform mat4 transform;
out vec2 TexCoord;

void main()
{
   gl_Position = transform * vec4(aPos, 0.0, 1.0);
   TexCoord = aTexCoord;
}
)";

/*
Fragment shader source code
If the user does not provide a texture for the planet, then the shader will use a uniform color instead
*/
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D texture1;
uniform vec4 color;
uniform bool useTexture;

void main()
{
    if (useTexture)
    {
        FragColor = texture(texture1, TexCoord);
    }
    else
    {
        FragColor = color;
    }
}
)";

/*---------------------------------------------------------------------------------------------------
Celestial Object Shader Program Source Code
Use separate shader program for the background texture to avoid binding issues with other planets
-----------------------------------------------------------------------------------------------------*/

// Vertex shader for the background texture coordinates - defines where (vetices) the texture needs to be applied
const char* backgroundVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

// Fragment shader for the background texture - applies texture using texture coordinates
const char* backgroundFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D backgroundTexture;
void main()
{
    FragColor = texture(backgroundTexture, TexCoord);
}
)";


 /*------------------------------------------------------------------------------------------------
 Define ellipse/planet vertices using trigonometry, where x = r_x*cos(angle) and y = r_y*sin(angle)
 Divide the angle into an arbitrary number to create desired segments on the circle circumference
 And iterate through the segments on the circumference to make planet/object vertex at each segment
 radius_x_axis is the max radius along x-axis and radius_y_axis is the max radius along y-axis
 
 Returns a 2D vector that contains planet or ellipse vertices
 --------------------------------------------------------------------------------------------------*/

vector<float> getObjectVertices(float radius_x_axis, float radius_y_axis, int segments) {
	//Delare a vector that can contain the vertices of objects
	std::vector<float> vertices;

	// iterate through the segments to form the verices (x and y coordinates)
	for (int segment = 0; segment <= segments; segment++) {
		// Divide the circumference into segments and get each angle for that point
		float angle = (2.0 * M_PI * float(segment)) / float(segments);
		// Get the value of x and y coordinates using trigonometry
		float x = radius_x_axis * cosf(angle);
		float y = radius_y_axis * sinf(angle);
		//push the xy coordinates into the vertices vector
		vertices.push_back(x);
		vertices.push_back(y);
	}
	// return the 2D vector that contains the object vertices
	return vertices;
}

/*------------------------------------------------------------------------------------------------------------------
Draw the celestial objects using this helper function
It takes as argument all the attributes of planet/object and its associated VAO to apply transformations and draw it

Returns a vector containing new xy coordinates of planet/object (if it was translated)
-------------------------------------------------------------------------------------------------------------------*/

vector<float> drawPlanet(unsigned int shaderProgram, unsigned int VAO, float planetMoveSpeed, float orbitRadiusX, float orbitRadiusY, float scale, int segments, 
						float updatePosX, float updatePosY, float rotationSpeed, bool isScale, bool isTranslate, bool isRotate, bool isDrawAsRing, glm::vec4 color, 
						bool useTexture, unsigned int textureID)
{
	//Use the shader pragram for the planets/objects, not the background shader
	glUseProgram(shaderProgram);

	// get the time to update the planet position 
	float time = (float)glfwGetTime();
	// Angle used to calculate the new position using time variable - move speed can be modified through the UI
	float angle = time * planetMoveSpeed;
	// Angle for rotation - rotation speed can be modified through the UI
	float angleRotate = time * rotationSpeed;

	// Store updated xy coordinates of planets in a variable
	float planetOrbitPosition_x = orbitRadiusX * cosf(angle) + updatePosX;
	float planetOrbitPosition_y = orbitRadiusY * sinf(angle) + updatePosY;
	
	glm::mat4 transformation = glm::mat4(1.0);
	// If an object needs to be translated, set the isTranslate to true and give it a moveSpeed
	if (isTranslate == true) {
		transformation = glm::translate(transformation, glm::vec3(planetOrbitPosition_x, planetOrbitPosition_y, 0.0));
	}
	// If an object needs to be scaled, set the isScale field to true and give it a scale factor
	if (isScale == true) {
		transformation = glm::scale(transformation, glm::vec3(scale, scale, 1.0));
	}
	// If the object needs to be rotated set the isRotate to true and give it a rotationSpeed
	if (isRotate == true) {
		transformation = glm::rotate(transformation, glm::radians(angleRotate), glm::vec3(0.0f, 0.0f, 1.0f));
	}

	// Get the location of "transform" variable in the shader program
	int transformLocation = glGetUniformLocation(shaderProgram, "transform");
	// Send the applied transfomation matrix to the vertex shader to aplly with every vertex in the planet/object
	glUniformMatrix4fv(transformLocation, 1, GL_FALSE, glm::value_ptr(transformation));
	// Get the location of "color" variable in the fragment shader and give it a color
	glUniform4f(glGetUniformLocation(shaderProgram, "color"), color.x, color.y, color.z, color.w);
	// Get the location of "useTexture" variable in the fragment shader and set the useTexture field to either true or false
	glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), useTexture);

	// If the planet/object has a texture, then use that texture in the fragment shader and bypass color attribute
	if (useTexture) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
	}

	// Tell the OpenGL that we want to apply all of the above transformations to the object contained in the specific VAO
	glBindVertexArray(VAO);
	// If we want to draw a ring (for example Saturn ring), set the isDrawAsRing field to true
	if (isDrawAsRing == true) {
		glDrawArrays(GL_LINE_LOOP, 0, segments);
	}
	// Otherwise, it will be drawn as a solid object/planet
	else {
		glDrawArrays(GL_TRIANGLE_FAN, 0, segments);
	}
	//unbind the VAO
	glBindVertexArray(0);

	// And return the updated xy coordinates back to where this function was called
	vector<float> updatedPlanetLocation;
	updatedPlanetLocation.push_back(planetOrbitPosition_x);
	updatedPlanetLocation.push_back(planetOrbitPosition_y);
	return updatedPlanetLocation;
}


// Main loop to run the Solar System simulation
int main()
{
	/*-----------------------------------------------------------------------
	Setup the Window
	-------------------------------------------------------------------------*/

	// Initialize GLFW
	glfwInit();

	// Tell GLFW what version of OpenGL we are using 
	// In this case we are using OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// Tell GLFW we are using the CORE profile
	// So that means we only have the modern functions
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create a GLFWwindow object of 950 by 950 pixels, naming it "Linear Transformations"
	GLFWwindow* window = glfwCreateWindow(950, 950, "Solar System", NULL, NULL);
	// Error check if the window fails to create
	if (window == NULL)
	{
		std::cout << "Failed to initialize the window object" << std::endl;
		glfwTerminate();
		return -1;
	}
	// Make the context of our window the main context in current window
	glfwMakeContextCurrent(window);

	// This function dynamically sets the viewport size when the user resizes window
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	//Load GLAD so it configures OpenGL
	//Glad helps getting the address of OpenGL functions which are OS specific
	gladLoadGL();

	/*---------------------------------------------------------------------------
	Setup and compile the Vertex and Fragment Shader programs
	----------------------------------------------------------------------------*/
	
	GLuint shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
	GLuint backgroundShaderProgram = createShaderProgram(backgroundVertexShaderSource, backgroundFragmentShaderSource);

	/*------------------------------------------------------------------------------
	 Set up VBO and VAO for the planets - used multiple VAOs to separate object data
	--------------------------------------------------------------------------------*/
	
	//Sun Buffers
	GLuint sunVAO, sunVBO;
	setupObjectBuffer(sunVAO, sunVBO, 0.05f, 0.05f, 100);
	
	//Mercury Buffers
	GLuint mercuryVAO, mercuryVBO, mercuryOrbitVAO, mercuryOrbitVBO;
	setupObjectBuffer(mercuryVAO, mercuryVBO, 0.05f, 0.05f, 100);
	setupObjectBuffer(mercuryOrbitVAO, mercuryOrbitVBO, 0.09f, 0.07f, 100);

	//Venus Buffer
	GLuint venusVAO, venusVBO, venusOrbitVAO, venusOrbitVBO;
	setupObjectBuffer(venusVAO, venusVBO, 0.05f, 0.05f, 100);
	setupObjectBuffer(venusOrbitVAO, venusOrbitVBO, 0.16f, 0.13f, 100);

	//Earth Buffers
	GLuint earthVAO, earthVBO, earthOrbitVAO, earthOrbitVBO;
	setupObjectBuffer(earthVAO, earthVBO, 0.05f, 0.05f, 100);
	setupObjectBuffer(earthOrbitVAO, earthOrbitVBO, 0.21f, 0.18f, 100);
	
	//Earth Moon Buffers
	GLuint earthMoonVAO, earthMoonVBO, earthMoonOrbitVAO, earthMoonOrbitVBO;
	setupObjectBuffer(earthMoonVAO, earthMoonVBO, 0.05f, 0.05f, 100);
	setupObjectBuffer(earthMoonOrbitVAO, earthMoonOrbitVBO, 0.05f, 0.04f, 100);

	//Mars Buffers
	GLuint marsVAO, marsMoonVBO, marsOrbitVAO, marsOrbitVBO;
	setupObjectBuffer(marsVAO, marsMoonVBO, 0.05f, 0.05f, 100);
	setupObjectBuffer(marsOrbitVAO, marsOrbitVBO, 0.32f, 0.29f, 100);

	//Asteroid Belt Buffer
	GLuint asteroidBeltVAO, asteroidBeltMoonVBO;
	setupObjectBuffer(asteroidBeltVAO, asteroidBeltMoonVBO, 0.05f, 0.05f, 100);
	
	//Jupiter Buffers
	GLuint jupiterVAO, jupiterVBO, jupiterOrbitVAO, jupiterOrbitVBO;
	setupObjectBuffer(jupiterVAO, jupiterVBO, 0.05f, 0.05f, 100);
	setupObjectBuffer(jupiterOrbitVAO, jupiterOrbitVBO, 0.52f, 0.49f, 100);

	//Jupiter Moons and their orbital Buffers
	GLuint jupiterMoon1VAO, jupiterMoon2VAO, jupiterMoon1VBO, jupiterMoon2VBO, 
		   jupiterMoon1OrbitVAO, jupiterMoon1OrbitVBO, jupiterMoon2OrbitVAO, jupiterMoon2OrbitVBO;
	setupObjectBuffer(jupiterMoon1VAO, jupiterMoon1VBO, 0.05f, 0.05f, 100);
	setupObjectBuffer(jupiterMoon2VAO, jupiterMoon2VBO, 0.05f, 0.05f, 100);
	setupObjectBuffer(jupiterMoon1OrbitVAO, jupiterMoon1OrbitVBO, 0.05f, 0.03f, 100);
	setupObjectBuffer(jupiterMoon2OrbitVAO, jupiterMoon2OrbitVBO, 0.07f, 0.05f, 100);


	//Saturn Buffers
	GLuint saturnVAO, saturnVBO, saturnOrbitVAO, saturnOrbitVBO;
	setupObjectBuffer(saturnVAO, saturnVBO, 0.05f, 0.05f, 100);
	setupObjectBuffer(saturnOrbitVAO, saturnOrbitVBO, 0.69f, 0.65f, 100);
	
	//Saturn Ring Buffers
	GLuint saturnRingVAO, saturnRingVBO;
	setupObjectBuffer(saturnRingVAO, saturnRingVBO, 0.05f, 0.05f, 100);

	//Uranus Buffers
	GLuint uranusVAO, uranusVBO, uranusOrbitVAO, uranusOrbitVBO;
	setupObjectBuffer(uranusVAO, uranusVBO, 0.05f, 0.05f, 100);
	setupObjectBuffer(uranusOrbitVAO, uranusOrbitVBO, 0.85f, 0.79f, 100);

	//Neptune Buffers
	GLuint neptuneVAO, neptuneVBO, neptuneOrbitVAO, neptuneOrbitVBO;
	setupObjectBuffer(neptuneVAO, neptuneVBO, 0.05f, 0.05f, 100);
	setupObjectBuffer(neptuneOrbitVAO, neptuneOrbitVBO, 0.95f, 0.89f, 100);

	//Comet Buffers
	GLuint cometVAO, cometVBO, cometOrbitVAO, cometOrbitVBO;
	setupObjectBuffer(cometVAO, cometVBO, 0.06f, 0.02f, 100);
	setupObjectBuffer(cometOrbitVAO, cometOrbitVBO, 0.95f, 0.89f, 100);

	// Get the background texture id to bind it
	unsigned int backgroundTextureID= loadTexture("textures/starryBackground.png");
	//Setup VAO and VBO buffers for the background texture
	GLuint backgroundVAO, backgroundVBO;
	setupBackgroundBuffers(backgroundVAO, backgroundVBO, backgroundVertices, sizeof(backgroundVertices) / sizeof(float));


	//--------------------Planet Texture IDs----------------------
	unsigned int sunTextureID = loadTexture("textures/sun.png");
	unsigned int mercuryTextureID = loadTexture("textures/mercury.png");
	unsigned int venusTextureID = loadTexture("textures/venus.png");
	unsigned int earthTextureID = loadTexture("textures/earth.png");
	unsigned int marsTextureID = loadTexture("textures/mars.png");
	unsigned int jupiterTextureID = loadTexture("textures/jupiter.png");
	unsigned int saturnTextureID = loadTexture("textures/saturn.png");
	unsigned int uranusTextureID = loadTexture("textures/uranus.png");
	unsigned int neptuneTextureID = loadTexture("textures/neptune.png");
	unsigned int moonTextureID = loadTexture("textures/moon.png");
	unsigned int ioTextureID = loadTexture("textures/io.png");
	unsigned int callistoTextureID = loadTexture("textures/callisto.png");

	/*------------------------------------------------------------------
	 Initialize the attributes of major celestial bodies such as planets
	--------------------------------------------------------------------*/

	// Sun's attributes
	CelestialBodies sun = {
		sunVAO, 0.0f, 0.0f, 0.0f, 0.9f, 100, 0.0f, 0.0f, 10.0f, true, true, true, true, false, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), sunTextureID
	};

	// Mercury's attributes
	CelestialBodies mercury = {
		mercuryVAO, 1.2f, 0.09f, 0.07f, 0.2f, 100, 0.0f, 0.0f, 50.0f, true, true, true, true, false, glm::vec4(0.42f, 0.38f, 0.35f, 1.0f), mercuryTextureID
	};

	// Venus's attributes
	CelestialBodies venus = {
		venusVAO, 0.9f, 0.16f, 0.13f, 0.24f, 100, 0.0f, 0.0f, 50.0f, true, true, true, true, false, glm::vec4(0.91f, 0.71f, 0.42f, 1.0f), venusTextureID
	};
	
	// Earth's attributes
	CelestialBodies earth = {
		earthVAO, 0.8f, 0.21f, 0.18f, 0.35f, 100, 0.0f, 0.0f, 50.0f, true, true, true, true,  false, glm::vec4(0.0f, 0.5f, 1.0f, 0.1f), earthTextureID
	};

	// Mars' attributes
	CelestialBodies mars = {
		marsVAO, 0.6f, 0.32f, 0.29f, 0.31f, 100, 0.0f, 0.0f, 50.0f, true, true, true, true, false, glm::vec4(0.80f, 0.36f, 0.23f, 1.0f), marsTextureID
	};
	
	// Jupiter's attributes
	CelestialBodies jupiter = {
		jupiterVAO, 0.4f, 0.52f, 0.49f, 0.6f, 100, 0.0f, 0.0f, 50.0f, true, true, true, true, false, glm::vec4(0.76f, 0.61f, 0.47f, 1.0f), jupiterTextureID
	};
	
	// Saturn's attributes
	CelestialBodies saturn = {
		saturnVAO, 0.3f, 0.69f, 0.65f, 0.43f, 100, 0.0f, 0.0f, 50.0f, true, true, true, true, false, glm::vec4(0.90f, 0.85f, 0.50f, 1.0f), saturnTextureID
	};
	
	// Uranus' attributes
	CelestialBodies uranus = {
		uranusVAO, 0.2f, 0.85f, 0.79f, 0.31f, 100, 0.0f, 0.0f, 50.0f, true, true, true, true, false, glm::vec4(0.4f, 0.6f, 0.8f, 1.0f), uranusTextureID
	};

	// Neptune's attributes
	CelestialBodies neptune = {
		neptuneVAO, 0.1f, 0.95f, 0.89f, 0.31f, 100, 0.0f, 0.0f, 50.0f, true, true, true, true, false, glm::vec4(0.2f, 0.3f, 0.8f, 1.0f), neptuneTextureID
	};

	// Earth Moon's attributes
	CelestialBodies moon = {
		earthMoonVAO, 1.3f, 0.04f,0.03f, 0.12f, 100, 0.0, 0.0, 50.0f, true, true, true, true, false, glm::vec4(0.72f, 0.72f, 0.72f, 1.0f), moonTextureID
	};

	// Jupiter Moon 1 attribute
	CelestialBodies jupiterMoonIo = {
		jupiterMoon1VAO, 0.8f, 0.05f,0.05f, 0.13f, 100, 0.0, 0.0, 50.0f, true, true, true, true, false, glm::vec4(1.0f, 0.85f, 0.35f, 1.0f), ioTextureID
	};

	// Jupiter Moon 2 attribute
	CelestialBodies jupiterMoonCallisto = {
		jupiterMoon2VAO, 0.6f, 0.07f,0.06f, 0.15f, 100, 0.0, 0.0, 50.0f, true, true, true, true, false, glm::vec4(0.85f, 0.24f, 0.21f, 1.0f), callistoTextureID
	};

	// Comet attributes
	CelestialBodies comet = {
		cometVAO, 0.2f, 0.5f,0.2f, 0.15f, 100, 0.0, 0.0, 50.0f, true, true, true, true, false, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f), 0
	};

	// Saturn ring 1 attributes
	CelestialBodies saturnRing1 = {
		saturnRingVAO, 0.0f, 0.0f, 0.0f, 0.765f, 100, 0.0, 0.0, 0.0f, true, true, true, true, true, glm::vec4(0.95f, 0.93f, 0.76f, 1.0f),0
	};
	// Saturn ring 2 attributes
	CelestialBodies SaturnRing2 = {
		saturnRingVAO, 0.0f, 0.0f, 0.0f, 0.68f, 100, 0.0, 0.0, 0.0f, true, true, true, true, true, glm::vec4(0.85f, 0.85f, 0.85f, 1.0f),0
	};
	// Saturn ring 3 attributes
	CelestialBodies SaturnRing3 = {
		saturnRingVAO, 0.0f, 0.0f, 0.0f, 0.64, 100, 0, 1, 0.0f, true, true, true, true, true, glm::vec4(0.95f, 0.93f, 0.76f, 1.0f), 0
	};

	//---------ImGui Library Setup (used for UI)---------
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
	//---------------------------------------------------

	// Variable to track user selected planet/object
	int selectedObject = 0;
	// A variable to control the visibility of asteroid belt
	bool isDrawAsteroidBelt = true;
	float asteroidBeltMoveSpeed = 0.07;

	// Initialize GIF
	GifWriter gifWriter;
	GifBegin(&gifWriter, "output.gif", 950, 950, 0);
	
	// rendering loop
	while (!glfwWindowShouldClose(window))
	{

		// Specify the color of the background
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		// Clean the back buffer and assign the new color to it
		// We need glClear since we do not want drawings to persist in the background
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use the starry sky background texture
		useBackgroundTexture(backgroundShaderProgram, backgroundVAO, backgroundTextureID);

		// setup needed for ImGui library inside the rendering loop
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		/*--------------------------------------------------------------------------------------------------
		 Draw the Planets and their associated objects depending on the visibility attributes of each planet
		 Draw the Orbit of Planets as elliptical ring, instead of solid object
		----------------------------------------------------------------------------------------------------*/
		
		//Draw Mars and its orbital
		if (mars.isVisible) {
			drawPlanet(shaderProgram, marsOrbitVAO, 0.0f, 0.32f, 0.29f, 0.31f, 100, 0.0f, 0.0f, 0.0f, false, false, false, true, glm::vec4(1.0f, 1.0f, 1.0f, 0.1f), false, 0);
			drawPlanet(shaderProgram, marsVAO, mars.moveSpeed, 0.32f, 0.29f, mars.scale, 100, 0.0f, 0.0f, mars.rotationSpeed, true, true, true, false, mars.color, true, mars.textureID);
		}

		//Draw asteroid belt between Mars and Jupite
		if (isDrawAsteroidBelt) {
			drawAsteroidBelt(shaderProgram, asteroidBeltVAO, asteroidBeltMoveSpeed);
		}

		//Draw Jupiter and its orbital
		if (jupiter.isVisible) {
			drawPlanet(shaderProgram, jupiterOrbitVAO, 0.0f, 0.52f, 0.49f, 0.6f, 100, 0.0f, 0.0f, 0.0f, false, false, false, true, glm::vec4(1.0f, 1.0f, 1.0f, 0.1f), false, 0);//orbital of jupiter
			vector<float> newJupiterLocation = drawPlanet(shaderProgram, jupiterVAO, jupiter.moveSpeed, 0.52f, 0.49f, jupiter.scale, 100, 0.0f, 0.0f, jupiter.rotationSpeed, true, true, true, false, jupiter.color, true, jupiter.textureID);


			//Draw 2 of Jupiter's Moons
			drawPlanet(shaderProgram, jupiterMoon1VAO, jupiterMoonIo.moveSpeed, 0.05f, 0.05f, jupiterMoonIo.scale, 100, newJupiterLocation[0], newJupiterLocation[1], jupiterMoonIo.rotationSpeed, true, true, true, false, jupiterMoonIo.color, true, jupiterMoonIo.textureID);
			drawPlanet(shaderProgram, jupiterMoon2VAO, jupiterMoonCallisto.moveSpeed, 0.07f, 0.06f, jupiterMoonCallisto.scale, 100, newJupiterLocation[0], newJupiterLocation[1], jupiterMoonCallisto.rotationSpeed, true, true, true, false, jupiterMoonCallisto.color, true, jupiterMoonCallisto.textureID);
		}

		//Draw Saturn and its orbital
		if (saturn.isVisible) {
			drawPlanet(shaderProgram, saturnOrbitVAO, 0.0f, 0.69f, 0.65f, 0.43f, 100, 0.0f, 0.0f, 0.0f, false, false, false, true, glm::vec4(1.0f, 1.0f, 1.0f, 0.1f), false, 0);
			vector<float> newSaturnLocation = drawPlanet(shaderProgram, saturnVAO, saturn.moveSpeed, 0.69f, 0.65f, saturn.scale, 100, 0.0f, 0.0f, saturn.rotationSpeed, true, true, true, false, saturn.color, true, saturn.textureID);

			//Draw 3 Saturn Belts - make the rings follow saturn by updateing passing saturn's new xy coordinates (this logic applies to all of moons as well)
			drawPlanet(shaderProgram, saturnRingVAO, 0.0f, 0.0f, 0.0f, 0.765f + saturn.scale, 100, newSaturnLocation[0], newSaturnLocation[1], 0.0f, true, true, false, true, glm::vec4(0.95f, 0.93f, 0.76f, 1.0f), false, 0);
			drawPlanet(shaderProgram, saturnRingVAO, 0.0f, 0.0f, 0.0f, 0.68f + saturn.scale, 100, newSaturnLocation[0], newSaturnLocation[1], 0.0f, true, true, false, true, glm::vec4(0.85f, 0.85f, 0.85f, 1.0f), false, 0);
			drawPlanet(shaderProgram, saturnRingVAO, 0.0f, 0.0f, 0.0f, 0.64 + saturn.scale, 100, newSaturnLocation[0], newSaturnLocation[1], 0.0f, true, true, false, true, glm::vec4(0.95f, 0.93f, 0.76f, 1.0f), false, 0);
		}

		//Draw Uranus and its orbital
		if (uranus.isVisible) {
			drawPlanet(shaderProgram, uranusOrbitVAO, 0.0f, 0.85f, 0.79f, 0.31f, 100, 0.0f, 0.0f, 0.0f, false, false, false, true, glm::vec4(1.0f, 1.0f, 1.0f, 0.1f), false, 0);
			drawPlanet(shaderProgram, uranusVAO, uranus.moveSpeed, 0.85f, 0.79f, uranus.scale, 100, 0.0f, 0.0f, uranus.rotationSpeed, true, true, true, false, uranus.color, true, uranus.textureID);
		}

		//Draw Neptune and its orbital
		if (neptune.isVisible) {
			drawPlanet(shaderProgram, neptuneOrbitVAO, 0.0f, 0.95f, 0.89f, 0.31f, 100, 0.0f, 0.0f, 0.0f, false, false, false, true, glm::vec4(1.0f, 1.0f, 1.0f, 0.1f), false, 0);
			drawPlanet(shaderProgram, neptuneVAO, neptune.moveSpeed, 0.95f, 0.89f, neptune.scale, 100, 0.0f, 0.0f, neptune.rotationSpeed, true, true, true, false, neptune.color, true, neptune.textureID);
		}

		// Draw a comet and its orbit
		if (comet.isVisible) {
			drawPlanet(shaderProgram, cometVAO, comet.moveSpeed, 0.5f, 0.2f, comet.scale, 100, 0.0, 0.0, comet.rotationSpeed, true, true, true, false, comet.color, false, 0);
		}


		//Draw Sun if visibility set to true
		if (sun.isVisible) {
			drawPlanet(shaderProgram, sunVAO, sun.moveSpeed, 0.0f, 0.0f, sun.scale, 100, 0.0f, 0.0f, sun.rotationSpeed, true, true, true, false, sun.color,true, sun.textureID);
		}

		//Draw Mercury and its orbital if visibility set to true
		if (mercury.isVisible) {
			drawPlanet(shaderProgram, mercuryOrbitVAO, 0.0f, 0.09f, 0.07, 0.2f, 100, 0.0f, 0.0f, 100.0f, false, false, false, true, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),false,0);
			drawPlanet(shaderProgram, mercuryVAO, mercury.moveSpeed, 0.09f, 0.07f, mercury.scale, 100, 0.0f, 0.0f, mercury.rotationSpeed, true, true, true, false, mercury.color,true,mercury.textureID);
		}

		//Draw Venus and its orbital if visibility set to true
		if (venus.isVisible) {
			drawPlanet(shaderProgram, venusOrbitVAO, 0.0f, 0.16f, 0.13f, 0.24f, 100, 0.0f, 0.0f, 0.0f, false, false, false, true, glm::vec4(1.0f, 1.0f, 1.0f, 0.5f),false,0);
			drawPlanet(shaderProgram, venusVAO, venus.moveSpeed, 0.16f, 0.13f, venus.scale, 100, 0.0f, 0.0f, venus.rotationSpeed, true, true, true, false, venus.color,true, venus.textureID);
		}

		//Draw Earth and its orbital if visibility set to true
		if (earth.isVisible) {
			drawPlanet(shaderProgram, earthOrbitVAO, 0.0f, 0.21f, 0.18f, 0.35f, 100, 0.0f, 0.0f, 0.0f, false, false, false, true, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), false, 0);
			vector<float> earthNewLocation = drawPlanet(shaderProgram, earthVAO, earth.moveSpeed, 0.21f, 0.18f, earth.scale, 100, 0.0f, 0.0f, earth.rotationSpeed, true, true, true, false, earth.color, true, earth.textureID);

			/*
			Draw Earth's Moon and its orbital
			For object that orbit around other planets rather than the Sun, need to be translated to match the correct planet; set the isTranslate argument to true
			That is why we are passing the Earth's new location to the drawPlanet function
			*/
			drawPlanet(shaderProgram, earthMoonVAO, moon.moveSpeed, 0.04f, 0.03f, moon.scale, 100, earthNewLocation[0], earthNewLocation[1], moon.rotationSpeed, true, true, true, false, moon.color, true, moon.textureID);
		}

		
		/*----------------------------------------------------------------------------
		  User has options to modify the planet attributes using the ImGui library
		------------------------------------------------------------------------------*/
		processInput(window, shaderProgram, selectedObject,sun, mercury,  venus,  earth, 
			mars,  jupiter,  saturn,  uranus, neptune,moon, jupiterMoonIo,  jupiterMoonCallisto, comet,  isDrawAsteroidBelt, asteroidBeltMoveSpeed);

		// Capture the frame
		std::vector<uint8_t> frame(950 * 950 * 4); // RGBA
		glReadPixels(0, 0, 950, 950, GL_RGBA, GL_UNSIGNED_BYTE, frame.data());
		
		// Flip the frame vertically
		for (int y = 0; y < 950 / 2; ++y) {
			for (int x = 0; x < 950; ++x) {
				int topIndex = (y * 950 + x) * 4;
				int bottomIndex = ((950 - y - 1) * 950 + x) * 4;

				// Swap the pixels
				std::swap(frame[topIndex], frame[bottomIndex]);
				std::swap(frame[topIndex + 1], frame[bottomIndex + 1]);
				std::swap(frame[topIndex + 2], frame[bottomIndex + 2]);
				std::swap(frame[topIndex + 3], frame[bottomIndex + 3]);
			}
		}
		// Add frame to GIF
		GifWriteFrame(&gifWriter, frame.data(), 950, 950, 0);

		// Swap the back buffer with the front buffer
		glfwSwapBuffers(window);
		// Take care of all GLFW events
		glfwPollEvents();
	}

	// end the ImGui
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	// end the gif writer
	GifEnd(&gifWriter);

	// Delete all the objects we've created
	/*glDeleteVertexArrays(1, &planet1VAO);
	glDeleteBuffers(1, &planet1VBO);*/
	glDeleteProgram(shaderProgram);
	// Delete window before ending the program
	glfwDestroyWindow(window);
	// Terminate GLFW before ending the program
	glfwTerminate();
	return 0;
}

 /*
 Funtion used in the render loop to process user input
 */
void processInput(GLFWwindow* window, unsigned int shaderProgram, int& selectedObject,
				  CelestialBodies& sun, CelestialBodies& mercury, CelestialBodies& venus, CelestialBodies& earth, 
			      CelestialBodies& mars, CelestialBodies& jupiter, CelestialBodies& saturn, CelestialBodies& uranus, CelestialBodies& neptune,
				  CelestialBodies& moon, CelestialBodies& jupiterMoonIo, CelestialBodies& jupiterMoonCallisto, CelestialBodies& comet, bool& isDrawAsteroidBelt, float &asteroidBeltMoveSpeed)
{
	
	// Names needed for selecting different celestial bodies in drop-down menu in ImGui render
	const char* celestialBodyNames[] = { "Sun", "Mercury", "Venus", "Earth", "Mars", "Jupiter", "Saturn", "Uranus", "Neptune", "Moon", "Io", "Callisto", "Comet", "Asteroid Belt" };
	
	// ESC key will cause the window to close
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	ImGui::Begin("Solar System Properties");
	const int celestialBodyCount = sizeof(celestialBodyNames) / sizeof(celestialBodyNames[0]);

	// Planet selection dropdown
	ImGui::Combo("Select Planet", &selectedObject, celestialBodyNames, celestialBodyCount);

	// Slider for modifying the properties of the selected planet
	switch (selectedObject) {
	case 0: // Sun
		ImGui::SliderFloat("Size", &sun.scale, 0.1f, 10.0f);			// size
		ImGui::SliderFloat("Rotation Speed", &sun.rotationSpeed, 0.0f, 200.0f);// rotation speed
		ImGui::ColorEdit4("Color", (float*)&sun.color);				// color
		ImGui::Checkbox("Add/Remove", &sun.isVisible);				// visibility
		break;
	case 1: // Mercury
		ImGui::SliderFloat("Size", &mercury.scale, 0.1f, 10.0f);		// size
		ImGui::SliderFloat("Rotation Speed", &mercury.rotationSpeed, 0.0f, 200.0f);// rotation speed
		ImGui::SliderFloat("Speed", &mercury.moveSpeed, 0.1f, 10.0f);// speed
		ImGui::ColorEdit4("Color", (float*)&mercury.color);			// color
		ImGui::Checkbox("Add/Remove", &mercury.isVisible);			// visibility
		break;
	case 2: // Venus
		ImGui::SliderFloat("Size", &venus.scale, 0.1f, 10.0f);		// size
		ImGui::SliderFloat("Rotation Speed", &venus.rotationSpeed, 0.0f, 200.0f);// rotation speed
		ImGui::SliderFloat("Speed", &venus.moveSpeed, 0.1f, 10.0f);	// speed
		ImGui::ColorEdit4("Color", (float*)&venus.color);			// color
		ImGui::Checkbox("Add/Remove", &venus.isVisible);			// visibility
		break;
	case 3: // Earth
		ImGui::SliderFloat("Size", &earth.scale, 0.1f, 10.0f);		// size
		ImGui::SliderFloat("Rotation Speed", &earth.rotationSpeed, 0.0f, 200.0f);// rotation speed
		ImGui::SliderFloat("Speed", &earth.moveSpeed, 0.1f, 10.0f);	// speed
		ImGui::ColorEdit4("Color", (float*)&earth.color);			// color
		ImGui::Checkbox("Add/Remove", &earth.isVisible);			// visibility
		break;
	case 4: // Mars
		ImGui::SliderFloat("Size", &mars.scale, 0.1f, 10.0f);		// size
		ImGui::SliderFloat("Rotation Speed", &mars.rotationSpeed, 0.0f, 200.0f);// rotation speed
		ImGui::SliderFloat("Speed", &mars.moveSpeed, 0.1f, 10.0f);	// speed
		ImGui::ColorEdit4("Color", (float*)&mars.color);			// color
		ImGui::Checkbox("Add/Remove", &mars.isVisible);				// visibility
		break;
	case 5: // Jupiter
		ImGui::SliderFloat("Size", &jupiter.scale, 0.1f, 10.0f);		// size
		ImGui::SliderFloat("Rotation Speed", &jupiter.rotationSpeed, 0.0f, 200.0f);// rotation speed
		ImGui::SliderFloat("Speed", &jupiter.moveSpeed, 0.1f, 10.0f);// speed
		ImGui::ColorEdit4("Color", (float*)&jupiter.color);			// color
		ImGui::Checkbox("Add/Remove", &jupiter.isVisible);			// visibility
		break;
	case 6: // Saturn
		ImGui::SliderFloat("Size", &saturn.scale, 0.1f, 10.0f);		// size
		ImGui::SliderFloat("Rotation Speed", &saturn.rotationSpeed, 0.0f, 200.0f);// rotation speed
		ImGui::SliderFloat("Speed", &saturn.moveSpeed, 0.1f, 10.0f);	// speed
		ImGui::ColorEdit4("Color", (float*)&saturn.color);			// color
		ImGui::Checkbox("Add/Remove", &saturn.isVisible);				// visibility
		break;
	case 7: // Uranus
		ImGui::SliderFloat("Size", &uranus.scale, 0.1f, 10.0f);		// size
		ImGui::SliderFloat("Rotation Speed", &uranus.rotationSpeed, 0.0f, 200.0f);// rotation speed
		ImGui::SliderFloat("Speed", &uranus.moveSpeed, 0.1f, 10.0f);	// speed
		ImGui::ColorEdit4("Color", (float*)&uranus.color);			// color
		ImGui::Checkbox("Add/Remove", &uranus.isVisible);			// visibility
		break;
	case 8: // Neptune
		ImGui::SliderFloat("Size", &neptune.scale, 0.1f, 10.0f);		// size
		ImGui::SliderFloat("Rotation Speed", &neptune.rotationSpeed, 0.0f, 200.0f);// rotation speed
		ImGui::SliderFloat("Speed", &neptune.moveSpeed, 0.1f, 10.0f);// speed
		ImGui::ColorEdit4("Color", (float*)&neptune.color);			// color
		ImGui::Checkbox("Add/Remove", &neptune.isVisible);			// visibility
		break;
	case 9:// Moon
		ImGui::SliderFloat("Size", &moon.scale, 0.1f, 10.0f);	 // size
		ImGui::SliderFloat("Rotation Speed", &moon.rotationSpeed, 0.0f, 200.0f);// rotation speed
		ImGui::SliderFloat("Speed", &moon.moveSpeed, 0.1f, 10.0f);// speed
		ImGui::ColorEdit4("Color", (float*)&moon.color);			// color
		ImGui::Checkbox("Add/Remove", &moon.isVisible);			// visibility
		break;
	case 10:// Io
		ImGui::SliderFloat("Size", &jupiterMoonIo.scale, 0.1f, 10.0f);	 // size
		ImGui::SliderFloat("Rotation Speed", &jupiterMoonIo.rotationSpeed, 0.0f, 200.0f);// rotation speed
		ImGui::SliderFloat("Speed", &jupiterMoonIo.moveSpeed, 0.1f, 10.0f);// speed
		ImGui::ColorEdit4("Color", (float*)&jupiterMoonIo.color);			// color
		ImGui::Checkbox("Add/Remove", &jupiterMoonIo.isVisible);			// visibility
		break;
	case 11:// Callisto
		ImGui::SliderFloat("Size", &jupiterMoonCallisto.scale, 0.1f, 10.0f);		// size
		ImGui::SliderFloat("Rotation Speed", &jupiterMoonCallisto.rotationSpeed, 0.0f, 200.0f);// rotation speed
		ImGui::SliderFloat("Speed", &jupiterMoonCallisto.moveSpeed, 0.1f, 10.0f);// speed
		ImGui::ColorEdit4("Color", (float*)&jupiterMoonCallisto.color);			// color
		ImGui::Checkbox("Add/Remove", &jupiterMoonCallisto.isVisible);			// visibility
		break;
	case 12:// Comet
		ImGui::SliderFloat("Size", &comet.scale, 0.1f, 10.0f);		// size
		ImGui::SliderFloat("Rotation Speed", &comet.rotationSpeed, 0.0f, 200.0f);// rotation speed
		ImGui::SliderFloat("Speed", &comet.moveSpeed, 0.1f, 10.0f);// speed
		ImGui::ColorEdit4("Color", (float*)&comet.color);			// color
		ImGui::Checkbox("Add/Remove", &comet.isVisible);			// visibility
		break;
	case 13:// Asteroid Belt
		ImGui::SliderFloat("Speed", &asteroidBeltMoveSpeed, 0.1f, 10.0f);// speed
		ImGui::Checkbox("Add/Remove", &isDrawAsteroidBelt);			// visibility
		break;
	}

	// End the ImGui function
	ImGui::End();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

/*------------------------------------------------------------------------------------------------------------
Helper function to create multiple small object/asteroid for asteroid belt which is called in the reder loop
It iterates for 100 times to make a complete angle rotation; where on each iteration an asteroid gets drawn
--------------------------------------------------------------------------------------------------------------*/

void drawAsteroidBelt(unsigned int shaderProgram, GLuint asteroidBeltVAO, float asteroidBeltSpeed) {
	// First belt
	for (int segment = 0; segment <= 100; segment += 2) {
		// Divide the circumference into segments and get the angle for segment
		float angle = (2.0 * M_PI * float(segment)) / 100.0f;
		// Get the value of x and y coordinates using trigonometric identities
		float x = ASTERIOD_BELT_RADIUS_X * cosf(angle + asteroidBeltSpeed * glfwGetTime());
		float y = ASTERIOD_BELT_RADIUS_Y * sinf(angle + asteroidBeltSpeed * glfwGetTime());
		// Draw the asteroid at the calculated xy coordinate. Essentially, asteroids in this case are just many small planets.
		drawPlanet(shaderProgram, asteroidBeltVAO, 0.5f, 0.00049f, 0.0005f, 0.05f, 100, x, y, 50.0f, true, true, true, false, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f), false, 0);
	}
	// Second Belt - larger one
	for (int segment = 0; segment <= 100; segment++) {
		// Divide the circumference into segments and get each angle for that point

		for (int i = 0; i <= 100; i++) {
			float angle = (2.0 * M_PI * float(segment))/ 100.0f;
			// Get the value of x and y coordinates using trigonometric identities
			float x = ASTERIOD_BELT_RADIUS2_X * cosf(angle + asteroidBeltSpeed * glfwGetTime());
			float y = ASTERIOD_BELT_RADIUS2_Y * sinf(angle + asteroidBeltSpeed * glfwGetTime());
			// Draw the asteroid at the calculated xy coordinate. Essentially, asteroids in this case are just many small planets.
			drawPlanet(shaderProgram, asteroidBeltVAO, 0.5f,0.0059f,0.0039f, 0.09f, 100, x, y, 50.0f, true, true, true, false, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f), false, 0);
		}
		

	}
	// Third Belt
	for (int segment = 0; segment <= 100; segment++) {
		// Divide the circumference into segments and get the angle for segment
		float angle = (2.0 * M_PI * float(segment)) / 100.0f;
		// Get the value of x and y coordinates using trigonometric identities
		float x = ASTERIOD_BELT_RADIUS3_X * cosf(angle + asteroidBeltSpeed * glfwGetTime());
		float y = ASTERIOD_BELT_RADIUS3_Y * sinf(angle + asteroidBeltSpeed * glfwGetTime());
		// Draw the asteroid at the calculated xy coordinate. Essentially, asteroids in this case are just many small planets.
		drawPlanet(shaderProgram, asteroidBeltVAO, 0.5f, 0.00019f, 0.00019f, 0.05f, 100, x, y, 50.0f, true, true, true, false, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f), false, 0);
	}
}

/*-------------------------------------------------------------------------------------------------
Helper function for buffers to send the vertex data to the GPU in batches using VAO and VBO buffers
This function takes 4 arguments:
   1) VAO variable to hold VBOs
   2) VBO variable to hold the vertex data
   3) Ellipse x-axis radius
   4) Ellipse y-axis radius
   5) Number of circle circumference segments that will be passed to the getCircleVertex() function
---------------------------------------------------------------------------------------------------*/
void setupObjectBuffer(GLuint& VAO, GLuint& VBO, float radius_x_axis, float radius_y_axis, int segments) {

	// Store the vertices of a circle in a vector of type float
	vector<float> vertices = getObjectVertices(radius_x_axis, radius_y_axis, segments);

	std::vector<float> data;
	// Pad the planet coordinates with its texture xy coordinates so that OpenGl knows how to apply its texture
	for (int i = 0; i < vertices.size() / 2; ++i) {
		data.push_back(vertices[2 * i]); // planet x coordinate
		data.push_back(vertices[2 * i + 1]); // planet y coordinate
		
		// importatnt to note that OpenGL uses normalised screen which ranges from -1 to 1 on x and y axis
		// So, when applying the texture to an object, we need to give the normalized xy coordinates of texture to the shader
		// This is why, we divide the length of xy coordinates by the width and height of panets respectively to normalize them
		data.push_back((vertices[2 * i] / (2 * radius_x_axis)) + 0.5f); // texture x coordinate
		data.push_back((vertices[2 * i + 1] / (2 * radius_y_axis)) + 0.5f); // texture y coordinate
	}

	// Generate the VAO and VBO with only 1 object each
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	// Make the VAO the current Vertex Array Object by binding it
	glBindVertexArray(VAO);
	// Bind the VBO specifying it's a GL_ARRAY_BUFFER
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// Introduce the vertices into the VBO
	glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
	// Configure the Vertex Attribute so that OpenGL knows how to read the VBO
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	// Enable the Vertex Attribute so that OpenGL knows to use it
	glEnableVertexAttribArray(0);

	// This is for the texture coordinates; so same as above introduce the vertex attributes
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	// Enable vertex attribute
	glEnableVertexAttribArray(1);

	// Bind both the VBO and VAO to 0 so that we don't accidentally modify the VAO and VBO we created
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

/*
This function is used to send background vertices to the GPU that will be used by the vertex and fragment shaders
*/

void setupBackgroundBuffers(GLuint& backgroundVAO, GLuint& backgroundVBO, float* backgroundVertices, size_t vertexCount) {
	// Generate the VAO and VBO
	glGenVertexArrays(1, &backgroundVAO);
	glGenBuffers(1, &backgroundVBO);

	// Bind the VAO
	glBindVertexArray(backgroundVAO);

	// Bind the VBO and set the buffer data
	glBindBuffer(GL_ARRAY_BUFFER, backgroundVBO);
	glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(float), backgroundVertices, GL_STATIC_DRAW);

	// Set up vertex attribute pointers
	// first 2 elements in the background vertex is for position
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// second 2 elements in the background vertex is for texture
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Unbind the VBO and VAO to avoid accidental modifications
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}


/*
Function used to tell the OpenGL how to use the background position and texture vertices
*/

void useBackgroundTexture(unsigned int shaderProgram, GLuint backgroundVAO, unsigned int backgroundTextureID) {
	// Activate the shader program
	glUseProgram(shaderProgram);

	// Enable texture usage in the shader
	glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), GL_TRUE);
	// Bind the background texture for rendering
	glBindTexture(GL_TEXTURE_2D, backgroundTextureID);
	// Set the active texture unit to 0
	glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0); // texture unit 0
	// Bind the background VAO containing vertex attributes
	glBindVertexArray(backgroundVAO);
	// Render the background using triangles
	glDrawArrays(GL_TRIANGLES, 0, 6); // Draw 6 vertices (2 triangles)
	// Unbind the VAO and texture
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

/*
Helper function to create and compile a shader program - returns shader ID
*/
GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource) {
	// Create Vertex Shader Object and get its reference
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	// Attach Vertex Shader source to the Vertex Shader Object
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	// Compile the Vertex Shader into machine code
	glCompileShader(vertexShader);

	// Create Fragment Shader Object and get its reference
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	// Attach Fragment Shader source to the Fragment Shader Object
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	// Compile the Fragment Shader into machine code
	glCompileShader(fragmentShader);

	// Create Shader Program Object and get its reference
	GLuint shaderProgram = glCreateProgram();
	// Attach the Vertex and Fragment Shaders to the Shader Program
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	// Wrap-up/Link all the shaders together into the Shader Program
	glLinkProgram(shaderProgram);

	// Delete the now useless shaders
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// return the shaderProgram ID as integer
	return shaderProgram;
}

/*
This function loads a texture from a user provided file path and generates an int texture ID
Returns the texture ID if successful, or 0 if the texture failed to load
*/

unsigned int loadTexture(char const* path)
{
	// variable to hold the textureID
	unsigned int textureID;
	stbi_set_flip_vertically_on_load(true);
	// Generate a texture object with its ID as well
	glGenTextures(1, &textureID);

	// Variables to hold texture dimensions and component count
	int width, height, nrComponents;
	// Load the texture data
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);

	if (data) // Check if the texture data was loaded witout any issues
	{
		// Determine the format based on the number of components
		GLenum format = GL_RGB; //rgb is the default
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		// Set the texture image data
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		// create a mipmaps for the texture
		glGenerateMipmap(GL_TEXTURE_2D);

		// Set the texture wrapping and filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Free the loaded image data
		stbi_image_free(data);
	}
	else // Handle loading failure
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
		return 0;
	}
	//return the texture ID
	return textureID;
}

/*
Function to adjust viewport dynamically
glfw: whenever the window size changed (by OS or user resize) this callback function executes
*/
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

