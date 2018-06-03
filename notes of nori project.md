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
```
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
```
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

**render** -- `static void render(Scene *scene, const std::string &filename)`  
1. 获得相机参数，输出图像大小 + reconstruction filter  
2. 获得积分器并preprocess  
3. 创建`BlockGenerator`和大小为`outputSize`的`ImageBlock`  
4. 创建`NORI_BLOCK_SIZE`个线程，每个线程内创建一个小的`ImageBlock`和对应的采样器`sampler`，每个小block的每个像素的每个采样点计算辐射度  
5. 并行执行线程，将所有block放到大的block内  
6. 输出图像并转化为Bitmap，保存至exr格式和png格式  

**scene** -- 

