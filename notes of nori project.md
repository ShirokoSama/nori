Notes of Nori Project -- Srf
========================

外部依赖external dependcy libraries
------------------------

pcg32 -- 伪随机数生成器  
filesystem -- 跨平台文件系统操作库  
pugixml -- 轻量级的xml解析库  
tinyformat -- type-safe c++11 printf and sprintf  
nanogui -- mini GUI for OpenGL  
OpenEXR(openexr) -- 高动态范围的图像格式库  
zlib -- OpenEXR使用的压缩库  
Intel's Thread Building Blocks -- 便于移植且灵活的并行库，在main.cpp的基础渲染循环中已并行化  
Eigen(eigen) -- 线性代数库，主要可能用到Point2i Point2f Point3f Vector2i Vector3f Vector2f Normal3f  
Hypoyhesis test support library -- 统计假设检验测试，判断|c' - c|是否可能由随机噪声造成

main.cpp 基础渲染循环
------------------------

.\nori.exe scene.xml执行程序  
若文件后缀为xml，则解析至NoriObject对象  

**NoriObject**(object.h/cpp内定义与实现)，是所有XML场景描述语言可构建的基类，其他接口(如相机)从这个类派生并暴露出更多具体的函数  

``` c++
enum EClassType = {  
        EScene = 0,  
        EMesh,  
        EBSDF,  
        EPhaseFunction,  
        EEmitter,  
        EMedium,  
        ECamera,  
        EIntegrator,  
        ESampler,  
        ETest,  
        EReconstructionFilter,  
        EClassTypeCount  
    };
EClassType getClassType();
static std::string classTypeName(EClassType type); //获得派生类类型  
void addChild(NoriObject *child); //添加子节点  
void setParent(NoriObject *parent); //设置父节点  
void activate(); //xml解析器在构造NorObject后调用这个函数  
std::string toString(); //输出字符串debug用  
```

*A trick*  
**NoriObjectFactory** -- NoriObject的工厂，本质是miniRTTI框架  

``` c++
static std::map<std::string, Constructor> *m_constructors; //成员变量，一个字符串-函数指针的map  
typedef std::function<NoriObject *(const PropertyList &)> Constructor; //指向构造函数的函数指针  
static void registerClass(const std::string &name, const Constructor &constr); //将类名与其构造函数注册到map内  
static NoriObject *createInstance(const std::string &name, const PropertyList &propList); //创建派生类的实例  
//一个预定义的宏用于调用registerClass进行注册  ##意为连接
#define NORI_REGISTER_CLASS(cls, name) \
    cls *cls ##_create(const PropertyList &list) { \
        return new cls(list); \
    } \
    static struct cls ##_{ \
        cls ##_() { \
            NoriObjectFactory::registerClass(name, cls ##_create); \
        } \
    } cls ##__NORI_;
/例如在Diffuse类的最后NORI_REGISTER_CLASS(Diffuse, "diffuse")，由于Diffuse_结构体是static的，会自动初始化，调用注册函数
```

**loadFromXML** -- 解析xml文件，递归解析，区分子节点和属性，返回根NoriObject  
从文件的根节点开始调用递归函数*parseTag(pugi::xml_node &node, PropertyList &list, int parentTag)*

1. 传入当前的xml_node + 父节点PropertyList的引用 + 父节点tag  
2. 获得当前node的tag  
3. 对所有子节点调用此函数，更新本节点的PropertyList，并获得子NoriObject数组  
4. 若当前node为NoriObject，则根据其type属性从*NoriObjectFactory*中实例化对象，如虚类*Integrator*则根据其type属性实例化*NormalIntegrator*或*SimpleIntegrator*等等，并执行*addChild*添加子节点数组中的对象  
5. 若当前node为为Property，则set传入的来自父节点的PropertyList  

**render** -- `static void render(Scene *scene, const std::string &filename)`  

1. 获得相机参数，输出图像大小 + reconstruction filter  
2. 获得积分器并preprocess  
3. 创建`BlockGenerator`和大小为`outputSize`的`ImageBlock`  
4. 创建`NORI_BLOCK_SIZE`个线程，每个线程内创建一个小的`ImageBlock`和对应的采样器`sampler`，每个小block的每个像素的每个采样点计算辐射度  
5. 并行执行线程，将所有block放到大的block内  
6. 输出图像并转化为Bitmap，保存至exr格式和png格式  

scene.h / scene.cpp 场景 - NoriObject的派生类
------------------------

```c++
// 成员变量
std::vector<Mesh *> m_meshes;
Integrator *m_integrator = nullptr;
Sampler *m_sampler = nullptr;
Camera *m_camera = nullptr;
Accel *m_accel = nullptr;
// 成员函数 省略constructor,destructor,getter,setter
bool rayIntersect(const Ray3f &ray, Intersection &its) const;   //调用m_accel的rayIntersect函数进行加速的相交检测
bool rayIntersect(const Ray3f &ray) const;                      //同上，但shadow_ray设为true，只判断是否相交，不计算额外信息
const BoundingBox3f &getBoundingBox() const;                    //获得包围盒
void activate();
void addChild(NoriObject *obj);
```

accel.h / accel.h - My Implement of Octree Accelration
------------------------

to be completed  

warp.h / warp.cpp - Monte Carlo Sampling
------------------------

首先生成两个0-1的独立随机变量到一个方形平面内，再利用inverse method，由PDF计算CDF，根据CDF的逆将两个独立随机变量映射到需求的分布上  

**squareToTentPdf** & **squareToTent**  
映射到长宽为-1到1的方形内，以“帐篷”状分布  

$$p(x, y)=p_1(x)\,p_1(y)\quad\text{and}\quad
    p_1(t) = \begin{cases}
    1-|t|, & -1\le t\le 1\\
    0,&\text{otherwise}\\
    \end{cases}$$
$$t =
    \begin{cases}
    \sqrt{2\xi}-1, & 0\le t\lt 0.5\\
    1 - \sqrt{2-2\xi}, & 0.5\le t\le 1
    \end{cases}$$

**squareToUniformDiskPdf** & **squareToUniformDisk**  
映射到圆心为原点，半径为1的圆盘内  

$$p(x, y) =
    \begin{cases}
    1/\pi, & x^2 + y^2 \le 1\\
    0, & otherwise
    \end{cases}$$
$$r = \sqrt{\xi_1} \quad \theta = 2\pi\xi_2$$

**squareToUniformSpherePdf** & **squareToUniformSphere**  
映射到圆心为球心，半径为1的球面上  

$$p(x, y, z) = 1/4\pi, \quad x^2 + y^2 + z^2 = 1$$
$$\theta = arccos(1-2\xi_1) \quad \phi = 2\pi\xi_2$$
$$x=sin\theta cos\phi \quad y=sin\theta sin\phi \quad z=cos\theta$$

**squareToUniformHemiSpherePdf** & **squareToUniformHemiSphere**  
映射到圆心为球心，半径为1的半球面上  

$$p(x, y, z) = 1/2\pi, \quad x^2 + y^2 + z^2 = 1 \quad z\ge0$$
$$\theta = arccos(1-\xi_1) \quad \phi = 2\pi\xi_2$$

**squareToCosineHemiSpherePdf** & **squareToCosineHemiSphere**  
映射到圆心为球心，半径为1的半球面上，概率与theta角的余弦成正比  

$$p(x, y, z) = z/\pi, \quad x^2 + y^2 + z^2 = 1 \quad z\ge0$$
$$\theta = \frac{arccos(1-2\xi_1)}{2} \quad \phi = 2\pi\xi_2$$

**squareToBeckmannPdf** & **squareToBeckmann**  
映射到贝克曼分布  

$$p(\omega) = {\frac{1}{2\pi}}\ \cdot\ {\frac{2 exp{\frac{-\tan^2{\theta}}{\alpha^2}}}{\alpha^2 \cos^3 \theta}}\!\!\!$$
$$\int_{0}^{2\pi}\int_0^{\frac{\pi}{2}} p(\omega) \sin\theta\,\mathrm{d}\theta\,\mathrm{d}\phi=1 \quad p(\theta, \phi) = p(\omega)sin\theta$$
$$\theta = arctan(\sqrt{-\alpha^2ln(1-\xi_1)}) \quad \phi=2\pi\xi_2$$

integrator.h -- Integrator抽象类/接口  
------------------------

积分器抽象类，必须实现的接口为  
*Color3f Li(const Scene \*scene, Sampler \*sampler, const Ray3f &ray) const*  
根据传入的光线ray与场景求交，获得相交信息，如交点位置、法线等，结合当前的光照信息和参数返回一个RGB颜色值  

simple.cpp -- 简单点光源Integrator My Implement
------------------------

读取配置文件获得点光源位置*Point3f m_position*和辐射能量*Color3f m_energy*  
辐射衰减公式为  

$$L(\mathbf{x})=\frac{\Phi}{4\pi^2} \frac{\mathrm{max}(0, \cos\theta)}{\|\mathbf{x}-\mathbf{p}\|^2} V(\mathbf{x}\leftrightarrow\mathbf{p}) \quad V(\mathbf{x}\leftrightarrow\mathbf{p}):=
    \begin{cases}
    1,&\text{if x and p are mutually visible}\\
    0,&\text{otherwise}
    \end{cases}$$

实现*Color3f Li*的步骤为  

1. 将传入的射线ray与场景相交检测，若无交点则返回0值(黑色)
2. 获得交点的位置，以其为原点，交点到光源的方向为延伸方向在场景中进行相交检测
3. 若相交则说明被遮挡，返回0值
4. 若不相交则根据衰减公式计算亮度

ao.cpp -- 环境光遮蔽Integrator My Implement
------------------------

假设场景内物体从各个方向接受均匀的照明，表面某点的亮度受是否被物体的其他部分遮挡所影响  
通过在物体表面某点为球心的半球面的各个方向进行积分来计算被遮蔽的程度，公式如下  

$$L(\mathbf{x})=\int_{H^2(\mathbf{x})}V(\mathbf{x}, \mathbf{x}+\alpha\omega)\,\frac{\cos\theta}{\pi}\,\mathrm{d}\omega$$

实现*Color3f Li*的步骤为  

1. 将传入的射线ray与场景相交检测，若无交点则返回0值(黑色)
2. 获得交点位置，以其为球心，在半球面上进行蒙特卡洛积分，采样方式见SquareToCosineHemiSphere的实现，以更好地匹配积分公式
3. 对每个采样获得的方向延伸出的射线求交，若无相交则加1，最后除以采样点数(见蒙特卡洛积分公式)
4. 返回改点求得的亮度

path_ems中间接照明和直接照明的结果直接相加？为何会过于亮？（猜测可能是因为在间接光的采样中采到了光源）  
应该如何控制光程衰减和迭代次数，在间接照明的递归中？  
mis里应该如何将对光源的采样（表面上离散采样）统一到点的半球面上？  
除pdf时如何不除0？  