# GAMES101_Asnmt
#### IMPORTANT: Projects Framework are provided by https://sites.cs.ucsb.edu/~lingqi/teaching/games101.html
#### Student(LIKE ME) only implements some core algorithms in these projects.

---------------------------------------------------------------------------------------------------------------------------------------------------

## A1: rasterization the triangle to screen and support rotation


------------------------------------------------------------------------------------------------------------------------------------------------

## A2: coloring two triangle and do the correct block&show (using Z-buffers)
### without SSAA:
![image](https://github.com/bobhansky/GAMES101_Asnmt/blob/master/Results/SSAA_MSAA/original.png)


### With SSAA(4 samples per pixel)

![image](https://github.com/bobhansky/GAMES101_Asnmt/blob/master/Results/SSAA_MSAA/My_SSAA.png)


-------------------------------------------------------------------------------------------------------------------------------------------------


## A3: implementes different shaders and rasterization the cow.
### Phon-Shading:
![image](https://github.com/bobhansky/GAMES101_Asnmt/blob/master/Results/Phon_Shading_model/Phon-Shading.png)


### bump_shader：
![image](https://github.com/bobhansky/GAMES101_Asnmt/blob/master/Results/Phon_Shading_model/bump_shader.png)

### texture_shader:
![image](https://github.com/bobhansky/GAMES101_Asnmt/blob/master/Results/Phon_Shading_model/texture_shader.png)



--------------------------------------------------------------------------------------------------------------------------------------------------
## A4: Bezier Curve


--------------------------------------------------------------------------------------------------------------------------------------------------
## A5: Whitted Sytle Ray Tracing
![image](https://github.com/bobhansky/GAMES101_Asnmt/blob/master/Results/RayTracing/whitted_style/my_perfect_sol.jpg)


-----------------------------------------------------------------------------------------------------------------------------------
## A6: implement BVH traversal && ray Intesect BVH algorithm
![image](https://github.com/bobhansky/GAMES101_Asnmt/blob/master/Results/RayTracing/pathTracing/binary%20(1).jpg)

---------------------------------------------------------------------------------------------------------------------------------------------------
## A7: using Path Tracing to get a photo of Cornell Box
### WITHOUT MSAA
#### direct illumination SPP(Sample per pixel)=100:
![image](https://github.com/bobhansky/GAMES101_Asnmt/blob/master/Results/RayTracing/pathTracing/dir_SPP100.jpg)

#### global illumination SPP=100:
![image](https://github.com/bobhansky/GAMES101_Asnmt/blob/master/Results/RayTracing/pathTracing/glo_SPP100.jpg)

### WITH MSAA
#### direct illumination SPP(Sample per pixel)=100:
![image](https://github.com/bobhansky/GAMES101_Asnmt/blob/master/Results/RayTracing/pathTracing/CornellBox_final/dir_MSAA_SPP100.jpg)

#### global illumination SPP=100:
![image](https://github.com/bobhansky/GAMES101_Asnmt/blob/master/Results/RayTracing/pathTracing/CornellBox_final/glo_MSAA_SPP100.jpg)
