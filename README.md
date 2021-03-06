# UE4ProceduralSkeletalMeshDemo

This demo shows how to generate skeletal mesh asset from program. 
Execute Content/EUW_NewSkeletalMesh.

![Spheres](ManySphere.jpeg "Spheres")

And this demo shows how to simulate slime.
Play Content\ThirdPersonCPP\Maps\ThirdPersonExampleMap

![Skeletal mesh to draw slime.](BoxWithTetrahedronJoints.jpeg "Skeletal mesh before setting material for slime.")
![Slime skeletal mesh.](slime2.gif "Slime skeletal mesh.")
![Slime down stairs.](SlimeDownStairs.gif "Slime down stairs.")

This has been tested on UE4.25.4 and 4.26.0, and only on Windows.

The material M_Slime is based on the material in https://github.com/DarknessFX/UE4Metaballs.
The liscence of the material is following that repogitry.
Read the README in that repogitry if you want to use the material for any reason.

This repositry includes the assets and source code files of Third Person template of UE4.
They are under the license which Epic Games Inc determined.

The source code files below Plugins/SPCRJointDynamics are copied from https://github.com/SPARK-inc/SPCRJointDynamicsUE4.
They are under the license which that repository defined.

The assets and source code files which is originaly made on this project is under MIT license.
