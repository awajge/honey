/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

/*
The MIT License (MIT)

Copyright (c) 2015 Recep Aslantas <info@recp.me>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
This library [initially] used some [piece of] implementations
(may include codes) from these open source projects/resources:

1. Initial Affine Transforms
The original glm repo (g-truc), url: https://github.com/g-truc/glm

LICENSE[S]:
The Happy Bunny License (Modified MIT License)
The MIT License
Copyright (c) 2005 - 2016 G-Truc Creation

FULL LICENSE: https://github.com/g-truc/glm/blob/master/copying.txt

2. Initial Quaternions
Anton's OpenGL 4 Tutorials book source code:

LICENSE:
OpenGL 4 Example Code.
Accompanies written series "Anton's OpenGL 4 Tutorials"
Email: anton at antongerdelan dot net
First version 27 Jan 2014
Copyright Dr Anton Gerdelan, Trinity College Dublin, Ireland.

3. Euler Angles
David Eberly
Geometric Tools, LLC http://www.geometrictools.com/
Copyright (c) 1998-2016. All Rights Reserved.

Computing Euler angles from a rotation matrix (euler.pdf)
Gregory G. Slabaugh

4. Extracting Planes
Fast Extraction of Viewing Frustum Planes from the World-View-Projection Matrix
Authors:
Gil Gribb (ggribb@ravensoft.com)
Klaus Hartmann (k_hartmann@osnabrueck.netsurf.de)

5. Transform AABB
Transform Axis Aligned Bounding Boxes:
http://dev.theomader.com/transform-bounding-boxes/
https://github.com/erich666/GraphicsGems/blob/master/gems/TransBox.c

6. Cull frustum
http://www.txutxi.com/?p=584
http://old.cescg.org/CESCG-2002/DSykoraJJelinek/

7. Quaternions
Initial mat4_quat is borrowed from Apple's simd library

8. Vector Rotation using Quaternion
https://gamedev.stackexchange.com/questions/28395/rotating-vector3-by-a-quaternion

9. Sphere AABB intersect
https://github.com/erich666/GraphicsGems/blob/master/gems/BoxSphere.c

10. Horizontal add
https://stackoverflow.com/questions/6996764/fastest-way-to-do-horizontal-float-vector-sum-on-x86

11. de casteljau implementation and comments
https://forums.khronos.org/showthread.php/10264-Animations-in-1-4-1-release-notes-revision-A/page2?highlight=bezier
https://forums.khronos.org/showthread.php/10644-Animation-Bezier-interpolation
https://forums.khronos.org/showthread.php/10387-2D-Tangents-in-Bezier-Splines?p=34164&viewfull=1#post34164
https://forums.khronos.org/showthread.php/10651-Animation-TCB-Spline-Interpolation-in-COLLADA?highlight=bezier

12. vec2 cross product
http://allenchou.net/2013/07/cross-product-of-2d-vectors/

13. Ray triangle intersect
Möller–Trumbore ray-triangle intersection algorithm, from "Fast, Minimum Storage Ray/Triangle Intersection"
Authors:
Thomas Möller (tompa@clarus.se)
Ben Trumbore (wbt@graphics.cornell.edu)
Link to paper: http://webserver2.tecgraf.puc-rio.br/~mgattass/cg/trbRR/Fast%20MinimumStorage%20RayTriangle%20Intersection.pdf

14. ARM NEON: Matrix Vector Multiplication
https://stackoverflow.com/a/57793352/2676533

16. ARM NEON Div

http://github.com/microsoft/DirectXMath

17. Pick Matrix

glu project -> project.c

18. Ray sphere intersection

RAY TRACING GEMS
HIGH-QUALITY AND REAL-TIME RENDERING WITH DXR AND OTHER APIS

CHAPTER 7
Precision Improvements for Ray/Sphere Intersection
Eric Haines (1), Johannes Günther (2), and Tomas Akenine-Möller (1)
(1) NVIDIA
(2) Intel

Wyman, C., and Haines, E. Getting Started with RTX Ray Tracing.
https://github.com/NVIDIAGameWorks/GettingStartedWithRTXRayTracing
*/

#ifndef cglm_h
#define cglm_h

#include "common.h"
#include "vec2.h"
#include "vec3.h"
#include "vec4.h"
#include "ivec2.h"
#include "ivec3.h"
#include "ivec4.h"
#include "mat4.h"
#include "mat4x2.h"
#include "mat4x3.h"
#include "mat3.h"
#include "mat3x2.h"
#include "mat3x4.h"
#include "mat2.h"
#include "mat2x3.h"
#include "mat2x4.h"
#include "affine.h"
#include "cam.h"
#include "frustum.h"
#include "quat.h"
#include "euler.h"
#include "plane.h"
#include "noise.h"
#include "aabb2d.h"
#include "box.h"
#include "color.h"
#include "util.h"
#include "io.h"
#include "project.h"
#include "sphere.h"
#include "ease.h"
#include "curve.h"
#include "bezier.h"
#include "ray.h"
#include "affine2d.h"
#include "affine2d-post.h"

#endif /* cglm_h */
