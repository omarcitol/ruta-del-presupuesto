#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>
#include <vector>

namespace {

constexpr int ScreenWidth = 1000;
constexpr int ScreenHeight = 600;
constexpr float GroundY = 500.0f;
constexpr float LevelWidth = 2000.0f;
constexpr float WorldWidth = LevelWidth * 3.0f;
constexpr float PlayerWidth = 34.0f;
constexpr float PlayerHeight = 48.0f;
constexpr int LevelCount = 3;

const Color DeepBlue = {26, 42, 75, 255};
const Color NightBlue = {24, 37, 64, 255};
const Color Red = {220, 65, 66, 255};
const Color BrightRed = {242, 84, 74, 255};
const Color Blue = {50, 117, 190, 255};
const Color SkyBlue = {122, 202, 244, 255};
const Color LeafGreen = {67, 159, 91, 255};
const Color Gold = {255, 197, 66, 255};
const Color Paper = {255, 249, 226, 255};
const Color Ink = {35, 44, 58, 255};
const Color Muted = {98, 108, 121, 255};
const Color White = {255, 255, 250, 255};

#if defined(PLATFORM_WEB)
Font GameFont = {};
#endif

void DrawText(const char *text, int x, int y, int fontSize, Color color)
{
#if defined(PLATFORM_WEB)
    if (GameFont.texture.id != 0)
    {
        DrawTextEx(GameFont, text, {static_cast<float>(x), static_cast<float>(y)},
                   static_cast<float>(fontSize), 0.0f, color);
        return;
    }
#endif
    ::DrawText(text, x, y, fontSize, color);
}

struct Platform {
    Rectangle bounds;
};

struct GroundGap {
    float start;
    float end;
};

struct Ladder {
    Rectangle bounds;
};

struct Coin {
    Vector2 position;
    bool collected;
};

struct PowerUp {
    Vector2 position;
    bool collected;
    bool grantsShield;
};

struct Enemy {
    Rectangle bounds;
    float patrolStart;
    float patrolEnd;
    float speed;
    bool alive;
};

struct Projectile {
    Rectangle bounds;
    float speed;
    bool alive;
};

struct Decision {
    float x;
    const char *title;
    const char *question;
    const char *options[2];
    const char *feedback[2];
    int balanceChange[2];
    int savingsChange[2];
    bool risky[2];
};

struct Records {
    int bestScore = 0;
    int ratingCount = 0;
    int ratingTotal = 0;
    bool storageError = false;
};

struct DifficultySettings {
    const char *name;
    const char *description;
    float duration;
    int startingLives;
    float enemySpeed;
};

const DifficultySettings Difficulties[] = {
    {"TRANQUILO", "210 s / 4 vidas / deudas mas lentas", 210.0f, 4, 0.78f},
    {"AVENTURA", "180 s / 3 vidas / reto equilibrado", 180.0f, 3, 1.0f},
    {"RETO", "165 s / 2 vidas / enemigos mas rapidos", 165.0f, 2, 1.22f}
};

const char *const StageNames[LevelCount] = {
    "Barrio del ahorro", "Avenida de las cuotas", "Mercado de las metas"
};

const char *const StageLessons[LevelCount] = {
    "Prioriza necesidades y aparta algo para tu meta.",
    "Compara el costo total antes de aceptar cuotas.",
    "Prepara un fondo y protege tu meta."
};

const std::vector<Decision> Decisions = {
    {600, "1 / INGRESO", "Recibes $12.000. Como te organizas?",
     {"Apartas $2.000 para tu meta", "Gastas $3.000 en un antojo"},
     {"Buena jugada: tu meta ya tiene un inicio.", "El gusto se disfruta, pero reduce tu margen."},
     {-2000, -3000}, {2000, 0}, {false, true}},
    {1450, "2 / NECESIDAD", "Tu telefono dejo de funcionar.",
     {"Reparas lo necesario: $2.500", "Compras uno nuevo a credito: $6.000"},
     {"Resuelves la necesidad sin cargar una deuda grande.", "La cuota parece pequena, pero compromete el presupuesto."},
     {-2500, -6000}, {0, 0}, {false, true}},
    {2600, "3 / TRANSPORTE", "Debes llegar a clases toda la semana.",
     {"Planificas bus: $900", "Pides taxi cada dia: $2.400"},
     {"Planificar el traslado libera dinero para otras prioridades.", "La comodidad cuesta mas esta semana."},
     {-900, -2400}, {0, 0}, {false, true}},
    {3450, "4 / OFERTA", "Una tienda ofrece una compra en cuotas.",
     {"Comparas el costo total antes de decidir", "Aceptas sin revisar intereses: $2.000 extra"},
     {"Comparar condiciones tambien es cuidar tu dinero.", "Sin revisar intereses, el precio final puede sorprenderte."},
     {0, -2000}, {0, 0}, {false, true}},
    {4600, "5 / IMPREVISTO", "Aparece un gasto medico de $1.500.",
     {"Usas el ahorro y cubres lo que falte", "Pides prestado y pagas $300 extra"},
     {"El fondo de emergencia existe para momentos como este.", "La deuda resuelve hoy, pero aumenta el costo."},
     {0, -1800}, {-1500, 0}, {false, true}},
    {5450, "6 / META", "Quieres reunir $6.000 para estudiar.",
     {"Proteges el ahorro y revisas tu avance", "Usas $2.000 del ahorro para un gusto"},
     {"Llegaste con un plan y el ahorro queda protegido.", "La meta se aleja; puedes intentarlo de nuevo con otro plan."},
     {0, 0}, {0, -2000}, {false, true}}
};

const std::vector<Platform> Platforms = {
    {{280, 420, 145, 18}}, {{940, 390, 150, 18}}, {{1285, 345, 160, 18}},
    {{1515, 430, 48, 18}}, {{1563, 405, 48, 18}}, {{1611, 380, 48, 18}},
    {{2180, 420, 150, 18}}, {{2880, 390, 150, 18}}, {{3250, 345, 160, 18}},
    {{3490, 430, 48, 18}}, {{3538, 405, 48, 18}}, {{3586, 380, 48, 18}},
    {{4180, 420, 150, 18}}, {{4880, 390, 150, 18}}, {{5250, 345, 160, 18}},
    {{5490, 430, 48, 18}}, {{5538, 405, 48, 18}}, {{5586, 380, 48, 18}},
    {{540, 440, 95, 16}}, {{1120, 430, 100, 16}},
    {{2370, 405, 105, 16}}, {{2525, 365, 115, 16}}, {{2680, 425, 65, 16}},
    {{3035, 435, 100, 16}}, {{3710, 410, 125, 16}},
    {{4360, 410, 110, 16}}, {{4540, 370, 130, 16}},
    {{4720, 425, 65, 16}}, {{4970, 405, 110, 16}}, {{5150, 355, 140, 16}},
    {{5750, 415, 125, 16}}
};

const std::vector<GroundGap> Gaps = {
    {760, 910}, {2760, 2920}, {4760, 4930}
};

const std::vector<Ladder> Ladders = {
    {{1020, 390, 28, 110}}, {{2960, 390, 28, 110}}, {{4960, 390, 28, 110}}
};

std::vector<Coin> MakeCoins()
{
    return {
        {{350, 380}, false}, {{980, 350}, false}, {{1430, 305}, false},
        {{2220, 380}, false}, {{2920, 350}, false}, {{3390, 305}, false},
        {{4220, 380}, false}, {{4920, 350}, false}, {{5390, 305}, false},
        {{575, 400}, false}, {{1165, 390}, false},
        {{2400, 365}, false}, {{2565, 325}, false}, {{3735, 370}, false},
        {{4395, 375}, false}, {{4590, 330}, false}, {{5010, 365}, false},
        {{5200, 315}, false}, {{5795, 375}, false}
    };
}

std::vector<PowerUp> MakePowerUps()
{
    return {
        {{610, 455}, false, false},
        {{2380, 455}, false, true},
        {{4380, 455}, false, false}
    };
}

std::vector<Enemy> MakeEnemies(float difficultySpeed)
{
    return {
        {{700, GroundY - 30, 34, 30}, 640, 735, 65 * difficultySpeed, true},
        {{1400, GroundY - 30, 34, 30}, 1340, 1490, 75 * difficultySpeed, true},
        {{1740, GroundY - 30, 34, 30}, 1680, 1810, 82 * difficultySpeed, true},
        {{2240, GroundY - 30, 34, 30}, 2160, 2320, 70 * difficultySpeed, true},
        {{2630, GroundY - 30, 34, 30}, 2560, 2710, 82 * difficultySpeed, true},
        {{3400, GroundY - 30, 34, 30}, 3330, 3480, 75 * difficultySpeed, true},
        {{3800, GroundY - 30, 34, 30}, 3730, 3870, 90 * difficultySpeed, true},
        {{4260, GroundY - 30, 34, 30}, 4180, 4340, 70 * difficultySpeed, true},
        {{4670, GroundY - 30, 34, 30}, 4600, 4730, 94 * difficultySpeed, true},
        {{5430, GroundY - 30, 34, 30}, 5360, 5510, 80 * difficultySpeed, true},
        {{5800, GroundY - 30, 34, 30}, 5740, 5880, 100 * difficultySpeed, true}
    };
}

Sound MakeTone(float frequency, float duration)
{
    const unsigned int sampleRate = 22050;
    const unsigned int frameCount = static_cast<unsigned int>(sampleRate * duration);
    std::vector<short> samples(frameCount);
    for (unsigned int i = 0; i < frameCount; ++i)
    {
        const float envelope = 1.0f - static_cast<float>(i) / frameCount;
        const float wave = std::sin(2.0f * PI * frequency * i / sampleRate);
        samples[i] = static_cast<short>(wave * envelope * 8500.0f);
    }

    Wave wave = {frameCount, sampleRate, 16, 1, samples.data()};
    return LoadSoundFromWave(wave);
}

std::string GetRecordsPath()
{
    return std::string(GetApplicationDirectory()) + "ruta-presupuesto-record.dat";
}

Records LoadRecords()
{
    Records records;
    const std::string path = GetRecordsPath();
    std::ifstream file(path);
    if (!file)
    {
        records.storageError = FileExists(path.c_str());
        return records;
    }

    std::string format;
    if (!(file >> format >> records.bestScore >> records.ratingCount >> records.ratingTotal) ||
        format != "RUTA1" || records.bestScore < 0 || records.ratingCount < 0 ||
        records.ratingTotal < records.ratingCount ||
        static_cast<long long>(records.ratingTotal) >
            static_cast<long long>(records.ratingCount) * 5)
    {
        records = {};
        records.storageError = true;
    }
    return records;
}

bool SaveRecords(const Records &records)
{
    std::ofstream file(GetRecordsPath(), std::ios::trunc);
    if (!file) return false;
    file << "RUTA1\n" << records.bestScore << '\n'
         << records.ratingCount << '\n' << records.ratingTotal << '\n';
    file.flush();
    return file.good();
}

bool IsOverGap(float x)
{
    for (const GroundGap &gap : Gaps)
        if (x > gap.start && x < gap.end) return true;
    return false;
}

void DrawBackdrop(float cameraX, int level)
{
    const Color skyTop[LevelCount] = {
        {102, 190, 245, 255}, {244, 166, 112, 255}, {60, 82, 145, 255}
    };
    const Color skyBottom[LevelCount] = {
        {211, 241, 252, 255}, {255, 225, 166, 255}, {177, 195, 235, 255}
    };
    DrawRectangleGradientV(0, 0, ScreenWidth, ScreenHeight, skyTop[level], skyBottom[level]);

    if (level == 2)
    {
        for (int i = 0; i < 20; ++i)
        {
            const int x = (i * 97 + 31) % ScreenWidth;
            const int y = 24 + (i * 43) % 150;
            DrawCircle(x, y, 2, Color{255, 249, 226, 220});
        }
    }

    for (int i = 0; i < 7; ++i)
    {
        const float x = i * 190.0f - std::fmod(cameraX * 0.18f, 190.0f);
        const int y = 93 + (i % 3) * 35;
        DrawCircle(static_cast<int>(x), y, 23, Color{255, 255, 255, 175});
        DrawCircle(static_cast<int>(x + 25), y + 5, 29, Color{255, 255, 255, 175});
        DrawCircle(static_cast<int>(x + 54), y, 22, Color{255, 255, 255, 175});
        DrawRectangle(static_cast<int>(x), y, 54, 18, Color{255, 255, 255, 175});
    }

    for (int i = 0; i < 9; ++i)
    {
        const float x = i * 155.0f - std::fmod(cameraX * 0.33f, 155.0f);
        const int height = 100 + (i % 3) * 27;
        const Color building[LevelCount] = {
            {154, 183, 219, 255}, {218, 164, 120, 255}, {95, 119, 177, 255}
        };
        DrawRectangle(static_cast<int>(x), static_cast<int>(GroundY - height), 112, height, building[level]);
        DrawRectangle(static_cast<int>(x + 16), static_cast<int>(GroundY - height + 18), 17, 24, Paper);
        DrawRectangle(static_cast<int>(x + 53), static_cast<int>(GroundY - height + 18), 17, 24, Paper);
        DrawRectangle(static_cast<int>(x + 16), static_cast<int>(GroundY - height + 58), 17, 24, Paper);
        DrawRectangle(static_cast<int>(x + 53), static_cast<int>(GroundY - height + 58), 17, 24, Paper);
        if (level == 1)
        {
            DrawRectangle(static_cast<int>(x), static_cast<int>(GroundY - height + 88), 112, 12, Red);
            DrawRectangle(static_cast<int>(x + 8), static_cast<int>(GroundY - height + 100), 96, 7, Gold);
        }
        else if (level == 2)
        {
            DrawRectangle(static_cast<int>(x + 8), static_cast<int>(GroundY - height - 12), 96, 12, Gold);
        }
    }
}

void DrawWorld(float cameraX, int nextDecision, const std::vector<Coin> &coins,
               const std::vector<PowerUp> &powerUps, const std::vector<Enemy> &enemies)
{
    const Color groundColors[LevelCount] = {
        {89, 163, 91, 255}, {184, 113, 75, 255}, {78, 111, 160, 255}
    };
    const Color trimColors[LevelCount] = {
        {255, 209, 94, 255}, {255, 203, 92, 255}, {255, 225, 133, 255}
    };

    for (int section = 0; section < LevelCount; ++section)
    {
        const float sectionStart = section * LevelWidth;
        const float sectionEnd = sectionStart + LevelWidth;
        float segmentStart = sectionStart;
        for (const GroundGap &gap : Gaps)
        {
            if (gap.start < sectionStart || gap.end > sectionEnd) continue;
            const float screenStart = segmentStart - cameraX;
            const float width = gap.start - segmentStart;
            if (width > 0)
            {
                DrawRectangle(static_cast<int>(screenStart), static_cast<int>(GroundY),
                              static_cast<int>(width), ScreenHeight - static_cast<int>(GroundY),
                              groundColors[section]);
                DrawRectangle(static_cast<int>(screenStart), static_cast<int>(GroundY),
                              static_cast<int>(width), 9, trimColors[section]);
            }
            segmentStart = gap.end;
        }

        const float screenStart = segmentStart - cameraX;
        const float width = sectionEnd - segmentStart;
        DrawRectangle(static_cast<int>(screenStart), static_cast<int>(GroundY),
                      static_cast<int>(width), ScreenHeight - static_cast<int>(GroundY),
                      groundColors[section]);
        DrawRectangle(static_cast<int>(screenStart), static_cast<int>(GroundY),
                      static_cast<int>(width), 9, trimColors[section]);
        if (section > 0)
        {
            DrawRectangle(static_cast<int>(sectionStart - cameraX), 220, 8, 280, Gold);
            DrawRectangle(static_cast<int>(sectionStart - cameraX - 18), 220, 44, 16, Red);
        }
    }

    const Color platformColors[LevelCount] = {
        Blue, {176, 93, 62, 255}, {75, 94, 151, 255}
    };
    for (const Platform &platform : Platforms)
    {
        Rectangle visible = platform.bounds;
        visible.x -= cameraX;
        const int section = std::clamp(static_cast<int>(platform.bounds.x / LevelWidth), 0, LevelCount - 1);
        DrawRectangleRounded(visible, 0.18f, 6, platformColors[section]);
        DrawRectangle(static_cast<int>(visible.x), static_cast<int>(visible.y),
                      static_cast<int>(visible.width), 5, Gold);
        for (int x = 8; x < static_cast<int>(visible.width); x += 32)
            DrawLine(static_cast<int>(visible.x + x), static_cast<int>(visible.y + 7),
                     static_cast<int>(visible.x + x), static_cast<int>(visible.y + visible.height - 2),
                     Color{255, 255, 255, 70});
    }

    for (const Ladder &ladder : Ladders)
    {
        const float x = ladder.bounds.x - cameraX;
        DrawRectangle(static_cast<int>(x), static_cast<int>(ladder.bounds.y), 5,
                      static_cast<int>(ladder.bounds.height), Red);
        DrawRectangle(static_cast<int>(x + ladder.bounds.width - 5), static_cast<int>(ladder.bounds.y), 5,
                      static_cast<int>(ladder.bounds.height), Red);
        for (float y = ladder.bounds.y + 10; y < GroundY; y += 22)
            DrawRectangle(static_cast<int>(x), static_cast<int>(y),
                          static_cast<int>(ladder.bounds.width), 5, Gold);
    }

    for (const Coin &coin : coins)
    {
        if (coin.collected) continue;
        const float x = coin.position.x - cameraX;
        DrawCircle(static_cast<int>(x), static_cast<int>(coin.position.y), 13, Gold);
        DrawCircleLines(static_cast<int>(x), static_cast<int>(coin.position.y), 13, Red);
        DrawText("$", static_cast<int>(x - 5), static_cast<int>(coin.position.y - 8), 16, DeepBlue);
    }

    for (const PowerUp &powerUp : powerUps)
    {
        if (powerUp.collected) continue;
        const Vector2 position = {powerUp.position.x - cameraX, powerUp.position.y};
        if (powerUp.grantsShield)
        {
            DrawPoly(position, 5, 18, -18, Gold);
            DrawCircle(static_cast<int>(position.x), static_cast<int>(position.y), 5, Paper);
        }
        else
        {
            DrawCircle(static_cast<int>(position.x), static_cast<int>(position.y + 2), 14, LeafGreen);
            DrawEllipse(static_cast<int>(position.x - 6), static_cast<int>(position.y - 8), 8, 5, Gold);
            DrawEllipse(static_cast<int>(position.x + 7), static_cast<int>(position.y - 7), 7, 5, BrightRed);
            DrawText("+", static_cast<int>(position.x - 5), static_cast<int>(position.y - 6), 15, White);
        }
    }

    for (const Enemy &enemy : enemies)
    {
        if (!enemy.alive) continue;
        const float x = enemy.bounds.x - cameraX;
        DrawRectangleRounded({x, enemy.bounds.y + 5, enemy.bounds.width, enemy.bounds.height - 5},
                             0.22f, 5, Red);
        DrawRectangle(static_cast<int>(x + 4), static_cast<int>(enemy.bounds.y + 2), 9, 6, Gold);
        DrawRectangle(static_cast<int>(x + 19), static_cast<int>(enemy.bounds.y + 2), 9, 6, Gold);
        DrawCircle(static_cast<int>(x + 11), static_cast<int>(enemy.bounds.y + 15), 3, White);
        DrawCircle(static_cast<int>(x + 24), static_cast<int>(enemy.bounds.y + 15), 3, White);
        DrawCircle(static_cast<int>(x + 11), static_cast<int>(enemy.bounds.y + 15), 1, Ink);
        DrawCircle(static_cast<int>(x + 24), static_cast<int>(enemy.bounds.y + 15), 1, Ink);
        DrawText("$", static_cast<int>(x + 12), static_cast<int>(enemy.bounds.y + 21), 12, Paper);
    }

    if (nextDecision < static_cast<int>(Decisions.size()))
    {
        const Decision &decision = Decisions[nextDecision];
        const float x = decision.x - cameraX;
        DrawRectangleRounded({x - 34, 378, 68, 87}, 0.15f, 7, Paper);
        DrawRectangle(static_cast<int>(x - 4), 398, 8, 44, Red);
        DrawCircle(static_cast<int>(x), 389, 8, Gold);
        DrawText("?", static_cast<int>(x - 4), 382, 14, DeepBlue);
    }

    const float vaultX = WorldWidth - 180.0f - cameraX;
    DrawRectangleRounded({vaultX, 390, 108, 110}, 0.12f, 8, DeepBlue);
    DrawRectangleRounded({vaultX + 13, 404, 82, 82}, 0.2f, 8, Gold);
    DrawCircleLines(static_cast<int>(vaultX + 54), 445, 27, DeepBlue);
    DrawCircle(static_cast<int>(vaultX + 54), 445, 7, Red);
    DrawText("META", static_cast<int>(vaultX + 26), 365, 16, DeepBlue);
}

void DrawPlayer(Rectangle player, bool facingRight, float walkTime, bool shielded)
{
    const Color bodyColor = shielded ? Gold : BrightRed;
    const int legOffset = static_cast<int>(std::sin(walkTime * 14.0f) * 3.0f);
    if (shielded)
        DrawCircleLines(static_cast<int>(player.x + player.width / 2),
                        static_cast<int>(player.y + player.height / 2), 31, Gold);
    DrawEllipse(static_cast<int>(player.x + player.width / 2),
                static_cast<int>(player.y + player.height + 2), 19, 5, Color{0, 0, 0, 45});
    DrawRectangleRounded({player.x + 4, player.y + 17, player.width - 8, player.height - 17},
                         0.22f, 5, Blue);
    DrawRectangleRounded({player.x + 3, player.y + 13, player.width - 6, 20},
                         0.22f, 5, bodyColor);
    DrawCircle(static_cast<int>(player.x + player.width / 2),
               static_cast<int>(player.y + 11), 12, Color{230, 178, 139, 255});
    DrawCircle(static_cast<int>(player.x + (facingRight ? 25 : 9)),
               static_cast<int>(player.y + 9), 3, Ink);
    DrawRectangle(static_cast<int>(player.x + 2), static_cast<int>(player.y + 5),
                  static_cast<int>(player.width - 4), 6, DeepBlue);
    DrawRectangle(static_cast<int>(player.x + 6), static_cast<int>(player.y + 30),
                  static_cast<int>(player.width - 12), 4, Gold);
    DrawLineEx({player.x + 10, player.y + player.height - 2},
               {player.x + 8 + legOffset, player.y + player.height + 3}, 4, NightBlue);
    DrawLineEx({player.x + 24, player.y + player.height - 2},
               {player.x + 26 - legOffset, player.y + player.height + 3}, 4, NightBlue);
}

void DrawHud(int balance, int savings, int progress, int score, int bestScore,
             int lives, int ammo, float timeLeft, int level, bool muted,
             bool soundAvailable)
{
    DrawRectangle(0, 0, ScreenWidth, 76, Color{255, 249, 226, 245});
    DrawText("RUTA DEL PRESUPUESTO", 18, 12, 19, DeepBlue);
    DrawText(TextFormat("NIVEL %d/3  %s", level + 1, StageNames[level]), 18, 43, 14, Red);
    if (balance < 0)
        DrawText(TextFormat("DEUDA $%d", -balance), 290, 12, 16, Red);
    else
        DrawText(TextFormat("SALDO $%d", balance), 290, 12, 16, Ink);
    DrawText(TextFormat("AHORRO $%d/6000", savings), 290, 42, 14, LeafGreen);
    DrawText(TextFormat("PUNTOS %06d", score), 500, 12, 16, DeepBlue);
    DrawText(TextFormat("RECORD %06d", bestScore), 500, 42, 14, Muted);
    DrawText(TextFormat("TIEMPO %03d", static_cast<int>(timeLeft)), 720, 12, 16, Red);
    DrawText(TextFormat("VIDAS %d   SEMILLAS %d", lives, ammo), 720, 42, 14, Ink);
    DrawText(TextFormat("%d/6", progress), 918, 15, 16, DeepBlue);
    DrawText(!soundAvailable ? "SIN AUDIO" : (muted ? "MUDO [M]" : "AUDIO [M]"),
             900, 45, 12, Muted);
}

void DrawDecisionCard(int index, int balance, int savings, int score)
{
    const Decision &decision = Decisions[index];
    DrawRectangle(0, 0, ScreenWidth, ScreenHeight, Color{17, 30, 53, 185});
    DrawRectangleRounded({135, 112, 730, 386}, 0.05f, 12, Paper);
    DrawRectangle(135, 112, 730, 12, Red);
    DrawText(decision.title, 180, 145, 18, Red);
    DrawText(decision.question, 180, 179, 26, Ink);
    if (balance < 0)
        DrawText(TextFormat("DEUDA $%d     AHORRO $%d     PUNTOS %d", -balance, savings, score),
                 180, 219, 16, Muted);
    else
        DrawText(TextFormat("SALDO $%d     AHORRO $%d     PUNTOS %d", balance, savings, score),
                 180, 219, 16, Muted);

    for (int i = 0; i < 2; ++i)
    {
        const Rectangle optionBounds = {170, static_cast<float>(264 + i * 82), 660, 60};
        const bool hovered = CheckCollisionPointRec(GetMousePosition(), optionBounds);
        const Color choiceColor = i == 0 ? Color{214, 239, 195, 255} : Color{255, 223, 198, 255};
        DrawRectangleRounded(optionBounds, 0.15f, 8, choiceColor);
        if (hovered) DrawRectangleLinesEx(optionBounds, 3, DeepBlue);
        DrawCircle(198, static_cast<int>(optionBounds.y + 30), 15, i == 0 ? LeafGreen : Red);
        DrawText(TextFormat("%d", i + 1), 193, static_cast<int>(optionBounds.y + 18), 17, White);
        DrawText(decision.options[i], 232, static_cast<int>(optionBounds.y + 18), 18, Ink);
    }
    DrawText("Haz clic en una opcion o pulsa 1 / 2. La respuesta se aplica al instante.",
             180, 438, 15, Muted);
}

void DrawFeedback(int index, int choice, int balanceChange, int savingsChange)
{
    const Decision &decision = Decisions[index];
    const bool risky = decision.risky[choice];
    DrawRectangle(0, 0, ScreenWidth, ScreenHeight, Color{17, 30, 53, 195});
    DrawRectangleRounded({175, 188, 650, 235}, 0.05f, 12, Paper);
    DrawRectangle(175, 188, 650, 12, risky ? Red : LeafGreen);
    DrawText(risky ? "CONSECUENCIA / ALERTA" : "CONSECUENCIA / BUENA JUGADA",
             220, 222, 18, risky ? Red : LeafGreen);
    DrawText(decision.feedback[choice], 220, 266, 18, Ink);
    DrawText(TextFormat("Saldo: %s$%d     Ahorro: %s$%d",
                        balanceChange < 0 ? "-" : "+", std::abs(balanceChange),
                        savingsChange < 0 ? "-" : "+", std::abs(savingsChange)),
             220, 304, 16, DeepBlue);
    if (risky)
        DrawText("Aparecio una factura perseguidora. Salta o usa una semilla.",
                 220, 340, 15, Red);
    DrawText("Presiona ENTER para seguir la ruta", 220, 385, 16, Muted);
}

void DrawMessagePanel(const char *label, const char *title, const char *detail, int score)
{
    DrawRectangle(0, 0, ScreenWidth, ScreenHeight, Color{17, 30, 53, 190});
    DrawRectangleRounded({175, 148, 650, 300}, 0.06f, 14, Paper);
    DrawRectangle(175, 148, 650, 12, Gold);
    DrawText(label, 225, 188, 18, Red);
    DrawText(title, 225, 229, 36, DeepBlue);
    DrawText(detail, 225, 285, 18, Ink);
    DrawText(TextFormat("PUNTUACION: %06d", score), 225, 334, 22, LeafGreen);
    DrawText("Presiona ENTER para continuar", 225, 391, 16, Muted);
}

} // namespace

int main()
{
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(ScreenWidth, ScreenHeight, "Ruta del presupuesto");
    SetTargetFPS(60);

#if defined(PLATFORM_WEB)
    std::vector<int> fontCodepoints;
    for (int codepoint = 32; codepoint <= 126; ++codepoint)
        fontCodepoints.push_back(codepoint);
    for (int codepoint = 160; codepoint <= 255; ++codepoint)
        fontCodepoints.push_back(codepoint);
    GameFont = LoadFontEx("/fonts/AtkinsonHyperlegible-Regular.ttf", 48,
                          fontCodepoints.data(), static_cast<int>(fontCodepoints.size()));
    if (GameFont.texture.id == 0)
        TraceLog(LOG_WARNING, "Could not load the browser font; using raylib's default font.");
    else
        SetTextureFilter(GameFont.texture, TEXTURE_FILTER_BILINEAR);
#endif

    InitAudioDevice();
    const bool audioAvailable = IsAudioDeviceReady();
    Sound chooseSound = {};
    Sound pickupSound = {};
    Sound hitSound = {};
    if (audioAvailable)
    {
        chooseSound = MakeTone(660.0f, 0.12f);
        pickupSound = MakeTone(880.0f, 0.08f);
        hitSound = MakeTone(190.0f, 0.16f);
    }
    const bool soundAvailable = audioAvailable && IsSoundValid(chooseSound) &&
        IsSoundValid(pickupSound) && IsSoundValid(hitSound);
    bool muted = false;

    enum class Screen { Title, Playing, Paused, Decision, Feedback, LevelComplete, Finished, GameOver, Rating };
    Screen screen = Screen::Title;
    int selectedDifficulty = 1;
    std::vector<Coin> coins = MakeCoins();
    std::vector<PowerUp> powerUps = MakePowerUps();
    std::vector<Enemy> enemies = MakeEnemies(Difficulties[selectedDifficulty].enemySpeed);
    std::vector<Projectile> projectiles;
    Rectangle player = {90, GroundY - PlayerHeight, PlayerWidth, PlayerHeight};
    Vector2 velocity = {0, 0};
    float cameraX = 0;
    float walkTime = 0;
    float remainingTime = Difficulties[selectedDifficulty].duration;
    float shieldTime = 0;
    float invulnerabilityTime = 0;
    float shotCooldown = 0;
    int balance = 12000;
    int savings = 0;
    int score = 0;
    Records records = LoadRecords();
    int bestScore = records.bestScore;
    int ratingCount = records.ratingCount;
    int ratingTotal = records.ratingTotal;
    int lives = Difficulties[selectedDifficulty].startingLives;
    int ammo = 0;
    int level = 0;
    int nextDecision = 0;
    int activeDecision = -1;
    int selectedOption = -1;
    int submittedRating = 0;
    int lastBalanceChange = 0;
    int lastSavingsChange = 0;
    float checkpointX = 90;
    bool facingRight = true;
    bool onGround = true;
    bool climbing = false;
    bool timedOut = false;

    auto playSound = [&](Sound sound) {
        if (soundAvailable && !muted && IsSoundValid(sound)) PlaySound(sound);
    };

    auto saveRecord = [&]() {
        if (score > bestScore)
        {
            bestScore = score;
            records.bestScore = bestScore;
            if (!records.storageError && !SaveRecords(records)) records.storageError = true;
        }
    };

    auto resetRun = [&]() {
        coins = MakeCoins();
        powerUps = MakePowerUps();
        enemies = MakeEnemies(Difficulties[selectedDifficulty].enemySpeed);
        projectiles.clear();
        player = {90, GroundY - PlayerHeight, PlayerWidth, PlayerHeight};
        velocity = {0, 0};
        cameraX = 0;
        walkTime = 0;
        remainingTime = Difficulties[selectedDifficulty].duration;
        shieldTime = 0;
        invulnerabilityTime = 0;
        shotCooldown = 0;
        balance = 12000;
        savings = 0;
        score = 0;
        lives = Difficulties[selectedDifficulty].startingLives;
        ammo = 0;
        level = 0;
        nextDecision = 0;
        activeDecision = -1;
        selectedOption = -1;
        submittedRating = 0;
        lastBalanceChange = 0;
        lastSavingsChange = 0;
        checkpointX = 90;
        facingRight = true;
        onGround = true;
        climbing = false;
        timedOut = false;
    };

    auto loseLife = [&]() {
        --lives;
        score = std::max(0, score - 150);
        playSound(hitSound);
        if (lives <= 0)
        {
            screen = Screen::GameOver;
            saveRecord();
            return;
        }
        player.x = checkpointX;
        player.y = GroundY - PlayerHeight;
        velocity = {0, 0};
        onGround = true;
        climbing = false;
        invulnerabilityTime = 1.8f;
    };

    while (!WindowShouldClose())
    {
        const float delta = std::min(GetFrameTime(), 1.0f / 30.0f);

        if (IsKeyPressed(KEY_M) && soundAvailable) muted = !muted;
        if (screen == Screen::Playing && IsKeyPressed(KEY_P)) screen = Screen::Paused;
        else if (screen == Screen::Paused && IsKeyPressed(KEY_P)) screen = Screen::Playing;

        if (screen == Screen::Title)
        {
            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A))
                selectedDifficulty = std::max(0, selectedDifficulty - 1);
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D))
                selectedDifficulty = std::min(static_cast<int>(std::size(Difficulties)) - 1,
                                              selectedDifficulty + 1);
            const Vector2 mouse = GetMousePosition();
            bool startGame = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) ||
                (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                 CheckCollisionPointRec(mouse, {165, 443, 248, 42}));
            for (int i = 0; i < static_cast<int>(std::size(Difficulties)); ++i)
            {
                if (IsKeyPressed(static_cast<KeyboardKey>(KEY_ONE + i)))
                {
                    selectedDifficulty = i;
                    startGame = true;
                }
                const Rectangle bounds = {165.0f + i * 222.0f, 374, 210, 38};
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                    CheckCollisionPointRec(mouse, bounds))
                {
                    selectedDifficulty = i;
                    startGame = true;
                }
            }
            if (startGame)
            {
                resetRun();
                screen = Screen::Playing;
                playSound(chooseSound);
            }
        }
        else if (screen == Screen::Paused)
        {
        }
        else if (screen == Screen::Playing)
        {
            remainingTime = std::max(0.0f, remainingTime - delta);
            shieldTime = std::max(0.0f, shieldTime - delta);
            invulnerabilityTime = std::max(0.0f, invulnerabilityTime - delta);
            shotCooldown = std::max(0.0f, shotCooldown - delta);

            if (remainingTime <= 0.0f)
            {
                timedOut = true;
                screen = Screen::GameOver;
                saveRecord();
            }
            else
            {
                float direction = 0;
                if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) direction += 1;
                if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) direction -= 1;
                velocity.x = direction * 250.0f;
                if (direction != 0)
                {
                    facingRight = direction > 0;
                    walkTime += delta;
                }

                const bool onLadder = std::any_of(Ladders.begin(), Ladders.end(),
                    [&](const Ladder &ladder) {
                        Rectangle overlap = {player.x + 7, player.y + 4, player.width - 14, player.height - 4};
                        return CheckCollisionRecs(overlap, ladder.bounds);
                    });
                const bool verticalInput = IsKeyDown(KEY_UP) || IsKeyDown(KEY_W) ||
                    IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S);
                if (onLadder && verticalInput) climbing = true;

                if (climbing)
                {
                    if (!onLadder) climbing = false;
                }

                if (climbing)
                {
                    velocity.y = 0;
                    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) player.y -= 175.0f * delta;
                    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) player.y += 175.0f * delta;
                    const Ladder *currentLadder = nullptr;
                    for (const Ladder &ladder : Ladders)
                    {
                        Rectangle overlap = {player.x + 7, player.y + 4, player.width - 14, player.height - 4};
                        if (CheckCollisionRecs(overlap, ladder.bounds))
                        {
                            currentLadder = &ladder;
                            break;
                        }
                    }
                    if (currentLadder && player.y + player.height <= currentLadder->bounds.y)
                    {
                        player.y = currentLadder->bounds.y - player.height;
                        climbing = false;
                        onGround = true;
                    }
                    else if (player.y + player.height >= GroundY)
                    {
                        player.y = GroundY - player.height;
                        climbing = false;
                        onGround = true;
                    }
                }
                else
                {
                    if (onGround && (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP) ||
                                     IsKeyPressed(KEY_W)))
                    {
                        velocity.y = -590.0f;
                        onGround = false;
                        playSound(chooseSound);
                    }
                    const float previousBottom = player.y + player.height;
                    player.y += velocity.y * delta;
                    velocity.y += 1450.0f * delta;
                    onGround = false;

                    for (const Platform &platform : Platforms)
                    {
                        const bool horizontalOverlap = player.x + player.width > platform.bounds.x &&
                            player.x < platform.bounds.x + platform.bounds.width;
                        const bool crossedTop = previousBottom <= platform.bounds.y &&
                            player.y + player.height >= platform.bounds.y && velocity.y >= 0;
                        if (horizontalOverlap && crossedTop)
                        {
                            player.y = platform.bounds.y - player.height;
                            velocity.y = 0;
                            onGround = true;
                        }
                    }

                    const float groundProbe = player.x + player.width * 0.5f;
                    if (player.y + player.height >= GroundY && !IsOverGap(groundProbe))
                    {
                        player.y = GroundY - player.height;
                        velocity.y = 0;
                        onGround = true;
                    }
                }

                player.x += velocity.x * delta;
                player.x = std::clamp(player.x, 0.0f, WorldWidth - PlayerWidth);

                if (player.y > ScreenHeight + 20)
                {
                    loseLife();
                }
                else
                {
                    Rectangle playerBounds = {player.x + 2, player.y + 2, player.width - 4, player.height - 3};

                    for (Coin &coin : coins)
                    {
                        if (!coin.collected && CheckCollisionCircleRec(coin.position, 16, playerBounds))
                        {
                            coin.collected = true;
                            balance += 500;
                            score += 100;
                            playSound(pickupSound);
                        }
                    }

                    for (PowerUp &powerUp : powerUps)
                    {
                        if (powerUp.collected || !CheckCollisionCircleRec(powerUp.position, 18, playerBounds))
                            continue;
                        powerUp.collected = true;
                        score += 150;
                        if (powerUp.grantsShield) shieldTime = 8.0f;
                        else ammo += 5;
                        playSound(pickupSound);
                    }

                    for (Enemy &enemy : enemies)
                    {
                        if (!enemy.alive) continue;
                        enemy.bounds.x += enemy.speed * delta;
                        if (enemy.bounds.x >= enemy.patrolEnd || enemy.bounds.x <= enemy.patrolStart)
                        {
                            enemy.speed = -enemy.speed;
                            enemy.bounds.x = std::clamp(enemy.bounds.x, enemy.patrolStart, enemy.patrolEnd);
                        }
                    }

                    if (IsKeyPressed(KEY_X) && ammo > 0 && shotCooldown <= 0.0f)
                    {
                        const float shotX = facingRight ? player.x + player.width : player.x - 18;
                        projectiles.push_back({{shotX, player.y + 20, 18, 10},
                                               facingRight ? 460.0f : -460.0f, true});
                        --ammo;
                        shotCooldown = 0.24f;
                        playSound(chooseSound);
                    }

                    for (Projectile &projectile : projectiles)
                    {
                        if (!projectile.alive) continue;
                        projectile.bounds.x += projectile.speed * delta;
                        if (projectile.bounds.x < 0 || projectile.bounds.x > WorldWidth)
                        {
                            projectile.alive = false;
                            continue;
                        }
                        for (Enemy &enemy : enemies)
                        {
                            if (enemy.alive && CheckCollisionRecs(projectile.bounds, enemy.bounds))
                            {
                                enemy.alive = false;
                                projectile.alive = false;
                                score += 250;
                                playSound(pickupSound);
                                break;
                            }
                        }
                    }
                    projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
                        [](const Projectile &projectile) { return !projectile.alive; }), projectiles.end());

                    for (Enemy &enemy : enemies)
                    {
                        if (!enemy.alive || !CheckCollisionRecs(playerBounds, enemy.bounds)) continue;
                        const bool stomp = velocity.y > 80.0f &&
                            player.y + player.height < enemy.bounds.y + 17.0f;
                        if (stomp)
                        {
                            enemy.alive = false;
                            velocity.y = -360.0f;
                            onGround = false;
                            score += 250;
                            playSound(pickupSound);
                        }
                        else if (shieldTime <= 0.0f && invulnerabilityTime <= 0.0f)
                        {
                            loseLife();
                        }
                        break;
                    }

                    if (screen == Screen::Playing && nextDecision < static_cast<int>(Decisions.size()) &&
                        player.x + player.width >= Decisions[nextDecision].x)
                    {
                        activeDecision = nextDecision;
                        selectedOption = -1;
                        screen = Screen::Decision;
                    }

                    if (screen == Screen::Playing && player.x >= (level + 1) * LevelWidth - 60.0f)
                    {
                        if (level == 2)
                        {
                            const int timeBonus = static_cast<int>(remainingTime) * 5;
                            score += timeBonus;
                            screen = Screen::Finished;
                            saveRecord();
                        }
                        else
                        {
                            screen = Screen::LevelComplete;
                            score += 300;
                        }
                    }

                    cameraX = std::clamp(player.x - ScreenWidth * 0.34f,
                                         0.0f, WorldWidth - ScreenWidth);
                }
            }
        }
        else if (screen == Screen::Decision)
        {
            if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) selectedOption = 0;
            if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) selectedOption = 1;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                const Vector2 mouse = GetMousePosition();
                for (int i = 0; i < 2; ++i)
                {
                    const Rectangle optionBounds = {170, static_cast<float>(264 + i * 82), 660, 60};
                    if (CheckCollisionPointRec(mouse, optionBounds))
                    {
                        selectedOption = i;
                        break;
                    }
                }
            }
            if (selectedOption >= 0)
            {
                const Decision &decision = Decisions[activeDecision];
                const int oldBalance = balance;
                const int oldSavings = savings;
                const int savingsChange = decision.savingsChange[selectedOption];
                if (savingsChange < 0)
                {
                    const int requested = -savingsChange;
                    const int usedFromSavings = std::min(savings, requested);
                    savings -= usedFromSavings;
                    balance += decision.balanceChange[selectedOption] -
                        (requested - usedFromSavings);
                }
                else
                {
                    balance += decision.balanceChange[selectedOption];
                    savings += savingsChange;
                }
                lastBalanceChange = balance - oldBalance;
                lastSavingsChange = savings - oldSavings;
                score += decision.risky[selectedOption] ? 50 : 300;
                if (decision.risky[selectedOption])
                {
                    const float enemyX = std::min(player.x + 240.0f, WorldWidth - 100.0f);
                    const float stageSpeed = 1.0f + level * 0.18f;
                    enemies.push_back({{enemyX, GroundY - 30, 34, 30},
                                       enemyX - 60, enemyX + 100,
                                       -90.0f * Difficulties[selectedDifficulty].enemySpeed * stageSpeed,
                                       true});
                }
                screen = Screen::Feedback;
                playSound(decision.risky[selectedOption] ? hitSound : pickupSound);
            }
        }
        else if (screen == Screen::Feedback && IsKeyPressed(KEY_ENTER))
        {
            ++nextDecision;
            player.x = std::min(player.x + 70.0f, WorldWidth - PlayerWidth);
            checkpointX = player.x;
            screen = Screen::Playing;
        }
        else if (screen == Screen::LevelComplete && IsKeyPressed(KEY_ENTER))
        {
            ++level;
            player.x = level * LevelWidth + 80.0f;
            player.y = GroundY - PlayerHeight;
            velocity = {0, 0};
            onGround = true;
            climbing = false;
            checkpointX = player.x;
            cameraX = std::clamp(player.x - ScreenWidth * 0.34f,
                                 0.0f, WorldWidth - ScreenWidth);
            screen = Screen::Playing;
        }
        else if ((screen == Screen::Finished || screen == Screen::GameOver) &&
                 (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)))
        {
            submittedRating = 0;
            screen = Screen::Rating;
        }
        else if (screen == Screen::Rating)
        {
            for (int value = 1; value <= 5; ++value)
            {
                if (IsKeyPressed(static_cast<KeyboardKey>(KEY_ONE + value - 1)))
                {
                    submittedRating = value;
                    if (ratingCount == std::numeric_limits<int>::max() ||
                        ratingTotal > std::numeric_limits<int>::max() - value)
                    {
                        records.storageError = true;
                    }
                    else
                    {
                        ++ratingCount;
                        ratingTotal += value;
                        records.ratingCount = ratingCount;
                        records.ratingTotal = ratingTotal;
                        if (!records.storageError && !SaveRecords(records))
                            records.storageError = true;
                    }
                    break;
                }
            }
            if (IsKeyPressed(KEY_ENTER))
                screen = Screen::Title;
        }

        BeginDrawing();
        DrawBackdrop(cameraX, level);
        DrawWorld(cameraX, nextDecision, coins, powerUps, enemies);
        for (const Projectile &projectile : projectiles)
        {
            if (!projectile.alive) continue;
            DrawRectangleRounded({projectile.bounds.x - cameraX, projectile.bounds.y,
                                  projectile.bounds.width, projectile.bounds.height},
                                 0.35f, 5, Gold);
        }
        const bool blinking = invulnerabilityTime > 0.0f &&
            static_cast<int>(invulnerabilityTime * 12.0f) % 2 == 0;
        if (!blinking)
            DrawPlayer({player.x - cameraX, player.y, player.width, player.height},
                       facingRight, walkTime, shieldTime > 0.0f);
        DrawHud(balance, savings, nextDecision, score, bestScore, lives, ammo,
                    remainingTime, level, muted, soundAvailable);

        if (screen == Screen::Title)
        {
            DrawRectangle(0, 0, ScreenWidth, ScreenHeight, Color{17, 30, 53, 120});
            DrawRectangleRounded({120, 92, 760, 440}, 0.05f, 14, Paper);
            DrawRectangle(120, 92, 760, 14, Red);
            DrawText("AVENTURA DE PLATAFORMAS / FINANZAS PERSONALES", 165, 128, 16, Red);
            DrawText("Ruta del", 165, 166, 48, DeepBlue);
            DrawText("presupuesto", 165, 215, 48, DeepBlue);
            DrawText("Tres niveles. Cada decision deja una consecuencia.", 165, 283, 19, Ink);
            DrawText("Monedas, deudas perseguidoras, escaleras y una meta final.", 165, 317, 16, Muted);
            DrawText("ELIGE UNA DIFICULTAD PARA EMPEZAR", 165, 350, 14, Red);
            const float difficultyCardWidth = 210.0f;
            for (int i = 0; i < static_cast<int>(std::size(Difficulties)); ++i)
            {
                const float x = 165.0f + i * (difficultyCardWidth + 12.0f);
                DrawRectangleRounded({x, 374, difficultyCardWidth, 38}, 0.16f, 7,
                                     i == selectedDifficulty ? DeepBlue : Color{231, 234, 224, 255});
                DrawText(TextFormat("%d  %s  /  JUGAR", i + 1, Difficulties[i].name),
                         static_cast<int>(x + 16), 385, 15,
                         i == selectedDifficulty ? White : Ink);
            }
            DrawText(Difficulties[selectedDifficulty].description, 165, 420, 14, Muted);
            DrawRectangleRounded({165, 443, 248, 42}, 0.15f, 8, Red);
            DrawText("JUGAR  /  ESPACIO", 184, 456, 15, White);
            DrawText("Mover A/D  Saltar ESPACIO  Escalar W/S  Pausa P",
                     165, 491, 13, Ink);
            DrawText(TextFormat("Semillas X  Sonido M  Record local: %06d", bestScore),
                     165, 508, 12, Blue);
            if (records.storageError)
                DrawText("No se pudo guardar datos en este equipo.", 650, 508, 12, Red);
        }
        else if (screen == Screen::Decision)
        {
            DrawDecisionCard(activeDecision, balance, savings, score);
        }
        else if (screen == Screen::Feedback)
        {
            DrawFeedback(activeDecision, selectedOption, lastBalanceChange, lastSavingsChange);
        }
        else if (screen == Screen::LevelComplete)
        {
            DrawMessagePanel("NIVEL COMPLETADO", StageNames[level], StageLessons[level], score);
        }
        else if (screen == Screen::Paused)
        {
            DrawRectangle(0, 0, ScreenWidth, ScreenHeight, Color{17, 30, 53, 185});
            DrawRectangleRounded({250, 210, 500, 170}, 0.06f, 14, Paper);
            DrawRectangle(250, 210, 500, 12, Gold);
            DrawText("PAUSA", 305, 255, 36, DeepBlue);
            DrawText(TextFormat("%s  /  %s", StageNames[level],
                                Difficulties[selectedDifficulty].name),
                     305, 304, 17, Ink);
            DrawText("Presiona P para continuar", 305, 344, 16, Muted);
        }
        else if (screen == Screen::Finished)
        {
            DrawRectangle(0, 0, ScreenWidth, ScreenHeight, Color{17, 30, 53, 195});
            DrawRectangleRounded({150, 112, 700, 380}, 0.05f, 14, Paper);
            DrawRectangle(150, 112, 700, 13, Gold);
            DrawText("META ALCANZADA / FONDO FUTURO", 205, 150, 18, LeafGreen);
            DrawText("Tu plan llego", 205, 192, 40, DeepBlue);
            DrawText("mas lejos que la deuda.", 205, 235, 40, DeepBlue);
            if (balance < 0)
                DrawText(TextFormat("Deuda final: $%d      Ahorro: $%d", -balance, savings),
                         205, 302, 20, Ink);
            else
                DrawText(TextFormat("Saldo final: $%d      Ahorro: $%d", balance, savings),
                         205, 302, 20, Ink);
            DrawText(TextFormat("Puntos: %06d     Tiempo restante: %d s", score,
                                static_cast<int>(remainingTime)), 205, 340, 18, Blue);
            DrawText(TextFormat("Record local: %06d", bestScore), 205, 373, 17, Red);
            DrawText("ENTER para calificar la experiencia", 205, 427, 16, Muted);
        }
        else if (screen == Screen::GameOver)
        {
            DrawMessagePanel("PARTIDA TERMINADA",
                             timedOut ? "Se acabo el tiempo" : "Aun puedes intentarlo",
                             "La siguiente pantalla te permite valorar la experiencia.", score);
        }
        else if (screen == Screen::Rating)
        {
            DrawRectangle(0, 0, ScreenWidth, ScreenHeight, Color{17, 30, 53, 190});
            DrawRectangleRounded({190, 150, 620, 300}, 0.05f, 14, Paper);
            DrawRectangle(190, 150, 620, 12, Gold);
            DrawText("CALIFICA LA EXPERIENCIA", 240, 191, 20, Red);
            if (submittedRating == 0)
            {
                DrawText("Que tan entretenida fue la aventura?", 240, 240, 22, DeepBlue);
                DrawText("1-5 para calificar; ENTER para omitir. Es anonima.", 240, 286, 16, Ink);
                DrawText("1   2   3   4   5", 240, 333, 30, Gold);
            }
            else
            {
                DrawText("Gracias por valorar el prototipo.", 240, 240, 22, DeepBlue);
                DrawText(TextFormat("Respuestas en este equipo: %d", ratingCount), 240, 289, 17, Ink);
                DrawText(TextFormat("Promedio: %.1f / 5",
                                    ratingCount > 0 ? static_cast<float>(ratingTotal) / ratingCount : 0.0f),
                         240, 323, 19, LeafGreen);
                if (records.storageError)
                    DrawText("No se pudo actualizar el archivo de resultados.", 240, 350, 13, Red);
                DrawText("ENTER para volver al inicio", 240, 377, 16, Muted);
            }
        }
        EndDrawing();
    }

    if (audioAvailable)
    {
        if (IsSoundValid(chooseSound)) UnloadSound(chooseSound);
        if (IsSoundValid(pickupSound)) UnloadSound(pickupSound);
        if (IsSoundValid(hitSound)) UnloadSound(hitSound);
        CloseAudioDevice();
    }
    CloseWindow();
    return 0;
}
