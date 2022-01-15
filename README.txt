Instructions:
+/-: Change fov
Arrow Keys: change angle of perspective
0: reset view angle
1: Close viewing angle
2: Distant viewing angle
m/M: toggle light movement
[]: move light down/up
a/A: change ambient light output
d/D: change diffuse light output
s/S: change specular light output
e/E: change emmision
y/Y: Increase/decrease quads on basketball court

Things to note:
The ground contours are randomly generated, as is the placement of the grass tufts that are on it.
To make the 2d grass objects look 3d, I made them all roughly face the center of the court and randomized their
orientation slightly
I was going to use a DEM file from the internet for the ground, but I couldn't find any good ones so I decided
to randomly generate one that fit my needs.
The metal hoop itself was an algorithm I made that makes rings of quad strips that link together to make
a realistic looking hoop. This was different from any hoop approach I saw on the internet.
The skybox has some lines in it, but that is because I am bad at photoshop and couldn't seperate the parts of the
skybox properly.
To get a more complete view of the scene, press 2 and use the arrow keys to circle the area
Don't like the look of the court? Press 'Y' to give it more quads and more cracks! 'y' decreases it.

Code Reuse:
Many of the necessary functions that are used to display the objects are from class examples. I specifically say
which ones in the code itself. Additionally, I used one outside resource to draw circles on the hoop (this is also
documented in the code).

Time spent on project:
36 hours
