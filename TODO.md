1. 音频重采样有问题，有杂音和破音
2. 需要添加速度限制，现在有的游戏模拟速度过快，需要稳定在60fps
3. PPSSPP可能需要虚拟文件系统，需要加入
4. 手柄断开与连接时PSP核心会崩溃

## 变更 
1. 需要给video上下文添加下面方法来保存上下文协商
```c
virtual void setHWRenderContextNegotiationInterface(const void *) = 0;
```
2. 需重新实现硬件渲染时的上下文中的get_proc_address, 获取GL或者Vulkan的函数指针
3. 在创建Video组件时，除了看配置的video驱动，还要根据核心返回的要求来实例化对应的驱动 。比如Vulkan核心要求Vulkan驱动，OpenGL核心要求OpenGL驱动