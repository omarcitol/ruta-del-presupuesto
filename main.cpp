#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <limits>
#include <random>
#include <string>
#include <vector>

#if defined(PLATFORM_WEB)
#include <emscripten.h>
#endif

namespace {

constexpr int ScreenWidth = 1000;
constexpr int ScreenHeight = 600;
constexpr float GroundY = 500.0f;
constexpr float LevelWidth = 2000.0f;
constexpr float WorldWidth = LevelWidth * 3.0f;
constexpr float VaultX = WorldWidth - 180.0f;
constexpr float PlayerWidth = 34.0f;
constexpr float PlayerHeight = 48.0f;
constexpr int LevelCount = 3;
constexpr int DecisionPoolPerStage = 5;
constexpr int DecisionCount = LevelCount;

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

Font GameFont = {};

void DrawGameText(const char *text, int x, int y, int fontSize, Color color)
{
    // El segundo y tercer trazo refuerzan la fuente incluida sin perder legibilidad.
    if (GameFont.texture.id != 0)
    {
        DrawTextEx(GameFont, text, {static_cast<float>(x), static_cast<float>(y)},
                   static_cast<float>(fontSize), 0.0f, color);
        if (fontSize >= 14)
        {
            DrawTextEx(GameFont, text, {static_cast<float>(x + 1), static_cast<float>(y)},
                       static_cast<float>(fontSize), 0.0f, color);
            DrawTextEx(GameFont, text, {static_cast<float>(x), static_cast<float>(y + 1)},
                       static_cast<float>(fontSize), 0.0f, color);
        }
        return;
    }
    ::DrawText(text, x, y, fontSize, color);
    if (fontSize >= 14)
    {
        ::DrawText(text, x + 1, y, fontSize, color);
        ::DrawText(text, x, y + 1, fontSize, color);
    }
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
    float velocityY = 0.0f;
};

struct Enemy {
    Rectangle bounds;
    float patrolStart;
    float patrolEnd;
    float speed;
    bool alive;
    bool throwsThings = false;
    bool jumps = false;
    float jumpCooldown = 1.5f;
    float verticalSpeed = 0.0f;
    float shotCooldown = 2.0f;
};

struct Projectile {
    Rectangle bounds;
    float speed;
    bool alive;
    int level;
};

struct HazardProjectile {
    Rectangle bounds;
    float speedX;
    float speedY;
    float gravity;
    bool alive;
    int level;
};

struct QuestionBlock {
    Rectangle bounds;
    bool used;
};

struct Cannon {
    Rectangle bounds;
    float shotCooldown;
};

struct Decision {
    float x;
    const char *title;
    const char *question;
    const char *options[3];
    const char *feedback[3];
    int balanceChange[3];
    int savingsChange[3];
    bool risky[3];
    bool losesLife[3];
};

struct Records {
    int bestScore = 0;
    int ratingCount = 0;
    int ratingTotal = 0;
    bool storageError = false;
};

struct GameModeSettings {
    const char *name;
    const char *description;
    float duration;
    int startingLives;
    float enemySpeed;
};

const GameModeSettings AdventureMode = {
    "AVENTURA", "180 s / 3 vidas / enemigos mas veloces", 180.0f, 3, 1.15f
};

const char *const StageNames[LevelCount] = {
    "Barrio del ahorro", "Avenida de las cuotas", "Mercado de las metas"
};

const char *const StageLessons[LevelCount] = {
    "Prioriza necesidades y aparta algo para tu meta.",
    "Compara el costo total antes de aceptar cuotas.",
    "Prepara un fondo y protege tu meta."
};

// Elige un reto al azar de cada nivel y separa las preguntas unos 10 segundos de carrera.
std::vector<Decision> MakeDecisions()
{
    std::vector<Decision> decisions = {
        {0, "BARRIO / OFERTA", "Un celular cuesta $4.000. La oferta vence hoy. Que haces?",
         {"Esperas y comparas antes de pagar", "Aceptas 6 cuotas de $750", "Depositas $1.000 por un enlace de reserva"},
         {"Comparar precios evita comprar bajo presion.", "Las cuotas suman $4.500: pagas $500 extra.", "El enlace falso se llevo el adelanto; perdiste una vida."},
         {0, -4500, -1000}, {0, 0, 0}, {false, true, true}, {false, false, true}},
        {0, "BARRIO / IMPREVISTO", "La bicicleta necesita una reparacion de $1.500.",
         {"Usas $1.200 del fondo y ajustas $300", "Pides prestado; devuelves $2.200", "Ignoras el freno y sigues manejando"},
         {"El fondo cubre casi todo y limitas la deuda.", "El apuro te costo $700 mas que reparar.", "El freno fallo: pierdes una vida y $2.000."},
         {-300, -2200, 0}, {-1200, 0, 0}, {false, true, true}, {false, false, true}},
        {0, "BARRIO / QUINCENA", "Tienes $6.000 disponibles y tus gastos necesarios suman $3.800.",
         {"Pagas lo necesario y guardas $700", "Gastas primero en antojos y luego pagas", "Pagas $800 para entrar a un grupo que duplica dinero"},
         {"Cubres lo basico y separas dinero para tu meta.", "Los gastos impulsivos dejaron menos para lo necesario.", "El grupo desaparecio con el adelanto; pierdes una vida."},
         {-4500, -5500, -800}, {700, 0, 0}, {false, true, true}, {false, false, true}},
        {0, "BARRIO / SUSCRIPCION", "Una aplicacion cobra $400 al mes y ya no la usas.",
         {"Cancelas hoy y revisas el proximo cobro", "La dejas activa por seis meses", "Pides reembolso desde un enlace que te enviaron"},
         {"Cancelar a tiempo evita pagos que no necesitas.", "Pagaste $2.400 por un servicio que no usaste.", "El enlace robo tus datos y te costo una vida."},
         {0, -2400, -500}, {0, 0, 0}, {false, true, true}, {false, false, true}},
        {0, "BARRIO / VENTA", "Vendes una consola que ya no usas.",
         {"Entregas en persona despues de confirmar el pago", "Aceptas un cheque y das el equipo antes de cobrar", "Vendes rapido por $900 en efectivo"},
         {"Confirmar el deposito protege tu dinero y el equipo.", "El cheque reboto y perdiste el equipo; una vida menos.", "Ganaste menos, pero recibiste un pago comprobable."},
         {1500, -1200, 900}, {0, 0, 0}, {false, true, false}, {false, true, false}},
        {0, "CUOTAS / COSTO REAL", "Contado $4.500; cuotas 4 x $1.100 mas $600 de apertura.",
         {"Pagas $4.500 al contado", "Aceptas las cuotas: total $5.000", "Pagas $500 a un gestor para quitar la apertura"},
         {"Comparaste el total y evitas cargos extra.", "La cuota parecia baja, pero sumaba $500 mas.", "El gestor no podia borrar cargos; pierdes el adelanto."},
         {-4500, -5000, -500}, {0, 0, 0}, {false, true, true}, {false, false, false}},
        {0, "CUOTAS / PRESTAMO", "Te ofrecen $3.000 y debes devolver hasta $3.900.",
         {"Pides solo lo necesario y negocias el plazo", "Aceptas $3.000 sin revisar el total", "Pagas $300 por adelantado para que aprueben el prestamo"},
         {"Pedir menos y negociar limita el costo futuro.", "El apuro agrego $900 de deuda.", "La aprobacion era falsa: pierdes una vida y $2.000."},
         {-500, -3900, -300}, {0, 0, 0}, {false, true, true}, {false, false, true}},
        {0, "CUOTAS / TARJETA", "Debes $2.000 en la tarjeta. El minimo es $200.",
         {"Pagas el total y evitas nuevos intereses", "Pagas el minimo; se agregan $400", "Pides un avance para cubrir otra deuda"},
         {"Liquidar la deuda evita intereses futuros.", "El pago minimo dejo $400 adicionales por pagar.", "El avance cobro cargos y empeoro la deuda."},
         {-2000, -600, -900}, {0, 0, 0}, {false, true, true}, {false, false, false}},
        {0, "CUOTAS / MATERIALES", "Materiales cuestan $2.000; al contado hay 10% de descuento.",
         {"Compras solo lo necesario por $1.800", "Aceptas cuotas y extras por $2.700", "Buscas otra tienda y consigues $1.600"},
         {"El descuento sirve porque mantuviste la compra prevista.", "Los cargos y extras borraron el descuento.", "Comparar tiendas te ahorro $400."},
         {-1800, -2700, -1600}, {0, 0, 0}, {false, true, false}, {false, false, false}},
        {0, "CUOTAS / PAGO DIFERIDO", "Un electrodomestico cuesta $2.000 o $2.400 a plazo.",
         {"Esperas y ahorras para pagarlo sin recargo", "Lo financias y aceptas el total de $2.400", "Pagas $500 en una pagina de preventa"},
         {"Esperar evita convertir una compra en deuda.", "El pago diferido agrego $400 al total.", "La preventa era falsa: pierdes el anticipo."},
         {0, -2400, -500}, {0, 0, 0}, {false, true, true}, {false, false, false}},
        {0, "MERCADO / PROMESA", "Un anuncio promete ganar 30% cada semana.",
         {"Verificas la entidad antes de invertir", "Pruebas con $1.000 en una opcion regulada", "Transfieres $2.500 hoy para asegurar la ganancia"},
         {"Una promesa no demuestra que la inversion sea segura.", "Invertir tiene riesgos; diversificar limita la perdida.", "La oferta era un fraude: pierdes una vida y $2.000."},
         {0, -1000, -2500}, {0, 1100, 0}, {false, false, true}, {false, false, true}},
        {0, "MERCADO / CODIGO", "Alguien dice ser del banco y te pide el codigo que llego al celular.",
         {"Cuelgas y llamas al numero oficial del banco", "Compartes el codigo para bloquear un cargo", "Instalas una app para que te ayuden a proteger la cuenta"},
         {"El banco no necesita que compartas codigos de acceso.", "El codigo permitio un cargo no autorizado.", "La app dio acceso a tu cuenta: pierdes una vida y dinero."},
         {0, -1200, -500}, {0, 0, 0}, {false, true, true}, {false, false, true}},
        {0, "MERCADO / META", "Tienes $4.000 ahorrados y surge un gasto necesario de $1.200.",
         {"Usas el fondo y reajustas la fecha de tu meta", "Pides credito por $2.500 para no tocar el ahorro", "Retiras todo el ahorro para una oferta que vence hoy"},
         {"El fondo cumple su funcion; luego retomas la meta.", "La deuda cuesta mas y no elimina el gasto.", "La oferta no existia: pierdes una vida y parte del fondo."},
         {0, -2500, 0}, {-1200, 0, -2000}, {false, true, true}, {false, false, true}},
        {0, "MERCADO / VENTA SEGURA", "Vendes una laptop por $2.500. El comprador envia un comprobante.",
         {"Verificas el deposito antes de entregar", "Entregas al ver una captura del pago", "Pagas $400 al supuesto mensajero por adelantado"},
         {"El dinero confirmado evita entregar sin cobrar.", "La captura era falsa; perdiste la laptop y una vida.", "El mensajero no existia y perdiste el adelanto."},
         {2500, -1500, -400}, {0, 0, 0}, {false, true, true}, {false, true, false}},
        {0, "MERCADO / BONO", "Recibes un bono de $2.000. Como lo distribuyes?",
         {"Guardas $1.500 y usas $500 para una necesidad", "Lo gastas todo en compras no planeadas", "Pagas $600 para desbloquear una inversion garantizada"},
         {"Separar ahorro y cubrir una necesidad mantiene el plan.", "El bono ayudo hoy, pero no quedo reserva.", "La inversion era una trampa: pierdes una vida y dinero."},
         {500, 0, 1400}, {1500, 0, 0}, {false, true, true}, {false, false, true}}
    };
    const float positions[] = {450, 3150, 5700};
    std::mt19937 generator(static_cast<unsigned int>(GetRandomValue(1, 2147483647)));
    std::vector<Decision> selectedDecisions;
    selectedDecisions.reserve(LevelCount);
    for (std::size_t stage = 0; stage < LevelCount; ++stage)
    {
        const auto first = decisions.begin() +
            static_cast<std::ptrdiff_t>(stage * DecisionPoolPerStage);
        std::shuffle(first, first + DecisionPoolPerStage, generator);

        Decision decision = *first;
        decision.x = positions[stage];
        for (int option = 2; option > 0; --option)
        {
            const int other = std::uniform_int_distribution<int>(0, option)(generator);
            std::swap(decision.options[option], decision.options[other]);
            std::swap(decision.feedback[option], decision.feedback[other]);
            std::swap(decision.balanceChange[option], decision.balanceChange[other]);
            std::swap(decision.savingsChange[option], decision.savingsChange[other]);
            std::swap(decision.risky[option], decision.risky[other]);
            std::swap(decision.losesLife[option], decision.losesLife[other]);
        }
        selectedDecisions.push_back(decision);
    }
    return selectedDecisions;
}

std::vector<Decision> Decisions;

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
    {{4720, 425, 65, 16}}, {{5050, 405, 110, 16}}, {{5090, 355, 140, 16}},
    {{5650, 415, 125, 16}}
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
        {{575, 400}, false}, {{1115, 360}, false},
        {{2480, 350}, false}, {{2565, 325}, false}, {{3735, 370}, false},
        {{4500, 335}, false}, {{4590, 330}, false}, {{5010, 365}, false},
        {{5200, 315}, false}, {{5795, 375}, false}
    };
}

std::vector<PowerUp> MakePowerUps()
{
    return {
        {{500, 472}, false, false},
        {{2100, 472}, false, true},
        {{4400, 390}, false, false}
    };
}

std::vector<QuestionBlock> MakeQuestionBlocks()
{
    return {
        {{650, 350, 42, 38}, false}, {{1240, 350, 42, 38}, false},
        {{1840, 350, 42, 38}, false}, {{3420, 330, 42, 38}, false},
        {{3980, 350, 42, 38}, false}, {{5438, 390, 42, 38}, false},
        {{5830, 350, 42, 38}, false}
    };
}

std::vector<Cannon> MakeCannons()
{
    return {
        {{1260, GroundY - 38, 48, 38}, 1.5f},
        {{3220, GroundY - 38, 48, 38}, 1.8f},
        {{5200, GroundY - 38, 48, 38}, 2.1f}
    };
}

std::vector<Enemy> MakeEnemies(float difficultySpeed)
{
    std::vector<Enemy> enemies = {
        {{700, GroundY - 30, 34, 30}, 630, 760, 95 * difficultySpeed, true},
        {{1100, GroundY - 30, 34, 30}, 1050, 1170, 105 * difficultySpeed, true},
        {{1400, GroundY - 30, 34, 30}, 1330, 1480, 110 * difficultySpeed, true},
        {{1740, GroundY - 30, 34, 30}, 1670, 1810, 120 * difficultySpeed, true},
        {{1900, GroundY - 30, 34, 30}, 1850, 1960, 125 * difficultySpeed, true},
        {{2240, GroundY - 30, 34, 30}, 2160, 2320, 100 * difficultySpeed, true},
        {{2420, GroundY - 30, 34, 30}, 2370, 2490, 115 * difficultySpeed, true},
        {{2650, GroundY - 30, 34, 30}, 2600, 2720, 125 * difficultySpeed, true},
        {{3050, GroundY - 30, 34, 30}, 2990, 3110, 110 * difficultySpeed, true},
        {{3650, GroundY - 30, 34, 30}, 3600, 3690, 125 * difficultySpeed, true},
        {{3800, GroundY - 30, 34, 30}, 3760, 3880, 135 * difficultySpeed, true},
        {{4260, GroundY - 30, 34, 30}, 4180, 4340, 105 * difficultySpeed, true},
        {{4440, GroundY - 30, 34, 30}, 4390, 4510, 120 * difficultySpeed, true},
        {{4670, GroundY - 30, 34, 30}, 4590, 4750, 130 * difficultySpeed, true},
        {{5100, GroundY - 30, 34, 30}, 5040, 5140, 120 * difficultySpeed, true},
        {{5350, GroundY - 30, 34, 30}, 5310, 5400, 135 * difficultySpeed, true},
        {{5650, GroundY - 30, 34, 30}, 5580, 5720, 145 * difficultySpeed, true}
    };
    for (std::size_t i = 2; i < enemies.size(); i += 4)
    {
        enemies[i].throwsThings = true;
        enemies[i].shotCooldown = 1.5f + static_cast<float>(i % 3) * 0.25f;
    }
    for (const std::size_t index : {6u, 10u, 14u, 16u})
    {
        enemies[index].jumps = true;
        enemies[index].throwsThings = true;
        enemies[index].jumpCooldown = 1.0f + static_cast<float>(index % 3) * 0.35f;
        enemies[index].shotCooldown = 1.8f;
    }
    return enemies;
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

// Recupera el récord y las valoraciones guardadas localmente entre partidas.
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
               const std::vector<PowerUp> &powerUps, const std::vector<Enemy> &enemies,
               const std::vector<QuestionBlock> &questionBlocks,
               const std::vector<Cannon> &cannons,
               const std::vector<HazardProjectile> &hazardProjectiles)
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
        DrawGameText("$", static_cast<int>(x - 5), static_cast<int>(coin.position.y - 8), 16, DeepBlue);
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
            DrawGameText("+", static_cast<int>(position.x - 5), static_cast<int>(position.y - 6), 15, White);
        }
    }

    for (const QuestionBlock &block : questionBlocks)
    {
        const Rectangle visible = {block.bounds.x - cameraX, block.bounds.y,
                                   block.bounds.width, block.bounds.height};
        DrawRectangleRounded(visible, 0.12f, 6, block.used ? Muted : Gold);
        DrawRectangleLinesEx(visible, 3, block.used ? DeepBlue : Red);
        DrawGameText(block.used ? "$" : "?",
                     static_cast<int>(visible.x + 13),
                     static_cast<int>(visible.y + 7), 23,
                     block.used ? White : DeepBlue);
    }

    for (const Cannon &cannon : cannons)
    {
        const float x = cannon.bounds.x - cameraX;
        DrawRectangleRounded({x, cannon.bounds.y + 12, cannon.bounds.width,
                              cannon.bounds.height - 12}, 0.15f, 6, DeepBlue);
        DrawRectangleRounded({x + 25, cannon.bounds.y + 3, 31, 16},
                             0.2f, 5, Red);
        DrawCircle(static_cast<int>(x + 12), static_cast<int>(cannon.bounds.y + 24),
                   6, Gold);
        DrawGameText("$", static_cast<int>(x + 8),
                     static_cast<int>(cannon.bounds.y + 18), 12, DeepBlue);
    }

    for (const HazardProjectile &projectile : hazardProjectiles)
    {
        if (!projectile.alive) continue;
        const int x = static_cast<int>(projectile.bounds.x - cameraX);
        const int y = static_cast<int>(projectile.bounds.y);
        DrawCircle(x + 8, y + 8, 9, Red);
        DrawCircle(x + 8, y + 8, 5, Gold);
        DrawGameText("$", x + 5, y + 2, 11, DeepBlue);
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
        DrawGameText("$", static_cast<int>(x + 12), static_cast<int>(enemy.bounds.y + 21), 12, Paper);
    }

    if (nextDecision < static_cast<int>(Decisions.size()))
    {
        const Decision &decision = Decisions[nextDecision];
        const float x = decision.x - cameraX;
        DrawRectangleRounded({x - 34, 378, 68, 87}, 0.15f, 7, Paper);
        DrawRectangle(static_cast<int>(x - 4), 398, 8, 44, Red);
        DrawCircle(static_cast<int>(x), 389, 8, Gold);
        DrawGameText("?", static_cast<int>(x - 4), 382, 14, DeepBlue);
    }

    const float vaultX = VaultX - cameraX;
    DrawRectangleRounded({vaultX, 390, 108, 110}, 0.12f, 8, DeepBlue);
    DrawRectangleRounded({vaultX + 13, 404, 82, 82}, 0.2f, 8, Gold);
    DrawCircleLines(static_cast<int>(vaultX + 54), 445, 27, DeepBlue);
    DrawCircle(static_cast<int>(vaultX + 54), 445, 7, Red);
    DrawGameText("META", static_cast<int>(vaultX + 26), 365, 16, DeepBlue);
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
    DrawGameText("RUTA DEL PRESUPUESTO", 18, 12, 19, DeepBlue);
    DrawGameText(TextFormat("NIVEL %d/3  %s", level + 1, StageNames[level]), 18, 43, 14, Red);
    if (balance < 0)
        DrawGameText(TextFormat("DEUDA $%d", -balance), 290, 12, 16, Red);
    else
        DrawGameText(TextFormat("SALDO $%d", balance), 290, 12, 16, Ink);
    DrawGameText(TextFormat("AHORRO $%d/6000", savings), 290, 42, 14, LeafGreen);
    DrawGameText(TextFormat("PUNTOS %06d", score), 500, 12, 16, DeepBlue);
    DrawGameText(TextFormat("RECORD %06d", bestScore), 500, 42, 14, Muted);
    DrawGameText(TextFormat("TIEMPO %03d", static_cast<int>(timeLeft)), 720, 12, 16, Red);
    DrawGameText(TextFormat("VIDAS %d   SEMILLAS %d", lives, ammo), 720, 42, 14, Ink);
    DrawGameText(TextFormat("%d/%d", progress, DecisionCount), 918, 15, 16, DeepBlue);
    DrawGameText(!soundAvailable ? "SIN AUDIO" : (muted ? "MUDO [M]" : "AUDIO [M]"),
             900, 45, 12, Muted);
}

void DrawDecisionCard(int index, int balance, int savings, int score)
{
    const Decision &decision = Decisions[index];
    DrawRectangle(0, 0, ScreenWidth, ScreenHeight, Color{17, 30, 53, 185});
    DrawRectangleRounded({135, 92, 730, 430}, 0.05f, 12, Paper);
    DrawRectangle(135, 92, 730, 12, Red);
    DrawGameText(decision.title, 175, 122, 18, Red);
    DrawGameText(decision.question, 175, 157, 19, Ink);
    if (balance < 0)
        DrawGameText(TextFormat("DEUDA $%d     AHORRO $%d     PUNTOS %d", -balance, savings, score),
                 175, 195, 15, Muted);
    else
        DrawGameText(TextFormat("SALDO $%d     AHORRO $%d     PUNTOS %d", balance, savings, score),
                 175, 195, 15, Muted);

    for (int i = 0; i < 3; ++i)
    {
        const Rectangle optionBounds = {170, static_cast<float>(225 + i * 68), 660, 54};
        const bool hovered = CheckCollisionPointRec(GetMousePosition(), optionBounds);
        const Color choiceColor = hovered ? Color{255, 237, 191, 255}
                                          : Color{231, 234, 224, 255};
        DrawRectangleRounded(optionBounds, 0.15f, 8, choiceColor);
        if (hovered) DrawRectangleLinesEx(optionBounds, 3, DeepBlue);
        DrawCircle(198, static_cast<int>(optionBounds.y + 27), 14, Blue);
        DrawGameText(TextFormat("%d", i + 1), 194, static_cast<int>(optionBounds.y + 16), 16, White);
        DrawGameText(decision.options[i], 228, static_cast<int>(optionBounds.y + 16), 16, Ink);
    }
    DrawGameText("Elige 1, 2 o 3. Algunas ofertas esconden cargos o riesgos.",
                 175, 445, 15, Muted);
}

void DrawFeedback(int index, int choice, int balanceChange, int savingsChange)
{
    const Decision &decision = Decisions[index];
    const bool risky = decision.risky[choice];
    DrawRectangle(0, 0, ScreenWidth, ScreenHeight, Color{17, 30, 53, 195});
    DrawRectangleRounded({175, 188, 650, 235}, 0.05f, 12, Paper);
    DrawRectangle(175, 188, 650, 12, risky ? Red : LeafGreen);
    DrawGameText(risky ? "CONSECUENCIA / ALERTA" : "CONSECUENCIA / BUENA JUGADA",
             220, 222, 18, risky ? Red : LeafGreen);
    DrawGameText(decision.feedback[choice], 220, 266, 18, Ink);
    DrawGameText(TextFormat("Saldo: %s$%d     Ahorro: %s$%d",
                        balanceChange < 0 ? "-" : "+", std::abs(balanceChange),
                        savingsChange < 0 ? "-" : "+", std::abs(savingsChange)),
             220, 304, 16, DeepBlue);
    if (decision.losesLife[choice])
        DrawGameText("La trampa costo una vida, $2.000 y 18 segundos.",
                     220, 340, 15, Red);
    else if (risky)
        DrawGameText("Aparecio una factura perseguidora. Salta o usa una semilla.",
                 220, 340, 15, Red);
    DrawGameText("Presiona ENTER para seguir la ruta", 220, 385, 16, Muted);
}

void DrawMessagePanel(const char *label, const char *title, const char *detail, int score)
{
    DrawRectangle(0, 0, ScreenWidth, ScreenHeight, Color{17, 30, 53, 190});
    DrawRectangleRounded({175, 148, 650, 300}, 0.06f, 14, Paper);
    DrawRectangle(175, 148, 650, 12, Gold);
    DrawGameText(label, 225, 188, 18, Red);
    DrawGameText(title, 225, 229, 36, DeepBlue);
    DrawGameText(detail, 225, 285, 18, Ink);
    DrawGameText(TextFormat("PUNTUACION: %06d", score), 225, 334, 22, LeafGreen);
    DrawGameText("Presiona ENTER para continuar", 225, 391, 16, Muted);
}

} // namespace

int main()
{
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(ScreenWidth, ScreenHeight, "Ruta del presupuesto");
    SetTargetFPS(60);
    Decisions = MakeDecisions();

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
#else
    std::vector<int> fontCodepoints;
    for (int codepoint = 32; codepoint <= 126; ++codepoint)
        fontCodepoints.push_back(codepoint);
    for (int codepoint = 160; codepoint <= 255; ++codepoint)
        fontCodepoints.push_back(codepoint);
    GameFont = LoadFontEx("assets/fonts/AtkinsonHyperlegible-Regular.ttf", 48,
                          fontCodepoints.data(), static_cast<int>(fontCodepoints.size()));
    if (GameFont.texture.id == 0)
        GameFont = LoadFontEx("../assets/fonts/AtkinsonHyperlegible-Regular.ttf", 48,
                              fontCodepoints.data(), static_cast<int>(fontCodepoints.size()));
    if (GameFont.texture.id == 0)
        TraceLog(LOG_WARNING, "Could not load the desktop font; using raylib's default font.");
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

    enum class Screen { Title, Playing, Paused, Decision, Feedback, LevelComplete, GameOver, Rating };
    Screen screen = Screen::Title;
    std::vector<Coin> coins = MakeCoins();
    std::vector<PowerUp> powerUps = MakePowerUps();
    std::vector<Enemy> enemies = MakeEnemies(AdventureMode.enemySpeed);
    std::vector<Projectile> projectiles;
    std::vector<HazardProjectile> hazardProjectiles;
    std::vector<QuestionBlock> questionBlocks = MakeQuestionBlocks();
    std::vector<Cannon> cannons = MakeCannons();
    Rectangle player = {90, GroundY - PlayerHeight, PlayerWidth, PlayerHeight};
    Vector2 velocity = {0, 0};
    float cameraX = 0;
    float walkTime = 0;
    float remainingTime = AdventureMode.duration;
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
    int lives = AdventureMode.startingLives;
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
    float dropThroughTime = 0.0f;
    float dropThroughPlatformY = -1.0f;
    bool timedOut = false;
    bool bankrupt = false;
    bool completedRun = false;

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

    // Reinicia el estado de juego y genera una nueva combinación de retos.
    auto resetRun = [&]() {
        Decisions = MakeDecisions();
        coins = MakeCoins();
        powerUps = MakePowerUps();
        enemies = MakeEnemies(AdventureMode.enemySpeed);
        projectiles.clear();
        hazardProjectiles.clear();
        questionBlocks = MakeQuestionBlocks();
        cannons = MakeCannons();
        player = {90, GroundY - PlayerHeight, PlayerWidth, PlayerHeight};
        velocity = {0, 0};
        cameraX = 0;
        walkTime = 0;
        remainingTime = AdventureMode.duration;
        shieldTime = 0;
        invulnerabilityTime = 0;
        shotCooldown = 0;
        balance = 12000;
        savings = 0;
        score = 0;
        lives = AdventureMode.startingLives;
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
        dropThroughTime = 0.0f;
        dropThroughPlatformY = -1.0f;
        timedOut = false;
        bankrupt = false;
        completedRun = false;
    };

    // Aplica la penalización común a los golpes y a las respuestas-trampa.
    auto loseLife = [&]() {
        --lives;
        balance -= 2000;
        score = std::max(0, score - 250);
        remainingTime = std::max(0.0f, remainingTime - 18.0f);
        playSound(hitSound);
        bankrupt = balance + savings <= 0;
        if (lives <= 0 || remainingTime <= 0.0f || bankrupt)
        {
            timedOut = lives > 0 && remainingTime <= 0.0f && !bankrupt;
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
            const Vector2 mouse = GetMousePosition();
            bool startGame = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) ||
                (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                 (CheckCollisionPointRec(mouse, {165, 374, 360, 52}) ||
                  CheckCollisionPointRec(mouse, {165, 443, 248, 42})));
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
            dropThroughTime = std::max(0.0f, dropThroughTime - delta);
            if (dropThroughTime <= 0.0f) dropThroughPlatformY = -1.0f;

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
                const float previousTop = player.y;
                if (onLadder && verticalInput) climbing = true;

                if (onGround && !onLadder && IsKeyPressed(KEY_DOWN))
                {
                    for (const Platform &platform : Platforms)
                    {
                        const bool standingOnPlatform =
                            std::abs(player.y + player.height - platform.bounds.y) < 1.0f &&
                            player.x + player.width > platform.bounds.x &&
                            player.x < platform.bounds.x + platform.bounds.width;
                        if (!standingOnPlatform) continue;
                        dropThroughTime = 0.2f;
                        dropThroughPlatformY = platform.bounds.y;
                        velocity.y = 130.0f;
                        onGround = false;
                        break;
                    }
                }

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
                        const bool dropThroughPlatform = dropThroughTime > 0.0f &&
                            std::abs(platform.bounds.y - dropThroughPlatformY) < 1.0f;
                        if (horizontalOverlap && crossedTop && !dropThroughPlatform)
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
                    for (QuestionBlock &block : questionBlocks)
                    {
                        const bool horizontalOverlap = player.x + player.width > block.bounds.x &&
                            player.x < block.bounds.x + block.bounds.width;
                        const bool hitFromBelow = !block.used && velocity.y < 0 &&
                            horizontalOverlap && previousTop >= block.bounds.y + block.bounds.height &&
                            player.y <= block.bounds.y + block.bounds.height;
                        if (!hitFromBelow) continue;

                        block.used = true;
                        player.y = block.bounds.y + block.bounds.height;
                        velocity.y = 0;
                        powerUps.push_back({{block.bounds.x + block.bounds.width / 2,
                                             block.bounds.y - 12},
                                            false, false, -170.0f});
                        score += 100;
                        playSound(pickupSound);
                    }

                    Rectangle playerBounds = {player.x + 2, player.y + 2,
                                               player.width - 4, player.height - 3};

                    for (Coin &coin : coins)
                    {
                        if (!coin.collected && CheckCollisionCircleRec(coin.position, 16, playerBounds))
                        {
                            coin.collected = true;
                            balance += 250;
                            score += 75;
                            playSound(pickupSound);
                        }
                    }

                    for (PowerUp &powerUp : powerUps)
                    {
                        if (powerUp.velocityY != 0.0f)
                        {
                            const float previousBottom = powerUp.position.y + 18.0f;
                            powerUp.position.y += powerUp.velocityY * delta;
                            powerUp.velocityY += 520.0f * delta;
                            if (powerUp.velocityY > 0.0f)
                            {
                                for (const Platform &platform : Platforms)
                                {
                                    const bool horizontalOverlap =
                                        powerUp.position.x + 14.0f > platform.bounds.x &&
                                        powerUp.position.x - 14.0f <
                                            platform.bounds.x + platform.bounds.width;
                                    if (horizontalOverlap && previousBottom <= platform.bounds.y &&
                                        powerUp.position.y + 18.0f >= platform.bounds.y)
                                    {
                                        powerUp.position.y = platform.bounds.y - 18.0f;
                                        powerUp.velocityY = 0.0f;
                                        break;
                                    }
                                }
                            }
                            if (powerUp.velocityY != 0.0f &&
                                powerUp.position.y >= GroundY - 18.0f)
                            {
                                powerUp.position.y = GroundY - 18.0f;
                                powerUp.velocityY = 0.0f;
                            }
                        }
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
                        const int enemyLevel = std::clamp(
                            static_cast<int>(enemy.bounds.x / LevelWidth), 0, LevelCount - 1);
                        const float stageSpeed = 1.0f + enemyLevel * 0.18f;
                        enemy.bounds.x += enemy.speed * stageSpeed * delta;
                        if (enemy.bounds.x >= enemy.patrolEnd || enemy.bounds.x <= enemy.patrolStart)
                        {
                            enemy.speed = -enemy.speed;
                            enemy.bounds.x = std::clamp(enemy.bounds.x, enemy.patrolStart, enemy.patrolEnd);
                        }
                        if (enemy.jumps)
                        {
                            if (enemy.bounds.y + enemy.bounds.height >= GroundY - 0.5f)
                            {
                                enemy.bounds.y = GroundY - enemy.bounds.height;
                                enemy.verticalSpeed = 0.0f;
                                enemy.jumpCooldown -= delta;
                                if (enemy.jumpCooldown <= 0.0f)
                                {
                                    enemy.verticalSpeed = -390.0f - enemyLevel * 30.0f;
                                    enemy.jumpCooldown = 2.8f - enemyLevel * 0.3f;
                                }
                            }
                            else
                            {
                                enemy.bounds.y += enemy.verticalSpeed * delta;
                                enemy.verticalSpeed += 1100.0f * delta;
                                if (enemy.bounds.y + enemy.bounds.height >= GroundY)
                                {
                                    enemy.bounds.y = GroundY - enemy.bounds.height;
                                    enemy.verticalSpeed = 0.0f;
                                }
                            }
                        }
                        if (enemy.throwsThings)
                        {
                            enemy.shotCooldown -= delta;
                            const float enemyCenter = enemy.bounds.x + enemy.bounds.width / 2;
                            const float distance = std::abs(enemyCenter -
                                (player.x + player.width / 2));
                            if (distance < 430.0f && enemy.shotCooldown <= 0.0f)
                            {
                                const float directionToPlayer =
                                    player.x < enemy.bounds.x ? -1.0f : 1.0f;
                                hazardProjectiles.push_back({
                                    {enemyCenter, enemy.bounds.y + 12, 16, 16},
                                    directionToPlayer * (145.0f + enemyLevel * 24.0f),
                                    enemy.jumps ? -350.0f - enemyLevel * 25.0f : 0.0f,
                                    enemy.jumps ? 680.0f : 0.0f, true, enemyLevel});
                                enemy.shotCooldown = std::max(1.0f,
                                    2.3f - enemyLevel * 0.35f);
                            }
                        }
                    }

                    for (Cannon &cannon : cannons)
                    {
                        cannon.shotCooldown -= delta;
                        const float cannonCenter = cannon.bounds.x + cannon.bounds.width / 2;
                        if (std::abs(cannonCenter - (player.x + player.width / 2)) < 560.0f &&
                            cannon.shotCooldown <= 0.0f)
                        {
                            const float directionToPlayer =
                                player.x < cannonCenter ? -1.0f : 1.0f;
                            hazardProjectiles.push_back({
                                {cannonCenter, cannon.bounds.y + 11, 18, 16},
                                directionToPlayer * (245.0f + level * 30.0f),
                                0.0f, 0.0f, true, level});
                            cannon.shotCooldown = std::max(0.9f,
                                2.6f - level * 0.45f);
                        }
                    }

                    for (HazardProjectile &projectile : hazardProjectiles)
                    {
                        if (!projectile.alive) continue;
                        projectile.bounds.x += projectile.speedX * delta;
                        projectile.bounds.y += projectile.speedY * delta;
                        projectile.speedY += projectile.gravity * delta;
                        const float projectileScreenX = projectile.bounds.x - cameraX;
                        const float stageStart = projectile.level * LevelWidth;
                        const float stageEnd = stageStart + LevelWidth;
                        if (projectile.bounds.x + projectile.bounds.width <= stageStart ||
                            projectile.bounds.x >= stageEnd ||
                            projectile.bounds.y >= GroundY ||
                            projectileScreenX < -60.0f ||
                            projectileScreenX > ScreenWidth + 60.0f)
                        {
                            projectile.alive = false;
                            continue;
                        }
                        if (CheckCollisionRecs(playerBounds, projectile.bounds))
                        {
                            projectile.alive = false;
                            if (shieldTime <= 0.0f && invulnerabilityTime <= 0.0f)
                            {
                                loseLife();
                                if (screen != Screen::Playing) break;
                            }
                        }
                    }
                    hazardProjectiles.erase(std::remove_if(hazardProjectiles.begin(),
                        hazardProjectiles.end(),
                        [](const HazardProjectile &projectile) { return !projectile.alive; }),
                        hazardProjectiles.end());

                    if (IsKeyPressed(KEY_X) && ammo > 0 && shotCooldown <= 0.0f)
                    {
                        const float shotX = facingRight ? player.x + player.width : player.x - 18;
                        projectiles.push_back({{shotX, player.y + 20, 18, 10},
                                               facingRight ? 460.0f : -460.0f, true, level});
                        --ammo;
                        shotCooldown = 0.24f;
                        playSound(chooseSound);
                    }

                    for (Projectile &projectile : projectiles)
                    {
                        if (!projectile.alive) continue;
                        projectile.bounds.x += projectile.speed * delta;
                        const float stageStart = projectile.level * LevelWidth;
                        const float stageEnd = stageStart + LevelWidth;
                        if (projectile.bounds.x + projectile.bounds.width <= stageStart ||
                            projectile.bounds.x >= stageEnd)
                        {
                            projectile.alive = false;
                            continue;
                        }
                        for (Enemy &enemy : enemies)
                        {
                            const int enemyLevel = std::clamp(
                                static_cast<int>(enemy.bounds.x / LevelWidth), 0, LevelCount - 1);
                            if (enemy.alive && enemyLevel == projectile.level &&
                                CheckCollisionRecs(projectile.bounds, enemy.bounds))
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

                    if (screen == Screen::Playing)
                    {
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
                    }

                    if (screen == Screen::Playing && nextDecision < static_cast<int>(Decisions.size()) &&
                        player.x + player.width >= Decisions[nextDecision].x)
                    {
                        activeDecision = nextDecision;
                        selectedOption = -1;
                        screen = Screen::Decision;
                    }

                    const bool reachedGoal = level == LevelCount - 1
                        ? player.x + player.width >= VaultX
                        : player.x >= (level + 1) * LevelWidth - 60.0f;
                    if (screen == Screen::Playing && reachedGoal)
                    {
                        if (level == 2)
                        {
                            const int timeBonus = static_cast<int>(remainingTime) * 5;
                            score += timeBonus;
                            completedRun = true;
                            submittedRating = 0;
                            screen = Screen::Rating;
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
            if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) selectedOption = 2;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                const Vector2 mouse = GetMousePosition();
                for (int i = 0; i < 3; ++i)
                {
                    const Rectangle optionBounds = {170, static_cast<float>(225 + i * 68), 660, 54};
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
                // Los gastos salen primero del ahorro reservado; solo el faltante afecta el saldo.
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
                score = std::max(0, score + (decision.risky[selectedOption] ? -250 : 250));
                if (balance + savings <= 0)
                {
                    bankrupt = true;
                    screen = Screen::GameOver;
                    saveRecord();
                }
                else
                {
                    if (decision.losesLife[selectedOption])
                    {
                        loseLife();
                    }
                    else if (decision.risky[selectedOption])
                    {
                        const float enemyX = std::min(player.x + 240.0f, WorldWidth - 100.0f);
                        enemies.push_back({{enemyX, GroundY - 30, 34, 30},
                                           enemyX - 60, enemyX + 100,
                                           -110.0f * AdventureMode.enemySpeed,
                                           true});
                    }
                    if (screen != Screen::GameOver)
                    {
                        lastBalanceChange = balance - oldBalance;
                        lastSavingsChange = savings - oldSavings;
                        screen = Screen::Feedback;
                    }
                }
                if (!decision.losesLife[selectedOption])
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
        else if (screen == Screen::GameOver &&
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
#if defined(PLATFORM_WEB)
                    // El navegador envía solo la calificación al formulario anónimo configurado en web/index.html.
                    EM_ASM({
                        if (typeof window.submitGameRating === "function")
                            window.submitGameRating($0);
                        else
                            console.error("Google Forms rating submission is unavailable.");
                    }, value);
#endif
                    break;
                }
            }
            if (IsKeyPressed(KEY_ENTER))
                screen = Screen::Title;
        }

        BeginDrawing();
        DrawBackdrop(cameraX, level);
        DrawWorld(cameraX, nextDecision, coins, powerUps, enemies,
                  questionBlocks, cannons, hazardProjectiles);
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
            DrawGameText("UNA AVENTURA DE PLATAFORMAS Y FINANZAS", 165, 128, 16, Red);
            DrawGameText("Ruta del", 165, 166, 48, DeepBlue);
            DrawGameText("presupuesto", 165, 215, 48, DeepBlue);
            DrawGameText("3 preguntas: una por nivel.", 165, 283, 19, Ink);
            DrawGameText("Cada nivel elige al azar entre cinco retos.", 165, 317, 16, Muted);
            DrawGameText("MODO AVENTURA", 165, 350, 14, Red);
            DrawCircle(770, 218, 48, Color{255, 197, 66, 100});
            DrawCircle(770, 218, 34, Gold);
            DrawGameText("$", 762, 202, 25, DeepBlue);
            DrawPlayer({728, 258, PlayerWidth, PlayerHeight}, true, 0, false);
            DrawGameText("15 RETOS", 716, 330, 14, DeepBlue);
            const Rectangle adventureCard = {165, 374, 360, 52};
            DrawRectangleRounded(adventureCard, 0.16f, 7, DeepBlue);
            DrawRectangleRoundedLinesEx(adventureCard, 0.16f, 7, 3, Gold);
            DrawGameText("AVENTURA  /  180 S  /  3 VIDAS", 184, 389, 17, White);
            DrawGameText("Enemigos mas veloces; tres niveles.", 165, 426, 14, Muted);
            DrawRectangleRounded({165, 443, 248, 42}, 0.15f, 8, Red);
            DrawGameText("JUGAR  /  ESPACIO", 184, 456, 15, White);
            DrawGameText("A/D mover  ESPACIO saltar y golpear frutas  ABAJO bajar  W/S escalar",
                     165, 491, 13, Ink);
            DrawGameText(TextFormat("Semillas X  Golpe: -$2000/vida  M sonido  Record: %06d", bestScore),
                     165, 508, 12, Blue);
            if (records.storageError)
                DrawGameText("No se pudo guardar datos en este equipo.", 650, 508, 12, Red);
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
            DrawGameText("PAUSA", 305, 255, 36, DeepBlue);
            DrawGameText(TextFormat("%s  /  %s", StageNames[level],
                                AdventureMode.name),
                     305, 304, 17, Ink);
            DrawGameText("Presiona P para continuar", 305, 344, 16, Muted);
        }
        else if (screen == Screen::GameOver)
        {
            const int finalCapital = balance + savings;
            const char *reason = bankrupt ? "BANCARROTA" :
                (timedOut ? "Se acabo el tiempo" : "Te quedaste sin vidas");
            DrawMessagePanel(bankrupt ? "FINANZAS EN ROJO" : "PARTIDA TERMINADA",
                             reason,
                             TextFormat("Capital final: $%d  (saldo $%d + ahorro $%d)",
                                        finalCapital, balance, savings),
                             score);
        }
        else if (screen == Screen::Rating)
        {
            DrawRectangle(0, 0, ScreenWidth, ScreenHeight, Color{17, 30, 53, 190});
            DrawRectangleRounded({190, 126, 620, 348}, 0.05f, 14, Paper);
            DrawRectangle(190, 126, 620, 12, Gold);
            DrawGameText(completedRun ? "META ALCANZADA / CALIFICA" : "CALIFICA LA EXPERIENCIA",
                         240, 165, 20, completedRun ? LeafGreen : Red);
            if (submittedRating == 0)
            {
                if (completedRun)
                {
                    DrawGameText(TextFormat("Capital final: $%d  /  Puntos: %06d",
                                             balance + savings, score),
                                 240, 205, 17, Ink);
                    DrawGameText("Que tan entretenida fue la aventura?", 240, 250, 21, DeepBlue);
                    DrawGameText("Pulsa 1-5 para calificar; ENTER para omitir.", 240, 294, 16, Ink);
                    DrawGameText("1   2   3   4   5", 240, 337, 30, Gold);
                }
                else
                {
                    DrawGameText("Que tan entretenida fue la aventura?", 240, 240, 22, DeepBlue);
                    DrawGameText("1-5 para calificar; ENTER para omitir. Es anonima.", 240, 286, 16, Ink);
                    DrawGameText("1   2   3   4   5", 240, 333, 30, Gold);
                }
            }
            else
            {
                DrawGameText("Gracias por valorar el prototipo.", 240, 214, 22, DeepBlue);
                if (completedRun)
                    DrawGameText(TextFormat("Capital final: $%d  /  Puntos: %06d",
                                             balance + savings, score),
                                 240, 251, 16, Ink);
                DrawGameText(TextFormat("Respuestas en este equipo: %d", ratingCount), 240, 290, 17, Ink);
                DrawGameText(TextFormat("Promedio: %.1f / 5",
                                    ratingCount > 0 ? static_cast<float>(ratingTotal) / ratingCount : 0.0f),
                         240, 324, 19, LeafGreen);
                if (records.storageError)
                    DrawGameText("No se pudo actualizar el archivo de resultados.", 240, 351, 13, Red);
#if defined(PLATFORM_WEB)
                DrawGameText("Enviando valoracion anonima a Google Forms.", 240, 355, 14, Blue);
#else
                DrawGameText("Valoracion guardada en este dispositivo.", 240, 355, 14, Blue);
#endif
                DrawGameText("ENTER para volver al inicio", 240, 397, 16, Muted);
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
