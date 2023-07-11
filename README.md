# DX12FrameCode
a frame code for me to coding dx12
I am changing it ,don't  hurry

![QQ截图20230711213129](https://github.com/Xiyinyue/DX12FrameCode/assets/83278582/c2179187-c574-4287-be56-37dcd1f4760a)
##### Chinese:
这是一个适用于普通**shadowmapping**以及**延迟渲染GBuffer**的DX12程序，你需要注意以下几点
1. 注意光源矩阵的构造
2. 本程序是基于**点光源**
3. 注意本程序采取的是**列向量**，所 以你需要注意对于列向量来说我们的乘法是如何构造的，是左乘还是右乘
4. 关于shadowmapping的文件放在`shadowmapping`分支里面
5. 注意对于shadowmapping PSO的构造和修改
6. 注意GBufferPSO的类型，以及Texture的类型
7. 这个程序有一个很烂的架构，所以不推荐学习
