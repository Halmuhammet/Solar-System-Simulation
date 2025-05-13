# OpenGL Solar System Simulation
![Solar-System-Simulation](output.gif)
## Description
This project is an interactive OpenGL solar system simulation that allows users to explore celestial bodies, adjust their properties (like rotation speed), and visualize their movements in real-time. The project also includes the ability to record animations as GIFs.

## How to run this program
I have included all the libraries necessary to run this code in the same project folder, you will need to configure your IDE to know where to find the "include" and "imgui" folders.
-Make sure to add "imgui" to additional include directories in C/C++ General tab in project properties in Microsoft Visual Studio.
-Also, you will need to create a new filter for header files and source files and move the .h and .cpp files to their correct folders inside the Microsoft Visual studio. (This is explained in the video I linked below)

After having libraries ready, you should be able to run the program without any issues.
If you are encountering any issues, please follow the videos below; I myself followed those videos to include libraries

-Setting up OpenGL with GLFW and GLAD: https://youtu.be/XpBGwZNyUh0?si=2Pojy5B1uRwIOw-9
-Setting ImGui: https://youtu.be/VRwhNKoxUtk?si=rBh-FYgBmy1fLG5l

## Usage
You will see solar system with multiple planets moving. On the top left side, you can use the drop-down menu to select any planet/object you want and change their attributes such as move speed, rotation speed, etc. When done, you can click "ESC" on your keyboard.




