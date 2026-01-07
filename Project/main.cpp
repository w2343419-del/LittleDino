#include <SFML/Graphics.hpp> // 引入SFML图形库头文件
#include <SFML/Audio.hpp>    // 引入SFML音频库头文件
#include <iostream>          // 引入标准输入输出流
#include <fstream>           // 引入文件流，用于文件读写
#include <vector>            // 引入向量容器
#include <string>            // 引入字符串类
#include <sstream>           // 引入字符串流，用于格式化字符串
#include <iomanip>           // 引入IO控制符，用于设置输出格式
#include <cstdlib>           // 引入标准库，用于随机数生成
#include <ctime>             // 引入时间库，用于随机数种子
#include <cmath>             // 引入数学库，用于数学计算

// ==========================================
// 全局常量配置
// ==========================================
const int WINDOW_WIDTH = 800;           // 游戏窗口宽度
const int WINDOW_HEIGHT = 400;          // 游戏窗口高度
const float GRAVITY = 0.8f;             // 重力加速度，每帧增加的垂直速度
const float JUMP_FORCE = -17.8f;        // 跳跃力度，负值表示向上
const float GROUND_Y = 250.0f;          // 地面在Y轴的坐标位置
const float SCORE_MULTIPLIER = 0.3f;    // 距离转分数的倍率
const float SPEED_MULTIPLIER = 1.4f;    // 游戏整体速度倍率
const float MAX_SPEED = 16.0f;          // 游戏最大滚动速度
const float MIN_BIRD_SPAWN_DISTANCE = 300.0f; // 鸟类生成的最小距离阈值

// ==========================================
// UI颜色定义
// ==========================================
const sf::Color UI_BG(255, 255, 255);         // 背景颜色（白色）
const sf::Color UI_PRIMARY(33, 150, 243);     // 主色调（蓝色）
const sf::Color UI_ACCENT(255, 82, 82);       // 强调色（红色）
const sf::Color UI_TEXT_DARK(45, 52, 54);     // 深色文字颜色
const sf::Color UI_TEXT_LIGHT(255, 255, 255); // 浅色文字颜色
const sf::Color UI_SHADOW(0, 0, 0, 30);       // 阴影颜色（带透明度的黑色）
const sf::Color UI_CARD_BG(255, 255, 255);    // 卡片背景颜色
const sf::Color UI_GOLD(255, 179, 0);         // 金色（用于最高分或强调）
const sf::Color UI_SUCCESS(76, 175, 80);      // 成功提示色（绿色）

// ==========================================
// 游戏状态枚举
// ==========================================
enum GameState { 
    MENU,       // 主菜单状态
    PLAYING,    // 游戏进行中状态
    INTRO,      // 游戏说明界面状态
    ABOUT,      // 关于界面状态
    GAME_OVER,  // 游戏结束状态
    COUNTDOWN   // 倒计时状态（用于暂停恢复或读档后）
};

// ==========================================
// 辅助工具函数
// ==========================================

// 将整数转换为字符串
std::string intToString(int value) {
    std::ostringstream oss; // 创建字符串输出流
    oss << value;           // 将整数写入流
    return oss.str();       // 返回字符串
}

// 格式化分数（例如将 10 格式化为 00010）
std::string formatScore(int value) {
    std::ostringstream oss; // 创建字符串输出流
    oss << std::setw(5) << std::setfill('0') << value; // 设置宽度为5，不足补0
    return oss.str();       // 返回格式化后的字符串
}

// 检查按键是否为跳跃键（空格、上箭头或W键）
bool isJumpKey(sf::Keyboard::Key key) {
    return (key == sf::Keyboard::Space || key == sf::Keyboard::Up || key == sf::Keyboard::W);
}

// 绘制居中文本
void drawCenteredText(sf::RenderWindow& window, sf::Text& text, float x, float y) {
    sf::FloatRect bounds = text.getLocalBounds(); // 获取文本的局部边界
    text.setOrigin(bounds.width / 2.0f, bounds.height / 2.0f); // 设置原点为文本中心
    text.setPosition(x, y); // 设置文本位置
    window.draw(text);      // 绘制文本
}

// ==========================================
// 数据存取函数
// ==========================================

// 读取最高分和金币记录
void loadHighData(int& hs, int& hc) {
    std::ifstream in("highscore.dat"); // 打开文件进行读取
    if (in.is_open()) { // 如果文件成功打开
        in >> hs >> hc; // 读取分数和金币
        in.close();     // 关闭文件
    } else {            // 如果文件不存在
        hs = 0;         // 初始化为0
        hc = 0;         // 初始化为0
    }
}

// 保存最高分和金币记录
void saveHighData(int hs, int hc) {
    std::ofstream out("highscore.dat"); // 打开文件进行写入
    if (out.is_open()) { // 如果文件成功打开
        out << hs << " " << hc; // 写入分数和金币
        out.close();     // 关闭文件
    }
}

// ==========================================
// UI 绘制函数
// ==========================================

// 绘制带阴影的卡片背景
void drawCard(sf::RenderWindow& window, float x, float y, float w, float h) {
    sf::RectangleShape shadow(sf::Vector2f(w, h)); // 创建阴影矩形
    shadow.setPosition(x + 5, y + 5); // 设置偏移位置
    shadow.setFillColor(UI_SHADOW);   // 设置阴影颜色
    window.draw(shadow);              // 绘制阴影
    
    sf::RectangleShape card(sf::Vector2f(w, h)); // 创建卡片矩形
    card.setPosition(x, y);           // 设置卡片位置
    card.setFillColor(UI_CARD_BG);    // 设置卡片背景色
    card.setOutlineThickness(1);      // 设置边框宽度
    card.setOutlineColor(sf::Color(200, 200, 200)); // 设置边框颜色
    window.draw(card);                // 绘制卡片
}

// 绘制按钮（支持悬停效果和颜色覆盖）
void drawButton(sf::RenderWindow& window, sf::Font& font, const std::string& label, float x, float y, float w, float h, bool hover, sf::Color overrideColor = UI_PRIMARY) {
    float offset = hover ? 2.0f : 0.0f; // 如果悬停，按钮轻微上浮
    sf::RectangleShape btn(sf::Vector2f(w, h)); // 创建按钮矩形
    btn.setPosition(x - offset, y - offset); // 设置按钮位置
    
    sf::Color btnColor = hover ? overrideColor : UI_CARD_BG; // 悬停变色
    sf::Color outlines = hover ? overrideColor : UI_TEXT_DARK; // 边框颜色变化
    
    btn.setFillColor(btnColor);       // 设置填充颜色
    btn.setOutlineThickness(2);       // 设置边框厚度
    btn.setOutlineColor(outlines);    // 设置边框颜色
    
    if (!hover) { // 如果未悬停，绘制底部阴影
        sf::RectangleShape shadow(sf::Vector2f(w, h));
        shadow.setPosition(x + 4, y + 4);
        shadow.setFillColor(UI_TEXT_DARK);
        window.draw(shadow);
    }
    window.draw(btn); // 绘制按钮主体

    sf::Text t; // 创建按钮文字
    t.setFont(font); t.setString(label); t.setCharacterSize(20);
    t.setFillColor(hover ? UI_TEXT_LIGHT : UI_TEXT_DARK); // 根据悬停状态改变文字颜色
    drawCenteredText(window, t, x + w/2 - offset, y + h/2 - offset - 4); // 绘制文字
}

// 绘制HUD（头部显示）项目
void drawHudItem(sf::RenderWindow& window, float x, float y, const std::string& label, const std::string& val, sf::Font& font, sf::Color color) {
    sf::RectangleShape cap(sf::Vector2f(140, 32)); // 创建背景条
    cap.setPosition(x, y);
    cap.setFillColor(sf::Color(255, 255, 255, 220)); // 半透明白色
    cap.setOutlineThickness(2);
    cap.setOutlineColor(UI_TEXT_DARK);
    window.draw(cap);

    sf::Text tL; tL.setFont(font); tL.setString(label); tL.setCharacterSize(14); tL.setFillColor(color);
    tL.setPosition(x + 10, y + 6); // 标签位置
    window.draw(tL);

    sf::Text tV; tV.setFont(font); tV.setString(val); tV.setCharacterSize(20); tV.setFillColor(UI_TEXT_DARK);
    tV.setPosition(x + 130 - tV.getLocalBounds().width, y + 3); // 数值右对齐
    window.draw(tV);
}

// ==========================================
// 游戏实体类定义
// ==========================================

// 恐龙（玩家）类
class Dino {
public:
    sf::Sprite sprite;      // 恐龙的精灵对象
    sf::Vector2f velocity;  // 速度向量
    bool onGround;          // 是否在地面标志
    sf::Clock animationClock; // 动画计时器
    sf::Texture run1Tex, run2Tex, jumpTex; // 跑步和跳跃的纹理
    bool showRun1;          // 当前显示的跑步帧
    float startY;           // 起始Y坐标（地面高度）

    // 构造函数
    Dino(const sf::Texture& r1, const sf::Texture& r2, const sf::Texture& j) : run1Tex(r1), run2Tex(r2), jumpTex(j), onGround(true), showRun1(true) {
        sprite.setTexture(run1Tex); // 初始纹理
        // 计算起始Y坐标，基于地面高度和纹理高度
        startY = (GROUND_Y + 30.0f) - static_cast<float>(run1Tex.getSize().y) + 12.0f;
        sprite.setPosition(50, startY); // 设置初始位置
        velocity.y = 0; // 初始垂直速度
    }

    // 跳跃动作
    void jump() { 
        if (onGround) { // 只有在地面时才能跳
            velocity.y = JUMP_FORCE; // 设置向上速度
            onGround = false; // 标记离地
            sprite.setTexture(jumpTex); // 切换为跳跃纹理
        } 
    }

    // 快速下落动作
    void fallFaster() { 
        if (!onGround) { // 只有在空中有效
            if (velocity.y < 0) velocity.y = 0; // 如果正在上升，立即停止上升
            velocity.y += 5.0f; // 增加向下速度
        } 
    }

    // 更新逻辑
    void update(float dt) {
        if (!onGround) { // 如果在空中
            velocity.y += GRAVITY; // 应用重力
            sprite.move(0, velocity.y); // 移动精灵
            // 如果落回地面
            if (sprite.getPosition().y >= startY) { 
                sprite.setPosition(50, startY); // 修正位置
                velocity.y = 0; // 速度归零
                onGround = true; // 标记着地
                showRun1 = true; 
                sprite.setTexture(run1Tex); // 恢复跑步纹理
            }
        }
        // 跑步动画逻辑
        if (onGround && animationClock.getElapsedTime().asMilliseconds() > 150) { 
            showRun1 = !showRun1; // 切换帧
            sprite.setTexture(showRun1 ? run1Tex : run2Tex); // 设置纹理
            animationClock.restart(); // 重置计时器
        }
    }

    // 获取碰撞边界（稍微缩小以优化手感）
    sf::FloatRect getBounds() const { 
        sf::FloatRect b = sprite.getGlobalBounds(); 
        return sf::FloatRect(b.left+8, b.top+8, b.width-16, b.height-16); 
    }

    // 绘制
    void draw(sf::RenderWindow& w) { w.draw(sprite); }
};

// 仙人掌类
class Cactus {
public:
    sf::Sprite sprite;    // 仙人掌精灵
    sf::Vector2f position;// 位置
    int type;             // 仙人掌类型

    // 初始化
    bool init(float x, int t, const sf::Texture& tex) {
        type = t; 
        sprite.setTexture(tex); 
        position.x = x;
        // 根据纹理高度计算Y坐标，使其贴地
        position.y = (GROUND_Y + 30.0f) - static_cast<float>(tex.getSize().y) + 15.0f;
        sprite.setPosition(position); 
        return true;
    }

    // 更新位置（向左移动）
    void update(float s) { 
        position.x -= s; 
        sprite.setPosition(position); 
    }

    // 碰撞检测
    bool checkCollision(const sf::FloatRect& o) const { 
        sf::FloatRect b = sprite.getGlobalBounds(); 
        // 缩小碰撞箱
        return sf::FloatRect(b.left+6, b.top+6, b.width-12, b.height-12).intersects(o); 
    }

    // 绘制
    void draw(sf::RenderWindow& w) { w.draw(sprite); }
};

// 金币类
class Coin {
public:
    sf::Sprite sprite;    // 金币精灵
    sf::Vector2f position;// 位置
    bool collected;       // 是否被收集

    // 初始化
    bool init(float x, float y, const sf::Texture& tex) { 
        sprite.setTexture(tex); 
        position = sf::Vector2f(x, y); 
        sprite.setPosition(position); 
        collected = false; 
        return true; 
    }

    // 更新位置
    void update(float s) { 
        position.x -= s; 
        sprite.setPosition(position); 
    }

    // 碰撞检测（略微扩大碰撞箱方便收集）
    bool checkCollision(const sf::FloatRect& o) const { 
        if(collected) return false; 
        sf::FloatRect b = sprite.getGlobalBounds(); 
        return sf::FloatRect(b.left-5, b.top-5, b.width+10, b.height+10).intersects(o); 
    }

    // 绘制（未收集时）
    void draw(sf::RenderWindow& w) { if(!collected) w.draw(sprite); }
};

// 鸟类（飞行障碍物）
class Bird {
public:
    sf::Sprite sprite;    // 鸟精灵
    sf::Vector2f position;// 位置
    sf::Clock wingClock;  // 翅膀动画计时器
    sf::Texture wingUpTex, wingDownTex; // 翅膀纹理
    bool showWingUp;      // 动画状态

    // 构造函数
    Bird(const sf::Texture& wu, const sf::Texture& wd) : wingUpTex(wu), wingDownTex(wd), showWingUp(true) {}

    // 初始化
    void init(float x, float y) { 
        position = sf::Vector2f(x, y); 
        sprite.setTexture(showWingUp ? wingUpTex : wingDownTex); 
        sprite.setPosition(position); 
    }

    // 更新位置和动画
    void update(float s, float dt) {
        position.x -= s; 
        sprite.setPosition(position);
        // 翅膀拍打逻辑
        if (wingClock.getElapsedTime().asSeconds() > 0.25f) { 
            showWingUp = !showWingUp; 
            sprite.setTexture(showWingUp ? wingUpTex : wingDownTex); 
            wingClock.restart(); 
        }
    }

    // 碰撞检测
    bool checkCollision(const sf::FloatRect& o) const { 
        sf::FloatRect b = sprite.getGlobalBounds(); 
        return sf::FloatRect(b.left+5, b.top+5, b.width-10, b.height-10).intersects(o); 
    }

    // 检查是否移出屏幕
    bool isOffScreen() const { return position.x + sprite.getGlobalBounds().width < 0; }
    
    // 绘制
    void draw(sf::RenderWindow& w) { w.draw(sprite); }
};

// ==========================================
// 资源管理与全局变量
// ==========================================
sf::Texture tDino1, tDino2, tJump, tCacL, tCacS1, tCacS2, tCoin, tTrack, tBirdU, tBirdD;
sf::Font font; 
sf::SoundBuffer shutBuf; 
sf::Sound shutSound; 
sf::Music bgm;

// 加载游戏资源
bool loadAssets() {
    bool ok = true;
    // 加载所有纹理文件
    ok &= tDino1.loadFromFile("DinoRun1.png"); ok &= tDino2.loadFromFile("DinoRun2.png"); ok &= tJump.loadFromFile("DinoJump.png");
    ok &= tCacL.loadFromFile("LargeCactus1.png"); ok &= tCacS1.loadFromFile("SmallCactus1.png"); ok &= tCacS2.loadFromFile("SmallCactus2.png");
    ok &= tCoin.loadFromFile("Coin.png"); ok &= tTrack.loadFromFile("Track.png");
    ok &= tBirdU.loadFromFile("BirdWingUp.png"); ok &= tBirdD.loadFromFile("BirdWingDown.png");
    // 加载字体和音频
    ok &= font.loadFromFile("Roboto-Regular.ttf"); ok &= shutBuf.loadFromFile("shutdown.wav");
    bgm.openFromFile("bgm.ogg"); bgm.setLoop(true); shutSound.setBuffer(shutBuf);
    return ok;
}

// ==========================================
// 存档系统
// ==========================================

// 保存游戏状态到文件
void saveGame(float d, int c, const Dino& dn, const std::vector<Cactus>& ca, const std::vector<Coin>& co, const std::vector<Bird>& bi) {
    std::ofstream out("savegame.txt");
    if (out.is_open()) {
        out << d << " " << c << "\n"; // 保存距离和金币
        out << dn.sprite.getPosition().y << " " << dn.velocity.y << " " << dn.onGround << "\n"; // 保存恐龙状态
        // 保存仙人掌数据
        out << ca.size() << "\n"; for(size_t i=0; i<ca.size(); ++i) out << ca[i].position.x << " " << ca[i].type << "\n";
        // 保存未收集金币数据
        int vc=0; for(size_t i=0; i<co.size(); ++i) if(!co[i].collected) vc++; out << vc << "\n";
        for(size_t i=0; i<co.size(); ++i) if(!co[i].collected) out << co[i].position.x << " " << co[i].position.y << "\n";
        // 保存鸟数据
        out << bi.size() << "\n"; for(size_t i=0; i<bi.size(); ++i) out << bi[i].position.x << " " << bi[i].position.y << "\n";
        out.close();
    }
}

// 从文件读取游戏状态
bool loadGame(float& d, int& c, Dino& dn, std::vector<Cactus>& ca, std::vector<Coin>& co, std::vector<Bird>& bi) {
    std::ifstream in("savegame.txt"); if (!in.is_open()) return false;
    ca.clear(); co.clear(); bi.clear(); // 清空当前实体
    in >> d >> c; // 读取距离和金币
    float dy, dvy; bool dog; in >> dy >> dvy >> dog; // 读取恐龙状态
    dn.sprite.setPosition(50, dy); dn.velocity.y = dvy; dn.onGround = dog;
    // 读取仙人掌
    int cnt; in >> cnt; sf::Texture* ts[3]; ts[0]=&tCacL; ts[1]=&tCacS1; ts[2]=&tCacS2;
    for(int i=0; i<cnt; ++i) { float x; int t; in >> x >> t; Cactus o; o.init(x, t, *ts[t%3]); ca.push_back(o); }
    // 读取金币
    in >> cnt; for(int i=0; i<cnt; ++i) { float x,y; in >> x >> y; Coin o; o.init(x, y, tCoin); co.push_back(o); }
    // 读取鸟
    in >> cnt; for(int i=0; i<cnt; ++i) { float x,y; in >> x >> y; Bird o(tBirdU, tBirdD); o.init(x, y); bi.push_back(o); }
    in.close(); return true;
}

// 自动生成一个简单的WAV文件（如果找不到文件时使用）
void generateShutdownWav() {
    const int sr = 44100; const double dur = 0.5; std::vector<short> s(sr * dur);
    for (size_t i = 0; i < s.size(); ++i) s[i] = static_cast<short>(10000 * sin(2 * 3.14159 * 440 * i / sr)); // 生成正弦波
    std::ofstream f("shutdown.wav", std::ios::binary);
    int sz = s.size() * 2; int h[11] = {0x46464952, 36+sz, 0x45564157, 0x20746d66, 16, 0x10001, 44100, 44100*2, 0x100002, 0x61746164, sz}; // WAV头
    f.write((char*)h, 44); f.write((char*)s.data(), sz);
}

// ==========================================
// 主函数
// ==========================================
int main() {
    std::srand((unsigned int)std::time(0)); // 初始化随机数种子
    
    // 制作团队信息（用于Credits）
    std::vector<std::string> team;
    team.push_back("Game Created By:");
    team.push_back("Yao Wang");
    team.push_back("Solo Developer");

    // 检查并生成音效文件，加载资源
    { std::ifstream c("shutdown.wav"); if(!c.is_open()) generateShutdownWav(); }
    if (!loadAssets()) { std::cerr << "Asset Error: Please ensure all .png and .ttf files are in the directory.\n"; return -1; }

    // 创建游戏窗口
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Little Dino - Final");
    window.setFramerateLimit(60); // 限制帧率为60

    // 创建游戏视图（用于适配窗口缩放）
    sf::View gameView(sf::FloatRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT));
    window.setView(gameView);

    // 读取本地最高记录
    int highScore = 0;
    int highCoins = 0;
    loadHighData(highScore, highCoins);

    // 初始化游戏变量
    GameState state = MENU; // 初始状态为菜单
    float dist = 0.0f; int coins = 0; float spd = 0.0f; // 距离、金币、速度
    float spawnTimer=0, coinSpawnTimer=0, birdTimer=0;  // 生成计时器
    bool paused = false; bool savedMsg = false; sf::Clock msgClk; // 暂停和消息状态
    
    // 倒计时变量
    int countdownVal = 3;
    float countdownTime = 0.0f;

    // 实例化游戏对象
    Dino dino(tDino1, tDino2, tJump);
    std::vector<Cactus> cacti; std::vector<Coin> coinList; std::vector<Bird> birds;
    sf::Sprite g1(tTrack), g2(tTrack); // 双背景滚动
    g1.setPosition(0, GROUND_Y + 30); g2.setPosition(tTrack.getSize().x, GROUND_Y + 30);

    // 菜单选项列表
    std::vector<std::string> menu;
    menu.push_back("Start Adventure");
    menu.push_back("Load Save");
    menu.push_back("How to Play");
    menu.push_back("Credits");
    menu.push_back("Exit");

    // 暂停菜单选项列表
    std::vector<std::string> pauseMenu;
    pauseMenu.push_back("Resume");
    pauseMenu.push_back("Save Game");
    pauseMenu.push_back("Main Menu");

    // 游戏主循环
    while (window.isOpen()) {
        float dt = 1.0f / 60.0f; // 固定时间步长
        sf::Event e;
        
        // 事件处理循环
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) window.close(); // 处理关闭请求
            
            // 处理窗口大小调整
            if (e.type == sf::Event::Resized) {
                gameView.reset(sf::FloatRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT));
                window.setView(gameView);
            }

            // 获取鼠标位置并转换为世界坐标
            sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
            sf::Vector2f worldPos = window.mapPixelToCoords(pixelPos);

            // --- 菜单状态逻辑 ---
            if (state == MENU) {
                if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left) {
                    for (int i = 0; i < 5; ++i) {
                        float bx = WINDOW_WIDTH/2 - 110; float by = 130 + i * 48;
                        // 检查按钮点击
                        if (worldPos.x > bx && worldPos.x < bx+220 && worldPos.y > by && worldPos.y < by+40) {
                            if (i==0) { // 开始新游戏
                                state=PLAYING; dist=0; coins=0; dino=Dino(tDino1, tDino2, tJump); 
                                cacti.clear(); coinList.clear(); birds.clear(); 
                                spd=4.0f*SPEED_MULTIPLIER; bgm.play(); 
                            }
                            else if (i==1) { // 读取存档
                                if(loadGame(dist, coins, dino, cacti, coinList, birds)) { 
                                    state = COUNTDOWN; // 进入倒计时
                                    countdownVal = 3; 
                                    countdownTime = 0.0f; 
                                    paused = false; 
                                    spd = 4.0f*SPEED_MULTIPLIER+(dist/100.0f)*0.8f; 
                                    if(spd>MAX_SPEED) spd=MAX_SPEED; 
                                    bgm.play(); 
                                } 
                            }
                            else if (i==2) state=INTRO; // 玩法说明
                            else if (i==3) state=ABOUT; // 关于页面
                            else if (i==4) window.close(); // 退出
                        }
                    }
                }
            }
            // --- 游戏状态逻辑 ---
            else if (state == PLAYING) {
                if (e.type == sf::Event::KeyPressed) {
                    // 处理P键暂停
                    if (e.key.code == sf::Keyboard::P) {
                        if (!paused) {
                            paused = true; // 暂停
                        } else {
                            // 从暂停恢复，进入倒计时
                            paused = false;
                            state = COUNTDOWN;
                            countdownVal = 3;
                            countdownTime = 0.0f;
                        }
                    }
                    if (e.key.code == sf::Keyboard::Escape) { state = MENU; bgm.stop(); } // ESC返回菜单
                    // 快捷键K保存
                    if (paused && e.key.code == sf::Keyboard::K) { 
                        saveGame(dist, coins, dino, cacti, coinList, birds); 
                        savedMsg = true; msgClk.restart(); 
                    }
                    // 处理跳跃
                    if (!paused && isJumpKey(e.key.code)) dino.jump();
                }
                
                // 处理暂停菜单点击
                if (paused && e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left) {
                    for (int i = 0; i < 3; ++i) {
                        float bx = WINDOW_WIDTH/2 - 110; float by = 160 + i * 50;
                        if (worldPos.x > bx && worldPos.x < bx+220 && worldPos.y > by && worldPos.y < by+40) {
                            if (i == 0) { // Resume
                                paused = false; 
                                state = COUNTDOWN; // 进入倒计时
                                countdownVal = 3; 
                                countdownTime = 0.0f; 
                            } 
                            else if (i == 1) { // Save Game
                                saveGame(dist, coins, dino, cacti, coinList, birds); 
                                savedMsg = true; msgClk.restart(); 
                            }
                            else if (i == 2) { state = MENU; bgm.stop(); } // Return to Menu
                        }
                    }
                }
            }
            // --- 倒计时状态逻辑 ---
            else if (state == COUNTDOWN) {
                 // 倒计时期间允许按ESC退出
                 if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Escape) {
                     state = MENU; bgm.stop();
                 }
            }
            // --- 说明/关于状态逻辑 ---
            else if (state == INTRO || state == ABOUT) {
                if (e.type == sf::Event::KeyPressed && (e.key.code == sf::Keyboard::Escape || e.key.code == sf::Keyboard::Return)) state = MENU;
            }
            // --- 游戏结束状态逻辑 ---
            else if (state == GAME_OVER) {
                if (e.type == sf::Event::KeyPressed) {
                    // R键重开
                    if (e.key.code == sf::Keyboard::R) { 
                        state=PLAYING; dist=0; coins=0; dino=Dino(tDino1, tDino2, tJump); 
                        cacti.clear(); coinList.clear(); birds.clear(); 
                        spd=4.0f*SPEED_MULTIPLIER; bgm.play(); 
                    }
                    else if (e.key.code == sf::Keyboard::Escape) state = MENU; // ESC返回菜单
                }
            }
        }

        // 获取最新的鼠标位置用于UI悬停效果
        sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
        sf::Vector2f worldPos = window.mapPixelToCoords(pixelPos);

        // --- 更新逻辑 ---

        // 如果是倒计时状态
        if (state == COUNTDOWN) {
            countdownTime += dt;
            if (countdownTime >= 1.0f) { // 每秒减少一次计数
                countdownVal--;
                countdownTime = 0.0f;
            }
            if (countdownVal <= 0) { // 倒计时结束
                state = PLAYING; // 开始游戏
            }
        }
        // 如果是游戏进行中状态且未暂停
        else if (state == PLAYING && !paused) {
            // 更新恐龙
            dino.update(dt); 
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) dino.fallFaster(); // 按下加速下落

            // 更新距离和速度
            dist += spd * dt;
            float target = 4.0f * SPEED_MULTIPLIER + (dist / 100.0f) * 0.8f; 
            if (target > MAX_SPEED) target = MAX_SPEED; 
            spd = target;
            
            // 生成仙人掌
            spawnTimer += dt;
            if (spawnTimer > 1.5f + (rand()%15)/10.0f) {
                Cactus c; sf::Texture* ts[3]; ts[0]=&tCacL; ts[1]=&tCacS1; ts[2]=&tCacS2; int t = rand()%3;
                if (c.init(WINDOW_WIDTH + 20, t, *ts[t])) cacti.push_back(c); 
                spawnTimer = 0;
            }
            
            // 生成金币
            coinSpawnTimer += dt;
            // 随机间隔 3.0 ~ 5.0 秒
            if (coinSpawnTimer > 3.0f + (rand() % 20) / 10.0f) {
                if (rand() % 100 < 50) { // 50% 概率生成
                    Coin c; 
                    bool safe = true; 
                    float cx = WINDOW_WIDTH + 100 + rand() % 100; 
                    
                    // 安全检测：避免与仙人掌或鸟重叠
                    for(size_t i = 0; i < cacti.size(); ++i) if(std::abs(cacti[i].position.x - cx) < 100) safe = false; 
                    for(size_t i = 0; i < birds.size(); ++i) if(std::abs(birds[i].position.x - cx) < 100) safe = false;

                    if(safe) {
                        if(c.init(cx, 90.0f, tCoin)) coinList.push_back(c); 
                    }
                } 
                coinSpawnTimer = 0;
            }

            // 生成鸟
            if (dist > MIN_BIRD_SPAWN_DISTANCE) {
                birdTimer += dt; 
                if (birdTimer > 4.0f) { 
                    float birdSpawnX = WINDOW_WIDTH + 50;
                    bool safe = true;
                    // 安全检测：避免与金币或仙人掌重叠
                    for(size_t i = 0; i < coinList.size(); ++i) if(std::abs(coinList[i].position.x - birdSpawnX) < 100) safe = false;
                    for(size_t i = 0; i < cacti.size(); ++i) if(std::abs(cacti[i].position.x - birdSpawnX) < 80) safe = false;

                    if (safe) {
                        Bird b(tBirdU, tBirdD); b.init(birdSpawnX, 130.0f); birds.push_back(b); birdTimer = 0; 
                    } else { birdTimer = 3.5f; } // 如果不安全，延迟重试
                }
            }

            // 更新所有障碍物位置
            for(size_t i=0; i<cacti.size(); ++i) cacti[i].update(spd);
            for(size_t i=0; i<coinList.size(); ++i) coinList[i].update(spd);
            for(size_t i=0; i<birds.size(); ++i) birds[i].update(spd, dt);

            // 碰撞检测
            sf::FloatRect pr = dino.getBounds();
            bool collision = false;
            for(size_t i=0; i<cacti.size(); ++i) if (cacti[i].checkCollision(pr)) collision = true;
            for(size_t i=0; i<birds.size(); ++i) if (birds[i].checkCollision(pr)) collision = true;
            
            // 如果发生碰撞
            if (collision) {
                state = GAME_OVER; bgm.stop(); shutSound.play();
                // 检查并保存最高记录
                int currentScore = (int)(dist * SCORE_MULTIPLIER);
                bool updated = false;
                if (currentScore > highScore) { highScore = currentScore; updated = true; }
                if (coins > highCoins) { highCoins = coins; updated = true; }
                if (updated) saveHighData(highScore, highCoins);
            }

            // 收集金币
            for(size_t i=0; i<coinList.size(); ++i) if (coinList[i].checkCollision(pr)) { coinList[i].collected = true; coins++; }

            // 清理移除屏幕的物体
            for(int i=cacti.size()-1; i>=0; --i) if(cacti[i].position.x < -100) cacti.erase(cacti.begin()+i);
            for(int i=coinList.size()-1; i>=0; --i) if(coinList[i].collected || coinList[i].position.x < -50) coinList.erase(coinList.begin()+i);
            for(int i=birds.size()-1; i>=0; --i) if(birds[i].isOffScreen()) birds.erase(birds.begin()+i);

            // 背景循环滚动逻辑
            float tw = tTrack.getSize().x; g1.move(-spd, 0); g2.move(-spd, 0);
            if(g1.getPosition().x+tw <= 0) g1.setPosition(g2.getPosition().x+tw, GROUND_Y+30);
            if(g2.getPosition().x+tw <= 0) g2.setPosition(g1.getPosition().x+tw, GROUND_Y+30);
        }

        // --- 渲染逻辑 ---

        window.clear(UI_BG); // 清除屏幕，填充背景色

        // 绘制菜单
        if (state == MENU) {
            sf::RectangleShape stripe(sf::Vector2f(WINDOW_WIDTH, 100)); // 装饰条
            stripe.setFillColor(sf::Color(230, 230, 240));
            window.draw(stripe);

            sf::Text title; // 游戏标题
            title.setFont(font); title.setString("LITTLE DINO"); title.setCharacterSize(60);
            title.setFillColor(UI_TEXT_DARK); title.setStyle(sf::Text::Bold);
            
            // 标题阴影
            sf::Text ts = title; ts.setFillColor(sf::Color(200, 200, 200)); ts.setPosition(WINDOW_WIDTH/2 + 4, 64);
            sf::FloatRect tb = title.getLocalBounds(); title.setOrigin(tb.width/2, tb.height/2); ts.setOrigin(tb.width/2, tb.height/2);
            window.draw(ts);
            title.setPosition(WINDOW_WIDTH/2, 60);
            window.draw(title);

            // 显示最高分
            sf::Text hsText;
            hsText.setFont(font); 
            std::string rec = "BEST SCORE: " + formatScore(highScore) + "   BEST COINS: " + intToString(highCoins);
            hsText.setString(rec);
            hsText.setCharacterSize(18);
            hsText.setFillColor(UI_PRIMARY);
            drawCenteredText(window, hsText, WINDOW_WIDTH/2, 100);

            // 绘制菜单按钮
            for (int i = 0; i < 5; ++i) {
                float bx = WINDOW_WIDTH/2 - 110; float by = 130 + i * 48;
                bool hover = (worldPos.x > bx && worldPos.x < bx+220 && worldPos.y > by && worldPos.y < by+40);
                drawButton(window, font, menu[i], bx, by, 220, 40, hover);
            }
        }
        // 绘制说明界面
        else if (state == INTRO) {
            drawCard(window, 50, 40, WINDOW_WIDTH-100, WINDOW_HEIGHT-80);
            sf::Text t; t.setFont(font); t.setFillColor(UI_PRIMARY);
            t.setCharacterSize(32); t.setStyle(sf::Text::Bold); t.setString("HOW TO PLAY");
            drawCenteredText(window, t, WINDOW_WIDTH/2, 80);
            t.setCharacterSize(20); t.setStyle(sf::Text::Regular); t.setFillColor(UI_TEXT_DARK);
            std::string content = 
                "Objective:\n  Run as far as you can and collect coins!\n\n"
                "Controls:\n"
                "  [SPACE] / [UP]   Jump\n"
                "  [DOWN]           Drop Fast\n"
                "  [P]              Pause Menu\n"
                "  [ESC]            Back to Menu";
            t.setString(content);
            t.setPosition(100, 130); t.setOrigin(0,0);
            window.draw(t);
            t.setString("[ ENTER to Return ]"); t.setCharacterSize(18); t.setFillColor(UI_ACCENT);
            drawCenteredText(window, t, WINDOW_WIDTH/2, 320);
        }
        // 绘制关于界面
        else if (state == ABOUT) {
            drawCard(window, 150, 50, 500, 300);
            sf::Text t; t.setFont(font); t.setFillColor(UI_PRIMARY);
            t.setCharacterSize(32); t.setStyle(sf::Text::Bold); t.setString("CREDITS");
            drawCenteredText(window, t, WINDOW_WIDTH/2, 90);
            t.setCharacterSize(24); t.setStyle(sf::Text::Regular); t.setFillColor(UI_TEXT_DARK);
            float startY = 150;
            for(size_t i = 0; i < team.size(); ++i) {
                t.setString(team[i]);
                drawCenteredText(window, t, WINDOW_WIDTH/2, startY);
                startY += 40;
            }
            t.setString("[ ENTER to Return ]"); t.setCharacterSize(18); t.setFillColor(UI_ACCENT);
            drawCenteredText(window, t, WINDOW_WIDTH/2, 310);
        }
        // 绘制游戏画面（PLAYING 或 COUNTDOWN 状态）
        else if (state == PLAYING || state == COUNTDOWN) {
            window.draw(g1); window.draw(g2); // 绘制地面
            dino.draw(window); // 绘制恐龙
            for(size_t i=0; i<cacti.size(); ++i) cacti[i].draw(window); // 绘制仙人掌
            for(size_t i=0; i<coinList.size(); ++i) coinList[i].draw(window); // 绘制金币
            for(size_t i=0; i<birds.size(); ++i) birds[i].draw(window); // 绘制鸟

            // 绘制HUD数据
            drawHudItem(window, 20, 20, "SCORE", formatScore((int)(dist * SCORE_MULTIPLIER)), font, UI_PRIMARY);
            drawHudItem(window, 180, 20, "HI", formatScore(highScore), font, UI_GOLD);
            drawHudItem(window, 340, 20, "COINS", intToString(coins), font, sf::Color(255, 140, 0));

            // 绘制暂停菜单
            if (state == PLAYING && paused) {
                sf::RectangleShape mask(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
                mask.setFillColor(sf::Color(0,0,0,100)); // 半透明遮罩
                window.draw(mask);
                
                drawCard(window, WINDOW_WIDTH/2 - 150, WINDOW_HEIGHT/2 - 120, 300, 260); // 菜单背景卡片
                
                sf::Text pt; pt.setFont(font); pt.setString("PAUSED"); pt.setCharacterSize(36); 
                pt.setFillColor(UI_TEXT_DARK); pt.setStyle(sf::Text::Bold);
                drawCenteredText(window, pt, WINDOW_WIDTH/2, WINDOW_HEIGHT/2 - 80);

                // 绘制暂停按钮
                for (int i = 0; i < 3; ++i) {
                    float bx = WINDOW_WIDTH/2 - 110; float by = 160 + i * 50;
                    bool hover = (worldPos.x > bx && worldPos.x < bx+220 && worldPos.y > by && worldPos.y < by+40);
                    sf::Color hoverColor = (i==1) ? UI_SUCCESS : UI_PRIMARY; // 保存按钮高亮为绿色
                    drawButton(window, font, pauseMenu[i], bx, by, 220, 40, hover, hoverColor);
                }

                // 显示保存成功提示
                if (savedMsg) {
                    if (msgClk.getElapsedTime().asSeconds() < 2.0f) {
                        sf::Text st; st.setFont(font); st.setString("Progress Saved Successfully!"); 
                        st.setFillColor(UI_SUCCESS); st.setCharacterSize(18); st.setStyle(sf::Text::Bold);
                        drawCenteredText(window, st, WINDOW_WIDTH/2, WINDOW_HEIGHT/2 + 115);
                    } else savedMsg = false;
                }
            }
            
            // 绘制倒计时
            if (state == COUNTDOWN) {
                sf::RectangleShape mask(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
                mask.setFillColor(sf::Color(255, 255, 255, 128)); // 白色半透明遮罩
                window.draw(mask);

                sf::Text ct; ct.setFont(font);
                ct.setString(intToString(countdownVal)); // 显示倒计时数字
                ct.setCharacterSize(120);
                ct.setFillColor(UI_PRIMARY);
                ct.setOutlineColor(UI_TEXT_DARK);
                ct.setOutlineThickness(4);
                ct.setStyle(sf::Text::Bold);
                
                // 简单的缩放动画
                float scale = 1.0f + (1.0f - countdownTime) * 0.3f; 
                ct.setScale(scale, scale);
                
                drawCenteredText(window, ct, WINDOW_WIDTH/2, WINDOW_HEIGHT/2);
                
                sf::Text sub; sub.setFont(font); sub.setString("Resuming Game...");
                sub.setCharacterSize(24); sub.setFillColor(UI_TEXT_DARK);
                drawCenteredText(window, sub, WINDOW_WIDTH/2, WINDOW_HEIGHT/2 + 80);
            }
        }
        // 绘制游戏结束界面
        else if (state == GAME_OVER) {
            window.draw(g1); window.draw(g2); // 绘制背景
            dino.draw(window); // 绘制死亡的恐龙
            for(size_t i=0; i<cacti.size(); ++i) cacti[i].draw(window); // 绘制障碍物
            sf::RectangleShape mask(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
            mask.setFillColor(sf::Color(0,0,0,150)); // 黑色遮罩
            window.draw(mask);

            drawCard(window, WINDOW_WIDTH/2 - 160, 60, 320, 280); // 结算卡片
            
            int currentScore = (int)(dist * SCORE_MULTIPLIER);
            bool newHs = (currentScore >= highScore && currentScore > 0);
            bool newHc = (coins >= highCoins && coins > 0);

            // 显示破纪录提示
            if (newHs || newHc) {
                sf::Text newHsTxt; newHsTxt.setFont(font); 
                if (newHs && newHc) newHsTxt.setString("NEW RECORDS!");
                else if (newHs) newHsTxt.setString("NEW HIGH SCORE!");
                else newHsTxt.setString("NEW BEST COINS!");
                
                newHsTxt.setCharacterSize(20); newHsTxt.setFillColor(UI_GOLD); newHsTxt.setStyle(sf::Text::Bold);
                drawCenteredText(window, newHsTxt, WINDOW_WIDTH/2, 80);
            }

            sf::Text t; t.setFont(font); 
            t.setString("GAME OVER"); t.setCharacterSize(40); t.setFillColor(UI_ACCENT); t.setStyle(sf::Text::Bold);
            drawCenteredText(window, t, WINDOW_WIDTH/2, 120);

            t.setFillColor(UI_TEXT_DARK); t.setCharacterSize(20); t.setStyle(sf::Text::Regular);
            t.setString("Final Score"); drawCenteredText(window, t, WINDOW_WIDTH/2, 170);
            
            t.setString(intToString(currentScore)); // 显示最终分数
            t.setCharacterSize(50); t.setStyle(sf::Text::Bold); t.setFillColor(UI_PRIMARY);
            drawCenteredText(window, t, WINDOW_WIDTH/2, 210);

            t.setCharacterSize(18); t.setStyle(sf::Text::Bold); t.setFillColor(UI_TEXT_DARK);
            t.setString("[R] RESTART      [ESC] MENU"); // 操作提示
            drawCenteredText(window, t, WINDOW_WIDTH/2, 290);
        }

        window.display(); // 刷新显示窗口
    }
    return 0; // 程序结束
}
