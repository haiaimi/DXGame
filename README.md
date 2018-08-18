# DXGame
使用D3DX开发的游戏

### 1、小车的移动
  * 小车的材质是镜面反射，其实现原理就是使用6个摄像机和RenderTarget分别放在小车的（上、下、左、右、前、后以获取6个方向的画面，然后构成CubeMap来供小车采样。这6张RenderTargte只用于小车。
  * 小车本身未移动，使用材质偏移来实现移动效果，移动的是墙壁。由于会不停的加速，要配合好材质偏移速度和墙移动的速度。
  
  ![image](https://github.com/haiaimi/PictureRepository/blob/master/PictureRepository/%E5%B0%8F%E8%BD%A6/%E5%B0%8F%E8%BD%A6%E7%A7%BB%E5%8A%A8.gif)
  
  
 ### 2、小车跳跃
  * 哈哈，小车跳跃看起来略浮夸，只是用来越过障碍物
  ![image](https://github.com/haiaimi/PictureRepository/blob/master/PictureRepository/%E5%B0%8F%E8%BD%A6/%E5%B0%8F%E8%BD%A6%E8%B7%B3%E8%B7%83.gif)
  
  
 ### 3、游戏结束
  * 小车碰撞到墙壁的时候游戏结束，其本质上就是AABB碰撞检测，很简单，因为所有物体都是轴对齐的。
  * 结束的时候也使用了高斯模糊，就是对最终的RenderTarget进行处理，然后进行双向滤波处理。
  ![image](https://github.com/haiaimi/PictureRepository/blob/master/PictureRepository/%E5%B0%8F%E8%BD%A6/%E6%B8%B8%E6%88%8F%E7%BB%93%E6%9D%9F.png)
