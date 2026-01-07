#include <SFML/Graphics.hpp> 
#include <SFML/Audio.hpp>    
#include <iostream>          
#include <fstream>           
#include <vector>            
#include <string>            
#include <sstream>           
#include <iomanip>           
#include <cstdlib>           
#include <ctime>             
#include <cmath>             

// ==========================================
// 全局常量定义
// ==========================================
const int WINDOW_WIDTH = 800;           
const int WINDOW_HEIGHT = 400;          
const float GRAVITY = 0.8f;             
const float JUMP_FORCE = -17.8f;        
const float GROUND_Y = 250.0f;          
const float SCORE_MULTIPLIER = 0.3f;    
const float SPEED_MULTIPLIER = 1.4f;    
const float MAX_SPEED = 16.0f;          
const float MIN_BIRD_SPAWN_DISTANCE = 300.0f; 

// ==========================================
// UI 配色方案
// ==========================================
const sf::Color UI_BG(255, 255, 255);         
const sf::Color UI_PRIMARY(33, 150, 243);     
const sf::Color UI_ACCENT(255, 82, 82);       
const sf::Color UI_TEXT_DARK(45, 52, 54);     
const sf::Color UI_TEXT_LIGHT(255, 255, 255); 
const sf::Color UI_SHADOW(0, 0, 0, 30);       
const sf::Color UI_CARD_BG(255, 255, 255);    
const sf::Color UI_GOLD(255, 179, 0);         
const sf::Color UI_SUCCESS(76, 175, 80);      

// ==========================================
// 游戏状态枚举
// ==========================================
enum GameState { 
    MENU,       
    PLAYING,    
    INTRO,      
    ABOUT,      
    GAME_OVER,  
    COUNTDOWN   
};

// ==========================================
// 常用工具函数
// ==========================================

std::string intToString(int value) {
    std::ostringstream oss; 
    oss << value;           
    return oss.str();       
}

std::string formatScore(int value) {
    std::ostringstream oss; 
    oss << std::setw(5) << std::setfill('0') << value; 
    return oss.str();       
}

bool isJumpKey(sf::Keyboard::Key key) {
    return (key == sf::Keyboard::Space || key == sf::Keyboard::Up || key == sf::Keyboard::W);
}

// 绘制居中文本
void drawCenteredText(sf::RenderWindow& window, sf::Text& text, float x, float y) {
    sf::FloatRect bounds = text.getLocalBounds(); 
    text.setOrigin(bounds.width / 2.0f, bounds.height / 2.0f); 
    text.setPosition(x, y); 
    window.draw(text);      
}

// ==========================================
// 数据读写函数
// ==========================================

void loadHighData(int& hs, int& hc) {
    std::ifstream in("highscore.dat"); 
    if (in.is_open()) { 
        in >> hs >> hc; 
        in.close();     
    } else {            
        hs = 0;         
        hc = 0;         
    }
}

void saveHighData(int hs, int hc) {
    std::ofstream out("highscore.dat"); 
    if (out.is_open()) { 
        out << hs << " " << hc; 
        out.close();     
    }
}

// ==========================================
// UI 绘制函数
// ==========================================

void drawCard(sf::RenderWindow& window, float x, float y, float w, float h) {
    sf::RectangleShape shadow(sf::Vector2f(w, h)); 
    shadow.setPosition(x + 5, y + 5); 
    shadow.setFillColor(UI_SHADOW);   
    window.draw(shadow);              
    
    sf::RectangleShape card(sf::Vector2f(w, h)); 
    card.setPosition(x, y);           
    card.setFillColor(UI_CARD_BG);    
    card.setOutlineThickness(1);      
    card.setOutlineColor(sf::Color(200, 200, 200)); 
    window.draw(card);                
}

void drawButton(sf::RenderWindow& window, sf::Font& font, const std::string& label, float x, float y, float w, float h, bool hover, sf::Color overrideColor = UI_PRIMARY) {
    float offset = hover ? 2.0f : 0.0f; 
    sf::RectangleShape btn(sf::Vector2f(w, h)); 
    btn.setPosition(x - offset, y - offset); 
    
    sf::Color btnColor = hover ? overrideColor : UI_CARD_BG; 
    sf::Color outlines = hover ? overrideColor : UI_TEXT_DARK; 
    
    btn.setFillColor(btnColor);       
    btn.setOutlineThickness(2);       
    btn.setOutlineColor(outlines);    
    
    if (!hover) { 
        sf::RectangleShape shadow(sf::Vector2f(w, h));
        shadow.setPosition(x + 4, y + 4);
        shadow.setFillColor(UI_TEXT_DARK);
        window.draw(shadow);
    }
    window.draw(btn); 

    sf::Text t; 
    t.setFont(font); t.setString(label); t.setCharacterSize(20);
    t.setFillColor(hover ? UI_TEXT_LIGHT : UI_TEXT_DARK); 
    drawCenteredText(window, t, x + w/2 - offset, y + h/2 - offset - 4); 
}

void drawHudItem(sf::RenderWindow& window, float x, float y, const std::string& label, const std::string& val, sf::Font& font, sf::Color color) {
    sf::RectangleShape cap(sf::Vector2f(140, 32)); 
    cap.setPosition(x, y);
    cap.setFillColor(sf::Color(255, 255, 255, 220)); 
    cap.setOutlineThickness(2);
    cap.setOutlineColor(UI_TEXT_DARK);
    window.draw(cap);

    sf::Text tL; tL.setFont(font); tL.setString(label); tL.setCharacterSize(14); tL.setFillColor(color);
    tL.setPosition(x + 10, y + 6); 
    window.draw(tL);

    sf::Text tV; tV.setFont(font); tV.setString(val); tV.setCharacterSize(20); tV.setFillColor(UI_TEXT_DARK);
    tV.setPosition(x + 130 - tV.getLocalBounds().width, y + 3); 
    window.draw(tV);
}

// ==========================================
// 游戏实体类定义
// ==========================================

class Dino {
public:
    sf::Sprite sprite;      
    sf::Vector2f velocity;  
    bool onGround;          
    sf::Clock animationClock; 
    sf::Texture run1Tex, run2Tex, jumpTex; 
    bool showRun1;          
    float startY;           

    Dino(const sf::Texture& r1, const sf::Texture& r2, const sf::Texture& j) : run1Tex(r1), run2Tex(r2), jumpTex(j), onGround(true), showRun1(true) {
        sprite.setTexture(run1Tex); 
        startY = (GROUND_Y + 30.0f) - static_cast<float>(run1Tex.getSize().y) + 12.0f;
        sprite.setPosition(50, startY); 
        velocity.y = 0; 
    }

    void jump() { 
        if (onGround) { 
            velocity.y = JUMP_FORCE; 
            onGround = false; 
            sprite.setTexture(jumpTex); 
        } 
    }

    void fallFaster() { 
        if (!onGround) { 
            if (velocity.y < 0) velocity.y = 0; 
            velocity.y += 5.0f; 
        } 
    }

    void update(float dt) {
        if (!onGround) { 
            velocity.y += GRAVITY; 
            sprite.move(0, velocity.y); 
            if (sprite.getPosition().y >= startY) { 
                sprite.setPosition(50, startY); 
                velocity.y = 0; 
                onGround = true; 
                showRun1 = true; 
                sprite.setTexture(run1Tex); 
            }
        }
        if (onGround && animationClock.getElapsedTime().asMilliseconds() > 150) { 
            showRun1 = !showRun1; 
            sprite.setTexture(showRun1 ? run1Tex : run2Tex); 
            animationClock.restart(); 
        }
    }

    sf::FloatRect getBounds() const { 
        sf::FloatRect b = sprite.getGlobalBounds(); 
        return sf::FloatRect(b.left+8, b.top+8, b.width-16, b.height-16); 
    }

    void draw(sf::RenderWindow& w) { w.draw(sprite); }
};

class Cactus {
public:
    sf::Sprite sprite;    
    sf::Vector2f position;
    int type;             

    bool init(float x, int t, const sf::Texture& tex) {
        type = t; 
        sprite.setTexture(tex); 
        position.x = x;
        position.y = (GROUND_Y + 30.0f) - static_cast<float>(tex.getSize().y) + 15.0f;
        sprite.setPosition(position); 
        return true;
    }

    void update(float s) { 
        position.x -= s; 
        sprite.setPosition(position); 
    }

    bool checkCollision(const sf::FloatRect& o) const { 
        sf::FloatRect b = sprite.getGlobalBounds(); 
        return sf::FloatRect(b.left+6, b.top+6, b.width-12, b.height-12).intersects(o); 
    }

    void draw(sf::RenderWindow& w) { w.draw(sprite); }
};

class Coin {
public:
    sf::Sprite sprite;    
    sf::Vector2f position;
    bool collected;       

    bool init(float x, float y, const sf::Texture& tex) { 
        sprite.setTexture(tex); 
        position = sf::Vector2f(x, y); 
        sprite.setPosition(position); 
        collected = false; 
        return true; 
    }

    void update(float s) { 
        position.x -= s; 
        sprite.setPosition(position); 
    }

    bool checkCollision(const sf::FloatRect& o) const { 
        if(collected) return false; 
        sf::FloatRect b = sprite.getGlobalBounds(); 
        return sf::FloatRect(b.left-5, b.top-5, b.width+10, b.height+10).intersects(o); 
    }

    void draw(sf::RenderWindow& w) { if(!collected) w.draw(sprite); }
};

class Bird {
public:
    sf::Sprite sprite;    
    sf::Vector2f position;
    sf::Clock wingClock;  
    sf::Texture wingUpTex, wingDownTex; 
    bool showWingUp;      

    Bird(const sf::Texture& wu, const sf::Texture& wd) : wingUpTex(wu), wingDownTex(wd), showWingUp(true) {}

    void init(float x, float y) { 
        position = sf::Vector2f(x, y); 
        sprite.setTexture(showWingUp ? wingUpTex : wingDownTex); 
        sprite.setPosition(position); 
    }

    void update(float s, float dt) {
        position.x -= s; 
        sprite.setPosition(position);
        if (wingClock.getElapsedTime().asSeconds() > 0.25f) { 
            showWingUp = !showWingUp; 
            sprite.setTexture(showWingUp ? wingUpTex : wingDownTex); 
            wingClock.restart(); 
        }
    }

    bool checkCollision(const sf::FloatRect& o) const { 
        sf::FloatRect b = sprite.getGlobalBounds(); 
        return sf::FloatRect(b.left+5, b.top+5, b.width-10, b.height-10).intersects(o); 
    }

    bool isOffScreen() const { return position.x + sprite.getGlobalBounds().width < 0; }
    
    void draw(sf::RenderWindow& w) { w.draw(sprite); }
};

// ==========================================
// 资源加载和全局变量
// ==========================================
sf::Texture tDino1, tDino2, tJump, tCacL, tCacS1, tCacS2, tCoin, tTrack, tBirdU, tBirdD;
sf::Font font; 
sf::SoundBuffer shutBuf; 
sf::Sound shutSound; 
sf::Music bgm;

bool loadAssets() {
    bool ok = true;
    ok &= tDino1.loadFromFile("DinoRun1.png"); ok &= tDino2.loadFromFile("DinoRun2.png"); ok &= tJump.loadFromFile("DinoJump.png");
    ok &= tCacL.loadFromFile("LargeCactus1.png"); ok &= tCacS1.loadFromFile("SmallCactus1.png"); ok &= tCacS2.loadFromFile("SmallCactus2.png");
    ok &= tCoin.loadFromFile("Coin.png"); ok &= tTrack.loadFromFile("Track.png");
    ok &= tBirdU.loadFromFile("BirdWingUp.png"); ok &= tBirdD.loadFromFile("BirdWingDown.png");
    ok &= font.loadFromFile("Roboto-Regular.ttf"); ok &= shutBuf.loadFromFile("shutdown.wav");
    bgm.openFromFile("bgm.ogg"); bgm.setLoop(true); shutSound.setBuffer(shutBuf);
    return ok;
}

// ==========================================
// 存档系统
// ==========================================

void saveGame(float d, int c, const Dino& dn, const std::vector<Cactus>& ca, const std::vector<Coin>& co, const std::vector<Bird>& bi) {
    std::ofstream out("savegame.txt");
    if (out.is_open()) {
        out << d << " " << c << "\n"; 
        out << dn.sprite.getPosition().y << " " << dn.velocity.y << " " << dn.onGround << "\n"; 
        out << ca.size() << "\n"; for(size_t i=0; i<ca.size(); ++i) out << ca[i].position.x << " " << ca[i].type << "\n";
        int vc=0; for(size_t i=0; i<co.size(); ++i) if(!co[i].collected) vc++; out << vc << "\n";
        for(size_t i=0; i<co.size(); ++i) if(!co[i].collected) out << co[i].position.x << " " << co[i].position.y << "\n";
        out << bi.size() << "\n"; for(size_t i=0; i<bi.size(); ++i) out << bi[i].position.x << " " << bi[i].position.y << "\n";
        out.close();
    }
}

bool loadGame(float& d, int& c, Dino& dn, std::vector<Cactus>& ca, std::vector<Coin>& co, std::vector<Bird>& bi) {
    std::ifstream in("savegame.txt"); if (!in.is_open()) return false;
    ca.clear(); co.clear(); bi.clear(); 
    in >> d >> c; 
    float dy, dvy; bool dog; in >> dy >> dvy >> dog; 
    dn.sprite.setPosition(50, dy); dn.velocity.y = dvy; dn.onGround = dog;
    int cnt; in >> cnt; sf::Texture* ts[3]; ts[0]=&tCacL; ts[1]=&tCacS1; ts[2]=&tCacS2;
    for(int i=0; i<cnt; ++i) { float x; int t; in >> x >> t; Cactus o; o.init(x, t, *ts[t%3]); ca.push_back(o); }
    in >> cnt; for(int i=0; i<cnt; ++i) { float x,y; in >> x >> y; Coin o; o.init(x, y, tCoin); co.push_back(o); }
    in >> cnt; for(int i=0; i<cnt; ++i) { float x,y; in >> x >> y; Bird o(tBirdU, tBirdD); o.init(x, y); bi.push_back(o); }
    in.close(); return true;
}

void generateShutdownWav() {
    const int sr = 44100; const double dur = 0.5; std::vector<short> s(sr * dur);
    for (size_t i = 0; i < s.size(); ++i) s[i] = static_cast<short>(10000 * sin(2 * 3.14159 * 440 * i / sr)); 
    std::ofstream f("shutdown.wav", std::ios::binary);
    int sz = s.size() * 2; int h[11] = {0x46464952, 36+sz, 0x45564157, 0x20746d66, 16, 0x10001, 44100, 44100*2, 0x100002, 0x61746164, sz}; 
    f.write((char*)h, 44); f.write((char*)s.data(), sz);
}

// ==========================================
// 主函数
// ==========================================
int main() {
    std::srand((unsigned int)std::time(0)); 
    
    std::vector<std::string> team;
    team.push_back("Game Created By");
    team.push_back("Yao Wang");
    team.push_back("Solo Developer");

    { std::ifstream c("shutdown.wav"); if(!c.is_open()) generateShutdownWav(); }
    if (!loadAssets()) { std::cerr << "Asset Error\n"; return -1; }

    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Little Dino - Final");
    window.setFramerateLimit(60); 

    sf::View gameView(sf::FloatRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT));
    window.setView(gameView);

    int highScore = 0;
    int highCoins = 0;
    loadHighData(highScore, highCoins);

    GameState state = MENU; 
    float dist = 0.0f; int coins = 0; float spd = 0.0f; 
    float spawnTimer=0, coinSpawnTimer=0, birdTimer=0;  
    bool paused = false; bool savedMsg = false; sf::Clock msgClk; 
    
    int countdownVal = 3;
    float countdownTime = 0.0f;

    Dino dino(tDino1, tDino2, tJump);
    std::vector<Cactus> cacti; std::vector<Coin> coinList; std::vector<Bird> birds;
    sf::Sprite g1(tTrack), g2(tTrack); 
    g1.setPosition(0, GROUND_Y + 30); g2.setPosition(tTrack.getSize().x, GROUND_Y + 30);

    std::vector<std::string> menu;
    menu.push_back("Start Adventure");
    menu.push_back("Load Save");
    menu.push_back("How to Play");
    menu.push_back("Credits");
    menu.push_back("Exit");

    std::vector<std::string> pauseMenu;
    pauseMenu.push_back("Resume");
    pauseMenu.push_back("Save Game");
    pauseMenu.push_back("Main Menu");

    while (window.isOpen()) {
        float dt = 1.0f / 60.0f; // 固定帧时间，便于统一物理更新
        sf::Event e;
        
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) window.close(); 
            
            if (e.type == sf::Event::Resized) {
                gameView.reset(sf::FloatRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT)); // 强制保持设计尺寸
                window.setView(gameView);
            }

            sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
            sf::Vector2f worldPos = window.mapPixelToCoords(pixelPos);

            // --- 菜单逻辑优化 ---
            if (state == MENU) {
                if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left) {
                    for (int i = 0; i < 5; ++i) {
                        // 计算按钮的矩形起始位置和宽高
                        float bx = WINDOW_WIDTH/2 - 110; 
                        float by = 140 + i * 46; // 略微下移按钮保持间距
                        if (worldPos.x > bx && worldPos.x < bx+220 && worldPos.y > by && worldPos.y < by+40) {
                            if (i==0) { 
                                state=PLAYING; dist=0; coins=0; dino=Dino(tDino1, tDino2, tJump); 
                                cacti.clear(); coinList.clear(); birds.clear(); 
                                spd=4.0f*SPEED_MULTIPLIER; bgm.play(); 
                            }
                            else if (i==1) { 
                                if(loadGame(dist, coins, dino, cacti, coinList, birds)) { 
                                    state = COUNTDOWN; // 读档后通过倒计时回到游戏，避免突兀
                                    countdownVal = 3; 
                                    countdownTime = 0.0f; 
                                    paused = false; 
                                    spd = 4.0f*SPEED_MULTIPLIER+(dist/100.0f)*0.8f; // 依行进距离恢复速度
                                    if(spd>MAX_SPEED) spd=MAX_SPEED; 
                                    bgm.play(); 
                                } 
                            }
                            else if (i==2) state=INTRO; 
                            else if (i==3) state=ABOUT; 
                            else if (i==4) window.close(); 
                        }
                    }
                }
            }
            // --- 游戏逻辑 ---
            else if (state == PLAYING) {
                if (e.type == sf::Event::KeyPressed) {
                    if (e.key.code == sf::Keyboard::P) {
                        if (!paused) {
                            paused = true; 
                        } else {
                            paused = false;
                            state = COUNTDOWN; // 继续前加 3 秒倒计时
                            countdownVal = 3;
                            countdownTime = 0.0f;
                        }
                    }
                    if (e.key.code == sf::Keyboard::Escape) { state = MENU; bgm.stop(); } 
                    if (paused && e.key.code == sf::Keyboard::K) { 
                        saveGame(dist, coins, dino, cacti, coinList, birds); // 暂停时按 K 快速存档
                        savedMsg = true; msgClk.restart(); 
                    }
                    if (!paused && isJumpKey(e.key.code)) dino.jump();
                }
                
                if (paused && e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left) {
                    for (int i = 0; i < 3; ++i) {
                        // 计算暂停菜单按钮位置
                        float bx = WINDOW_WIDTH/2 - 110; 
                        float by = 155 + i * 50;
                        if (worldPos.x > bx && worldPos.x < bx+220 && worldPos.y > by && worldPos.y < by+40) {
                            if (i == 0) { 
                                paused = false; 
                                state = COUNTDOWN; 
                                countdownVal = 3; 
                                countdownTime = 0.0f; 
                            } 
                            else if (i == 1) { 
                                saveGame(dist, coins, dino, cacti, coinList, birds); 
                                savedMsg = true; msgClk.restart(); 
                            }
                            else if (i == 2) { state = MENU; bgm.stop(); } 
                        }
                    }
                }
            }
            else if (state == COUNTDOWN) {
                 if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Escape) {
                     state = MENU; bgm.stop();
                 }
            }
            else if (state == INTRO || state == ABOUT) {
                if (e.type == sf::Event::KeyPressed && (e.key.code == sf::Keyboard::Escape || e.key.code == sf::Keyboard::Return)) state = MENU;
            }
            else if (state == GAME_OVER) {
                if (e.type == sf::Event::KeyPressed) {
                    if (e.key.code == sf::Keyboard::R) { 
                        state=PLAYING; dist=0; coins=0; dino=Dino(tDino1, tDino2, tJump); 
                        cacti.clear(); coinList.clear(); birds.clear(); 
                        spd=4.0f*SPEED_MULTIPLIER; bgm.play(); 
                    }
                    else if (e.key.code == sf::Keyboard::Escape) state = MENU; 
                }
            }
        }

        sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
        sf::Vector2f worldPos = window.mapPixelToCoords(pixelPos);

        // --- 更新时间 ---

        if (state == COUNTDOWN) {
            countdownTime += dt;
            if (countdownTime >= 1.0f) { 
                countdownVal--;
                countdownTime = 0.0f;
            }
            if (countdownVal <= 0) { 
                state = PLAYING; 
            }
        }
        else if (state == PLAYING && !paused) {
            dino.update(dt); 
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) dino.fallFaster(); // 长按下加速下落

            dist += spd * dt;
            float target = 4.0f * SPEED_MULTIPLIER + (dist / 100.0f) * 0.8f; // 距离越远速度越快
            if (target > MAX_SPEED) target = MAX_SPEED; // 限制最大速度
            spd = target;
            
            spawnTimer += dt;
            if (spawnTimer > 1.5f + (rand()%15)/10.0f) { // 随机生成仙人掌，间隔 1.5~3.0s
                Cactus c; sf::Texture* ts[3]; ts[0]=&tCacL; ts[1]=&tCacS1; ts[2]=&tCacS2; int t = rand()%3;
                if (c.init(WINDOW_WIDTH + 20, t, *ts[t])) cacti.push_back(c); 
                spawnTimer = 0;
            }
            
            coinSpawnTimer += dt;
            if (coinSpawnTimer > 3.0f + (rand() % 20) / 10.0f) { // 约 3~5 秒尝试刷一枚硬币
                if (rand() % 100 < 50) { // 50% 概率生成，避免过密
                    Coin c; 
                    bool safe = true; 
                    float cx = WINDOW_WIDTH + 100 + rand() % 100; // 生成在屏外 100~200 像素
                    
                    for(size_t i = 0; i < cacti.size(); ++i) if(std::abs(cacti[i].position.x - cx) < 100) safe = false; // 与仙人掌保持距离
                    for(size_t i = 0; i < birds.size(); ++i) if(std::abs(birds[i].position.x - cx) < 100) safe = false; // 与飞鸟保持距离

                    if(safe) {
                        if(c.init(cx, 90.0f, tCoin)) coinList.push_back(c); 
                    }
                } 
                coinSpawnTimer = 0;
            }

            if (dist > MIN_BIRD_SPAWN_DISTANCE) { // 距离超过一定值后才刷飞鸟
                birdTimer += dt; 
                if (birdTimer > 4.0f) { // 每 4 秒尝试刷一只
                    float birdSpawnX = WINDOW_WIDTH + 50;
                    bool safe = true;
                    for(size_t i = 0; i < coinList.size(); ++i) if(std::abs(coinList[i].position.x - birdSpawnX) < 100) safe = false; // 与硬币保持间隔
                    for(size_t i = 0; i < cacti.size(); ++i) if(std::abs(cacti[i].position.x - birdSpawnX) < 80) safe = false;  // 与仙人掌保持间隔

                    if (safe) {
                        Bird b(tBirdU, tBirdD); b.init(birdSpawnX, 130.0f); birds.push_back(b); birdTimer = 0; 
                    } else { birdTimer = 3.5f; } 
                }
            }

            for(size_t i=0; i<cacti.size(); ++i) cacti[i].update(spd);
            for(size_t i=0; i<coinList.size(); ++i) coinList[i].update(spd);
            for(size_t i=0; i<birds.size(); ++i) birds[i].update(spd, dt);

            sf::FloatRect pr = dino.getBounds();
            bool collision = false;
            for(size_t i=0; i<cacti.size(); ++i) if (cacti[i].checkCollision(pr)) collision = true; // 碰到仙人掌
            for(size_t i=0; i<birds.size(); ++i) if (birds[i].checkCollision(pr)) collision = true; // 碰到飞鸟
            
            if (collision) {
                state = GAME_OVER; bgm.stop(); shutSound.play();
                int currentScore = (int)(dist * SCORE_MULTIPLIER);
                bool updated = false;
                if (currentScore > highScore) { highScore = currentScore; updated = true; }
                if (coins > highCoins) { highCoins = coins; updated = true; }
                if (updated) saveHighData(highScore, highCoins);
            }

            for(size_t i=0; i<coinList.size(); ++i) if (coinList[i].checkCollision(pr)) { coinList[i].collected = true; coins++; } // 吃硬币加计数

            for(int i=cacti.size()-1; i>=0; --i) if(cacti[i].position.x < -100) cacti.erase(cacti.begin()+i); // 清理离屏仙人掌
            for(int i=coinList.size()-1; i>=0; --i) if(coinList[i].collected || coinList[i].position.x < -50) coinList.erase(coinList.begin()+i); // 清理吃掉/离屏硬币
            for(int i=birds.size()-1; i>=0; --i) if(birds[i].isOffScreen()) birds.erase(birds.begin()+i); // 清理离屏飞鸟

            float tw = tTrack.getSize().x; g1.move(-spd, 0); g2.move(-spd, 0);
            if(g1.getPosition().x+tw <= 0) g1.setPosition(g2.getPosition().x+tw, GROUND_Y+30); // 双贴图循环滚动地面
            if(g2.getPosition().x+tw <= 0) g2.setPosition(g1.getPosition().x+tw, GROUND_Y+30);
        }

        // --- 渲染逻辑 ---

        window.clear(UI_BG); 

        // 绘制主菜单
        if (state == MENU) {
            sf::RectangleShape stripe(sf::Vector2f(WINDOW_WIDTH, 100)); 
            stripe.setFillColor(sf::Color(230, 230, 240));
            window.draw(stripe);

            sf::Text title; 
            title.setFont(font); title.setString("LITTLE DINO"); title.setCharacterSize(60);
            title.setFillColor(UI_TEXT_DARK); title.setStyle(sf::Text::Bold);
            
            // 给标题添加轻微阴影
            sf::Text ts = title; ts.setFillColor(sf::Color(200, 200, 200)); ts.setPosition(WINDOW_WIDTH/2 + 4, 54);
            sf::FloatRect tb = title.getLocalBounds(); title.setOrigin(tb.width/2, tb.height/2); ts.setOrigin(tb.width/2, tb.height/2);
            window.draw(ts);
            title.setPosition(WINDOW_WIDTH/2, 50); // 绘制标题
            window.draw(title);

            // 显示高分
            sf::Text hsText;
            hsText.setFont(font); 
            std::string rec = "BEST SCORE: " + formatScore(highScore) + "   BEST COINS: " + intToString(highCoins);
            hsText.setString(rec);
            hsText.setCharacterSize(18);
            hsText.setFillColor(UI_PRIMARY);
            drawCenteredText(window, hsText, WINDOW_WIDTH/2, 95); // 绘制高分

            // 绘制菜单按钮
            for (int i = 0; i < 5; ++i) {
                float bx = WINDOW_WIDTH/2 - 110; float by = 140 + i * 46; 
                bool hover = (worldPos.x > bx && worldPos.x < bx+220 && worldPos.y > by && worldPos.y < by+40);
                drawButton(window, font, menu[i], bx, by, 220, 40, hover);
            }
        }
        // 绘制说明页面（轻度美化）
        else if (state == INTRO) {
            drawCard(window, 80, 40, WINDOW_WIDTH-160, WINDOW_HEIGHT-80); // 调整卡片尺寸

            sf::Text t; t.setFont(font); t.setFillColor(UI_PRIMARY);
            
            // 标题
            t.setCharacterSize(32); t.setStyle(sf::Text::Bold); t.setString("HOW TO PLAY");
            drawCenteredText(window, t, WINDOW_WIDTH/2, 80);
            
            // 正文（含缩进，优化排版）
            t.setCharacterSize(18); t.setStyle(sf::Text::Regular); t.setFillColor(UI_TEXT_DARK);
            std::string content = 
                "OBJECTIVE:\n"
                "  Run as far as possible and collect coins!\n\n"
                "CONTROLS:\n"
                "  [Space / Up]     Jump\n"
                "  [Down]             Drop Fast\n"
                "  [P]                   Pause Menu\n"
                "  [ESC]               Back to Menu";
            t.setString(content);
            t.setPosition(160, 120); // 设定正文起始位置
            t.setOrigin(0,0); // 确保左上角对齐
            window.draw(t);

            // 底部提示
            t.setString("[ ENTER to Return ]"); t.setCharacterSize(16); t.setFillColor(UI_ACCENT); t.setStyle(sf::Text::Bold);
            // 将提示文字居中
            sf::FloatRect b = t.getLocalBounds(); t.setOrigin(b.width/2, b.height/2);
            t.setPosition(WINDOW_WIDTH/2, 320);
            window.draw(t);
        }
        // 绘制关于界面（轻度美化）
        else if (state == ABOUT) {
            drawCard(window, 150, 50, 500, 300);
            sf::Text t; t.setFont(font); t.setFillColor(UI_PRIMARY);
            t.setCharacterSize(32); t.setStyle(sf::Text::Bold); t.setString("CREDITS");
            drawCenteredText(window, t, WINDOW_WIDTH/2, 90);

            // 名单列表（增加层次）
            t.setCharacterSize(22); t.setStyle(sf::Text::Regular); t.setFillColor(UI_TEXT_DARK);
            float startY = 150;
            
            t.setString("Game Created By");
            drawCenteredText(window, t, WINDOW_WIDTH/2, startY);
            
            t.setStyle(sf::Text::Bold); t.setFillColor(UI_TEXT_DARK); t.setCharacterSize(26);
            t.setString("Yao Wang");
            drawCenteredText(window, t, WINDOW_WIDTH/2, startY + 40);

            t.setStyle(sf::Text::Regular); t.setCharacterSize(18); t.setFillColor(sf::Color(100,100,100));
            t.setString("Solo Developer");
            drawCenteredText(window, t, WINDOW_WIDTH/2, startY + 80);

            t.setString("[ ENTER to Return ]"); t.setCharacterSize(16); t.setFillColor(UI_ACCENT); t.setStyle(sf::Text::Bold);
            drawCenteredText(window, t, WINDOW_WIDTH/2, 310);
        }
        else if (state == PLAYING || state == COUNTDOWN) {
            window.draw(g1); window.draw(g2); 
            dino.draw(window); 
            for(size_t i=0; i<cacti.size(); ++i) cacti[i].draw(window); 
            for(size_t i=0; i<coinList.size(); ++i) coinList[i].draw(window); 
            for(size_t i=0; i<birds.size(); ++i) birds[i].draw(window); 

            drawHudItem(window, 20, 20, "SCORE", formatScore((int)(dist * SCORE_MULTIPLIER)), font, UI_PRIMARY);
            drawHudItem(window, 180, 20, "HI", formatScore(highScore), font, UI_GOLD);
            drawHudItem(window, 340, 20, "COINS", intToString(coins), font, sf::Color(255, 140, 0));

            if (state == PLAYING && paused) {
                sf::RectangleShape mask(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
                mask.setFillColor(sf::Color(0,0,0,100)); 
                window.draw(mask);
                
                drawCard(window, WINDOW_WIDTH/2 - 150, WINDOW_HEIGHT/2 - 120, 300, 260); 
                
                sf::Text pt; pt.setFont(font); pt.setString("PAUSED"); pt.setCharacterSize(36); 
                pt.setFillColor(UI_TEXT_DARK); pt.setStyle(sf::Text::Bold);
                drawCenteredText(window, pt, WINDOW_WIDTH/2, WINDOW_HEIGHT/2 - 80);

                for (int i = 0; i < 3; ++i) {
                    float bx = WINDOW_WIDTH/2 - 110; float by = 155 + i * 50; 
                    bool hover = (worldPos.x > bx && worldPos.x < bx+220 && worldPos.y > by && worldPos.y < by+40);
                    sf::Color hoverColor = (i==1) ? UI_SUCCESS : UI_PRIMARY; 
                    drawButton(window, font, pauseMenu[i], bx, by, 220, 40, hover, hoverColor);
                }

                if (savedMsg) {
                        if (msgClk.getElapsedTime().asSeconds() < 2.0f) {
                            sf::Text st; st.setFont(font); st.setString("Progress Saved!"); 
                            st.setFillColor(UI_SUCCESS); st.setCharacterSize(18); st.setStyle(sf::Text::Bold);
                        drawCenteredText(window, st, WINDOW_WIDTH/2, WINDOW_HEIGHT/2 + 110); // 放在下方提示
                    } else savedMsg = false;
                }
            }
            
            if (state == COUNTDOWN) {
                sf::RectangleShape mask(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
                mask.setFillColor(sf::Color(255, 255, 255, 128)); 
                window.draw(mask);

                sf::Text ct; ct.setFont(font);
                ct.setString(intToString(countdownVal)); 
                ct.setCharacterSize(120);
                ct.setFillColor(UI_PRIMARY);
                ct.setOutlineColor(UI_TEXT_DARK);
                ct.setOutlineThickness(4);
                ct.setStyle(sf::Text::Bold);
                
                float scale = 1.0f + (1.0f - countdownTime) * 0.3f; 
                ct.setScale(scale, scale);
                
                drawCenteredText(window, ct, WINDOW_WIDTH/2, WINDOW_HEIGHT/2);
                
                sf::Text sub; sub.setFont(font); sub.setString("Resuming Game...");
                sub.setCharacterSize(24); sub.setFillColor(UI_TEXT_DARK);
                drawCenteredText(window, sub, WINDOW_WIDTH/2, WINDOW_HEIGHT/2 + 80);
            }
        }
        // 绘制游戏结束界面（轻度美化）
        else if (state == GAME_OVER) {
            window.draw(g1); window.draw(g2); 
            dino.draw(window); 
            for(size_t i=0; i<cacti.size(); ++i) cacti[i].draw(window); 
            sf::RectangleShape mask(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
            mask.setFillColor(sf::Color(0,0,0,150)); 
            window.draw(mask);

            drawCard(window, WINDOW_WIDTH/2 - 160, 50, 320, 280); // 绘制卡片
            
            int currentScore = (int)(dist * SCORE_MULTIPLIER);
            bool newHs = (currentScore >= highScore && currentScore > 0);
            bool newHc = (coins >= highCoins && coins > 0);

            // 显示破纪录提示（避免重复提示）
            if (newHs || newHc) {
                sf::Text newHsTxt; newHsTxt.setFont(font); 
                if (newHs && newHc) newHsTxt.setString("NEW RECORDS!");
                else if (newHs) newHsTxt.setString("NEW HIGH SCORE!");
                else newHsTxt.setString("NEW BEST COINS!");
                
                newHsTxt.setCharacterSize(18); newHsTxt.setFillColor(UI_GOLD); newHsTxt.setStyle(sf::Text::Bold);
                drawCenteredText(window, newHsTxt, WINDOW_WIDTH/2, 70);
            }

            sf::Text t; t.setFont(font); 
            t.setString("GAME OVER"); t.setCharacterSize(42); t.setFillColor(UI_ACCENT); t.setStyle(sf::Text::Bold);
            drawCenteredText(window, t, WINDOW_WIDTH/2, 110);

            t.setFillColor(UI_TEXT_DARK); t.setCharacterSize(20); t.setStyle(sf::Text::Regular);
            t.setString("Final Score"); drawCenteredText(window, t, WINDOW_WIDTH/2, 160);
            
            // 放大得分显示
            t.setString(intToString(currentScore)); 
            t.setCharacterSize(60); t.setStyle(sf::Text::Bold); t.setFillColor(UI_PRIMARY);
            drawCenteredText(window, t, WINDOW_WIDTH/2, 205);

            t.setCharacterSize(16); t.setStyle(sf::Text::Bold); t.setFillColor(UI_TEXT_DARK);
            t.setString("[R] RESTART      [ESC] MENU"); 
            drawCenteredText(window, t, WINDOW_WIDTH/2, 280);
        }

        window.display(); 
    }
    return 0; 
}
